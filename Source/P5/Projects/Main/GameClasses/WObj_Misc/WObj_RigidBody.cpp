#include "PCH.h"
#include "WObj_RigidBody.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Damage.h"
#include "../CConstraintSystem.h"
#include "../CConstraintSystemClient.h"

/////////////////////////////////////////////////////////////////////////////
// CWObject_RigidBody

CWObject_RigidBody::CWObject_RigidBody()
{
	MAUTOSTRIP(CWObject_RigidBody_ctor, MAUTOSTRIP_VOID);
	m_iBSPModel = 0;
	m_spRagdoll = NULL;
	m_bActivated = false;
	m_bAutoActivate = false;
	m_bPhysicsSet = false;

	m_iHardImpactSound = -1;
	m_iSoftImpactSound = -1;

	m_LastActiveTick = 0;
	ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
}


//---------------------------------------------------------------------------------------------------------------------

void CWObject_RigidBody::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
{
	MAUTOSTRIP(CWObject_RigidBody_OnEvalKey, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_RigidBody::OnEvalKey);

	switch (_KeyHash)
	{
	case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
		{
			CStr ModelName = CStrF("$WORLD:%d", _pKey->GetThisValuei());
			m_iBSPModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
			Model_Set(0,m_iBSPModel, false);
			break;
		}
	case MHASH5('HARD','COLL','ISIO','NSOU','ND'): // "HARDCOLLISIONSOUND"
		{
			m_iHardImpactSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
			break;
		}

	case MHASH4('COLL','ISIO','NSOU','ND'): // "COLLISIONSOUND"
		{
			m_iSoftImpactSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
			break;
		}

	case MHASH3('AUTO','ACTI','VATE'): // "AUTOACTIVATE"
		{
			m_bAutoActivate = true;
			break;
		}
		
	/*else if(_pKey->GetThisName().CompareSubStr("ONDESTROY") == 0)
	{
		CWO_SimpleMessage Msg;
		Msg.Parse(_pKey->GetThisValue(), m_pWServer);

		if((Msg.m_Target == "" || Msg.m_Target == "<NULL>") && Msg.m_Msg == 0x110)
		{
			// Have to do this for now, because Ogier had a bug which saved this type of message on all dynamics
		}
		else
			m_lDestroyMessages.Add(Msg);
	}
	}
	else if(_pKey->GetThisName().CompareSubStr("DESTROYSOUND") == 0)
	{
		CWO_SimpleMessage Msg;
		Msg.m_iSpecialTarget = CWO_SimpleMessage::SPECIAL_TARGET_THIS;
		Msg.m_Msg = OBJSYSMSG_PLAYSOUND;
		Msg.m_StrParam = _pKey->GetThisValue();
		Msg.m_StrParam.Trim();
		m_lDestroyMessages.Add(Msg);
	}
	case MHASH4('DEST','ROYE','FFEC','T'): // "DESTROYEFFECT"
		{
			m_DestroyEffect = _pKey->GetThisValue();
			break;
		}
		*/
	default:
		{
			CWObject_Model::OnEvalKey(_KeyHash, _pKey);
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------

void CWObject_RigidBody::OnSpawnWorld()
{
	MAUTOSTRIP(CWObject_Attach_OnSpawnWorld, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Attach::OnSpawnWorld);

	CWObject_Model::OnSpawnWorld();

	// Make sure all resources are included from the timedmessages
//	for(int i = 0; i < m_lDestroyMessages.Len(); i++)
//		m_lDestroyMessages[i].SendPrecache(m_iObject, m_pWServer);
}


//---------------------------------------------------------------------------------------------------------------------

void CWObject_RigidBody::OnFinishEvalKeys()
{
	CWObject_Model::OnFinishEvalKeys();
	if(m_iModel[0])
	{
		if (SetPhysics())
		{
			m_bPhysicsSet = true;
		}
	}

	if(m_bAutoActivate)
		ActivateRigidBody();
}


//---------------------------------------------------------------------------------------------------------------------

void CWObject_RigidBody::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CRPG_Object_Item_OnIncludeClass, MAUTOSTRIP_VOID);
	CWObject_Model::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
	IncludeSoundFromKey("COLLISIONSOUND", _pReg, _pMapData);
	//IncludeClassFromKey("DESTROYEFFECT", _pReg, _pMapData);
}

bool CWObject_RigidBody::SetPhysics()
{
	// Setup physics
	CWO_PhysicsState Phys;
	if(m_iBSPModel)
	{
		Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_PHYSMODEL, m_iBSPModel, 0, 0);
	}
	else
	{	// Find the dimension of the system
		CVec3Dfp32 Dim = CVec3Dfp32(16.0f);	// Safing, at least we get a box
		if (m_spRagdoll)
		{
			CVec3Dfp32 Min,Max;
			m_spRagdoll->GetBBox(Min,Max);
			Dim = Max - Min;
		}
		else
		{
			CBox3Dfp32 Box;
			CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
			if(pModel)
			{
				pModel->GetBound_Box(Box);
				Dim = Box.m_Max - Box.m_Min;
			}
		}
		Dim = Dim / 2.0f;
		Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, Dim, CVec3Dfp32(0,0,Dim[2]));
	}
	Phys.m_nPrim = 1;

	Phys.m_PhysFlags = OBJECT_PHYSFLAGS_OFFSET;
	Phys.m_ObjectFlags = OBJECT_FLAGS_TRIGGER | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
	Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_PROJECTILE;
	if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
	{
		return(false);
	}
	else
	{
		return(true);
	}
};

