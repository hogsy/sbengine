#include "PCH.h"
#include "WRPGCreature.h"
#include "../WObj_Char.h"
#include "WRPGWeapon.h"

const char* CRPG_Object_CreatureAttack::ms_pszDirectionFlags[] = {"forward", "backward", "move", "usebasepos", "nowait", "checklos", NULL};

void CRPG_Object_CreatureAttack::ParseActiveTime(const CRegistry* _pKey)
{
	CFStr First = "";
	CFStr Second = "";
	int Comma = _pKey->GetThisValue().Find(",");
	if (Comma != -1)
	{
		First = _pKey->GetThisValue().Copy(0, Comma);
		Second = _pKey->GetThisValue().Copy(Comma+1,_pKey->GetThisValue().Len());
	} else {
		First = _pKey->GetThisValue();
		Second = First;
	}

	m_StartTick = First.Val_int();
	m_StopTick = Second.Val_int();
}

int CRPG_Object_CreatureAttack::FindAttackTargets(const CMat4Dfp32& _Mat, int _iObject, const int16** _ppRetList)
{
	//Calculate size. Size must encompass the four points defined by pos + fwd*depth/2, pos - fwd*depth/2,
	//pos + side * width/2 and pos - side * width/2, with a lowest possible height.
	CVec3Dfp32 Size;
	{
		CVec3Dfp32 Fwd = CVec3Dfp32::GetRow(_Mat, 0);
		CVec3Dfp32 Side = CVec3Dfp32::GetRow(_Mat, 1);
		CVec3Dfp32 Max = CVec3Dfp32(Fwd * m_HalfAttackDepth);
		CVec3Dfp32 Min = CVec3Dfp32(Fwd * m_HalfAttackDepth);

		//Check the remaining positions
		CVec3Dfp32 Temp;
		for (int i = 0; i < 3; i++)
		{
			switch (i)
			{
			case 0:
				Temp = -Fwd * m_HalfAttackDepth;
				break;
			case 1:
				Temp = Side * m_HalfAttackWidth;
				break;
			case 2:
				Temp = -Side * m_HalfAttackWidth;
				break;
			}
            for (int j = 0; j < 3; j++)
			{
				if (Temp[j] > Max[j])
					Max[j] = Temp[j];
				else if (Temp[j] < Min[j])
					Min[j] = Temp[j];
			}
		}
		Size = Max - Min;
	
		//Pad size if appropriate
		{
			for (int i = 0; i < 2; i++)
                if (Size[i] < 10.0f)
					Size[i] = 10.0f;
			if (Size[2] < m_MinAttackHeight)
				Size[2] = m_MinAttackHeight;
		}
	}

	//Selection should be centered on given position, so we must offset used position accordingly
	CVec3Dfp32 Pos = CVec3Dfp32::GetRow(_Mat, 3);
	Pos = Pos - (Size * 0.5);
	
	//Make selection
	TSelection<CSelection::LARGE_BUFFER> Selection;
	CWO_PhysicsState PhysState;
	PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_BOX, 0, Size, 0));
	PhysState.m_ObjectFlags = OBJECT_FLAGS_PROJECTILE;
	PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
	PhysState.m_iExclude = _iObject;
	m_pWServer->Selection_AddIntersection(Selection, Pos, PhysState);
	int nSel = m_pWServer->Selection_Get(Selection, _ppRetList);

#ifndef M_RTM
	//DEBUG render selection box
	static const int s_lSides[12][2][3] = 
	{
		{{0,0,0},{0,0,1}},
		{{0,0,0},{0,1,0}},
		{{0,0,0},{1,0,0}},
		{{0,0,1},{0,1,1}},
		{{0,0,1},{1,0,1}},
		{{0,1,0},{0,1,1}},
		{{0,1,0},{1,1,0}},
		{{0,1,1},{1,1,1}},
		{{1,0,0},{1,0,1}},
		{{1,0,0},{1,1,0}},
		{{1,0,1},{1,1,1}},
		{{1,1,0},{1,1,1}},
	};
	CPixel32 Colour = (nSel > 0) ? 0xffff0000 : 0xff00ff00; 
	for (int i = 0; i < 12; i++)
	{
		CVec3Dfp32 From = Pos + CVec3Dfp32(s_lSides[i][0][0] * Size[0], s_lSides[i][0][1] * Size[1], s_lSides[i][0][2] * Size[2]);
		CVec3Dfp32 To = Pos + CVec3Dfp32(s_lSides[i][1][0] * Size[0], s_lSides[i][1][1] * Size[1], s_lSides[i][1][2] * Size[2]); 
		m_pWServer->Debug_RenderWire(From, To, Colour, 0.1f);
	}
	//DEBUG
