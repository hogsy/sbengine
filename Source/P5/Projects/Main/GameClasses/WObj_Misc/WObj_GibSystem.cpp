#include "PCH.h"
#include "WObj_GibSystem.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Damage.h"
#include "../CConstraintSystem.h"
#include "../CConstraintSystemClient.h"

#ifdef NEW_GIBSYSTEM

/////////////////////////////////////////////////////////////////////////////
// CWObject_GibSystem

CWObject_GibSystem::CWObject_GibSystem()
{
	MAUTOSTRIP(CWObject_GibSystem_ctor, MAUTOSTRIP_VOID);

}

//---------------------------------------------------------------------------------------------------------------------

void CWObject_GibSystem::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
{
	MAUTOSTRIP(CWObject_GibSystem_OnEvalKey, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_GibSystem::OnEvalKey);
	CWObject::OnEvalKey(_KeyHash, _pKey);
}

//---------------------------------------------------------------------------------------------------------------------

void CWObject_GibSystem::OnSpawnWorld()
{
	MAUTOSTRIP(CWObject_Attach_OnSpawnWorld, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Attach::OnSpawnWorld);

	CWObject::OnSpawnWorld();
}


//---------------------------------------------------------------------------------------------------------------------

void CWObject_GibSystem::OnFinishEvalKeys()
{
	CWObject::OnFinishEvalKeys();
}

void CWObject_GibSystem::OnRefresh()
{
	if (m_pWServer->GetGameTick() > m_CreationGameTick + GIBSYSTEM_LIFETIME)
	{
		m_pWServer->Object_Destroy(m_iObject);
		return;
	}

	if(m_lGibInfo.Len())
	{
		CNetMsg Msg(NETMSG_GIBSYSTEM_INIT);

		int nGib = m_lGibInfo.Len();

		Msg.AddVecInt16_DynPrecision(m_GibExplosionOrigin);
		Msg.AddVecInt16_DynPrecision(m_GibExplosionParams);
		Msg.AddInt8(nGib); // Number of parts

		for(int i = 0; i < nGib; ++i)
		{
			const CGibInfo& Gib = m_lGibInfo[i];
			Msg.AddInt16(Gib.m_iModel);
			Msg.AddInt16(Gib.m_Pos[0]);
			Msg.AddInt16(Gib.m_Pos[1]);
			Msg.AddInt16(Gib.m_Pos[2]);
			Msg.AddInt8(Gib.m_Angles[0]);
			Msg.AddInt8(Gib.m_Angles[1]);
			Msg.AddInt8(Gib.m_Angles[2]);
		}
		m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
		m_lGibInfo.Destroy();
	}
}


//---------------------------------------------------------------------------------------------------------------------
void CWObject_GibSystem::OnIncludeClass(CMapData* _pMapData, CWorld_Server*)
{
	MAUTOSTRIP(CRPG_Object_Item_OnIncludeClass, MAUTOSTRIP_VOID);
	_pMapData->GetResourceIndex_Class("ClientGib");
}

void CWObject_GibSystem::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CRPG_Object_Item_OnIncludeTemplate, MAUTOSTRIP_VOID);
	CWObject::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
}


//---------------------------------------------------------------------------------------------------------------------


aint CWObject_GibSystem::OnMessage(const CWObject_Message &_Msg)
{

	// There mustn't be any breaks in below FORCE msg
	MAUTOSTRIP(CWObject_GibSystem_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GIB_INITIALIZE:
		{
			m_lGibInfo.SetLen(_Msg.m_Param0);
			if (m_lGibInfo.ListSize() == _Msg.m_DataSize)
				memcpy(m_lGibInfo.GetBasePtr(), _Msg.m_pData, _Msg.m_DataSize);
			m_GibExplosionOrigin = _Msg.m_VecParam0;
			m_GibExplosionParams = _Msg.m_VecParam1;
		}
		break;

	default:
		return CWObject::OnMessage(_Msg);
	}

	return CWObject::OnMessage(_Msg);
}


