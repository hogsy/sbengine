#include "PCH.h"
#include "WServer_Core.h"
#include "../../Render/MRenderCapture.h"
#include "../../../XR/XRBlockNavInst.h"
#include "MFloat.h"
#include "../WObjects/WObj_Game.h"
#include "../../../MSystem/Misc/MPerfGraph.h"

#include "../WObjects/WObj_PhysCluster.h"

#define DYNAMICSENGINE2
#define WCLIENTSERVER_NOCLIENTMIRRORREFRESH

// -------------------------------------------------------------------
/*#ifndef _DEBUG
#define WSERVER_TRYONREFRESH
#endif*/

//#pragma inline_depth(0)
//#pragma optimize( "", off )

void CWorld_ServerCore::Simulate_SetCurrentLocalFrameFraction(fp32 _LocalFrameFraction)
{
	m_Simulate_LocalFrameFraction = _LocalFrameFraction;
}

void CWorld_ServerCore::SimulateOnTickUp(CPerfGraph* _pPerfGraph)
{
	MAUTOSTRIP(CWorld_ServerCore_SimulateOnTickUp, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::SimulateOnTickUp, WORLD_SERVER);
#ifdef SERVER_STATS
	m_TSimulate += m_Simulate_CPUTime;
	if (_pPerfGraph)
	{
		CMTime TGame = m_Simulate_CPUTime - m_TFunc_IntersectWorld - m_TFunc_IntersectLine - m_TFunc_GetMedium - m_TFunc_Selection;

		fp32 PlotData[5];
		PlotData[0] = m_TFunc_IntersectWorld.GetTime();
		PlotData[1] = m_TFunc_IntersectLine.GetTime();
		PlotData[2] = m_TFunc_GetMedium.GetTime();
		PlotData[3] = m_TFunc_Selection.GetTime();
		PlotData[4] = TGame.Max(CMTime()).GetTime();
		CPixel32 PlotColors[5] = { 0xff003000, 0xff00ff00, 0xff400040, 0xff7f5f10, 0xff5f4f10 };

		_pPerfGraph->Plotn(5, PlotData, PlotColors);
	}

	AccumulateFrameStatistics();

	m_Simulate_CPUTime.Reset();
#endif

	m_SimulationTick++;
	m_SimulationTime += CMTime::CreateFromSeconds(GetGameTickTime());
//	m_World_PendingName = "";
//	m_World_PendingFlags = 0;
}

void GetStartEndID(CControlFrame& _Ctrl, int& _Start, int& _End)
{
	_Start = -1;
	_End = -1;

	int Pos = _Ctrl.GetCmdStartPos();
	if (Pos >= _Ctrl.m_MsgSize)
		return;

	CCmd Cmd;
	_Ctrl.GetCmd(Pos, Cmd);
	_Start = Cmd.m_ID;

	while(Pos < _Ctrl.m_MsgSize)
	{
		_Ctrl.GetCmd(Pos, Cmd);
		_End = Cmd.m_ID;
	}
}

#if 1

#define DYNAMICS_GRAPHICS_DEBUG 

#include "../WDynamicsEngine.h"
#include "../WDynamics.h"
#include "../WDynamicsSupport.h"

#include "../WDynamicsEngine/WDynamicsEngine2.h"

static void RigidBodyStateToPhysCluster(CWPhys_Cluster * _pPC,int _iObject, const CWD_RigidBodyState& _State, fp32 _GameTickTime)
{
	CWPhys_ClusterObject &PCO = _pPC->m_lObjects[_iObject];

	const fp32 PosScale = 32.0f;
	const fp32 MoveVelScale = 32.0f * _GameTickTime;		// [m/s] -> [units/tick]
	const fp32 RotVelScale = (-1.0f/_PI2) * _GameTickTime;	// [rad/s] -> [angle1/tick]

	CMat4Dfp32 T = _State.GetTransform();
//	CVec3Dfp32 Mc = PCO.m_pRB->m_CenterOfMass;
//	Mc.MultiplyMatrix3x3(T);
	CVec3Dfp32 tmp = (T.GetRow(3) * PosScale);// - Mc;
	T.GetRow(3) = tmp;

	CVec3Dfp32 AngVel = _State.GetAngularVelocity();
	AngVel *= RotVelScale;
	fp32 m = AngVel.Length();
	if (m > 0.001f)
		AngVel *= (1.0f / m);

	PCO.m_Transform = T;
	PCO.m_Velocity.m_Move = _State.GetVelocity() * MoveVelScale;
	PCO.m_Velocity.m_Rot = CAxisRotfp32(AngVel,m);
}

static void RigidBodyStateToObject(CWorld_ServerCore *_pServer, CWObject *_pObj, const CWD_RigidBodyState& _State, fp32 _GameTickTime)
{
	const fp32 PosScale = 32.0f;
	const fp32 MoveVelScale = 32.0f * _GameTickTime;		// [m/s] -> [units/tick]
	const fp32 RotVelScale = (-1.0f/_PI2) * _GameTickTime;	// [rad/s] -> [angle1/tick]

	CMat4Dfp32 T = _State.GetTransform();
	CVec3Dfp32 Mc = _pObj->m_pRigidBody2->m_CenterOfMass;
	Mc.MultiplyMatrix3x3(T);
	CVec3Dfp32 tmp = (T.GetRow(3) * PosScale) - Mc;
//	tmp *= PosScale;
	T.GetRow(3) = tmp;

	/*CMat4Dfp32 Tinv;
	T.InverseOrthogonal(Tinv);
	T.GetRow(3) = (_pObj->m_pRigidBody2->m_CenterOfMass * Tinv) * PosScale;*/

	/*CVec3Dfp32 Pos = T.GetRow(3);
	T.GetRow(3) = CVec3Dfp32(0.0f);
	CVec3Dfp32 Offset = _pObj->m_pRigidBody2->m_CenterOfMass * (1.0f / 32.0f);
	Offset *= T;
	T.GetRow(3) = Pos - Offset;
	T.GetRow(3) *= PosScale;
	*/
	
	CVec3Dfp32 AngVel = _State.GetAngularVelocity();
	AngVel *= RotVelScale;
	fp32 m = AngVel.Length();
	if (m > 0.001f)
		AngVel *= (1.0f / m);

	int iObj = _pObj->m_iObject;
	_pServer->Object_SetPositionNoIntersection(iObj, T);
	_pServer->Object_SetVelocity(iObj, _State.GetVelocity() * MoveVelScale);
	_pServer->Object_SetRotVelocity(iObj, CAxisRotfp32(AngVel, m));
}

class CDynamicsWireDebugRenderer : public CWD_DynamicsDebugRenderer
{
public:
	CDynamicsWireDebugRenderer(CWireContainer *_pWireContainer)
	{
		m_pWireContainer = _pWireContainer;
	}

	virtual void RenderVector(const CVec3Dfp32& _P, const CVec3Dfp32& _V)
	{
		CPixel32 C=CPixel32::From_fp32(m_Colour[0], m_Colour[1], m_Colour[2]);
		
		m_pWireContainer->RenderVector(_P * 32.0f, _V * 8.0f, C, 1.0f / 10.0f, false);
	}

	CWireContainer *m_pWireContainer;
};


static CVec4Dfp32 ScaleFirstThree(const CVec4Dfp32& _V, fp32 _ScaleFactor)
{
	return CVec4Dfp32(_V[0] * _ScaleFactor, _V[1] * _ScaleFactor, _V[2] * _ScaleFactor, _V[3]);
}

