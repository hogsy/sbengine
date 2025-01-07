#include "PCH.h"
#include "../../../WObj_Char.h"
#include "../../AICore.h"
#include "../../AI_Custom/AICore_Bosses.h"
#include "../../../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
#include "AI_Behaviour_Custom.h"
#include "../../../WRPG/WRPGAmmo.h"


#define START_TEL_TIME 30
#define TRAIL_TEL_TIME 20
#define END_TEL_TIME 30

//Initializes behaviour
CAI_Behaviour_Vatar::CAI_Behaviour_Vatar(CAI_Core * _pAI, int _iArea)
:CAI_Behaviour_Engage(_pAI)
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_ctor, MAUTOSTRIP_VOID);

	m_iType = VATAR;
	m_iMode = NONE;
	m_iCurPoint = -1;
	m_StartPos = m_pAI->m_pGameObject->GetPosition();
	m_WaitTimer = 0;
	m_TeleportTimer = 0;
	m_pTelEffStart = m_pAI->GetSingleTarget(0, CStr("vat_teleffstart"),0);
	if(m_pTelEffStart)
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_MODEL_SETFLAGS, 0),m_pTelEffStart->m_iObject);

	m_pTelEffEnd = m_pAI->GetSingleTarget(0, CStr("vat_teleffend"),0);
	if(m_pTelEffEnd)
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_MODEL_SETFLAGS, 0),m_pTelEffEnd->m_iObject);

	m_pTelTrail = m_pAI->GetSingleTarget(0, CStr("vatar_teltrail"),0);
	if(m_pTelTrail)
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_MODEL_SETFLAGS, 0),m_pTelTrail->m_iObject);
	m_FramesTargetOutRange = 0;
	m_bTargetOnPlatform = false;
	m_bOnPlattform = false;
	m_PlattformTimer = 0;
	m_MinTelTime = 100;
	m_RandTelTime = 100;
	m_pTarget = NULL;
	m_SettleTimer = 0;
	m_bSwitchWeapon = true;
	m_bCharging = false;
	m_bForceTeleport = false;
	m_pAI->m_PathFinder.Init(m_pAI, m_pAI->m_pServer->Path_GetNavGraph(),  m_pAI->m_pServer->Path_GetBlockNav(), &m_pAI->ms_PFResource_Graph, &m_pAI->ms_PFResource_Grid, 96, 8, 8, 10, 0, 16, 500);
	m_iModel = m_pAI->m_pGameObject->m_iModel[0];
	m_StartTelTime = START_TEL_TIME;
	m_TransitTelTime = TRAIL_TEL_TIME;
	m_EndTelTime = END_TEL_TIME;
	m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_SETFLAGS,PLAYER_FLAGS_VATAR));

};

void CAI_Behaviour_Vatar::TeleportEffect(bool _On)
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_TeleportEffect, MAUTOSTRIP_VOID);
/*	if(m_pTelEff1 && m_pTelEff2)
	{
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_MODEL_SETFLAGS, _On),m_pTelEff1->m_iObject);
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_MODEL_SETFLAGS, _On),m_pTelEff2->m_iObject);
	}*/
}

bool CAI_Behaviour_Vatar::IsPointAvailable(int _Index)
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_IsPointAvailable, false);
	CWObject *pTeleportTo =  m_pAI->m_pServer->Object_Get(_Index);
	CMat43fp32 Pos;
	if (pTeleportTo)
	{
		Pos = pTeleportTo->GetPositionMatrix();
								
		CWO_PhysicsState Phys;
		Phys.m_MediumFlags |= XW_MEDIUM_PLAYERSOLID;
		Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(32.0f, 32.0f, 100.0f), CVec3Dfp32(0,0,32.0f), 1.0f);
		Phys.m_nPrim = 1;
		Phys.m_ObjectFlags = OBJECT_FLAGS_CHARACTER;
		Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_CHARACTER;
																
		Pos.k[3][2] += 1.0f;
								
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pAI->m_pServer->Selection_AddIntersection(Selection, Pos, Phys);
		int nSelObj = m_pAI->m_pServer->Selection_Get(Selection);
		m_pAI->m_pServer->Selection_Pop();
								
		if (!nSelObj)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

