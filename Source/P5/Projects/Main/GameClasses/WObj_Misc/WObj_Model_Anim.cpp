#include "PCH.h"
#include "WObj_Model_Anim.h"
#include "../../GameWorld/WServerMod.h"

// -------------------------------------------------------------------
//  Model_Anim
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Model_Anim, CWObject_Model_Anim_Parent, 0x0100);

CStr CWObject_Model_Anim::m_sAttachObjectName;
CWObject_Model_Anim::CWObject_Model_Anim()
{
	GetAnimModelClientData(this);
}

void CWObject_Model_Anim::OnCreate()
{
	m_iAttachObject = 0;
	m_PosOffset = 0.0f;
	m_RotOffset.Unit();
	m_AnimModelFlags = 0;
	m_AnimBaseMat.Unit();
	CWObject_Model_Anim_Parent::OnCreate();
}

void CWObject_Model_Anim::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	if(_pKey->GetThisName().Find("FLAGS") != -1)
	{
		static const char *FlagsTranslate[] =
		{
			"skiprotoffset","skipposoffset","physics", NULL
		};

		m_AnimModelFlags |= _pKey->GetThisValue().TranslateFlags(FlagsTranslate);
	}
	else if (_pKey->GetThisName().Find("ATTACH") != -1)
	{
		// Find object to attach to... (only available in onfinishevalkeys or onspawn..)
		m_sAttachObjectName = _pKey->GetThisValue();
	}
	else
		CWObject_Model_Anim_Parent::OnEvalKey(_KeyHash, _pKey);
}

void CWObject_Model_Anim::OnFinishEvalKeys()
{
	CWObject_Model_Anim_Parent::OnFinishEvalKeys();
}

void CWObject_Model_Anim::OnSpawnWorld()
{
	// Set norefresh to class
	ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;

	if (m_sAttachObjectName.Len() > 0)
	{
		m_iAttachObject = m_pWServer->Selection_GetSingleTarget(m_sAttachObjectName);
		// Clear static string
		m_sAttachObjectName.Clear();
		
		// Find offset
		CWObject_CoreData* pObj = m_pWServer->Object_GetCD(m_iAttachObject);
		if (pObj)
		{
			// Find Offset
			m_PosOffset = GetPosition() - pObj->GetPosition();
			CQuatfp32 Rot,RotSelf;
			Rot.Create(pObj->GetPositionMatrix());
			Rot.Inverse();
			RotSelf.Create(GetPositionMatrix());
			m_RotOffset = RotSelf * Rot;
		}
	}

	if (m_AnimModelFlags & MODEL_ANIM_FLAG_PHYSICS)
	{
		// Set some physics, find size of model or something...

		/*CVec3Dfp32 Offset(0,0,0);
		CWO_PhysicsState Phys;
		Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, 
			CVec3Dfp32(0, 0, 0),Offset);
		Phys.m_nPrim = 1;
		Phys.m_ObjectFlags = OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_PICKUP;
		Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD;
		Phys.m_PhysFlags = OBJECT_PHYSFLAGS_OFFSET;// | OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
		Phys.m_iExclude = -1;//_iExclude == -1 ? m_iOwner : _iExclude;
		// Well then, this doesn't work for the moment
		//m_pWServer->Object_SetPosition(m_iObject,GetPosition() + Offset);
		m_pWServer->Object_SetPhysics(m_iObject, Phys);*/
	}
}

CWObject_Model_Anim::CAnimModelClientData* CWObject_Model_Anim::GetAnimModelClientData(CWObject_CoreData* _pObj)
{
	if(_pObj->m_lspClientObj[0] == NULL)
	{
		_pObj->m_lspClientObj[0] = MNew(CAnimModelClientData);
		if(!_pObj->m_lspClientObj[0])
			Error_static("CWObject_Model_Anim::GetAnimModelClientData", "Could not allocate AttachData.")
		CAnimModelClientData *pData = (CAnimModelClientData *)(CReferenceCount *)_pObj->m_lspClientObj[0];
		return pData;
	}
	else
		return (CAnimModelClientData *)(CReferenceCount *)_pObj->m_lspClientObj[0];
}

const CWObject_Model_Anim::CAnimModelClientData* CWObject_Model_Anim::GetAnimModelClientData(const CWObject_CoreData* _pObj)
{
	return (const CAnimModelClientData *)(const CReferenceCount *)_pObj->m_lspClientObj[0];
}

int CWObject_Model_Anim::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const
{
	const CAnimModelClientData *pCD = GetAnimModelClientData(this);
	if(!pCD)
		Error_static("CWObject_Model_Anim::OnCreateClientUpdate", "Unable to pack client update.");
	
	const uint8* pD = _pData;
	int Flags = CWO_CLIENTUPDATE_EXTRADATA;
	if(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
		Flags |= CWO_CLIENTUPDATE_AUTOVAR;
	
	pD += CWObject_Model_Anim_Parent::OnCreateClientUpdate(_iClient, _pClObjInfo, _pObj, _pData, Flags);
	if ((_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT) & 1)
		pD += pCD->m_AnimQueue.OnCreateClientUpdate(pD);

	return (uint8*)pD - _pData;
}

int CWObject_Model_Anim::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	MAUTOSTRIP(CWObject_Attach_OnClientUpdate, 0);
	MSCOPESHORT(CWObject_Model_Anim::OnClientUpdate);

	const uint8* pD = &_pData[CWObject_Model_Anim_Parent::OnClientUpdate(_pObj, _pWClient, _pData, _Flags)];
	if (_pObj->m_iClass == 0 || pD - _pData == 0) return pD - _pData;

	CAnimModelClientData *pCD = GetAnimModelClientData(_pObj);
	if (_pObj->m_bAutoVarDirty)
		pD += pCD->m_AnimQueue.OnClientUpdate(pD);

	return (uint8*)pD - _pData;
}