void CWorld_ServerCore::AddConstraintToSimulation(CWD_ConstraintDescriptor& _ConstraintDesc)
{
	uint32 iObjectA, iObjectB;
	uint32 iSubA,iSubB;
	_ConstraintDesc.GetConnectedObjects(iObjectA, iObjectB);
	_ConstraintDesc.GetConnectedSubObjects(iSubA,iSubB);

	CWObject *pObjA = NULL;
	CWObject *pObjB = NULL;

	CWObject** lpObjects = reinterpret_cast<CWObject**>(m_lspObjects.GetBasePtr());

	pObjA = lpObjects[iObjectA];
	pObjB = lpObjects[iObjectB];

	// TODO: This should never happen!
	if (pObjA && pObjA->m_pRigidBody2 == NULL && pObjA->m_pPhysCluster == NULL)
	{
		//			M_BREAKPOINT;
		return;
	}

	// TODO: This should never happen!
	if (pObjB && pObjB->m_pRigidBody2 == NULL && pObjB->m_pPhysCluster == NULL)
	{
		//			M_BREAKPOINT;
		return;
	}

	CWD_RigidBody2 * pRBA=NULL,*pRBB=NULL;
	if( pObjA ) pRBA = (pObjA->m_pPhysCluster) ? pObjA->m_pPhysCluster->m_lObjects[iSubA].m_pRB : pObjA->m_pRigidBody2;
	if( pObjB ) pRBB = (pObjB->m_pPhysCluster) ? pObjB->m_pPhysCluster->m_lObjects[iSubB].m_pRB : pObjB->m_pRigidBody2;

	if (pObjA && pObjB)
	{
		if (pRBA->IsStationary() && pRBB->IsStationary())
			return;

		// TODO: Temp hack due to lack of contact-graph!!!
		/*			if (pObjA->m_pRigidBody2->m_FreezeCounter > 20 && pObjB->m_pRigidBody2->m_FreezeCounter > 20 )
		{
		continue;			
		}
		*/
	}
	else if (pObjA && pRBA->IsStationary())
	{
		return;
	}
	else if (pObjB && pRBB->IsStationary())
	{
		return;
	}

	if (pObjA && (!pRBA->m_bInSimulation))
	{
		pRBA->SetStationary(false);
		Phys_AddRigidBodyToSimulation(iObjectA, false);
	}

	if (pObjB && (!pRBB->m_bInSimulation))
	{
		pRBB->SetStationary(false);
		Phys_AddRigidBodyToSimulation(iObjectB, false);
	}

	int iRBA = 0;
	int iRBB = 0;

	if (pObjA)
	{
		iRBA = pRBA->m_iRB;
		pRBA->SetStationary(false);
	}

	if (pObjB)
	{
		iRBB = pRBB->m_iRB;
		pRBB->SetStationary(false);
	}

	if (pObjA) 
	{
		pRBA->AddConnection(iRBB);
		pRBA->m_bUseOriginalFreezeThreshold = _ConstraintDesc.UseOriginalFreezeThreshold();
	}
	if (pObjB) 
	{
		pRBB->AddConnection(iRBA);
		pRBB->m_bUseOriginalFreezeThreshold = _ConstraintDesc.UseOriginalFreezeThreshold();
	}


	_ConstraintDesc.SetInSimulation(true);
	fp32 InvScale = 1.0f / 32.0f;

	switch (_ConstraintDesc.GetType())
	{
	case CWD_ConstraintDescriptor::BALL:
		{
			m_DynamicsWorld2.AddConstraint(CWD_BallJoint(iRBA, iRBB, 
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale), 
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RB), InvScale),
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale), 
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RB), InvScale)));															
		}
		break;

	case CWD_ConstraintDescriptor::BALLWORLD:
		{
			m_DynamicsWorld2.AddConstraint(CWD_BallJointWorld(iRBA, 
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale),
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale),
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::WORLDREF), InvScale)));

		}
		break;

	case CWD_ConstraintDescriptor::HINGEWORLD:
		{
			m_DynamicsWorld2.AddConstraint(CWD_HingeJointWorld(&m_DynamicsWorld2, iRBA, 
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale),
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::AXISA), InvScale),
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::WORLDREF), InvScale),
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::WORLDAXIS), InvScale),
				_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::ANGLEAXISA),
				_ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::MAXANGLE) * _PI2
				));

		}
		break;

	case CWD_ConstraintDescriptor::HINGEWORLD2:
		{
			m_DynamicsWorld2.AddConstraint(CWD_HingeJointWorld2(&m_DynamicsWorld2, iRBA, 
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale),
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::AXISA), InvScale),
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::WORLDREF), InvScale),
				_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::ANGLEAXISA),
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::WORLDAXIS), InvScale),
				_ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::RELATIVEANGLE),
				_ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::MINANGLE) * _PI2,
				_ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::MAXANGLE) * _PI2
				));

		}
		break;

	case CWD_ConstraintDescriptor::HINGE:
		{
			m_DynamicsWorld2.AddConstraint(CWD_HingeJoint(&m_DynamicsWorld2, iRBA, iRBB, 
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale), 
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::AXISA), InvScale), 
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::ANGLEAXISA), InvScale), 
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RB), InvScale),
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::AXISB), InvScale),
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::ANGLEAXISB), InvScale),
				_ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::RELATIVEANGLE),
				_ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::MAXANGLE) * _PI2 
				));

		}
		break;

	case CWD_ConstraintDescriptor::HINGE2:
		{
			m_DynamicsWorld2.AddConstraint(CWD_HingeJoint2(&m_DynamicsWorld2, iRBA, iRBB, 
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale), 
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::AXISA), InvScale), 
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RB), InvScale),
				ScaleFirstThree(_ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::AXISB), InvScale),
				_ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::RELATIVEANGLE),
				_ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::MINANGLE) * _PI2,
				_ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::MAXANGLE) * _PI2 
				));

		}
		break;


	default:
		M_ASSERT(false, "!");
	}
}

