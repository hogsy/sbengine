#include "PCH.h"

//#include "WObj_Misc.h"
#include "../../../../Projects/Main/GameClasses/WObj_AI/AICore.h"
#include "../../../../Projects/Main/GameClasses/WObj_AI/AI_Custom/AICore_Turret.h"
#include "WObj_PosHistory.h"

#include "MFloat.h"
#include "WObj_Rail.h"

using namespace NRail;

// #define RAIL_DEBUG

// -------------------------------------------------------------------
// 
// team handler
// 
// -------------------------------------------------------------------
void CTeamHandler::AddTeam(uint16 _iTeam)
{
	if(IsMember(_iTeam))
		return;

	m_liTeams.Add(_iTeam);
}

int CTeamHandler::IsMember(uint16 _iTeam)
{
	for(int i = 0; i < m_liTeams.Len(); i++)
		if(m_liTeams[i] == _iTeam)
			return 1;

	return 0;
}

void CTeamHandler::AddTeam(CWorld_Server *_pWServer, const char *_pName)
{
	//Assume team names are unique
	int iTeam = _pWServer->Selection_GetSingleTarget(_pWServer->World_MangleTargetName(_pName));
//	CWObject *pObj = _pWServer->Object_Get(iTeam);

	if (iTeam > 0)
	{
		if(!IsMember(iTeam))
			m_liTeams.Add(iTeam);
	}
	else
	{
		//Team does not exist, create
		iTeam = _pWServer->Object_Create("Team");
		if (iTeam)
		{
			_pWServer->Object_SetName(iTeam, _pName);
			m_liTeams.Add(iTeam);
		}
	}
}

//
//
//
int CTeamHandler::IsTeamMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_TEAM_BELONGTOTEAM: return 1;
	case OBJMSG_TEAM_GETTEAM: return 1;
	case OBJMSG_TEAM_NUMTEAMS: return 1;
	case OBJMSG_TEAM_JOINTEAM: return 1;
	}

	return 0;
}

//
//
//
aint CTeamHandler::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_TEAM_BELONGTOTEAM:
		{
			for(int i = 0; i < m_liTeams.Len(); i++)
			{	
				if (_Msg.m_Param0 == m_liTeams[i])
					return 1;
			}	

			return 0;
		}
	case OBJMSG_TEAM_GETTEAM: return m_liTeams[_Msg.m_Param0];
	case OBJMSG_TEAM_NUMTEAMS: return m_liTeams.Len();

	case OBJMSG_TEAM_JOINTEAM:
		{
			if (_Msg.m_Param0)
			{
				AddTeam(_Msg.m_Param0);
				return 1;
			}
			else
				return 0;
		}
	}

	M_ASSERT(0, "Logic error. Please check message with CTeamHandler::IsTeamMessage before passing it to CTeamHandler::OnMessage\n");
	
	return 0;
}

//
//
//
void CWObject_RailMessagePoint::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	int32 MessageType = -1;

	if(KeyName.Find("MESSAGE_ONDETECT") != -1) MessageType = TYPE_DETECT;
	else if(KeyName.Find("MESSAGE_ONENEMY") != -1) MessageType = TYPE_ENEMY;
	else if(KeyName.Find("MESSAGE_ONDESTROYED") != -1) MessageType = TYPE_DESTROY;
	else if(KeyName.Find("MESSAGE_ONSCAN") != -1) MessageType = TYPE_SCAN;
	else if(KeyName.Find("MESSAGE_ONLOST") != -1) MessageType = TYPE_LOST;
	else if(KeyName.Find("MESSAGE_ONCORPSE") != -1) MessageType = TYPE_CORPSE;
	else if(KeyName.Find("MESSAGE_ONMELEEFIGHT") != -1) MessageType = TYPE_MELEEFIGHT;
	else if(KeyName.Find("USER") != -1)
	{
		m_lUsers.Add(_pKey->GetThisValue().UpperCase());
	}
	else
		CWObject::OnEvalKey(_KeyHash, _pKey);

	if(MessageType != -1)
	{
		CWO_SimpleMessage Msg;
		Msg.Parse(_pKey->GetThisValue(), m_pWServer);
		m_lMessages[MessageType].Add(Msg);
	}


}
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_RailMessagePoint, CWObject, 0x0100);

// -------------------------------------------------------------------
// 
// Rail
// 
// -------------------------------------------------------------------
enum { INVALID_POINT = -1 };
typedef int32 PointIndex;

class CPoint
{
public:
	CMat4Dfp32 m_RailMatrix;
	CMat4Dfp32 m_Matrix;
	uint8 m_Flags;

	CVec3Dfp32 GetPosition() { return CVec3Dfp32::GetRow(m_Matrix, 3); }
};

class CConnection
{
public:
	CConnection()
	{
		m_aiPoints[0] = INVALID_POINT;
		m_aiPoints[1] = INVALID_POINT;
		m_Flags = 0;
	}
	CConnection(PointIndex _Point0, PointIndex _Point1, uint8 _Flags)
	{
		m_aiPoints[0] = _Point0;
		m_aiPoints[1] = _Point1;
		m_Flags = _Flags;
	}

	PointIndex m_aiPoints[2];
	uint8 m_Flags;
};


//
//
//
CDataPacker<0,0xFFFFFFFF,0> CWObject_RailWagon::m_Data_UpVector0;
CDataPacker<1,0xFFFFFFFF,0> CWObject_RailWagon::m_Data_UpVector1;
CDataPacker<2,0xFFFFFFFF,0> CWObject_RailWagon::m_Data_Direction0;
CDataPacker<3,0xFFFFFFFF,0> CWObject_RailWagon::m_Data_Direction1;

CDataPacker<4,0xFFFF,16> CWObject_RailWagon::m_Data_iLaserBeam;
CDataPacker<4,0xF,12> CWObject_RailWagon::m_Data_iAttach_Laser;
CDataPacker<4,0xF,8> CWObject_RailWagon::m_Data_iAttach_Look;
CDataPacker<4,0xF,4> CWObject_RailWagon::m_Data_Scan;
CDataPacker<4,0xF,0> CWObject_RailWagon::m_Data_FireFlash;

CDataPacker<5,0x3,0> CWObject_RailWagon::m_Data_FlashLight;
CDataPacker<5,0x3,2> CWObject_RailWagon::m_Data_ModelMode;

CWObject_RailWagon::CWObject_RailWagon()
{
	m_iHandler = -1;

	m_Static = 0;
	m_Drops = false;

	m_UpVector = CVec3Dfp32(0,1,0);
	m_AimDirection = CVec3Dfp32(1,0,0);
	m_ReloadCounter = 0;
	m_ReloadTime = 0.2f;
	m_ReloadTimeRandom = 0.1f;
	m_AimRandom = 0.075f;

	m_Health = 1;
	m_Connected = 1;
	m_Goal = GOAL_ROAM;

	m_Speed_Normal = 10;
	m_Speed_Search = 5;
	m_Speed_Detected = 2;
	m_Brake = 20;

	//
	m_iSound_Engine = -1;
	m_iSound_Destroy = -1;
	m_iSound_Fire = -1;
	
	m_iSound_Scan = -1;
	m_iSound_Lost = -1;
	m_iSound_Detect = -1;
	m_iSound_Corpse = -1;
	m_iSound_MeleeFight = -1;
	m_iSound_Enemy = -1;

	m_iSound_Turn = -1;
	m_iSound_TurnStop = -1;
	m_iSound_TurnStart = -1;

	//
	m_ScanTime = 0;
	m_ScanToDetectTime = 3000;
	m_LastEnemyMsgTick = -1;
	//
	m_Data_Scan.Set(this, 0);
	m_Data_iAttach_Laser.Set(this, 0);
	m_Data_iAttach_Look.Set(this, 0);
	m_Data_FlashLight.Set(this, 0);

	m_MovementTime = 0;
	m_MovementPhase = 0;
	m_iScriptedTarget = 0;

	m_CanUseLight = 1;

//	M_TRACEALWAYS("CAI_Core_Turret size=%d\n", sizeof(CAI_Core_Turret));

#ifndef M_RTM
	m_DestructStatus = -1;
#endif
}

#ifndef M_RTM
CWObject_RailWagon::~CWObject_RailWagon()
{
	if (m_DestructStatus != 2)
		ConOut("Unauthorized RailWagon destruct!");
};
#endif


//
//
//
void CWObject_RailWagon::OnCreate()
{
	// init AI
	m_spAI = MNew(CAI_Core_Turret);
	m_spAI->Init(this, m_pWServer); 

	m_Data_FireFlash.Set(this, 0);
	m_Data_ModelMode.Set(this, MODELMODE_RAIL);

	m_SweepTime = 10;
	m_PauseTime = 2;

	m_Pitch_Min = 0;
	m_Pitch_Max = 0;

	m_Heading_Min = -45/360.0f;
	m_Heading_Max = 45/360.0f;

	GetHandler().m_lWagons.Add(m_iObject);
	// the rail handler will "eat up" all rails

	//CXR_Model_Flare
	//m_iModel[1] = m_pWServer->GetMapData()->GetResourceIndex_Model("Flare");
	m_iModel[1] = m_pWServer->GetMapData()->GetResourceIndex_Model("SpotLightVolume");

#ifndef M_RTM
	m_DestructStatus = 0;
#endif

	CWObject_Interface_AI::OnCreate();
}

void CWObject_RailWagon::OnDestroy()
{
#ifndef M_RTM
	m_DestructStatus = 1;
#endif

	for(int32 i = 0; i < GetHandler().m_lWagons.Len(); i++)
		if(GetHandler().m_lWagons[i] == m_iObject)
		{
			GetHandler().m_lWagons.Del(i);
#ifndef M_RTM
			m_DestructStatus = 2;
			//Check if there are more occurrances of this object
			for (int32 j = i; j < GetHandler().m_lWagons.Len(); j++)
			{
				if(GetHandler().m_lWagons[j] == m_iObject)
					break;
			}
#endif
			break;
		}


	CWObject_Interface_AI::OnDestroy();
}