void CAI_Behaviour_Vatar::LookAround()
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_LookAround, MAUTOSTRIP_VOID);
	if (m_pAI->m_iActionFlags & CAI_Core::LOOKING)
		m_pAI->OnLook();
	else if (Random < 0.4f)
	{
		//Sweep gaze to another angle within 45 degrees of the default heading and reset pitch
		m_pAI->OnLook(m_pAI->m_DefaultHeading + (Random/4) - 0.125f, 0, 25 + Random * 50);
	};
}	



void CAI_Behaviour_Vatar::ResetWaitTimer()
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_ResetWaitTimer, MAUTOSTRIP_VOID);
	m_WaitTimer = m_MinTelTime + (int)(Random * m_RandTelTime);
}

#define MAX_RAND_POINTS 3
#define POINTS_TO_USE (m_bTargetOnPlatform? m_iPlattformPoints: m_iTeleportationPoints)

int CAI_Behaviour_Vatar::ChoosePoint(CWObject * _pTarget)
{	
	MAUTOSTRIP(CAI_Behaviour_Vatar_ChoosePoint, 0);
	int NumPoints = POINTS_TO_USE.Len();
	if(NumPoints)
	{
		if(_pTarget)
		{
			CVec3Dfp32 TarPos = _pTarget->GetPosition();
			TArray<fp32> lDists;
			lDists.QuickSetLen(NumPoints);
			int i;
			for (i = 0; i < NumPoints; i++) 
			{
				if(IsPointAvailable(POINTS_TO_USE[i]))				
				{
					CWObject *pTeleportTo =  m_pAI->m_pServer->Object_Get(POINTS_TO_USE[i]);
					lDists[i] = pTeleportTo->GetPosition().DistanceSqr(TarPos);
				}
				else
					lDists[i] = _FP32_MAX;
			}
			// Get the three points with the least distance. And pick one of them randomly.
			
			TArray<int> liPoints;
			liPoints.QuickSetLen(MAX_RAND_POINTS);
			for (i = 0; i < MAX_RAND_POINTS; i++) liPoints[i] = -1;
			
			//Find a good point in front of the target
			CVec3Dfp32 MoveDir = CVec3Dfp32::GetMatrixRow(_pTarget->GetPositionMatrix(),0);
			CVec3Dfp32 Dir;
			fp32 BestDist = _FP32_MAX;
			int BestPoint = -1;
			CVec3Dfp32 TarHeadPos = 0;
			TarHeadPos[2] = _pTarget->GetAbsBoundBox()->m_Min[2] + 0.6f * (_pTarget->GetAbsBoundBox()->m_Max[2] - _pTarget->GetAbsBoundBox()->m_Min[2]) - _pTarget->GetPosition()[2]; 
			CVec3Dfp32 TelPos;
 			for (i = 0; i < NumPoints; i++)				
			{
				if(lDists[i]<BestDist)
				{
					CWObject *pTeleportTo =  m_pAI->m_pServer->Object_Get(POINTS_TO_USE[i]);
					TelPos = pTeleportTo->GetPosition();
					Dir = TelPos - TarPos;
					Dir.Normalize();
//					TelPos[2] += 10;
					fp32 Cp = Dir * MoveDir;
					if(Cp > 0 && m_pAI->CheckLOS(TelPos, _pTarget, &TarHeadPos, 1) != CVec3Dfp32(_FP32_MAX))
					{
						BestPoint = POINTS_TO_USE[i];
						BestDist = lDists[i];
					}
				}
			}
			
			if(BestPoint >= 0)
			{
				bool IsInPlattformPoints = false;
				if(m_bTargetOnPlatform)
					IsInPlattformPoints = true;
				else
				{
					for(int j = 0; i < m_iPlattformPoints.Len() ; i++)
					{
						if(BestPoint == m_iPlattformPoints[j])
						{
							IsInPlattformPoints = true;
							break;
						}
					}
				}
				if(IsInPlattformPoints)
					m_bOnPlattform = true;
				else
					m_bOnPlattform = false;
				return BestPoint;
			}
			
			for (i = 0; i < NumPoints; i++)				
			{
				for (int j = 0; j < MAX_RAND_POINTS; j++)
				{
					if(liPoints[j] == -1 || lDists[i]<lDists[liPoints[j]])
					{
						for (int k = MAX_RAND_POINTS-1; k > j; k--)
						{
							liPoints[k] = liPoints[k-1];
						}
						liPoints[j] = i;
					}
				}
			}
			int Rnd = MRTC_RAND() % MAX_RAND_POINTS;
			bool IsInPlattformPoints = false;
			if(m_bTargetOnPlatform)
				IsInPlattformPoints = true;
			else
			{
				for(int j = 0; i < m_iPlattformPoints.Len() ; i++)
				{
					if(m_iTeleportationPoints[liPoints[Rnd]] == m_iPlattformPoints[j])
					{
						IsInPlattformPoints = true;
						break;
					}
				}
			}
			if(IsInPlattformPoints)
				m_bOnPlattform = true;
			else
				m_bOnPlattform = false;
			
			return POINTS_TO_USE[liPoints[Rnd]];
		}
		else
		{			
			int nIters = NumPoints;
			//Set up check array
			TArray<bool> lbChecked;
			lbChecked.QuickSetLen(NumPoints);
			int i;
			for (i = 0; i < NumPoints; i++) lbChecked[i] = false;
			
			int Rnd;
			while(nIters > 0)
			{
				//Find random non-checked object index
				Rnd = (int)(Random * NumPoints * 0.9999f);
				// if this index has been checked loop over all indexes and find an unchecked index.
				if(lbChecked[Rnd])
				{
					int j = 0;
					while (j<NumPoints && lbChecked[j])
						j++;
					Rnd = j;
					if(lbChecked[Rnd])
						return 0;
				}
				lbChecked[Rnd] = true;
				if(IsPointAvailable(POINTS_TO_USE[Rnd]))				
				{
					bool IsInPlattformPoints = false;
					if(m_bTargetOnPlatform)
						IsInPlattformPoints = true;
					else
					{
						for(int j = 0; i < m_iPlattformPoints.Len() ; i++)
						{
							if(m_iTeleportationPoints[Rnd] == m_iPlattformPoints[j])
							{
								IsInPlattformPoints = true;
								break;
							}
						}
					}
					if(IsInPlattformPoints)
						m_bOnPlattform = true;
					else
						m_bOnPlattform = false;

					return POINTS_TO_USE[Rnd];
				}
				nIters--;
			}
		}
	}
	return -1;
}

