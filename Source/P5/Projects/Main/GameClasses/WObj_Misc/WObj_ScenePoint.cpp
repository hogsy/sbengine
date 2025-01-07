#include "PCH.h"
#include "WObj_ScenePoint.h"
#include "WObj_Room.h"
#include "WObj_Object.h"

#define DEBUG_LOG(x) (void)0
#define QReset(var) (void)0
#define QStart(var) (void)0
#define QStop(var) (void)0
/*
#define DEBUG_LOG(x) M_TRACE(x + "\n")
#define QReset(var) \
	__asm xor eax, eax \
	__asm mov dword ptr [var], eax

#define QStart(var) \
	__asm mov ebx, dword ptr [var] \
	__asm rdtsc \
	__asm sub ebx, eax \
	__asm mov dword ptr [var], ebx

#define QStop(var) \
	__asm rdtsc \
	__asm mov ebx, dword ptr [var] \
	__asm add ebx, eax \
	__asm mov dword ptr [var], ebx
*/

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_ScenePoint
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ScenePoint, CWObject, 0x0100);

CWObject_ScenePoint::CWObject_ScenePoint()
: CWObject()
{
	//Single user only as default
	m_NumUsers = 1;
	m_InitBlock.m_lSecondarySPNames.Clear();
	m_InitBlock.m_lSecondarySPActions.Clear();
	m_InitBlock.m_lEnginepathEPNames.Clear();
	m_InitBlock.m_lEnginepathSPNames.Clear();
	m_iScenePoint = -1;
};


//Helper "constant" array
int CWObject_ScenePoint::ms_2DDirs[8][2] = 
{   //Straights
	{1,0},{0,1},{0,-1},{-1,0},
		//Diagonals
	{1,1},{1,-1},{-1,1},{-1,-1}	
};


//Code copied from CAI_Pathfinder.
//Returns the closest position above or below the given position which is a traversable ground cell, 
//or fails with CVec3Dfp32(_FP32_MAX). If the optional _iMaxDiff argument is greater than -1, then this is 
//the maximum height-difference in cells tolerated. If the ordinary algorithm fails and the optional 
//_iRadius argument is greater than 0, it will continue to check _iRadius cell-columns in eight directions 
//outward from the given position.
CVec3Dfp32 CWObject_ScenePoint::GetPathPosition(CWorld_Server* _pWServer, const CVec3Dfp32& _Pos, int _iMaxDiff, int _iRadius)
{
	MAUTOSTRIP(CWObject_ScenePoint_GetPathPosition, CVec3Dfp32());

	if (_Pos == CVec3Dfp32(_FP32_MAX))
		return _Pos;

	CXR_BlockNavSearcher *pSearcher = _pWServer->Path_GetBlockNavSearcher();
	if(!pSearcher)
		return CVec3Dfp32(_FP32_MAX);

	CXR_BlockNav* pGridPF = pSearcher->GetBlockNav();
	if (!pGridPF) 
		//No navigation grid
		return CVec3Dfp32(_FP32_MAX);

	static const int BASESIZE = 24;
	static const int CROUCHHEIGHT = 32;
	static const bool WALLCLIMB = false;

	//Find the closest ground cell in the same xy-position 
	CVec3Dint16 GridPos = pGridPF->GetGridPosition(_Pos);

	//Pass 0
	if (pGridPF->IsOnGround(GridPos, BASESIZE, WALLCLIMB)&& 
		(pGridPF->IsTraversable(GridPos, BASESIZE, CROUCHHEIGHT, WALLCLIMB)))
		return _Pos;
	else
	{
		//Pass 1-_iMaxDiff
		if (_iMaxDiff == -1) 
			_iMaxDiff = 20;
		uint16 MaxUp = _iMaxDiff;
		uint16 MaxDown = _iMaxDiff;
		CVec3Dint16 Check;
		int iCheckRadius;
		int iLevel;
		for (int i = 1; i <= _iMaxDiff; i++)
		{
			//Check straight down if possible
			if (i <= MaxDown)
			{
				Check = GridPos - CVec3Dint16(0,0,i);
				if (pGridPF->IsOutsideGridBoundaries(Check))
				{
					//We can't go further downwards, since we're alredy below the grid floor
					MaxDown = i;
				}
				else if ( pGridPF->IsOnGround(Check, BASESIZE, WALLCLIMB) && 
					pGridPF->IsTraversable(Check, BASESIZE, CROUCHHEIGHT, WALLCLIMB) )
				{
					//_pWServer->Debug_RenderWire(_Pos, pGridPF->GetWorldPosition(Check), 0xffff0000, 100);//DEBUG

					//Found traversable ground cell!
					return pGridPF->GetWorldPosition(Check);
				};
				//_pWServer->Debug_RenderWire(_Pos, pGridPF->GetWorldPosition(Check), 0xffffff00, 100);//DEBUG
			};

			//Check straight up if possible
			if (i <= MaxUp)
			{
				Check = GridPos + CVec3Dint16(0,0,i);
				if (pGridPF->IsOutsideGridBoundaries(Check))
				{
					//We can't go further upwards, since we're alredy above the grid ceiling
					MaxUp = i;
				}
				else if ( pGridPF->IsOnGround(Check, BASESIZE, WALLCLIMB) && 
					pGridPF->IsTraversable(Check, BASESIZE, CROUCHHEIGHT, WALLCLIMB) )
				{
					//_pWServer->Debug_RenderWire(_Pos, pGridPF->GetWorldPosition(Check), 0xffff0000, 100);//DEBUG

					//Found traversable ground cell!
					return pGridPF->GetWorldPosition(Check);
				}
				//_pWServer->Debug_RenderWire(_Pos, pGridPF->GetWorldPosition(Check), 0xffffff00, 100);//DEBUG
			};

			//Check around the center column if appropriate 
			iCheckRadius = 1;
			while ( (iCheckRadius <= i) &
				(iCheckRadius <= _iRadius) )
			{
				//Set the height offset level we're currently checking at
				iLevel = i - iCheckRadius;

				//Should we check below? (or at same level if iLevel == 0)
				if (iLevel <= MaxDown)
				{
					for (int j = 0; j < 8; j++)
					{
						//Check cells around the center column and below the center 
						Check[0] = GridPos[0] + ms_2DDirs[j][0] * iCheckRadius;
						Check[1] = GridPos[1] + ms_2DDirs[j][1] * iCheckRadius;
						Check[2] = GridPos[2] - iLevel;
						if ( pGridPF->IsOnGround(Check, BASESIZE, WALLCLIMB) && 
							pGridPF->IsTraversable(Check, BASESIZE, CROUCHHEIGHT, WALLCLIMB) )
						{
							//_pWServer->Debug_RenderWire(_Pos, pGridPF->GetWorldPosition(Check), 0xffff0000, 100);//DEBUG

							//Found traversable ground cell!
							return pGridPF->GetWorldPosition(Check);
						};

						//_pWServer->Debug_RenderWire(_Pos, pGridPF->GetWorldPosition(Check), 0xffffff00, 100);//DEBUG

					};
				};

				//Should we check above? (Don't check if we're at level 0, since we've already checked those)
				if (iLevel && (iLevel <= MaxUp))
				{
					for (int j = 0; j < 8; j++)
					{
						//Check cells around the center column and below the center 
						Check[0] = GridPos[0] + ms_2DDirs[j][0] * iCheckRadius;
						Check[1] = GridPos[1] + ms_2DDirs[j][1] * iCheckRadius;
						Check[2] = GridPos[2] + iLevel;
						if ((pGridPF->IsOnGround(Check, BASESIZE, WALLCLIMB))&& 
							(pGridPF->IsTraversable(Check, BASESIZE, CROUCHHEIGHT, WALLCLIMB)))
						{
							//_pWServer->Debug_RenderWire(_Pos, pGridPF->GetWorldPosition(Check), 0xffff0000, 100);//DEBUG

							//Found traversable ground cell!
							return pGridPF->GetWorldPosition(Check);
						};

						//_pWServer->Debug_RenderWire(_Pos, pGridPF->GetWorldPosition(Check), 0xffffff00, 100);//DEBUG

					};
				};

				iCheckRadius++;
			};

			//Stop checking if we cannot check anything else
			if (i >= Max(MaxDown, MaxUp))
				break;
		};

		//Fail if we can't check anything else
		return CVec3Dfp32(_FP32_MAX);
	};
};