void CWObject_GibSystem::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_GibSystem_OnClientRefresh, MAUTOSTRIP_VOID);
	MSCOPESHORT( CWObject_GibSystem::OnClientRefresh );
}

void CWObject_GibSystem::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_GibSystem_OnClientRender, MAUTOSTRIP_VOID);
	MSCOPESHORT( CWObject_GibSystem::OnClientRender );
}

int CWObject_GibSystem::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	CBox3Dfp32 BBox;
	_pObj->GetVisBoundBox(BBox);
	int Ret = CWObject::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);
	_pObj->SetVisBoundBox(BBox);
	return Ret;
}


void CWObject_GibSystem::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	MSCOPESHORT(CWObject_GibSystem::OnClientNetMsg);

	switch(_Msg.m_MsgType)
	{
	case NETMSG_GIBSYSTEM_INIT:
		{
			int MsgPos = 0;

			CVec3Dfp32 ExplodeOrigin = _Msg.GetVecInt16_DynPrecision(MsgPos);
			CVec3Dfp32 ExplodeParams = _Msg.GetVecInt16_DynPrecision(MsgPos);
			ExplodeOrigin *= _pObj->GetPositionMatrix();

			int8 NumParts = _Msg.GetInt8(MsgPos); // Number of parts

			for(int i= 0; i < NumParts; ++i)
			{
				uint16 iModel = _Msg.GetInt16(MsgPos);
				CVec3Dfp32 Pos;
				Pos[0] = _Msg.GetInt16(MsgPos);
				Pos[1] = _Msg.GetInt16(MsgPos);
				Pos[2] = _Msg.GetInt16(MsgPos);
				CVec3Dfp32 Angles;
				Angles[0] = fp32(_Msg.GetInt8(MsgPos)) / 256.0f;
				Angles[1] = fp32(_Msg.GetInt8(MsgPos)) / 256.0f;
				Angles[2] = fp32(_Msg.GetInt8(MsgPos)) / 256.0f;

				CMat4Dfp32 Mat, Mat2;
				Angles.CreateMatrixFromAngles(0, Mat);
				Pos.SetRow(Mat, 3);

				Mat.Multiply(_pObj->GetPositionMatrix(), Mat2);
//				Mat *= _pObj->GetPositionMatrix();

				CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(iModel);
				int iObj = _pWClient->ClientObject_Create("ClientGib", Mat2);
				CWObject_CoreData* pObj = _pWClient->Object_GetCD(iObj);
				if (pObj && pModel)
				{
					pObj->m_iModel[0] = iModel;
					CBox3Dfp32 Box;
					pModel->GetBound_Box(Box, NULL);
					pObj->SetVisBoundBox(Box);

					CVec3Dfp32 Pos = pObj->GetPosition();
					fp32 Range = Pos.Distance(ExplodeOrigin);
					fp32 Blast = 0.5f * ExplodeParams[0] * (1.0f - Clamp01(Range / ExplodeParams[1])) * (1.0f + ExplodeParams[2] * (Random - Random));
					CVec3Dfp32 Dir = (Pos - ExplodeOrigin).Normalize();
					Dir[0] += 0.2f * (Random - 0.5f);
					Dir[1] += 0.2f * (Random - 0.5f);
					Dir[2] += 0.2f * (Random - 0.5f);
					Dir.Scale(Blast, Dir);
					CVelocityfp32 Vel;
					Vel.m_Move = Dir;
					CVec3Dfp32 Axis(Random - 0.5f, Random - 0.5f, 0.1f);
					Axis.Normalize();
					Vel.m_Rot.m_Axis = Axis;
					Vel.m_Rot.m_Angle = Random * 0.1f + 0.025f;
					_pWClient->Object_SetVelocity(iObj, Vel);
				}
			}
		}
		break;
	}
}



