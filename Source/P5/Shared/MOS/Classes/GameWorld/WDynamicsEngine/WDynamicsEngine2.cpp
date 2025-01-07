
#include "pch.h"
//#include "MMath_Vec128.h"
//#include "MMath_Vec128_SSE3.h"
#include "WDynamicsEngine2.h"
#include "WDynamicsEngineUtil.h"

#define DYNAMICSENGINE_DEPENETRATION


CVec3Dfp32 CWD_Inertia::Block(fp32 _Mass, const CVec3Dfp32& _Dim)
{
	return Block(_Mass, _Dim[0], _Dim[1], _Dim[2]);
}

CVec3Dfp32 CWD_Inertia::Block(fp32 _Mass, fp32 _x, fp32 _y, fp32 _z)
{
	return CVec3Dfp32(_y*_y + _z*_z, _x*_x+_z*_z, _x*_x + _y*_y) * _Mass * (1.0f / 12.0f);
}

CWD_RigidBody2::CWD_RigidBody2()
{
	m_bActive = 1;
	m_bStationary = 0;
	m_bInSimulation = 0;
	m_iRB = ~0;
	m_FreezeCounter = 0;

	m_ExternalForce = CVec4Dfp32(0.0f);
	m_ExternalTorque = CVec4Dfp32(0.0f);

	m_lConnectedTo.SetLen(0);

	m_StaticFriction = 0.4f;
	m_DynamicFriction = 0.3f;

	m_bDebris = false;

	m_pUserData2 = NULL;
}


void CWD_RigidBodyState::UpdateWorldInertiaTensorInvert()
{
	CMat4Dfp32 TensorInverted;
	TensorInverted.Unit();
	TensorInverted.k[0][0] = m_TensorInverted[0];
	TensorInverted.k[1][1] = m_TensorInverted[1];
	TensorInverted.k[2][2] = m_TensorInverted[2];
	TensorInverted.k[3][3] = 0.0f;

	CMat4Dfp32 T = m_MatrixOrientation;

	CMat4Dfp32 TT;
	TT.Unit();
	T.Transpose3x3(TT);

	CMat4Dfp32 Tmp;
	M_VMatMul(TT, TensorInverted, Tmp);
	M_VMatMul(Tmp, T, m_WorldInertiaTensorInvert);
	WDYNAMICS_CHECK_VEC128(m_WorldInertiaTensorInvert.r[0]);
	WDYNAMICS_CHECK_VEC128(m_WorldInertiaTensorInvert.r[1]);
	WDYNAMICS_CHECK_VEC128(m_WorldInertiaTensorInvert.r[2]);
	WDYNAMICS_CHECK_VEC128(m_WorldInertiaTensorInvert.r[3]);
}


CWD_DynamicsWorld::CWD_DynamicsWorld()
{
	m_Gravity = CVec4Dfp32(0.0f, 0.0f, -9.81f, 0.0f);

	m_lpRigidBodies.QuickSetLen(100);
	m_lRigidBodyStates.QuickSetLen(100);
	m_lRigidBodyStatesSaved.QuickSetLen(100);
	m_lpRigidBodies.QuickSetLen(0);
	m_lRigidBodyStates.QuickSetLen(0);
	m_lRigidBodyStatesSaved.QuickSetLen(0);

	m_pCollider = NULL;
	m_pDebugRenderer = NULL;

	m_DummyWorldState.Clear();
	m_DummyWorldState.SetMass(100000000.0f);
	m_DummyWorldState.SetInertiaTensor(CVec3Dfp32(100000000.0f));

	m_DummyRigidBody.SetActive(false);
	m_DummyRigidBody.SetStationary(1);
	m_DummyRigidBody.m_CenterOfMass = CVec3Dfp32(0.0f);
}

int CWD_DynamicsWorld::AddRigidBody(CWD_RigidBody2 *_pRigidBody, const CWD_RigidBodyState& _State)
{
	int ID = m_lpRigidBodies.Len();
	m_lpRigidBodies.Add(_pRigidBody);
	m_lRigidBodyStates.Add(_State);
	m_lRigidBodyStatesSaved.Add(_State);

	CWD_RigidBodyState& State = m_lRigidBodyStates[ID];
	State.m_bStationary = _pRigidBody->IsStationary();
	State.m_bActive = _pRigidBody->m_bActive;
	State.m_FreezeCounter = _pRigidBody->m_FreezeCounter;

	_pRigidBody->m_iRB = ID;
	return ID;
}

/*template <typename S>
int CWD_DynamicsWorld::AddConstraint(const S& _Constraint)
{
	int ID = m_lConstraints.Len();
	m_lConstraints.Add(_Constraint);
	return ID;
}*/

void CWD_DynamicsWorld::SetCollider(CWD_Collider *_pCollider)
{
	m_pCollider = _pCollider;
}

CWD_Collider *CWD_DynamicsWorld::GetCollider()
{
	return m_pCollider;
}

void CWD_DynamicsWorld::SetDebugRenderer(CWD_DynamicsDebugRenderer *_pRenderer)
{
	m_pDebugRenderer = _pRenderer;
}

CWD_DynamicsDebugRenderer *CWD_DynamicsWorld::GetDebugRenderer()
{
	return m_pDebugRenderer;
}

void CWD_DynamicsWorld::SetGravity(const CVec3Dfp32& _Gravity)
{
	m_Gravity = CWD_RigidBodyState::To4D_LastZero(_Gravity);
}

CVec3Dfp32 CWD_DynamicsWorld::GetGravity() const
{
	return CWD_RigidBodyState::To3D(m_Gravity);
}

void CWD_DynamicsWorld::RenderDebugInfo()
{
	if (!m_pDebugRenderer) return;

	m_pDebugRenderer->SetColour(0, 0, 255.0f);

	TAP_RCD<CWD_ContactInfo> pContactInfo = m_lContactInfo;
	for (int i = 0; i < m_nContacts; i++)
	{
		const CWD_ContactInfo& CI = pContactInfo[i];
		m_pDebugRenderer->RenderVector(CI.GetPointOfCollision(), CI.GetNormal() * 1.0f);
	}

	TAP_RCD<CWD_ContactInfo> pConstraintContactInfo = m_lConstraintContactInfo;
	for (int i = 0; i < m_nConstraintContacts; i++)
	{
		const CWD_ContactInfo& CI = pConstraintContactInfo[i];
		m_pDebugRenderer->RenderVector(CI.GetPointOfCollision(), CI.GetNormal() * 1.0f);

		CVec3Dfp32 P1 = GetTransform(CI.m_iRB1).GetRow(3);
		CVec3Dfp32 P2 = GetTransform(CI.m_iRB2).GetRow(3);

		m_pDebugRenderer->RenderVector(P1, CI.GetPointOfCollision() - P1);
		m_pDebugRenderer->RenderVector(P2, CI.GetPointOfCollision() - P2);
	}


#if 1
	TAP_RCD<CWD_RigidBodyState> pRigidBodyStates = m_lRigidBodyStates;

	for (int i = 1; i < pRigidBodyStates.Len(); i++)
	{
		CWD_RigidBodyState& State= pRigidBodyStates[i];

		CMat4Dfp32 M = State.GetTransform();

		CVec3Dfp32 P = M.GetRow(3);
		m_pDebugRenderer->SetColour(255.0f, 0, 0);
		m_pDebugRenderer->RenderVector(P, M.GetRow(0) * 1.0f);

		m_pDebugRenderer->SetColour(0, 255.0f, 0);
		m_pDebugRenderer->RenderVector(P, M.GetRow(1) * 1.0f);

		m_pDebugRenderer->SetColour(0, 0, 255.0f);
		m_pDebugRenderer->RenderVector(P, M.GetRow(2) * 1.0f);

	}
#endif

}

void CWD_DynamicsWorld::Clear()
{
	m_lpRigidBodies.QuickSetLen(0);
	m_lRigidBodyStates.QuickSetLen(0);
	m_lRigidBodyStatesSaved.QuickSetLen(0);
	m_lConstraints.QuickSetLen(0);
	m_nContacts = 0;
	m_nConstraintContacts = 0;

	m_nCollisionEvents = 0;

	AddRigidBody(&m_DummyRigidBody, m_DummyWorldState);
}

TAP_RCD<CWD_RigidBody2 *> CWD_DynamicsWorld::GetRigidBodies()
{
	TAP_RCD<CWD_RigidBody2 *> pB = m_lpRigidBodies;
	// Exclude "Dummy object"
	pB.m_Len--;
	pB.m_pArray++;
	return pB;
	//return m_lpRigidBodies;
}