bool CAI_Behaviour_Vatar::ShouldTeleport(CWObject * _pTarget)
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_ShouldTeleport, false);
	if(m_bTargetOnPlatform && !m_bOnPlattform)
		return true;
	if(m_FramesTargetOutRange > 20)
		return true;
	return false;
}


void CAI_Behaviour_Vatar::SwitchWeapon(CWObject * _pTarget)
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_SwitchWeapon, MAUTOSTRIP_VOID);
	if(m_pTarget && m_bSwitchWeapon && m_pAI->Wait() <= 0)
	{		
		fp32 DistSqr =  m_pAI->m_pGameObject->GetPosition().DistanceSqr(_pTarget->GetPosition());
		
		if(DistSqr < Sqr(190))// && !m_pAI->m_Weapon.GetWielded()->IsMeleeWeapon())
		{
			if(Random < 0.3f)
				m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH, 2);
			else
				m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH, 4);
			m_bSwitchWeapon = false;

		}
		else
		{
			m_bSwitchWeapon = false;
			int iWeap = (MRTC_RAND() % 3) +1;
			m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH, iWeap);
		}
	}
}

bool CAI_Behaviour_Vatar::ShouldAttack(CWObject * _pTarget)
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_ShouldAttack, false);
	if(m_bCharging)
	{
		if((m_pAI->m_iTimer % 13 == 0) && Random < 0.2f)
			m_bCharging = false;
		else
			return false;
	}
	if(m_pAI->m_Weapon.GetWielded()->IsMeleeWeapon())
	{
		fp32 DistSqr =  m_pAI->m_pGameObject->GetPosition().DistanceSqr(_pTarget->GetPosition());
		if(DistSqr < Sqr(190))
			return true;
		else
			return false;

	}
	else
		return  m_pAI->m_spPersonality->ShouldAttack(m_pAI->m_Weapon.GetWielded(), _pTarget);
}

