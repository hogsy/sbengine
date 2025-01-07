#include "PCH.h"
#include "WObj_CreepingDark.h"
#include "../WObj_Game/WObj_GameCore.h"
#include "../WObj_Sys/WObj_Trigger.h"
#include "WObj_TentacleSystem.h"
#include "../../../../Shared/mos/XRModels/Model_BSP4Glass/WBSP4Glass.h"

void RenderSphere(CWorld_PhysState* _pWPhys, const CVec3Dfp32& _Pos, fp32 _Size, fp32 _Duration = 1.0f, int32 _Color = 0xffffffff);

#define CREEPINGDARK_SPEED (120.0f)
#define CREEPINGDARK_JUMP_SPEED (-130.0f)
#define CREEPINGDARK_MOVESPEEDINFLUENCE (0.6f)
#define CREEPINGDARK_BACKTRACKSPEED (800.0f)
#define CREEPINGDARK_STEPUPSIZE (5.0f)
#define CREEPINGDARK_MAXTURNSPEED (0.5f)
// Maximum length of physical movement between surface checks
#define CREEPINGDARK_MAXLENGTH (10.0f)
#define CREEPINGDARK_CHECKPOINTSTURNDIFFVAL (0.8f)
#define CREEPINGDARK_CHECKPOINTDMINDISTANCE (8.0f)
#define CREEPINGDARK_CHECKPOINTSTURNDIFFVAL_HIRES (0.9f)
#define CREEPINGDARK_CHECKPOINTDMINDISTANCE_HIRES (4.0f)
#define CREEPINGDARK_AUTOPULLUPINPUTLIMITHORIZ (0.1f)
#define CREEPINGDARK_AUTOPULLUPINPUTLIMITVERT (0.05f)
#define CREEPINGDARK_DEMONHEADTOKEN (0)
#define CREEPINGDARK_SPHERESIZE (5.0f)
#define CREEPINGDARK_UNITSPERMOVE (10.0f)

#ifndef M_RTM
//#define DEBUG_CREEP
#endif

#ifdef DEBUG_CREEP
#define CREEPDBG_RENDER
#define CREEPDBG(p) {ConOut(p); M_TRACE(p);}
#else
#define CREEPDBG(p)
#endif

enum { 
	class_CharNPC =				MHASH5('CWOb','ject','_','Char','NPC'),
	class_CharPlayer =			MHASH6('CWOb','ject','_','Char','Play','er'),
	class_Object_Lamp =			MHASH6('CWOb','ject','_','Obje','ct_','Lamp'),
	class_TriggerExt =			MHASH6('CWOb','ject','_','Trig','ger_','Ext'),
	class_ObjectItem =			MHASH4('CWOb','ject','_','Item'),
	class_WorldGlassSpawn =		MHASH7('CWOb','ject','_','Worl','dGla','ssSp','awn'),
	class_Glass_Dynamic =		MHASH7('CWOb','ject','_','Glas','s_Dy','nami','c'),
	class_SwingDoor =			MHASH6('CWOb','ject','_','Swin','g','Door'),
};


CSpline_RenderTrail::CSpline_RenderTrail() 
	: m_nTempPoints(0)
{
}


void CSpline_RenderTrail::Create(const int& _nPoints)
{
	m_lPoints.SetLen(_nPoints);
	m_lSegments.SetLen(_nPoints-1);
}


void CSpline_RenderTrail::AddPoint(const CMat4Dfp32& _Pos, const CVec3Dfp32& _Tangent)
{
	// Add a point before all temporary points (Temporaries needs to be re-added before finalizing)
	m_EndMat = _Pos;

	uint iPoint = m_lPoints.Len() - m_nTempPoints;
	m_lPoints.SetLen(iPoint + 1);
	m_lPoints[iPoint].m_Pos = _Pos.GetRow(3);
	m_lPoints[iPoint].m_TangentIn = _Tangent;
	m_lPoints[iPoint].m_TangentOut = _Tangent;

	m_nTempPoints = 0;
}


void CSpline_RenderTrail::PreFinalize()
{
	if(m_nTempPoints > 0)
	{
		m_lPoints.SetLen(m_lPoints.Len() - m_nTempPoints);
		m_nTempPoints = 0;
	}
}


void CSpline_RenderTrail::AddTempPoint(const CMat4Dfp32& _Pos, const CVec3Dfp32& _Tangent)
{
	m_EndMat = _Pos;

	uint iPoint = m_lPoints.Len();
	m_lPoints.SetLen(iPoint + 1);
	m_lPoints[iPoint].m_Pos = _Pos.GetRow(3);
	m_lPoints[iPoint].m_TangentIn = _Tangent;
	m_lPoints[iPoint].m_TangentOut = _Tangent;

	m_nTempPoints++;
}


void CSpline_RenderTrail::Finalize(uint8 _nCachePoints)
{

	M_ASSERT(_nCachePoints <= Segment::MaxCachePoints, "");
	uint nPoints = m_lPoints.Len();
	uint nSegments = nPoints - 1;
	m_lSegments.SetLen(nSegments);
	m_nCachePoints = _nCachePoints;

	// Do not do anything with only one point!
	if(nPoints < 2)
		return;

	m_lTangentOut.SetLen(nPoints);
	m_lTangentIn.SetLen(nPoints);

	for (uint iPoint = 0; iPoint < nPoints; iPoint++)
	{
		Point& p = m_lPoints[iPoint];
		CVec3Dfp32& TangIn = m_lTangentIn[iPoint];
		CVec3Dfp32& TangOut = m_lTangentOut[iPoint];

		fp32 k = p.m_TangentIn.Length();
		fp32 Len1, Len2;
		if (iPoint == 0)
		{
			Len1 = Len2 = (m_lPoints[iPoint+1].m_Pos - p.m_Pos).Length() * k;
		}
		else if (iPoint == nPoints - 1)
		{
			Len1 = Len2 = (p.m_Pos - m_lPoints[iPoint-1].m_Pos).Length() * k;
		}
		else
		{
			CVec3Dfp32 a = (p.m_Pos - m_lPoints[iPoint-1].m_Pos);
			CVec3Dfp32 b = (m_lPoints[iPoint+1].m_Pos - p.m_Pos);
			Len1 = a.Length() * k;
			Len2 = b.Length() * k;
			//p.m_TangentIn = p.m_TangentOut = a + b;
		}
		TangIn = p.m_TangentIn;
		TangOut = p.m_TangentOut;
		TangIn.SetLength(Len1);
		TangOut.SetLength(Len2);
	}

	// Calculate length of each segment (and total length)
	fp32 tStep = 1.0f / _nCachePoints;
	m_Length = 0.0f;
	for (uint iSeg = 0; iSeg < nSegments; iSeg++)
	{
		Segment& s = m_lSegments[iSeg];
		CVec3Dfp32 Pos1, Pos2;
		Pos1 = m_lPoints[iSeg].m_Pos;
		fp32 Len = 0.0f, t = fp32(iSeg);
		for (uint i = 0; i < _nCachePoints; i++)
		{
			Segment::CachedPos& c = s.m_Cache[i];
			c.t = t;
			c.sum = Len;
			t += tStep;
			CalcPos(t, Pos2);
			c.len = Pos1.Distance(Pos2);
			Len += c.len;
			Pos1 = Pos2;
		}
		s.m_SegLen = Len;
		s.m_InvSegLen = 1.0f / Len;
		m_Length += Len;
	}
	m_MaxT = (fp32)nSegments;
}


void CSpline_RenderTrail::CalcPos(fp32 _Time, CVec3Dfp32& _Result) const
{
//	_Time = Clamp(_Time, 0.0f, m_MaxT);
	uint8 iSeg = Min(m_lSegments.Len()-1, (int)_Time);

	const Point& p0 = m_lPoints[iSeg];
	const Point& p1 = m_lPoints[iSeg+1];
	const CVec3Dfp32& TangIn1 = m_lTangentIn[iSeg+1];
	const CVec3Dfp32& TangOut0 = m_lTangentOut[iSeg];

	fp32 t  = _Time - (fp32)iSeg;
	fp32 t2 = t * t;
	fp32 t3 = t * t * t;

	fp32 h0 =  2.0f*t3 + -3.0f*t2 + 1.0f;
	fp32 h1 = -2.0f*t3 +  3.0f*t2;
	fp32 h2 =       t3 + -2.0f*t2 + t;
	fp32 h3 =       t3 -       t2;

	_Result.k[0] = h0 * p0.m_Pos.k[0] + h1 * p1.m_Pos.k[0] + h2 * TangOut0.k[0] + h3 * TangIn1.k[0];
	_Result.k[1] = h0 * p0.m_Pos.k[1] + h1 * p1.m_Pos.k[1] + h2 * TangOut0.k[1] + h3 * TangIn1.k[1];
	_Result.k[2] = h0 * p0.m_Pos.k[2] + h1 * p1.m_Pos.k[2] + h2 * TangOut0.k[2] + h3 * TangIn1.k[2];
}


void CSpline_RenderTrail::CalcRot(fp32 _Time, CMat4Dfp32& _Result, const CVec3Dfp32& _RefUp) const
{
	//NOTE: This is just a helper method - _Result.GetRow(3) must be valid!
	CalcPos(_Time + 0.01f, _Result.GetRow(0));							// Forward
	_Result.GetRow(0) -= _Result.GetRow(3);
	_Result.GetRow(0).Normalize();

	_RefUp.CrossProd(_Result.GetRow(0), _Result.GetRow(1));				// Side
	_Result.GetRow(1).Normalize();

	_Result.GetRow(0).CrossProd(_Result.GetRow(1), _Result.GetRow(2));	// Up
}


void CSpline_RenderTrail::CalcMat(fp32 _Time, CMat4Dfp32& _Result, const CVec3Dfp32& _RefUp) const
{
	_Result.UnitNot3x3();
	CalcPos(_Time, _Result.GetRow(3));
	CalcRot(_Time, _Result, _RefUp); 
}


void CSpline_RenderTrail::FindPos(fp32 _Distance, SplinePos& _Result) const
{
	uint nSegments = m_lSegments.Len();
	const Segment* pSegs = m_lSegments.GetBasePtr();
	// First, step entire segments
	for (uint iSeg = 0; iSeg < nSegments; iSeg++)
	{
		const Segment& s = pSegs[iSeg];
		if (_Distance < s.m_SegLen)
		{
			// Then, step through cached points
			const fp32 tCacheStep = 1.0f / m_nCachePoints;
			for (uint i = 0; i < m_nCachePoints; i++)
			{
				if (_Distance <= s.m_Cache[i].len + 0.001f)
				{
					// Then linear interpolate to find t (not exact, but perhaps good enough)
					_Result.t = s.m_Cache[i].t + (_Distance / s.m_Cache[i].len) * tCacheStep;
					CalcPos(_Result.t, _Result.mat.GetRow(3));
					return;
				}
				_Distance -= s.m_Cache[i].len;
			}
			M_ASSERT(false, "error error error");
		}
		_Distance -= s.m_SegLen;
	}
	// Not found.. set to last point then...
	_Result.t = m_MaxT;
	_Result.mat.GetRow(3) = m_lPoints[nSegments].m_Pos;
}


void CSpline_RenderTrail::FindPos(fp32 _Distance, SplinePos& _Result, const CVec3Dfp32& _RefUp) const
{
	_Result.mat.UnitNot3x3();
	FindPos(_Distance, _Result);
	CalcRot(_Result.t, _Result.mat, _RefUp);
}


void CSpline_RenderTrail::Render(CWireContainer& _WC)
{
	for (uint iSeg = 0; iSeg < m_lSegments.Len(); iSeg++)
	{
		const CSpline_RenderTrail::Segment& s = m_lSegments[iSeg];
		CVec3Dfp32 Pos, PrevPos;
		for (uint i = 0; i < m_nCachePoints; i++)
		{
			CalcPos(s.m_Cache[i].t, Pos);
			_WC.RenderWire(Pos - CVec3Dfp32(0.5f,0,0), Pos + CVec3Dfp32(0.5f,0,0), 0xff00ff00, 0.0f, false);
			_WC.RenderWire(Pos - CVec3Dfp32(0,0.5f,0), Pos + CVec3Dfp32(0,0.5f,0), 0xff00ff00, 0.0f, false);
			_WC.RenderWire(Pos - CVec3Dfp32(0,0,0.5f), Pos + CVec3Dfp32(0,0,0.5f), 0xff00ff00, 0.0f, false);

			if (i > 0)
				_WC.RenderWire(PrevPos, Pos, 0xffff00ff, 0.0f, false);
			PrevPos = Pos;
		}
	}

	for (uint iPoint = 0; iPoint < m_lPoints.Len(); iPoint++)
	{
		const CSpline_RenderTrail::Point& p = m_lPoints[iPoint];
		_WC.RenderVertex(p.m_Pos, 0xff800080, 0.0f, false);
	}
}


void CWObject_CreepingDark::OnCreate()
{
	CWObject_CreepingDarkParent::OnCreate();
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	//m_CurrentOrientation = GetPositionMatrix();
	m_AttackPath.Clear();
	m_LastChange = 0;
	m_LastCheckPointDir = 0.0f;
	m_LastHiResCheckPointDir = 0.0f;
	m_CurrentCheckpoint = 0;
	m_LastHitbelow = 0;
	m_bLastChangeDown = false;
	m_bAttackDamageSent = false;
	m_bAttackAnimSent = false;
	m_bIsSnaking = false;
	m_bBloodSpawned = 0;
	m_LastAttack = -1;
	m_iAttackTarget = -1;
	m_bSendStartSound = true;
	if (pCD)
		pCD->Clear();
	m_LastAutoAimTickCheck = 0;
	m_DevourAttackStartTick = 0;
	m_DevourTarget = 0;
	m_LastCost = 0;
	m_MovingAgainstWallTicks = 0;
	m_LastPosCost = GetPosition();
}

void CWObject_CreepingDark::FindAutoAimTargets(void)
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	if(m_LastAutoAimTickCheck + 10 < m_pWServer->GetGameTick())
	{
		m_LastAutoAimTickCheck = m_pWServer->GetGameTick();

		CVec3Dfp32 At = GetPositionMatrix().GetRow(0);

		TSelection<CSelection::SMALL_BUFFER> Selection;
		m_pWServer->Selection_AddBoundSphere(Selection, OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_TRIGGER | OBJECT_FLAGS_PICKUP, GetPosition() + At * 16.0f, 48.0f);
		const int16* piSel;
		int nSel = m_pWServer->Selection_Get(Selection, &piSel);

		fp32 BestDot = -1;

		uint16 iTarget = 0;
		//pCD->m_iAutoAimTarget = 0;

		for(int i = 0; i < nSel; i++)
		{
			int iObj = piSel[i];
			CWObject* pObj = m_pWServer->Object_Get(iObj);
			if(!pObj)
				continue;

			if(iObj == pCD->m_iStartingCharacter)
				continue;

			bool bIsLamp = pObj->IsClass(class_Object_Lamp);
			bool bIsChar = pObj->IsClass(class_CharNPC);
			bool bIsPickup = pObj->IsClass(class_ObjectItem);
			bool bIsTrigger = pObj->IsClass(class_TriggerExt);

			if(bIsTrigger)
			{	//Check if CD can trigger this trigger
				uint16 Notifyflags = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_TRIGGER_GET_INTERSECT_NOTIFYFLAGS), iObj);
				if(!(Notifyflags & OBJECT_FLAGS_CREEPING_DARK))
					bIsTrigger = false;
			}

			if(bIsChar)
			{
				CWObject_Character *pChar = safe_cast<CWObject_Character>(pObj);
				if (!pChar->HasHeart())
					bIsChar = false;
			}

			if(!(bIsLamp || bIsChar || bIsPickup || bIsTrigger))
				continue;

			CVec3Dfp32 Vec = pObj->GetPosition() - GetPosition();
			fp32 Dist = Vec.LengthSqr();
			Vec.Normalize();
			fp32 NewDot = Vec * At;
			if(NewDot > BestDot && Dist < 128.0f)
			{
				BestDot = NewDot;
				iTarget = iObj;
			}
		}
		if (pCD->m_iAutoAimTarget != iTarget)
			pCD->m_iAutoAimTarget = iTarget;
	}
}

CVec3Dfp32 CWObject_CreepingDark::GetHeartPos(int32 _iObj)
{
	CVec3Dfp32 Result(0.0f, 0.0f, 0.0f);
	CWObject *pObj = m_pWServer->Object_Get(_iObj);
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	CWObject* pObjChar = m_pWServer->Object_Get(pCD->m_iStartingCharacter);
	CWO_Character_ClientData *pCDChar = CWObject_Character::GetClientData(pObjChar);
	CWO_TentacleSystem_ClientData& CDTentacle = CWObject_TentacleSystem::GetClientData(m_pWServer->Object_Get(pCDChar->m_iDarkness_Tentacles));
	CDTentacle.GetCharBonePos(*pObj, TENTACLE_DEVOUR_ROTTRACK_HEART, Result);

	return Result;
}

void CWObject_CreepingDark::OnRefresh()
{
	CWObject_CreepingDarkParent::OnRefresh();

	// Send initial sounds (Didn't work on spawn tick ??)
	if(m_bSendStartSound && m_pWServer->GetGameTick() > m_CreationGameTick)
	{
		// Play enter sound
		NetMsgSendSound(m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_drk_crp01"), CREEPINGDARK_SOUND_AT, 0);

		// Play looping sound
		iSound(0) = m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_drk_crp03");
		m_DirtyMask |= CWO_DIRTYMASK_SOUND;

		m_bSendStartSound = false;
	}

	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	if (pCD->m_MoveMode == CREEPINGDARK_MOVEMODE_CREEPING)
		FindAutoAimTargets();


	// Move the damn thing...
	//UpdateOrientation();
	
	CWO_PhysicsState Phys = GetPhysState();
	if (Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSOBJECT)
		UpdateVelocityModifiers();
	
	CMat4Dfp32 LookMat = GetPositionMatrix();
	CVec3Dfp32 Velocity;
	GetVelocity(this,CVec3Dfp32::GetMatrixRow(LookMat,0),Velocity);
	CVec3Dfp32 ControlMove(0,0,0);
	CVec3Dfp32 Acc;
	//NewGetAcc(m_pWServer, this, iSel, ControlMove, Acc);
	//Velocity = Acc;// * m_pWServer->GetGameTickTime();

	switch (pCD->m_MoveMode)
	{
	case CREEPINGDARK_MOVEMODE_UNDEFINED:
		pCD->m_MoveMode = CREEPINGDARK_MOVEMODE_CREEPING;
	case CREEPINGDARK_MOVEMODE_CREEPING:
		{
			MoveCreeping(Velocity);
			break;
		}
	case CREEPINGDARK_MOVEMODE_BACKTRACKING:
		{
			MoveBackTrack();
			break;
		}
	case CREEPINGDARK_MOVEMODE_ATTACKING:
		{
			MoveAttack();
			break;
		}
	case CREEPINGDARK_MOVEMODE_DEVOUR:
		{
			MoveDevour();
			break;
		}
	default:
		break;
	}
	//ConOut(GetPosition().GetString());
	// Store a dummy camera - this is used by TentacleSystem
	pCD->m_LastCamera = LookMat;

	m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}

void CWObject_CreepingDark::GetMedium(CXR_MediumDesc& _Medium, const CVec3Dfp32& _MediumV)
{
	_Medium.SetAir();
	TSelection<CSelection::LARGE_BUFFER> Selection;
	//m_pWServer->Selection_AddBoundBox(Selection,OBJECT_FLAGS_STATIC,_MediumV - CVec3Dfp32(0.5f,0.5f,0.5f),_MediumV + CVec3Dfp32(0.5f,0.5f,0.5f));
	m_pWServer->Selection_AddBoundSphere(Selection, OBJECT_FLAGS_STATIC, _MediumV, 100.0f);
	//m_pWServer->Phys_GetMediums(iSel,&MediumV, 1, &Medium);
	
	const int16* piSel;
	int nSel = m_pWServer->Selection_Get(Selection, &piSel);

	fp32 dist = -1.0f;
	for(int i = 0; i < nSel; i++)
	{
		int iObj = piSel[i];
		CWObject_CoreData* pObj = m_pWServer->Object_GetCD(iObj);
		if (!pObj) continue;
		const CWO_PhysicsState& PhysState = pObj->GetPhysState();

		for(int iPrim = 0; iPrim < PhysState.m_nPrim; iPrim++)
		{
			const CWO_PhysicsPrim& PhysPrim = PhysState.m_Prim[iPrim];
			if (PhysPrim.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL)
			{
				CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(PhysPrim.m_iPhysModel);
				if (!pModel) continue;
				CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();
				if (!pPhysModel) continue;
				CXR_AnimState Anim(CMTime::CreateFromTicks(m_pWServer->GetGameTick() - pObj->m_CreationGameTick, m_pWServer->GetGameTickTime(), -pObj->m_CreationGameTickFraction), CMTime(), pObj->m_iAnim0, pObj->m_iAnim1, 0, 0);
				CXR_PhysicsContext PhysContext(pObj->GetPositionMatrix(), &Anim);
				PhysContext.m_PhysGroupMaskThis = PhysPrim.m_PhysModelMask;
				pPhysModel->Phys_Init(&PhysContext);
				pPhysModel->Phys_GetMedium(&PhysContext, &pObj->GetPosition(), 1, &_Medium);

/*					CCollisionInfo CInfo;
					m_pWServer->Debug_RenderWire(cp_pos, GetPosition(), 0xff00ff00);
					m_pWServer->Phys_IntersectLine(cp_pos, GetPosition(), OBJECT_FLAGS_PHYSOBJECT, 0, XW_MEDIUM_LIQUID, &CInfo, m_iObject);
					if(CInfo.m_bIsCollision)
					{
						CVec3Dfp32 dir = GetPositionMatrix().GetRow(0);
						if(dir * CInfo.m_Plane.n < 0.0f)
						{
							CVec3Dfp32 dir = CInfo.m_Pos - GetPosition();
							fp32 Length = dir.Length();

							CBox3Dfp32 BBox;
							pPhysModel->GetBound_Box(BBox);

							fp32 deep = BBox.m_Max.k[0];
							fp32 p = 1.0f - (Length / deep);
							
							CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
							pCD->m_SoftBlock = p;
							m_PhysVelocity.m_Move *= p;

							CMat4Dfp32 Mat;
							Mat.GetRow(3) = CInfo.m_Pos;
							m_pWServer->Debug_RenderMatrix(Mat, 1.0f);
						}
					}
				}*/
			}
		}
	}
}

void CWObject_CreepingDark::MoveCreeping(const CVec3Dfp32& _Velocity)
{
	CVec3Dfp32 Velocity = _Velocity;
	CWO_PhysicsState Phys = GetPhysState();
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);

	if(pCD->m_MoveSpeedTarget > 0.1f && GetMoveVelocity().LengthSqr() < 100.0f)
	{
		m_MovingAgainstWallTicks++;
		if(m_MovingAgainstWallTicks > TruncToInt(m_pWServer->GetGameTicksPerSecond() * 0.5f))
			pCD->m_bCanClimb = true;
	}
	else
		m_MovingAgainstWallTicks = 0;

	if ((!(Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSOBJECT) && pCD->m_bOnGround) || pCD->m_Phys_nInAirFrames > 5)
	{
		// Add physobject to phys so we collide with characters(?)
		// Set forward velocity
		Velocity = CVec3Dfp32::GetMatrixRow(GetPositionMatrix(),0) * CREEPINGDARK_SPEED;
		Phys.m_ObjectFlags |= OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CREEPING_DARK;
		Phys.m_ObjectIntersectFlags |= OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CREEPING_DARK;
		m_pWServer->Object_SetPhysics(m_iObject,Phys);
	}
	
	// Check mediums
	CXR_MediumDesc Medium;
	CVec3Dfp32 MediumV;
	GetAbsBoundBox()->GetCenter(MediumV);
	pCD->m_SoftBlock = 1.0f;
//	GetMedium(Medium,MediumV);

	CMat4Dfp32 LookMat;
	pCD->GetOrientation(m_pWServer,this,LookMat);

	CWO_PhysicsState TmpPhys = GetPhysState();
	fp32 slow_down = 100.0f;
	TmpPhys.m_Prim[0].Create(OBJECT_PRIMTYPE_SPHERE, -1, slow_down, 0.0f);
	TmpPhys.m_MediumFlags = XW_MEDIUM_LIQUID;
	TmpPhys.m_ObjectFlags = OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CREEPING_DARK;
	TmpPhys.m_ObjectIntersectFlags = OBJECT_FLAGS_CREEPING_DARK;
	CMat4Dfp32 p0, p1;
	p0.Unit();
	p0.GetRow(3) = MediumV;
	CCollisionInfo CInfo;
	bool bHit = m_pWServer->Phys_IntersectWorld((CSelection *) NULL, TmpPhys, p0, p0, m_iObject, &CInfo);
	if(bHit && CInfo.m_bIsValid)
	{
		fp32 p = 1.0f - (CInfo.m_Distance / -slow_down);
		if(p < 0.0f)
			p = 0.0f;

		CMat4Dfp32 LookMat;
		CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
		pCD->GetOrientation(m_pWServer,this,LookMat);
		fp32 d = CInfo.m_Plane.n * LookMat.GetRow(0);

		if(pCD->m_MoveSpeedTarget < 0.0f)
			d *= -1.0f;

		if(d > 0.0f)
		{
			pCD->m_MoveSpeed = Max(pCD->m_MoveSpeed, -p);
			p += 0.25f;
			p = Min(p, 1.0f);
		}
		else if(p < 0.1f)
			p = 0.0f;

		pCD->m_SoftBlock = p;
		CVec3Dfp32 Move = GetMoveVelocity();
		m_pWServer->Object_SetVelocity(m_iObject, (Move * p));
	}
	else
	{	//Softblock the player so we can't go inside jackie
		CVec3Dfp32 PlayerPos, Vec;
		CWObject *pPlayer = m_pWServer->Game_GetObject()->Player_GetObject(0);
		PlayerPos = pPlayer->GetPosition() + pPlayer->GetPositionMatrix().GetRow(0) * 16.0f;
		Vec = PlayerPos - LookMat.GetRow(3);
		fp32 vlen = Vec.Length();

		if(vlen < 32.0f)	
		{
			fp32 p = (vlen - 16.0f) / 16.0f;
			p = Max(p, 0.0f);
			fp32 d = LookMat.GetRow(0) * Vec.Normalize();

			if(pCD->m_MoveSpeedTarget < 0.0f)
				d *= -1.0f;

			if(d < 0.0f)
				p = 1.0f;
			else if(p < 0.1f)
				p = 0.0f;

			pCD->m_SoftBlock = p;
			CVec3Dfp32 Move = GetMoveVelocity();
			m_pWServer->Object_SetVelocity(m_iObject, (Move * p));
		}
	}

	pCD->m_bInWater = (Medium.m_MediumFlags & XW_MEDIUM_LIQUID) != 0;
	
	//ConOut(CStrF("MediumFlags: %.8x",Medium.m_MediumFlags));
	if(pCD->m_bOnGround)
	{
		fp32 l = GetMoveVelocity().Length();
		l *= pCD->m_MoveSpeed;
		CVec3Dfp32 NM = LookMat.GetRow(0) * l;
		m_pWServer->Object_SetVelocity(m_iObject, NM);
	}

	// Divide into "steps"
	int32 NumTimeSteps = (int32)M_Ceil(CREEPINGDARK_SPEED / CREEPINGDARK_UNITSPERMOVE);
	fp32 TimeSlice = m_pWServer->GetGameTickTime() / ((fp32)NumTimeSteps);
	for (int32 i = 0; i < NumTimeSteps; i++)
		pCD->Phys_Move(m_pWServer,this,TimeSlice);

	if (!pCD->m_bOnGround)
		pCD->m_Phys_nInAirFrames++;

	//pCD->NewPhys_Move(m_pWServer,this,-1,m_pWServer->GetGameTickTime(),Velocity, false);

	// If it's the first time hitting ground add a checkpoint
	if (pCD->m_bOnGround && (m_lCheckPoints.Len() == 1))
		AddCheckpoint(-pCD->m_Gravity);

	// If direction has changed too much, add a new checkpoint
	CVec3Dfp32 CurrentDir = CVec3Dfp32::GetMatrixRow(LookMat,0);
	if ((m_LastCheckPointDir * CurrentDir < CREEPINGDARK_CHECKPOINTSTURNDIFFVAL) && 
		(m_lCheckPoints.Len() > 1))
	{
		fp32 DistanceSqr = GetPosition().DistanceSqr( GetLastCheckPointType()->m_Position );
		//fp32 DistanceSqr = GetPosition().DistanceSqr( m_lCheckPoints[m_lCheckPoints.Len()-1].m_Position );
		if (DistanceSqr > Sqr(CREEPINGDARK_CHECKPOINTDMINDISTANCE))
		{
			AddCheckpoint(CVec3Dfp32::GetMatrixRow(LookMat,2));
			//ConOut(CStrF("Direction changed tooo much, adding checkpoint, tick: %d",m_pWServer->GetGameTick()));
			// Update checkpointdir
			m_LastCheckPointDir = m_LastHiResCheckPointDir = CurrentDir;
		}
	}
	else if ((m_LastHiResCheckPointDir * CurrentDir < CREEPINGDARK_CHECKPOINTSTURNDIFFVAL_HIRES) && (m_lCheckPoints.Len() > 1))
	{
		//fp32 DistanceSqr = GetPosition().DistanceSqr(m_lCheckPoints[m_lCheckPoints.Len() - 1].m_Position);
		fp32 DistanceSqr = GetPosition().DistanceSqr( GetLastCheckPoint()->m_Position );
		if(DistanceSqr > Sqr(CREEPINGDARK_CHECKPOINTDMINDISTANCE_HIRES))
		{
			AddCheckpoint(CVec3Dfp32::GetMatrixRow(LookMat,2), CREEPINGDARK_CHECKPOINT_HIRES);
			m_LastHiResCheckPointDir = CurrentDir;
		}
	}

	// Check if the demonhead should snake forward
	fp32 dMove = (GetLastPosition() - GetPosition()).LengthSqr();
	if (M_Fabs(pCD->m_MoveSpeedTarget) > 0.2f && (M_Fabs(dMove) > Sqr(2.0f)))
	{
		if (!m_bIsSnaking)
		{
			// Start snaking
			m_bIsSnaking = true;
			iSound(1) = m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_drk_crp06");
			CWObject_CoreData* pObjChar = m_pWServer->Object_GetCD(pCD->m_iStartingCharacter);
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pObjChar);
			if (pCD)
			{
				CStr ImpulseStr = CStrF("%d,%d",TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARK_SNAKE2,CREEPINGDARK_DEMONHEADTOKEN);
				CWObject_Message DemonMsg(OBJMSG_CHAR_ANIMIMPULSE,TENTACLE_AG2_IMPULSETYPE_DEMONHEAD);
				DemonMsg.m_pData = (void*)ImpulseStr.Str();
				m_pWServer->Phys_Message_SendToObject(DemonMsg,pCD->m_iDarkness_Tentacles);
				//TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKATTACK	= 15,
				//TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKRETURN	= 16,
			}
		}
	}
	else if (m_bIsSnaking)
	{
		// Stop snaking
		m_bIsSnaking = false;
		iSound(1) = 0;
		CWObject_CoreData* pObjChar = m_pWServer->Object_GetCD(pCD->m_iStartingCharacter);
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pObjChar);
		if (pCD)
		{
			CStr ImpulseStr = CStrF("%d,%d",TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKRETURN,CREEPINGDARK_DEMONHEADTOKEN);
			CWObject_Message DemonMsg(OBJMSG_CHAR_ANIMIMPULSE,TENTACLE_AG2_IMPULSETYPE_DEMONHEAD);
			DemonMsg.m_pData = (void*)ImpulseStr.Str();
			m_pWServer->Phys_Message_SendToObject(DemonMsg,pCD->m_iDarkness_Tentacles);
			//TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKATTACK	= 15,
			//TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKRETURN	= 16,
		}
	}

	// Check if we should render head
	pCD->m_bRenderHead = m_lCheckPoints.Len() > 2;
}