void CWObject_ClientGib::OnClientCreate(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient)
{
	CWO_PhysicsState PhysState;
	PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_BOX, 0, CVec3Dfp32(4), 0));

	PhysState.m_PhysFlags = OBJECT_PHYSFLAGS_SLIDEABLE | OBJECT_PHYSFLAGS_ROTVELREFLECT | OBJECT_PHYSFLAGS_APPLYROTVEL;
	PhysState.m_ObjectFlags = 0;
	PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PLAYERPHYSMODEL;
	if (!_pWClient->Object_SetPhysics(_pObj->m_iObject, PhysState))
	{
		_pWClient->Object_Destroy(_pObj->m_iObject);
		return;
	}
	_pObj->m_ClientFlags |= CWO_CLIENTFLAGS_SHADOWCASTER;

	_pObj->m_PhysAttrib.m_Elasticy = 0.7f;
	_pObj->m_Data[0] = 1;	// Enable
}

void CWObject_ClientGib::OnClientExecute(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_ClientGib::OnClientExecute);
	_pObj->m_Data[1]++;
	if (_pObj->m_Data[1] > GIBSYSTEM_LIFETIME)
	{
		_pWClient->Object_Destroy(_pObj->m_iObject);
		return;
	}

//	return;

	if (_pObj->m_Data[0])
	{
		_pWClient->Object_MovePhysical(_pObj->m_iObject, 1.0f);
		if ((_pObj->m_Data[0] & 2) && _pObj->GetMoveVelocity().LengthSqr() < Sqr(1.0f))
			_pObj->m_Data[0] = 0;
		else
			_pObj->m_Data[0] &= ~2;
	}
}

void CWObject_ClientGib::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetRenderTickFrac();

	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

	CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if(pModel)
	{
		if (_pObj->m_Data[1] > GIBSYSTEM_LIFETIME-20)
		{
			CBox3Dfp32 Bound;
			pModel->GetBound_Box(Bound);
			CVec3Dfp32 Size;
			Bound.m_Max.Sub(Bound.m_Min, Size);
			fp32 MaxSize = Max3(Size[0], Size[1], Size[2]);

			fp32 FadeOut = fp32(_pObj->m_Data[1] - (GIBSYSTEM_LIFETIME-20) + IPTime) / 20.0f;
			MatIP.k[3][2] -= FadeOut * MaxSize;
		}


		CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
		_pEngine->Render_AddModel(pModel, MatIP, AnimState);
	}
}

int CWObject_ClientGib::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData*, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	switch(_Event)
	{
	case CWO_PHYSEVENT_GETACCELERATION :
		{
			_pMat->Unit();
			_pMat->k[3][2] = -1.0f;
			return SERVER_PHYS_HANDLED;
		}

	case CWO_PHYSEVENT_IMPACT :
		{
			if (_pObj)
			{
				CVelocityfp32 Vel = _pObj->GetVelocity();
				Vel.m_Move *= 0.9f;
				Vel.m_Rot.m_Angle *= 0.9f;

//				Swap(Vel.m_Rot.m_Axis[0], Vel.m_Rot.m_Axis[1]);
//				Swap(Vel.m_Rot.m_Axis[1], Vel.m_Rot.m_Axis[2]);

				if (_pCollisionInfo && _pCollisionInfo->m_bIsValid)
				{
					const fp32 Radius = 6.0f;

					CVec3Dfp32 N = _pCollisionInfo->m_Plane.n;

					CVec3Dfp32 MoveProj;
					Vel.m_Move.Combine(N, -(N * Vel.m_Move), MoveProj);

					fp32 ProjVel = MoveProj.Length();
					fp32 TargetRotVel = ProjVel / (Radius * _PI2);

					CVec3Dfp32 Axis2;
					_pObj->GetMoveVelocity().CrossProd(_pCollisionInfo->m_Plane.n, Axis2);
//					_pCollisionInfo->m_Plane.n.CrossProd(_pObj->GetMoveVelocity(), Axis2);
					Axis2.Normalize();
					Vel.m_Rot.m_Axis.Lerp(Axis2, 0.5f, Vel.m_Rot.m_Axis);
					Vel.m_Rot.m_Axis.Normalize();
					Vel.m_Rot.m_Angle = LERP(Vel.m_Rot.m_Angle, TargetRotVel, 0.5f);
				}

//				Vel.m_Rot.m_Axis = -Vel.m_Rot.m_Axis;
				_pPhysState->Object_SetVelocity(_pObj->m_iObject, Vel);
			}
			if (_pCollisionInfo && _pCollisionInfo->m_bIsValid)
			{
				if (_pCollisionInfo->m_Plane.n[2] > 0.8f)
					_pObj->m_Data[0] |= 2;	// On ground
			}
			return SERVER_PHYS_DEFAULTHANDLER;
		}

	default :
		return SERVER_PHYS_DEFAULTHANDLER;
	}
}


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_GibSystem, CWObject_Model, 0x0100);
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ClientGib, CWObject_Model, 0x0100);