void CWObject_RailWagon::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();
	int KeyValuei = KeyValue.Val_int();
	
	if (KeyName.CompareSubStr("TEAM") == 0)
		m_TeamHandler.AddTeam(m_pWServer, KeyValue);

	//
	else
	switch (_KeyHash)
	{
	case MHASH2('STAT','IC'): // "STATIC"
		{
			m_Data_ModelMode.Set(this, MODELMODE_STATIC);
			m_Static = 1;
			break;
		}
	case MHASH2('NOLI','GHT'): // "NOLIGHT"
		{
			m_CanUseLight = 0;
			break;
		}

	// attach points
	case MHASH3('ATTA','CH_L','OOK'): // "ATTACH_LOOK"
		{
			m_Data_iAttach_Look.Set(this, KeyValuei);
			break;
		}
	case MHASH3('ATTA','CH_L','ASER'): // "ATTACH_LASER"
		{
			m_Data_iAttach_Laser.Set(this, KeyValuei);
			break;
		}

	// models
	case MHASH2('LASE','R'): // "LASER"
		{
			m_Data_iLaserBeam.Set(this, m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue));
			break;
		}
	case MHASH2('DROP','S'): // "DROPS"
		{
			m_Drops = true;
			break;
		}

	// sound
	case MHASH3('SOUN','D_FI','RE'): // "SOUND_FIRE"
		{
			m_iSound_Fire = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_EN','GINE'): // "SOUND_ENGINE"
		{
			m_iSound_Engine = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_DE','STRO','Y'): // "SOUND_DESTROY"
		{
			m_iSound_Destroy = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_DE','TECT'): // "SOUND_DETECT"
		{
			m_iSound_Detect = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_SC','AN'): // "SOUND_SCAN"
		{
			m_iSound_Scan = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_LO','ST'): // "SOUND_LOST"
		{
			m_iSound_Lost = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_EN','EMY'): // "SOUND_ENEMY"
		{
			m_iSound_Enemy = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_CO','RPSE'): // "SOUND_CORPSE"
		{
			m_iSound_Corpse = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_ME','LEEF','IGHT'): // "SOUND_MELEEFIGHT"
		{
			m_iSound_MeleeFight = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_TU','RN'): // "SOUND_TURN"
		{
			m_iSound_Turn = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_TU','RNST','OP'): // "SOUND_TURNSTOP"
		{
			m_iSound_TurnStop = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_TU','RNST','ART'): // "SOUND_TURNSTART"
		{
			m_iSound_TurnStart = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	// firing
	case MHASH3('RELO','ADTI','ME'): // "RELOADTIME"
		{
			m_ReloadTime = KeyValuef;
			break;
		}
	case MHASH4('RELO','ADTI','MERA','NDOM'): // "RELOADTIMERANDOM"
		{
			m_ReloadTimeRandom = KeyValuef;
			break;
		}
	case MHASH5('RELO','ADTI','MEMA','GAZI','NE'): // "RELOADTIMEMAGAZINE"
		{
			m_ReloadTimeMagazine = KeyValuef;
			break;
		}
	case MHASH3('MAGA','ZINE','SIZE'): // "MAGAZINESIZE"
		{
			m_MagazineSize = int(KeyValuef);
			break;
		}
	case MHASH3('AIMR','ANDO','M'): // "AIMRANDOM"
		{
			m_AimRandom = KeyValuef;
			break;
		}
	case MHASH3('PROJ','ECTI','LE'): // "PROJECTILE"
		{
			m_Projectile = KeyValue;
			break;
		}
	case MHASH3('MUZZ','LEFL','ASH'): // "MUZZLEFLASH"
		{
			m_MuzzleFlash = KeyValue;
			break;
		}

	// speed
	case MHASH3('SPEE','D_NO','RMAL'): // "SPEED_NORMAL"
		{
			m_Speed_Normal = KeyValuef;
			break;
		}
	case MHASH3('SPEE','D_SE','ARCH'): // "SPEED_SEARCH"
		{
			m_Speed_Search = KeyValuef;
			break;
		}
	case MHASH4('SPEE','D_DE','TECT','ED'): // "SPEED_DETECTED"
		{
			m_Speed_Detected = KeyValuef;
			break;
		}
	case MHASH2('BRAK','E'): // "BRAKE"
		{
			m_Brake = KeyValuef;
			break;
		}

	// movement
	case MHASH5('MOVE','MENT','_ENG','INEP','ATH'): // "MOVEMENT_ENGINEPATH"
		{
			m_EnginePathName = KeyValue;
			break;
		}
	case MHASH5('MOVE','MENT','_PIT','CH_M','IN'): // "MOVEMENT_PITCH_MIN"
		{
			m_Pitch_Min = KeyValuef/360.0f;
			break;
		}
	case MHASH5('MOVE','MENT','_PIT','CH_M','AX'): // "MOVEMENT_PITCH_MAX"
		{
			m_Pitch_Max = KeyValuef/360.0f;
			break;
		}
	case MHASH5('MOVE','MENT','_HEA','DING','_MIN'): // "MOVEMENT_HEADING_MIN"
		{
			m_Heading_Min = KeyValuef/360.0f;
			break;
		}
	case MHASH5('MOVE','MENT','_HEA','DING','_MAX'): // "MOVEMENT_HEADING_MAX"
		{
			m_Heading_Max = KeyValuef/360.0f;
			break;
		}
	case MHASH5('MOVE','MENT','_PAU','SETI','ME'): // "MOVEMENT_PAUSETIME"
		{
			m_PauseTime = KeyValuef;
			break;
		}
	case MHASH5('MOVE','MENT','_SWE','EPTI','ME'): // "MOVEMENT_SWEEPTIME"
		{
			m_SweepTime = KeyValuef;
			break;
		}

	//
	case MHASH4('DEST','ROY_','EFFE','CT'): // "DESTROY_EFFECT"
		{
			m_DestroyEffect = KeyValue;
			break;
		}
	case MHASH2('HEAL','TH'): // "HEALTH"
		{
			m_Health = KeyValuei;
			break;
		}
	case MHASH1('TYPE'): // "TYPE"
		{
			static const char *TranslateType[] = {"TRANSPORT", "CAMERA", "TURRET", NULL};
			m_Type = KeyValue.TranslateInt(TranslateType);
			break;
		}
	default:
		{
		if(!m_spAI->OnEvalKey(_KeyHash, _pKey))
			CWObject_Interface_AI::OnEvalKey(_KeyHash, _pKey);
		break;
		}
	}
}

void CWObject_RailWagon::OnSpawnWorld()
{
	m_iEnginePath = m_pWServer->Selection_GetSingleTarget(m_EnginePathName);
	m_EnginePathName = ""; // save some space
	GetHandler(); // assure we got a handler
}

//
//
//
//		static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);

void CWObject_RailWagon::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	IncludeModelFromKey("LASERBEAM", _pReg, _pMapData);
	IncludeClassFromKey("PROJECTILE", _pReg, _pMapData);
	IncludeClassFromKey("MUZZLEFLASH", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_FIRE", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_ENGINE", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_DESTROY", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_DETECT", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_SCAN", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_LOST", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_ENEMY", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_CORPSE", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_MELEEFIGHT", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_TURN", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_TURNSTART", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_TURNSTOP", _pReg, _pMapData);
	IncludeClassFromKey("DESTROY_EFFECT", _pReg, _pMapData);
	CWObject_Interface_AI::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
}

//
//
//
CWObject_RailHandler &CWObject_RailWagon::GetHandler()
{
	if(m_iHandler == -1)
	{
		// find handler
		m_iHandler = m_pWServer->Selection_GetSingleClass("RailHandler");
		
		// if not found, create it
		if(m_iHandler == -1)
		{
			CMat4Dfp32 Mat;
			Mat.Unit();
			m_iHandler = m_pWServer->Object_Create("RailHandler", Mat);
			M_ASSERT(m_iHandler != -1, "Couldn't create railhandler");
		}
	}

    CWObject_RailHandler *pHandler = safe_cast<CWObject_RailHandler>(m_pWServer->Object_Get(m_iHandler)) ;
	M_ASSERT(pHandler, "Couldn't find railhandler");
	return *pHandler;
}

//
//
//
void CWObject_RailWagon::ReachedNode()
{
	//M_ASSERT(m_lPlannedPath.Len() >= 2, "Path error");

	CWObject_RailHandler &rHandler = GetHandler();

	NodeIndex aNodes[10];
	int32 NumNodes = rHandler.GetPotentialNodes(m_iNextPosition, m_iPosition, aNodes, 10);
	
	if(NumNodes)
	{
		// Move onto next node
		m_iPosition = m_iNextPosition;
		m_iNextPosition = aNodes[((int32)(Random*NumNodes))%NumNodes];
		//NodeIndex iNode = aNodes[(int32)(Random*NumNodes)];
	}
	else
	{ 
		// mark wagon for removal
		m_Flags |= FLAG_REMOVE;
	}
}


void CWObject_RailWagon::Disconnect()
{
	m_Connected = 0;
	m_Velocity = CVec3Dfp32(0,0,0);
}


//Get the number of teams the agent belongs to and add the team indices to the given list
uint CWObject_RailWagon::AI_GetTeams(uint16* _piResult, uint _MaxElem) const
{
	TArrayPtr<const uint16> piTeams = m_TeamHandler.m_liTeams;
	uint n = Min(_MaxElem, (uint)piTeams.Len());

	for (uint i = 0; i < n; i++)
		_piResult[i] = piTeams[i];
	return n;
}


CAI_Core* CWObject_RailWagon::AI_GetAI() { return m_spAI; }
bool CWObject_RailWagon::AI_IsAlive() { return 1; }
void CWObject_RailWagon::AI_GetAimPosition(CVec3Dfp32& _RetValue)
{
	_RetValue = GetPosition();
	CMat4Dfp32 Matrix;
	if(m_Static)
	{
		GetPointMatrix(this, m_pWServer, Matrix, 2, m_Data_iAttach_Look.Get(this));
		_RetValue = CVec3Dfp32::GetRow(Matrix, 3);
	}
	else
	{
		GetPointMatrix(this, m_pWServer, Matrix, 3, m_Data_iAttach_Look.Get(this));
		_RetValue = CVec3Dfp32::GetRow(Matrix, 3);
	}
}

void CWObject_RailWagon::AI_GetLookMatrix(CMat4Dfp32& _RetValue)
{
	//_RetValue = GetPosition();
	if(m_Static)
		GetPointMatrix(this, m_pWServer, _RetValue, 2, m_Data_iAttach_Look.Get(this));
	else
		GetPointMatrix(this, m_pWServer, _RetValue, 3, m_Data_iAttach_Look.Get(this));
}

void CWObject_RailWagon::ClearKnowledge()
{
	int N = m_spAI->m_KB.NumAgentInfo();
	for (int i = 0; i < N; i++)
	{
		CAI_AgentInfo* pCurAgent = m_spAI->m_KB.IterateAgentInfo(i);
		if (pCurAgent)
		{
			pCurAgent->SetAwareness(CAI_AgentInfo::NONE,false,true);
			CAI_Core* pAgentAI = pCurAgent->GetAgentAI();
			if ((pAgentAI) && (pAgentAI->IsPlayer()))
			{
				pCurAgent->SetRelation(CAI_AgentInfo::UNFRIENDLY);
			}
			else
			{
				pCurAgent->SetRelation(m_spAI->m_KB.DefaultRelation(pCurAgent->GetObjectID()));
			}
		}
	}
};
//
//
//
//
void CWObject_RailWagon::UpdatePosition(fp32 _Dist)
{
	CWObject_RailHandler &rHandler = GetHandler();

	if(m_Connected)
	{
		M_ASSERT(rHandler.m_pNodes, "Handler has no nodes");
		M_ASSERT(rHandler.CheckConnectivity(m_iPosition, m_iNextPosition), "Nodes are not connected");

		CMat4Dfp32 Matrix = rHandler.GetMatrix(m_iPosition, m_iNextPosition, m_TravelTime);
		(CVec3Dfp32::GetRow(Matrix,3)-(CVec3Dfp32::GetRow(Matrix,2)*3)).SetRow(Matrix,3);

		if(!SetPosition(Matrix))
		{
			m_TravelTime -= _Dist;
			if(m_TravelTime < 0)
				m_TravelTime = 0;
		}

		CMat4Dfp32 InvMatrix;
		Matrix.InverseOrthogonal(InvMatrix);
		CVec3Dfp32(0).SetRow(InvMatrix,3);

		CMat4Dfp32 RotationMatrix, FinalMatrix;
		RotationMatrix.Unit();
		RotationMatrix.SetZRotation3x3(0.1f);

		InvMatrix.Multiply(RotationMatrix, FinalMatrix);
		CVec3Dfp32 TargetPos = CVec3Dfp32::GetRow(FinalMatrix,2);
		
		m_UpVector = TargetPos; 

		// pack upvector
		m_Data_UpVector0.Set(this, m_Data_UpVector1.Get(this));
		m_Data_UpVector1.Set(this, m_UpVector.Pack32(1.1f));
		//GetAnimState()
	}
	else
	{
		m_Velocity = m_Velocity + CVec3Dfp32(0,0,-2);
		CMat4Dfp32 PositionMatrix = GetPositionMatrix();
		(CVec3Dfp32::GetRow(PositionMatrix, 3)+m_Velocity).SetRow(PositionMatrix, 3);

		#ifdef RAIL_DEBUG
			m_pWServer->Debug_RenderWire(	CVec3Dfp32::GetRow(GetPositionMatrix(), 3)-CVec3Dfp32(0,0,32),
											CVec3Dfp32::GetRow(PositionMatrix, 3)-CVec3Dfp32(0,0,32), 0xFFFFFFFF, 1.0f);
		#endif

		bool Explode = false;	
		if(m_pWServer->Phys_IntersectLine( CVec3Dfp32::GetRow(GetPositionMatrix(), 3)-CVec3Dfp32(0,0,32), 
										CVec3Dfp32::GetRow(PositionMatrix, 3)-CVec3Dfp32(0,0,32), 
										m_iObject, OBJECT_FLAGS_PHYSMODEL|OBJECT_FLAGS_PHYSOBJECT, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID))
		{
			Explode = true;
		}

		if(!SetPosition(PositionMatrix))
			Explode = true;

		if(Explode)
		{
			//m_pWServer->Object_Destroy(m_iObject);
			CMat4Dfp32 Mat = GetPositionMatrix();
			//if(!m_Static)
			Mat.k[3][2] -= 16;
			int iObj = m_pWServer->Object_Create(m_DestroyEffect, Mat);
			if(iObj > 0)
				m_pWServer->Sound_At(CVec3Dfp32::GetRow(Mat, 3), m_iSound_Destroy, 0);

			Destroy();
		}
					// queue for destruction
			//;

		
	}

	
	#ifdef RAIL_DEBUG
	{
		CVec3Dfp32 AimPos;
		AI_GetAimPosition(AimPos);

		m_pWServer->Debug_RenderWire(AimPos-CVec3Dfp32(8,0,0),AimPos+CVec3Dfp32(8,0,0), 0xFFFFFFFF, 0.1f);
		m_pWServer->Debug_RenderWire(AimPos-CVec3Dfp32(0,8,0),AimPos+CVec3Dfp32(0,8,0), 0xFFFFFFFF, 0.1f);
		m_pWServer->Debug_RenderWire(AimPos-CVec3Dfp32(0,0,8),AimPos+CVec3Dfp32(0,0,8), 0xFFFFFFFF, 0.1f);
	}
	#endif
}

//
//
//
void CWObject_RailWagon::GetPointMatrix(CWObject_CoreData *_pObj, CWorld_PhysState *_pWorld, CXR_AnimState &rAnimState, CMat4Dfp32 &_Mat, int32 _iBone, int32 _iAttach)
{
	if(rAnimState.m_pSkeletonInst && ((_iBone >= 0) && (_iBone < rAnimState.m_pSkeletonInst->m_nBoneTransform)))
	{
		_Mat = rAnimState.m_pSkeletonInst->GetBoneTransform(_iBone);

		CXR_Skeleton *pSkeleton = GetSkeleton(_pObj, _pWorld);
		CVec3Dfp32 Vec;

		if(_iAttach >= 0 && pSkeleton->m_lAttachPoints.ValidPos(_iAttach))
			Vec = pSkeleton->m_lAttachPoints[_iAttach].m_LocalPos.GetRow(3);
		else
			Vec = pSkeleton->m_lNodes[_iBone].m_LocalCenter;
		
		Vec = Vec * _Mat;
		Vec.SetRow(_Mat, 3);
	}
	else
		_Mat = _pObj->GetPositionMatrix();
}

void CWObject_RailWagon::GetPointMatrix(CWObject_CoreData *_pObj, CWorld_PhysState *_pWorld, CMat4Dfp32 &_Mat, int32 _iBone, int32 _iAttach)
{
	CXR_AnimState AnimState;
	CMat4Dfp32 PositionMatrix = _pObj->GetPositionMatrix();
	GetAnimState(_pObj, _pWorld, AnimState, PositionMatrix, 0, 0, 0);
	GetPointMatrix(_pObj, _pWorld, AnimState, _Mat, _iBone, _iAttach);
}

//
//
//
CXR_Skeleton *CWObject_RailWagon::GetSkeleton(CWObject_CoreData *_pObj, CWorld_PhysState* _pWorld)
{
	CXR_Model* pModel = _pWorld->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if (pModel == NULL)
		return NULL;

	return (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
}

//
//
//
bool CWObject_RailWagon::GetAnimState(	CWObject_CoreData* _pObj, CWorld_PhysState* _pWorld, CXR_AnimState& _AnimState,
										const CMat4Dfp32& _Matrix, fp32 _AnimTime, fp32 _TickFrac, int _iModel)
{
	CXR_AnimState *pAnimState = &_AnimState;
	int iModel = _iModel;

	if (pAnimState == NULL)
		return 0;

	if ((iModel < 0) || (iModel >= CWO_NUMMODELINDICES))
		return 0;

	pAnimState->m_iObject = _pObj->m_iObject;
	//int iAnim = _pObj->m_Data[7];

	CXR_Skeleton *pSkeleton = GetSkeleton(_pObj, _pWorld);

	// Return success, for we have a model, but it has no skeleton, and is thus not skeleton animated.
	// Though the AnimState is properly default initialised for a regular non-skeleton-animated model.
	if (pSkeleton == NULL)
		return 1; 

	CReferenceCount* RefCount = (CReferenceCount*)_pObj->m_lspClientObj[iModel];
	pAnimState->m_pSkeletonInst = safe_cast<CXR_SkeletonInstance>(RefCount);
	if (pAnimState->m_pSkeletonInst == NULL)
	{
		if (RefCount != NULL)
			return 0;

		MRTC_SAFECREATEOBJECT_NOEX(spSI, "CXR_SkeletonInstance", CXR_SkeletonInstance);
		pAnimState->m_pSkeletonInst = spSI;

		if (pAnimState->m_pSkeletonInst == NULL)
			return 0;

		_pObj->m_lspClientObj[iModel] = pAnimState->m_pSkeletonInst;

	}

	CXR_SkeletonInstance *pSkeletionInstace = pAnimState->m_pSkeletonInst;
	pSkeletionInstace->Create(pSkeleton->m_lNodes.Len());

//	int32 len = pSkeletionInstace->m_nBoneLocalPos;

	//
	CVec3Dfp32 aDirection[2], Direction;
	
	aDirection[0].Unpack32(*((uint32*)&_pObj->m_Data[2]), 1.1f);
	aDirection[1].Unpack32(*((uint32*)&_pObj->m_Data[3]), 1.1f);
	aDirection[0].Lerp(aDirection[1], _TickFrac, Direction);
	Direction.Normalize();
	
	if(m_Data_ModelMode.Get(_pObj) == MODELMODE_STATIC)
	{
		//if(Direction.k[2] > 0.4f) M_TRACEALWAYS("Dir=%f,%f,%f\n", Direction.k[0], Direction.k[1], Direction.k[2]);
		//if(Direction.k[2] <-0.4f) M_TRACEALWAYS("Dir=%f,%f,%f\n", Direction.k[0], Direction.k[1], Direction.k[2]);
		
		// ----------- STATIC ------------------
		pSkeletionInstace->m_pBoneLocalPos[0].Unit();
		pSkeletionInstace->m_pBoneLocalPos[1].Unit();
		pSkeletionInstace->m_pBoneLocalPos[2].Unit();

		CMat4Dfp32 MatLook;
		MatLook.Unit();
		Direction.SetRow(MatLook, 0);
		CVec3Dfp32(0,0,1).SetRow(MatLook, 2);
		MatLook.RecreateMatrix(0,2);
		CVec3Dfp32(0).SetRow(MatLook, 3);
		//MatLook.Normalize3x3();

		//
		CMat4Dfp32 InvMatrix;
		CMat4Dfp32 Matrix;
		Matrix = _pObj->GetPositionMatrix();
		//CVec3Dfp32 Bone3WorldPos = CVec3Dfp32::GetRow(Matrix, 3);
		CVec3Dfp32(0).SetRow(Matrix,3);
		Matrix.InverseOrthogonal(InvMatrix);

		MatLook.Multiply(InvMatrix, pSkeletionInstace->m_pBoneLocalPos[2]);

		//if(MatLook.)

		//pSkeletionInstace->m_lBoneLocalPos[2] = MatLook;
		
		//MatLook.Multiply(InvMatrix, pSkeletionInstace->m_lBoneLocalPos[3]);

		pSkeleton->EvalNode(0, &_Matrix, pSkeletionInstace);

	}
	else if(m_Data_ModelMode.Get(_pObj) == MODELMODE_RAIL)
	{
		// ----------- NON-STATIC ------------------

		// BoneAnimated Specifics...
		{
			//
			//CMat4Dfp32 InterpolatedMatrix;
			
			//pSkeletionInstace->
			{
				pSkeletionInstace->m_pBoneLocalPos[0].Unit();
				pSkeletionInstace->m_pBoneLocalPos[1].Unit();
				pSkeletionInstace->m_pBoneLocalPos[2].Unit();
				if(pSkeletionInstace->m_nBoneLocalPos > 3)
					pSkeletionInstace->m_pBoneLocalPos[3].Unit();

				CVec3Dfp32 aUpVector[2], UpVector;
				aUpVector[0].Unpack32(m_Data_UpVector0.Get(_pObj), 1.1f);
				aUpVector[1].Unpack32(m_Data_UpVector1.Get(_pObj), 1.1f);
				aUpVector[0].Lerp(aUpVector[1], _TickFrac, UpVector);
				UpVector.Normalize();


				CMat4Dfp32 FinalMatrix;
				FinalMatrix.Unit();
				UpVector.SetRow(FinalMatrix,2); // up vector
				CVec3Dfp32(1,0,0).SetRow(FinalMatrix,0); // direction
				FinalMatrix.RecreateMatrix(0,2);
				pSkeletionInstace->m_pBoneLocalPos[2] = FinalMatrix;
				if(pSkeletionInstace->m_nBoneLocalPos > 3)
					pSkeletionInstace->m_pBoneLocalPos[3].Unit();


				//pSkeletionInstace->
				//GetAnimState
				pSkeleton->EvalNode(0, &_Matrix, pSkeletionInstace);


				// aiming part
				if(pSkeletionInstace->m_nBoneLocalPos > 3)
				{
					CMat4Dfp32 InvMatrix;
					CMat4Dfp32 Matrix = pSkeletionInstace->m_pBoneTransform[3];
					CVec3Dfp32 Bone3WorldPos = CVec3Dfp32::GetRow(Matrix, 3);
					CVec3Dfp32(0).SetRow(Matrix,3);
					Matrix.Inverse(InvMatrix);

					CMat4Dfp32 MatLook;
					MatLook.Unit();
					
					//CVec3Dfp32 Direction = (LookAt-Bone3WorldPos);
					Direction.SetRow(MatLook, 0);
					CVec3Dfp32(0,0,1).SetRow(MatLook, 2);
					MatLook.RecreateMatrix(0,2);
					
					MatLook.Multiply(InvMatrix, pSkeletionInstace->m_pBoneLocalPos[3]);

					// reavaluate
					pSkeleton->EvalNode(0, &_Matrix, pSkeletionInstace);
				}
			}
			

		}
	}
	else
	{
		// darn
	}

	return 0;
}

//
//
//
void CWObject_RailWagon::OnClientCreate(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient)
{
	CWObject_Interface_AI::OnClientCreate(_pObj, _pWClient);
	_pObj->m_iModel[0] = _pWClient->GetMapData()->GetResourceIndex_Model("CoordSys");
}

//
//
//
void CWObject_RailWagon::OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, const CMat4Dfp32& _ParentMat)
{
	MSCOPESHORT(CWObject_RailWagon::OnClientRenderVis);
	//return;
	if(m_Data_FlashLight.Get(_pObj))
	{
		// get position matrix
		CMat4Dfp32 PositionMatrix;
		fp32 IPTime = _pWClient->GetRenderTickFrac();
		Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), PositionMatrix, IPTime);

		// anim state
		CXR_AnimState AnimState;
		GetAnimState(_pObj, _pWClient, AnimState, PositionMatrix, 0, IPTime, 0);

		CMat4Dfp32 Matrix;
		GetPointMatrix(_pObj, _pWClient, AnimState, Matrix, 3, m_Data_iAttach_Laser.Get(_pObj));
		CXR_Light Light(Matrix, CVec3Dfp32(4,0.5f,0.5f), 512, 0, CXR_LIGHTTYPE_SPOT);
		
		/*
		if(pCD->m_iPlayer != -1)
		{
			Light.m_SpotHeight = M_Tan(50 * (_PI / 180.0f ));
			Light.m_SpotWidth = M_Tan(50 * (_PI / 180.0f ));
		}
		else*/
		//{
			Light.m_SpotHeight = M_Tan(30 * (_PI / 180.0f ));
			Light.m_SpotWidth = M_Tan(30 * (_PI / 180.0f ));
		//}
		Light.m_LightGUID = _pObj->m_iObject + 0x4001;
		Light.m_iLight = 0;
		//if (!Char_IsPlayer(_pObj))
		//Light.m_Flags |= CXR_LIGHT_NOSPECULAR;
		CXR_SceneGraphInstance* pSGI = _pWClient->World_GetSceneGraphInstance();
		if(pSGI)
			pSGI->SceneGraph_Light_LinkDynamic(Light);

		CMat4Dfp32 Mat;
		//if(OnRefresh_WeaponLights(_pObj, _pWClient, MatIP, IPTime, &Mat))
		{
			/*
			bool bCutSceneView = (_pObj->m_ClientFlags & (PLAYER_CLIENTFLAGS_CUTSCENE | PLAYER_CLIENTFLAGS_DIALOGUE)) != 0;
			bool bThirdPerson = (pCD->m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON | PLAYER_EXTRAFLAGS_FORCETHIRPERSON)) != 0;
			int iPass = _pEngine->GetVCDepth();
			bool bFirstPerson = bLocalPlayer && !iPass && !bThirdPerson && !bCutSceneView;
			*/

			//if (!bFirstPerson)
			// {
			CXR_AnimState AnimState;
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[1]);
			_pEngine->Render_AddModel(pModel, Matrix, AnimState);
			//}
		}		
	}
}

//
//
//
void CWObject_RailWagon::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	// get position matrix
	CMat4Dfp32 PositionMatrix;
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), PositionMatrix, IPTime);

	// anim state
	CXR_AnimState AnimState;
	GetAnimState(_pObj, _pWClient, AnimState, PositionMatrix, 0, IPTime, 0);

	// main model
	{
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
		_pEngine->Render_AddModel(pModel, _pObj->GetPositionMatrix(), AnimState);
	}
	/*
	// laser sight
	if(m_Data_Scan.Get(_pObj))
	{
		CMat4Dfp32 Matrix;
		AnimState.m_Colors[3] = 0xffff0000;
		GetPointMatrix(_pObj, _pWClient, AnimState, Matrix, 3, m_Data_iAttach_Laser.Get(_pObj));

		CCollisionInfo Info;
		Info.SetReturnValues(CXR_COLLISIONRETURNVALUE_TIME);

		AnimState.m_AnimAttr0 = 256; // Max length
		CVec3Dfp32 &From = CVec3Dfp32::GetRow(Matrix, 3);
		CVec3Dfp32 To = From + CVec3Dfp32::GetRow(Matrix, 0) * AnimState.m_AnimAttr0;
		if(_pWClient->Phys_IntersectLine(From, To, 0, OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_CHARACTER, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &Info) && Info.m_bIsValid)
			AnimState.m_AnimAttr0 *= Info.m_Time;

		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(m_Data_iLaserBeam.Get(_pObj));
		AnimState.m_AnimTime0.Reset();
		_pEngine->Render_AddModel(pModel, Matrix, AnimState);
	}
	*/

	// muzzle flash
	/*
	if(_pObj->m_Data[4])
	{
		CXR_AnimState MuzzleState;
		MuzzleState.m_AnimTime0 = 1+IPTime;
		MuzzleState.m_AnimTime1 = 1+IPTime;
		CMat4Dfp32 Matrix;
		GetPointMatrix(_pObj, _pWClient, AnimState, Matrix, 3, 0);

		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[1]);
		_pEngine->Render_AddModel(pModel, Matrix, MuzzleState);
	}*/

//	CWObject_Model::OnClientRender(_pObj, _pWClient, _pEngine);
}

//
//
//
aint CWObject_RailWagon::OnMessage(const CWObject_Message& _Msg)
{
	//return CWObject_Interface_AI::OnMessage(_Msg);

	// let AI catch messages
	if (m_spAI->OnMessage(_Msg))
		return true;

	if(_Msg.m_Msg == OBJMSG_AIQUERY_ISALIVE)
		return 1;

	
	if(_Msg.m_Msg == OBJMSG_DAMAGE)
	{
		const CWO_DamageMsg *pMsg = CWO_DamageMsg::GetSafe(_Msg);
		if(pMsg->m_DamageType == DAMAGETYPE_BLOW)
			return 0;
		m_Health -= pMsg->m_Damage;
		// TODO: handle damage
		//if(pMsg)
		//	return Physics_Damage(*pMsg, _Msg.m_iSender);
	}
	
	if(_Msg.m_Msg == OBJMSG_AIQUERY_GETAI)
	{
		CAI_Core ** ppAI = (CAI_Core **)(_Msg.m_pData);
		*ppAI = m_spAI;

		if (m_spAI)
			return 1;
		return 0;
	}

	// give the team handler a chance
	if(m_TeamHandler.IsTeamMessage(_Msg))
		return m_TeamHandler.OnMessage(_Msg);

	return CWObject_Interface_AI::OnMessage(_Msg);
}

//
//
//
void CWObject_RailWagon::Fire(const CMat4Dfp32 &_Mat)
{
	MAUTOSTRIP(CWObject_RailWagon_Fire, MAUTOSTRIP_VOID);

	int iCurrentObject;
	if((m_SpawnedObjectGUID != 0) && ((iCurrentObject = m_pWServer->Object_GetIndex(m_SpawnedObjectGUID)) != 0))
	{
		// Use already spawned object?
		CWObject_Message Msg(OBJMSG_SUMMON_SPAWNPROJECTILES);
		Msg.m_pData = (void*)&_Mat;
		m_pWServer->Message_SendToObject(Msg, iCurrentObject);
	}
	else
	{
		CMat4Dfp32 Matrix = _Mat;
		int iObj = m_pWServer->Object_Create(m_Projectile, _Mat, m_iObject);
		if(iObj > 0)
			m_SpawnedObjectGUID = m_pWServer->Object_GetGUID(iObj);
	};

	m_Data_FireFlash.Set(this, 2);

	m_pWServer->Object_Create(m_MuzzleFlash, _Mat, m_iObject);
}



void CWObject_RailWagon::Destroy()
{
	// send destroy message
	GetHandler().SendMessages(this, GetPosition(), CWObject_RailMessagePoint::TYPE_DESTROY);

	for(int32 i = 0; i < GetHandler().m_lMobileWagons.Len(); i++)
		if(GetHandler().m_lMobileWagons[i] == m_iObject)
		{
			GetHandler().m_lMobileWagons.Del(i);
			break;
		}

	// queue for destruction
	m_pWServer->Object_Destroy(m_iObject);
}
//
//
//
void CWObject_RailWagon::OnRefresh()
{
//	if(m_Health <= 0)
//		return;

	// update AI
	m_spAI->m_KB.SetAlertness(CAI_KnowledgeBase::ALERTNESS_WATCHFUL);
	bool bFirstRefresh = m_spAI->m_bFirstRefresh;

	if(GetHandler().GotPower()) // only refresh AI when we got powah
		m_spAI->OnRefresh(false);

	// We let the ai do its first refresh before we remove processed dead/add new enemies
	if (bFirstRefresh)
	{
		CWObject_RailHandler& rHandler = GetHandler();
		for (int i = 0; i < rHandler.m_lProcessedDead.Len(); i++)
		{
			m_spAI->m_KB.RemoveDead(rHandler.m_lProcessedDead[i]);
		}
		for (int i = 0; i < rHandler.m_lEnemies.Len(); i++)
		{
			CAI_AgentInfo* pEnemy = m_spAI->m_KB.GetAgentInfo(rHandler.m_lEnemies[i]);
			if (pEnemy)
			{
				pEnemy->SetRelation(CAI_AgentInfo::ENEMY);
			}
			else
			{
				pEnemy = m_spAI->m_KB.AddAgent(rHandler.m_lEnemies[i]); 
				if (pEnemy)
				{
					pEnemy->SetRelation(CAI_AgentInfo::ENEMY);
				}
			}
		}
	}

	if (!m_Static)
	{
		// ----------- NON STATIC --------------
		//              MOVEMENT
		// -------------------------------------

		// get handler
		CWObject_RailHandler &rHandler = GetHandler();

		//
		fp32 SpeedAdjust = 1.0f;

		// see if other bots are on the same rail, perhaps we must slow down

		// get all wagons that are connected with this one
		const int32 MaxWagons = 128;
		CWObject_RailWagon *lWagons[MaxWagons];
		int32 NumWagons = 0;
		for(int32 i = 0; i < rHandler.NumWagons(); i++)
		{
			CWObject_RailWagon &rWagon = rHandler.GetWagon(i);
			if(rWagon.m_iObject == m_iObject) // don't count our self
				continue;

			if(rWagon.m_iPosition == m_iPosition ||
				rWagon.m_iPosition == m_iNextPosition ||
				rWagon.m_iNextPosition == m_iNextPosition)
			{
				lWagons[NumWagons++] = &rWagon;
				if(NumWagons >= MaxWagons) // shouldn't happen
					break;
				continue;
			}
		}

		for(int32 i = 0; i < NumWagons; i++)
		{
			CWObject_RailWagon *pWagon = lWagons[i];

			// --- = rail
			// < and > = direction
			// o = other wagon
			// t = this wagon
			// | = node
			//
			// 0 = nothing, could be ---<t----<o----
			// 1 = ---<o----<t----
			//			we might slowdown if we get to close the the other wagon
			// 2 = ---o>----<t----
			//			don't do anything, we should slow the one with lower priority and move them out of the way
			int32 Situation = 0;
			fp32 Distance = 0;

			if(pWagon->m_iPosition == m_iPosition && pWagon->m_iNextPosition == m_iNextPosition)
			{
				// *** on the same track and going in the same direction
				if(pWagon->m_TravelTime > m_TravelTime)
				{
					// *** the other wagon is onfront of us
					// --|---<o---<t---|--
					Situation = 1;
					Distance = (pWagon->m_TravelTime-m_TravelTime)/m_TimeScale;
				}
				else
				{
					// *** we are infront of the other wagon
					// --|---<t---<o---|--
				}
			}
			else if(pWagon->m_iPosition == m_iNextPosition)
			{
				// *** our target position is the others current position
				if(pWagon->m_iNextPosition == m_iPosition)
				{
					// *** there is a wagon heading towards us
					// |---o>---<t---|
					Situation = 2;
					Distance = (pWagon->m_TravelTime-m_TravelTime)/m_TimeScale;
				}
				else
				{
					// *** there is a wagon infront of us on the next lane
					// |---<o---|---<t---|
					Situation = 1;
					Distance = (1-m_TravelTime)/m_TimeScale;
					Distance += (pWagon->m_TravelTime)/pWagon->m_TimeScale;
				}
			}
			else if(pWagon->m_iNextPosition == m_iNextPosition)
			{
				// *** There is another wagon heading towards us
				// |---o>---|---<t---|
				Situation = 2;
				Distance = (1-m_TravelTime)/m_TimeScale;
				Distance += (1-pWagon->m_TravelTime)/pWagon->m_TimeScale;
			}
			else if(pWagon->m_iNextPosition == m_iPosition)
			{
				// *** There is a bot heading towards our position
				// ---<t---|---<o---
			}
			else
			{
				// *** some undefined situation
				// could be one of these
				// ---<t---|---o>---
			}

			if(Situation == 0)
			{
				// do nothing
			}
			else if(Situation == 1)
			{
				// *** another wagon is infront of us
				// ---<o----<t----
				if(Distance < 64) // we are getting to close, slow down
					SpeedAdjust = 0;
			}
			else if(Situation == 2)
			{
				// *** we are on a colliding course with another wagon
				// ---o>----<t----
			}
		}
		


		// adjust speed
		if(m_CurrentSpeed < m_MaxSpeed*SpeedAdjust)
		{
 			m_CurrentSpeed += m_pWServer->GetGameTickTime() * 2.5f;
			if(m_CurrentSpeed > m_MaxSpeed*SpeedAdjust)
				m_CurrentSpeed = m_MaxSpeed*SpeedAdjust;
		}
		else if(m_CurrentSpeed > m_MaxSpeed*SpeedAdjust)
		{
			m_CurrentSpeed -= m_pWServer->GetGameTickTime() * 40.0f;

			if(m_CurrentSpeed < m_MaxSpeed*SpeedAdjust)
				m_CurrentSpeed = m_MaxSpeed*SpeedAdjust;
		}


		fp32 Dist = m_CurrentSpeed*m_TimeScale*rHandler.m_PowerLevel;
		m_TravelTime += Dist;

		if(m_TravelTime > 1)
		{
			// let it update it's path
			ReachedNode();

			// Calculate travelspeed and reset traveltime
			m_TimeScale = rHandler.GetTimeScale(m_iPosition, m_iNextPosition);
			m_TravelTime = 0;
		}//fp32 _TravelTime

		UpdatePosition(Dist);

		// fix sound
		if(GetHandler().GotPower())
		{
			if(m_iSound[0] != m_iSound_Engine)
			{
				m_iSound[0] = m_iSound_Engine;
				m_DirtyMask |= CWO_DIRTYMASK_SOUND;
				m_pWServer->Sound_On(m_iObject, m_iSound[0], 2); // seams to be nessesary
			}
		}
		else
		{
			if(m_iSound[0] != 0)
			{
				m_iSound[0] = 0;
				m_DirtyMask |= CWO_DIRTYMASK_SOUND;
			}
		}


		//CMat4Dfp32 Matrix;
		//GetPointMatrix(this, m_pWServer, Matrix, 3, m_Data_iAttach_Look.Get(this));
	}

	//
	if(m_Health <= 0)
	{
		bool DoDestroyEffect = false;
		CWO_PhysicsState State;
		State.Clear();
		m_pWServer->Object_SetPhysics(m_iObject, State);
		
		if(m_Static)
		{
			if(m_Data_ModelMode.Get(this) != MODELMODE_STATIC_DESTROYED)
			{
				DoDestroyEffect = true;
				// static once just change their model
				m_iModel[0] = m_iModel[2];
				m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);
				m_Data_ModelMode.Set(this, MODELMODE_STATIC_DESTROYED);
			}
		}
		else if(m_Drops)
		{
			if(m_Connected)
			{
				// send destroy message
				GetHandler().SendMessages(this, GetPosition(), CWObject_RailMessagePoint::TYPE_DESTROY);

				for(int32 i = 0; i < GetHandler().m_lMobileWagons.Len(); i++)
					if(GetHandler().m_lMobileWagons[i] == m_iObject)
					{
						GetHandler().m_lMobileWagons.Del(i);
						break;
					}

				
				Disconnect();
			}
			DoDestroyEffect = false;
		}
		else
		{
			DoDestroyEffect = true;
			Destroy();
		}

		if(DoDestroyEffect)
		{
			CMat4Dfp32 Mat = GetPositionMatrix();
			CVec3Dfp32 AimPos;
			AI_GetAimPosition(AimPos);
			AimPos.SetRow(Mat, 3);

			int iObj = m_pWServer->Object_Create(m_DestroyEffect, Mat);
			if(iObj > 0)
				m_pWServer->Sound_At(CVec3Dfp32::GetRow(Mat, 3), m_iSound_Destroy, 0);
		}

		if(m_iSound[0] != -1)
		{
			m_DirtyMask |= CWO_DIRTYMASK_SOUND;
			m_iSound[0] = -1;
		}
	
		// static 
		return;
		//*/
	}

	// reload stuff
	if(m_ReloadCounter > 0)
		m_ReloadCounter -= m_pWServer->GetGameTickTime();

	int32 FireFlash = m_Data_FireFlash.Get(this);
	if(FireFlash)
		m_Data_FireFlash.Set(this, FireFlash); // reset muzzleflash
 

	//
	CAI_AgentCriteria AC;
	AC.Relation(CAI_AgentInfo::NEUTRAL);
	AC.Awareness(CAI_AgentInfo::NOTICED);
	TArray<CWObject*> lObjects;
	CVec3Dfp32 WantedDirection = CVec3Dfp32::GetRow(GetPositionMatrix(), 0);

	/*
	if(WantedDirection.k[0] > 1.1f || WantedDirection.k[0] < -1.1f) M_TRACEALWAYS("Fuckad WantedDir 1\n");
	if(WantedDirection.k[1] > 1.1f || WantedDirection.k[1] < -1.1f) M_TRACEALWAYS("Fuckad WantedDir 2\n");
	if(WantedDirection.k[2] > 1.1f || WantedDirection.k[2] < -1.1f) M_TRACEALWAYS("Fuckad WantedDir 3\n");
	if(WantedDirection.Length() > 1.01f) M_TRACEALWAYS("Fuckad WantedDir 4\n");
	if(WantedDirection.Length() < 0.99f) M_TRACEALWAYS("Fuckad WantedDir 5\n");
	*/

	bool Tracking = false;

	if((GetHandler().GotPower()) && (m_Type != TYPE_TRANSPORT) && ((m_iScriptedTarget)||(m_spAI->m_KB.FindAllAgents(AC,&lObjects))))
	{
		CWObject_RailHandler& Handler = GetHandler();
		int32 WorstRelation = CAI_AgentInfo::NEUTRAL;
		int32 Awareness = CAI_AgentInfo::AWARENESS_INVALID;
		CWObject* pObject = NULL;

		// Check scripted target first
		if (Handler.m_iScriptedTarget)
		{
			pObject = m_pWServer->Object_Get(Handler.m_iScriptedTarget);
			if (pObject)
			{
				Awareness = CAI_AgentInfo::SPOTTED;
				WorstRelation = CAI_AgentInfo::ENEMY;
			}
		}

		if ((!pObject)||(WorstRelation < CAI_AgentInfo::ENEMY))
		{
			for(int32 i = 0; i < lObjects.Len(); i++)
			{
				CAI_AgentInfo *pAgent = m_spAI->m_KB.GetAgentInfo(lObjects[i]->m_iObject);

				if(!pAgent)
					continue;

				int32 Relation = pAgent->GetRelation();
				if(pAgent->GetAwareness() > CAI_AgentInfo::NOTICED && Relation > WorstRelation)
				{
					WorstRelation = Relation;
					pObject = lObjects[i];
					Awareness = pAgent->GetAwareness();
				}
			}
		}

		// Here we sometimes check for corpses if nothing else of interest has popped up
		if (((!pObject)||(WorstRelation < CAI_AgentInfo::ENEMY)) && ((m_spAI->m_Timer % 10) == 0))
		{
			SDead Dead;

			// Matrix: CAI_AgentInfo::UNKNOWN means we care about all corpses
			int CheckResult = m_spAI->m_KB.CheckDead(&Dead,CAI_AgentInfo::UNKNOWN,0,m_spAI->m_SightRange,CAI_AgentInfo::DETECTED,true);
			if (CheckResult > 0)
			{	// Found a corpse
				// Fake the param to get a response
				Awareness = CAI_AgentInfo::SPOTTED;		// Fake spotted
				if (CheckResult == 2)
				{	// Murderer was also found
					pObject = m_spAI->m_pServer->Object_Get(Dead.m_iCause);
					if (pObject)
					{
						WorstRelation = CAI_AgentInfo::ENEMY;
						Handler.AddUniqueEnemy(Dead.m_iCause);
						CAI_AgentInfo* pMurderer = m_spAI->m_KB.GetAgentInfo(Dead.m_iCause);
						if (pMurderer)
						{
							pMurderer->SetRelation(CAI_AgentInfo::ENEMY);
						}
					}
				}
				else
				{
					pObject = m_spAI->m_pServer->Object_Get(Dead.m_iVictim);
					if (pObject)
					{
						WorstRelation = CAI_AgentInfo::HOSTILE;
						CAI_AgentInfo* pSuspect = m_spAI->m_KB.GetAgentInfo(Dead.m_iCause);
						if ((pSuspect) && (pSuspect->GetCurRelation() < CAI_AgentInfo::HOSTILE))
						{
							pSuspect->SetRelation(CAI_AgentInfo::HOSTILE);
						}
					}
				}
				// We should remove the corpse from all wagons, we should also set iCause relation
				// to WorstRelation for all wagons.
				Handler.AddUniqueProcessedDead(Dead.m_iVictim);
				int N = Handler.NumWagons();
				for (int i = 0; i < N; i++)
				{
					CWObject_RailWagon& Wagon = Handler.GetWagon(i);
					CAI_AgentInfo* pPerpInfo = Wagon.m_spAI->m_KB.GetAgentInfo(Dead.m_iCause);
					if ((pPerpInfo) && (pPerpInfo->GetCurRelation() < WorstRelation))
					{
						pPerpInfo->SetRelation(WorstRelation);
					}
					Wagon.m_spAI->m_KB.RemoveDead(Dead.m_iVictim);
				}
				Handler.SendMessages(this, pObject->GetPosition(),CWObject_RailMessagePoint::TYPE_CORPSE,Dead.m_iVictim);
				
				// Say something appropriate
				if (GetHandler().CanPlaySound(RAIL_SOUND_CORPSE))
				{
					m_pWServer->Sound_At(GetPosition(),m_iSound_Corpse,0);
				}
			}
		}

		// Here we check for meleefights
		if ((!pObject)||(WorstRelation < CAI_AgentInfo::ENEMY))
		{
			CAI_AgentInfo* pMeleeFighter = m_spAI->m_KB.GetLastMeleeRelationIncrease();
			if (pMeleeFighter)
			{
				int FighterRelation = pMeleeFighter->GetCurRelation();
				if (FighterRelation >= WorstRelation)
				{
					WorstRelation = FighterRelation;
					pObject = pMeleeFighter->GetObject();
					CWObject_RailHandler& Handler = GetHandler();
					if (pObject)
					{
						Handler.SendMessages(this,pObject->GetPosition(),CWObject_RailMessagePoint::TYPE_MELEEFIGHT,pMeleeFighter->GetObjectID());
					}
					if (GetHandler().CanPlaySound(RAIL_SOUND_MELEE))
					{
						m_pWServer->Sound_At(GetPosition(),m_iSound_MeleeFight,0);
					}
				}
			}
		}

		if(pObject && WorstRelation != CAI_AgentInfo::RELATION_INVALID)
		{
			m_LastScanPos = pObject->GetPosition();

			// scan
			m_Data_Scan.Set(this, 1);
			if (m_Type == TYPE_CAMERA)
			{
				if ((WorstRelation >= CAI_AgentInfo::ENEMY) && (m_spAI->UseFlash(10,2)))
				{	// The flash is ours, shine!
					m_ClientFlags |= CWO_CLIENTFLAGS_VISIBILITY;
					m_Data_FlashLight.Set(this,1);
				}
				else
				{	// Someone else with higher prio uses the flash
					m_Data_FlashLight.Set(this, 0);
					m_ClientFlags &= ~CWO_CLIENTFLAGS_VISIBILITY;
				}
			}

			// adjust speed
			if(WorstRelation >= CAI_AgentInfo::ENEMY)
			{
				m_MaxSpeed = m_Speed_Detected;
				Handler.AddUniqueEnemy(pObject->m_iObject);
			}
			else if(WorstRelation >= CAI_AgentInfo::HOSTILE)
				m_MaxSpeed = m_Speed_Search;
			else
				m_MaxSpeed = m_Speed_Normal;

			//
			if(m_ScanTime == 0) // start scanning
			{
				if (GetHandler().CanPlaySound(RAIL_SOUND_SCAN))
				{
					m_pWServer->Sound_At(GetPosition(),m_iSound_Scan, 0);
				}
				GetHandler().SendMessages(this, pObject->GetPosition(), CWObject_RailMessagePoint::TYPE_SCAN);
			}

			// increase scan time
			if (m_ScanTime < m_ScanToDetectTime)
			{
				m_ScanTime += (uint16)(m_pWServer->GetGameTickTime() * 1000);
				if (m_ScanTime >= m_ScanToDetectTime)
				{
					m_ReloadCounter = m_ReloadTimeMagazine; // headstart before firing
					m_AmmoLeft = m_MagazineSize;
				}
				else
				{
					GetHandler().SendMessages(this, pObject->GetPosition(), CWObject_RailMessagePoint::TYPE_DETECT);
					if (GetHandler().CanPlaySound(RAIL_SOUND_DETECT))
					{
						m_pWServer->Sound_At(GetPosition(),m_iSound_Detect,0);
					}
				}
			}

			if (m_ScanTime >= m_ScanToDetectTime)
			{
				if ((WorstRelation >= CAI_AgentInfo::ENEMY) && (Awareness >= CAI_AgentInfo::SPOTTED))
				{	// enemy
					int GameTick = m_pWServer->GetGameTick();
					if (GameTick >= m_LastEnemyMsgTick + 1.0f * m_pWServer->GetGameTicksPerSecond())
					{
						GetHandler().SendMessages(this, pObject->GetPosition(), CWObject_RailMessagePoint::TYPE_ENEMY,pObject->m_iObject);
						if (GetHandler().CanPlaySound(RAIL_SOUND_ENEMY))
						{
							m_pWServer->Sound_At(GetPosition(), m_iSound_Enemy, 0);
						}
						m_LastEnemyMsgTick = GameTick;
					}
				}
			}

			// find projectile matrix
			CVec3Dfp32 Position = m_spAI->GetTorsoPosition(pObject);
			WantedDirection = Position - GetPosition();
			WantedDirection.Normalize();
			Tracking = true;

			if(m_Type == TYPE_TURRET && m_ReloadCounter <= 0)
			{
				// CAI_AgentInfo *pInfo = m_spAI->m_KB.GetAgentInfo(pObject->m_iObject);
				// if ((pInfo && pInfo->GetRelation() >= CAI_AgentInfo::ENEMY) && (Awareness >= CAI_AgentInfo::SPOTTED))
				if ((pObject) && (WorstRelation >= CAI_AgentInfo::ENEMY) && (Awareness >= CAI_AgentInfo::SPOTTED))
				{
					if ((m_AmmoLeft) && (m_spAI->m_pAIResources->AllowActions()) && (GetHandler().CanFire()))
					{
						CMat4Dfp32 Matrix;
						if(m_Static)
							GetPointMatrix(this, m_pWServer, Matrix, 2, m_Data_iAttach_Look.Get(this));
						else
							GetPointMatrix(this, m_pWServer, Matrix, 3, m_Data_iAttach_Look.Get(this));

						// set reload time
						m_ReloadCounter = m_ReloadTime + (Random*m_ReloadTimeRandom);
						m_AmmoLeft--;

						// Add noise
						CVec3Dfp32 RandomFactor = CVec3Dfp32(Random-0.5f, Random-0.5f, Random-0.5f);
						RandomFactor.Normalize();
						CVec3Dfp32 Direction = CVec3Dfp32::GetRow(Matrix, 0);
						// Direction.Normalize();
						Direction = WantedDirection;
						Direction += RandomFactor*Random*m_AimRandom;
						Direction.Normalize();
						Direction.SetRow(Matrix, 0);
						CVec3Dfp32(0,0,1).SetRow(Matrix, 2);
						
						Matrix.RecreateMatrix(0,2);

						Fire(Matrix);
	#ifdef RAIL_DEBUG
						m_pWServer->Debug_RenderWire(	CVec3Dfp32::GetRow(Matrix,3),
														CVec3Dfp32::GetRow(Matrix,3)+CVec3Dfp32::GetRow(Matrix,0)* 512, 0xff7f0000);
	#endif
						// make noise
						m_pWServer->Sound_On(m_iObject, m_iSound_Fire, 0);
					}
					else
					{
						m_AmmoLeft = m_MagazineSize;
						m_ReloadCounter = m_ReloadTimeMagazine;
					}
				}
				
			}
		}
		else
		{
			if (m_ScanTime)
			{	// Send lost, but only if all other cams also are lost
				CWObject_RailHandler& Handler = GetHandler();
				if (Handler.AllScanLost(this))
				{	// We only send thid when we're the last to loose scan
					GetHandler().SendMessages(this, m_LastScanPos, CWObject_RailMessagePoint::TYPE_LOST);
					if (GetHandler().CanPlaySound(RAIL_SOUND_LOST))
					{
						m_pWServer->Sound_At(GetPosition(), m_iSound_Lost, 0);
					}
				}
			}

			m_ScanTime = 0;
			m_Data_Scan.Set(this, 0);
			m_Data_FlashLight.Set(this, 0);
			m_ClientFlags &= ~CWO_CLIENTFLAGS_VISIBILITY;
		}
	}
	else
	{
		m_Data_Scan.Set(this, 0);
		m_MaxSpeed = m_Speed_Normal;
	}

	if(m_Static)
	{
		int32 iSound = -1;

		if(!Tracking)
		{
			if(m_iEnginePath != -1)
			{
				CMat4Dfp32 EngineMatrix;
				CWObject_Message Msg(OBJMSG_HOOK_GETCURRENTMATRIX, (aint)&EngineMatrix);
				m_pWServer->Phys_Message_SendToObject(Msg, m_iEnginePath);
				WantedDirection = CVec3Dfp32::GetRow(EngineMatrix, 0);
			}
			else
			{

				m_MovementTime += m_pWServer->GetGameTickTime();

				// move
				fp32 HeadDiff = m_Heading_Max-m_Heading_Min;
				fp32 PitchDiff = m_Pitch_Max-m_Pitch_Min;
				fp32 Amount;
				fp32 MaxTime;

				if(m_MovementPhase&1)
				{
					MaxTime = m_PauseTime;
					if(m_MovementPhase&2)
						Amount = 0;
					else
						Amount = 1;
				}
				else
				{
					iSound = m_iSound_Turn;
					MaxTime = m_SweepTime;
					Amount = Min(m_MovementTime/m_SweepTime, 1.0f);
					if(m_MovementPhase&2)
						Amount = 1-Amount;
				}

				if(m_MovementTime > MaxTime)
				{
					m_MovementTime = 0;
					m_MovementPhase++;
					if(m_MovementPhase&1)
					{
						iSound = m_iSound_TurnStop;
						//m_pWServer->Sound_At(GetPosition(), m_iSound_TurnStop, 0);
					}
					else
					{
						iSound = m_iSound_TurnStart;
						m_pWServer->Sound_At(GetPosition(), m_iSound_TurnStart, 0);
					}
				}

				CMat4Dfp32 Matrix, FinalMatrix, Inv;
				Matrix.SetZRotation(m_Heading_Min+HeadDiff*Amount); // why? pi/2
				Matrix.RotY_x_M(m_Pitch_Min+PitchDiff*Amount);
				Matrix.Multiply(GetPositionMatrix(), FinalMatrix);
				WantedDirection = CVec3Dfp32::GetRow(FinalMatrix, 0);
				//WantedDirection = CVec3Dfp32(1, 0, 0);


			}
		}

		if(m_iSound[0] != iSound)
		{
			m_DirtyMask |= CWO_DIRTYMASK_SOUND;
			m_pWServer->Sound_On(m_iObject, m_iSound[0], 0); // seams to be nessesary
			m_iSound[0] = iSound;
		}
	}

	fp32 TransformSpeed = 0.75f;

	if(!GetHandler().GotPower())
	{
		CMat4Dfp32 Matrix, FinalMatrix, Inv;
		Matrix.SetYRotation(0.15f);
		Matrix.Multiply(GetPositionMatrix(), FinalMatrix);
		WantedDirection = CVec3Dfp32::GetRow(FinalMatrix, 0);
		TransformSpeed = 0.1f;
		//WantedDirection
	}

	// static turrets can't rotate
	/*
	if(m_Static)
	{
		CMat4Dfp32 Pos = GetPositionMatrix();
		CVec3Dfp32 Direction = CVec3Dfp32::GetRow(Pos, 0);
		Direction.Normalize();
		fp32 Dot = WantedDirection*Direction;//CVec3Dfp32(1,0,0);

		if(0)
		if(Dot < 0.8f)
		{
			CVec3Dfp32 Temp;
			WantedDirection.Lerp(Direction, (-Dot+0.8f)*(1/1.8f)/2, Temp);
			WantedDirection = Temp;
			WantedDirection.Normalize();
		}

		WantedDirection.Normalize();
		m_AimDirection = WantedDirection;
	}
	
	else
	{
		*/

	CVec3Dfp32 Half;
	m_AimDirection.Lerp(WantedDirection, 0.5f, Half);
	Half.Normalize();

	m_AimDirection.Lerp(Half, TransformSpeed, m_AimDirection);
	m_AimDirection.Normalize();
	//}

	*((uint32*)&m_Data[2]) = *((uint32*)&m_Data[3]);
	*((uint32*)&m_Data[3]) = m_AimDirection.Pack32(1.1f);
	
	//m_Data_Direction0.Set(this, m_Data_Direction1.Get(this));
	//m_Data_Direction1.Set(this, m_AimDirection.Pack32(1.5f));
	m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);


/*
#ifdef RAIL_DEBUG
	m_pWServer->Debug_RenderWire(	CVec3Dfp32::GetRow(Matrix,3),
									CVec3Dfp32::GetRow(Matrix,3)+WantedDirection*1000, 0xFF00FF00, 0.06f);
	m_pWServer->Debug_RenderWire(	CVec3Dfp32::GetRow(Matrix,3),
									CVec3Dfp32::GetRow(Matrix,3)+m_AimDirection*1000, 0xFFFFFFFF, 0.06f);
#endif
*/
	CWObject_Interface_AI::OnRefresh();
}

void CWObject_RailWagon::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	_pFile->ReadLE(m_iHandler);
	_pFile->ReadLE(m_iPosition);
	_pFile->ReadLE(m_iNextPosition);
	_pFile->ReadLE(m_CurrentSpeed);
	_pFile->ReadLE(m_TimeScale);
	_pFile->ReadLE(m_Health);
	int32 Mode;
	_pFile->ReadLE(Mode);
	 m_Data_ModelMode.Set(this, Mode);
}

