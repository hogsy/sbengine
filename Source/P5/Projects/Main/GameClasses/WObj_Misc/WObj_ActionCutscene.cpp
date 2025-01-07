/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WObj_ActionCutscene.cpp

Author:			Olle Rosenquist

Copyright:		Copyright O3 Games AB 2002

Contents:		

Comments:		

History:		
021014:		Created File
\*____________________________________________________________________________________________*/

#include "PCH.h"
#include "WObj_ActionCutScene.h"
#include "../WObj_Char.h"
#include "../WObj_Game/WObj_GameMod.h"
#include "../WObj_Player.h"
#include "../../../../Shared/MOS/Classes/GameWorld/Client/WClient_Core.h"
#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Game.h"
#include "../WRPG/WRPGChar.h"

static CVec3Dfp32 ACSTempDefinedDirection;
// Static offset positions from normal
CVec3Dfp32 CWObject_ActionCutscene::m_slStartPosition[ACTIONCUTSCENE_TYPE_NUMTYPES] = 
{
	CVec3Dfp32(-32.0f,0.0f,-95.0f),				// ACTIONCUTSCENE_TYPE_HATCHINVERTICALHIGH
	CVec3Dfp32(-32.0f,0.0f,0.0f),				// ACTIONCUTSCENE_TYPE_HATCHOUTVERTICALHIGH
	CVec3Dfp32(-32.0f,0.0f,0.0f),				// ACTIONCUTSCENE_TYPE_HATCHINVERTICALLOW
	CVec3Dfp32(-16.5f,0.0f,-39.0f),				// ACTIONCUTSCENE_TYPE_HATCHOUTVERTICALLOW
	CVec3Dfp32(-32.0f,0.0f,-56.0f),				// ACTIONCUTSCENE_TYPE_HATCHINHORIZONTAL
	CVec3Dfp32(-32.0f,0.0f,-20.0f),				// ACTIONCUTSCENE_TYPE_HATCHOUTHORIZONTAL
	CVec3Dfp32(-20.0f,0.0f,0.0f),					// ACTIONCUTSCENE_TYPE_PICKUPITEMGROUND
	CVec3Dfp32(-20.0f,0.0f,-40.0f),					// ACTIONCUTSCENE_TYPE_PICKUPITEMTABLE
	CVec3Dfp32(-32.0f,0,-40.0f),					// ACTIONCUTSCENE_TYPE_USEKEYPAD
	CVec3Dfp32(-38.0f,0.0f,-54.0f),				// ACTIONCUTSCENE_TYPE_USEMEDSTATION
	CVec3Dfp32(-24.0f,0.0f,-48.0f),				// ACTIONCUTSCENE_TYPE_USEPOWERCELL
	CVec3Dfp32(0.0f,0.0f,0.0f),					// ACTIONCUTSCENE_TYPE_JUSTACTIVATE
	CVec3Dfp32(-23.0f,-2.0f,-40.0f),				// ACTIONCUTSCENE_TYPE_USEBUTTON
	CVec3Dfp32(-16.75f,-4.25f,-40.0f),			// ACTIONCUTSCENE_TYPE_LEVER
	CVec3Dfp32(-24.0f,3.0f,-40.0f),				// ACTIONCUTSCENE_TYPE_VALVE
	CVec3Dfp32(-19.0f,-8.0f,-40.0f),				// ACTIONCUTSCENE_TYPE_USEDNA
	CVec3Dfp32(-52.0f,0.0f,-54.0f),				// ACTIONCUTSCENE_TYPE_RECHARGEMEDSTATION
	CVec3Dfp32(-6.0f,1.0f,-54.0f),				// ACTIONCUTSCENE_TYPE_USEEYESCANNER
	CVec3Dfp32(-32.0f,0,-40.0f),					// ACTIONCUTSCENE_TYPE_USETERMINAL
	CVec3Dfp32(-32.0f,0,-40.0f),					// ACTIONCUTSCENE_TYPE_ENTERHEAVYGUARD
	CVec3Dfp32(-32.0f,0,-40.0f),					// ACTIONCUTSCENE_TYPE_GETMINIFROMPOD,
	//16.954431f
	CVec3Dfp32(-38.0f,34.002949f,-36.0f),			// ACTIONCUTSCENE_TYPE_USEHEALTHSTATION,
	CVec3Dfp32(-48.0f,0,-40.0f),					// ACTIONCUTSCENE_TYPE_USESECURITYCARD,
	CVec3Dfp32(-41.0f,0,-18.0f),					// ACTIONCUTSCENE_TYPE_KICKGRATE,
	CVec3Dfp32(-20.0f,0.0f,0.0f),				// ACTIONCUTSCENE_TYPE_CROUCH_PICKUPITEMGROUND
	CVec3Dfp32(-20.0f,0.0f,-40.0f),				// ACTIONCUTSCENE_TYPE_CROUCH_PICKUPITEMTABLE
	CVec3Dfp32(-23.0f,-2.0f,-40.0f),				// ACTIONCUTSCENE_TYPE_USEBUTTON_LONG
	CVec3Dfp32(-23.0f,-2.0f,-40.0f),				// ACTIONCUTSCENE_TYPE_ENTERRIOT
	CVec3Dfp32(-48.0f,0,-40.0f),					// ACTIONCUTSCENE_TYPE_TELEPHONE
	CVec3Dfp32(-48.0f,0,-40.0f),					// ACTIONCUTSCENE_TYPE_PERISCOPE
};
CVec3Dfp32 CWObject_ActionCutscene::m_slEndPosition[ACTIONCUTSCENE_TYPE_NUMTYPES] =
{
	CVec3Dfp32(-32.0f,0.0f,-95.0f),				// ACTIONCUTSCENE_TYPE_HATCHINVERTICALHIGH
	CVec3Dfp32(-32.0f,0.0f,0.0f),				// ACTIONCUTSCENE_TYPE_HATCHOUTVERTICALHIGH
	CVec3Dfp32(-32.0f,0.0f,0.0f),				// ACTIONCUTSCENE_TYPE_HATCHINVERTICALLOW
	CVec3Dfp32(-16.5f,0.0f,-39.0f),				// ACTIONCUTSCENE_TYPE_HATCHOUTVERTICALLOW
	CVec3Dfp32(-32.0f,0.0f,-56.0f),				// ACTIONCUTSCENE_TYPE_HATCHINHORIZONTAL
	CVec3Dfp32(-32.0f,0.0f,-20.0f),				// ACTIONCUTSCENE_TYPE_HATCHOUTHORIZONTAL
	CVec3Dfp32(-20.0f,0.0f,0.0f),					// ACTIONCUTSCENE_TYPE_PICKUPITEMGROUND
	CVec3Dfp32(-20.0f,0.0f,-40.0f),					// ACTIONCUTSCENE_TYPE_PICKUPITEMTABLE
	CVec3Dfp32(-32.0f,0,-40.0f),					// ACTIONCUTSCENE_TYPE_USEKEYPAD
	CVec3Dfp32(-38.0f,0.0f,-54.0f),				// ACTIONCUTSCENE_TYPE_USEMEDSTATION
	CVec3Dfp32(-24.0f,0.0f,-48.0f),				// ACTIONCUTSCENE_TYPE_USEPOWERCELL
	CVec3Dfp32(0.0f,0.0f,0.0f),					// ACTIONCUTSCENE_TYPE_JUSTACTIVATE
	CVec3Dfp32(-23.0f,-2.0f,-40.0f),				// ACTIONCUTSCENE_TYPE_USEBUTTON
	CVec3Dfp32(-28.0f,0.0f,-40.0f),				// ACTIONCUTSCENE_TYPE_LEVER
	CVec3Dfp32(-24.0f,3.0f,-40.0f),				// ACTIONCUTSCENE_TYPE_VALVE
	CVec3Dfp32(-19.0f,-8.0f,-40.0f),				// ACTIONCUTSCENE_TYPE_USEDNA
	CVec3Dfp32(-52.0f,0.0f,-54.0f),				// ACTIONCUTSCENE_TYPE_RECHARGEMEDSTATION
	CVec3Dfp32(-6.0f,1.0f,-54.0f),				// ACTIONCUTSCENE_TYPE_USEEYESCANNER
	CVec3Dfp32(-32.0f,0,-40.0f),					// ACTIONCUTSCENE_TYPE_USETERMINAL
	CVec3Dfp32(-32.0f,0,-40.0f),					// ACTIONCUTSCENE_TYPE_ENTERHEAVYGUARD
	CVec3Dfp32(-32.0f,0,-40.0f),					// ACTIONCUTSCENE_TYPE_GETMINIFROMPOD,
	//16.954431f
	CVec3Dfp32(-38.0f,34.002949f,-36.0f),			// ACTIONCUTSCENE_TYPE_USEHEALTHSTATION,
	CVec3Dfp32(-48.0f,0,-40.0f),					// ACTIONCUTSCENE_TYPE_USESECURITYCARD,
	CVec3Dfp32(-41.0f,0,-18.0f),					// ACTIONCUTSCENE_TYPE_KICKGRATE,
	CVec3Dfp32(-20.0f,0.0f,0.0f),					// ACTIONCUTSCENE_TYPE_CROUCH_PICKUPITEMGROUND
	CVec3Dfp32(-20.0f,0.0f,-40.0f),					// ACTIONCUTSCENE_TYPE_CROUCH_PICKUPITEMTABLE
	CVec3Dfp32(-23.0f,-2.0f,-40.0f),				// ACTIONCUTSCENE_TYPE_USEBUTTON_LONG
	CVec3Dfp32(-23.0f,-2.0f,-40.0f),				// ACTIONCUTSCENE_TYPE_ENTERRIOT
	CVec3Dfp32(-48.0f,0,-40.0f),					// ACTIONCUTSCENE_TYPE_TELEPHONE
	CVec3Dfp32(-48.0f,0,-40.0f),					// ACTIONCUTSCENE_TYPE_PERISCOPE
};
/*CACSOffsetEntry CWObject_ActionCutscene::m_slStartPosition[ACTIONCUTSCENE_TYPE_NUMTYPES] = 
{
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_HATCHINVERTICALHIGH
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_HATCHOUTVERTICALHIGH
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_HATCHINVERTICALLOW
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_HATCHOUTVERTICALLOW
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_HATCHINHORIZONTAL
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_HATCHOUTHORIZONTAL
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_PICKUPITEMGROUND
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_PICKUPITEMTABLE
CACSOffsetEntry(CVec3Dfp32(-32.0f,0,-40.0f),0.0f,0.0f,0.5f),			// ACTIONCUTSCENE_TYPE_USEKEYPAD
CACSOffsetEntry(CVec3Dfp32(-22.0f,-2.0f,-54.0f),0.0f,0.0f,-0.25f),	// ACTIONCUTSCENE_TYPE_USEMEDSTATION
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_USEPOWERCELL
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_JUSTACTIVATE
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_USEBUTTON
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_LEVER
CACSOffsetEntry(CVec3Dfp32(-25.0f, 0.0f,-40.0f),0.0f,0.0f,0.5f),		// ACTIONCUTSCENE_TYPE_VALVE
CACSOffsetEntry(CVec3Dfp32(-28.777649f,-8.0f, -40.0f),0.0f,0.0f,0.5f)// ACTIONCUTSCENE_TYPE_USEDNA
};
CACSOffsetEntry CWObject_ActionCutscene::m_slEndPosition[ACTIONCUTSCENE_TYPE_NUMTYPES] =
{
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_HATCHINVERTICALHIGH
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_HATCHOUTVERTICALHIGH
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_HATCHINVERTICALLOW
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_HATCHOUTVERTICALLOW
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_HATCHINHORIZONTAL
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_HATCHOUTHORIZONTAL
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_PICKUPITEMGROUND
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_PICKUPITEMTABLE
CACSOffsetEntry(CVec3Dfp32(-32.0f,0,-40.0f),0.0f,0.0f,0.5f),			// ACTIONCUTSCENE_TYPE_USEKEYPAD
CACSOffsetEntry(CVec3Dfp32(-22.0f,-2.0f,-54.0f),0.0f,0.0f,-0.25f),	// ACTIONCUTSCENE_TYPE_USEMEDSTATION
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_USEPOWERCELL
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_JUSTACTIVATE
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_USEBUTTON
CACSOffsetEntry(0.0f,0.0f,0.0f,0.0f),								// ACTIONCUTSCENE_TYPE_LEVER
CACSOffsetEntry(CVec3Dfp32(-25.0f, 0.0f,-40.0f),0.0f,0.0f,0.5f),		// ACTIONCUTSCENE_TYPE_VALVE
CACSOffsetEntry(CVec3Dfp32(-28.777649f,-8.0f, -40.0f),0.0f,0.0f,0.5f)// ACTIONCUTSCENE_TYPE_USEDNA
};*/

// -------------------------------------------------------------------
//  CWObject_ActionCutscene
// -------------------------------------------------------------------
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Initializes the action cutscene