void CWorld_ServerCore::SimulatePhysics2()
{
	MSCOPESHORT(CWorld_ServerCore::SimulatePhysics2);
	M_NAMEDEVENT("SimulatePhysics2", 0xff000000);

	for(int iClient = 0; iClient < m_lspClients.Len(); iClient++)
	{
		if (m_lspClientInfo[iClient])
		{
			CWObject_Game *pGame = Game_GetObject();
			CWO_Player* pP = pGame ? pGame->Player_Get(m_lspClientInfo[iClient]->m_iPlayer) : NULL;
			if (!pP || pP->m_spCmdQueue == NULL) continue;

			int iObj = pP->m_iObject;
			CWObject* pObj = Object_Get(iObj);

			if (pObj)
			{
				if (pObj->m_pRigidBody2 == NULL || 
					((pObj->m_pPhysCluster != NULL) && (pObj->m_pPhysCluster->m_lObjects[0].m_pRB == NULL)) )
				{
					pObj->SetMass(80.0f);
					Object_InitRigidBody(pP->m_iObject, false);
					if (!pObj->m_pRigidBody2)
						continue;
				}
				Phys_SetStationary(pP->m_iObject, false);
				if( pObj->m_pRigidBody2 ) pObj->m_pRigidBody2->m_FreezeCounter = 0;
				else if( pObj->m_pPhysCluster )
				{
					TAP<CWPhys_ClusterObject> pPCO = pObj->m_pPhysCluster->m_lObjects;
					for(int i = 0;i < pPCO.Len();i++)
					{
						pPCO[i].m_pRB->SetStationary(false);
						pPCO[i].m_pRB->m_FreezeCounter = 0;
					}
					m_ObjPoolDynamics.Insert(pObj->m_iObject);
				}
				//Phys_AddRigidBodyToSimulation(pP->m_iObject, false);
			}
		}
	}

	const int MAX_RIGIDBODIES = 1000;
	uint16 liObjects[MAX_RIGIDBODIES];
	int nObjects = m_ObjPoolDynamics.EnumAll(liObjects, MAX_RIGIDBODIES);

	fp32 GameTickTime = GetGameTickTime();
	fp32 GameTickTimeInv = 1.0f / GameTickTime;

	m_DynamicsWorld2.Clear();

	CWObject** lpObjects = reinterpret_cast<CWObject**>(m_lspObjects.GetBasePtr());
	for (int i = 0; i < nObjects; i++)
	{
		int iObj = liObjects[i];
		CWObject* pObj = lpObjects[iObj];
//		if (pObj && pObj->m_pRigidBody2 && pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_PHYSICSCONTROLLED)
		if (pObj && pObj->m_pRigidBody2)
		{
			if (!pObj->m_pRigidBody2->IsStationary())
				Phys_AddRigidBodyToSimulation(iObj, false);
		}
		else if (pObj && pObj->m_pPhysCluster)
		{

#ifdef DEBUG_RENDER_PHYSCLUSTER_OBJECTS

			for(int i = 0;i < pObj->m_pPhysCluster->m_lObjects.Len();i++)
			{
				CWPhys_ClusterObject &PCO = pObj->m_pPhysCluster->m_lObjects[i];
				CPixel32 Bright = PCO.m_pRB->IsStationary() ? CPixel32(255,0,0,255) : CPixel32(0,255,0,255);
				if( PCO.m_PhysPrim.m_PrimType == OBJECT_PRIMTYPE_CAPSULE )
				{
					CMat4Dfp32 Mt = PCO.m_Transform;
					CVec3Dfp32 Dim = PCO.m_PhysPrim.GetDim();
					CVec3Dfp32 Offs;
					CVec3Dfp32 Lines[8];

					Lines[0] = Lines[4] = CVec3Dfp32(0,Dim.k[1],0);
					Lines[1] = Lines[5] = CVec3Dfp32(0,0,Dim.k[1]);
					Lines[2] = Lines[6] = CVec3Dfp32(0,-Dim.k[1],0);
					Lines[3] = Lines[7] = CVec3Dfp32(0,0,-Dim.k[1]);

					Offs = Mt.GetRow(0);
					Offs *= Dim.k[0];
					Mt.GetRow(3) += Offs;
					Debug_RenderSphere(Mt,Dim.k[1],Bright,0.1f,false);
					Lines[0].MultiplyMatrix(Mt);
					Lines[1].MultiplyMatrix(Mt);
					Lines[2].MultiplyMatrix(Mt);
					Lines[3].MultiplyMatrix(Mt);
					Mt.GetRow(3) -= Offs * 2.0f;
					Debug_RenderSphere(Mt,Dim.k[1],Bright,0.1f,false);
					Lines[4].MultiplyMatrix(Mt);
					Lines[5].MultiplyMatrix(Mt);
					Lines[6].MultiplyMatrix(Mt);
					Lines[7].MultiplyMatrix(Mt);

					Debug_RenderWire(Lines[0],Lines[4],Bright,0.1f,false);
					Debug_RenderWire(Lines[1],Lines[5],Bright,0.1f,false);
					Debug_RenderWire(Lines[2],Lines[6],Bright,0.1f,false);
					Debug_RenderWire(Lines[3],Lines[7],Bright,0.1f,false);
				}
				else if( PCO.m_PhysPrim.m_PrimType == OBJECT_PRIMTYPE_BOX )
					Debug_RenderOBB(PCO.m_Transform,PCO.m_PhysPrim.GetDim(),Bright,0.1f,false);
			}
#endif

			Phys_AddRigidBodyToSimulation(iObj,false);
		}
	}

	TAP_RCD<CWD_ConstraintDescriptor> pConstraints = m_lConstraints;

	for (int i = 0; i < pConstraints.Len(); i++)
	{
		const CWD_ConstraintDescriptor& ConstraintDesc = pConstraints[i];

		uint32 iObjectA, iObjectB;
		uint32 iSubA,iSubB;
		ConstraintDesc.GetConnectedObjects(iObjectA, iObjectB);
		ConstraintDesc.GetConnectedSubObjects(iSubA, iSubB);

		CWObject *pObjA = NULL;
		CWObject *pObjB = NULL;

		pObjA = lpObjects[iObjectA];
		pObjB = lpObjects[iObjectB];

		// TODO: This should never happen!
		if (pObjA && pObjA->m_pRigidBody2 == NULL && pObjA->m_pPhysCluster == NULL)
		{
//			M_BREAKPOINT;
			continue;
		}

		// TODO: This should never happen!
		if (pObjB && pObjB->m_pRigidBody2 == NULL && pObjB->m_pPhysCluster == NULL)
		{
//			M_BREAKPOINT;
			continue;
		}

		CWD_RigidBody2 * pRBA = NULL,*pRBB = NULL;
		if( pObjA ) pRBA = (pObjA->m_pPhysCluster) ? pObjA->m_pPhysCluster->m_lObjects[iSubA].m_pRB : pObjA->m_pRigidBody2;
		if( pObjB ) pRBB = (pObjB->m_pPhysCluster) ? pObjB->m_pPhysCluster->m_lObjects[iSubB].m_pRB : pObjB->m_pRigidBody2;
		
		if (pObjA && pObjB)
		{
			if (pRBA->IsStationary() && pRBB->IsStationary())
				continue;

			// TODO: Temp hack due to lack of contact-graph!!!
/*			if (pObjA->m_pRigidBody2->m_FreezeCounter > 20 && pObjB->m_pRigidBody2->m_FreezeCounter > 20 )
			{
				continue;			
			}
			*/
		}
		else if (pObjA && pRBA->IsStationary())
		{
			continue;
		}
		else if (pObjB && pRBB->IsStationary())
		{
			continue;
		}

		if (pObjA && (!pRBA->m_bInSimulation))
		{
			pRBA->SetStationary(false);
			Phys_AddRigidBodyToSimulation(iObjectA, false);
		}

		if (pObjB && (!pRBB->m_bInSimulation))
		{
			pRBB->SetStationary(false);
			Phys_AddRigidBodyToSimulation(iObjectB, false);
		}

		int iRBA = 0;
		int iRBB = 0;

		if (pObjA)
		{
			iRBA = pRBA->m_iRB;
			pRBA->SetStationary(false);
		}

		if (pObjB)
		{
			iRBB = pRBB->m_iRB;
			pRBB->SetStationary(false);
		}

		if (pObjA) pRBA->AddConnection(iRBB);
		if (pObjB) pRBB->AddConnection(iRBA);

/*
		if (ConstraintDesc.GetType() == CWD_ConstraintDescriptor::BALL)
		{
			if (!m_DynamicsWorld2.IsValidID(iRBA))
			{
				int breakme = 0;
			}

			if (!m_DynamicsWorld2.IsValidID(iRBB))
			{
				int breakme = 0;
			}


			CWD_RigidBodyState Foo1 = m_DynamicsWorld2.GetRigidBodyState(iRBA);
			CWD_RigidBodyState Foo2 = m_DynamicsWorld2.GetRigidBodyState(iRBB);
		}
*/

		/*
		if (pObjA->m_pRigidBody2->IsStationary())
		{
			pObjA->m_pRigidBody2->m_bStationary = false;
		}

		if (pObjB->m_pRigidBody2->IsStationary())
		{
			pObjB->m_pRigidBody2->m_bStationary = false;
		}
*/

		fp32 InvScale = 1.0f / 32.0f;

		switch (ConstraintDesc.GetType())
		{
			case CWD_ConstraintDescriptor::BALL:
			{
				m_DynamicsWorld2.AddConstraint(CWD_BallJoint(iRBA, iRBB, 
															 ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale), 
															 ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RB), InvScale),
															 ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale), 
															 ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RB), InvScale)));															
			}
			break;

			case CWD_ConstraintDescriptor::BALLWORLD:
			{
				m_DynamicsWorld2.AddConstraint(CWD_BallJointWorld(iRBA, 
																  ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale),
																  ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale),
																  ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::WORLDREF), InvScale)));
																	
			}
			break;

			case CWD_ConstraintDescriptor::HINGEWORLD:
			{
				m_DynamicsWorld2.AddConstraint(CWD_HingeJointWorld(&m_DynamicsWorld2, iRBA, 
																   ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale),
																   ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::AXISA), InvScale),
																   ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::WORLDREF), InvScale),
																   ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::WORLDAXIS), InvScale),
																   ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::ANGLEAXISA),
																   ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::MAXANGLE) * _PI2
																   ));
																   
			}
			break;

			case CWD_ConstraintDescriptor::HINGEWORLD2:
			{
				m_DynamicsWorld2.AddConstraint(CWD_HingeJointWorld2(&m_DynamicsWorld2, iRBA, 
																   ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale),
																   ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::AXISA), InvScale),
																   ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::WORLDREF), InvScale),
																   ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::ANGLEAXISA),
																   ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::WORLDAXIS), InvScale),
																   ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::RELATIVEANGLE),
																   ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::MINANGLE) * _PI2,
																   ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::MAXANGLE) * _PI2
																   ));
																   
			}
			break;

			case CWD_ConstraintDescriptor::HINGE:
			{
				m_DynamicsWorld2.AddConstraint(CWD_HingeJoint(&m_DynamicsWorld2, iRBA, iRBB, 
															  ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale), 
															  ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::AXISA), InvScale), 
															  ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::ANGLEAXISA), InvScale), 
															  ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RB), InvScale),
															  ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::AXISB), InvScale),
															  ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::ANGLEAXISB), InvScale),
															  ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::RELATIVEANGLE),
															  ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::MAXANGLE) * _PI2 
															  ));

			}
			break;

			case CWD_ConstraintDescriptor::HINGE2:
			{
				m_DynamicsWorld2.AddConstraint(CWD_HingeJoint2(&m_DynamicsWorld2, iRBA, iRBB, 
															  ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RA), InvScale), 
															  ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::AXISA), InvScale), 
															  ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::RB), InvScale),
															  ScaleFirstThree(ConstraintDesc.GetVectorParam(CWD_ConstraintDescriptor::AXISB), InvScale),
															  ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::RELATIVEANGLE),
															  ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::MINANGLE) * _PI2,
															  ConstraintDesc.GetScalarParam(CWD_ConstraintDescriptor::MAXANGLE) * _PI2 
															  ));

			}
			break;


			default:
				M_ASSERT(false, "!");
		}
	}

	const int nSteps = 8;
	m_DynamicsWorld2.Simulate(GameTickTime, nSteps, (CWorld_Server *) this, NULL);

	for (int i = 0; i < pConstraints.Len(); i++)
	{
		pConstraints[i].SetInSimulation(false);
	}


#ifndef M_RTM
	CWireContainer *pWireContainer = Debug_GetWireContainer();
	if (pWireContainer)
	{
		CDynamicsWireDebugRenderer DebugRenderer(pWireContainer);
		m_DynamicsWorld2.SetDebugRenderer(&DebugRenderer);
		m_DynamicsWorld2.RenderDebugInfo(); 
	}