#else























































































































/////////////////////////////////////////////////////////////////////////////
// CWObject_GibSystem

CWObject_GibSystem::CWObject_GibSystem()
{
	MAUTOSTRIP(CWObject_GibSystem_ctor, MAUTOSTRIP_VOID);

	CGibClientData *pCD = GetClientData(this);
	if (pCD)
	{
		pCD->m_spRagdoll = NULL;
	}
}


CWObject_GibSystem::CGibClientData *CWObject_GibSystem::GetClientData(CWObject_CoreData* _pObj)
{
	if(!_pObj->m_lspClientObj[0])
	{
		_pObj->m_lspClientObj[0] = MNew(CGibClientData);
		if(!_pObj->m_lspClientObj[0])
			Error_static("CWObject_GameCore::GetClientData", "Unable to create client objects.");
	}

	return (CWObject_GibSystem::CGibClientData*)(CReferenceCount*)_pObj->m_lspClientObj[0];
}

CWObject_GibSystem::CGibClientData *CWObject_GibSystem::GetClientData()
{
	return GetClientData(this);
}

const CWObject_GibSystem::CGibClientData *CWObject_GibSystem::GetClientData(const CWObject_CoreData* _pObj)
{
	return GetClientData(const_cast<CWObject_CoreData *>(_pObj));
}


//---------------------------------------------------------------------------------------------------------------------

void CWObject_GibSystem::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
{
	MAUTOSTRIP(CWObject_GibSystem_OnEvalKey, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_GibSystem::OnEvalKey);
	CWObject::OnEvalKey(_pKey);
}

//---------------------------------------------------------------------------------------------------------------------

void CWObject_GibSystem::OnSpawnWorld()
{
	MAUTOSTRIP(CWObject_Attach_OnSpawnWorld, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Attach::OnSpawnWorld);

	CWObject::OnSpawnWorld();

	// Make sure all resources are included from the timedmessages
	//	for(int i = 0; i < m_lDestroyMessages.Len(); i++)
	//		m_lDestroyMessages[i].SendPrecache(m_iObject, m_pWServer);
}


//---------------------------------------------------------------------------------------------------------------------

void CWObject_GibSystem::OnFinishEvalKeys()
{
	CWObject::OnFinishEvalKeys();
}

void CWObject_GibSystem::OnRefresh()
{
	if (m_pWServer->GetGameTick() > m_CreationGameTick + GIBSYSTEM_LIFETIME)
	{
		m_pWServer->Object_Destroy(m_iObject);
		return;
	}

	CGibClientData *pCD = GetClientData(this);
	if(pCD->m_liGibPartModel.Len() > 0)
	{
		CNetMsg Msg(NETMSG_GIBSYSTEM_INIT);
		Msg.AddInt8(pCD->m_lGibPartPos.Len()); // Number of parts

		for(int i = 0; i < pCD->m_lGibPartPos.Len(); ++i)
		{
			Msg.AddInt16(pCD->m_liGibPartModel[i]);
			CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(pCD->m_lGibPartPos[i],3);
			Msg.AddVecInt16_DynPrecision(Pos);
			CVec3Dfp32 v;
			v.CreateAnglesFromMatrix(0, pCD->m_lGibPartPos[i]);
			Msg.AddVecInt8_DynPrecision(v);
		}
		Msg.AddVecInt16_DynPrecision(pCD->m_GibExplosionOrigin);
		Msg.AddVecInt16_DynPrecision(pCD->m_GibExplosionParams);
		m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
		pCD->m_lGibPartPos.Clear();
		pCD->m_liGibPartModel.Clear();
	}
}