//---------------------------------------------------------------------------------------------------------------------

void CWObject_RigidBody::OnRefresh()
{
	if (m_spRagdoll)
	{
		// if ((!m_bPhysicsSet) && (m_spRagdoll->IsStopped()) && (SetPhysics()))
		if (SetPhysics())
		{
			m_bPhysicsSet = true;
		}
		
		if (m_bActivated)
		{
			CMat4Dfp32 Mat = GetPositionMatrix();
			m_spRagdoll->Animate(m_pWServer->GetGameTick(), Mat);
			CMat4Dfp32 Mat2;
			m_spRagdoll->GetPosition(Mat2);
			if(m_spRagdoll->IsStopped())
			{
				m_bActivated = false;
			}
			m_pWServer->Object_SetPosition(m_iObject, Mat2);
			m_LastActiveTick = m_pWServer->GetGameTick();
		}

		if ((m_spRagdoll->IsStopped()) && ((m_LastActiveTick + RIGIDBODY_LIFETIME) < m_pWServer->GetGameTick()))
		{
			m_spRagdoll = NULL;
			ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
		}
	}
}


void CWObject_RigidBody::ActivateRigidBody()
{
	if (!m_spRagdoll)
	{
		m_spRagdoll = MNew(CConstraintRigidObject);
		CXR_Model *pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
		SConstraintSystemSettings Settings;
		Settings.m_SkeletonType	= SConstraintSystemSettings::RAGDOLL_NONE;

		m_spRagdoll->Init(m_iObject,this, m_pWServer);
		m_spRagdoll->SetOrgMat(GetPositionMatrix());
		m_spRagdoll->Setup(&Settings, pModel);
		m_spRagdoll->SetCollisionSound(m_iHardImpactSound,true);
		m_spRagdoll->SetCollisionSound(m_iSoftImpactSound,false);
	}
	m_spRagdoll->Activate(true);
	m_bActivated = true;
	ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
}