void CWObject_RailWagon::OnDeltaSave(CCFile* _pFile)
{
	_pFile->WriteLE(m_iHandler);
	_pFile->WriteLE(m_iPosition);
	_pFile->WriteLE(m_iNextPosition);
	_pFile->WriteLE(m_CurrentSpeed);
	_pFile->WriteLE(m_TimeScale);
	_pFile->WriteLE(m_Health);
	int32 Mode = m_Data_ModelMode.Get(this);
	_pFile->WriteLE(Mode);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_RailWagon, CWObject_Interface_AI, 0x0100);






// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------



//
//
//
CWObject_RailHandler::CWObject_RailHandler()
{
	// init data
	m_pDirNodes = NULL;
	m_pNodes = NULL;
	m_pEntryNodes = NULL;

	m_GotPower = true;
	m_PowerLevel = 1; // maximum power

	m_iScriptedTarget = 0;
}


CWObject_RailHandler::~CWObject_RailHandler()
{
	if(m_pDirNodes)
		delete [] m_pDirNodes;

	if(m_pNodes)
		delete [] m_pNodes;

	if(m_pEntryNodes)
		delete [] m_pEntryNodes;
}

//
//
//
void CWObject_RailHandler::OnCreate()
{
//	M_TRACEALWAYS("-- MESSAGE: Railhandler created\n");

	// Register rail system to the game
	CWObject_Message Msg(OBJMSG_RAIL_REGISTER);
	Msg.m_iSender = m_iObject;
	m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());

	//
