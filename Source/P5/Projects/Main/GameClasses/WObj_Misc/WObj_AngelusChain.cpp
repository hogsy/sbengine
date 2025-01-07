/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			CWObject_AngelusChain.cpp

	Author:			Patrik Willbo

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CAngelusChainSetup				(SEC_AA)
					CAngelusChainState				(SEC_BB)
					CWO_AngelusChain_ClientData		(SEC_CC)
					CWObject_AngelusChain			(SEC_DD)

	Comments:		Based upon the tentacle system

	History:		
		051120		Created file
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_AngelusChain.h"
#include "WObj_TentacleSystem_ClientData.h"
#include "WObj_Object.h"
#include "../../GameWorld/WServerMod.h"
#include "../CConstraintSystem.h"
#include "../WObj_AI/AICore.h"


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_AngelusChain, CWObject_Model, 0x0100);


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CAngelusChainSetup (SEC_AA Section AA)
|__________________________________________________________________________________________________
\*************************************************************************************************/
CAngelusChainSetup::CAngelusChainSetup()
	: m_iTemplateModel(0)
	//, m_AttachPoint(0)
	, m_TargetAttachPoint(0)
{
}


void CAngelusChainSetup::Setup(const CRegistry& _Reg, CWorld_Server& _WServer)
{
	CMapData& MapData = *_WServer.GetMapData();

	m_Name = _Reg.GetThisValue();
	for (uint i = 0; i < _Reg.GetNumChildren(); i++)
	{
		CStr KeyName = _Reg.GetName(i);
		CStr KeyValue = _Reg.GetValue(i);

		if (KeyName == "MODEL")
			m_iTemplateModel = MapData.GetResourceIndex_Model(KeyValue.Str());

		else if (KeyName == "ATTACH")
			m_AttachPoint.Parse(KeyValue);

		else if (KeyName == "TARGET")
			m_TargetAttachPoint.ParseString(KeyValue);
	}
}


void CAngelusChainSetup::Pack(uint8*& _pD) const
{
	TAutoVar_Pack(m_Name, _pD);
	TAutoVar_Pack(m_iTemplateModel, _pD);
	TAutoVar_Pack(m_AttachPoint.m_iNode, _pD);
	TAutoVar_Pack(m_AttachPoint.m_LocalPos, _pD);
	TAutoVar_Pack(m_TargetAttachPoint, _pD);
}