void CWObject_ScenePoint::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr Val = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH2('FLAG','S'): // "FLAGS"
		{
			static const char *FlagsTranslate[] =
			{	// We do not expose "Moving" flag to scripters, it is here to give "Sit" the correct value
				"WaitSpawn", "HeadingOnly", "Crouch", "Dynamic", 
				"Moving", "Sit", "LowPrio", "PlayOnce", "LightConnected", 
				"CombatRelease", "AllowMoving", "NoTracechecks", "AllowNear", NULL
			};

			m_Point.m_Flags = Val.TranslateFlags(FlagsTranslate);
			break;
		}
	case MHASH1('TYPE'): // "TYPE"
		{
			m_Point.m_Type = Val.TranslateFlags(CWO_ScenePoint::ms_lTranslateType);
			break;
		}
	case MHASH4('OLD_','ONRE','LEAS','E'): // "OLD_ONRELEASE"
		{
			if (Val.Val_int())
			{
				m_Point.m_ActivateFlags &= ~CWO_ScenePoint::FLAGS_ACTIVATE_RELEASE;
			}
			else
			{
				m_Point.m_ActivateFlags |= CWO_ScenePoint::FLAGS_ACTIVATE_RELEASE;
			}
			break;
		}
	default:
		{
			break;
		}
	}

	switch (_KeyHash)
	{
	case MHASH2('TACF','LAGS'): // "TACFLAGS"
		{
			static const char *FlagsTranslate[] =
			{
				"Left", "Right", "Stand", "Crouch", "Popup", "Coward", "Exit", "Charge", "Fire", "ChangePosture", "NoTarget", NULL
			};
			m_Point.m_TacFlags = Val.TranslateFlags(FlagsTranslate);
			break;
		}

	case MHASH2('DELA','Y'): // "DELAY"
		{
			m_Point.m_DelayTicks = RoundToInt(m_pWServer->GetGameTicksPerSecond() * (fp32)Val.Val_fp64());
			break;
		}

	case MHASH2('RADI','US'): // "RADIUS"
		{
			m_Point.m_SqrRadius = Sqr((fp32)Val.Val_fp64());
			if (m_Point.m_SqrRadius < Sqr(8.0f))
			{
				m_Point.m_SqrRadius = 0.0f;
			}
			break;
		}
	case MHASH2('ARCR','ANGE'): // "ARCRANGE"
		{
			m_Point.m_SqrArcMaxRange = Sqr((fp32)Val.Val_fp64());
			break;
		}
	case MHASH3('ARCM','INRA','NGE'): // "ARCMINRANGE"
		{
			m_Point.m_SqrArcMinRange = Sqr((fp32)Val.Val_fp64());
			break;
		}
	case MHASH2('ARCA','NGLE'): // "ARCANGLE"
		{
			//Get angle in radians (Val is angle in degrees)
			fp32 Angle = _PI2 * (fp32)Val.Val_fp64() / 360.0f;
			if (Angle < _PI)
			{
				m_Point.m_CosAngle = M_Cos(Angle); 
			}
			else
			{
				m_Point.m_CosAngle = -1.0f;
			}
			break;
		}
	case MHASH3('ARCO','FFSE','T'): // "ARCOFFSET"
		{
			m_Point.m_ArcOffset = (fp32)Val.Val_fp64() / 360.0f;
			break;
		}

	case MHASH1('ROOM'): // "ROOM"
		{
			m_InitBlock.m_RoomNameHash = CNameHash(Val);
			break;
		}
	case MHASH3('BEHA','VIOU','R'): // "BEHAVIOUR"
		{
			//Set the behaviour nbr to run with scenepoint is activated
			// Entirely passive, users of the scenepoint must retrieve the data and call CAI_Core::SetBehaviour etc
			m_Point.m_iBehaviour = Val.Val_int();
			break;
		}
	case MHASH2('DURA','TION'): // "DURATION"
		{
			// Optional duration for the behaviour
			m_Point.m_BehaviourDuration = (fp32)Val.Val_fp64();
			break;
		}
	case MHASH2('NUMU','SERS'): // "NUMUSERS"
		{
			m_NumUsers = Val.Val_int();
			break;
		}
	case MHASH4('LIGH','TCON','NECT','ED'): // "LIGHTCONNECTED"
		{
			m_InitBlock.m_ConnectedLightHash = CNameHash(Val);
			break;
		}
	default:
		{
			if (KeyName.CompareSubStr("USER") == 0)
			{
				uint iSlot = atoi(KeyName.Str() + 4);
				m_InitBlock.m_lUsers.SetMinLen(iSlot + 1);
				m_InitBlock.m_lUsers[iSlot] = m_pWServer->World_MangleTargetName(Val);
			}
			else if (KeyName.CompareSubStr("TEAM") == 0)
			{
				uint iSlot = atoi(KeyName.Str() + 4);
				m_InitBlock.m_lTeams.SetMinLen(iSlot + 1);
				m_InitBlock.m_lTeams[iSlot] = m_pWServer->World_MangleTargetName(Val);
			}
			else if (KeyName.CompareSubStr("MESSAGE_ACTIVATE_PRIO") == 0)
			{
				uint iSlot = atoi(KeyName.Str() + 21);
				m_lMessages_Activate_Prio.SetMinLen(iSlot + 1);
				m_lMessages_Activate_Prio[iSlot].Parse(Val, m_pWServer);
			}
			else if (KeyName.CompareSubStr("MESSAGE_ACTIVATE") == 0)
			{
				uint iSlot = atoi(KeyName.Str() + 16);
				m_Point.m_lMessages_Activate.SetMinLen(iSlot + 1);
				m_Point.m_lMessages_Activate[iSlot].Parse(Val, m_pWServer);
			}
			else if (KeyName.CompareSubStr("MESSAGE_TAKE") == 0)
			{
				uint iSlot = atoi(KeyName.Str() + 12);
				m_Point.m_lMessages_Take.SetMinLen(iSlot + 1);
				m_Point.m_lMessages_Take[iSlot].Parse(Val, m_pWServer);
			}
			else if (KeyName.CompareSubStr("MESSAGE_RELEASE") == 0)
			{
				uint iSlot = atoi(KeyName.Str() + 15);
				m_Point.m_lMessages_Release.SetMinLen(iSlot + 1);
				m_Point.m_lMessages_Release[iSlot].Parse(Val, m_pWServer);
			}
			else if (KeyName.CompareSubStr("MESSAGE_ANIMATION") == 0)
			{
				uint iSlot = atoi(KeyName.Str() + 17);
				m_Point.m_lMessages_Animation.SetMinLen(iSlot + 1);
				m_Point.m_lMessages_Animation[iSlot].Parse(Val, m_pWServer);
			}
			else if (KeyName.CompareSubStr("SECONDARYSP") == 0)
			{
				// Value consist of the scenepoint name and a flagfield separated by a comma
				CStr SSPName = Val.GetStrSep(",");
				CStr SSPAction = Val.GetStrSep(",");
				if ((SSPName.Len() > 0) && (SSPAction.Len() > 0))
				{
					uint iSlot = atoi(KeyName.Str() + 11);
					m_InitBlock.m_lSecondarySPNames.SetMinLen(iSlot + 1);
					m_InitBlock.m_lSecondarySPActions.SetMinLen(iSlot + 1);
					m_InitBlock.m_lSecondarySPNames[iSlot] = m_pWServer->World_MangleTargetName(SSPName);
					m_InitBlock.m_lSecondarySPActions[iSlot] = SSPAction.TranslateFlags(CWO_ScenePoint::CSecondaryScenePoint::ms_lTranslateActions);
				}
			}
			else if (KeyName.CompareSubStr("SCENEPOINTEP_NAME") == 0)
			{
				// Value consist of the enginepathname and the scenepoint name
				CStr EPName = Val.GetStrSep(",");
				CStr SPName = Val.GetStrSep(",");
				if ((EPName.Len() > 0) && (SPName.Len() > 0))
				{
					uint iSlot = atoi(KeyName.Str() + 17);
					m_InitBlock.m_lEnginepathEPNames.SetMinLen(iSlot + 1);
					m_InitBlock.m_lEnginepathSPNames.SetMinLen(iSlot + 1);
					m_InitBlock.m_lEnginepathEPNames[iSlot] = m_pWServer->World_MangleTargetName(EPName);
					m_InitBlock.m_lEnginepathSPNames[iSlot] = m_pWServer->World_MangleTargetName(SPName);
				}
			}
	 		CWObject::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_ScenePoint::OnFinishEvalKeys()
{
	m_Point.m_TargetName = GetName();
	m_Point.m_Position = GetPositionMatrix();
	m_Point.m_LocalPos = GetLocalPositionMatrix();

	// Look scenepoints always valid when moving
	if (m_Point.m_Type & CWO_ScenePoint::LOOK)
		m_Point.m_Flags |= CWO_ScenePoint::FLAGS_ALLOWMOVING;
	
	if (m_NumUsers > 0)
		m_Point.m_Resource.Init(m_NumUsers, CWO_ScenePoint::RESOURCE_MINRESERVATIONTIME, CWO_ScenePoint::RESOURCE_POLLINTERVAL);	

	if (m_lMessages_Activate_Prio.Len())
	{
		m_Point.m_lMessages_Activate.Insertx(0,m_lMessages_Activate_Prio.GetBasePtr(),m_lMessages_Activate_Prio.Len());
		m_lMessages_Activate_Prio.Clear();
	}

	//Calculate forward heading vector
	if (m_Point.m_ArcOffset != 0.0f)
	{
		CMat4Dfp32 Mat = m_Point.m_Position;
		Mat.M_x_RotZ(m_Point.m_ArcOffset);
		m_Point.m_FwdHeading = Mat.GetRow(0);
	}
	else
	{
		m_Point.m_FwdHeading = m_Point.m_Position.GetRow(0);
	}
	if (m_Point.m_Flags & CWO_ScenePoint::FLAGS_HEADINGONLY)
	{
		m_Point.m_FwdHeading[2] = 0.0f;
	}
	m_Point.m_FwdHeading.Normalize();

	// Store parent object for dynamic scenepoints
	m_InitBlock.m_ParentNameHash = m_ParentNameHash;

	// Hopefully this one is spawned :/
	//Add scene point. Note that we haven't set any secondary scene points etc yet. This must be done 
	//when all CWO_ScenePoints have been created, i.e. in OnSpawnWorld2. 
	m_iScenePoint = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_ADDSCENEPOINT, (aint)&m_Point,(aint)&m_InitBlock), m_pWServer->Game_GetObjectIndex());
	//Now we can safely remove this object
	m_pWServer->Object_DestroyInstant(m_iObject);
}



// -------------------------------------------------------------------
//  CWO_ScenePoint
// -------------------------------------------------------------------
const char * CWO_ScenePoint::ms_lTranslateType[] = 
{
	"ROAM", 
	"SEARCH",
	"TACTICAL", 
	"COVER",
	"TALK",
	"LOOK",
	"DOOR",
	"WALKCLIMB",
	"JUMPCLIMB",
	"DARKLING",
	"DYNAMIC",

	NULL,
};

const char * CWO_ScenePoint::CSecondaryScenePoint::ms_lTranslateActions[] = 
{
	"SPAWN",
	"UNSPAWN",
	"PRIOUP",
	"PRIODOWN",
	"MOVEUNSPAWN",
	NULL,
};

// Searches for the first Speak message in OnActivate and returns its number
int CWO_ScenePoint::GetSpeakDialog()
{
	for(int i = 0; i < m_lMessages_Activate.Len(); i++)
	{
		if (m_lMessages_Activate[i].m_Msg == OBJMSG_RPG_SPEAK_OLD)
		{
			return(m_lMessages_Activate[i].m_Param0);			
		}
	}

	return(0);
};

CVec3Dfp32& CWO_ScenePoint::GetPathPosition(CWorld_Server* _pWServer)
{
	if (VALID_POS(m_PathPos))
	{
		return(m_PathPos);
	}
	else
	{
		if ((m_Type & (WALK_CLIMB | JUMP_CLIMB | LOOK))||(m_lEnginepathSPs.Len() > 0))
		{
			m_PathPos = GetPosition();
			return(m_PathPos);
		}

		//Calculate path position
		m_PathPos = CWObject_ScenePoint::GetPathPosition(_pWServer, GetPosition(), 3, 0);
		if (INVALID_POS(m_PathPos))
		{	//Try a little wider
			m_PathPos = CWObject_ScenePoint::GetPathPosition(_pWServer, GetPosition(), 10, 3);
			if (INVALID_POS(m_PathPos))
			{	//Can't find suitable path position, use original position. 
				//Scene point will propably be useless for pathfinding actions though.
				m_PathPos = GetPosition();
				if (!(m_Type & DYNAMIC))
				{	// Invalid Dynamics are so common we just skip on reporting them
					// (they will be tem disabled when chosen anyway)
					ConOutL(CStrF("§c3f0WARNING: ScenePoint with no PathPos, Fix! (Position: %f, %f, %f;  Name: %s)", m_PathPos[0], m_PathPos[1], m_PathPos[2], m_TargetName.Str()));
				}
			}
		}
	}

	return(m_PathPos);
};

//Perform secondary scene point event
void CWO_ScenePoint::SecondaryScenePointEvent(int _Event, CWObject_ScenePointManager* _pSPM)
{
	if (!_pSPM)
		return;

	for (int i = 0; i < m_lSecondarySPs.Len(); i++)
	{
		//Get scene point
		CWO_ScenePoint * pSSP = _pSPM->GetScenePointFromIndex(m_lSecondarySPs[i].m_iSP);
		if (pSSP)
		{
			int16 Flags = m_lSecondarySPs[i].m_Flags;
			switch(_Event)
			{
			case CSecondaryScenePoint::EVENT_ONTAKE:
				// Parent SP was taken
				if (Flags & CSecondaryScenePoint::ACTION_ONTAKE_SPAWN)
				{	// Spawn SP
					pSSP->Spawn(_pSPM);
				}
				if (Flags & CSecondaryScenePoint::ACTION_ONTAKE_RAISE)
				{	// Raise prio
					pSSP->RaisePrio();
				}
				if (Flags & CSecondaryScenePoint::ACTION_ONRELEASE_UNSPAWN)
				{	// UnSpawn SP
					pSSP->SetUnspawnOnReleaseFlag(true);
				}
				break;

			case CSecondaryScenePoint::EVENT_ONACTIVATE:
				// Parent SP was activated
				break;

			case CSecondaryScenePoint::EVENT_ONRELEASE:
				// Parent SP was released
				if (Flags & CSecondaryScenePoint::ACTION_ONRELEASE_LOWER)
				{	// Lower prio
					pSSP->RestorePrio();
				}
				break;
			}
		}
	}
};


CWO_ScenePoint::CWO_ScenePoint()
{
	m_SqrRadius = 32.0f*32.0f;		//Allow some deviancy from exact position as default
 	m_SqrArcMaxRange = _FP32_MAX;	//Always within range as default
	m_SqrArcMinRange = 0.0f;		//Always within range as default
	m_ArcOffset = 0.0f;				//No arcoffset as default
	m_CosAngle = -1.0f;				//Always within front angle as default
	m_pRoom = NULL;	
	m_ReenableTick = 0;
	m_DelayTicks = 0;
	m_bUnspawnOnRelease = false;
	m_iConnectedLight = 0;
	INVALIDATE_POS(m_PathPos);
	m_iBehaviour = 0;				// Behaviour to run when > 0
	m_iSecondaryBehaviour = 0;		// Secondary behaviour, complimentary to m_iBehaviour when > 0
	m_BehaviourDuration = 3.0f;		// How long if > 0
	INVALIDATE_POS(m_Offset);		// Offset position where behaviour should start
	m_bReverseDir = false;
	m_Prio = 0;
	m_Light = 0.0f;					// We presuppose light to be 0.0 until measured
	m_LightTick = 0;
	m_Flags = 0;
	m_ActivateFlags = FLAGS_ACTIVATE_RELEASE;
	m_TacFlags = 0;
	m_Type = ROAM; //Roam is used as default. There is no point in having an invalid scene point.
	for (int i = 0; i < MAXUSERS; i++)
	{
		m_lUsers[i] = 0;
		m_liTeams[i] = 0;
	}
	m_lSecondarySPs.Clear();
	m_iParent = 0;
}

void CWO_ScenePoint::UpdateLight(fp32 _Light,int _Tick)
{
	m_Light = _Light;
	m_LightTick = _Tick;
};

bool CWO_ScenePoint::Activate(int _iActivator, CWorld_Server *_pWServer, CWObject_ScenePointManager* _pSPM)
{
	//Can we activate?
	if (!(m_Flags & FLAGS_WAITSPAWN) &&
		(_pWServer->GetGameTick() >= m_ReenableTick))
	{
		//Send messages
		for(int i = 0; i < m_lMessages_Activate.Len(); i++)
			m_lMessages_Activate[i].SendMessage(_iActivator, _iActivator, _pWServer);

		//Set next tick scene point can get activated
		// *** Should not clear this!
		if (m_DelayTicks > 0)
		{
			m_DelayTicks = 0;
		}
		// ***
		m_ReenableTick = _pWServer->GetGameTick() + m_DelayTicks;
		//Do secondary scene point activation events
		SecondaryScenePointEvent(CSecondaryScenePoint::EVENT_ONACTIVATE, _pSPM);
		m_Prio = 0;	// Reset prio
		if (m_ActivateFlags & FLAGS_ACTIVATE_RELEASE)
		{
			m_ActivateFlags |= FLAGS_ACTIVATED;
		}
		return true;
	}
	else
	{
		//Can't activate yet
		return false;
	}
}


