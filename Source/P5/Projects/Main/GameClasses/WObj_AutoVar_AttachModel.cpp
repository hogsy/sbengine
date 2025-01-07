#include "PCH.h"
#include "WObj_AutoVar_AttachModel.h"

//#ifdef PLATFORM_DOLPHIN

#include "../../Shared/MOS/Classes/GameWorld/WPhysState.h"
#include "../../Shared/MOS/Classes/GameWorld/Client/WClient.h"
#include "WObj_Game/WObj_GameMessages.h"
//#endif

CAutoVar_AttachModel::CAutoVar_AttachModel()
{
	MAUTOSTRIP(CAutoVar_AttachModel_ctor, MAUTOSTRIP_VOID);
	Clear();
	m_iWeaponToken = 0;
}

void CAutoVar_AttachModel::CopyFrom(const CAutoVar_AttachModel& _From)
{
	MAUTOSTRIP(CAutoVar_AttachModel_SCopyFrom, MAUTOSTRIP_VOID);

	for(int i = 0; i < ATTACHMODEL_NUMMODELS; i++)
	{
		m_iModel[i] = _From.m_iModel[i];
		m_iAttachPoint[i] = _From.m_iAttachPoint[i];
		m_ModelFlags[i] = _From.m_ModelFlags[i];

		if (_From.m_lspModelInstances[i])
		{
//			if (!This.m_lspModelInstances[i] || typeid(*This.m_lspModelInstances[i]) != typeid(*From.m_lspModelInstances[i]))
			if (!m_lspModelInstances[i] || strcmp(m_lspModelInstances[i]->MRTC_ClassName(), m_lspModelInstances[i]->MRTC_ClassName()))
				m_lspModelInstances[i] = _From.m_lspModelInstances[i]->Duplicate();
			else
				*m_lspModelInstances[i] = *_From.m_lspModelInstances[i];

/*			if(This.m_lspModelInstances[i])
			{
				CXR_ModelInstance *p1 = This.m_lspModelInstances[i];
				const CXR_ModelInstance *p2 = From.m_lspModelInstances[i];
				int bTest1 = typeid(*p1) != typeid(*p2);
				int bTest2 = p1->MRTC_GetRuntimeClass() != p2->MRTC_GetRuntimeClass();
				if((bTest1 ^ bTest2) != 0)
					int i = 0;
			}*/

/*			if (!This.m_lspModelInstances[i] || This.m_lspModelInstances[i]->MRTC_GetStaticRuntimeClass() != From.m_lspModelInstances[i]->MRTC_GetStaticRuntimeClass())
				This.m_lspModelInstances[i] = From.m_lspModelInstances[i]->Duplicate();
			else
				*This.m_lspModelInstances[i] = *From.m_lspModelInstances[i];*/
		}
		else
			m_lspModelInstances[i] = NULL;

	}

	m_ExtraModels.CopyFrom(_From.m_ExtraModels);

	m_Flags = _From.m_Flags;
	m_iAttachRotTrack = _From.m_iAttachRotTrack;

	m_AC_Cur = _From.m_AC_Cur;
	m_AC_Target = _From.m_AC_Target;
	m_AC_Timestamp = _From.m_AC_Timestamp;
	m_AC_Speed = _From.m_AC_Speed;
	m_AC_RandSeed = _From.m_AC_RandSeed;
	m_SurfaceOcclusionMask = _From.m_SurfaceOcclusionMask;
	m_SurfaceShadowOcclusionMask = _From.m_SurfaceShadowOcclusionMask;

//	m_AC_LastKeyframe = m_AC_LastKeyframe;
//	m_AC_iSequence = m_AC_iSequence;

	m_iWeaponToken = _From.m_iWeaponToken;
	m_pAG2I = _From.m_pAG2I;
}