void CAI_Behaviour_Vatar::Settle(int _Time)
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_Settle, MAUTOSTRIP_VOID);
	m_SettleTimer = _Time;
	m_iMode = SETTLING;
}


void CAI_Behaviour_Vatar::OnEngage(CWObject * _pTarget)
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_OnEngage, MAUTOSTRIP_VOID);
	if(m_iTeleportationPoints.Len() > 0 && m_iMode == NONE && ShouldTeleport(_pTarget))
	{
		StartTeleporting();
		return;
	}


	fp32 DistSqr =  m_pAI->m_pGameObject->GetPosition().DistanceSqr(_pTarget->GetPosition());
	CWObject *pCurPoint =  m_pAI->m_pServer->Object_Get(m_iCurPoint);
	CVec3Dfp32 CPos = m_StartPos;
	if(pCurPoint)
		CPos = pCurPoint->GetPosition();

	fp32 DistSqr2 =  m_pAI->m_pGameObject->GetPosition().DistanceSqr(CPos);
	fp32 TargetDistToPosSqr =  _pTarget->GetPosition().DistanceSqr(CPos);
	/*if(DistSqr > Sqr(96) && (DistSqr < Sqr(300) || TargetDistToPosSqr < Sqr(300)) && 
		(DistSqr2<Sqr(480) || TargetDistToPosSqr < DistSqr2) &&
		m_iMode == NONE )
	{
		//Go towards target
		m_pAI->OnFollowTarget(_pTarget);
		m_bCharging = false;
	}
	else */
	if(m_iMode == NONE && DistSqr < Sqr(960) && DistSqr > Sqr(120))
	{
		if(m_bCharging)
		{
			if(DistSqr < Sqr(120))
			{
				m_bCharging = false;
				m_pAI->OnIdle();
			}
			else if(m_pAI->m_iTimer % 10 == 0 && !m_pAI->m_PathFinder.GroundTraversable(m_pAI->GetPathPosition(m_pAI->m_pGameObject), m_pAI->GetPathPosition(_pTarget)))
			{
				m_bCharging = false;
				m_pAI->OnIdle();
			}
			else
				m_pAI->AddMoveTowardsPositionCmd(_pTarget->GetPosition());
		}
		else if(DistSqr < Sqr(190) || (Random < 0.20f && m_pAI->m_PathFinder.GroundTraversable(m_pAI->GetPathPosition(m_pAI->m_pGameObject), m_pAI->GetPathPosition(_pTarget))))
		{
			m_bCharging = true;
			m_pAI->AddMoveTowardsPositionCmd(_pTarget->GetPosition());
		}
		else
			m_pAI->OnIdle();
	}
	else
	{
		m_bCharging = false;
		m_pAI->OnIdle();
	}

	if(ShouldAttack(_pTarget))
	{
		//Attack!
		m_bSwitchWeapon = true;
		
		//Make sure we try to grab in the right direction (fix proper)
		m_pAI->ForceAlignBody(0.3f);
		//m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_STRAIGHTENUP,1));
		if(m_pAI->m_Weapon.GetWielded()->GetItem()->m_iItemType == 2)
		{
			Settle(122);
		}
		else if(m_pAI->m_Weapon.GetWielded()->IsMeleeWeapon())
		{
			Settle(52);
		}
		else
		{
			Settle(42);
		}
		m_pAI->OnAttack(_pTarget);
	}
	else
	{
		if((m_pAI->m_iTimer % 20 == 0))
		{
			m_bSwitchWeapon = true;
			SwitchWeapon(_pTarget);
		}
	}


}