void CAngelusChainSetup::Unpack(const uint8*& _pD)
{
	TAutoVar_Unpack(m_Name, _pD);
	TAutoVar_Unpack(m_iTemplateModel, _pD);
	TAutoVar_Unpack(m_AttachPoint.m_iNode, _pD);
	TAutoVar_Unpack(m_AttachPoint.m_LocalPos, _pD);
	TAutoVar_Unpack(m_TargetAttachPoint, _pD);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CAngelusChainState (SEC_BB Section BB)
|__________________________________________________________________________________________________
\*************************************************************************************************/
CAngelusChainState::CAngelusChainState()
	: m_Task(ANGELUSCHAIN_TASK_IDLE)
	, m_State(ANGELUSCHAIN_STATE_IDLE)
	, m_iTarget(0)
	, m_Speed(0)
	, m_TargetPos(0)
	, m_GrabPoint(0)
	, m_Length(0)
	, m_iRotTrack(0)
	, m_Pos(0)
	, m_LastTargetPos(0)
	, m_CurrTargetPos(0)
{
	m_Attach1_Cache.Unit();
}


void CAngelusChainState::Pack(uint8*& _pD) const
{
	TAutoVar_Pack(m_Task, _pD);
	TAutoVar_Pack(m_State, _pD);
	TAutoVar_Pack(m_iTarget, _pD);
	TAutoVar_Pack(m_Speed, _pD);
	TAutoVar_Pack(m_TargetPos, _pD);
	TAutoVar_Pack(m_GrabPoint, _pD);
	TAutoVar_Pack(m_Length, _pD);
	TAutoVar_Pack(m_iRotTrack, _pD);
//	TAutoVar_Pack(m_QueueState, _pD);
//	TAutoVar_Pack(m_QueueTask, _pD);
//	TAutoVar_Pack(m_QueueSpeed, _pD);
}


void CAngelusChainState::Unpack(const uint8*& _pD)
{
	TAutoVar_Unpack(m_Task, _pD);
	TAutoVar_Unpack(m_State, _pD);
	TAutoVar_Unpack(m_iTarget, _pD);
	TAutoVar_Unpack(m_Speed, _pD);
	TAutoVar_Unpack(m_TargetPos, _pD);
	TAutoVar_Unpack(m_GrabPoint, _pD);
	TAutoVar_Unpack(m_Length, _pD);
	TAutoVar_Unpack(m_iRotTrack, _pD);
//	TAutoVar_Unpack(m_QueueState, _pD);
//	TAutoVar_Unpack(m_QueueTask, _pD);
//	TAutoVar_Unpack(m_QueueSpeed, _pD);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_AngelusChain_ClientData (SEC_CC Section CC)
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWO_AngelusChain_ClientData::CWO_AngelusChain_ClientData()
	: m_pObj(NULL)
	, m_pWPhysState(NULL)
{
}


void CWO_AngelusChain_ClientData::Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	m_pObj = _pObj;
	m_pWPhysState = _pWPhysState;
}


void CWO_AngelusChain_ClientData::OnRefresh()
{
	CWorld_Server* pWServer = m_pWPhysState->IsServer() ? safe_cast<CWorld_Server>(m_pWPhysState) : NULL;
	CWorld_Client* pWClient = m_pWPhysState->IsClient() ? safe_cast<CWorld_Client>(m_pWPhysState) : NULL;

	// Make sure chain is positioned at the same place as owner
	CWObject_CoreData* pOwner = m_pWPhysState->Object_GetCD(m_iOwner);
	if (pOwner)
	{
		const CMat4Dfp32& OwnerPos = pOwner->GetPositionMatrix();

		fp32 DistSqr = OwnerPos.GetRow(3).DistanceSqr(m_pObj->GetPosition());
		if (DistSqr > 8.0f)
			m_pWPhysState->Object_SetPosition(m_pObj->m_iObject, OwnerPos);
	}

	const CVec3Dfp32& ObjPos = m_pObj->GetPosition();
	CBox3Dfp32 BBox(ObjPos - CVec3Dfp32(4.0f), ObjPos + CVec3Dfp32(4.0f));

	// Update all the chains
	uint nChains = m_lChainState.Len();
	for (uint iChain = 0; iChain < nChains; iChain++)
	{
		CAngelusChainState& ChainState = m_lChainState[iChain];
		CAngelusChainSetup& ChainSetup = m_lChainSetup[iChain];

		if (ChainState.m_State == ANGELUSCHAIN_STATE_IDLE)
			continue;

		CSpline_Chain Spline;
		GetSpline(iChain, Spline);

		// Enlarge bounding box
		for(uint iPoint = 0; iPoint < Spline.m_lPoints.Len(); iPoint++)
			BBox.Expand(Spline.m_lPoints[iPoint].m_Pos);

		// Move chain towards target
		ChainState.m_Length = Clamp(ChainState.m_Length + ChainState.m_Speed, 0.0f, Spline.m_Length);
		if (M_Fabs(ChainState.m_Speed - ANGELUSCHAIN_SPEED_SNAPTOTARGET) < 0.1f)
			ChainState.m_Length = Spline.m_Length;

		bool bAtTarget = (ChainState.m_Speed < 0.0f) ? (ChainState.m_Length <= 0.0f) : (ChainState.m_Length >= Spline.m_Length);

		CSpline_Chain::SplinePos Pos;
		Spline.FindPos(ChainState.m_Length, Pos, Spline.m_EndMat.GetRow(2));
		ChainState.m_Pos = Pos.mat.GetRow(3);

		CMat4Dfp32 Rot;
		Spline.CalcMat(1.0f, Rot, CVec3Dfp32(0,0,1));
//		ChainState.m_Dir = Rot.GetRow(2);

		// Update chain states on server only
		if (pWServer)
			Server_UpdateState(ChainState, bAtTarget, Pos.mat);
	}

	// Debug rendering (client)
	#ifndef M_RTM
	if (0) {
		if (pWClient)
			pWClient->Debug_RenderAABB(BBox.m_Min, BBox.m_Max, 0xffff0000, 1.0f, true);
	}
	#endif

	// Set new bound box
	CMat4Dfp32 InvObjMat;
	m_pObj->GetPositionMatrix().InverseOrthogonal(InvObjMat);
	BBox.Transform(InvObjMat, BBox);
	if(pWClient)
		m_pWPhysState->Object_SetVisBox(m_pObj->m_iObject, BBox.m_Min, BBox.m_Max);
	m_pObj->SetVisBoundBox(BBox);
}


void CWO_AngelusChain_ClientData::Server_UpdateState(CAngelusChainState& _ChainState, bool _bAtTarget, const CMat4Dfp32& _EndPos)
{
	CWorld_Server* pWServer = safe_cast<CWorld_Server>(m_pWPhysState);
	CWObject_AngelusChain* pServerObj = safe_cast<CWObject_AngelusChain>(m_pObj);
	CWObject_CoreData* pOwner = m_pWPhysState->Object_GetCD(m_iOwner);
	const CMat4Dfp32& ObjMat = pOwner ? pOwner->GetPositionMatrix() : m_pObj->GetPositionMatrix();
	const CVec3Dfp32& ObjPos = ObjMat.GetRow(3);
	//fp32 Time = (m_pWPhysState->GetGameTime() - m_StartTime).GetTime();
	CWObject* pTarget = NULL;

	// Handle objects being destroyed while holding them etc.
	if (_ChainState.m_iTarget > 0)
	{
		pTarget = pWServer->Object_Get(_ChainState.m_iTarget);
		if (!pTarget)
		{
			_ChainState.m_iTarget = 0;
			_ChainState.m_State = ANGELUSCHAIN_STATE_RETURN;
			_ChainState.m_Speed = -6.0f;
			return;
		}
	}

	// Do stuff when chain reaches it's target/objective
	if(_bAtTarget)
	{
		switch (_ChainState.m_State)
		{
			case ANGELUSCHAIN_STATE_REACHPOSITION:
			{
				// Send damage to untargeted stuff
				//const CVec3Dfp32& Forward = _EndPos.GetRow(0);
				//SendProjectileDamage(pWServer, _Arm.m_Pos + Forward * -16.0f, _Arm.m_Pos + Forward * 5.0f, Forward, 10);

				// Update chain state
				_ChainState.m_State = ANGELUSCHAIN_STATE_RETURN;
				_ChainState.m_Speed = -6.0f;
				m_lChainState.MakeDirty();

				// Play 'return' sound (once)
				pWServer->Sound_At(_ChainState.m_Pos, m_liSounds[ANGELUSCHAIN_SOUND_RETURN], 0);
			}
			break;

			case ANGELUSCHAIN_STATE_REACHOBJECT:
			{
				CWObject_Character* pChar = TDynamicCast<CWObject_Character>(pTarget);
				if (pChar)
				{
				}
				else if (pTarget->m_pRigidBody2)
				{
					_ChainState.m_PidIntegrateTerm = 0;
					_ChainState.m_PidDeriveTerm = 0;
					_ChainState.m_LastTargetPos = pTarget->GetPosition();
				}

				if (_ChainState.m_Task == ANGELUSCHAIN_TASK_GRABHOLD)
				{
					if (pChar)
					{
						// Create blood effect on character
						//SCollisionFlags ColFlags(_Arm.m_Pos + (_Arm.m_Dir * -4.0f), _Arm.m_Pos + (_Arm.m_Dir * 4.0f));
						//ColFlags.m_iExclude = m_iOwner;
						//ColFlags.m_ObjectFlags = OBJECT_FLAGS_CHARACTER;
						//CreateBloodEffect(pTarget->m_iObject, _Arm.m_Pos, &ColFlags, true, "hitEffect_tentacle", &_Arm.m_Pos, NULL, 1, 2);
					}
					else
					{
						// Damage the targeted object
						//CWO_Damage Damage(10, DAMAGETYPE_DARKNESS, 0);
						//Damage.SendExt(pTarget->m_iObject, m_iOwner, pWServer, NULL, NULL, 0, 0);
					}

					// Play 'grab' sound (once)
					//pWServer->Sound_At(_Arm.m_Pos, m_liSounds[TENTACLE_SOUND_GRAB], 0);

					// Make the character scream
					//int iVoice = 300;
					//pWServer->Message_SendToObject(CWObject_Message(OBJMSG_RPG_SPEAK, iVoice), pTarget->m_iObject);

					// Force drop his/her weapon
					//pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_FORCEDROPWEAPON), pTarget->m_iObject);

					// Enter hold-state
					//_ChainState.UpdateState(ANGELUSCHAIN_STATE_GRABOBJECT, ANGELUSCHAIN_SPEED_SNAPTOTARGET);
					//m_ArmControl.k[0] = (_ChainState.m_Pos - ObjPos).Length();
				}
				else if (_ChainState.m_Task == ANGELUSCHAIN_TASK_BREAKOBJECT)
				{
					// Damage the targeted object
					//CWO_Damage Damage(10, DAMAGETYPE_DARKNESS, 0);
					//Damage.SendExt(pTarget->m_iObject, m_iOwner, pWServer, NULL, NULL, 0, 0);

					// Go back to idle-state
					_ChainState.m_State = ANGELUSCHAIN_STATE_RETURN;
					_ChainState.m_Speed = -6.0f;
				}
				m_lChainState.MakeDirty();
			}
			break;

			case ANGELUSCHAIN_STATE_GRABOBJECT:
			{
				// Nope, I don't belive that's how a chain behaves when grabing something.
				/*
				CVec3Dfp32 NewPos = ObjPos;

				NewPos.k[2] += 56.0f; // yeah yeah
				NewPos += ObjMat.GetRow(0) * m_ChainControl.k[0];

				// wiggle it
				NewPos += CVec3Dfp32(16.0f*M_Sin(Time*1.0f + 0.0f), 
									16.0f*M_Sin(Time*1.1f + 1.0f),
									16.0f*M_Sin(Time*1.2f + 2.0f));

				// don't go below "floor"
				NewPos.k[2] = Max(NewPos.k[2], ObjPos.k[2]);

				#ifndef M_RTM
				{
					pWServer->Debug_RenderVector(ObjPos, NewPos - ObjPos, 0xff0000ff, 0.5f, true);
				}
				#endif
				
				CWObject_Character* pCorpse = TDynamicCast<CWObject_Character>(pTarget);
				if (pCorpse && pCorpse->m_spRagdoll)
				{
					pCorpse->m_spRagdoll->SetPendingBonepos(_ChainState.m_iRotTrack, NewPos);
				}
				else if (pTarget->m_pRigidBody)
				{
					const CVec3Dfp32& CurrPos = pTarget->GetPosition();
					const CVec3Dfp32& LastPos = _ChainState.m_LastTargetPos;

					CVec3Dfp32 Diff = NewPos - CurrPos;
					fp32 Mass = pTarget->GetMass();

					fp32 K = 2.0f * Mass;
					fp32 Ti = 350.0f;
					fp32 Td = 5.0f;
					fp32 N = 10.0f;
					fp32 h = 1.0f;
					_ChainState.m_PidIntegrateTerm += Diff * (K/Ti);
					_ChainState.m_PidDeriveTerm *= (Td / (Td + N*h));
					_ChainState.m_PidDeriveTerm -= (CurrPos - LastPos)*(K*Td*N/(Td + N*h));

					pWServer->Phys_AddForce(_ChainState.m_iTarget, CVec3Dfp32(0, 0, 9.8f*Mass)); // Cancel out gravity
					pWServer->Phys_AddForce(_ChainState.m_iTarget, Diff*K + _ChainState.m_PidIntegrateTerm + _Arm.m_PidDeriveTerm);

					_ChainState.m_LastTargetPos = CurrPos;
				}
				*/
			}
			break;

			case ANGELUSCHAIN_STATE_RETURN:
			{
				CWObject_Character* pCorpse = TDynamicCast<CWObject_Character>(pTarget);

				// Stop 'hold' sound
				pServerObj->iSound(0) = 0;

				_ChainState.m_Task = ANGELUSCHAIN_TASK_IDLE;
				_ChainState.m_State = ANGELUSCHAIN_STATE_IDLE;
				
				// Reset any targets
				_ChainState.m_iTarget = 0;
				m_lChainState.MakeDirty();
			}
			break;
		}
	}

	// Handle chain states on it's way to target/objective
	//else
	//{
	//	switch (_ChainState.m_State)
	//	{
	//	}
	//}
}


void CWO_AngelusChain_ClientData::GetSpline(uint8 _iChain, CSpline_Chain& _Spline)
{
	bool bClient = m_pWPhysState->IsClient();
	bool bRendering = (m_Flags.m_Value & ANGELUSCHAIN_CD_ONCLIENTRENDER) != 0;

	// Get some info about the owner object, if any
	CWObject_CoreData* pOwner = m_pWPhysState->Object_GetCD(m_iOwner);
	if (pOwner && bClient && bRendering)
		pOwner = CWObject_Character::Player_GetLastValidPrediction(safe_cast<CWObject_Client>(pOwner));

	const CMat4Dfp32& BasePos = pOwner ? pOwner->GetPositionMatrix() : m_pObj->GetPositionMatrix();

	CMat4Dfp32 CameraMat;
	CVec3Dfp32 CameraDiff;

	CXR_Skeleton* pOwnerSkeleton = NULL;
	CXR_SkeletonInstance* pOwnerSkelInstance = NULL;
	if (pOwner)
	{
		CWO_Character_ClientData* pOwnerCD = CWObject_Character::GetClientData(pOwner);
		CXR_Model* pModel = m_pWPhysState->GetMapData()->GetResource_Model(pOwner->m_iModel[0]);
		if (pOwnerCD && pModel)
		{
			CXR_AnimState Anim;
	        fp32 IPTime = pOwnerCD->m_PredictFrameFrac;

			CWObject_Character::GetEvaluatedPhysSkeleton(pOwner, m_pWPhysState, pOwnerSkelInstance, pOwnerSkeleton, Anim, IPTime, &BasePos);
			CameraMat = pOwnerSkelInstance->m_pBoneTransform[24];
			CameraDiff = 0;

			bool bCutSceneView = (pOwner->m_ClientFlags & (PLAYER_CLIENTFLAGS_CUTSCENE | PLAYER_CLIENTFLAGS_DIALOGUE)) != 0;
			bool bThirdPerson = (pOwnerCD->m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON | PLAYER_EXTRAFLAGS_FORCETHIRPERSON)) != 0;

			CWO_Character_ClientData* pCDFirst = pOwnerCD;
			if (bClient && bRendering && !bThirdPerson && !bCutSceneView)
			{
				CWObject_CoreData* pObjFirst = ((CWorld_Client *)m_pWPhysState)->Object_GetFirstCopy(m_iOwner);
				if (pObjFirst)
					pCDFirst = CWObject_Character::GetClientData(pObjFirst);

				CVec3Dfp32 x = pOwnerSkeleton->m_lNodes[24].m_LocalCenter;
				x.MultiplyMatrix3x3(CameraMat);
				CameraDiff = CameraMat.GetRow(3);
				CameraMat.GetRow(3) = pCDFirst->m_Camera_LastPosition - x;
				CameraDiff = CameraMat.GetRow(3) - CameraDiff;
			}
		}
	}
	const CAngelusChainSetup& ChainSetup = m_lChainSetup[_iChain];
	      CAngelusChainState& ChainState = m_lChainState[_iChain];

	// Calculate attach positions
	CMat4Dfp32 AttachMat;
	//const fp32 TanLen[] = { 4.0f, 2.0f, 1.0f, 1.0f };
	const fp32 TanLen = 0.1f;//1.0f;
	if(pOwnerSkeleton && pOwnerSkelInstance)
	{
		int iRotTrack = ChainSetup.m_AttachPoint.m_iNode;
		iRotTrack = 21;
		const CMat4Dfp32& BoneTransform = pOwnerSkelInstance->m_pBoneTransform[iRotTrack];
		ChainSetup.m_AttachPoint.m_LocalPos.Multiply(BoneTransform, AttachMat);
		
		/*
		CMat4Dfp32 TestLocalPos;
		TestLocalPos.CreateFrom(ChainSetup.m_AttachPoint.m_LocalPos);
		TestLocalPos.GetRow(3) = CVec3Dfp32(4,32,32);
		TestLocalPos.Multiply(BoneTransform, AttachMat);
		*/

		// Safety code to fix bad BoneTransforms
		if (!(AttachMat.k[0][0] > -_FP32_MAX && AttachMat.k[0][0] < _FP32_MAX))
			ChainSetup.m_AttachPoint.m_LocalPos.Multiply(BasePos, AttachMat);
	}
	else
	{
		ChainSetup.m_AttachPoint.m_LocalPos.Multiply(BasePos, AttachMat);
	}

	_Spline.AddPoint(AttachMat, AttachMat.GetRow(0) * TanLen);

	// Calculate target pos (if any)
	CVec3Dfp32 TargetPos = ChainState.m_TargetPos;
	if (ChainState.m_iTarget)
	{
		CWObject_CoreData* pTarget = m_pWPhysState->Object_GetCD(ChainState.m_iTarget);
		if (pTarget)
		{
			fp32 IPTime = 1.0f;
			if (bClient && bRendering)
				IPTime = safe_cast<CWorld_Client>(m_pWPhysState)->GetRenderTickFrac();

			TargetPos = pTarget->GetPosition() * IPTime + pTarget->GetLastPosition() * (1.0f - IPTime);

			if (pTarget->m_iClass == m_pWPhysState->GetMapData()->GetResourceIndex_Class("CharNPC"))
				GetCharBonePos(*pTarget, ChainState.m_iRotTrack, TargetPos);
			else
			{
				CVec3Dfp32 Offset = ChainState.m_GrabPoint;
				Offset.MultiplyMatrix3x3(pTarget->GetPositionMatrix());
				TargetPos += Offset;
			}
		}
	}

	if (TargetPos != CVec3Dfp32(0))
	{
		// move to target
		CMat4Dfp32 Mat;
		
		Mat.GetRow(0) = (TargetPos - AttachMat.GetRow(3)).Normalize();
		Mat.GetRow(2) = AttachMat.GetRow(2);
		Mat.RecreateMatrix(0, 2);
		Mat.GetRow(3) = TargetPos;
		_Spline.AddPoint(Mat, Mat.GetRow(0));
	}

	_Spline.Finalize();
}