void CAutoVar_AttachModel::Clear()
{
	MAUTOSTRIP(CAutoVar_AttachModel_Clear, MAUTOSTRIP_VOID);
	for(int i = 0; i < ATTACHMODEL_NUMMODELS; i++)
	{
		m_iModel[i] = 0;
		m_iAttachPoint[i] = 0;
		m_ModelFlags[i] = 0;
	}

	m_ExtraModels.Clear();

	m_Flags = 0;
	m_iAttachRotTrack = 0;

	m_AC_Cur.Reset();
	m_AC_Target.Reset();
	m_AC_Timestamp.MakeInvalid();
	m_AC_Speed = 1.0f;
	m_AC_RandSeed = 0;
	m_SurfaceOcclusionMask = 0;
	m_SurfaceShadowOcclusionMask = 0;

	m_pAG2I = NULL;
}

bool CAutoVar_AttachModel::IsValid()
{
	MAUTOSTRIP(CAutoVar_AttachModel_IsValid, false);
	return m_iModel[0] != 0;
}

void CAutoVar_AttachModel::UpdateModel(const CAutoVar_AttachModel &_Model)
{
	for(int i = 0; i < ATTACHMODEL_NUMMODELS; i++)
	{
		m_iModel[i] = _Model.m_iModel[i];
		m_iAttachPoint[i] = _Model.m_iAttachPoint[i];
		m_ModelFlags[i] = _Model.m_ModelFlags[i];
	}

	// Shouldn't be anything from rpgitem, but anyways...
	m_ExtraModels.CopyFrom(_Model.m_ExtraModels);

	m_Flags = _Model.m_Flags;
	m_iAttachRotTrack = _Model.m_iAttachRotTrack;
	// Reset occlusion masks?
	m_SurfaceOcclusionMask = 0;
	m_SurfaceShadowOcclusionMask = 0;
}

void CAutoVar_AttachModel::SetBaseModel(int _iModel, int _iAttachPoint, int _iAttachRotTrack)
{
	MAUTOSTRIP(CAutoVar_AttachModel_SetBaseModel, MAUTOSTRIP_VOID);
	m_iModel[0] = _iModel;
	m_iAttachPoint[0] = _iAttachPoint;
	m_iAttachRotTrack = _iAttachRotTrack;
}

void CAutoVar_AttachModel::SetModel(int _Index, int _iModel, int _iAttachPoint)
{
	MAUTOSTRIP(CAutoVar_AttachModel_SetModel, MAUTOSTRIP_VOID);
	m_iModel[_Index] = _iModel;
	m_iAttachPoint[_Index] = _iAttachPoint;
}

void CAutoVar_AttachModel::SetFlags(int _Index, int _Flags)
{
	MAUTOSTRIP(CAutoVar_AttachModel_SetFlags, MAUTOSTRIP_VOID);
	m_ModelFlags[_Index] = _Flags;
}

CMTime CAutoVar_AttachModel::AC_Get(const CMTime& _Time)
{
	if(_Time.Compare(m_AC_Timestamp) < 0)
		return m_AC_Cur;

	CMTime Cur = m_AC_Cur + (_Time - m_AC_Timestamp).Scale(m_AC_Speed);
	if(m_AC_Cur.Compare(m_AC_Target) <= 0)
		Cur = m_AC_Target.Min(Cur);
	else
		Cur = m_AC_Target.Max(Cur);

	return Cur;
}

void CAutoVar_AttachModel::AC_SetTarget(const CMTime& _Target, fp32 _Speed, const CMTime& _Timestamp)
{
	MAUTOSTRIP(CAutoVar_AttachModel_SetAnimTarget, MAUTOSTRIP_VOID);

	if (_Speed == 0)
	{
		m_AC_Cur = _Target;
		m_AC_Target = _Target;
		m_AC_Timestamp = _Timestamp;
		m_AC_Speed = 1.0f;
	}
	else
	{
		m_AC_Cur = AC_Get(_Timestamp);
		m_AC_Target = _Target;
		m_AC_Timestamp = _Timestamp;
		m_AC_Speed = _Speed;
	}
}

void CAutoVar_AttachModel::SetAG2I(CWAG2I* _pAG2I)
{
	m_pAG2I = _pAG2I;
}

void CAutoVar_AttachModel::CreateClientSkelInstance()
{
	// Create skeleton
	if (m_spSkeletonInstance == NULL)
	{
		MRTC_SAFECREATEOBJECT_NOEX(spSkel, "CXR_SkeletonInstance", CXR_SkeletonInstance);
		m_spSkeletonInstance = spSkel;
	}
}