void CWD_DynamicsWorld::Simulate(fp32 _dt, int _nSteps, void *_pArgument1, void *_pArgument2)
{
	MSCOPESHORT(CWD_DynamicsWorld::Simulate);
	M_NAMEDEVENT("CWD_DynamicsWorld::Simulate", 0xff000000);

	m_nCollisionEvents = 0;

	m_pArgument1 = _pArgument1;
	m_pArgument2 = _pArgument2;

	for (int i = 0; i < _nSteps; i++)
	{
		Step(_dt / fp32(_nSteps) , i, _nSteps);
	}

	ClearExternalForces();
	CheckStationary(_dt);
	CopyFlags();
}

void CWD_DynamicsWorld::FilterDebrisCollisions(TAP_RCD<CWD_ContactInfo> _pContactInfo)
{
	MSCOPESHORT(CWD_DynamicsWorld::FilterDebrisCollisions);

	TAP_RCD<CWD_RigidBody2 *> pRigidBodies = m_lpRigidBodies;

	for (int i = 0; i < _pContactInfo.Len(); i++)
	{	
		CWD_ContactInfo& CI = _pContactInfo[i];

		CWD_RigidBody2 *pRB1 = pRigidBodies[CI.m_iRB1];
		CWD_RigidBody2 *pRB2 = pRigidBodies[CI.m_iRB2];

		if (pRB1->m_bDebris && CI.m_iRB2 != 0) 
		{
			if (CI.m_Normal * CVec4Dfp32(0.0f, 0.0f, -1.0f, 0.0f) > 0.8f)
			{
				CI.m_iRB1 = 0;
				CI.m_iRB2 = 0;
			}

			CNormalCluster &Cluster = pRB1->m_CollisionToWorldCluster;
			for (int j = 0; j < CNORMALCLUSTER_N_DIRECTIONS; j++)
			{
				if (Cluster.m_nNormals[j] > 0)
				{
					if (CI.m_Normal * Cluster.m_Normals[j] < -0.7f)
					{
						CI.m_iRB1 = 0;
						CI.m_iRB2 = 0;
						break;
					}
				}
			}
		}
		else if (pRB2->m_bDebris && CI.m_iRB1 != 0) 
		{

			if (CI.m_Normal * CVec4Dfp32(0.0f, 0.0f, 1.0f, 0.0f) > 0.8f)
			{
				CI.m_iRB1 = 0;
				CI.m_iRB2 = 0;
			}

			CNormalCluster &Cluster = pRB2->m_CollisionToWorldCluster;
			for (int j = 0; j < CNORMALCLUSTER_N_DIRECTIONS; j++)
			{
				if (Cluster.m_nNormals[j] > 0)
				{
					if (CI.m_Normal * Cluster.m_Normals[j] > 0.7f)
					{
						CI.m_iRB1 = 0;
						CI.m_iRB2 = 0;
						break;
					}
				}
			}
		}
	}
}

void CWD_DynamicsWorld::Step(fp32 _dt, int _iStep, int _nSteps)
{
	MSCOPESHORT(CWD_DynamicsWorld::Step);
	M_NAMEDEVENT("CWD_DynamicsWorld::Step", 0xff000000);
	
	m_pCollider->PreApplyExternalForces(this, m_pArgument1, m_pArgument2);

	AddExternalForces(_dt);

	UpdateOrientatioMatrixAndTensor();

	int nCollisions = DetectCollisions(_dt);
	TAP_RCD<CWD_ContactInfo> pContactInfo = m_lContactInfo;
	pContactInfo.m_Len = nCollisions;

	FilterDebrisCollisions(pContactInfo);

	m_pCollider->ApplyExternalForces(this, m_pArgument1, m_pArgument2);

	PreCalculateImpulseDenominator(pContactInfo);

	//int nConstraintCollisions = DetectConstraintCollisions(_dt);
	TAP_RCD<CWD_ContactInfo> pConstraintContactInfo = m_lConstraintContactInfo;
//	pConstraintContactInfo.m_Len = nConstraintCollisions;
	pConstraintContactInfo.m_Len = m_nConstraintContacts;
	PreCalculateImpulseDenominator(pConstraintContactInfo);

	// TODO: Anropar man denna här crashar det, varför???!!!
	//pConstraintContactInfo.m_Len = DetectConstraintCollisions(_dt);

	ProcessCollisionsFirst(pContactInfo, 1.0, _dt);
	ProcessConstraintCollisionsFirst(pConstraintContactInfo, 0.0f, _dt);
	//ProcessCollisionsFirst(pConstraintContactInfo, 0.0f, _dt);

	for (int i = 0; i < pConstraintContactInfo.Len(); i++)
	{
		pConstraintContactInfo[i].m_BiasVelocity = M_VZero();
		pConstraintContactInfo[i].m_Distance = 0.0f;
	}

	UpdateVelocity(_dt);

	CVec4Dfp32 dt(_dt);

	for (int i = 0; i < 10; i++)
	{
		//pConstraintContactInfo.m_Len = DetectConstraintCollisions(_dt);

		ProcessCollisionsFirst(pContactInfo, 0.0f, _dt);
		ProcessConstraintCollisionsFirst(pConstraintContactInfo, 0.0f, _dt);
		//ProcessCollisionsFirst(pConstraintContactInfo, 0.0f, _dt);


		//ProcessCollisionsSimpleFriction(pConstraintContactInfo, 0.0f, _dt);


		int nConstraints = m_lConstraints.Len();

		for (int j = 0; j < nConstraints; j++)
		{
			CWD_Constraint *Constraint = m_lConstraints.Get(j);

			if (Constraint->m_bUseSolve)
			{
				Constraint->Solve(*this, dt);
			}
		}

//		ProcessCollisionsSimpleFriction(pContactInfo, 0.0f, _dt);
	}

	UpdatePosition(_dt);
	UpdateOrientation(_dt);

	CollectCollisionEvents(m_lContactInfo, m_nContacts);


	m_pCollider->PostApplyExternalForces(this, m_pArgument1, m_pArgument2);

}

void CWD_DynamicsWorld::CollectCollisionEvents(const TArray<CWD_ContactInfo>& _lCollisionList, int _nCollisions)
{
	MSCOPESHORT(CWD_DynamicsWorld::CollectCollisionEvents);

	fp32 MinEventImpulse = 0.5f;
	fp32 MinCollectEventImpulse = MinEventImpulse / 5.0f;
	int nMaxEvents = 20;
	int nMaxTemporaryEvents = 100;
	int nTempEvents = 0;

	m_lCollisionEvents.SetLen(nMaxEvents);
	m_lTemporaryCollisionEvents.SetLen(nMaxTemporaryEvents);

	TAP_RCD<const CWD_ContactInfo> pCollisions = _lCollisionList;
	TAP_RCD<CWD_CollisionEvent> pCollisionEvents = m_lCollisionEvents;
	TAP_RCD<CWD_CollisionEvent> pTempCollisionEvents = m_lTemporaryCollisionEvents;

	TAP_RCD<CWD_RigidBody2 *> ppRigidBodes = m_lpRigidBodies;

	for (int i = 0; i < _nCollisions && nTempEvents < nMaxTemporaryEvents; i++)
	{
		int iRB1 = pCollisions[i].m_iRB1;
		int iRB2 = pCollisions[i].m_iRB2;

		CWD_RigidBody2 *pBody1 = ppRigidBodes[iRB1];
		CWD_RigidBody2 *pBody2 = ppRigidBodes[iRB2];

		/*
		if (pBody1 == &m_DummyRigidBody)
			pBody1 = NULL;

		if (pBody2 == &m_DummyRigidBody)
			pBody2 = NULL;
			*/

		fp32 MinMass = _FP32_MAX;
		if (iRB1 != 0)
			MinMass = Min(MinMass, pBody1->GetMass());

		if (iRB2 != NULL)
			MinMass = Min(MinMass, pBody2->GetMass());

		if (pCollisions[i].m_MaxAppliedImpulse[0] / MinMass > MinCollectEventImpulse)
		{
			bool found = false;
			for (int j = 0; j < nTempEvents; j++)
			{
				CWD_CollisionEvent& Event = pTempCollisionEvents[j];
				if (Event.m_iRB1 == iRB1 && Event.m_iRB2 == iRB2 ||
					Event.m_iRB1 == iRB2 && Event.m_iRB2 == iRB1)
				{
					Event.m_ImpulseTimesInvMass += pCollisions[i].m_MaxAppliedImpulse[0] / MinMass;
					found = true;
					break;
				}
			}

			if (!found)
			{
				pTempCollisionEvents[nTempEvents].m_ImpulseTimesInvMass = pCollisions[i].m_MaxAppliedImpulse[0] / MinMass;
				pTempCollisionEvents[nTempEvents].m_iRB1 = iRB1;
				pTempCollisionEvents[nTempEvents].m_iRB2 = iRB2;
				pTempCollisionEvents[nTempEvents].m_PointOfCollision = CWD_RigidBodyState::To3D(pCollisions[i].m_PointOfCollision);
				pTempCollisionEvents[nTempEvents].m_UserData1 = pCollisions[i].m_UserData1;
				pTempCollisionEvents[nTempEvents].m_UserData2 = pCollisions[i].m_UserData2;
				nTempEvents++;
			}
		}
	}

	m_lTemporaryCollisionEvents.QuickSetLen(nTempEvents);

	int nEvents = m_nCollisionEvents;
	//			fp32 SmallestImpulse = -1;
	for (int i = 0; i < nTempEvents; i++)
	{
		CWD_CollisionEvent& Event = pTempCollisionEvents[i];

		if (Event.m_ImpulseTimesInvMass > MinEventImpulse)
		{
			//M_TRACEALWAYS("ImpulseScaled: %f\n", Event.m_ImpulseTimesInvMass);

			int j = 0;
			if (nEvents < nMaxEvents)
			{
				pCollisionEvents[nEvents] = Event;
				j = nEvents++;
			}
			else
			{
				for (j = 0; j < nEvents; j++)
				{
					if (pCollisionEvents[j].m_ImpulseTimesInvMass < Event.m_ImpulseTimesInvMass)
					{
						pCollisionEvents[j] = Event;
						break;
					}
				}
			}

			if (j < nEvents)
			{
				const CWD_RigidBodyState& RB1 = GetRigidBodyState(Event.m_iRB1);
				const CWD_RigidBodyState& RB2 = GetRigidBodyState(Event.m_iRB2);
				CVec3Dfp32 RotVel1, RotVel2;
				RB1.GetAngularVelocity().CrossProd(Event.m_PointOfCollision - RB1.GetPosition(), RotVel1);
				RB2.GetAngularVelocity().CrossProd(Event.m_PointOfCollision - RB2.GetPosition(), RotVel2);
				pCollisionEvents[j].m_Velocity1 = RB1.GetVelocity() + RotVel1;
				pCollisionEvents[j].m_Velocity2 = RB2.GetVelocity() + RotVel2;
			}
		}
	}

	m_nCollisionEvents = nEvents;
	m_lCollisionEvents.QuickSetLen(nEvents);
}