#endif

	nObjects = m_ObjPoolDynamics.EnumAll(liObjects, MAX_RIGIDBODIES);
	for (int i = 0; i < nObjects; i++)
	{
		int iObj = liObjects[i];
		CWObject* pObj = lpObjects[iObj];
		if (pObj && (pObj->m_pRigidBody2 || pObj->m_pPhysCluster))   // && pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_PHYSICSCONTROLLED)
		{
			if( pObj->m_pPhysCluster )
			{
				CWPhys_Cluster * pPC = pObj->m_pPhysCluster;
				int nPhys = pPC->m_lObjects.Len();
				CWPhys_ClusterObject *pPO = pObj->m_pPhysCluster->m_lObjects.GetBasePtr();
				uint nObj = 0;
				for(int i = 0;i < nPhys;i++)
				{
					if( !pPO[i].m_pRB->m_bInSimulation ) continue;
					if (!(pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_NODYNAMICSUPDATE))
					{
						const CWD_RigidBodyState& State = m_DynamicsWorld2.GetRigidBodyState(pPO[i].m_pRB->m_iRB);
						RigidBodyStateToPhysCluster(pPC,i,State,GameTickTime);
					}
					pPO[i].m_pRB->m_iRB = 0xFFFFFFFF;
					pPO[i].m_pRB->m_bInSimulation = false;
					pPO[i].m_pRB->ClearConnections();
					if(!pPO[i].m_pRB->IsStationary()) nObj++;
				}

				if( nObj )
					m_ObjPoolDynamics.Insert(iObj);
				else
					m_ObjPoolDynamics.Remove(iObj);

				if( pObj->m_pPhysCluster->m_Flags & PHYSCLUSTER_SKIPFIRSTTRANSFORM )
					pObj->m_pPhysCluster->m_Flags &= ~PHYSCLUSTER_SKIPFIRSTTRANSFORM;
				else
					Object_SetPositionNoIntersection(pObj->m_iObject,pPO[0].m_Transform.GetRow(3));
			}
			else if( pObj->m_pRigidBody2->m_bInSimulation )
			{
				CWD_RigidBody2* pRB = pObj->m_pRigidBody2;

				M_ASSERT(m_DynamicsWorld2.IsValidID(pRB->m_iRB), "Invalid rigid body in m_ObjPoolDynamics!");
			
				if (!(pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_NODYNAMICSUPDATE))
				{
					const CWD_RigidBodyState& State = m_DynamicsWorld2.GetRigidBodyState(pObj->m_pRigidBody2->m_iRB);
					RigidBodyStateToObject(this, pObj, State, GameTickTime);
				}
				pObj->m_pRigidBody2->m_iRB = 0xffffffff;
				pRB->m_iRB = 0xffffffff; // reset ID
				pRB->m_bInSimulation = 0;
				Object_SetPositionNoIntersection(pObj->m_iObject, pObj->GetPositionMatrix());
				pRB->ClearConnections();

				if (pRB->IsStationary())
					m_ObjPoolDynamics.Remove(iObj);
				else
					m_ObjPoolDynamics.Insert(iObj);
			}
		}
	}

	const TArray<CWD_CollisionEvent>& CollisionEvents = m_DynamicsWorld2.GetCollisionEvents();
	TAP_RCD<const CWD_CollisionEvent> pCollisionEvents = CollisionEvents;

	CCollisionInfo CInfo;
	for (int i = 0; i < pCollisionEvents.Len(); i++)
	{
		const CWD_CollisionEvent& Event = pCollisionEvents[i];
		CWObject* pObjA = Object_Get(Event.m_UserData1);
		CWObject* pObjB = Object_Get(Event.m_UserData2);

		CInfo.Clear();
		CInfo.m_Pos = Event.m_PointOfCollision * 32.0f;
		CInfo.m_Velocity.k[0] = Event.m_ImpulseTimesInvMass;						// Impact amount
		CInfo.m_Velocity.k[1] = Event.m_Velocity1.Distance(Event.m_Velocity2);		// Magnitude of relative velocities between objects
		CInfo.m_Velocity.k[2] = Event.m_Velocity1 * Event.m_Velocity2;				// Dot product of relative velocities between objects
	
		if (pObjA)
		{
			CInfo.m_LocalNodePos.GetRow(0) = Event.m_Velocity2;		// HACK: reuse "m_LocalNodePos"
			pObjA->m_pRTC->m_pfnOnPhysicsEvent(pObjA, pObjB, this, CWO_PHYSEVENT_DYNAMICS_COLLISION, 0.0f, 0.0f, NULL, &CInfo);
		}
		if (pObjB)
		{
			CInfo.m_LocalNodePos.GetRow(0) = Event.m_Velocity1;		// HACK: reuse "m_LocalNodePos"
			pObjB->m_pRTC->m_pfnOnPhysicsEvent(pObjB, pObjA, this, CWO_PHYSEVENT_DYNAMICS_COLLISION, 0.0f, 0.0f, NULL, &CInfo);
		}
	}

}

void CWorld_ServerCore::SimulatePhysics()
{
#ifndef DYNAMICSENGINE2

	MSCOPESHORT(CWorld_ServerCore::SimulatePhysics);

  #define MAX_RIGIDBODIES 1000	
	uint16 liObjects[MAX_RIGIDBODIES];
	int nObjects = m_ObjPoolDynamics.EnumAll(liObjects, MAX_RIGIDBODIES);
  #undef MAX_RIGIDBODIES

	CWObject** lpObjects = reinterpret_cast<CWObject**>(m_lspObjects.GetBasePtr());
	for (int i = 0; i < nObjects; i++)
	{
		int iObj = liObjects[i];
		CWObject* pObj = lpObjects[iObj];
		if (pObj && pObj->m_pRigidBody && pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_PHYSICSCONTROLLED)
		{
			CRigidBody *pRB = pObj->m_pRigidBody;
			pRB->SetMass(pObj->GetMass());
  #ifndef	DYNAMICS_KEEP_STATE
			const CMat4Dfp32& mat = pObj->GetPositionMatrix();
			CVec3Dfp32 pos = pRB->m_MassCenter * mat;
			pRB->SetPosition(pos.Getfp64());
			CQuatfp32 orient;
			mat.Transpose3x3();
			orient.Create(mat);
			pRB->SetOrientaion(ConvertQuat(orient));
  #endif
		}
	}

	CWO_DynamicsDebugRenderer* pDebugRenderer = NULL;
  #ifdef DYNAMICS_GRAPHICS_DEBUG
	CWO_DynamicsDebugRenderer DebugRenderer(this);
	pDebugRenderer = &DebugRenderer;
  #endif

  #define DYNAMICS_N_STEPS 4
//	const fp64 dt = 0.012f;
	const fp64 dt = 0.05f;
	if (nObjects > 0)
	{
		m_DynamicsWorld.Simulate(dt, DYNAMICS_N_STEPS, (CMapData *) m_spMapData, (CWorld_PhysState *) this, pDebugRenderer);

//		for (int i=0; i<DYNAMICS_N_STEPS; i++)
//			m_DynamicsWorld.Step(dt/DYNAMICS_N_STEPS, (CMapData *) m_spMapData, (CWorld_PhysState *) this, pDebugRenderer);
	}

	const TArray<CCollisionEvent>& CollisionEvents = m_DynamicsWorld.GetCollisionEvents();
	TAP_RCD<const CCollisionEvent> pCollisionEvents = CollisionEvents;

	CCollisionInfo CInfo;
	for (int i = 0; i < pCollisionEvents.Len(); i++)
	{
		const CCollisionEvent& Event = pCollisionEvents[i];
		CWObject* pObjA = Object_Get(Event.m_UserData1);
		CWObject* pObjB = Object_Get(Event.m_UserData2);

		CInfo.Clear();
		CInfo.m_Pos = Event.m_PointOfCollision;
		CInfo.m_Velocity.k[0] = Event.m_ImpulseTimesInvMass;

		if (pObjA)
			pObjA->m_pRTC->m_pfnOnPhysicsEvent(pObjA, pObjB, this, CWO_PHYSEVENT_DYNAMICS_COLLISION, 0.0f, 0.0f, NULL, &CInfo);
		if (pObjB)
			pObjB->m_pRTC->m_pfnOnPhysicsEvent(pObjB, pObjA, this, CWO_PHYSEVENT_DYNAMICS_COLLISION, 0.0f, 0.0f, NULL, &CInfo);
	}

  #undef DYNAMICS_N_STEPS

	fp32 MoveVelScale = 32.0f * GetGameTickTime();		// [m/s] -> [units/tick]
	fp32 RotVelScale = (-1.0f/_PI2) * GetGameTickTime(); // [rad/s] -> [angle1/tick]
	for (int i = 0; i < nObjects; i++)
	{
		int iObj = liObjects[i];
		CWObject* pObj = lpObjects[iObj];
		if (pObj && pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_PHYSICSCONTROLLED)
		{
			CRigidBody* pRB = pObj->m_pRigidBody;
			if (pRB && !pRB->IsStationary())
			{
				CVec3Dfp32 pos = pRB->GetPosition().Get<fp32>();

				CQuatfp64 orient = pRB->GetOrientation();
				CQuatfp32 orient2;
				orient2.k[0] = orient.k[0];
				orient2.k[1] = orient.k[1];
				orient2.k[2] = orient.k[2];
				orient2.k[3] = orient.k[3];

				CMat4Dfp32 Mat; 
				Mat.UnitNot3x3();
				orient2.CreateMatrix3x3(Mat);
				Mat.Transpose3x3();
				CVec3Dfp32 offset = pRB->m_MassCenter.Get<fp32>();
				offset *= Mat;
				pos -= offset;
				Mat.GetRow(3) = pos;
				Object_SetPositionNoIntersection(iObj, Mat);

				CVec3Dfp32 Vel = pRB->GetVelocity().Get<fp32>();
				Vel *= MoveVelScale;
				pObj->m_PhysVelocity.m_Move = Vel;

				CVec3Dfp32 AngVel = pRB->GetAngularVelocity().Get<fp32>();
				AngVel *= RotVelScale;
				fp32 m = AngVel.Length();
				if (m > 0.01f)
					AngVel *= (1.0f / m);
				pObj->m_PhysVelocity.m_Rot.m_Angle = m;
				pObj->m_PhysVelocity.m_Rot.m_Axis = AngVel;
			}
		}
	}
#endif // !DYNAMICSENGINE2
}