bool CAutoVar_AttachModel::GetAnimLayers(CXR_Skeleton* _pSkel, CXR_AnimState* _pAnimState, CMat4Dfp32* _pMatrix, CWorld_PhysState* _pWPhys, const CMTime& _Time)
{
	MSCOPESHORT(CAutoVar_AttachModel::GetAnimLayers);
	// Hmm, ok then, find skeleton for the model, find animation instance and apply it to the 
	// skeleton model....?
	//CMat4Dfp32* pMatrix = (CMat4Dfp32*)(((void**)_Msg.m_pData)[1]);
	//fp32 AnimTime = m_SkelAnimTime;//_Msg.m_VecParam0[0];
	//int iModel = _Msg.m_Param0; // Find model somewhere else...
	if (!m_pAG2I)
		return false;

	if (!_pSkel || !_pAnimState || !_pMatrix || !_pWPhys)
	{
		ConOutL(CStr("CAutoVar_AttachModel::GetAnimLayers() - Invalid pointer (NULL)."));
		return false;
	}

	// BoneAnimated Specifics...
	if (_pAnimState->m_pSkeletonInst)
	{
		// Use active animations to influence skeleton
		CXR_AnimLayer lLayer[ATTACHMODEL_MAXANIMLAYERS];
		int nLayers = ATTACHMODEL_MAXANIMLAYERS;

		CWAG2I_Context AG2IContext(NULL, _pWPhys, _Time);
		m_pAG2I->GetAnimLayersFromToken(&AG2IContext, m_iWeaponToken, lLayer, nLayers, 0);
		
		if (nLayers > 0)
		{
			CMat4Dfp32 MatrixEval = *_pMatrix;
			_pSkel->EvalAnim(lLayer, nLayers, _pAnimState->m_pSkeletonInst, MatrixEval, 0);
		}
		else
		{
			_pAnimState->m_pSkeletonInst = NULL;
		}
	}

	return true;
}

void CAutoVar_AttachModel::Pack(uint8*& _pD, CMapData* _pMapData) const
{
	MAUTOSTRIP(CAutoVar_AttachModel_Pack, MAUTOSTRIP_VOID);
	uint8& Mask = _pD[0];
	_pD++;
	Mask = 0;
	// 0x01
	int CurBit = 1;
	for(int i = 0; i < ATTACHMODEL_NUMMODELS; i++)
	{
		if ((m_iModel[i] != 0) || (m_iAttachPoint[i] != 0))
		{
			PTR_PUTINT16(_pD, m_iModel[i]);
			PTR_PUTINT8(_pD, m_iAttachPoint[i]);
			Mask |= CurBit;
		}
		// 0x02
		CurBit <<= 1;
	}
	if(m_iAttachRotTrack != 0)
	{
		PTR_PUTINT8(_pD, m_iAttachRotTrack);
		Mask |= CurBit;
	}
	// 0x04
	CurBit <<= 1;

	if (!m_AC_Timestamp.IsInvalid())
	{
		PTR_PUTCMTIME(_pD, m_AC_Cur);
		PTR_PUTCMTIME(_pD, m_AC_Target);
		PTR_PUTCMTIME(_pD, m_AC_Timestamp);
		PTR_PUTFP32(_pD, m_AC_Speed);
		PTR_PUTINT16(_pD, m_AC_RandSeed);

		Mask |= CurBit;
	}
	// 0x08
	CurBit <<= 1;

	if (m_iWeaponToken != 0)
	{
		PTR_PUTUINT8(_pD, m_iWeaponToken);
		Mask |= CurBit;
	}
	// 0x10
	CurBit <<= 1;

	// Pack extra models
	m_ExtraModels.Pack(_pD, Mask, CurBit);

	M_ASSERT(CurBit <= M_Bit(8), "Too many bits!");
}