//	int32 NumNodes = 0;

	// Set physics so it won't be selected
	CWO_PhysicsState Phys;
	Phys.m_ObjectFlags = OBJECT_FLAGS_WORLD;
	Phys.m_PhysFlags = OBJECT_FLAGS_WORLD;
	Phys.m_iExclude = ~0;
	m_pWServer->Object_SetPhysics(m_iObject, Phys);

	// Do a selection on the "usable" type which should all be rails
	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pWServer->Selection_AddClass(Selection, "Rail");
	
	// get selection
	const int16 *pSel;
	int32 nSel = m_pWServer->Selection_Get(Selection, &pSel);
	

	// Sound stuff
	m_SoundPeriod = int(2.0f * m_pWServer->GetGameTicksPerSecond());
	m_iLastPlaySound = -1;
	m_LastPlaySoundTick = -1;
	m_CanFireTick = -1;
	//
	CRailSystem RailSystem;

	//
	// 1. Load path for the rail
	// 2. Add paths to this object
	// 3. Remove the rail if not this object
	//
	int32 DeletedRailCount = 0;
	for(int32 i = 0; i < nSel; i++)
	{
		// get rail
		int16 iObject = pSel[i];
		CWObject_Rail *pRail = safe_cast<CWObject_Rail>(m_pWServer->Object_Get(iObject));

		// loads the rail
		CWO_PosHistory *KeyFrames = pRail->GetClientData(pRail);
		if(pRail->m_iAnim0 != -1 && !KeyFrames->IsValid())
			pRail->LoadPath(m_pWServer, pRail, pRail->m_iAnim0);

		// get num keys
		int32 NumKeys = 0;
		if(KeyFrames->m_lSequences.Len() > 0)
			NumKeys = KeyFrames->m_lSequences[0].GetNumKeyframes();

		// add paths
		for(int32 k = 1; k < NumKeys; k++)
		{
			CMat4Dfp32 Matrix, PrevMatrix;
			KeyFrames->m_lSequences[0].GetMatrix(k-1, PrevMatrix);
			KeyFrames->m_lSequences[0].GetMatrix(k, Matrix);
			RailSystem.AddConnection(PrevMatrix, Matrix, KeyFrames->m_lSequences[0].GetFlags(k)&1); // KeyFrames->GetFlags(i)
		}

		// remove rail
		m_pWServer->Object_DestroyInstant(iObject);
		DeletedRailCount++;
	}

	// check if we got any data at all
	if(!RailSystem.m_lConnections.Len())
	{
		//m_pWServer->Object_DestroyInstant(m_iObject);
		return;
	}

	//
	RailSystem.ConstructExtraPoints();
	RailSystem.RemoveUnnessesaryData();
	RailSystem.ConstructMatrices();

	//
	TArray<CRailSystem::CPoint> &lPoints = RailSystem.m_lPoints;
	TArray<CRailSystem::CConnection> &lConnections = RailSystem.m_lConnections;

	// Build final data
	{
		//
		m_NumNodes = lPoints.Len();
		
		m_NumDirNodes = 0;
		for(int32 i = 0; i < lPoints.Len(); i++)
			m_NumDirNodes = int32(m_NumDirNodes + RailSystem.GetNumConnections(i));

		// Find entry nodes
		m_NumEntryNodes = 0;
		for(int32 i = 0; i < lPoints.Len(); i++)
		{
			if(RailSystem.GetNumConnections(i) == 1)
				m_NumEntryNodes++;
		}

		// allocate data
		m_pNodes = DNew(CNode) CNode[m_NumNodes];
		m_pDirNodes = DNew(CDirNode) CDirNode[m_NumDirNodes];

		if(m_NumEntryNodes)
			m_pEntryNodes = DNew(NodeIndex) NodeIndex[m_NumEntryNodes];
		else
		{
			// pseudo entry
			m_NumEntryNodes++;
			m_pEntryNodes = DNew(NodeIndex) NodeIndex[m_NumEntryNodes];
			m_pEntryNodes[0] = 0;
		}	

		// build nodes
		DirNodeIndex iDirNodeCounter = 0;
		NodeIndex iEntryNodeCounter = 0;
		for(int32 i = 0; i < m_NumNodes; i++)
		{
			int32 nConnections = int32(RailSystem.GetNumConnections(i));

			m_pNodes[i].m_Position = CVec3Dfp32::GetMatrixRow(lPoints[i].m_Matrix,3);
			m_pNodes[i].m_UpVector = CVec3Dfp32::GetMatrixRow(lPoints[i].m_Matrix,0);
			
			CVec3Dfp32 Dir = CVec3Dfp32::GetMatrixRow(lPoints[i].m_RailMatrix,0);
			m_pNodes[i].m_Direction = Dir;

			m_pNodes[i].m_NumDirNodes = nConnections;
			m_pNodes[i].m_iDirNodeStart = iDirNodeCounter;
			m_pNodes[i].m_Priority = 0;

			// build direction nodes
			int32 c = -1;
			for(int32 d = 0; d < nConnections; d++)
			{
				//m_pDirNodes[iDirNodeCounter+d].m_iThis = i;
				for(c++; c < lConnections.Len(); c++)
				{
					if(lConnections[c].m_aiPoints[0] == i)
					{
						m_pDirNodes[iDirNodeCounter].m_iNext = lConnections[c].m_aiPoints[1];
						iDirNodeCounter++;
						break;
					}
					else if(lConnections[c].m_aiPoints[1] == i)
					{
						m_pDirNodes[iDirNodeCounter].m_iNext = lConnections[c].m_aiPoints[0];
						iDirNodeCounter++;
						break;
					}
				}
			}

			// add it to the entry list if it's an entry node
			if(nConnections == 1)
			{
				m_pEntryNodes[iEntryNodeCounter] = i;
				iEntryNodeCounter++;

				m_pNodes[i].m_Flags |= CNode::FLAG_ENTRY;
			}

			//iDirNodeCounter += nConnections;
		}
	}

	//