void CWObject_Model_Anim::OnDeltaSave(CCFile* _pFile)
{
	CWObject_Model_Anim_Parent::OnDeltaSave(_pFile);
	const CAnimModelClientData* pCD = GetAnimModelClientData(this);
	int8 Flags = 0;
	if (!m_AnimBaseMat.AlmostUnit(0.0001f))
		Flags |= M_Bit(0);
	if (!pCD->m_StartPos.AlmostEqual(CVec3Dfp32(0.0f,0.0f,0.0f),0.0001f))
		Flags |= M_Bit(1);
	// ... rot
	Flags |= M_Bit(2);

	if (pCD->m_AnimQueue.m_lAnimations.Len())
		Flags |= M_Bit(3);

	if (GetParent() != 0)
		Flags |= M_Bit(4);
	
	// Write the stuff
	_pFile->WriteLE(Flags);
	if (Flags & M_Bit(0))
		m_AnimBaseMat.Write(_pFile);
	if (Flags & M_Bit(1))
		pCD->m_StartPos.Write(_pFile);
	if (Flags & M_Bit(2))
		pCD->m_StartRot.Write(_pFile);
	if (Flags & M_Bit(3))
	{
		TAP_RCD<const CAnimInstance> lAnims = pCD->m_AnimQueue.m_lAnimations;
		uint8 NumAnims = lAnims.Len();
		_pFile->WriteLE(NumAnims);
		for (int32 i = 0; i < lAnims.Len(); i++)
		{
			_pFile->WriteLE(lAnims[i].m_iAnimContainerResource);
			_pFile->WriteLE(lAnims[i].m_iAnimSeq);
			_pFile->WriteLE(lAnims[i].m_StartTick);
			_pFile->WriteLE(lAnims[i].m_TimeScale);
			_pFile->WriteLE(lAnims[i].m_BlendDuration);
			_pFile->WriteLE(lAnims[i].m_SkeletonScale);
			_pFile->WriteLE(lAnims[i].m_bUseAnimPhys);
		}
	}
	if (Flags & M_Bit(4))
	{
		int16 iParent = GetParent();
		_pFile->WriteLE(iParent);
		CMat4Dfp32 CurPos = GetLocalPositionMatrix();
		CurPos.Write(_pFile);
	}
}

void CWObject_Model_Anim::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	CWObject_Model_Anim_Parent::OnDeltaLoad(_pFile, _Flags);
	CAnimModelClientData* pCD = GetAnimModelClientData(this);
	int8 Flags;
	// Write the stuff
	_pFile->ReadLE(Flags);
	if (Flags & M_Bit(0))
		m_AnimBaseMat.Read(_pFile);
	if (Flags & M_Bit(1))
		pCD->m_StartPos.Read(_pFile);
	if (Flags & M_Bit(2))
		pCD->m_StartRot.Read(_pFile);
	if (Flags & M_Bit(3))
	{
		int8 NumAnims;
		_pFile->ReadLE(NumAnims);
		pCD->m_AnimQueue.m_lAnimations.SetLen(NumAnims);
		TAP_RCD<CAnimInstance> lAnims = pCD->m_AnimQueue.m_lAnimations;	
		for (int32 i = 0; i < lAnims.Len(); i++)
		{
			_pFile->ReadLE(lAnims[i].m_iAnimContainerResource);
			_pFile->ReadLE(lAnims[i].m_iAnimSeq);
			_pFile->ReadLE(lAnims[i].m_StartTick);
			_pFile->ReadLE(lAnims[i].m_TimeScale);
			_pFile->ReadLE(lAnims[i].m_BlendDuration);
			_pFile->ReadLE(lAnims[i].m_SkeletonScale);
			_pFile->ReadLE(lAnims[i].m_bUseAnimPhys);
		}
	}
	if (Flags & M_Bit(4))
	{
		int16 iObjParent;
		_pFile->ReadLE(iObjParent);
		CMat4Dfp32 CurPos;
		CurPos.Read(_pFile);
		if (GetParent() == 0)
			m_pWServer->Object_AddChild(iObjParent, m_iObject);
		InternalState_GetLocalPositionMatrix() = CurPos;
	}
}