void CAutoVar_AttachModel::Unpack(const uint8 *&_pD, CMapData* _pMapData)
{
	MAUTOSTRIP(CAutoVar_AttachModel_Unpack, MAUTOSTRIP_VOID);
	uint8 Mask;
	PTR_GETINT8(_pD, Mask);
	
	int CurBit = 1;
	for(int i = 0; i < ATTACHMODEL_NUMMODELS; i++)
	{
		if(Mask & CurBit)
		{
			int iOldModel = m_iModel[i];
			PTR_GETINT16(_pD, m_iModel[i]);
			
			if(iOldModel != m_iModel[i])
			{
				CXR_Model *pModel = _pMapData->GetResource_Model(m_iModel[i]);
				if (pModel)
				{
					m_lspModelInstances[i] = pModel->CreateModelInstance();
					if (m_lspModelInstances[i] != NULL)
						m_lspModelInstances[i]->Create(pModel, CXR_ModelInstanceContext(NULL, NULL));
				}
				else
					m_lspModelInstances[i] = NULL;
			}

			PTR_GETINT8(_pD, m_iAttachPoint[i]);
		}
		else
		{
			m_iModel[i] = 0;
			m_iAttachPoint[i] = 0;
			m_lspModelInstances[i] = NULL;
		}
		CurBit <<= 1;
	}
	
	if(Mask & CurBit)
	{
		PTR_GETINT8(_pD, m_iAttachRotTrack);
	}
	else
		m_iAttachRotTrack = 0;
	CurBit <<= 1;

	if(Mask & CurBit)
	{
		PTR_GETCMTIME(_pD, m_AC_Cur);
		PTR_GETCMTIME(_pD, m_AC_Target);
		PTR_GETCMTIME(_pD, m_AC_Timestamp);
		PTR_GETFP32(_pD, m_AC_Speed);
		PTR_GETINT16(_pD, m_AC_RandSeed);
	}
	else
	{
		m_AC_Cur.Reset();
		m_AC_Target.Reset();
		m_AC_Timestamp.MakeInvalid();
		m_AC_Speed = 1.0f;
	}
	CurBit <<= 1;

	if (Mask & CurBit)
	{
		PTR_GETUINT8(_pD, m_iWeaponToken);
	}
	else
	{
		m_iWeaponToken = 0;
	}

	// 0x10
	CurBit <<= 1;
	// Unpack extramodels
	m_ExtraModels.UnPack(_pD, _pMapData, Mask, CurBit);

	M_ASSERT(CurBit <= M_Bit(8), "Too many bits!");
}


bool CAutoVar_AttachModel::GetModel0_RenderMatrix(CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, CXR_Model *_pModel, int _iRotTrack, int _iAttachPoint, CMat4Dfp32 &_Mat)
{
	MAUTOSTRIP(CAutoVar_AttachModel_GetModel0_RenderMatrix, false);
	if(!_pSkel || !_pSkelInstance || !((_iRotTrack >= 0) && (_iRotTrack < _pSkelInstance->m_nBoneTransform)))
		return false;

	// Get character attachment matrix
	CMat4Dfp32 Pos;
	if (_pSkel)
	{
		const CXR_SkeletonAttachPoint* pHand = _pSkel->GetAttachPoint(_iAttachPoint);
		if (pHand)
		{
			const CVec3Dfp32& LocalPos = _pSkelInstance->m_pBoneLocalPos[_iRotTrack].GetRow(3);

			// This is a hack for old data...
			if (_iRotTrack == 22 && pHand->m_iNode == 0 && pHand->m_LocalPos.GetRow(3).LengthSqr() > 0.01f)
			{
				Pos = _pSkelInstance->m_pBoneTransform[22];
				Pos.GetRow(3) = pHand->m_LocalPos.GetRow(3) * _pSkelInstance->m_pBoneTransform[16];
			}
			else if (_iRotTrack == 22 && LocalPos.LengthSqr() > 16.0f && _pSkel->m_lNodes[_iRotTrack].m_LocalCenter.LengthSqr() < 0.1f)
			{ // This is another hack for old data...
				Pos = _pSkelInstance->m_pBoneTransform[_iRotTrack];
				const CVec3Dfp32& BindPosPos = _pSkel->m_lNodes[_iRotTrack].m_LocalCenter;
				CVec3Dfp32 LocalPos = BindPosPos;
				LocalPos.k[1] += 40.0f;

				Pos.GetRow(3) = LocalPos * Pos;
			}
			else
			{
				if (pHand->m_iNode)
					_iRotTrack = pHand->m_iNode;
				const CMat4Dfp32& BoneTransform = _pSkelInstance->m_pBoneTransform[_iRotTrack];
				pHand->m_LocalPos.Multiply(BoneTransform, Pos);
			}
		}
		else
		{
			Pos = _pSkelInstance->m_pBoneTransform[_iRotTrack];
		//	const CVec3Dfp32& BindPosPos = _pSkel->m_lNodes[_iRotTrack].m_LocalCenter;
		//	Pos.GetRow(3) = BindPosPos * Pos;
			Pos.GetRow(3) = CVec3Dfp32(0) * Pos;
		}
	}
	else
	{
		Pos = _pSkelInstance->m_pBoneTransform[_iRotTrack];
	}

	if (FloatIsInvalid(Pos.k[0][0]))
		return false;

	// Get Weapon attachment matrix
	CXR_Skeleton* pSkelGun = (CXR_Skeleton*)_pModel->GetParam(MODEL_PARAM_SKELETON);
	CMat4Dfp32 Handle;
	const CXR_SkeletonAttachPoint* pGunHandle = (pSkelGun) ? pSkelGun->GetAttachPoint(0) : NULL;
	if (pGunHandle)
		pGunHandle->m_LocalPos.InverseOrthogonal(Handle);
	else
		Handle.Unit();

	Handle.Multiply(Pos, _Mat);
	return true;
}