/*	M_TRACEALWAYS("-- MESSAGE: %d railes removed\n", DeletedRailCount);
	M_TRACEALWAYS("-- MESSAGE: %d nodes\n", m_NumNodes);
	M_TRACEALWAYS("-- MESSAGE: Rail memory usage %d (nodesize=%dx%d, dirnodesize=%d)\n",
		m_NumNodes*sizeof(CNode)+sizeof(CDirNode)*m_NumDirNodes,sizeof(CNode), m_NumDirNodes, sizeof(CDirNode));*/

	/////////////////////////////////////////////////////////////////////////////////

	// precache
	//m_pWServer->GetMapData()->GetResourceIndex_Class("RailWagon_Turret");
	//m_pWServer->GetMapData()->GetResourceIndex_Class("RailWagon_Transport");
	//m_pWServer->GetMapData()->GetResourceIndex_Class("RailWagon_Camera");
}

void CWObject_RailHandler::SetPower(bool _State)
{
	if (_State == true)
	{
		// Clear all wagons knowledge, clear all processed dead
		m_lEnemies.Clear();
		m_lProcessedDead.Clear();
		for (int i = 0; i < m_lWagons.Len(); i++)
		{
			CWObject_RailWagon &Wagon = GetWagonFromObj(m_lWagons[i]);
			if (&Wagon)
			{
				Wagon.ClearKnowledge();
			}
		}
		m_CanFireTick = -1;
		m_GotPower = true;
	}
	else
	{	// Clear all wagons knowledge, clear all processed dead
		m_lEnemies.Clear();
		m_lProcessedDead.Clear();
		for (int i = 0; i < m_lWagons.Len(); i++)
		{
			CWObject_RailWagon &Wagon = GetWagonFromObj(m_lWagons[i]);
			if (&Wagon)
			{
				Wagon.ClearKnowledge();
			}
		}
		m_CanFireTick = -1;
		m_GotPower = false;
	}
};