#endif

#ifdef PLATFORM_XENON
bool bTraceDynamics = false;
#include "xtl.h"
#include "tracerecording.h"
#endif


bool CWorld_ServerCore::Simulate(CPerfGraph* _pPerfGraph)
{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	if (MRTC_GetRD()->m_EnableFlags)
		MRTC_GetRD()->SendData(ERemoteDebug_NextServerTick, 0, 0, false, false);
#endif

	MAUTOSTRIP(CWorld_ServerCore_Simulate, false);
	MSCOPE(CWorld_ServerCore::Simulate, WORLD_SERVER);
	M_NAMEDEVENT("Simulate", 0xff000000);

	CWObject** lspObjects = reinterpret_cast<CWObject**>(m_lspObjects.GetBasePtr());

	if (m_bNetShowMsg) ConOutL("(CWorld_ServerCore::Simulate)");
	bool bHalted = false;

	// Init phys capture-renderer?
	if (m_PhysRenderFlags)
	{
#ifndef	PLATFORM_PS2
		MSCOPESHORT(m_PhysRenderFlags);
		if (!m_spCapture)
		{
			MRTC_SAFECREATEOBJECT_NOEX(spRC, "CRenderContextCapture", CRenderContextCapture);
			m_spCapture = spRC;
			if (!m_spCapture)
			{
				ConOut("Unable to create capture-renderer.");
				m_PhysRenderFlags = 0;
			}
			else
				m_spCapture->Create(NULL, NULL);

		}

		if (m_spCapture)
		{
			CRC_Viewport VP;
			m_spCapture->BeginScene(&VP);
			if (m_PhysRenderFlags & 1)
				m_spPhysCapture = m_spCapture;
		}
#endif

		if (!m_spWireContainer)
		{
			MRTC_SAFECREATEOBJECT_NOEX(spWC, "CDebugRenderContainer", CDebugRenderContainer);
			m_spWireContainer = spWC;
			if (!m_spWireContainer)
			{
				ConOut("Unable to create wire-container.");
			}
			else
			{
				CRC_Font* pFont = m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("MONOPRO"));
				m_spWireContainer->Create(8192+32, 64, pFont);
				MRTC_GOM()->RegisterObject((CReferenceCount*)m_spWireContainer, "GAMECONTEXT.SERVER.WIRECONTAINER");
			}
		}

		if (m_spWireContainer)
		{
			m_spWireContainer->Refresh(GetGameTickTime());
		}
	}
	else
	{
		MSCOPESHORT(NoPhysRenderFlags);
		if (m_spWireContainer)
			MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spWireContainer, "GAMECONTEXT.SERVER.WIRECONTAINER");

		m_spCapture = NULL;
		m_spPhysCapture = NULL;
		m_spWireContainer = NULL;
	}


	int nObj = m_lspObjects.Len();

	if (!m_Simulate_iNextObject)
	{
		MSCOPESHORT(SimulateOnTickUp);
		SimulateOnTickUp(_pPerfGraph);
	}

#ifdef	SERVER_STATS
	CMTime TSimulate;
	TStart(TSimulate);