bool CAutoVar_AttachModel::GetModel0_RenderInfo(CMapData *_pMapData, CXR_Engine* _pEngine, CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, const CMTime& _Time,
	CXR_AnimState &_RetAnim, CMat4Dfp32 &_RetPos, CXR_Model *&_pRetModel, CWorld_PhysState* _pWPhys)
{
	MAUTOSTRIP(CAutoVar_AttachModel_GetModel0_RenderInfo, false);
	_pRetModel = _pMapData->GetResource_Model(m_iModel[0]);
	if(_pRetModel)
	{
		if(GetModel0_RenderMatrix(_pSkelInstance, _pSkel, _pRetModel, m_iAttachRotTrack, m_iAttachPoint[0], _RetPos))
		{
			if(_pRetModel->GetParam(CXR_MODEL_PARAM_TIMEMODE) == CXR_MODEL_TIMEMODE_CONTINUOUS)
			{
				_RetAnim.m_AnimTime0 = _Time;
				_RetAnim.m_AnimTime1.Reset();
			}
			else
			{
				_RetAnim.m_AnimTime0 = AC_Get(_Time);
				_RetAnim.m_AnimTime1 = _Time;
			}
			_RetAnim.m_Anim0 = m_AC_RandSeed & 0x7fff;
			_RetAnim.m_Data[3] = m_ModelFlags[0];
			_RetAnim.m_pModelInstance = m_lspModelInstances[0];

			// Do skeleton animation
			if (_pWPhys != NULL)
			{
				_RetAnim.m_AnimTime0 = _Time;
				//DoSkelAnim(&_RetAnim, &_RetPos, _pWPhys, _Time);
				CXR_Skeleton* pSkel = (CXR_Skeleton*)_pRetModel->GetParam(MODEL_PARAM_SKELETON);
				// Alloc skelinstance from vbm if possible

				if (pSkel)
				{
					//...
					if (_pEngine)
					{
						_RetAnim.m_pSkeletonInst = CXR_VBMHelper::Alloc_SkeletonInst(_pEngine->GetVBM(), pSkel->m_lNodes.Len(), pSkel->m_nUsedRotations, pSkel->m_nUsedMovements);
					}
					else
					{
						if (!m_spSkeletonInstance)
						{
							CreateClientSkelInstance();
							if (m_spSkeletonInstance)
								m_spSkeletonInstance->Create(pSkel->m_lNodes.Len());
						}
						_RetAnim.m_pSkeletonInst = m_spSkeletonInstance;
					}

					GetAnimLayers(pSkel, &_RetAnim, &_RetPos, _pWPhys, _Time);
				}
			}
			
			return true;
		}
	}
	return false;
}