#define CREEPINGDARK_ATTACKDISTANCE (5.0f)
#define CREEPINGDARK_ATTACKSPEED (20.0f)
#define CREEPINGDARK_ATTACKDURATION (1.0f)
void CWObject_CreepingDark::MoveAttack()
{
	// Do the attack move thingy...
	// Check target position,
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	CWObject_CoreData* pChar = m_pWServer->Object_GetCD(pCD->m_iStartingCharacter);
	CWObject_CoreData* pObj = m_pWServer->Object_GetCD(m_iAttackTarget);
	if (!pObj || !pChar)
	{
		// If object doesn't exist go back to normal movement
		pCD->m_MoveMode = CREEPINGDARK_MOVEMODE_CREEPING;
		return;
	}

	int BlendTicks = TruncToInt(m_pWServer->GetGameTicksPerSecond() / 2);
	if(pCD->m_AttackStartTime + BlendTicks > m_pWServer->GetGameTick())
	{
		CVec3Dfp32 MovePerTick = m_AttackLocalOffset / (fp32)BlendTicks;
		CMat4Dfp32 WMat = GetPositionMatrix();
		WMat.UnitNot3x3();
		CVec3Dfp32 WVec = MovePerTick * WMat;
		CVec3Dfp32 NewPos = GetPosition() + WVec;
		m_pWServer->Object_SetPosition(m_iObject, NewPos);
	}

//  Remake the attack path since the character might have moved
//	MakeAttackPath(pObj);

	// Find position on the attack path where we should be atm
	fp32 Time = (m_pWServer->GetGameTick() - pCD->m_AttackStartTime) * m_pWServer->GetGameTickTime();
	// Scale along the path
	Time /= CREEPINGDARK_ATTACKDURATION;
	// Mod time a bit...
	///Time *= Time;
	Time *= m_AttackPath.m_MaxT;

	CMat4Dfp32 Pos,NewRot;
	CVec3Dfp32 SemiUp = CVec3Dfp32(0.0f,0.0f,1.0f) - m_AttackStartDir;
	SemiUp.Normalize();
	m_AttackPath.CalcMat(Min(m_AttackPath.m_MaxT,Time),Pos,SemiUp);
//	m_pWServer->Object_SetVelocity(m_iObject, Pos.GetRow(3) - GetPosition());

	if(Time >= m_AttackPath.m_MaxT * 1.1f && m_bBloodSpawned < 1)
	{
		CWObject_Character* pChar = TDynamicCast<CWObject_Character>(pObj);
		if (pChar)
		{
			CMat4Dfp32 Pos = pChar->GetPositionMatrix();
			Pos.GetRow(3) = GetHeartPos(m_iAttackTarget);
			m_pWServer->Object_Create("hitEffect_CreepingDark", Pos);
			m_bBloodSpawned++;
		}
	}

	if(Time >= m_AttackPath.m_MaxT * 1.5f && m_bBloodSpawned < 2)
	{
		CWObject_Character* pChar = TDynamicCast<CWObject_Character>(pObj);
		if (pChar)
		{
			CMat4Dfp32 Pos = pChar->GetPositionMatrix();
			Pos.GetRow(3) = GetHeartPos(m_iAttackTarget);
			m_pWServer->Object_Create("hitEffect_CreepingDark", Pos);
			m_bBloodSpawned++;
		}
	}

/*	if ((Time >= m_AttackPath.m_MaxT * 0.8f) && !m_bAttackAnimSent)
	{
		m_bAttackAnimSent = true;
		// Start attack anim
		CWObject_CoreData* pObjChar = m_pWServer->Object_GetCD(pCD->m_iStartingCharacter);
		CWObject_Character* pChar = safe_cast<CWObject_Character>(pObjChar);
		if (pChar)
		{
			CStr ImpulseStr = CStrF("%d,%d",TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKATTACK,CREEPINGDARK_DEMONHEADTOKEN);
			CWObject_Message DemonMsg(OBJMSG_CHAR_ANIMIMPULSE,TENTACLE_AG2_IMPULSETYPE_DEMONHEAD);
			DemonMsg.m_pData = (void*)ImpulseStr.Str();
			m_pWServer->Phys_Message_SendToObject(DemonMsg,pChar->m_iDarkness_Tentacles);
			//TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKATTACK	= 15,
			//TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKRETURN	= 16,
		}
	}*/
	
	// Start backtracking if we've reached the end
	if (Time >= m_AttackPath.m_MaxT * 1.3f)
	{
		if (!m_bAttackDamageSent)
		{
			m_bAttackDamageSent = true;
			// Kill the target
			// Get collision info a bit outside current position to neck
			CCollisionInfo CInfo;
			CVec3Dfp32 StartPos,EndPos;
			int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
			StartPos = GetPosition() - m_AttackStartDir * 15.0f;
			// should be inside the guy now...
			EndPos = GetPosition() + m_AttackStartDir * 5.0f;

			int32 TraceLineFlags = (OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PROJECTILE|OBJECT_FLAGS_CREEPING_DARK);
			int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
			bool bHit = m_pWServer->Phys_IntersectLine(StartPos,EndPos,OwnFlags,TraceLineFlags,MediumFlags,&CInfo,m_iObject);
			if (!bHit || !CInfo.m_bIsValid)
				CInfo.m_Pos = Pos.GetRow(3);

			CInfo.m_Pos += Pos.GetRow(0) * 3.0f;
			/*{
				m_pWServer->Debug_RenderWire(StartPos,EndPos,0xff007f7f,20.0f,false);
				CMat4Dfp32 Mat;
				Mat.Unit();
				CInfo.m_Pos.SetMatrixRow(Mat,3);
				m_pWServer->Debug_RenderMatrix(Mat,20.0f,false);
			}*/
			

			m_pWServer->Debug_RenderMatrix(Pos,20.0f,false);

			CWO_Damage Damage(10000,DAMAGETYPE_DARKNESS,0);
			CVec3Dfp32 SplatDir1(-Pos.GetRow(1));
			CVec3Dfp32 SplatDir2(Pos.GetRow(1));
			// hmm, blood effect...?
			CVec3Dfp32 Force(Pos.GetRow(0) * 5.0f);// = Pos.GetRow(0)*0.1f;
			// Send a couple damages so we get a bit more blood...
			Damage.SendExt(m_iAttackTarget, m_iObject, m_pWServer, &CInfo, &Force, &SplatDir1, 0);
			Damage.SendExt(m_iAttackTarget, m_iObject, m_pWServer, &CInfo, &Force, &SplatDir2, 0);
			//Damage.SendExt(m_iAttackTarget, m_iObject, m_pWServer, &CInfo, &Force, &SplatDir, 0);
		}
		if (Time >= m_AttackPath.m_MaxT * 1.5f)
		{
			//Temporary disabled for E3
			/*BackTrackStart();
			pCD->m_MoveMode = CREEPINGDARK_MOVEMODE_BACKTRACKING;*/
			// Reset demon heads
			// Check what token to run on
			CWO_Character_ClientData* pCharCD = CWObject_Character::GetClientData(pChar);
			if (pCharCD)
			{
				CStr ImpulseStr = CStrF("%d,%d",TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKRETURN,CREEPINGDARK_DEMONHEADTOKEN);
				CWObject_Message DemonMsg(OBJMSG_CHAR_ANIMIMPULSE,TENTACLE_AG2_IMPULSETYPE_DEMONHEAD);
				DemonMsg.m_pData = (void*)ImpulseStr.Str();
				m_pWServer->Phys_Message_SendToObject(DemonMsg,pCharCD->m_iDarkness_Tentacles);
				//TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKATTACK	= 15,
				//TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKRETURN	= 16,
			}
			pCD->m_MoveMode = CREEPINGDARK_MOVEMODE_CREEPING;
			m_pWServer->Object_SetPosition(m_iObject, m_AttackStartPos);
			NewSetPhysics();
		}
	}

/*	CQuatfp32 QC,QT,QRes;

	QC.Create(GetPositionMatrix());
	QT.Create(Pos);
	QC.Lerp(QT, 0.3f, QRes);
	QRes.CreateMatrix3x3(NewRot);
	m_pWServer->Object_SetRotation(m_iObject,NewRot);
	m_pWServer->Object_MovePhysical(m_iObject);*/
	//m_pWServer->Debug_RenderMatrix(Pos,20.0f,false);

	// Move towards targetposition with attack velocity
	// 
/*	CMat4Dfp32 TargetPos;
	bool bGetPos = GetCharBonePos(*pObj,PLAYER_ROTTRACK_NECK,TargetPos);
	// Hrmmm, if this gets screwed that's no good now is it
	if (!bGetPos)
		return;

	CVec3Dfp32 Dir = TargetPos.GetRow(3) - GetPosition();
	fp32 Len = Dir.Length();
	// We want to stop 20 units away or something I guess...
	if (Len <= CREEPINGDARK_ATTACKDISTANCE)
	{
		// If we are within killing distance, kill the target and rewind the creeping dark..?
		m_pWServer->Object_SetVelocity(m_iObject, CVec3Dfp32(0.0f,0.0f,0.0f));
	}
	else
	{
		Dir = Dir/Len;
		// Check so we don't get too close
		Len -= CREEPINGDARK_ATTACKDISTANCE;
		fp32 Speed = Len >= CREEPINGDARK_ATTACKSPEED ? CREEPINGDARK_ATTACKSPEED : Len;
		m_pWServer->Debug_RenderWire(GetPosition(),GetPosition() + Dir * Speed,0xff7f7f7f,10.0f,false);
		m_pWServer->Object_SetVelocity(m_iObject, Dir * Speed);
	}
	m_pWServer->Object_MovePhysical(-1, m_iObject);*/
}

void CWObject_CreepingDark::MoveDevour()
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	CWObject* pObj = m_pWServer->Object_Get(m_DevourTarget);
	
	CMat4Dfp32 LookMat = GetPositionMatrix();
	if(pObj && !m_bBloodSpawned && m_DevourAttackStartTick && m_DevourAttackStartTick + TruncToInt(0.5f * m_pWServer->GetGameTicksPerSecond()) < m_pWServer->GetGameTick())
	{
		CVec3Dfp32 TargetPos = GetHeartPos(m_DevourTarget);

		CMat4Dfp32 Pos = pObj->GetPositionMatrix();
		Pos.GetRow(3) = TargetPos;
		m_pWServer->Object_Create("hitEffect_CreepingDark", Pos);

		CWObject* pObjChar = m_pWServer->Object_Get(pCD->m_iStartingCharacter);
		CWO_Character_ClientData *pCDChar = CWObject_Character::GetClientData(pObjChar);
		CWO_TentacleSystem_ClientData& CDTentacle = CWObject_TentacleSystem::GetClientData(m_pWServer->Object_Get(pCDChar->m_iDarkness_Tentacles));
		CTentacleArmState& ArmState = CDTentacle.m_lArmState[TENTACLE_DEMONHEAD1];
		ArmState.m_iTarget = m_DevourTarget;
		CDTentacle.Server_CreateDevourBlood(ArmState);
		ArmState.m_iTarget = 0;
		
		CWObject_Message Msg(OBJMSG_CHAR_PUSHRAGDOLL);
		Msg.m_VecParam1 = LookMat.GetRow(2) * -4.0f;
		m_pWServer->Message_SendToObject(Msg, m_DevourTarget);
		m_bBloodSpawned++;
	}

	if(pObj && m_bBloodSpawned < 2 && m_DevourAttackStartTick && m_DevourAttackStartTick + TruncToInt(0.65f * m_pWServer->GetGameTicksPerSecond()) < m_pWServer->GetGameTick())
	{
		CMat4Dfp32 Pos = pObj->GetPositionMatrix();
		Pos.GetRow(3) = GetHeartPos(m_DevourTarget);
		m_pWServer->Object_Create("hitEffect_CreepingDark", Pos);

		CWObject_Message Msg(OBJMSG_CHAR_PUSHRAGDOLL);
		Msg.m_VecParam1 = LookMat.GetRow(2) * -4.0f;
		m_pWServer->Message_SendToObject(Msg, m_DevourTarget);
		m_bBloodSpawned++;
	}

	if(pObj && m_bBloodSpawned < 3 && m_DevourAttackStartTick && m_DevourAttackStartTick + TruncToInt(0.8f * m_pWServer->GetGameTicksPerSecond()) < m_pWServer->GetGameTick())
	{
		CMat4Dfp32 Pos = pObj->GetPositionMatrix();
		Pos.GetRow(3) = GetHeartPos(m_DevourTarget);
		m_pWServer->Object_Create("hitEffect_CreepingDark", Pos);

		CWObject_Message Msg(OBJMSG_CHAR_PUSHRAGDOLL);
		Msg.m_VecParam1 = LookMat.GetRow(2) * -4.0f;
		m_pWServer->Message_SendToObject(Msg, m_DevourTarget);
		m_bBloodSpawned++;
	}

	if(m_DevourAttackStartTick && m_DevourAttackStartTick + TruncToInt(3.5f * m_pWServer->GetGameTicksPerSecond()) < m_pWServer->GetGameTick())
	{
		m_DevourAttackStartTick = 0;
		
		CWObject* pObjChar = m_pWServer->Object_Get(pCD->m_iStartingCharacter);
		CWObject_Character* pChar = safe_cast<CWObject_Character>(pObjChar);

		pChar->Char_DevourTarget_Finish(pObjChar->m_iObject);

		m_pWServer->Phys_AddMassInvariantImpulse(m_DevourTarget, m_pWServer->Object_GetPosition(m_DevourTarget), LookMat.GetRow(0) * 5.0f);

		pCD->m_MoveMode = CREEPINGDARK_MOVEMODE_CREEPING;
		m_DevourTarget = 0;
	}
}

void CWObject_CreepingDark::MoveBackTrack()
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	CWObject_CoreData* pChar = m_pWServer->Object_GetCD(pCD->m_iStartingCharacter);
	CWO_Character_ClientData* pCharCD = (pChar ? CWObject_Character::GetClientData(pChar) : NULL);
	if (!pCharCD)
		return;

	// Set position of current backtrack
	CMat4Dfp32 Mat;
	if (BackTrackGetCamera(Mat, m_pWServer->GetGameTick()))
	{
		// Currently the position matrix doesn't seem to get over correctly
		//pCharCD->m_Creep_PrevOrientation = pCharCD->m_Creep_Orientation;
		//pCharCD->m_Creep_Orientation.m_Value.Create(Mat);
		m_pWServer->Object_SetPosition(m_iObject, Mat);
		CQuatfp32 QTest2,QDest;
		QTest2.Create(Mat);
		pCharCD->m_Creep_Orientation.m_Value.Lerp(QTest2,0.15f,QDest);
		pCharCD->m_Creep_Orientation.m_Value = QDest;
		pCharCD->m_Creep_Orientation.MakeDirty();
	}

	if(m_CurrentCheckpoint == 1 && m_bPlayExitSound)
	{
		m_bPlayExitSound = false;
		NetMsgSendSound(m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_drk_crp02"), CREEPINGDARK_SOUND_AT, 0);
	}
	// Check if we've reached the end
	if (m_CurrentCheckpoint <= 0)
	{
		pCD->m_bBackTrackReachedEnd = true;

		CStr ImpulseStr = CStrF("%d,%d",TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKRETURN,CREEPINGDARK_DEMONHEADTOKEN);
		CWObject_Message DemonMsg(OBJMSG_CHAR_ANIMIMPULSE,TENTACLE_AG2_IMPULSETYPE_DEMONHEAD);
		DemonMsg.m_pData = (void*)ImpulseStr.Str();
		m_pWServer->Phys_Message_SendToObject(DemonMsg,pCharCD->m_iDarkness_Tentacles);
	}

	// Check if we should render head
	pCD->m_bRenderHead = m_CurrentCheckpoint > 2;
}

void CWObject_CreepingDark::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_CreepingDark::OnClientRefresh);
	//UpdateVelocity(_pWClient, _pObj);
	//DebugRenderCheckPoints(_pWClient, _pObj);
	//CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	//ConOut(CStrF("CheckPoint: %d StartTime: %f",pCD->m_CurrentCheckpoint.m_Value, pCD->m_BackTrackStart.m_Value.GetTime()));

	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	
	// We need to "pre-finalize" spline before actually finalizeing it so we get rid of any previous
	// temporary points in the spline. Then we re-add any temporaries and finally finalize it to get an updated spline
	CMat4Dfp32 SplinePosMat = pCD->m_LastCamera;

	fp32 d = Clamp01(SplinePosMat.GetRow(0) * pCD->m_Gravity);
	SplinePosMat.GetRow(3) -= SplinePosMat.GetRow(0)*(8.0f + 8.0f*d);
	SplinePosMat.GetRow(3) -= SplinePosMat.GetRow(1)*4.0f;
	SplinePosMat.GetRow(3) -= SplinePosMat.GetRow(2)*3.0f;

	pCD->m_TrailSpline.PreFinalize();
	pCD->m_TrailSpline.AddTempPoint(SplinePosMat, -SplinePosMat.GetRow(0).Normalize());
	pCD->m_TrailSpline.Finalize();
	
	CWObject_CreepingDarkParent::OnClientRefresh(_pObj,_pWClient);
}

bool CWObject_CreepingDark::GetCharBonePos(CWObject_CoreData& _Char, int _RotTrack, CMat4Dfp32& _Pos)
{
	CXR_AnimState Anim;
	CXR_Skeleton* pSkel;
	CXR_SkeletonInstance* pSkelInstance;
	if(CWObject_Character::GetEvaluatedPhysSkeleton(&_Char, m_pWServer, pSkelInstance, pSkel, Anim))
	{
		CVec3Dfp32 Temp;
		const CMat4Dfp32& Mat = pSkelInstance->m_pBoneTransform[_RotTrack];
		_Pos = Mat;
		Temp = pSkel->m_lNodes[_RotTrack].m_LocalCenter;
		Temp *= Mat;
		Temp.SetMatrixRow(_Pos,3);
		return true;
	}
	return false;
}