Comments:		
\*____________________________________________________________________*/
void CWObject_ActionCutscene::OnCreate()
{
	CWObject_ActionCutsceneParent::OnCreate();
	//m_Flags |= FLAGS_UNRELIABLE;
	m_ActionRetryCountdownValue = 0;
	m_PhysAttrib.m_Elasticy = 0.4f;
	//m_IntersectNotifyFlags = OBJECT_FLAGS_PLAYER;
	//	m_AnimTime = MRTC_RAND() % 255;
	// disable physics by default
	//m_iSuccessAnimation = -1;
	//m_iFailAnimation = -1;
	m_iStartPosition = 0;
	m_StartPositionVector = 0;
	m_StartRotationVector = 0;
	m_iEndPosition = 0;
	m_pWServer->Object_SetPhysics_ObjectFlags(m_iObject, GetPhysState().m_ObjectFlags | OBJECT_FLAGS_PICKUP);
	m_AGCrapMode = 0;
	m_ActionCutsceneTypeSuccess = ACTIONCUTSCENE_TYPE_UNDEFINED;
	m_ActionCutsceneTypeFail = ACTIONCUTSCENE_TYPE_UNDEFINED;
	m_FocusFrameOffset = 0;
	m_iLinkACS = -1;
	m_CachedRunTime.Reset();
	m_SetNoRefreshTick = 0;

	// Zero the cameraconfig array
	memset(m_lCameraConfigs, 0, sizeof(m_lCameraConfigs));
	m_CameraCount = 0;

	// How long time the ACS (atleast) must be activated
	m_MinActivationTime = 0.0f;
	m_MinActivationTimer = 0;
	// Set the string init array thingy (released in "OnSpawnWorld")
	m_lInitParams.SetLen(ACS_INITSTRING_NEEDED);
	m_FocusCameraConfig.m_CameraMode = 0;
	CACSClientData* pCD = GetACSClientData(this);
	if (pCD)
	{
		pCD->Clear();
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		?

Parameters:		
_pWData:		?
_pWServer:		World server

Comments:		
\*____________________________________________________________________*/
void CWObject_ActionCutscene::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	CWObject_ActionCutsceneParent::OnIncludeClass(_pWData, _pWServer);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		?

Parameters:		
_pWClient:		World client
_pEngine:		Renderer

Comments:		
\*____________________________________________________________________*/
void CWObject_ActionCutscene::OnClientPrecacheClass(CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	CWObject_ActionCutsceneParent::OnClientPrecacheClass(_pWClient, _pEngine);

	CXW_Surface* pOrgSurf = _pEngine->m_pSC->GetSurface("pickupglow");
	if(pOrgSurf)
	{
		CXW_Surface* pSurf = pOrgSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
		if(pSurf)
			pSurf->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	}
}

void CWObject_ActionCutscene::TagAnimationsForPrecache(CWAGI_Context* _pContext, CWAGI* _pAGI, TArray<int32>& _liACS)
{
	if (m_ActionCutsceneTypeSuccess > 0)
		_liACS.Add(m_ActionCutsceneTypeSuccess);
	if (m_ActionCutsceneTypeFail > 0)
		_liACS.Add(m_ActionCutsceneTypeFail);
	// MovBas+MovBasAss+....
	/*CStr AnimSetsNeeded;
	for (int32 i = 0; i < 2; i++)
	{
	int32 Type = (i == 0 ? m_ActionCutsceneTypeSuccess : m_ActionCutsceneTypeFail);
	switch(Type)
	{
	case ACTIONCUTSCENE_TYPE_HATCHINVERTICALHIGH:
	case ACTIONCUTSCENE_TYPE_HATCHOUTVERTICALHIGH:
	case ACTIONCUTSCENE_TYPE_HATCHINVERTICALLOW:
	case ACTIONCUTSCENE_TYPE_HATCHOUTVERTICALLOW:
	case ACTIONCUTSCENE_TYPE_HATCHINHORIZONTAL:
	case ACTIONCUTSCENE_TYPE_HATCHOUTHORIZONTAL:
	{
	AnimSetsNeeded += "+ANIMSET_ACTIONCUTSCENE_HATCHES";
	break;
	}
	case ACTIONCUTSCENE_TYPE_PICKUPITEMGROUND:
	case ACTIONCUTSCENE_TYPE_PICKUPITEMTABLE:
	case ACTIONCUTSCENE_TYPE_GETMINIFROMPOD:
	{
	AnimSetsNeeded += "+ANIMSET_ACTIONCUTSCENE_PICKUP";
	break;
	}
	case ACTIONCUTSCENE_TYPE_USEKEYPAD:
	case ACTIONCUTSCENE_TYPE_USEMEDSTATION:
	case ACTIONCUTSCENE_TYPE_USEPOWERCELL:
	case ACTIONCUTSCENE_TYPE_USEBUTTON:
	case ACTIONCUTSCENE_TYPE_USEDNA:
	case ACTIONCUTSCENE_TYPE_USETERMINAL:
	case ACTIONCUTSCENE_TYPE_USEHEALTHSTATION:
	case ACTIONCUTSCENE_TYPE_USESECURITYCARD:
	{
	AnimSetsNeeded += "+ANIMSET_ACTIONCUTSCENE_USE";
	break;
	}
	case ACTIONCUTSCENE_TYPE_LEVER:
	{
	AnimSetsNeeded += "+ANIMSET_ACTIONCUTSCENE_LEVER";
	break;
	}
	case ACTIONCUTSCENE_TYPE_VALVE:
	case ACTIONCUTSCENE_TYPE_USEEYESCANNER:
	{
	AnimSetsNeeded += "+ANIMSET_ACTIONCUTSCENE_VALVE";
	break;
	}		
	case ACTIONCUTSCENE_TYPE_ENTERHEAVY:
	{
	AnimSetsNeeded += "+ANIMSET_ACTIONCUTSCENE_SEQUENCES";
	break;
	}	
	default:
	break;
	}
	}

	while (AnimSetsNeeded.Len() > 0)
	{
	CStr SetName = AnimSetsNeeded.GetStrSep("+");
	if (SetName.Len() > 0)
	_pAGI->TagAnimSetFromName(_pContext, m_pWServer->GetMapData(), m_pWServer->m_spWData,SetName);
	}*/
}

void CWObject_ActionCutscene::TagAnimationsForPrecache(CWAG2I_Context* _pContext, CWAG2I* _pAGI, TArray<int32>& _liACS)
{
	if (m_ActionCutsceneTypeSuccess > 0)
		_liACS.Add(m_ActionCutsceneTypeSuccess);
	if (m_ActionCutsceneTypeFail > 0)
		_liACS.Add(m_ActionCutsceneTypeFail);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Configures the actioncutscene from registry keys

Parameters:		
_pKey:		Registry key

Returns:		Whether a match to the keys were made or not

Comments:		
\*____________________________________________________________________*/
void CWObject_ActionCutscene::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	CACSClientData* pCD = GetACSClientData(this);

	switch (_KeyHash)
	{
	case MHASH2('FLAG','S'): // "FLAGS"
		{
			static const char *FlagsTranslate[] =
			{
				"rotate", "fall", "waitspawn", "crouchatend", "disablephysics", "playsuccessanimation",
				"playfailanimation", "usethirdperson", "triggersuccess","triggersuccessend",
				"triggerfail","triggerfailend",	"dependsonitem", 
				"successsetstartposition", "successsetendposition", "failsetstartposition", 
				"failsetendposition", "locked","disableonsuccess", "pauseallai", "issuccess","isfail",
				"usefloatingstart","hasdefineddirection","triggersuccessmiddle","triggerfailmiddle",
				"useperfectpos","disable3pi","","useorgpos","saveseqpos",NULL
			};

			pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | KeyValue.TranslateFlags(FlagsTranslate));
			break;
		}
	case MHASH5('ACTI','ONCU','TSCE','NECA','MERA'): // "ACTIONCUTSCENECAMERA"
		{
			// All objects might not have been created yet, so set the string to a temporary parameter
			m_lInitParams[ACS_INITSTRING_CAMERAINITPARAMS] = KeyValue;
			break;
		}
	case MHASH3('FOCU','SCAM','ERA'): // "FOCUSCAMERA"
		{
			// All objects might not have been created yet, so set the string to a temporary parameter
			m_lInitParams[ACS_INITSTRING_FOCUSCAMERA] = KeyValue;
			break;
		}
	case MHASH3('DEPE','NDIT','EM'): // "DEPENDITEM"
		{
			// The item type neccessary for successful activation
			m_DependItem = KeyValue;
			CACSClientData* pCD = GetACSClientData(this);
			pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_DEPENDSONITEM);
			break;
		}
	case MHASH4('DEPE','NDME','SSAG','E'): // "DEPENDMESSAGE"
		{
			m_DependMessage.Parse(KeyValue, m_pWServer);
			break;
		}
	case MHASH4('RETR','YCOU','NTDO','WN'): // "RETRYCOUNTDOWN"
		{
			m_ActionRetryCountdownValue = int(KeyValue.Val_fp64() * m_pWServer->GetGameTicksPerSecond());
			break;
		}
	case MHASH4('ACS_','SUCC','TYPE','V3'): // "ACS_SUCCTYPEV3"
		{
			m_ActionCutsceneTypeSuccess = (uint16)_pKey->GetThisValuei();
			pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION);
			break;
		}
	case MHASH4('ACS_','FAIL','TYPE','V3'): // "ACS_FAILTYPEV3"
		{
			m_ActionCutsceneTypeFail = (uint16)_pKey->GetThisValuei();
			pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_PLAYFAILANIMATION);
			break;
		}
	case MHASH4('ACS_','TYPE','SUCC','V2'): // "ACS_TYPESUCCV2"
		{
			m_ActionCutsceneTypeSuccess = ResolveActionCutsceneType(KeyValue);

			if (m_ActionCutsceneTypeSuccess != ACTIONCUTSCENE_TYPE_UNDEFINED)
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION);
			else
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags & ~ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION);
			break;
		}
	case MHASH3('ACS_','TYPE','SUCC'): // "ACS_TYPESUCC"
		{
			m_ActionCutsceneTypeSuccess = ResolveActionCutsceneType(KeyValue);

			if (m_ActionCutsceneTypeSuccess != ACTIONCUTSCENE_TYPE_UNDEFINED)
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION);
			else
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags & ~ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION);
			break;
		}
	case MHASH4('ACS_','TYPE','FAIL','V2'): // "ACS_TYPEFAILV2"
		{
			m_ActionCutsceneTypeFail = ResolveActionCutsceneType(KeyValue);

			if (m_ActionCutsceneTypeFail != ACTIONCUTSCENE_TYPE_UNDEFINED)
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_PLAYFAILANIMATION);
			else
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags & ~ACTIONCUTSCENE_OPERATION_PLAYFAILANIMATION);
			break;
		}
	case MHASH3('ACS_','TYPE','FAIL'): // "ACS_TYPEFAIL"
		{
			m_ActionCutsceneTypeFail = ResolveActionCutsceneType(KeyValue);

			if (m_ActionCutsceneTypeFail != ACTIONCUTSCENE_TYPE_UNDEFINED)
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_PLAYFAILANIMATION);
			else
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags & ~ACTIONCUTSCENE_OPERATION_PLAYFAILANIMATION);
			break;
		}
	case MHASH5('MINA','CTIV','ATIO','NTIM','E'): // "MINACTIVATIONTIME"
		{
			m_MinActivationTime = KeyValue.Val_fp64();
			break;
		}
	case MHASH6('ACSC','AMER','ACON','TROL','LEDV','2'): // "ACSCAMERACONTROLLEDV2"
		{
			// Hmm, ok we got a version2 camera selection, controlled either enginepath 
			// or regular controlled
			m_lInitParams[ACS_INITSTRING_CAMERAV2CONTROLLED] = KeyValue;
			/*CStr Params = _pKey->GetThisValue();
			CStr Target = Params.GetStrSep(",");
			bool bContinue = ((Target.Len() > 0) && (m_CameraCount < ACTIONCUTSCENE_MAXCAMERAS));
			while (bContinue)
			{
			int32 iTarget = m_pWServer->Selection_GetSingleTarget(Target);
			if (CActionCutsceneCamera::MakeConfigBlock(m_lCameraConfigs[m_CameraCount],
			CActionCutsceneCamera::ACS_CAMERAMODE_CONTROLLED,iTarget))
			m_CameraCount++;

			Target = Params.GetStrSep(",");
			bContinue = ((Target.Len() > 0) && (m_CameraCount < ACTIONCUTSCENE_MAXCAMERAS));
			}*/
			break;
		}
	case MHASH7('ACSC','AMER','ACON','TROL','LEDO','NCHA','RV2'): // "ACSCAMERACONTROLLEDONCHARV2"
		{
			// Hmm, ok we got a version2 camera selection, controlled either enginepath 
			// or regular controlled
			m_lInitParams[ACS_INITSTRING_CAMERAV2CONTROLLEDONCHAR] = KeyValue;
			/*CStr Params = _pKey->GetThisValue();
			CStr Target = Params.GetStrSep(",");
			bool bContinue = ((Target.Len() > 0) && (m_CameraCount < ACTIONCUTSCENE_MAXCAMERAS));
			while (bContinue)
			{
			int32 iTarget = m_pWServer->Selection_GetSingleTarget(Target);
			if (CActionCutsceneCamera::MakeConfigBlock(m_lCameraConfigs[m_CameraCount],
			CActionCutsceneCamera::ACS_CAMERAMODE_CONTROLLEDONCHAR,iTarget))
			m_CameraCount++;

			Target = Params.GetStrSep(",");
			bContinue = ((Target.Len() > 0) && (m_CameraCount < ACTIONCUTSCENE_MAXCAMERAS));
			}*/
			break;
		}
	case MHASH3('FOCU','SNAM','E'): // "FOCUSNAME"
		{
			m_UseName = KeyValue;
			break;
		}
	case MHASH3('FOCU','SDES','C'): // "FOCUSDESC"
		{
			m_DescName = KeyValue;
			break;
		}
	case MHASH4('FOCU','SFRA','MEOF','FSET'): // "FOCUSFRAMEOFFSET"
		{
			m_FocusFrameOffset = (int8)KeyValue.Val_int();
			break;
		}
	case MHASH4('ACS_','DIRE','CTIO','N'): // "ACS_DIRECTION"
		{
			/*CVec3Dfp32 Position = GetPosition();
			CVec3Dfp32 Direction;
			Direction.ParseString(_pKey->GetThisValue());
			Direction -= Position;
			Direction.Normalize();
			// Invert direction (we want to face this direction)
			Direction = -Direction;
			Direction = CWObject_Character::GetLook(Direction);
			m_DefinedDirection = Direction.k[2];
			m_CutsceneFlags |= ACTIONCUTSCENE_FLAGS_HASDEFINEDDIRECTION;*/
			ACSTempDefinedDirection.ParseString(KeyValue);
			pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_FLAGS_HASDEFINEDDIRECTION);
			break;
		}
	case MHASH2('LINK','_ACS'): // "LINK_ACS"
		{
			m_lInitParams[ACS_INITSTRING_LINKACS] = KeyValue;
			break;
		}
	case MHASH2('PARE','NT'): // "PARENT"
	case MHASH2('ATTA','CH'): // "ATTACH"
		{
			m_Flags |= FLAGS_UNRELIABLE | FLAGS_GAMEPHYSICS;
			CWObject_ActionCutsceneParent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	case MHASH4('ACTI','VATI','ONAR','C'):
		{
			pCD->SetActivationArc(M_Cos((_PI / 180.0f) * _pKey->GetThisValuef()));
			break;
		}
	default:
		{
			if(KeyName.Find("MSG_LIGHTSWITCH") != -1)
			{
				// OBJMSG_LIGHT_IMPULSE  and MSG_TOGGLEto target
				CStr Message = CStrF("0x0112;%s;2;",KeyValue.GetStr());
				m_lMsg_TriggerSuccessMiddle.SetLen(m_lMsg_TriggerSuccessMiddle.Len() + 1);
				m_lMsg_TriggerSuccessMiddle[m_lMsg_TriggerSuccessMiddle.Len()-1].Parse(Message, m_pWServer);
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESSMIDDLE);
			}
			else if(KeyName.Find("MSG_TRIGGERMIDDLE") != -1)
			{
				m_lMsg_TriggerSuccessMiddle.SetLen(m_lMsg_TriggerSuccessMiddle.Len() + 1);
				m_lMsg_TriggerSuccessMiddle[m_lMsg_TriggerSuccessMiddle.Len()-1].Parse(KeyValue, m_pWServer);
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESSMIDDLE);
			}
			else if(KeyName.Find("MSG_TRIGGERSUCCESS") != -1)
			{
				m_lMsg_TriggerSuccess.SetLen(m_lMsg_TriggerSuccess.Len() + 1);
				m_lMsg_TriggerSuccess[m_lMsg_TriggerSuccess.Len()-1].Parse(KeyValue, m_pWServer);
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS);
			}
			else if(KeyName.Find("MSG_TRIGGERFAIL") != -1)
			{
				m_lMsg_TriggerFail.SetLen(m_lMsg_TriggerFail.Len() + 1);
				m_lMsg_TriggerFail[m_lMsg_TriggerFail.Len()-1].Parse(KeyValue, m_pWServer);
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_TRIGGERFAIL);
			}
			else if(KeyName.Find("MSG_ENDTRIGGERSUCCESS") != -1)
			{
				m_lMsg_TriggerSuccessEnd.SetLen(m_lMsg_TriggerSuccessEnd.Len() + 1);
				m_lMsg_TriggerSuccessEnd[m_lMsg_TriggerSuccessEnd.Len()-1].Parse(KeyValue, m_pWServer);
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESSEND);
			}
			else if(KeyName.Find("MSG_ENDTRIGGERFAIL") != -1)
			{
				m_lMsg_TriggerFailEnd.SetLen(m_lMsg_TriggerFailEnd.Len() + 1);
				m_lMsg_TriggerFailEnd[m_lMsg_TriggerFailEnd.Len()-1].Parse(KeyValue, m_pWServer);
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_TRIGGERFAILEND);
			}
			else if(KeyName.Find("MSG_MIDDLETRIGGERFAIL") != -1)
			{
				m_lMsg_TriggerFailMiddle.SetLen(m_lMsg_TriggerFailMiddle.Len() + 1);
				m_lMsg_TriggerFailMiddle[m_lMsg_TriggerFailMiddle.Len()-1].Parse(KeyValue, m_pWServer);
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_TRIGGERFAILMIDDLE);
			}
			else if(KeyName.Find("MSG_ONENTERFOCUSCAMERA") != -1)
			{
				m_lMsg_OnEnterFocusCamera.SetLen(m_lMsg_OnEnterFocusCamera.Len() + 1);
				m_lMsg_OnEnterFocusCamera[m_lMsg_OnEnterFocusCamera.Len()-1].Parse(KeyValue, m_pWServer);
			}
			else if(KeyName.Find("MSG_ONLEAVEFOCUSCAMERA") != -1)
			{
				m_lMsg_OnLeaveFocusCamera.SetLen(m_lMsg_OnLeaveFocusCamera.Len() + 1);
				m_lMsg_OnLeaveFocusCamera[m_lMsg_OnLeaveFocusCamera.Len()-1].Parse(KeyValue, m_pWServer);
			}
			else if (KeyName.Find("STARTPOSITION") != -1)
			{
				m_lInitParams[ACS_INITSTRING_STARTPOSITION] = KeyValue;
			}
			else if (KeyName.Find("ENDPOSITION") != -1)
			{
				m_lInitParams[ACS_INITSTRING_ENDPOSITION] = KeyValue;
			}
			else
				CWObject_ActionCutsceneParent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_ActionCutscene::OnFinishEvalKeys()
{
	// Use glow on objects
	CWObject_ActionCutsceneParent::OnFinishEvalKeys();
	CACSClientData* pCD = GetACSClientData(this);
	m_Flags |= FLAGS_USEHINT;
	// DISABLE ALL 3PI CAMERAS!!
	pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_DISABLE3PI);

	if (pCD->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_ROTATE)
		ClientFlags() |= ACS_CLIENTFLAG_ROTATE;

	// Set so that the actioncutscene won't be updated when it's not activated
	if(!(m_Flags & FLAGS_UNRELIABLE))
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;

	if (pCD->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_HASDEFINEDDIRECTION)
	{
		CVec3Dfp32 Position = GetPosition();
		ACSTempDefinedDirection -= Position;
		ACSTempDefinedDirection.Normalize();
		// Invert direction (we want to face this direction)
		ACSTempDefinedDirection = -ACSTempDefinedDirection;
		ACSTempDefinedDirection = CWObject_Character::GetLook(ACSTempDefinedDirection);
		m_DefinedDirection = 1.0f - ACSTempDefinedDirection.k[2];
	}

	// Set usename to clientdata
	pCD->SetChoiceString(m_UseName);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Message handler for this object

Parameters:		
_Msg:			Message that was sent to this object

Returns:		Return value depends on the message

Comments:		
\*____________________________________________________________________*/
aint CWObject_ActionCutscene::OnMessage(const CWObject_Message& _Msg)
{
	CACSClientData* pCD = GetACSClientData(this);
	switch (_Msg.m_Msg)
	{
	case OBJMSG_GAME_SPAWN:
		{
			if (_Msg.m_Param0 != 0)
			{
				pCD->m_CutsceneFlags |= ACTIONCUTSCENE_FLAGS_WAITSPAWN;
				ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;
				return true;
			}
			else
			{
				if(!(pCD->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_WAITSPAWN))
					return 0;
				pCD->SetCutsceneFlags(pCD->m_CutsceneFlags & ~ACTIONCUTSCENE_FLAGS_WAITSPAWN);
				ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
				m_pWServer->Phys_InsertPosition(m_iObject, this);
				m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);
				pCD->SetChoiceString(m_UseName);
				pCD->m_CopyFlags |= CACSClientData::TELEPHONE_CHOICESTRING_DIRTY;
				// "disdisable"
				pCD->SetDisabled(false);
				return true;
			}
		}
	case OBJMSG_ACTIONCUTSCENE_ISACTIONCUTSCENE:
		{
			return true;
		}
	case OBJMSG_CHAR_USE:
		if(m_iAnim2 != 0)
			return m_pWServer->Message_SendToObject(_Msg, m_iAnim2);

		// Run OBJMSG_ACTIONCUTSCENE_ACTIVATE if not

	case OBJMSG_ACTIONCUTSCENE_ACTIVATE:
		{
			// Do some activating stuff.....

			// Assuming sender is the character id
			if (_Msg.m_iSender != -1 && CanActivate(_Msg.m_iSender))
				return DoAction(_Msg.m_iSender);

			return false;
		}
	case OBJMSG_ACTIONCUTSCENE_GETRELATIVESTARTPOSITION:
		{
			if((m_StartPositionVector == CVec3Dfp32(0))
				&& (m_StartRotationVector == CVec3Dfp32(0)))
				return false;	// Nothing to see here.

			fp32 RelStartData[6];	// Only used to check datasize of this message. Any other better way to do this? -rune
			if (_Msg.m_DataSize == sizeof(RelStartData))
			{
				fp32* pRelStartData = (fp32*)_Msg.m_pData;
				pRelStartData[0] = m_StartPositionVector.k[0];
				pRelStartData[1] = m_StartPositionVector.k[1];
				pRelStartData[2] = m_StartPositionVector.k[2];
				pRelStartData[3] = m_StartRotationVector.k[0];
				pRelStartData[4] = m_StartRotationVector.k[1];
				pRelStartData[5] = m_StartRotationVector.k[2];
				return 1;
			}
			return 0;
		}
	case OBJMSG_ACTIONCUTSCENE_GETENDPOSITION:
		{
			// Return the index of the endposition
			return m_iEndPosition;
		}
	case OBJMSG_ACTIONCUTSCENE_ADDCUTSCENECAMERA:
		{
			if (m_CameraCount >= ACTIONCUTSCENE_MAXCAMERAS)
				return false;
			// Assume the first param is a pointer to a cstr
			CStr* pStr = (CStr*) _Msg.m_Param0;

			if (pStr && CActionCutsceneCamera::MakeConfigBlockFromString(
				m_lCameraConfigs[m_CameraCount], *pStr, m_pWServer))
				m_CameraCount++;
		}
	case OBJMSG_ACTIONCUTSCENE_GETNEWCUTSCENECAMERA:
		{
			// BE CAREFUL USING THIS MESSAGE, IT WILL REPLACE PREVIOUS CAMERA
			// Find new cutscene camera, sender must be specified
			/*m_spActiveCutsceneCamera = GetActionCutsceneCamera(m_iObject, _Msg.m_iSender);
			CActionCutsceneCamera* pCam = m_spActiveCutsceneCamera;

			// Assume data pointer is pointer to a pointer
			if (_Msg.m_pData)
			{
			CActionCutsceneCamera** pGCam = (CActionCutsceneCamera**)_Msg.m_pData;
			*pGCam = pCam;

			return true;
			}*/

			return GetActionCutsceneCamera(m_iObject, _Msg.m_iSender, (CCameraUtil*) _Msg.m_pData);

			return false;
		}
	case OBJMSG_ACTIONCUTSCENE_GETFOCUSCUTSCENECAMERA:
		{
			GetFocusCutsceneCamera(m_iObject, _Msg.m_iSender, (CCameraUtil*) _Msg.m_pData);
			// Send "enterfocus" messages
			OnEnterFocusCamera(_Msg.m_iSender);
			return true;
		}
	case OBJMSG_ACTIONCUTSCENE_REUSEFOCUSCUTSCENECAMERA:
		{
			// Only create if it's valid
			//if (m_FocusCameraConfig.m_CameraMode != 0)
			return GetFocusCutsceneCamera(m_iObject, _Msg.m_iSender, (CCameraUtil*) _Msg.m_pData,true);
			return false;
		}
	case OBJMSG_ACTIONCUTSCENE_LEAVEFOCUSCUTSCENECAMERA:
		{
			OnLeaveFocusCamera(_Msg.m_iSender);
			return true;
		}
	case OBJMSG_ACTIONCUTSCENE_GETACTIVECUTSCENECAMERA:
		{
			CActionCutsceneCamera* pCam = m_spActiveCutsceneCamera;

			// Assume data pointer is pointer to a pointer
			if (_Msg.m_pData)
			{
				CActionCutsceneCamera** pGCam = (CActionCutsceneCamera**)_Msg.m_pData;
				*pGCam = pCam;

				return true;
			}

			return false;
		}
	case OBJMSG_IMPULSE:
		{
			// Ok, "impulse 0" is the message to unlock it
			// and, "impulse 1" is the message to lock it

			switch (_Msg.m_Param0)
			{
			case ACTIONCUTSCENE_IMPULSE_UNLOCK:
				{
					pCD->SetCutsceneFlags(pCD->m_CutsceneFlags & ~ACTIONCUTSCENE_OPERATION_LOCKED);
					break;
				}
			case ACTIONCUTSCENE_IMPULSE_LOCK:
				{
					pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_LOCKED);
					break;
				}
			case ACTIONCUTSCENE_IMPULSE_SENDSUCCESSMESSAGES:
				{
					// Send the success messages
					for (int32 i = 0; i < m_lMsg_TriggerSuccess.Len(); i++)
						m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _Msg.m_iSender, m_pWServer);
					break;
				}
			case ACTIONCUTSCENE_IMPULSE_FORCEACTIVATE:
				{
					return DoActionSuccess(_Msg.m_iSender);
				}
			default:
				{
					// Maybe the impulse should be sent to the parent
					// and then again, maybe not...?
					CWObject_ActionCutsceneParent::OnMessage(_Msg);
					break;
				}
			}

			return true;
		}
	case OBJMSG_ACTIONCUTSCENE_ONENDACS:
		{
			return OnEndACS(_Msg.m_iSender);
		}
	case OBJMSG_ACTIONCUTSCENE_GETTYPE:
		{
			return ACTIONCUTSCENE_TYPE_ACS;
		}
	case OBJMSG_ACTIONCUTSCENE_CANENDACS:
		{
			return m_pWServer->GetGameTick() >= m_MinActivationTimer;
		}
	case OBJMSG_ACTIONCUTSCENE_ISWAITSPAWNED:
		{
			return (pCD->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_WAITSPAWN);
		}
	case OBJMSG_ACTIONCUTSCENE_DOTRIGGER:
		{
			// Send the success trigger messages 
			// Check if trigger message should be sent
			CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_Msg.m_iSender);
			CWO_Character_ClientData* pCDChar = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
			if (!pCDChar)
				return 0;

			int32 Mode = pCDChar->m_ControlMode_Param4;

			if (Mode & ACTIONCUTSCENE_FLAGS_ISSUCCESS)
			{
				if (pCD->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS)
				{
					// Send the trigger message
					for (int32 i = 0; i < m_lMsg_TriggerSuccess.Len(); i++)
						m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _Msg.m_iSender, m_pWServer);

					// Remove ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS from controlmode param
					pCDChar->m_ControlMode_Param4 = pCDChar->m_ControlMode_Param4 & ~ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS;
				}
				return 1;
			}
			else if (Mode & ACTIONCUTSCENE_FLAGS_ISFAIL)
			{
				// Check if trigger message should be sent
				if ((pCD->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERFAIL) && !(m_AGCrapMode & ACS_AGCRAPMODE_FAILSENT))
				{
					// Send the trigger message
					m_AGCrapMode |= ACS_AGCRAPMODE_FAILSENT;
					for (int32 i = 0; i < m_lMsg_TriggerFail.Len(); i++)
						m_lMsg_TriggerFail[i].SendMessage(m_iObject, _Msg.m_iSender, m_pWServer);
				}
				return 1;
			}

			return 0;
		}
	case OBJMSG_ACTIONCUTSCENE_DOTRIGGERMIDDLE:
		{
			// Send the success trigger middle messages
			// Check if trigger message should be sent
			CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_Msg.m_iSender);
			CWO_Character_ClientData* pCDChar = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
			if (!pCDChar)
				return 0;

			int32 Mode = pCDChar->m_ControlMode_Param4;

			if (Mode & ACTIONCUTSCENE_FLAGS_ISSUCCESS)
			{
				if ((pCD->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESSMIDDLE) && !(m_AGCrapMode & ACS_AGCRAPMODE_MIDDLESUCCSENT))
				{
					m_AGCrapMode |= ACS_AGCRAPMODE_MIDDLESUCCSENT;
					// Send the trigger message
					for (int32 i = 0; i < m_lMsg_TriggerSuccessMiddle.Len(); i++)
						m_lMsg_TriggerSuccessMiddle[i].SendMessage(m_iObject, _Msg.m_iSender, m_pWServer);

					// Remove ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS from controlmode param
					pCDChar->m_ControlMode_Param4 = pCDChar->m_ControlMode_Param4 & ~ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESSMIDDLE;
				}
				return 1;
			}
			else if (Mode & ACTIONCUTSCENE_FLAGS_ISFAIL)
			{
				if ((pCD->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERFAILMIDDLE) && !(m_AGCrapMode & ACS_AGCRAPMODE_MIDDLEFAILSENT))
				{
					m_AGCrapMode |= ACS_AGCRAPMODE_MIDDLEFAILSENT;
					// Send the trigger message
					for (int32 i = 0; i < m_lMsg_TriggerFailMiddle.Len(); i++)
						m_lMsg_TriggerFailMiddle[i].SendMessage(m_iObject, _Msg.m_iSender, m_pWServer);

					// Remove ACTIONCUTSCENE_OPERATION_TRIGGERFAIL from controlmode param
					pCDChar->m_ControlMode_Param4 = pCDChar->m_ControlMode_Param4 & ~ACTIONCUTSCENE_OPERATION_TRIGGERFAILMIDDLE;
				}
				return 1;
			}

			return 0;
		}
	case OBJMSG_ACTIONCUTSCENE_CANACTIVATE:
		{
			// Check if the actioncutscene can be activated (ie character holds correct items..)
			// and such
			// Assuming sender is the character id
			if (_Msg.m_iSender != -1)
				return CanActivate(_Msg.m_iSender);

			return 0;
		}
	case OBJMSG_CHAR_GETCHOICES:
		{
			if(m_iAnim2 != 0)
				m_pWServer->Message_SendToObject(_Msg, m_iAnim2);

			return 1;
		}

	case OBJMSG_CHAR_GETUSENAME:
		{
			CFStr *pSt = (CFStr *)_Msg.m_pData;
			if(!pSt)
				return 0;

			CACSClientData* pCD = GetACSClientData(this);
			if (!pCD->m_bDisabled)
				*pSt = m_UseName;
			return 1;
		}
	case OBJMSG_CHAR_GETDESCNAME:
		{
			CFStr *pSt = (CFStr *)_Msg.m_pData;
			if(!pSt)
				return 0;

			CACSClientData* pCD = GetACSClientData(this);
			if (!pCD->m_bDisabled)
				*pSt = m_DescName;
			return 1;
		}
	case OBJMSG_CHAR_SETUSENAME:
		{
			if(!_Msg.m_pData)
				return 0;

			m_UseName = (const char*) _Msg.m_pData;
			return 1;
		}
	case OBJMSG_CHAR_SETDESCNAME:
		{
			if(!_Msg.m_pData)
				return 0;

			m_DescName = (const char*) _Msg.m_pData;
			return 1;
		}
	case OBJMSG_CHAR_GETFOCUSOFFSET:
		{
			return m_FocusFrameOffset;
		}
	case OBJMSG_CHAR_SETFOCUSFRAMEOFFSET:
		{
			m_FocusFrameOffset = (int8)_Msg.m_Param0;
			return true;
		}
	case OBJMSG_ACTIONCUTSCENE_SHOULDPAUSEALLAI:
		{
			//Check if we should pause AI when in this cutscene
			return (pCD->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_PAUSEALLAI);
		}
	case OBJMSG_ACTIONCUTSCENE_SETDIALOGUEPROXY:
		{
			if(_Msg.m_pData)
				iAnim2() = m_pWServer->Selection_GetSingleTarget((char *)_Msg.m_pData);
			else
				iAnim2() = 0;
			return 1;
		}
	case OBJMSG_ACTIONCUTSCENE_USE3PI:
		{
			return !(pCD->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_DISABLE3PI);
		}
	default:
		return CWObject_ActionCutsceneParent::OnMessage(_Msg);
	}
}