#endif

	return nSel;
}

bool CRPG_Object_CreatureAttack::CanAttack()
{
	if (m_DelayTicks == 0)
		return true;

	return false;
}

void CRPG_Object_CreatureAttack::Attack(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
//	m_pWServer->Debug_RenderWire(CVec3Dfp32::GetRow(_Mat, 3), CVec3Dfp32::GetRow(_Mat, 3) + CVec3Dfp32(0,0,100));//DEBUG
	m_lHitList.Clear();

	m_DelayTicks = 0;//PlayAnimSequence(_iObject, GetActivateAnim(_iObject), PLAYER_ANIM_ONLYTORSO);
	if (m_DelayTicks <= m_StopTick)
		m_DelayTicks = m_StopTick;

	//Always attack for at least one tick
	if (m_DelayTicks <= 0)
		m_DelayTicks = 1;		

	m_ActiveTick = 0;

	if (!(m_DirectionFlags & DIRECTION_NOWAIT))
		ApplyWait(_iObject, m_DelayTicks, m_NoiseLevel, m_Visibility);
}

void CRPG_Object_CreatureAttack::OnCreate()
{
	CRPG_Object_Item::OnCreate();

	m_DelayTicks = 0;
	m_Damage = 1;
	m_DamageType = DAMAGETYPE_BLOW;
	m_StartTick = 0;
	m_StopTick = 0;
	m_DirectionFlags = DIRECTION_FWD;
	m_ForwardAttackDistance = 20.0f;
	m_BackwardAttackDistance = 20.0f;
	m_HalfAttackWidth = 20;
	m_HalfAttackDepth = 20;
	m_MinAttackHeight = 20;
	m_AttackForce = 4;
	m_Flags |= RPG_ITEM_FLAGS_NOPICKUP;
}

bool CRPG_Object_CreatureAttack::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();

	// How much damage should the attack give
	switch (_KeyHash)
	{
	case MHASH6('CREA','TURE','ATTA','CK_D','AMAG','E'): // "CREATUREATTACK_DAMAGE"
		{
			m_Damage = KeyValue.Val_int();
			break;
		}

	// What kind of damage should the attack give
	case MHASH7('CREA','TURE','ATTA','CK_D','AMAG','ETYP','E'): // "CREATUREATTACK_DAMAGETYPE"
		{
			m_DamageType = (uint32)KeyValue.TranslateFlags(CRPG_Object_Weapon::ms_DamageTypeStr);
			break;
		}

	// In what direction should the attack be
	case MHASH6('CREA','TURE','ATTA','CK_D','IREC','TION'): // "CREATUREATTACK_DIRECTION"
		{
			m_DirectionFlags = KeyValue.TranslateFlags(ms_pszDirectionFlags);
			break;
		}

	// Between what ticks should the attack give damage
	case MHASH7('CREA','TURE','ATTA','CK_A','CTIV','ETIM','E'): // "CREATUREATTACK_ACTIVETIME"
		{
			ParseActiveTime(_pKey);
			break;
		}

	// How many units ahead of the character should forward attack be
	case MHASH8('CREA','TURE','ATTA','CK_F','ORWA','RDDI','STAN','CE'): // "CREATUREATTACK_FORWARDDISTANCE"
		{
			m_ForwardAttackDistance = KeyValuef;
			break;
		}

	// How many units back of the character should backward attack be
	case MHASH8('CREA','TURE','ATTA','CK_B','ACKW','ARDD','ISTA','NCE'): // "CREATUREATTACK_BACKWARDDISTANCE"
		{
			m_BackwardAttackDistance = KeyValuef;
			break;
		}

	// Width of attack selection
	case MHASH5('CREA','TURE','ATTA','CK_W','IDTH'): // "CREATUREATTACK_WIDTH"
		{
			m_HalfAttackWidth = Max(0, Min(255, (int)(KeyValuef / 2)));
			break;
		}

	// Depth of attack selection
	case MHASH5('CREA','TURE','ATTA','CK_D','EPTH'): // "CREATUREATTACK_DEPTH"
		{
			m_HalfAttackDepth = Max(0, Min(255, (int)(KeyValuef / 2)));
			break;
		}

	// Minimum height of attack selection
	case MHASH6('CREA','TURE','ATTA','CK_H','EIGH','T'): // "CREATUREATTACK_HEIGHT"
		{
			m_MinAttackHeight = Max(0, Min(255, (int)KeyValuef));
			break;
		}

	// Force of attack 
	case MHASH5('CREA','TURE','ATTA','CK_F','ORCE'): // "CREATUREATTACK_FORCE"
		{
			m_AttackForce = Max(0, Min(255, (int)KeyValuef));
			break;
		}

	// This wasn't a creatureattack registry key
	default:
		{
			return CRPG_Object_Item::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}

	return true;
}