void CAutoVar_AttachModel::RenderExtraModels(CMapData *_pMapData, CXR_Engine* _pEngine,
					   CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, const CMat4Dfp32 &_Mat, const CMTime& _Time, uint16 _iObject)
{
	MAUTOSTRIP(CAutoVar_AttachModel_RenderExtraModels, MAUTOSTRIP_VOID);
	CXR_Model *pBaseModel = _pMapData->GetResource_Model(m_iModel[0]);
	if(!pBaseModel)
		return;

	m_ExtraModels.RenderExtraModels(_pMapData,_pEngine,pBaseModel,_pSkelInstance,_pSkel,_Mat,_Time,_iObject);

	for(int i = 1; i < ATTACHMODEL_NUMMODELS; i++)
	{
		if(m_iModel[i] != 0)
		{
			CXR_Model *pModel = _pMapData->GetResource_Model(m_iModel[i]);
			if(pModel)
			{
				//ConOutL(CStrF("Time: %f  (GT: %f) TS: %f",AC_Get(_Time).GetTime(),_Time.GetTime(),m_AC_Timestamp.GetTime()));
				CXR_AnimState AnimState;
				if(pModel->GetParam(CXR_MODEL_PARAM_TIMEMODE) == CXR_MODEL_TIMEMODE_CONTINUOUS)
				{
					AnimState.m_AnimTime0 = _Time;
					AnimState.m_AnimTime1.Reset();
				}
				else
				{
					AnimState.m_AnimTime0 = AC_Get(_Time);
					AnimState.m_AnimTime1 = _Time;
				}

				CMat4Dfp32 Mat = _Mat;
				CXR_Skeleton* pSkelItem = (CXR_Skeleton*)pBaseModel->GetParam(MODEL_PARAM_SKELETON);
				if(pSkelItem)
				{
					const CXR_SkeletonAttachPoint *pAttach = pSkelItem->GetAttachPoint(m_iAttachPoint[i]);
					if(pAttach)
						CVec3Dfp32::GetRow(Mat, 3) = pAttach->m_LocalPos.GetRow(3) * _Mat;
				}
				
/*				{
					int iModel = _pMapData->GetResourceIndex_Model("coordsys");
					CXR_Model *pModel = _pMapData->GetResource_Model(iModel);
					if(pModel)
						_pEngine->Render_AddModel(pModel, Mat, AnimState);
				}*/
				
				AnimState.m_iObject = _iObject;
				AnimState.m_pContext = (CXR_Skeleton*)pBaseModel->GetParam(MODEL_PARAM_SKELETON);
				AnimState.m_Anim0 = (2 << 8) | 3;
				AnimState.m_pModelInstance = m_lspModelInstances[i];
				AnimState.m_Anim0 = m_AC_RandSeed & 0x7fff;
				AnimState.m_Data[3] = m_ModelFlags[i];
				_pEngine->Render_AddModel(pModel, Mat, AnimState);
			}
		}
	}
}

void CAutoVar_AttachModel::RenderAll(CMapData *_pMapData, CXR_Engine* _pEngine,
	CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel,
	const CMat4Dfp32 &_Mat, const CMTime& _Time, CWorld_PhysState* _pWPhys, uint16 _iObject)
{
	MAUTOSTRIP(CAutoVar_AttachModel_RenderAll, MAUTOSTRIP_VOID);
	CXR_AnimState AnimState;
	CXR_Model *pModel;
	CMat4Dfp32 Pos;
	if(GetModel0_RenderInfo(_pMapData, _pEngine, _pSkelInstance, _pSkel, _Time, AnimState, Pos, pModel, _pWPhys))
	{
		AnimState.m_iObject = _iObject;
		_pEngine->Render_AddModel(pModel, Pos, AnimState);
		RenderExtraModels(_pMapData, _pEngine, _pSkelInstance, _pSkel, Pos, _Time, _iObject);
	}
}