bool CWO_AngelusChain_ClientData::GetCharBonePos(CWObject_CoreData& _Char, int _RotTrack, CVec3Dfp32& _Result)
{
	CXR_AnimState Anim;
	CXR_Skeleton* pSkel;
	CXR_SkeletonInstance* pSkelInstance;					
	if (CWObject_Character::GetEvaluatedPhysSkeleton(&_Char, m_pWPhysState, pSkelInstance, pSkel, Anim))
	{
		const CMat4Dfp32& Mat = pSkelInstance->m_pBoneTransform[_RotTrack];
		_Result = pSkel->m_lNodes[_RotTrack].m_LocalCenter;
		_Result *= Mat;
		return true;
	}
	return false;
}


void CWO_AngelusChain_ClientData::OnRender(CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	// Check if we should render anything
	uint nChains = m_lChainSetup.Len();
	if (nChains == 0 || ((int16)m_Flags & ANGELUSCHAIN_CD_NORENDER))
		return;

	CWorld_Client* pWClient = safe_cast<CWorld_Client>(m_pWPhysState);
	fp32 IPTime = pWClient->GetRenderTickFrac();

	// Get interpolated matrix
	CMat4Dfp32 BasePos;
	CWObject_Client* pOwner = pWClient->Object_Get(m_iOwner);
	if (pOwner)
	{
		pOwner = CWObject_Character::Player_GetLastValidPrediction(pOwner);
		BasePos = pOwner->GetPositionMatrix();
	}
	else
	{
		Interpolate2(m_pObj->GetLastPositionMatrix(), m_pObj->GetPositionMatrix(), BasePos, IPTime);
	}

	// Check render flags
	uint RenderFlags = 0;
	if (!pWClient->m_Render_CharacterShadows)
		RenderFlags |= CXR_MODEL_ONRENDERFLAGS_NOSHADOWS;

	// Fetch wire container
	#ifndef M_RTM
		CWireContainer* pWC = pWClient->Debug_GetWireContainer();
	#endif

	// Render each chain object
	for (uint iChain = 0; iChain < nChains; iChain++)
	{
		// Is chain idle or active?
		const CAngelusChainState& ChainState = m_lChainState[iChain];
		if (ChainState.m_State == ANGELUSCHAIN_STATE_IDLE)
			continue;

		const CAngelusChainSetup& ChainSetup = m_lChainSetup[iChain];

		// Get rendering spline
		CSpline_Chain Spline;
		m_Flags.m_Value |= ANGELUSCHAIN_CD_ONCLIENTRENDER;
		GetSpline(iChain, Spline);
		m_Flags.m_Value &= ~ANGELUSCHAIN_CD_ONCLIENTRENDER;
		
		const fp32 Scale = 1.0f;

        // Get some template-mesh information
		CXR_Model* pMesh = NULL;
		CXR_Skeleton* pMeshSkel = NULL;
		uint nMeshBones = 0;
		uint iMeshStartBone = 0;
		uint iMeshEndBone = 0;
		fp32 MeshLength = 0.0f;
		{
			pMesh = _pWClient->GetMapData()->GetResource_Model(ChainSetup.m_iTemplateModel);
			pMeshSkel = (pMesh) ? pMesh->GetSkeleton() : NULL;
			nMeshBones = (pMeshSkel) ? pMeshSkel->m_lNodes.Len() : 0;
			
			if (!pMesh || !pMeshSkel || nMeshBones < 2)
				continue;

			fp32 xMin = _FP32_MAX, xMax = -_FP32_MAX;
			for (uint iEvalBone = 0; iEvalBone < nMeshBones; iEvalBone++)
			{
				fp32 x = pMeshSkel->m_lNodes[iEvalBone].m_LocalCenter.k[0];
				if (x < xMin)
				{
					iMeshStartBone = iEvalBone;
					xMin = x;
				}
				if (x > xMax)
				{
					iMeshEndBone = iEvalBone;
					xMax = x;
				}
			}
			MeshLength = xMax - xMin;
		}
		
		// Place template meshes along spline
		fp32 ChainLength = ChainState.m_Length + IPTime * ChainState.m_Speed;
		fp32 PosOnSpline = Clamp(ChainLength, 0.0f, Spline.m_Length);

		if (M_Fabs(ChainState.m_Speed - ANGELUSCHAIN_SPEED_SNAPTOTARGET) < 0.1f)
			PosOnSpline = Spline.m_Length;

		fp32 TotalSpin = 0.0f;
		CSpline_Tentacle::SplinePos SpEndPos;
		Spline.FindPos(PosOnSpline, SpEndPos, Spline.m_EndMat.GetRow(2));
		CMat4Dfp32 EndPos = SpEndPos.mat, StartPos;

		// Debug rendering
		#ifndef M_RTM
		{
			if (pWC)
				pWC->RenderMatrix(SpEndPos.mat, 0.0f, false);
		}
		#endif

		for (uint i = 0; PosOnSpline > 0.0f; i++)
		{
			CXR_AnimState Anim;
			Anim.m_iObject = m_pObj->m_iObject;
			Anim.m_pSkeletonInst = CXR_VBMHelper::Alloc_SkeletonInst(_pEngine->GetVBM(), nMeshBones, pMeshSkel->m_nUsedRotations, pMeshSkel->m_nUsedMovements);
			
			// break if we can't allocate skeleton instance
			if (!Anim.m_pSkeletonInst)
				break;

			CMat4Dfp32* pDestBones = Anim.m_pSkeletonInst->m_pBoneTransform;
			CMat4Dfp32 Inv, Mat, AMat;
			Inv.Unit();

			// Evaluate each bone
			fp32 Length = MeshLength * Scale;
			const CVec3Dfp32& Origo = pMeshSkel->m_lNodes[iMeshStartBone].m_LocalCenter;
			CSpline_Tentacle::SplinePos sp;
			CVec3Dfp32 LastPos = EndPos.GetRow(3);
						
			for (int iBone = nMeshBones - 1; iBone >= 0; iBone--)
			{
				const CVec3Dfp32& LocalCenter = pMeshSkel->m_lNodes[iBone].m_LocalCenter;
				fp32 CurrX = (LocalCenter.k[0] - Origo.k[0]) * Scale;
				fp32 d = PosOnSpline - Length + CurrX;
				d = Max(0.0f, d);
				Spline.FindPos(d, sp, EndPos.GetRow(2));
				sp.mat.GetRow(3) += sp.mat.GetRow(1) * ((LocalCenter.k[1] - Origo.k[1]) * Scale);
				sp.mat.GetRow(3) += sp.mat.GetRow(2) * ((LocalCenter.k[2] - Origo.k[2]) * Scale);

				fp32 Spin = 0.0f; //(i==0) ? 0.0f : TotalSpin + (Length - CurrX)/Scale * ChainSetup.m_Spin;
				Inv.SetXRotation(Spin);
				Inv.GetRow(3) = -LocalCenter;

				CMat4Dfp32 MatScale;
				MatScale.Unit();
				MatScale.Multiply(Scale);
				MatScale.Multiply(pDestBones[iBone], pDestBones[iBone]);
				
				Mat = Inv;

				// Create bone matrix
				bool bEndOfTileSegment = (iBone == iMeshEndBone);
				if (bEndOfTileSegment)
					Mat.Multiply(EndPos, pDestBones[iBone]); // force snap
				else
					Mat.Multiply(sp.mat, pDestBones[iBone]);

				// Calculate actual world position
				Mat = pDestBones[iBone];
				Mat.GetRow(3) = LocalCenter * Mat;

				if (iBone == iMeshStartBone)
					StartPos = Mat;

			}

			#ifndef M_RTM
			{
				if (pWC)
					pWC->RenderMatrix(StartPos, 0.0f, false);
			}
			#endif

			_pEngine->Render_AddModel(pMesh, StartPos, Anim, XR_MODEL_STANDARD, RenderFlags);

			EndPos = StartPos;
			PosOnSpline -= Length;
			
			//if (i > 0)
			//	TotalSpin += Length * ChainSetup.m_Spin;
		}
	}

	m_LastRender = pWClient->GetRenderTime();
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_AngelusChain (SEC_DD Section DD)
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_AngelusChain::CWObject_AngelusChain()
{
}


const CWO_AngelusChain_ClientData& CWObject_AngelusChain::GetClientData(const CWObject_CoreData* _pObj)
{
	const CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who deleted my client data?!");
	return *safe_cast<const CWO_AngelusChain_ClientData>(pData);
}


CWO_AngelusChain_ClientData& CWObject_AngelusChain::GetClientData(CWObject_CoreData* _pObj)
{
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who deleted my client data?!");
	return *safe_cast<CWO_AngelusChain_ClientData>(pData);
}


void CWObject_AngelusChain::OnIncludeClass(CMapData* _pMapData, CWorld_Server* _pWServer)
{
	parent::OnIncludeClass(_pMapData, _pWServer);
	//_pMapData->GetResourceIndex_Class("hitEffect_tentacle");
	//_pMapData->GetResourceIndex_Class("hitEffect_tentacle2");
}


void CWObject_AngelusChain::OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer)
{
	uint nChildren = _pReg->GetNumChildren();
	for (uint i = 0; i < nChildren; i++)
	{
		CStr KeyName = _pReg->GetName(i);
		CStr KeyValue = _pReg->GetValue(i);

		if (KeyName == "CHAIN")
		{
			CAngelusChainSetup tmp;
			tmp.Setup(*_pReg->GetChild(i), *_pWServer);
		}
		else if (KeyName.CompareSubStr("SOUND_") == 0)
		{
			_pMapData->GetResourceIndex_Sound(KeyValue.Str());
		}
		else
			parent::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
	}
}