void CWObject_Model_Anim::OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject_Model_Anim_Parent::OnClientLoad(_pObj, _pWorld, _pFile, _pWData, _Flags);
	CAnimModelClientData* pCD = GetAnimModelClientData(_pObj);
	int8 Flags;
	// Write the stuff
	_pFile->ReadLE(Flags);
	if (Flags & M_Bit(0))
		pCD->m_StartPos.Read(_pFile);
	if (Flags & M_Bit(1))
		pCD->m_StartRot.Read(_pFile);
	if (Flags & M_Bit(2))
	{
		int8 NumAnims;
		_pFile->ReadLE(NumAnims);
		pCD->m_AnimQueue.m_lAnimations.SetLen(NumAnims);
		TAP_RCD<CAnimInstance> lAnims = pCD->m_AnimQueue.m_lAnimations;	
		for (int32 i = 0; i < lAnims.Len(); i++)
		{
			_pFile->ReadLE(lAnims[i].m_iAnimContainerResource);
			_pFile->ReadLE(lAnims[i].m_iAnimSeq);
			_pFile->ReadLE(lAnims[i].m_StartTick);
			_pFile->ReadLE(lAnims[i].m_TimeScale);
			_pFile->ReadLE(lAnims[i].m_BlendDuration);
			_pFile->ReadLE(lAnims[i].m_SkeletonScale);
			_pFile->ReadLE(lAnims[i].m_bUseAnimPhys);
		}
	}
}

void CWObject_Model_Anim::OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject_Model_Anim_Parent::OnClientSave(_pObj, _pWorld, _pFile, _pWData, _Flags);
	const CAnimModelClientData* pCD = GetAnimModelClientData(_pObj);
	int8 Flags = 0;
	if (!pCD->m_StartPos.AlmostEqual(CVec3Dfp32(0.0f,0.0f,0.0f),0.0001f))
		Flags |= M_Bit(0);
	// ... rot
	Flags |= M_Bit(1);

	if (pCD->m_AnimQueue.m_lAnimations.Len())
		Flags |= M_Bit(2);

	// Write the stuff
	_pFile->WriteLE(Flags);
	if (Flags & M_Bit(0))
		pCD->m_StartPos.Write(_pFile);
	if (Flags & M_Bit(1))
		pCD->m_StartRot.Write(_pFile);
	if (Flags & M_Bit(2))
	{
		TAP_RCD<const CAnimInstance> lAnims = pCD->m_AnimQueue.m_lAnimations;
		uint8 NumAnims = lAnims.Len();
		_pFile->WriteLE(NumAnims);
		for (int32 i = 0; i < lAnims.Len(); i++)
		{
			_pFile->WriteLE(lAnims[i].m_iAnimContainerResource);
			_pFile->WriteLE(lAnims[i].m_iAnimSeq);
			_pFile->WriteLE(lAnims[i].m_StartTick);
			_pFile->WriteLE(lAnims[i].m_TimeScale);
			_pFile->WriteLE(lAnims[i].m_BlendDuration);
			_pFile->WriteLE(lAnims[i].m_SkeletonScale);
			_pFile->WriteLE(lAnims[i].m_bUseAnimPhys);
		}
	}
}

int CWObject_Model_Anim::CAnimInstanceQueue::OnCreateClientUpdate(const uint8* _pData) const
{
	// This is only done when something is changed anyway so just save the info
	int32 Len = m_lAnimations.Len();
	const uint8* pD = _pData;

	PTR_PUTUINT8(pD,Len);

	for (int32 i = 0; i < Len; i++)
	{
		PTR_PUTINT16(pD, m_lAnimations[i].m_iAnimContainerResource);
		PTR_PUTINT16(pD, m_lAnimations[i].m_iAnimSeq);
		PTR_PUTINT32(pD, m_lAnimations[i].m_StartTick);
		PTR_PUTFP32(pD,m_lAnimations[i].m_BlendDuration);
		PTR_PUTFP32(pD,m_lAnimations[i].m_SkeletonScale);
	}

	return (pD - _pData);
}

int CWObject_Model_Anim::CAnimInstanceQueue::OnClientUpdate(const uint8* _pData)
{
	// This is only done when something is changed anyway so just save the info
	int32 Len;
	const uint8* pD = _pData;

	PTR_GETUINT8(pD,Len);
	m_lAnimations.SetLen(Len);

	for (int32 i = 0; i < Len; i++)
	{
		// force recache of animseq (might still be there from previous one)
		m_lAnimations[i].m_pSeq = NULL;
		PTR_GETINT16(pD, m_lAnimations[i].m_iAnimContainerResource);
		PTR_GETINT16(pD, m_lAnimations[i].m_iAnimSeq);
		PTR_GETINT32(pD, m_lAnimations[i].m_StartTick);
		PTR_GETFP32(pD,m_lAnimations[i].m_BlendDuration);
		PTR_GETFP32(pD,m_lAnimations[i].m_SkeletonScale);
	}

	return (pD - _pData);
}

void CWObject_Model_Anim::CAnimInstanceQueue::QueueAnimation(const CAnimInstance& _Instance)
{
	m_lAnimations.Add(_Instance);
}

bool CWObject_Model_Anim::CAnimInstanceQueue::Refresh(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj)
{
	if (_pObj->m_Data[7] != 0)
		return true;

	// If animation after this one has blended in fully, remove first anim
	const int32 GameTick = _pWPhys->GetGameTick();
	for (int32 i = 1; i < m_lAnimations.Len(); i++)
	{
		// If current has blended in fully remove previous anim
		if ((GameTick - m_lAnimations[i].m_StartTick) * _pWPhys->GetGameTickTime() > m_lAnimations[i].m_BlendDuration)
		{
			//ConOut(CStrF("Removing anim: %d on %s",i-1,_pWPhys->IsServer() ? "Server" : "Client"));
			// Delete previous animation
			m_lAnimations.Del(i-1);
			// Decrease i since previous was removed
			i--;
		}
	}

	return (m_lAnimations.Len() > 1);
}