void CAutoVar_AttachModel::RefreshModelInstances(CWObject_Client* _pObj, CWorld_Client* _pWClient, int _GameTick)
{
	MAUTOSTRIP(CAutoVar_AttachModel_RefreshModelInstances, MAUTOSTRIP_VOID);
	bool bInit = false;
	CMat4Dfp32 Mat;

	CXR_ModelInstanceContext ModelInstanceContext(_GameTick, _pWClient->GetGameTickTime(), _pObj, _pWClient);

	int nExtraModels = m_ExtraModels.GetNumExtraModels();
	for(int i = 0; i < ATTACHMODEL_NUMMODELS + nExtraModels; i++)
	{
		CXR_ModelInstance* pInstance; 
		int16 iModel;
		int8 ModelFlags;
		if (i < ATTACHMODEL_NUMMODELS)
		{
			pInstance = m_lspModelInstances[i];
			iModel = m_iModel[i];
			ModelFlags = m_ModelFlags[i];
		}
		else
		{
			int32 iSlot = i - ATTACHMODEL_NUMMODELS;
			pInstance = m_ExtraModels.m_lspExtraModelInstances[iSlot];
			iModel = m_ExtraModels.m_liExtraModel[iSlot];
			ModelFlags = 0;
		}
		if(pInstance)
		{
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(iModel);
			if(pInstance->NeedRefresh(pModel, ModelInstanceContext))
			{
				CXR_AnimState AnimState;
				if(!bInit)
				{
					CWObject_Message Msg(OBJSYSMSG_GETANIMSTATE);
					Msg.m_pData = &AnimState;
					Msg.m_DataSize = sizeof(AnimState);
					if(!_pWClient->ClientMessage_SendToObject(Msg, _pObj->m_iObject))
						return;

					ModelInstanceContext.m_pAnimState = &AnimState;
					if(pModel->GetParam(CXR_MODEL_PARAM_TIMEMODE) != CXR_MODEL_TIMEMODE_CONTINUOUS)
					{
						//CMTime AnimTime0 = AC_Get(AnimState.m_AnimTime0);
						AnimState.m_AnimTime1 = AnimState.m_AnimTime0;
						AnimState.m_AnimTime0 = AC_Get(AnimState.m_AnimTime0);
//						const fp32 Time0 = AnimTime0.GetTime();
//						
//						if(Time0 > 0.0f)
//							// ... do something with anim time maybe?
					}

					if(!AnimState.m_pSkeletonInst)
						return;
					
					CXR_Model *pChar = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
					CXR_Skeleton *pSkel = pChar ? (CXR_Skeleton*)pChar->GetParam(MODEL_PARAM_SKELETON) : NULL;
					if(!pSkel)
						return;

					CXR_Model *pBase = _pWClient->GetMapData()->GetResource_Model(m_iModel[0]);
					if(!pBase)
						return;

					if(!GetModel0_RenderMatrix(AnimState.m_pSkeletonInst, pSkel, pBase, m_iAttachRotTrack, m_iAttachPoint[0], Mat))
						return;

					bInit = true;
				}
				
				pInstance->OnRefresh(ModelInstanceContext, &Mat, 1, ModelFlags);
			}
		}
	}
}

void CAutoVar_AttachModel::Refresh(CWObject_Client* _pObj, CWorld_Client* _pWClient, int _GameTick)
{
	MAUTOSTRIP(CAutoVar_AttachModel_Refresh, MAUTOSTRIP_VOID);
	RefreshModelInstances(_pObj, _pWClient, _GameTick);
}