void CWObject_AngelusChain::OnCreate()
{
	m_bNoSave = true;
	GetClientData(this).m_iOwner = m_iOwner;

	m_pSelectedObject = NULL;
	m_LocalGrabPoint = 0.0f;
}


void CWObject_AngelusChain::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MSCOPE(CWObject_TentacleSystem::OnEvalKey, TENTACLES);

	CWO_AngelusChain_ClientData& CD = GetClientData(this);
	CMapData* pMapData = m_pWServer->GetMapData();
	CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH2('CHAI','N'): // "CHAIN"
		{
			int iChain = CD.m_lChainSetup.Len();
			if(iChain >= CD.m_lChainSetup.GetMax())
				Error_static("CWobject_AngelusChain::OnEvalKey", CStrF("Too many chain \"arms\"! (max = %d)", CD.m_lChainSetup.GetMax()));

			CD.m_lChainSetup.SetLen(iChain + 1);
			CD.m_lChainSetup[iChain].Setup(*_pKey, *m_pWServer);
			CD.m_lChainSetup.MakeDirty();

			CD.m_lChainState.SetLen(iChain + 1);
			CD.m_lChainState.MakeDirty();
			break;
		}
	case MHASH2('SHAD','OWS'): // "SHADOWS"
		{
			if (KeyValue.Val_int() != 0)
				m_ClientFlags |= CWO_CLIENTFLAGS_SHADOWCASTER;
			else
				m_ClientFlags &= ~CWO_CLIENTFLAGS_SHADOWCASTER;
			break;
		}
	default:
		{
			if (KeyName.CompareSubStr("SOUND_") == 0)
			{
				static const char* SoundKeys[] = { "SOUND_REACH", "SOUND_GRAB", "SOUND_HOLD", "SOUND_RETURN", NULL };
				int iSlot = KeyName.TranslateInt(SoundKeys);
				if(iSlot >= 0)
				{
					if(CD.m_liSounds.Len() <= iSlot)
						CD.m_liSounds.SetLen(iSlot + 1);

					//CD.m_liSounds.SetLen(iSlot + 1);
					CD.m_liSounds[iSlot] = pMapData->GetResourceIndex_Sound(_pKey->GetThisValue().Str());
					CD.m_liSounds.MakeDirty();
				}
			}
			else
				parent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}