#endif

	// Get quick pointers to all clients' object-info arrays.
	CWS_ClientObjInfo* lpClientObjInfo[WSERVER_MAXCLIENTS];
	int nClInfo = 0;
	{
		for(int i = 0; i < m_lspClientInfo.Len(); i++)
			if (m_lspClientInfo[i] != NULL)
				lpClientObjInfo[nClInfo++] = m_lspClientInfo[i]->m_lObjInfo.GetBasePtr();
	}

	// Lock a PVS for each client.
	if (m_pSceneGraph)
	{
		MSCOPESHORT(m_pSceneGraph);
		int nClientInfoLen = m_lspClientInfo.Len();
		for(int i = 0; i < nClientInfoLen; i++)
		{
			CWServer_ClientInfo* pClientInfo = m_lspClientInfo[i];
			if (pClientInfo != NULL)
			{
				pClientInfo->m_pPVS = m_pSceneGraph->SceneGraph_PVSLock(0, Client_GetPVSPosition(pClientInfo));
			}
		}
	}

	m_ServerMode |= SERVER_MODE_SIMULATE;


	// Render names of all (visible) objects?  
	//
	//     TODO: This is to make life easier for QA. 
	//           Ideally, it should not be included in RTM, but that's the config QA uses, so for now, leave it on (please remove when shipping game)
	//
	CDebugRenderContainer* pDebugRenderNames = Debug_GetWireContainer(CWO_DEBUGRENDER_NAMES);

	// -------------------------------------------------------------------
	// Run all objects...
	TProfileDef(T_ExecuteObj);
	{
		MSCOPESHORT(Runallobjects);
		M_NAMEDEVENT("RunAllObjects", 0xff000000);
		TMeasureProfile(T_ExecuteObj);
		if (!m_bWorld_PendingChange)
		{
			for(int iObj = m_Simulate_iNextObject; iObj < nObj; iObj++)
			{
				CWObject* pObj = lspObjects[iObj];

				//TODO: remove in RTM!
				if (pObj && pDebugRenderNames)
				{
					// check if object should be rendered
					const char* pName = pObj->GetName();
					if (pName && 
					    pObj->GetLastTickInClientPVS() > (GetGameTick() - 2) &&
					    pObj->m_iModel[0] &&
					    !(pObj->m_ClientFlags & CWO_CLIENTFLAGS_INVISIBLE)
					    )
					{
						CVec3Dfp32 Pos;
						pObj->GetAbsBoundBox()->GetCenter(Pos);
						pDebugRenderNames->RenderText(Pos, pName, 0xff00ff00, GetGameTickTime(), false);
					}
				}

				if (pObj && !(pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOREFRESH))
				{
					if (pObj->m_NextRefresh > m_SimulationTick)
						continue;
					
		#ifndef DEF_DISABLE_PERFGRAPH
					CMTime T_Obj; 
					if(m_PerfGraph)
					{
						TStart(T_Obj);
					}
		#endif
					CMat4Dfp32 LastPos = pObj->m_Pos;

					// Try execute the object
		#ifndef WSERVER_TRYONREFRESH
					{
						MSCOPESHORT(OnRefresh);
						pObj->OnRefresh();
					}
		#else
					const int iClass = pObj->m_iClass;
					int Except = 0;
					try 
						{ pObj->OnRefresh(); }
					catch(CCException) 
						{ Except = 1; }
					catch(...) 
						{ Except = 2; }

					// Exception?
					if (Except)
					{
						MRTC_CRuntimeClass_WObject* pRTC = m_spMapData->GetResource_Class(iClass);

						CStr s;
						switch(Except)
						{
						case 1 : s = CStrF("Object %d (%s) raised an exception.", iObj, (pRTC) ? pRTC->m_ClassName : "Unknown class"); break;
						case 2 : s = CStrF("Object %d (%s) raised a hardware exception.", iObj, (pRTC) ? pRTC->m_ClassName : "Unknown class"); break;
						}
						ConOut(s);
						LogFile(s);

						// Dump object.
						pObj = lspObjects[iObj];
						if (pObj)
						{
							ConOut("    " + pObj->Dump(m_spMapData, 0));
							LogFile("    " + pObj->Dump(m_spMapData, 0));
						}

		/*				while (CCException::ErrorAvail()) 
						{
							CStr s("    " + CCException::ErrorMsg());
							ConOut(s);
							LogFile(s);
						}*/

						// Remove object that's possibly causing trouble.
						m_spSpaceEnum->Remove(iObj);
						lspObjects[iObj] = NULL;
					}
					else
		#endif //WSERVER_TRYONREFRESH
					{
						// We must read the object-pointer again since the object might as well have been deleted.
						CWObject* pObj = lspObjects[iObj];
						if (pObj)
						{
							// Set lastpos to the position before OnRefresh()
							pObj->m_LastPos = LastPos;
							pObj->m_NextRefresh++;

							// Add dirtymask to all clients
		/*					if (pObj->m_DirtyMask)
							{
								int Mask = pObj->m_DirtyMask;
								for(int i = 0; i < nClInfo; i++)
									lpClientObjInfo[i][iObj].m_DirtyMask |= Mask;
								pObj->m_DirtyMask = 0;
							}*/
						}
					}

		#ifndef DEF_DISABLE_PERFGRAPH
					if(m_PerfGraph)
					{
						TStop(T_Obj); 
						int iClass = pObj->m_iClass;

						{
							MSCOPESHORT(GetResource_ClassStatistics);
							CWorldData::CWD_ClassStatistics* pCS = m_spMapData->GetResource_ClassStatistics(iClass);
							if (pCS)
							{
								pCS->m_ExecuteTime += T_Obj;
								pCS->m_nExecute++;
							}
						}

						// Plot object perf-graph
						MSCOPESHORT(PerfGraph_Plot);
						if(m_PerfGraph_lObjects.Len() == 0)
						{
							MSCOPE(PerfGraph, IGNORE);
							m_PerfGraph_lObjects.Clear();
							m_PerfGraph_lObjects.SetLen(nObj);
							for(int i = 0; i < nObj; i++)
								m_PerfGraph_lObjects[i].Create();
							m_PerfGraph_Tick_OnRefresh = 0;
						}

						CWS_ObjectPerfGraph& ObjPerf = m_PerfGraph_lObjects[iObj];
						
						ObjPerf.m_LastTouch_OnRefresh = m_PerfGraph_Tick_OnRefresh;
						ObjPerf.m_Time_OnRefresh = T_Obj;
						ObjPerf.m_spGraph_OnRefresh->Plot2(T_Obj.GetTime(), Clamp01(0.001f-T_Obj.GetTime()), 0x6fff8000, 0x3f000000);
					}
		#endif

					if (m_bWorld_PendingChange) 
						break;
				}
		#ifndef DEF_DISABLE_PERFGRAPH
				else
				{
					if(m_PerfGraph)
					{
						if(m_PerfGraph_lObjects.Len() == 0)
						{
							MSCOPE(PerfGraph, IGNORE);
							m_PerfGraph_lObjects.Clear();
							m_PerfGraph_lObjects.SetLen(nObj);
							for(int i = 0; i < nObj; i++)
								m_PerfGraph_lObjects[i].Create();
							m_PerfGraph_Tick_OnRefresh = 0;
						}

						CWS_ObjectPerfGraph& ObjPerf = m_PerfGraph_lObjects[iObj];
						ObjPerf.m_LastTouch_OnRefresh = m_PerfGraph_Tick_OnRefresh;
						ObjPerf.m_Time_OnRefresh.Reset();
						ObjPerf.m_spGraph_OnRefresh->Plot2(0, Clamp01(0.001f-0), 0x6fff8000, 0x3f000000);
					}
				}
		#endif
			}
		}

		// -------------------------------------------------------------------
		// Run players
	#ifdef	SERVER_STATS
		CMTime T_Execute; TStart(T_Execute);
	#endif
		TProfileDef(T_AsyncPlayer); 
		{
			M_NAMEDEVENT("RunPlayers", 0xff000000);
			TMeasureProfile(T_AsyncPlayer);
			for(int iClient = 0; iClient < m_lspClients.Len(); iClient++)
			if (m_lspClientInfo[iClient])
			{
				MSCOPESHORT(m_lspClientInfo[iClient]);

				CWObject_Game *pGame = Game_GetObject();
				CWO_Player* pP = pGame ? pGame->Player_Get(m_lspClientInfo[iClient]->m_iPlayer) : NULL;
				if (!pP || pP->m_spCmdQueue == NULL) continue;

				CWServer_ClientInfo* pCI = m_lspClientInfo[iClient];
#ifdef WCLIENT_FIXEDRATE
				CCmdQueue* pCQ = pCI->m_spLocalClient->Local_GetCmdQueue();

				pP->m_CurrentControl.m_MsgSize = 0;
				pCQ->GetAllNoTime(pP->m_CurrentControl);
				CCmd Cmd;
				Cmd.Create_Cmd0(255, 128);
				pP->m_CurrentControl.AddCmd(Cmd);

				pCI->m_spLocalClient->Local_ClearCmdQueue();

				pP->m_ServerControlSerial = 0x7fff;
				pP->m_ServerControlAccumTime = 0; //pP->m_spCmdQueue->m_LastAccumTime * 1000.0;
				pP->m_bCtrlValid = true;

				CWObject* pObj = Object_Get(pP->m_iObject);
				if (pObj)
				{
					pObj->m_LastPos = pObj->m_Pos;
					pObj->OnRefresh();
					pObj->m_NextRefresh = 0x7fffffff;
				}
#else
		//		int iClient = Player_GetClient(i);
		//		if (iClient < 0) continue;

				int bForceSync = Message_SendToObject(CWObject_Message(OBJSYSMSG_REQUESTFORCESYNC), pP->m_iObject);

				CRegistry* pReg = Registry_GetClientVar(iClient);
				if (bForceSync || (pReg && pReg->GetValuei("SYNC", 1)))
				{
					MSCOPESHORT(SyncronizedClient);
					// -------------------------------------------------------------------
					// Syncronized client
			pP->m_CurrentControl.m_MsgSize = 0;
					if (!pP->m_spCmdQueue->Empty())
					{
		//				ConOutL(CStrF("Server AccumTime %f", pP->m_spCmdQueue->m_LastAccumTime));
						fp32 TickTime = GetGameTickTime();
						fp32 dTime = TickTime;
						const fp32 TQueue = pP->m_spCmdQueue->GetTotalCmdTime() + pP->m_spCmdQueue->m_LastAccumTime;
						if (TQueue > TickTime+0.100f)
						{
							dTime += 0.010f;
		//					if (i) ConOut(CStrF("Eat time, TQueue %f", TQueue));
						}
						else if (TQueue > TickTime+0.040f)
						{
							dTime += 0.005f;
		//					if (i) ConOut(CStrF("Eat time, TQueue %f", TQueue));
						}
						else if (TQueue > TickTime+0.020f)
						{
							dTime += 0.001f;
		//					if (i) ConOut(CStrF("Eat time, TQueue %f", TQueue));
						}
						else if (TQueue < TickTime+0.020f)
						{
							dTime -= 0.001f;
		//					if (i) ConOut(CStrF("Release time, TQueue %f", TQueue));
						}

						pP->m_spCmdQueue->GetFrame(pP->m_CurrentControl, dTime);

		//				int Start, End;
		//				GetStartEndID(pP->m_CurrentControl, Start, End);
		//			LogFile(CStrF("Server current control %d -> %d", Start, End));
					}
					else
					{
						// Mega-lag?
		//				if (pP->m_spCmdQueue->m_LastAccumTime > -1.0f)
		//					pP->m_spCmdQueue->m_LastAccumTime -= SERVER_TIMEPERFRAME;

						// Was the code above. Set AccumTime to 0 to prevent strange input-waits
						// when time-leaping (and forcing sync without input) -JA
						pP->m_spCmdQueue->m_LastAccumTime = 0;

						pP->m_CurrentControl.m_MsgSize = 1;
						pP->m_ServerControlSerial = 0x7fff;
					}

			// During Mega-lag situations this code wasn't running, which gave problems during time-leap.
			// The code was moved down here, so that OnRefresh is always running on sync clients - JA
			CWObject* pObj = Object_Get(pP->m_iObject);
			if (pObj)
			{
				pObj->m_LastPos = pObj->m_Pos;
				pObj->OnRefresh();
				pObj->m_NextRefresh = 0x7fffffff;
			}

					pP->m_ServerControlAccumTime = RoundToInt(pP->m_spCmdQueue->m_LastAccumTime * 1000.0f);
					pP->m_spCmdQueue->m_LastAccumTime = pP->m_ServerControlAccumTime * ( 1.0f / 1000.0f );

					pP->m_bCtrlValid = true;
				}
				else
				{
					MSCOPESHORT(AsyncronizedClient);
					// -------------------------------------------------------------------
					// Asyncronized client
					if (!pP->m_spCmdQueue->Empty())
					{
		#ifndef	DEF_DISABLE_PERFGRAPH
						CMTime T_Obj; 
						if(m_PerfGraph)
						{
							TStart(T_Obj);
						}
		#endif

						int MaxRefresh = 5;
						pP->m_bCtrlValid = true;
						pP->m_ServerControlSerial = 0x7fff;
						fp32 TQueue = pP->m_spCmdQueue->GetTotalCmdTime() + pP->m_spCmdQueue->m_LastAccumTime;

						fp32 TickTime = GetGameTickTime();
						int nRefresh = 0;
						while (TQueue >= TickTime)
						{
		//					fp32 Accum = pP->m_spCmdQueue->m_LastAccumTime;
							pP->m_spCmdQueue->GetFrame(pP->m_CurrentControl, TickTime);
							pP->m_ServerControlAccumTime = RoundToInt(Min(50.0f, pP->m_spCmdQueue->m_LastAccumTime * 1000.0f));
							pP->m_spCmdQueue->m_LastAccumTime = pP->m_ServerControlAccumTime * ( 1.0f / 1000.0f );

							CWObject* pObj = Object_Get(pP->m_iObject);
							if (pObj)
							{
		//						M_CALLGRAPH

								pObj->m_LastPos = pObj->m_Pos;
								pObj->OnRefresh();
							}
		//ConOutL(CStrF("SERVER SIMULATE ID ?->%d, ACCUM %f -> %f", pP->m_ServerControlSerial, Accum, pP->m_spCmdQueue->m_LastAccumTime));

							nRefresh++;
							if (!pCI->m_bEatAllInput && nRefresh >= MaxRefresh)
								break;

							TQueue = pP->m_spCmdQueue->GetTotalCmdTime_IncAccum();
						}
//						M_TRACEALWAYS("SERVER SIMULATE Ticks %d, TQueue %f\n", nRefresh, TQueue * 1000.0f);
//						M_TRACEALWAYS("SERVER SIMULATE Ticks %d, ID ?->%d, ACCUM %d -> %f\n", nRefresh, pP->m_ServerControlSerial, pP->m_ServerControlAccumTime, pP->m_spCmdQueue->m_LastAccumTime);

//M_TRACEALWAYS("SERVER SIMULATE Ticks %d, ID ?->%d, ACCUM %d -> %f\n", nRefresh, pP->m_ServerControlSerial, pP->m_ServerControlAccumTime, pP->m_spCmdQueue->m_LastAccumTime);

						pP->m_bCtrlValid = false;
						pCI->m_bEatAllInput = 0;

						CWObject* pObj = Object_Get(pP->m_iObject);
						if (pObj)
						{
		//					pObj->m_NextRefresh = m_SimulationTick+1;
							pObj->m_NextRefresh = 0x7fffffff;

							if (nRefresh)
							{
								// Add dirtymask to all clients
		/*						if (pObj->m_DirtyMask)
								{
									int Mask = pObj->m_DirtyMask;
									for(int i = 0; i < nClInfo; i++)
										lpClientObjInfo[i][pP->m_iObject].m_DirtyMask |= Mask;
									pObj->m_DirtyMask = 0;
								}*/

		#ifndef DEF_DISABLE_PERFGRAPH
								if(m_PerfGraph)
								{
									TStop(T_Obj);
									CWorldData::CWD_ClassStatistics* pCS = m_spMapData->GetResource_ClassStatistics(pObj->m_iClass);
									if (pCS)
									{
										pCS->m_ExecuteTime += T_Obj;
									}

									// Plot object perf-graph
									if(m_PerfGraph_lObjects.Len() == 0)
									{
										MSCOPE(PerfGraph, IGNORE);
										m_PerfGraph_lObjects.Clear();
										m_PerfGraph_lObjects.SetLen(nObj);
										for(int i = 0; i < nObj; i++)
											m_PerfGraph_lObjects[i].Create();
										m_PerfGraph_Tick_OnRefresh = 0;
									}

									CWS_ObjectPerfGraph& ObjPerf = m_PerfGraph_lObjects[pP->m_iObject];
									ObjPerf.m_LastTouch_OnRefresh = m_PerfGraph_Tick_OnRefresh;
									ObjPerf.m_Time_OnRefresh = T_Obj;
									ObjPerf.m_spGraph_OnRefresh->Plot2(T_Obj.GetTime(), Clamp01(0.001f-T_Obj.GetTime()), 0x6fff8000, 0x3f000000);
								}
		#endif	// DEF_DISABLE_PERFGRAPH
							}
						}

						// Allways set full dirty-mask on player objects.
						// Object_SetDirty(pP->m_iObject, CWO_DIRTYMASK_COREMASK);
					}
					else
					{
						pP->m_bCtrlValid = false;
						pP->m_CurrentControl.m_MsgSize = 0;
						pP->m_ServerControlSerial = 0x7fff;
						pP->m_ServerControlAccumTime = 0; //pP->m_spCmdQueue->m_LastAccumTime * 1000.0;
					}
				}
#endif
			}
		}

#if 1
//		SimulatePhysics();

#ifdef PLATFORM_XENON
		if (bTraceDynamics)
			XTraceStartRecording( "game:\\Dynamics.bin" );
#endif

		SimulatePhysics2();

#ifdef PLATFORM_XENON
		if (bTraceDynamics)
			XTraceStopRecording();

		bTraceDynamics = false;
#endif

#endif

		// -------------------------------------------------------------------
		// Simulation done
		m_ServerMode &= ~SERVER_MODE_SIMULATE;
		m_Simulate_iNextObject = 0;

		Object_CommitDeferredDestruction();
		World_CommitDeferredSceneGraphLinkage();


	#ifdef	SERVER_STATS
		TStop(T_Execute); m_TExecute += T_Execute;
	#endif
	}