//Spawn/unspawn scenepoint
void CWO_ScenePoint::Spawn(CWObject_ScenePointManager* _pSPM)
{
	if (m_Flags & FLAGS_WAITSPAWN)
	{
		// Add listener
		if (m_iParent > 0)
			_pSPM->m_pWServer->Object_AddListener(m_iParent, _pSPM->m_iObject, CWO_LISTENER_EVENT_MOVED);

		m_Flags &= ~FLAGS_WAITSPAWN;
	}
}

void CWO_ScenePoint::UnSpawn(CWObject_ScenePointManager* _pSPM)
{
	//Should perhaps notify users of unspawning... 
	//but they'll notice next time they try to request the resource anyway.
	//No need to release all resource slot, they'll time out after a while anyway
	//and if we should happen to get spawned again soon then we might as well let
	//users keep their slots.
	if (!(m_Flags & FLAGS_WAITSPAWN))
	{
		// Remove listener
		if (m_iParent > 0)
			_pSPM->m_pWServer->Object_RemoveListener(m_iParent, _pSPM->m_iObject);

		m_Flags |= FLAGS_WAITSPAWN;
		//Do secondary scene point release events
		SecondaryScenePointEvent(CSecondaryScenePoint::EVENT_ONRELEASE, _pSPM);
	}
}

// Returns a direction within the arc of the SP with _Period
// If arc is less than 360 degrees the sweep will oscillate
CVec3Dfp32 CWO_ScenePoint::GetSweepDir(int32 _Timer,int32 _Period)
{
	if (_Period <= 1)
	{
		_Period = 10;
	}
	int32 timerMod = _Timer % _Period;
	int32 timerDiv = _Timer / _Period;
	fp32 t = fp32(timerMod) / fp32(_Period);
	fp32 Angle;
	if (m_CosAngle > -1.0f)
	{	// Sweep
		fp32 ArcAngle = M_ACos(m_CosAngle) / _PI2;
		if (timerDiv % 2)
		{	// Even
			Angle = -ArcAngle + 2.0f * ArcAngle * t;
		}
		else
		{	// Odd
			Angle = ArcAngle - 2.0f * ArcAngle * t;
		}
	}
	else
	{	// Rotate
		Angle = t;
	}

	// Fractional arc angle
	CMat4Dfp32 MatSp = GetPositionMatrix();
	MatSp.RotZ_x_M(Angle);

	return(MatSp.GetRow(0));
};

//Check if given position is in front arc of scene point
bool CWO_ScenePoint::InFrontArcImpl(const CVec3Dfp32& _Pos, const CVec3Dfp32& _ThisPos)
{
	// Check room
	if (m_pRoom)
	{
		if (m_pRoom->PointIsInsideRoom(_Pos))
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}

	//Check range
	if (m_SqrArcMaxRange != _FP32_MAX)
	{
		if (m_Flags & FLAGS_HEADINGONLY)
		{
			if (Sqr(_Pos[0]-_ThisPos[0])+ Sqr(_Pos[1]-_ThisPos[1]) > m_SqrArcMaxRange)
				return false;
			if (Sqr(_Pos[2]-_ThisPos[2]) > m_SqrArcMaxRange)
				return false;
		}
		else
		{
			if (_Pos.DistanceSqr(_ThisPos) > m_SqrArcMaxRange)
				return false;
		}
	}
	if (m_SqrArcMinRange > 0.0f)
	{
		if (m_Flags & FLAGS_HEADINGONLY)
		{
			if (Sqr(_Pos[0]-_ThisPos[0])+ Sqr(_Pos[1]-_ThisPos[1]) < m_SqrArcMinRange)
				return false;
			/* Heading only never considers vertical minrange; it would be stupid wouldn't it?
			if (Sqr(_Pos[2]-_ThisPos[2]) < m_SqrArcMinRange)
				return false;
			*/
		}
		else
		{
			if (_Pos.DistanceSqr(_ThisPos) < m_SqrArcMinRange)
				return false;
		}
	}
	//Check angle
	if (m_CosAngle > -1.0f)
	{
		CVec3Dfp32 Fwd = m_FwdHeading;
		CVec3Dfp32 Dir = _Pos - _ThisPos;
		if (m_Flags & FLAGS_HEADINGONLY)
			Dir[2] = 0.0f;
		Dir.Normalize();
	
		if (m_Type & (JUMP_CLIMB | WALK_CLIMB))
		{
			fp32 dot;
			if (m_Flags & FLAGS_HEADINGONLY)
			{	// Dot horisontally
				dot = Dir[0]*Fwd[0] + Dir[1]*Fwd[1];
			}
			else
			{	// Dot normally
				dot = Dir * Fwd;
			}
			if (dot >= m_CosAngle)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			// Dot horisontally
			fp32 dot = Dir[0]*Fwd[0] + Dir[1]*Fwd[1];
			if (m_Flags & FLAGS_HEADINGONLY)
			{
				if (dot >= m_CosAngle)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				if (dot >= m_CosAngle)
				{
					if (Sqr(Dir[2]) <= Sqr(Dir[0]) + Sqr(Dir[1]))
					{
						return true;
					}
					else
					{
						return false;
					}
					
				}
				else
				{
					return false;
				}
			}
		}
	}

	//Both within range and angle
	return true;
};

// Expensive!
bool CWO_ScenePoint::DirInFrontArc()
{
	if ((m_pRoom)||(Abs(m_ArcOffset) > 0.001f))
	{
		CVec3Dfp32 DirPos = GetPosition(false) + GetDirection(false) * (M_Sqrt(m_SqrArcMinRange) + M_Sqrt(m_SqrArcMaxRange)) * 0.5f;
		return(InFrontArc(DirPos));
	}
	else
	{
		return(true);
	}
};

//Check if given position is sufficiently near scene point
bool CWO_ScenePoint::IsAt(const CVec3Dfp32& _Pos, bool _bUseOffset)
{
	//Allow some minor Z-diff ("move" z-pos closer to given height)
	CVec3Dfp32 Pos = GetPosition(_bUseOffset);
	fp32 Diff = _Pos[2] - Pos[2];
	if (GetType() & (WALK_CLIMB | JUMP_CLIMB))
	{	// Climbing SP code
		Pos += m_Position.GetRow(2) * DARKLING_PF_OFFSET;
	}
	else
	{	// Regular SP code
		if (Diff < -32.0f)
			Pos[2] -= 32.0f;
		else if (Diff > 32.0f)
			Pos[2] += 32.0f;
		else 
			Pos[2] = _Pos[2];
	}
	
	if (m_SqrRadius > 0.0f)
	{	// Use the given radius
		//We must be within radius (+ epsilon) distance of modified position
		return (Pos.DistanceSqr(_Pos) <= m_SqrRadius + 0.1f*0.1f);
	}
	else
	{	// Assume perfect placement ie, 8 units
		return (Pos.DistanceSqr(_Pos) <= Sqr(8.0f));
	}
};

//Check if given vector is sufficiently aligned with scene points direction
bool CWO_ScenePoint::IsAligned(CVec3Dfp32 _Dir, bool _bReverseDir)
{
	//Always aligned if no angle value
	if (m_CosAngle <= -1)
		return true;

	CVec3Dfp32 SPDir = GetDirection();
	if (_bReverseDir)
	{
		SPDir = -SPDir;
	}
	if (m_Flags & FLAGS_HEADINGONLY)
	{
		_Dir[2] = 0.0f;
		SPDir[2] = 0.0f;
		SPDir.Normalize();
	}

    //Normalize dir if necessary
    if (_Dir.LengthSqr() != 1.0f)
		_Dir.Normalize();

	//Allow angle deviancy (plus epsilon)
	return (_Dir * SPDir >= m_CosAngle - 0.01f);
};

// Make the scenepoint invalid for _Duration ticks
void CWO_ScenePoint::InvalidateScenepoint(CWorld_Server *_pWServer, int _Duration)
{
	int32 gameTick = _pWServer->GetGameTick();
	if (m_ReenableTick < gameTick+_Duration)
	{
		m_ReenableTick = gameTick+_Duration;
	}
	m_Resource.ReleaseAll();
};

//Request use of scene point for given time (frames, default is current frame only) with given priority 
//(or very low prio as default). Succeed if we're granted request, fail otherwise.
bool CWO_ScenePoint::Request(int _iUser, CWorld_Server* _pWServer, CWObject_ScenePointManager* _pSPM, int _Duration, uint8 _Prio)
{
	//Not available if unspawned
	if ((m_Flags & FLAGS_WAITSPAWN)||(!IsValidDynamic()))
	{
		return false;
	}

	//Check resource manager
	if (m_Resource.IsInitialized())
	{
		//Poll resource manager
		bool bNewUse;
		int32 gameTick = _pWServer->GetGameTick();
		if (_pWServer && m_Resource.Poll(_iUser, _Prio, gameTick, _Duration, &bNewUse))
		{
			if (bNewUse)
			{
				// Disabled or light out
				if ((gameTick < m_ReenableTick)||((m_Flags & FLAGS_LIGHTCONNECTED)&&(!_pWServer->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LIGHT_GETSTATUS),m_iConnectedLight))))
				{	// No yet ready to request
					m_Resource.Release(_iUser,gameTick);
					return false;
				}

				// Ugh, find any nearby scenepoints that are already held
				if (!(m_Flags & FLAGS_ALLOW_NEAR))
				{
					TArray<CWO_ScenePoint*> lNearbySPs;
					CBox3Dfp32 Box;
					CVec3Dfp32 Pos = GetPosition();
					Box.m_Min = Pos - CVec3Dfp32(64.0f,64.0f,64.0f);
					Box.m_Max = Pos + CVec3Dfp32(64.0f,64.0f,64.0f);
					_pSPM->Selection_Clear();
					// _pSPM->Selection_AddChunk_Hashed(Pos,64.0f,GetType(),0,0,NULL,0);
					_pSPM->Selection_AddBox_Approx(Box,GetType());
					lNearbySPs = _pSPM->Selection_Get();
					int N = lNearbySPs.Len();
					for (int i = 0; i < N; i++)
					{
						CWO_ScenePoint* pCurSP = lNearbySPs[i];
						if ((pCurSP)&&(pCurSP != this))
						{
							int iUser = pCurSP->m_Resource.GetRandomUser();
							if ((iUser)&&(iUser != _iUser))
							{
								if ((!(pCurSP->m_Flags & FLAGS_ALLOW_NEAR))&&(Pos.DistanceSqr(pCurSP->GetPosition()) <= Sqr(32.0f)))
								{	// Too close to another user
									m_Resource.Release(_iUser,gameTick);
#ifndef M_RTM
									CAI_Core* pAI = CAI_Core::GetAI(_iUser,_pWServer);
									if (pAI)
									{
										CVec3Dfp32 Start, End;
										Start = pAI->GetBasePos();
										End = GetPosition();
										pAI->Debug_RenderWire(Start,End,kColorWhite,30.0f);
										pAI->m_AH.DebugDrawScenePoint(this,true,kColorWhite,30.0f);
									}
#endif
									InvalidateScenepoint(_pWServer,600L);
									return false;
								}
							}
						}
					}
					_pSPM->Selection_Clear();
				}

				// Assume we can be reached, flag SP as unreachable when/if pathfinder fails to get there isntead
				/*
				// Here we check if the scenepoint really is reachable, if not we pause it for a couple of seconds
				if ((!(m_Type & (LOOK | JUMP_CLIMB)))&&(!(m_Flags & FLAGS_SIT))&&(m_lEnginepathSPs.Len() == 0)&&(m_SqrRadius > Sqr(0.0f)))
				{
					CXR_BlockNav* pGridPF = _pWServer->Path_GetBlockNav();

					if (pGridPF)
					{
						static const int BASESIZE = 24;
						static const int CROUCHHEIGHT = 32;
						bool WALLCLIMB = false;
						if (m_Type & WALK_CLIMB)
						{
							WALLCLIMB = true;
						}

						CVec3Dint16 GridPos = pGridPF->GetGridPosition(GetPosition()); 
						if (!(pGridPF->IsOnGround(GridPos, BASESIZE, WALLCLIMB))||
							(!pGridPF->IsTraversable(GridPos, BASESIZE, CROUCHHEIGHT, WALLCLIMB)))
						{
#ifndef M_RTM
							CVec3Dfp32 Pos = GetPosition();
							ConOutL(CStrF("§c3f0WARNING: Unreachable ScenePoint! (Position: %f, %f, %f;  Name: %s)", Pos[0], Pos[1], Pos[2], m_TargetName.Str()));
#endif
							m_Resource.Release(_iUser,gameTick);
							m_ReenableTick = (uint32)(gameTick + _pWServer->GetGameTicksPerSecond() * 5.0f);
							return false;
						}
					}
				}
				*/

				//Send take messages if new use
				for(int i = 0; i < m_lMessages_Take.Len(); i++)
					m_lMessages_Take[i].SendMessage(_iUser, _iUser, _pWServer);

				//Do secondary scene point activation events
				SecondaryScenePointEvent(CSecondaryScenePoint::EVENT_ONTAKE, _pSPM);
			}
            return true;
		}
		else
			//Request failed
			return false;
	}
	else
	{
		//No resource, unlimited users. Take is sent every time.... perhaps fix...
		for(int i = 0; i < m_lMessages_Take.Len(); i++)
			m_lMessages_Take[i].SendMessage(_iUser, _iUser, _pWServer);
		//Do secondary scene point activation events
		SecondaryScenePointEvent(CSecondaryScenePoint::EVENT_ONTAKE, _pSPM);
		return true;
	}
};	