aint CWObject_ActionCutscene::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_CHAR_GETCHOICES:
		{
			// If we can't activate this acs, don't set any choices?
			if (!CanActivateClient(_pWClient, _pObj, _Msg.m_iSender))
				return 0;

			if(_pObj->m_iAnim2 != 0)
				return _pWClient->ClientMessage_SendToObject(_Msg, _pObj->m_iAnim2);
			CACSClientData* pCD = GetACSClientData(_pObj);
			CStr *pSt = (CStr *)_Msg.m_pData;
			if(!pSt || !pCD)
				return 1;

			*pSt = pCD->m_ChoiceString;
			return 1;
		}
	case OBJMSG_ACTIONCUTSCENE_CANACTIVATE:
		{
			// Check if the actioncutscene can be activated (ie character holds correct items..)
			// and such
			// Assuming sender is the character id
			if (_Msg.m_iSender != -1)
				return CanActivateClient(_pWClient, _pObj, _Msg.m_iSender);

			return 0;
		}
	case OBJMSG_HOOK_GETRENDERMATRIX:
		{
			CMTime *pTime = (CMTime *)_Msg.m_Param0;
			CMat4Dfp32 *pMat = (CMat4Dfp32 *)_Msg.m_Param1;
			*pMat = ((CWObject_ActionCutscene *)_pObj)->GetRenderMatrix(_pWClient, *pTime, _pWClient->GetGameTick(), _pWClient->GetRenderTickFrac());
			return 1;
		}
	case OBJMSG_HOOK_GETCURRENTMATRIX:
		{
			CMTime Time = GetTime(_pObj, _pWClient, _pWClient->GetGameTick(), _pWClient->GetRenderTickFrac());
			CMat4Dfp32* pMat = (CMat4Dfp32*)_Msg.m_Param0;
			*pMat = ((CWObject_ActionCutscene *)_pObj)->GetRenderMatrix(_pWClient, Time, _pWClient->GetGameTick(), _pWClient->GetRenderTickFrac());
			return 1;
		}
	default:
		break;
	}
	return CWObject_ActionCutsceneParent::OnClientMessage(_pObj, _pWClient, _Msg);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Disables the cutscene camera at the end of the 
cutscene

Parameters:		
_iCharacter:	Character index of the character who activated
this object

Returns:		Whether the operation was ok or not

Comments:		
\*____________________________________________________________________*/
bool CWObject_ActionCutscene::OnEndACS(int _iCharacter)
{
	// Invalidate actioncutscene camera
	CACSClientData* pCDACS = GetACSClientData(this);
	CWObject* pChar = m_pWServer->Object_Get(_iCharacter);
	CWO_Character_ClientData* pCD = (pChar ? CWObject_Character::GetClientData(pChar) : NULL);
	int32 CutsceneFlags = 0;
	// Retry a bit later
	pCDACS->SetRetryCountDown(m_pWServer->GetGameTick() + m_ActionRetryCountdownValue);

	if (pCD)
	{
		int32 ACSType = (int32)pCD->m_ControlMode_Param2;
		if (ACSType == ACTIONCUTSCENE_TYPE_RECHARGEMEDSTATION)
			pCDACS->SetRetryCountDown(0);
		if (m_spActiveCutsceneCamera)
			m_spActiveCutsceneCamera->SetInvalidToPcd(pCD);
		CutsceneFlags = pCD->m_ControlMode_Param4;

		// Reset controlmode params
		pCD->m_ControlMode_Param0 = 0;
		pCD->m_ControlMode_Param1 = 0;
		pCD->m_ControlMode_Param2 = 0;
		pCD->m_ControlMode_Param3 = 0;
		pCD->m_ControlMode_Param4 = 0;
	}

	m_spActiveCutsceneCamera = NULL;

	if (m_MinActivationTime > 0.0f)
		m_MinActivationTimer = 0;

	if ((CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESSEND) && !(m_AGCrapMode & ACS_AGCRAPMODE_SUCCENDSENT))
	{
		m_AGCrapMode |= ACS_AGCRAPMODE_SUCCENDSENT;
		for (int32 i = 0; i < m_lMsg_TriggerSuccessEnd.Len(); i++)
			m_lMsg_TriggerSuccessEnd[i].SendMessage(m_iObject, _iCharacter, m_pWServer);
	}
	if ((CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERFAILEND) && !(m_AGCrapMode & ACS_AGCRAPMODE_FAILENDSENT))
	{
		m_AGCrapMode |= ACS_AGCRAPMODE_FAILENDSENT;
		for (int32 i = 0; i < m_lMsg_TriggerFailEnd.Len(); i++)
			m_lMsg_TriggerFailEnd[i].SendMessage(m_iObject, _iCharacter, m_pWServer);
	}

	// If the acs should be disabled, just remove it
	if (CutsceneFlags & ACTIONCUTSCENE_OPERATION_DISABLEONSUCCESS)
	{
		//m_pWServer->Object_Destroy(m_iObject);
		pCDACS->SetDisabled(true);
	}

	// Link to another actioncutscene
	if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_LINKACS)
	{
		m_pWServer->Phys_Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_ACTIVATE,0,0,_iCharacter),m_iLinkACS);
	}

	if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SAVESEQPOS)
	{
		CMat4Dfp32 TempMat,OrgMat;
		CMTime Time = GetTime(this,m_pWServer,m_pWServer->GetGameTick(),0.0f);
		TempMat = GetRenderMatrix(m_pWServer,Time,m_pWServer->GetGameTick(),0.0f);
		if (m_pWServer->Phys_Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETORGMATRIX,(aint)&OrgMat),m_iObject))
		{
			CMat4Dfp32 InvOrg;
			OrgMat.InverseOrthogonal(InvOrg);
			TempMat.Multiply(InvOrg,OrgMat);
			pCDACS->SetCachedMat(OrgMat);
		}
		// Can't remove refresh until after 1 update (so new cached mat gets to the client...)
		pCDACS->m_CutsceneFlags |= ACTIONCUTSCENE_OPERATION_SETNOREFRESH;
		m_SetNoRefreshTick = m_pWServer->GetGameTick() + 2;
		// Stop..
		OnImpulse(-1,_iCharacter);
	}
	else if(!(m_Flags & FLAGS_UNRELIABLE))
	{
		// Disable further updates of this object since it won't be updated anymore
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
	}

	return true;
}

CMat4Dfp32 CWObject_ActionCutscene::GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFraction)
{
	// Render mat....
	CACSClientData* pCD = GetACSClientData(this);
	if (!pCD || !(pCD->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SAVESEQPOS))
		return CWObject_ActionCutsceneParent::GetRenderMatrix(_pWPhysState,_Time,_GameTick,_TickFraction);

	CMat4Dfp32 EPMat,ResMat;
	EPMat = CWObject_ActionCutsceneParent::GetRenderMatrix(_pWPhysState,_Time,_GameTick,_TickFraction);
	pCD->m_CachedSeqMat.Multiply(EPMat,ResMat);

	return	ResMat;
}

void CWObject_ActionCutscene::Run(int _iType)
{
	// Getcurrentmatrix
	CACSClientData* pCD = GetACSClientData(this);
	if (pCD && (pCD->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SAVESEQPOS))
	{
		CMat4Dfp32 TempMat,OrgMat;
		TempMat = GetRenderMatrix(m_pWServer,m_CachedRunTime,m_pWServer->GetGameTick(),0.0f);
		if (/*m_pWServer->Phys_Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (aint)&m_CachedRunTime,(aint)&TempMat),m_iObject) && */
			m_pWServer->Phys_Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETORGMATRIX,(aint)&OrgMat),m_iObject))
		{
			CMat4Dfp32 InvOrg;
			OrgMat.InverseOrthogonal(InvOrg);
			TempMat.Multiply(InvOrg,OrgMat);
			pCD->SetCachedMat(OrgMat);
		}
	}
	return CWObject_ActionCutsceneParent::Run(_iType);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Determines if the character has all the neccessary
requirements for successful activation, if not the
failed animation is played (if any) instead

Parameters:		
_iCharacter:	Character index of the character who activated
this object

Returns:		Whether the operation was ok or not

Comments:		
\*____________________________________________________________________*/
bool CWObject_ActionCutscene::DoAction(int _iCharacter)
{
	CACSClientData* pCDACS = GetACSClientData(this);
	// Reset AG CRAP MODE
	m_AGCrapMode = 0;
	// Make sure we cannot activate all the time
	int32 GameTick = m_pWServer->GetGameTick();
	if ((GameTick < pCDACS->m_ActionRetryCountdown) || pCDACS->m_bDisabled /*|| 
																		   (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_WAITSPAWN)*/)
	{
		return false;
	}

	// Reset engine_path
	Reset();
	// Deactivate item
	m_pWServer->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DEACTIVATEITEM, AG2_ITEMSLOT_WEAPONS,1), _iCharacter);

	CWObject* pObj = m_pWServer->Object_Get(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);

	if (!pCD)
		return false;
	// Save actioncutscene position
	pCD->m_DialoguePlayerPos = pObj->GetPositionMatrix();

	if ((pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_DEPENDSONITEM) || 
		(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_LOCKED) ||
		m_DependMessage.IsValid())
	{
		// Check if the character has the item this action depends on... FIXME
		bool bDoSuccess = false;

		if (!(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_LOCKED))
		{
			bDoSuccess = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETITEMFROMSTRING,0,0,
				m_iObject,0,0,0, &m_DependItem, sizeof(CStr*)), _iCharacter) != NULL;
		}

		if(m_DependMessage.IsValid())
			bDoSuccess = m_DependMessage.SendMessage(m_iObject, _iCharacter, m_pWServer) != 0;

		return (bDoSuccess ? DoActionSuccess(_iCharacter) : DoActionFail(_iCharacter));
	}
	else
	{
		// No restrictions, just do the successaction
		return DoActionSuccess(_iCharacter);
	}
}

bool CWObject_ActionCutscene::CanActivate(int _iCharacter)
{
	CACSClientData* pCDACS = GetACSClientData(this);
	int32 GameTick = m_pWServer->GetGameTick();
	if ((GameTick < pCDACS->m_ActionRetryCountdown))
		return false;

	// If we are already active on this character, return false
	CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
	if (pCD)
	{
		if ((CWObject_Character::Char_GetControlMode(pObj) == PLAYER_CONTROLMODE_ACTIONCUTSCENE) &&
			((int)pCD->m_ControlMode_Param0 == m_iObject))
			return false;
	}

	// Check so that characters look direction is within "arc" and position is within arc as well
	if (m_pWServer->Object_GetPositionMatrix(_iCharacter).GetRow(0) * -GetPositionMatrix().GetRow(0) >= pCDACS->m_ActivationArc)
	{
		CVec3Dfp32 Dir = GetPosition() - m_pWServer->Object_GetPositionMatrix(_iCharacter).GetRow(3);
		Dir.k[2] = 0.0f;
		Dir.Normalize();
		if (Dir * -GetPositionMatrix().GetRow(0) < pCDACS->m_ActivationArc)
			return false;
	}
	else
		return false;

	if ((pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_DEPENDSONITEM) || 
		/*(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_LOCKED) ||*/
		m_DependMessage.IsValid())
	{
		// Check if the character has the item this action depends on... FIXME
		bool bCanActivate = false;

		if (!(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_LOCKED))
		{
			bCanActivate = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETITEMFROMSTRING,0,0,
				m_iObject,0,0,0, &m_DependItem, sizeof(CStr*)), _iCharacter) != NULL;
		}

		if(m_DependMessage.IsValid())
			bCanActivate = m_DependMessage.SendMessage(m_iObject, _iCharacter, m_pWServer) != 0;

		return bCanActivate;
	}
	else
	{
		if (pCDACS->m_bDisabled || (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_WAITSPAWN))
			return false;

		// No restrictions, just do the successaction
		return true;
	}
}