const TArray<CWD_CollisionEvent>& CWD_DynamicsWorld::GetCollisionEvents()
{
	return m_lCollisionEvents;
}

void CWD_DynamicsWorld::PreCalculateImpulseDenominator(TAP_RCD<CWD_ContactInfo> _pContactInfo)
{
	MSCOPESHORT(CWD_DynamicsWorld::PreCalculateImpulseDenominator);

	TAP_RCD<CWD_RigidBodyState> pState = m_lRigidBodyStates;
	TAP_RCD<CWD_RigidBody2 *> pRigidBodies = m_lpRigidBodies;

	for (int i = 0; i < _pContactInfo.Len(); i++)
	{
		CWD_ContactInfo& CI = _pContactInfo[i];

		int iRB1 = CI.m_iRB1;
		int iRB2 = CI.m_iRB2;

		/*fp32 StaticFriction = Min(pRigidBodies[CI.m_iRB1]->m_StaticFriction, pRigidBodies[CI.m_iRB2]->m_StaticFriction);
		fp32 DynamicFriction = Min(pRigidBodies[CI.m_iRB1]->m_DynamicFriction, pRigidBodies[CI.m_iRB2]->m_DynamicFriction);
		CI.m_Friction = CVec4Dfp32(DynamicFriction, StaticFriction, 0.0f, 0.0f);*/

		CI.m_MaxAppliedImpulse = CVec4Dfp32(0.0f);

		CWD_RigidBodyState *pRBState1 = &pState[CI.m_iRB1];
		CWD_RigidBodyState *pRBState2 = &pState[CI.m_iRB2];

		vec128 B1 = pRBState1->m_MassInverted;
		vec128 B2 = pRBState2->m_MassInverted;

		vec128 Normal = CI.m_Normal;
		vec128 RA = M_VSub(CI.m_PointOfCollision, pRBState1->m_Position);
		vec128 RB = M_VSub(CI.m_PointOfCollision, pRBState2->m_Position);

		const CMat4Dfp32& WITInv1 = pRBState1->m_WorldInertiaTensorInvert;
		const CMat4Dfp32& WITInv2 = pRBState2->m_WorldInertiaTensorInvert;

		vec128 tmp;
		// TODO: DENNA MULTIPLICERING ÄR ÅT FEL HÅLL!!!
		tmp = M_VMulMat(M_VXpd(RA, Normal), WITInv1); 
		vec128 B3 = M_VDp3(Normal, M_VXpd(tmp, RA));

		// TODO: DENNA MULTIPLICERING ÄR ÅT FEL HÅLL!!!
		tmp = M_VMulMat(M_VXpd(RB, Normal), WITInv2);
		vec128 B4 = M_VDp3(Normal, M_VXpd(tmp, RB));

		vec128 B1_ADD_B2 = M_VAdd(B1, B2);
		vec128 B3_ADD_B4 = M_VAdd(B3, B4);
		vec128 Denom = M_VAdd(B1_ADD_B2, B3_ADD_B4);

		//Denom = B1_ADD_B2;

		CI.m_Misc = M_VSelComp(2, Denom, CI.m_Misc);

		CI.m_RestImpulse = CWD_DynamicsUtilFunctions::RestImpulse(pRBState1, pRBState2, CI, RA, RB);

		vec128 RelativeTangentVelocity = CWD_DynamicsUtilFunctions::RelativeTangentVelocity(pRBState1, pRBState2, RA, RB, CI.m_Normal);
		CI.m_RelativeTangentVelocityDirection = CWD_DynamicsUtilFunctions::M_VSafeNrm3(RelativeTangentVelocity);
	}
}


int CWD_DynamicsWorld::DetectConstraintCollisions(fp32 _dt)
{
	MSCOPESHORT(CWD_DynamicsWorld::DetectConstraintCollisions);

	vec128 dt = M_VLdScalar(_dt);
	int nConstraints = m_lConstraints.Len();

	CWD_ContactInfo *pBase = m_lConstraintContactInfo.GetBasePtr();
	TAP_RCD<CWD_ContactInfo> pConstraintContactInfo = m_lConstraintContactInfo;
	int nConstraintContactInfo = m_lConstraintContactInfo.Len();

	int nContacts = 0;
	// TODO: Range check!!!
	for (int i = 0; i < nConstraints; i++)
	{
		pConstraintContactInfo.m_pArray = pBase + nContacts;
		pConstraintContactInfo.m_Len = nConstraintContactInfo - nContacts;
		const CWD_Constraint *Constraint = m_lConstraints.Get(i);

//		Constraint->GenerateContact(*this, dt, pConstraintContactInfo[i]);
		if (!Constraint->m_bUseSolve)
		{
			nContacts += Constraint->GenerateContact(*this, dt, pConstraintContactInfo);
		}

	}
//	m_nConstraintContacts = nConstraints;
	m_nConstraintContacts = nContacts;

	return nContacts;
}

int CWD_DynamicsWorld::DetectCollisions(fp32 _dt)
{
	MSCOPESHORT(CWD_DynamicsWorld::DetectCollisions);

	if (m_lContactInfo.Len() < CWD_DYNAMICS_MAX_COLLISIONS)
		m_lContactInfo.QuickSetLen(CWD_DYNAMICS_MAX_COLLISIONS);

	if (m_lConstraintContactInfo.Len() < CWD_DYNAMICS_MAX_CONSTRAINT_COLLISIONS)
		m_lConstraintContactInfo.QuickSetLen(CWD_DYNAMICS_MAX_CONSTRAINT_COLLISIONS);

	SaveFullState();

	UpdateVelocity(_dt);
	UpdatePosition(_dt);
	UpdateOrientation(_dt);

	// Can't do this- m_lpRigidBodies is updated in CWD_WorldModelCollider::Collide
	//TAP_RCD<CWD_RigidBody2 *> pRigidBodies = m_lpRigidBodies;

	// Exclude "Dummy object"
	//pRigidBodies.m_Len--;
	//pRigidBodies.m_pArray++;

	TAP_RCD<CWD_ContactInfo> pContactInfo = m_lContactInfo;
	int nCollisions = m_pCollider->Collide(this, m_lpRigidBodies, pContactInfo, m_pArgument1, m_pArgument2);
	m_nContacts = nCollisions;

	/*
	vec128 dt = M_VLdScalar(_dt);
	int nConstraints = m_lConstraints.Len();

	TAP_RCD<CWD_ContactInfo> pConstraintContactInfo = m_lConstraintContactInfo;

	// TODO: Range check!!!
	for (int i = 0; i < nConstraints; i++)
	{
		const CWD_Constraint *Constraint = m_lConstraints.Get(i);
		Constraint->GenerateContact(*this, dt, pConstraintContactInfo[i]);
	}
	//nCollisions += nConstraints;
	m_nConstraintContacts = nConstraints;
	*/

	m_nConstraintContacts = DetectConstraintCollisions(_dt);
	pContactInfo.m_Len = nCollisions;

	RestoreFullState();

	return nCollisions;
}