//---------------------------------------------------------------------------------------------------------------------

void CWObject_GibSystem::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CRPG_Object_Item_OnIncludeClass, MAUTOSTRIP_VOID);
	CWObject::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
}


//---------------------------------------------------------------------------------------------------------------------


int CWObject_GibSystem::OnMessage(const CWObject_Message &_Msg)
{

	// There mustn't be any breaks in below FORCE msg
	MAUTOSTRIP(CWObject_GibSystem_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GIB_ADDPART:
		{
			CGibClientData *pCD = GetClientData(this);
			pCD->m_liGibPartModel.Add(_Msg.m_Param0);
			CMat4Dfp32 Pos = *((CMat4Dfp32*) _Msg.m_Param1);
			pCD->m_lGibPartPos.Add(Pos);
			return 1;
		}
		break;
	case OBJMSG_GIB_EXPLOSION:
		{
			CGibClientData *pCD = GetClientData(this);
			pCD->m_GibExplosionOrigin = _Msg.m_VecParam0;
			pCD->m_GibExplosionParams = _Msg.m_VecParam1;
			return 1;
		}
		break;
	default:
		return CWObject::OnMessage(_Msg);
	}

	return CWObject::OnMessage(_Msg);
}


void CWObject_GibSystem::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_GibSystem_OnClientRefresh, MAUTOSTRIP_VOID);
	MSCOPESHORT( CWObject_GibSystem::OnClientRefresh );
	CGibClientData *pCD = GetClientData(_pObj);

	CBox3Dfp32 BBox;
	CVec3Dfp32 Pos;
	CVec3Dfp32 ObjPos = _pObj->GetPosition();

	BBox.m_Min = CVec3Dfp32(_FP32_MAX);
	BBox.m_Max = CVec3Dfp32(-_FP32_MAX);
	if (pCD->m_spRagdoll)
	{
		pCD->m_spRagdoll->Animate(_pWClient->GetGameTick());
		pCD->m_spRagdoll->DrawPoints(0.05f,0x7f7f007f);	// Purple
	}
	for(int i = 0; i < pCD->m_lGibPartPos.Len(); i++)
	{		
		pCD->m_lGibPartLastPos[i] = pCD->m_lGibPartPos[i];
		// If gib ragdoll code goes here
		// CVec3Dfp32::GetMatrixRow(pCD->m_lGibPartPos[i],3) += CVec3Dfp32::GetMatrixRow(pCD->m_lGibPartPos[i],0);   
		pCD->m_spRagdoll->GetBoxMatrix(i,pCD->m_lGibPartPos[i]);

		Pos = CVec3Dfp32::GetMatrixRow(pCD->m_lGibPartPos[i],3);
		// Pos -= ObjPos;

		BBox.m_Min[0] = Min(BBox.m_Min[0], Pos[0]);
		BBox.m_Min[1] = Min(BBox.m_Min[1], Pos[1]);
		BBox.m_Min[2] = Min(BBox.m_Min[2], Pos[2]);

		BBox.m_Max[0] = Max(BBox.m_Max[0], Pos[0]);
		BBox.m_Max[1] = Max(BBox.m_Max[1], Pos[1]);
		BBox.m_Max[2] = Max(BBox.m_Max[2], Pos[2]);
	}