bool CWObject_ActionCutscene::CanActivateClient(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int _iCharacter)
{
	// If we are already active on this character, return false
	CACSClientData* pCDACS = GetACSClientData(_pObj);
	int32 GameTick = _pWPhys->GetGameTick();
	if ((GameTick < pCDACS->m_ActionRetryCountdown))
		return false;
	CWObject_CoreData* pObj = _pWPhys->Object_GetCD(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
	if (pCD)
	{
		if ((CWObject_Character::Char_GetControlMode(pObj) == PLAYER_CONTROLMODE_ACTIONCUTSCENE) &&
			((int)pCD->m_ControlMode_Param0 == _pObj->m_iObject))
			return false;
	}

	// Check so that characters look direction is within "arc"
	if (_pWPhys->Object_GetPositionMatrix(_iCharacter).GetRow(0) * -_pObj->GetPositionMatrix().GetRow(0) >= pCDACS->m_ActivationArc)
	{
		CVec3Dfp32 Dir = _pObj->GetPosition() - _pWPhys->Object_GetPositionMatrix(_iCharacter).GetRow(3);
		Dir.k[2] = 0.0f;
		Dir.Normalize();
		if (Dir * -_pObj->GetPositionMatrix().GetRow(0) < pCDACS->m_ActivationArc)
			return false;
	}
	else
		return false;

	if (/*(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_DEPENDSONITEM) || */
		(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_LOCKED))
	{
		// Check if the character has the item this action depends on... FIXME
		bool bCanActivate = false;

		/*if (!(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_LOCKED))
		{
		bCanActivate = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETITEMFROMSTRING,0,0,
		m_iObject,0,0,0, &m_DependItem, sizeof(CStr*)), _iCharacter) != NULL;
		}

		if(m_DependMessage.IsValid())
		bCanActivate = m_DependMessage.SendMessage(m_iObject, _iCharacter, m_pWServer) != 0;*/

		return bCanActivate;
	}
	else
	{
		if (pCDACS->m_bDisabled || (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_WAITSPAWN))
			return false;

		// No restrictions, just do the successaction
		return true;
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Constructs a custum animation block from the 
flags that are set for successful activation

Parameters:		
_iCharacter:	Character index of the character who activated
this object

Returns:		Whether the operation was ok or not

Comments:		
\*____________________________________________________________________*/
bool CWObject_ActionCutscene::DoActionSuccess(int _iCharacter)
{
	/*// Check if trigger message should be sent
	if (m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS)
	{
	// Send the trigger message
	int32 Len = m_lMsg_TriggerSuccess.Len();
	for (int32 i = 0; i < Len; i++)
	{
	if (m_lMsg_TriggerSuccess[i].IsValid())
	m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _iCharacter, m_pWServer);
	}
	}*/

	m_AGCrapMode = 0;
	CACSClientData* pCDACS = GetACSClientData(this);
	// Send impulse 1 to the base class when successful
	OnImpulse(1, _iCharacter);
	pCDACS->SetRetryCountDown(m_pWServer->GetGameTick() + m_ActionRetryCountdownValue);

	// If the animation isn't to be played, return.. (maybe should change this)
	if (!(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION) || 
		(m_ActionCutsceneTypeSuccess == ACTIONCUTSCENE_TYPE_UNDEFINED))
	{
		return false;
	}

	// Hmm, ok we need the same functionality on the new animation system thingy
	CWObject* pObj = m_pWServer->Object_Get(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);

	if (!pCD)
		return false;

	if (m_ActionCutsceneTypeSuccess == ACTIONCUTSCENE_TYPE_JUSTACTIVATE)
	{
		SetStartPosition(_iCharacter);
		// Send the success trigger messages 
		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS)
		{
			// Send the trigger message
			for (int32 i = 0; i < m_lMsg_TriggerSuccess.Len(); i++)
				m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _iCharacter, m_pWServer);
		}

		// If the acs should be disabled, just remove it
		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_DISABLEONSUCCESS)
		{
			//m_pWServer->Object_Destroy(m_iObject);
			pCDACS->SetDisabled(true);
		}
		// Link to another actioncutscene
		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_LINKACS)
		{
			m_pWServer->Phys_Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_ACTIVATE,0,0,_iCharacter),m_iLinkACS);
		}
	}
	else
	{
		// Enter actioncutscene controlmode
		CWObject_Character::Char_SetControlMode(pObj, PLAYER_CONTROLMODE_ACTIONCUTSCENE);
		((CWObject_Character *)pObj)->UpdateVisibilityFlag();
		// Send ACS impulse to animgraph
		CWAG2I_Context AG2Context(pObj,m_pWServer,CMTime::CreateFromTicks(pCD->m_GameTick + 1,m_pWServer->GetGameTickTime()));
		pCD->m_AnimGraph2.GetAG2I()->ClearExtraTokens();
		pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&AG2Context, CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,m_ActionCutsceneTypeSuccess),0);

		// Set action cutscene id to controlmode param0
		pCD->m_ControlMode_Param0 = (fp32)m_iObject;

		// Set cutscenetype to param2
		pCD->m_ControlMode_Param2 = (fp32)m_ActionCutsceneTypeSuccess;

		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_HASDEFINEDDIRECTION)
		{
			// Set defined direction to param1
			pCD->m_ControlMode_Param1 = m_DefinedDirection;
		}

		SetStartPosition(_iCharacter);
		// Ok determine what type of cutscene we're having and deal with positioning accordingly
		// Only the hatch animations should be able to move around (and thus use the old positioning)
		// Filter out success actions and set to param4
		if (m_ActionCutsceneTypeSuccess > ACTIONCUTSCENE_TYPE_HATCHOUTHORIZONTAL)
		{
			int32 Flags = ((pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SUCCESSMASK) & 
				~(ACTIONCUTSCENE_OPERATION_SETSTARTPOSMASK/*|ACTIONCUTSCENE_OPERATION_SETENDPOSMASK*/)) | 
				ACTIONCUTSCENE_FLAGS_ISSUCCESS | ACTIONCUTSCENE_FLAGS_USEFLOATINGSTART;
			if (m_ActionCutsceneTypeSuccess == ACTIONCUTSCENE_TYPE_USEHEALTHSTATION)
				Flags |= ACTIONCUTSCENE_OPERATION_USEPERFECTPOS;
			pCD->m_ControlMode_Param4 = Flags;
		}
		else
		{
			pCD->m_ControlMode_Param4 = ((pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SUCCESSMASK) & 
				~(ACTIONCUTSCENE_OPERATION_SETSTARTPOSMASK)) | 
				ACTIONCUTSCENE_FLAGS_ISSUCCESS | ACTIONCUTSCENE_FLAGS_USEFLOATINGSTART;
			/*// Add startpos mask (implied for hatches nowadays)
			pCD->m_ControlMode_Param4 = (m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SUCCESSMASK) | 
			ACTIONCUTSCENE_FLAGS_ISSUCCESS | ACTIONCUTSCENE_OPERATION_SETSTARTPOSMASK;*/
		}


		// Make sure the camera will be updated when the ACS is active
		ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

		// Start activation timer
		if (m_MinActivationTime > 0.0f)
			m_MinActivationTimer = m_pWServer->GetGameTick() + 
			(int)(m_MinActivationTime * m_pWServer->GetGameTicksPerSecond());
		else
			m_MinActivationTimer = 0;
	}

	return true;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Constructs a custum animation block from the 
flags that are set for failed activation

Parameters:		
_iCharacter:	Character index of the character who activated
this object

Returns:		Whether the operation was ok or not

Comments:		
\*____________________________________________________________________*/
bool CWObject_ActionCutscene::DoActionFail(int _iCharacter)
{
	/*// Check if trigger message should be sent
	if (m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERFAIL)
	{
	// Send the trigger message
	m_Msg_TriggerFail.SendMessage(m_iObject, _iCharacter, m_pWServer);
	}*/

	CACSClientData* pCDACS = GetACSClientData(this);
	// If the animation isn't to be played, return.. (maybe should change this)
	if (!(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_PLAYFAILANIMATION) || 
		(m_ActionCutsceneTypeFail == ACTIONCUTSCENE_TYPE_UNDEFINED))
	{
		pCDACS->SetRetryCountDown(m_pWServer->GetGameTick() + m_ActionRetryCountdownValue);
		return false;
	}

	// Hmm, ok we need the same functionality on the new animation system thingy
	CWObject* pObj = m_pWServer->Object_Get(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);

	if (!pCD)
		return false;

	if (m_ActionCutsceneTypeFail == ACTIONCUTSCENE_TYPE_JUSTACTIVATE)
	{
		SetStartPosition(_iCharacter);
		// Send the success trigger messages 
		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERFAIL)
		{
			// Send the trigger message
			for (int32 i = 0; i < m_lMsg_TriggerFail.Len(); i++)
				m_lMsg_TriggerFail[i].SendMessage(m_iObject, _iCharacter, m_pWServer);
		}
	}
	else
	{
		// Enter actioncutscene controlmode
		CWObject_Character::Char_SetControlMode(pObj, PLAYER_CONTROLMODE_ACTIONCUTSCENE);
		((CWObject_Character *)pObj)->UpdateVisibilityFlag();

		// Set action cutscene id to controlmode param0
		pCD->m_ControlMode_Param0 = (fp32)m_iObject;
		// Set cutscenetype to param2
		pCD->m_ControlMode_Param2 = (fp32)m_ActionCutsceneTypeFail;

		if (m_ActionCutsceneTypeFail > ACTIONCUTSCENE_TYPE_HATCHOUTHORIZONTAL)
		{
			// Filter out success actions and set to param4
			pCD->m_ControlMode_Param4 = ((pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_FAILMASK) &
				~(ACTIONCUTSCENE_OPERATION_SETSTARTPOSMASK|ACTIONCUTSCENE_OPERATION_SETENDPOSMASK))
				| ACTIONCUTSCENE_FLAGS_ISFAIL  | ACTIONCUTSCENE_FLAGS_USEFLOATINGSTART;;
		}
		else
		{
			pCD->m_ControlMode_Param4 = ((pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_FAILMASK) &
				~(ACTIONCUTSCENE_OPERATION_SETSTARTPOSMASK))
				| ACTIONCUTSCENE_FLAGS_ISFAIL  | ACTIONCUTSCENE_FLAGS_USEFLOATINGSTART;;
			/*pCD->m_ControlMode_Param4 = (m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_FAILMASK)
			| ACTIONCUTSCENE_FLAGS_ISFAIL;*/
		}

		// Make sure the ledge camera will be updated when the ACS is active
		ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
	}

	return true;
}

void CWObject_ActionCutscene::SetStartPosition(int32 _iCharacter)
{
	CACSClientData* pCD = GetACSClientData(this);
	if (!(pCD->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SETSTARTPOSMASK))
		return;

	CMat4Dfp32 PosMat;
	bool bOk = false;
	if (m_iStartPosition > 0)
	{
		// We got an object index to get camera position from, horay for us!
		PosMat = m_pWServer->Object_GetPositionMatrix(m_iStartPosition);
		bOk = true;
	} 
	else
	{
		// Try to ask for a position relative to ourselves
		CWObject_Message Msg(OBJMSG_ACTIONCUTSCENE_GETRELATIVESTARTPOSITION);
		fp32 RelStartData[6];
		Msg.m_pData = &RelStartData;
		Msg.m_DataSize = sizeof(RelStartData);
		if (m_pWServer->Message_SendToObject(Msg, m_iObject))
		{
			CVec3Dfp32 StartPos;
			CVec3Dfp32 StartRot;
			StartPos.k[0] = RelStartData[0];
			StartPos.k[1] = RelStartData[1];
			StartPos.k[2] = RelStartData[2];
			StartRot.k[0] = RelStartData[3];
			StartRot.k[1] = RelStartData[4];
			StartRot.k[2] = RelStartData[5];
			PosMat = GetPositionMatrix();
			(StartPos * PosMat).SetMatrixRow(PosMat, 3);
			if (StartRot.k[0] != 0.0f) PosMat.RotX_x_M(StartRot.k[0]);
			if (StartRot.k[1] != 0.0f) PosMat.RotY_x_M(StartRot.k[1]);
			if (StartRot.k[2] != 0.0f) PosMat.RotZ_x_M(StartRot.k[2]);
			bOk = true;
		}
	}

	if (bOk)
	{
		CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(PosMat,3);
		// Make sure we don't collide with floor
		Pos.k[2] += 0.001f;
		Pos.SetMatrixRow(PosMat, 3);
		m_pWServer->Object_SetPosition(_iCharacter, PosMat);
		m_pWServer->Object_SetVelocity(_iCharacter, CVec3Dfp32(0.0f));
	}	
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Renders the model

Parameters:		
_pObj:			Model object
_pWClient:		World client
_pEngine:		Renderer

Comments:		
\*____________________________________________________________________*/
/*void CWObject_ActionCutscene::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
if((_pObj->m_ClientFlags & CLIENTFLAGS_INVISWHENSTOPPED) && !(_pObj->m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED)))
return;
if (_pObj->m_ClientFlags & CLIENTFLAGS_DEACTIVATED)
return;

bool bMat = false;
CMat4Dfp32 Mat;

fp32 Time = 0;
for(int i = 0; i < CWO_NUMMODELINDICES; i++)
{
CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
if(pModel)
{
if(!bMat)
{
Time = GetTime(_pObj, _pWClient, _pWClient->GetGameTick() + _pWClient->GetRenderTickFrac());
Mat = ((CWObject_Attach *)_pObj)->GetRenderMatrix(_pWClient, Time, Time);
bMat = true;
}

CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient, i);

if (pModel->GetParam(CXR_MODEL_PARAM_NEEDPATHDATA))
AnimState.m_pspClientData = &(_pObj->m_lspClientObj[1]);
else
AnimState.m_pspClientData = &(_pObj->m_lspClientObj[2]);

if(pModel && pModel->GetParam(CXR_MODEL_PARAM_TIMEMODE) == CXR_MODEL_TIMEMODE_CONTINUOUS)
{
AnimState.m_AnimTime0 = (fp32(_pWClient->GetGameTick()) + _pWClient->GetRenderTickFrac()) * SERVER_TIMEPERFRAME;
AnimState.m_Colors[3] = ~(_pObj->m_Data[3]);
AnimState.m_AnimTime1 = AnimState.m_AnimTime0;
}
else
{
AnimState.m_AnimTime0 = Time;
AnimState.m_Colors[3] = (_pObj->m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED)) != 0;
AnimState.m_AnimTime1 = (fp32(_pWClient->GetGameTick()) + _pWClient->GetRenderTickFrac()) * SERVER_TIMEPERFRAME;
}

AnimState.m_AnimAttr0 = 0;

if(_pObj->m_ClientFlags & CLIENTFLAGS_USEHINT && Time == 0)
{
CXW_Surface* pOrgSurf = _pEngine->m_pSC->GetSurface("pickupglow");
if(pOrgSurf)
{
CXW_Surface* pSurf = pOrgSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
if(pSurf)
AnimState.m_lpSurfaces[1] = pSurf;
}
}

_pEngine->Render_AddModel(pModel, Mat, AnimState);
}
}


}*/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Refreshes the active camera and counts down the
retry counter

Comments:		
\*____________________________________________________________________*/
void CWObject_ActionCutscene::OnRefresh()
{
	if (m_spActiveCutsceneCamera)
		m_spActiveCutsceneCamera->OnRefresh();

	// Only refresh actioncutscenes
	//if (IsACS())
	CWObject_ActionCutsceneParent::OnRefresh();

	CACSClientData* pCD = GetACSClientData(this);
	if ((pCD->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SETNOREFRESH) && m_SetNoRefreshTick < m_pWServer->GetGameTick())
	{
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
		pCD->m_CutsceneFlags &= ~ACTIONCUTSCENE_OPERATION_SETNOREFRESH;
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Initializes camera(s) and start/end positions

Comments:		
\*____________________________________________________________________*/
void CWObject_ActionCutscene::OnSpawnWorld()
{
	// Old camera inits
	CACSClientData* pCD = GetACSClientData(this);
	while (m_CameraCount < ACTIONCUTSCENE_MAXCAMERAS)
	{
		CStr Camera = m_lInitParams[ACS_INITSTRING_CAMERAINITPARAMS].GetStrSep("+");
		if (Camera.Len() > 0) 
		{
			if (CActionCutsceneCamera::MakeConfigBlockFromString(m_lCameraConfigs[m_CameraCount],
				Camera, m_pWServer))
				m_CameraCount++;
		}
		else
		{
			break;
		}
	}

	// New camerainit
	// Two loops controlled and controlledonchar
	int32 Mode = CActionCutsceneCamera::ACS_CAMERAMODE_CONTROLLED;
	for (int i = ACS_INITSTRING_CAMERAV2CONTROLLED; i <= ACS_INITSTRING_CAMERAV2CONTROLLEDONCHAR; i++)
	{
		CStr Params = m_lInitParams[i];
		CStr Target = Params.GetStrSep(",");
		bool bContinue = ((Target.Len() > 0) && (m_CameraCount < ACTIONCUTSCENE_MAXCAMERAS));
		while (bContinue)
		{
			int32 iTarget = m_pWServer->Selection_GetSingleTarget(Target);
			if (CActionCutsceneCamera::MakeConfigBlock(m_lCameraConfigs[m_CameraCount],
				Mode, iTarget))
				m_CameraCount++;

			Target = Params.GetStrSep(",");
			bContinue = ((Target.Len() > 0) && (m_CameraCount < ACTIONCUTSCENE_MAXCAMERAS));
		}
		Mode = CActionCutsceneCamera::ACS_CAMERAMODE_CONTROLLEDONCHAR;
	}
	if (m_lInitParams[ACS_INITSTRING_FOCUSCAMERA].Len())
	{
		CStr Params = m_lInitParams[ACS_INITSTRING_FOCUSCAMERA];
		CStr Target = Params.GetStrSep(",");
		int32 iTarget = m_pWServer->Selection_GetSingleTarget(Target);
		CActionCutsceneCamera::MakeConfigBlock(m_FocusCameraConfig,	CActionCutsceneCamera::ACS_CAMERAMODE_CONTROLLED, iTarget);
	}

	if (m_lInitParams[ACS_INITSTRING_LINKACS].Len())
	{
		m_iLinkACS = m_pWServer->Selection_GetSingleTarget(m_lInitParams[ACS_INITSTRING_LINKACS]);
		if (m_iLinkACS > 0)
		{
			pCD->SetCutsceneFlags(pCD->m_CutsceneFlags|ACTIONCUTSCENE_OPERATION_LINKACS);
		}
	}

	if(pCD->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_WAITSPAWN)
		ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;

	// Find start position
	if (m_lInitParams[ACS_INITSTRING_STARTPOSITION].Len() != 0)
	{
		if (m_lInitParams[ACS_INITSTRING_STARTPOSITION].Copy(0, 4) == "REL:")
		{
			// Position relative to actioncutscene object
			m_iStartPosition = 0;
			CStr VecStr = m_lInitParams[ACS_INITSTRING_STARTPOSITION].Right(m_lInitParams[ACS_INITSTRING_STARTPOSITION].Len() - 4);
			m_StartPositionVector.k[0] = (fp32)VecStr.Getfp64Sep(",");
			m_StartPositionVector.k[1] = (fp32)VecStr.Getfp64Sep(",");
			m_StartPositionVector.k[2] = (fp32)VecStr.Getfp64Sep(",");
			m_StartRotationVector.k[0] = (fp32)VecStr.Getfp64Sep(",") * ( 1.0f / 360.0f );
			m_StartRotationVector.k[1] = (fp32)VecStr.Getfp64Sep(",") * ( 1.0f / 360.0f );
			m_StartRotationVector.k[2] = (fp32)VecStr.Getfp64Sep(",") * ( 1.0f / 360.0f );
		} else {
			m_iStartPosition = m_pWServer->Selection_GetSingleTarget(m_lInitParams[ACS_INITSTRING_STARTPOSITION]);
		}
	}

	// Find endposition
	if (m_lInitParams[ACS_INITSTRING_ENDPOSITION].Len() != 0)
	{
		m_iEndPosition = m_pWServer->Selection_GetSingleTarget(m_lInitParams[ACS_INITSTRING_ENDPOSITION]);
	}
	else
	{
		pCD->SetCutsceneFlags(pCD->m_CutsceneFlags & ~ACTIONCUTSCENE_OPERATION_SETENDPOSMASK);
	}

	for (int32 i = 0; i < m_lMsg_TriggerSuccess.Len(); i++)
		m_lMsg_TriggerSuccess[i].SendPrecache(m_iObject, m_pWServer);
	for (int32 i = 0; i < m_lMsg_TriggerSuccessMiddle.Len(); i++)
		m_lMsg_TriggerSuccessMiddle[i].SendPrecache(m_iObject, m_pWServer);
	for (int32 i = 0; i < m_lMsg_TriggerFail.Len(); i++)
		m_lMsg_TriggerFail[i].SendPrecache(m_iObject, m_pWServer);
	for (int32 i = 0; i < m_lMsg_TriggerSuccessEnd.Len(); i++)
		m_lMsg_TriggerSuccessEnd[i].SendPrecache(m_iObject, m_pWServer);
	for (int32 i = 0; i < m_lMsg_TriggerFailMiddle.Len(); i++)
		m_lMsg_TriggerFailMiddle[i].SendPrecache(m_iObject, m_pWServer);
	for (int32 i = 0; i < m_lMsg_TriggerFailEnd.Len(); i++)
		m_lMsg_TriggerFailEnd[i].SendPrecache(m_iObject, m_pWServer);
	for (int32 i = 0; i < m_lMsg_OnEnterFocusCamera.Len(); i++)
		m_lMsg_OnEnterFocusCamera[i].SendPrecache(m_iObject, m_pWServer);
	for (int32 i = 0; i < m_lMsg_OnLeaveFocusCamera.Len(); i++)
		m_lMsg_OnLeaveFocusCamera[i].SendPrecache(m_iObject, m_pWServer);

	m_DependMessage.SendPrecache(m_iObject, m_pWServer);

	// Remove initstring data
	m_lInitParams.Clear();

	// Use and desc names
	if (m_UseName == "")
	{
		m_UseName = "§LACS_NAME_";
		m_UseName += GetTemplateName();
		m_DescName = "§LACS_DESC_";
		m_DescName += GetTemplateName();
	}

	CWObject_ActionCutsceneParent::OnSpawnWorld();
}

void CWObject_ActionCutscene::OnEnterFocusCamera(int32 _iChar)
{
	// onenter
	for (int32 i = 0; i < m_lMsg_OnEnterFocusCamera.Len(); i++)
		m_lMsg_OnEnterFocusCamera[i].SendMessage(m_iObject, _iChar, m_pWServer);
}

void CWObject_ActionCutscene::OnLeaveFocusCamera(int32 _iChar)
{
	// Onleave
	for (int32 i = 0; i < m_lMsg_OnLeaveFocusCamera.Len(); i++)
		m_lMsg_OnLeaveFocusCamera[i].SendMessage(m_iObject, _iChar, m_pWServer);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Creates a ranom action cutscene camera, or if given 
a specific camera index it will be created instead

Parameters:		
_iObject:		Object to focus on if thats what the camera 
should do
_iCharacter:	Character to focus on, if thats what the camera 
should do
_Specific:		Specific camera to use (if wanted)

Returns:		Random cutscene camera, or the specific one if
if exists

Comments:		
\*____________________________________________________________________*/
spCActionCutsceneCamera CWObject_ActionCutscene::GetActionCutsceneCamera(int _iObject, 
																		 int _iCharacter, int _Specific)
{
	int iIndex = -1;

	if ((_Specific != -1) && (_Specific < m_CameraCount))
		iIndex = _Specific;
	else if (m_CameraCount > 0)
	{
		iIndex = MRTC_RAND() % m_CameraCount;
	}

	spCActionCutsceneCamera spCamera = MNew(CActionCutsceneCamera);

	if (spCamera != NULL)
	{
		spCamera->SetServer(m_pWServer);
		spCamera->OnCreate();
		if (iIndex != -1)
		{
			spCamera->ConfigureCamera(m_lCameraConfigs[iIndex]);
		}
		else
		{
			// Ok, no camera specifies, try to create a nice one automatically
			// should also be a bit random, eg left/rightside/close/far
			// Form:    "Mode:ViewXY:ViewXY:ViewZ:DistanceOffset:HeightOffset"
			// Example: "CHARACTER:BEHIND:NONE:LEVELED:100.0:60.0"
			const int nCams = 4;
			static char* pCameraConfigs[nCams] = 
			{
				"CHARACTER:BEHIND:CAMERAVIEW_RIGHTSIDE:CAMERAVIEW_ABOVE:70.0:70.0", 
				"CHARACTER:BEHIND:CAMERAVIEW_LEFTSIDE:CAMERAVIEW_ABOVE:70.0:70.0",
				"CHARACTER:FRONT:CAMERAVIEW_RIGHTSIDE:CAMERAVIEW_ABOVE:70.0:70.0",
				"CHARACTER:FRONT:CAMERAVIEW_LEFTSIDE:CAMERAVIEW_ABOVE:70.0:70.0", 
			};
			iIndex = MRTC_RAND() % nCams;

			if (!spCamera->ConfigureFromString(CStr(pCameraConfigs[iIndex])))
				spCamera->MakeDefaultCamera();
		}

		spCamera->SetCharacterAndObject(_iCharacter, _iObject);
		/*spCamera->SetServer(m_pWServer);
		spCamera->OnCreate();
		if (iIndex != -1)
		spCamera->ConfigureCamera(m_lCameraConfigs[iIndex]);
		else
		spCamera->MakeDefaultCamera();

		spCamera->SetCharacterAndObject(_iCharacter, _iObject);*/
	}

	return spCamera;
}

bool CWObject_ActionCutscene::GetActionCutsceneCamera(int _iObject, int _iCharacter, CCameraUtil* _pCameraUtil)
{
	int iIndex = -1;

	if (m_CameraCount > 0)
	{
		iIndex = MRTC_RAND() % m_CameraCount;
	}

	if (iIndex != -1)
	{
		// Meh, make basic one for now...
		if (m_lCameraConfigs[iIndex].m_CameraMode & CActionCutsceneCamera::ACS_CAMERAMODE_MASKCONTROLLED)
		{
			int iEP = *(int*)&(m_lCameraConfigs[iIndex].m_DistanceOffset);
			CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_iCharacter);
			CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
			if (!pCD)
				return false;

			_pCameraUtil->SetACSCamera(CAMERAUTIL_MODE_ACS|CAMERAUTIL_MODE_ORIGIN_FIXEDPOSITION|CAMERAUTIL_MODE_ORIGIN_ENGINEPATH|CAMERAUTIL_MODE_USERINPUT,m_iObject, iEP, _iCharacter,pCD->m_GameTime);
			return true;
		}
	}

	// Try automatic one
	if (this->OnMessage(CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETTYPE)) == ACTIONCUTSCENE_TYPE_ACS)
		_pCameraUtil->SetAutomaticCamera(m_pWServer, m_iObject, _iCharacter);
	else
		_pCameraUtil->SetAutomaticCamera(m_pWServer, m_iObject, _iCharacter, true);

	return true;
}

bool CWObject_ActionCutscene::GetFocusCutsceneCamera(int _iObject, int _iCharacter, CCameraUtil* _pCameraUtil, bool _bIgnorePrev)
{
	// Check if a camera is already active...
	if (!_bIgnorePrev && _pCameraUtil->IsActive(CAMERAUTIL_MODE_ACS) && m_iObject == _pCameraUtil->GetExtraObj())
		return true;
	// Meh, make basic one for now...
	if (m_FocusCameraConfig.m_CameraMode & CActionCutsceneCamera::ACS_CAMERAMODE_MASKCONTROLLED)
	{
		int iEP = *(int*)&(m_FocusCameraConfig.m_DistanceOffset);
		CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_iCharacter);
		CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
		if (!pCD)
			return false;

		_pCameraUtil->SetACSCamera(CAMERAUTIL_MODE_ACS|CAMERAUTIL_MODE_ORIGIN_FIXEDPOSITION|CAMERAUTIL_MODE_ORIGIN_ENGINEPATH|CAMERAUTIL_MODE_USERINPUT,m_iObject, iEP, _iCharacter,pCD->m_GameTime);
		return true;
	}
	else
	{
		// Try automatic one
		if (this->OnMessage(CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETTYPE)) == ACTIONCUTSCENE_TYPE_ACS)
			_pCameraUtil->SetAutomaticCamera(m_pWServer, m_iObject, _iCharacter);
		else
			_pCameraUtil->SetAutomaticCamera(m_pWServer, m_iObject, _iCharacter, true);
	}

	return true;
}