//Notify scenepoint that we don't use it anymore
void CWO_ScenePoint::Release(int _iUser, CWorld_Server* _pWServer, CWObject_ScenePointManager* _pSPM)
{
	if ((!(m_ActivateFlags & FLAGS_ACTIVATE_RELEASE))||(m_ActivateFlags & FLAGS_ACTIVATED))
	{
		//Send release messages
		for(int i = 0; i < m_lMessages_Release.Len(); i++)
			m_lMessages_Release[i].SendMessage(_iUser, _iUser, _pWServer);

		//Do secondary scene point activation events
		SecondaryScenePointEvent(CSecondaryScenePoint::EVENT_ONRELEASE, _pSPM);
		if (m_bUnspawnOnRelease)
		{
			UnSpawn(_pSPM);
		}
		m_ActivateFlags &= ~FLAGS_ACTIVATED;
	}

	//Release resource (if held)
	m_Resource.Release(_iUser, _pWServer->GetGameTick());
};

//Notify scenepoint that we don't use it anymore
void CWO_ScenePoint::AnimationEvent(int _iUser, int _iEvent, CWorld_Server* _pWServer, CWObject_ScenePointManager* _pSPM)
{
	//Send release messages
	for(int i = 0; i < m_lMessages_Animation.Len(); i++)
		m_lMessages_Animation[i].SendMessage(_iUser, _iUser, _pWServer);

};

// true if not DYNAMIC, true if DYNAMIC and !FLAGS_MOVING and Up within threshold
bool CWO_ScenePoint::IsValidDynamic() const
{
	const CVec3Dfp32 Up = CVec3Dfp32(0,0,1);
	const fp32 MinCosUp = 0.99f;
	if (!(m_Type & DYNAMIC))
	{
		return(true);
	}
	else if ((m_Flags & FLAGS_MOVING) && !(m_Flags & FLAGS_ALLOWMOVING))
	{	// Ignore all moving scenepoints, except those explicitly allowed
		return false;
	}
	else if ((m_CosAngle > -1.0f)&&(GetUp() * Up < MinCosUp))
	{	// Stationary DYNAMIC with bad up vector
		return false;
	}

	return(true);
}

//Check if the given user (who belongs to the given teams) can use this scene point
bool CWO_ScenePoint::IsValid(CWorld_Server* _pWServer, uint32 _ObjectName, uint32 _OwnerName, const uint16* _piTeams, uint _nTeams, CWObject_Room* _pRoom) const
{
	//Invalid if not spawned

  	if (m_Flags & FLAGS_WAITSPAWN)
	{
		return false;
	}
	else if (!IsValidDynamic())
	{	// Ignore all moving scenepoints, except those that allow movement
		return false;
	}
	//Valid if it matches object or team or if both objects and teams are undefined
	else if (!m_lUsers[0] && !m_liTeams[0])
	{
		// Bad! Scenepoint with neither users nor teams!
#ifndef M_RTM
		// Only bitch twice every minute, tops!
		int32 gameTick = _pWServer->GetGameTick();
		if (m_ReenableTick < gameTick)
		{
			CVec3Dfp32 Pos = GetPosition();
			ConOutL(CStrF("§c3f0WARNING: ScenePoint with neither users nor teams, Fix! (Position: %f, %f, %f;  Name: %s)", Pos[0], Pos[1], Pos[2], m_TargetName.Str()));
			int32 gameTick = _pWServer->GetGameTick();
			const_cast<CWO_ScenePoint*>(this)->m_ReenableTick = gameTick+600;
		}
#endif
		return false;
	}
	else
	{
		// Check if we're connected to a light and that it's on
		if ((m_Flags & FLAGS_LIGHTCONNECTED) && !_pWServer->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LIGHT_GETSTATUS),m_iConnectedLight))
			return false;

		if ((_pRoom)&&(!_pRoom->PointIsInsideRoom(GetPosition())))
		{
			return(false);
		}

		if (_ObjectName != 0)
		{
			//Check objects
			for (int j = 0; j < CWO_ScenePoint::MAXUSERS; j++)
			{
				if (!m_lUsers[j])
					//Point has no further users
					break;

				if ((m_lUsers[j] == MHASH1('$ALL')) || (m_lUsers[j] == _ObjectName) || (m_lUsers[j] == _OwnerName))
				{
					//Found matching object!
					return true;
				}
			}
		}

		if (_nTeams > 0)
		{
			//Check teams
			for (int j = 0; j < CWO_ScenePoint::MAXUSERS; j++)
			{
				uint16 iTeam = m_liTeams[j];
				if(iTeam == 0)
					//Point has no further teams
					break;
				else
				{
					//Check team index against every team index supplied
					for (int k = 0; k < _nTeams; k++)
					{
						if(iTeam == _piTeams[k])
						{	
							//Found matching team
							return true;
						}
					}
				}
			}
		}

		//No matching team or object
		return false;
	}
};


void CWO_ScenePoint::SetOffsetAndSecondaryBehaviour(uint16 _iSecondaryBehaviour, CVec3Dfp32& _Offset, bool _bReverseDir)
{
	if (_iSecondaryBehaviour)
	{
		m_iSecondaryBehaviour = _iSecondaryBehaviour;
	}
	if ((VALID_POS(_Offset))&&(!_Offset.AlmostEqual(CVec3Dfp32(0.0f,0.0f,0.0f),0.1f)))
	{
		m_Offset = _Offset;
	}
	m_bReverseDir = _bReverseDir;
};

bool CWO_ScenePoint::IsWaypoint()
{
	if ((!(m_Type & ROAM))||(m_iBehaviour)||(m_BehaviourDuration != 0.0f)||(m_CosAngle >= 0.0f)||(m_SqrRadius < Sqr(16.0f)))
	{
		return(false);
	}
	else
	{
		return(true);
	}
};

void CWO_ScenePoint::SetLight(fp32 _Light,int _Tick)
{
	m_Light = _Light;
	m_LightTick = _Tick + SCENEPOINT_LIGHT_PERIOD;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_ScenePointManager
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ScenePointManager, CWObject, 0x0100);

void CWObject_ScenePointManager::OnCreate()
{
	//Reset
	m_lScenePointInitBlocks.SetGrow(1024);
	m_lScenePoints.SetGrow(1024);
	m_bHasSearchedForSpawners = false;
	m_nNextID = 0;

	m_bNoSave = 1;
	ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;
}


// Return value:
//	- upper 16 bits: The set (SET_REGULAR, SET_DARKLING or SET_DYNAMIC)
//  - lower 16 bits: Typemask (ROAM, SEARCH, TACTICAL, COVER, TALK, LOOK, DOOR)
static M_INLINE uint32 GetContainerIndices(uint16 _nType)
{
	uint iSet = (_nType & CWO_ScenePoint::DYNAMIC) ? CWO_ScenePointHash::SET_DYNAMIC :
	            (_nType & CWO_ScenePoint::DARKLING) ? CWO_ScenePointHash::SET_DARKLING : CWO_ScenePointHash::SET_REGULAR;

	const uint16 TypeMask = CWO_ScenePoint::TYPE_MASK;
	uint16 TypeBase = _nType & TypeMask;
	return (iSet << 16) | TypeBase;
}

static uint8 GetContainerAndMasks(uint16 _Types, uint16* _pRetMaskType, uint16* _pRetMaskAccept, uint16* _pRetMaskReject)
{
	enum { DarklingJumpMask = CWO_ScenePoint::WALK_CLIMB | CWO_ScenePoint::JUMP_CLIMB };
	uint16 MaskAccept, MaskReject;
	if (_Types & CWO_ScenePoint::DARKLING)
	{
		if (_Types & CWO_ScenePoint::WALK_CLIMB)
		{
			MaskAccept = CWO_ScenePoint::DARKLING;				// accept all
			MaskReject = 0;										// reject none
		}
		else if (_Types & CWO_ScenePoint::JUMP_CLIMB)
		{
			MaskAccept = DarklingJumpMask;						// accept ground or climb
			MaskReject = 0;										// reject none
		}
		else // is not at ground-point nor climb-point
		{
			MaskAccept = CWO_ScenePoint::DARKLING;				// accept all
			MaskReject = CWO_ScenePoint::JUMP_CLIMB;			// reject climb-points
		}
	}
	else
	{
		MaskAccept = 0xFFFF;	// not really needed for non-darkling selections...
		MaskReject = 0x0000;	// not really needed for non-darkling selections...
	}

	const uint16 TypeMask = CWO_ScenePoint::TYPE_MASK;

	*_pRetMaskType = _Types & TypeMask;
	*_pRetMaskAccept = MaskAccept;
	*_pRetMaskReject = MaskReject;

	uint32 x = GetContainerIndices(_Types);
	uint16 iSet = (x >> 16);
	uint16 Types = (x & 0xffff);
	M_ASSERT(IsPow2(Types), "Scenepoint queries only support one type per call");
	uint8 nIndex = Log2(Types) + iSet * CWO_ScenePointHash::NUM_TYPES;
	return nIndex;
}


int CWObject_ScenePointManager::AddScenePoint(const CWO_ScenePoint& _Point, const CWO_ScenePointInitBlock& _InitBlock)
{
	int iPoint = m_lScenePoints.Add(_Point);
	m_lScenePointInitBlocks.Add(_InitBlock);

	// Check number of types used for this point
	uint16 Types = GetContainerIndices(_Point.m_Type) & 0xffff;
	uint8 nTypes = 0;
	for (uint8 i = 0; i < CWO_ScenePointHash::NUM_TYPES; i++)
		if (Types & M_BitD(i)) nTypes++;

	m_lScenePoints[iPoint].m_ID = 1 + m_nNextID;
	m_nNextID += nTypes;

	return iPoint;
}


void CWObject_ScenePointManager::InitHash()
{
	MSCOPE(CWObject_ScenePointManager::InitHash, SCENEPOINTS);

	uint nScenePoints = m_lScenePoints.Len();
	uint nIDs = m_nNextID;

	CBox3Dfp32 Bound(CVec3Dfp32(-1.0f), CVec3Dfp32(1.0f));
	if (nScenePoints)
	{
		Bound = CBox3Dfp32(_FP32_MAX, -_FP32_MAX);
		for (uint i = 0; i < nScenePoints; i++)
			Bound.Expand( m_lScenePoints[i].GetPosition() );
	}

	int nMemUsed = -MRTC_MemUsed();
	m_Hash.Create(nIDs);

	// Init hash grids
	for (uint8 iSet = 0; iSet < CWO_ScenePointHash::NUM_SETS; iSet++)
	{
		for (uint8 iType = 0; iType < CWO_ScenePointHash::NUM_TYPES; iType++)
		{
			uint8 iGrid = iType + iSet * CWO_ScenePointHash::NUM_TYPES;

			int nBoxes = 16;
			if ((iSet == CWO_ScenePointHash::SET_DARKLING) && (iType == CWO_ScenePoint::ROAM))
				nBoxes = 32; // just a tweak..

			m_Hash.InitGrid(iGrid, nBoxes, Bound);
		}
	}

	// Add points
	for (uint iSP = 0; iSP < nScenePoints; iSP++)
	{
		const CWO_ScenePoint& p = m_lScenePoints[iSP];
		m_Hash.Insert(p, iSP);
	}

	//m_Hash.DumpHashUsage();
	nMemUsed += MRTC_MemUsed();
	M_TRACE("SCENEPOINT_HASH: Num points: %d, Mem used: %d KiB\n", nScenePoints, nMemUsed >> 10);

	//TEMP hack: make sure selection array is allocated
	Selection_AddAll();
	Selection_Clear();

	m_lActivePoints.SetLen(0);
	m_lActivePoints.SetGrow(nScenePoints);
}


void CWObject_ScenePointManager::Selection_Clear()
{
	m_lSelection.QuickSetLen(0);
}

void CWObject_ScenePointManager::Selection_AddAll()
{
	// How to fill an array of sequential pointers in the slowest possible way:
/*	m_lSelection.QuickSetLen(0);
	for (int i = 0; i < m_lScenePoints.Len(); i++)
	{
		m_lSelection.Add(&m_lScenePoints[i]);
	}*/

	m_lSelection.QuickSetLen(m_lScenePoints.Len());
	CWO_ScenePoint* pSP = m_lScenePoints.GetBasePtr();
	TAP<CWO_ScenePoint*> lSel = m_lSelection;
	for(int i = 0; i < lSel.Len(); i++)
		lSel[i] = pSP++;
}