bool CRPG_Object_CreatureAttack::OnProcess(CRPG_Object* _pRoot, int _iObject)
{
	//Should weapon be active?
 	if (m_DelayTicks > 0)
	{
		//Is weapon active time in the damage dealing interval?
		//Start/stoptime -1 means openended. Openended start is of course same as 0 as well but...
		if((m_ActiveTick >= m_StartTick) && 
		   ((m_ActiveTick < m_StopTick) || (m_StopTick == -1)))
		{
			CWObject* pUser = m_pWServer->Object_Get(_iObject);

			CMat4Dfp32 Mat;
			CWObject_Message Msg(OBJMSG_AIQUERY_GETACTIVATEPOSITION);
			Msg.m_pData = &Mat;
			Msg.m_DataSize = sizeof(CMat4Dfp32);
			if (!pUser || !pUser->OnMessage(Msg))
				return false;

			if (m_DirectionFlags & DIRECTION_USEBASEPOS)
				if (pUser)
					(pUser->GetPosition()).SetRow(Mat, 3);

			uint8 DirectionFlags = (m_DirectionFlags & DIRECTION_ATTACKSELECTIONMASK);
			do
			{
				fp32 AttackDistance = 0.0f;
				bool bBwd = false;
				if ((DirectionFlags & DIRECTION_FWD) == DIRECTION_FWD)
				{	//Forward attack
					DirectionFlags &= ~DIRECTION_FWD;
					AttackDistance = m_ForwardAttackDistance;
				} 
				else if ((DirectionFlags & DIRECTION_BWD) == DIRECTION_BWD)
				{	//Backward attack
					DirectionFlags &= ~DIRECTION_BWD;
					AttackDistance = m_BackwardAttackDistance;
					bBwd = true;
				} 
				else if ((DirectionFlags & DIRECTION_MOVE) == DIRECTION_MOVE)
				{	//Movement based attack; simple for now use backward or forward only
					CVec3Dfp32 Move = pUser->GetPosition() - pUser->GetLastPosition();
					if (Move.LengthSqr() > 0)
					{	//Don't use movedir attacks when immobile
						Move.Normalize();
						if (Move * CVec3Dfp32::GetRow(Mat, 0) > 0)
							AttackDistance = m_ForwardAttackDistance;
						else
						{
							AttackDistance = m_BackwardAttackDistance;					
							bBwd = true;
						}
					}
					DirectionFlags &= ~DIRECTION_MOVE;
				} 
				else
					DirectionFlags = 0;

				if(AttackDistance != 0.0f)
				{
					CVec3Dfp32 Pos;
					if(!bBwd)
						Pos = Mat.GetRow(3) + Mat.GetRow(0) * AttackDistance;
					else
						Pos = Mat.GetRow(3) - Mat.GetRow(0) * AttackDistance;
					TSelection<CSelection::LARGE_BUFFER> Selection;
					m_pWServer->Selection_AddBoundSphere(Selection, OBJECT_FLAGS_CHARACTER, Pos, AttackDistance);
					Mat.GetRow(3) = Pos;
					m_pWServer->Debug_RenderSphere(Mat, AttackDistance, 0xffff00ff);

					const int16* pTargetList;
					int nTargets = m_pWServer->Selection_Get(Selection, &pTargetList);
					for(int iTarget = 0; iTarget < nTargets; iTarget++)
					{
						int iObj = pTargetList[iTarget];
						if (m_lHitList.Find(iObj) == -1 && iObj != _iObject)
						{
							//Check LOS if necessary
							CVec3Dfp32 ForceVec = CVec3Dfp32::GetRow(Mat, 0) * m_AttackForce;
							bool bHit = true;
							if (m_DirectionFlags & DIRECTION_CHECKLOS)
							{
								CWObject * pTarget = m_pWServer->Object_Get(iObj);
								if (pTarget)
								{
									//Hardcoded traceline height... wtf 
									CVec3Dfp32 From = pUser->GetPosition() + CVec3Dfp32(0.0f,0.0f,32.0f);
									CVec3Dfp32 To = pTarget->GetPosition() + CVec3Dfp32(0.0f,0.0f,32.0f);
									if (m_pWServer->Phys_IntersectLine(From, To, 0,
										// ObjectIntersectFlags, Intersect with physobjects, characters...
										OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER,
										//...and solids
										XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID,
										//Exclude user and target
										_iObject, iObj))
									{
										//Something was hit, LOS obstructed
										bHit = false;
									}
								}
								else 
								{
									//No target, no LOS
									bHit = false;
								}
							}
							if (bHit)
							{
								m_lHitList.Add(iObj);
								// Send damage to a generic location
								SendDamage(iObj, CVec3Dfp32::GetRow(Mat, 3), _iObject, m_Damage, m_DamageType, -1, &ForceVec);
								SendMsg(iObj, OBJMSG_RPG_SPEAK_OLD, 1);
							}
						}
					}
				}
			} while(DirectionFlags);

			//int nSel = m_pWServer->Selection_Get(Selection, _ppRetList);
			/*
			//Weapon should deal damage
			uint8 DirectionFlags = (m_DirectionFlags & DIRECTION_ATTACKSELECTIONMASK);

			CWObject * pUser = m_pWServer->Object_Get(_iObject);

			CMat4Dfp32 Mat;
			CWObject_Message Msg(OBJMSG_AIQUERY_GETACTIVATEPOSITION);
			Msg.m_pData = &Mat;
			Msg.m_DataSize = sizeof(CMat4Dfp32);
			if (!pUser || !pUser->OnMessage(Msg))
				return false;


			if (m_DirectionFlags & DIRECTION_USEBASEPOS)
			{
//				CWObject * pUser = m_pWServer->Object_Get(_iObject);
				if (pUser)
					(pUser->GetPosition()).SetRow(Mat, 3);
			}

			do
			{
				const int16* pTargetList;
				int nTargets;
				CVec3Dfp32 ForceVec;
//				fp32 Force = 0.0f;
				fp32 AttackDistance = 0.0f;
				bool bBwd = false;

				if ((DirectionFlags & DIRECTION_FWD) == DIRECTION_FWD)
				{
					//Forward attack
					DirectionFlags &= ~DIRECTION_FWD;
					AttackDistance = m_ForwardAttackDistance;
				} 
				else if ((DirectionFlags & DIRECTION_BWD) == DIRECTION_BWD)
				{
					//Backward attack
					DirectionFlags &= ~DIRECTION_BWD;
					AttackDistance = m_BackwardAttackDistance;
					bBwd = true;
				} 
				else if ((DirectionFlags & DIRECTION_MOVE) == DIRECTION_MOVE)
				{
					//Movement based attack; simple for now use backward or forward only
					CVec3Dfp32 Move = pUser->GetPosition() - pUser->GetLastPosition();
					if (Move.LengthSqr() > 0)
					{
						//Don't use movedir attacks when immobile
						Move.Normalize();
						if (Move * CVec3Dfp32::GetRow(Mat, 0) > 0)
						{
							//Forward attack
							AttackDistance = m_ForwardAttackDistance;
						}
						else
						{
							//Backward attack
							AttackDistance = m_BackwardAttackDistance;
							bBwd = true;
						}
					}
					DirectionFlags &= ~DIRECTION_MOVE;
				} 
				else
					DirectionFlags = 0;

				if (AttackDistance != 0.0f)
				{
					if (bBwd)
					{
						//Backward, mirror fwd direction (note that matrix won't be righthanded any more, but that doesn't matter)
						CVec3Dfp32 Temp = -CVec3Dfp32::GetRow(Mat, 0);
						Temp.SetRow(Mat, 0);
					}

					//Offset position
					CVec3Dfp32 Temp = CVec3Dfp32::GetRow(Mat, 3) + CVec3Dfp32::GetRow(Mat, 0) * AttackDistance;
					Temp.SetRow(Mat, 3);

					//Get targets
					nTargets = FindAttackTargets(Mat, _iObject, &pTargetList);
					ForceVec = CVec3Dfp32::GetRow(Mat, 0) * m_AttackForce;
					int iTarget;
					for(iTarget=0; iTarget<nTargets; iTarget++)
					{
						int iObj = pTargetList[iTarget];
						if (m_lHitList.Find(iObj) == -1)
						{
							//Check LOS if necessary
							bool bHit = true;
							if (m_DirectionFlags & DIRECTION_CHECKLOS)
							{
								CWObject * pTarget = m_pWServer->Object_Get(iObj);
								if (pTarget)
								{
									//Hardcoded traceline height... wtf 
									CVec3Dfp32 From = pUser->GetPosition() + CVec3Dfp32(0.0f,0.0f,32.0f);
									CVec3Dfp32 To = pTarget->GetPosition() + CVec3Dfp32(0.0f,0.0f,32.0f);
									if (m_pWServer->Phys_IntersectLine(From, To, 0,
																	   // ObjectIntersectFlags, Intersect with physobjects, characters...
																	   OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER,
																	   //...and solids
																	   XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID,
																	   //Exclude user and target
																	   _iObject, iObj))
									{
										//Something was hit, LOS obstructed
										bHit = false;
									}
								}
								else 
								{
									//No target, no LOS
									bHit = false;
								}
							}
	
							if (bHit)
							{
								m_lHitList.Add(iObj);
								// Send damage to a generic location
								SendDamage(iObj, CVec3Dfp32::GetRow(Mat, 3), _iObject, m_Damage, m_DamageType, -1, &ForceVec);
								SendMsg(iObj, OBJMSG_RPG_SPEAK_OLD, 1);
							}
						}
					}
				}
			} while(DirectionFlags);
			*/
		}

		//Weapon ceases to be active when delay is up. Open-ended stoptime means weapon should be 
		//active until explicitly deactivated
		if (m_StopTick != -1)
			m_DelayTicks--;
		m_ActiveTick++;
	}

	return CRPG_Object_Item::OnProcess(_pRoot, _iObject);
}