int CWObject_RigidBody::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWObject_Character_OnPhysicsEvent, 0);

	// We ignore impacts with characters
	/*
	if(_Event == CWO_PHYSEVENT_IMPACTOTHER)
	{
		CVec3Dfp32 Dir = (_pObj->GetPosition() - _pObjOther->GetPosition()).Normalize();
		Dir *= 4.0f;
		Dir[2] = 0.0f;
		// param0 Pusher, Vec0 pusherpos
		CWObject_Message Msg(OBJMSG_PUSH_RIGID,_pObjOther->m_iObject,0,-1,0,_pObjOther->GetPosition());
		_pPhysState->Phys_Message_SendToObject(Msg,_pObj->m_iObject);
	}
	*/

	return SERVER_PHYS_DEFAULTHANDLER;
}


bool CWObject_RigidBody::OnIntersectLine(CWO_OnIntersectLineContext& _Context, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWObject_Character_OnIntersectLine, false);

	CWObject_CoreData* _pObj = _Context.m_pObj;

	for(int i = 0; i < 3; i++)
	{
		if(_pObj->m_iModel[i] > 0)
		{
			CXR_Model* pModel = _Context.m_pPhysState->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
			if(!pModel)
				continue;
			CXR_PhysicsModel* pPhys = pModel->Phys_GetInterface();
			if(!pPhys)
				continue;
			CXR_PhysicsContext PhysContext(_pObj->GetPositionMatrix());
			pPhys->Phys_Init(&PhysContext);

			if(pPhys->Phys_IntersectLine(&PhysContext, _Context.m_p0, _Context.m_p1, _Context.m_MediumFlags, _pCollisionInfo))
				return true;
		}
	}
	return false;
}


aint CWObject_RigidBody::OnMessage(const CWObject_Message &_Msg)
{

	// There mustn't be any breaks in below FORCE msg
	MAUTOSTRIP(CWObject_RigidBody_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
	case OBJMSG_RADIALSHOCKWAVE:
		{
			ActivateRigidBody();
			m_bPhysicsSet = false;
			const CWO_ShockwaveMsg* pMsg = (CWO_ShockwaveMsg*)_Msg.m_pData;
			if(pMsg)
			{
				CVec3Dfp32 Dir = m_spRagdoll->m_SystemAvgPos - pMsg->m_Center;
				fp32 d = (m_spRagdoll->m_SystemAvgPos - pMsg->m_Center).Length();
				if (d < 1.0f) {d = 1.0f;}
				if (d >= pMsg->m_ObjectRange)
				{
					return 1;
				}
				Dir *= 1.0f / d;
				fp32 Force = (1.0f - (d / pMsg->m_ObjectRange)) * pMsg->m_Force * 0.25f;
				m_spRagdoll->AddPendingImpulse(pMsg->m_Center,Dir * Force);
			}

			return 1;
		}
	case OBJMSG_DAMAGE:
		{
			ActivateRigidBody();
			m_bPhysicsSet = false;
			const CWO_DamageMsg *pMsg = CWO_DamageMsg::GetSafe(_Msg);
			if (pMsg)
				m_spRagdoll->AddPendingImpulse(pMsg->m_Position, pMsg->m_Force);

			return 1;
		}
	case OBJMSG_THROW_RIGID:
		{
			ActivateRigidBody();
			m_bPhysicsSet = false;
			// param0 thrower,param1 thrown obj (this),vec0 start pos,vec1 force
			m_spRagdoll->AddPendingImpulse(_Msg.m_VecParam0,_Msg.m_VecParam1);
			return 1;
		}
		break;
	case OBJMSG_PUSH_RIGID:
		{
			ActivateRigidBody();
			m_bPhysicsSet = false;
			if ((m_spRagdoll)||(_Msg.m_Param0 > 0))
			{
				ActivateRigidBody();
				m_bPhysicsSet = false;
				m_spRagdoll->AddPushObject(_Msg.m_Param0);
			}
			return 1;
		}
	default:
		return CWObject_Model::OnMessage(_Msg);
	}

	return CWObject_Model::OnMessage(_Msg);
}


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_RigidBody, CWObject_Model, 0x0100);