bool CWObject_RailHandler::GotPower()
{
	return(m_GotPower);
};

bool CWObject_RailHandler::CanPlaySound(int _iSound)
{
	int Tick = m_pWServer->GetGameTick();
	if ((Tick  > m_LastPlaySoundTick + m_SoundPeriod)||(_iSound > m_iLastPlaySound))
	{
		m_iLastPlaySound = _iSound;
		m_LastPlaySoundTick = Tick;
		return(true);
	}
	else
	{
		return(false);
	}
}

bool CWObject_RailHandler::CanFire()
{
	int Tick = m_pWServer->GetGameTick();
	if (m_CanFireTick != -1)
	{
		if (Tick >= m_CanFireTick)
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}
	else
	{
		m_CanFireTick = Tick + 40;
		return(false);
	}
};

void CWObject_RailHandler::AddUniqueProcessedDead(int _iDead)
{
	for (int i = 0; i < m_lProcessedDead.Len(); i++)
	{
		if (m_lProcessedDead[i] == _iDead)
		{
			return;
		}
	}
	m_lProcessedDead.Add(_iDead);
}

// Makes _iUnfriendly relation UNFRIENDLY for all rail KBS and also
// removes him from enemies list
void CWObject_RailHandler::SetUnfriendly(int _iUnfriendly)
{
	if (_iUnfriendly == m_iScriptedTarget)
	{
		return;
	}

	for (int i = m_lEnemies.Len()-1; i >= 0; i--)
	{
		if (m_lEnemies[i] == _iUnfriendly)
		{
			m_lEnemies.Del(i);
		}
	}

	for (int i = 0; i < m_lWagons.Len(); i++)
	{
		CWObject_RailWagon &Wagon = GetWagonFromObj(m_lWagons[i]);
		if ((&Wagon) && (Wagon.m_spAI))
		{
			CAI_AgentInfo* pEnemy = Wagon.m_spAI->m_KB.GetAgentInfo(_iUnfriendly);
			if ((pEnemy) && (pEnemy->GetCurRelation() != CAI_AgentInfo::UNFRIENDLY))
			{	// No delay
				pEnemy->SetRelation(CAI_AgentInfo::UNFRIENDLY);
			}
		}
	}
};