#ifdef	SERVER_STATS
	CMTime T_Cl; TStart(T_Cl);
#endif
	if (!m_bWorld_PendingChange)
	{
		MSCOPESHORT(m_World_PendingName);
		for(int i = 0; i < m_lspClients.Len(); i++)
			if (m_lspClients[i] != NULL)
			{
				CWServer_ClientInfo* pCI = m_lspClientInfo[i];
//				CWorld_Client* pC = m_lspClients[i];
				if (pCI->m_State == WCLIENT_STATE_INGAME)
				{
					// We don't want the client to catch up too much. 40 updates is two seconds.
					if (pCI->m_nClientSimTicks < 40)
					{
#ifndef WCLIENTSERVER_NOCLIENTMIRRORREFRESH
						pC->Simulate(true);
#endif
						pCI->m_nClientSimTicks++;
					}

					int UpdateDivisor = pCI->m_spClientReg->GetValuei("UPDATEDIVIDE", 1, 0);
					int iPlayer = pCI->m_iPlayer;
//					if (pC->m_LocalPlayer.m_StatusBar != m_lPlayers[iPlayer].m_StatusBar)
//						SetUpdate(i, SERVER_CLIENTUPDATE_STATUSBAR);

					CWObject_Game *pGame = Game_GetObject();
					CWO_Player* pP = pGame ? pGame->Player_Get(iPlayer) : NULL;
					if (true ||pP->m_ServerControlSerial != 0x7fff)
					{
						if (UpdateDivisor <= 1 || !(m_SimulationTick % UpdateDivisor)) 
						{
/*							if (pCI->m_ServerUpdateMask & SERVER_CLIENTUPDATE_DELTAFRAME)
								ConOutD(CStrF("Update already set %d, possibly unfinished tick.", m_SimulationTick));*/

							SetUpdate(i, SERVER_CLIENTUPDATE_DELTAFRAME);
						}
					}
					else
					{
						// ConOut(CStrF("Skipped update frame %d", m_SimulationTick));
						pCI->m_ServerUpdateMask &= ~SERVER_CLIENTUPDATE_DELTAFRAME;
					}

					//if ((m_SimulationTick & 3) == (i & 3)) SetUpdate(i, SERVER_CLIENTUPDATE_DELTAREGISTRY);
					SetUpdate(i, SERVER_CLIENTUPDATE_DELTAREGISTRY);
				}
				else 
				if ((pCI->m_State == WCLIENT_STATE_CHANGELEVEL) &&
					(pCI->m_ChangeLevelState == 1))
				{
					// We don't want the client to catch up too much. 40 updates is two seconds.
					if (pCI->m_nClientSimTicks < 40)
					{
#ifndef WCLIENTSERVER_NOCLIENTMIRRORREFRESH
						pC->Simulate(true);
#endif
						pCI->m_nClientSimTicks++;
					}
				}
			}

	}
#ifdef	SERVER_STATS
	TStop(T_Cl); m_TClients += T_Cl;
#endif

	// Unlock PVS for each client.
	if (m_pSceneGraph)
	{
		MSCOPESHORT(UnlockPVS);
		for(int i = 0; i < m_lspClientInfo.Len(); i++)
			if (m_lspClientInfo[i] != NULL)
			{
				m_pSceneGraph->SceneGraph_PVSRelease(m_lspClientInfo[i]->m_pPVS);
				m_lspClientInfo[i]->m_pPVS = NULL;
			}
	}

#ifdef	SERVER_STATS
	TStop(TSimulate);
	m_Simulate_CPUTime += TSimulate;
	m_NumSimulate++;
#endif

	// DEBUG: Render search-instances.
	if (m_spWireContainer != NULL)
	{
		MSCOPESHORT(RenderSearchInstances);
		CXR_BlockNavSearcher* pNav = Path_GetBlockNavSearcher();
		if (pNav)
		{
			for(int iClient = 0; iClient < m_lspClients.Len(); iClient++)
				if (m_lspClientInfo[iClient])
				{
					CWObject_Game *pGame = Game_GetObject();
					CWO_Player *pPlayer = pGame ? pGame->Player_Get(m_lspClientInfo[iClient]->m_iPlayer) : NULL;
					if(pPlayer && m_lspObjects[pPlayer->m_iObject])
					{
						pNav->Debug_Render(m_spWireContainer, m_lspObjects[pPlayer->m_iObject]->GetPosition());
					}
				}
		}
	}

	if(m_spCapture)
	{
		m_spCapture->EndScene();
		m_spPhysCapture	= NULL;
	}

	// DEBUG: Plot execution time