int32 CWObject_Model_Anim::CAnimInstanceQueue::GetAnimLayers(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CXR_AnimLayer* _pLayers, fp32* _pRetScale, fp32 _Frac)
{
	*_pRetScale = 0.0f;

	// If we're syncing to anims, take from them instead
	if (_pObj->m_Data[7] != 0)
	{
		// Try and get animation from a "sync" character
		int iSyncObj = _pObj->m_Data[7];

		CWObject_CoreData* pSyncObj = _pWPhys->Object_GetCD(iSyncObj);
		if (pSyncObj && (pSyncObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER))
		{
			CWO_Character_ClientData* pSyncCD = CWObject_Character::GetClientData(pSyncObj);
			*_pRetScale = pSyncCD->m_CharSkeletonScale;
		}

		CWObject_Message Msg(OBJMSG_CHAR_GETANIMLAYERFROMONLYANIMTYPE, _pObj->m_Data[6]);
		Msg.m_pData = (void*)_pLayers;
		Msg.m_DataSize = AG2I_MAXANIMLAYERS;
		int32 NumLayers = _pWPhys->Phys_Message_SendToObject(Msg, iSyncObj);
		if (NumLayers == 1)
		{
			// Max blend on layer
			_pLayers[0].m_Blend = 1.0f;
		}
		return NumLayers;
	}

    int32 NumLayers = 0;
	int32 Len = m_lAnimations.Len();
	CMTime Time = _pWPhys->GetGameTime() + CMTime::CreateFromSeconds(_Frac * _pWPhys->GetGameTickTime());
	for (int32 i = 0; i < Len; i++)
	{
		CMTime StartTime = PHYSSTATE_TICKS_TO_TIME(m_lAnimations[i].m_StartTick, _pWPhys);
		fp32 Blend = 1.0f;
		if (i > 0 && (m_lAnimations[i].m_BlendDuration > 0.0f))
		{
			Blend = Clamp01((Time - StartTime).GetTime() / m_lAnimations[i].m_BlendDuration);
		}
		CXR_Anim_SequenceData* pSeq = m_lAnimations[i].GetAnimSequenceData(_pWPhys->GetMapData());
		if (!pSeq)
			continue;
		CMTime AnimTime = Time - StartTime;
		_pLayers[NumLayers].Create3(pSeq, pSeq->GetLoopedTime(AnimTime), m_lAnimations[i].m_TimeScale, Blend, 0);
		*_pRetScale = m_lAnimations[i].m_SkeletonScale;
		//ConOut(CStrF("Looped: %f Time: %f Start: %f", spSeq->GetLoopedTime(AnimTime).GetTime(),Time.GetTime(),m_lAnimations[i].m_StartTime.GetTime()));
		NumLayers++;
	}

	return NumLayers;
}

bool CWObject_Model_Anim::CAnimInstanceQueue::GetAnimPhysLayer(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CXR_AnimLayer& _Layer, fp32 _Frac)
{
	if (_pObj->m_Data[7] != 0)
	{
		// Try and get animation from a "sync" character
		CWObject_Message Msg(OBJMSG_CHAR_GETANIMLAYERFROMONLYANIMTYPE,_pObj->m_Data[6]);
		Msg.m_pData = (void*)&_Layer;
		Msg.m_DataSize = 1;
		return _pWPhys->Phys_Message_SendToObject(Msg,_pObj->m_Data[7]) != 0;
	}
//	int32 NumLayers = 0;
	int32 Len = m_lAnimations.Len();
	CMTime Time = _pWPhys->GetGameTime() + CMTime::CreateFromSeconds(_Frac * _pWPhys->GetGameTickTime());
	for (int32 i = Len - 1; i >= 0; i--)
	{
		if (m_lAnimations[i].m_bUseAnimPhys)
		{
			CMTime StartTime = PHYSSTATE_TICKS_TO_TIME(m_lAnimations[i].m_StartTick, _pWPhys);
			fp32 Blend = 1.0f;
			CXR_Anim_SequenceData* pSeq = m_lAnimations[i].GetAnimSequenceData(_pWPhys->GetMapData());
			if (!pSeq)
				continue;
			CMTime AnimTime = Time - StartTime;
			_Layer.Create3(pSeq, pSeq->GetLoopedTime(AnimTime), m_lAnimations[i].m_TimeScale, Blend, 0);
			return true;
		}
	}

	return false;
}

bool CWObject_Model_Anim::CAnimInstanceQueue::UsingAnimPhys(CWObject_CoreData* _pObj)
{
	if (_pObj->m_Data[7] != 0 && _pObj->GetParent() == 0)
		return true;

	int32 Len = m_lAnimations.Len();

	for (int32 i = 0; i < Len; i++)
	{
		if (m_lAnimations[i].m_bUseAnimPhys)
			return true;
	}

	return false;
}