void CWObject_RailHandler::AddUniqueEnemy(int _iEnemy)
{
	if (_iEnemy == m_iScriptedTarget)
	{
		return;
	}

	for (int i = 0; i < m_lEnemies.Len(); i++)
	{
		if (m_lEnemies[i] == _iEnemy)
		{
			return;
		}
	}
	m_lEnemies.Add(_iEnemy);
	for (int i = 0; i < m_lWagons.Len(); i++)
	{
		CWObject_RailWagon &Wagon = GetWagonFromObj(m_lWagons[i]);
		if ((&Wagon) && (Wagon.m_spAI))
		{
			CAI_AgentInfo* pEnemy = Wagon.m_spAI->m_KB.GetAgentInfo(_iEnemy);
			if (!pEnemy)
			{
				pEnemy = Wagon.m_spAI->m_KB.AddAgent(_iEnemy);
			}
			if ((pEnemy) && (pEnemy->GetCurRelation() < CAI_AgentInfo::ENEMY))
			{
				pEnemy->SetRelation(CAI_AgentInfo::ENEMY);
			}
		}
	}
}

// Returns true if all wagons but _pCallWagon have their m_ScanTime == 0
bool CWObject_RailHandler::AllScanLost(const CWObject_RailWagon* _pCallWagon)
{
	for (int i = 0; i < m_lWagons.Len(); i++)
	{
		CWObject_RailWagon &Wagon = GetWagonFromObj(m_lWagons[i]);
		if (!(&Wagon)) {continue;}
		if (&Wagon == _pCallWagon) {continue;}
		if (Wagon.m_ScanTime)
		{
			return(false);
		}
	}

	return(true);
};

//
//
//
CWObject_RailWagon &CWObject_RailHandler::GetWagonFromObj(const int32 _iObj)
{
	M_ASSERT(_iObj!=-1, "Can't pass -1 as object index");
	CWObject_RailWagon *pWagon = safe_cast<CWObject_RailWagon>(m_pWServer->Object_Get(_iObj));
	M_ASSERT(pWagon, "Couldn't find wagon object");
	return *pWagon;
}

//
//
//
CWObject_RailWagon &CWObject_RailHandler::GetWagon(const WagonIndex _Index)
{
	AssertWagon(_Index);
	return GetWagonFromObj(m_lMobileWagons[_Index]);
}

//
//
//
int32 CWObject_RailHandler::NumWagons()
{
	return m_lMobileWagons.Len();
}

//
//
//
CWObject_RailWagon *CWObject_RailHandler::CreateWagon(const char *_pName, const NodeIndex _iStart, const NodeIndex _iDirection)
{
	M_ASSERT(m_pWServer, "No server!");
	M_ASSERT(CheckConnectivity(_iStart, _iDirection), "Nodes are not connected"); // will assert on errorus nodes aswell

	// create inital position matrix
	CVec3Dfp32 StartPosition = m_pNodes[_iStart].m_Position;
	CMat4Dfp32 Matrix;
	Matrix.Unit();
	StartPosition.SetMatrixRow(Matrix, 3);

	// create object
	int32 iObject = -1;
	iObject = m_pWServer->Object_Create(_pName, Matrix);

	if(iObject == -1)
		return NULL;

	//M_ASSERT(iObject!=-1, "Error creating Wagon object");
	
	// add wagon to list
	CWObject_RailWagon &rWagon = GetWagonFromObj(iObject);
	if (&rWagon)
	{
		m_lMobileWagons.Add(rWagon.m_iObject);

		// init wagon parameters
		rWagon.m_iHandler = m_iObject;
		rWagon.m_iPosition = _iStart;
		rWagon.m_iNextPosition = _iDirection;
		rWagon.m_TravelTime = 0;
		rWagon.m_Flags = 0;
		rWagon.m_TimeScale = GetTimeScale(rWagon.m_iPosition, rWagon.m_iNextPosition);
		rWagon.m_CurrentSpeed = 0;

		static PriorityIndex Pri = 1;
		rWagon.m_Priority = Pri;
		Pri++;

		rWagon.m_iSound[0] = rWagon.m_iSound_Engine;
		m_pWServer->Sound_On(rWagon.m_iObject, rWagon.m_iSound[0], 2);
	}

	return &rWagon;
}

//
//
//
void CWObject_RailHandler::DeleteWagon(const WagonIndex _iWagon)
{
	AssertWagon(_iWagon);
	CWObject_RailWagon &rWagon = GetWagon(_iWagon);
	m_lMobileWagons.Del(_iWagon);
	m_pWServer->Object_Destroy(rWagon.m_iObject);
}

//
// returns true if nodes are connected
//
const bool CWObject_RailHandler::CheckConnectivity(const uint16 _iNode1, const uint16 _iNode2)
{
	AssertInit();
	AssertNode(_iNode1); AssertNode(_iNode2); // Assert on node error

	for(int32 i = 0; i < m_pNodes[_iNode1].m_NumDirNodes; i++)
	{
		if(m_pDirNodes[m_pNodes[_iNode1].m_iDirNodeStart+i].m_iNext == _iNode2)
			return true;
	}

	return false;
}

//
//
//
fp32 CWObject_RailHandler::GetTimeScale(const NodeIndex _iStart, const NodeIndex _iNext)
{
	M_ASSERT(CheckConnectivity(_iStart, _iNext), "Nodes are not connected"); // will assert on errorus nodes aswell
	fp32 Length = (m_pNodes[_iNext].m_Position - m_pNodes[_iStart].m_Position).Length();
	return (1/Length);
}

//
//
//
int32 CWObject_RailHandler::GetPotentialNodes(const NodeIndex _iThis, const NodeIndex _iPrev, NodeIndex _aiNodes[], int32 _Max)
{
	// fin entry angle
	CVec3Dfp32 EntryDir = (m_pNodes[_iThis].m_Position-m_pNodes[_iPrev].m_Position);
	EntryDir.Normalize();

	CNode &rNode = m_pNodes[_iThis];
	int32 NumFoundNodes = 0;

	for(int32 p = 0; p < rNode.m_NumDirNodes; p++)
	{
		if(m_pDirNodes[rNode.m_iDirNodeStart+p].m_iNext != _iPrev)
		{
			CVec3Dfp32 Direction = m_pNodes[m_pDirNodes[rNode.m_iDirNodeStart+p].m_iNext].m_Position-m_pNodes[_iThis].m_Position;
			Direction.Normalize();

			// we can't to sharp turns
			if(EntryDir*Direction > 0)
			{
				_aiNodes[NumFoundNodes] = m_pDirNodes[rNode.m_iDirNodeStart+p].m_iNext;
				NumFoundNodes++;

				if(NumFoundNodes == _Max)
					break;
			}
		}
	}

	return NumFoundNodes;
}

//
//
//
void CWObject_RailHandler::Prioritize(const NodeIndex _iNode)
{
	AssertNode(_iNode);

	CNode &rNode = GetNode(_iNode);
	rNode.m_Priority = 0;

	for(int32 w = 0; w < NumWagons(); w++)
	{
		CWObject_RailWagon &rWagon = GetWagon(w);

		// check if this got larger priority
		if(rWagon.m_Priority < rNode.m_Priority)
			continue;

		for(int32 p = 0; p < rWagon.m_lPlannedPath.Len(); p++)
		{
			if(rWagon.m_lPlannedPath[p] == _iNode)
			{
				rNode.m_Priority = rWagon.m_Priority;
				break;
			}
		}
	}
}

//
//
//
template<class T> 
T Bezier(T _Value1, T _Value2, T _Value3, fp32 _Amount) 
{
	return _Value3*(_Amount*_Amount) + _Value2*(2*_Amount*(1-_Amount)) + _Value1*((1-_Amount)*(1-_Amount));
}

template<class T>
T Hermite(T _Value1, T _Tanget1, T _Value2, T _Tanget2, fp32 _Amount)
{
	fp32 a = _Amount;
	fp32 a2 = _Amount*_Amount;
	fp32 a3 = a2*_Amount;
	fp32 h1 =  2*a3 - 3*a2 + 1;
	fp32 h2 = -2*a3 + 3*a2;
	fp32 h3 =  a3 - 2*a2 + a;
	fp32 h4 = -a3 + a2;

	return _Value1*h1 + _Value2*h2 + _Tanget1*h3 + _Tanget2*h4;
}

CMat4Dfp32 CWObject_RailHandler::GetMatrix(const NodeIndex _iStart, const NodeIndex _iDirection, fp32 _Time)
{
	AssertInit();
	AssertNode(_iStart); AssertNode(_iDirection); // Assert on node error
   //CWObject_RailHandler &rHandler = GetHandler();

	// nodes
	CNode &rStart = m_pNodes[_iStart];
	CNode &rEnd = m_pNodes[_iDirection];

	// the stuff that we need
	CVec3Dfp32 Position, UpVector, Direction;

	// position and up vector
	rStart.m_UpVector.Lerp(rEnd.m_UpVector, _Time, UpVector); // OK!
	UpVector.Normalize();
	CVec3Dfp32 Delta = rEnd.m_Position-rStart.m_Position;

	// Bended
	CVec3Dfp32 aDirs[] = {rStart.m_Direction, rEnd.m_Direction};
	CVec3Dfp32 aWantedDirs[] = {Delta, Delta*-1};
	fp32 Length = Delta.Length()*1.25f;
	
	if(aDirs[0]*aWantedDirs[0] < 0) aDirs[0] = aDirs[0]*-1.0f;
	if(aDirs[1]*aWantedDirs[1] < 0) aDirs[1] = aDirs[1]*-1.0f;

	// do fakie model
	
	// Hermite
	CVec3Dfp32 aControlPoints[] = {aDirs[0]*Length, aDirs[1]*Length};
	Position = Hermite(rStart.m_Position, aControlPoints[0], rEnd.m_Position, aControlPoints[1], _Time);
	CVec3Dfp32 Pos2 = Hermite(rStart.m_Position, aControlPoints[0], rEnd.m_Position, aControlPoints[1], _Time+0.01f);

	Direction = Pos2-Position;
	Direction.Normalize();

	//
	if(Direction*(rEnd.m_Position-rStart.m_Position) < 0)
		Direction = Direction * -1;

	// build matrix
	CMat4Dfp32 Result;
	Result.Unit();
	Direction.SetMatrixRow(Result, 0);	// Set direction (OK!)
	UpVector.SetMatrixRow(Result, 2);	// Set Up vector (OK!)
	Result.RecreateMatrix(0, 2);		// Recreate (OK!)
	Position.SetRow(Result, 3);			// Position (OK!)

	return Result;
}


//
//
//
CWObject_RailMessagePoint *CWObject_RailHandler::GetMessagePoint(CVec3Dfp32 _Point, const char *_pName)
{
	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pWServer->Selection_AddClass(Selection, "RailMessagePoint");
	const int16 *pPoints;
	int32 nSel = m_pWServer->Selection_Get(Selection, &pPoints);

	fp32 Length = 10000000;
	int16 iObj = -1;
	CStr WagonName = _pName;
	WagonName.MakeUpperCase();
	for(int32 i = 0; i < nSel; i++)
	{
		CWObject_RailMessagePoint *pPoint = safe_cast<CWObject_RailMessagePoint>(m_pWServer->Object_Get(pPoints[i]));

		for(int32 u = 0; u < pPoint->m_lUsers.Len(); u++)
		{
			if(pPoint->m_lUsers[u] == WagonName)
			{
				fp32 ThisLength = (m_pWServer->Object_GetPosition(pPoints[i])-_Point).Length();
				if(ThisLength < Length)
				{
					Length = ThisLength;
					iObj = pPoints[i];
				}
				break;
			}
		}
	}

	if(iObj != -1)
		return safe_cast<CWObject_RailMessagePoint>(m_pWServer->Object_Get(iObj));

	return NULL;
}

//
//
//
void CWObject_RailHandler::SendMessages(CWObject_RailWagon *_pWagon, CVec3Dfp32 _Point, int32 _Type, int32 _iActivator)
{
	M_ASSERT(_pWagon, "Please don't do this to my. WHYYYYYYY?");

	CWObject_RailMessagePoint* pPoint = GetMessagePoint(_Point, _pWagon->GetName());
	if(!pPoint)
		return;

	for(int32 i = 0; i < pPoint->m_lMessages[_Type].Len(); i++)
	{
		pPoint->m_lMessages[_Type][i].SendMessage(_pWagon->m_iObject, _iActivator, m_pWServer);
	}
}

//
//
//
NodeIndex CWObject_RailHandler::GetClosestNode(CVec3Dfp32 &_Point)
{
	M_ASSERT(m_pNodes, "No node data");
	
	NodeIndex iNode = RAIL_INVALIDNODE;
	fp32 Length = 10000000;
	for(int32 i = 0; i < m_NumNodes; i++)
	{
		fp32 ThisLength = (m_pNodes[i].m_Position-_Point).Length();
		if(ThisLength < Length)
		{
			Length = ThisLength;
			iNode = i;
		}
	}

	return iNode;
}

//
//
//
bool CWObject_RailHandler::GetTargetsByName(int _iSender,int _iObject,CStr _Names,TArray<int16> _liTargets)
{
	int iSpecialTarget = CWO_SimpleMessage::ResolveSpecialTargetName(_Names);
	if (iSpecialTarget >= 0)
	{
		if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ACTIVATOR)
		{	//Spot activator, which should be the supplied onject ID
			_liTargets.Add(_iObject);
			return(true);
		}
		else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_THIS)
		{
			_liTargets.Add(_iSender);
			return(true);
		}
		else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_PLAYER)
		{
			bool bFoundPlayer = false;
			CWObject* pChar;
			CWObject_Game *pGame = m_pWServer->Game_GetObject();
			for (int i = 0; i < pGame->Player_GetNum(); i++)
			{
				if ((pChar = pGame->Player_GetObject(i)))
				{
					_liTargets.Add(pChar->m_iObject);
					bFoundPlayer = true;
				}
			};
			if (bFoundPlayer)
			{
				return(true);
			}
			else
			{
				return(false);
			}
		}
		else
		{
			return(false);
		};
	}
	else
	{
		int N = _liTargets.Len();
		CAI_Core::GetTargetIndices(_Names,m_pWServer,_liTargets);
		if (_liTargets.Len() > N)
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}
};

