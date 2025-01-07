//--------------------------------------------------------------------------------

#include "PCH.h"

#include "WAGI.h"
#include "../WDataRes_AnimGraph.h"
#include "../../../../../Projects/Main/GameWorld/WServerMod.h" // Forbidden! Move func to WServer_Core?

//--------------------------------------------------------------------------------

void CWAGI::SetResourceIndex_AnimGraph(int32 _iAnimGraphRes)
{
	m_iAnimGraphRes = _iAnimGraphRes;
}

//--------------------------------------------------------------------------------

void CWAGI::SetResourceIndex_AnimList(int32 _iAnimListRes)
{
	m_iAnimListRes = _iAnimListRes;
}

//--------------------------------------------------------------------------------

void CWAGI::SetOverlayAnim(int32 _iAnimContainerResource, int16 _iAnimSeq, CMTime _StartTime)
{
	m_OverlayAnim = CXRAG_Animation(_iAnimContainerResource, _iAnimSeq);
	m_OverlayAnim_StartTime = _StartTime;
	m_bOverlayAnimFirstVelocityRequest = true;
}

void CWAGI::SetOverlayAnimLipSync(int32 _iAnimContainerResource, int16 _iAnimSeq, int8 _BaseJoint)
{
	m_OverlayAnimLipSync = CXRAG_Animation(_iAnimContainerResource, _iAnimSeq);
	m_OverLayAnimLipSyncBaseJoint = _BaseJoint;
}

//--------------------------------------------------------------------------------

void CWAGI::ClearOverlayAnim()
{
	m_OverlayAnim.Clear();
	m_OverlayAnimLipSync.Clear();
	m_OverLayAnimLipSyncBaseJoint = 0;
	m_OverlayAnim_StartTime = AGI_UNDEFINEDTIME;
	m_bOverlayAnimFirstVelocityRequest = false;
}

//--------------------------------------------------------------------------------