void CWObject_Model_Anim::MoveObject(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj)
{
	CAnimModelClientData* pCD = GetAnimModelClientData(_pObj);
	CXR_AnimLayer Layer;
	CMat4Dfp32 ObjMatrix = _pObj->GetPositionMatrix();
	if (!pCD || _pObj->GetParent() != 0 || !pCD->m_AnimQueue.GetAnimPhysLayer(_pWPhys,_pObj,Layer))
		return;

	VecUnion dMove;
	vec128 Move, Move1;
	CQuatfp32 Rot, Rot1, dRot;
	CMTime Time = CMTime::CreateFromSeconds(Layer.m_Time);
	Layer.m_spSequence->EvalTrack0(Time, Move, Rot);

	CMTime Time1 = Time + CMTime::CreateFromTicks(1, _pWPhys->GetGameTickTime()); 
	Layer.m_spSequence->EvalTrack0(Time1, Move1, Rot1);

	dMove.v128 = M_VSub(Move1, Move);
	dMove.v128 = M_VMulMat3x3(dMove.v128, m_AnimBaseMat);
	Rot.Inverse();
	dRot = Rot * Rot1;

	// Set velocity
	CVelocityfp32 Vel;
	Vel.m_Move = dMove.v3;
	Vel.m_Rot.Create(dRot);
	_pWPhys->Object_SetVelocity(_pObj->m_iObject, Vel);
	_pWPhys->Object_MovePhysical(m_iObject);

	// Calculate absolute positions for next tick
	/*
	// Ok, got animlayer with stuff
	CVec3Dfp32 Pos = 0.0f;
	CQuatfp32 Rot;
	Rot.Unit();
	CMTime TimeA = CMTime::CreateFromSeconds(Layer.m_Time);

	Layer.m_spSequence->EvalTrack0(TimeA, Pos, Rot);

	// Pos = startpos + animpos
	// Rot = startrot + animrot

	// Rotate pos with startrot
	CMat4Dfp32 RotMat;
	pCD->m_StartRot.CreateMatrix(RotMat);
	Pos = Pos * RotMat;

	Pos += pCD->m_StartPos;
	Rot *= pCD->m_StartRot;

	//ConOut(CStrF("StartPos: %s  CurrentPos: %s",pCD->m_StartPos.GetString().GetStr(),Pos.GetString().GetStr()));

	CMat4Dfp32 Mat;
	Rot.CreateMatrix(Mat);
	Pos.SetMatrixRow(Mat,3);

	_pWPhys->Object_SetPosition(_pObj->m_iObject,Mat);

	CMTime TimeB = CMTime::CreateFromSeconds(Layer.m_Time + _pWPhys->GetGameTickTime());
	TimeB = Layer.m_spSequence->GetLoopedTime(TimeB);

	TimeB += CMTime::CreateFromSeconds(0.00001f);
	bool bLooping = TimeB.Compare(TimeA) < 0;
	if (bLooping)
	{
		CVec3Dfp32 LoopMove;
		CQuatfp32 LoopRot;
		Layer.m_spSequence->GetTotalTrack0(LoopMove, LoopRot);
		// Make a new startpos and startrot
		pCD->m_StartRot.CreateMatrix(RotMat);
		LoopMove = LoopMove * RotMat;
		pCD->m_StartPos += LoopMove;
		pCD->m_StartRot *= LoopRot;
	}*/
}

void CWObject_Model_Anim::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_Model_Anim::OnClientRefresh);
	CWObject_Model_Anim_Parent::OnClientRefresh(_pObj,_pWClient);

	// Refresh stuff...
	CAnimModelClientData* pCD = GetAnimModelClientData(_pObj);
	if (!pCD)
		return;

	pCD->m_AnimQueue.Refresh(_pWClient,_pObj);
}

void CWObject_Model_Anim::OnRefresh()
{
	CWObject_Model_Anim_Parent::OnRefresh();

	// Refresh stuff...
	CAnimModelClientData* pCD = GetAnimModelClientData(this);
	if (!pCD)
		return;

	//ConOut("Refreshing");

	bool bNeedRefresh = pCD->m_AnimQueue.Refresh(m_pWServer,this);

	// Update position from attach object (try to get rendermatrix first...)
	if (m_iAttachObject > 0)
	{
		bNeedRefresh = true;
		CMat4Dfp32 Mat;
		Mat.Unit();
		CWObject_Message RenderMat(OBJMSG_HOOK_GETCURRENTMATRIX,aint(&Mat));
		if (m_pWServer->Phys_Message_SendToObject(RenderMat, m_iAttachObject))
		{
			CMat4Dfp32 Result;
			// Add position offset and rotation offset
			if (!(m_AnimModelFlags & MODEL_ANIM_FLAG_SKIPROTOFFSET))
			{
				CMat4Dfp32 RotMat;
				m_RotOffset.CreateMatrix(RotMat);
				RotMat.Multiply(Mat,Result);
			}
			else
			{
				Result = Mat;
			}

			if (!(m_AnimModelFlags & MODEL_ANIM_FLAG_SKIPPOSOFFSET))
				CVec3Dfp32::GetMatrixRow(Result,3) += m_PosOffset;
			
			SetPosition(Result);
		}
		else
		{
			CWObject_CoreData* pObj = m_pWServer->Object_GetCD(m_iAttachObject);
			if (pObj)
			{
				Mat = pObj->GetPositionMatrix();
				CMat4Dfp32 Result;
				// Add position offset and rotation offset
				if (!(m_AnimModelFlags & MODEL_ANIM_FLAG_SKIPROTOFFSET))
				{
					CMat4Dfp32 RotMat;
					m_RotOffset.CreateMatrix(RotMat);
					RotMat.Multiply(Mat,Result);
				}
				else
				{
					Result = Mat;
				}

				if (!(m_AnimModelFlags & MODEL_ANIM_FLAG_SKIPPOSOFFSET))
					CVec3Dfp32::GetMatrixRow(Result,3) += m_PosOffset;
				

				SetPosition(Result);
			}
		}
	}
	else if (pCD->m_AnimQueue.UsingAnimPhys(this))
	{
		bNeedRefresh = true;
		// Move object
		MoveObject(m_pWServer,this);//,MoveVelocity,RotVelocity);
		/*m_pWServer->Object_SetVelocity(m_iObject, MoveVelocity);
		m_pWServer->Object_SetRotVelocity(m_iObject, RotVelocity);

		// Hmm, well that didn't work too well, guess perfect positioning has to be employed...
		SetPosition(GetPosition() + MoveVelocity);*/
	}

	if (!bNeedRefresh)
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
}