void CWObject_CreepingDark::CheckTargets(fp32 _Length, fp32 _SphereSize)
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	CWObject_CoreData* pChar = m_pWServer->Object_GetCD(pCD->m_iStartingCharacter);
	CWO_Character_ClientData* pCharCD = (pChar ? CWObject_Character::GetClientData(pChar) : NULL);
	if (!pCharCD || (pCD->m_MoveMode != CREEPINGDARK_MOVEMODE_CREEPING))
		return;

	// Function testing =)
	CWO_Damage Damage(10,DAMAGETYPE_DARKNESS,0);
	bool bDoneAction = false;

	CCollisionInfo CInfo;
	CWObject* pSelectedTarget = SelectTarget(&CInfo);
	if(pSelectedTarget)
	{
		// Check if it's a character
		if (pSelectedTarget->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER)
		{
			bool bCantAttack = false;

			// Check so that we haven't found a darkling
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pSelectedTarget);
			if (pCD)
				bCantAttack = (pCD->IsDarkling() || pCD->m_iPlayer != -1);

			if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETIMMUNE), pSelectedTarget->m_iObject))
				bCantAttack = true;

			// Found a target character
			if (!bCantAttack)
			{
				CVec3Dfp32 TargetPos;
				pSelectedTarget->GetAbsBoundBox()->GetCenter(TargetPos);
				
				if(CreateAttack(pSelectedTarget))
					bDoneAction = true;
			}
		}
		else
		{
			if(pSelectedTarget->IsClass(class_SwingDoor))
			{
				CVec3Dfp32 ForceDir = pSelectedTarget->GetPosition() - GetPosition();
				ForceDir += GetPositionMatrix().GetRow(2);
				ForceDir.Normalize();
				m_pWServer->Phys_AddMassInvariantImpulse(pSelectedTarget->m_iObject, pSelectedTarget->GetPosition(), ForceDir * 3.0f);
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_SWINGDOOR, 4, 1), pSelectedTarget->m_iObject);
				pCD->m_bCanClimb = true;
				pCD->SetGravity(CVec3Dfp32(0.0f, 0.0f, -1.0f), m_pWServer, false, false);
				pCD->m_bCanClimb = false;
			}
			else if(pSelectedTarget->IsClass(class_WorldGlassSpawn) || pSelectedTarget->IsClass(class_Glass_Dynamic))
			{
				// Create matrix to translate point by
				CMat4Dfp32 TempMat; pSelectedTarget->GetLocalPositionMatrix().InverseOrthogonal(TempMat);

				CMat4Dfp32 LookMat;
				pCD->GetOrientation(m_pWServer, this, LookMat, true);
				CVec3Dfp32 Pos = LookMat.GetRow(3) + LookMat.GetRow(0) * 16.0f;

				// Create some crushing data to send down to glass object
				CWO_Glass_MsgData MsgData;
				MsgData.m_Radius = 8.0f;
				MsgData.m_CrushType = GLASS_CRUSH_SPHERE;              
				MsgData.m_ForceDir = LookMat.GetRow(0);                
				MsgData.m_ForceScale = 1.0f;                 
				MsgData.m_ForceRange = 1.0f;                           
				MsgData.m_LocalPosition = Pos * TempMat;               // Transform world pos to object local space
				MsgData.m_iInstance = 0xFFFF;                          // Let glass object decide which instance we hit (Otherwise try this instance)

				CWObject_Message MsgCrush(OBJMSG_GLASS_CRUSH);         // Create crush message
				CWO_Glass_MsgData::SetData(MsgCrush, MsgData);         // Set MsgData in MsgCrush
				m_pWServer->Message_SendToObject(MsgCrush, pSelectedTarget->m_iObject);      // Pray, beg and hope for the best when shooting this message at the object
			}
			else
			{
				// Send destroy beam to "normal" targets and play attack anim
				CMat4Dfp32 LookMat;
				pCD->GetOrientation(m_pWServer,this,LookMat,false);
				
				// Just tweak it untill it feels right enough
				CVec3Dfp32 Force = LookMat.GetRow(0) * 10.0f;
				Damage.SendExt(pSelectedTarget->m_iObject, m_iObject, m_pWServer, NULL, NULL, 0, 0);

				if(pSelectedTarget->GetMass() < 200.0f && m_pWServer->Phys_IsStationary(pSelectedTarget->m_iObject))
					m_pWServer->Phys_AddMassInvariantImpulse(pSelectedTarget->m_iObject, pSelectedTarget->GetPosition(), LookMat.GetRow(0) * 3.0f);

//				Damage.m_Damage = 0;
//				Damage.SendExt(pSelectedTarget->m_iObject, m_iObject, m_pWServer, &CInfo, &Force, 0, 0);
			}
		}
	}

	/*
	// Hit character??, give it a bit of damage if runover :)
	int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
	int32 ObjectFlags = (OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PROJECTILE | OBJECT_FLAGS_TRIGGER);
	int32 TraceLineFlags = (OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PROJECTILE);

	// PLAYER_PHYSFLAGS_NOPROJECTILECOLL fix: Replace with this after Riddick GM
	//int32 ObjectFlags = (_CollisionObjects != 0) ? _CollisionObjects: (OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT);
	int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
	CVec3Dfp32 Origin,Forward;

	CMat4Dfp32 LookMat;
	pCD->GetOrientation(m_pWServer,this,LookMat,false);
	Forward = CVec3Dfp32::GetMatrixRow(LookMat,0);
	GetAbsBoundBox()->GetCenter(Origin);
	CVec3Dfp32 Pos1 = Origin + Forward * (_SphereSize + 0.5f);
	CVec3Dfp32 Pos2 = Pos1 + Forward * _Length;

	CCollisionInfo CInfo;
	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pWServer->Selection_AddBoundSphere(Selection,ObjectFlags,Pos2,_SphereSize);
	//RenderSphere(m_pWServer,Pos2,CREEPINGDARK_SPHERESIZE);
	const int16* pSel;
	int32 NumSel = m_pWServer->Selection_Get(Selection,&pSel);
	CWO_Damage Damage(10,DAMAGETYPE_PISTOL,0);
	bool bDoneAction = false;
	for (int32 i = 0; i < NumSel; i++)
	{
		if (pSel[i] == pCD->m_iStartingCharacter || pSel[i] == m_iObject)
			continue;

		// Make sure we have a clear line of sight towards our target
		CWObject_CoreData* pTargetObj = m_pWServer->Object_GetCD(pSel[i]);
		if (!pTargetObj)
			continue;

		CVec3Dfp32 TargetPos;
		pTargetObj->GetAbsBoundBox()->GetCenter(TargetPos);

		// Make sure nothing's in the way (a door for instance)
		bool bHit = m_pWServer->Phys_IntersectLine(Pos1,TargetPos,OwnFlags,TraceLineFlags,MediumFlags,&CInfo,pSel[i]);
		if (bHit && CInfo.m_bIsValid)
			continue;

		// Check if it's a character
		if (pTargetObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER)
		{
			// Check so that we haven't found a darkling
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pTargetObj);
			if (pCD && pCD->IsDarkling())
				continue;
			// Found a target character
			CVec3Dfp32 TargetPos;
			pTargetObj->GetAbsBoundBox()->GetCenter(TargetPos);
			
			// Make sure we're under the center of the target?
			if (Origin.k[2] > TargetPos.k[2])
				continue;

			// Make sure we don't have anything in the way FIXME, DO BOXTEST HERE INSTEAD!!
			if (!bHit && CreateAttack(pTargetObj))
			{
				bDoneAction = true;
				break;
			}
			//Damage.SendExt(pSel[i], m_iObject, m_pWServer, NULL, &Forward, 0, 0);
		}
		else
		{
			// Send destroy beam to "normal" targets and play attack anim
			Damage.SendExt(pSel[i], m_iObject, m_pWServer, NULL, &Forward, 0, 0);
		}
	}
	*/
	
	// If we didn't find any action to do, play a "nudge" animation on the demon head
	if (!bDoneAction)
	{
		CStr ImpulseStr = CStrF("%d,%d",TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKNUDGE,CREEPINGDARK_DEMONHEADTOKEN);
		CWObject_Message DemonMsg(OBJMSG_CHAR_ANIMIMPULSE,TENTACLE_AG2_IMPULSETYPE_DEMONHEAD);
		DemonMsg.m_pData = (void*)ImpulseStr.Str();
		m_pWServer->Phys_Message_SendToObject(DemonMsg, pCharCD->m_iDarkness_Tentacles);
		m_bIsSnaking = false;
	}
	//m_pWServer->Debug_RenderWire(Pos1, Pos2, 0xFF00FF00, 1.0f);
}

CWObject* CWObject_CreepingDark::SelectTarget(CCollisionInfo* _pCInfo, uint _SelectionMask, const fp32& _MaxRange)
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	
	CMat4Dfp32 Mat;
	pCD->GetOrientation(m_pWServer,this,Mat,false);
	const CVec3Dfp32& Dir = Mat.GetRow(0);
	CVec3Dfp32 StartPos = Mat.GetRow(3);
	CVec3Dfp32 Center = StartPos + Dir * (_MaxRange * 0.5f);
	fp32 Radius = _MaxRange * 0.7f;
	CVec3Dfp32 EndPos;
	
	// Setup collision info
	CCollisionInfo InternalCInfo;
	CCollisionInfo* pCInfo = &InternalCInfo;
	if(_pCInfo)
		pCInfo = _pCInfo;

	uint nObjectFlags = OBJECT_FLAGS_TRIGGER;
	if(_SelectionMask & CREEPINGDARK_SELECTION_CHARACTER)
		nObjectFlags |= OBJECT_FLAGS_CHARACTER;

	if(_SelectionMask & CREEPINGDARK_SELECTION_LAMP)
		nObjectFlags |= OBJECT_FLAGS_OBJECT | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;

	if(_SelectionMask & CREEPINGDARK_SELECTION_OBJECT)
		nObjectFlags |= OBJECT_FLAGS_OBJECT | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;

	const int16* pSel = NULL;
	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pWServer->Selection_AddBoundSphere(Selection, nObjectFlags, Center, Radius);
	uint nSel = m_pWServer->Selection_Get(Selection, &pSel);

	// Render Selection sphere
	#ifndef M_RTM
		CMat4Dfp32 DbgSphere;
		DbgSphere.Unit();
		DbgSphere.GetRow(3) = Center;
		m_pWServer->Debug_RenderSphere(DbgSphere, Radius, 0xff0000ff, 4.0f, true);
		m_pWServer->Debug_RenderVertex(Center, 0xff0000ff, 4.0f, true);
	#endif

	CWObject* pBestObj = NULL;
	if(nSel > 0)
	{
		fp32 BestScore = 0.0f;
		for(uint i=0; i<nSel; i++)
		{
			int iObject = pSel[i];
			if(iObject == pCD->m_iStartingCharacter || iObject == m_iObject)
				continue;

			CWObject* pObj = m_pWServer->Object_Get(iObject);
			const CWO_PhysicsState& Phys = pObj->GetPhysState();
			
			uint nAcceptMask = 0;
			
			// Characters
			bool bIsChar = pObj->IsClass(class_CharNPC) || pObj->IsClass(class_CharNPC);
			bool bIsLamp = pObj->IsClass(class_Object_Lamp);
			bool bIsTrigger = false;
			bool bIsDoor = false;
			bool bIsGlass = pObj->IsClass(class_Glass_Dynamic) || pObj->IsClass(class_WorldGlassSpawn);
			if(bIsChar)
			{
				// Make sure character isn't already dead
				CWObject_Character* pChar = safe_cast<CWObject_Character>(pObj);
				if(CWObject_Character::Char_GetPhysType(pChar) != PLAYER_PHYS_DEAD)
					nAcceptMask |= CREEPINGDARK_SELECTION_CHARACTER;
			}

			// Lamps
			else if (bIsLamp)
			{
				// Make sure light isn't already broken
				if (!(pObj->m_Data[1] & M_Bit(1)))									// DATA_LAMP_FLAGS = 1,  FLAGS_LAMP_STATE_BROKEN = M_Bit(1)
					nAcceptMask |= CREEPINGDARK_SELECTION_LAMP;
			}

			// Rigid objects
			else if(Phys.m_ObjectFlags & OBJECT_FLAGS_OBJECT)
			{
				// Make sure object has a rigid body
				if(pObj->m_pRigidBody2)
					nAcceptMask |= CREEPINGDARK_SELECTION_OBJECT;
				if(pObj->IsClass(class_SwingDoor))
					bIsDoor = true;
			}

			else if(((Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSMODEL) || (Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSOBJECT)) && !(Phys.m_ObjectFlags & OBJECT_FLAGS_WORLD))
				nAcceptMask |= CREEPINGDARK_SELECTION_OBJECT;

			else if(Phys.m_ObjectFlags & OBJECT_FLAGS_TRIGGER)
			{
				CWObject_Trigger_Ext *pTrigger = TDynamicCast<CWObject_Trigger_Ext>(pObj);
				if(Phys.m_ObjectIntersectFlags & OBJECT_FLAGS_CREEPING_DARK)
				{
					nAcceptMask |= CREEPINGDARK_SELECTION_TIGGER;
					bIsTrigger = true;
				}
			}
			else if(bIsGlass)
				nAcceptMask |= CREEPINGDARK_SELECTION_GLASS;

			// Validation
			if((_SelectionMask & nAcceptMask) == 0)
				continue;

			// Fetch center of objects bounding box
			CVec3Dfp32 ObjPos;
			pObj->GetAbsBoundBox()->GetCenter(ObjPos);

			// Do a line collision against the bounding box and check if we have a hit
			CVec3Dfp32 HitPos;
			EndPos = StartPos + ((ObjPos - StartPos).Normalize() * _MaxRange);
			
			#ifndef M_RTM
				// Wire to object testing
				m_pWServer->Debug_RenderWire(StartPos, EndPos, 0xffff00ff, 4.0f, true);
			#endif

			//Glass gets a special treatment here because the glass objects position isn't where the actual glass we are trying hit is
			//so we don't have any data that can be used in the LOS and angle/distance checks, so glass will pass them all but get a lower default score
			if(!pObj->GetAbsBoundBox()->IntersectLine(StartPos, EndPos, HitPos) && !bIsGlass)
				continue;

			// Distance validation
			CVec3Dfp32 PosToObj = HitPos - StartPos;
			fp32 ProjDist = Dir * PosToObj;
			if(ProjDist > _MaxRange && !bIsGlass)
				continue;

			// Angle validation
			fp32 Distance = PosToObj.Length();
			fp32 CosAngle = MaxMT(ProjDist,1.0f) / Distance;
			if (CosAngle < 0.4f && !bIsGlass)
				continue;

			EndPos = ObjPos;
			if (Phys.m_ObjectFlags & OBJECT_FLAGS_OBJECT)
			{
				// Trace forward for objects,
				EndPos = StartPos + Dir * ProjDist;
			}
			else if ((Phys.m_nPrim > 0) && (Phys.m_PhysFlags & OBJECT_PHYSFLAGS_OFFSET))
			{
				CVec3Dfp32 Offset = Phys.m_Prim[0].GetOffset();
				if (Phys.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION)
					Offset.MultiplyMatrix3x3(pObj->GetPositionMatrix());
				EndPos = pObj->GetPosition() + Offset;
			}

			// Test intersection
			int ObjectFlags = OBJECT_FLAGS_PROJECTILE;
			int IntersectionFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
			int MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;

			bool bHit = m_pWServer->Phys_IntersectLine(StartPos, EndPos, ObjectFlags, IntersectionFlags, MediumFlags, pCInfo, m_iObject);
			//We don't do this on glass since the glass object can be pretty much anywhere in the level
			if(bHit && pCInfo->m_bIsValid && pCInfo->m_iObject != iObject && pCInfo->m_iObject != pCD->m_iStartingCharacter && !bIsGlass)
				continue;

			#ifndef M_RTM
				// Draw a wire line to scoring object
				m_pWServer->Debug_RenderWire(Center, ObjPos, 0xff0000ff, 4.0f, true);
			#endif
			// Skip grab points, otherwise it would be wise to calculate it here!

			// Score the object, the higher the better
			fp32 Score = (CosAngle * (1.0f - Distance / _MaxRange)) + 0.5f;
			if(bIsGlass)
				Score = 0.1f;
			
			// If this is a character we raise the priority slightly since this is probably what we want to attack, hopefully =)
			if(bIsChar)
				Score += 1.0f;
			else if(bIsLamp)
				Score += 0.5f;
			else if(bIsTrigger)
				Score += 0.7f;
			else if(bIsDoor)
				Score +=0.25f;

			// If we got a better score, replace old object with new one
			if (Score > BestScore)
			{
				/*
				#ifndef M_RTM
				    if(bIsChar)
				    {
					    M_TRACEALWAYS("Scoring character as target %d: %f\n", i, Score);
				    }
				    else if(bIsLamp)
				    {
					    M_TRACEALWAYS("Scoring lamp as target %d: %f\n", i, Score);
				    }
				    else
				    {
					    M_TRACEALWAYS("Scoring object as target %d: %f\n", i, Score);
				    }
				#endif
				*/
				BestScore = Score;
				pBestObj = pObj;
			}
		}
	}

	// Watch out, can be NULL if don't find any objects
	return pBestObj;
}

void CWObject_CreepingDark::MakeAttackPath(CWObject_CoreData* _pObj)
{
//	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);

	// Found target, check so the path to the target position is correct...
	CMat4Dfp32 TargetPos;
	GetCharBonePos(*_pObj,PLAYER_ROTTRACK_NECK,TargetPos);

	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	pCD->m_NeckPos = TargetPos.GetRow(3);

	// wait with final attack point until we're close...?
	m_AttackPath.Clear();

	// Make sure posmat is aligned...?
	CMat4Dfp32 PosMat;
	PosMat.Unit();
	m_AttackStartPos.SetRow(PosMat,3);
	m_AttackStartPos.SetMatrixRow(PosMat,3);
	CVec3Dfp32 Tangent = m_AttackStartDir;
	Tangent.k[2] = 0.5f;
	Tangent.Normalize();

	m_AttackPath.AddPoint(PosMat,Tangent);

	// Set a point halfway up a little bit out (use pelvis bone as a point?)

	CMat4Dfp32 PelvisPoint;
	GetCharBonePos(*_pObj,PLAYER_ROTTRACK_ROOT,PelvisPoint);
	m_AttackStartDir.SetRow(PelvisPoint,0);
	PelvisPoint.RecreateMatrix(2,0);
	//m_pWServer->Debug_RenderMatrix(PelvisPoint,30.0f,false);
	CVec3Dfp32 Pos(CVec3Dfp32::GetMatrixRow(PelvisPoint,3));
	Pos -= PelvisPoint.GetRow(0) * 15;
	//Pos.k[2] += (TargetPos.GetRow(3).k[2] - CVec3Dfp32::GetMatrixRow(PosMat,3).k[2]) * 0.5f;
	Tangent = Pos - m_AttackStartPos;
	Tangent.Normalize();
	Pos.SetMatrixRow(PosMat,3);

	PosMat.GetRow(3) -= m_AttackStartDir * 10.0f;	

	m_AttackPath.AddPoint(PosMat,Tangent);
	/// REMOVE
#ifndef M_RTM
	bool bDebugPath = false;
	if (bDebugPath)
	{
		m_pWServer->Debug_RenderMatrix(PosMat,1.0f,false);
		m_pWServer->Debug_RenderWire(PosMat.GetRow(3),PosMat.GetRow(3) + Tangent * 10.0f,0xffff0000);
	}
#endif


	// Set another point a bit further up
	m_AttackStartDir.SetRow(TargetPos,0);
	TargetPos.RecreateMatrix(2,0);
	
	// Maybe not entirely safe if target is looking at an extreme direction
	Pos = TargetPos.GetRow(3) - CVec3Dfp32::GetMatrixRow(TargetPos,0) * 13.0f;
	//Pos.k[2] += 5.0f;
	Pos.SetMatrixRow(PosMat,3);
	Tangent = (TargetPos.GetRow(3) - Pos) * 3.0f + (Pos - PelvisPoint.GetRow(3));//CVec3Dfp32(0.0f,0.0f,1.0f) + m_AttackStartDir*0.3f;
	Tangent.Normalize();

	PosMat.GetRow(3) -= m_AttackStartDir * 8.0f;

	m_AttackPath.AddPoint(PosMat,Tangent);
	// REMOVE
#ifndef M_RTM
	if (bDebugPath)
	{
		m_pWServer->Debug_RenderMatrix(PosMat,1.0f,false);
		m_pWServer->Debug_RenderWire(PosMat.GetRow(3),PosMat.GetRow(3) + Tangent * 10.0f,0xffff0000);
	}
#endif

	TargetPos.GetRow(3) -= m_AttackStartDir * 10.0f;	

	// Set end position, get tangent more in line with current direction though (when killing sideways..)
	m_AttackPath.AddPoint(TargetPos,CVec3Dfp32::GetMatrixRow(TargetPos,0));
	m_AttackPath.Finalize();
	// REMOVE
#ifndef M_RTM
	if (bDebugPath)
	{
		m_pWServer->Debug_RenderMatrix(TargetPos,1.0f,false);
		m_pWServer->Debug_RenderWire(TargetPos.GetRow(3),TargetPos.GetRow(3) + CVec3Dfp32::GetMatrixRow(TargetPos,0) * 10.0f,0xffff0000);
	}
#endif

	// Remove...
#ifndef M_RTM
	if (bDebugPath)
	{
		// Debug render the spline attack path
		fp32 NumSegs = 40;
		CVec3Dfp32 PrevPos;
		m_AttackPath.CalcPos(0.0f,PrevPos);
		for (int32 i = 1; i < NumSegs; i++)
		{
			fp32 Time = m_AttackPath.m_MaxT * (((fp32) i) / NumSegs);
			CVec3Dfp32 CurrentPos;
			m_AttackPath.CalcPos(Time,CurrentPos);
			m_pWServer->Debug_RenderWire(PrevPos,CurrentPos,0xff7f7f00,1.0f,false);
			PrevPos = CurrentPos;
		}
	}
#endif
}

bool CWObject_CreepingDark::CreateAttack(CWObject_CoreData* _pObj)
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	CWObject_CoreData* pObjChar = m_pWServer->Object_GetCD(pCD->m_iStartingCharacter);
	if (!pObjChar)
		return false;

	m_pWServer->Debug_RenderSphere(GetPositionMatrix(), 2.5, 0xffff0000, 5.0f);

	pCD->m_MoveMode = CREEPINGDARK_MOVEMODE_ATTACKING;
	m_bAttackDamageSent = false; 
	m_bAttackAnimSent = false;
	m_bBloodSpawned = 0;
	
	// Turn off look/movement etc on target object
	m_pWServer->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETCLIENTFLAGS, 
		PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK, 0, m_iObject), _pObj->m_iObject);
	m_pWServer->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETCOLLISIONMODE, 
		PLAYER_PHYSFLAGS_IMMOBILE|PLAYER_PHYSFLAGS_NOCHARACTERCOLL, 0, m_iObject), _pObj->m_iObject);

	// Set physics so we don't collide with target...
	CWO_PhysicsState Phys = GetPhysState();