//Add all scenepoints that are within the given range of the given position, as well as a bunch that aren't :)
//Optionally we exclude those unavailable to the given teams and object. Also, if a lower range is 
//provided, we exclude all those that would have been selected given that range.
void CWObject_ScenePointManager::Selection_AddChunk(const CVec3Dfp32& _Pos, fp32 _RangeDelimiter, uint32 _ObjectName, uint32 _OwnerName, const uint16* _piTeams, uint _nTeams, CWObject_Room* _pRoom)
{
	MSCOPESHORT(CWObject_ScenePointManager::Selection_AddChunk);
	TArray<CWO_ScenePoint *> lSelection;

	//Use RangeDelimiter and LowerRangeDelimiter to get selection
	//Just add all scene points for now... TODO: optimize with proximity hash later
	TAP<CWO_ScenePoint> pSP = m_lScenePoints;
	lSelection.SetLen(pSP.Len());
	for (int i = 0; i < pSP.Len(); i++)
		lSelection[i] = &pSP[i];

    //Check which scene points should be kept of those selected above
	if ((_ObjectName != 0) || (_nTeams != 0))
	{	
		//Check each entry
		TAP<CWO_ScenePoint*> pSelection = lSelection;
		for (int i = 0; i < pSelection.Len(); i++)
		{
			CWO_ScenePoint * pPoint = lSelection[i];
			if (pPoint && pPoint->IsValid(m_pWServer, _ObjectName, _OwnerName, _piTeams, _nTeams))
			{
				//Some criteria met, add scene point
				m_lSelection.Add(pPoint);
			}
		}
	}
	else
	{
		//No further pruning needed, add all entries
		m_lSelection.Insertx(m_lSelection.Len(), lSelection.GetBasePtr(), lSelection.Len());
	}
};


void CWObject_ScenePointManager::Selection_AddRange(const CVec3Dfp32 &_Pos, fp32 _Range)
{
	fp32 RangeSqr = _Range * _Range;
	for(int i = 0; i < m_lScenePoints.Len(); i++)
	{
		if((_Pos - CVec3Dfp32::GetRow(m_lScenePoints[i].m_Position, 3)).LengthSqr() <= RangeSqr)
			m_lSelection.Add(&m_lScenePoints[i]);
	}
}

const TArray<CWO_ScenePoint*>& CWObject_ScenePointManager::GetScenePoints(int _iObject, fp32 _MaxRadius, int _Types, CWObject_Room* _pRoom)
{
	Selection_Clear();

	if (_iObject > 0)
	{
		CWObject* pObj = m_pWServer->Object_Get(_iObject);
		if (pObj)
		{
			// Get object position
			const CVec3Dfp32& Pos = pObj->GetPosition();

			// Get teams..
			uint16 liTeams[32];
			uint nTeams = 0;

			CWObject_Interface_AI* pObjAI = pObj->GetInterface_AI();
			if (pObjAI)
				nTeams = pObjAI->AI_GetTeams(liTeams, sizeof_buffer(liTeams));

			// Select points!
			Selection_AddChunk_Hashed(Pos, _MaxRadius, _Types, _iObject, pObj->m_iOwner, liTeams, nTeams);
		}
	}

	return m_lSelection;
}


//void CWObject_ScenePointManager::Selection_AddChunk_Hashed(CWorld_Server* _pWServer,	uint16 _Types, const CVec3Dfp32& _Pos, fp32 _MaxRange, int _iObject)
void CWObject_ScenePointManager::Selection_AddChunk_Hashed(const CVec3Dfp32& _Pos, fp32 _MaxRange, uint16 _Types, uint32 _ObjectName, uint32 _OwnerName, const uint16* _piTeams, uint _nTeams)
{
	MSCOPESHORT(CWObject_ScenePointManager::Selection_AddChunk_Hashed);
	//
	// TODO: Add a proxy function that calls this one with increasing radius, until a scenepoint is returned
	//       (queries with small radius are faster)
	//       Requirement for this to work: A history of position of previous queries/iObjects, or a query-handle.
	//
	// TODO2: Add support for dynamic scene-points
	//

//uint32 tTotal = 0;
//QStart(tTotal);

	fp32 RangeSqr   = Sqr(_MaxRange);
//	fp32 RangeSqr_Z = Sqr(_MaxRange * 0.25f);
	CBox3Dfp32 Box(_Pos - CVec3Dfp32(_MaxRange), _Pos + CVec3Dfp32(_MaxRange));

	uint16 MaskType, MaskAccept, MaskReject;
	uint8 iContainer = GetContainerAndMasks(_Types, &MaskType, &MaskAccept, &MaskReject);

	uint16 aIDs[250];
	uint nIDs = m_Hash.EnumerateBox(iContainer, Box, aIDs, sizeof(aIDs)/sizeof(aIDs[0]));

	// Prepare to add points to selection
	CWO_ScenePoint* pPoints = m_lScenePoints.GetBasePtr();
	CWO_ScenePoint** pSelection = m_lSelection.GetBasePtr();
	uint nCapacity = m_lSelection.ListAllocatedSize() / sizeof(CWO_ScenePoint*);
	uint nFirstFree = m_lSelection.Len();
	uint nFree = nCapacity - nFirstFree;
	uint nToAdd = Min(nIDs, nFree);

	// Check the points we got..
	for (uint j = 0; j < nToAdd; j++)
	{
		uint iPoint = aIDs[j];
		CWO_ScenePoint& p = pPoints[iPoint];
		M_ASSERT((p.m_Type & _Types & MaskType) != 0, "!");

		if (((p.m_Type & MaskAccept) == 0) || ((p.m_Type & MaskReject) != 0))
			continue;

		const CVec3Dfp32& Pos = CVec3Dfp32::GetRow(p.m_Position, 3);
		fp32 dx = Pos.k[0] - _Pos.k[0];
		fp32 dy = Pos.k[1] - _Pos.k[1];
		fp32 dz = Pos.k[2] - _Pos.k[2];
		fp32 DistanceSqr = dx*dx + dy*dy + dz*dz*16;		// weight Z harder (squeezed sphere) 
		if (DistanceSqr > RangeSqr)
			continue;

		if (!p.IsValid(m_pWServer, _ObjectName, _OwnerName, _piTeams, _nTeams))
			continue;

		pSelection[nFirstFree++] = &p;
	}
	m_lSelection.QuickSetLen(nFirstFree);

//QStop(tTotal);

//	DEBUG_LOG(CStrF("HASHED: Result(_Types=%d): nAdded = %d (nIDs = %d), tTotal: %d", _Types, m_lSelection.Len(), nIDs, tTotal));
//	for (uint i=0; i<m_lSelection.Len(); i++)
//		DEBUG_LOG(CStrF(" - %d: '%s'", m_lSelection[i]->m_ID & 0xFFFF, m_lSelection[i]->GetName().Str()));
}


void CWObject_ScenePointManager::Selection_AddBox_Approx(const CBox3Dfp32& _Box, uint16 _Types)
{
	{ // check for multiple types
		uint Mask = GetContainerIndices(_Types) & 0xffff;
		if (Mask && !IsPow2(Mask))
	{
		// the system only allows one type at a time in the input, but the game sometimes passes a value with several type flags set, so this piece of code works around that
		// (recursively call the function for each bit set in the input)
		for (uint i = 0, Mask = 1; i < CWO_ScenePointHash::NUM_TYPES; i++, Mask<<=1)
			if (_Types & Mask)
				Selection_AddBox_Approx(_Box, (_Types & ~CWO_ScenePoint::TYPE_MASK) | Mask);
		return;
	}
	}

	uint16 MaskType, MaskAccept, MaskReject;
	uint8 iContainer = GetContainerAndMasks(_Types, &MaskType, &MaskAccept, &MaskReject);

	uint16 aIDs[250];
	uint nIDs = m_Hash.EnumerateBox(iContainer, _Box, aIDs, sizeof(aIDs)/sizeof(aIDs[0]));

	// Prepare to add points to selection
	CWO_ScenePoint* pPoints = m_lScenePoints.GetBasePtr();
	CWO_ScenePoint** pSelection = m_lSelection.GetBasePtr();
	uint nCapacity = m_lSelection.ListAllocatedSize() / sizeof(CWO_ScenePoint*);
	uint nFirstFree = m_lSelection.Len();
	uint nFree = nCapacity - nFirstFree;
	uint nToAdd = Min(nIDs, nFree);

	// Check the points we got..
	for (uint j=0; j < nToAdd; j++)
	{
		uint iPoint = aIDs[j];
		CWO_ScenePoint& p = pPoints[iPoint];
		M_ASSERT((p.m_Type & _Types & MaskType) != 0, "!");

		if (((p.m_Type & MaskAccept) == 0) || ((p.m_Type & MaskReject) != 0))
			continue;

		pSelection[nFirstFree++] = &p;
	}
	m_lSelection.QuickSetLen(nFirstFree);
}


void CWObject_ScenePointManager::Selection_AddByName(const char* _pName)
{
	TAP<CWO_ScenePoint> pScenePoints = m_lScenePoints;
	for (uint i = 0; i < pScenePoints.Len(); i++)
	{
		CWO_ScenePoint* pSP = &pScenePoints[i];
		if (pSP->GetName().CompareNoCase(_pName) == 0)
			m_lSelection.Add(pSP);
	}
}


void CWObject_ScenePointManager::Selection_AddAffectingObject(uint32 _ObjectName, uint32 _OwnerName)
{
	TAP<CWO_ScenePoint> pSP = m_lScenePoints;
	for(int i = 0; i < pSP.Len(); i++)
	{
		CWO_ScenePoint* pPoint = &pSP[i];
		const CNameHash* pUsers = pPoint->m_lUsers;
		for(int j = 0; j < CWO_ScenePoint::MAXUSERS; j++)
		{
			if (!pUsers[j])
				break;
			
			if ((pUsers[j] == _ObjectName) || (pUsers[j] == _OwnerName))
			{
				m_lSelection.Add(pPoint);
				break;
			}
		}
	}
}

void CWObject_ScenePointManager::Selection_AddAffectingTeam(int _iTeam)
{
	TAP<CWO_ScenePoint> pSP = m_lScenePoints;
	for(int i = 0; i < pSP.Len(); i++)
	{
		CWO_ScenePoint* pPoint = &pSP[i];
		const uint16* pliTeams = pPoint->m_liTeams;
		for(int j = 0; j < CWO_ScenePoint::MAXTEAMS; j++)
		{
			if (pliTeams[j] == 0)
				break;

			if (pliTeams[j] == _iTeam)
			{
				m_lSelection.Add(pPoint);
				break;
			}
		}
	}
}


void CWObject_ScenePointManager::Selection_AddInRoom(const CWObject_Room& _Room)
{
	// add all points which are potentially in the room
	uint iFirstNew = m_lSelection.Len();
	Selection_AddBox_Approx(*_Room.GetAbsBoundBox(), CWO_ScenePoint::TYPE_MASK);
	Selection_AddBox_Approx(*_Room.GetAbsBoundBox(), CWO_ScenePoint::TYPE_MASK | CWO_ScenePoint::DYNAMIC);

	// remove points that aren't actually in the room
	TAP<CWO_ScenePoint*> pSelection = m_lSelection;
	uint nNewLen = pSelection.Len();
	for (uint i = iFirstNew; i < nNewLen; i++)
	{
		if (!_Room.PointIsInsideRoom(pSelection[i]->GetPos()))
		{
			pSelection[i] = pSelection[--nNewLen]; // copy last elem to current, decrease length
			i--; // revisit current entry
		}
	}
	m_lSelection.QuickSetLen(nNewLen);
}


void CWObject_ScenePointManager::Selection_RemoveNotInRoom(const CWObject_Room& _Room)
{
	TAP<CWO_ScenePoint*> pSelection = m_lSelection;
	uint nNewLen = pSelection.Len();
	for (uint i = 0; i < nNewLen; i++)
	{
		if (!_Room.PointIsInsideRoom(pSelection[i]->GetPos()))
		{
			pSelection[i] = pSelection[--nNewLen]; // copy last elem to current, decrease length
			i--; // revisit current entry
		}
	}
	m_lSelection.QuickSetLen(nNewLen);
}



//Find scenepoint given target name. Not optimized yet.
CWO_ScenePoint* CWObject_ScenePointManager::Find(const char *_pTargetName)
{
	//Brute force! Fix hash or something...
	CStr pName = CStr(_pTargetName).UpperCase();
	for (int i = 0; i < m_lScenePoints.Len(); i++)
	{
		CWO_ScenePoint * pPoint = &m_lScenePoints[i];
		if (pPoint && (pName == pPoint->m_TargetName))
			return pPoint;
	}
	return NULL;
};