// Bah use char_playanim for now
aint CWObject_Model_Anim::OnMessage(const CWObject_Message &_Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJSYSMSG_PRECACHEMESSAGE:
		{
			CWObject_Message *pMsg = (CWObject_Message *)_Msg.m_pData;
			if(pMsg->m_Msg == OBJMSG_ANIMMODEL_PLAYANIM)
			{
				CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
				if (!pServerMod)
					return 0;

				CFStr St = (char *)pMsg->m_pData;
				St = St.GetStrSep(",");
				CFStr Container = St.GetStrSep(":");
//				int iAnim = m_pWServer->GetMapData()->GetResourceIndex_Anim(Container);
				int iSeq = St.Val_int();


				pServerMod->AddAnimContainerEntry(Container, iSeq);

				/*CWObject_Message Msg(OBJMSG_GAME_RESOLVEANIMHANDLE);
				Msg.m_pData = pMsg->m_pData;
				m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());*/
				return 1;
			}
			return 0;
		}
	case OBJMSG_ANIMMODEL_PLAYANIM:
		{
			CWorldData* pWData = m_pWServer->m_spWData;
			CAnimModelClientData* pCD = GetAnimModelClientData(this);
			if (!pCD)
				return 0;

			CFStr OgierStr = (char *)_Msg.m_pData;
			CFStr St = OgierStr.GetStrSep(",");
			CFStr Container = St.GetStrSep(":").UpperCase();

			int iAnim = pWData->ResourceExistsPartial("ANM:" + Container);
			int iSeq = St.GetStrSep(":").Val_int();
			fp32 SkeletonScale = OgierStr.Val_fp64();
			if (SkeletonScale != 0.0f)
				SkeletonScale = 1.0f - SkeletonScale;

			CWResource* pRes = pWData->GetResource(iAnim);
			if (!pRes)
				return 0;
			int32 iAnimMapData = m_pWServer->GetMapData()->GetResourceIndex(pRes->m_Name);
			CXR_Anim_Base* pAnim = (pRes && (pRes->GetClass() == WRESOURCE_CLASS_XSA)) ? safe_cast<CWRes_Anim>(pRes)->GetAnim() : NULL;

			if(pAnim)
			{
				CXR_Anim_SequenceData *pSeq = pAnim->GetSequence(iSeq);
				if(pSeq)
				{
					//ConOut(CStrF("Found anim: %d iSeq: %d", iAnim,iSeq));
					CAnimInstance Instance;
					Instance.m_iAnimContainerResource = iAnimMapData;
					Instance.m_iAnimSeq = iSeq;
					CXR_Anim_SequenceData* pAnim = Instance.GetAnimSequenceData(m_pWServer->GetMapData());
					Instance.m_StartTick = m_pWServer->GetGameTick();
					Instance.m_bUseAnimPhys = _Msg.m_Param0 & 1;
					Instance.m_SkeletonScale = SkeletonScale;
					if (pAnim)
					{
						//Instance.m_Duration = pAnim->GetDuration();
						Instance.m_TimeScale = 1.0f;
						Instance.m_BlendDuration = (_Msg.m_Param0 & 2 ? 0.2f : 0.0f);
						pCD->m_AnimQueue.QueueAnimation(Instance);
						// Make dirty
						m_pWServer->Object_SetDirty(m_iObject,1 << CWO_DIRTYMASK_USERSHIFT);
					}
					if (Instance.m_bUseAnimPhys)
					{
						// Teleport to start position
						m_AnimBaseMat = GetPositionMatrix();
						vec128 Move;
						CQuatfp32 Rot;
						pSeq->EvalTrack0(CMTime(), Move, Rot);
						uint zero=M_VCmpAllEq(M_VSetW0(Move),M_VZero());
						if(!zero || Rot.k[0] != 0 || Rot.k[1] != 0 || Rot.k[2] != 0 || Rot.k[3] != 1)
						{
							CMat4Dfp32 Mat, Res;
							Mat.Create(Rot,Move);
							Mat.Multiply(GetPositionMatrix(), Res);
							CWObject_Message Msg(OBJSYSMSG_TELEPORT, 3);
							Msg.m_pData = &Res;
							OnMessage(Msg);
						}
						// Set startposition and rotation
						pCD->m_StartPos = CVec3Dfp32::GetMatrixRow(GetPositionMatrix(),3);
						pCD->m_StartRot.Create(GetPositionMatrix());
					}
					// Set refresh to class
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
				}
			}
			else
			{
				ConOut(CStrF("CWObject_Model_Anim::PlayAnim: Unable to find animation: %s:%d",Container.GetStr(),iSeq));
			}

			return 1;
		}
	case OBJMSG_CHAR_LOCKTOPARENT:
		{
			// Lock ourselves to parent
			if (_Msg.m_pData && _Msg.m_Param0 == 0)
			{
				int iChar = m_pWServer->Selection_GetSingleTarget((char*)_Msg.m_pData);
				// Set exact position to that of our "parent" before making it our real parent
				m_pWServer->Object_SetPosition(m_iObject,m_pWServer->Object_GetPositionMatrix(iChar));
				// Make given character our parent
				m_pWServer->Object_AddChild(iChar, m_iObject);
			}
			else
			{
				// Remove parenting
				m_pWServer->Object_RemoveChild(m_Data[7],m_iObject);
			}
			return 1;
		}
	case OBJMSG_CHAR_SYNCANIMATIONTOCHARACTER:
		{
			// ...
			if (_Msg.m_pData)
			{
				int iChar = m_pWServer->Selection_GetSingleTarget((char*)_Msg.m_pData);
				if (iChar > 0)
				{
					Data(7) = iChar;
					Data(6) = _Msg.m_Param0;
					// Need refresh
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
				}
			}
			else
			{
				if (m_Data[7] != 0)
					Data(7) = 0;
				if (m_Data[6] != 0)
					Data(6) = 0;
			}

			return 1;
		}
	case OBJMSG_CHAR_RENDERATTACHED:
		{
			// Do some render attached magic stuffo... (must sync to player times etc, parenting won't work...)
			if(!_Msg.m_pData)
			{
				if (m_Data[5])
					Data(5) = 0;
			}
			else
			{
				CFStr Str = (const char *)_Msg.m_pData;
				CNameHash TargetNameHash( Str.GetStrSep("/") );
				Data(5) = m_pWServer->Selection_GetSingleTarget(TargetNameHash);
			}
			return 1;
		}
	case OBJSYSMSG_TELEPORT:
		{
			//Get object position to teleport to
			CMat4Dfp32 Mat;
			if(_Msg.m_Param0 == 0)
			{
				CWObject * pObj = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget((char *)_Msg.m_pData));
				if (!pObj)
					return 0;

				Mat = pObj->GetPositionMatrix();
				Mat.k[3][2] += 0.01f;
			}
			else if(_Msg.m_Param0 == 1)
			{
				Mat.Unit();
				CVec3Dfp32::GetRow(Mat, 3).ParseString((char *)_Msg.m_pData);
			}
			else if(_Msg.m_Param0 == 2)
			{
				Mat.Unit();
				CVec3Dfp32::GetRow(Mat, 3) = _Msg.m_VecParam0;
			}
			else if(_Msg.m_Param0 == 3)
			{
				Mat = *(CMat4Dfp32 *)_Msg.m_pData;
			}
			else if(_Msg.m_Param0 == 4)
			{
				CWObject *pObj = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget((char *)_Msg.m_pData));
				if (!pObj)
					return 0;

				Mat = GetPositionMatrix();
				CVec3Dfp32::GetRow(Mat, 3) = pObj->GetPosition();
				Mat.k[3][2] += 0.01f;
			}
			else if (_Msg.m_Param0 == 5)
			{
				// Teleport to target but ignore XY-rotation
				CWObject* pObj = m_pWServer->Object_Get( m_pWServer->Selection_GetSingleTarget((char *)_Msg.m_pData) );
				if (!pObj)
					return 0;

				CVec3Dfp32 Angles1, Angles2;
				Angles1.CreateAnglesFromMatrix(0, GetPositionMatrix());
				Angles2.CreateAnglesFromMatrix(0, pObj->GetPositionMatrix());
				Angles1.k[2] = Angles2.k[2];
				Angles1.CreateMatrixFromAngles(0, Mat);
				Mat.GetRow(3) = pObj->GetPosition();
				Mat.k[3][2] += 0.01f;
			}
			else if (_Msg.m_Param0 == 8)
			{ // Delta movement
				CVec3Dfp32 Offset;
				Offset.ParseString((const char*)_Msg.m_pData);

				Mat = GetPositionMatrix();
				Mat.GetRow(3) += Offset;

				return m_pWServer->Object_SetPosition_World(m_iObject, Mat);
			}

			CCollisionInfo Info;
			if(m_pWServer->Phys_IntersectWorld((CSelection *) NULL, GetPhysState(), Mat, Mat, m_iObject, &Info))
			{
				CWObject *pObj = m_pWServer->Object_Get(Info.m_iObject);
				if (pObj)
					ConOutL(CStrF("Failed to teleport Character (%i, %s, %s) to position %s, because of object (%i, %s, %s, Class: %s, Position: %s", 
					m_iObject, GetName(), GetTemplateName(), 
					Mat.GetRow(3).GetString().Str(),
					pObj->m_iObject, pObj->GetName(), pObj->GetTemplateName(),
					pObj->MRTC_GetRuntimeClass()->m_ClassName,
					pObj->GetPosition().GetString().Str()		));
				return 0;
			}

			//			m_pWServer->Debug_RenderWire(GetPosition(), CVec3Dfp32::GetRow(Mat, 3), 0xff7f7f7f, 20.0f);

			if(!m_pWServer->Object_SetPosition(m_iObject, Mat))
			{
				ConOutL(CStrF("Failed to teleport Character (%i, %s)", m_iObject, GetName()));
				return 0;
			}
			SetVelocity(0);
			return 1;
		}
	default:
		return CWObject_Model_Anim_Parent::OnMessage(_Msg);
	}
}