bool CWAGI::AcquireAllResources(const CWAGI_Context *_pContext)
{
	MSCOPESHORT(CWAGI::AcquireAllResources);
	if (!AcquireResource_AnimList(_pContext))
		return false;

	if (!AcquireResource_AnimGraph(_pContext))
	{
		m_spAnimList = NULL;
		return false;
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CWAGI::AcquireResource_AnimGraph(const CWAGI_Context* _pContext)
{
	MSCOPESHORT(CWAGI::AcquireResource_AnimGraph);
	if ((m_nAnimGraphRefs > 0) && (m_spAnimGraph != NULL))
	{
		m_nAnimGraphRefs++;
		return true;
	}

	m_nAnimGraphRefs = 0;

	CWRes_AnimGraph* pAnimGraphRes;
	if(_pContext->m_pWPhysState)
		pAnimGraphRes = _pContext->m_pWPhysState->GetMapData()->GetResource_AnimGraph(m_iAnimGraphRes);
	else
	{
		M_ASSERT(_pContext->m_pMapData, "!");
		pAnimGraphRes = _pContext->m_pMapData->GetResource_AnimGraph(m_iAnimGraphRes);
	}

	if (pAnimGraphRes == NULL)
		return false;

	m_spAnimGraph = pAnimGraphRes->GetAnimGraph();
	if (m_spAnimGraph == NULL)
		return false;

	m_nAnimGraphRefs = 1;

	return true;
}

//--------------------------------------------------------------------------------


bool CWAGI::AcquireResource_AnimList(const CWAGI_Context* _pContext)
{
	MSCOPESHORT(CWAGI::AcquireResource_AnimList);
	if ((m_nAnimListRefs > 0) && (m_spAnimList != NULL))
	{
		m_nAnimListRefs++;
		return true;
	}

	m_nAnimListRefs = 0;

	CWRes_AGAnimList* pAnimListRes;
	if(_pContext->m_pWPhysState)
		pAnimListRes = _pContext->m_pWPhysState->GetMapData()->GetResource_AGAnimList(m_iAnimListRes);
	else
	{
		M_ASSERT(_pContext->m_pMapData, "!");
		pAnimListRes = _pContext->m_pMapData->GetResource_AGAnimList(m_iAnimListRes);
	}

	if (pAnimListRes == NULL)
		return false;

	m_spAnimList = pAnimListRes->GetAnimList();
	if (m_spAnimList == NULL)
		return false;

	m_nAnimListRefs = 1;

	return true;
}

void CWAGI::ClearAnimListCache()
{
	if (m_spAnimList)
		m_spAnimList->ClearCache();
}

//--------------------------------------------------------------------------------

void CWAGI::UnacquireAllResources()
{
	MSCOPESHORT(CWAGI::UnacquireAllResources);
	if ((m_nAnimGraphRefs--) <= 0)
	{
		m_nAnimGraphRefs = 0;
		m_spAnimGraph = NULL;
	}

	if ((m_nAnimListRefs--) <= 0)
	{
		m_nAnimListRefs = 0;
		m_spAnimList = NULL;
	}
}
#ifdef WAGI_USEAGRESOURCEMANAGEMENT

//--------------------------------------------------------------------------------

void CWAGI::TagAnimSetFromName(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const CStr& _SetName)
{
	if (!AcquireAllResources(_pContext) || _pContext->m_pWPhysState->IsClient())
		return;

	CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(_pContext->m_pWPhysState);
	if (!pServerMod)
		return;

	// Ok, find action, and get effects from that action?
	CStr ActionName = _SetName.UpperCase()+".ACTION_ANIMSET";
	uint32 ActionHashKey = StringToHash(ActionName);
	int16 iAction = GetActionIndexFromHashKey(ActionHashKey);
	const CXRAG_Action* pAction = GetAction(iAction);
	if (!pAction)
		return;

	// Get animlist and hope for the best
	CWRes_AGAnimList* pAnimListRes;
	if(_pContext->m_pWPhysState)
	{
		pAnimListRes = _pContext->m_pWPhysState->GetMapData()->GetResource_AGAnimList(m_iAnimListRes);
	}
	else
	{
		M_ASSERT(_pContext->m_pMapData, "!");
		pAnimListRes = _pContext->m_pMapData->GetResource_AGAnimList(m_iAnimListRes);
	}

	CXRAG_AnimList* pAnimList = (pAnimListRes ? pAnimListRes->GetAnimList() : NULL);
	if (!pAnimList)
		return;
	
	// Tag directly into animalist
	int32 AnimListLen = pAnimList->m_lResources.Len();
	int32 NumEffects = pAction->GetNumEffectInstances();
	int32 iBaseEffect = pAction->GetBaseEffectInstanceIndex();

	//CContainerCollector Collector;

	// Not interested in the effects themselves, but rather the parameters in the effects
	for (int32 i = 0; i < NumEffects; i++)
	{
		uint32 iEffectInstance = iBaseEffect + i;
		const CXRAG_EffectInstance* pEffectInstance = m_spAnimGraph->GetEffectInstance(iEffectInstance);
		uint32 EffectID = pEffectInstance->m_ID;
		const CXRAG_ICallbackParams& IParams = m_spAnimGraph->GetICallbackParams(pEffectInstance->m_iParams, pEffectInstance->m_nParams);
		int32 NumParams = IParams.GetNumParams();
		for (int32 j = 0; j < NumParams; j++)
		{
			int32 iAnim = (int32)IParams.GetParam(j);
			if (iAnim < AnimListLen)
			{
				//pAnimList->m_lAnims[iAnim].m_Flags |= CXRAG_Animation::XRAG_ANIMATION_TAGLOAD
				{
					CStr ContainerName = m_spAnimList->GetContainerName(iAnim);
					// New test, collect all entries first and then group together containernames
					int32 iAnimSeq = m_spAnimList->GetAnimSeqIndex(iAnim);
					pServerMod->AddAnimContainerEntry(ContainerName, iAnimSeq, iAnim);

					/*int32 iAnimContainerResource = _pWData->GetResourceIndex("ANM:" + ContainerName, WRESOURCE_CLASS_XSA, _pMapData);
					m_spAnimList->SetContainer(iAnim, iAnimContainerResource, _pWData->GetResourceRef(iAnimContainerResource));*/
				}
				
				// Log tagged animations... (debug)
				/*if (pAnimList->m_lResources[iAnim])
					ConOutL(CStrF("Tagging: %s:%d (iAnim: %d)", pAnimList->m_lResources[iAnim]->GetName().GetStr(),pAnimList->m_lAnims[iAnim].m_iAnimSeq));*/
			}
		}
	}

	// Now we must load all collected resources
	//Collector.LoadData(_pMapData, _pWData, m_spAnimList);
	UnacquireAllResources();
}

// This is VERY PB specific, should be moved elsewhere
enum
{
	// Dialog types
	AG_DIALOG_MOOD = 0,
	AG_DIALOG_GESTURE,
	AG_DIALOG_FACIAL_MOOD,
	AG_DIALOG_FACIAL_GESTURE,

	// Used for caching actions when loading
	AG_DIALOG_MAX_ANIMSETS = 10,

	// Dialog moods
	AG_DIALOG_MOOD_STANDING_NORMALIDLE = 1,
	AG_DIALOG_MOOD_STANDING_NORMAL1,
	AG_DIALOG_MOOD_STANDING_NORMAL2,
	AG_DIALOG_MOOD_STANDING_INDIFFERENT1,
	AG_DIALOG_MOOD_STANDING_INDIFFERENT2,
	AG_DIALOG_MOOD_STANDING_ANGRY1,
	AG_DIALOG_MOOD_STANDING_ANGRY2,
	AG_DIALOG_MOOD_STANDING_PRO,
	AG_DIALOG_MOOD_STANDING_CON,

	AG_DIALOG_MOOD_SITTING_NORMAL1,
	AG_DIALOG_MOOD_SITTING_NORMAL2,
	AG_DIALOG_MOOD_SITTING_INDIFFERENT1,
	AG_DIALOG_MOOD_SITTING_INDIFFERENT2,
	AG_DIALOG_MOOD_SITTING_DICE,

	AG_DIALOG_MOOD_GUARD_NORMAL1,
	AG_DIALOG_MOOD_GUARD_NORMAL2,

	AG_DIALOG_MOOD_POPEJOE_NORMAL,
	AG_DIALOG_MOOD_POPEJOE_DIALOGWITHGOD,
	AG_DIALOG_MOOD_POPEJOE_BREAKDIALOG,

	AG_DIALOG_MOOD_LYING_LYING,
	AG_DIALOG_MOOD_LYING_SLEEPING,

	AG_DIALOG_MOOD_POPEJOE_IDLE,

	AG_DIALOG_MOOD_GUARD_PISTOLDRAWN_NORMAL1,
	AG_DIALOG_MOOD_GUARD_PISTOLDRAWN_NORMAL2,

	AG_DIALOGFACIAL_MOOD_NEUTRAL	= 1,
	AG_DIALOGFACIAL_MOOD_SCARED,
	AG_DIALOGFACIAL_MOOD_ANGRY,
	AG_DIALOGFACIAL_MOOD_SCARED_BIG,
	AG_DIALOGFACIAL_MOOD_ANGRY_BIG,
	AG_DIALOGFACIAL_MOOD_SAD_BIG,
	AG_DIALOGFACIAL_MOOD_SAD_SMALL,
	AG_DIALOGFACIAL_MOOD_HAPPY_BIG,
	AG_DIALOGFACIAL_MOOD_HAPPY_SMALL,
	AG_DIALOGFACIAL_MOOD_DRUNK_BIG,
	AG_DIALOGFACIAL_MOOD_MADDOG,
	AG_DIALOGFACIAL_MOOD_SARCASM_SMALL,
	AG_DIALOGFACIAL_MOOD_NEUTRAL2,


	AG_DIALOGFACIAL_GESTURE_NODINSECURE = 1,
	AG_DIALOGFACIAL_GESTURE_THINKINSECURE,
	AG_DIALOGFACIAL_GESTURE_SHAKEHEADCOOL,
	AG_DIALOGFACIAL_GESTURE_BOTHBROWSDOWN,
	AG_DIALOGFACIAL_GESTURE_BOTHBROWSUP,
	AG_DIALOGFACIAL_GESTURE_BOTHEYESSHUT,
	AG_DIALOGFACIAL_GESTURE_BOTHEYESSQUINT,
	AG_DIALOGFACIAL_GESTURE_BOTHEYESWIDEOPEN,
	AG_DIALOGFACIAL_GESTURE_LEFTBROWDOWN,
	AG_DIALOGFACIAL_GESTURE_LEFTBROWUP,
	AG_DIALOGFACIAL_GESTURE_LEFTEYESHUT,
	AG_DIALOGFACIAL_GESTURE_LEFTEYESQUINT,
	AG_DIALOGFACIAL_GESTURE_LEFTEYEWIDEOPEN,
	AG_DIALOGFACIAL_GESTURE_RIGHTBROWDOWN,
	AG_DIALOGFACIAL_GESTURE_RIGHTBROWUP,
	AG_DIALOGFACIAL_GESTURE_RIGHTEYESHUT,
	AG_DIALOGFACIAL_GESTURE_RIGHTEYESQUINT,
	AG_DIALOGFACIAL_GESTURE_RIGHTEYEWIDEOPEN,
	AG_DIALOGFACIAL_GESTURE_COCKLEFT,
	AG_DIALOGFACIAL_GESTURE_COCKRIGHT,
	AG_DIALOGFACIAL_GESTURE_LOOKLEFT,
	AG_DIALOGFACIAL_GESTURE_LOOKRIGHT,
	AG_DIALOGFACIAL_GESTURE_NO,
	AG_DIALOGFACIAL_GESTURE_YES,
	AG_DIALOGFACIAL_GESTURE_NODDOWN,
	AG_DIALOGFACIAL_GESTURE_NODLEFT,
	AG_DIALOGFACIAL_GESTURE_NODRIGHT,
	AG_DIALOGFACIAL_GESTURE_NODUP,
	AG_DIALOGFACIAL_GESTURE_COUGHBIG,



	// Standing gestures
	AG_DIALOG_GESTURE_STANDING_NOD = 1,
	AG_DIALOG_GESTURE_STANDING_WAVE,
	AG_DIALOG_GESTURE_STANDING_SALUTE,
	AG_DIALOG_GESTURE_STANDING_GIVESHAKE,
	AG_DIALOG_GESTURE_STANDING_RECEIVESHAKE,
	AG_DIALOG_GESTURE_STANDING_DISMISSIVEWAVE,
	AG_DIALOG_GESTURE_STANDING_FINGER,
	AG_DIALOG_GESTURE_STANDING_FIST,
	AG_DIALOG_GESTURE_STANDING_FUCK,
	AG_DIALOG_GESTURE_STANDING_STEPUP1,
	AG_DIALOG_GESTURE_STANDING_STEPUP2,
	AG_DIALOG_GESTURE_STANDING_BECKON,
	AG_DIALOG_GESTURE_STANDING_POINTLEFT,
	AG_DIALOG_GESTURE_STANDING_POINTRIGHT,
	AG_DIALOG_GESTURE_STANDING_POINTFORWARD,
	AG_DIALOG_GESTURE_STANDING_POINTBACKWARD,
	AG_DIALOG_GESTURE_STANDING_STOP,
	AG_DIALOG_GESTURE_STANDING_COME,
	AG_DIALOG_GESTURE_STANDING_NODRIGHT,
	AG_DIALOG_GESTURE_STANDING_NODLEFT,
	AG_DIALOG_GESTURE_STANDING_GOSEARCH,
	AG_DIALOG_GESTURE_STANDING_SICK,
	AG_DIALOG_GESTURE_STANDING_SEIZURE,
	AG_DIALOG_GESTURE_STANDING_GIVEOPEN,
	AG_DIALOG_GESTURE_STANDING_RECEIVEOPEN,
	AG_DIALOG_GESTURE_STANDING_GIVECOVERT,
	AG_DIALOG_GESTURE_STANDING_RECEIVECOVERT,
	AG_DIALOG_GESTURE_STANDING_SHAKEHEADSIDE,
	AG_DIALOG_GESTURE_STANDING_SHAKEHEADUP,
	AG_DIALOG_GESTURE_STANDING_CROSSARMS,
	AG_DIALOG_GESTURE_STANDING_LOOKOUT,
	AG_DIALOG_GESTURE_STANDING_LOOKOUTALARM,
	AG_DIALOG_GESTURE_STANDING_USETERMINAL,
	AG_DIALOG_GESTURE_STANDING_USETERMINALB,
	AG_DIALOG_GESTURE_STANDING_BUZZED,
	AG_DIALOG_GESTURE_STANDING_SMOKE,
	AG_DIALOG_GESTURE_STANDING_OPENLOCKER,
	AG_DIALOG_GESTURE_STANDING_CLOSELOCKER,
	AG_DIALOG_GESTURE_STANDING_USELOCKER,

	// Sitting gestures
	AG_DIALOG_GESTURE_SITTING_BECKON = 50,
	AG_DIALOG_GESTURE_SITTING_WAVE,
	AG_DIALOG_GESTURE_SITTING_WAVEOFF,
	AG_DIALOG_GESTURE_SITTING_POINTFORWARD,
	AG_DIALOG_GESTURE_SITTING_POINTLEFT,
	AG_DIALOG_GESTURE_SITTING_POINTRIGHT,
	AG_DIALOG_GESTURE_SITTING_POINTBACKWARD,
	AG_DIALOG_GESTURE_SITTING_STOP,
	AG_DIALOG_GESTURE_SITTING_SHRUG,
	AG_DIALOG_GESTURE_SITTING_NODOFF,
	AG_DIALOG_GESTURE_SITTING_SLEEP,
	AG_DIALOG_GESTURE_SITTING_WAKE,
	AG_DIALOG_GESTURE_SITTING_DICETHROWA,
	AG_DIALOG_GESTURE_SITTING_DICETHROWB,
	AG_DIALOG_GESTURE_SITTING_DICELOOKA,
	AG_DIALOG_GESTURE_SITTING_DICELOOKB,

	// Guard gestures
	AG_DIALOG_GESTURE_GUARD_POINTRIFLERIGHT = 75,
	AG_DIALOG_GESTURE_GUARD_POINTRIFLELEFT,
	AG_DIALOG_GESTURE_GUARD_USEHELMCOM1,
	AG_DIALOG_GESTURE_GUARD_USEHELMCOM2,
	AG_DIALOG_GESTURE_GUARD_USEHELMCOMBROKEN,
	AG_DIALOG_GESTURE_GUARD_WEAPONTWIRL,
	AG_DIALOG_GESTURE_GUARD_WEAPONTOSS,
	AG_DIALOG_GESTURE_GUARD_READYWEAPON,
	AG_DIALOG_GESTURE_GUARD_USETERMINAL,

	// Lying gestures
	AG_DIALOG_GESTURE_LYING_FALLASLEEP = 90,
	AG_DIALOG_GESTURE_LYING_WAKEUP,
	AG_DIALOG_GESTURE_LYING_STANDUPFAST1,
	AG_DIALOG_GESTURE_LYING_STANDUPFAST2,

	// Pope joe gestures
	AG_DIALOG_GESTURE_POPEJOE_BENDNECK = 100,
	AG_DIALOG_GESTURE_POPEJOE_SCRATCHNECK,


	// Extra getures
	AG_DIALOG_GESTURE_STANDING_N1GES1 = 110,
	AG_DIALOG_GESTURE_STANDING_N1GES2,
	AG_DIALOG_GESTURE_STANDING_N1GES3,
	AG_DIALOG_GESTURE_STANDING_N1GES4,
	AG_DIALOG_GESTURE_STANDING_N1GES5,
	AG_DIALOG_GESTURE_STANDING_N1GES6,
	AG_DIALOG_GESTURE_STANDING_N2GES1,
	AG_DIALOG_GESTURE_STANDING_N2GES2,
	AG_DIALOG_GESTURE_STANDING_N2GES3,
	AG_DIALOG_GESTURE_STANDING_N2GES4,
	AG_DIALOG_GESTURE_STANDING_N2GES5,
	AG_DIALOG_GESTURE_STANDING_I1GES1,
	AG_DIALOG_GESTURE_STANDING_I1GES2,
	AG_DIALOG_GESTURE_STANDING_I1GES3,
	AG_DIALOG_GESTURE_STANDING_I2GES1,
	AG_DIALOG_GESTURE_STANDING_I2GES2,
	AG_DIALOG_GESTURE_STANDING_I2GES3,
	AG_DIALOG_GESTURE_STANDING_I2GES4,
	AG_DIALOG_GESTURE_STANDING_I2GES5,
	AG_DIALOG_GESTURE_STANDING_EXTRADIFF = (AG_DIALOG_GESTURE_STANDING_N1GES1 - AG_DIALOG_GESTURE_STANDING_USELOCKER),

	AG_DIALOG_GESTURE_SITTING_NA1 = 140,
	AG_DIALOG_GESTURE_SITTING_NA2,
	AG_DIALOG_GESTURE_SITTING_NA3,
	AG_DIALOG_GESTURE_SITTING_NA4,
	AG_DIALOG_GESTURE_SITTING_NB1,
	AG_DIALOG_GESTURE_SITTING_NB2,
	AG_DIALOG_GESTURE_SITTING_NB3,
	AG_DIALOG_GESTURE_SITTING_IA1,
	AG_DIALOG_GESTURE_SITTING_IA2,
	AG_DIALOG_GESTURE_SITTING_IA3,
	AG_DIALOG_GESTURE_SITTING_IB1,

	AG_DIALOG_GESTURE_SITTING_EXTRADIFF = (AG_DIALOG_GESTURE_SITTING_NA1 - AG_DIALOG_GESTURE_SITTING_DICELOOKB),

	AG_DIALOG_GESTURE_NUM_ANIMSETS = 5,


	// Behaviors
	AG_BEHAVIOR_PRAYINGMUSLIM = 1,
	AG_BEHAVIOR_COWARD,
	AG_BEHAVIOR_BANGWALL,
	AG_BEHAVIOR_TABLESITTER,
	AG_BEHAVIOR_REPAIRGUY,
	AG_BEHAVIOR_LEDGEFALLER,
	AG_BEHAVIOR_NAZIINMATE,
	AG_BEHAVIOR_KICKGUARD,
	AG_BEHAVIOR_SITBENCH,
	AG_BEHAVIOR_ARMCROSSER,
	AG_BEHAVIOR_SITANDSLEEP,
	AG_BEHAVIOR_SMOKER,
	AG_BEHAVIOR_PUSHBUTTON,
	AG_BEHAVIOR_USEKEYPAD,
	AG_BEHAVIOR_DRILLER,
	AG_BEHAVIOR_SICKBOY,
	AG_BEHAVIOR_SEIZURE,
	AG_BEHAVIOR_PICKUPGROUND,
	AG_BEHAVIOR_ELECTROSHOCK,
	AG_BEHAVIOR_PRODINMATE,
	AG_BEHAVIOR_BACKPRODDED,
	AG_BEHAVIOR_DESKWORKER,
			
	AG_BEHAVIOR_ABUSEDFETAL,
	AG_BEHAVIOR_ABUSEFETAL_GUARDKICKBODY,
	AG_BEHAVIOR_ABUSEFETAL_GUARDSTOMPBODY,
	AG_BEHAVIOR_ABUSEFETAL_INMATEKICKBODY,
	AG_BEHAVIOR_ABUSEFETAL_INMATESTOMPBODY,
	AG_BEHAVIOR_ABUSEFETAL_GUARDCLUBBODY,
	AG_BEHAVIOR_ABUSEFETAL_INMATECLUBBODY,
	AG_BEHAVIOR_ABUSEFETAL_INMATEPOKEBODY,

	AG_BEHAVIOR_1ARMPUSHUP,
	AG_BEHAVIOR_PUSHUP,
	AG_BEHAVIOR_PULLUP,
	AG_BEHAVIOR_SITUP,
	AG_BEHAVIOR_SITUPASSIST,
			
	// POPE JOE BEHAVIORS
	AG_BEHAVIOR_PREACH = 37,
	AG_BEHAVIOR_DIALOGWITHGOD,
	AG_BEHAVIOR_PRAY,
	AG_BEHAVIOR_WALKDIALOGLEFT,
	AG_BEHAVIOR_WALKDIALOGRIGHT,
			
	// ZOMBIE BEHAVIORS
	AG_BEHAVIOR_PA3WAKEUP,
	AG_BEHAVIOR_PA3ONSTRETCHER,
	AG_BEHAVIOR_PA3ONSTRETCHERLYING,
			
	// FIGHTING BEHAVIORS
	AG_BEHAVIOR_FIGHT_PUSH,
	AG_BEHAVIOR_PUSHEDTOFLOOR,
	AG_BEHAVIOR_PUSHEDTOWALL,
	AG_BEHAVIOR_GRABTHROW,
	AG_BEHAVIOR_GRABBEDTHROW,
			
	// GOT IN/MAIN/OUT
	AG_BEHAVIOR_FIGHT_ARGUMENTA,
	// GOT IN/MAIN/OUT
	AG_BEHAVIOR_FIGHT_ARGUMENTB,
			
	// ONE BIG ARGUMENT
	AG_BEHAVIOR_FIGHT_BEATHELD,
	AG_BEHAVIOR_FIGHT_BEATENHELD,
	AG_BEHAVIOR_FIGHT_BEATHOLDRIGHT,
	AG_BEHAVIOR_FIGHT_BEATHOLDLEFT,
			
	AG_BEHAVIOR_FIGHT_KNEETOGROIN,
	AG_BEHAVIOR_FIGHT_KNEEDTOGROIN,
			
	// BEAT GUY ON GROUND (ONE BIG SCENE)
	AG_BEHAVIOR_FIGHT_SMACKHEADTOGROUND,
	AG_BEHAVIOR_FIGHT_SMACKEDHEADTOGROUND,
	AG_BEHAVIOR_FIGHT_SMACKHEADTOGROUNDCHEER,
			
	// GUARD GESTURES
	AG_BEHAVIOR_GUARD_POINTASSRIGHT,
	AG_BEHAVIOR_GUARD_POINTASSLEFT,
	AG_BEHAVIOR_GUARD_USEHELMCOM,
	AG_BEHAVIOR_GUARD_USEHELMCOMB,
	AG_BEHAVIOR_GUARD_BADHELMCOM,
	AG_BEHAVIOR_GUARD_WEAPONTWIRL,
	AG_BEHAVIOR_GUARD_WEAPONTOSS,
	AG_BEHAVIOR_GUARD_USETERM,

	AG_BEHAVIOR_LYINGDOWN,
	AG_BEHAVIOR_LYINGDOWN_SLEEP,

	AG_BEHAVIOR_GUARD_WEAPONHIT,

	// ANOTHER ZOMBIE BEHAVIOR
	AG_BEHAVIOR_PICKEDUPCRANE,
			
	AG_BEHAVIOR_COMBAT_STANDINGDODGELEFT,
	AG_BEHAVIOR_COMBAT_STANDINGDODGERIGHT,
			
	AG_BEHAVIOR_COMBAT_STANDINGROLLLEFT,
	AG_BEHAVIOR_COMBAT_STANDINGROLLRIGHT,
			
	AG_BEHAVIOR_COMBAT_CROUCHROLLLEFT,
	AG_BEHAVIOR_COMBAT_CROUCHROLLRIGHT,
	
	AG_BEHAVIOR_GIVEOPEN,
	AG_BEHAVIOR_RECEIVEOPEN,
	AG_BEHAVIOR_GIVECOVERT,
	AG_BEHAVIOR_RECEIVECOVERT,

	AG_BEHAVIOR_GUARD_GRENADE_THROW,
	AG_BEHAVIOR_GUARD_GRENADE_ROLL,

	AG_BEHAVIOR_DICE_SIT,
	AG_BEHAVIOR_DICE_THROW_WIN,
	AG_BEHAVIOR_DICE_THROW_LOSE,
	AG_BEHAVIOR_DICE_LOOK_WIN,
	AG_BEHAVIOR_DICE_LOOK_LOSE,

	WANTEDBEHAVIORFLAG_MASKTYPE	= 0x8000,
			

	AG_BEHAVIOR_LIGHTGUARDTALK = 100,

	AI_IDLESTANCE_DEFAULT		= 0,
	AI_IDLESTANCE_SEARCHING		= 1,
	AI_IDLESTANCE_WARY			= 2,
	AI_IDLESTANCE_HOSTILE		= 3,
	AI_IDLESTANCE_JUMPY			= 4,

	RPG_EQUIPPEDITEMCLASS_UNDEFINED	= 0,
	RPG_EQUIPPEDITEMCLASS_BAREHANDS	= 1,
	RPG_EQUIPPEDITEMCLASS_GUN		= 2,
	RPG_EQUIPPEDITEMCLASS_SHOTGUN	= 4,
	RPG_EQUIPPEDITEMCLASS_ASSAULT	= 8,
	RPG_EQUIPPEDITEMCLASS_MINGUN		= 16,
	RPG_EQUIPPEDITEMCLASS_KNUCKLEDUSTER	= 32,
	RPG_EQUIPPEDITEMCLASS_SHANK		= 64,
	RPG_EQUIPPEDITEMCLASS_CLUB		= 128,
	RPG_EQUIPPEDITEMCLASS_GRENADE	= 256,
	RPG_EQUIPPEDITEMCLASS_TRANQ		= 512,
};

void DialogMood(int32& _iMood, int32& _iMoodAction, CStr& _MoodAction)
{
	switch (_iMood)
	{
	case AG_DIALOG_MOOD_STANDING_NORMALIDLE:
	case AG_DIALOG_MOOD_STANDING_NORMAL1:
	case AG_DIALOG_MOOD_STANDING_NORMAL2:
	case AG_DIALOG_MOOD_STANDING_INDIFFERENT1:
	case AG_DIALOG_MOOD_STANDING_INDIFFERENT2:
	case AG_DIALOG_MOOD_STANDING_ANGRY1:
	case AG_DIALOG_MOOD_STANDING_ANGRY2:
	case AG_DIALOG_MOOD_STANDING_PRO:
	case AG_DIALOG_MOOD_STANDING_CON:
		{
			// Find effect index
			_iMood -= AG_DIALOG_MOOD_STANDING_NORMALIDLE;
			// Ok, find action, and get effects from that action?
			_MoodAction = ("ANIMSET_DIALOG_MOOD_STANDING");
			_iMoodAction = 0;
			break;
		}
	case AG_DIALOG_MOOD_SITTING_NORMAL1:
	case AG_DIALOG_MOOD_SITTING_NORMAL2:
	case AG_DIALOG_MOOD_SITTING_INDIFFERENT1:
	case AG_DIALOG_MOOD_SITTING_INDIFFERENT2:
	case AG_DIALOG_MOOD_SITTING_DICE:
		{
			// Find effect index
			_iMood -= AG_DIALOG_MOOD_SITTING_NORMAL1;
			// Ok, find action, and get effects from that action?
			_MoodAction = ("ANIMSET_DIALOG_MOOD_SITTING");
			_iMoodAction = 1;
			break;
		}
	case AG_DIALOG_MOOD_GUARD_NORMAL1:
	case AG_DIALOG_MOOD_GUARD_NORMAL2:
		{
			// Find effect index
			_iMood -= AG_DIALOG_MOOD_GUARD_NORMAL1;
			// Ok, find action, and get effects from that action?
			_MoodAction = ("ANIMSET_DIALOG_MOOD_GUARD");	
			_iMoodAction = 2;
			break;
		}
	case AG_DIALOG_MOOD_GUARD_PISTOLDRAWN_NORMAL1:
	case AG_DIALOG_MOOD_GUARD_PISTOLDRAWN_NORMAL2:
		{
			// Find effect index
			_iMood -= AG_DIALOG_MOOD_GUARD_NORMAL1 + 2;
			// Ok, find action, and get effects from that action?
			_MoodAction = ("ANIMSET_DIALOG_MOOD_GUARD");	
			_iMoodAction = 2;
			break;
		}
	case AG_DIALOG_MOOD_POPEJOE_NORMAL:
	case AG_DIALOG_MOOD_POPEJOE_DIALOGWITHGOD:
	case AG_DIALOG_MOOD_POPEJOE_BREAKDIALOG:
		{
			// Find effect index
			_iMood -= AG_DIALOG_MOOD_POPEJOE_NORMAL;
			// Ok, find action, and get effects from that action?
			_MoodAction = ("ANIMSET_DIALOG_MOOD_POPEJOE");
			_iMoodAction = 3;
			break;
		}
	case AG_DIALOG_MOOD_POPEJOE_IDLE:
		{
			// Find effect index
			_iMood -= AG_DIALOG_MOOD_POPEJOE_NORMAL + 3;
			// Ok, find action, and get effects from that action?
			_MoodAction = ("ANIMSET_DIALOG_MOOD_POPEJOE");
			_iMoodAction = 3;
			break;
		}
	case AG_DIALOG_MOOD_LYING_LYING:
	case AG_DIALOG_MOOD_LYING_SLEEPING:
		{
			// Find effect index
			_iMood -= AG_DIALOG_MOOD_LYING_LYING;
			// Ok, find action, and get effects from that action?
			_MoodAction = ("ANIMSET_DIALOG_MOOD_LYING");
			_iMoodAction = 4;

			break;
		}
	default:
		break;
	};
}

void DialogGesture(int32& _iGesture, int32& _iGestureAction, CStr& _GestureAction)
{
	switch (_iGesture)
	{
	case AG_DIALOG_GESTURE_STANDING_NOD:
	case AG_DIALOG_GESTURE_STANDING_WAVE:
	case AG_DIALOG_GESTURE_STANDING_SALUTE:
	case AG_DIALOG_GESTURE_STANDING_GIVESHAKE:
	case AG_DIALOG_GESTURE_STANDING_RECEIVESHAKE:
	case AG_DIALOG_GESTURE_STANDING_DISMISSIVEWAVE:
	case AG_DIALOG_GESTURE_STANDING_FINGER:
	case AG_DIALOG_GESTURE_STANDING_FIST:
	case AG_DIALOG_GESTURE_STANDING_FUCK:
	case AG_DIALOG_GESTURE_STANDING_STEPUP1:
	case AG_DIALOG_GESTURE_STANDING_STEPUP2:
	case AG_DIALOG_GESTURE_STANDING_BECKON:
	case AG_DIALOG_GESTURE_STANDING_POINTLEFT:
	case AG_DIALOG_GESTURE_STANDING_POINTRIGHT:
	case AG_DIALOG_GESTURE_STANDING_POINTFORWARD:
	case AG_DIALOG_GESTURE_STANDING_POINTBACKWARD:
	case AG_DIALOG_GESTURE_STANDING_STOP:
	case AG_DIALOG_GESTURE_STANDING_COME:
	case AG_DIALOG_GESTURE_STANDING_NODRIGHT:
	case AG_DIALOG_GESTURE_STANDING_NODLEFT:
	case AG_DIALOG_GESTURE_STANDING_GOSEARCH:
	case AG_DIALOG_GESTURE_STANDING_SICK:
	case AG_DIALOG_GESTURE_STANDING_SEIZURE:
	case AG_DIALOG_GESTURE_STANDING_GIVEOPEN:
	case AG_DIALOG_GESTURE_STANDING_RECEIVEOPEN:
	case AG_DIALOG_GESTURE_STANDING_GIVECOVERT:
	case AG_DIALOG_GESTURE_STANDING_RECEIVECOVERT:
	case AG_DIALOG_GESTURE_STANDING_SHAKEHEADSIDE:
	case AG_DIALOG_GESTURE_STANDING_SHAKEHEADUP:
	case AG_DIALOG_GESTURE_STANDING_CROSSARMS:
	case AG_DIALOG_GESTURE_STANDING_LOOKOUT:
	case AG_DIALOG_GESTURE_STANDING_LOOKOUTALARM:
	case AG_DIALOG_GESTURE_STANDING_USETERMINAL:
	case AG_DIALOG_GESTURE_STANDING_USETERMINALB:
	case AG_DIALOG_GESTURE_STANDING_BUZZED:
	case AG_DIALOG_GESTURE_STANDING_SMOKE:
	case AG_DIALOG_GESTURE_STANDING_OPENLOCKER:
	case AG_DIALOG_GESTURE_STANDING_CLOSELOCKER:
	case AG_DIALOG_GESTURE_STANDING_USELOCKER:
		{
			_GestureAction = ("ANIMSET_DIALOG_GESTURE_STANDING");
			_iGestureAction = 0;
			_iGesture -= AG_DIALOG_GESTURE_STANDING_NOD;
			break;
		}
		// Sitting gestures
	case AG_DIALOG_GESTURE_SITTING_BECKON:
	case AG_DIALOG_GESTURE_SITTING_WAVE:
	case AG_DIALOG_GESTURE_SITTING_WAVEOFF:
	case AG_DIALOG_GESTURE_SITTING_POINTFORWARD:
	case AG_DIALOG_GESTURE_SITTING_POINTLEFT:
	case AG_DIALOG_GESTURE_SITTING_POINTRIGHT:
	case AG_DIALOG_GESTURE_SITTING_POINTBACKWARD:
	case AG_DIALOG_GESTURE_SITTING_STOP:
	case AG_DIALOG_GESTURE_SITTING_SHRUG:
	case AG_DIALOG_GESTURE_SITTING_NODOFF:
	case AG_DIALOG_GESTURE_SITTING_SLEEP:
	case AG_DIALOG_GESTURE_SITTING_WAKE:
	case AG_DIALOG_GESTURE_SITTING_DICETHROWA:
	case AG_DIALOG_GESTURE_SITTING_DICETHROWB:
	case AG_DIALOG_GESTURE_SITTING_DICELOOKA:
	case AG_DIALOG_GESTURE_SITTING_DICELOOKB:
		{
			_GestureAction = ("ANIMSET_DIALOG_GESTURE_SITTING");
			_iGestureAction = 1;
			_iGesture -= AG_DIALOG_GESTURE_SITTING_BECKON;
			break;
		}
		// Guard gestures
	case AG_DIALOG_GESTURE_GUARD_POINTRIFLERIGHT:
	case AG_DIALOG_GESTURE_GUARD_POINTRIFLELEFT:
	case AG_DIALOG_GESTURE_GUARD_USEHELMCOM1:
	case AG_DIALOG_GESTURE_GUARD_USEHELMCOM2:
	case AG_DIALOG_GESTURE_GUARD_USEHELMCOMBROKEN:
	case AG_DIALOG_GESTURE_GUARD_WEAPONTWIRL:
	case AG_DIALOG_GESTURE_GUARD_WEAPONTOSS:
	case AG_DIALOG_GESTURE_GUARD_READYWEAPON:
	case AG_DIALOG_GESTURE_GUARD_USETERMINAL:
		{
			_GestureAction = ("ANIMSET_DIALOG_GESTURE_GUARD");
			_iGestureAction = 2;
			_iGesture -= AG_DIALOG_GESTURE_GUARD_POINTRIFLERIGHT;
			break;
		}
		// Lying gestures
	case AG_DIALOG_GESTURE_LYING_FALLASLEEP:
	case AG_DIALOG_GESTURE_LYING_WAKEUP:
	case AG_DIALOG_GESTURE_LYING_STANDUPFAST1:
	case AG_DIALOG_GESTURE_LYING_STANDUPFAST2:
		{
			_GestureAction = ("ANIMSET_DIALOG_GESTURE_LYING");
			_iGestureAction = 3;
			_iGesture -= AG_DIALOG_GESTURE_LYING_FALLASLEEP;
			break;
		}
	case AG_DIALOG_GESTURE_STANDING_N1GES1:
	case AG_DIALOG_GESTURE_STANDING_N1GES2:
	case AG_DIALOG_GESTURE_STANDING_N1GES3:
	case AG_DIALOG_GESTURE_STANDING_N1GES4:
	case AG_DIALOG_GESTURE_STANDING_N1GES5:
	case AG_DIALOG_GESTURE_STANDING_N1GES6:
	case AG_DIALOG_GESTURE_STANDING_N2GES1:
	case AG_DIALOG_GESTURE_STANDING_N2GES2:
	case AG_DIALOG_GESTURE_STANDING_N2GES3:
	case AG_DIALOG_GESTURE_STANDING_N2GES4:
	case AG_DIALOG_GESTURE_STANDING_N2GES5:
	case AG_DIALOG_GESTURE_STANDING_I1GES1:
	case AG_DIALOG_GESTURE_STANDING_I1GES2:
	case AG_DIALOG_GESTURE_STANDING_I1GES3:
	case AG_DIALOG_GESTURE_STANDING_I2GES1:
	case AG_DIALOG_GESTURE_STANDING_I2GES2:
	case AG_DIALOG_GESTURE_STANDING_I2GES3:
	case AG_DIALOG_GESTURE_STANDING_I2GES4:
	case AG_DIALOG_GESTURE_STANDING_I2GES5:
		{
			_GestureAction = ("ANIMSET_DIALOG_GESTURE_STANDING");
			_iGestureAction = 0;
			_iGesture -= AG_DIALOG_GESTURE_STANDING_EXTRADIFF;
			break;
		}
	case AG_DIALOG_GESTURE_SITTING_NA1:
	case AG_DIALOG_GESTURE_SITTING_NA2:
	case AG_DIALOG_GESTURE_SITTING_NA3:
	case AG_DIALOG_GESTURE_SITTING_NA4:
	case AG_DIALOG_GESTURE_SITTING_NB1:
	case AG_DIALOG_GESTURE_SITTING_NB2:
	case AG_DIALOG_GESTURE_SITTING_NB3:
	case AG_DIALOG_GESTURE_SITTING_IA1:
	case AG_DIALOG_GESTURE_SITTING_IA2:
	case AG_DIALOG_GESTURE_SITTING_IA3:
	case AG_DIALOG_GESTURE_SITTING_IB1:
		{
			
			_GestureAction = ("ANIMSET_DIALOG_GESTURE_SITTING");
			_iGestureAction = 1;
			_iGesture -= AG_DIALOG_GESTURE_SITTING_EXTRADIFF;
			break;
		}
	case AG_DIALOG_GESTURE_POPEJOE_BENDNECK:
	case AG_DIALOG_GESTURE_POPEJOE_SCRATCHNECK:
		{
			_GestureAction = ("ANIMSET_DIALOG_GESTURE_POPEJOE");
			_iGestureAction = 5;
			_iGesture -= AG_DIALOG_GESTURE_POPEJOE_BENDNECK;
			break;
		}
	default:
		break;
	};
}

void DialogFacialMood(int32& _iMood, int32& _iMoodAction, CStr& _MoodAction)
{
	switch (_iMood)
	{
	case AG_DIALOGFACIAL_MOOD_NEUTRAL:
	case AG_DIALOGFACIAL_MOOD_SCARED:
	case AG_DIALOGFACIAL_MOOD_ANGRY:
	case AG_DIALOGFACIAL_MOOD_SCARED_BIG:
	case AG_DIALOGFACIAL_MOOD_ANGRY_BIG:
	case AG_DIALOGFACIAL_MOOD_SAD_BIG:
	case AG_DIALOGFACIAL_MOOD_SAD_SMALL:
	case AG_DIALOGFACIAL_MOOD_HAPPY_BIG:
	case AG_DIALOGFACIAL_MOOD_HAPPY_SMALL:
	case AG_DIALOGFACIAL_MOOD_DRUNK_BIG:
	case AG_DIALOGFACIAL_MOOD_MADDOG:
	case AG_DIALOGFACIAL_MOOD_SARCASM_SMALL:
	case AG_DIALOGFACIAL_MOOD_NEUTRAL2:
		{
			// Find effect index
			_iMood -= AG_DIALOGFACIAL_MOOD_NEUTRAL;
			// Ok, find action, and get effects from that action?
			_MoodAction = ("ANIMSET_DIALOG_MOOD_FACIAL");
			_iMoodAction = 0;
			break;
		}
	default:
		break;
	}
}

void DialogFacialGesture(int32& _iGesture, int32& _iGestureAction, CStr& _GestureAction)
{
	switch (_iGesture)
	{
	case AG_DIALOGFACIAL_GESTURE_NODINSECURE:
	case AG_DIALOGFACIAL_GESTURE_THINKINSECURE:
	case AG_DIALOGFACIAL_GESTURE_SHAKEHEADCOOL:
	case AG_DIALOGFACIAL_GESTURE_BOTHBROWSDOWN:
	case AG_DIALOGFACIAL_GESTURE_BOTHBROWSUP:
	case AG_DIALOGFACIAL_GESTURE_BOTHEYESSHUT:
	case AG_DIALOGFACIAL_GESTURE_BOTHEYESSQUINT:
	case AG_DIALOGFACIAL_GESTURE_BOTHEYESWIDEOPEN:
	case AG_DIALOGFACIAL_GESTURE_LEFTBROWDOWN:
	case AG_DIALOGFACIAL_GESTURE_LEFTBROWUP:
	case AG_DIALOGFACIAL_GESTURE_LEFTEYESHUT:
	case AG_DIALOGFACIAL_GESTURE_LEFTEYESQUINT:
	case AG_DIALOGFACIAL_GESTURE_LEFTEYEWIDEOPEN:
	case AG_DIALOGFACIAL_GESTURE_RIGHTBROWDOWN:
	case AG_DIALOGFACIAL_GESTURE_RIGHTBROWUP:
	case AG_DIALOGFACIAL_GESTURE_RIGHTEYESHUT:
	case AG_DIALOGFACIAL_GESTURE_RIGHTEYESQUINT:
	case AG_DIALOGFACIAL_GESTURE_RIGHTEYEWIDEOPEN:
	case AG_DIALOGFACIAL_GESTURE_COCKLEFT:
	case AG_DIALOGFACIAL_GESTURE_COCKRIGHT:
	case AG_DIALOGFACIAL_GESTURE_LOOKLEFT:
	case AG_DIALOGFACIAL_GESTURE_LOOKRIGHT:
	case AG_DIALOGFACIAL_GESTURE_NO:
	case AG_DIALOGFACIAL_GESTURE_YES:
	case AG_DIALOGFACIAL_GESTURE_NODDOWN:
	case AG_DIALOGFACIAL_GESTURE_NODLEFT:
	case AG_DIALOGFACIAL_GESTURE_NODRIGHT:
	case AG_DIALOGFACIAL_GESTURE_NODUP:
	case AG_DIALOGFACIAL_GESTURE_COUGHBIG:
		{
			// Find effect index
			_iGesture -= AG_DIALOGFACIAL_GESTURE_NODINSECURE;
			// Ok, find action, and get effects from that action?
			_GestureAction = ("ANIMSET_DIALOG_GESTURE_FACIAL");
			_iGestureAction = 0;
			break;
		}
	default:
		break;
	}
}

void CWAGI::TagAnimSetFromDialog(const CWAGI_Context* _pContext, CMapData* _pMapData, 
									 CWorldData* _pWData, int32 _Type, const TArray<int32>& _lDialog)
{
	if (!AcquireAllResources(_pContext) || _pContext->m_pWPhysState->IsClient())
		return;

	CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(_pContext->m_pWPhysState);
	if (!pServerMod)
		return;

	// Ok, got the action, now find the animindex
	CWRes_AGAnimList* pAnimListRes;
	if(_pContext->m_pWPhysState)
	{
		pAnimListRes = _pContext->m_pWPhysState->GetMapData()->GetResource_AGAnimList(m_iAnimListRes);
	}
	else
	{
		M_ASSERT(_pContext->m_pMapData, "!");
		pAnimListRes = _pContext->m_pMapData->GetResource_AGAnimList(m_iAnimListRes);
	}

	CXRAG_AnimList* pAnimList = (pAnimListRes ? pAnimListRes->GetAnimList() : NULL);
	if (!pAnimList)
		return;

	int32 AnimListLen = pAnimList->m_lResources.Len();

	int32 DialogLen = _lDialog.Len();
	// Save found actions so we don't need to find them again
	const CXRAG_Action*  lpActions[AG_DIALOG_MAX_ANIMSETS];
	for (int32 i = 0; i < AG_DIALOG_MAX_ANIMSETS; i++)
		lpActions[i] = NULL;

	// Find all gesture animations
	for (int32 i = 0; i < DialogLen; i++)
	{
		int32 iDialog = _lDialog[i];
		int32 iDialogAction = -1;
		CStr DialogAction;
		switch (_Type)
		{
		case AG_DIALOG_MOOD:
			{
				DialogMood(iDialog, iDialogAction, DialogAction);
				break;
			}
		case AG_DIALOG_GESTURE:
			{
				DialogGesture(iDialog, iDialogAction, DialogAction);
				break;
			}
		case AG_DIALOG_FACIAL_MOOD:
			{
				DialogFacialMood(iDialog, iDialogAction, DialogAction);
				break;
			}
		case AG_DIALOG_FACIAL_GESTURE:
			{
				DialogFacialGesture(iDialog, iDialogAction, DialogAction);
				break;
			}
		default:
			break;
		};

		if (iDialogAction < 0 || iDialog < 0)
			continue;

		const CXRAG_Action* pAction = lpActions[iDialogAction];
		if (pAction == NULL)
		{
			CStr ActionName = DialogAction + ".ACTION_ANIMSET";
			int32 ActionHashKey = StringToHash(ActionName);
			int32 iAction = GetActionIndexFromHashKey(ActionHashKey);
			pAction = GetAction(iAction);

			lpActions[iDialogAction] = pAction;
		}
		if (!pAction)
			continue;
		//CContainerCollector Collector;

		// Tag directly into animlist
		int32 NumEffects = pAction->GetNumEffectInstances();
		int32 iBaseEffect = pAction->GetBaseEffectInstanceIndex();
		uint32 iEffectInstance = iBaseEffect + iDialog;
		const CXRAG_EffectInstance* pEffectInstance = m_spAnimGraph->GetEffectInstance(iEffectInstance);
		uint32 EffectID = pEffectInstance->m_ID;
		const CXRAG_ICallbackParams& IParams = m_spAnimGraph->GetICallbackParams(pEffectInstance->m_iParams, pEffectInstance->m_nParams);
		int32 NumParams = IParams.GetNumParams();
		for (int32 j = 0; j < NumParams; j++)
		{
			int32 iAnim = (int32)IParams.GetParam(j);
			if (iAnim < AnimListLen)
			{
				CStr ContainerName = m_spAnimList->GetContainerName(iAnim);
				int32 iAnimSeq = m_spAnimList->GetAnimSeqIndex(iAnim);
				//Collector.AddEntry(ContainerName,iAnimSeq, iAnim);
				pServerMod->AddAnimContainerEntry(ContainerName, iAnimSeq, iAnim);
				/*int32 iAnimContainerResource = _pWData->GetResourceIndex("ANM:" + ContainerName, WRESOURCE_CLASS_XSA, _pMapData);
				m_spAnimList->SetContainer(iAnim, iAnimContainerResource, _pWData->GetResourceRef(iAnimContainerResource));*/
			}
		}
		// Now we must load all collected resources
		//Collector.LoadData(_pMapData, _pWData, m_spAnimList);
	}

	UnacquireAllResources();
}

void CWAGI::TagAnimSetFromBehavior(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const TThinArray<int32>& _lBehavior)
{
	if (!AcquireAllResources(_pContext) || _pContext->m_pWPhysState->IsClient())
		return;

	int32 Len = _lBehavior.Len();
	if (Len <= 0)
		return;

	CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(_pContext->m_pWPhysState);
	if (!pServerMod)
		return;

	//ConOutL(CStrF("LOADING BEHAVIOR RESOURCES: %d",Len));
	
	// Ok, find action, and get effects from that action?
	CStr ActionName = "ANIMSET_BEHAVIOR.ACTION_ANIMSET";
	uint32 ActionHashKey = StringToHash(ActionName);
	int16 iAction = GetActionIndexFromHashKey(ActionHashKey);
	const CXRAG_Action* pAction = GetAction(iAction);
	if (!pAction)
		return;

	// Ok, got the action, now find the animindex
	CWRes_AGAnimList* pAnimListRes;
	if(_pContext->m_pWPhysState)
	{
		pAnimListRes = _pContext->m_pWPhysState->GetMapData()->GetResource_AGAnimList(m_iAnimListRes);
	}
	else
	{
		M_ASSERT(_pContext->m_pMapData, "!");
		pAnimListRes = _pContext->m_pMapData->GetResource_AGAnimList(m_iAnimListRes);
	}

	CXRAG_AnimList* pAnimList = (pAnimListRes ? pAnimListRes->GetAnimList() : NULL);
	if (!pAnimList)
		return;

	// Tag directly into animalist
	int32 AnimListLen = pAnimList->m_lResources.Len();
	int32 NumEffects = pAction->GetNumEffectInstances();
	int32 iBaseEffect = pAction->GetBaseEffectInstanceIndex();

	for (int32 i = 0; i < Len; i++)
	{
		int32 iBehavior = _lBehavior[i];
		iBehavior &= ~WANTEDBEHAVIORFLAG_MASKTYPE;
		//ConOut(CStrF("Behavior: %d", iBehavior));
		/*// Lightguardtalk is offset from normal behaviors (not anymore...)
		if (iBehavior >= AG_BEHAVIOR_LIGHTGUARDTALK)
			iBehavior = AG_BEHAVIOR_DICE_LOOK_LOSE + 1 + (iBehavior - AG_BEHAVIOR_LIGHTGUARDTALK);*/
		// There's a gab between AG_BEHAVIOR_PREACH and the behavior before that
		// Add empty entry in ag instead?
		/*if (iBehavior >= AG_BEHAVIOR_PREACH)
			iBehavior--;*/

		// Behaviors start at 1 take -1 to get correct index
		iBehavior--;

		if ((iBehavior < 0) || (iBehavior >= NumEffects))
			continue;

		//CContainerCollector Collector;

		uint32 iEffectInstance = iBaseEffect + iBehavior;
		const CXRAG_EffectInstance* pEffectInstance = m_spAnimGraph->GetEffectInstance(iEffectInstance);
		uint32 EffectID = pEffectInstance->m_ID;
		const CXRAG_ICallbackParams& IParams = m_spAnimGraph->GetICallbackParams(pEffectInstance->m_iParams, pEffectInstance->m_nParams);
		int32 NumParams = IParams.GetNumParams();
		for (int32 j = 0; j < NumParams; j++)
		{
			int32 iAnim = (int32)IParams.GetParam(j);
			if (iAnim < AnimListLen)
			{
				CStr ContainerName = m_spAnimList->GetContainerName(iAnim);
				// New test, collect all entries first and then group together containernames
				int32 iAnimSeq = m_spAnimList->GetAnimSeqIndex(iAnim);
				//Collector.AddEntry(ContainerName,iAnimSeq, iAnim);
				pServerMod->AddAnimContainerEntry(ContainerName, iAnimSeq, iAnim);
				/*int32 iAnimContainerResource = _pWData->GetResourceIndex("ANM:" + ContainerName, WRESOURCE_CLASS_XSA, _pMapData);
				//ConOutL(CStrF("Loading: %s:%d iBehavior: %d", ContainerName.GetStr(),m_spAnimList->m_lAnims[iAnim].m_iAnimSeq,iBehavior));
				m_spAnimList->SetContainer(iAnim, iAnimContainerResource, _pWData->GetResourceRef(iAnimContainerResource));*/
			}
		}
		// Now we must load all collected resources
		//Collector.LoadData(_pMapData, _pWData, m_spAnimList);
	}

	UnacquireAllResources();
}

void CWAGI::TagAnimSetFromActionCutscene(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const TArray<int32>& _liACS)
{
	if (!AcquireAllResources(_pContext) || _pContext->m_pWPhysState->IsClient())
		return;

	int32 Len = _liACS.Len();
	if (Len <= 0)
		return;

	CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(_pContext->m_pWPhysState);
	if (!pServerMod)
		return;

	//ConOutL(CStrF("LOADING BEHAVIOR RESOURCES: %d",Len));

	// Ok, find action, and get effects from that action?
	CStr ActionName = "ANIMSET_ACTIONCUTSCENE.ACTION_ANIMSET";
	uint32 ActionHashKey = StringToHash(ActionName);
	int16 iAction = GetActionIndexFromHashKey(ActionHashKey);
	const CXRAG_Action* pAction = GetAction(iAction);
	if (!pAction)
		return;

	// Ok, got the action, now find the animindex
	CWRes_AGAnimList* pAnimListRes;
	if(_pContext->m_pWPhysState)
	{
		pAnimListRes = _pContext->m_pWPhysState->GetMapData()->GetResource_AGAnimList(m_iAnimListRes);
	}
	else
	{
		M_ASSERT(_pContext->m_pMapData, "!");
		pAnimListRes = _pContext->m_pMapData->GetResource_AGAnimList(m_iAnimListRes);
	}

	CXRAG_AnimList* pAnimList = (pAnimListRes ? pAnimListRes->GetAnimList() : NULL);
	if (!pAnimList)
		return;

	// Tag directly into animalist
	int32 AnimListLen = pAnimList->m_lResources.Len();
	int32 NumEffects = pAction->GetNumEffectInstances();
	int32 iBaseEffect = pAction->GetBaseEffectInstanceIndex();

	for (int32 i = 0; i < Len; i++)
	{
		int32 iACS = _liACS[i];

		// Behaviors start at 1 take -1 to get correct index
		iACS--;

		if ((iACS < 0) || (iACS >= NumEffects))
			continue;

		//CContainerCollector Collector;

		uint32 iEffectInstance = iBaseEffect + iACS;
		const CXRAG_EffectInstance* pEffectInstance = m_spAnimGraph->GetEffectInstance(iEffectInstance);
		uint32 EffectID = pEffectInstance->m_ID;
		const CXRAG_ICallbackParams& IParams = m_spAnimGraph->GetICallbackParams(pEffectInstance->m_iParams, pEffectInstance->m_nParams);
		int32 NumParams = IParams.GetNumParams();
		for (int32 j = 0; j < NumParams; j++)
		{
			int32 iAnim = (int32)IParams.GetParam(j);
			if (iAnim < AnimListLen)
			{
				CStr ContainerName = m_spAnimList->GetContainerName(iAnim);
				// New test, collect all entries first and then group together containernames
				int32 iAnimSeq = m_spAnimList->GetAnimSeqIndex(iAnim);
				//Collector.AddEntry(ContainerName,iAnimSeq, iAnim);
				pServerMod->AddAnimContainerEntry(ContainerName, iAnimSeq, iAnim);
				/*int32 iAnimContainerResource = _pWData->GetResourceIndex("ANM:" + ContainerName, WRESOURCE_CLASS_XSA, _pMapData);
				ConOutL(CStrF("Loading: %s:%d iACS: %d, iAnim: %d", ContainerName.GetStr(),m_spAnimList->m_lAnims[iAnim].m_iAnimSeq,iACS+1,iAnim));
				m_spAnimList->SetContainer(iAnim, iAnimContainerResource, _pWData->GetResourceRef(iAnimContainerResource));*/
			}
		}
		// Now we must load all collected resources
		//Collector.LoadData(_pMapData, _pWData, m_spAnimList);
	}

	UnacquireAllResources();
}

int32 GetWeapon(int32 _Class)
{
	switch (_Class)
	{
	case RPG_EQUIPPEDITEMCLASS_UNDEFINED:
	case RPG_EQUIPPEDITEMCLASS_BAREHANDS:
		return 0;
	case RPG_EQUIPPEDITEMCLASS_GUN:
		return 1;
	case RPG_EQUIPPEDITEMCLASS_SHOTGUN:
	case RPG_EQUIPPEDITEMCLASS_ASSAULT:
		return 2;
	case RPG_EQUIPPEDITEMCLASS_MINGUN:
		return 3;
	case RPG_EQUIPPEDITEMCLASS_KNUCKLEDUSTER:
		return 4;
	case RPG_EQUIPPEDITEMCLASS_SHANK:
		return 5;
	case RPG_EQUIPPEDITEMCLASS_CLUB:
		return 6;
	case RPG_EQUIPPEDITEMCLASS_GRENADE:
		return 7;
	case RPG_EQUIPPEDITEMCLASS_TRANQ:
		return 8;
	default:
		return 0;
	}
}

// Ok then, need a itemclass crossreference table
#define NUMCROSSTYPES 9
void CWAGI::TagAnimSetFromItems(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, int32 _ItemClass1, int32 _ItemClass2)
{
	static char* sCrossRef[NUMCROSSTYPES][NUMCROSSTYPES] =
	{
		{ "","ANIMSET_CROSS_UNARMED_GUN","ANIMSET_CROSS_UNARMED_ASSAULT","","","ANIMSET_CROSS_UNARMED_SHANK","ANIMSET_CROSS_UNARMED_CLUB","","ANIMSET_CROSS_TRANQ"},
		{ "","","","","","","","","ANIMSET_CROSS_TRANQ"},
		{ "","","","","","","","","ANIMSET_CROSS_TRANQ"},
		{ "","","","","","","","","ANIMSET_CROSS_TRANQ"},
		{ "","","","","","","","","ANIMSET_CROSS_TRANQ"},
		{ "","ANIMSET_CROSS_SHANK_GUN","ANIMSET_CROSS_SHANK_ASSAULT","","","","ANIMSET_CROSS_SHANK_CLUB","","ANIMSET_CROSS_TRANQ"},
		{ "","ANIMSET_CROSS_CLUB_GUN","ANIMSET_CROSS_CLUB_ASSAULT","","","","","","ANIMSET_CROSS_TRANQ"},
		{ "","","","","","","","","ANIMSET_CROSS_TRANQ"},
		{ "","","","","","","","","ANIMSET_CROSS_TRANQ"}
	};

	// Mmkay then, find cross referenced animset and load it
	CStr Animset;
	int32 Weapon1 = GetWeapon(_ItemClass1);
	int32 Weapon2 = GetWeapon(_ItemClass2);
	if ((Weapon1 >= 0) && (Weapon1 < NUMCROSSTYPES) && (Weapon2 >= 0) && (Weapon2 < NUMCROSSTYPES))
		Animset = sCrossRef[Weapon1][Weapon2];

	if (Animset.Len() > 0)
	{
		//ConOut(CStrF("Loading: %s",Animset.GetStr()));
		TagAnimSetFromName(_pContext,_pMapData,_pWData,Animset);
	}
}

void CWAGI::TagAnimSetFromTargetState(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, CStr _StateAction)
{
	if (!AcquireAllResources(_pContext) || _pContext->m_pWPhysState->IsClient())
		return;
	
	CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(_pContext->m_pWPhysState);
	if (!pServerMod)
		return;

	// Ok, find action, and get effects from that action?
	uint32 ActionHashKey = StringToHash(_StateAction);
	int16 iAction = GetActionIndexFromHashKey(ActionHashKey);
	const CXRAG_Action* pAction = GetAction(iAction);
	if (!pAction)
		return;

	int16 iState = pAction->GetTargetStateIndex();
	const CXRAG_State* pState = GetState(iState);
	if (pState != NULL && pState->GetNumAnimLayers())
	{
		const CXRAG_AnimLayer* pAnimLayer = GetAnimLayer(pState->GetBaseAnimLayerIndex());
		if (pAnimLayer != NULL)
		{
			int16 iAnim = pAnimLayer->GetAnimIndex();
			CStr ContainerName = m_spAnimList->GetContainerName(iAnim);
			// New test, collect all entries first and then group together containernames
			int32 iAnimSeq = m_spAnimList->GetAnimSeqIndex(iAnim);
			pServerMod->AddAnimContainerEntry(ContainerName, iAnimSeq, iAnim);
		}
	}

	UnacquireAllResources();
}

void CWAGI::TagAnimSetFromLadderStepOffType(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, int32 _StepOffType)
{
	if (!AcquireAllResources(_pContext) || _pContext->m_pWPhysState->IsClient())
		return;

	CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(_pContext->m_pWPhysState);
	if (!pServerMod)
		return;

	//ConOutL(CStrF("LOADING BEHAVIOR RESOURCES: %d",Len));

	// Ok, find action, and get effects from that action?
	CStr ActionName = "ANIMSET_LADDERSTEPOFF.ACTION_ANIMSET";
	uint32 ActionHashKey = StringToHash(ActionName);
	int16 iAction = GetActionIndexFromHashKey(ActionHashKey);
	const CXRAG_Action* pAction = GetAction(iAction);
	if (!pAction)
		return;

	// Ok, got the action, now find the animindex
	CWRes_AGAnimList* pAnimListRes;
	if(_pContext->m_pWPhysState)
	{
		pAnimListRes = _pContext->m_pWPhysState->GetMapData()->GetResource_AGAnimList(m_iAnimListRes);
	}
	else
	{
		M_ASSERT(_pContext->m_pMapData, "!");
		pAnimListRes = _pContext->m_pMapData->GetResource_AGAnimList(m_iAnimListRes);
	}

	CXRAG_AnimList* pAnimList = (pAnimListRes ? pAnimListRes->GetAnimList() : NULL);
	if (!pAnimList)
		return;

	// Tag directly into animalist
	int32 AnimListLen = pAnimList->m_lResources.Len();
	int32 NumEffects = pAction->GetNumEffectInstances();
	int32 iBaseEffect = pAction->GetBaseEffectInstanceIndex();

	// Stepofftype starts at 1
	_StepOffType--;

	if ((_StepOffType < 0) || (_StepOffType >= NumEffects))
		return;

	//CContainerCollector Collector;

	uint32 iEffectInstance = iBaseEffect + _StepOffType;
	const CXRAG_EffectInstance* pEffectInstance = m_spAnimGraph->GetEffectInstance(iEffectInstance);
	uint32 EffectID = pEffectInstance->m_ID;
	const CXRAG_ICallbackParams& IParams = m_spAnimGraph->GetICallbackParams(pEffectInstance->m_iParams, pEffectInstance->m_nParams);
	int32 NumParams = IParams.GetNumParams();
	for (int32 j = 0; j < NumParams; j++)
	{
		int32 iAnim = (int32)IParams.GetParam(j);
		if (iAnim < AnimListLen)
		{
			CStr ContainerName = m_spAnimList->GetContainerName(iAnim);
			// New test, collect all entries first and then group together containernames
			int32 iAnimSeq = m_spAnimList->GetAnimSeqIndex(iAnim);
			//Collector.AddEntry(ContainerName,iAnimSeq, iAnim);
			pServerMod->AddAnimContainerEntry(ContainerName, iAnimSeq, iAnim);
		}
	}

	UnacquireAllResources();
}

void CWAGI::TagAnimSetFromVigilance(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, int32 _WeaponClass, const TThinArray<int32>& _lVigilance)
{
	int32 Len = _lVigilance.Len();
	if (Len <= 0)
		return;
	for (int32 i = 0; i < Len; i++)
	{
		CStr SetName;
		switch (_lVigilance[i])
		{
		case AI_IDLESTANCE_DEFAULT:
			{
				// Not interested in base case.?
				continue;
			}
		case AI_IDLESTANCE_SEARCHING:
			{
				if (_WeaponClass == RPG_EQUIPPEDITEMCLASS_GUN)
				{
					SetName = "ANIMSET_GUN_SEARCHING";
				}
				else if ((_WeaponClass == RPG_EQUIPPEDITEMCLASS_SHOTGUN) ||
							(_WeaponClass == RPG_EQUIPPEDITEMCLASS_ASSAULT))
				{
					SetName = "ANIMSET_ASSAULT_SEARCHING";
				}
				break;
			}
		case AI_IDLESTANCE_WARY:
			{
				if (_WeaponClass == RPG_EQUIPPEDITEMCLASS_GUN)
				{
					SetName = "ANIMSET_GUN_INVESTIGATE";
				}
				else if ((_WeaponClass == RPG_EQUIPPEDITEMCLASS_SHOTGUN) ||
					(_WeaponClass == RPG_EQUIPPEDITEMCLASS_ASSAULT))
				{
					SetName = "ANIMSET_ASSAULT_INVESTIGATE";
				}
				break;
			}
		case AI_IDLESTANCE_HOSTILE:
			{
				if (_WeaponClass == RPG_EQUIPPEDITEMCLASS_GUN)
				{
					SetName = "ANIMSET_GUN_COMBAT";
				}
				else if ((_WeaponClass == RPG_EQUIPPEDITEMCLASS_SHOTGUN) ||
					(_WeaponClass == RPG_EQUIPPEDITEMCLASS_ASSAULT))
				{
					SetName = "ANIMSET_ASSAULT_COMBAT";
				}
				break;
			}
		case AI_IDLESTANCE_JUMPY:
			{
				if (_WeaponClass == RPG_EQUIPPEDITEMCLASS_GUN)
				{
					SetName = "ANIMSET_GUN_INDARK";
				}
				else if ((_WeaponClass == RPG_EQUIPPEDITEMCLASS_SHOTGUN) ||
					(_WeaponClass == RPG_EQUIPPEDITEMCLASS_ASSAULT))
				{
					SetName = "ANIMSET_ASSAULT_INDARK";
				}
				break;
			}
		default:
			break;
		}

		// Tag animset from name
		if (SetName.Len() > 0)
			TagAnimSetFromName(_pContext,_pMapData,_pWData,SetName);

		//ConOutL(CStrF("LOADING ANIMATION: %s", SetName.GetStr()));		
	}
}
#else
void CWAGI::TagAnimSetFromName(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const CStr& _SetName)
{
	return;
}
void CWAGI::TagAnimSetFromDialog(const CWAGI_Context* _pContext, CMapData* _pMapData, 
								 CWorldData* _pWData, int32 _Type, const TArray<int32>& _lDialog)
{
	return;
}
 void CWAGI::TagAnimSetFromBehavior(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const TThinArray<int32>& _lBehavior)
 {
	 return;
 }
 void CWAGI::TagAnimSetFromVigilance(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, int32 _WeaponClass, const TThinArray<int32>& _lVigilance)
 {
	 return;
 }
 void CWAGI::TagAnimSetFromActionCutscene(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const TArray<int32>& _liACS)
 {
	 return;
 }
 void CWAGI::TagAnimSetFromItems(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, int32 _ItemClass1, int32 _ItemClass2)
 {
	 return;
 }
 void CWAGI::TagAnimSetFromTargetState(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, CStr _StateAction)
 {
	 return;
 }

 void CWAGI::TagAnimSetFromLadderStepOffType(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, int32 _StepOffType)
 {
	 return;
 }

#endif


// Tag entire animation set from containername (ie MovBas)
/*void CWAGI::TagAnimSet(const CWAGI_Context* _pContext, CStr _ContainerName)
{
	// Oki then, hopefully the animation list is loaded but not the animations, tag the animations
	// with the given containername to load them in later stage

	// Get animlist and hope for the best
	CWRes_AGAnimList* pAnimListRes;
	if(_pContext->m_pWPhysState)
	{
		pAnimListRes = _pContext->m_pWPhysState->GetMapData()->GetResource_AGAnimList(m_iAnimListRes);
	}
	else
	{
		M_ASSERT(_pContext->m_pMapData, "!");
		pAnimListRes = _pContext->m_pMapData->GetResource_AGAnimList(m_iAnimListRes);
	}

	CXRAG_AnimList* pAnimList = (pAnimListRes ? pAnimListRes->GetAnimList() : NULL);
	if (pAnimList)
	{
		
		// Go through each animation in the animlist, check for given containername and tag 
		// state? (seems like it could be done more efficiently)
		int32 Len = pAnimList->m_lResources.Len();
		CStr UpperContainer = _ContainerName.UpperCase();
		for (int32 i = 0; i < Len; i++)
		{
			// Bleh, small test for now
			if (pAnimList->m_lResources[i])
			{
				CStr Upper = pAnimList->m_lResources[i]->GetName();
				Upper.GetStrSep("ANM:");
				Upper = Upper.UpperCase();
				
				if (Upper == UpperContainer)
				{
					// Tag animation for loading
					pAnimList->m_lAnims[i].m_Flags |= CXRAG_Animation::XRAG_ANIMATION_TAGLOAD;
					//ConOut(CStrF("Tagging: %s:%d", pAnimList->m_lResources[i]->GetName().GetStr(),pAnimList->m_lAnims[i].m_iAnimSeq));
				}
			}
		}
	}
}*/

// Tag single animation from containername and index (ie MovBas:index)
/*void CWAGI::TagAnim(const CWAGI_Context* _pContext, CStr _ContainerName, int32 _iResource)
{
	// Oki then, hopefully the animation list is loaded but not the animations, tag the animations
	// with the given containername to load them in later stage

	// Get animlist and hope for the best
	CWRes_AGAnimList* pAnimListRes;
	if(_pContext->m_pWPhysState)
	{
		pAnimListRes = _pContext->m_pWPhysState->GetMapData()->GetResource_AGAnimList(m_iAnimListRes);
	}
	else
	{
		M_ASSERT(_pContext->m_pMapData, "!");
		pAnimListRes = _pContext->m_pMapData->GetResource_AGAnimList(m_iAnimListRes);
	}

	CXRAG_AnimList* pAnimList = (pAnimListRes ? pAnimListRes->GetAnimList() : NULL);
	if (pAnimList)
	{

		// Go through each animation in the animlist, check for given containername and tag 
		// state? (seems like it could be done more efficiently)
		int32 Len = pAnimList->m_lResources.Len();
		CStr UpperContainer = _ContainerName.UpperCase();
		for (int32 i = 0; i < Len; i++)
		{
			// Bleh, small test for now
			if (pAnimList->m_lResources[i])
			{
				CStr Upper = pAnimList->m_lResources[i]->GetName();
				Upper.GetStrSep("ANM:");
				Upper = Upper.UpperCase();

				if (Upper == UpperContainer)
				{
					if (pAnimList->m_lAnims[i].m_iAnimSeq == _iResource)
					{
						pAnimList->m_lAnims[i].m_Flags |= CXRAG_Animation::XRAG_ANIMATION_TAGLOAD;
						//ConOut(CStrF("Tagging: %s:%d", pAnimList->m_lResources[i]->GetName().GetStr(),_iResource));
					}
				}
			}
		}
	}
}*/

/*void CWAGI::TagAnim(const CWAGI_Context* _pContext, int32 _iAnim)
{
	// Tag animation directly in list

	// Get animlist and hope for the best
	CWRes_AGAnimList* pAnimListRes;
	if(_pContext->m_pWPhysState)
	{
		pAnimListRes = _pContext->m_pWPhysState->GetMapData()->GetResource_AGAnimList(m_iAnimListRes);
	}
	else
	{
		M_ASSERT(_pContext->m_pMapData, "!");
		pAnimListRes = _pContext->m_pMapData->GetResource_AGAnimList(m_iAnimListRes);
	}

	CXRAG_AnimList* pAnimList = (pAnimListRes ? pAnimListRes->GetAnimList() : NULL);
	if (pAnimList)
	{
		// Tag directly into animalist
		int32 Len = pAnimList->m_lResources.Len();
		if (_iAnim < Len)
		{
			pAnimList->m_lAnims[_iAnim].m_Flags |= CXRAG_Animation::XRAG_ANIMATION_TAGLOAD;
		}
	}
}*/