// TODO: Friktionen funkar inte helt som den ska när objekt "börjar vila"
// Objekten stannar inte helt utan snurrar långsamt runt...

void CWD_DynamicsWorld::ProcessCollisionsFirst(TAP_RCD<CWD_ContactInfo> _pContactInfo, fp32 _Epsilon, fp32 _dt)
{
	MSCOPESHORT(CWD_DynamicsWorld::ProcessCollisionsFirst);

	TAP_RCD<CWD_RigidBodyState> pBodyStates = m_lRigidBodyStates;

	vec128 Epsilon = M_VLdScalar(_Epsilon);
	vec128 Zero = M_VZero();
	// TODO: Som funktion av dt?
#ifdef DYNAMICSENGINE_DEPENETRATION
	vec128 DepenetrationSpeed = M_VScalar(4.0f);
#else
	vec128 DepenetrationSpeed = M_VScalar(0.0f);
#endif

	vec128 VelocityEpsilon = M_VScalar(0.01f);

	vec128 nProcessedEpsilon = M_VZero();

	// TODO: Som parameter...
//	vec128 DynamicFriction = M_VScalar(0.3f);
//	vec128 StaticFriction = M_VScalar(0.4f);

	for (int i = 0; i < _pContactInfo.Len(); i++)
	{
		CWD_ContactInfo& CI = _pContactInfo[i];

		vec128 Friction = CI.m_Friction_Elasticity;
		vec128 DynamicFriction = M_VSplatX(Friction);
		vec128 StaticFriction = M_VSplatY(Friction);
		vec128 Elasticity = M_VMul(Epsilon, M_VSplatZ(Friction));

		CWD_RigidBodyState *pRBState1 = &pBodyStates[CI.m_iRB1];
		CWD_RigidBodyState *pRBState2 = &pBodyStates[CI.m_iRB2];

		vec128 RA = M_VSub(CI.m_PointOfCollision, pRBState1->m_Position);
		vec128 RB = M_VSub(CI.m_PointOfCollision, pRBState2->m_Position);

		// TODO: Temp-test!!
		//RA = M_VSelComp(3, M_VZero(), RA);
		//RB = M_VSelComp(3, M_VZero(), RB);

		vec128 RelativeVelocity = CWD_DynamicsUtilFunctions::RelativeVelocity(pRBState1, pRBState2, RA, RB, CI.m_Normal);
		RelativeVelocity = M_VAdd(RelativeVelocity, CI.m_BiasVelocity);
		vec128 IsApproaching = M_VCmpGEMsk(RelativeVelocity, Zero);
		vec128 IsApproachingEpsilon = M_VCmpGEMsk(RelativeVelocity, VelocityEpsilon);

		vec128 ImpulseMag = CollisionImpulseMag(CI, RelativeVelocity, Elasticity, pRBState1, pRBState2);

		CI.m_MaxAppliedImpulse = M_VMax(CI.m_MaxAppliedImpulse, M_VAbs(ImpulseMag));
		
		// Only apply impulse if objects are approaching <=> relative veloicty >= 0.0
		//		ImpulseMag = M_VSelMsk(M_VCmpGEMsk(RelativeVelocity, Zero), ImpulseMag, Zero);
		ImpulseMag = M_VSelMsk(IsApproaching, ImpulseMag, Zero);

		nProcessedEpsilon = M_VAdd(nProcessedEpsilon, M_VSelMsk(IsApproachingEpsilon, M_VOne(), Zero));

		vec128 MinDepenetrationSpeed = M_VMul(DepenetrationSpeed, M_VRcp_Est(M_VMax(pRBState1->m_MassInverted, pRBState2->m_MassInverted)));


		vec128 Distance = M_VLdScalar(CI.m_Distance);
		vec128 DepenetrationImpulse = M_VMul(Distance, MinDepenetrationSpeed);
		//		DepenetrationImpulse = M_VSelMsk(M_VCmpGEMsk(RelativeVelocity, Zero), DepenetrationImpulse , Zero);
		DepenetrationImpulse = M_VSelMsk(IsApproaching, DepenetrationImpulse , Zero);
		DepenetrationImpulse = M_VSelComp(3, M_VZero(), DepenetrationImpulse);

		vec128 Impulse = M_VMul(CI.m_Normal, M_VAdd(ImpulseMag, DepenetrationImpulse));
		WDYNAMICS_CHECK_VEC128(Impulse);

		// TODO: HACK!
		//Impulse = M_VMul(Impulse, M_VLd(0.9f, 0.9f, 0.9f, 0.0f));
		//vec128 Impulse = M_VMul(CI.m_Normal, ImpulseMag);

		if (pRBState1->m_bActive)
		{
			pRBState1->AddImpulse(Impulse, RA);
		//	pRBState1->AddImpulse(M_VMul(CI.m_Normal,DepenetrationImpulse));
		}

		if (pRBState2->m_bActive)
		{
			pRBState2->AddImpulse(M_VNeg(Impulse), RB);
		//	pRBState2->AddImpulse(M_VMul(CI.m_Normal,M_VNeg(DepenetrationImpulse)));
		}

		// Friction
		vec128 RelativeTangentVelocity = CWD_DynamicsUtilFunctions::RelativeTangentVelocity(pRBState1, pRBState2, RA, RB, CI.m_Normal);
		vec128 RestImpulse = CWD_DynamicsUtilFunctions::RestImpulse(pRBState1, pRBState2, CI, RA, RB);
		vec128 RestImpulseMag = M_VLen3(RestImpulse);

//		vec128 T = M_VNrm3(RelativeTangentVelocity);
		vec128 T = CWD_DynamicsUtilFunctions::M_VSafeNrm3(RelativeTangentVelocity);

		vec128 DynamicFrictionImpulseMag = M_VMul(ImpulseMag, DynamicFriction);
		vec128 StaticFrictionImpulseMag = M_VMul(ImpulseMag, StaticFriction);

		vec128 DynamicFrictionImpulse = M_VMul(T, DynamicFrictionImpulseMag);

//		vec128 Foo1 = M_VSplatX(RestImpulseMag);
//		vec128 Foo2 = M_VSplatX(StaticFrictionImpulseMag);

//		vec128 FrictionImpulse = M_VSelMsk(M_VCmpLEMsk(Foo1, Foo2), RestImpulse, DynamicFrictionImpulse);
		vec128 FrictionImpulse = M_VSelMsk(M_VCmpLEMsk(RestImpulseMag, StaticFrictionImpulseMag), RestImpulse, DynamicFrictionImpulse);

//		vec128 FrictionImpulse = M_VMul(T, FrictionImpulseMag);
		WDYNAMICS_CHECK_VEC128(FrictionImpulse);

#if 1
		if (pRBState1->m_bActive)
			pRBState1->AddImpulse(FrictionImpulse, RA);

		if (pRBState2->m_bActive)
			pRBState2->AddImpulse(M_VNeg(FrictionImpulse), RB);
#endif 

	}
}