//	Phys.m_ObjectFlags &= ~(OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CREEPING_DARK);
//	Phys.m_ObjectIntersectFlags &= ~(OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CREEPING_DARK);
	Phys.Clear();
	m_pWServer->Object_SetPhysics(m_iObject,Phys);

	// Set attack object
	m_iAttackTarget = _pObj->m_iObject;	
	m_AttackStartPos = GetPosition();
	m_AttackStartDir = _pObj->GetPosition() - GetPosition();
	m_AttackStartDir.k[2] = 0.0f;
	m_AttackStartDir.Normalize();
	pCD->m_AttackStartTime = m_pWServer->GetGameTick();

	// Make an attack path (must be updated each tick anyway...?)
	MakeAttackPath(_pObj);

	// Set orientation to that of our target direction of the host character...
	CWO_Character_ClientData* pCDChar = CWObject_Character::GetClientData(pObjChar);
	CMat4Dfp32 Orientation;
	pCDChar->m_Creep_Orientation.m_Value.CreateMatrix3x3(Orientation);
	Orientation.UnitNot3x3();
	m_pWServer->Object_SetRotation(m_iObject, Orientation);

	bool bIsFromBehind = GetPositionMatrix().GetRow(0) * _pObj->GetPositionMatrix().GetRow(0) > 0.0f;
	bool bIsCrouching = CWObject_Character::Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH;

	CVec3Dfp32 Offset = _pObj->GetPosition() - GetPosition();
	CMat4Dfp32 InvWMat;
	GetPositionMatrix().Inverse3x3(InvWMat);
	CVec3Dfp32 LocalOffset = Offset * InvWMat;

	//TODO
	//Make sure left offset is correct
	CStr ImpulseStr, BehStr;
	if(!bIsCrouching)
	{
		if(!bIsFromBehind)
		{
			LocalOffset -= CVec3Dfp32(19.9f, -2.8f, 0.0f);
			BehStr = CStrF("%d,0", AG2_IMPULSEVALUE_CREEPINGDARK_RESPONSE_ATTACK_HIGH);
			ImpulseStr = CStrF("%d,%d",TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARK_ATTACKHIGH,CREEPINGDARK_DEMONHEADTOKEN);
		}
		else
		{
			LocalOffset -= CVec3Dfp32(19.9f, -2.8f, 0.0f);
			BehStr = CStrF("%d,0", AG2_IMPULSEVALUE_CREEPINGDARK_RESPONSE_ATTACK_HIGH);
			ImpulseStr = CStrF("%d,%d",TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARK_ATTACKHIGH,CREEPINGDARK_DEMONHEADTOKEN);
		}
	}
	else
	{
		if(!bIsFromBehind)
		{
			LocalOffset -= CVec3Dfp32(26.8f, -1.4f, 0.0f);
			BehStr = CStrF("%d,0", AG2_IMPULSEVALUE_CREEPINGDARK_RESPONSE_ATTACK_CROUCH);
			ImpulseStr = CStrF("%d,%d",TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARK_ATTACKCROUCH,CREEPINGDARK_DEMONHEADTOKEN);
		}
		else
		{
			LocalOffset -= CVec3Dfp32(26.8f, -1.4f, 0.0f);
			BehStr = CStrF("%d,0", AG2_IMPULSEVALUE_CREEPINGDARK_RESPONSE_ATTACK_CROUCH);
			ImpulseStr = CStrF("%d,%d",TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARK_ATTACKCROUCH,CREEPINGDARK_DEMONHEADTOKEN);
		}
	}
	m_AttackLocalOffset = LocalOffset;

	CWObject_Message DemonMsg(OBJMSG_CHAR_ANIMIMPULSE,TENTACLE_AG2_IMPULSETYPE_DEMONHEAD);
	CWObject_Message BehMsg(OBJMSG_CHAR_ANIMIMPULSE,AG2_IMPULSETYPE_ACTIONCUTSCENE);
	DemonMsg.m_pData = (void*)ImpulseStr.Str();
	BehMsg.m_pData = (void*)BehStr.Str();
	m_pWServer->Phys_Message_SendToObject(DemonMsg,pCDChar->m_iDarkness_Tentacles);
	m_pWServer->Phys_Message_SendToObject(BehMsg,_pObj->m_iObject);

    // Play attack sound
	NetMsgSendSound(m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_drk_crp05"), CREEPINGDARK_SOUND_AT, 0);

	return true;
}

void CWObject_CreepingDark::NetMsgSendSound(int32 iSound, int8 _Type, int8 _AttnType)
{
	// If valid sound passed, send a net msg to play on clients
	if(iSound)
	{
		CNetMsg NetMsg(CREEPINGDARK_NETMSG_SOUND);
		NetMsg.AddInt8(_Type);
		NetMsg.AddInt8(_AttnType);
		NetMsg.AddInt32(iSound);
		
		m_pWServer->NetMsg_SendToObject(NetMsg, m_iObject);
	}
}

void CWObject_CreepingDark::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	switch(_Msg.m_MsgType)
	{
		// Play a creepingdark sound on client, using the cutscene channel since we are fiddling with the channel volumes
		case CREEPINGDARK_NETMSG_SOUND:
		{
			int iPos = 0;
			int8 Type = _Msg.GetInt8(iPos);
			int8 AttnType = _Msg.GetInt8(iPos);
			int32 iSound = _Msg.GetInt32(iPos);
			
			if(iSound)
			{
				switch(Type)
				{
					case CREEPINGDARK_SOUND_AT:
					{
						_pWClient->Sound_At(WCLIENT_CHANNEL_CUTSCENE, _pObj->GetPosition(), iSound, AttnType);
						break;
					}
					case CREEPINGDARK_SOUND_ON:
					{
						_pWClient->Sound_On(WCLIENT_CHANNEL_CUTSCENE, _pObj->m_iObject, iSound, AttnType);
						break;
					}

					default:
						break;
				}
			}

			return;
		}

		//default:
		//{
			//CWObject::OnClientNetMsg(_pObj, _pWClient, _Msg);
			//return;
		//}
	}
}

CWObject_CreepingDark::CCreepingDarkClientData* CWObject_CreepingDark::GetCreepingDarkClientData(CWObject_CoreData* _pObj)
{
	if(_pObj->m_lspClientObj[0] == NULL)
	{
		_pObj->m_lspClientObj[0] = MNew(CCreepingDarkClientData);
		if(!_pObj->m_lspClientObj[0])
			Error_static("CWObject_CreepingDark::GetCreepingDarkClientData", "Could not allocate ClientData.")
			CCreepingDarkClientData *pData = (CCreepingDarkClientData *)(CReferenceCount *)_pObj->m_lspClientObj[0];
		return pData;
	}
	else
		return (CCreepingDarkClientData *)(CReferenceCount *)_pObj->m_lspClientObj[0];
}

const CWObject_CreepingDark::CCreepingDarkClientData* CWObject_CreepingDark::GetCreepingDarkClientData(const CWObject_CoreData* _pObj)
{
	return (const CCreepingDarkClientData *)(const CReferenceCount *)_pObj->m_lspClientObj[0];
}

int CWObject_CreepingDark::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const
{
	CCreepingDarkClientData* pCD = const_cast<CCreepingDarkClientData*>(GetCreepingDarkClientData(this));
	if(!pCD)
		Error_static("CWObject_CreepingDark::OnCreateClientUpdate", "Unable to pack client update.");

	int Flags = CWO_CLIENTUPDATE_EXTRADATA;
	if(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
		Flags = CWO_CLIENTUPDATE_AUTOVAR;
	uint8* pD = _pData;
	pD += CWObject_CreepingDarkParent::OnCreateClientUpdate(_iClient, _pClObjInfo, _pObj, _pData, Flags);
	if (pD - _pData == 0)
		return pD - _pData;

	pD += pCD->OnCreateClientUpdate(pD);
	{
		// Copy checkpoints
		uint8 nCheckPoints = (uint8)m_lCheckPoints.Len();
		uint8 nCopyPoints = nCheckPoints - pCD->m_nLastCheckPoints;

		PTR_PUTUINT8(pD, nCopyPoints);
		if (nCopyPoints > 0)
		{
			const CCheckPoint* pCheckPoints = m_lCheckPoints.GetBasePtr();
			
			// Pack data
			for (uint8 i = nCheckPoints - nCopyPoints; i < nCheckPoints; i++)
			{
				//PTR_PUTFP32(pD, pCheckPoints[i].m_Position.k[0]);
				//PTR_PUTFP32(pD, pCheckPoints[i].m_Position.k[1]);
				//PTR_PUTFP32(pD, pCheckPoints[i].m_Position.k[2]);

				PTR_PUTDATA(pD, pCheckPoints[i].m_Position.k, sizeof(fp32) * 3);
				PTR_PUTDATA(pD, pCheckPoints[i].m_Normal.k, sizeof(fp32) * 3);
			}

			pCD->m_nLastCheckPoints = nCheckPoints;
		}
	}
	int AFlags = 0;
	pCD->AutoVar_Pack(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT, pD, m_pWServer->GetMapData(), AFlags);

	return pD - _pData;
}

int CWObject_CreepingDark::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	MAUTOSTRIP(CWObject_CreepingDark_OnClientUpdate, 0);
	MSCOPESHORT(CWObject_CreepingDark::OnClientUpdate);

	const uint8* pD = &_pData[CWObject_CreepingDarkParent::OnClientUpdate(_pObj, _pWClient, _pData, _Flags)];
	if (_pObj->m_iClass == 0 || pD - _pData == 0) return pD - _pData;

	CCreepingDarkClientData *pCD = GetCreepingDarkClientData(_pObj);

	pD += pCD->OnClientUpdate(pD);
	{
		// Unpack copied checkpoints
		uint8 nCopyPoints = 0;
		PTR_GETUINT8(pD, nCopyPoints);
		
		if (nCopyPoints > 0)
		{
			uint8 nCheckPointsPos = pCD->m_lCheckPointsPos.Len();
			nCopyPoints += nCheckPointsPos;

			pCD->m_lCheckPointsPos.SetLen(nCopyPoints);
			pCD->m_lCheckPointsN.SetLen(nCopyPoints);
			CVec3Dfp32* pCheckPointsPos = pCD->m_lCheckPointsPos.GetBasePtr();
			CVec3Dfp32* pCheckPointsN = pCD->m_lCheckPointsN.GetBasePtr();

			// Add first creeping dark point
			if(nCheckPointsPos == 0)
			{
				PTR_GETDATA(pD, pCheckPointsPos[0].k, sizeof(fp32) * 3);
				PTR_GETDATA(pD, pCheckPointsN[0].k, sizeof(fp32) * 3);

				CMat4Dfp32 SplinePosMat;
				SplinePosMat.Unit3x3();
				SplinePosMat.GetRow(3) = pCheckPointsPos[0];
				pCD->m_TrailSpline.AddPoint(SplinePosMat, SplinePosMat.GetRow(0).Normalize());

				nCheckPointsPos++;
			}

			// Add any point on spline
			for (uint8 i = nCheckPointsPos; i < nCopyPoints; i++)
			{
				PTR_GETDATA(pD, pCheckPointsPos[i].k, sizeof(fp32) * 3);
				PTR_GETDATA(pD, pCheckPointsN[i].k, sizeof(fp32) * 3);

				CMat4Dfp32 SplinePosMat;// = pCD->m_LastCamera;
				CVec3Dfp32 Tangent = (pCheckPointsPos[i] - pCheckPointsPos[i-1]).Normalize();
				SplinePosMat.Unit3x3();
				SplinePosMat.GetRow(3) = pCheckPointsPos[i];
				pCD->m_TrailSpline.AddPoint(SplinePosMat, Tangent.Normalize());

				// Some debug info about client spline
				M_TRACEALWAYS(CStrF("Creeping Dark Client Spline Positions (Add): %d\n", i));
			}
		}
	}
	if(_pObj->m_bAutoVarDirty)
	{
		int Flags = 0;
		//		if(pCD->m_iPlayer != -1)
		//			Flags = 0x1000;
		pCD->AutoVar_Unpack(pD, _pWClient->GetMapData(), Flags);
	}

	return (uint8*)pD - _pData;
}

void CWObject_CreepingDark::UpdateVelocity(const CSelection& _Selection, CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, const CVec3Dfp32& _Velocity, const CVec3Dfp32& _Gravity)
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	if (BackTrackActive(pCD))
		return;

	// Set stepsize...
	_pObj->m_PhysAttrib.m_StepSize = CREEPINGDARK_STEPUPSIZE;

	CMat4Dfp32 Mat;
	Mat = _pObj->GetPositionMatrix();
	CVec3Dfp32 Velocity = _Velocity;
	//GetOrientation(_pObj,Mat);
	/*Velocity.m_Rot.m_Axis = CVec3Dfp32::GetMatrixRow(Mat,2);
	Velocity.m_Rot.m_Angle = pCD->m_TurnSpeed;*/
	//Velocity += CVec3Dfp32::GetMatrixRow(Mat,2) * -CREEPINGDARK_SPEED;
	// Gravity needed? (check if there's ground below us)
	int32 ObjectFlags = 0;
	int32 ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL|OBJECT_FLAGS_CREEPING_DARK;
	int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_CAMERASOLID | XW_MEDIUM_PLAYERSOLID | XW_MEDIUM_DYNAMICSSOLID;
	CCollisionInfo Info;

	// Pos should be middle of bbox
	CVec3Dfp32 Origin;
	_pObj->GetAbsBoundBox()->GetCenter(Origin);
	//Origin += CVec3Dfp32::GetMatrixRow(m_CurrentOrientation,2) * CREEPINGDARK_BOXSIZE;
	// Put feelers 1 tick ahead...
	CVec3Dfp32 TargetPos = Origin + CVec3Dfp32::GetMatrixRow(Mat,2) * -4.0f;

	bool bHit = _pWPhys->Phys_IntersectLine(_Selection, Origin,TargetPos,ObjectFlags, ObjectIntersectFlags, MediumFlags, &Info);//, _pObj->m_iObject);
	/*int32 Color = bHit ? 0xffff0000 : 0xff00ff00;
	_pWPhys->Debug_RenderWire(Origin,TargetPos,Color);*/
	if (!bHit)// && m_LastChange + 3 < m_pWServer->GetGameTick())
	{
		//
		fp32 PrevGrav = _pObj->GetMoveVelocity() * _Gravity;

		Velocity += _Gravity * (PrevGrav + 9.82f * 20 * _pWPhys->GetGameTickTime());
	}
	else if (bHit && _pWPhys->IsServer() && (m_lCheckPoints.Len() == 1))
	{
		// Add checkpoint if it's the first time we hit ground (first checkpoint is when it's created)
		AddCheckpoint(-_Gravity);
		UpdateLastCheckPointDir(_Velocity);
	}
	_pWPhys->Object_SetVelocity(_pObj->m_iObject,Velocity);
	//_pWPhys->Object_MovePhysical(-1, _pObj->m_iObject, _dTime, NULL);
}

void CWObject_CreepingDark::GetVelocity(CWObject_CoreData* _pObj, const CVec3Dfp32& _Direction, CVec3Dfp32& _Velocity)
{
	// First check directional velocity
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	fp32 XFactor, YFactor, ZFactor;
	
	// Flatten direction
	CVec3Dfp32 Direction = _Direction;
	/*Direction -= m_GravityVector * (Direction * m_GravityVector);
	Direction.Normalize();*/

	XFactor = CVec3Dfp32(1.0f,0.0f,0.0f) * Direction;
	YFactor = CVec3Dfp32(0.0f,1.0f,0.0f) * Direction;
	ZFactor = CVec3Dfp32(0.0f,0.0f,0.5f) * Direction;

	fp32 Part1 = M_Sqrt(Sqr(XFactor) + Sqr(YFactor));
	fp32 Final = M_Sqrt(Sqr(Part1) + Sqr(ZFactor));

	// Ok, got final directional velocity factor, now take into account that player can control
	// it somewhat
	Final *= pCD->m_MoveSpeed;

	_Velocity = Direction * (Final * CREEPINGDARK_SPEED);
}

void CWObject_CreepingDark::UpdateVelocityModifiers()
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	// Cut diff in half or something
	pCD->m_MoveSpeed += (pCD->m_MoveSpeedTarget - pCD->m_MoveSpeed) * 0.9f * 20.f * m_pWServer->GetGameTickTime();
	if(pCD->m_MoveFlags & CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD)
		pCD->m_MoveSpeed *= 0.5f;
//	else if(pCD->m_LastGravityTick + 1 >= m_pWServer->GetGameTick())
// 		pCD->m_MoveSpeed *= 0.3f;

	pCD->m_MoveSpeed = Max(pCD->m_MoveSpeed, -0.75f);
//	pCD->m_MoveSpeed = Max(pCD->m_MoveSpeed, 0.0f); // Don't allow travelling backwards
}

/*void CWObject_CreepingDark::UpdateOrientation()
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);

	m_LastDirectionChange = m_pWServer->GetGameTick();
	
	// Set position as new orientation
	CMat4Dfp32 PosMat = m_CurrentOrientation;
	GetPosition().SetMatrixRow(PosMat,3);
	m_pWServer->Object_SetPosition(m_iObject, PosMat);
	
	// Update checkpointdir
	m_LastCheckPointDir = CVec3Dfp32::GetMatrixRow(m_CurrentOrientation,0);
}*/

void CWObject_CreepingDark::UpdateLastCheckPointDir(const CVec3Dfp32& _Dir)
{
	m_LastCheckPointDir = m_LastHiResCheckPointDir = _Dir;
}

/*void CWObject_CreepingDark::GetOrientation(CWObject_CoreData* _pObj, CMat4Dfp32& _Mat)
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	pCD->m_qCurrentOrientation.CreateMatrix(_Mat);
}*/

void CWObject_CreepingDark::OnSpawnWorld()
{
	CMat4Dfp32 Pos = GetPositionMatrix();
	CVec3Dfp32::GetMatrixRow(Pos,3).k[2] += CREEPINGDARK_SPHERESIZE + 0.001f;
	/*// Align position with grid
	for (int i = 0; i < 2; i++)
	{
		int iCurrent = (i == 0 ? 0 : 2);
		fp32 DotX = CVec3Dfp32::GetMatrixRow(Pos,iCurrent) * CVec3Dfp32(1.0f,0.0f,0.0f);
		fp32 DotY = CVec3Dfp32::GetMatrixRow(Pos,iCurrent) * CVec3Dfp32(0.0f,1.0f,0.0f);
		fp32 DotZ = CVec3Dfp32::GetMatrixRow(Pos,iCurrent) * CVec3Dfp32(0.0f,0.0f,1.0f);
		fp32 DotXA = M_Fabs(DotX);
		fp32 DotYA = M_Fabs(DotY);
		fp32 DotZA = M_Fabs(DotZ);
		if (DotXA > DotYA)
		{
			if (DotXA > DotZA)
				CVec3Dfp32::GetMatrixRow(Pos,iCurrent) = CVec3Dfp32(1.0f,0.0f,0.0f) * (DotX / DotXA);
			else
				CVec3Dfp32::GetMatrixRow(Pos,iCurrent) = CVec3Dfp32(0.0f,0.0f,1.0f) * (DotZ / DotZA);
			Pos.RecreateMatrix(0,2);
		}
		else
		{
			if (DotYA > DotZA)
				CVec3Dfp32::GetMatrixRow(Pos,iCurrent) = CVec3Dfp32(0.0f,1.0f,0.0f) * (DotY / DotYA);
			else
				CVec3Dfp32::GetMatrixRow(Pos,iCurrent) = CVec3Dfp32(0.0f,0.0f,1.0f) * (DotZ / DotZA);
		}
	}
	Pos.RecreateMatrix(0,2);*/
	m_pWServer->Object_SetPosition(m_iObject,Pos);
	//m_pWServer->Object_SetPhysics(m_iObject, Phys);
	NewSetPhysics();
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	//m_CurrentOrientation = GetPositionMatrix();
	//UpdateOrientation();
	/*pCD->m_qCurrentCamera = pCD->m_qCurrentOrientation;
	pCD->m_qPrevCamera = pCD->m_qCurrentOrientation;*/
	pCD->m_bCopyFirst = true;
	pCD->m_StartTick = m_pWServer->GetGameTick();
	AddCheckpoint(CVec3Dfp32::GetMatrixRow(GetPositionMatrix(),2));

	// Set current gravity
//	CQuatfp32 Grav;
//	Grav.Create(GetPositionMatrix());
	UpdateLastCheckPointDir(CVec3Dfp32::GetMatrixRow(GetPositionMatrix(),0));

	pCD->m_MoveSpeed = 1.0f;
}

aint CWObject_CreepingDark::OnMessage(const CWObject_Message& _Msg)
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	switch (_Msg.m_Msg)
	{
	case OBJMSG_CREEPINGDARK_BACKTRACK:
		{
			if (pCD->m_MoveMode == CREEPINGDARK_MOVEMODE_ATTACKING || pCD->m_MoveMode == CREEPINGDARK_MOVEMODE_DEVOUR)
				return false;

			BackTrackStart();
			pCD->m_MoveMode = CREEPINGDARK_MOVEMODE_BACKTRACKING;
			return true;
		}
	case OBJMSG_CREEPINGDARK_SETSTARTINGCHARACTER:
		{
			pCD->m_iStartingCharacter = _Msg.m_Param0;
			pCD->m_bCopyStartChar = true;
			return true;
		}
	case OBJMSG_CREEPINGDARK_DOATTACK:
		{
//			if (pCD->m_MoveMode != CREEPINGDARK_MOVEMODE_CREEPING || (m_LastAttack != -1 && 
//				((m_pWServer->GetGameTick() - m_LastAttack) < m_pWServer->GetGameTicksPerSecond() * 2.0f)))
//				return false;

			m_LastAttack = m_pWServer->GetGameTick();
			CheckTargets(10.0f,CREEPINGDARK_SPHERESIZE);
			return true;
		}

	case OBJMSG_CHAR_CANPICKUPITEM:
		{
			m_pWServer->Message_SendToObject(_Msg, pCD->m_iStartingCharacter);
			return true;
		}
	case OBJMSG_RPG_AVAILABLEPICKUP:
		{
			m_pWServer->Message_SendToObject(_Msg, pCD->m_iStartingCharacter);
			return true;
		}
	case OBJMSG_CREEPINGDARK_DEVOUR:
		{
			if (pCD->m_MoveMode == CREEPINGDARK_MOVEMODE_ATTACKING || pCD->m_MoveMode == CREEPINGDARK_MOVEMODE_DEVOUR)
				return false;

			DevourStart();
			return true;
		}
	case OBJMSG_CREEPINGDARK_GETDARKNESSCOST:
		{
			return GetDarknessCost();
		}
	case OBJMSG_CREEPINGDARK_JUMP:
		{
			Jump();
		}
		return 1;
	case OBJMSG_CHAR_CREEPINGDARK_SETWALLCLIMB:
		{
			CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
			pCD->m_bNoClimbBlock = _Msg.m_Param0 ? true : false;
		}
		return 1;
	case OBJMSG_CHAR_GETLOOKDIRANDPOSITION:
		{
			if (!_Msg.m_pData || _Msg.m_DataSize < 2*sizeof(CVec3Dfp32))
				return 0;
			CVec3Dfp32* pVec = (CVec3Dfp32*)_Msg.m_pData;
			CVec3Dfp32 tmp(0,0,0);
			CMat4Dfp32 LookMat;
			pCD->GetOrientation(m_pWServer, this, LookMat, true);

			pVec[0] = LookMat.GetRow(3);
			pVec[1] = LookMat.GetRow(0);
			return 1;
		}
	default:
		break;
	}
	return CWObject_CreepingDarkParent::OnMessage(_Msg);
}

void CWObject_CreepingDark::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	CWObject_CreepingDarkParent::OnIncludeClass(_pWData, _pWServer);
	// Darkness sounds
	_pWData->GetResourceIndex_Sound("Gam_drk_crp01");	// Creeping Dark – Enter sound
	_pWData->GetResourceIndex_Sound("Gam_drk_crp02");	// Creeping Dark – Leave sound
	_pWData->GetResourceIndex_Sound("Gam_drk_crp03");	// Creeping Dark – Loop sound
	_pWData->GetResourceIndex_Sound("Gam_drk_crp04");	// Creeping Dark – exit loop sound
	_pWData->GetResourceIndex_Sound("Gam_drk_crp05");	// Creeping Dark – Attack sound
	_pWData->GetResourceIndex_Sound("Gam_drk_crp06");	// Creeping Dark – Move sound
	_pWData->GetResourceIndex_Sound("Gam_ten_dev01");	// Creeping Dark – Devour

	_pWData->GetResourceIndex_Model("CreepingDark");	// Creeping Dark - Trail effect

	_pWData->GetResourceIndex_Surface("darkling_spawn_particle");
	_pWData->GetResourceIndex_Surface("creepingdark_trail");
	_pWData->GetResourceIndex_Class("hitEffect_CreepingDark");
}

void CWObject_CreepingDark::UpdateCameraLook(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CVec2Dfp32& _Look, CVec3Dfp32 _dLook)
{
	if (!_pObj)
		return;

//	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);

	// Update look, rotate to creep space, bind max and rotate back..?
	_Look[0] = Min(0.249f, Max(-0.249f, _Look[0] + _dLook[1]));
	_Look[1] = M_FMod((fp32)_Look[1] + _dLook[2] + 2.0f, 1.0f);
	//pCD->NewProcessLook(_pWPhys,_pObj,_dLook);
}