int CWObject_AngelusChain::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const
{
	MSCOPE(CWObject_AngelusChain::OnCreateClientUpdate, ANGELUSCHAIN);

	uint8* pD = _pData;

	int Flags = CWO_CLIENTUPDATE_EXTRADATA;
	if (_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
		Flags |= CWO_CLIENTUPDATE_AUTOVAR;

	pD += parent::OnCreateClientUpdate(_iClient, _pClObjInfo, _pOld, _pData, Flags);
	if ((pD - _pData) == 0)
		return 0;

	const CWO_AngelusChain_ClientData& CD = GetClientData(this);

	{ // Handle AutoVars, remove flag used for ag2)
		CD.AutoVar_Pack((_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT), pD, m_pWServer->GetMapData(), 0);
	}

	return (pD - _pData);
}


void CWObject_AngelusChain::OnRefresh()
{
	CWO_AngelusChain_ClientData& CD = GetClientData(this);
	CD.OnRefresh();

	m_DirtyMask |= (CD.AutoVar_RefreshDirtyMask()) << CWO_DIRTYMASK_USERSHIFT;
}


void CWObject_AngelusChain::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_AngelusChain::OnClientRefresh);
	CWO_AngelusChain_ClientData& CD = GetClientData(_pObj);
	CD.OnRefresh();
}