void CWD_DynamicsWorld::ProcessConstraintCollisionsFirst(TAP_RCD<CWD_ContactInfo> _pContactInfo, fp32 _Epsilon, fp32 _dt)
{
	MSCOPESHORT(CWD_DynamicsWorld::ProcessConstraintCollisionsFirst);

	TAP_RCD<CWD_RigidBodyState> pBodyStates = m_lRigidBodyStates;

	vec128 Epsilon = M_VLdScalar(_Epsilon);
	vec128 Zero = M_VZero();
	// TODO: Som funktion av dt?
#ifdef DYNAMICSENGINE_DEPENETRATION
	vec128 DepenetrationSpeed = M_VScalar(16.0f);
#else
	vec128 DepenetrationSpeed = M_VScalar(0.0f);
#endif

	vec128 VelocityEpsilon = M_VScalar(0.01f);

	vec128 nProcessedEpsilon = M_VZero();

	// TODO: Som parameter...
	//	vec128 DynamicFriction = M_VScalar(0.3f);
	//	vec128 StaticFriction = M_VScalar(0.4f);

	for (int i = 0; i < _pContactInfo.Len(); i++)
	{
		CWD_ContactInfo& CI = _pContactInfo[i];

		vec128 Friction = CI.m_Friction_Elasticity;
		vec128 DynamicFriction = M_VSplatX(Friction);
		vec128 StaticFriction = M_VSplatY(Friction);

		CWD_RigidBodyState *pRBState1 = &pBodyStates[CI.m_iRB1];
		CWD_RigidBodyState *pRBState2 = &pBodyStates[CI.m_iRB2];

		vec128 RA = M_VSub(CI.m_PointOfCollision, pRBState1->m_Position);
		vec128 RB = M_VSub(CI.m_PointOfCollision, pRBState2->m_Position);

		// TODO: DETTA BORDE INTE BEHÖVAS!!
		RA = M_VSelComp(3, M_VZero(), RA);
		RB = M_VSelComp(3, M_VZero(), RB);

		vec128 RelativeVelocity = CWD_DynamicsUtilFunctions::RelativeVelocity(pRBState1, pRBState2, RA, RB, CI.m_Normal);
		RelativeVelocity = M_VAdd(RelativeVelocity, CI.m_BiasVelocity);
		vec128 IsApproaching = M_VCmpGEMsk(RelativeVelocity, Zero);
		vec128 IsApproachingEpsilon = M_VCmpGEMsk(RelativeVelocity, VelocityEpsilon);

		vec128 ImpulseMag = CollisionImpulseMag(CI, RelativeVelocity, Epsilon, pRBState1, pRBState2);

		CI.m_MaxAppliedImpulse = M_VMax(CI.m_MaxAppliedImpulse, M_VAbs(ImpulseMag));

		// Only apply impulse if objects are approaching <=> relative veloicty >= 0.0
		//		ImpulseMag = M_VSelMsk(M_VCmpGEMsk(RelativeVelocity, Zero), ImpulseMag, Zero);
		ImpulseMag = M_VSelMsk(IsApproaching, ImpulseMag, Zero);

		nProcessedEpsilon = M_VAdd(nProcessedEpsilon, M_VSelMsk(IsApproachingEpsilon, M_VOne(), Zero));

		vec128 MinDepenetrationSpeed = M_VMul(DepenetrationSpeed, M_VRcp_Est(M_VMax(pRBState1->m_MassInverted, pRBState2->m_MassInverted)));


		vec128 Distance = M_VLdScalar(CI.m_Distance);
		vec128 DepenetrationImpulse = M_VMul(Distance, MinDepenetrationSpeed);
		//		DepenetrationImpulse = M_VSelMsk(M_VCmpGEMsk(RelativeVelocity, Zero), DepenetrationImpulse , Zero);
		DepenetrationImpulse = M_VSelMsk(IsApproaching, DepenetrationImpulse , Zero);
		DepenetrationImpulse = M_VSelComp(3, M_VZero(), DepenetrationImpulse);

		vec128 Impulse = M_VMul(CI.m_Normal, M_VAdd(ImpulseMag, DepenetrationImpulse));

		// TODO: HACK!
		//Impulse = M_VMul(Impulse, M_VLd(0.9f, 0.9f, 0.9f, 0.0f));
		//vec128 Impulse = M_VMul(CI.m_Normal, ImpulseMag);

		if (pRBState1->m_bActive)
		{
			pRBState1->AddImpulse(Impulse, RA);
			//	pRBState1->AddImpulse(M_VMul(CI.m_Normal,DepenetrationImpulse));
		}

		if (pRBState2->m_bActive)
		{
			pRBState2->AddImpulse(M_VNeg(Impulse), RB);
			//	pRBState2->AddImpulse(M_VMul(CI.m_Normal,M_VNeg(DepenetrationImpulse)));
		}

		// Friction
		vec128 RelativeTangentVelocity = CWD_DynamicsUtilFunctions::RelativeTangentVelocity(pRBState1, pRBState2, RA, RB, CI.m_Normal);
		vec128 RestImpulse = CWD_DynamicsUtilFunctions::RestImpulse(pRBState1, pRBState2, CI, RA, RB);
		vec128 RestImpulseMag = M_VLen3(RestImpulse);

		//		vec128 T = M_VNrm3(RelativeTangentVelocity);
		vec128 T = CWD_DynamicsUtilFunctions::M_VSafeNrm3(RelativeTangentVelocity);

		vec128 DynamicFrictionImpulseMag = M_VMul(ImpulseMag, DynamicFriction);
		vec128 StaticFrictionImpulseMag = M_VMul(ImpulseMag, StaticFriction);

		vec128 DynamicFrictionImpulse = M_VMul(T, DynamicFrictionImpulseMag);

		//		vec128 Foo1 = M_VSplatX(RestImpulseMag);
		//		vec128 Foo2 = M_VSplatX(StaticFrictionImpulseMag);

		//		vec128 FrictionImpulse = M_VSelMsk(M_VCmpLEMsk(Foo1, Foo2), RestImpulse, DynamicFrictionImpulse);
		vec128 FrictionImpulse = M_VSelMsk(M_VCmpLEMsk(RestImpulseMag, StaticFrictionImpulseMag), RestImpulse, DynamicFrictionImpulse);

		//		vec128 FrictionImpulse = M_VMul(T, FrictionImpulseMag);

#if 0
		if (pRBState1->m_bActive)
			pRBState1->AddImpulse(FrictionImpulse, RA);

		if (pRBState2->m_bActive)
			pRBState2->AddImpulse(M_VNeg(FrictionImpulse), RB);
#endif 

	}
}