void CAI_Behaviour_Vatar::StartTeleporting()
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_StartTeleporting, MAUTOSTRIP_VOID);
	m_iMode = START_TELEPORT;
	m_TeleportTimer = 	m_StartTelTime;
	if(m_pTelEffStart)
	{
		CMat43fp32 Pos = m_pAI->m_pGameObject->GetPositionMatrix();
		m_pAI->m_pServer->Object_SetPosition(m_pTelEffStart->m_iObject, CVec3Dfp32::GetMatrixRow(Pos, 3));
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_MODEL_SETFLAGS, 1),m_pTelEffStart->m_iObject);
		m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, 133));
		m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_SPEAK, 70));
	}
	ResetWaitTimer();
}


void CAI_Behaviour_Vatar::TeleportTo(int _iObj, int _Ticks)
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_TeleportTo, MAUTOSTRIP_VOID);
	CWObject *pTeleportTo =  m_pAI->m_pServer->Object_Get(_iObj);
	CMat43fp32 Pos;
	if (pTeleportTo && m_pTelEffEnd && m_pTelTrail)
	{
		m_pAI->m_pGameObject->m_iModel[0] = 0;
		Pos = pTeleportTo->GetPositionMatrix();
		m_pAI->m_pServer->Object_SetPosition(m_pTelEffEnd->m_iObject, CVec3Dfp32::GetMatrixRow(Pos, 3));
		if(_Ticks <=8)
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_MODEL_SETFLAGS, 1),m_pTelEffEnd->m_iObject);
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_MODEL_SETFLAGS, 1),m_pTelTrail->m_iObject);
		CVec3Dfp32 Dir = CVec3Dfp32::GetMatrixRow(Pos, 3) - m_pTelTrail->GetPosition();

		Dir[0] /= (fp32)_Ticks;
		Dir[1] /= (fp32)_Ticks;
		Dir[2] /= (fp32)_Ticks;
	
		m_pAI->m_pServer->Object_SetPosition(m_pTelTrail->m_iObject, m_pTelTrail->GetPosition() + Dir);
		
		ResetWaitTimer();
	}							
}

void CAI_Behaviour_Vatar::EndTeleport()
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_EndTeleport, MAUTOSTRIP_VOID);
	if (m_pTelEffEnd)
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_MODEL_SETFLAGS, 0),m_pTelEffEnd->m_iObject);
	if (m_pTelTrail)
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_MODEL_SETFLAGS, 0),m_pTelTrail->m_iObject);
	if (m_pTelEffStart)
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_MODEL_SETFLAGS, 0),m_pTelEffStart->m_iObject);
	Settle(20);
	m_TeleportTimer = 0;
	m_bForceTeleport = false;
}

void CAI_Behaviour_Vatar::TeleportVatar()
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_TeleportVatar, MAUTOSTRIP_VOID);
	CWObject *pTeleportTo =  m_pAI->m_pServer->Object_Get(m_iCurPoint);
	CMat43fp32 Pos;
	if (pTeleportTo && IsPointAvailable(m_iCurPoint))
	{
		Pos = pTeleportTo->GetPositionMatrix();
		m_pAI->m_pServer->Object_SetPosition(m_pAI->m_pGameObject->m_iObject, CVec3Dfp32::GetMatrixRow(Pos, 3));
		pTeleportTo->OnMessage(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_pAI->m_pGameObject->m_iObject));
		if (m_pTelEffStart)
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_MODEL_SETFLAGS, 1),m_pTelEffStart->m_iObject);
		
		m_pAI->m_pGameObject->m_iModel[0] = m_iModel;
		m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_SPEAK, 71));

		ResetWaitTimer();
	}
	else 
	{
		m_pAI->m_pGameObject->m_iModel[0] = m_iModel;
		EndTeleport();
	}
}