void CWObject_AngelusChain::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	GetClientData(_pObj).OnRender(_pWClient, _pEngine);
}


int CWObject_AngelusChain::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	CBox3Dfp32 BBox;
	_pObj->GetVisBoundBox(BBox);

	const uint8 *pD = _pData;
	pD += CWObject::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);
	if (_pObj->m_iClass == 0 || (pD - _pData) == 0)
		return 0;

	CWO_AngelusChain_ClientData& CD = GetClientData(_pObj);

	if (_pObj->m_bAutoVarDirty)
	{ // Handle AutoVars
		CD.AutoVar_Unpack(pD, _pWClient->GetMapData(), 0);
	}

	_pWClient->Object_SetVisBox(_pObj->m_iObject, BBox.m_Min, BBox.m_Max);

	return (pD - _pData);
}


void CWObject_AngelusChain::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA];
	CWO_AngelusChain_ClientData* pCD = TDynamicCast<CWO_AngelusChain_ClientData>(pData);

	// Allocate clientdata
	if (!pCD || pCD->m_pObj != _pObj || pCD->m_pWPhysState != _pWPhysState)
	{
		pCD = MNew(CWO_AngelusChain_ClientData);
		if (!pCD)
			Error_static("CWObject_AngelusChain", "Could not allocate client data!");

		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = pCD;
		pCD->Clear(_pObj, _pWPhysState);
	}
}