void CAttach_ExtraModels::RenderExtraModels(CMapData *_pMapData, CXR_Engine* _pEngine, CXR_Model* _pBaseModel,
				  CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, const CMat4Dfp32 &_Mat, const CMTime& _Time, uint16 _iObject)
{
	MAUTOSTRIP(CAttach_ExtraModels_RenderExtraModels, MAUTOSTRIP_VOID);
	if(!_pBaseModel)
		return;

	CXR_AnimState AnimState;
	AnimState.m_iObject = _iObject;
	AnimState.m_pContext = (CXR_Skeleton*)_pBaseModel->GetParam(MODEL_PARAM_SKELETON);
	//...
	//AnimState.m_Data[3] = m_ModelFlags[i];

	int Len = GetNumExtraModels();
	for(int i = 0; i < Len; i++)
	{
		if(m_liExtraModel[i] != 0)
		{
			CXR_Model *pModel = _pMapData->GetResource_Model(m_liExtraModel[i]);
			if(pModel)
			{
				//ConOutL(CStrF("Time: %f  (GT: %f) TS: %f",AC_Get(_Time).GetTime(),_Time.GetTime(),m_AC_Timestamp.GetTime()));
				if(pModel->GetParam(CXR_MODEL_PARAM_TIMEMODE) == CXR_MODEL_TIMEMODE_CONTINUOUS)
				{
					AnimState.m_AnimTime0 = _Time;
					AnimState.m_AnimTime1.Reset();
				}
				else
				{
					AnimState.m_AnimTime0 = GetTime(_Time,i);
					AnimState.m_AnimTime1 = _Time;

					// If we're over duration, just continue...
					if (AnimState.m_AnimTime0.GetTime() > m_lExtraModelDuration[i])
						continue;
				}

				// Create model instance if not created yet
				/*if (!m_lspExtraModelInstances[i])
				{
					m_lspExtraModelInstances[i] = pModel->CreateModelInstance();
					if (m_lspExtraModelInstances[i] != NULL)
						m_lspExtraModelInstances[i]->Create(pModel, CXR_ModelInstanceContext(NULL, NULL));
				}*/

				/*if (AC_Get(_Time).GetTime() > 0.0f)
				int i = 0;*/

				CMat4Dfp32 Mat = _Mat;
				CXR_Skeleton* pSkelItem = (CXR_Skeleton*)_pBaseModel->GetParam(MODEL_PARAM_SKELETON);
				if(pSkelItem)
				{
					const CXR_SkeletonAttachPoint *pAttach = pSkelItem->GetAttachPoint(m_liExtraModelAttachPoint[i]);
					if(pAttach)
						CVec3Dfp32::GetRow(Mat, 3) = pAttach->m_LocalPos.GetRow(3) * _Mat;
				}

				/*				{
				int iModel = _pMapData->GetResourceIndex_Model("coordsys");
				CXR_Model *pModel = _pMapData->GetResource_Model(iModel);
				if(pModel)
				_pEngine->Render_AddModel(pModel, Mat, AnimState);
				}*/

				//ConOut(CStrF("Render Time: %f StartTime: %f CurrentTime: %f",AnimState.m_AnimTime0.GetTime(),m_ExtraModelStartTime[i].GetTime(),_Time.GetTime()));

				AnimState.m_iObject = _iObject;
				AnimState.m_pContext = (CXR_Skeleton*)_pBaseModel->GetParam(MODEL_PARAM_SKELETON);
				AnimState.m_Anim0 = (2 << 8) | 3;
				AnimState.m_pModelInstance = m_lspExtraModelInstances[i];
				AnimState.m_Anim0 = 0;//m_AC_RandSeed & 0x7fff;
				AnimState.m_Anim0 = m_lExtraModelSeed[i];
				AnimState.m_Data[3] = 0;//m_ModelFlags[i];
				_pEngine->Render_AddModel(pModel, Mat, AnimState);
			}
			else
			{
				m_lspExtraModelInstances[i] = NULL;
			}
		}
	}
}

/* -- no longer supported
void CAutoVar_AttachModel::EvalAnimKey(int _iController, const CXR_Anim_DataKey* _pKey, int _Tick)
{
	if (_pKey->m_EventParams[1] == 10)
		m_ModelFlags[0] = _pKey->m_EventParams[2];
	if (_pKey->m_EventParams[1] == 1)
		m_ModelFlags[0] = 1;
	if (_pKey->m_EventParams[1] == 2)
		m_ModelFlags[0] = 0;

	for (int iModel = 1; iModel < ATTACHMODEL_NUMMODELS; iModel++)
		m_ModelFlags[iModel] = m_ModelFlags[0];

	if (_pKey->m_EventParams[1] == 3 + _iController)
	{
		if(_pKey->m_EventParams[3] == 0)
			AC_SetTarget(CMTime::CreateFromSeconds(_pKey->m_EventParams[2]), 0, _Tick);
		else
			AC_SetTarget(CMTime::CreateFromSeconds(_pKey->m_EventParams[2]), 100.0f / fp32(_pKey->m_EventParams[3]), _Tick);
	}
}
*/