void CWD_DynamicsWorld::ProcessCollisionsSimpleFriction(TAP_RCD<CWD_ContactInfo> _pContactInfo, fp32 _Epsilon, fp32 _dt)
{
	TAP_RCD<CWD_RigidBodyState> pBodyStates = m_lRigidBodyStates;

	vec128 Epsilon = M_VLdScalar(_Epsilon);
	vec128 Zero = M_VZero();
	// TODO: Som funktion av dt?
#ifdef DYNAMICSENGINE_DEPENETRATION
	vec128 DepenetrationSpeed = M_VScalar(2.0f);
#else
	vec128 DepenetrationSpeed = M_VScalar(0.0f);
#endif

	vec128 VelocityEpsilon = M_VScalar(0.01f);

	vec128 nProcessedEpsilon = M_VZero();

	for (int i = 0; i < _pContactInfo.Len(); i++)
	{
		const CWD_ContactInfo& CI = _pContactInfo[i];

		CWD_RigidBodyState *pRBState1 = &pBodyStates[CI.m_iRB1];
		CWD_RigidBodyState *pRBState2 = &pBodyStates[CI.m_iRB2];

		// TODO: Som parameter...
//		vec128 DynamicFriction = M_VScalar(0.3f);
//		vec128 StaticFriction = M_VScalar(0.4f);
		vec128 Friction = CI.m_Friction_Elasticity;
		vec128 DynamicFriction = M_VSplatX(Friction);
		vec128 StaticFriction = M_VSplatY(Friction);
		vec128 Elasticity = M_VMul(Epsilon, M_VSplatZ(Friction));

		vec128 RA = M_VSub(CI.m_PointOfCollision, pRBState1->m_Position);
		vec128 RB = M_VSub(CI.m_PointOfCollision, pRBState2->m_Position);
		vec128 RelativeVelocity = CWD_DynamicsUtilFunctions::RelativeVelocity(pRBState1, pRBState2, RA, RB, CI.m_Normal);
		vec128 IsApproaching = M_VCmpGEMsk(RelativeVelocity, Zero);
		vec128 IsApproachingEpsilon = M_VCmpGEMsk(RelativeVelocity, VelocityEpsilon);

		vec128 ImpulseMag = CollisionImpulseMag(CI, RelativeVelocity, Elasticity, pRBState1, pRBState2);
		// Only apply impulse if objects are approaching <=> relative veloicty >= 0.0
		//		ImpulseMag = M_VSelMsk(M_VCmpGEMsk(RelativeVelocity, Zero), ImpulseMag, Zero);
		ImpulseMag = M_VSelMsk(IsApproaching, ImpulseMag, Zero);

		nProcessedEpsilon = M_VAdd(nProcessedEpsilon, M_VSelMsk(IsApproachingEpsilon, M_VOne(), Zero));

		vec128 MinDepenetrationSpeed = M_VMul(DepenetrationSpeed, M_VRcp_Est(M_VMax(pRBState1->m_MassInverted, pRBState2->m_MassInverted)));


		vec128 Distance = M_VLdScalar(CI.m_Distance);
		vec128 DepenetrationImpulse = M_VMul(Distance, MinDepenetrationSpeed);
		//		DepenetrationImpulse = M_VSelMsk(M_VCmpGEMsk(RelativeVelocity, Zero), DepenetrationImpulse , Zero);
		DepenetrationImpulse = M_VSelMsk(IsApproaching, DepenetrationImpulse , Zero);
		DepenetrationImpulse = M_VSelComp(3, M_VZero(), DepenetrationImpulse);

		vec128 Impulse = M_VMul(CI.m_Normal, M_VAdd(ImpulseMag, DepenetrationImpulse));
		//vec128 Impulse = M_VMul(CI.m_Normal, ImpulseMag);

		if (pRBState1->m_bActive)
		{
			pRBState1->AddImpulse(Impulse, RA);
			//	pRBState1->AddImpulse(M_VMul(CI.m_Normal,DepenetrationImpulse));
		}

		if (pRBState2->m_bActive)
		{
			pRBState2->AddImpulse(M_VNeg(Impulse), RB);
			//	pRBState2->AddImpulse(M_VMul(CI.m_Normal,M_VNeg(DepenetrationImpulse)));
		}

		// Friction
		//vec128 RelativeTangentVelocity = CWD_DynamicsUtilFunctions::RelativeTangentVelocity(pRBState1, pRBState2, RA, RB, CI.m_Normal);
		vec128 RestImpulse = CI.m_RestImpulse;
		vec128 RestImpulseMag = M_VLen3(RestImpulse);

		//		vec128 T = M_VNrm3(RelativeTangentVelocity);
		vec128 T = CI.m_RelativeTangentVelocityDirection;

		vec128 DynamicFrictionImpulseMag = M_VMul(ImpulseMag, DynamicFriction);
		vec128 StaticFrictionImpulseMag = M_VMul(ImpulseMag, StaticFriction);

		vec128 DynamicFrictionImpulse = M_VMul(T, DynamicFrictionImpulseMag);

		vec128 FrictionImpulse = M_VSelMsk(M_VCmpLEMsk(RestImpulseMag, StaticFrictionImpulseMag), RestImpulse, DynamicFrictionImpulse);

		//		vec128 FrictionImpulse = M_VMul(T, FrictionImpulseMag);

		if (pRBState1->m_bActive)
			pRBState1->AddImpulse(FrictionImpulse, RA);

		if (pRBState2->m_bActive)
			pRBState2->AddImpulse(M_VNeg(FrictionImpulse), RB);
	}
}

void CWD_DynamicsWorld::ProcessCollisions(TAP_RCD<CWD_ContactInfo> _pContactInfo, fp32 _Epsilon, fp32 _dt)
{
	TAP_RCD<CWD_RigidBodyState> pBodyStates = m_lRigidBodyStates;

	vec128 Epsilon = M_VLdScalar(_Epsilon);
	vec128 Zero = M_VZero();
	// TODO: Som funktion av dt?
#ifdef DYNAMICSENGINE_DEPENETRATION
	vec128 DepenetrationSpeed = M_VScalar(0.0f);
#else
	vec128 DepenetrationSpeed = M_VScalar(0.0f);
#endif

	vec128 VelocityEpsilon = M_VScalar(0.01f);

	vec128 nProcessedEpsilon = M_VZero();

	for (int i = 0; i < _pContactInfo.Len(); i++)
	{
 		const CWD_ContactInfo& CI = _pContactInfo[i];

		CWD_RigidBodyState *pRBState1 = &pBodyStates[CI.m_iRB1];
		CWD_RigidBodyState *pRBState2 = &pBodyStates[CI.m_iRB2];

		// TODO: Som parameter...
		//vec128 DynamicFriction = M_VScalar(0.3f);
		//vec128 StaticFriction = M_VScalar(0.4f);
		vec128 Friction = CI.m_Friction_Elasticity;
		vec128 DynamicFriction = M_VSplatX(Friction);
		vec128 StaticFriction = M_VSplatY(Friction);
		vec128 Elasticity = M_VMul(Epsilon, M_VSplatZ(Friction));

		vec128 RA = M_VSub(CI.m_PointOfCollision, pRBState1->m_Position);
		vec128 RB = M_VSub(CI.m_PointOfCollision, pRBState2->m_Position);
		vec128 RelativeVelocity = CWD_DynamicsUtilFunctions::RelativeVelocity(pRBState1, pRBState2, RA, RB, CI.m_Normal);
		vec128 IsApproaching = M_VCmpGEMsk(RelativeVelocity, Zero);
		vec128 IsApproachingEpsilon = M_VCmpGEMsk(RelativeVelocity, VelocityEpsilon);

		vec128 ImpulseMag = CollisionImpulseMag(CI, RelativeVelocity, Elasticity, pRBState1, pRBState2);
		// Only apply impulse if objects are approaching <=> relative veloicty >= 0.0
//		ImpulseMag = M_VSelMsk(M_VCmpGEMsk(RelativeVelocity, Zero), ImpulseMag, Zero);
		ImpulseMag = M_VSelMsk(IsApproaching, ImpulseMag, Zero);

		nProcessedEpsilon = M_VAdd(nProcessedEpsilon, M_VSelMsk(IsApproachingEpsilon, M_VOne(), Zero));

		vec128 MinDepenetrationSpeed = M_VMul(DepenetrationSpeed, M_VRcp_Est(M_VMax(pRBState1->m_MassInverted, pRBState2->m_MassInverted)));


		vec128 Distance = M_VLdScalar(CI.m_Distance);
		vec128 DepenetrationImpulse = M_VMul(Distance, MinDepenetrationSpeed);
//		DepenetrationImpulse = M_VSelMsk(M_VCmpGEMsk(RelativeVelocity, Zero), DepenetrationImpulse , Zero);
		DepenetrationImpulse = M_VSelMsk(IsApproaching, DepenetrationImpulse , Zero);
		DepenetrationImpulse = M_VSelComp(3, M_VZero(), DepenetrationImpulse);

//		vec128 Impulse = M_VMul(CI.m_Normal, M_VAdd(ImpulseMag, DepenetrationImpulse));
		vec128 Impulse = M_VMul(CI.m_Normal, ImpulseMag);

		if (pRBState1->m_bActive)
		{
			pRBState1->AddImpulse(Impulse, RA);
			pRBState1->AddImpulse(M_VMul(CI.m_Normal,DepenetrationImpulse));
		}

		if (pRBState2->m_bActive)
		{
			pRBState2->AddImpulse(M_VNeg(Impulse), RB);
			pRBState2->AddImpulse(M_VMul(CI.m_Normal,M_VNeg(DepenetrationImpulse)));
		}

		// Friction
		vec128 RelativeTangentVelocity = CWD_DynamicsUtilFunctions::RelativeTangentVelocity(pRBState1, pRBState2, RA, RB, CI.m_Normal);
		vec128 RestImpulse = CWD_DynamicsUtilFunctions::RestImpulse(pRBState1, pRBState2, CI, RA, RB);

		vec128 DynamicFrictionImpulse = M_VMul(ImpulseMag, DynamicFriction);
		vec128 StaticFrictionImpulse = M_VMul(ImpulseMag, StaticFriction);

		vec128 FrictionImpulseMag = M_VSelMsk(M_VCmpLEMsk(RestImpulse, StaticFrictionImpulse), RestImpulse, DynamicFrictionImpulse);
		
		vec128 T = M_VNrm3(RelativeTangentVelocity);
		WDYNAMICS_CHECK_VEC128(T);

		vec128 FrictionImpulse = M_VMul(T, FrictionImpulseMag);

		if (pRBState1->m_bActive)
			pRBState1->AddImpulse(FrictionImpulse, RA);

		if (pRBState2->m_bActive)
			pRBState2->AddImpulse(M_VNeg(FrictionImpulse), RB);
	}
}

void CWD_DynamicsWorld::ProcessContacts(fp32 _dt)
{
}