void CAI_Behaviour_Vatar::OnGoHunting()
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_OnGoHunting, MAUTOSTRIP_VOID);
	m_WaitTimer--;
	m_LookTimer--;
	if(!(m_bTargetOnPlatform && m_bOnPlattform) && m_iTeleportationPoints.Len() > 0 && m_WaitTimer <= 0 && m_iMode != TELEPORTING)
	{
		StartTeleporting();
	}
	else
	{
		LookAround();
		m_pAI->OnIdle();
	}
}


//Act according to current mode or just kill anything around
void CAI_Behaviour_Vatar::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_OnTakeAction, MAUTOSTRIP_VOID);
	if (!m_pAI || !m_pAI->IsValid())
		return;
	if(m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_GETIMMUNE)))
		m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_SETFLAGS, PLAYER_FLAGS_NOTARGETING, 0));
	else
		m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_SETFLAGS, 0, PLAYER_FLAGS_NOTARGETING));
	m_PlattformTimer--;

	if(m_PlattformTimer <= 0)
		m_bTargetOnPlatform = false;
	if(!m_pTarget || m_pAI->IsValidCharacter(m_pTarget))
		m_pTarget = (CWObject_Character *)m_pAI->GetSingleTarget(0, "$player", 0);


	fp32 DistSqr = 0;
	if(m_pTarget)
		DistSqr = m_pAI->m_pGameObject->GetPosition().DistanceSqr(m_pTarget->GetPosition());
	if(DistSqr > Sqr(800))
	{
		m_FramesTargetOutRange++;
	}
	else
		m_FramesTargetOutRange = 0;



	SwitchWeapon(m_pTarget);

	bool bSeeTarget = false;
	if(m_pTarget)
	{
		bSeeTarget = m_pAI->IsInLOS(m_pAI->GetCharacterListIndex(m_pTarget->m_iObject));
		//		m_pAI->OnAim(m_pTarget);
		
		fp32 Pitch;
		CVec3Dfp32 RelPos = m_pTarget->GetPosition() - m_pAI->GetAimPosition(m_pAI->m_pGameObject);
		
		Pitch = M_ASin(RelPos[2]/RelPos.Length());
		//Convert to fractions
		Pitch /= 2*_PI;
		m_pAI->OnLook(m_pAI->HeadingToPosition(m_pAI->GetAimPosition(m_pAI->m_pGameObject), m_pTarget->GetPosition()),
			Pitch,
			2);
		//			m_pAI->AddAimAtPositionCmd(m_pTarget->GetPosition());//m_pAI->GetFocusPosition(m_pTarget));
	}
	if(m_iMode == SETTLING)
	{
		m_SettleTimer--;
		if(m_SettleTimer <= 0)
			m_iMode = NONE;
		m_pAI->OnIdle();
		m_pAI->m_DeviceMove.Free();
		m_pAI->m_DeviceMove.Use();
	}
	else
	{
		
		CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
		if(m_iMode == START_TELEPORT)
		{
			if(m_TeleportTimer <= 0)
			{
				int iPoint;
				if(m_bForceTeleport)
					iPoint = m_iCurPoint;
				else
					iPoint = ChoosePoint(m_pTarget);
				if(iPoint >= 0)
				{
					m_iCurPoint = iPoint;
					m_TeleportTimer = m_TransitTelTime;
					if(m_pTelTrail)
					{
						m_pAI->m_pServer->Object_SetPosition(m_pTelTrail->m_iObject, m_pAI->m_pGameObject->GetPosition());
					}
				}
				m_iMode = TELEPORTING;
			}
			else
			{
				m_TeleportTimer--;
				//				if(pCD)
				//					pCD->m_FadeTargetInv = ((fp32)(m_TeleportTimer/((fp32) START_TEL_TIME))) * 255;
			}
		}
		else if(m_iMode == TELEPORTING)
		{
			if(m_TeleportTimer <= 0)
			{
				m_iMode = TELEPORTED;				
				m_TeleportTimer = m_EndTelTime;
				TeleportVatar();
			}
			else
			{
				TeleportTo(m_iCurPoint, m_TeleportTimer);
				m_TeleportTimer--;
			}		
		}
		else if(m_iMode == TELEPORTED)
		{
			if(m_TeleportTimer <= 0)
			{
				m_iMode = NONE;
				EndTeleport();
			}
			else
			{
				m_TeleportTimer--;
				//				if(pCD)
				//					pCD->m_FadeTargetInv = ((fp32)((END_TEL_TIME - m_TeleportTimer)/((fp32) END_TEL_TIME))) * 255;
			}		
		}
		else if(m_iMode != SETTLING)
		{
			if(!bSeeTarget)
				OnGoHunting();
			else
			{
				if(DistSqr < Sqr(500))
					ResetWaitTimer();
				OnEngage(m_pTarget);
			}
		}
	}
};