uint16 CWObject_ActionCutscene::ResolveActionCutsceneType(const CStr _Value)
{
	CStr Upper = _Value.UpperCase();

	if ((Upper.Find("HATCHINVERTICALHIGH") != -1) || (Upper.Find("HTC_IN_VRT_HI") != -1))
	{
		return ACTIONCUTSCENE_TYPE_HATCHINVERTICALHIGH;
	}
	else if ((Upper.Find("HATCHOUTVERTICALHIGH") != -1) || (Upper.Find("HTC_OUT_VRT_HI") != -1))
	{
		return ACTIONCUTSCENE_TYPE_HATCHOUTVERTICALHIGH;
	}
	else if ((Upper.Find("HATCHINVERTICALLOW") != -1)  || (Upper.Find("HTC_IN_VRT_LO") != -1))
	{
		return ACTIONCUTSCENE_TYPE_HATCHINVERTICALLOW;
	}
	else if ((Upper.Find("HATCHOUTVERTICALLOW") != -1) || (Upper.Find("HTC_OUT_VRT_LO") != -1))
	{
		return ACTIONCUTSCENE_TYPE_HATCHOUTVERTICALLOW;
	}
	else if ((Upper.Find("HATCHINHORIZONTAL") != -1) || (Upper.Find("HTC_IN_HRZ") != -1))
	{
		return ACTIONCUTSCENE_TYPE_HATCHINHORIZONTAL;
	}
	else if ((Upper.Find("HATCHOUTHORIZONTAL") != -1) || (Upper.Find("HTC_OUT_HRZ") != -1))
	{
		return ACTIONCUTSCENE_TYPE_HATCHOUTHORIZONTAL;
	}
	else if ((Upper.Find("PICKUPITEMGROUND") != -1) || (Upper.Find("PICKUPGROUND") != -1))
	{
		return ACTIONCUTSCENE_TYPE_PICKUPITEMGROUND;
	}
	else if ((Upper.Find("PICKUPITEMTABLE") != -1) || (Upper.Find("PICKUPTABLE") != -1))
	{
		return ACTIONCUTSCENE_TYPE_PICKUPITEMTABLE;
	}
	else if ((Upper.Find("USEKEYPAD") != -1) || (Upper.Find("USEKEYPAD") != -1))
	{
		return ACTIONCUTSCENE_TYPE_USEKEYPAD;
	}
	else if ((Upper.Find("USEMEDSTATION") != -1) || (Upper.Find("USEMED") != -1))
	{
		return ACTIONCUTSCENE_TYPE_USEMEDSTATION;
	}
	else if ((Upper.Find("USEPOWERCELL") != -1) || (Upper.Find("USEPOWER") != -1))
	{
		return ACTIONCUTSCENE_TYPE_USEPOWERCELL;
	}
	else if ((Upper.Find("JUSTACTIVATE") != -1) || (Upper.Find("JUSTACTIVATE") != -1))
	{
		return ACTIONCUTSCENE_TYPE_JUSTACTIVATE;
	}
	else if (Upper.Find("USEBUTTON_LONG") != -1)
	{
		return ACTIONCUTSCENE_TYPE_USEBUTTON_LONG;
	}
	else if ((Upper.Find("USEBUTTON") != -1) || (Upper.Find("USEBUTTON") != -1))
	{
		return ACTIONCUTSCENE_TYPE_USEBUTTON;
	}
	else if ((Upper.Find("LEVER") != -1) || (Upper.Find("LEVER") != -1))
	{
		return ACTIONCUTSCENE_TYPE_LEVER;
	}
	else if ((Upper.Find("VALVE") != -1) || (Upper.Find("VALVE") != -1))
	{
		return ACTIONCUTSCENE_TYPE_VALVE;
	}
	else if ((Upper.Find("USEDNA") != -1) || (Upper.Find("USEDNA") != -1))
	{
		return ACTIONCUTSCENE_TYPE_USEDNA;
	}
	else if ((Upper.Find("USEEYESCANNER") != -1) || (Upper.Find("USEEYESCANNER") != -1))
	{
		return ACTIONCUTSCENE_TYPE_USEEYESCANNER;
	}
	else if (Upper.Find("USETERMINAL") != -1)
	{
		return ACTIONCUTSCENE_TYPE_USETERMINAL;
	}
	else if (Upper.Find("ENTERHEAVY") != -1)
	{
		return ACTIONCUTSCENE_TYPE_ENTERHEAVY;
	}
	else if (Upper.Find("ENTERRIOT") != -1)
	{
		return ACTIONCUTSCENE_TYPE_ENTERRIOT;
	}
	else if (Upper.Find("GETMINIFROMPOD") != -1)
	{
		return ACTIONCUTSCENE_TYPE_GETMINIFROMPOD;
	}
	else if (Upper.Find("USEHEALTHSTATION") != -1)
	{
		return ACTIONCUTSCENE_TYPE_USEHEALTHSTATION;
	}
	else if (Upper.Find("USESECURITYCARD") != -1)
	{
		return ACTIONCUTSCENE_TYPE_USESECURITYCARD;
	}
	else if (Upper.Find("RECHARGEMEDSTATION") != -1)
	{
		return ACTIONCUTSCENE_TYPE_RECHARGEMEDSTATION;
	}
	else if (Upper.Find("KICKGRATE") != -1)
	{
		return ACTIONCUTSCENE_TYPE_KICKGRATE;
	}
	else if (Upper.Find("PERISCOPE") != -1)
	{
		return ACTIONCUTSCENE_TYPE_PERISCOPE;
	}
	/*else if (Upper.Find("STEALCRYOBOX") != -1)
	{
	return ACTIONCUTSCENE_TYPE_STEALCRYOBOX;
	}*/

	return ACTIONCUTSCENE_TYPE_UNDEFINED;
}

bool CWObject_ActionCutscene::GetStartPosition(CMat4Dfp32& _StartPos, CWO_Character_ClientData* pCD, CWorld_PhysState* _pWPhysState)
{
	// Actioncutscene type in param 2
	int32 iStart = ((int32)pCD->m_ControlMode_Param2);

	switch (iStart)
	{
	case ACTIONCUTSCENE_TYPE_CROUCH_PICKUPITEMTABLE:
	case ACTIONCUTSCENE_TYPE_PICKUPITEMTABLE:
	case ACTIONCUTSCENE_TYPE_CROUCH_PICKUPITEMGROUND:
	case ACTIONCUTSCENE_TYPE_PICKUPITEMGROUND:
	case ACTIONCUTSCENE_TYPE_TELEPHONE:
		{
			return false;
		}
	default:
		break;
	}

	// Hatches not used here yet
	if (false)//(iStart >= 0) && (iStart <= ACTIONCUTSCENE_TYPE_NUMTYPES))
	{
		// Set action cutscene id to controlmode param0
		int32 Flags = pCD->m_ControlMode_Param4;
		int32 iACS = (int32)pCD->m_ControlMode_Param0;
		//if (Flags & ACTIONCUTSCENE_OPERATION_USEPERFECTPOS)
		if (true)
		{
			// Find perfect rendering position...
			CMat4Dfp32 Pos;
			if (_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETCURRENTMATRIX,aint(&Pos)),iACS))
			{
				_StartPos = Pos;
			}
			else
			{
				CWObject_CoreData* pObj = _pWPhysState->Object_GetCD(iACS);
				if (!pObj)
					return false;

				_StartPos = pObj->GetPositionMatrix();
			}
		}
		else
		{
			_StartPos = _pWPhysState->Object_GetPositionMatrix(iACS);
		}

		/*CVec3Dfp32 Test(1.0f,0.0f,0.0f);
		CVec3Dfp32 Test2;
		Test.MultiplyMatrix3x3(CAS.m_Mat);*/

		// Ok, direction of riddick is always opposite direction of the actioncutscene...?
		// So invert that and set the offset
		CVec3Dfp32 Forward;
		// Disabled for these types
		switch (iStart)
		{
		case ACTIONCUTSCENE_TYPE_CROUCH_PICKUPITEMTABLE:
		case ACTIONCUTSCENE_TYPE_PICKUPITEMTABLE:
		case ACTIONCUTSCENE_TYPE_CROUCH_PICKUPITEMGROUND:
		case ACTIONCUTSCENE_TYPE_PICKUPITEMGROUND:
			{
				Forward = CVec3Dfp32::GetMatrixRow(_StartPos,0);
				break;
			}
		default:
			{
				if (Flags & ACTIONCUTSCENE_FLAGS_HASDEFINEDDIRECTION)
				{
					// Direction set in param1
					fp32 Angle = pCD->m_ControlMode_Param1;
					Forward.k[0] = M_Cos(Angle * _PI2);
					Forward.k[1] = M_Sin(Angle * _PI2);
					Forward.k[2] = 0.0f;
					Forward.Normalize();
				}
				else
				{
					Forward  = -CVec3Dfp32::GetMatrixRow(_StartPos,0);
				}
				break;
			}
		}
		Forward.SetMatrixRow(_StartPos,0);
		CVec3Dfp32& Up = CVec3Dfp32::GetMatrixRow(_StartPos,2);
		CVec3Dfp32& Right = CVec3Dfp32::GetMatrixRow(_StartPos,1);
		Up = CVec3Dfp32(0.0f,0.0f,1.0f);
		Up.CrossProd(Forward,Right);
		Forward.CrossProd(Right,Up);
		CVec3Dfp32 Offset;
		CVec3Dfp32 Temp = CVec3Dfp32::GetMatrixRow(_StartPos,3);
		CVec3Dfp32(0.0f,0.0f,0.0f).SetMatrixRow(_StartPos,3);
		m_slStartPosition[iStart-1].MultiplyMatrix(_StartPos,Offset);
		Temp.SetMatrixRow(_StartPos,3);
		CVec3Dfp32::GetMatrixRow(_StartPos,3) += Offset;
		return true;
	}
	else
	{
		// New type (uses constants in the ag...) (better be in right state :P)
		int32 iACS = (int32)pCD->m_ControlMode_Param0;
		CWObject_CoreData* pObjACS = _pWPhysState->Object_GetCD(iACS);
		CACSClientData* pCDACS = pObjACS ? GetACSClientData(pObjACS) : NULL;
		CWObject_Message Msg(OBJMSG_HOOK_GETORGMATRIX,(aint)&_StartPos);
		if (!pCDACS || !(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_USEORGPOS) || !_pWPhysState->Phys_Message_SendToObject(Msg,iACS))
			_StartPos = _pWPhysState->Object_GetPositionMatrix(iACS);
		// Recreate mat so it's upright
		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_USEORGPOS)
		{
			_StartPos.GetRow(2) = CVec3Dfp32(0.0f,0.0f,1.0f);
			_StartPos.RecreateMatrix(2,1);
		}
		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_HASDEFINEDDIRECTION)
		{
			// Set defined direction to param1
			// Direction set in param1
			CVec3Dfp32 Forward;
			fp32 Angle = pCD->m_ControlMode_Param1;
			Forward.k[0] = -M_Cos(Angle * _PI2);
			Forward.k[1] = -M_Sin(Angle * _PI2);
			Forward.k[2] = 0.0f;
			Forward.SetMatrixRow(_StartPos,0);
			_StartPos.RecreateMatrix(0,2);
		}

		CVec3Dfp32::GetMatrixRow(_StartPos,3) += _StartPos.GetRow(0) * pCD->m_AnimGraph2.GetMaxLookAngleY() + _StartPos.GetRow(1) * pCD->m_AnimGraph2.GetAdjustmentOffset() - _StartPos.GetRow(2) * pCD->m_AnimGraph2.GetMaxLookAngleZ();
		_StartPos.GetRow(0) = -_StartPos.GetRow(0);
		_StartPos.GetRow(1) = -_StartPos.GetRow(1);
		return true;
	}

	return false;
}
bool CWObject_ActionCutscene::GetAnimPosition(CMat4Dfp32& _StartPos, CWO_Character_ClientData* pCD, CWorld_PhysState* _pWPhysState)
{
	// Actioncutscene type in param 2
	int32 iStart = ((int32)pCD->m_ControlMode_Param2);

	switch (iStart)
	{
	case ACTIONCUTSCENE_TYPE_USEHEALTHSTATION:
		{
			int32 iACS = (int32)pCD->m_ControlMode_Param0;
			CMat4Dfp32 Pos;
			if (_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETCURRENTMATRIX,aint(&Pos)),iACS))
			{
				_StartPos = Pos;
			}
			else
			{
				_StartPos = _pWPhysState->Object_GetPositionMatrix(iACS);
			}
			CVec3Dfp32 Forward;
			Forward  = -CVec3Dfp32::GetMatrixRow(_StartPos,0);
			Forward.SetMatrixRow(_StartPos,0);
			CVec3Dfp32& Up = CVec3Dfp32::GetMatrixRow(_StartPos,2);
			CVec3Dfp32& Right = CVec3Dfp32::GetMatrixRow(_StartPos,1);
			Up = CVec3Dfp32(0.0f,0.0f,1.0f);
			Up.CrossProd(Forward,Right);
			Forward.CrossProd(Right,Up);
			CVec3Dfp32 Offset;
			CVec3Dfp32 Temp = CVec3Dfp32::GetMatrixRow(_StartPos,3);
			CVec3Dfp32(0.0f,0.0f,0.0f).SetMatrixRow(_StartPos,3);
			//m_slStartPosition[iStart-1].MultiplyMatrix(_StartPos,Offset);
			Temp.SetMatrixRow(_StartPos,3);
			Offset = -Forward * 38.0f;
			CVec3Dfp32::GetMatrixRow(_StartPos,3) += Offset;

			return true;
		}
	default:
		{
			return GetStartPosition(_StartPos,pCD,_pWPhysState);
		}
	}
}