static bool bDo = true;
// Set orientation of creepingdark from player controlled "look"
void CWObject_CreepingDark::CWObject_SetCreepOrientation(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CVec2Dfp32& _Look)
{
	if (!_pObj)
		return;

	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);	
	if (!_pWPhys || (pCD->m_MoveMode != CREEPINGDARK_MOVEMODE_CREEPING))
		return;

	// Set angles (offset from current grav...?)
	
	CVec3Dfp32 dLook = 0.0f;
	dLook[1] =_Look[0];
	dLook[2] = _Look[1];
	//pCD->NewProcessLook(_pWPhys,_pObj,dLook);
	CWObject_CoreData* pChar = _pWPhys->Object_GetCD(pCD->m_iStartingCharacter);
	CWO_Character_ClientData* pCharCD = (pChar ? CWObject_Character::GetClientData(pChar) : NULL);
	if (pCharCD)
	{
		pCD->SetOrientation(pCharCD,dLook);
	
		if (bDo && pCD->m_MoveMode == CREEPINGDARK_MOVEMODE_CREEPING)
		{
			if(pCD->m_iAutoAimTarget && pCD->m_MoveSpeed > 0.1f)
			{
				CWObject_CoreData* pObj = _pWPhys->Object_GetCD(pCD->m_iAutoAimTarget);
				if(pObj)
				{
					CMat4Dfp32 LookMat;
					pCD->GetOrientation(_pWPhys, _pObj, LookMat);
					CVec3Dfp32 Vec = pObj->GetPosition() - LookMat.GetRow(3);
					CVec3Dfp32 Vec2;
					Vec2.k[0] = Vec * LookMat.GetRow(0);
					Vec2.k[1] = Vec * LookMat.GetRow(1);
					Vec2.k[2] = 0.0f;
					Vec2.Normalize();
					fp32 angle = M_ACos(Vec2.k[0]) / (_PI * 2.0f);
					angle = Min(angle, 0.01f);
					if(Vec2.k[0] < 0.95f && Vec2.k[0] > 0.2f)
					{
						CVec3Dfp32 RotAxis = LookMat.GetRow(2);
						fp32 RightDot = Vec * LookMat.GetRow(1);
						if (RightDot > 0.0f)
							RotAxis = -RotAxis;
						CMat4Dfp32 RotMat, Tmp, Res;
						CQuatfp32(RotAxis, angle).CreateMatrix3x3(RotMat);
						LookMat.Multiply3x3(RotMat, Tmp);
						Res = Tmp;
						Res.GetRow(3) = LookMat.GetRow(3);
						//m_pWServer->Object_SetPosition(m_iObject, Res);
						pCharCD->m_Creep_Orientation.m_Value.Create(Res);
						pCharCD->m_Creep_Orientation.MakeDirty();
					}
				}
			}
		}
	}
	_Look = 0.0f;

	/*CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	CWObject_CoreData* pChar = _pWPhys->Object_GetCD(pCD->m_iStartingCharacter);
	CWO_Character_ClientData* pCharCD = (pChar ? CWObject_Character::GetClientData(pChar) : NULL);
	if (pCharCD && !pCD->m_bLockedRot)
	{
		// Update x look...
		CMat4Dfp32 Mat;
		pCharCD->m_Creep_TargetGravity.m_Value.CreateMatrix3x3(Mat);
		CAxisRotfp32 LookX,LookY;
		CQuatfp32 qLookY;
		LookY.m_Axis = CVec3Dfp32::GetMatrixRow(Mat,2);
		LookY.m_Angle = pCharCD->m_Creep_Control_Look_Wanted.m_Value[1];
		pCharCD->m_Creep_Control_Look_Wanted.m_Value[1] = 0.0f;
		pCharCD->m_Creep_Control_Look_Wanted.MakeDirty();
		LookY.CreateQuaternion(qLookY);
		pCharCD->m_Creep_TargetGravity.m_Value = pCharCD->m_Creep_TargetGravity.m_Value * qLookY;
		pCharCD->m_Creep_TargetGravity.MakeDirty();
		if (pCD->m_bCopyFirst)
		{
			pCD->m_qPrevGravity = pCD->m_qCurrentGravity = pCharCD->m_Creep_TargetGravity.m_Value;
		}
		else
		{
			pCD->m_qCurrentGravity = pCD->m_qCurrentGravity * qLookY;
			pCD->m_qPrevGravity = pCD->m_qPrevGravity * qLookY;
		}
	}*/

	//pCD->m_qCurrentGravity = pCD->m_qCurrentGravity * qLookY;
	//pCD->m_qPrevGravity = pCD->m_qPrevGravity * qLookY;
	//qLook.CreateMatrix3x3(Mat);
	//Look.CreateMatrixFromAngles(0, Mat);
	//_pWPhys->Object_SetRotation(_pObj->m_iObject, Mat);
}

// Set orientation playercontrolled look to creeps orientation
/*void CWObject_CreepingDark::CWObject_SetOrientationFromCreep(CWObject_CoreData* _pObj, CVec2Dfp32& _Look)
{
	if (!_pObj)
		return;

	CVec3Dfp32 Look;
	Look = CWObject_Character::GetLook(_pObj->GetPositionMatrix());
	_Look.k[0] = Look.k[1];
	_Look.k[1] = Look.k[2];
}*/

bool CWObject_CreepingDark::GetCamera(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, CMat4Dfp32& _Cam, const CVec2Dfp32& _Look, int32 _GameTick, fp32 _IPTime)
{
	// Pos of cam
	//UpdateVelocity(_pWPhys,_pObj,SERVER_TIMEPERFRAME * _IPTime);
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	CWObject_CoreData* pChar = _pWClient->Object_GetCD(pCD->m_iStartingCharacter);
	CWO_Character_ClientData* pCharCD = (pChar ? CWObject_Character::GetClientData(pChar) : NULL);
	if (!pCharCD)
		return false;

	_Cam.Unit();

	if (BackTrackActive(_pObj))
	{
		CQuatfp32 Camera;
		pCharCD->m_Creep_PrevOrientation.m_Value.Lerp(pCharCD->m_Creep_Orientation.m_Value,_IPTime,Camera);
		Camera.CreateMatrix3x3(_Cam);
		//_Cam = _pObj->GetPositionMatrix();
	}
	else if (pCD->m_MoveMode == CREEPINGDARK_MOVEMODE_ATTACKING || pCD->m_MoveMode == CREEPINGDARK_MOVEMODE_DEVOUR)
	{
		//Old camera
/*		fp64 PeriodScale = _pWClient->GetModeratedFramePeriod() * _pWClient->GetGameTicksPerSecond();
		Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), _Cam, _pWClient->GetRenderTickFrac() / PeriodScale);

		//Move back a bit to avoid some clipping
		CVec3Dfp32 AttackStartDir;
		AttackStartDir = pCD->m_NeckPos.m_Value - _pObj->GetPosition();
		AttackStartDir.k[2] = 0.0f;
		AttackStartDir.Normalize();

		CVec3Dfp32 Offset = AttackStartDir * -6.0f;
		CVec3Dfp32::GetMatrixRow(_Cam,3) = LERP(_pObj->GetLastPosition() + Offset, _pObj->GetPosition() + Offset, Clamp01(_pWClient->GetRenderTickFrac()));*/

		_Cam = _pObj->GetPositionMatrix();

		CWObject_Message Msg(OBJSYSMSG_GETCAMERA);
		Msg.m_pData = &_Cam;
		Msg.m_DataSize = sizeof(_Cam);
		_pWClient->ClientMessage_SendToObject(Msg, pCharCD->m_iDarkness_Tentacles);
	}
	else
	{
		CQuatfp32 Dest;
		pCharCD->m_Creep_PrevOrientation.m_Value.Lerp(pCharCD->m_Creep_Orientation.m_Value,_IPTime,Dest);
		Dest.CreateMatrix(_Cam);


	/*	CQuatfp32 Gravity;
		pCD->m_qPrevGravity.Interpolate(pCD->m_qCurrentGravity,Gravity,_pWClient->GetRenderTickFrac());
		CMat4Dfp32 Mat;
		Gravity.CreateMatrix3x3(Mat);
		CAxisRotfp32 LookX,LookY;
		CQuatfp32 qLookX,qLookY,qLook;
		LookY.m_Axis = CVec3Dfp32::GetMatrixRow(Mat,2);
		LookY.m_Angle = _Look[1];
		LookY.CreateQuaternion(qLookY);
		LookX.m_Axis = CVec3Dfp32::GetMatrixRow(Mat,1);
		LookX.m_Angle = _Look[0];
		LookX.CreateQuaternion(qLookX);
		
		qLook = Gravity * qLookX * qLookY;
		qLook.CreateMatrix3x3(_Cam);*/


/*
		CAxisRotfp32 LookX,LookY;
		CQuatfp32 qLookX,qLookY,qLook;
		LookY.m_Axis = CVec3Dfp32(0.0f,0.0f,1.0f);
		LookY.m_Angle = _Look[1];
		LookY.CreateQuaternion(qLookY);
		LookX.m_Axis = CVec3Dfp32(0.0f,1.0f,0.0f);
		LookX.m_Angle = _Look[0];
		LookX.CreateQuaternion(qLookX);

		qLook = qLookX * qLookY;

		qLook.CreateMatrix3x3(_Cam);*/


		/*CVec3Dfp32 Look;
		Look.k[0] = 0.0f;
		Look.k[1] = _Look.k[0];
		Look.k[2] = _Look.k[1];
		Look.CreateMatrixFromAngles(0, _Cam);*/
	}

	//CVec3Dfp32 Offset = _pObj->GetAbsBoundBox()->GetCenter(CVec3Dfp32::GetMatrixRow(_Cam,3));
	if (pCD->m_MoveMode != CREEPINGDARK_MOVEMODE_ATTACKING && pCD->m_MoveMode != CREEPINGDARK_MOVEMODE_DEVOUR)
	{	
		CVec3Dfp32::GetMatrixRow(_Cam,3) = LERP(_pObj->GetLastPosition(), _pObj->GetPosition(), Clamp01(_pWClient->GetRenderTickFrac()));
		CMat4Dfp32 FlatOrientation;
		pCD->GetOrientation(_pWClient,_pObj,FlatOrientation,false);

		fp32 MoveScale = 2.0f + Min(-(FlatOrientation.GetRow(2) * _Cam.GetRow(0)),0.0f);

		_Cam.GetRow(3) += _Cam.GetRow(2) * CREEPINGDARK_SPHERESIZE;
 		_Cam.GetRow(3) -= _Cam.GetRow(0) * (MoveScale*CREEPINGDARK_SPHERESIZE);
		
		// Store the calculated camera - this is used by TentacleSystem
		CVec3Dfp32 OldCamPos = pCD->m_LastCamera.GetRow(3);
		pCD->m_LastCamera = _Cam;
		_Cam.GetRow(3) -= _Cam.GetRow(0) * 2.0f;
		_Cam.GetRow(3) += _Cam.GetRow(1) * 2.0f;

		if(!BackTrackActive(_pObj))
		{
			CVec3Dfp32 start, end;
			start = _pObj->GetPosition();
			start += _Cam.GetRow(1) * 4.9f;
			start += _Cam.GetRow(2) * 4.9f;
			end = _Cam.GetRow(3);
			end += _Cam.GetRow(1) * 5.0f;
			CCollisionInfo cinfo;
			int32 ObjectFlags = 0;
			int32 ObjectIntersectionFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_CREEPING_DARK;
			int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
			_pWClient->Phys_IntersectLine(start, end, ObjectFlags, ObjectIntersectionFlags, MediumFlags, &cinfo, _pObj->m_iObject);
			if(cinfo.m_bIsCollision && cinfo.m_bIsValid)
			{
				CVec3Dfp32 cam_to_wall = cinfo.m_Pos - start;
				CVec3Dfp32 new_pos = start + cam_to_wall + cinfo.m_Plane.n * 5.0f;	
				_Cam.GetRow(3) = new_pos;
			}
			else
			{
				end = _Cam.GetRow(3);
				end -= _Cam.GetRow(1) * 2.0f;
				_pWClient->Phys_IntersectLine(start, end, ObjectFlags, ObjectIntersectionFlags, MediumFlags, &cinfo, _pObj->m_iObject);
				if(cinfo.m_bIsCollision && cinfo.m_bIsValid)
				{
					CVec3Dfp32 cam_to_wall = cinfo.m_Pos - start;
					CVec3Dfp32 new_pos = start + cam_to_wall + cinfo.m_Plane.n * 2.0f;
					_Cam.GetRow(3) = new_pos;
				}
			}

			cinfo.Clear();
			end = _Cam.GetRow(3);
			end += _Cam.GetRow(1) * 5.0f;
			//Need another check incase we are in a corner, we could clip through 2 walls, so we gotta do 'em one at a time
			_pWClient->Phys_IntersectLine(start, end, ObjectFlags, ObjectIntersectionFlags, MediumFlags, &cinfo, _pObj->m_iObject);
			if(cinfo.m_bIsCollision && cinfo.m_bIsValid)	
			{	
				CVec3Dfp32 cam_to_wall = cinfo.m_Pos - start;
				CVec3Dfp32 new_pos = start + cam_to_wall + cinfo.m_Plane.n * 5.0f;	
				_Cam.GetRow(3) = new_pos;
			}
		}
	}
/*	else if(pCD->m_MoveMode == CREEPINGDARK_MOVEMODE_ATTACKING)
	{	//We need to fix the camera so it's looking more against it's target
		CVec3Dfp32 up(0.0f, 0.0f, 1.0f);
		CVec3Dfp32 at, right;
		at = pCD->m_NeckPos.m_Value - _pObj->GetPosition();
		at.Normalize();
		at.CrossProd(up, right);
		right.Normalize();
		right.CrossProd(at, up);
		up.Normalize();

		CMat4Dfp32 NewCam, OldCam;
		NewCam.GetRow(0) = at;
		NewCam.GetRow(1) = -right;
		NewCam.GetRow(2) = up;
		NewCam.GetRow(3) = _Cam.GetRow(3);

		up.k[0] = 0.0f;
		up.k[1] = 0.0f;
		up.k[2] = 1.0f;
		at = pCD->m_NeckPos.m_Value - _pObj->GetLastPosition();
		at.Normalize();
		at.CrossProd(up, right);
		right.Normalize();
		right.CrossProd(at, up);
		up.Normalize();
		OldCam.GetRow(0) = at;
		OldCam.GetRow(1) = -right;
		OldCam.GetRow(2) = up;
		OldCam.GetRow(3) = OldCamPos;

//		_Cam = NewCam;
		CMat4Dfp32 Org;
		Org = _Cam;

		fp64 PeriodScale = _pWClient->GetModeratedFramePeriod() * _pWClient->GetGameTicksPerSecond();
		Interpolate2(OldCam, NewCam, _Cam, _pWClient->GetRenderTickFrac() / PeriodScale);
	}*/

	//pCD->m_LastCamera.GetRow(2) = -pCD->m_Gravity;
	//pCD->m_LastCamera.RecreateMatrix(2, 0);

	_Cam.RotX_x_M(-0.25f);
	_Cam.RotY_x_M(0.25f);
	//CVec3Dfp32::GetMatrixRow(_Cam,3).k[2] += 3.0f;
	return true;
}

bool CWObject_CreepingDark::BackTrackGetCamera(CMat4Dfp32& _Cam, int32 _GameTick, fp32 _IPTime)
{
	const CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	if (!BackTrackActive(pCD))
		return false;

	// Ok then find positions
	_Cam.Unit();
	while (!BackTrackGetPos(CVec3Dfp32::GetMatrixRow(_Cam,3),_GameTick, _IPTime)){}
	if (m_CurrentCheckpoint <= 0)
	{
		if (m_lCheckPoints.Len() > 1)
		{
			// Use last position (we've reached endpoint
			CVec3Dfp32::GetMatrixRow(_Cam,2) = m_lCheckPoints[0].GetNormal();
			
			CVec3Dfp32 Direction = m_lCheckPoints[1].m_Position - m_lCheckPoints[0].m_Position;
			Direction.Normalize();
			Direction.SetMatrixRow(_Cam,0);
			_Cam.RecreateMatrix(2,0);
			return true;
		}
		return false;
	}

	// Get direction (from prev to this checkpoint), up is orientation of current checkpoint
	CVec3Dfp32::GetMatrixRow(_Cam,2) = m_lCheckPoints[m_CurrentCheckpoint].GetNormal();
	CVec3Dfp32 Direction = m_lCheckPoints[m_CurrentCheckpoint].m_Position - m_lCheckPoints[m_CurrentCheckpoint-1].m_Position;
	Direction.Normalize();
	Direction.SetMatrixRow(_Cam,0);
	_Cam.RecreateMatrix(0,2);

	return true;
}

void CWObject_CreepingDark::OnDeltaSave(CCFile* _pFile)
{
	CWObject_CreepingDarkParent::OnDeltaSave(_pFile);
}

void CWObject_CreepingDark::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	CWObject_CreepingDarkParent::OnDeltaLoad(_pFile, _Flags);
}

void CWObject_CreepingDark::GetLookQuat(CCreepingDarkClientData* _pCD, const CQuatfp32& _qGravity, CQuatfp32& _qLook, const CVec2Dfp32& _Look, bool _bIncludeY)
{
	CMat4Dfp32 Mat;
	_qGravity.CreateMatrix3x3(Mat);
	CAxisRotfp32 LookX,LookY;
	CQuatfp32 qLookX,qLookY;
	LookX.m_Axis = CVec3Dfp32::GetMatrixRow(Mat,2);
	LookX.m_Angle = _Look[1];
	LookX.CreateQuaternion(qLookX);
	if (_bIncludeY)
	{
		LookY.m_Axis = CVec3Dfp32::GetMatrixRow(Mat,1);
		LookY.m_Angle = _Look[0];
		LookY.CreateQuaternion(qLookY);
		_qLook = _qGravity * qLookY * qLookX;
	}
	else
	{
		_qLook = _qGravity * qLookX;
	}
}

bool CWObject_CreepingDark::BBoxHit(const CSelection& _Selection, const CVec3Dfp32& _Pos, const CWO_PhysicsState& _PhysState)
{
	CMat4Dfp32 Mat; Mat.Unit();
	CVec3Dfp32::GetMatrixRow(Mat, 3) = _Pos;

	const int16* lpObjs;
	int32 nObjs = m_pWServer->Selection_Get(_Selection,&lpObjs);
	for(int i = 0; i < nObjs; i++)
	{
		int iObj = lpObjs[i];

		if (iObj == m_iObject) continue;
		const CWObject_CoreData* pObj = m_pWServer->Object_GetCD(iObj);
		if (!pObj) continue;

		const CWO_PhysicsState& ObjPhysState = pObj->GetPhysState();

		// Do they have any flags in common?
		if (!(ObjPhysState.m_ObjectIntersectFlags & _PhysState.m_ObjectFlags) &&
			!(ObjPhysState.m_ObjectFlags & _PhysState.m_ObjectIntersectFlags)) continue;

		// Intersect physics-states...
		int bIntersect = false;

		if (ObjPhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION)
			bIntersect = m_pWServer->Phys_IntersectStates(ObjPhysState, _PhysState, pObj->GetPositionMatrix(), pObj->GetPositionMatrix(), Mat, Mat, NULL, 0, 0);
		else
		{
			CMat4Dfp32 ObjPos;
			ObjPos.Unit();
			pObj->GetPosition().SetRow(ObjPos, 3);
			bIntersect = m_pWServer->Phys_IntersectStates(ObjPhysState, _PhysState, ObjPos, ObjPos, Mat, Mat, NULL, 0, 0);
		}

		// Intersection?
		if (bIntersect)
			return true;
	}

	return false;
}

int CWObject_CreepingDark::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWObject_CreepingDark_OnPhysicsEvent, 0);
	//return CWObject_CreepingDarkParent::OnPhysicsEvent(_pObj, _pObjOther, _pPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	switch(_Event)
	{
	// Skip for now, use the checkcurrentsurface thingy instead
	case CWO_PHYSEVENT_GETACCELERATION:
		{
			// No accell, just set velocity
			if (_pMat)
				_pMat->Unit();
			if (pCD->m_MoveMode == CREEPINGDARK_MOVEMODE_CREEPING)
			{
				/*CVec3Dfp32 ControlMove(0,0,0);
				pCD->NewGetAcc(_pPhysState, _pObj, -1, ControlMove, _pMat->GetRow(3));*/
				pCD->GetAcc(_pPhysState,_pObj, *_pMat);
			}
			//ConOut(CStrF("CurrentVel: %s",_pObj->GetMoveVelocity().GetString().Str()));
			break;
		}
	case CWO_PHYSEVENT_IMPACT:
		{
			pCD->NewPhys_OnImpact(_pPhysState,_pObj, *_pCollisionInfo);
			break;
		}
	default:
		return CWObject_CreepingDarkParent::OnPhysicsEvent(_pObj, _pObjOther, _pPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);
	}

	return SERVER_PHYS_DEFAULTHANDLER;
}

void CWObject_CreepingDark::BackTrackStart()
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	CWObject_CoreData* pChar = m_pWServer->Object_GetCD(pCD->m_iStartingCharacter);
	CWO_Character_ClientData* pCharCD = (pChar ? CWObject_Character::GetClientData(pChar) : NULL);
	if (!pCD || BackTrackActive(pCD) || !pCharCD)
		return;
	
	CMat4Dfp32 LookMat;
	pCD->GetOrientation(m_pWServer,this,LookMat,false);

	// Add final checkpoint
	AddCheckpoint(CVec3Dfp32::GetMatrixRow(LookMat,2));
	//pCD->m_BackTrackStart = m_pWServer->GetGameTime();
	pCD->m_BackTrackStart = m_pWServer->GetGameTime();
	m_CurrentCheckpoint = m_lCheckPoints.Len() - 1;
	m_pWServer->Object_SetDirty(m_iObject,1);

	// Play leave darkness sound
	iSound(0) = m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_drk_crp04");;

	// Ok, change physics so we don't hit anything on the way back...
	CWO_PhysicsState Phys;
	Phys.Clear();
	m_pWServer->Object_SetPhysics(m_iObject,Phys);
	// Stop snaking
	m_bIsSnaking = false;

	// Reset demon head
	CStr ImpulseStr = CStrF("%d,%d",TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARK_SNAKE2,CREEPINGDARK_DEMONHEADTOKEN);
	CWObject_Message DemonMsg(OBJMSG_CHAR_ANIMIMPULSE,TENTACLE_AG2_IMPULSETYPE_DEMONHEAD);
	DemonMsg.m_pData = (void*)ImpulseStr.Str();
	m_pWServer->Phys_Message_SendToObject(DemonMsg, pCharCD->m_iDarkness_Tentacles);

	m_bPlayExitSound = true;
}

void CWObject_CreepingDark::DevourStart(void)
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);

	CMat4Dfp32 LookMat;
	pCD->GetOrientation(m_pWServer, this, LookMat, false);

	TSelection<CSelection::SMALL_BUFFER> Selection;
	LookMat.GetRow(3) += LookMat.GetRow(0) * 16.0f;
	m_pWServer->Selection_AddBoundSphere(Selection, OBJECT_FLAGS_PICKUP | OBJECT_FLAGS_CHARACTER, LookMat.GetRow(3), 64.0f);
	const int16* pSel = NULL;
	uint8 nSel = m_pWServer->Selection_Get(Selection, &pSel);
	int16 iCorpse = 0;

	for(int i = 0; i < nSel; i++)
	{
		if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ISDEAD), pSel[i]))
		{
			CVec3Dfp32 HeartVec = GetHeartPos(pSel[i]) - GetPosition();
			HeartVec.k[2] = 0.0f;
			HeartVec.Normalize();
			if(HeartVec * LookMat.GetRow(0) > 0.5f)
			{
				CWObject *pObjChar = m_pWServer->Object_Get(pSel[i]);
				CWObject_Character *pChar = TDynamicCast<CWObject_Character>(pObjChar);
				if(pChar && pChar->HasHeart())
				{
					iCorpse = pSel[i];
					break;
				}
			}
		}
	}

	if(iCorpse)
	{
		CWObject_CoreData* pObjChar = m_pWServer->Object_GetCD(pCD->m_iStartingCharacter);
		CWO_Character_ClientData* pCharCD = CWObject_Character::GetClientData(pObjChar);
		if (pCharCD)
		{
			m_DevourTarget = iCorpse;
			CStr ImpulseStr = CStrF("%d,%d",TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARK_DEVOUR, CREEPINGDARK_DEMONHEADTOKEN);
			CWObject_Message DemonMsg(OBJMSG_CHAR_ANIMIMPULSE, TENTACLE_AG2_IMPULSETYPE_DEMONHEAD);
			DemonMsg.m_pData = (void*)ImpulseStr.Str();
			m_pWServer->Phys_Message_SendToObject(DemonMsg, pCharCD->m_iDarkness_Tentacles);

			m_bIsSnaking = false;
			pCD->m_MoveMode = CREEPINGDARK_MOVEMODE_DEVOUR;
			m_DevourAttackStartTick = m_pWServer->GetGameTick();

			NetMsgSendSound(m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_ten_dev01"), CREEPINGDARK_SOUND_AT, 0);
			m_bBloodSpawned = 0;
		}
	}
}

bool CWObject_CreepingDark::BackTrackActive(const CCreepingDarkClientData* pCD)
{
	return !pCD->m_BackTrackStart.m_Value.IsReset();
}

bool CWObject_CreepingDark::BackTrackReachedEnd(CWObject_CoreData* _pObj)
{
	const CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	//return (BackTrackActive(pCD) && (pCD->m_CurrentCheckpoint <= 0));
	return pCD->m_bBackTrackReachedEnd;
}

int CWObject_CreepingDark::GetDarknessCost()
{
	m_LastCost += (m_LastPosCost - GetPosition()).Length();
	int Cost = TruncToInt(m_LastCost / 32.0f);
	m_LastCost -= (fp32(Cost) * 32.0f);
	m_LastPosCost = GetPosition();
	return Cost;
}

bool CWObject_CreepingDark::Renderhead(CWObject_CoreData* _pObj)
{
	const CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	return pCD->m_bRenderHead;
	//return (BackTrackActive(pCD) ? pCD->m_CurrentCheckpoint > 1 : m_lCheckPoints.Len() > 1);
}

int32 CWObject_CreepingDark::GetStartTick(CWObject_CoreData* _pObj)
{
	const CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	return pCD->m_StartTick;
}