//Find random scenepoint given target name. Not optimized yet.
CWO_ScenePoint* CWObject_ScenePointManager::FindRandom(int _iObject, const char* _pTargetName)
{
	CWObject* pObj = m_pWServer->Object_Get(_iObject);
	if (!pObj)
		return NULL;
	
	CWObject_Interface_AI* pObjAI = pObj->GetInterface_AI();
	if (!pObjAI)
		return NULL;

	uint32 ObjectNameHash = pObj->GetNameHash();

	// Get teams..
	uint16 liTeams[16];
	uint nTeams = pObjAI->AI_GetTeams(liTeams, sizeof_buffer(liTeams));
	CNameHash TargetName(_pTargetName);
	TArray<int> lHits;

	//Brute force! Fix hash or something...
	for (int i = 0; i < m_lScenePoints.Len(); i++)
	{
		CWO_ScenePoint* pPoint = &m_lScenePoints[i];
		if (pPoint && (TargetName == CNameHash(pPoint->m_TargetName)))
		{
			if (!pPoint->IsValid(m_pWServer, ObjectNameHash, ObjectNameHash, liTeams, nTeams))
				continue;
			lHits.Add(i);
		}
	}

	int N = lHits.Len();
	if (N)
	{
		int Pick = (int)(Random * 0.999f * N);
		CWO_ScenePoint* pPoint = &m_lScenePoints[lHits[Pick]];
		return(pPoint);
	}

	return NULL;
};

CWO_ScenePoint* CWObject_ScenePointManager::FindRandomInRoom(uint32 _Type, const CWObject_Room* _pRoom)
{
	TArray<int> lHits;

	//Brute force! Fix hash or something...
	for (int i = 0; i < m_lScenePoints.Len(); i++)
	{
		CWO_ScenePoint* pPoint = &m_lScenePoints[i];
		if ((pPoint)&&(pPoint->CheckType(_Type)))
		{
			if (pPoint->m_Flags & CWO_ScenePoint::FLAGS_WAITSPAWN)
			{
				continue;
			}
			if ((_pRoom)&&(_pRoom->PointIsInsideRoom(pPoint->GetPosition())))
			{
				lHits.Add(i);
			}
		}
	}

	int N = lHits.Len();
	if (N)
	{
		int Pick = (int)(Random * 0.999f * N);
		CWO_ScenePoint* pPoint = &m_lScenePoints[lHits[Pick]];
		return(pPoint);
	}

	return NULL;
}


/* -not used-
int32 CWObject_ScenePointManager::SpawnNamed(CStr _TargetName, bool _bSpawn)
{
	M_ASSERT(m_pWServer, "No server!");
	if (_TargetName == CStr(""))
	{
		return(0);
	}

	CStr Target = CStr(_TargetName).UpperCase();
	int nPoints = m_lScenePoints.Len();
	int Count = 0;
	for(int i = 0; i < nPoints; i++)
	{	
		CWO_ScenePoint* pSP = GetScenePointFromIndex(i);
		if (pSP)
		{
			if ((_bSpawn)&&(!pSP->IsSpawned()))
			{
				if (Target == pSP->m_TargetName)
				{
					pSP->Spawn(this);
					Count++;
				}
			}
			else if ((!_bSpawn)&&(pSP->IsSpawned()))
			{
				if (Target == pSP->m_TargetName)
				{
					pSP->UnSpawn(this);
					Count++;
				}	
			}
		}
	}
	
	return(Count);
};
*/

/* -not used-
//Get object indices of all spawners. Optionally only spawners whose spawn have the given target name are considered.
TArray<int> CWObject_ScenePointManager::GetSpawners(CStr _TargetName)
{
	M_ASSERT(m_pWServer, "No server!");
	if (!m_bHasSearchedForSpawners)
	{
		//Get all spawners
		m_lSpawners.Clear();
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddAll(Selection);
		const int16 *pSel;
		int nSel = m_pWServer->Selection_Get(Selection, &pSel);
		CWObject_Message CanSpawn(OBJMSG_SPAWNER_CANSPAWN);
		for(int i = 0; i < nSel; i++)
		{	
			if (m_pWServer->Message_SendToObject(CanSpawn, pSel[i]))
			{
				//Found spawner
				m_lSpawners.Add(pSel[i]);
			}
		}
		m_bHasSearchedForSpawners = true;
	}

	if (_TargetName == CStr(""))
	{
		return m_lSpawners;
	}
	else
	{
		//Check target names of spawners
		TArray<int> lRes; lRes.Clear();
		CWObject_Message CanSpawnNamed(OBJMSG_SPAWNER_CANSPAWNNAMED);
		CanSpawnNamed.m_pData = (char *)_TargetName;
		for (int i = 0; i < m_lSpawners.Len(); i++)
		{
			if (m_pWServer->Message_SendToObject(CanSpawnNamed, m_lSpawners[i]))
			{
				lRes.Add(m_lSpawners[i]);
			}
		}
		return lRes;
	}
}
*/

//Get scenepoint list index of given scenepoint. Currently only used for save/load purposes
int CWObject_ScenePointManager::GetScenePointIndex(const CWO_ScenePoint* _pScenePoint)
{
	const CWO_ScenePoint* pStart = m_lScenePoints.GetBasePtr();
	uint iSP = int(_pScenePoint - pStart);

	// Out of range?
	if (iSP >= m_lScenePoints.Len())
		return -1;

	return iSP;
}

//Get scenepoint from index. This is currently used to preserve scenepoints over saves.
CWO_ScenePoint* CWObject_ScenePointManager::GetScenePointFromIndex(int _i)
{
	if (m_lScenePoints.ValidPos(_i))
		return &(m_lScenePoints[_i]);
	else
		return NULL;
}


// Update light
void CWObject_ScenePointManager::SetScenePointLight(CWO_ScenePoint* _pScenePoint,fp32 _Light,int _Tick)
{
	if (_pScenePoint)
		_pScenePoint->SetLight(_Light,_Tick);
}

void CWObject_ScenePointManager::SetScenePointLight(int _i,fp32 _Light,int _Tick)
{
	CWO_ScenePoint* pSP = GetScenePointFromIndex(_i);
	if (pSP)
	{
		SetScenePointLight(pSP,_Light,_Tick);
	}
};

int32 CWObject_ScenePointManager::GetEnginepathSPCount(CWO_ScenePoint* _pScenePoint)
{
	if (_pScenePoint)
	{
		return(_pScenePoint->m_lEnginepathSPs.Len());
	}
	else
	{
		return(0);
	}
};

CWO_ScenePoint* CWObject_ScenePointManager::GetNthEnginepathSP(CWO_ScenePoint* _pScenePoint,int32 _iESP)
{
	if ((_pScenePoint)&&(_iESP >= 0)&&(_iESP < _pScenePoint->m_lEnginepathSPs.Len()))
	{
		int ID = _pScenePoint->m_lEnginepathSPs[_iESP].m_iSP;
		return(GetScenePointFromIndex(ID));
	}
	else
	{
		return(NULL);
	}
};

int32 CWObject_ScenePointManager::GetEnginepathID(CWO_ScenePoint* _pFromSP,CWO_ScenePoint* _pToSP)
{
	if ((_pFromSP)&&(_pToSP))
	{
		int32 iToSP = GetScenePointIndex(_pToSP);
		for (int i = 0; i < _pFromSP->m_lEnginepathSPs.Len(); i++)
		{
			if (_pFromSP->m_lEnginepathSPs[i].m_iSP == iToSP)
			{
				return(_pFromSP->m_lEnginepathSPs[i].m_iEP);
			}
		}
	}
	return(0);
};


void CWObject_ScenePointManager::OnRefresh()
{
	MSCOPESHORT(CWObject_ScenePointManager::OnRefresh);

	// Update all active dynamic scenepoints
	CWO_ScenePoint* pScenePoints = m_lScenePoints.GetBasePtr();
	CWO_ScenePoint** pActivePoints = m_lActivePoints.GetBasePtr();
	uint nActivePoints = m_lActivePoints.Len();

	for (uint i = 0; i < nActivePoints; i++)
	{
		CWO_ScenePoint& p = *pActivePoints[i];
		M_ASSERTHANDLER(p.m_Flags & CWO_ScenePoint::FLAGS_MOVING, "Inactive but in active list?!", continue);
		M_ASSERT(p.m_Type & CWO_ScenePoint::DYNAMIC, "!");

		CWObject* pParent = m_pWServer->Object_Get(p.m_iParent);
		if (pParent) 
		{
			const CMat4Dfp32& OldPos = p.GetPositionMatrix();
			const CMat4Dfp32& LocalPos = p.GetLocalPositionMatrix();
			const CMat4Dfp32& ParentPos = pParent->GetPositionMatrix();

			// Calculate new position
			CMat4Dfp32 NewPos;
			LocalPos.Multiply(ParentPos, NewPos);

			// Check if we're moving
			/*
			if (NewPos.AlmostEqual(OldPos, 0.01f))
				p.m_Flags &= ~CWO_ScenePoint::FLAGS_MOVING;
			else
				p.m_Flags |=  CWO_ScenePoint::FLAGS_MOVING;
			*/


			if ((NewPos.GetRow(3).DistanceSqr(OldPos.GetRow(3)) >= 4.0f)||
				((p.m_CosAngle > -1.0f)&&((NewPos.GetRow(0) * OldPos.GetRow(0) < 0.985f)||(NewPos.GetRow(1) * OldPos.GetRow(1) < 0.985f))))
			{	// Obj has moved (or rotated) far enough to be considered moving (4 units or 5 degrees)
				p.m_Flags |=  CWO_ScenePoint::FLAGS_MOVING;
			}
			else
			{
				p.m_Flags &= ~CWO_ScenePoint::FLAGS_MOVING;
			}

			// Update hash grid if scenepoint moved
			if (p.m_Flags & CWO_ScenePoint::FLAGS_MOVING)
			{
				{
					// Store new pos
					if (p.m_CosAngle > -1.0f)
					{	// Regular dynamic
						p.m_Position = NewPos;
					}
					else
					{	// Translate only dynamic
						p.m_Position.GetRow(3) = NewPos.GetRow(3);
					}
					//Calculate forward heading vector
					if (p.m_ArcOffset != 0.0f)
					{
						CMat4Dfp32 Mat = p.m_Position;
						Mat.M_x_RotZ(p.m_ArcOffset);
						p.m_FwdHeading = Mat.GetRow(0);
					}
					else
					{
						p.m_FwdHeading = p.m_Position.GetRow(0);
					}
					if (p.m_Flags & CWO_ScenePoint::FLAGS_HEADINGONLY)
					{
						p.m_FwdHeading[2] = 0.0f;
					}
					p.m_FwdHeading.Normalize();

					// Unspawn Roam scenepoints that move
					if ((p.m_Type & CWO_ScenePoint::ROAM)&&(!(p.m_Flags & CWO_ScenePoint::FLAGS_ALLOWMOVING)))
					{
						p.UnSpawn(this);
						for (int j = 0; i < p.m_lSecondarySPs.Len(); j++)
						{	//Get scene point
							CWO_ScenePoint* pSSP = GetScenePointFromIndex(p.m_lSecondarySPs[j].m_iSP);
							if ((pSSP)&&(p.m_lSecondarySPs[j].m_Flags & CWO_ScenePoint::CSecondaryScenePoint::ACTION_ONMOVE_UNSPAWN))
							{
								pSSP->UnSpawn(this);
							}
						}
					}
					uint iPoint = &p - pScenePoints;
					m_Hash.Update(p, iPoint);

				}
			}
		}
		else
		{	//If parent is removed, unspawn scenepoint and its children!
			p.UnSpawn(this);
			for (int j = 0; i < p.m_lSecondarySPs.Len(); j++)
			{	//Get scene point
				CWO_ScenePoint* pSSP = GetScenePointFromIndex(p.m_lSecondarySPs[j].m_iSP);
				if ((pSSP)&&(p.m_lSecondarySPs[j].m_Flags & CWO_ScenePoint::CSecondaryScenePoint::ACTION_ONMOVE_UNSPAWN))
				{
					pSSP->UnSpawn(this);
				}
			}
		}
	}

	// Clean up the list
	for (uint i = 0; i < nActivePoints; i++)
	{
		CWO_ScenePoint* pPoint = pActivePoints[i];
		if (!(pPoint->m_Flags & CWO_ScenePoint::FLAGS_MOVING) || (pPoint->m_Flags & CWO_ScenePoint::FLAGS_WAITSPAWN))
		{
			// Remove it
			pActivePoints[i] = pActivePoints[nActivePoints - 1];
			nActivePoints--;
			i--;
		}
	}
	m_lActivePoints.QuickSetLen(nActivePoints);
//	if (nActivePoints) M_TRACE("OnRefresh, %d active points\n", nActivePoints);

	// Debug render all scene points
	//for (int i = 0; i < m_lScenePoints.Len(); i++)
	//{
	//	m_pWServer->Debug_RenderWire(m_lScenePoints[i].GetPosition(), m_lScenePoints[i].GetPosition() + CVec3Dfp32(0,0,100), 0xff00ff00);
	//}
}


void CWObject_ScenePointManager::OnSpawnWorld2() 
{
	// nothing.
	// initalization of scenepoints used to be done here,
	// but was moved to "InitManager()" instead, so that the game-object
	// can do the init manually.
}