bool CWObject_ActionCutscene::GetEndPosition(CMat4Dfp32& _EndPos, CWO_Character_ClientData* pCD, CWorld_PhysState* _pWPhysState)
{
	// Actioncutscene type in param 2
	/*int32 iEnd = ((int32)pCD->m_ControlMode_Param2) - 1;
	// Hatches not used here yet
	if ((iEnd > ACTIONCUTSCENE_TYPE_HATCHOUTHORIZONTAL) && 
	(iEnd < ACTIONCUTSCENE_TYPE_NUMTYPES))
	{
	// Set action cutscene id to controlmode param0
	int32 iACS = (int32)pCD->m_ControlMode_Param0;
	_EndPos = _pWPhysState->Object_GetPositionMatrix(iACS);

	// Ok, direction of riddick is alwas opposite direction of the actioncutscene...?
	// So invert that and set the offset
	CVec3Dfp32 Forward  = CVec3Dfp32::GetMatrixRow(_EndPos,0);
	Forward.MultiplyMatrix3x3(m_slStartPosition[iEnd].m_Mat);
	Forward.SetMatrixRow(_EndPos,0);
	CVec3Dfp32& Up = CVec3Dfp32::GetMatrixRow(_EndPos,2);
	CVec3Dfp32& Right = CVec3Dfp32::GetMatrixRow(_EndPos,1);
	Up = CVec3Dfp32(0.0f,0.0f,1.0f);
	Up.CrossProd(Forward,Right);
	Forward.CrossProd(Right,Up);
	CVec3Dfp32 Offset;
	CVec3Dfp32 Temp = CVec3Dfp32::GetMatrixRow(_EndPos,3);
	CVec3Dfp32(0.0f,0.0f,0.0f).SetMatrixRow(_EndPos,3);
	CVec3Dfp32::GetMatrixRow(m_slStartPosition[iEnd].m_Mat,3).MultiplyMatrix(_EndPos,Offset);
	Temp.SetMatrixRow(_EndPos,3);
	CVec3Dfp32::GetMatrixRow(_EndPos,3) += Offset;
	return true;
	}*/

	return false;
}

void CWObject_ActionCutscene::OnDeltaSave(CCFile* _pFile)
{
	CWObject_ActionCutsceneParent::OnDeltaSave(_pFile);
	CACSClientData* pCD = GetACSClientData(this);
	_pFile->WriteLE((int8)pCD->m_bDisabled);
	_pFile->WriteLE((int32)pCD->m_CutsceneFlags);

	// Save choicestring (might be changed)
	_pFile->Writeln(pCD->m_ChoiceString);
}

void CWObject_ActionCutscene::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	CWObject_ActionCutsceneParent::OnDeltaLoad(_pFile, _Flags);
	CACSClientData* pCD = GetACSClientData(this);
	CStr TempStr;
	int8 Temp;
	int32 Temp2;
	_pFile->ReadLE(Temp);
	pCD->SetDisabled(Temp != 0);
	_pFile->ReadLE(Temp2);
	pCD->SetCutsceneFlags(Temp2);
	pCD->SetChoiceString(_pFile->Readln());
}

CWObject_ActionCutscene::CACSClientData* CWObject_ActionCutscene::GetACSClientData(CWObject_CoreData* _pObj)
{
	if(_pObj->m_lspClientObj[2] == NULL)
	{
		_pObj->m_lspClientObj[2] = MNew(CACSClientData);
		if(!_pObj->m_lspClientObj[2])
			Error_static("CWObject_Telephone::GetACSClientData", "Could not allocate ClientData.")
			CACSClientData *pData = (CACSClientData *)(CReferenceCount *)_pObj->m_lspClientObj[2];
		return pData;
	}
	else
		return (CACSClientData *)(CReferenceCount *)_pObj->m_lspClientObj[2];
}

const CWObject_ActionCutscene::CACSClientData* CWObject_ActionCutscene::GetACSClientData(const CWObject_CoreData* _pObj)
{
	return (const CACSClientData *)(const CReferenceCount *)_pObj->m_lspClientObj[2];
}

int CWObject_ActionCutscene::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const
{
	CACSClientData* pCD = const_cast<CACSClientData*>(GetACSClientData(this));
	if(!pCD)
		Error_static("CWObject_ActionCutscene::OnCreateClientUpdate", "Unable to pack client update.");

	int Flags = CWO_CLIENTUPDATE_EXTRADATA;
	if(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
		Flags = CWO_CLIENTUPDATE_AUTOVAR;
	uint8* pD = _pData;
	pD += CWObject_ActionCutsceneParent::OnCreateClientUpdate(_iClient, _pClObjInfo, _pObj, _pData, Flags);
	if (pD - _pData == 0)
		return pD - _pData;

	pD += pCD->OnCreateClientUpdate(pD);

	return (uint8*)pD - _pData;
}

int CWObject_ActionCutscene::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	MAUTOSTRIP(CWObject_ActionCutscene_OnClientUpdate, 0);
	MSCOPESHORT(CWObject_ActionCutscene::OnClientUpdate);

	/*const uint8* pD = &_pData[CWObject_ActionCutsceneParent::OnClientUpdate(_pObj, _pWClient, _pData, _Flags)];
	if (_pObj->m_iClass == 0 || pD - _pData == 0) return pD - _pData;*/

	// Copied from CWObject_Attach::OnClientUpdate
	const uint8* pD = &_pData[CWObject_Model::OnClientUpdate(_pObj, _pWClient, _pData, _Flags)];
	if (_pObj->m_iClass == 0 || pD - _pData == 0) return pD - _pData;

	if(_pObj->m_bAutoVarDirty)
	{
		GetAttachClientData(_pObj)->AutoVar_Unpack(pD, _pWClient->GetMapData());
		GetAttachClientData(_pObj)->m_CachedTime.MakeInvalid();
	}

	CACSClientData *pCD = GetACSClientData(_pObj);

	int32 FlagsBefore = pCD->m_CutsceneFlags;
	pD += pCD->OnClientUpdate(pD);

	if ((FlagsBefore ^ pCD->m_CutsceneFlags) & ACTIONCUTSCENE_FLAGS_TELEPHONEACTIVE)
		pCD->m_TelePhoneActivationTick = pCD->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_TELEPHONEACTIVE ? _pWClient->GetGameTick() : 0;

	return (uint8*)pD - _pData;
}

int CWObject_ActionCutscene::CACSClientData::OnCreateClientUpdate(uint8* _pData)
{
	// This is only done when something is changed anyway so just save the info
	uint8* pD = _pData;
	PTR_PUTUINT16(pD,m_CopyFlags);
	if (m_CopyFlags & TELEPHONE_CURRENTSELECTION_DIRTY)
	{
		uint8 X,Y;
		X = m_CurrentSelection.k[0];
		Y = m_CurrentSelection.k[1];
		PTR_PUTUINT8(pD,X);
		PTR_PUTUINT8(pD,Y);
	}
	if (m_CopyFlags & TELEPHONE_CURRENTNUMBER_DIRTY)
	{
		PTR_PUTSTR(pD,m_CurrentNumber);
	}

	if (m_CopyFlags & TELEPHONE_CHOICESTRING_DIRTY)
	{
		PTR_PUTSTR(pD,m_ChoiceString);
	}
	if (m_CopyFlags & TELEPHONE_CUTSCENEFLAGS_DIRTY)
	{
		PTR_PUTINT32(pD, m_CutsceneFlags);
	}
	if (m_CopyFlags & TELEPHONE_DISABLED_DIRTY)
	{
		PTR_PUTINT8(pD, m_bDisabled);
	}
	if (m_CopyFlags & TELEPHONE_TELEPHONEREGISTRY_DIRTY)
	{
		PTR_PUTUINT16(pD, m_iTelephoneRegistry);
	}
	if (m_CopyFlags & ACS_ACTIVATIONARC_DIRTY)
	{
		PTR_PUTFP32(pD, m_ActivationArc);
	}
	if (m_CopyFlags & ACS_CACHEDMAT_DIRTY)
	{
		PTR_PUTDATA(pD,&m_CachedSeqMat,sizeof(CMat4Dfp32));
	}
	if (m_CopyFlags & ACS_RETRYCOUNTDOWN_DIRTY)
	{
		PTR_PUTINT32(pD,m_ActionRetryCountdown);
	}

	m_CopyFlags = 0;

	return (pD - _pData);
}

int CWObject_ActionCutscene::CACSClientData::OnClientUpdate(const uint8* _pData)
{
	const uint8* pD = _pData;

	uint16 Flags;
	PTR_GETUINT16(pD,Flags);

	if (Flags & TELEPHONE_CURRENTSELECTION_DIRTY)
	{
		uint8 X,Y;
		PTR_GETUINT8(pD,X);
		PTR_GETUINT8(pD,Y);
		m_CurrentSelection.k[0] = X;
		m_CurrentSelection.k[1] = Y;
	}
	if (Flags & TELEPHONE_CURRENTNUMBER_DIRTY)
	{
		PTR_GETSTR(pD,m_CurrentNumber);
	}
	if (Flags & TELEPHONE_CHOICESTRING_DIRTY)
	{
		PTR_GETSTR(pD,m_ChoiceString);
	}
	if (Flags & TELEPHONE_CUTSCENEFLAGS_DIRTY)
	{
		PTR_GETINT32(pD, m_CutsceneFlags);
	}
	if (Flags & TELEPHONE_DISABLED_DIRTY)
	{
		PTR_GETINT8(pD, (int8&)m_bDisabled);
	}
	if (Flags & TELEPHONE_TELEPHONEREGISTRY_DIRTY)
	{
		PTR_GETUINT16(pD, m_iTelephoneRegistry);
	}
	if (Flags & ACS_ACTIVATIONARC_DIRTY)
	{
		PTR_GETFP32(pD, m_ActivationArc);
	}
	if (Flags & ACS_CACHEDMAT_DIRTY)
	{
		PTR_GETDATA(pD,&m_CachedSeqMat,sizeof(CMat4Dfp32));
	}
	if (Flags & ACS_RETRYCOUNTDOWN_DIRTY)
	{
		PTR_GETINT32(pD,m_ActionRetryCountdown);
	}

	return (pD - _pData);
}

/*int CWObject_ActionCutscene::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
// Copied from CWObject_Attach::OnClientUpdate
const uint8* pD = &_pData[CWObject_Model::OnClientUpdate(_pObj, _pWClient, _pData, _Flags)];
if (_pObj->m_iClass == 0 || pD - _pData == 0) return pD - _pData;

if(_pObj->m_bAutoVarDirty)
{
GetAttachClientData(_pObj)->AutoVar_Unpack(pD, _pWClient->GetMapData());
GetAttachClientData(_pObj)->m_CachedTime.MakeInvalid();
}

return (uint8*)pD - _pData;
}*/

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ActionCutscene, CWObject_ActionCutsceneParent, 0x0100);



//bool	m_LeverState;
void CWObject_LeverActionCutscene::OnCreate()
{
	CWObject_LeverActionCutsceneParent::OnCreate();
	m_LeverState = LEVERSTATE_UP;
}

void CWObject_LeverActionCutscene::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	if (_pKey->GetThisName().Find("LEVERSTATE") != -1)
	{
		CStr Value = _pKey->GetThisValue().UpperCase();
		if (Value.Find("DOWN") != -1)
			m_LeverState = LEVERSTATE_DOWN;
		else
			m_LeverState = LEVERSTATE_UP;
	}
	else
	{
		CWObject_LeverActionCutsceneParent::OnEvalKey(_KeyHash, _pKey);
	}
}

aint CWObject_LeverActionCutscene::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_ACTIONCUTSCENE_LEVER_SETSTATE:
		{
			if ((_Msg.m_Param0 >= LEVERSTATE_UP) && (_Msg.m_Param0 <= LEVERSTATE_DOWN) &&
				(_Msg.m_Param0 != m_LeverState))
			{
				m_LeverState = _Msg.m_Param0;

				//ConOut(CStrF("Setting %s state", (m_LeverState == LEVERSTATE_UP ? "UP" : "DOWN")));
				OnChangeState(_Msg.m_iSender);
				return true;
			}
			return false;
		}
	case OBJMSG_ACTIONCUTSCENE_LEVER_GETSTATE:
		{
			return m_LeverState;
		}
	case OBJMSG_ACTIONCUTSCENE_GETFOCUSPOS:
		{
			if (_Msg.m_pData && _Msg.m_DataSize == sizeof(CVec3Dfp32))
				*(CVec3Dfp32*)_Msg.m_pData = GetPosition();
			return 1;
		}
	default:
		return CWObject_LeverActionCutsceneParent::OnMessage(_Msg);
	}
}

void CWObject_LeverActionCutscene::OnChangeState(int32 _iTarget)
{
	CACSClientData* pCDACS = GetACSClientData(this);
	CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_iTarget);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
	if (!pCD)
		return;

	// Update to animgraph
	pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ACSLEVERSTATE,m_LeverState);
	//	int32 Mode = pCD->m_ControlMode_Param4;

	if (m_LeverState == LEVERSTATE_UP)
	{
		// Check if trigger message should be sent
		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERFAIL)
		{
			// Send the trigger message
			for (int32 i = 0; i < m_lMsg_TriggerFail.Len(); i++)
				m_lMsg_TriggerFail[i].SendMessage(m_iObject, _iTarget, m_pWServer);
		}
	}
	else if (m_LeverState == LEVERSTATE_DOWN)
	{
		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS)
		{
			// Send the trigger message
			for (int32 i = 0; i < m_lMsg_TriggerSuccess.Len(); i++)
				m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _iTarget, m_pWServer);
		}
	}
}

bool CWObject_LeverActionCutscene::DoActionSuccess(int _iCharacter)
{
	/*// Check if trigger message should be sent
	if (m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS)
	{
	// Send the trigger message
	int32 Len = m_lMsg_TriggerSuccess.Len();
	for (int32 i = 0; i < Len; i++)
	{
	if (m_lMsg_TriggerSuccess[i].IsValid())
	m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _iCharacter, m_pWServer);
	}
	}*/
	CACSClientData* pCDACS = GetACSClientData(this);

	// If the animation isn't to be played, return.. (maybe should change this)
	if (!(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION) || 
		(m_ActionCutsceneTypeSuccess == ACTIONCUTSCENE_TYPE_UNDEFINED))
	{
		return false;
	}

	// Hmm, ok we need the same functionality on the new animation system thingy
	CWObject* pObj = m_pWServer->Object_Get(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);

	if (!pCD)
		return false;

	{

		// Enter actioncutscene controlmode
		CWObject_Character::Char_SetControlMode(pObj, PLAYER_CONTROLMODE_ACTIONCUTSCENE);
		((CWObject_Character *)pObj)->UpdateVisibilityFlag();

		pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ACSLEVERSTATE,m_LeverState);

		CWAG2I_Context AG2Context(pObj,m_pWServer,pCD->m_GameTime);
		pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&AG2Context, CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,m_ActionCutsceneTypeSuccess),0);

		// Set action cutscene id to controlmode param0
		pCD->m_ControlMode_Param0 = (fp32)m_iObject;
		// Set cutscenetype to param2
		pCD->m_ControlMode_Param2 = (fp32) m_ActionCutsceneTypeSuccess;

		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_HASDEFINEDDIRECTION)
		{
			// Set defined direction to param1
			pCD->m_ControlMode_Param1 = m_DefinedDirection;
		}

		// Ok determine what type of cutscene we're having and deal with positioning accordingly
		// Only the hatch animations should be able to move around (and thus use the old positioning)
		// Filter out success actions and set to param4
		if (m_ActionCutsceneTypeSuccess > ACTIONCUTSCENE_TYPE_HATCHOUTHORIZONTAL)
		{
			int32 Flags = ((pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SUCCESSMASK) & 
				~(ACTIONCUTSCENE_OPERATION_SETSTARTPOSMASK|ACTIONCUTSCENE_OPERATION_SETENDPOSMASK)) | 
				ACTIONCUTSCENE_FLAGS_ISSUCCESS | ACTIONCUTSCENE_FLAGS_USEFLOATINGSTART;
			if (m_ActionCutsceneTypeSuccess == ACTIONCUTSCENE_TYPE_USEHEALTHSTATION)
				Flags |= ACTIONCUTSCENE_OPERATION_USEPERFECTPOS;
			pCD->m_ControlMode_Param4 = Flags;

		}
		else
		{
			pCD->m_ControlMode_Param4 = (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SUCCESSMASK) | 
				ACTIONCUTSCENE_FLAGS_ISSUCCESS;
		}

		// Make sure the camera will be updated when the ACS is active
		ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

		// Start activation timer
		if (m_MinActivationTime > 0.0f)
			m_MinActivationTimer = m_pWServer->GetGameTick() + 
			(int)(m_MinActivationTime * m_pWServer->GetGameTicksPerSecond());
		else
			m_MinActivationTimer = 0;
	}

	return true;
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_LeverActionCutscene, CWObject_LeverActionCutsceneParent, 0x0100);


void CWObject_ValveActionCutscene::OnCreate()
{
	CWObject_ValveActionCutsceneParent::OnCreate();
	//m_ValveState = 0;
	m_ValveFlags = VALVESTATE_ISRIGHT;
	//m_ValveStateActivateLeft = 0;
	//m_ValveStateActivateRight = 0;
}

void CWObject_ValveActionCutscene::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	/*CStr KeyName = _pKey->GetThisName();
	int KeyValuei = _pKey->GetThisValuei();

	if (KeyName.Find("ACTIVATELEFT") != -1)
	{
	m_ValveStateActivateLeft = -(int8)KeyValuei;
	m_ValveFlags |= VALVESTATE_CANACTIVATELEFT;
	}
	else if (KeyName.Find("ACTIVATERIGHT") != -1)
	{
	m_ValveStateActivateRight = (int8)KeyValuei;
	m_ValveFlags |= VALVESTATE_CANACTIVATERIGHT;
	}
	else*/
	{
		CWObject_ValveActionCutsceneParent::OnEvalKey(_KeyHash, _pKey);
	}
}

void CWObject_ValveActionCutscene::OnFinishEvalKeys()
{
	CWObject_ValveActionCutsceneParent::OnFinishEvalKeys();
	CACSClientData* pCD = GetACSClientData(this);

	if (pCD->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERFAIL)
		m_ValveFlags |= VALVESTATE_CANACTIVATERIGHT;
	if (pCD->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS)
		m_ValveFlags |= VALVESTATE_CANACTIVATELEFT;

	pCD->m_CutsceneFlags |= ACTIONCUTSCENE_OPERATION_USEORGPOS;
}

aint CWObject_ValveActionCutscene::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_ACTIONCUTSCENE_VALVE_ONCHANGESTATE:
		{
			switch (_Msg.m_Param0)
			{
			case VALVESTATE_CHANGELEFT:
				{
					m_ValveFlags &= ~VALVESTATE_ISRIGHT;
					//m_ValveState--;
					OnChangeState(_Msg.m_iSender);
					return true;
				}
			case VALVESTATE_CHANGERIGHT:
				{
					m_ValveFlags |= VALVESTATE_ISRIGHT;
					//m_ValveState++;
					OnChangeState(_Msg.m_iSender);
					return true;
				}
			case VALVESTATE_STARTLEFT:
				{
					// Start enginepath to the left
					m_CachedRunTime = GetTime(this,m_pWServer,m_pWServer->GetGameTick(),0.0f);
					OnImpulse(-1, _Msg.m_iSender);
					OnImpulse(1,_Msg.m_iSender);
					return true;
				}
			case VALVESTATE_STARTRIGHT:
				{
					// Start enginepath to the right
					m_CachedRunTime = GetTime(this,m_pWServer,m_pWServer->GetGameTick(),0.0f);
					OnImpulse(-1, _Msg.m_iSender);
					OnImpulse(2,_Msg.m_iSender);
					return true;
				}
			default:
				return false;
			}
		}
	case OBJMSG_ACTIONCUTSCENE_VALVE_CANTURN:
		{
			return m_ValveFlags & VALVESTATE_ISRIGHT;
		}
	default:
		return CWObject_ValveActionCutsceneParent::OnMessage(_Msg);
	}
}

void CWObject_ValveActionCutscene::OnChangeState(int32 _iTarget)
{
	CACSClientData* pCDACS = GetACSClientData(this);

	CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_iTarget);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
	if (!pCD)
		return;

	//	int32 Mode = pCD->m_ControlMode_Param4;

	if ((m_ValveFlags & VALVESTATE_CANACTIVATELEFT) && !(m_ValveFlags & VALVESTATE_ISRIGHT))
	{
		//ConOut("Valve: ACTIVATING LEFT");
		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS)
		{
			// Send the trigger message
			for (int32 i = 0; i < m_lMsg_TriggerSuccess.Len(); i++)
				m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _iTarget, m_pWServer);
		}
	}
	else if ((m_ValveFlags & VALVESTATE_CANACTIVATERIGHT) && (m_ValveFlags & VALVESTATE_ISRIGHT))
	{
		//ConOut("Valve: ACTIVATING RIGHT");
		// Check if trigger message should be sent
		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERFAIL)
		{
			// Send the trigger message
			for (int32 i = 0; i < m_lMsg_TriggerFail.Len(); i++)
				m_lMsg_TriggerFail[i].SendMessage(m_iObject, _iTarget, m_pWServer);
		}
	}
}