void CWD_DynamicsWorld::CheckStationary(fp32 _dt)
{
	TAP_RCD<CWD_RigidBodyState> pRigidBodyStates = m_lRigidBodyStates;
	TAP_RCD<CWD_RigidBody2 *> pRigidBodies = m_lpRigidBodies;


	CVec4Dfp32 Gravity = m_Gravity;

	for (int i = 0; i < pRigidBodyStates.Len(); i++)
	{
		CWD_RigidBodyState& State = pRigidBodyStates[i];

		CWD_RigidBody2 *pRB = pRigidBodies[i];

		if (!State.m_bActive) continue;

		fp32 FreezeEnergyThreshold = GetFreezeEnergyThreshold(State, Gravity, _dt);
		fp32 KineticEnergy = GetKineticEnergy(State);

		if (pRB->IsConnectedTo(0))
		{
			if(!pRB->m_bUseOriginalFreezeThreshold)
				FreezeEnergyThreshold *= 0.001f;	//This is a hack to make lamps look better, however it will fuck up swing doors
		}

		if (State.m_bStationary)
		{
			if (KineticEnergy > FreezeEnergyThreshold)
			{
				//m_lJustUnfreezedObjects.Add(pRigidBody);

				State.m_bStationary = 0;
				State.m_FreezeCounter = 0;
			}
			else
			{
				State.m_Velocity = CVec4Dfp32(0.0f);
				State.m_AngularVelocity = CVec4Dfp32(0.0f);
			}
		}
		else
		{
			if (KineticEnergy < FreezeEnergyThreshold)
			{
				State.m_FreezeCounter++;
				if (State.m_FreezeCounter > 20)
				{
					State.m_bStationary = 1;
				}
			}
			else 
			{
				State.m_FreezeCounter = 0;
			}
		}
	}

	int nChanged;
	do 
	{
		nChanged = 0;

		for (int i = 0; i < pRigidBodies.Len(); i++)
		{
			CWD_RigidBody2 *pRigidBody = pRigidBodies[i];
			CWD_RigidBodyState& State = pRigidBodyStates[i];

			if (State.m_bStationary)
			{
				for (int j = 0; j < pRigidBody->m_lConnectedTo.Len(); j++)
				{
					int ind = pRigidBody->m_lConnectedTo[j];
					CWD_RigidBodyState &OtherState = pRigidBodyStates[ind];

					if (!OtherState.m_bStationary)
					{
						nChanged++;
						State.m_bStationary = 0;
					}
				}
			}

		}
	} 
	while(nChanged > 0);
}

void CWD_DynamicsWorld::CopyFlags()
{
	TAP_RCD<CWD_RigidBody2 *> pRigidBodies = m_lpRigidBodies;
	TAP_RCD<CWD_RigidBodyState> pRigidBodyStates = m_lRigidBodyStates;

	for (int i = 0; i < pRigidBodies.Len(); i++)
	{
		CWD_RigidBody2 *pRigidBody = pRigidBodies[i];
		CWD_RigidBodyState& pRBState = pRigidBodyStates[i];

		pRigidBody->SetStationary(pRBState.m_bStationary);
		pRigidBody->m_FreezeCounter = pRBState.m_FreezeCounter;
		pRigidBody->m_bActive = pRBState.m_bActive;
	}
}

void CWD_DynamicsWorld::GetWorldInertiaTensorInverted(int _iObject, CMat4Dfp32& _TensorInverted)
{
	CWD_RigidBodyState& State = m_lRigidBodyStates[_iObject];

	CMat4Dfp32 TensorInverted;
	GetTensorInverted(_iObject, _TensorInverted);

	CMat4Dfp32 T = State.m_MatrixOrientation;
	CMat4Dfp32 TT;
	TT.Unit();
	T.Transpose3x3(TT);

	CMat4Dfp32 Tmp;
	//	_TensorInverted = T * TensorInverted * TT;

#if 1
	M_VMatMul(T, TensorInverted, Tmp);
	M_VMatMul(Tmp, TT, _TensorInverted);
#else
	M_VMatMul(TT, TensorInverted, Tmp);
	M_VMatMul(Tmp, T, _TensorInverted);
#endif
}

vec128 CWD_DynamicsWorld::CollisionImpulseMag(const CWD_ContactInfo& _CI, vec128 _RelativeVelocity, vec128 _Epsilon, CWD_RigidBodyState *_pRBState1, CWD_RigidBodyState *_pRBState2)
{
	vec128 Normal = _CI.m_Normal;
	vec128 Denominator = M_VLdScalar(_CI.m_CollisionImpulseDenominator);
	vec128 DenominatorRecip = M_VRcp(Denominator);

	// A = -(1.0 + Epsilon) * RelativeVelocity
	vec128 A = M_VMul(M_VNeg(M_VAdd(M_VOne(), _Epsilon)), _RelativeVelocity);

	vec128 ImpulseMag = M_VMul(A, DenominatorRecip);
	ImpulseMag = M_VSelComp(3, M_VZero(), ImpulseMag);
	// TODO: Fel tecken här tydligen!
	ImpulseMag = M_VNeg(ImpulseMag);

	return ImpulseMag;	 
}

void CWD_DynamicsWorld::SaveFullState()
{
	MSCOPESHORT(CWD_DynamicsWorld::SaveFullState);

	TAP_RCD<CWD_RigidBodyState> pBodyStates = m_lRigidBodyStates;
	TAP_RCD<CWD_RigidBodyState> pBodyStatesSaved = m_lRigidBodyStatesSaved;

	for (int i = 0; i < pBodyStates.Len(); i++)
	{
		// TODO: Här spara man inte m_MatrixOrientation, ska man?
		pBodyStatesSaved[i].m_Position = pBodyStates[i].m_Position;
		pBodyStatesSaved[i].m_Orientation = pBodyStates[i].m_Orientation;
		pBodyStatesSaved[i].m_Velocity = pBodyStates[i].m_Velocity;
		pBodyStatesSaved[i].m_AngularVelocity = pBodyStates[i].m_AngularVelocity;

		//pBodyStatesSaved[i].m_MatrixOrientation = pBodyStates[i].m_MatrixOrientation;
		//pBodyStates[i].m_WorldInertiaTensorInvert = pBodyStatesSaved[i].m_WorldInertiaTensorInvert;
	}
}

void CWD_DynamicsWorld::RestoreFullState()
{
	TAP_RCD<CWD_RigidBodyState> pBodyStates = m_lRigidBodyStates;
	TAP_RCD<CWD_RigidBodyState> pBodyStatesSaved = m_lRigidBodyStatesSaved;

	for (int i = 0; i < pBodyStates.Len(); i++)
	{
		pBodyStates[i].m_Position = pBodyStatesSaved[i].m_Position;
		pBodyStates[i].m_Orientation = pBodyStatesSaved[i].m_Orientation;
		pBodyStates[i].m_Velocity = pBodyStatesSaved[i].m_Velocity;
		pBodyStates[i].m_AngularVelocity = pBodyStatesSaved[i].m_AngularVelocity;

		//pBodyStates[i].m_MatrixOrientation = pBodyStatesSaved[i].m_MatrixOrientation;
		//pBodyStates[i].m_WorldInertiaTensorInvert = pBodyStatesSaved[i].m_WorldInertiaTensorInvert;
	}
}

void CWD_DynamicsWorld::AddExternalForces(fp32 _dt)
{
	MSCOPESHORT(CWD_DynamicsWorld::AddExternalForces);

	TAP_RCD<CWD_RigidBodyState> pBodyStates = m_lRigidBodyStates;
	TAP_RCD<CWD_RigidBody2 *> pRigidBodies = m_lpRigidBodies;

	vec128 dt = M_VLdScalar(_dt);
//	vec128 Gravity = M_VConst(0.0f, -9.81f, 0.0f, 0.0f);
	vec128 Gravity = m_Gravity;

	for (int i = 0; i < pBodyStates.Len(); i++)
	{
		CWD_RigidBodyState& State = pBodyStates[i];
		CWD_RigidBody2 *pRB = pRigidBodies[i];
		pRB->m_CollisionToWorldCluster.Clear();

//		if (!State.m_bStationary && State.m_bActive)
		if (State.m_bActive)
		{
			State.m_Force = M_VZero();
			//State.m_Force = M_VMAdd(Gravity, State.m_Mass, State.m_Force);

			vec128 V = M_VMAdd(Gravity, dt, State.m_Velocity);
			V = M_VMAdd(M_VMul(pRB->m_ExternalForce, State.m_MassInverted), dt, V);
			WDYNAMICS_CHECK_VEC128(V);
			State.m_Velocity = V;

			const CMat4Dfp32& WorldInertiaTensorInvert = State.m_WorldInertiaTensorInvert;
			// TODO: Matrismult. åt fel håll???!??!?! VET EJ!?!?!?!
			WDYNAMICS_CHECK_VEC128(State.m_AngularVelocity);
			State.m_AngularVelocity = M_VMAdd(M_VMulMat(pRB->m_ExternalTorque, WorldInertiaTensorInvert), dt, State.m_AngularVelocity);
			WDYNAMICS_CHECK_VEC128(State.m_AngularVelocity);

			State.m_Torque = M_VZero();
		//	int breakme = 0;
		}
	}
}