// Since we have too many scenepoints on a map, move the init info to the manager 
void CWObject_ScenePointManager::InitManager()
{
	MSCOPESHORT(CWObject_ScenePointManager::OnSpawnWorld2);
	//
	// This will do final initialization of scenepoints and then create scenepoint hash
	// NOTE: do not move this to OnSpawnWorld(), since other objects might add SPs
	//       in their OnSpawnWorld()
	//

	uint NumScenePoints = m_lScenePoints.Len();
	// Shouldn't happen
	if (NumScenePoints != m_lScenePointInitBlocks.Len())
		return;

	CWO_ScenePoint* lPoints = m_lScenePoints.GetBasePtr();
//	CWO_ScenePointInitBlock* lBlocks = m_lScenePointInitBlocks.GetBasePtr();

	const CNameHash DollarRoomHash("$ROOM");
	for (int32 i = 0; i < NumScenePoints; i++)
	{
		CWO_ScenePoint& Point = lPoints[i];
		CWO_ScenePointInitBlock& InitBlock = m_lScenePointInitBlocks[i];
		{
			// Evaluate users
			int iTarget = 0;
			for (int i = 0; i < InitBlock.m_lUsers.Len(); i++)
			{
				const CNameHash& NameHash = InitBlock.m_lUsers[i];

				if (iTarget < CWO_ScenePoint::MAXUSERS)
				{
					if ((uint32)NameHash != 0)
					{	// Skip zeroes as Ogier sometimes give us SPs that has a USER1, omitting USER0
						Point.m_lUsers[iTarget++] = NameHash;
					}
				}
				else
					ConOutL(CStrF("§c3f0WARNING: ScenePoint '%s' %s has too many users! (1. Target = '%s')",
						Point.GetName().Str(), Point.GetPosition().GetString().Str(), NameHash.DbgName().Str()));
			}
			if (iTarget < CWO_ScenePoint::MAXUSERS)
				Point.m_lUsers[iTarget] = 0;
		}

		{
			// Evaluate teams
			int nTeam = 0;
			for (int i = 0; i < InitBlock.m_lTeams.Len(); i++)
			{
				const CNameHash& NameHash = InitBlock.m_lTeams[i];
				int iTeam = m_pWServer->Selection_GetSingleTarget(NameHash);
				if(iTeam > 0)
				{
					if (nTeam < CWO_ScenePoint::MAXTEAMS)
					{
						Point.m_liTeams[nTeam++] = iTeam;
					}
					else
					{
						ConOutL(CStrF("§c3f0WARNING: ScenePoint '%s' %s has too many teams - Team '%s' ignored",
							Point.GetName().Str(), Point.GetPosition().GetString().Str(), NameHash.DbgName().Str()));
					}
				}
			}
			if (nTeam < CWO_ScenePoint::MAXTEAMS)
				Point.m_liTeams[nTeam] = 0;
		}

		{
			// Check room
			if (InitBlock.m_RoomNameHash)
			{
				CWO_RoomManager* pRoomManager = (CWO_RoomManager*)m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETROOMMANAGER),m_pWServer->Game_GetObjectIndex());
				if (pRoomManager)
				{
					Point.m_pRoom = pRoomManager->GetRoom(InitBlock.m_RoomNameHash);
					if ((!Point.m_pRoom)&&(InitBlock.m_RoomNameHash == DollarRoomHash))
					{
						Point.m_pRoom = pRoomManager->GetRoom(Point.GetPosition(false));
					}
				}
			}
		}
		{
			// Check connectedlight
			if (InitBlock.m_ConnectedLightHash)
			{
				Point.m_iConnectedLight = m_pWServer->Selection_GetSingleTarget(InitBlock.m_ConnectedLightHash);
			}
		}

		for(int i = 0; i < Point.m_lMessages_Activate.Len(); i++)
			Point.m_lMessages_Activate[i].SendPrecache(-1, m_pWServer);
		for(int i = 0; i < Point.m_lMessages_Take.Len(); i++)
			Point.m_lMessages_Take[i].SendPrecache(-1, m_pWServer);
		for(int i = 0; i < Point.m_lMessages_Release.Len(); i++)
			Point.m_lMessages_Release[i].SendPrecache(-1, m_pWServer);

		INVALIDATE_POS(Point.m_PathPos);
		/*
		//Calculate path position
		CVec3Dfp32 PathPos = CWObject_ScenePoint::GetPathPosition(m_pWServer, Point.GetPosition(), 3, 0);
		if (PathPos == CVec3Dfp32(_FP32_MAX))
		{
			//Try a little wider
			PathPos = CWObject_ScenePoint::GetPathPosition(m_pWServer, Point.GetPosition(), 10, 3);
			if (PathPos == CVec3Dfp32(_FP32_MAX))
			{
				//Can't find suitable path position, use original position. 
				//Scene point will propably be useless for pathfinding actions though.
				Point.m_PathPos = Point.GetPosition();
			}
			else
			{
				//Use found path position
				Point.m_PathPos = PathPos;
			}
		}
		else
		{
			//Suitable path position directly above/below position. Use z-coordinate of found path position
			Point.m_PathPos = Point.GetPosition();
			Point.m_PathPos[2] = PathPos[2];
		}
		*/

		// Add the DYNAMICS to type if m_Flags contains FLAGS_DYNAMICS
		// The reason for this weird looking code is to group the dynamic checkbox with the other ones
		// (waitspawn, crouch etc) to simplify life for the scripters. Type checkboxes are hidden in the
		// Template.xrg versions of scenepoints
		if (Point.m_Flags & CWO_ScenePoint::FLAGS_DYNAMICS)
		{
			Point.m_Type |= CWO_ScenePoint::DYNAMIC;
			Point.m_Flags &= ~CWO_ScenePoint::FLAGS_DYNAMICS;
		}


		//Set secondary scene point data now, when all CWO_ScenePoints have been created
		int nSSP = 0;
		CWO_ScenePoint * pSSP = NULL;
		for (int i = 0; i < InitBlock.m_lSecondarySPNames.Len(); i++)
		{
			pSSP = Find(InitBlock.m_lSecondarySPNames[i]);
			if (pSSP)
				nSSP++;
		}
		if (nSSP > 0)
		{
			int j = 0;
			Point.m_lSecondarySPs.SetLen(nSSP);
			for (int i = 0; i < InitBlock.m_lSecondarySPNames.Len(); i++)
			{
				pSSP = Find(InitBlock.m_lSecondarySPNames[i]);
				if (pSSP)
				{
					int iSSP = GetScenePointIndex(pSSP);
					Point.m_lSecondarySPs[j].m_iSP = iSSP;
					Point.m_lSecondarySPs[j].m_Flags = InitBlock.m_lSecondarySPActions[i];
					j++;
				}
			}
		}

		//Set enginepath SP data now, when all CWO_ScenePoints have been created
		int nESP = 0;
		CWO_ScenePoint* pESP;
		for (int i = 0; i < InitBlock.m_lEnginepathSPNames.Len(); i++)
		{
			pESP = Find(InitBlock.m_lEnginepathSPNames[i]);
			if (pESP)
				nESP++;
		}
		if (nESP > 0)
		{
			int j = 0;
			Point.m_lEnginepathSPs.SetLen(nESP);
			for (int i = 0; i < InitBlock.m_lEnginepathSPNames.Len(); i++)
			{
				pESP = Find(InitBlock.m_lEnginepathSPNames[i]);
				if (pESP)
				{
					int iESP = GetScenePointIndex(pESP);
					int iEP = m_pWServer->Selection_GetSingleTarget(m_pWServer->World_MangleTargetName(InitBlock.m_lEnginepathEPNames[i]));
					if (iEP <= 0)
					{
						iEP = InitBlock.m_lEnginepathEPNames[i].Val_int();
					}
					Point.m_lEnginepathSPs[j].m_iSP = iESP;
					Point.m_lEnginepathSPs[j].m_iEP = iEP;
					j++;
				}
			}
		}

		// Set Parent index
		Point.m_iParent = m_pWServer->Selection_GetSingleTarget(InitBlock.m_ParentNameHash);
		if (Point.m_iParent > 0)
		{
			const CMat4Dfp32& ParentMat = m_pWServer->Object_GetPositionMatrix(Point.m_iParent);
			CMat4Dfp32 ParentInv;
			ParentMat.InverseOrthogonal(ParentInv);
			Point.m_Position.Multiply(ParentInv, Point.m_LocalPos);

			// listen to parent object movement
			m_pWServer->Object_AddListener(Point.m_iParent, m_iObject, CWO_LISTENER_EVENT_MOVED);
		}
		else
		{
			Point.m_LocalPos = Point.m_Position;
		}
		
	}

	// Destroy initblocks
	m_lScenePointInitBlocks.Clear();

	m_lScenePoints.OptimizeMemory();
	InitHash();
}



aint CWObject_ScenePointManager::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJSYSMSG_LISTENER_EVENT:
		{
			uint Event = _Msg.m_Param0;
			if (Event & CWO_LISTENER_EVENT_MOVED) 
			{
				CWObject* pObj = m_pWServer->Object_Get(_Msg.m_iSender);
				OnObjectMoved(*pObj);
			}
			return 1;
		}

	case OBJMSG_OBJECT_ISBREAKING:
		{
			TAP<const uint16> piNewObjects;
			piNewObjects.Set((const uint16*)_Msg.m_Param1, _Msg.m_Param0);
			CWObject* pObj = m_pWServer->Object_Get(_Msg.m_iSender);
			fp32 OldVolume = _Msg.m_VecParam0.k[0];
			OnObjectBreaking(*pObj, OldVolume, piNewObjects);
		}
		break;
	}
	return CWObject::OnMessage(_Msg);
}


aint CWObject_ScenePointManager::Message_SendToSelection(const CWObject_Message& _Msg)
{
	MSCOPESHORT(CWObject_ScenePointManager::Message_SendToSelection);
	aint ret = 0;
	TAP<CWO_ScenePoint*> pSelection = m_lSelection;
	for (uint i = 0; i < pSelection.Len(); i++)
		ret = Message_SendToScenePoint(_Msg, pSelection[i]);
	return ret;
}


//NOTE: only some specific messages can be sent to scenepoints, and m_pData is reserved for the scenepoint name when sent as script messages
aint CWObject_ScenePointManager::Message_SendToScenePoint(const CWObject_Message& _Msg, CWO_ScenePoint* _pSP)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_GAME_SCENEPOINT_SPAWN:
		_pSP->Spawn(this);
		return 1;

	case OBJMSG_GAME_SCENEPOINT_UNSPAWN:
		_pSP->UnSpawn(this);
		return 1;

	case OBJMSG_GAME_SCENEPOINT_ACTIVATE:
		_pSP->Activate(_Msg.m_iSender, m_pWServer, this);
		return 1;

	case OBJMSG_GAME_SCENEPOINT_RAISEPRIO:
		_pSP->RaisePrio();
		return 1;

	case OBJMSG_GAME_SCENEPOINT_RESTOREPRIO:
		_pSP->RestorePrio();
		return 1;

	default:
		ConOutL(CStrF("WARNING: Trying to send non-scenepoint message to scenepoint  (Msg: 0x%x)", _Msg.m_Msg));
	}
	return 0;
}


uint CWObject_ScenePointManager::GetDynamicScenepoints(uint _iParentObj, CWO_ScenePoint** _ppRet, uint _nMaxRet)
{
	MSCOPESHORT(CWObject_ScenePointManager::GetDynamicScenepoints);

	TAP_RCD<CWO_ScenePoint> pPoints = m_lScenePoints;
	uint16 aIDs[2000];
	uint nRet = 0;

	// loop through all dynamic scenepoints   (FIXME:  would be nice with a list of scenepoints for each 'parent' object)
	for (uint iType = 0; iType < CWO_ScenePointHash::NUM_TYPES; iType++)
	{
		uint iGrid = iType + (CWO_ScenePointHash::SET_DYNAMIC * CWO_ScenePointHash::NUM_TYPES);
		uint nIDs = m_Hash.EnumerateAll(iGrid, aIDs, sizeof(aIDs)/sizeof(aIDs[0]));

		for (uint i = 0; i < nIDs; i++)
		{
			uint iPoint = aIDs[i];
			CWO_ScenePoint* pSP = &pPoints[iPoint];
			if (pSP->m_iParent == _iParentObj)
			{
				_ppRet[nRet++] = pSP;
				if (nRet >= _nMaxRet)
					return nRet;
			}
		}
	}
	return nRet;
}


void CWObject_ScenePointManager::OnObjectMoved(const CWObject& _Obj)
{
	MSCOPESHORT(CWObject_ScenePointManager::OnObjectMoved);
	CWO_ScenePoint* lpSP[100];
	uint nSP = GetDynamicScenepoints(_Obj.m_iObject, lpSP, 100);

	for (uint i = 0; i < nSP; i++)
	{
		CWO_ScenePoint& p = *lpSP[i];

		// We may not assume moving objects are uninteresting, scenepoints with the FLAGS_ALLOWMOVING  flag are valid while moving
		if (p.m_Flags & CWO_ScenePoint::FLAGS_MOVING)
		{	// Already moving
			continue;
		}

/*#if defined(_DEBUG) || defined(M_MIXEDDEBUG)
		TAP<CWO_ScenePoint*> pActive = m_lActivePoints;
		for (uint j = 0; j < pActive.Len(); j++)
			M_ASSERTHANDLER(pActive[j] != &p, "Already in active list!", break);
#endif*/

		p.m_Flags |= CWO_ScenePoint::FLAGS_MOVING;
		m_lActivePoints.Add(&p);
	}
}


