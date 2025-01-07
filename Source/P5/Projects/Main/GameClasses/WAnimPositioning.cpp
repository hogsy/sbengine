/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Animation positioning
					
	Author:			Olle Rosenquist
					
	Copyright:		2003 Starbreeze Studios AB
					
	Contents:		

	Comments:
					
	History:		
		030912:		Created

\*____________________________________________________________________________________________*/

#include "PCH.h"
#include "WAnimPositioning.h"
#include "WObj_Misc/WObj_ActionCutscene.h"

CRelativeAnimPos::CAnimPosEntry CRelativeAnimPos::m_slPosEntryTable[RELATIVEANIMPOS_MOVE_NRMOVES] = 
{
	// Old breakneck version
	//CRelativeAnimPos::CAnimPosEntry(16.0f,0.0f,0.0f),		// Break neck (face opponent)
	//CRelativeAnimPos::CAnimPosEntry(16.0f,0.0f,0.5f),		// Get neck broken (face away from opponent 
	CRelativeAnimPos::CAnimPosEntry(18.287f,-0.9165f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_UNARMED_BREAKNECK
	CRelativeAnimPos::CAnimPosEntry(18.287f,-0.9165f,0.0f,0.5f),		// RELATIVEANIMPOS_MOVE_UNARMED_GETNECKBROKEN
	
	CRelativeAnimPos::CAnimPosEntry(20.0f,1.0f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_PUSH
	CRelativeAnimPos::CAnimPosEntry(20.0f,1.0f,0.0f,0.5f),		// RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GETPUSHED
	
	//CRelativeAnimPos::CAnimPosEntry(21.3545f,0.085f,0.0f,0.1f),	//RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABSLOW
	//CRelativeAnimPos::CAnimPosEntry(21.3545f,0.085f,0.5f,0.1f),	//RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABBEDSLOW
	CRelativeAnimPos::CAnimPosEntry(42.709f,0.085f,0.0f,0.0f),		//RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABSLOW
	CRelativeAnimPos::CAnimPosEntry(0.0f,0.085f,0.0f,0.5f),			//RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABBEDSLOW

	CRelativeAnimPos::CAnimPosEntry(18.6965f,1.5065f,0.0f,0.0f),	//RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABSLOW
	CRelativeAnimPos::CAnimPosEntry(18.6965f,1.5065f,0.0f,0.5f),	//RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABBEDSLOW

	CRelativeAnimPos::CAnimPosEntry(19.4205f,1.6495f,0.0f,0.0f),	//RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABSLOW
	CRelativeAnimPos::CAnimPosEntry(19.4205f,1.6495f,0.0f,0.5f),	//RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABBEDSLOW

	CRelativeAnimPos::CAnimPosEntry(16.4515f,3.1245f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_UNARMED_MIDDLE	
	CRelativeAnimPos::CAnimPosEntry(19.9135f,1.4305f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_CLUB_MIDDLE
	CRelativeAnimPos::CAnimPosEntry(23.1075f,0.0595f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_SHANK_MIDDLE
	CRelativeAnimPos::CAnimPosEntry(26.012f,-0.137f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_PISTOL
	CRelativeAnimPos::CAnimPosEntry(29.3575f,-1.7865f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_ASSAULT

	CRelativeAnimPos::CAnimPosEntry(15.2955f,0.725f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_RIGHT	
	CRelativeAnimPos::CAnimPosEntry(15.7215f,1.084f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_LEFT
	CRelativeAnimPos::CAnimPosEntry(15.9375f,1.594f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_MIDDLE
	CRelativeAnimPos::CAnimPosEntry(20.9345f,-0.855f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_CLUB_COUNTER_CLUB_MIDDLE
	CRelativeAnimPos::CAnimPosEntry(17.7845f,1.9395f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_CLUB_COUNTER_SHANK_MIDDLE
	CRelativeAnimPos::CAnimPosEntry(30.969f,-0.1825f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_CLUB_COUNTER_PISTOL,
	CRelativeAnimPos::CAnimPosEntry(26.357f,3.4525f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_CLUB_COUNTER_ASSAULT,

	CRelativeAnimPos::CAnimPosEntry(16.691f,-1.1725f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_RIGHT	
	CRelativeAnimPos::CAnimPosEntry(17.5575f,1.37f,0.0f,0.0f),			// RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_LEFT
	CRelativeAnimPos::CAnimPosEntry(15.146f,0.3685f,0.0f,0.0f),			// RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_MIDDLE
	CRelativeAnimPos::CAnimPosEntry(17.358f,0.749f,0.0f,0.0f),			// RELATIVEANIMPOS_MOVE_SHANK_COUNTER_CLUB_MIDDLE
	CRelativeAnimPos::CAnimPosEntry(18.821f,0.663f,0.0f,0.0f),			// RELATIVEANIMPOS_MOVE_SHANK_COUNTER_SHANK_MIDDLE
	CRelativeAnimPos::CAnimPosEntry(22.563f,-1.626f,0.0f,0.0f),			// RELATIVEANIMPOS_MOVE_SHANK_COUNTER_PISTOL
	CRelativeAnimPos::CAnimPosEntry(32.666f,4.7725f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_SHANK_COUNTER_ASSAULT

	CRelativeAnimPos::CAnimPosEntry(16.4515f,3.1245f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_UNARMED_MIDDLE	
	CRelativeAnimPos::CAnimPosEntry(15.2955f,0.725f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_RIGHT
	CRelativeAnimPos::CAnimPosEntry(15.7215f,1.084f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_LEFT
	CRelativeAnimPos::CAnimPosEntry(15.9375f,1.594f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_MIDDLE
	CRelativeAnimPos::CAnimPosEntry(16.691f,-1.1725f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_RIGHT
	CRelativeAnimPos::CAnimPosEntry(17.5575f,1.37f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_LEFT
	CRelativeAnimPos::CAnimPosEntry(15.146f,0.3685f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_MIDDLE

	CRelativeAnimPos::CAnimPosEntry(19.9135f,1.4305f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_UNARMED	
	CRelativeAnimPos::CAnimPosEntry(20.9345f,-0.855f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_CLUB
	CRelativeAnimPos::CAnimPosEntry(17.7845f,1.9395f,0.0f,0.0f),			// RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_SHANK

	CRelativeAnimPos::CAnimPosEntry(23.1075f,0.0595f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_UNARMED	
	CRelativeAnimPos::CAnimPosEntry(17.9095f,-1.9895f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_CLUB
	CRelativeAnimPos::CAnimPosEntry(18.821f,0.663f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_SHANK

	CRelativeAnimPos::CAnimPosEntry(26.012f,-0.137f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_UNARMED,
	CRelativeAnimPos::CAnimPosEntry(30.969f,-0.1825f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_CLUB,
	CRelativeAnimPos::CAnimPosEntry(22.563f,-1.626f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_SHANK,

	CRelativeAnimPos::CAnimPosEntry(29.3575f,-1.7865f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_UNARMED,
	CRelativeAnimPos::CAnimPosEntry(26.357f,3.4525f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_CLUB,
	CRelativeAnimPos::CAnimPosEntry(32.666f,4.7725f,0.0f,0.0f),	// RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_SHANK,

	CRelativeAnimPos::CAnimPosEntry(18.0365f,-0.425f,0.0f,0.0f),		// Club Break neck (face opponent)
	CRelativeAnimPos::CAnimPosEntry(18.0365f,-0.425f,0.0f,0.5f),		// Club get neck broken (face away from opponent 
	CRelativeAnimPos::CAnimPosEntry(18.037f,-0.1455f,0.0f,0.0f),		// Shank Break neck (face opponent)
	CRelativeAnimPos::CAnimPosEntry(18.037f,-0.1455f,0.0f,0.5f),		// Shank get neck broken (face away from opponent 

	CRelativeAnimPos::CAnimPosEntry(20.0f,16.0f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_SNEAK_DROPKILL,
	CRelativeAnimPos::CAnimPosEntry(0.0f,0.0f,0.0f,0.5f),		// RELATIVEANIMPOS_MOVE_SNEAK_DROPKILLED,

	CRelativeAnimPos::CAnimPosEntry(20.0f,0.0f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_TRANQ_STOMP,
	CRelativeAnimPos::CAnimPosEntry(0.0f,0.0f,0.0f,0.5f),		// RELATIVEANIMPOS_MOVE_TRANQ_STOMPED,

	CRelativeAnimPos::CAnimPosEntry(18.287f,-0.9165f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_UNARMED_GRABSLOW_FAKE
	CRelativeAnimPos::CAnimPosEntry(18.0365f,-0.425f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_CLUB_GRABSLOW_FAKE
	CRelativeAnimPos::CAnimPosEntry(18.037f,-0.1455f,0.0f,0.0f),		// RELATIVEANIMPOS_MOVE_SHANK_GRABSLOW_FAKE
};

CRelativeAnimPos::CRelativeAnimPos()
{
	Clear();
}

void CRelativeAnimPos::Clear()
{
	m_StartPos = 0.0f;
	m_StartRot.Unit();
	m_LastEndPos = 0.0f;
	m_LastEndRot.Unit();
	m_Midpoint = 0;
	m_iAnim = -1;
	m_TickStartOffset = 0;
	m_Angle = 0.0f;
	m_TimeOffset = 0.0f;
	m_MoveTokenStart = 0;
	m_iOther = 0;
	m_MoveType = 0;
	m_Flags = 0;
	m_ControlLookX = 0;
}


/*bool CRelativeAnimPos::GetCounterMove(int32 _Weapon, int32 _OpponentMove, int32& _Move, int32& _Response, fp32& _MaxCounterTime, fp32 _RelativeHealth)
{
	int32 CounterMoveIndex = GetCounterMoveIndex(_OpponentMove);
	int32 Weapon = _Weapon -1;
	if (Weapon < 0 || CounterMoveIndex < 0)
		return false;

	_Move = slCounterTypeTable[Weapon][CounterMoveIndex];
	_MaxCounterTime = slMaxCounterTimeTable[Weapon][CounterMoveIndex];
	_Response = slCounterResponseTypeTable[CounterMoveIndex][Weapon];
	
	return (_Move > 0 && _Response > 0 && (_RelativeHealth <= slCounterRelativeHealthTable[Weapon][CounterMoveIndex]));
}*/

// If only the move is needed (still needs to be activated from animgraph though)
void CRelativeAnimPos::CreateAnimMove(int32 _Move, const CWObject_CoreData* _pSelf, const CWObject_CoreData* _pOther, int16 _iAnim)
{
	m_LastEndPos = 0.0f;
	m_LastEndRot.Unit();
	if (_Move > 0 && _Move <= RELATIVEANIMPOS_MOVE_NRMOVES)
	{
		m_iAnim = _iAnim;
		m_MoveType = _Move;
		m_Flags = RELATIVEANIMPOS_FLAG_HASTARGETPOS | RELATIVEANIMPOS_FLAG_HASTARGETANGLE | RELATIVEANIMPOS_FLAG_HASANIMINDEX;
		GetMidPoint(_pSelf, _pOther);
		/*
		m_Midpoint = (_pSelf->GetPosition() + _pOther->GetPosition()) * 0.5f;
		m_Midpoint.k[2] = Max(_pSelf->GetPosition().k[2],_pOther->GetPosition().k[2]);

		CVec3Dfp32 Dir;
		if (_Move == RELATIVEANIMPOS_MOVE_SNEAK_DROPKILL)
		{
			Dir = CVec3Dfp32::GetMatrixRow(_pSelf->GetPositionMatrix(),0);
			m_Midpoint = _pOther->GetPosition();
		}
		else if (_Move == RELATIVEANIMPOS_MOVE_SNEAK_DROPKILLED)
		{
			Dir = -CVec3Dfp32::GetMatrixRow(_pOther->GetPositionMatrix(),0);
			m_Midpoint = _pSelf->GetPosition();
		}
		else
		{
			Dir = m_Midpoint - _pSelf->GetPosition();
			Dir.Normalize();
		}
		// Don't need an extra angle offset, just add the one calculated
		m_Angle = CVec3Dfp32::AngleFromVector(Dir[0], Dir[1]);*/
	}
	else
	{
		InvalidateTargetPos();
		InvalidateTargetAngle();
	}
}

// Ok, synced animation times with character placement
/*void CRelativeAnimPos::MakeSynchedAnim(CWorld_PhysState* _pWPhys, int32 _Move, CWO_Character_ClientData* _pCDSelf, CWObject_CoreData* _pSelf,
		CWO_Character_ClientData* _pCDOther, CWObject_CoreData* _pOther, fp32 _TimeOffset, fp32 _StartOffset)
{
	// Ok find highest gametick and go from there. Initially I will have offset of 1 tick
	// because I'm not sure the animation will be started on time (other char might have already
	// finished it's refresh for that tick

	m_MoveTokenStart = Max(_pCDSelf->m_GameTick,_pCDOther->m_GameTick) + 1;
	m_TimeOffset = _TimeOffset;
	if (_StartOffset != 0.0f)
	{
		m_TickStartOffset = (uint16)M_Ceil(_StartOffset * _pWPhys->GetGameTicksPerSecond());
		m_TimeOffset += ((fp32)(m_TickStartOffset * _pWPhys->GetGameTickTime())) - _StartOffset;//(uint16)_TickStartOffset;
	}
	m_iOther = _pOther->m_iObject;

	// Find offset position for myself, we use the normal startposition and then add this
	// value to calculated position to get the correct offset later
	// Have to assume single animation during that time, so we must find target animation 
	// somehow
	// This function call is probably very expensive.... (think there are some linear lookups
	// in the AG)
	CStr MoveAction;
	GetActionString(_Move, MoveAction);
	CWAGI_Context AGContext(_pSelf,_pWPhys,CMTime::CreateFromSeconds(0.0f),0,false);
	fp32 Temp;
	int16 iAnim = _pCDSelf->m_AnimGraph.GetAGI()->GetAnimFromAction(&AGContext,MoveAction,Temp);
	
//	ConOutL(CStrF("MAKING Synched anim: player %d Start: %d Self: %d Other: %d", (_pCDSelf->m_iPlayer != -1), m_MoveTokenStart,_pCDSelf->m_GameTick, _pCDOther->m_GameTick));

	CreateAnimMove(_Move, _pSelf, _pOther, iAnim);
	m_Flags |= RELATIVEANIMPOS_FLAG_HASMOVETOKEN | RELATIVEANIMPOS_FLAG_HASOTHERINDEX;
	MakeSynchedAnimExtras(_pWPhys, _Move, _pCDSelf, _pSelf, _pCDOther, _pOther);
}*/

/*void CRelativeAnimPos::MakeSynchedAnimExtras(CWorld_PhysState* _pWPhys, int32 _Move, CWO_Character_ClientData* _pCD, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCDOther, CWObject_CoreData* _pOther)
{
	switch (_Move)
	{
	case RELATIVEANIMPOS_MOVE_SNEAK_DROPKILL:
		{
			// Create actioncutscenecamera
			if (_pWPhys->IsClient())
				break;
			CWorld_Server* pWServer = safe_cast<CWorld_Server>(_pWPhys);
			m_spActiveCutsceneCamera = MNew(CActionCutsceneCamera);

			if (m_spActiveCutsceneCamera != NULL)
			{
				m_spActiveCutsceneCamera->SetServer(pWServer);
				m_spActiveCutsceneCamera->OnCreate();
				// should  be a bit random, eg left/rightside/close/far
				// Form:    "Mode:ViewXY:ViewXY:ViewZ:DistanceOffset:HeightOffset"
				// Example: "CHARACTER:BEHIND:NONE:LEVELED:100.0:60.0"
				//const int nCams = 4;
				#define NCAMS 1
				static char* pCameraConfigs[NCAMS] = 
				{
					"FIXEDPOS:FRONT:CAMERAVIEW_FIXEDPOSLEFT:CAMERAVIEW_BELOW:80.0:25.0", 
					//"FIXEDPOS:FRONT:CAMERAVIEW_FIXEDPOSRIGHT:CAMERAVIEW_BELOW:80.0:25.0",
				};
				int32 iIndex = 0;//MRTC_RAND() % NCAMS;

				if (!m_spActiveCutsceneCamera->ConfigureFromString(CStr(pCameraConfigs[iIndex])))
					m_spActiveCutsceneCamera->MakeDefaultCamera();

				m_spActiveCutsceneCamera->SetCharacterAndObject(_pObj->m_iObject, _pOther->m_iObject);

				m_spActiveCutsceneCamera->SetActive();
				m_spActiveCutsceneCamera->OnRefresh();
				m_spActiveCutsceneCamera->SetValid(_pCD);
			}
			break;
		}
	default:
		break;
	}

	return;
}*/

/*bool CRelativeAnimPos::MakeLedgeMove(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, int32 _Move, const CVec3Dfp32& _LeftPoint, const CVec3Dfp32& _RightPoint, const CVec3Dfp32& _Normal, fp32 _CurrentLedgePos, fp32 _RightLength)
{
	_Move += RELATIVEANIMPOS_MOVE_LEDGE_FIRSTMOVE;
	if (_Move < RELATIVEANIMPOS_MOVE_LEDGE_FIRSTMOVE || _Move > RELATIVEANIMPOS_MOVE_LEDGE_MAXNR)
	{
		Clear();
		return false;
	}
	// This will start our ledge move directly
	m_MoveTokenStart = _pCD->m_GameTick;
	m_TimeOffset = 0.0f;
	m_TickStartOffset = 0;
	m_iOther = 0;

	// Find offset position for myself, we use the normal startposition and then add this
	// value to calculated position to get the correct offset later
	// Have to assume single animation during that time, so we must find target animation 
	// somehow
	// This function call is probably very expensive.... (think there are some linear lookups
	// in the AG)
	CStr MoveAction;
	GetActionStringLedge(_Move, MoveAction);
	CWAGI_Context AGContext(_pObj,_pWPhys,CMTime::CreateFromSeconds(0.0f),0,false);
	m_iAnim = _pCD->m_AnimGraph.GetAGI()->GetAnimFromAction(&AGContext,MoveAction,m_TimeOffset);

	// Do a movetoken directly
	CWObject_Character::MoveToken(_pObj, _pWPhys,_pCD,AG_TOKEN_MAIN,MoveAction, 
		PHYSSTATE_TICKS_TO_TIME(m_MoveTokenStart, _pWPhys), m_TimeOffset);
	// No need for this flag as we do movetoken directly
	//m_Flags |= RELATIVEANIMPOS_FLAG_HASMOVETOKEN;

	//	ConOutL(CStrF("MAKING Synched anim: player %d Start: %d Self: %d Other: %d", (_pCDSelf->m_iPlayer != -1), m_MoveTokenStart,_pCDSelf->m_GameTick, _pCDOther->m_GameTick));
	m_LastEndPos = 0.0f;
	m_LastEndRot.Unit();
	m_MoveType = _Move;
	m_Flags = RELATIVEANIMPOS_FLAG_HASTARGETPOS | RELATIVEANIMPOS_FLAG_HASTARGETANGLE | RELATIVEANIMPOS_FLAG_HASANIMINDEX;
	// GetLedgePoint finds target position of new anchor point
	GetLedgePoint(_Move, m_iAnim, _LeftPoint, _RightPoint, _Normal, _CurrentLedgePos, _RightLength);

	return true;
}*/

bool CRelativeAnimPos::GetLedgePoint(int32 _Move, int16 m_iAnim, const CVec3Dfp32& _LeftPoint, const CVec3Dfp32& _RightPoint, const CVec3Dfp32& _Normal, fp32 _CurrentLedgePos, fp32 _RightLength)
{
	// Most of these just involve setting the anchor point as far away as the animation says, the 
	// corner animations need to be set at the far side of the ledges (whatever side we're at)
	CVec3Dfp32 LedgeDir = _LeftPoint - _RightPoint;
	fp32 Length = LedgeDir.Length();
	LedgeDir = LedgeDir / Length;

	// FIXED FOR DYNAMIC LEDGES

	// Preprocess ledgeposition
	if (_CurrentLedgePos > (Length - 14.0f))
		_CurrentLedgePos = Length - 12.0f;
	else if (_CurrentLedgePos < 14.0f)
		_CurrentLedgePos = 12.0f;
	
	CMat4Dfp32 Mat,InvMat;
	Mat.Unit();
	_Normal.SetMatrixRow(Mat,0);
	CVec3Dfp32(0.0f,0.0f,1.0f).SetMatrixRow(Mat,2);
	Mat.RecreateMatrix(0,2);
	Mat.InverseOrthogonal(InvMat);

	LedgeDir = LedgeDir * InvMat;
	CVec3Dfp32 Normal = _Normal * InvMat;
	CVec3Dfp32 RightPoint = _RightPoint * InvMat;
	CVec3Dfp32 LeftPoint = RightPoint + LedgeDir * Length;

	CVec3Dfp32 LedgePos = /*RightPoint +*/ LedgeDir * _CurrentLedgePos;

	CVec3Dfp32 Dir(1.0f,0.0f,0.0f);
	switch (_Move)
	{
	case RELATIVEANIMPOS_MOVE_LEDGE_IDLE:
	case RELATIVEANIMPOS_MOVE_LEDGE_LEFT:
	case RELATIVEANIMPOS_MOVE_LEDGE_RIGHT:
	case RELATIVEANIMPOS_MOVE_LEDGE_ALTCLIMBUPFROMHANGING:
	case RELATIVEANIMPOS_MOVE_LEDGE_DROPDOWN:
		{
			Dir = -_Normal;
			m_Midpoint = LedgePos + Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			m_Midpoint.k[2] -= 62.0f;
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_CLIMBUPFROMHANGING:
		{
			Dir = -_Normal;
			m_Midpoint = LedgePos + Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			m_Midpoint.k[2] -= 61.0f;
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_UPLOW:
		{
			Dir = -_Normal;
			m_Midpoint = LedgePos + Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			m_Midpoint.k[2] -= 48.0f;
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_UPMEDIUM:
		{
			Dir = -_Normal;
			m_Midpoint = LedgePos + Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			m_Midpoint.k[2] -= 64.0f;
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_JUMPUPANDGRAB:
		{
			Dir = -_Normal;
			m_Midpoint = LedgePos + Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			m_Midpoint.k[2] -= 96.0f;
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_CLIMBDOWNTOLEDGE:
		{
			Dir = -_Normal;
			m_Midpoint = LedgePos - Normal * 9.7f;// + CVec3Dfp32(0,0,1.0f);
			m_Midpoint.k[2] -= 62.0f;
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_ALTCLIMBDOWNTOLEDGE:
		{
			Dir = -_Normal;
			m_Midpoint = LedgePos + Normal * 12.0f;// + CVec3Dfp32(0,0,1.0f);
			m_Midpoint.k[2] -= 62.0f;
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_CLIMBCORNERINLEFT:
		{
			// Calculate offset from next ledge... :/
			CVec3Dfp32 NormalRight;
			CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(_Normal,NormalRight);
			Dir = -_Normal;
			CMat4Dfp32 Mat,InvMat;
			Mat.Unit();
			_Normal.SetMatrixRow(Mat,0);
			CVec3Dfp32(0.0f,0.0f,1.0f).SetMatrixRow(Mat,2);
			Mat.RecreateMatrix(0,2);
			Mat.InverseOrthogonal(InvMat);
			NormalRight = NormalRight * InvMat;
			m_Midpoint = /*LeftPoint -*/ -NormalRight * 12.0f + Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			m_Midpoint.k[2] -= 62.0f;
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_CLIMBCORNEROUTLEFT:
		{
			CVec3Dfp32 NormalRight;
			_Normal.CrossProd(CVec3Dfp32(0.0f,0.0f,1.0f),NormalRight);
			Dir = -_Normal;
			CMat4Dfp32 Mat,InvMat;
			Mat.Unit();
			_Normal.SetMatrixRow(Mat,0);
			CVec3Dfp32(0.0f,0.0f,1.0f).SetMatrixRow(Mat,2);
			Mat.RecreateMatrix(0,2);
			Mat.InverseOrthogonal(InvMat);
			NormalRight = NormalRight * InvMat;
			m_Midpoint = /*LeftPoint -*/ -NormalRight * 12.0f - Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			m_Midpoint.k[2] -= 62.0f;
			// Set point 12 units from corner
			/*Dir = -_Normal;
			m_Midpoint = LeftPoint - LedgeDir * (Length - 12.0f) + Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			m_Midpoint.k[2] -= 62.0f;*/
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_CLIMBCORNERINRIGHT:
		{
			//WTF
			// Calculate offset from next ledge... :/
			CVec3Dfp32 NormalRight;
			//CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(_Normal,NormalRight);
			_Normal.CrossProd(CVec3Dfp32(0.0f,0.0f,1.0f),NormalRight);
			Dir = -_Normal;
			CMat4Dfp32 Mat,InvMat;
			Mat.Unit();
			_Normal.SetMatrixRow(Mat,0);
			CVec3Dfp32(0.0f,0.0f,1.0f).SetMatrixRow(Mat,2);
			Mat.RecreateMatrix(0,2);
			Mat.InverseOrthogonal(InvMat);
			NormalRight = NormalRight * InvMat;
			m_Midpoint = /*LeftPoint -*/ NormalRight * (_RightLength - 12.0f) + Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			m_Midpoint.k[2] -= 62.0f;
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_CLIMBCORNEROUTRIGHT:
		{
			CVec3Dfp32 NormalRight;
			_Normal.CrossProd(CVec3Dfp32(0.0f,0.0f,1.0f),NormalRight);
			//CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(_Normal,NormalRight);
			Dir = -_Normal;
			CMat4Dfp32 Mat,InvMat;
			Mat.Unit();
			_Normal.SetMatrixRow(Mat,0);
			CVec3Dfp32(0.0f,0.0f,1.0f).SetMatrixRow(Mat,2);
			Mat.RecreateMatrix(0,2);
			Mat.InverseOrthogonal(InvMat);
			NormalRight = NormalRight * InvMat;
			m_Midpoint = /*LeftPoint -*/ NormalRight * (_RightLength + 12.0f) - Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			m_Midpoint.k[2] -= 62.0f;
			// Set point 12 units from corner
			/*Dir = -_Normal;
			m_Midpoint = LeftPoint - LedgeDir * (Length - 12.0f) + Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			m_Midpoint.k[2] -= 62.0f;*/
			break;
			/*Dir = -_Normal;
			m_Midpoint = RightPoint + LedgeDir * 12.0f + Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			m_Midpoint.k[2] -= 62.0f;
			break;*/
		}
	default:
		return false;
	}
	m_Angle = CVec3Dfp32::AngleFromVector(Dir[0], Dir[1]);
	return true;
}

bool CRelativeAnimPos::MakeLadderMove(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, int32 _Move, const CVec3Dfp32& _Bottom, const CVec3Dfp32& _Top, const CVec3Dfp32& _Normal, fp32 _CurrentLadderPos)
{
	_Move += RELATIVEANIMPOS_MOVE_LADDER_FIRSTMOVE;
	if (_Move < RELATIVEANIMPOS_MOVE_LADDER_FIRSTMOVE || _Move > RELATIVEANIMPOS_MOVE_LADDER_MAXNR)
	{
		Clear();
		return false;
	}
	// This will start our ledge move directly
	m_MoveTokenStart = _pCD->m_GameTick;
	m_TimeOffset = 0.0f;
	m_TickStartOffset = 0;
	m_iOther = 0;

	// Find offset position for myself, we use the normal startposition and then add this
	// value to calculated position to get the correct offset later
	// Have to assume single animation during that time, so we must find target animation 
	// somehow
	// This function call is probably very expensive.... (think there are some linear lookups
	// in the AG)
	CXRAG2_Impulse LadderImpulse;
	GetLadderImpulse(_Move-RELATIVEANIMPOS_MOVE_LADDER_FIRSTMOVE, LadderImpulse);
	CWAG2I_Context AGContext(_pObj,_pWPhys,_pCD->m_GameTime);
	m_iAnim = _pCD->m_AnimGraph2.GetAG2I()->GetAnimFromReaction(&AGContext,0,LadderImpulse,0,m_TimeOffset);
	_pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&AGContext, LadderImpulse);
	// No need for this flag as we do movetoken directly
	//m_Flags |= RELATIVEANIMPOS_FLAG_HASMOVETOKEN;

	//	ConOutL(CStrF("MAKING Synched anim: player %d Start: %d Self: %d Other: %d", (_pCDSelf->m_iPlayer != -1), m_MoveTokenStart,_pCDSelf->m_GameTick, _pCDOther->m_GameTick));
	m_LastEndPos = 0.0f;
	m_LastEndRot.Unit();
	m_MoveType = _Move;
	m_Flags = RELATIVEANIMPOS_FLAG_HASTARGETPOS | RELATIVEANIMPOS_FLAG_HASTARGETANGLE | RELATIVEANIMPOS_FLAG_HASANIMINDEX;
	// GetLedgePoint finds target position of new anchor point
	GetLadderPoint(_pObj, _Move, m_iAnim, _Bottom, _Top, _Normal, _CurrentLadderPos);

	return true;
}

#define LADDER_STEPSIZE (20.0f)
bool CRelativeAnimPos::GetLadderPoint(CWObject_CoreData* _pObj, int32 _Move, int16 m_iAnim, const CVec3Dfp32& _Bottom, const CVec3Dfp32& _Top, const CVec3Dfp32& _Normal, fp32 _CurrentLadderPos)
{
	CVec3Dfp32 Dir;
	CVec3Dfp32 LadderDir = _Top - _Bottom;
	fp32 LadderLen  = LadderDir.Length();
	LadderDir = LadderDir / LadderLen;
	int32 CurrentStep = RoundToInt(_CurrentLadderPos / LADDER_STEPSIZE);
	int32 MaxStep = RoundToInt(LadderLen / LADDER_STEPSIZE);
	if (CurrentStep < 0)
		CurrentStep = 0;
	if (CurrentStep > MaxStep)
		CurrentStep = MaxStep;

	if (_CurrentLadderPos > LadderLen)
		_CurrentLadderPos = LadderLen;
	else if (_CurrentLadderPos < 0.0f)
		_CurrentLadderPos = 0.0f;

	switch (_Move)
	{
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBUP:
	case RELATIVEANIMPOS_MOVE_LADDER_SLIDEDOWN:
	case RELATIVEANIMPOS_MOVE_LADDER_SLIDESTART:
	case RELATIVEANIMPOS_MOVE_LADDER_SLIDESTOP:
	case RELATIVEANIMPOS_MOVE_LADDER_IDLE:
		{
			// Find closest ladderstep and set midpoint there
			int32 MaxStepIdle = RoundToInt((LadderLen - 58.0f)/ LADDER_STEPSIZE);
			if (CurrentStep > MaxStepIdle)
				CurrentStep = MaxStepIdle;
			Dir = -_Normal;
			m_Midpoint = _Bottom + LadderDir * LADDER_STEPSIZE * CurrentStep + _Normal * 12.0f;;
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_SLIDESTOPEND:
		{
			// This is the bottom step
			Dir = -_Normal;
			m_Midpoint = _Bottom + _Normal * 12.0f;
			m_Midpoint.k[2] += 20.0f;
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBONDOWN:
		{
			Dir = -_Normal;
			m_Midpoint = _Bottom + _Normal * 12.0f;
			m_Midpoint.k[2] -= 20.0f;
			break;
		}

	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBONUP:
		{
			// _CurrentLadderPos is preprocessed to currect height I hope
			Dir = -_Normal;
			m_Midpoint = _Bottom + LadderDir * LADDER_STEPSIZE * CurrentStep + _Normal * 12.0f;;
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL4MINUS:
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL8MINUS:
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL12MINUS:
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR4MINUS:
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR8MINUS:
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR12MINUS:
		{
			// Base is just the top step
			Dir = -_Normal;
			m_Midpoint = _Top + _Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			m_Midpoint.k[2] -= 60.0f;
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR4PLUS:
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR0:
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL0:
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL4PLUS:
		{
			// Base is just the top step
			Dir = -_Normal;
			m_Midpoint = _Top + _Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			m_Midpoint.k[2] -= 80.0f;
			break;
		}
	case RELATIVEANIMPOS_MOVE_HANGRAIL_IDLE:
	case RELATIVEANIMPOS_MOVE_HANGRAIL_STARTFORWARD:
	case RELATIVEANIMPOS_MOVE_HANGRAIL_FORWARD:
	case RELATIVEANIMPOS_MOVE_HANGRAIL_TURN180:
	case RELATIVEANIMPOS_MOVE_HANGRAIL_STOP:
		{
			// Check current char dir
			CVec3Dfp32 CharDir = CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(),0);
			if (CharDir * LadderDir > 0.0f)
				Dir = LadderDir;
			else
				Dir = -LadderDir;
			m_Midpoint = _Bottom + LadderDir * _CurrentLadderPos;
			m_Midpoint.k[2] -= 68.5f;
			break;
		}
	case RELATIVEANIMPOS_MOVE_HANGRAIL_JUMPUP:
		{
			CVec3Dfp32 CharDir = CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(),0);
			if (CharDir * LadderDir > 0.0f)
				Dir = LadderDir;
			else
				Dir = -LadderDir;
			m_Midpoint = _Bottom + LadderDir * _CurrentLadderPos;
			m_Midpoint.k[2] -= 96.924f;
			break;
		}
	default:
		return false;
	}

	m_Angle = CVec3Dfp32::AngleFromVector(Dir[0], Dir[1]);
	return true;
}

void CRelativeAnimPos::GetMidPoint(const CWObject_CoreData* _pSelf, const CWObject_CoreData* _pOther)
{
	CVec3Dfp32 Dir;
	switch (m_MoveType)
	{
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABSLOW:
	case RELATIVEANIMPOS_MOVE_TRANQ_STOMP:
	case RELATIVEANIMPOS_MOVE_SNEAK_DROPKILL:
		{
			Dir = CVec3Dfp32::GetMatrixRow(_pSelf->GetPositionMatrix(),0);
			m_Midpoint = _pOther->GetPosition();
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABBEDSLOW:
	case RELATIVEANIMPOS_MOVE_TRANQ_STOMPED:
	case RELATIVEANIMPOS_MOVE_SNEAK_DROPKILLED:
		{
			Dir = -CVec3Dfp32::GetMatrixRow(_pOther->GetPositionMatrix(),0);
			m_Midpoint = _pSelf->GetPosition();
			break;
		}
	default:
		{
			m_Midpoint = (_pSelf->GetPosition() + _pOther->GetPosition()) * 0.5f;
			m_Midpoint.k[2] = Max(_pSelf->GetPosition().k[2],_pOther->GetPosition().k[2]);
			Dir = m_Midpoint - _pSelf->GetPosition();
			Dir.Normalize();
			break;
		}
	}

	// Don't need an extra angle offset, just add the one calculated
	m_Angle = CVec3Dfp32::AngleFromVector(Dir[0], Dir[1]);
}

// Finds perfect position at time +servertick and tries to get a nice velocity towards that point
// Dig upp animation and layertime
// Mostly a quick and dirty solution
void CRelativeAnimPos::GetPosition(const CWAG2I_Context* _pContext, const CWAG2I* _pAGI, CMTime _Offset, CVec3Dfp32& _Pos, CQuatfp32& _Rot) const
{
	_Rot.Unit();
	CXR_AnimLayer Layer;
	int32 MoveTokenStart = m_MoveTokenStart+m_TickStartOffset;
	_pAGI->GetSpecificAnimLayer(_pContext, Layer, AG2_TOKEN_MAIN, m_iAnim,MoveTokenStart);
	if (Layer.m_iBlendBaseNode > 1)
		return;

	// Calculate absolute positions.
	// Calculate absolute positions for next tick
	CMTime Time = CMTime::CreateFromSeconds(Layer.m_Time) + _Offset;
	VecUnion tmp;
	tmp.v128=M_VConst(0,0,0,1.f);
	Layer.m_spSequence->EvalTrack0(Time, tmp.v128, _Rot);
	_Pos = tmp.v3;

}

void CRelativeAnimPos::GetVelocityLedge(const CWAG2I_Context* _pContext, const CWAG2I* _pAGI, const CVec3Dfp32& _RightPoint, const CVec3Dfp32& _Normal, CVec3Dfp32& _Velocity, CQuatfp32& _RotVel)
{
	_Velocity = 0.0f;
	_RotVel.Unit();
	VecUnion Pos;
	Pos.v128 = M_VConst(0,0,0,1.0f);
	// Find current position relative startposition
	CVec3Dfp32 CurrentPos(_pContext->m_pObj->GetPosition());
	CQuatfp32 CurrentRot;
	//CurrentRot.Create(_pContext->m_pObj->GetPositionMatrix());

	{
		CVec3Dfp32 Direction = CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
		CVec3Dfp32 Angles = CWObject_Character::GetLook(Direction);
		CurrentRot.Create(CVec3Dfp32(0,0,1.0f), Angles.k[2]);
	}

	// Get start position
	//if (!(m_Flags & RELATIVEANIMPOS_FLAG_HASSTARTPOS))
	{
		// Create new startposition
		CMat4Dfp32 Mat;
		Mat.Unit();
		_Normal.SetMatrixRow(Mat,0);
		CVec3Dfp32(0.0f,0.0f,1.0f).SetMatrixRow(Mat,2);
		Mat.RecreateMatrix(0,2);
		if (GetStartPosition(m_StartPos, m_StartRot))
			m_Flags |= RELATIVEANIMPOS_FLAG_HASSTARTPOS;
		else
			return;

		m_StartPos = m_StartPos * Mat;
		m_StartPos += _RightPoint;
	}
	// Make current position relative startposition
	CurrentPos -= m_StartPos;
	CQuatfp32 Rot = m_StartRot;
	Rot.Inverse();
	CurrentRot = Rot * CurrentRot;
	//CurrentRot = m_StartRot;

	// Ok, need rotation as well
	//CQuatfp32 Rot;
	Rot.Unit();

	CXR_AnimLayer Layer;
	int32 MoveTokenStart = m_MoveTokenStart+m_TickStartOffset;
	if (!_pAGI->GetSpecificAnimLayer(_pContext, Layer, AG2_TOKEN_MAIN, m_iAnim, MoveTokenStart) || Layer.m_iBlendBaseNode > 1)
		return;

	// Calculate absolute positions for next tick
	CMTime TimeA = CMTime::CreateFromSeconds(Layer.m_Time + _pContext->m_TimeSpan);
	Layer.m_spSequence->EvalTrack0(TimeA, Pos.v128, Rot);

	// Must rotate pos to syncanim space (pos matrix won't work though since it might point into the air
	// must be in xy plane)
	// Bleh, could be done better I guess, too tired now
	CVec3Dfp32 Direction;
	CVec3Dfp32 Side;
	fp32 Angle = m_Angle;
	Direction.k[0] = M_Cos(Angle * _PI2);
	Direction.k[1] = M_Sin(Angle * _PI2);
	Direction.k[2] = 0.0f;
	CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(Direction,Side);
	CVec3Dfp32 NewPos;
	NewPos = Direction * Pos.v3.k[0];
	NewPos += Side * Pos.v3.k[1];
	NewPos.k[2] = Pos.v3.k[2];
	Pos.v3 = NewPos;

	// Calculate relative movement/rotation
	CVec3Dfp32 dMove;
	CQuatfp32 dRot;
	dMove = (Pos.v3 - CurrentPos);
	dRot = CurrentRot;
	dRot.Inverse();
	dRot = Rot * dRot;

	if (CurrentRot.DotProd(Rot) < 0.0f)
	{
		dRot.k[0] = -dRot.k[0];
		dRot.k[1] = -dRot.k[1];
		dRot.k[2] = -dRot.k[2];
		dRot.k[3] = -dRot.k[3];
	}

	// Cap velocity
	if (dMove.LengthSqr() > 400.0f)
	{
		dMove.Normalize();
		dMove *= 20.0f;
	}
	
	_Velocity = dMove;
	_RotVel = dRot;
}

void CRelativeAnimPos::GetVelocityLadder(const CWAG2I_Context* _pContext, const CWAG2I* _pAG2I, CVec3Dfp32& _Velocity, CQuatfp32& _RotVel)
{
	_Velocity = 0.0f;
	_RotVel.Unit();
	VecUnion Pos;
	Pos.v128 = M_VConst(0,0,0,1.0f);
	// Find current position relative startposition
	CVec3Dfp32 CurrentPos(_pContext->m_pObj->GetPosition());
	CQuatfp32 CurrentRot;
	//CurrentRot.Create(_pContext->m_pObj->GetPositionMatrix());

	{
		CVec3Dfp32 Direction = CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
		CVec3Dfp32 Angles = CWObject_Character::GetLook(Direction);
		CurrentRot.Create(CVec3Dfp32(0,0,1.0f), Angles.k[2]);
	}

	// Get start position
	//if (!(m_Flags & RELATIVEANIMPOS_FLAG_HASSTARTPOS))
	{
		// Create new startposition
		if (GetStartPosition(m_StartPos, m_StartRot))
			m_Flags |= RELATIVEANIMPOS_FLAG_HASSTARTPOS;
		else
			return;
	}
	// Make current position relative startposition
	CurrentPos -= m_StartPos;
	CQuatfp32 Rot = m_StartRot;
	Rot.Inverse();
	CurrentRot = Rot * CurrentRot;
	//CurrentRot = m_StartRot;

	// Ok, need rotation as well
	//CQuatfp32 Rot;
	Rot.Unit();

	CXR_AnimLayer Layer;
	int32 MoveTokenStart = m_MoveTokenStart+m_TickStartOffset;
	if (!_pAG2I->GetSpecificAnimLayer(_pContext, Layer, AG2_TOKEN_MAIN, m_iAnim, MoveTokenStart) || Layer.m_iBlendBaseNode > 1)
		return;

	// Calculate absolute positions for next tick
	CMTime TimeA = CMTime::CreateFromSeconds(Layer.m_Time + _pContext->m_TimeSpan);
	Layer.m_spSequence->EvalTrack0(TimeA, Pos.v128, Rot);

	// Must rotate pos to syncanim space (pos matrix won't work though since it might point into the air
	// must be in xy plane)
	// Bleh, could be done better I guess, too tired now
	CVec3Dfp32 Direction;
	CVec3Dfp32 Side;
	fp32 Angle = m_Angle;
	Direction.k[0] = M_Cos(Angle * _PI2);
	Direction.k[1] = M_Sin(Angle * _PI2);
	Direction.k[2] = 0.0f;
	CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(Direction,Side);
	CVec3Dfp32 NewPos;
	NewPos = Direction * Pos.v3.k[0];
	NewPos += Side * Pos.v3.k[1];
	NewPos.k[2] = Pos.v3.k[2];
	Pos.v3 = NewPos;

	// Calculate relative movement/rotation
	CVec3Dfp32 dMove;
	CQuatfp32 dRot;
	dMove = (Pos.v3 - CurrentPos);
	dRot = CurrentRot;
	dRot.Inverse();
	dRot = Rot * dRot;

	if (CurrentRot.DotProd(Rot) < 0.0f)
	{
		dRot.k[0] = -dRot.k[0];
		dRot.k[1] = -dRot.k[1];
		dRot.k[2] = -dRot.k[2];
		dRot.k[3] = -dRot.k[3];
	}

	// Cap velocity
	if (dMove.LengthSqr() > 400.0f)
	{
		dMove.Normalize();
		dMove *= 20.0f;
	}

	_Velocity = dMove;
	_RotVel = dRot;
}

void CRelativeAnimPos::GetVelocity(const CWAG2I_Context* _pContext, const CWAG2I* _pAGI, CVec3Dfp32& _Velocity, CQuatfp32& _RotVel)
{
	_Velocity = 0.0f;
	_RotVel.Unit();
	VecUnion Pos;
	Pos.v128 = M_VConst(0,0,0,1.0f);
	// Find current position relative startposition
	CVec3Dfp32 CurrentPos(_pContext->m_pObj->GetPosition());
	CQuatfp32 CurrentRot;
	//CurrentRot.Create(_pContext->m_pObj->GetPositionMatrix());

	{
		CVec3Dfp32 Direction = CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
		CVec3Dfp32 Angles = CWObject_Character::GetLook(Direction);
		CurrentRot.Create(CVec3Dfp32(0,0,1.0f), Angles.k[2]);
	}

	// Get start position
	if (!(m_Flags & RELATIVEANIMPOS_FLAG_HASSTARTPOS))
	{
		// Create new startposition
		if (GetStartPosition(m_StartPos, m_StartRot))
			m_Flags |= RELATIVEANIMPOS_FLAG_HASSTARTPOS;
		else
			return;
	}
	// Make current position relative startposition
	CurrentPos -= m_StartPos;
	CQuatfp32 Rot = m_StartRot;
	Rot.Inverse();
	//CurrentRot = Rot * CurrentRot;
	CurrentRot = m_StartRot;

	/*CVec3Dfp32 Diff = m_Midpoint - _pContext->m_pObj->GetPosition();
	//CAnimPosEntry PosEntryTest = m_slPosEntryTable[m_MoveType-1];
	CVec3Dfp32 DirTest;
	CVec3Dfp32 SideTest;
	DirTest.k[0] = M_Cos(m_Angle * _PI2);
	DirTest.k[1] = M_Sin(m_Angle * _PI2);
	DirTest.k[2] = 0.0f;
	CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(DirTest,SideTest);
	fp32 X,Y;
	X = DirTest * Diff;
	Y = SideTest * Diff;
	//ConOut(CStrF("iObj: %d XDiff: %f  YDiff: %f", _pContext->m_pObj->m_iObject,X,Y));*/
	//CWO_Character_ClientData* pCDOther = CWObject_Character::GetClientData(_pContext->m_pWPhysState->Object_GetCD(m_iOther));
	
	// Ok, need rotation as well
	//CQuatfp32 Rot;
	Rot.Unit();

	CXR_AnimLayer Layer;
	int32 MoveTokenStart = m_MoveTokenStart+m_TickStartOffset;
	if (!_pAGI->GetSpecificAnimLayer(_pContext, Layer, AG2_TOKEN_MAIN, m_iAnim,MoveTokenStart) || Layer.m_iBlendBaseNode > 1)
		return;

	// Calculate absolute positions for next tick
	CMTime TimeA = CMTime::CreateFromSeconds(Layer.m_Time + _pContext->m_TimeSpan);
	Layer.m_spSequence->EvalTrack0(TimeA, Pos.v128, Rot);

	// Must rotate pos to syncanim space (pos matrix won't work though since it might point into the air
	// must be in xy plane)
	//Pos.MultiplyMatrix3x3(_pContext->m_pObj->GetPositionMatrix());
	// Bleh, could be done better I guess, too tired now
	CVec3Dfp32 Direction;
	CVec3Dfp32 Side;
	CAnimPosEntry PosEntry = m_slPosEntryTable[m_MoveType-1];
	fp32 Angle = m_Angle + PosEntry.m_AngleOffset;
	Direction.k[0] = M_Cos(Angle * _PI2);
	Direction.k[1] = M_Sin(Angle * _PI2);
	Direction.k[2] = 0.0f;
	CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(Direction,Side);
	CVec3Dfp32 NewPos;
	NewPos = Direction * Pos.v3.k[0];
	NewPos += Side * Pos.v3.k[1];
	NewPos.k[2] = Pos.v3.k[2];
	Pos.v3 = NewPos;

	// Calculate relative movement/rotation
	CVec3Dfp32 dMove;
	CQuatfp32 dRot;
	dMove = (Pos.v3 - CurrentPos);
	dRot = CurrentRot;
	//dRot.Inverse();
	dRot = Rot * dRot;

	// Compensate for moving while rotating Should not be needed since we have world velocity...?
	/*CQuatfp32 InvRotA;
	InvRotA = dRot; InvRotA.Inverse();
	CMat4Dfp32 InvMatA;
	InvRotA.CreateMatrix3x3(InvMatA);
	dMove.MultiplyMatrix3x3(InvMatA);*/

	if (CurrentRot.DotProd(Rot) < 0.0f)
	{
		dRot.k[0] = -dRot.k[0];
		dRot.k[1] = -dRot.k[1];
		dRot.k[2] = -dRot.k[2];
		dRot.k[3] = -dRot.k[3];
	}

	_Velocity = dMove;
	//_RotVel = dRot;
	_RotVel.Unit();

	/*CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)*/
	{
		//CMat4Dfp32 Mat;
		CAxisRotfp32 Rotation;
		Rotation.Create(dRot);
		m_ControlLookX = Rotation.m_Angle * Rotation.m_Axis.k[2];
		//ConOutL(Rotatio							n.GetString());
		/*pCD->m_Control_Look.k[2] = m_ControlLookX;
		pCD->m_Control_Look.k[1] = 0.0f;//m_ControlLookX
		pCD->m_Control_Look.k[0] = 0.0f;
		pCD->m_Control_Look_Wanted = pCD->m_Control_Look;*/
	}

	/*if (_pContext->m_pObj->m_iObject == 50)
		ConOutL(CStrF("%s: Velocity: %s StartPos: %s  CharPos: %s Targetpos: %s iAnim: %d", (_pContext->m_pWPhysState->IsServer() ? "S":"C"),_Velocity.GetString().GetStr(),m_StartPos.GetString().GetStr(),_pContext->m_pObj->GetPosition().GetString().GetStr(), Pos.GetString().GetStr(), m_iAnim));*/
}

bool CRelativeAnimPos::GetStartPosition(CVec3Dfp32& _Position, CQuatfp32& _Rot) const
{
	// If we have no movetype, return
	if (!(m_Flags & RELATIVEANIMPOS_FLAG_HASTARGETPOS))
		return false;

	// Ledge positioning have their startposition in midpoint
	if (m_MoveType >= RELATIVEANIMPOS_MOVE_LEDGE_IDLE)
	{
		_Position = m_Midpoint;
		_Rot.Create(CVec3Dfp32(0,0,1.0f), 1.0f - m_Angle);
		return true;
	}

	//ConOut(CStrF("Getting Targetpos: m_Angle: %f Middle: %s MoveType: %.2x", m_Angle,m_Midpoint.GetString().GetStr(), m_MoveType));
	// Get targetposition from midpoint/type/and offsets
	CAnimPosEntry PosEntry = m_slPosEntryTable[m_MoveType-1];
	CVec3Dfp32 Direction;
	CVec3Dfp32 Side;
	Direction.k[0] = M_Cos(m_Angle * _PI2);
	Direction.k[1] = M_Sin(m_Angle * _PI2);
	Direction.k[2] = 0.0f;
	CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(Direction,Side);
	_Position = m_Midpoint - Direction * PosEntry.m_OffsetX + 
		Side * PosEntry.m_OffsetY;

	// Add a little height for safety
	_Position.k[2] += 0.001f + PosEntry.m_OffsetZ;

	fp32 Angle = 1.0f - m_Angle + PosEntry.m_AngleOffset;
	if (Angle > 1.0f)
		Angle -= 1.0f;
	else if (Angle < 0.0f)
		Angle += 1.0f;

	_Rot.Create(CVec3Dfp32(0,0,1.0f), Angle);

	return true;
}

void CRelativeAnimPos::GetTargetDirection(CVec3Dfp32& _CharLook) const
{
    // If we have no movetype, return
	if (!(m_Flags & RELATIVEANIMPOS_FLAG_HASTARGETANGLE))
		return;

	// Get targetposition from midpoint/type/and offsets
	CAnimPosEntry PosEntry = m_slPosEntryTable[m_MoveType-1];
	fp32 Angle = 1.0f - m_Angle + PosEntry.m_AngleOffset;
	if (Angle > 1.0f)
		Angle -= 1.0f;
	else if (Angle < 0.0f)
		Angle += 1.0f;

	_CharLook.k[0] = 0.0f;
	_CharLook.k[1] = 0.0f;
	_CharLook.k[2] = Angle;
}

extern fp32 AngleAdjust(fp32 _AngleA, fp32 _AngleB);
// Moderate some helper angles towards the angle we want
// Should try to get some kind of timeframe into this as in the movevelocity
void CRelativeAnimPos::ModerateLook(CWO_Character_ClientData* _pCD)
{
	CVec3Dfp32 WantedLook;
	GetTargetDirection(WantedLook);
	// Mmmkay then, borrow some variables from fight correction
	// Ah yes we must moderate towards the closest angle as well...
	WantedLook[2] += AngleAdjust(_pCD->m_Control_Look_Wanted[2], WantedLook[2]);
	WantedLook[1] += AngleAdjust(_pCD->m_Control_Look_Wanted[1], WantedLook[1]);
	//fp32 WantedLookCacheX = WantedLook[2];
	fp32 WantedLookCacheY = WantedLook[1];
	
	/*_pCD->m_FightLastAngleX = _pCD->m_FightTargetAngleX;
	WantedLook[2] *= 100.0f;
	_pCD->m_FightTargetAngleX *= 100.0f;
	_pCD->m_FightAngleXChange *= 100.0f;
	Moderatef(_pCD->m_FightTargetAngleX, WantedLook[2], _pCD->m_FightAngleXChange, 320);
	_pCD->m_FightTargetAngleX *= (1.0f/100.0f);
	_pCD->m_FightAngleXChange *= (1.0f/100.0f);*/

	_pCD->m_FightLastAngleY = _pCD->m_FightTargetAngleY;
	WantedLook[1] *= 100.0f;
	_pCD->m_FightTargetAngleY *= 100.0f;
	_pCD->m_FightAngleYChange *= 100.0f;
	Moderatef(_pCD->m_FightTargetAngleY, WantedLook[1], _pCD->m_FightAngleYChange, 320);
	_pCD->m_FightTargetAngleY *= (1.0f/100.0f);
	_pCD->m_FightAngleYChange *= (1.0f/100.0f);

	/*ConOut(CStrF("Target angle diffx: %f diffy: %f",
		M_Fabs(_pCD->m_FightTargetAngleX - WantedLookCacheX),
		M_Fabs(_pCD->m_FightTargetAngleY - WantedLookCacheY)));*/
	// If the angles are sufficiently close, invalidate anglemovement
	if (//(M_Fabs(_pCD->m_FightTargetAngleX - WantedLookCacheX) < 0.001f) &&
		(M_Fabs(_pCD->m_FightTargetAngleY - WantedLookCacheY) < 0.001f))
	{
		//ConOut("Invalidating target angle bitch!!!");
		InvalidateTargetAngle();
	}
}

/*void CRelativeAnimPos::DoStart(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhys)
{
	// Enable move/look
	//m_Flags |= RELATIVEANIMPOS_FLAG_MOVEENABLED | RELATIVEANIMPOS_FLAG_LOOKENABLED;
	
//	ConOut(CStrF("DOING START, tick: %d", _pWPhys->GetGameTick()));
	m_LastEndPos = 0.0f;
	m_LastEndRot.Unit();

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj); 
	if (!pCD)
		return;
	// True for all moves
	// Set synced controlmode
	CWObject_Character::Char_SetControlMode(_pObj,PLAYER_CONTROLMODE_ANIMSYNC);
	// Probably going to die anyway, set nocharcollision
	pCD->m_Phys_Flags = pCD->m_Phys_Flags | PLAYER_PHYSFLAGS_NOCHARACTERCOLL;
	//pCD->m_AnimGraph.ResetActionPressCount();
	// Must set the physics as well
	CWObject_Character::Char_SetPhysics(_pObj, _pWPhys, NULL, 
		CWObject_Character::Char_GetPhysType(_pObj), false, true);

	// Kill extra tokens
	CStr MoveToken("TerminateMe.Action_Die");
	CWObject_Character::MoveToken(_pObj,_pWPhys,pCD,AG_TOKEN_RESPOSE,MoveToken);
	CWObject_Character::MoveToken(_pObj,_pWPhys,pCD,AG_TOKEN_EFFECT,MoveToken);
	CWObject_Character::MoveToken(_pObj,_pWPhys,pCD,AG_TOKEN_WALLCOLLISION,MoveToken);
	CWObject_Character::MoveToken(_pObj,_pWPhys,pCD,AG_TOKEN_DIALOG,MoveToken);

	switch (m_MoveType)
	{
		// Pushed off cliff or dropkilled
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GETPUSHED:
	case RELATIVEANIMPOS_MOVE_SNEAK_DROPKILLED:
		{
			// param0=attacker,param1=target
			// Tell target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_BLOW,0,ENTERFIGHTMODE_ENTERBREAKNECK);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			// Tell attacker
			EnterFightMsg = CWObject_Message(OBJMSG_AIEFFECT_MELEEATTACK,_pObj->m_iObject);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,m_iOther);
			break;
		}
		// Snap neck by hands or club
	case RELATIVEANIMPOS_MOVE_UNARMED_GETNECKBROKEN:
	case RELATIVEANIMPOS_MOVE_CLUB_GETNECKBROKEN:
		{
			// param0=attacker,param1=target
			// Tell target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_DARKNESS,0,ENTERFIGHTMODE_ENTERBREAKNECK);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			// Tell attacker
			EnterFightMsg = CWObject_Message(OBJMSG_AIEFFECT_MELEEATTACK,_pObj->m_iObject);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,m_iOther);
			break;
		}
		// Shaft the guy
	case RELATIVEANIMPOS_MOVE_SHANK_GETNECKBROKEN:
		{
			// param0=attacker,param1=target
			// Tell target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_PIERCE,0,ENTERFIGHTMODE_ENTERBREAKNECK);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			// Tell attacker
			EnterFightMsg = CWObject_Message(OBJMSG_AIEFFECT_MELEEATTACK,_pObj->m_iObject);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,m_iOther);
			break;
		}
		// Fast and messy breakneck
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABBEDSLOW:
	case RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABBEDSLOW:
		{
			// param0=attacker,param1=target
			// Tell target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_DARKNESS,0,ENTERFIGHTMODE_ENTERBREAKNECK_LOUD);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			// Tell attacker
			EnterFightMsg = CWObject_Message(OBJMSG_AIEFFECT_MELEEATTACK,_pObj->m_iObject);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,m_iOther);
			break;
		}
		// Fast and messy shafting
	case RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABBEDSLOW:
		{
			// param0=attacker,param1=target
			// Tell target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_PIERCE,0,ENTERFIGHTMODE_ENTERBREAKNECK_LOUD);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			// Tell attacker
			EnterFightMsg = CWObject_Message(OBJMSG_AIEFFECT_MELEEATTACK,_pObj->m_iObject);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,m_iOther);
			break;
		}
		// Fast and very messy headstomp
	case RELATIVEANIMPOS_MOVE_TRANQ_STOMP:
		{
			// param0=attacker,param1=target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_BLOW,0,ENTERFIGHTMODE_ENTERBREAKNECK);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,m_iOther);
			// Tell attacker
			EnterFightMsg = CWObject_Message(OBJMSG_AIEFFECT_MELEEATTACK,_pObj->m_iObject);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,m_iOther);
			break;
		}
	case RELATIVEANIMPOS_MOVE_TRANQ_STOMPED:
	case RELATIVEANIMPOS_MOVE_UNARMED_BREAKNECK:
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_PUSH:
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABSLOW:
	case RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABSLOW:
	case RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABSLOW:

	case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_UNARMED:
	case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_CLUB:
	case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_SHANK:

	case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_UNARMED:
	case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_CLUB:
	case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_SHANK:
		break;

	case RELATIVEANIMPOS_MOVE_SNEAK_DROPKILL:
		{
			// Need to find and teleport startposition here... (otherwise flowing stuff.)
			// Get start position
			if (!(m_Flags & RELATIVEANIMPOS_FLAG_HASSTARTPOS))
			{
				// Create new startposition
				GetStartPosition(m_StartPos, m_StartRot);
				m_Flags |= RELATIVEANIMPOS_FLAG_HASSTARTPOS;
			}

			// Set thirdperson
			pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_THIRDPERSON;

			// Bleh, set physics on opponent as well
			CWObject_CoreData* pTarget = _pWPhys->Object_GetCD(m_iOther);
			CWO_Character_ClientData* pCDTarget = (pTarget ? CWObject_Character::GetClientData(pTarget) : NULL);
			if (pCDTarget)
			{
				// Probably going to die anyway, set nocharcollision
				pCDTarget->m_Phys_Flags = pCDTarget->m_Phys_Flags | PLAYER_PHYSFLAGS_NOCHARACTERCOLL;
				//pCD->m_AnimGraph.ResetActionPressCount();
				// Must set the physics as well
				CWObject_Character::Char_SetPhysics(pTarget, _pWPhys, NULL, 
					CWObject_Character::Char_GetPhysType(pTarget), false, true);
			}
			// Set startposition
			CXR_AnimLayer Layer;
			CWAGI_Context AGContext(_pObj, _pWPhys, pCD->m_GameTime);
			int32 MoveTokenStart = m_MoveTokenStart+m_TickStartOffset;
			if (!pCD->m_AnimGraph.GetAGI()->GetSpecificAnimLayer(&AGContext, Layer, AG_TOKEN_MAIN, m_iAnim, MoveTokenStart) || Layer.m_iBlendBaseNode > 1)
				break;

			// Calculate absolute positions for next tick
			CMTime TimeA = CMTime::CreateFromSeconds(Layer.m_Time);
			CVec3Dfp32 Pos;
			CQuatfp32 Rot;
			Layer.m_spSequence->EvalTrack0(TimeA, Pos, Rot);
			Pos += m_StartPos;
			//ConOutL(CStrF("Setting Startpos: %s", Pos.GetString().GetStr()));
			_pWPhys->Object_SetPosition(_pObj->m_iObject,Pos);

			break;
		}

	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_UNARMED_MIDDLE:
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_CLUB_MIDDLE:
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_SHANK_MIDDLE:
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_PISTOL:
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_ASSAULT:

	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_RIGHT:
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_LEFT:
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_MIDDLE:
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_CLUB_MIDDLE:
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_SHANK_MIDDLE:
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_PISTOL:
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_ASSAULT:

	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_RIGHT:
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_LEFT:
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_MIDDLE:
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_CLUB_MIDDLE:
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_SHANK_MIDDLE:
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_PISTOL:
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_ASSAULT:

	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_UNARMED_MIDDLE:
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_RIGHT:
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_LEFT:
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_MIDDLE:
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_RIGHT:
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_LEFT:
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_MIDDLE:

	case RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_UNARMED:
	case RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_CLUB:
	case RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_SHANK:

	case RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_UNARMED:
	case RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_CLUB:
	case RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_SHANK:

	case RELATIVEANIMPOS_MOVE_CLUB_BREAKNECK:
	case RELATIVEANIMPOS_MOVE_SHANK_BREAKNECK:
		break;
	default:
		break;
	}
}*/

/*void CRelativeAnimPos::DoStop(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhys)
{
	// Mmmmkay then, if we're stopping, remove all flags
	//m_Flags &= ~(RELATIVEANIMPOS_FLAG_MOVEENABLED | RELATIVEANIMPOS_FLAG_LOOKENABLED);
	m_Flags = 0;
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
	if (!pCD)
		return;

//	ConOut(CStrF("DOING STOP, tick: %d", _pWPhys->GetGameTick()));

	uint32 DamageType = DAMAGETYPE_BLOW;
	switch (m_MoveType)
	{
	case RELATIVEANIMPOS_MOVE_UNARMED_GETNECKBROKEN:
	case RELATIVEANIMPOS_MOVE_CLUB_GETNECKBROKEN:
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABBEDSLOW:
	case RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABBEDSLOW:
		{	// param0=attacker,param1=target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_DARKNESS,0,ENTERFIGHTMODE_EXITBREAKNECK_KILL);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GETPUSHED:
		{	// param0=attacker,param1=target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_BLOW,0,ENTERFIGHTMODE_EXITBREAKNECK_KILL);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABBEDSLOW:
	case RELATIVEANIMPOS_MOVE_SHANK_GETNECKBROKEN:
		{	// param0=attacker,param1=target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_PIERCE,0,ENTERFIGHTMODE_EXITBREAKNECK_KILL);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			break;
		}

	case RELATIVEANIMPOS_MOVE_UNARMED_BREAKNECK:
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_PUSH:
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABSLOW:
	case RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABSLOW:
	case RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABSLOW:
		break;

	case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_UNARMED:
	case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_UNARMED:
		{	// param0=attacker,param1=target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_PISTOL,0,ENTERFIGHTMODE_EXITBREAKNECK_KILL);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			break;
		}
	case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_CLUB:
	case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_CLUB:
		{	// param0=attacker,param1=target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_BLOW,0,ENTERFIGHTMODE_EXITBREAKNECK_KILL);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			break;
		}
	case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_SHANK:
	case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_SHANK:
		{	// param0=attacker,param1=target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_PIERCE,0,ENTERFIGHTMODE_EXITBREAKNECK_KILL);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			break;
		}

	case RELATIVEANIMPOS_MOVE_SNEAK_DROPKILL:
		{
			if (_pWPhys->IsServer())
			{
				m_spActiveCutsceneCamera = NULL;
				CActionCutsceneCamera::SetInvalidToPcd(pCD);
			}
			pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_THIRDPERSON;
			break;
		}
	case RELATIVEANIMPOS_MOVE_SNEAK_DROPKILLED:
	case RELATIVEANIMPOS_MOVE_TRANQ_STOMPED:
		{	// param0=attacker,param1=target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_BLOW,0,ENTERFIGHTMODE_EXITBREAKNECK_KILL);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			break;
		}

	case RELATIVEANIMPOS_MOVE_TRANQ_STOMP:
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_UNARMED_MIDDLE:
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_CLUB_MIDDLE:
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_SHANK_MIDDLE:
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_PISTOL:
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_ASSAULT:

	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_RIGHT:
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_LEFT:
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_MIDDLE:
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_CLUB_MIDDLE:
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_SHANK_MIDDLE:
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_PISTOL:
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_ASSAULT:

	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_RIGHT:
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_LEFT:
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_MIDDLE:
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_CLUB_MIDDLE:
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_SHANK_MIDDLE:
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_PISTOL:
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_ASSAULT:
		break;

	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_UNARMED_MIDDLE:
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_RIGHT:
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_LEFT:
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_MIDDLE:
		{	// param0=attacker,param1=target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_BLOW,0,ENTERFIGHTMODE_EXITBREAKNECK_KILL);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			break;
		}

	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_RIGHT:
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_LEFT:
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_MIDDLE:
		{	// param0=attacker,param1=target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_PIERCE,0,ENTERFIGHTMODE_EXITBREAKNECK_KILL);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			break;
		}

	case RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_UNARMED:
	case RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_CLUB:
	case RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_UNARMED:
	case RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_CLUB:
		{	// param0=attacker,param1=target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_BLOW,0,ENTERFIGHTMODE_EXITBREAKNECK_KILL);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			break;
		}

	case RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_SHANK:
	case RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_SHANK:
		{	// param0=attacker,param1=target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,DAMAGETYPE_PIERCE,0,ENTERFIGHTMODE_EXITBREAKNECK_KILL);
			_pWPhys->Phys_Message_SendToObject(EnterFightMsg,_pObj->m_iObject);
			break;
		}	

	case RELATIVEANIMPOS_MOVE_CLUB_BREAKNECK:
	case RELATIVEANIMPOS_MOVE_SHANK_BREAKNECK:
		break;

	default:
		break;
	}

	// Set that we can collide with other characters again
	pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~PLAYER_PHYSFLAGS_NOCHARACTERCOLL;
	pCD->m_AnimGraph.SetPendingFightInterrupt(0);
	// Must set the physics as well
	CWObject_Character::Char_SetPhysics(_pObj, _pWPhys, NULL, 
		CWObject_Character::Char_GetPhysType(_pObj), false, true);
}*/

void CRelativeAnimPos::OnRefresh()
{
	if (m_spActiveCutsceneCamera)
		m_spActiveCutsceneCamera->OnRefresh();

}

/*void CRelativeAnimPos::GetActionString(int32 _Move, CStr& _Str)
{
	// Mmkay then seems we can start the animation now (movetoken), so lets do that
	switch (_Move)
	{
	case RELATIVEANIMPOS_MOVE_UNARMED_BREAKNECK:
		{
			//MoveToken = CStr("FightMode_BreakNeck.Action_OneRef");
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Unarmed_Grab");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_GETNECKBROKEN:
		{
			//MoveToken = CStr("FightMode_GetNeckBroken.Action_OneRef");
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Response_Unarmed_Grab");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_BREAKNECK:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Club_Grab");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_GETNECKBROKEN:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Response_Club_Grab");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_BREAKNECK:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Shank_Grab");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_GETNECKBROKEN:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Response_Shank_Grab");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SNEAK_DROPKILL:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_DropKill");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SNEAK_DROPKILLED:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Response_DropKill");
			break;
		}
	case RELATIVEANIMPOS_MOVE_TRANQ_STOMP:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Stomp");
			break;
		}
	case RELATIVEANIMPOS_MOVE_TRANQ_STOMPED:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Response_Stomp");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_PUSH:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Unarmed_Push");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GETPUSHED:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Response_Unarmed_Push");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABSLOW:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Unarmed_GrabSlow");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABBEDSLOW:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Response_Unarmed_GrabSlow");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABSLOW:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Club_GrabSlow");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABBEDSLOW:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Response_Club_GrabSlow");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABSLOW:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Shank_GrabSlow");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABBEDSLOW:
		{
			_Str = CStr("Fight_StealthDecl.Action_Fight_Stealth_Response_Shank_GrabSlow");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_UNARMED_MIDDLE:
		{
			_Str = CStr("Fight_CounterDecl.Action_Fight_Unarmed_Counter_Unarmed_Middle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_CLUB_MIDDLE:
		{
			_Str = CStr("Fight_CounterDecl.Action_Fight_Unarmed_Counter_Club_Middle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_SHANK_MIDDLE:
		{
			_Str = CStr("Fight_CounterDecl.Action_Fight_Unarmed_Counter_Shank_Middle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_PISTOL:
		{
			_Str = CStr("Weapon_CounterDecl.Action_Fight_Unarmed_Counter_Pistol");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_ASSAULT:
		{
			_Str = CStr("Weapon_CounterDecl.Action_Fight_Unarmed_Counter_Assault");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_RIGHT:
		{
			_Str = CStr("Fight_CounterDecl.Action_Fight_Club_Counter_Unarmed_Right");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_LEFT:
		{
			_Str = CStr("Fight_CounterDecl.Action_Fight_Club_Counter_Unarmed_Left");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_MIDDLE:
		{
			_Str = CStr("Fight_CounterDecl.Action_Fight_Club_Counter_Unarmed_Middle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_CLUB_MIDDLE:
		{
			_Str = CStr("Fight_CounterDecl.Action_Fight_Club_Counter_Club_Middle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_SHANK_MIDDLE:
		{
			_Str = CStr("Fight_CounterDecl.Action_Fight_Club_Counter_Shank_Middle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_PISTOL:
		{
			_Str = CStr("Weapon_CounterDecl.Action_Fight_Club_Counter_Pistol");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_ASSAULT:
		{
			_Str = CStr("Weapon_CounterDecl.Action_Fight_Club_Counter_Assault");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_RIGHT:
		{
			_Str = CStr("Fight_CounterDecl.Action_Fight_Shank_Counter_Unarmed_Right");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_LEFT:
		{
			_Str = CStr("Fight_CounterDecl.Action_Fight_Shank_Counter_Unarmed_Left");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_MIDDLE:
		{
			_Str = CStr("Fight_CounterDecl.Action_Fight_Shank_Counter_Unarmed_Middle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_CLUB_MIDDLE:
		{
			_Str = CStr("Fight_CounterDecl.Action_Fight_Shank_Counter_Club_Middle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_SHANK_MIDDLE:
		{
			_Str = CStr("Fight_CounterDecl.Action_Fight_Shank_Counter_Shank_Middle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_PISTOL:
		{
			_Str = CStr("Weapon_CounterDecl.Action_Fight_Shank_Counter_Pistol");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_ASSAULT:
		{
			_Str = CStr("Weapon_CounterDecl.Action_Fight_Shank_Counter_Assault");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_UNARMED_MIDDLE:
		{
			_Str = CStr("Fight_CounterResponseDecl.Action_Fight_Unarmed_Response_Counter_Unarmed_Middle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_RIGHT:
		{
			_Str = CStr("Fight_CounterResponseDecl.Action_Fight_Unarmed_Response_Counter_Club_Right");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_LEFT:
		{
			_Str = CStr("Fight_CounterResponseDecl.Action_Fight_Unarmed_Response_Counter_Club_Left");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_MIDDLE:
		{
			_Str = CStr("Fight_CounterResponseDecl.Action_Fight_Unarmed_Response_Counter_Club_Middle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_RIGHT:
		{
			_Str = CStr("Fight_CounterResponseDecl.Action_Fight_Unarmed_Response_Counter_Shank_Right");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_LEFT:
		{
			_Str = CStr("Fight_CounterResponseDecl.Action_Fight_Unarmed_Response_Counter_Shank_Left");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_MIDDLE:
		{
			_Str = CStr("Fight_CounterResponseDecl.Action_Fight_Unarmed_Response_Counter_Shank_Middle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_UNARMED:
		{
			_Str = CStr("Fight_CounterResponseDecl.Action_Fight_Club_Response_Counter_Unarmed");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_CLUB:
		{
			_Str = CStr("Fight_CounterResponseDecl.Action_Fight_Club_Response_Counter_Club");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_SHANK:
		{
			_Str = CStr("Fight_CounterResponseDecl.Action_Fight_Club_Response_Counter_Shank");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_UNARMED:
		{
			_Str = CStr("Fight_CounterResponseDecl.Action_Fight_Shank_Response_Counter_Unarmed");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_CLUB:
		{
			_Str = CStr("Fight_CounterResponseDecl.Action_Fight_Shank_Response_Counter_Club");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_SHANK:
		{
			_Str = CStr("Fight_CounterResponseDecl.Action_Fight_Shank_Response_Counter_Shank");
			break;
		}
	case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_UNARMED:
		{
			_Str = CStr("Weapon_CounterResponseDecl.Action_Fight_Pistol_Response_Counter_Unarmed");
			break;
		}
	case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_CLUB:
		{
			_Str = CStr("Weapon_CounterResponseDecl.Action_Fight_Pistol_Response_Counter_Club");
			break;
		}
	case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_SHANK:
		{
			_Str = CStr("Weapon_CounterResponseDecl.Action_Fight_Pistol_Response_Counter_Shank");
			break;
		}
	case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_UNARMED:
		{
			_Str = CStr("Weapon_CounterResponseDecl.Action_Fight_Assault_Response_Counter_Unarmed");
			break;
		}
	case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_CLUB:
		{
			_Str = CStr("Weapon_CounterResponseDecl.Action_Fight_Assault_Response_Counter_Club");
			break;
		}
	case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_SHANK:
		{
			_Str = CStr("Weapon_CounterResponseDecl.Action_Fight_Assault_Response_Counter_Shank");
			break;
		}
	case RELATIVEANIMPOS_MOVE_UNARMED_GRABSLOW_FAKE:
		{
			_Str = CStr("Fight_StealthDeclFake.Action_Fight_Stealth_Unarmed_Grab");
			break;
		}
	case RELATIVEANIMPOS_MOVE_CLUB_GRABSLOW_FAKE:
		{
			_Str = CStr("Fight_StealthDeclFake.Action_Fight_Stealth_Club_Grab");
			break;
		}
	case RELATIVEANIMPOS_MOVE_SHANK_GRABSLOW_FAKE:
		{
			_Str = CStr("Fight_StealthDeclFake.Action_Fight_Stealth_Shank_Grab");
			break;
		}
	default:
		break;
	}
}

void CRelativeAnimPos::GetActionStringLedge(int32 _Move, CStr& _Str)
{
	switch (_Move)
	{
	case RELATIVEANIMPOS_MOVE_LEDGE_IDLE:
		{
			_Str = CStr("LedgeMove.Action_LedgeIdle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_LEFT:
		{
			_Str = CStr("LedgeMove.Action_LedgeLeft");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_RIGHT:
		{
			_Str = CStr("LedgeMove.Action_LedgeRight");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_UPLOW:
		{
			_Str = CStr("LedgeMove.Action_UpLow");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_UPMEDIUM:
		{
			_Str = CStr("LedgeMove.Action_UpMedium");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_JUMPUPANDGRAB:
		{
			_Str = CStr("LedgeMove.Action_JumpUpAndGrab");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_CLIMBUPFROMHANGING:
		{
			_Str = CStr("LedgeMove.Action_ClimbUpFromHanging");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_ALTCLIMBUPFROMHANGING:
		{
			_Str = CStr("LedgeMove.Action_AltClimbUpFromHanging");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_CLIMBDOWNTOLEDGE:
		{
			_Str = CStr("LedgeMove.Action_ClimbDownToLedge");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_ALTCLIMBDOWNTOLEDGE:
		{
			_Str = CStr("LedgeMove.Action_AltClimbDownToLedge");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_DROPDOWN:
		{
			_Str = CStr("LedgeMove.Action_DropDown");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_CLIMBCORNERINLEFT:
		{
			_Str = CStr("LedgeMove.Action_ClimbCornerInleft");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_CLIMBCORNEROUTLEFT:
		{
			_Str = CStr("LedgeMove.Action_ClimbCornerOutLeft");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_CLIMBCORNERINRIGHT:
		{
			_Str = CStr("LedgeMove.Action_ClimbCornerInRight");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LEDGE_CLIMBCORNEROUTRIGHT:
		{
			_Str = CStr("LedgeMove.Action_ClimbCornerOutRight");
			break;
		}
	default:
		break;
	}
}

void CRelativeAnimPos::GetActionStringLadder(int32 _Move, CStr& _Str)
{
	switch (_Move)
	{
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBUP:
		{
			_Str = CStr("LadderMove.Action_ClimbUp");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_SLIDEDOWN:
		{
			_Str = CStr("LadderMove.Action_SlideDown");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_SLIDESTART:
		{
			_Str = CStr("LadderMove.Action_SlideStart");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_SLIDESTOP:
		{
			_Str = CStr("LadderMove.Action_SlideStop");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_SLIDESTOPEND:
		{
			_Str = CStr("LadderMove.Action_SlideStopEnd");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBONDOWN:
		{
			_Str = CStr("LadderMove.Action_ClimbOnDown");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBONUP:
		{
			_Str = CStr("LadderMove.Action_ClimbOnUp");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_IDLE:
		{
			_Str = CStr("LadderMove.Action_Idle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL4PLUS:
		{
			_Str = CStr("LadderMove.Action_ClimbOffUpL4Plus");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL0:
		{
			_Str = CStr("LadderMove.Action_ClimbOffUpL0");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL4MINUS:
		{
			_Str = CStr("LadderMove.Action_ClimbOffUpL4Minus");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL8MINUS:
		{
			_Str = CStr("LadderMove.Action_ClimbOffUpL8Minus");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL12MINUS:
		{
			_Str = CStr("LadderMove.Action_ClimbOffUpL12Minus");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR4PLUS:
		{
			_Str = CStr("LadderMove.Action_ClimbOffUpR4Plus");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR0:
		{
			_Str = CStr("LadderMove.Action_ClimbOffUpR0");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR4MINUS:
		{
			_Str = CStr("LadderMove.Action_ClimbOffUpR4Minus");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR8MINUS:
		{
			_Str = CStr("LadderMove.Action_ClimbOffUpR8Minus");
			break;
		}
	case RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR12MINUS:
		{
			_Str = CStr("LadderMove.Action_ClimbOffUpR12Minus");
			break;
		}
	case RELATIVEANIMPOS_MOVE_HANGRAIL_IDLE:
		{
			_Str = CStr("HangrailMove.Action_Idle");
			break;
		}
	case RELATIVEANIMPOS_MOVE_HANGRAIL_STARTFORWARD:
		{
			_Str = CStr("HangrailMove.Action_StartForward");
			break;
		}
	case RELATIVEANIMPOS_MOVE_HANGRAIL_FORWARD:
		{
			_Str = CStr("HangrailMove.Action_Forward");
			break;
		}
	case RELATIVEANIMPOS_MOVE_HANGRAIL_TURN180:
		{
			_Str = CStr("HangrailMove.Action_Turn180");
			break;
		}
	case RELATIVEANIMPOS_MOVE_HANGRAIL_STOP:
		{
			_Str = CStr("HangrailMove.Action_Stop");
			break;
		}
	case RELATIVEANIMPOS_MOVE_HANGRAIL_JUMPUP:
		{
			_Str = CStr("HangrailMove.Action_JumpUp");
			break;
		}
	default:
		break;
	}
}*/

void CRelativeAnimPos::GetLadderImpulse(int32 _Move, CXRAG2_Impulse& _Impulse)
{
	_Impulse.m_ImpulseType = AG2_IMPULSETYPE_LADDERMOVE;
	_Impulse.m_ImpulseValue = _Move;
}

/*void CRelativeAnimPos::CheckForMoveTokens(CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, 
										  CWorld_PhysState* _pWPhys)
{
	int32 MoveTokenStart = m_MoveTokenStart+m_TickStartOffset;
	if ((m_Flags & RELATIVEANIMPOS_FLAG_HASMOVETOKEN) && (_pCD->m_GameTick >= MoveTokenStart))
	{
		CStr MoveToken;
		GetActionString(m_MoveType,MoveToken);

		CWObject_Character::MoveToken(_pObj, _pWPhys,_pCD,AG_TOKEN_MAIN,MoveToken, 
			PHYSSTATE_TICKS_TO_TIME(MoveTokenStart, _pWPhys), m_TimeOffset);
		// Remove movetoken flag
		m_Flags &= ~RELATIVEANIMPOS_FLAG_HASMOVETOKEN;
	}
}*/

/*int16 CRelativeAnimPos::LinkAnimations(const CWAGI_Context* _pContext, const CWAGI* _pAGI)
{
	// Skip on client...?
	if (_pContext->m_pWPhysState->IsClient())
		return -1;

	// First find endposition of current animation, reset current startposition and startangle
	CVec3Dfp32 CurrentEndPos;
	CQuatfp32 CurrentEndRot;
	//ConOut(CStrF("Getting Targetpos: m_Angle: %f Middle: %s MoveType: %.2x", m_Angle,m_Midpoint.GetString().GetStr(), m_MoveType));
	// Get targetposition from midpoint/type/and offsets
	CAnimPosEntry PosEntry = m_slPosEntryTable[m_MoveType-1];
	CVec3Dfp32 Direction;
	CVec3Dfp32 Side;
	Direction.k[0] = M_Cos(m_Angle * _PI2);
	Direction.k[1] = M_Sin(m_Angle * _PI2);
	Direction.k[2] = 0.0f;
	CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(Direction,Side);
	CurrentEndPos = m_Midpoint - Direction * PosEntry.m_OffsetX + 
		Side * PosEntry.m_OffsetY;

	// Add a little height for safety
	CurrentEndPos.k[2] += 0.001f;

	fp32 Angle = 1.0f - m_Angle + PosEntry.m_AngleOffset;
	if (Angle > 1.0f)
		Angle -= 1.0f;
	else if (Angle < 0.0f)
		Angle += 1.0f;

	CurrentEndRot.Create(CVec3Dfp32(0,0,1.0f), Angle);

	// Add end position/rotation to start position
	CXR_AnimLayer Layer;
	int32 MoveTokenStart = m_MoveTokenStart+m_TickStartOffset;
	if (!_pAGI->GetSpecificAnimLayer(_pContext, Layer, AG_TOKEN_MAIN, m_iAnim,MoveTokenStart) || Layer.m_iBlendBaseNode > 1)
	{
		// If we couldn't find the animation layer, assume current position/direction to be
		// start
		m_StartPos = _pContext->m_pObj->GetPosition();
		m_StartRot.Unit();
		return -1;
	}

	// Calculate absolute positions for next tick
	// Hmm, if I set endtime very high, I assume I get the correct endposition
	CVec3Dfp32 Pos;
	CQuatfp32 Rot;
	CMTime TimeA = CMTime::CreateFromSeconds(60.0f);
	Layer.m_spSequence->EvalTrack0(TimeA, Pos, Rot);

	PosEntry = m_slPosEntryTable[m_MoveType-1];
	Angle = m_Angle + PosEntry.m_AngleOffset;
	Direction.k[0] = M_Cos(Angle * _PI2);
	Direction.k[1] = M_Sin(Angle * _PI2);
	Direction.k[2] = 0.0f;
	CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(Direction,Side);
	CVec3Dfp32 NewPos;
	NewPos = Direction * Pos.k[0];
	NewPos += Side * Pos.k[1];
	Pos = NewPos;

	CVec3Dfp32 OldStartPos = m_StartPos;
	CVec3Dfp32 OldEndPos = m_LastEndPos;

	m_StartPos = CurrentEndPos + Pos + m_LastEndPos;
	// Hopfully (A * B == +) and (A * B.Inverse == -)
	m_StartRot = CurrentEndRot * Rot * m_LastEndRot;
	m_LastEndPos += Pos;
	m_LastEndRot *= Rot;
	m_Flags |= RELATIVEANIMPOS_FLAG_LINKED;

	//ConOut(CStrF("%s: iObj: %d Making New Startpos: %s Old Start: %s Old End: %s", _pContext->m_pWPhysState->IsServer() ? "S":"C",_pContext->m_pObj->m_iObject,m_StartPos.GetString().GetStr(),OldStartPos.GetString().GetStr(),OldEndPos.GetString().GetStr()));

	// Alrighty then, found new endposition, need to find new animation now
	// For this to work it has to be the first action in the state that has
	// a movetoken to a state that has an animation (shouldn't be much of a problem?)

	m_TimeOffset = 0.0f;//_TimeOffset;
	int16 iAnim;
	CAGActionIndex iAction;
	if (_pAGI->GetAnimFromFirstActionInState(_pContext,m_iAnim,AG_TOKEN_MAIN, iAnim, iAction))
	{
		m_iAnim = iAnim;
		return iAction;
	}

	return -1;
}*/

/*bool CRelativeAnimPos::Signal(int32 _Signal, const CWAGI_Context* _pContext, const CWAGI* _pAGI, bool _bInitiator)
{
	// Receive signals
	switch (_Signal)
	{
	case RELATIVEANIMPOS_SIGNAL_START:
		{
			DoStart(_pContext->m_pObj, _pContext->m_pWPhysState);
			break;
		}
	case RELATIVEANIMPOS_SIGNAL_STOP:
		{
			if (_bInitiator && (m_Flags & RELATIVEANIMPOS_FLAG_HASOTHERINDEX))
			{
				// Find opponent and do linkanim on him, and force move with the given action
				CWObject_CoreData* pTarget = _pContext->m_pWPhysState->Object_GetCD(m_iOther);
				CWO_Character_ClientData* pCDTarget = (pTarget ? CWObject_Character::GetClientData(pTarget) : NULL);
				if (pCDTarget)
				{
					CStr Action("Fight_Stealth_BreakNeck_Return.Action_Go");
					CWObject_Character::MoveToken(pTarget, _pContext->m_pWPhysState, 
						pCDTarget, AG_TOKEN_MAIN, Action);
				}
			}
			// Flags will disappear if we do this before
			DoStop(_pContext->m_pObj, _pContext->m_pWPhysState);
			break;
		}
	case RELATIVEANIMPOS_SIGNAL_LINKANIMATION:
		{
			// Must reset position at endposition of current animation, and find new animation
			LinkAnimations(_pContext, _pAGI);
			// Must link with other character as well?!?!
			if (_bInitiator && (m_Flags & RELATIVEANIMPOS_FLAG_HASOTHERINDEX))
			{
				// Find opponent and do linkanim on him, and force move with the given action
				CWObject_CoreData* pTarget = _pContext->m_pWPhysState->Object_GetCD(m_iOther);
				CWO_Character_ClientData* pCDTarget = (pTarget ? CWObject_Character::GetClientData(pTarget) : NULL);
				if (pCDTarget)
				{
					CWAGI_Context AGContext(pTarget,_pContext->m_pWPhysState,_pContext->m_GameTime,_pContext->m_TimeSpan);
					pCDTarget->m_AnimGraph.GetAGI()->AcquireAllResources(&AGContext);
					CAGActionIndex iAction = pCDTarget->m_RelAnimPos.LinkAnimations(&AGContext,pCDTarget->m_AnimGraph.GetAGI());
					if (iAction != -1)
					{
						pCDTarget->m_AnimGraph.GetAGI()->DoActionEffects(&AGContext,iAction);
						pCDTarget->m_AnimGraph.GetAGI()->MoveAction(&AGContext, AG_TOKEN_MAIN, iAction);
					}
					pCDTarget->m_AnimGraph.GetAGI()->UnacquireAllResources();
				}
			}
			return true;
		}
	case RELATIVEANIMPOS_SIGNAL_SETGRABDIFFICULTY:
		{
			// Set grab difficulty
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
			if (pCD)
				pCD->m_AnimGraph.SetActionPressCount((int8)-_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETGRABDIFFICULTY),m_iOther));
			break;
		}
	case RELATIVEANIMPOS_SIGNAL_STOPBREAKNECK:
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
			if (!pCD)
				return false;
			// Send "I'm dead" message to AI
			// param0=attacker,param1=target
			// Tell target
			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				m_iOther,_pContext->m_pObj->m_iObject,0,ENTERFIGHTMODE_EXITBREAKNECK);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(EnterFightMsg,_pContext->m_pObj->m_iObject);
			// Tell attacker
			EnterFightMsg = CWObject_Message(OBJMSG_AIEFFECT_MELEEATTACK,_pContext->m_pObj->m_iObject);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(EnterFightMsg,m_iOther);
			pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~PLAYER_PHYSFLAGS_NOCHARACTERCOLL;
			pCD->m_AnimGraph.SetPendingFightInterrupt(0);
			// Must set the physics as well
			CWObject_Character::Char_SetPhysics(_pContext->m_pObj, _pContext->m_pWPhysState, NULL, 
				CWObject_Character::Char_GetPhysType(_pContext->m_pObj), false, true);
			// DoStop(_pContext->m_pObj, _pContext->m_pWPhysState);
			break;
		}
	case RELATIVEANIMPOS_SIGNAL_KILLOTHER:
		{
			// If the other one's not running synched
			_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(
				OBJMSG_CHAR_KILLPLAYER), m_iOther);

			CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
				_pContext->m_pObj->m_iObject,m_iOther,0,ENTERFIGHTMODE_ENTERBREAKNECK_LOUD);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(EnterFightMsg,m_iOther);
			break;
		}
	case RELATIVEANIMPOS_SIGNAL_WEAPONKILL:
		{
			// Blood and stuff from selfshot in the head
			// Use activateposition and stuff to send force...
			// What to do, try activating the item first
			int32 iObj = _pContext->m_pObj->m_iObject;
			_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ACTIVATEITEM,AG_ITEMSLOT_WEAPONS,3,-1,true),iObj);
			// Should perhaps do fake sound/muzzle flash instead
			// Give input as 3 or something to activate "fake"

			CRPG_Object_Item* pMelee = (CRPG_Object_Item*)_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_CHAR_GETEQUIPPEDITEM, AG_ITEMSLOT_WEAPONS),iObj);
			if (!pMelee)
				break;
			
			CVec3Dfp32 Force = CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
			CVec3Dfp32 DamagePos = _pContext->m_pObj->GetPosition();
			switch (m_MoveType)
			{					
			case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_UNARMED:
			case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_CLUB:
			case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_SHANK:
				{
					Force = -Force;
					Force.k[2] += 2.0f;

					DamagePos.k[2] += 20.0f;
					break;
				}
			case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_UNARMED:
			case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_CLUB:
			case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_SHANK:
				{
					Force = -Force;
					Force.k[2] += 5.0f;

					DamagePos.k[2] += 55.0f;
					break;
				}
			default:
				break;
			}
			
			pMelee->SendDamage(iObj,DamagePos,iObj,1,DAMAGETYPE_BLOW,0,&Force);
			
			break;
		}
	default:
		break;
	}

	return false;
}*/

void CRelativeAnimPos::InvalidateTargetPos()
{
	m_Flags &= ~RELATIVEANIMPOS_MASK_CANMOVE;
}

void CRelativeAnimPos::InvalidateTargetAngle()
{
	//m_Flags &= ~RELATIVEANIMPOS_FLAG_LOOKENABLED;
}

void CRelativeAnimPos::CopyFrom(const CRelativeAnimPos& _From)
{
	m_StartPos = _From.m_StartPos;
	m_LastEndPos = _From.m_LastEndPos;
	m_StartRot = _From.m_StartRot;
	m_LastEndRot = _From.m_LastEndRot;
	m_Midpoint = _From.m_Midpoint;
	m_iAnim = _From.m_iAnim;
	m_TickStartOffset = _From.m_TickStartOffset;
	m_Angle = _From.m_Angle;
	m_TimeOffset = _From.m_TimeOffset;
	m_MoveTokenStart = _From.m_MoveTokenStart;
	m_iOther = _From.m_iOther;
	m_MoveType = _From.m_MoveType;
	m_Flags = _From.m_Flags;
}

void CRelativeAnimPos::Pack(uint8 *&_pD, CMapData* _pMapData) const
{
	uint8 Flags = m_Flags & RELATIVEANIMPOS_MASK_COPYMASK;
	//ConOut(CStrF("Pack: %.2x",Flags));
	PTR_PUTUINT8(_pD, Flags);
	if (Flags != 0)
	{
		if (Flags & RELATIVEANIMPOS_FLAG_LINKED)
		{
			PTR_PUTUINT8(_pD, m_MoveType);
			PTR_PUTFP32(_pD, m_TimeOffset);
			PTR_PUTUINT16(_pD, m_TickStartOffset);
			if (m_Flags & RELATIVEANIMPOS_FLAG_HASTARGETPOS)
			{
				PTR_PUTFP32(_pD, m_Midpoint[0]);
				PTR_PUTFP32(_pD, m_Midpoint[1]);
				PTR_PUTFP32(_pD, m_Midpoint[2]);
				PTR_PUTFP32(_pD, m_Angle);
			}
			if (m_Flags & RELATIVEANIMPOS_FLAG_HASOTHERINDEX)
			{
				PTR_PUTUINT16(_pD, m_iOther);
			}
			if (m_Flags & RELATIVEANIMPOS_FLAG_HASANIMINDEX)
			{
				PTR_PUTINT16(_pD, m_iAnim);
			}

			//ConOut(CStrF("Packing: %s",m_StartPos.GetString().GetStr()));
			PTR_PUTFP32(_pD, m_StartPos[0]);
			PTR_PUTFP32(_pD, m_StartPos[1]);
			PTR_PUTFP32(_pD, m_StartPos[2]);
			PTR_PUTFP32(_pD, m_StartRot.k[0]);
			PTR_PUTFP32(_pD, m_StartRot.k[1]);
			PTR_PUTFP32(_pD, m_StartRot.k[2]);
			PTR_PUTFP32(_pD, m_StartRot.k[3]);
		}
		else
		{
			PTR_PUTUINT8(_pD, m_MoveType);
			PTR_PUTFP32(_pD, m_TimeOffset);
			PTR_PUTUINT16(_pD, m_TickStartOffset);
			if (m_Flags & RELATIVEANIMPOS_FLAG_HASTARGETPOS)
			{
				PTR_PUTFP32(_pD, m_Midpoint[0]);
				PTR_PUTFP32(_pD, m_Midpoint[1]);
				PTR_PUTFP32(_pD, m_Midpoint[2]);
				PTR_PUTFP32(_pD, m_Angle);
			}
			if (m_Flags & RELATIVEANIMPOS_FLAG_HASOTHERINDEX)
			{
				PTR_PUTUINT16(_pD, m_iOther);
			}
			if (m_Flags & RELATIVEANIMPOS_FLAG_HASANIMINDEX)
			{
				PTR_PUTINT16(_pD, m_iAnim);
			}
		}
	}
}

void CRelativeAnimPos::Unpack(const uint8 *&_pD, CMapData* _pMapData)
{
	//Clear();
	PTR_GETUINT8(_pD, m_Flags);
	//ConOut(CStrF("UnPack: %.2x",m_Flags));
	if (m_Flags != 0)
	{
		if (m_Flags & RELATIVEANIMPOS_FLAG_LINKED)
		{
			PTR_GETUINT8(_pD, m_MoveType);
			PTR_GETFP32(_pD, m_TimeOffset);
			PTR_GETUINT16(_pD, m_TickStartOffset);
			if (m_Flags & RELATIVEANIMPOS_FLAG_HASTARGETPOS)
			{
				PTR_GETFP32(_pD, m_Midpoint[0]);
				PTR_GETFP32(_pD, m_Midpoint[1]);
				PTR_GETFP32(_pD, m_Midpoint[2]);
				PTR_GETFP32(_pD, m_Angle);
				//ConOut(CStrF("Unpacking midpoint: %s",m_Midpoint.GetString().GetStr()));
			}
			if (m_Flags & RELATIVEANIMPOS_FLAG_HASOTHERINDEX)
			{
				PTR_GETUINT16(_pD, m_iOther);
			}
			if (m_Flags & RELATIVEANIMPOS_FLAG_HASANIMINDEX)
			{
				PTR_GETINT16(_pD, m_iAnim);
			}

			PTR_GETFP32(_pD, m_StartPos[0]);
			PTR_GETFP32(_pD, m_StartPos[1]);
			PTR_GETFP32(_pD, m_StartPos[2]);
			//ConOut(CStrF("Unpacking: %s",m_StartPos.GetString().GetStr()));
			PTR_GETFP32(_pD, m_StartRot.k[0]);
			PTR_GETFP32(_pD, m_StartRot.k[1]);
			PTR_GETFP32(_pD, m_StartRot.k[2]);
			PTR_GETFP32(_pD, m_StartRot.k[3]);
		}
		else
		{
			PTR_GETUINT8(_pD, m_MoveType);
			PTR_GETFP32(_pD, m_TimeOffset);
			PTR_GETUINT16(_pD, m_TickStartOffset);
			if (m_Flags & RELATIVEANIMPOS_FLAG_HASTARGETPOS)
			{
				PTR_GETFP32(_pD, m_Midpoint[0]);
				PTR_GETFP32(_pD, m_Midpoint[1]);
				PTR_GETFP32(_pD, m_Midpoint[2]);
				PTR_GETFP32(_pD, m_Angle);
				//ConOut(CStrF("Unpacking midpoint: %s",m_Midpoint.GetString().GetStr()));
			}
			if (m_Flags & RELATIVEANIMPOS_FLAG_HASOTHERINDEX)
			{
				PTR_GETUINT16(_pD, m_iOther);
			}
			if (m_Flags & RELATIVEANIMPOS_FLAG_HASANIMINDEX)
			{
				PTR_GETINT16(_pD, m_iAnim);
			}
		}
	}
}

/////////////////////////////////////////////////////////////
// Animsyncer, make a move and corresponding move on opponent
/////////////////////////////////////////////////////////////
void CWObject_AnimSyncer::OnCreate()
{
	CWObject_AnimSyncerParent::OnCreate();
	m_PrimaryMove = -1;
	m_SecondaryMove = -1;
	ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
}

int32 CWObject_AnimSyncer::GetMoveFromString(const CStr& _Move)
{
	if (_Move.Find("UNARMED_BREAKNECK") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_GRABSLOW_FAKE;
	else if (_Move.Find("UNARMED_GETNECKBROKEN") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_GETNECKBROKEN;
	else if (_Move.Find("UNARMED_SNEAK_PUSH") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_PUSH;
	else if (_Move.Find("UNARMED_SNEAK_GETPUSHED") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GETPUSHED;
	else if (_Move.Find("UNARMED_SNEAK_GRABSLOW") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABSLOW;
	else if (_Move.Find("UNARMED_SNEAK_GRABBEDSLOW") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABBEDSLOW;
	else if (_Move.Find("CLUB_SNEAK_GRABSLOW") != -1)
		return RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABSLOW;
	else if (_Move.Find("CLUB_SNEAK_GRABBEDSLOW") != -1)
		return RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABBEDSLOW;
	else if (_Move.Find("SHANK_SNEAK_GRABSLOW") != -1)
		return RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABSLOW;
	else if (_Move.Find("SHANK_SNEAK_GRABBEDSLOW") != -1)
		return RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABBEDSLOW;
	else if (_Move.Find("UNARMED_COUNTER_UNARMED_MIDDLE") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_UNARMED_MIDDLE;
	else if (_Move.Find("UNARMED_COUNTER_CLUB_MIDDLE") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_CLUB_MIDDLE;
	else if (_Move.Find("UNARMED_COUNTER_SHANK_MIDDLE") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_SHANK_MIDDLE;
	else if (_Move.Find("UNARMED_COUNTER_PISTOL") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_PISTOL;
	else if (_Move.Find("UNARMED_COUNTER_ASSAULT") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_ASSAULT;
	else if (_Move.Find("CLUB_COUNTER_UNARMED_RIGHT") != -1)
		return RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_RIGHT;
	else if (_Move.Find("CLUB_COUNTER_UNARMED_LEFT") != -1)
		return RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_LEFT;
	else if (_Move.Find("CLUB_COUNTER_UNARMED_MIDDLE") != -1)
		return RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_MIDDLE;
	else if (_Move.Find("CLUB_COUNTER_CLUB_MIDDLE") != -1)
		return RELATIVEANIMPOS_MOVE_CLUB_COUNTER_CLUB_MIDDLE;
	else if (_Move.Find("CLUB_COUNTER_SHANK_MIDDLE") != -1)
		return RELATIVEANIMPOS_MOVE_CLUB_COUNTER_SHANK_MIDDLE;
	else if (_Move.Find("CLUB_COUNTER_PISTOL") != -1)
		return RELATIVEANIMPOS_MOVE_CLUB_COUNTER_PISTOL;
	else if (_Move.Find("CLUB_COUNTER_ASSAULT") != -1)
		return RELATIVEANIMPOS_MOVE_CLUB_COUNTER_ASSAULT;
	else if (_Move.Find("SHANK_COUNTER_UNARMED_RIGHT") != -1)
		return RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_RIGHT;
	else if (_Move.Find("SHANK_COUNTER_UNARMED_LEFT") != -1)
		return RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_LEFT;
	else if (_Move.Find("SHANK_COUNTER_UNARMED_MIDDLE") != -1)
		return RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_MIDDLE;
	else if (_Move.Find("SHANK_COUNTER_CLUB_MIDDLE") != -1)
		return RELATIVEANIMPOS_MOVE_SHANK_COUNTER_CLUB_MIDDLE;
	else if (_Move.Find("SHANK_COUNTER_SHANK_MIDDLE") != -1)
		return RELATIVEANIMPOS_MOVE_SHANK_COUNTER_SHANK_MIDDLE;
	else if (_Move.Find("SHANK_COUNTER_PISTOL") != -1)
		return RELATIVEANIMPOS_MOVE_SHANK_COUNTER_PISTOL;
	else if (_Move.Find("SHANK_COUNTER_ASSAULT") != -1)
		return RELATIVEANIMPOS_MOVE_SHANK_COUNTER_ASSAULT;

	// Kinda bad names, but first weapon is the one we have, second weapon the weapon of the other
	// person and last bit is the move we last made (attack middle/right/left), or nothing in the
	// case of club/shank
	else if (_Move.Find("UNARMED_RESPONSE_COUNTER_UNARMED_MIDDLE") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_UNARMED_MIDDLE;
	else if (_Move.Find("RESPONSE_COUNTER_CLUB_RIGHT") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_RIGHT;
	else if (_Move.Find("UNARMED_RESPONSE_COUNTER_CLUB_LEFT") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_LEFT;
	else if (_Move.Find("UNARMED_RESPONSE_COUNTER_CLUB_MIDDLE") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_MIDDLE;
	else if (_Move.Find("UNARMED_RESPONSE_COUNTER_SHANK_RIGHT") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_RIGHT;
	else if (_Move.Find("UNARMED_RESPONSE_COUNTER_SHANK_LEFT") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_LEFT;
	else if (_Move.Find("UNARMED_RESPONSE_COUNTER_SHANK_MIDDLE") != -1)
		return RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_MIDDLE;
	else if (_Move.Find("CLUB_RESPONSE_COUNTER_UNARMED") != -1)
		return RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_UNARMED;
	else if (_Move.Find("CLUB_RESPONSE_COUNTER_CLUB") != -1)
		return RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_CLUB;
	else if (_Move.Find("CLUB_RESPONSE_COUNTER_SHANK") != -1)
		return RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_SHANK;
	else if (_Move.Find("SHANK_RESPONSE_COUNTER_UNARMED") != -1)
		return RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_UNARMED;
	else if (_Move.Find("SHANK_RESPONSE_COUNTER_CLUB") != -1)
		return RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_CLUB;
	else if (_Move.Find("SHANK_RESPONSE_COUNTER_SHANK") != -1)
		return RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_SHANK;

	// Pistol responses
	else if (_Move.Find("PISTOL_RESPONSE_COUNTER_UNARMED") != -1)
		return RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_UNARMED;
	else if (_Move.Find("PISTOL_RESPONSE_COUNTER_CLUB") != -1)
		return RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_CLUB;
	else if (_Move.Find("PISTOL_RESPONSE_COUNTER_SHANK") != -1)
		return RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_SHANK;

	// Assault/shotgun responses
	else if (_Move.Find("ASSAULT_RESPONSE_COUNTER_UNARMED") != -1)
		return RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_UNARMED;
	else if (_Move.Find("ASSAULT_RESPONSE_COUNTER_CLUB") != -1)
		return RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_CLUB;
	else if (_Move.Find("ASSAULT_RESPONSE_COUNTER_SHANK") != -1)
		return RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_SHANK;
	else if (_Move.Find("CLUB_BREAKNECK") != -1)
		return RELATIVEANIMPOS_MOVE_CLUB_GRABSLOW_FAKE;
	else if (_Move.Find("CLUB_GETNECKBROKEN") != -1)
		return RELATIVEANIMPOS_MOVE_CLUB_GETNECKBROKEN;
	else if (_Move.Find("SHANK_BREAKNECK") != -1)
		return RELATIVEANIMPOS_MOVE_SHANK_GRABSLOW_FAKE;
	else if (_Move.Find("SHANK_GETNECKBROKEN") != -1)
		return RELATIVEANIMPOS_MOVE_SHANK_GETNECKBROKEN;
	else if (_Move.Find("SNEAK_DROPKILL") != -1)
		return RELATIVEANIMPOS_MOVE_SNEAK_DROPKILL;
	else if (_Move.Find("SNEAK_DROPKILLED") != -1)
		return RELATIVEANIMPOS_MOVE_SNEAK_DROPKILLED;
	else if (_Move.Find("TRANQ_STOMP") != -1)
		return RELATIVEANIMPOS_MOVE_TRANQ_STOMP;
	else if (_Move.Find("TRANQ_STOMPED") != -1)
		return RELATIVEANIMPOS_MOVE_TRANQ_STOMPED;

	return -1;
}

int32 CWObject_AnimSyncer::GetCounterMove(int32 _Move)
{
	switch (_Move)
	{
	case RELATIVEANIMPOS_MOVE_UNARMED_BREAKNECK:
		return RELATIVEANIMPOS_MOVE_UNARMED_GETNECKBROKEN;
	case RELATIVEANIMPOS_MOVE_UNARMED_GETNECKBROKEN:
		return RELATIVEANIMPOS_MOVE_UNARMED_BREAKNECK;
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_PUSH:
		return RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GETPUSHED;
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GETPUSHED:
		return RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_PUSH;

	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABSLOW:
		return RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABBEDSLOW;
	case RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABBEDSLOW:
		return RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABSLOW;

	case RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABSLOW:
		return RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABBEDSLOW;
	case RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABBEDSLOW:
		return RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABSLOW;

	case RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABSLOW:
		return RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABBEDSLOW;
	case RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABBEDSLOW:
		return RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABSLOW;

	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_UNARMED_MIDDLE:
		return RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_UNARMED_MIDDLE;
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_CLUB_MIDDLE:
		return RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_UNARMED;
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_SHANK_MIDDLE:
		return RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_UNARMED;
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_PISTOL:
		return RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_UNARMED;
	case RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_ASSAULT:
		return RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_UNARMED;

	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_RIGHT:
		return RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_RIGHT;
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_LEFT:
		return RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_LEFT;
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_MIDDLE:
		return RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_MIDDLE;
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_CLUB_MIDDLE:
		return RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_CLUB;
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_SHANK_MIDDLE:
		return RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_CLUB;
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_PISTOL:
		return RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_CLUB;
	case RELATIVEANIMPOS_MOVE_CLUB_COUNTER_ASSAULT:
		return RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_CLUB;

	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_RIGHT:
		return RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_RIGHT;
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_LEFT:
		return RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_LEFT;
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_MIDDLE:
		return RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_MIDDLE;
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_CLUB_MIDDLE:
		return RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_SHANK;
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_SHANK_MIDDLE:
		return RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_SHANK;
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_PISTOL:
		return RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_SHANK;
	case RELATIVEANIMPOS_MOVE_SHANK_COUNTER_ASSAULT:
		return RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_SHANK;

		// Kinda bad names, but first weapon is the one we have, second weapon the weapon of the other
		// person and last bit is the move we last made (attack middle/right/left), or nothing in the
		// case of club/shank
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_UNARMED_MIDDLE:
		return RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_UNARMED_MIDDLE;
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_RIGHT:
		return RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_RIGHT;
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_LEFT:
		return RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_LEFT;
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_MIDDLE:
		return RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_MIDDLE;
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_RIGHT:
		return RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_RIGHT;
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_LEFT:
		return RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_LEFT;
	case RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_MIDDLE:
		return RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_MIDDLE;
	case RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_UNARMED:
		return RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_CLUB_MIDDLE;
	case RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_CLUB:
		return RELATIVEANIMPOS_MOVE_CLUB_COUNTER_CLUB_MIDDLE;
	case RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_SHANK:
		return RELATIVEANIMPOS_MOVE_SHANK_COUNTER_CLUB_MIDDLE;
	case RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_UNARMED:
		return RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_SHANK_MIDDLE;
	case RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_CLUB:
		return RELATIVEANIMPOS_MOVE_CLUB_COUNTER_SHANK_MIDDLE;
	case RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_SHANK:
		return RELATIVEANIMPOS_MOVE_SHANK_COUNTER_SHANK_MIDDLE;

		// Pistol responses
	case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_UNARMED:
		return RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_PISTOL;
	case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_CLUB:
		return RELATIVEANIMPOS_MOVE_CLUB_COUNTER_PISTOL;
	case RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_SHANK:
		return RELATIVEANIMPOS_MOVE_SHANK_COUNTER_PISTOL;

		// Assault/shotgun responses
	case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_UNARMED:
		return RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_ASSAULT;
	case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_CLUB:
		return RELATIVEANIMPOS_MOVE_CLUB_COUNTER_ASSAULT;
	case RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_SHANK:
		return RELATIVEANIMPOS_MOVE_SHANK_COUNTER_ASSAULT;

	case RELATIVEANIMPOS_MOVE_CLUB_BREAKNECK:
		return RELATIVEANIMPOS_MOVE_CLUB_GETNECKBROKEN;
	case RELATIVEANIMPOS_MOVE_CLUB_GETNECKBROKEN:
		return RELATIVEANIMPOS_MOVE_CLUB_BREAKNECK;
	case RELATIVEANIMPOS_MOVE_SHANK_BREAKNECK:
		return RELATIVEANIMPOS_MOVE_SHANK_GETNECKBROKEN;
	case RELATIVEANIMPOS_MOVE_SHANK_GETNECKBROKEN:
		return RELATIVEANIMPOS_MOVE_SHANK_BREAKNECK;


	case RELATIVEANIMPOS_MOVE_SNEAK_DROPKILL:
		return RELATIVEANIMPOS_MOVE_SNEAK_DROPKILLED;
	case RELATIVEANIMPOS_MOVE_SNEAK_DROPKILLED:
		return RELATIVEANIMPOS_MOVE_SNEAK_DROPKILL;

	case RELATIVEANIMPOS_MOVE_TRANQ_STOMP:
		return RELATIVEANIMPOS_MOVE_TRANQ_STOMPED;
	case RELATIVEANIMPOS_MOVE_TRANQ_STOMPED:
		return RELATIVEANIMPOS_MOVE_TRANQ_STOMP;
	case RELATIVEANIMPOS_MOVE_UNARMED_GRABSLOW_FAKE:
		return RELATIVEANIMPOS_MOVE_UNARMED_GETNECKBROKEN;
	case RELATIVEANIMPOS_MOVE_CLUB_GRABSLOW_FAKE:
		return RELATIVEANIMPOS_MOVE_CLUB_GETNECKBROKEN;
	case RELATIVEANIMPOS_MOVE_SHANK_GRABSLOW_FAKE:
		return RELATIVEANIMPOS_MOVE_SHANK_GETNECKBROKEN;
	default:
		return -1;
	}
}

void CWObject_AnimSyncer::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	if (_pKey->GetThisName().Find("PRIMARY") != -1)
	{
		m_Primary = _pKey->GetThisValue();
	}
	else if (_pKey->GetThisName().Find("SECONDARY") != -1)
	{
		m_Secondary = _pKey->GetThisValue();
	}
	else if (_pKey->GetThisName().Find("MOVE") != -1)
	{
		// Find what move to make
		m_PrimaryMove = GetMoveFromString(_pKey->GetThisValue());
		m_SecondaryMove = GetCounterMove(m_PrimaryMove);
	}
	else
	{
		CWObject_AnimSyncerParent::OnEvalKey(_KeyHash, _pKey);
	}
}

/*aint CWObject_AnimSyncer::OnMessage(const CWObject_Message& _Msg)
{
	if (_Msg.m_Msg == OBJMSG_IMPULSE)
	{
		if (_Msg.m_Param0 == 1)
		{
			// Start it
			// Find objects
			CWObject_CoreData* pObjPrimary = m_pWServer->Object_GetCD(m_pWServer->Selection_GetSingleTarget(Selection));
			CWObject_CoreData* pObjSecondary = m_pWServer->Object_GetCD(m_pWServer->Selection_GetSingleTarget(Selection));
			CWO_Character_ClientData* pCDPrimary = (pObjPrimary ? CWObject_Character::GetClientData(pObjPrimary) : NULL);
			CWO_Character_ClientData* pCDSecondary = (pObjSecondary ? CWObject_Character::GetClientData(pObjSecondary) : NULL);
			if (pCDPrimary && pCDSecondary)
			{
				pCDPrimary->m_RelAnimPos.MakeSynchedAnim(m_pWServer,m_PrimaryMove,pCDPrimary,pObjPrimary,pCDSecondary,pObjSecondary);
				pCDPrimary->m_RelAnimPos.MakeDirty();
				pCDSecondary->m_RelAnimPos.MakeSynchedAnim(m_pWServer,m_SecondaryMove,pCDSecondary, pObjSecondary, pCDPrimary, pObjPrimary);
				pCDSecondary->m_RelAnimPos.MakeDirty();
			}
		}
		return 1;
	}
	return CWObject_AnimSyncerParent::OnMessage(_Msg);
}*/

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_AnimSyncer, CWObject_AnimSyncerParent, 0x0100);