bool CWObject_ValveActionCutscene::DoActionSuccess(int _iCharacter)
{
	/*// Check if trigger message should be sent
	if (m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS)
	{
	// Send the trigger message
	int32 Len = m_lMsg_TriggerSuccess.Len();
	for (int32 i = 0; i < Len; i++)
	{
	if (m_lMsg_TriggerSuccess[i].IsValid())
	m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _iCharacter, m_pWServer);
	}
	}*/
	CACSClientData* pCDACS = GetACSClientData(this);

	// If the animation isn't to be played, return.. (maybe should change this)
	if (!(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION) || 
		(m_ActionCutsceneTypeSuccess == ACTIONCUTSCENE_TYPE_UNDEFINED))
	{
		return false;
	}

	// Hmm, ok we need the same functionality on the new animation system thingy
	CWObject* pObj = m_pWServer->Object_Get(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);

	if (!pCD)
		return false;

	{
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
		if (m_ActionCutsceneTypeSuccess > ACTIONCUTSCENE_TYPE_HATCHOUTHORIZONTAL)
		{
			pCD->m_ControlMode_Param4 = ((pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SUCCESSMASK) & 
				~(ACTIONCUTSCENE_OPERATION_SETSTARTPOSMASK/*|ACTIONCUTSCENE_OPERATION_SETENDPOSMASK*/)) | 
				ACTIONCUTSCENE_FLAGS_ISSUCCESS | ACTIONCUTSCENE_FLAGS_USEFLOATINGSTART;
		}
		else
		{
			pCD->m_ControlMode_Param4 = (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SUCCESSMASK) | 
				ACTIONCUTSCENE_FLAGS_ISSUCCESS;
		}

		// Make sure the camera will be updated when the ACS is active
		ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

		// Start activation timer
		if (m_MinActivationTime > 0.0f)
			m_MinActivationTimer = m_pWServer->GetGameTick() + 
			(int)(m_MinActivationTime * m_pWServer->GetGameTicksPerSecond());
		else
			m_MinActivationTimer = 0;
	}

	return true;
}
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ValveActionCutscene, CWObject_ValveActionCutsceneParent, 0x0100);

CStr CWObject_ActionCutsceneRiot::m_sRiotAcsName;
TPtr<CRPG_Object> CWObject_ActionCutsceneRiot::m_sspRPGObject;
bool CWObject_ActionCutsceneRiot::m_bNeedRespawn = 0;
int32 CWObject_ActionCutsceneRiot::m_sMaxHealth = 0;
int32 CWObject_ActionCutsceneRiot::m_sRiotHealth = 0;
int32 CWObject_ActionCutsceneRiot::m_sCharHealth = 0;
//int32 CWObject_ActionCutsceneRiot::m_siCurrentActive = -1;

void CWObject_ActionCutsceneRiot::OnCreate()
{
	CWObject_ActionCutsceneRiotParent::OnCreate();
}

void CWObject_ActionCutsceneRiot::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	CWObject_ActionCutsceneRiotParent::OnIncludeClass(_pWData,_pWServer);
	_pWData->GetResourceIndex_Sound("Itm_riotenter");
	_pWData->GetResourceIndex_Sound("Itm_riotleave");
}

void CWObject_ActionCutsceneRiot::OnFinishEvalKeys()
{
	// Set some physics
	CVec3Dfp32 Offset(0,0,32);
	CWO_PhysicsState Phys;
	Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, 
		CVec3Dfp32(22, 22, 32),Offset);
	Phys.m_nPrim = 1;
	Phys.m_ObjectFlags = OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_PICKUP;
	Phys.m_ObjectIntersectFlags = 0;//OBJECT_FLAGS_WORLD;
	Phys.m_PhysFlags = OBJECT_PHYSFLAGS_OFFSET;//OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
	Phys.m_iExclude = ~0;//_iExclude == -1 ? m_iOwner : _iExclude;
	// Well then, this doesn't work for the moment
	//m_pWServer->Object_SetPosition(m_iObject,GetPosition() + Offset);
	m_pWServer->Object_SetPhysics(m_iObject, Phys);

	CWObject_ActionCutsceneRiotParent::OnFinishEvalKeys();
}

bool CWObject_ActionCutsceneRiot::DoActionSuccess(int _iCharacter)
{
	// Send impulse 1 to the base class when successful
	CACSClientData* pCDACS = GetACSClientData(this);
	OnImpulse(1, _iCharacter);

	// If the animation isn't to be played, return.. (maybe should change this)
	if (!(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION) || 
		(m_ActionCutsceneTypeSuccess == ACTIONCUTSCENE_TYPE_UNDEFINED))
	{
		pCDACS->SetRetryCountDown(m_pWServer->GetGameTick() + m_ActionRetryCountdownValue);
		return false;
	}

	// Hmm, ok we need the same functionality on the new animation system thingy
	CWObject* pObj = m_pWServer->Object_Get(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);

	if (!pCD)
		return false;

	if (m_ActionCutsceneTypeSuccess == ACTIONCUTSCENE_TYPE_ENTERRIOT)
	{
		CMat4Dfp32 Pos = GetPositionMatrix();
		CVec3Dfp32::GetMatrixRow(Pos,3).k[2] += 0.01f;
		// Make invisible
		ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;
		// Blech, this object will be in the way, so set physics before
		CWO_PhysicsState Phys;
		Phys.m_nPrim = 0;
		Phys.m_ObjectFlags = OBJECT_FLAGS_PICKUP;
		Phys.m_ObjectIntersectFlags = 0;//OBJECT_FLAGS_WORLD;
		Phys.m_PhysFlags = 0;//OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
		Phys.m_iExclude = ~0;//_iExclude == -1 ? m_iOwner : _iExclude;
		// Well then, this doesn't work for the moment
		//m_pWServer->Object_SetPosition(m_iObject,GetPosition() + Offset);
		m_pWServer->Object_SetPhysics(m_iObject, Phys);

		m_pWServer->Object_SetPosition(_iCharacter, Pos);
		pCDACS->SetRetryCountDown(m_pWServer->GetGameTick() + m_ActionRetryCountdownValue);
		// Send the success trigger messages 
		int32 iRespawned = -1;
		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS)
		{
			// Send the trigger message
			for (int32 i = 0; i < m_lMsg_TriggerSuccess.Len(); i++)
			{
				int32 Res = m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _iCharacter, m_pWServer);
				if (i == 0)
					iRespawned = Res;
			}
		}

		// If the acs should be disabled, just remove it
		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_DISABLEONSUCCESS)
		{
			//m_pWServer->Object_Destroy(m_iObject);
			pCDACS->SetDisabled(true);
		}

		// Must save character name and inventory in here....?
		CWObject_Character* pObj = safe_cast<CWObject_Character>(m_pWServer->Object_Get(_iCharacter));
		if (pObj)
			m_sspRPGObject = pObj->Char_GetRpg();
		m_sRiotAcsName = GetTemplateName();
		m_sMaxHealth = pObj->Char()->MaxHealth();
		m_sCharHealth = pObj->Char()->Health();

		// Set riot health..
		if (iRespawned != -1 && m_sRiotHealth > 0)
		{
			//ConOut(CStrF("Setting health: %d iObj: %d cTick: %d sTick: %d",m_RiotHealth,iRespawned,pCD->m_GameTick,m_pWServer->GetGameTick()));
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETHEALTH,m_sRiotHealth),iRespawned);
		}

		int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("Itm_riotenter");
		m_pWServer->Sound_At(CVec3Dfp32::GetRow(Pos,3)+CVec3Dfp32(0.0f,0.0f,50.0f),iSound, WCLIENT_ATTENUATION_3D);

		// Delete ourselves
		m_pWServer->Object_Destroy(m_iObject);
	}

	return true;
}

int32 CWObject_ActionCutsceneRiot::RespawnCharacter(CWorld_Server* _pWServer, int32 _iChar)
{
	// Attempt to spawn a riotguard acs and exit the riotguard
	// Make riot visible again and set to current position
	if (m_sRiotAcsName.Len() <= 0)
		return false;

	// Respawn character
	CWObject_CoreData* pRiot = _pWServer->Object_GetCD(_iChar);
	CWO_Character_ClientData* pCDRiot = (pRiot ? CWObject_Character::GetClientData(pRiot) : NULL);
	if (pCDRiot)
	{
		// Make sure there is room infront of the vehicle
		fp32 HalfWidth = 14;
		fp32 HalfHeight = 32;
		fp32 LengthR = M_Sqrt(Sqr((fp32)pCDRiot->m_Phys_Width+5) * 2.0f);
		fp32 LengthC = M_Sqrt(HalfWidth * HalfWidth * 2.0f);

		CMat4Dfp32 Pos = _pWServer->Object_GetPositionMatrix(_iChar);
		CVec3Dfp32 Dir = CVec3Dfp32::GetMatrixRow(Pos,0);
		Dir.k[2] = 0.0f;
		Dir.Normalize();
		CVec3Dfp32 TargetPos = CVec3Dfp32::GetMatrixRow(Pos,3) + Dir * (LengthR + LengthC);
		TargetPos.k[2] += /*HalfHeight +*/ 0.01f;
		CVec3Dfp32 Offset(0.0f,0.0f,HalfHeight);

		// Make sure there is not a door in the way (do traceline)
		{
			int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
			int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;// | OBJECT_FLAGS_PLAYERPHYSMODEL;
			int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
			int32 iExclude = _iChar;
			CVec3Dfp32 Origin = pRiot->GetPosition();
			Origin.k[2] += 30.0f;
			bool bHit = _pWServer->Phys_IntersectLine(Origin, TargetPos, OwnFlags, ObjectFlags, MediumFlags, NULL, iExclude);
			if (bHit)
				return false;
		}


		// Make intersection test at targetpos
		CWO_PhysicsState PhysState;
		/*	PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_BOX, 0, 
		CVec3Dfp32(_Width*0.5f, _Width*0.5f, LEDGETYPE_HEIGHT_MEDIUM*0.5f), 0, 1.0f));*/
		PhysState.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, 
			CVec3Dfp32(HalfWidth, HalfWidth, HalfHeight),Offset);
		PhysState.m_nPrim = 1;
		PhysState.m_ObjectFlags = OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_WORLDTELEPORT | OBJECT_FLAGS_PHYSOBJECT;
		PhysState.m_MediumFlags = XW_MEDIUM_SOLID|XW_MEDIUM_PHYSSOLID | XW_MEDIUM_PLAYERSOLID | XW_MEDIUM_DYNAMICSSOLID;
		PhysState.m_PhysFlags = OBJECT_PHYSFLAGS_SLIDEABLE | OBJECT_PHYSFLAGS_PHYSMOVEMENT |
			OBJECT_PHYSFLAGS_OFFSET | OBJECT_PHYSFLAGS_PUSHABLE;
		// Intersect doors and doors/platforms
		PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_PLAYER | OBJECT_FLAGS_PLAYERPHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_PROJECTILE | OBJECT_FLAGS_ANIMPHYS;
		PhysState.m_iExclude = _iChar;

		TSelection<CSelection::LARGE_BUFFER> Selection;
		_pWServer->Selection_AddIntersection(Selection, TargetPos, PhysState);

		const int16* pSel;
		int nSel = _pWServer->Selection_Get(Selection, &pSel);

		bool bResult = (nSel == 0);

#ifndef M_RTM
		CBox3Dfp32 SomeBox;
		SomeBox.m_Min = TargetPos - CVec3Dfp32(HalfWidth, HalfWidth, HalfHeight) + Offset;
		SomeBox.m_Max = TargetPos + CVec3Dfp32(HalfWidth, HalfWidth, HalfHeight) + Offset;
		int32 Color = (bResult ? 0xff00ff00 : 0xffff0000);
		_pWServer->Debug_RenderAABB(SomeBox,Color,10.0f,false);
#endif

		if (!bResult)
			return false;

		int32 iRiot = _pWServer->Object_Create(m_sRiotAcsName);
		CWObject* pObj = _pWServer->Object_Get(iRiot);
		if (pObj)
		{
			pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
			// Set riot health..
			m_sRiotHealth = _pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETHEALTH),pRiot->m_iObject);
			//_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_RIOTACS_SETHEALTH,Health),iRiot);
		}

		CWO_PhysicsState Phys;
		Phys.m_nPrim = 0;
		Phys.m_ObjectFlags = OBJECT_FLAGS_PICKUP;
		Phys.m_ObjectIntersectFlags = 0;//OBJECT_FLAGS_WORLD;
		Phys.m_PhysFlags = 0;//OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
		Phys.m_iExclude = ~0;//_iExclude == -1 ? m_iOwner : _iExclude;
		// Well then, this doesn't work for the moment
		//m_pWServer->Object_SetPosition(m_iObject,GetPosition() + Offset);
		_pWServer->Object_SetPhysics(iRiot, Phys);

		CVec3Dfp32::GetMatrixRow(Pos,3).k[2] += 0.01f;

		Dir.SetMatrixRow(Pos,0);
		Pos.RecreateMatrix(0,1);
		_pWServer->Object_SetPosition(iRiot,Pos);
		//_pWServer->Phys_InsertPosition(m_siCurrentActive, pObj);
		//_pWServer->Object_SetDirty(m_siCurrentActive, CWO_DIRTYMASK_COREMASK);
		// GET DEFAULT SPAWN CLASS INSTEAD....
		//_pWServer->GetDefualtSpawnClass();
		/*		CStr ClassName;
		_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAMECORE_GETDEFAULTSPAWNCLASS,0,0,0,0,0,0,&ClassName),_pWServer->Game_GetObjectIndex());*/

		int32 iObj = _pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_RESPAWN,0,0,0,0,0,0,NULL/*ClassName.GetStr()*/),_iChar);

		if (iObj > 0)	
		{
			CWObject_Character* pChar = safe_cast<CWObject_Character>(_pWServer->Object_Get(iObj));
			if (!pChar)
				return false;

			pChar->Char()->MaxHealth() = m_sMaxHealth;
			if (m_sCharHealth > 0)
				pChar->Char()->Health() = m_sCharHealth;

			// Play sound
			int iSound = _pWServer->GetMapData()->GetResourceIndex_Sound("Itm_riotleave");
			_pWServer->Sound_At(TargetPos+CVec3Dfp32(0.0f,0.0f,50.0f),iSound,0);

			// Set rpg to the new object
			//			pChar->Char_SetRpg(m_sspRPGObject);
			CRPG_Object_Char* pRPGObj = (CRPG_Object_Char*)(CRPG_Object *)m_sspRPGObject;
			CRPG_Object_Char* pRPGObjChar = (CRPG_Object_Char*)(CRPG_Object *)pChar->Char_GetRpg();
			if (!pRPGObj || !pRPGObjChar)
				return false;
			for(int32 i = 0; i < RPG_CHAR_NUMCHILDREN; i++)
			{
				//
				// Clear what's in this inventory first
				//
				uint32 iItem;
				uint32 nItems;
				nItems = pRPGObjChar->GetInventory(i)->GetNumItems();
				for (iItem = 0; iItem < nItems; iItem++)
					pRPGObjChar->GetInventory(i)->RemoveItemByIndex(0, 0);

				nItems = pRPGObj->GetInventory(i)->GetNumItems();

				for(int32 j = 0; j < nItems; j++)
				{
					//VERBOSE_OUT(CStrF("Inventory item %i on player is a %s.", j, ItemName.GetStr()));
					pRPGObjChar->GetInventory(i)->AddItem(pChar->m_iObject,pRPGObj->GetInventory(i)->GetItemByIndex(j));
				}
			}
			m_sspRPGObject = NULL;
			// Update quest items
			pChar->Char_UpdateQuestItems();

			// Force equip fists
			pChar->Char_SelectItemByType(0);
			pChar->SetPosition(TargetPos);

			// Reset physics on riot acs
			PhysState.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, 
				CVec3Dfp32(22.0f, 22.0f, 32.0f),CVec3Dfp32(0.0f,0.0f,32.0f));
			PhysState.m_nPrim = 1;
			PhysState.m_ObjectFlags = OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_PICKUP;
			PhysState.m_ObjectIntersectFlags = 0;//OBJECT_FLAGS_WORLD;
			PhysState.m_PhysFlags = OBJECT_PHYSFLAGS_OFFSET;//OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
			PhysState.m_iExclude = ~0;//_iExclude == -1 ? m_iOwner : _iExclude;
			// Well then, this doesn't work for the moment
			//m_pWServer->Object_SetPosition(m_iObject,GetPosition() + Offset);
			_pWServer->Object_SetPhysics(iRiot, PhysState);

			m_sspRPGObject = NULL;
			m_sRiotAcsName.Clear();

			return iObj;
		}
	}

	return false;
}

void CWObject_ActionCutsceneRiot::CharSave(CWorld_Server* _pWServer, CCFile* _pFile, int32 _iChar, CWO_Character_ClientData* _pCD)
{
	int8 Status = 0;//(_pCD ? _pCD->m_Aim_SkeletonType == SKELETONTYPE_RIOTGUARD : 0);//(m_siCurrentActive != -1 && m_siCurrentActive == _iChar);
	// Save status
	_pFile->WriteLE(Status);

	// Save the character type to respawn?
	_pFile->Writeln(m_sRiotAcsName);
	_pFile->WriteLE(m_sMaxHealth);
	_pFile->WriteLE(m_sCharHealth);

	// Get riot health..
	if (Status)
		m_sRiotHealth = _pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETHEALTH),_iChar);
	_pFile->WriteLE(m_sRiotHealth);

	if (!Status)
		return;

	// Ok, save down the stuffo... (inventory and name)
	//
	// Save entire contents of inventory
	//
	//VERBOSE_OUT(CStr("Saving inventory on player objekt."));
	CRPG_Object_Char* pRPGObj = (CRPG_Object_Char*)(CRPG_Object *)m_sspRPGObject;
	if (!pRPGObj)
		return;
	for(int32 i = 0; i < RPG_CHAR_NUMCHILDREN; i++)
	{
		uint32 nItems = pRPGObj->GetNumItems(i);
		_pFile->WriteLE(&nItems, 1);
		for(int32 j = 0; j < nItems; j++)
		{
			CRPG_Object_Item *pItem = pRPGObj->GetInventory(i)->GetItemByIndex(j);
			if (pItem != NULL)
			{
				CFStr ItemName = pItem->GetDesc(0);
				//VERBOSE_OUT(CStrF("Inventory item %i on player is a %s.", j, ItemName.GetStr()));
				_pFile->Writeln(ItemName.GetStr());
				pItem->OnDeltaSave(_pFile, pRPGObj);
				{
					CWObject_Message Msg(OBJMSG_GAMEP4_SAVEPRECACHEINFO, PRECACHEINFO_RPG);
					Msg.m_pData = ItemName.GetStr();
					_pWServer->Message_SendToObject(Msg, _pWServer->Game_GetObjectIndex());
				}
			}
		}
	}
}

void CWObject_ActionCutsceneRiot::CharLoad(CWorld_Server* _pWServer, CCFile* _pFile, int32 _iChar)
{
	// Get Status
	int8 Status = 0;
	_pFile->ReadLE(Status);

	// Read character name
	m_sRiotAcsName = _pFile->Readln();
	_pFile->ReadLE(m_sMaxHealth);
	_pFile->ReadLE(m_sCharHealth);
	_pFile->ReadLE(m_sRiotHealth);

	if (!Status)
		return;

	//
	// Read inventory
	//
	// Create rpg object
	CVec3Dfp32 Pos = _pWServer->Object_GetPosition(_iChar);
	m_sspRPGObject = (CRPG_Object *)CRPG_Object::CreateObject("Char", _pWServer);
	CRPG_Object_Char* pRPG = (CRPG_Object_Char *)(CRPG_Object *)m_sspRPGObject;
	if (!pRPG)
		return;
	for(int32 i = 0; i < RPG_CHAR_NUMCHILDREN; i++)
	{
		//
		// Clear what's in this inventory first
		//
		uint32 iItem;
		uint32 nItems;
		nItems = pRPG->GetInventory(i)->GetNumItems();
		for (iItem=0; iItem<nItems; iItem++)
			pRPG->GetInventory(i)->RemoveItemByIndex(0, 0);

		_pFile->ReadLE(&nItems, 1);
		for(int32 j = 0; j < nItems; j++)
		{
			CStr ItemName = _pFile->Readln();
			//VERBOSE_OUT(CStrF("Inventory item %i on player is a %s.", j, ItemName.GetStr()));
			pRPG->PickupItem(ItemName, 1, 0, 0, _iChar, 1, _iChar);

			CRPG_Object_Item* pItem = pRPG->GetInventory(i)->GetItemByIndex(j);
			M_ASSERT(pItem != NULL, "God damn, son of a bitch!");
			if (pItem)	// If this fails we're fucked
				pItem->OnDeltaLoad(_pFile, pRPG);
		}
	}

	// Skip this and create from template instead? (save that name too...)
	//m_siCurrentActive = m_iObject;
	m_bNeedRespawn = true;

	/*CWObject_Message Msg(OBJMSG_CHAR_RESPAWN,0,0,0,0,0,0,"player_riddick_riotguard");
	_pWServer->Message_SendToObject(Msg,_iChar);
	Pos.k[2] += 0.01f;
	_pWServer->Object_SetPosition(_iChar,Pos);*/
}