/*	if (_pPerfGraph)
	{
		_pPerfGraph->Plot3(
			T_ExecuteObj / GetCPUFrequency(), 
			T_AsyncPlayer / GetCPUFrequency(), 
			T_Cl / GetCPUFrequency(), 0xffffff01, 0xff400101, 0xff404080);
	}*/

/*	if (m_World_PendingName != "")
	{
		MSCOPESHORT(ChangeMap);
		MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
		if (!pCon) Error("Simulate", "No console.");

		pCon->ExecuteString(CStr("changemap(\"%s\", %d)", m_World_PendingName.Str(), m_World_PendingFlags));
//		World_Change(m_World_PendingName, m_World_PendingFlags);
		m_World_PendingName = "";
		m_World_PendingFlags = 0;
		bHalted = true;
	}*/

	return bHalted;
}


bool CWorld_ServerCore::SimulateAhead(CPerfGraph* _pPerfGraph, fp32 _MaxTime)
{
	MAUTOSTRIP(CWorld_ServerCore_SimulateAhead, false);
	MSCOPE(CWorld_ServerCore::SimulateAhead, WORLD_SERVER);
	M_NAMEDEVENT("SimulateAhead", 0xff000000);

#if 0
//	SimulatePhysics();
#endif

	CMTime TMaxSimulate = CMTime::CreateFromSeconds(_MaxTime);
//	CMTime TStartTime = CMTime::GetCPU();
	CMTime TStartTime;
	TStartTime.Snapshot();

	int nObj = m_lspObjects.Len();
	if (m_Simulate_iNextObject >= nObj)
		return false;

	CWObject** lspObjects = reinterpret_cast<CWObject**>(m_lspObjects.GetBasePtr());

#ifdef	SERVER_STATS
	CMTime TSimulate;
	TStart(TSimulate);
#endif

	if (!m_Simulate_iNextObject)
	{
		SimulateOnTickUp(_pPerfGraph);
	}

	// Get quick pointers to all clients' object-info arrays.
	CWS_ClientObjInfo* lpClientObjInfo[WSERVER_MAXCLIENTS];
	int nClInfo = 0;
	{
		for(int i = 0; i < m_lspClientInfo.Len(); i++)
			if (m_lspClientInfo[i] != NULL)
				lpClientObjInfo[nClInfo++] = m_lspClientInfo[i]->m_lObjInfo.GetBasePtr();
	}

	// Lock a PVS for each client.
	if (m_pSceneGraph)
	{
		for(int i = 0; i < m_lspClientInfo.Len(); i++)
			if (m_lspClientInfo[i] != NULL)
				m_lspClientInfo[i]->m_pPVS = m_pSceneGraph->SceneGraph_PVSLock(0, Client_GetPVSPosition(m_lspClientInfo[i]));
	}

	m_ServerMode |= SERVER_MODE_SIMULATE;

	bool bFullFrame = false;

	if (!m_bWorld_PendingChange) 
	{
		for(; (m_Simulate_iNextObject < nObj); m_Simulate_iNextObject++)
		{
			CMTime TCurrent;
			TCurrent.Snapshot();
			if (m_Simulate_iNextObject && !bFullFrame &&  ( TCurrent - TStartTime ).Compare( TMaxSimulate ) > 0 )
				break;
			int iObj = m_Simulate_iNextObject;

			CWObject* pObj = lspObjects[iObj];
			if (pObj && 
				!(pObj->m_PhysState.m_ObjectFlags & OBJECT_FLAGS_PLAYER) &&
				!(pObj->m_ClientFlags & (CWO_CLIENTFLAGS_NOREFRESH | CWO_CLIENTFLAGS_DESTROYED)))
			{
				if (pObj->m_NextRefresh > m_SimulationTick)
					continue;
				
	#ifndef DEF_DISABLE_PERFGRAPH
				CMTime T_Obj; TStart(T_Obj);
	#endif
//				const int iClass = pObj->m_iClass;
				CMat4Dfp32 LastPos = pObj->m_Pos;

				// Try execute the object
	#ifndef WSERVER_TRYONREFRESH
				{
					MSCOPESHORT(Refresh);
					pObj->OnRefresh();
				}
	#else
				const int iClass = pObj->m_iClass;
				int Except = 0;
				{
					MSCOPESHORT(Refresh);
					try 
					{ pObj->OnRefresh(); }
					catch(CCException) 
					{ Except = 1; }
					catch(...) 
					{ Except = 2; }
				}
				// Exception?
				if (Except)
				{
					MSCOPESHORT(Exception);
					MRTC_CRuntimeClass_WObject* pRTC = m_spMapData->GetResource_Class(iClass);

					CStr s;
					switch(Except)
					{
					case 1 : s = CStrF("Object %d (%s) raised an exception.", iObj, (pRTC) ? pRTC->m_ClassName : "Unknown class"); break;
					case 2 : s = CStrF("Object %d (%s) raised a hardware exception.", iObj, (pRTC) ? pRTC->m_ClassName : "Unknown class"); break;
					}
					ConOut(s);
					LogFile(s);

					// Dump object.
					pObj = lspObjects[iObj];
					if (pObj)
					{
						ConOut("    " + pObj->Dump(m_spMapData, 0));
						LogFile("    " + pObj->Dump(m_spMapData, 0));
					}

	/*				while (CCException::ErrorAvail()) 
					{
						CStr s("    " + CCException::ErrorMsg());
						ConOut(s);
						LogFile(s);
					}*/

					// Remove object that's possibly causing trouble.
					m_spSpaceEnum->Remove(iObj);
					lspObjects[iObj] = NULL;
				}
				else
	#endif //WSERVER_TRYONREFRESH
				{
					MSCOPESHORT(NoException);
					// We must read the object-pointer again since the object might as well have been deleted.
					CWObject* pObj = lspObjects[iObj];
					if (pObj)
					{
						// Set lastpos to the position before OnRefresh()
						pObj->m_LastPos = LastPos;
						pObj->m_NextRefresh++;

						// Add dirtymask to all clients
	/*					if (pObj->m_DirtyMask)
						{
							int Mask = pObj->m_DirtyMask;
							for(int i = 0; i < nClInfo; i++)
								lpClientObjInfo[i][iObj].m_DirtyMask |= Mask;
							pObj->m_DirtyMask = 0;
						}*/
					}
				}

	#ifndef DEF_DISABLE_PERFGRAPH
				TStop(T_Obj); 
	

				const int iClass = pObj->m_iClass;

				CWorldData::CWD_ClassStatistics* pCS = m_spMapData->GetResource_ClassStatistics(iClass);
				if (pCS)
				{
					pCS->m_ExecuteTime += T_Obj;
					pCS->m_nExecute++;
				}

				// Plot object perf-graph
				if(m_PerfGraph)
				{
					MSCOPESHORT(m_PerfGraph);
					CWS_ObjectPerfGraph& ObjPerf = m_PerfGraph_lObjects[iObj];
					ObjPerf.m_LastTouch_OnRefresh = m_PerfGraph_Tick_OnRefresh;
					ObjPerf.m_Time_OnRefresh = T_Obj;
					ObjPerf.m_spGraph_OnRefresh->Plot2(T_Obj.GetTime(), Clamp01(0.001f-T_Obj.GetTime()), 0x6fff8000, 0x3f000000);
				}
	#endif

				if (m_bWorld_PendingChange) 
					break;
			}
			else
			{
	#ifndef DEF_DISABLE_PERFGRAPH
				if(m_PerfGraph)
				{
					if(m_PerfGraph_lObjects.Len() == 0)
					{
						MSCOPE(PerfGraph, IGNORE);
						m_PerfGraph_lObjects.Clear();
						m_PerfGraph_lObjects.SetLen(nObj);
						for(int i = 0; i < nObj; i++)
							m_PerfGraph_lObjects[i].Create();
						m_PerfGraph_Tick_OnRefresh = 0;
					}

					CWS_ObjectPerfGraph& ObjPerf = m_PerfGraph_lObjects[iObj];
					ObjPerf.m_LastTouch_OnRefresh = m_PerfGraph_Tick_OnRefresh;
					ObjPerf.m_Time_OnRefresh.Reset();
					ObjPerf.m_spGraph_OnRefresh->Plot2(0, Clamp01(0.001f-0), 0x6fff8000, 0x3f000000);
				}
	#endif
			}
		}
	}


	m_ServerMode &= ~SERVER_MODE_SIMULATE;
	

	// Unlock PVS for each client.
	if (m_pSceneGraph)
	{
		for(int i = 0; i < m_lspClientInfo.Len(); i++)
			if (m_lspClientInfo[i] != NULL)
			{
				m_pSceneGraph->SceneGraph_PVSRelease(m_lspClientInfo[i]->m_pPVS);
				m_lspClientInfo[i]->m_pPVS = NULL;
			}
	}

#ifdef	SERVER_STATS
	TStop(TSimulate);
	m_Simulate_CPUTime += TSimulate;
#endif

	return true;
}