aint CWObject_RailHandler::OnMessage(const CWObject_Message &_Msg)
{
	if(_Msg.m_Msg == OBJMSG_RAIL_SPAWN)
	{
		CFStr Str = (char*)_Msg.m_pData;
		CFStr WagonType = Str.GetStrSep(",");
		CFStr WagonName = Str.GetStrSep(",");
		CFStr EntryName = Str.GetStrSep(",");
		CFStr DirectionName = Str.GetStrSep(",");

		int iTarget = m_pWServer->Selection_GetSingleTarget(EntryName);
		if (iTarget <= 0)
			return 0;

		// get start node
		CMat4Dfp32 TargetMatrix = m_pWServer->Object_GetPositionMatrix(iTarget);
		CVec3Dfp32 TargetSpawnPoint = CVec3Dfp32::GetRow(TargetMatrix, 3);
		CVec3Dfp32 TargetSpawnDirection = CVec3Dfp32::GetRow(TargetMatrix, 0);
		NodeIndex iNode = GetClosestNode(TargetSpawnPoint);

		if(iNode == RAIL_INVALIDNODE)
			return 0;

		CNode *pNode = &m_pNodes[iNode];

		// get direction
		DirNodeIndex iStart = pNode->m_iDirNodeStart;
		fp32 BestAngle = -2;
		NodeIndex iNextNode = RAIL_INVALIDNODE;
		for(int32 i = 0; i < pNode->m_NumDirNodes; i++)
		{
			//TargetSpawnDirection
			NodeIndex iNext = m_pDirNodes[iStart + i].m_iNext;
			CVec3Dfp32 Dir = m_pNodes[iNext].m_Position-pNode->m_Position;
			Dir.Normalize();
			fp32 ThisAngle = Dir*TargetSpawnDirection;

			if(ThisAngle > BestAngle)
			{
				BestAngle = ThisAngle;
				iNextNode = iNext;
			}
		}

		if(iNextNode == RAIL_INVALIDNODE)
			return 0;

		// Spawn time
		CWObject_RailWagon *pWagon = CreateWagon(WagonType, iNode, iNextNode);
		if(pWagon)
		{
			// wagon sucessfully created
			m_pWServer->Object_SetName(pWagon->m_iObject,WagonName);
		}

		//
		return 1;
	}
	else if(_Msg.m_Msg == OBJMSG_RAIL_NUMBYTYPE)
	{
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddClass(Selection, "RailWagon");
		int32 nSel = m_pWServer->Selection_Get(Selection);
		return nSel;
	}
	else if(_Msg.m_Msg == OBJMSG_RAIL_NUMBYNAME)
	{
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddClass(Selection, "RailWagon");

		const int16 *pBots;
		int32 nSel = m_pWServer->Selection_Get(Selection, &pBots);

		int32 Count = 0;
		for(int32 i = 0; i < nSel; i++)
		{
			CWObject *pObj = m_pWServer->Object_Get(pBots[i]);
			if(!pObj)
				continue;

			if(CStrBase::stricmp((char *)_Msg.m_pData, pObj->GetName()) == 0)
				Count++;
		}
		return Count;
	}
	else if(_Msg.m_Msg == OBJMSG_RAIL_POWER)
	{
		SetPower(_Msg.m_Param0 != 0);
	}
	else if (_Msg.m_Msg == OBJMSG_RAIL_ENEMY)
	{
		CStr Str = CStr((char*)_Msg.m_pData);
		TArray<int16> lTargets;
		//Param1 is activator
		if (GetTargetsByName(_Msg.m_iSender,_Msg.m_Param1,Str,lTargets))
		{
			for (int i = 0; i < lTargets.Len(); i++)
			{
				AddUniqueEnemy(lTargets[i]);
			}
			return(lTargets.Len());
		}
		else
		{
			return(0);
		}
	}
	else if (_Msg.m_Msg == OBJMSG_RAIL_UNFRIENDLY)
	{
		CStr Str = CStr((char*)_Msg.m_pData);
		TArray<int16> lTargets;
		//Param1 is activator
		if (GetTargetsByName(_Msg.m_iSender,_Msg.m_Param1,Str,lTargets))
		{
			for (int i = 0; i < lTargets.Len(); i++)
			{
				SetUnfriendly(lTargets[i]);
			}
			return(lTargets.Len());
		}
		else
		{
			return(0);
		}
	}
	else if (_Msg.m_Msg == OBJMSG_RAIL_ATTACK)
	{
		CStr Str = CStr((char*)_Msg.m_pData);
		TArray<int16> lTargets;
		//Param1 is activator
		if (GetTargetsByName(_Msg.m_iSender,_Msg.m_Param1,Str,lTargets))
		{	// Pick a random target
			int iPick = int(Random * lTargets.Len() * 0.999f);
			m_iScriptedTarget = lTargets[iPick];
			return(1);
		}
		else
		{
			m_iScriptedTarget = 0;
			return(0);
		}
	}
	else if(_Msg.m_Msg == OBJSYSMSG_PRECACHEMESSAGE)
	{
		// PRECACHE
		CWObject_Message *pMsg = (CWObject_Message *)_Msg.m_pData;

		if(pMsg->m_Msg == OBJMSG_RAIL_SPAWN)
		{
			CFStr Str = (char*)pMsg->m_pData;
			CFStr WagonType = Str.GetStrSep(",");
			m_pWServer->GetMapData()->GetResourceIndex_Class(WagonType);
		}
	}



	return CWObject::OnMessage(_Msg);
}

//
//
//
void CWObject_RailHandler::OnRefresh()
{
	if(!ShouldUpdate())
		return;

	static fp32 Time = 0;
	static fp32 LoopTime = 0;

	Time += m_pWServer->GetGameTickTime();
	LoopTime += m_pWServer->GetGameTickTime() * 0.25f;
	while(LoopTime > 1)
		LoopTime -= 1;

	// Adjust power level
	if(m_PowerLevel < 1 && GotPower())
		m_PowerLevel += 0.02f;
	else if(m_PowerLevel > 0 && !GotPower())
		m_PowerLevel -= 0.02f;

	m_PowerLevel = Clamp01(m_PowerLevel);
	
	// remove wagons that are marked for removal
	for(int32 i = 0; i < NumWagons(); i++)
	{
		CWObject_RailWagon &rWagon = GetWagon(i);
		if(rWagon.m_Flags&CWObject_RailWagon::FLAG_REMOVE)
		{
			DeleteWagon(i);
			i--;
		}
	}

	//
	// Debug render stuff
	//
	#ifdef RAIL_DEBUG
	{
		for(int32 i = 0; i < m_NumNodes; i++)
		{
			CVec3Dfp32 Pos = m_pNodes[i].m_Position- CVec3Dfp32(0,0,16);
			/*
			// direction of the node
			m_pWServer->Debug_RenderWire(Pos - CVec3Dfp32(0,0,4), Pos + CVec3Dfp32(0,0,4), 0xFFFF0000, 1);
			m_pWServer->Debug_RenderWire(Pos - CVec3Dfp32(0,4,0), Pos + CVec3Dfp32(0,4,0), 0xFFFF0000, 1);
			m_pWServer->Debug_RenderWire(Pos - CVec3Dfp32(4,0,0), Pos + CVec3Dfp32(4,0,0), 0xFFFF0000, 1);

			m_pWServer->Debug_RenderWire(Pos, Pos+m_pNodes[i].m_Direction*16, 0xFF00FF00, 1);
			*/

			for(int32 d = 0; d < m_pNodes[i].m_NumDirNodes; d++)
			{
				NodeIndex iNextNode = m_pDirNodes[m_pNodes[i].m_iDirNodeStart+d].m_iNext;
				CVec3Dfp32 EndPos = m_pNodes[iNextNode].m_Position - CVec3Dfp32(0,0,16);
				
				//if(m_pNodes[m_pNodes[i].m_aConnections[c]].m_DebugFlags && m_pNodes[i].m_DebugFlags)
				//	m_pWServer->Debug_RenderWire(Start, End, 0xFEFF0000);
				//else

				// rail path
				for(fp32 n = 0; n < 0.5f; n+= 0.1f)
				{
					fp32 nn = n + 0.1f;
					CMat4Dfp32 ThisMatrix = GetMatrix(i, iNextNode, n);
					CMat4Dfp32 NextMatrix = GetMatrix(i, iNextNode, nn);

					m_pWServer->Debug_RenderWire(	CVec3Dfp32::GetRow(ThisMatrix, 3) - CVec3Dfp32(0,0,8),
													CVec3Dfp32::GetRow(NextMatrix, 3) - CVec3Dfp32(0,0,8),
													0xFF00FF00, 1);
				}

				//m_pWServer->Debug_RenderWire(Pos, EndPos, 0xFF00FF00);
				
				// matrix debug
				CMat4Dfp32 Matrix = GetMatrix(i, iNextNode, LoopTime); //(M_Sin(Time)+1)/2);
				CVec3Dfp32 Center = CVec3Dfp32::GetRow(Matrix, 3);
				CVec3Dfp32 Direction = CVec3Dfp32::GetRow(Matrix, 0);
				CVec3Dfp32 UpVector = CVec3Dfp32::GetRow(Matrix, 2);
				CVec3Dfp32 aPos[] = { Center+Direction*8, Center-Direction*8 };
				
				m_pWServer->Debug_RenderWire(aPos[0]-UpVector*16, aPos[1]-UpVector*16, 0xFFFF0000, SERVER_TIMEPERFRAME*1.5f);
				m_pWServer->Debug_RenderWire(Center-UpVector*16, Center, 0xFF0000FF, SERVER_TIMEPERFRAME*1.5f);
			}
		}
	}
	#endif
}

// Load and store processed dead and changed relations
void CWObject_RailHandler::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	int32 N,Temp32;
	int8 Temp8;

	// Save power members
	_pFile->ReadLE(Temp8);
	if (Temp8 != 0)
	{
		SetPower(true);
	}
	else
	{
		SetPower(false);
	}

	m_lProcessedDead.Clear();
	m_lEnemies.Clear();
	
	_pFile->ReadLE(N);
	for (int i = 0; i < N; i++)
	{
		_pFile->ReadLE(Temp32);
		/* *** Reinsert code when GUID works ***
		CWObject* pObj = m_pWServer->Object_GetWithGUID(Temp32);
		if (pObj)
		{
			Temp32 = pObj->m_iObject;
		}
		*/
		m_lProcessedDead.Add(Temp32);
	};
	_pFile->ReadLE(N);
	for (int i = 0; i < N; i++)
	{
		_pFile->ReadLE(Temp32);
		/* *** Reinsert code when GUID works *** 
		CWObject* pObj = m_pWServer->Object_GetWithGUID(Temp32);
		if (pObj)
		{
			Temp32 = pObj->m_iObject;
		}
		*/
		m_lEnemies.Add(Temp32);
	};
	_pFile->ReadLE(Temp32);
	m_iScriptedTarget = Temp32;
};

void CWObject_RailHandler::OnDeltaSave(CCFile* _pFile)
{
	int32 Temp32;
	int8 Temp8;

	// Save power members
	Temp8 = GotPower(); _pFile->WriteLE(Temp8);
	
	// Save processed dead
	Temp32 = m_lProcessedDead.Len(); _pFile->WriteLE(Temp32);
	for (int i = 0; i < m_lProcessedDead.Len(); i++)
	{
		Temp32 = m_lProcessedDead[i];
		/* *** Reinsert code when GUID works ***
		CWObject* pObj = m_pWServer->Object_Get(Temp32);
		if (pObj)
		{
			Temp32 = m_pWServer->Object_GetGUID(pObj->m_iObject);
		}
		*/
		_pFile->WriteLE(Temp32);
	};
	
	//Should we remember enemies?
#ifndef AI_PB_PC
	if (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_RAIL_FORGETENEMIES), m_pWServer->Game_GetObjectIndex()))
	{
		//No enemies are saved
		Temp32 = 0; _pFile->WriteLE(Temp32);
	}
	else
#endif
	{
		Temp32 = m_lEnemies.Len(); _pFile->WriteLE(Temp32);
		for (int i = 0; i < m_lEnemies.Len(); i++)
		{	
			Temp32 = m_lEnemies[i];
			/* *** Reinsert code when GUID works ***
			CWObject* pObj = m_pWServer->Object_Get(Temp32);
			if (pObj)
			{
				Temp32 = m_pWServer->Object_GetGUID(pObj->m_iObject);
			}
			*/
			_pFile->WriteLE(Temp32);
		};
	}

	Temp32 = m_iScriptedTarget;
	_pFile->WriteLE(Temp32);
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_RailHandler, CWObject, 0x0100);

//
// 
//
void CWObject_Rail::OnSpawnWorld()
{
	M_ASSERT(m_pWServer, "No server!");

	//Rail handler is created in gameP4 onspawnworld nowadays...
	// the rail handler will "eat up" all rails
	//m_pWServer->Object_Create("RailHandler", CMat4Dfp32());
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Rail, CWObject_Engine_Path, 0x0100);
//*/