void CWObject_ActionCutsceneRiot::CheckRespawn(CWorld_Server* _pWServer, int32 _iChar)
{
	if (!m_bNeedRespawn)
		return;

	ConOut(CStr("Needed respawn, doing it now"));
	m_bNeedRespawn = false;
	CWObject_Message Msg(OBJMSG_CHAR_RESPAWN,0,0,0,0,0,0,(void*)"player_riddick_riotguard");
	int32 iRespawned = _pWServer->Message_SendToObject(Msg,_iChar);
	// Set riot health..
	if (iRespawned != -1 && m_sRiotHealth > 0)
	{
		//ConOut(CStrF("Setting health: %d iObj: %d cTick: %d sTick: %d",m_RiotHealth,iRespawned,pCD->m_GameTick,m_pWServer->GetGameTick()));
		_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETHEALTH,m_sRiotHealth),iRespawned);
	}
	//Pos.k[2] += 0.01f;
	//_pWServer->Object_SetPosition(_iChar,Pos);
}

aint CWObject_ActionCutsceneRiot::OnMessage(const CWObject_Message& _Msg)
{
	if (_Msg.m_Msg == OBJMSG_ACTIONCUTSCENE_GETTYPE)
	{
		return ACTIONCUTSCENE_TYPE_RIOT;
	}
	/*else if (_Msg.m_Msg == OBJMSG_ACTIONCUTSCENE_RIOTACS_SETHEALTH)
	{
	m_sRiotHealth = _Msg.m_Param0;
	return 1;
	}*/
	else
	{
		return CWObject_ActionCutsceneRiotParent::OnMessage(_Msg);
	}
}

void CWObject_ActionCutsceneRiot::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{

	MAUTOSTRIP(CWObject_ActionCutsceneRiot_OnClientRender, MAUTOSTRIP_VOID);
	if((_pObj->m_ClientFlags & CLIENTFLAGS_INVISWHENSTOPPED) && !(_pObj->m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED)))
		return;
	if (_pObj->m_ClientFlags & CLIENTFLAGS_DEACTIVATED)
		return;

	CMat4Dfp32 Mat = _pObj->GetPositionMatrix();

	CMTime Time;
	CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if(pModel)
	{
		CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient, 0);


		if(pModel && pModel->GetParam(CXR_MODEL_PARAM_TIMEMODE) == CXR_MODEL_TIMEMODE_CONTINUOUS)
		{
			AnimState.m_AnimTime0 = _pWClient->GetRenderTime();
			AnimState.m_Data[3] = ~(_pObj->m_Data[3]);
			AnimState.m_AnimTime1 = AnimState.m_AnimTime0;
		}
		else
		{
			AnimState.m_AnimTime0 = Time;
			AnimState.m_Data[3] = (_pObj->m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED)) != 0;
			AnimState.m_AnimTime1 = _pWClient->GetRenderTime();
		}

		AnimState.m_AnimAttr0 = 0;

		uint32 RenderFlags = 0;
		if (_pObj->m_ClientFlags & CLIENTFLAGS_USEHINT)// && Time.Compare(0) == 0)
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

		_pEngine->Render_AddModel(pModel, Mat, AnimState, XR_MODEL_STANDARD, RenderFlags);
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ActionCutsceneRiot, CWObject_ActionCutsceneRiotParent, 0x0100);

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
int32 CWObject_DCActioncutscene::m_sVolumeLock = 0;
void CWObject_DCActioncutscene::OnCreate()
{
	CWObject_DCActioncutsceneParent::OnCreate();
	m_StartTick = 0;
	m_PauseStartTick = 0;
	m_iActivator = 0;
	m_iSyncedEntity = 0;
	m_PlayBackMode = DCACS_PLAYBACKMODE_NORMAL;

	// Reset static var between maps and load
	m_sVolumeLock = 0;
}

void CWObject_DCActioncutscene::OnSpawnWorld()
{
	// If we're not in commentary mode, destroy ourselves
	if (!m_pWServer->World_TestSpawnFlags( M_Bit(DIFFICULTYLEVEL_COMMENTARY-1) ))
	{
		m_pWServer->Object_Destroy(m_iObject);
		return;
	}

	CWObject_DCActioncutsceneParent::OnSpawnWorld();

	// Ok, sounds should be precached now, try and find it and check length of it
	//	int32 Len = m_lTimedMessages.Len();
	fp32 Duration = 0.0f;
	int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(m_Sound.GetStr());
	CSC_SFXDesc* pSound = m_pWServer->GetMapData()->GetResource_SoundDesc(iSound);
	if (pSound)
	{
		MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
		if(pWC)
		{
			Duration = pWC->SFX_GetLength(pSound, 0);
		}
	}

	// Mmmkay then, add how long the audio track is (if playing audio...)
	if (Duration > 0.0f)
	{
		if (m_UseName.Len() > 0)
			m_UseName += CStrF("§p0(%d:%.2d)§pq", (int)M_Floor(Duration / 60), int(Duration) % 60);

		// Set repeat delay to duration
		m_ActionRetryCountdownValue = (int)((Duration + 0.5f) * m_pWServer->GetGameTicksPerSecond());
	}

	// Get synced entity
	m_iSyncedEntity = m_pWServer->Selection_GetSingleTarget(m_SyncedEntity);
}

void CWObject_DCActioncutscene::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	if (_pKey->GetThisName().CompareSubStr("TIMEDSOUND") == 0)
	{
		m_Sound = _pKey->GetThisValue();
		m_Sound.GetStrMSep(";, ").Val_fp64();
		m_Sound.Trim();
	}
	else if (_pKey->GetThisName().CompareSubStr("SYNCEDENTITY") == 0)
	{
		m_SyncedEntity = _pKey->GetThisValue();
	}
	else
	{
		CWObject_DCActioncutsceneParent::OnEvalKey(_KeyHash, _pKey);
	}
}
#define COMMENTARY_VOLUMEMULTIPLIER 0.1f
#define COMMENTARY_VOLUME			1/COMMENTARY_VOLUMEMULTIPLIER
aint CWObject_DCActioncutscene::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_ACTIONCUTSCENE_CANACTIVATE:
		{
			if (_Msg.m_Param0 == 2)
				return 0;
			else
			{
				if (_Msg.m_iSender != -1)
					return CanActivate(_Msg.m_iSender);
			}
			return 0;
		}
	case OBJMSG_IMPULSE:
		{
			if (_Msg.m_Param0 == 5)
			{
				// Play 2d sound that can't be interrupted
				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(m_Sound.GetStr());
				///m_pWServer->Sound_At(GetPosition(), iSound, 0);
				m_pWServer->Sound_Global(iSound, COMMENTARY_VOLUME);
				m_PlayBackMode = DCACS_PLAYBACKMODE_FORCED;
				RaiseVolume();
				m_StartTick = m_pWServer->GetGameTick();
				m_PauseStartTick = 0;
				//return CWObject_DCActioncutsceneParent::DoActionSuccess(_iCharacter);
				CACSClientData* pCDACS = GetACSClientData(this);
				pCDACS->SetRetryCountDown(m_pWServer->GetGameTick() + m_ActionRetryCountdownValue);
				ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
			}
		}
	default:
		return CWObject_DCActioncutsceneParent::OnMessage(_Msg);
	}
}

void CWObject_DCActioncutscene::Pause()
{
	// Pause
	const int32 GameTick = m_pWServer->GetGameTick();
	Data(DCACS_CONTROLPOS) = DCACS_CONTROL_PAUSE;
	m_PauseStartTick = GameTick;
	Data(DCACS_PAUSEPOS) = m_PauseStartTick+1;
	// Restore volume
	NormalizeVolume();

	// Send pause message to synced entity
	if (m_iSyncedEntity > 0)
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE,0), m_iSyncedEntity);
}

void CWObject_DCActioncutscene::Continue()
{
	const int32 GameTick = m_pWServer->GetGameTick();
	CACSClientData* pCDACS = GetACSClientData(this);
	pCDACS->SetRetryCountDown(pCDACS->m_ActionRetryCountdown + GameTick - m_PauseStartTick + 1);
	Data(DCACS_CONTROLPOS) = DCACS_CONTROL_CONTINUE;
	Data(DCACS_ROTATEPOS) = GameTick+1 + Data(DCACS_ROTATEPOS) - Data(DCACS_PAUSEPOS);
	Data(DCACS_PAUSEPOS) = GameTick+1;
	RaiseVolume();

	// Send start message to synced entity
	if (m_iSyncedEntity > 0)
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE,1), m_iSyncedEntity);
}

void CWObject_DCActioncutscene::NormalizeVolume()
{
	if (m_sVolumeLock <= 1)
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_SETVOLUMEMULTIPLIER,0,0,0,0,1.0f), m_pWServer->Game_GetObjectIndex());

	if (m_sVolumeLock > 0)
		m_sVolumeLock--;
}

void CWObject_DCActioncutscene::RaiseVolume()
{
	if (!m_sVolumeLock)
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_SETVOLUMEMULTIPLIER,0,0,0,0,COMMENTARY_VOLUMEMULTIPLIER), m_pWServer->Game_GetObjectIndex());

	m_sVolumeLock++;
}

bool CWObject_DCActioncutscene::DoAction(int _iCharacter)
{
	// Check if the acs is playing, if so pause it
	CACSClientData* pCDACS = GetACSClientData(this);
	const int32 GameTick = m_pWServer->GetGameTick();
	if (Data(DCACS_CONTROLPOS) != DCACS_CONTROL_PAUSE && (GameTick  > m_StartTick) && 
		(GameTick < pCDACS->m_ActionRetryCountdown))
	{
		Pause();
		return 1;
	}

	if ((Data(DCACS_CONTROLPOS) == DCACS_CONTROL_PAUSE) && (m_PauseStartTick > 0))
	{
		// Unpause
		Continue();
		return 1;
	}

	// Reset AG CRAP MODE
	m_AGCrapMode = 0;
	if ((GameTick < pCDACS->m_ActionRetryCountdown) || pCDACS->m_bDisabled || 
		(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_WAITSPAWN))
	{
		return false;
	}

	// Reset engine_path
	Reset();
	// Deactivate item
	m_pWServer->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DEACTIVATEITEM, AG2_ITEMSLOT_WEAPONS,1), _iCharacter);

	CWObject* pObj = m_pWServer->Object_Get(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);

	if (!pCD)
		return false;
	// Save actioncutscene position
	pCD->m_DialoguePlayerPos = pObj->GetPositionMatrix();

	if ((pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_DEPENDSONITEM) || 
		(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_LOCKED) ||
		m_DependMessage.IsValid())
	{
		// Check if the character has the item this action depends on... FIXME
		bool bDoSuccess = false;

		if (!(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_LOCKED))
		{
			bDoSuccess = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETITEMFROMSTRING,0,0,
				m_iObject,0,0,0, &m_DependItem, sizeof(CStr*)), _iCharacter) != NULL;
		}

		if(m_DependMessage.IsValid())
			bDoSuccess = m_DependMessage.SendMessage(m_iObject, _iCharacter, m_pWServer) != 0;

		return (bDoSuccess ? DoActionSuccess(_iCharacter) : DoActionFail(_iCharacter));
	}
	else
	{
		// No restrictions, just do the successaction
		return DoActionSuccess(_iCharacter);
	}
}

bool CWObject_DCActioncutscene::DoActionSuccess(int _iCharacter)
{
	int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(m_Sound.GetStr());
	m_iActivator = _iCharacter;
	// Set volume
	RaiseVolume();
	//return CWObject_DCActioncutsceneParent::DoActionSuccess(_iCharacter);
	// Set sound id to objectdata
	Data(DCACS_DATAPOS) = iSound;
	Data(DCACS_CONTROLPOS) = DCACS_CONTROL_START;
	m_StartTick = m_pWServer->GetGameTick();
	Data(DCACS_ROTATEPOS) = m_StartTick;
	m_PauseStartTick = 0;
	//return CWObject_DCActioncutsceneParent::DoActionSuccess(_iCharacter);
	CACSClientData* pCDACS = GetACSClientData(this);
	pCDACS->SetRetryCountDown(m_pWServer->GetGameTick() + m_ActionRetryCountdownValue);
	ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
	m_PlayBackMode = DCACS_PLAYBACKMODE_NORMAL;

	// Send success messages
	for (int32 i = 0; i < m_lMsg_TriggerSuccess.Len(); i++)
		m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _iCharacter, m_pWServer);

	// Send start message to synced entity
	if (m_iSyncedEntity > 0)
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE,1), m_iSyncedEntity);

	return 1;
}

bool CWObject_DCActioncutscene::OnEndACS(int _iCharacter)
{
	m_iActivator = 0;
	Data(DCACS_CONTROLPOS) = 0;
	Data(DCACS_DATAPOS) = 0;
	Data(DCACS_PAUSEPOS) = 0;
	m_PlayBackMode = 0;
	// Restore volume
	NormalizeVolume();

	return true;
}

void CWObject_DCActioncutscene::OnRefresh()
{
	CWObject_DCActioncutsceneParent::OnRefresh();
	if (m_sVolumeLock > 0)
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_SETVOLUMEMULTIPLIER,0,0,0,0,COMMENTARY_VOLUMEMULTIPLIER), m_pWServer->Game_GetObjectIndex());
	CACSClientData* pCDACS = GetACSClientData(this);
	if (m_PlayBackMode == DCACS_PLAYBACKMODE_FORCED)
	{
		// At time when reactivation is possible again, turn off refresh
		if (m_pWServer->GetGameTick() > pCDACS->m_ActionRetryCountdown)
		{
			ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
			OnEndACS(m_iActivator);
		}
	}
	else
	{
		if (m_Data[DCACS_DATAPOS] != 0 && m_pWServer->GetGameTick() > m_StartTick)
		{
			// Reset sound to be played, so it won't be played again when the sound runs out
			Data(DCACS_DATAPOS) = 0;
		}

		// At time when reactivation is possible again, turn off refresh
		if (m_pWServer->GetGameTick() > pCDACS->m_ActionRetryCountdown)
		{
			ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
			OnEndACS(m_iActivator);
		}

		// If character is out of range, pause the sound
		if (m_iActivator != 0 && Data(DCACS_CONTROLPOS) != DCACS_CONTROL_PAUSE)
		{
			CVec3Dfp32 Position = m_pWServer->Object_GetPosition(m_iActivator);
			if ((GetPosition() - Position).LengthSqr() > 300*300)
				Pause();
		}
	}
}

void CWObject_DCActioncutscene::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_DCActioncutscene::OnClientRefresh);
	CWObject_DCActioncutsceneParent::OnClientRefresh(_pObj,_pWClient);
	if (_pObj->m_Data[DCACS_DATAPOS] > 0 && (_pObj->m_ClientData[0] == 0))
	{
		// Start sound
		int32 iSoundHandle = _pWClient->Sound_At(WCLIENT_CHANNEL_SFX, _pObj->GetPosition(), _pObj->m_Data[DCACS_DATAPOS], 0,0,COMMENTARY_VOLUME);	
		_pObj->m_ClientData[0] = iSoundHandle;
	}
	else
	{
		// Do some checkup, if the sound hass played out lets just zero our handle
		// If character is out of range, pause sound
		MACRO_GetRegisterObject(CSoundContext, pSound, "SYSTEM.SOUND");
		if (pSound && _pObj->m_ClientData[0])
		{
			if (_pObj->m_Data[DCACS_CONTROLPOS] == DCACS_CONTROL_PAUSE && 
				pSound->Voice_IsPlaying(_pObj->m_ClientData[0]) /*&&
																_pObj->m_Data[DCACS_PAUSEPOS] == _pWClient->GetGameTick()*/)
			{
				// Pause sound
				pSound->Voice_Pause(_pObj->m_ClientData[0]);
			}

			if (_pObj->m_Data[DCACS_CONTROLPOS] == DCACS_CONTROL_CONTINUE && 
				pSound->Voice_IsPlaying(_pObj->m_ClientData[0]) /*&&
																_pObj->m_Data[DCACS_PAUSEPOS] == _pWClient->GetGameTick()*/)
			{
				pSound->Voice_Unpause(_pObj->m_ClientData[0]);
			}

			// Remove voice if not playing anymore
			if (!pSound->Voice_IsPlaying(_pObj->m_ClientData[0]))
			{
				_pObj->m_ClientData[0] = 0;
			}
		}
	}
}

void CWObject_DCActioncutscene::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Model_OnClientRender, MAUTOSTRIP_VOID);
	MSCOPESHORT( CWObject_Model::OnClientRender );

	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

	//ConOut(CStrF("CreationTick: %d",_pObj->m_CreationGameTick));
	fp32 Time = 0.0f;
	if(_pObj->m_Data[DCACS_CONTROLPOS] == DCACS_CONTROL_START || 
		_pObj->m_Data[DCACS_CONTROLPOS] == DCACS_CONTROL_CONTINUE)//(_pObj->m_ClientFlags & ACS_CLIENTFLAG_ROTATE)
	{
		Time = ((_pWClient->GetGameTick() - _pObj->m_Data[DCACS_ROTATEPOS]) + _pWClient->GetRenderTickFrac());
	}
	else if (_pObj->m_Data[DCACS_CONTROLPOS] == DCACS_CONTROL_PAUSE)
	{
		Time = (_pObj->m_Data[DCACS_PAUSEPOS] - _pObj->m_Data[DCACS_ROTATEPOS]);
	}

	CMat4Dfp32 Rot,Mat;
	Rot.SetZRotation(Time / 100);
	Rot.Multiply(MatIP, Mat);
	MatIP = Mat;

	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		if(_pObj->m_iModel[i] > 0)
		{
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
			if(pModel)
			{
				CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
				AnimState.m_Data[3] = ~(_pObj->m_Data[3]);
				_pEngine->Render_AddModel(pModel, MatIP, AnimState);
			}
		}
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_DCActioncutscene, CWObject_DCActioncutsceneParent, 0x0100);




void CWObject_ActionCutscenePeriscope::OnCreate()
{
	CWObject_ActionCutscenePeriscopeParent::OnCreate();
	m_FOV = 90.0f;
}

//aint CWObject_ActionCutscenePeriscope::OnMessage(const CWObject_Message& _Msg);
bool CWObject_ActionCutscenePeriscope::DoActionSuccess(int _iCharacter)
{
	if (CWObject_ActionCutscenePeriscopeParent::DoActionSuccess(_iCharacter))
	{
		CWObject* pObj = m_pWServer->Object_Get(_iCharacter);
		CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
		// Set fov to controlmode param3
		if (pCD)
			pCD->m_ControlMode_Param3 = m_FOV;

		return true;
	}
	return false;
}

void CWObject_ActionCutscenePeriscope::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	switch (_KeyHash)
	{
	case MHASH1('FOV'): // "FOV"
		{
			m_FOV = _pKey->GetThisValuef();
			break;
		}
	default:
		{
			CWObject_ActionCutscenePeriscopeParent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

spCActionCutsceneCamera CWObject_ActionCutscenePeriscope::GetActionCutsceneCamera(int _iObject, int _iCharacter, 
																				  int _Specific)
{
	CWObject* pObj = m_pWServer->Object_Get(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
	// Set fov to controlmode param3
	if (pCD)
	{
		pCD->m_ControlMode_Param3 = m_FOV;
		pCD->m_ControlMode_Param2 = ACTIONCUTSCENE_TYPE_PERISCOPE;
	}

	return CWObject_ActionCutscenePeriscopeParent::GetActionCutsceneCamera(_iObject, _iCharacter, _Specific);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ActionCutscenePeriscope, CWObject_ActionCutscenePeriscopeParent, 0x0100);