#ifdef DEBUG
	CBox3Dfp32 BBox2 = BBox;
	BBox2.m_Min	+= ObjPos;
	BBox2.m_Max	+= ObjPos;
	_pWClient->Debug_RenderAABB(BBox2);
#endif
	_pObj->SetVisBoundBox(BBox);
}

void CWObject_GibSystem::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_GibSystem_OnClientRender, MAUTOSTRIP_VOID);
	MSCOPESHORT( CWObject_GibSystem::OnClientRender );

	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetInterpolateTime() / fp32(SERVER_TIMEPERFRAME);
	CGibClientData *pCD = GetClientData(_pObj);

	for(int i = 0; i < pCD->m_lGibPartPos.Len(); i++)
	{
		Interpolate2(pCD->m_lGibPartLastPos[i], pCD->m_lGibPartPos[i], MatIP, IPTime);
		if(pCD->m_liGibPartModel[i] > 0)
		{
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(pCD->m_liGibPartModel[i]);
			if(pModel)
			{
				CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
				AnimState.m_iObject = 0;	// This is to prevent the renderer from using the visiblity box of the CWObject_GibSystem for lighting and occlusion culling. If the object is unknown, lighting and occlusion for the model will be done dynamically.
				_pEngine->Render_AddModel(pModel, MatIP, AnimState);
			}
		}
	}
}

int CWObject_GibSystem::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	CBox3Dfp32 BBox;
	_pObj->GetVisBoundBox(BBox);
	int Ret = CWObject::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);
	_pObj->SetVisBoundBox(BBox);
	return Ret;
}


void CWObject_GibSystem::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	MSCOPESHORT(CWObject_GibSystem::OnClientNetMsg);

	switch(_Msg.m_MsgType)
	{
	case NETMSG_GIBSYSTEM_INIT:
		{
			CGibClientData *pCD = GetClientData(_pObj);
			int MsgPos = 0;
			int8 NumParts = _Msg.GetInt8(MsgPos); // Number of parts
			pCD->m_liGibPartModel.SetLen(NumParts);
			pCD->m_lGibPartPos.SetLen(NumParts);
			pCD->m_lGibPartLastPos.SetLen(NumParts);

			// Setup the gib ragdoll system
			if (!pCD->m_spRagdoll)
			{
				pCD->m_spRagdoll = MNew(CConstraintGib);
				pCD->m_spRagdoll->Init(_pObj,_pWClient);
				pCD->m_spRagdoll->SetOrgMat(_pObj->GetPositionMatrix());
			}
			for(int i= 0; i < NumParts; ++i)
			{	
				pCD->m_liGibPartModel[i] = _Msg.GetInt16(MsgPos);
				CVec3Dfp32 Pos = _Msg.GetVecInt16_DynPrecision(MsgPos);

				// Use euler angles with 8 bit precision per angle
				CVec3Dfp32 v  = _Msg.GetVecInt8_DynPrecision(MsgPos);
				v.CreateMatrixFromAngles(0, pCD->m_lGibPartPos[i]);
				Pos.SetMatrixRow(pCD->m_lGibPartPos[i],3);
				pCD->m_lGibPartLastPos[i] = pCD->m_lGibPartPos[i];
				if (pCD->m_liGibPartModel[i])
				{
					CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(pCD->m_liGibPartModel[i]);
					pCD->m_spRagdoll->AddBox(pModel,pCD->m_lGibPartPos[i]);
				}
			}
			// Explosion
			pCD->m_GibExplosionOrigin = _Msg.GetVecInt16_DynPrecision(MsgPos);
			pCD->m_GibExplosionParams = _Msg.GetVecInt16_DynPrecision(MsgPos);
			CVec3Dfp32 Pos = pCD->m_GibExplosionOrigin * _pObj->GetPositionMatrix();
			pCD->m_spRagdoll->Explode(Pos,pCD->m_GibExplosionParams);
		}
		break;
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_GibSystem, CWObject_Model, 0x0100);
#endif