//Handle special messages which influence the behaviour
bool CAI_Behaviour_Vatar::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_OnMessage, false);
	switch (_Msg.m_Msg)
	{
	case OBJMSG_SPECIAL_IMPULSE:
		{
			//Check if we should switch to the given behaviour mode (note that I don't support changing variables)
			CStr str = (char *)_Msg.m_pData;
			CStr s;
			CWObject * pObj;
			if (str.CompareSubStr("CLEARPOINTS") == 0)
			{
				m_iTeleportationPoints.Clear();
				m_iPlattformPoints.Clear();
			}
			else if (str.CompareSubStr("ADDPOINTS") == 0)
			{
				//Trim message
				s = str.GetStrSep(";"); 
				//Objects are attack paths to use.
				bool FoundPoint = true;
				while(FoundPoint)
				{
					s = str.GetStrSep(";"); 
					pObj = m_pAI->GetSingleTarget(0, s, 0);
					if(pObj)
						m_iTeleportationPoints.Add(pObj->m_iObject);
					else
						FoundPoint = false;
				}
				ResetWaitTimer();
			}
			else if (str.CompareSubStr("PLATTFORMPOINTS") == 0)
			{
				//Trim message
				s = str.GetStrSep(";"); 
				//Objects are attack paths to use.
				bool FoundPoint = true;
				while(FoundPoint)
				{
					s = str.GetStrSep(";"); 
					pObj = m_pAI->GetSingleTarget(0, s, 0);
					if(pObj)
						m_iPlattformPoints.Add(pObj->m_iObject);
					else
						FoundPoint = false;
				}
				ResetWaitTimer();
			}
			else if (str.CompareSubStr("ONPLATTFORM") == 0)
			{
				m_bTargetOnPlatform = true;
				m_PlattformTimer = 100;
			}
			else if (str.CompareSubStr("TELEPORTTO") == 0)
			{
				s = str.GetStrSep(";"); 
				s = str.GetStrSep(";"); 
				pObj = m_pAI->GetSingleTarget(0, s, 0);
				if(pObj)
				{
					m_iCurPoint = pObj->m_iObject;
					m_bForceTeleport = true;
					StartTeleporting();
				}
			}
			else if (str.CompareSubStr("TELEPORTTIME") == 0)
			{
				s = str.GetStrSep(";"); 
				m_MinTelTime = (str.GetIntSep(";"));
				m_RandTelTime = (str.GetIntSep(";") - m_MinTelTime) * SERVER_TICKSPERSECOND;
				m_MinTelTime *= SERVER_TICKSPERSECOND;
				fp64 Temp = str.Getfp64Sep(";");
				if( Temp > 0.0f)
					m_StartTelTime = (int)(Temp * SERVER_TICKSPERSECOND);
				Temp = str.Getfp64Sep(";");
				if( Temp > 0.0f)
					m_TransitTelTime = (int)(Temp * SERVER_TICKSPERSECOND);
				Temp = str.Getfp64Sep(";");
				if( Temp > 0.0f)
					m_EndTelTime = (int)(Temp * SERVER_TICKSPERSECOND);
			}
			return true;
		};
	default:
		return CAI_Behaviour::OnMessage(_Msg);
	};
};


//Always max prio
int CAI_Behaviour_Vatar::GetPathPrio(fp32 _CameraScore)
{
	MAUTOSTRIP(CAI_Behaviour_Vatar_GetPathPrio, 0);
	return 255;
};