int32 CWObject_CreepingDark::GetEndTick(CWObject_CoreData* _pObj)
{
	const CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	return pCD->m_EndTick;
}

bool CWObject_CreepingDark::BackTrackActive(CWObject_CoreData* _pObj)
{
	const CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	return BackTrackActive(pCD);
}


void CWObject_CreepingDark::SetMoveSpeed(CWorld_PhysState* _pWPhys, int32 _iCreep, fp32 _Speed, fp32 _Look)
{
	CWObject_CoreData* pCreep = _pWPhys->Object_GetCD(_iCreep);
	CCreepingDarkClientData* pCD = (pCreep ? GetCreepingDarkClientData(pCreep): NULL);
	CWObject_CoreData* pChar = _pWPhys->Object_GetCD(pCD->m_iStartingCharacter);
	CWO_Character_ClientData* pCDChar = CWObject_Character::GetClientData(pChar);
	if (pCD && pCDChar)
	{
		fp32 Speed = pCD->m_bInWater ? _Speed * 0.5 : _Speed;
		pCD->m_MoveSpeedTarget = Speed;

//		pCDChar->m_Creep_Control_Look_Wanted.m_Value.k[1] = M_FMod(pCDChar->m_Creep_Control_Look_Wanted.m_Value.k[1] + (_Look / -100.0f) + 2.0f, 1.0f);
		if(_Look)
		{
			pCDChar->m_Creep_Control_Look_Wanted.m_Value.k[1] += _Look / -300.0f;
			pCDChar->m_Creep_Control_Look_Wanted.MakeDirty();
		}
	}
}

void CWObject_CreepingDark::SetWallClimb(CWorld_PhysState* _pWPhys, int32 _iCreep, bool _bOn)
{
	CWObject_CoreData* pCreep = _pWPhys->Object_GetCD(_iCreep);
	CCreepingDarkClientData* pCD = (pCreep ? GetCreepingDarkClientData(pCreep): NULL);
	if (pCD)
		pCD->m_bCanClimb = _bOn;
}

bool CWObject_CreepingDark::BackTrackGetPos(CVec3Dfp32& _Pos, int32 _GameTick, fp32 _IPTime)
{
	// If we haven't started backtracking do it now
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	if (!pCD || pCD->m_BackTrackStart.m_Value.IsReset())
		return false;

	if (m_CurrentCheckpoint > 0 && m_CurrentCheckpoint <= m_lCheckPoints.Len())
	{
		// Check next point
		CVec3Dfp32 PosNext = m_lCheckPoints[m_CurrentCheckpoint-1].m_Position;
		CVec3Dfp32 Current = m_lCheckPoints[m_CurrentCheckpoint].m_Position;
		fp32 Len = (PosNext - Current).Length();
		if (Len == 0.0f)
			Len = 0.05f;
		fp32 Speed = CREEPINGDARK_BACKTRACKSPEED;
		CMTime GameTime = m_pWServer->GetGameTime();
		fp32 Time = (GameTime - pCD->m_BackTrackStart).GetTime() + (_IPTime * m_pWServer->GetGameTickTime());
		fp32 Scaled = Time * (Speed / Len);
		//CMTime Scaled = Time.Scale(Speed / Len);

		if (Scaled > 1.0f)//Scaled.Compare(1.0f) > 0)
		{
			// Switch to next
			m_CurrentCheckpoint = m_CurrentCheckpoint - 1;
			// TEMP
			pCD->m_BackTrackStart = GameTime - CMTime::CreateFromSeconds((Scaled - 1.0f) * (Len/Speed));
			//pCD->m_BackTrackStart = GameTime - (Scaled - CMTime::CreateFromSeconds(1.0f)).Scale(Len/Speed);
			return false;
		}

		_Pos = LERP(Current,PosNext,/*Scaled.GetTime()*/Scaled);
	}
	else
	{
		if (!pCD->m_EndTick)
		{
			pCD->m_EndTick = m_pWServer->GetGameTick();
			pCD->m_bCopyEnd = true;
		}

		// Reached the end of the line, return last pos
		if (m_lCheckPoints.Len() > 0)
		{
			_Pos = m_lCheckPoints[0].m_Position;
			return true;
		}

		return false;
	}

	return true;
}

void CWObject_CreepingDark::AddCheckpoint(const CVec3Dfp32& _Normal, const uint8& _Flags)
{
	// Force update
	// Check so position isn't the same as previous, if so just skip...?
	//m_pWServer->Object_SetDirty(m_iObject,1);
	
	// Find orientation
	//m_lCheckPoints.Add(CCheckPoint(GetPosition(),CVec3Dfp32::GetMatrixRow(m_CurrentOrientation,2)));
//	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	
	// Normal is now negative gravity
	m_lCheckPoints.Add(CCheckPoint(GetPosition(),_Normal,_Flags));
	//M_TRACE("AddCheckpoint, n = %d\n", m_lCheckPoints.Len());
}

CWObject_CreepingDark::CCheckPoint* CWObject_CreepingDark::GetLastCheckPointType(const uint8& _Flags)
{
	int i = m_lCheckPoints.Len() - 1;
	CCheckPoint* pCheckPoints = m_lCheckPoints.GetBasePtr();
	for(; i >= 0; i--)
	{
		if(pCheckPoints[i].m_Flags & _Flags)
			return &pCheckPoints[i];
	}

	return NULL;
}

CWObject_CreepingDark::CCheckPoint* CWObject_CreepingDark::GetLastCheckPoint()
{
	return &m_lCheckPoints[m_lCheckPoints.Len()-1];
}

/*void CWObject_CreepingDark::DebugRenderCheckPoints(CWorld_PhysState* _pWPhys)
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	if (!pCD)
		return;

	CVec3Dfp32 Offset = (_pWPhys->IsClient() ? CVec3Dfp32(0,0,2) : 0);
	int32 Color = (_pWPhys->IsClient() ? 0xff00ff00 : 0xffffffff);
	int32 ColorUp = (_pWPhys->IsClient() ? 0xff0000ff : 0xff0000ff);

	int32 Len = m_lCheckPoints.Len();
	for (int32 i = 0; i < Len; i++)
	{
		_pWPhys->Debug_RenderVertex(m_lCheckPoints[i].m_Position + Offset, Color);
		if (i+1 < Len)
		{
			// Render line to next
			_pWPhys->Debug_RenderWire(m_lCheckPoints[i].m_Position + Offset, m_lCheckPoints[i+1].m_Position + Offset, Color);
		}

		// Render "up"
		CVec3Dfp32 Up = m_lCheckPoints[i].GetNormal();
		_pWPhys->Debug_RenderWire(m_lCheckPoints[i].m_Position + Offset, m_lCheckPoints[i].m_Position + Offset + Up * 10.0f, ColorUp);

	}
}*/

CWObject_CreepingDark::CCheckPoint::CCheckPoint(const CVec3Dfp32& _Position, CVec3Dfp32 _Normal, const uint8& _Flags)
	: m_Position(_Position)
	, m_Normal(_Normal)
	, m_Flags(_Flags)
{
}

CWObject_CreepingDark::CCheckPoint::CCheckPoint(const CVec3Dfp32& _Position, CVec2Dfp32 _Orientation, const uint8& _Flags)
	: m_Position(_Position)
	, m_Flags(_Flags)
{
	//m_Position = _Position;
	CVec3Dfp32 Angles(0.0f, _Orientation[0], _Orientation[1]);

	CMat4Dfp32 Mat;
	Angles.CreateMatrixFromAngles(0, Mat);
	m_Normal = CVec3Dfp32::GetRow(Mat, 0);
}

CVec2Dfp32 CWObject_CreepingDark::CCheckPoint::GetOrientation() const
{
	CVec3Dfp32 Orientation = CWObject_Character::GetLook(m_Normal);
	return (CVec2Dfp32(Orientation[1],Orientation[2]));
}

CVec3Dfp32 CWObject_CreepingDark::CCheckPoint::GetNormal() const
{
	return m_Normal;
}

int CWObject_CreepingDark::CCreepingDarkClientData::OnCreateClientUpdate(uint8* _pData)
{
	// This is only done when something is changed anyway so just save the info
	uint8* pD = _pData;

	uint8 &Flags = pD[0];
	Flags = 0;
	pD++;

	PTR_PUTUINT8(pD,m_MoveFlags);

	if (m_bBackTrackReachedEnd)
		Flags |= CREEPINGDARK_COPYFLAGS_REACHEDEND;

	if (m_bRenderHead)
		Flags |= CREEPINGDARK_COPYFLAGS_RENDERHEAD;
	
	if (m_bLockedRot)
		Flags |= CREEPINGDARK_COPYFLAGS_LOCKEDROT;

	if (m_bCopyFirst)
	{
		Flags |= CREEPINGDARK_COPYFLAGS_FIRSTCOPY | CREEPINGDARK_COPYFLAGS_COPYSTARTTICK;
		m_bCopyFirst = false;
		PTR_PUTINT32(pD, m_StartTick);
	}
	if (m_bCopyEnd)
	{
		Flags |= CREEPINGDARK_COPYFLAGS_COPYENDTICK;
		m_bCopyEnd = false;
		PTR_PUTINT32(pD, m_EndTick);
	}

	if (m_bCopyStartChar)
	{
		Flags |= CREEPINGDARK_COPYFLAGS_COPYSTARTCHAR;
		m_bCopyStartChar = false;
		PTR_PUTINT16(pD, m_iStartingCharacter);
	}

	if (m_bCopyGravity)
	{
		Flags |= CREEPINGDARK_COPYFLAGS_COPYGRAVITY;
		m_bCopyGravity = false;
		PTR_PUTFP32(pD, m_Gravity[0]);
		PTR_PUTFP32(pD, m_Gravity[1]);
		PTR_PUTFP32(pD, m_Gravity[2]);
	}

	return (pD - _pData);
}

int CWObject_CreepingDark::CCreepingDarkClientData::OnClientUpdate(const uint8* _pData)
{
	const uint8* pD = _pData;

	uint8 Flags;
	PTR_GETUINT8(pD,Flags);

	PTR_GETUINT8(pD,m_MoveFlags);

	m_bRenderHead = ((Flags & CREEPINGDARK_COPYFLAGS_RENDERHEAD) != 0);
	m_bBackTrackReachedEnd = ((Flags & CREEPINGDARK_COPYFLAGS_REACHEDEND) != 0);
	m_bLockedRot = ((Flags & CREEPINGDARK_COPYFLAGS_LOCKEDROT) != 0);

	m_bCopyFirst = (Flags & CREEPINGDARK_COPYFLAGS_FIRSTCOPY) != 0;

	if (Flags & CREEPINGDARK_COPYFLAGS_COPYSTARTTICK)
	{
		PTR_GETINT32(pD, m_StartTick);
	}
	if (Flags & CREEPINGDARK_COPYFLAGS_COPYENDTICK)
	{
		PTR_GETINT32(pD, m_EndTick);
	}
	if (Flags & CREEPINGDARK_COPYFLAGS_COPYSTARTCHAR)
	{
		Flags |= CREEPINGDARK_COPYFLAGS_COPYSTARTCHAR;
		PTR_GETINT16(pD, m_iStartingCharacter);
	}

	if (Flags & CREEPINGDARK_COPYFLAGS_COPYGRAVITY)
	{
		PTR_GETFP32(pD, m_Gravity[0]);
		PTR_GETFP32(pD, m_Gravity[1]);
		PTR_GETFP32(pD, m_Gravity[2]);
	}

	return (pD - _pData);

}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_CreepingDark, CWObject_CreepingDarkParent, 0x0100);



void CWObject_CreepingDark::CCreepingDarkClientData::NewProcessLook(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, const CVec3Dfp32& _dLook)
{
	const CMat4Dfp32& PosMat = _pObj->GetPositionMatrix();

	CMat4Dfp32 BaseMat = PosMat;
	BaseMat.GetRow(2) = -m_Gravity;
	BaseMat.RecreateMatrix(2, 0);

	CMat4Dfp32 dRot;
	CMat4Dfp32 Temp, NewPosMat;

	// Yaw
	fp32 CosAngle = PosMat.GetRow(0) * BaseMat.GetRow(0);
	CosAngle = Sign(CosAngle) * Clamp01(CosAngle); // Safety precaution
	fp32 Angle = M_ACos(CosAngle) * (1.0f / _PI2) * Sign(PosMat.GetRow(0) * BaseMat.GetRow(2));
	fp32 NewAngle = Min(CREEPINGDARK_MAX_CAMERATILT, Max(-CREEPINGDARK_MAX_CAMERATILT, Angle + _dLook.k[1]));
	fp32 dAngle = NewAngle - Angle;
	dRot.Unit();
	dRot.M_x_RotY(dAngle);
	dRot.Multiply(PosMat, Temp);		// Rotate around local Y-axis

	// Tilt
	CQuatfp32(m_Gravity, -_dLook.k[2]).SetMatrix(dRot);
	Temp.Multiply(dRot, NewPosMat);			// Rotate around gravity Z-axis

	#define IsValidFloat(v) (v > -_FP32_MAX && v < _FP32_MAX)
	#define IsValidVec(v) (IsValidFloat(v.k[0]) && IsValidFloat(v.k[1]) && IsValidFloat(v.k[2]))
	#define IsValidMat(m) (IsValidVec(m.GetRow(0)) && IsValidVec(m.GetRow(1)) && IsValidVec(m.GetRow(2)))
	M_ASSERT(IsValidMat(NewPosMat), "!");

	_pWPhys->Object_SetRotation(_pObj->m_iObject, NewPosMat);
}

void CWObject_CreepingDark::NewSetPhysics()
{
	CWO_PhysicsState Phys;
	/*	PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_BOX, 0, 
	CVec3Dfp32(_Width*0.5f, _Width*0.5f, LEDGETYPE_HEIGHT_MEDIUM*0.5f), 0, 1.0f));*/
	Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_SPHERE, -1, CREEPINGDARK_SPHERESIZE,0);
	// Skip intersection with physobject for now (so we can spawn inside players bb) 
	// (and always spawn)
	Phys.m_ObjectFlags = OBJECT_FLAGS_WORLDTELEPORT | OBJECT_FLAGS_CREEPING_DARK;// | OBJECT_FLAGS_PLAYER;// | OBJECT_FLAGS_PHYSOBJECT;
	Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PLAYERPHYSMODEL | OBJECT_FLAGS_PROJECTILE | OBJECT_FLAGS_ANIMPHYS;// | OBJECT_FLAGS_PHYSOBJECT;
	Phys.m_PhysFlags = OBJECT_PHYSFLAGS_SLIDEABLE | OBJECT_PHYSFLAGS_PHYSMOVEMENT |
		OBJECT_PHYSFLAGS_OFFSET;
	Phys.m_MediumFlags |= XW_MEDIUM_PLAYERSOLID | XW_MEDIUM_DYNAMICSSOLID;
	Phys.m_nPrim = 1;

	m_pWServer->Object_SetPhysics(m_iObject, Phys);
}

void CWObject_CreepingDark::CCreepingDarkClientData::SetGravity(const CVec3Dfp32& _NewGravity, CWorld_PhysState* _pWPhys, bool bFakeGrav, bool _bSnapToGround)
{
	if((!m_bCanClimb || (m_bCanClimb && m_bNoClimbBlock)) && !_bSnapToGround)
		return;

	fp32 Len2 = _NewGravity.LengthSqr();
	bool bOk = Len2 > 0.99f && Len2 < 1.01f;
	//	M_ASSERT(bOk, "Invalid Gravity vector!");
	if (!bOk) return;

	CVec3Dfp32 Down(0.0f, 0.0f, -1.0f);
	if(!Down.AlmostEqual(_NewGravity, 0.1f))
	{	//New gravity is not down, it may not change unreasonable much, ie 180 deegres
		if(_NewGravity * m_Gravity < -0.7f)
			return;
	}

	if (bFakeGrav)
		m_MoveFlags |= CREEPINGDARK_MOVEFLAGS_FAKEGRAV;
	else
		m_MoveFlags &= ~CREEPINGDARK_MOVEFLAGS_FAKEGRAV;

	//ConOutL(CStrF("Setting Gravity: %s",_NewGravity.GetString().Str()));

	if(m_Gravity.AlmostEqual(_NewGravity, 0.01f))
		return;

	m_LastGravityTick = _pWPhys->GetGameTick();

	m_Gravity = _NewGravity;
	m_bCopyGravity = true;
}


void CWObject_CreepingDark::Jump(void)
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(this);
	if(pCD->m_bJumping || pCD->m_Phys_nInAirFrames || !pCD->m_bOnGround || pCD->m_bInWater)
		return;

	pCD->m_bOnGround = false;
	pCD->m_bJumping = true;
	pCD->m_JumpStartTick = m_pWServer->GetGameTick();
	CVec3Dfp32 Vel = m_pWServer->Object_GetVelocity(m_iObject);
	Vel += pCD->m_Gravity * CREEPINGDARK_JUMP_SPEED;
	m_pWServer->Object_SetVelocity(m_iObject, Vel);
}

void CWObject_CreepingDark::CCreepingDarkClientData::NewPhys_OnImpact(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, const CCollisionInfo& _CInfo)
{
	//	M_TRACE("[CWObject_CharDarkling, %s] CWO_PHYSEVENT_IMPACT, new gravity=[%.1f, %.1f, %.1f]\n", 
	//		_pPhysState->IsServer() ? "server" : "client", CD.m_Gravity.m_Value.k[0], CD.m_Gravity.m_Value.k[1], CD.m_Gravity.m_Value.k[2]);
	// Don't hit character we started from...
	if(!m_bCanClimb)
	{
		if(_pWPhys->IsServer() && _CInfo.m_bIsValid)
		{
			CWorld_Server *pServer = (CWorld_Server *)_pWPhys;
			CWObject* pObj = pServer->Object_Get(_CInfo.m_iObject);
			pServer->Message_SendToObject(CWObject_Message(OBJMSG_RPG_AVAILABLEPICKUP), _CInfo.m_iObject);
			if(pObj->GetMass() < 50.0f && !pObj->IsClass(class_SwingDoor))
			{
				CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
				CVec3Dfp32 ForceDir = pServer->Object_GetPosition(_CInfo.m_iObject) - pServer->Object_GetPosition(_pObj->m_iObject);
				ForceDir += pServer->Object_GetPositionMatrix(_pObj->m_iObject).GetRow(2);
				ForceDir.Normalize();
				pServer->Phys_AddMassInvariantImpulse(_CInfo.m_iObject, pServer->Object_GetPosition(_CInfo.m_iObject), ForceDir * 0.5f);
				pCD->m_MoveSpeed = Max(pCD->m_MoveSpeed, -0.5f);
			}
		}
		if(!m_bJumping)
			return;
		else
		{
			CVec3Dfp32 Down(0.0f, 0.0f, -1.0f);
			SetGravity(Down, _pWPhys, false, true);
		}
	}

	int32 TickDiff = _pWPhys->GetGameTick() - m_LastContactTick;
	if ((TickDiff < (_pWPhys->GetGameTicksPerSecond() / 2)) || !_CInfo.m_bIsValid)
		return;

	// Make sure it's not a character or object we're colliding with
	CWObject_CoreData* pCollideObj = _pWPhys->Object_GetCD(_CInfo.m_iObject);
	if (!pCollideObj || (pCollideObj->GetPhysState().m_ObjectFlags & (OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_OBJECT)))
		return;

	CVec3Dfp32 NewGravity = -_CInfo.m_Plane.n;
	// If it's an impact currently below us, don't care??
	//ConOutL(CStrF("%d IMPACT: %d!!!!  %s",_CInfo.m_bIsValid,_pWPhys->GetGameTick(),NewGravity.GetString().Str()));

	fp32 Proj = (NewGravity * m_Gravity);
	if (Proj > 0.99f || Proj < -0.7f)
		return;

	if (Proj < 0.95f)
	{
		m_LookSinceGravChange = 0.0f;
	}

	// Make sure point of impact and our position is in line with impact normal....
	CVec3Dfp32 ImpactDir = _pObj->GetPosition() - _CInfo.m_Pos;
	ImpactDir.Normalize();
	if (ImpactDir * _CInfo.m_Plane.n < 0.7)
	{
		//ConOut(CStrF("Invalid impactdir: %f:%s (Norm: %s)",ImpactDir * _CInfo.m_Plane.n,ImpactDir.GetString().Str(),_CInfo.m_Plane.n.GetString().Str()));
		return;
	}

	CMat4Dfp32 PosMat;// = _pObj->GetPositionMatrix();
	GetOrientation(_pWPhys, _pObj, PosMat);
	// If the impact normal is too close to our right vector, skip it 
	// (should fix getting rotated on walls in tight places)
	if (M_Fabs(_CInfo.m_Plane.n * CVec3Dfp32::GetMatrixRow(PosMat,1)) > 0.75f)
		return;

	// Change speed and stuff here as well..?
	const CVec3Dfp32& MoveVelocity = _pObj->GetMoveVelocity();
	fp32 VelLen = MoveVelocity.Length();
	CMat4Dfp32 VelMat;
	VelMat.Unit();
	(-NewGravity).SetRow(VelMat,2);
	PosMat.GetRow(1).SetRow(VelMat,1);
	VelMat.RecreateMatrix(2,1);
	CVec3Dfp32 NewVel = VelMat.GetRow(0) * VelLen;
	_pWPhys->Object_SetVelocity(_pObj->m_iObject,NewVel);
	m_LastContactTick = _pWPhys->GetGameTick();

	//ConOut(CStrF("%d Setting impact gravity: %s",_pWPhys->GetGameTick(),NewGravity.GetString().Str()));
	if(!m_bJumping)
		SetGravity(NewGravity, _pWPhys);
	else
		SetGravity(NewGravity, _pWPhys, false, true);

	if (!NewGravity.AlmostEqual(m_Gravity, 0.001f))
		m_MoveFlags = m_MoveFlags | CREEPINGDARK_MOVEFLAGS_AUTOPULLUP;

	m_bJumping = false;
}
/*
static fp32 GetAngle(const CVec3Dfp32& _Up, const CVec3Dfp32& _Vec, const CVec3Dfp32& _Ref)
{
	CVec3Dfp32 IdealVec;
	_Up.CrossProd(_Ref, IdealVec);
	fp32 u = _Vec * IdealVec;
	fp32 v = _Vec * _Up;
	fp32 Angle = atan2f(v, u) * (1.0f / _PI2);
	return Angle;
}
*/
#ifndef M_RTM
void RenderSphere(CWorld_PhysState* _pWPhys, const CVec3Dfp32& _Pos, fp32 _Size, fp32 _Duration, int32 _Color)
{
	int32 NumSegs = 15;
	for (int32 i = 0; i < NumSegs; i++)
	{
		CVec3Dfp32 Pos1,Pos2;
		fp32 X1 = M_Cos(((fp32)i / (fp32)NumSegs) * _PI * 2.0f) * _Size;
		fp32 Y1 = M_Sin(((fp32)i / (fp32)NumSegs) * _PI * 2.0f) * _Size;
		fp32 X2 = M_Cos(((fp32)(i+1) / (fp32)NumSegs) * _PI * 2.0f) * _Size;
		fp32 Y2 = M_Sin(((fp32)(i+1) / (fp32)NumSegs) * _PI * 2.0f) * _Size;

		_pWPhys->Debug_RenderWire(_Pos + CVec3Dfp32(X1,Y1,0), _Pos + CVec3Dfp32(X2,Y2,0),_Color,_Duration,false);
		_pWPhys->Debug_RenderWire(_Pos + CVec3Dfp32(0,Y1,X1), _Pos + CVec3Dfp32(0,Y2,X2),_Color,_Duration,false);
		_pWPhys->Debug_RenderWire(_Pos + CVec3Dfp32(X1,0,Y1), _Pos + CVec3Dfp32(X2,0,Y2),_Color,_Duration,false);
	}
}
#endif
extern fp32 GetPlanarRotation(const CVec3Dfp32& _From, const CVec3Dfp32& _To, const CVec3Dfp32& _Axis);
void CWObject_CreepingDark::CCreepingDarkClientData::Phys_Move(CWorld_PhysState* _pWPhysState, CWObject_CoreData* _pCreep, fp32 _dTime)
{
	//--- This seems pretty stupid, but the normal characters does it, so... 
	//disabled step-up--- m_pObj->m_PhysAttrib.m_StepSize = PLAYER_STEPSIZE; 
	//m_pObj->m_PhysAttrib.m_StepSize = 0.0f; 

	//ConOut(CStrF("NumSlices: %d MoveSpeed: %f",NumTimeSteps,m_MoveSpeed));

	// Move around a bit
	_pWPhysState->Object_MovePhysical((CSelection *) NULL, _pCreep->m_iObject, _dTime, NULL);

	if (!m_bOnGround && (m_MoveFlags & CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD))
		SnapToGround(_pWPhysState,_pCreep, _dTime);

	//ConOut(CStrF("Gravity: %s", m_Gravity.GetString().Str()));
	CMat4Dfp32 PosMat,LookMat;
	GetOrientation(_pWPhysState, _pCreep,PosMat,false);
	GetOrientation(_pWPhysState, _pCreep,LookMat);
//	const CVec3Dfp32& Up = PosMat.GetRow(2);
	const CVec3Dfp32& LookUp = LookMat.GetRow(2);
	const CVec3Dfp32& Gravity = m_Gravity;//m_Gravity_Estimated;
	CMat4Dfp32 RotMat;


	// Have we been in air for too long?
	if (m_Phys_nInAirFrames > _pWPhysState->GetGameTicksPerSecond())
	{
		// Time's up - turn normal gravity back on!
		CVec3Dfp32 DefaultGrav(0,0,-1);
		if (!DefaultGrav.AlmostEqual(m_Gravity, 0.01f))
		{
			SetGravity(DefaultGrav, _pWPhysState, false, true);
			m_MoveFlags &= ~CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD;
			CREEPDBG(CStrF("Gravity reset! %s", ((CVec3Dfp32)m_Gravity).GetString().Str()));
		}
	}

	bool bFromPhysMove = false;
	if (bFromPhysMove)
	{
		fp32 RotScale = 5.0f * _dTime;
		if (m_MoveFlags & CREEPINGDARK_MOVEFLAGS_AUTOPULLUP)
		{
			// Do both auto-roll and auto-yaw to match current gravity vector
			CVec3Dfp32 bisec = (LookUp - Gravity).Normalize();
			fp32 CosHalfAngle = LookUp * bisec;
			if (CosHalfAngle > 0.01f)
			{
                CVec3Dfp32 Axis; LookUp.CrossProd(-Gravity, Axis);
				fp32 Angle = -M_ACos(CosHalfAngle) * 2.0f * (0.5f/_PI) * 0.1f;
				if (M_Fabs(Angle) > 0.01f)
				{
					//				DBG_OUT("Auto-pullup, %.2f\n", Angle);
					Angle = Sign(Angle) * Min(0.1f, M_Fabs(Angle)) * RotScale;
					CMat4Dfp32 RotMat, NewMat;
					CQuatfp32(Axis, Angle).SetMatrix(RotMat);
					LookMat.Multiply(RotMat, NewMat);
					_pWPhysState->Object_SetRotation(_pCreep->m_iObject, NewMat);
					m_MoveFlags |= CREEPINGDARK_MOVEFLAGS_IS_ROTATING;
				}
				else
				{
					m_MoveFlags &= ~CREEPINGDARK_MOVEFLAGS_AUTOPULLUP;
					m_MoveFlags &= ~CREEPINGDARK_MOVEFLAGS_IS_ROTATING;
				}
			}
		}
		else
		{
			// Auto-roll to match current gravity vector
			const CVec3Dfp32& Axis = LookMat.GetRow(0);
			//			fp32 Angle = GetAngle(-Gravity, PosMat.GetRow(1), PosMat.GetRow(0));
			fp32 Angle = -GetPlanarRotation(LookMat.GetRow(2), -Gravity, Axis);
			if (M_Fabs(Angle) > 0.01f)
			{
				//			DBG_OUT("Auto-roll, %.2f\n", Angle);
				Angle = Sign(Angle) * Min(0.05f, M_Fabs(Angle)) * RotScale;
				CMat4Dfp32 RotMat, NewMat;
				CQuatfp32(Axis, Angle).SetMatrix(RotMat);
				LookMat.Multiply(RotMat, NewMat);
				_pWPhysState->Object_SetRotation(_pCreep->m_iObject, NewMat);
				UpdateOrientation(_pWPhysState,_pCreep,NewMat);
				m_MoveFlags |= CREEPINGDARK_MOVEFLAGS_IS_ROTATING;
			}
			else
			{
				m_MoveFlags &= ~CREEPINGDARK_MOVEFLAGS_IS_ROTATING;
			}
		}

		{ // Pull up "up vector"
			//CVec3Dfp32 bisec = (Up + m_UpVector.m_Value).Normalize();
			//fp32 CosHalfAngle = Up * bisec;
			const CVec3Dfp32& UpVec = LookUp;
			CVec3Dfp32 bisec = UpVec - Gravity;
			fp32 Len = bisec.Length();
			if (Len < 0.01f)
				bisec = LookMat.GetRow(1);
			else
				bisec *= (1.0f / Len);
			fp32 CosHalfAngle = UpVec * bisec;
			//if (CosHalfAngle > 0.01f)
			{
				fp32 Speed = (CosHalfAngle < 0.4f) ? 0.1f : 0.05f;
				CVec3Dfp32 Axis; UpVec.CrossProd(bisec, Axis);
				fp32 Angle = -M_ACos(CosHalfAngle) * 2.0f * (0.5f/_PI);
				if (M_Fabs(Angle) > 0.01f)
				{
					//				DBG_OUT("Pull-up, %.2f\n", Angle);
						Angle = Sign(Angle) * Min(Speed, M_Fabs(Angle)) * RotScale;
					CMat4Dfp32 RotMat;
					CQuatfp32(Axis, Angle).SetMatrix(RotMat);
					LookMat.GetRow(2) = LookUp * RotMat;
					LookMat.RecreateMatrix(2,0);
					UpdateOrientation(_pWPhysState,_pCreep,LookMat);
				}
			}
		}
	}
}