void CWD_DynamicsWorld::ClearExternalForces()
{
	TAP_RCD<CWD_RigidBody2 *> pRigidBodies = m_lpRigidBodies;

	for (int i = 0; i < pRigidBodies.Len(); i++)
	{
		CWD_RigidBody2 *pRB = pRigidBodies[i];

		pRB->m_ExternalForce = CVec4Dfp32(0.0f);
		pRB->m_ExternalTorque = CVec4Dfp32(0.0f);
	}
}

void CWD_DynamicsWorld::UpdateVelocity(fp32 _dt)
{
	MSCOPESHORT(CWD_DynamicsWorld::UpdateVelocity);

	TAP_RCD<CWD_RigidBody2 *> pRigidBodies = m_lpRigidBodies;
	TAP_RCD<CWD_RigidBodyState> pBodyStates = m_lRigidBodyStates;

	vec128 dt = M_VLdScalar(_dt);

	for (int i = 0; i < pBodyStates.Len(); i++)
	{
		// TODO: Det är kanske bättre att sätta hastigheten till (0,0,0) om den är stationary och uppdatera ändå!
		// Se även UpdatePosition(.)
		CWD_RigidBodyState& State = pBodyStates[i];
		if (!State.m_bStationary && State.m_bActive)
		{
			const CMat4Dfp32& WorldInertiaTensorInvert = State.m_WorldInertiaTensorInvert;

			WDYNAMICS_CHECK_VEC128(State.m_Force);

			State.m_Velocity = M_VMAdd(State.m_Force, M_VMul(dt, State.m_MassInverted), State.m_Velocity);
			State.m_AngularVelocity = M_VMAdd(M_VMulMat(State.m_Torque, WorldInertiaTensorInvert), dt, State.m_AngularVelocity);

			// Dampening test
			State.m_Velocity = M_VSub(State.m_Velocity, M_VMul(State.m_Velocity, M_VLdScalar(0.001f)));
			State.m_AngularVelocity = M_VSub(State.m_AngularVelocity, M_VMul(State.m_AngularVelocity, M_VLdScalar(0.002f)));

			WDYNAMICS_CHECK_VEC128(State.m_Velocity);
			WDYNAMICS_CHECK_VEC128(State.m_AngularVelocity);
		}
	}
}

void CWD_DynamicsWorld::UpdatePosition(fp32 _dt)
{
	MSCOPESHORT(CWD_DynamicsWorld::UpdatePosition);

	TAP_RCD<CWD_RigidBody2 *> pRigidBodies = m_lpRigidBodies;
	TAP_RCD<CWD_RigidBodyState> pBodyStates = m_lRigidBodyStates;

	vec128 dt = M_VLdScalar(_dt);
	// TODO: Load? Align?
	//vec128 ScaleFactorInv = m_ScaleFactorInv;

	for (int i = 0; i < pBodyStates.Len(); i++)
	{
		CWD_RigidBodyState& State = pBodyStates[i];
		if (!State.m_bStationary && State.m_bActive)
		{
			//			State.m_Position = M_VMAdd(M_VMul(State.m_Velocity, dt), ScaleFactorInv, State.m_Position);
			WDYNAMICS_CHECK_VEC128(State.m_Velocity);

			State.m_Position = M_VMAdd(State.m_Velocity, dt,State.m_Position);
			
			WDYNAMICS_CHECK_VEC128(State.m_Position);
		}
	}
}

void CWD_DynamicsWorld::UpdateOrientation(fp32 _dt)
{
	MSCOPESHORT(CWD_DynamicsWorld::UpdateOrientation);

	TAP_RCD<CWD_RigidBody2 *> pRigidBodies = m_lpRigidBodies;
	TAP_RCD<CWD_RigidBodyState> pBodyStates = m_lRigidBodyStates;

	vec128 dt = M_VLdScalar(_dt);
	vec128 Half = M_VHalf();
	// TODO: Load? Align?

	for (int i = 0; i < pBodyStates.Len(); i++)
	{
		CWD_RigidBodyState& State = pBodyStates[i];
		if (!State.m_bStationary && State.m_bActive)
		{
			vec128 Orientation = State.m_Orientation;
			vec128 V = M_VSelComp(3, M_VZero(), State.m_AngularVelocity);

			vec128 DeltaRot = M_VQuatMul(V, Orientation);
			DeltaRot = M_VMul(DeltaRot, M_VMul(Half, dt));
			//vec128 DeltaRot = M_VMul(M_VMul(V, Orientation), M_VMul(Half, dt));

			Orientation = M_VAdd(Orientation, DeltaRot);
			State.m_Orientation = M_VNrm4(Orientation);

			WDYNAMICS_CHECK_VEC128(State.m_Orientation);

			int brekame = 0;
			//Orientation = M_VMAdd(V, M_VMul(Half, dt), Orientation);
			//
			// TODO: Create State.m_MatrixOrientation
			// TODO: Ska man uppdatera WorldInertiaTensorInvert här också?
		}
	}
}

void CWD_DynamicsWorld::UpdateOrientatioMatrixAndTensor()
{
	MSCOPESHORT(CWD_DynamicsWorld::UpdateOrientatioMatrixAndTensor);

	TAP_RCD<CWD_RigidBodyState> pBodyStates = m_lRigidBodyStates;
	for (int i = 0; i < pBodyStates.Len(); i++)
	{
		CWD_RigidBodyState& State = pBodyStates[i];

		State.UpdateOrientationMatrix();
		State.UpdateWorldInertiaTensorInvert();
	}
}

void CWD_DynamicsWorld::GetTensorInverted(int _iRigidBody, CMat4Dfp32& _TensorInverted)
{
	M_ASSERTHANDLER(IsValidID(_iRigidBody), "Invalid iRigidBody!", return);
	_TensorInverted.Unit();
	CVec4Dfp32 TI = m_lRigidBodyStates[_iRigidBody].m_TensorInverted;
	_TensorInverted.k[0][0] = TI[0];
	_TensorInverted.k[1][1] = TI[1];
	_TensorInverted.k[2][2] = TI[2];
}

const CWD_RigidBodyState& CWD_DynamicsWorld::GetRigidBodyState(uint _iRB) const
{
	if (_iRB >= (uint)m_lRigidBodyStates.Len())
		M_BREAKPOINT;

	return m_lRigidBodyStates[_iRB];
}

CWD_RigidBodyState& CWD_DynamicsWorld::GetRigidBodyState(uint _iRB)
{
	if (_iRB >= (uint)m_lRigidBodyStates.Len())
		M_BREAKPOINT;

	return m_lRigidBodyStates[_iRB];
}

void CWD_DynamicsWorld::AddLocalImpulse(int _iRigidBody, vec128 _Impulse, vec128 _Position)
{
	CWD_RigidBodyState& State = m_lRigidBodyStates[_iRigidBody];
	const CMat4Dfp32& WorldInertiaTensorInvert = State.m_WorldInertiaTensorInvert;

	State.m_Velocity = M_VMAdd(_Impulse, State.m_MassInverted, State.m_Velocity);

	vec128 Tmp = M_VXpd(_Position, _Impulse);
	State.m_AngularVelocity = M_VAdd(State.m_AngularVelocity, M_VMulMat(Tmp, WorldInertiaTensorInvert));
	WDYNAMICS_CHECK_VEC128(State.m_AngularVelocity);
}

void CWD_DynamicsWorld::AddWorldImpulse(int _iRigidBody, vec128 _Impulse, vec128 _Position)
{
	CWD_RigidBodyState& State = m_lRigidBodyStates[_iRigidBody];
	AddLocalImpulse(_iRigidBody, _Impulse, M_VSub(_Position,  State.m_Position));
}

/*
	CWD_Island
 */

TAP_RCD<CWD_RigidBody2 *> CWD_Island::GetBodies() 
{
	return m_pRigidBodies;
}

const TAP_RCD<CWD_RigidBody2 *> CWD_Island::GetBodies() const
{
	return m_pRigidBodies;
}

TAP_RCD<CWD_RigidBodyState> CWD_Island::GetBodyStates()
{
	return m_pRigidBodyStates;
}

const TAP_RCD<CWD_RigidBodyState> CWD_Island::GetBodyStates() const
{
	return m_pRigidBodyStates;
}