void CWObject_Model_Anim::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_AnimModel_OnClientRender, MAUTOSTRIP_VOID);

	CAnimModelClientData* pCD = GetAnimModelClientData(_pObj);
	if (!pCD)
		return;

	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	bool bOK = false;
	if (_pObj->m_Data[5])
	{
		// Render attached to some object.. (need to get perfect position, so using hook_getcurrentmat...)
		if (_pWClient->Phys_Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETCURRENTMATRIX, (aint)&MatIP),_pObj->m_Data[5]))
		{
			// Remove look component so we're not flying around so much...?
			bOK = true;
			CVec3Dfp32(0.0f,0.0f,1.0f).SetRow(MatIP,2);
			MatIP.RecreateMatrix(2,1);
		}
		else
		{
			CWObject_CoreData* pAttachObj = _pWClient->Object_GetCD(_pObj->m_Data[5]);
			if (pAttachObj)
			{
				Interpolate2(pAttachObj->GetLastPositionMatrix(), pAttachObj->GetPositionMatrix(), MatIP, IPTime);
				bOK = true;
			}
		}
	}
	if (!bOK)
	{
		Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);
	}

	// Assume all models use the same skeleton (ex body and head...)
	CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if(!pModel)
		return;

	CXR_AnimState Anim = _pObj->GetDefaultAnimState(_pWClient);