// Remake to be more like darkling (that works quite well...)
void CWObject_CreepingDark::CCreepingDarkClientData::GetAcc(CWorld_PhysState* _pWPhysState, CWObject_CoreData* _pCreep, CMat4Dfp32& _AccMat)
{
	_AccMat.Unit();
	CMat4Dfp32 PosMat,MoveMat;
	GetOrientation(_pWPhysState,_pCreep,PosMat);
	GetOrientation(_pWPhysState,_pCreep,MoveMat,false);
	const CVec3Dfp32& MoveVelocity = _pCreep->GetMoveVelocity();
//	const CVec3Dfp32& Pos = PosMat.GetRow(3);
	const CVec3Dfp32& Gravity = m_Gravity;
	CWO_PhysicsState Phys(_pCreep->GetPhysState());

	// Get movement vector from player input
	CVec3Dfp32 Movement(0.0f);
	bool bJustStarted = !(Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSOBJECT);
	
	// Move forward until we have physics
	if (bJustStarted)
		Movement = MoveMat.GetRow(0) * CREEPINGDARK_SPEED;
	else
		Movement = MoveMat.GetRow(0) * (m_MoveSpeed * CREEPINGDARK_SPEED);

	//m_CurrMovement = Movement;

	bool bOnGround = CheckOnGround(_pWPhysState,_pCreep);

	CVec3Dfp32 dV(0);
	if (bJustStarted)
	{
		// If we just started move down/forward until we hit ground
//		fp32 PrevGrav = MoveVelocity * Gravity;

		//Velocity += Gravity * (PrevGrav + 9.82f * _pWPhysState->GetGameTickTime());
		Movement +=  Gravity * (5.0f + MoveVelocity * Gravity);
		_pWPhysState->Object_SetVelocity(_pCreep->m_iObject,Movement);
		//ConOut(CStrF("Vel: %s GravSpeed: %f",Movement.GetString().Str(),Movement * Gravity));
	}
	else if (bOnGround)
	{
		{
			// Add regular movement
			//dV += Movement;

			//fp32 k = (MoveVelocity.LengthSqr() > Sqr(10.0f)) ? 0.01f : 0.7f;
			//dV.Lerp(MoveVelocity, k, dV);

			//	if (m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD && dV.LengthSqr() > Sqr(2.0f))
			//		dV *= 0.7f; // Slow down!


			_AccMat.GetRow(3) = (Movement - MoveVelocity) * (10.0f * m_SoftBlock);

			//_pWPhysState->Object_SetVelocity(_pCreep->m_iObject,Movement);


#if DBG_RENDER
			_pWPhysState->Debug_RenderVector(Pos, MoveVelocity, 0xff0000ff, 1.0f, false);
			_pWPhysState->Debug_RenderVector(Pos, dV, 0xffff0000, 1.0f, false);
			_pWPhysState->Debug_RenderSphere(PosMat, CREEPINGDARK_SPHERESIZE, 0xff0000ff, 0.0f, false);

			CMat4Dfp32 Tmp = PosMat;
			Tmp.GetRow(3) += dV;
			_pWPhysState->Debug_RenderSphere(Tmp, CREEPINGDARK_SPHERESIZE, 0xffff0000, 0.0f, false);
#endif
		}
	}
	else
	{
		// In air
		/*	if (m_Phys_nInAirFrames >= 1 && (m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD))
		{
		_pMat->GetRow(3) = -MoveVelocity + Gravity;
		SetGravity(-(CVec3Dfp32)m_SteepSlopeNormal);
		m_Flags = m_Flags & ~DARKLING_FLAGS_STEEP_SLOPE_AHEAD;
		//M_TRACE("Changing grav due to steep slope, new grav = %s\n", ((CVec3Dfp32)m_Gravity).GetString().Str());
		}
		else*/

#ifdef CREEPDBG_RENDER
		_pWPhysState->Debug_RenderSphere(PosMat, CREEPINGDARK_SPHERESIZE, 0xffff0000, 0.0f, false);
		_pWPhysState->Debug_RenderVector(Pos, MoveVelocity, 0xff0000ff, 3.0f, false);
#endif

		if (!(m_MoveFlags & CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD))
		{
			if(!m_bJumping)
			{
				// Don't move too fast
				if (Gravity * CVec3Dfp32(0.0f,0.0f,-1.0f) > 0.99f || MoveVelocity * Gravity < 10.0f)
					_AccMat.GetRow(3) = Gravity * CREEPINGDARK_SPEED;
				if(m_Phys_nInAirFrames > 1 && m_Phys_nInAirFrames < 20)
				{
					_AccMat.GetRow(3) *= 2.0f;
					_pWPhysState->Object_SetVelocity(_pCreep->m_iObject,_AccMat.GetRow(3));
				}
			}
			else
			{
				if(m_Phys_nInAirFrames > 1)
				{
					CVec3Dfp32 MoveVel = _pWPhysState->Object_GetVelocity(_pCreep->m_iObject);
					MoveVel += CVec3Dfp32(0.0, 0.0f, -1.0f);
					_pWPhysState->Object_SetVelocity(_pCreep->m_iObject, MoveVel);
				}
			}
		}
	}
}

bool CWObject_CreepingDark::CCreepingDarkClientData::CheckOnGround(CWorld_PhysState* _pWPhysState, CWObject_CoreData* _pCreep)
{
	if(m_JumpStartTick + 1 >= _pWPhysState->GetGameTick())
		return false;
	// Check if we're on the ground
	m_bOnGround = false;
	bool bSteepSlopeAhead = false;

	// Getorientation
	const CVec3Dfp32& Velocity = _pCreep->GetMoveVelocity();
	CMat4Dfp32 PosMat;
	GetOrientation(_pWPhysState,_pCreep,PosMat,false);
	const CMat4Dfp32 LookMat = PosMat;
	const CVec3Dfp32& Gravity = m_Gravity;

	CSelection* pSelection = NULL;// m_iPhysSelection;

	// Check if we're standing on ground
	CWO_PhysicsState PhysState(_pCreep->GetPhysState());
	CMat4Dfp32 p0 = PosMat;
	CMat4Dfp32 p1 = PosMat;
	p1.GetRow(3) += Gravity;
	CCollisionInfo CInfo_1;
	m_bOnGround = _pWPhysState->Phys_IntersectWorld(pSelection, PhysState, p0, p1, _pCreep->m_iObject, &CInfo_1);
	bool bForceNoclimb = false;
	if(CInfo_1.m_bIsValid && CInfo_1.m_iObject)
	{
		CWObject_CoreData *pObj = _pWPhysState->Object_GetCD(CInfo_1.m_iObject);
		if(pObj && !pObj->IsClass(class_SwingDoor))
		{
			CWO_PhysicsState PState = pObj->GetPhysState();
			if(PState.m_ObjectFlags & OBJECT_FLAGS_OBJECT)
				bForceNoclimb = true;
		}
	}
	if (!CInfo_1.m_bIsValid)
		m_bOnGround = false;
	else if(!bForceNoclimb)
	{
		if(m_bCanClimb)
			SetGravity(-CInfo_1.m_Plane.n, _pWPhysState, false, true);
		else
		{	//This code will make it harder to "slide" up on walls and we should still be able to crawl down slopes
			CVec3Dfp32 Right = PosMat.GetRow(1);
			fp32 dot = Right * -CInfo_1.m_Plane.n;	
			if(dot < 0.5f && dot > - 0.5f)	
				SetGravity(-CInfo_1.m_Plane.n, _pWPhysState, false, true);
		}
	}

	// Check if there is ground ahead of us
	if (m_bOnGround && /*m_Control_Move.LengthSqr() > 0.1f &&*/ m_MoveMode == CREEPINGDARK_MOVEMODE_CREEPING)
	{
		m_bJumping = false;
		CVec3Dfp32 MoveDir = Velocity;
		MoveDir.SetLength(CREEPINGDARK_SPHERESIZE * 2.0f);

		//MoveDir.Normalize();

		p0.GetRow(3) = LookMat.GetRow(3) + MoveDir;// + (MoveDir * 32.0f);
		p1.GetRow(3) = p0.GetRow(3) + (Gravity * CREEPINGDARK_SPHERESIZE * 0.5f);
#ifndef M_RTM
		//m_pWPhysState->Debug_RenderVector(p0.GetRow(3), p1.GetRow(3) - p0.GetRow(3), 0xF0FF0000, 0.0f, false);
		//m_pWPhysState->Debug_RenderOBB(p0, 16.0f, 0xF0FFFF00, 0.0f, false);
		_pWPhysState->Debug_RenderSphere(p1, CREEPINGDARK_SPHERESIZE, 0xF0FF00FF, 0.0f, false);
#endif
		CInfo_1.Clear();
		if (!_pWPhysState->Phys_IntersectWorld(pSelection, PhysState, p0, p1, _pCreep->m_iObject, &CInfo_1))
		{
			bSteepSlopeAhead = true;

			if (!(m_MoveFlags & CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD))
			{
				CREEPDBG("Convex Corner detected!");
				CREEPDBG(CStrF(" - speed is %s\n", _pCreep->GetMoveVelocity().GetString().Str()));
				m_MoveFlags |= CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD;

				// Find new ground plane
				m_Gravity_Estimated = Gravity;
				m_CornerRef = PosMat.GetRow(3) + PosMat.GetRow(2) * (-CREEPINGDARK_SPHERESIZE);
				p0.GetRow(3) = LookMat.GetRow(3) + MoveDir;
				p1.GetRow(3) = p0.GetRow(3) + (Gravity * CREEPINGDARK_SPHERESIZE * 2.0f);
				CInfo_1.Clear();
				bool bNewGround = _pWPhysState->Phys_IntersectWorld(pSelection, PhysState, p0, p1, _pCreep->m_iObject, &CInfo_1);
				if (bNewGround)
				{
					m_Gravity_Estimated = -CInfo_1.m_Plane.n;
				}
				else
				{
					p0.GetRow(3) = p1.GetRow(3);
					p1.GetRow(3) -= MoveDir * 0.5f;
					bNewGround = _pWPhysState->Phys_IntersectWorld(pSelection, PhysState, p0, p1, _pCreep->m_iObject, &CInfo_1);
					if (bNewGround)
					{
						m_Gravity_Estimated = -CInfo_1.m_Plane.n;
					}
					else
					{
						CREEPDBG("Could not find new gravity plane... gaaaah!");
					}
				}
				if (bNewGround)
				{
					CREEPDBG(CStrF(" - next gravity is %s\n", CVec3Dfp32(m_Gravity_Estimated).GetString().Str()));

					m_CornerRef = ((CVec3Dfp32)m_CornerRef + CInfo_1.m_Pos) * 0.5f;
					CREEPDBG(CStrF(" - corner pos is %s\n", CVec3Dfp32(m_CornerRef).GetString().Str()));

					_pWPhysState->Debug_RenderVertex(m_CornerRef, 0xff0000ff, 3.0f);
					_pWPhysState->Debug_RenderVector(m_CornerRef, (CVec3Dfp32(m_Gravity) + CVec3Dfp32(m_Gravity_Estimated))*-16.0f, 0xff0000ff, 3.0f);
				}
			}
		}
	}

	if (m_bOnGround)
	{
		if (m_Phys_nInAirFrames)
		{
			//DBG_OUT("Back on ground\n");

			/*if (m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD)
			{
			SetGravity(m_Gravity_Estimated);
			m_Flags = m_Flags & ~DARKLING_FLAGS_STEEP_SLOPE_AHEAD;
			}*/

			//	if (!bSteepSlopeAhead)
			//		m_Flags = m_Flags & ~DARKLING_FLAGS_STEEP_SLOPE_AHEAD;	// was in air, but no more..
		}
		m_Phys_nInAirFrames = 0;
	}

	if (m_bOnGround && !bSteepSlopeAhead)
	{
		// Update gravity if any difference
		/*fp32 Proj = CInfo_1.m_Plane.n * -Gravity;
		if (Proj < 0.999f && Proj > 0.5f)
			SetGravity(CInfo_1.m_Plane.n);*/
		if (m_MoveFlags & CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD)
		{
			//ConOut("Clearing Steep is clear!\n");
			//	SetGravity(m_Gravity_Estimated);
			m_MoveFlags &= ~CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD;
		}
	}

	return m_bOnGround;
}

//extern fp32 GetPlanarRotation(const CVec3Dfp32& _From, const CVec3Dfp32& _To, const CVec3Dfp32& _Axis);

void CWObject_CreepingDark::CCreepingDarkClientData::SnapToGround(CWorld_PhysState* _pWPhysState, CWObject_CoreData* _pCreep, fp32 _dTime)
{
	if(m_bJumping)
		return;
	// Find a new orientation to be at
	CMat4Dfp32 PosMat;
	GetOrientation(_pWPhysState,_pCreep,PosMat);
	const CVec3Dfp32& MoveVelocity = _pCreep->GetMoveVelocity();
//	const CVec3Dfp32& Pos = PosMat.GetRow(3);
	const CVec3Dfp32& Gravity = m_Gravity;

	CVec3Dfp32 ToGround = m_CornerRef;
	ToGround -= PosMat.GetRow(3);
#ifdef CREEPDBG_RENDER
	_pWPhysState->Debug_RenderVector(Pos, ToGround, 0xff00ff00, 3.0f, false);
#endif
	CSelection *pSelection=NULL;

	// Find ground.
	fp32 DistToGround = ToGround.Length();
	CWO_PhysicsState PhysState(_pCreep->GetPhysState());
	CMat4Dfp32 p0 = PosMat;
	CMat4Dfp32 p1 = PosMat;
	p1.GetRow(3) += ToGround * (DistToGround - (CREEPINGDARK_SPHERESIZE - 1.0f))/DistToGround;
	CCollisionInfo CollInfo;
	_pWPhysState->Debug_RenderSphere(p1, CREEPINGDARK_SPHERESIZE, 0x7f007f00, 3.0f, true);
	if (_pWPhysState->Phys_IntersectWorld(pSelection, PhysState, p0, p1, _pCreep->m_iObject, &CollInfo) && CollInfo.m_bIsValid)
	{
		CWObject_CoreData *pObj = _pWPhysState->Object_GetCD(CollInfo.m_iObject);
		if(pObj)
		{
			CWO_PhysicsState PhysState = pObj->GetPhysState();
			if(PhysState.m_ObjectFlags & OBJECT_FLAGS_OBJECT)
				return;
		}
		m_CornerRef = CollInfo.m_Pos;

		ToGround = m_CornerRef;
		ToGround -= PosMat.GetRow(3);
		ToGround.Normalize();

		CVec3Dfp32 NewPos = (CVec3Dfp32)m_CornerRef + (ToGround * -(CREEPINGDARK_SPHERESIZE + 0.5f));
		_pWPhysState->Object_SetPosition(_pCreep->m_iObject, NewPos);

#ifdef CREEPDBG_RENDER
		CMat4Dfp32 Tmp = PosMat; Tmp.GetRow(3) = NewPos;
		_pWPhysState->Debug_RenderSphere(Tmp, CREEPINGDARK_SPHERESIZE, 0x7f007f00, 3.0f, true);
		_pWPhysState->Debug_RenderSphere(PosMat, CREEPINGDARK_SPHERESIZE, 0x7f00007f, 3.0f, true);
#endif

		CMat4Dfp32 RotMat;
		CVec3Dfp32 Axis, Dir = MoveVelocity;
		Gravity.CrossProd(Dir, Axis);
		Axis.Normalize();
		if(Axis.LengthSqr())
		{
			fp32 Angle = (-1.1f) * GetPlanarRotation(m_Gravity, ToGround, Axis);
			if (Angle < 0.0f)
				Angle += 1.0f;
			CQuatfp32(Axis, Angle).SetMatrix(RotMat);

			CVec3Dfp32 NewVel = MoveVelocity;
			NewVel.MultiplyMatrix3x3(RotMat);
			_pWPhysState->Object_SetVelocity(_pCreep->m_iObject, NewVel);
		}

#ifdef CREEPDBG_RENDER
		_pWPhysState->Debug_RenderVector(Pos, MoveVelocity, 0xff00ff00, 3.0f, false);
		_pWPhysState->Debug_RenderVector(Pos, NewVel, 0xffff0000, 3.0f, false);
#endif

		m_MoveFlags |= CREEPINGDARK_MOVEFLAGS_AUTOPULLUP;
		SetGravity(ToGround, _pWPhysState, false, true);
		//UpdateMatrices();
		//UpdateOrientation(_pWPhysState,_pCreep,_dTime);
	}
}