void CWObject_ScenePointManager::OnObjectBreaking(const CWObject& _Obj, fp32 _OldObjVolume, TAP<const uint16> _piNewObjects)
{
	MSCOPESHORT(CWObject_ScenePointManager::OnObjectBreaking);
	CWO_ScenePoint* lpSP[100];
	uint nSP = GetDynamicScenepoints(_Obj.m_iObject, lpSP, 100);
	if (!nSP)
		return;

	fp32 InvOldObjVolume = 1.0f / _OldObjVolume;

	for (uint i = 0; i < nSP; i++)
	{
		CWO_ScenePoint& p = *lpSP[i];

		CVec3Dfp32 SPPos = p.m_Position.GetRow(3);
		fp32 Best = _FP32_MAX;
		uint iBest = 0;
		for (uint j = 0; j < _piNewObjects.Len(); j++)
		{
			uint iNewObj = _piNewObjects[j];
			CWObject* pNewObj = m_pWServer->Object_Get(iNewObj);
			if (pNewObj)
			{
				CBox3Dfp32 BBox = *pNewObj->GetAbsBoundBox();
				CVec3Dfp32 Pos, Size;
				BBox.GetCenter(Pos);
				fp32 d = SPPos.DistanceSqr(Pos);
				if (d < Best)
				{
					// Make sure the sub-object is large enough (tweak: currently 50% of the original object's volume)
					fp32 NewObjMass, NewObjVolume; // Use for auto-setting physics 
					m_pWServer->Phys_GetPhysicalProperties(pNewObj->GetPhysState(), NewObjMass, NewObjVolume);
					if ((NewObjVolume * InvOldObjVolume) >= 0.50f)
					{
						Best = d;
						iBest = iNewObj;
					}
				}
			}
		}
		if (iBest > 0)
		{
			// Move scenepoint to new parent
			const CMat4Dfp32& ParentMat = m_pWServer->Object_GetPositionMatrix(iBest);
			CMat4Dfp32 ParentInv;
			ParentMat.InverseOrthogonal(ParentInv);
			p.m_Position.Multiply(ParentInv, p.m_LocalPos);
			p.m_iParent = iBest;
			m_pWServer->Object_AddListener(iBest, m_iObject, CWO_LISTENER_EVENT_MOVED);
		}
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_ScenePointHash
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CWO_ScenePointHash::Create(int _MaxIDs)
{
	m_lLinks.SetLen(_MaxIDs + 1);
	m_pLinks = m_lLinks.GetBasePtr();
	FillChar(m_pLinks, m_lLinks.ListSize(), 0);
}


void CWO_ScenePointHash::InitGrid(uint8 _iGrid, int _nBoxes, const CBox3Dfp32& _Bound)
{
	M_ASSERT(_iGrid < NUM_GRIDS, "Bad category!");
	M_ASSERT(IsPow2(_nBoxes), "Number of boxes must be power of two!");

	HashGrid& G = m_Grids[_iGrid];

	G.m_nBoxNum = _nBoxes;
	G.m_nBoxNumMask = G.m_nBoxNum - 1;
	G.m_nBoxNumShift = Log2(G.m_nBoxNum);
	G.m_nBuckets = Sqr(G.m_nBoxNum) + 1;	// one extra needed, since iHash=0 means invalid

	fp32 Range = Max( _Bound.m_Max.k[0] - _Bound.m_Min.k[0], _Bound.m_Max.k[1] - _Bound.m_Min.k[1] );
	G.m_nBoxSize = GetGEPow2(uint(Range / G.m_nBoxNum));
	G.m_nBoxSizeShift = Log2(G.m_nBoxSize);
	G.m_Range = G.m_nBoxSize * (G.m_nBoxNum - 2);

	G.m_lHash.SetLen(G.m_nBuckets);
	G.m_pHash = G.m_lHash.GetBasePtr();
	FillChar(G.m_pHash, G.m_lHash.ListSize(), 0);
}


void CWO_ScenePointHash::Insert(uint8 _iGrid, uint16 _ID, uint16 _iPoint, const CVec3Dfp32& _Pos)
{
	M_ASSERT(m_pLinks, "!");
	M_ASSERT(_iGrid < NUM_GRIDS, "Bad category!");

	HashGrid& G = m_Grids[_iGrid];
	M_ASSERT(G.m_pHash, "!");

	int x = (TruncToInt(_Pos.k[0]) >> G.m_nBoxSizeShift) & G.m_nBoxNumMask;
	int y = (TruncToInt(_Pos.k[1]) >> G.m_nBoxSizeShift) & G.m_nBoxNumMask;
	uint16 iHash = G.GetHashIndex(x, y);

	M_ASSERT(_ID && _ID < m_lLinks.Len(), "!");

	HashLink& L = m_pLinks[_ID];
	M_ASSERT(L.m_iHash == 0, "!");
	L.m_iHash = iHash;
	L.m_iGrid = _iGrid;
	L.m_iPoint = _iPoint;

	uint16 iCurrent = G.m_pHash[iHash];
	if (iCurrent)
	{
		M_ASSERT(m_pLinks[iCurrent].m_iGrid == _iGrid, "!");
		m_pLinks[iCurrent].m_iPrev = _ID;
	}
	L.m_iNext = iCurrent;
	L.m_iPrev = 0;
	G.m_pHash[iHash] = _ID;
}


void CWO_ScenePointHash::Remove(uint16 _ID)
{
	M_ASSERT(m_pLinks, "!");
	M_ASSERT(_ID && _ID < m_lLinks.Len(), "!");
	HashLink& L = m_pLinks[_ID];

	uint8 iGrid = L.m_iGrid;
	M_ASSERT(iGrid < NUM_GRIDS, "!");
	HashGrid& G = m_Grids[iGrid];
	M_ASSERT(G.m_pHash, "!");

	uint16 iHash = L.m_iHash;
	if (iHash)
	{
		if (L.m_iPrev)
			m_pLinks[L.m_iPrev].m_iNext = L.m_iNext;
		else
			G.m_pHash[iHash] = L.m_iNext;

		if (L.m_iNext)
			m_pLinks[L.m_iNext].m_iPrev = L.m_iPrev;

		L.m_iHash = 0;
	}
}


void CWO_ScenePointHash::Insert(const CWO_ScenePoint& _Obj, uint16 _iPoint)
{
	uint32 x = GetContainerIndices(_Obj.GetType());
	uint8 iSet = (x >> 16);
	uint16 Types = (x & 0xffff);

	uint16 ID = _Obj.m_ID;
	for (uint8 iType = 0; iType < NUM_TYPES; iType++)
	{
		if (!(Types & M_BitD(iType)))
			continue;

		uint8 iGrid = iType + iSet * NUM_TYPES;
		Insert(iGrid, ID++, _iPoint, _Obj.GetPos());
	}
}


void CWO_ScenePointHash::Remove(const CWO_ScenePoint& _Obj)
{
	uint32 x = GetContainerIndices(_Obj.GetType());
//	uint8 iSet = (x >> 16);
	uint16 Types = (x & 0xffff);

	uint16 ID = _Obj.m_ID;
	for (uint8 iType = 0; iType < NUM_TYPES; iType++)
	{
		if (!(Types & M_BitD(iType)))
			continue;

		Remove(ID++);
	}
}


void CWO_ScenePointHash::Update(const CWO_ScenePoint& _Obj, uint16 _iPoint)
{
	uint32 x = GetContainerIndices(_Obj.GetType());
//	uint8 iSet = (x >> 16);
	uint16 Types = (x & 0xffff);

	uint16 ID = _Obj.m_ID;
	for (uint8 iType = 0; iType < NUM_TYPES; iType++)
	{
		if (!(Types & M_BitD(iType)))
			continue;

		HashLink& L = m_pLinks[ID];
		uint8 iGrid = L.m_iGrid;
		M_ASSERT(iGrid < NUM_GRIDS, "!");
		HashGrid& G = m_Grids[iGrid];
		M_ASSERT(G.m_pHash, "!");

		const CVec3Dfp32& Pos = _Obj.GetPos();
		int x = (TruncToInt(Pos.k[0]) >> G.m_nBoxSizeShift) & G.m_nBoxNumMask;
		int y = (TruncToInt(Pos.k[1]) >> G.m_nBoxSizeShift) & G.m_nBoxNumMask;
		uint16 iHash = G.GetHashIndex(x, y);

		if (iHash != L.m_iHash)
		{
			Remove(ID);
			Insert(iGrid, ID, _iPoint, Pos);
		}
		ID++;
	}
}



int CWO_ScenePointHash::EnumerateBox(uint8 _iGrid, const CBox3Dfp32& _Box, uint16* _pEnumRetIDs, uint _MaxEnumPoints) const
{
	M_ASSERT(_iGrid < NUM_GRIDS, "Bad category!");
	M_ASSERT(m_pLinks, "!");

	const HashGrid& G = m_Grids[_iGrid];
	M_ASSERT(G.m_pHash, "!");

	uint16 BoxNumMask = G.m_nBoxNumMask;

	// Calculate extents of the rect in the hash grid
	int bmin[2], bmax[2];
	for (uint i=0; i<2; i++)
	{
		if (_Box.m_Max.k[i] - _Box.m_Min.k[i] > G.m_Range)
		{
			bmin[i] = 0;
			bmax[i] = G.m_nBoxNumMask;
		}
		else
		{
			bmin[i] = (RoundToInt(_Box.m_Min.k[i]) >> G.m_nBoxSizeShift) & BoxNumMask;
			bmax[i] = (RoundToInt(_Box.m_Max.k[i]) >> G.m_nBoxSizeShift) & BoxNumMask;
		}
	}

	// Scan the blocks
	int nIDs = 0;
	for (uint y = bmin[1]; ; )
	{
		for (uint x = bmin[0]; ; )
		{
			uint iHash = G.GetHashIndex(x, y);
			for (int iLink = G.m_pHash[iHash]; iLink; )
			{
				if (nIDs >= _MaxEnumPoints)
					return nIDs;

				const HashLink& L = m_pLinks[iLink];
				M_ASSERT(L.m_iGrid == _iGrid, "!");

				_pEnumRetIDs[nIDs++] = L.m_iPoint;
				iLink = L.m_iNext;
			}
			if (x == bmax[0]) break;
			x = (x + 1) & BoxNumMask;
		}
		if (y == bmax[1]) break;
		y = (y + 1) & BoxNumMask;
	}
	return nIDs;
}


int CWO_ScenePointHash::EnumerateAll(uint8 _iGrid, uint16* _pEnumRetIDs, uint _MaxEnumIDs) const
{
	M_ASSERT(_iGrid < NUM_GRIDS, "Bad category!");
	const HashLink* pLinks = m_pLinks;
	M_ASSERT(pLinks, "!");

	const HashGrid& G = m_Grids[_iGrid];
	const uint16* pHash = G.m_pHash;
	uint16 nBuckets = G.m_nBuckets;
	M_ASSERT(pHash, "!");

	// Scan all buckets
	int nIDs = 0;
	for (uint16 iHash = 1; iHash < nBuckets; iHash++)
	{
		for (int iLink = pHash[iHash]; iLink; )
		{
			if (nIDs >= _MaxEnumIDs)
				return nIDs;

			const HashLink& L = pLinks[iLink];
			M_ASSERT(L.m_iGrid == _iGrid, "!");

			_pEnumRetIDs[nIDs++] = L.m_iPoint;
			iLink = L.m_iNext;
		}
	}
	return nIDs;
}


struct HistElement { uint16 m_Value; HistElement() : m_Value(0) {} };
void CWO_ScenePointHash::DumpHashUsage() const
{
/*
	struct Histogram
	{
		TThinArray<HistElement> m_Buckets;

		void Add(uint _Index) 
		{ 
			if (_Index >= m_Buckets.Len())
				m_Buckets.SetLen(_Index+1); 
			m_Buckets[_Index].m_Value++; 
		}

		void Report() const
		{
			for (uint i=0; i < m_Buckets.Len(); i++)
				if (m_Buckets[i].m_Value)
					DEBUG_LOG(CStrF("%d: %d", i, m_Buckets[i].m_Value));
		}
	};

	for (uint i=0; i < NUM_SLOTS; i++)
	{
		const HashGrid& G = m_Grids[i];
		DEBUG_LOG(CStr("-----"));
		Histogram h;
		uint nUnused = 0;
		for (uint iHash=0; iHash < G.m_nBuckets; iHash++)
		{
			uint nLinks = 0;
			for (uint16 iLink = G.m_pHash[iHash]; iLink; iLink = m_pLinks[iLink].m_iNext)
				nLinks++;
			h.Add(nLinks);
		}
		h.Report();
	}
*/
}