CWObject* CWObject_AngelusChain::SelectTarget(uint _SelectionMask)
{
	int iClassCharPlayer = m_pWServer->GetMapData()->GetResourceIndex_Class("CharPlayer");
	//int iClassCharNPC = m_pWServer->GetMapData()->GetResourceIndex_Class("CharNPC");
	//int iClassLamp = m_pWServer->GetMapData()->GetResourceIndex_Class("Trigger_Lamp");

	const fp32 MaxArmLength = 800.0f; // 25 meters
	const CMat4Dfp32& Mat = GetPositionMatrix();
	const CVec3Dfp32& Dir = Mat.GetRow(0);
	CVec3Dfp32 StartPos = Mat.GetRow(3) + CVec3Dfp32(0,0,54);
	CVec3Dfp32 Center = StartPos + Dir * (MaxArmLength * 0.5f);
	fp32 Radius = MaxArmLength * 0.7f;
	CVec3Dfp32 EndPos;
	CCollisionInfo CInfo;

	uint nObjectFlags = 0;
	if (_SelectionMask & ANGELUSCHAIN_SELECTION_CHARACTER)
		nObjectFlags |= OBJECT_FLAGS_CHARACTER;

	if (_SelectionMask & ANGELUSCHAIN_SELECTION_OBJECT)
		nObjectFlags |= OBJECT_FLAGS_OBJECT;

	CWObject* pBestObj = NULL;

	const int16* pSel = NULL;
	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pWServer->Selection_AddBoundSphere(Selection, nObjectFlags, Center, Radius);
	uint nSel = m_pWServer->Selection_Get(Selection, &pSel);
	if (nSel > 0)
	{
		fp32 BestScore = 0.0f;
		for (uint i=0; i<nSel; i++)
		{
			int iObject = pSel[i];
			if (iObject == m_iObject || iObject == m_iOwner)
				continue;

			CWObject* pObj = m_pWServer->Object_Get(iObject);
			const CWO_PhysicsState& Phys = pObj->GetPhysState();

			uint nAcceptMask = 0;
			//bool bIsChar = (pObj->m_iClass == iClassCharPlayer) || (pObj->m_iClass == iClassCharNPC);
			bool bIsPlayer = (pObj->m_iClass == iClassCharPlayer);
			bool bIsDead = false;
			//if (bIsChar)
			if (bIsPlayer)
			{
				CWObject_Character* pChar = safe_cast<CWObject_Character>(pObj);
//				if (pChar->m_Flags & PLAYER_FLAGS_RAGDOLL) // must have ragdoll...
//				{
					bIsDead = (CWObject_Character::Char_GetPhysType(pChar) == PLAYER_PHYS_DEAD);
//					if (bIsDead)
//						nAcceptMask |= TENTACLE_SELECTION_CORPSE;
//
					bool bIsZombie = (pChar->m_spAI->m_CharacterClass == CAI_Core::CLASS_UNDEAD);
					if (!pChar->IsImmune() && !bIsZombie && !bIsDead)
						nAcceptMask |= ANGELUSCHAIN_SELECTION_CHARACTER;
//				}
			}
//			else if (pObj->m_iClass == iClassLamp)
//			{
//				if (pObj->iAnim0() != 2) // check if lamp is broken
//					nAcceptMask = TENTACLE_SELECTION_LAMP;
//			}
			else if (Phys.m_ObjectFlags & OBJECT_FLAGS_OBJECT)
			{
				if (pObj->m_pRigidBody2)
					nAcceptMask = TENTACLE_SELECTION_OBJECT;
			}

			if ((_SelectionMask & nAcceptMask) == 0)
				continue;

			CVec3Dfp32 ObjPos;
			pObj->GetAbsBoundBox()->GetCenter(ObjPos);

			CVec3Dfp32 PosToObj = ObjPos - StartPos;
			fp32 ProjDist = Dir * PosToObj;
			if (ProjDist < 1.0f || ProjDist > MaxArmLength)
				continue;

			fp32 Distance = PosToObj.Length();
			fp32 CosAngle = ProjDist / Distance;
			if (CosAngle < 0.2f)
				continue;

			// test intersection
			EndPos = ObjPos;
			if (Phys.m_ObjectFlags & OBJECT_FLAGS_OBJECT)
			{
				// For objects, just trace forward (will look for grab point anyway)
				EndPos = StartPos + Dir * ProjDist;
			}
			else if ((Phys.m_nPrim > 0) && (Phys.m_PhysFlags & OBJECT_PHYSFLAGS_OFFSET))
			{
				CVec3Dfp32 Offset = Phys.m_Prim[0].GetOffset();
				if (Phys.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION)
					Offset.MultiplyMatrix3x3(pObj->GetPositionMatrix());
				EndPos = pObj->GetPosition() + Offset;
			}

			bool bHit = m_pWServer->Phys_IntersectLine(StartPos, EndPos, 
				OBJECT_FLAGS_PROJECTILE, OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL, 
				XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS, &CInfo, m_iOwner);
			if (bHit && CInfo.m_bIsValid && CInfo.m_iObject != iObject && CInfo.m_iObject != m_iObject)
				continue;

			// Find grab-point
			if (Phys.m_ObjectFlags & OBJECT_FLAGS_OBJECT)
			{
				CWObject_Message Msg(OBJMSG_OBJECT_FIND_DEMONARMATTACHPOINT);
				Msg.m_VecParam0 = EndPos; // Msg.m_VecParam0 = StartPos; Msg.m_VecParam1 = Dir;
				const CVec3Dfp32* pAttachPoint = (const CVec3Dfp32*)pObj->OnMessage(Msg);
				if (!pAttachPoint)
					continue; // don't grab phys-objects that have no attachpoints

				EndPos = *pAttachPoint;
				EndPos *= pObj->GetPositionMatrix();
				m_pWServer->Debug_RenderVertex(EndPos);
			}

			fp32 Score = CosAngle * (1.0f - Distance / MaxArmLength);
			//if (bIsChar && !bIsDead)
			if (bIsPlayer && !bIsDead)
				Score += 1.0f;
			//else if (!bIsChar)
			else if (!bIsPlayer)
				Score += 0.5f;

			if (Score > BestScore)
			{
				BestScore = Score;
				pBestObj = pObj;
				m_LocalGrabPoint = EndPos;
			}
		}
	}
	m_pSelectedObject = pBestObj;

	if (pBestObj)
	{
		CMat4Dfp32 InvMat;
		pBestObj->GetPositionMatrix().InverseOrthogonal(InvMat);
		m_LocalGrabPoint *= InvMat;
	}

	return pBestObj;
}


void CWObject_AngelusChain::ReachPosition(const CVec3Dfp32& _Position)
{
	CWO_AngelusChain_ClientData& CD = GetClientData(this);
	CAngelusChainState& ChainState = CD.m_lChainState[ANGELUSCHAIN_CHAIN];

	ChainState.m_TargetPos = _Position;
	ChainState.m_Task = ANGELUSCHAIN_TASK_GETNOTHING;
	ChainState.m_State = ANGELUSCHAIN_STATE_REACHPOSITION;
	ChainState.m_Speed = 24.0f;
	CD.m_lChainState.MakeDirty();

	m_pWServer->Sound_At(GetPosition(), CD.m_liSounds[ANGELUSCHAIN_SOUND_REACH], 0);

	m_iSound[0] = CD.m_liSounds[ANGELUSCHAIN_SOUND_HOLD];
	m_pWServer->Sound_On(m_iObject, m_iSound[0], 2);
	m_DirtyMask |= CWO_DIRTYMASK_SOUND;
}


bool CWObject_AngelusChain::ReleaseObject()
{
	return true;
}