bool CRPG_Object_CreatureAttack::IsEquippable(CRPG_Object* _pRoot)
{
	return true;
}

bool CRPG_Object_CreatureAttack::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	if (CanAttack())
	{
		Attack(_Mat, _pRoot, _iObject, _Input);

		{
			fp32 Target = 0;
			fp32 Speed = 0;
			CWObject_Message Msg(OBJMSG_CHAR_SETITEMANIMTARGET, *(int *)&Target, *(int *)&Speed);
			Msg.m_Reason = m_iItemType >> 8;
			m_pWServer->Message_SendToObject(Msg, _iObject);
		}
		//m_MuzzleFlameOffTick = GetGameTick(_iObject) + 10;
		fp32 Target = 10000000;
		fp32 Speed = 1.0f;
		CWObject_Message Msg(OBJMSG_CHAR_SETITEMANIMTARGET, *(int *)&Target, *(int *)&Speed);
		Msg.m_Reason = m_iItemType >> 8;
		m_pWServer->Message_SendToObject(Msg, _iObject);

		return true;
	}

	return false;
}

bool CRPG_Object_CreatureAttack::Deactivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	//Stop open-ended attack activation
    if (m_StopTick == -1)
		m_DelayTicks = 0;

	return CRPG_Object_Item::Deactivate(_Mat, _pRoot, _iObject, _Input);
}

bool CRPG_Object_CreatureAttack::IsMeleeWeapon()
{
	return true;
}


MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_CreatureAttack, CRPG_Object_Item);