void CWObject_CreepingDark::CCreepingDarkClientData::NewGetAcc(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CSelection& _Selection, const CVec3Dfp32& _ControlMove, CVec3Dfp32& _Acc)
{
	CMat4Dfp32 MatLook;// = _pObj->GetPositionMatrix();
	GetOrientation(_pWPhys, _pObj, MatLook,false);
	const CVec3Dfp32& MoveVelocity = _pObj->GetMoveVelocity();
//	const CVec3Dfp32& Pos = MatLook.GetRow(3);
	const CVec3Dfp32& Gravity = m_Gravity;

	CVec3Dfp32 Movement;//(0.0f);
	CWO_PhysicsState Phys(_pObj->GetPhysState());

	// Move forward until we have physics
	if (!(Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSOBJECT))
		Movement = MatLook.GetRow(0) * CREEPINGDARK_SPEED;
	else
		Movement = MatLook.GetRow(0) * (m_MoveSpeed * CREEPINGDARK_SPEED);
	//Movement += MatLook.GetRow(1) * (_ControlMove.k[1] * (fp32)m_Speed_SideStep);
	//Movement += MatLook.GetRow(2) * (Move.k[2] * (fp32)m_Speed_Up);

	bool bSteepSlopeAhead;

	// Check if we're standing on ground
	CMat4Dfp32 p0 = MatLook, p1 = MatLook;
	p1.GetRow(3) += Gravity;
	CCollisionInfo CInfo_1;
	//_pWPhys->Phys_IntersectLine(CVec3Dfp32::GetMatrixRow(p0,3),CVec3Dfp32::GetMatrixRow(p1,3),Phys.m_ObjectFlags,Phys.m_ObjectIntersectFlags,Phys.m_MediumFlags,&CInfo_1,_pObj->m_iObject);
	//_pWPhys->Debug_RenderWire(CVec3Dfp32::GetMatrixRow(p0,3),CVec3Dfp32::GetMatrixRow(p1,3),0xff007f7f,20.0f,false);
	//RenderSphere(_pWPhys,MatLook.GetRow(3),CREEPINGDARK_SPHERESIZE,20.0f);
	m_bOnGround = _pWPhys->Phys_IntersectWorld(&_Selection, Phys, p0, p1, _pObj->m_iObject, &CInfo_1);
//	CWObject_CoreData* pObjColl = _pWPhys->Object_GetCD(CInfo_1.m_iObject);
	CVec3Dfp32 GroundNormal;
	if (m_bOnGround)
	{
		CVec3Dfp32 P2 = CVec3Dfp32::GetMatrixRow(MatLook,3);
		P2 += Gravity * (CREEPINGDARK_SPHERESIZE + 0.1f);
		_pWPhys->Phys_IntersectLine(CVec3Dfp32::GetMatrixRow(p0,3),P2,Phys.m_ObjectFlags,Phys.m_ObjectIntersectFlags,Phys.m_MediumFlags,&CInfo_1,_pObj->m_iObject);
	}

	if (!CInfo_1.m_bIsValid)
	{
		GroundNormal = -Gravity;
		m_bOnGround = CInfo_1.m_bIsCollision;
	}
	else
	{
		m_bOnGround = true;
		GroundNormal = CInfo_1.m_Plane.n;
	}


	// Check if there is ground ahead of us
	bSteepSlopeAhead = false;
	if (m_bOnGround && Movement.LengthSqr() > 0.1f)
	{
		CVec3Dfp32 MoveDir = Movement;
		MoveDir.Normalize();
		Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_SPHERE, -1, 2.0f,0);

		p0.GetRow(3) = MatLook.GetRow(3) + (MoveDir * 6.0f);
		p1.GetRow(3) = p0.GetRow(3) + (Gravity * 3.0f);
#ifndef M_RTM
		_pWPhys->Debug_RenderVector(p0.GetRow(3), p1.GetRow(3) - p0.GetRow(3), 0xF0FF0000,5.0f, false);
		RenderSphere(_pWPhys,p0.GetRow(3),2.0,5.0f,0xF0FFFF00);
		RenderSphere(_pWPhys,p1.GetRow(3),2.0,5.0f,0xF0FF00FF);
#endif
		CInfo_1.Clear();
		if (!_pWPhys->Phys_IntersectWorld(&_Selection, Phys, p0, p1, _pObj->m_iObject, &CInfo_1))
		{
			bSteepSlopeAhead = true;

			if (!(m_MoveFlags & CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD))
			{
				m_MoveFlags |= CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD;
				m_SteepSlopeNormal = (MoveDir - Gravity).Normalize();	// not necessarily true, but should work...
				//ConOut("STEEP SLOPE AHEAD");
				//M_TRACE("Steep slope ahead, new next grav = %s\n", ((CVec3Dfp32)m_SteepSlopeNormal).GetString().Str());
			}
		}

		if (!bSteepSlopeAhead && (m_MoveFlags & CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD))
			m_MoveFlags &= ~CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD;
	}

	CVec3Dfp32 dV(0);
	if (m_bOnGround || !(Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSOBJECT))
	{
		dV += Movement;
		fp32 k = (MoveVelocity.LengthSqr() > Sqr(10.0f)) ? 0.01f : 0.7f;
		dV.Lerp(MoveVelocity, k, dV);
		_Acc = dV - MoveVelocity;
		if (!(Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSOBJECT))
			Movement +=  Gravity * (5.0f + MoveVelocity * Gravity);

		_Acc = 0.0f;

		_pWPhys->Object_SetVelocity(_pObj->m_iObject,Movement);

		// If gravity doesn't fit ground normal fix it
		if ((Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSOBJECT) && 
			(_pWPhys->GetGameTick() % (int)(_pWPhys->GetGameTicksPerSecond()*0.25f)) == 0)
		{
			fp32 Proj = m_Gravity * GroundNormal;
			fp32 Diff = 1.0f + Proj;
			if (Diff > 0.005f && Diff < 0.25f)
			{
				//ConOut(CStrF("Setting Ground normal Grav: %s (Diff: %f)",(-GroundNormal).GetString().Str(),Diff));
				SetGravity(-GroundNormal, _pWPhys);
				/*if (Diff > 0.1f)
				{
					m_MoveFlags |= CREEPINGDARK_MOVEFLAGS_AUTOPULLUP;
				}*/
			}
		}

		/*CVec3Dfp32 v = _pObj->GetMoveVelocity();
		const fp32 vlenSqr = dV.LengthSqr();
		CVec3Dfp32 dV2(dV - v);

		// This is the acceleration formula we've allways had.
		// VRet += dV2.Normalize() * Min(dV2.Length() / 2.0f, Max(2.6f, vlen / 2.0f));
		const fp32 dV2LenSqr = dV2.LengthSqr();
		if (dV2LenSqr > Sqr(0.001f))
			_Acc = dV2.Normalize() * Min(M_Sqrt(dV2LenSqr) * (1.0f / 2.0f), Max(2.6f, M_Sqrt(vlenSqr) * (1.0f / 2.0f)));*/
	}
	else
	{
		// In air
		//if (m_Phys_nInAirFrames > 1)
		_Acc = Gravity * 10.0f / _pWPhys->GetGameTickTime();

		if ((_pWPhys->GetGameTick() - m_LastContactTick) > 0  &&
			(m_MoveFlags & CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD))
		{
			//m_Phys_nInAirFrames = 0;
			//_Acc = -MoveVelocity + Gravity;
			// Half current speed?
			CVec3Dfp32 GravCached = m_Gravity;
			/*m_MoveSpeed = 0.5f;
			CVec3Dfp32 MoveDir = MoveVelocity;
			fp32 VelLen = MoveVelocity.Length();
			MoveDir = MoveDir / VelLen;
			m_Phys_nInAirFrames = 0;
			SetGravity(-(CVec3Dfp32)m_SteepSlopeNormal,true);
			CVec3Dfp32 NewVel = m_Gravity + MoveDir;
			NewVel *= VelLen;
			_pWPhys->Object_SetVelocity(_pObj->m_iObject,NewVel);*/
			m_MoveFlags &= ~CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD;
			//m_MoveFlags |= CREEPINGDARK_MOVEFLAGS_AUTOPULLUP;
			//M_TRACE("Changing grav due to steep slope, new grav = %s\n", ((CVec3Dfp32)m_Gravity).GetString().Str());

			// Try to find a different surface below us, take startpoint at the edge of our physball
			// tracing downwards and a bit backwards
			CVec3Dfp32 TraceStart = MatLook.GetRow(3) + MatLook.GetRow(0) * CREEPINGDARK_SPHERESIZE*0.3f + GravCached * CREEPINGDARK_SPHERESIZE;
			CVec3Dfp32 TraceDir = GravCached - MatLook.GetRow(0);
			TraceDir.Normalize();
#ifndef M_RTM
			RenderSphere(_pWPhys,MatLook.GetRow(3),CREEPINGDARK_SPHERESIZE,30.0f);
			_pWPhys->Debug_RenderWire(TraceStart,TraceStart + MatLook.GetRow(0) * 10.0f,0xffff0000,30.0f,false);
			_pWPhys->Debug_RenderWire(TraceStart,TraceStart + GravCached * 10.0f,0xff0000ff,30.0f,false);
			//ConOut(CStrF("%d: Trying: %s Vel: %s",_pWPhys->GetGameTick(),GravCached.GetString().Str(),MoveVelocity.GetString().Str()));
			_pWPhys->Debug_RenderWire(TraceStart,TraceStart + TraceDir * 10.0f,0xff7f7f7f,30.0f,false);
#endif
			bool bHit = _pWPhys->Phys_IntersectLine(TraceStart, TraceStart + TraceDir * 10.0f,Phys.m_ObjectFlags,Phys.m_ObjectIntersectFlags,Phys.m_MediumFlags,&CInfo_1,_pObj->m_iObject);
			if (bHit && CInfo_1.m_bIsValid && CInfo_1.m_iObject != m_iStartingCharacter)
			{
				// Check so that normal is reasonably in line with current move dir
				//ConOut(CStrF("SomeVal: %f",CInfo_1.m_Plane.n * MatLook.GetRow(0)));
				if ((GravCached * -CInfo_1.m_Plane.n < 0.99f) && (CInfo_1.m_Plane.n * MatLook.GetRow(0)) > 0.5f)
				{
					SetGravity(-CInfo_1.m_Plane.n, _pWPhys);
					CMat4Dfp32 VelMat;
					VelMat.Unit();
					(-m_Gravity).SetRow(VelMat,2);
					MatLook.GetRow(1).SetRow(VelMat,1);
					VelMat.RecreateMatrix(2,1);
					CVec3Dfp32 NewVel = VelMat.GetRow(0) * MoveVelocity.Length() + MatLook.GetRow(0) * MoveVelocity.Length()*0.25f;
					//ConOut(CStrF("%d: Got contact with something(%d): %s (%s) NewVel: %s",_pWPhys->GetGameTick(),CInfo_1.m_iObject,(-CInfo_1.m_Plane.n).GetString().Str(),GravCached.GetString().Str(),NewVel.GetString().Str()));
					_pWPhys->Object_SetVelocity(_pObj->m_iObject,NewVel);
					m_LastContactTick = _pWPhys->GetGameTick();
					// Change velocity to something...?
					m_MoveFlags |= CREEPINGDARK_MOVEFLAGS_AUTOPULLUP;
					m_LookSinceGravChange = 0.0f;
					_Acc = m_Gravity * 10.0f / _pWPhys->GetGameTickTime();
					
					_pWPhys->Debug_RenderVertex(CInfo_1.m_Pos,0xffff0000,30.0f,false);
					_pWPhys->Debug_RenderVector(CInfo_1.m_Pos,CInfo_1.m_Plane.n ,0xff00ff00,30.0f,false);
				}
			}
			/*else
			{
				ConOut("OMG FAILED!!!");
			}*/
		}
		else if (m_Phys_nInAirFrames > 20)
		{
			//ConOut("Setting grav in air");
			SetGravity(CVec3Dfp32(0,0,-1), _pWPhys);
			m_MoveFlags &= ~CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD;
			m_MoveFlags |= CREEPINGDARK_MOVEFLAGS_AUTOPULLUP;
			m_LookSinceGravChange = 0.0f;
			//M_TRACE("Gravity reset! %s\n", ((CVec3Dfp32)m_Gravity).GetString().Str());
		}
	}
}

void CWObject_CreepingDark::CCreepingDarkClientData::GetOrientation(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CMat4Dfp32& _Mat, bool _bTilt)
{
	CWObject_CoreData* pChar = _pWPhys->Object_GetCD(m_iStartingCharacter);
	CWO_Character_ClientData* pCharCD = (pChar ? CWObject_Character::GetClientData(pChar) : NULL);
	M_ASSERTHANDLER(pCharCD, "INVALID CHARACTER INDEX", return);
	_Mat.Create(pCharCD->m_Creep_Orientation.m_Value,_pObj->GetPosition_vec128());
	if (!_bTilt)
	{
		CVec3Dfp32 Pos = _Mat.GetRow(3);
		_Mat.GetRow(2) = -m_Gravity;
		_Mat.RecreateMatrix(2, 1);
		_Mat.GetRow(3) = Pos;
	}
}

void CWObject_CreepingDark::CCreepingDarkClientData::NewPhys_Move(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CSelection& _Selection, fp32 _dTime, const CVec3Dfp32& _UserVel, bool _bPredicted)
{
	//_pWPhys->Object_SetVelocity(_pObj->m_iObject,_UserVel);
	//UpdateOrientation(_pWPhys,_pObj,_dTime);
	// Meh, divide it into couple parts, so we have better resolution when going around corners..

	//int32 NumTimeSteps = 2;
	int32 NumTimeSteps = Max(1, (int)(m_MoveSpeed / 0.10f));
	fp32 TimeSlice = _dTime / (fp32)NumTimeSteps;

	//ConOut(CStrF("NumSlices: %d MoveSpeed: %f",NumTimeSteps,m_MoveSpeed));

	m_bOnGround = true;
	for (int32 i = 0; i < NumTimeSteps; i++)
		_pWPhys->Object_MovePhysical((CSelection *) NULL, _pObj->m_iObject, TimeSlice, NULL);

	if (m_bOnGround)
		m_Phys_nInAirFrames = 0;
	else
		m_Phys_nInAirFrames++;
}

void CWObject_CreepingDark::UpdateOrientation(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, fp32 _dTime)
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	M_ASSERTHANDLER(pCD,"INVALID OBJECT", return);
	pCD->UpdateOrientation(_pWPhys,_pObj, _dTime);
}

void CWObject_CreepingDark::CCreepingDarkClientData::UpdateOrientation(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, fp32 _dTime)
{
	// Auto-roll to match current gravity vector
	CWObject_CoreData* pChar = _pWPhys->Object_GetCD(m_iStartingCharacter);
	CWO_Character_ClientData* pCharCD = (pChar ? CWObject_Character::GetClientData(pChar) : NULL);
	M_ASSERTHANDLER(pCharCD, "Invalid m_iStartingCharacter?", return);

	CMat4Dfp32 PosMat,NewMat;
	GetOrientation(_pWPhys,_pObj,PosMat);
	const CVec3Dfp32& Up = PosMat.GetRow(2);
	const CVec3Dfp32& Gravity = m_Gravity;
	pCharCD->m_Creep_PrevOrientation = pCharCD->m_Creep_Orientation;

	fp32 RotScale = _dTime*4.0f;
	if (m_MoveFlags & CREEPINGDARK_MOVEFLAGS_AUTOPULLUP)
	{
		// Do both auto-roll and auto-yaw to match current gravity vector
		CVec3Dfp32 bisec = (Up - Gravity).Normalize();
		fp32 CosHalfAngle = Up * bisec;
		if (CosHalfAngle > 0.01f)
		{
			CVec3Dfp32 Axis; Up.CrossProd(-Gravity, Axis);
			fp32 Angle = -M_ACos(CosHalfAngle) * 2.0f * (0.5f/_PI) * 0.1f;
			if (M_Fabs(Angle) > 0.01f)
			{
				//				DBG_OUT("Auto-pullup, %.2f\n", Angle);
				Angle = Sign(Angle) * Min(0.1f, M_Fabs(Angle)) * RotScale;
				CMat4Dfp32 RotMat;
				CQuatfp32(Axis, Angle).SetMatrix(RotMat);
				PosMat.Multiply(RotMat, NewMat);
				pCharCD->m_Creep_Orientation.m_Value.Create(NewMat);
				GetOrientation(_pWPhys,_pObj,PosMat);
				//_pWPhysState->Object_SetRotation(_pCreep->m_iObject, NewMat);
				m_MoveFlags |= CREEPINGDARK_MOVEFLAGS_IS_ROTATING;
			}
			else
			{
				m_MoveFlags &= ~CREEPINGDARK_MOVEFLAGS_AUTOPULLUP;
				m_MoveFlags &= ~CREEPINGDARK_MOVEFLAGS_IS_ROTATING;
			}
		}
	}
	else
	{
		// Auto-roll to match current gravity vector
		const CVec3Dfp32& Axis = PosMat.GetRow(0);
		//			fp32 Angle = GetAngle(-Gravity, PosMat.GetRow(1), PosMat.GetRow(0));
		fp32 Angle = -GetPlanarRotation(PosMat.GetRow(2), -Gravity, Axis);
		if (M_Fabs(Angle) > 0.001f)
		{
			//			DBG_OUT("Auto-roll, %.2f\n", Angle);
			Angle = Sign(Angle) * Min(0.05f, M_Fabs(Angle)) * RotScale;
			CMat4Dfp32 RotMat, NewMat;
			CQuatfp32(Axis, Angle).SetMatrix(RotMat);
			PosMat.Multiply(RotMat, NewMat);
			pCharCD->m_Creep_Orientation.m_Value.Create(NewMat);
			GetOrientation(_pWPhys,_pObj,PosMat);
			//_pWPhysState->Object_SetRotation(_pCreep->m_iObject, NewMat);
			//UpdateOrientation(_pWPhysState,_pCreep,NewMat);
			m_MoveFlags |= CREEPINGDARK_MOVEFLAGS_IS_ROTATING;
		}
		else
		{
			m_MoveFlags &= ~CREEPINGDARK_MOVEFLAGS_IS_ROTATING;
		}
	}

	{ // Pull up "up vector"
		//CVec3Dfp32 bisec = (Up + m_UpVector.m_Value).Normalize();
		//fp32 CosHalfAngle = Up * bisec;
		const CVec3Dfp32& UpVec = Up;
		CVec3Dfp32 bisec = UpVec - Gravity;
		fp32 Len = bisec.Length();
		if (Len < 0.01f)
			bisec = PosMat.GetRow(1);
		else
			bisec *= (1.0f / Len);
		fp32 CosHalfAngle = UpVec * bisec;
		//if (CosHalfAngle > 0.01f)
		{
			fp32 Speed = (CosHalfAngle < 0.4f) ? 0.1f : 0.05f;
			CVec3Dfp32 Axis; UpVec.CrossProd(bisec, Axis);
			fp32 Angle = -M_ACos(CosHalfAngle) * 2.0f * (0.5f/_PI);
			if (M_Fabs(Angle) > 0.01f)
			{
				//				DBG_OUT("Pull-up, %.2f\n", Angle);
				Angle = Sign(Angle) * Min(Speed, M_Fabs(Angle)) * RotScale;
				CMat4Dfp32 RotMat;
				CQuatfp32(Axis, Angle).SetMatrix(RotMat);
				PosMat.GetRow(2) = UpVec * RotMat;
				PosMat.RecreateMatrix(2,0);
				//UpdateOrientation(_pWPhysState,_pCreep,LookMat);
				pCharCD->m_Creep_Orientation.m_Value.Create(PosMat);
			}
		}
	}

	/*if (!(m_MoveFlags & CREEPINGDARK_MOVEFLAGS_FAKEGRAV) && (m_MoveFlags & CREEPINGDARK_MOVEFLAGS_AUTOPULLUP))
	{
		//ConOut(CStrF("%d AutoPull: %s  %s",_pWPhys->GetGameTick(),Gravity.GetString().Str(),m_Gravity.GetString().Str()));
		CVec3Dfp32 bisec = (Up - Gravity).Normalize();
		fp32 CosHalfAngle = Up * bisec;
		if (CosHalfAngle > 0.01f)
		{
			CVec3Dfp32 Axis;
			Up.CrossProd(-Gravity, Axis);
			fp32 Angle = -M_ACos(CosHalfAngle) * 2.0f * (0.5f/_PI);
			if (M_Fabs(Angle) > 0.00001f)
			{
				Angle = Sign(Angle) * Min(0.1f, M_Fabs(Angle)) * _dTime * 2.0f;
				CMat4Dfp32 RotMat, NewMat;
				CQuatfp32(Axis, Angle).SetMatrix(RotMat);
				PosMat.Multiply(RotMat, NewMat);
				//m_pWPhysState->Object_SetRotation(m_pObj->m_iObject, NewMat);
				pCharCD->m_Creep_Orientation.m_Value.Create(NewMat);
				pCharCD->m_Creep_Orientation.MakeDirty();
				m_MoveFlags = m_MoveFlags | CREEPINGDARK_MOVEFLAGS_IS_ROTATING;
			}
			else
			{
				m_MoveFlags = m_MoveFlags & ~CREEPINGDARK_MOVEFLAGS_AUTOPULLUP;
				m_MoveFlags = m_MoveFlags & ~CREEPINGDARK_MOVEFLAGS_IS_ROTATING;
			}
		}
 
		if (M_Fabs(m_LookSinceGravChange[1]) > CREEPINGDARK_AUTOPULLUPINPUTLIMITVERT)
		{
			m_MoveFlags &= ~CREEPINGDARK_MOVEFLAGS_AUTOPULLUP;
			//ConOut(CStrF("Look Change CutOff: %s",m_LookSinceGravChange.GetString().Str()));
		}
	}
	else
	{
		// Auto-roll to match current gravity vector
		fp32 Angle = GetAngle(-Gravity, PosMat.GetRow(1), PosMat.GetRow(0));
		if (M_Fabs(Angle) > 0.01f)
		{
			const CVec3Dfp32& Axis = PosMat.GetRow(0);
			Angle = Sign(Angle) * Min(0.05f, M_Fabs(Angle)) * _dTime * _pWPhys->GetGameTicksPerSecond() * 0.5f;
			CMat4Dfp32 RotMat, NewMat;
			CQuatfp32(Axis, Angle).SetMatrix(RotMat);
			PosMat.Multiply(RotMat, NewMat);
			pCharCD->m_Creep_Orientation.m_Value.Create(NewMat);
			pCharCD->m_Creep_Orientation.MakeDirty();
			//_pWPhys->Object_SetRotation(_pObj->m_iObject, NewMat);
			m_MoveFlags |= CREEPINGDARK_MOVEFLAGS_IS_ROTATING;
		}
		else
		{
			m_MoveFlags &= ~CREEPINGDARK_MOVEFLAGS_IS_ROTATING;
		}

		m_LookSinceGravChange = 0.0f;
	}*/
}

void CWObject_CreepingDark::CCreepingDarkClientData::UpdateOrientation(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, const CMat4Dfp32& _NewMat)
{
	// Auto-roll to match current gravity vector
	CWObject_CoreData* pChar = _pWPhys->Object_GetCD(m_iStartingCharacter);
	CWO_Character_ClientData* pCharCD = (pChar ? CWObject_Character::GetClientData(pChar) : NULL);

	pCharCD->m_Creep_Orientation.m_Value.Create(_NewMat);
	pCharCD->m_Creep_Orientation.MakeDirty();
	// Should have diff here still :/
	//pCharCD->m_Creep_PrevOrientation = pCharCD->m_Creep_Orientation;
}

void CWObject_CreepingDark::CCreepingDarkClientData::SetOrientation(CWO_Character_ClientData* _pCD, const CVec3Dfp32& _dLook)
{
	if(_dLook.k[1] > 0.005f && m_MoveSpeedTarget > 0.1f)
		m_bCanClimb = true;
	else if(_dLook.k[1] < -0.005f && m_MoveSpeedTarget < -0.1f)
		m_bCanClimb = true;

	// Lazy ftw
	if (m_MoveFlags & CREEPINGDARK_MOVEFLAGS_AUTOPULLUP)
	{
		m_LookSinceGravChange[0] += _dLook[2];
		m_LookSinceGravChange[1] += _dLook[1];
	}
	for (int32 i = 0; i < 2; i++)
	{
		CQuatfp32& qQuat = i == 0 ? _pCD->m_Creep_Orientation.m_Value : _pCD->m_Creep_PrevOrientation.m_Value;
		CMat4Dfp32 PosMat;
		qQuat.CreateMatrix(PosMat);

		CMat4Dfp32 BaseMat = PosMat;
		BaseMat.GetRow(2) = -m_Gravity;
		BaseMat.RecreateMatrix(2, 0);

		CMat4Dfp32 dRot;
		CMat4Dfp32 Temp, NewPosMat;

		// Yaw
		fp32 CosAngle = PosMat.GetRow(0) * BaseMat.GetRow(0);
		CosAngle = Sign(CosAngle) * Clamp01(CosAngle); // Safety precaution
		fp32 Angle = M_ACos(CosAngle) * (1.0f / _PI2) * Sign(PosMat.GetRow(0) * BaseMat.GetRow(2));
		fp32 NewAngle = Min(CREEPINGDARK_MAX_CAMERATILT, Max(-CREEPINGDARK_MAX_CAMERATILT, Angle + _dLook.k[1]));
		fp32 dAngle = NewAngle - Angle;
		dRot.Unit();
		dRot.M_x_RotY(dAngle);
		dRot.Multiply(PosMat, Temp);		// Rotate around local Y-axis

		// Tilt
		CQuatfp32(m_Gravity, -_dLook.k[2]).SetMatrix(dRot);
		Temp.Multiply(dRot, NewPosMat);			// Rotate around gravity Z-axis

		#define IsValidFloat(v) (v > -_FP32_MAX && v < _FP32_MAX)
		#define IsValidVec(v) (IsValidFloat(v.k[0]) && IsValidFloat(v.k[1]) && IsValidFloat(v.k[2]))
		#define IsValidMat(m) (IsValidVec(m.GetRow(0)) && IsValidVec(m.GetRow(1)) && IsValidVec(m.GetRow(2)))
		M_ASSERT(IsValidMat(NewPosMat), "!");
		NewPosMat.RecreateMatrix(0,2);

		qQuat.Create(NewPosMat);
	}

	_pCD->m_Creep_Orientation.MakeDirty();
	_pCD->m_Creep_PrevOrientation.MakeDirty();
}

void CSpline_Creep::Clear()
{
	m_lPoints.Clear();
	m_lSegments.Clear();
	m_Length = 0.0f;
	m_MaxT = 0;
}

void CWObject_CreepingDark::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	CCreepingDarkClientData* pCD = GetCreepingDarkClientData(_pObj);
	if (!pCD)
		return;

	
	#ifndef M_RTM
	{
		// Debug rendering
		CWireContainer* pWC = _pWClient->Debug_GetWireContainer();
		if(pWC)
			pCD->m_TrailSpline.Render(*pWC);
	}
	#endif
}

/*void CWObject_CreepingDarkEntity::OnCreate()
{
	CWObject_CreepingDarkEntityParent::OnCreate();
	m_CreepFlags = 0;
}

void CWObject_CreepingDarkEntity::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	if (StringToHash("CREEPFLAGS") == _KeyHash)
	{
		static const char *FlagsTranslate[] =
		{
			"collide", "slow", NULL
		};

		m_CreepFlags = _pKey->GetThisValue().TranslateFlags(FlagsTranslate);
	}
	else
		CWObject_CreepingDarkEntityParent::OnEvalKey(_KeyHash, _pKey);
}*/

void CWObject_CreepingDarkEntity::OnFinishEvalKeys()
{
	CWObject_CreepingDarkEntityParent::OnFinishEvalKeys();
	// Set physics with the "creeping dark" flag (wind)
	// Physmodel so medium gets selected, though...?
	// If in "slow" mode, don't collide...
	/*if (m_CreepFlags & CREEPINGDARKENTITY_FLAG_SLOW)
		Model_SetPhys(m_iModel[0],false,OBJECT_FLAGS_STATIC);
	else if (m_CreepFlags & CREEPINGDARKENTITY_FLAG_COLLIDE)*/
	Model_SetPhys(m_iModel[0],false,OBJECT_FLAGS_STATIC|OBJECT_FLAGS_CREEPING_DARK);
	ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH|CWO_CLIENTFLAGS_INVISIBLE;
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_CreepingDarkEntity, CWObject_CreepingDarkEntityParent, 0x0100);