/*
	if (!_pObj->m_lspClientObj[1])
	{
		MRTC_SAFECREATEOBJECT_NOEX(spSkel, "CXR_SkeletonInstance", CXR_SkeletonInstance);
		_pObj->m_lspClientObj[1] = spSkel;
		if (!_pObj->m_lspClientObj[1]) return;
	}
	CXR_SkeletonInstance* pSkelInstance = (CXR_SkeletonInstance*)(CReferenceCount*)_pObj->m_lspClientObj[1];
	CXR_Skeleton* pSkel = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
	if(pSkel && pSkelInstance)
		pSkelInstance->Create(pSkel->m_lNodes.Len());
*/
	CXR_Skeleton* pSkel = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
	if (pSkel)
	{
		// Eval animation
		CXR_AnimLayer Layers[AG2I_MAXANIMLAYERS];
		fp32 SkeletonScaleDiff = 0.0f;
		int32 NumLayers = pCD->m_AnimQueue.GetAnimLayers(_pWClient, _pObj, Layers, &SkeletonScaleDiff, _pWClient->GetRenderTickFrac());
		if (NumLayers > 0)
		{
			CMat4Dfp32 Src = MatIP;
			Anim.m_pSkeletonInst = CXR_VBMHelper::Alloc_SkeletonInst(_pEngine->GetVBM(), pSkel->m_lNodes.Len(), pSkel->m_nUsedRotations, pSkel->m_nUsedMovements);
			if (!Anim.m_pSkeletonInst)
				return;

			// Set model scale
			if (SkeletonScaleDiff != 0.0f)
			{
				// this is pretty bad. assumes character skeleton (body bags)
				const static int BoneSet[] =
				{
					PLAYER_ROTTRACK_ROOT, PLAYER_ROTTRACK_RLEG, PLAYER_ROTTRACK_RKNEE, 
					PLAYER_ROTTRACK_RFOOT, PLAYER_ROTTRACK_LLEG, PLAYER_ROTTRACK_LKNEE, 
					PLAYER_ROTTRACK_LFOOT, PLAYER_ROTTRACK_SPINE, PLAYER_ROTTRACK_SPINE2,
					PLAYER_ROTTRACK_TORSO, PLAYER_ROTTRACK_NECK, PLAYER_ROTTRACK_HEAD, 
					PLAYER_ROTTRACK_RSHOULDER, PLAYER_ROTTRACK_RARM, PLAYER_ROTTRACK_RELBOW,  
					PLAYER_ROTTRACK_RHAND, PLAYER_ROTTRACK_LSHOULDER, PLAYER_ROTTRACK_LARM,
					PLAYER_ROTTRACK_LELBOW, PLAYER_ROTTRACK_LHAND,
				};
				const int BoneSetLen = sizeof(BoneSet) / sizeof(int);
				Anim.m_pSkeletonInst->ApplyScale(SkeletonScaleDiff, BoneSet, BoneSetLen);
			}

			pSkel->EvalAnim(Layers, NumLayers, Anim.m_pSkeletonInst, Src);
		}
	}

	_pEngine->Render_AddModel(pModel, MatIP, Anim);

	// Add any extra models (assume they use the same skeleton and anim)
	for (int32 i = 1; i < CWO_NUMMODELINDICES; i++)
	{
		pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
		if (pModel)
			_pEngine->Render_AddModel(pModel, MatIP, Anim);
	}
}
