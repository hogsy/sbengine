#include "PCH.h"
#include "WObj_Turret.h"
#include "../../GameWorld/WServerMod.h"
#include "../../GameWorld/WClientMod_Defines.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Turret, CWObject, 0x0100);

void CWObject_Turret::OnCreate()
{
	CWObject_TurretParent::OnCreate();
	m_LastActivation = 0;
	m_bDismountAvailable = true;
	m_TrackingTime = 0;
	m_TempGUID = 0;
}

void CWObject_Turret::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	CWO_Turret_ClientData& CD = GetClientData(this);
	const CStr KeyName = _pKey->GetThisName();
	CStr Value = _pKey->GetThisValue();
	// Can't hash "dynamic" keys ie mountmessage1,mountmessage2 etc..
	if (KeyName.Find("DISMOUNTMESSAGE") != -1)
	{
		m_lMsgOnDismount.SetLen(m_lMsgOnDismount.Len() + 1);
		m_lMsgOnDismount[m_lMsgOnDismount.Len()-1].Parse(Value, m_pWServer);
		return;
	}
	else if (KeyName.Find("MOUNTMESSAGE") != -1)
	{
		m_lMsgOnMount.SetLen(m_lMsgOnMount.Len() + 1);
		m_lMsgOnMount[m_lMsgOnMount.Len()-1].Parse(Value, m_pWServer);
		return;
	}

	switch (_KeyHash)
	{
	case MHASH4('VISI','BILI','TYMA','SK'): // "VISIBILITYMASK"
		CD.m_VisibilityMask = _pKey->GetThisValuei();
		break;
	case MHASH3('ANIM','GRAP','H2'): // "ANIMGRAPH2"
		{
			int32 iAG2 = m_pWServer->GetMapData()->GetResourceIndex_AnimGraph2(Value);
			CD.m_AnimGraph2.GetAG2I()->AddResourceIndex_AnimGraph2(iAG2,Value);
			break;
		}
	case MHASH3('MAXA','NGLE','Z'): // "MAXANGLEZ"
		{
			fp32 Val;
			_pKey->GetThisValueaf(1,&Val);
			CD.m_MaxTurrentAngleZ = Val;
			break;
		}
	case MHASH3('MAXA','NGLE','Y'): // "MAXANGLEY"
		{
			fp32 Val;
			_pKey->GetThisValueaf(1,&Val);
			CD.m_MaxTurrentAngleY = Val;
			break;
		}
	case MHASH3('MAXA','NGLE','ZI'): // "MAXANGLEZI"
		{
			int32 Val;
			_pKey->GetThisValueai(1,&Val);
			CD.m_MaxTurrentAngleZ = ((fp32)Val) / 360.0f;
			break;
		}
	case MHASH3('MAXA','NGLE','YI'): // "MAXANGLEYI"
		{
			int32 Val;
			_pKey->GetThisValueai(1,&Val);
			CD.m_MaxTurrentAngleY = ((fp32)Val) / 360.0f;
			break;
		}
	case MHASH3('AIMD','AMPE','R'): // "AIMDAMPER"
		{
			fp32 Val;
			_pKey->GetThisValueaf(1,&Val);
			CD.m_AimDamper = Val;
		}
	case MHASH2('WEAP','ON'): // "WEAPON"
		{
			spCRPG_Object spObj = CRPG_Object::CreateObject(Value,m_pWServer);
			if (spObj != NULL && TDynamicCast<CRPG_Object_Item >((CRPG_Object *)spObj) != NULL)
			{
				m_spWeapon = (CRPG_Object_Item *)((CRPG_Object *)spObj);
				m_spWeapon->m_Flags2 |= RPG_ITEM_FLAGS2_NOAMMODRAW;
			}
			break;
		}
	case MHASH3('SHOO','TBON','E'): // "SHOOTBONE"
		{
			int32 ShootBone;
			_pKey->GetThisValueai(1,&ShootBone);
			CD.m_iShootBone0 = ShootBone;
			break;
		}
	case MHASH3('SHOO','TBON','E1'): // "SHOOTBONE1"
		{
			int32 ShootBone;
			_pKey->GetThisValueai(1,&ShootBone);
			CD.m_iShootBone1 = ShootBone;
			break;
		}
	case MHASH3('CAME','RAOF','FSET'): // "CAMERAOFFSET"
		{
			CVec3Dfp32 TempVec;
			_pKey->GetThisValueaf(3,(fp32*)&(TempVec.k));
			CD.m_CameraOffset = TempVec;
			break;
		}
	case MHASH4('CAME','RARO','TTRA','CK'): // "CAMERAROTTRACK"
		{
			int32 iRotTrack;
			_pKey->GetThisValueai(1,&iRotTrack);
			CD.m_iCameraRotTrack = (uint8)iRotTrack;
			break;
		}
	case MHASH4('TURR','ETRO','TTRA','CK'): // "TURRETROTTRACK"
		{
			int32 iRotTrack;
			_pKey->GetThisValueai(1,&iRotTrack);
			CD.m_iTurretRotTrack = (uint8)iRotTrack;
			break;
		}
	case MHASH4('REND','ERAT','TACH','BONE'): // "RENDERATTACHBONE"
		{
			CD.m_iRenderAttachBone = (int8)_pKey->GetThisValuei();
			break;
		}
	case MHASH5('REND','ERAT','TACH','POIN','T'): // "RENDERATTACHPOINT"
		{
			CD.m_iRenderAttachPoint = (int8)_pKey->GetThisValuei();
			break;
		}
	case MHASH4('REND','ERAT','TACH','ED'): // "RENDERATTACHED"
		{
			m_RenderAttachedObj = Value;
			break;
		}
	case MHASH4('ATTA','CHLE','FTHA','ND'): // "ATTACHLEFTHAND"
		{
			CVec3Dfp32 TempVec;
			_pKey->GetThisValueaf(3,(fp32*)&(TempVec.k));
			CD.m_AttachLeftHand = TempVec;
			break;
		}
	case MHASH4('ATTA','CHRI','GHTH','AND'): // "ATTACHRIGHTHAND"
		{
			CVec3Dfp32 TempVec;
			_pKey->GetThisValueaf(3,(fp32*)&(TempVec.k));
			CD.m_AttachRightHand = TempVec;
			break;
		}
	case MHASH4('CHAR','ACTE','ROFF','SET'): // "CHARACTEROFFSET"
		{
			CVec3Dfp32 TempVec;
			_pKey->GetThisValueaf(3,(fp32*)&(TempVec.k));
			CD.m_CharacterOffset = TempVec;
			break;
		}
	case MHASH4('FLAS','HLIG','HTCO','LOR'): // "FLASHLIGHTCOLOR"
		{
			int32 Color;
			_pKey->GetThisValueai(1,&Color);
			CD.m_FlashLightColor = Color;
			break;
		}
	case MHASH4('FLAS','HLIG','HTRA','NGE'): // "FLASHLIGHTRANGE"
		{
			int32 Range;
			_pKey->GetThisValueai(1,&Range);
			CD.m_FlashLightRange = Range;
			break;
		}
	case MHASH4('MUZZ','LELI','GHTC','OLOR'): // "MUZZLELIGHTCOLOR"
		{
			int32 Color;
			_pKey->GetThisValueai(1,&Color);
			CD.m_MuzzleLightColor = Color;
			break;
		}
	case MHASH4('MUZZ','LELI','GHTR','ANGE'): // "MUZZLELIGHTRANGE"
		{
			int32 Range;
			_pKey->GetThisValueai(1,&Range);
			CD.m_MuzzleLightRange = Range;
			break;
		}
	case MHASH3('TURR','ETFL','AGS'): // "TURRETFLAGS"
		{
			// 
			static const char *FlagsTranslate[] =
			{
				"flashlight","nomuzzleflash","mountlight","waitspawn","blackholeenabled","scriptedlight","nolightshadow","noturretcam",NULL
			};

			CD.m_TurretFlags = CD.m_TurretFlags | Value.TranslateFlags(FlagsTranslate);
			break;
		}
	case MHASH3('TURR','ETTY','PE'): // "TURRETTYPE"
		{
			int32 Type;
			_pKey->GetThisValueai(1,&Type);
			CD.m_TurretType = Type;
			break;
		}
	case MHASH3('TRAC','KING','TIME'): // "TRACKINGTIME"
		{
			m_TrackingTime = (int16)(Value.Val_fp64() * m_pWServer->GetGameTicksPerSecond());
			break;
		}
	default:
		CWObject_TurretParent::OnEvalKey(_KeyHash, _pKey);
		break;
	}
}

void CWObject_Turret::OnFinishEvalKeys()
{
	// ...
	CWObject_TurretParent::OnFinishEvalKeys();
	CWO_Turret_ClientData& CD = GetClientData(this);
	// Make sure all animations are precached
	CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
	CWAG2I* pAGI = CD.m_AnimGraph2.GetAG2I();

	TArray<CXRAG2_Impulse> lImpulses;
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE, 0));
	pAGI->TagAnimSetFromImpulses(&AGIContext, m_pWServer->GetMapData(), m_pWServer->m_spWData, lImpulses);

	CWServer_Mod* pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
	uint nAnimGraphs = pAGI->GetNumResource_AnimGraph2();
	for (uint i = 0; i < nAnimGraphs; i++)
		pServerMod->RegisterAnimGraph2(pAGI->GetResourceIndex_AnimGraph2(i));

	// Set weapon ag for now...
	if (m_spWeapon && m_spWeapon->m_iAnimGraph)
	{
		CD.m_AnimGraph2.GetAG2I()->SetResourceIndex_AnimGraph2(m_spWeapon->m_iAnimGraph,"",0);
	}
}

void CWObject_Turret::OnSpawnWorld()
{
	CWObject_TurretParent::OnSpawnWorld();

	for (int32 i = 0; i < m_lMsgOnMount.Len(); i++)
		m_lMsgOnMount[i].SendPrecache(m_iObject, m_pWServer);
	for (int32 i = 0; i < m_lMsgOnDismount.Len(); i++)
		m_lMsgOnDismount[i].SendPrecache(m_iObject, m_pWServer);
}

void CWObject_Turret::OnSpawnWorld2()
{
	CWObject_TurretParent::OnSpawnWorld2();

	if (m_RenderAttachedObj.Len())
	{
		CWO_Turret_ClientData& CD = GetClientData(this);
		CD.m_iRenderAttached = m_pWServer->Selection_GetSingleTarget(m_RenderAttachedObj);

		if (CD.m_iRenderAttached > 0)
		{
			CMat4Dfp32 ParentMat;
			CWObject_Message Msg(OBJMSG_HOOK_GETCURRENTMATRIX, (aint)&ParentMat);
			if (m_pWServer->Phys_Message_SendToObject(Msg, CD.m_iRenderAttached))
			{
				CMat4Dfp32 ParentInv, LocalMat;
				ParentMat.InverseOrthogonal(ParentInv);
				GetPositionMatrix().Multiply(ParentInv, LocalMat);
				CD.m_TurretLocalPos = LocalMat;
			}
		}
	}
}

void CWObject_Turret::GetShootDirection0(CMat4Dfp32& _Cam)
{
	CWO_Turret_ClientData& CD = GetClientData(this);
	CXR_AnimState Anim;
	CXR_SkeletonInstance* pSkelInst = NULL;
	CXR_Skeleton* pSkel = NULL;
	if (GetEvaluatedPhysSkeleton(this,m_pWServer,pSkelInst,pSkel,Anim,0.0f,NULL))
	{
		if (pSkelInst && pSkelInst->m_nBoneTransform >= CD.m_iShootBone0)
		{
			_Cam = pSkelInst->m_pBoneTransform[CD.m_iShootBone0];
			return;
		}
	}

	_Cam = GetPositionMatrix();
}

void CWObject_Turret::GetShootDirection1(CMat4Dfp32& _Cam)
{
	CWO_Turret_ClientData& CD = GetClientData(this);
	CXR_AnimState Anim;
	CXR_SkeletonInstance* pSkelInst = NULL;
	CXR_Skeleton* pSkel = NULL;
	if (GetEvaluatedPhysSkeleton(this,m_pWServer,pSkelInst,pSkel,Anim,0.0f,NULL))
	{
		if (pSkelInst && pSkelInst->m_nBoneTransform >= CD.m_iShootBone1)
		{
			_Cam = pSkelInst->m_pBoneTransform[CD.m_iShootBone1];
			return;
		}
	}

	_Cam = GetPositionMatrix();
}

void CWObject_Turret::OnGetAnimState(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CXR_Skeleton* _pSkel, int _iCacheSlot, const CMat4Dfp32 &_Pos, fp32 _IPTime, CXR_AnimState& _Anim, CVec3Dfp32 *_pRetPos, bool _bIgnoreCache)
{
	CWO_Turret_ClientData& CD = GetClientData(_pObj);
	CXR_SkeletonInstance* pSkelInstance = CD.m_spSkelInstance;
	
	if(_Anim.m_pSkeletonInst)
	{
		pSkelInstance = _Anim.m_pSkeletonInst;
		_bIgnoreCache = true;
	}
	else if (_pSkel && !pSkelInstance && (_iCacheSlot == 1))
	{
		MRTC_SAFECREATEOBJECT(spSkel, "CXR_SkeletonInstance", CXR_SkeletonInstance);
		CD.m_spSkelInstance = spSkel;
		spSkel->Create(_pSkel->m_lNodes.Len());
		pSkelInstance = spSkel;
		_bIgnoreCache = true;
	}

	if (_bIgnoreCache || CD.m_LastSkelInstanceTick != _pWPhysState->GetGameTick() || 
		CD.m_LastSkelInstanceFrac != _IPTime)
	{
		// Eval skel
		CXR_AnimLayer Layers[16];
		int nLayers = 16;
		CWAG2I_Context AGIContext(_pObj, _pWPhysState, CMTime::CreateFromTicks(_pWPhysState->GetGameTick(),_pWPhysState->GetGameTickTime(),_IPTime));
		CD.m_AnimGraph2.GetAG2I()->GetAnimLayers(&AGIContext, Layers, nLayers, 0);
		CMat4Dfp32 Pos;// = _Pos;
		Pos.Unit();
		if (nLayers > 0)
			_pSkel->EvalAnim(Layers, nLayers, pSkelInstance, Pos);
		else if (_iCacheSlot == 1)
		{
			// Clear skeleton
			for (int32 i = 0; i < pSkelInstance->m_nBoneLocalPos; i++)
				pSkelInstance->m_pBoneLocalPos[i].Unit();
			for (int32 i = 0; i < pSkelInstance->m_nBoneTransform; i++)
				pSkelInstance->m_pBoneTransform[i].Unit();
		}

		// Add aim modifier
		CD.AddAimModifier(pSkelInstance, _pWPhysState);
		_pSkel->EvalNode(0, &_Pos, pSkelInstance);
		if (!_bIgnoreCache)
		{
			CD.m_LastSkelInstanceTick = _pWPhysState->GetGameTick();
			CD.m_LastSkelInstanceFrac = _IPTime;
		}
	}

	_Anim.m_pSkeletonInst = pSkelInstance;
	_Anim.m_SurfaceOcclusionMask = 0;
	_Anim.m_SurfaceShadowOcclusionMask = 0;
	_Anim.m_AnimTime0 = _pWPhysState->GetGameTime();
}

void CWObject_Turret::GetRenderAttachPos(CWorld_PhysState* _pWPhysState, CWO_Turret_ClientData& CD, CMat4Dfp32& _Mat, fp32 _IPTime)
{
	CWObject_CoreData* pObj = _pWPhysState->Object_GetCD(CD.m_iRenderAttached);
	if(pObj)
	{
		CMat4Dfp32 ParentMat;
		CMTime Time = CMTime::CreateFromTicks(_pWPhysState->GetGameTick(),_pWPhysState->GetGameTickTime(),_IPTime);
		CWObject_Message Msg;
		struct SkelProp
		{
			CXR_Skeleton* m_pSkel;
			CXR_SkeletonInstance* m_pSkelInst;
		};
		struct SkelProp SkelProps;
		SkelProps.m_pSkel = NULL;
		SkelProps.m_pSkelInst = NULL;
		if (_pWPhysState->IsClient())
		{
			Msg = CWObject_Message(OBJMSG_HOOK_GETCURRENTMATRIX, (aint)&ParentMat,0,CD.m_iRenderAttachBone);
			Msg.m_pData = &SkelProps;
		}
		else
		{
			CWObject_Message TimeMsg(OBJMSG_HOOK_GETTIME);
			TimeMsg.m_pData = &Time;
			TimeMsg.m_DataSize = sizeof(Time);
			_pWPhysState->Phys_Message_SendToObject(TimeMsg, CD.m_iRenderAttached);
			Msg = CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (aint)&Time,(aint)&ParentMat,CD.m_iRenderAttachBone);
			Msg.m_pData = &SkelProps;
		}
		if(_pWPhysState->Phys_Message_SendToObject(Msg, CD.m_iRenderAttached))
		{
			if (CD.m_iRenderAttachBone >= 0 && SkelProps.m_pSkelInst && 
				SkelProps.m_pSkel && SkelProps.m_pSkelInst->m_nBoneTransform >= CD.m_iRenderAttachBone)
			{
				CMat4Dfp32 AttachMat = SkelProps.m_pSkelInst->m_pBoneTransform[CD.m_iRenderAttachBone];
				if (CD.m_iRenderAttachPoint >= 0)
				{
					const CXR_SkeletonAttachPoint* pPoint = SkelProps.m_pSkel->GetAttachPoint(CD.m_iRenderAttachPoint);
					if (pPoint)
					{
						CVec3Dfp32 Point = pPoint->m_LocalPos.GetRow(3) * AttachMat;
						AttachMat.GetRow(3) = Point;
					}
				}
				ParentMat = AttachMat;
			}
			CD.GetLocalPos().Multiply(ParentMat, _Mat);		
		}
		else
			_Mat = pObj->GetPositionMatrix();
	}
}

bool CWObject_Turret::GetEvaluatedPhysSkeleton(CWObject_CoreData *_pObj, CWorld_PhysState* _pWPhysState, CXR_SkeletonInstance *&_pInstance, CXR_Skeleton *&_pSkeleton, CXR_AnimState& _Anim, fp32 _IPTime, const CMat4Dfp32* _pPos)
{
	MAUTOSTRIP(CWObject_Character_GetEvaluatedSkeleton, false);
	CXR_Model* pModel = _pWPhysState->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if(!pModel)
		return false;
	CXR_Skeleton* pSkel = pModel->GetPhysSkeleton();
	if(!pSkel)
		return false;

	CWO_Turret_ClientData& CD = GetClientData(_pObj);

	CMat4Dfp32 TempMat;
	if (CD.m_iRenderAttached != 0)
	{
		GetRenderAttachPos(_pWPhysState,CD,TempMat,_IPTime);
		_pPos = &TempMat;
	}

	if (_pPos)
		OnGetAnimState(_pObj, _pWPhysState, pSkel, 1, *_pPos, _IPTime, _Anim);
	else
		OnGetAnimState(_pObj, _pWPhysState, pSkel, 1, _pObj->GetPositionMatrix(), _IPTime, _Anim);

	_pInstance = CD.m_spSkelInstance;
	if(!_pInstance)
		return false;

	_pSkeleton = pSkel;
	return true;
}

void CWObject_Turret::Spawn(bool _bSpawn)
{
	CWO_Turret_ClientData& CD = GetClientData(this);
	if (_bSpawn)
	{
		if (CD.m_TurretFlags & TURRET_FLAG_WAITSPAWNED)
		{
			CD.m_TurretFlags = CD.m_TurretFlags & ~TURRET_FLAG_WAITSPAWNED;
			//ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
			//m_DirtyMask |= CD.AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
		}
	}
	else
	{
		// Dismount and stuff ?
		if (!(CD.m_TurretFlags & TURRET_FLAG_WAITSPAWNED))
		{
			CD.m_TurretFlags = CD.m_TurretFlags | TURRET_FLAG_WAITSPAWNED;
			//ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
		}
	}
}

aint CWObject_Turret::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	/*case OBJMSG_CHAR_UPDATEMOUNTEDLOOK:
		{
			// Update look
			CWO_Turret_ClientData& CD = GetClientData(this);
			CD.m_TurrentAngleZ = M_FMod(CD.m_TurrentAngleZ + _Msg.m_VecParam0[2] + 2.0f, 1.0f);
			MACRO_ADJUSTEDANGLE(CD.m_TurrentAngleZ);
			CD.m_TurrentAngleZ = Max(Min(CD.m_TurrentAngleZ > 0.5f ? CD.m_TurrentAngleZ - 1.0f : CD.m_TurrentAngleZ,CD.m_MaxTurrentAngleZ.m_Value),-CD.m_MaxTurrentAngleZ.m_Value);
			CD.m_TurrentAngleY = Min(CD.m_MaxTurrentAngleY.m_Value, Max(-CD.m_MaxTurrentAngleY.m_Value, CD.m_TurrentAngleY + _Msg.m_VecParam0[1]));
			return 1;
		}*/
	case OBJMSG_CHAR_REQUESTMOUNT:
		{
			CWO_Turret_ClientData& CD = GetClientData(this);
			if (CD.m_TurretFlags & TURRET_FLAG_WAITSPAWNED)
				return 0;

			CD.m_iMountedChar = _Msg.m_iSender;
			// If turret type is floodlight, add the flashlight param when mounting
			if ((CD.m_TurretFlags & TURRET_FLAG_MOUNTLIGHT) && !(CD.m_TurretFlags & TURRET_FLAG_LIGHTSCRIPTED))
				CD.m_ItemFlags = CD.m_ItemFlags |RPG_ITEM_FLAGS_FLASHLIGHT;
			
			// send mount messages
			for (int32 i = 0; i < m_lMsgOnMount.Len(); i++)
				m_lMsgOnMount[i].SendMessage(m_iObject, _Msg.m_iSender, m_pWServer);
			return 2;
		}
	case OBJMSG_CHAR_FORCERELEASEMOUNT:
		{
			if (m_bDismountAvailable)
			{
				CWO_Turret_ClientData& CD = GetClientData(this);

				// Safe deactivate weapon
				if (m_spWeapon)
				{
					CMat4Dfp32 Mat;
					GetShootDirection0(Mat);
					m_spWeapon->Deactivate(Mat,NULL,m_iObject,0);
				}

				CD.m_iMountedChar = 0;
				// Disable light (if any)
				if (!(CD.m_TurretFlags & TURRET_FLAG_LIGHTSCRIPTED))
					CD.m_ItemFlags = CD.m_ItemFlags & ~RPG_ITEM_FLAGS_FLASHLIGHT;
				// Send dismount messages
				for (int32 i = 0; i < m_lMsgOnDismount.Len(); i++)
					m_lMsgOnDismount[i].SendMessage(m_iObject, _Msg.m_iSender, m_pWServer);

				return 1;
			}
			return 0;
		}
	case OBJMSG_CHAR_DISABLE:
		{
			static const char *FlagsTranslate[] =
			{
				"Trigger", "Activate", "Attack", "Move", "Look", "Grab", "Darknesspowers", NULL
			};
			int32 Flags = CStrBase::TranslateFlags((char*)_Msg.m_pData, FlagsTranslate);
			m_bDismountAvailable = !((Flags & M_Bit(0)) != 0);
			return true;
		}
	case OBJMSG_TURRET_GETMINMAXYZ:
		{
			CWO_Turret_ClientData& CD = GetClientData(this);
			if (!_Msg.m_pData || _Msg.m_DataSize < sizeof(CVec3Dfp32))
				return 0;

			CVec3Dfp32* pVec = (CVec3Dfp32*)_Msg.m_pData;
			pVec->k[0] = CD.m_AimDamper;
			pVec->k[1] = CD.m_MaxTurrentAngleY;
			pVec->k[2] = CD.m_MaxTurrentAngleZ;
			return 1;
		}
	case OBJSYSMSG_GETPHYSANIMSTATE:
		{
			CXR_AnimState Anim;
			if (!_Msg.m_pData || _Msg.m_DataSize != sizeof(Anim)) return 0;

			CXR_Skeleton* pSkel;
			CXR_SkeletonInstance* pSkelInstance;
			if(!GetEvaluatedPhysSkeleton(this, m_pWServer, pSkelInstance, pSkel, Anim, 0.0f, NULL))
				return 0;

			*(CXR_AnimState*)_Msg.m_pData = Anim;
			return 1;
		}
	case OBJMSG_TURRET_HANDLEINPUT:
		{
			CWO_Turret_ClientData& CD = GetClientData(this);
			if (_Msg.m_Param0 & CONTROLBITS_PRIMARY)// && !(_Msg.m_Param1 & CONTROLBITS_PRIMARY))
			{
				// Test activate the weapon
				if (m_spWeapon && m_LastActivation < m_pWServer->GetGameTick())
				{
					// Get firing position..?
					// Get direction from gun (atm..)
					CMat4Dfp32 Mat;
					GetShootDirection0(Mat);
					CWObject_CoreData* pObjMount = m_pWServer->Object_Get(CD.m_iMountedChar);
					/*if (pObjMount && pObjMount->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
						m_PhysState.m_ObjectFlags |= OBJECT_FLAGS_PLAYER;*/
					bool bRes = m_spWeapon->Activate(Mat,NULL,m_iObject,0);
					m_LastActivation = m_pWServer->GetGameTick() + m_spWeapon->m_FireTimeout;

					// Add muzzle flash
					if (!(CD.m_TurretFlags & TURRET_FLAG_MUZZLEFLASHDISABLED))
						CD.m_MuzzleFlashTick = m_pWServer->GetGameTick();

					// Do effect
					if (bRes && m_spWeapon->m_Model.m_ExtraModels.m_lEffects.Len() > 0)
					{
						CD.m_TurretEffect0.SetExtraModel(m_pWServer->GetMapData(),CMTime::CreateFromTicks(m_pWServer->GetGameTick()+2,m_pWServer->GetGameTickTime()),2.0f,m_spWeapon->m_Model.m_ExtraModels.m_lEffects[0].m_iModel,m_spWeapon->m_Model.m_ExtraModels.m_lEffects[0].m_iAttachPoint,m_spWeapon->m_Model.m_iAttachRotTrack,m_pWServer->GetGameTick());
						CD.m_TurretEffect0.MakeDirty();
					}

					if (CD.m_TurretType == TURRET_TYPE_OWTANK)
					{
						// Activate the second rifle on the owtank turret
						GetShootDirection1(Mat);
						m_spWeapon->m_Model.m_iAttachRotTrack = CD.m_iShootBone1;
						m_spWeapon->Deactivate(Mat,NULL,m_iObject,0);
						m_spWeapon->Activate(Mat,NULL,m_iObject,0);
						// Do effect
						if (m_spWeapon->m_Model.m_ExtraModels.m_lEffects.Len() > 0)
						{
							CD.m_TurretEffect1.SetExtraModel(m_pWServer->GetMapData(),CMTime::CreateFromTicks(m_pWServer->GetGameTick()+2,m_pWServer->GetGameTickTime()),2.0f,m_spWeapon->m_Model.m_ExtraModels.m_lEffects[0].m_iModel,m_spWeapon->m_Model.m_ExtraModels.m_lEffects[0].m_iAttachPoint + 1,m_spWeapon->m_Model.m_iAttachRotTrack,m_pWServer->GetGameTick());
							CD.m_TurretEffect1.MakeDirty();
						}
						m_spWeapon->m_Model.m_iAttachRotTrack = CD.m_iShootBone0;
					}
					// Remove player flag again
					//m_PhysState.m_ObjectFlags &= ~OBJECT_FLAGS_PLAYER;
				}

				if (!(_Msg.m_Param1 & CONTROLBITS_PRIMARY))
				{
					CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_PRIMARYATTACK);
					CWAG2I_Context Context(this,m_pWServer,m_pWServer->GetGameTime());
					CD.m_AnimGraph2.GetAG2I()->SendImpulse(&Context,Impulse,AG2_TOKEN_MAIN);
					m_pWServer->Object_SetDirty(m_iObject,TURRET_AG2_DIRTYMASK);
				}
			}
			else if (_Msg.m_Param1 & CONTROLBITS_PRIMARY)
			{
				// Last press
				if (m_spWeapon)
				{
					// Deactivate
					CMat4Dfp32 Mat;
					GetShootDirection0(Mat);
					m_spWeapon->Deactivate(Mat,NULL,m_iObject,0);
					CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_RESET);
					CWAG2I_Context Context(this,m_pWServer,m_pWServer->GetGameTime());
					CD.m_AnimGraph2.GetAG2I()->SendImpulse(&Context,Impulse,AG2_TOKEN_MAIN);
					m_pWServer->Object_SetDirty(m_iObject,TURRET_AG2_DIRTYMASK);
				}
			}
			if ((_Msg.m_Param0 & CONTROLBITS_SECONDARY) && !(_Msg.m_Param1 & CONTROLBITS_SECONDARY))
			{
				CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_SECONDARYATTACK);
				CWAG2I_Context Context(this,m_pWServer,m_pWServer->GetGameTime());
				CD.m_AnimGraph2.GetAG2I()->SendImpulse(&Context,Impulse,AG2_TOKEN_MAIN);
				m_pWServer->Object_SetDirty(m_iObject,TURRET_AG2_DIRTYMASK);
			}

			if (m_spWeapon && (_Msg.m_Param1 & CONTROLBITS_PRIMARY) && !(_Msg.m_Param0 & CONTROLBITS_PRIMARY))
			{
				CMat4Dfp32 Mat;
				GetShootDirection0(Mat);
				m_spWeapon->Deactivate(Mat,NULL,m_iObject,0);
			}
			
			int16 iMountedChar = CD.m_iMountedChar;
			if ((_Msg.m_Param0 & CONTROLBITS_BUTTON0) && !(_Msg.m_Param1 & CONTROLBITS_BUTTON0))
			{
				// Dismount from the turret
				m_pWServer->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_MOUNT),iMountedChar);
			}

			if ((_Msg.m_Param0 & CONTROLBITS_BUTTON1) && !(_Msg.m_Param1 & CONTROLBITS_BUTTON1))
			{
				// Turn on flashlight (TEMP!!!)
				if (CD.m_ItemFlags & RPG_ITEM_FLAGS_FLASHLIGHT)
					CD.m_ItemFlags = CD.m_ItemFlags & ~RPG_ITEM_FLAGS_FLASHLIGHT;
				else
					CD.m_ItemFlags = CD.m_ItemFlags | RPG_ITEM_FLAGS_FLASHLIGHT;
			}
			if ((CD.m_TurretFlags & TURRET_FLAG_BLACKHOLEENABLED) &&
				(_Msg.m_Param0 & CONTROLBITS_BUTTON2) && !(_Msg.m_Param1 & CONTROLBITS_BUTTON2))
			{
				// Activate black hole
				CStr Power("BlackHole");
				CWObject_Message Msg(OBJMSG_CHAR_FORCEDARKNESSPOWER,1);
				Msg.m_pData = Power.GetStr();
				CMat4Dfp32 Mat,MatPrev;
				MatPrev = m_pWServer->Object_GetPositionMatrix(iMountedChar);
				GetShootDirection0(Mat);
				m_pWServer->Object_SetRotation(iMountedChar,Mat);
				m_pWServer->Message_SendToObject(Msg,iMountedChar);
				m_pWServer->Object_SetRotation(iMountedChar,MatPrev);
			}
			return 1;
		}
	case OBJMSG_TURRET_GETPOSITION:
		{
			CWO_Turret_ClientData& CD = GetClientData(this);
			if (!_Msg.m_pData || _Msg.m_DataSize != sizeof(CMat4Dfp32))
				return 0;
			CMat4Dfp32* pMat = (CMat4Dfp32*)_Msg.m_pData;
			
			aint bOK = false;
			if (CD.m_iRenderAttached != 0)
			{
				CWObject_CoreData* pObj = m_pWServer->Object_GetCD(CD.m_iRenderAttached);
				if(pObj)
				{
					CMTime Time = CMTime::CreateFromTicks(m_pWServer->GetGameTick(),m_pWServer->GetGameTickTime(),0.0f);
					CWObject_Message Msg;
					CWObject_Message TimeMsg(OBJMSG_HOOK_GETTIME);
					TimeMsg.m_pData = &Time;
					TimeMsg.m_DataSize = sizeof(Time);
					CMat4Dfp32 ParentMat;
					m_pWServer->Phys_Message_SendToObject(TimeMsg, CD.m_iRenderAttached);
					Msg = CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (aint)&Time,(aint)&ParentMat,CD.m_iRenderAttachBone);
					//			fp32 Time = 0;
					//			CWObject_Message Msg(OBJMSG_HOOK_GETRENDERMATRIX, (int)&Time, (int)&MatIP);
					bOK = m_pWServer->Phys_Message_SendToObject(Msg, CD.m_iRenderAttached);
					if (bOK)
						CD.GetLocalPos().Multiply(ParentMat, *pMat);
				}
			}
			if (!bOK)
				*pMat = GetPositionMatrix();

			return 1;
		}
	case OBJMSG_TURRET_GETRELATIVEANGLES:
		{
			CVec2Dfp32* pVec = (CVec2Dfp32*)_Msg.m_pData;
			if (!pVec)
				return 0;
			CWO_Turret_ClientData& CD = GetClientData(this);
			*pVec = CVec2Dfp32((_Msg.m_VecParam0.k[2] + CD.m_MaxTurrentAngleZ) / (CD.m_MaxTurrentAngleZ * 2.0f),
				(_Msg.m_VecParam0.k[1] + CD.m_MaxTurrentAngleY) / (CD.m_MaxTurrentAngleY * 2.0f));
			
			return 1;
		}
	case OBJMSG_TURRET_GETATTACHPOSITIONS:
		{
			CVec3Dfp32* pPos = (CVec3Dfp32*)_Msg.m_pData;
			if (pPos)
			{
				CWO_Turret_ClientData& CD = GetClientData(this);
				CWObject_CoreData* pAttached = CWObject_Character::IsCharacter(CD.m_iRenderAttached,m_pWServer);
				CWO_Character_ClientData* pAttachedCD = pAttached ? CWObject_Character::GetClientData(pAttached) : NULL;;
				if (pAttachedCD && pAttachedCD->m_iMountedObject == m_iObject)
					return 0;
				CXR_AnimState AnimState;
				CXR_Skeleton* pSkel;
				CXR_SkeletonInstance* pSkelInstance;
				if(!GetEvaluatedPhysSkeleton(this, m_pWServer, pSkelInstance, pSkel, AnimState, 0.0f, NULL))
					return 0;

				if (pSkel && pSkelInstance && CD.m_iShootBone0 > 0 && 
					pSkelInstance->m_nBoneTransform > CD.m_iShootBone0)
				{
					CMat4Dfp32 Temp,Temp2;
					Temp.Unit();
					CD.m_AttachLeftHand.m_Value.SetMatrixRow(Temp,3);

					Temp.Multiply(pSkelInstance->m_pBoneTransform[CD.m_iShootBone0],Temp2);
					pPos[0] = Temp2.GetRow(3);

					CD.m_AttachRightHand.m_Value.SetMatrixRow(Temp,3);
					Temp.Multiply(pSkelInstance->m_pBoneTransform[CD.m_iShootBone0],Temp2);
					pPos[1] = Temp2.GetRow(3);
					//_pWClient->Debug_RenderVertex(pPos[0],0xffffff00,0.05f);
					//_pWClient->Debug_RenderVertex(pPos[1],0xffffff00,0.05f);
				}
				return 1;
			}
			return 0;
		}
	case OBJSYSMSG_GETCAMERA:
		{
			// Get camera
			CWO_Turret_ClientData& CD = GetClientData(this);
			CXR_AnimState Anim;
			if (!_Msg.m_pData || _Msg.m_DataSize != sizeof(CMat4Dfp32)) return 0;

			CXR_Skeleton* pSkel;
			CXR_SkeletonInstance* pSkelInstance;
			if(!GetEvaluatedPhysSkeleton(this, m_pWServer, pSkelInstance, pSkel, Anim, 0.0f, NULL))
				return 0;

			if (pSkel && pSkelInstance && pSkelInstance->m_nBoneTransform > CD.m_iCameraRotTrack)
			{
				CMat4Dfp32& Camera = *(CMat4Dfp32*)_Msg.m_pData;
				Camera = pSkelInstance->m_pBoneTransform[CD.m_iCameraRotTrack];
				CVec3Dfp32 Temp = pSkel->m_lNodes[CD.m_iCameraRotTrack].m_LocalCenter;
				Temp += CD.m_CameraOffset;
				Temp *= Camera;
				Camera.GetRow(3) = Temp;
				//Camera.RotX_x_M(-0.25f);
				//Camera.RotY_x_M(0.25f);
				return 1;
			}
			return 0;
		}
	case OBJMSG_USE:
		{
			// Make sender mount itself to us
			CStr Name = GetName();
			CWObject_Message MountMsg(OBJMSG_CHAR_MOUNT);
			MountMsg.m_pData = (void*)Name.Str();
			return m_pWServer->Message_SendToObject(MountMsg,_Msg.m_iSender);
		}
	case OBJMSG_GAME_SPAWN:
		{
			Spawn(_Msg.m_Param0 == 0);
			return 1;
		}
	case OBJSYSMSG_PRECACHEMESSAGE:
		{
			if(_Msg.m_DataSize == sizeof(CWObject_Message))
			{
				CWObject_Message *pMsg = (CWObject_Message *)_Msg.m_pData;
				if (pMsg->m_Msg == OBJMSG_CHAR_SETATTACHEXTRAMODEL)
				{
					CFStr AttachModel = (char *)_Msg.m_pData;
					CFStr ModelName = AttachModel.GetStrSep(",");
					m_pWServer->GetMapData()->GetResourceIndex_Model(ModelName);
					return 1;
				}
			}
			return CWObject_TurretParent::OnMessage(_Msg);
		}
	case OBJMSG_CHAR_SETATTACHEXTRAMODEL:
		{
			// Scripted extramodels on model1
			CWO_Turret_ClientData& CD = GetClientData(this);
			fp32 Duration = _FP32_MAX;
			// On the form, "model/object index:attachrottrack:attachpoint"
			CFStr AttachModel = (char *)_Msg.m_pData;
			CFStr ModelName = AttachModel.GetStrSep(",");
			int32 iModel = m_pWServer->GetMapData()->GetResourceIndex_Model(ModelName);
			if (iModel <= 0)
			{
				// Try it as a object instead
				int iObj = m_pWServer->Selection_GetSingleTarget(ModelName);
				CWObject_CoreData* pObj = m_pWServer->Object_GetCD(iObj);
				if (pObj)
					iModel = pObj->m_iModel[0];
			}

			CD.m_TurretEffect1.SetExtraModel(m_pWServer->GetMapData(),CMTime::CreateFromTicks(m_pWServer->GetGameTick()+2,m_pWServer->GetGameTickTime()),Duration,iModel,m_spWeapon->m_Model.m_ExtraModels.m_lEffects[0].m_iAttachPoint,m_spWeapon->m_Model.m_iAttachRotTrack,m_pWServer->GetGameTick());
			CD.m_TurretEffect1.MakeDirty();
			return 1;
		}
	case OBJMSG_IMPULSE:
		{
			CWO_Turret_ClientData& CD = GetClientData(this);
			// turn on/off light
			if (CD.m_TurretFlags & TURRET_FLAG_LIGHTSCRIPTED)
			{
				if (_Msg.m_Param0)
					CD.m_ItemFlags = CD.m_ItemFlags |RPG_ITEM_FLAGS_FLASHLIGHT;
				else
					CD.m_ItemFlags = CD.m_ItemFlags & ~RPG_ITEM_FLAGS_FLASHLIGHT;
			}
			return 1;
		}
	case OBJMSG_TURRET_GETWEAPON:
		{
			*(CRPG_Object_Item**)(_Msg.m_pData) = m_spWeapon;
			return (*(CRPG_Object_Item **)(_Msg.m_pData) != NULL);
		}
	case OBJMSG_TURRET_GETTRACKINGTIME:
		{
			return m_TrackingTime;
		}
	case OBJMSG_HOOK_GETRENDERMATRIX:
		{
			CWO_Turret_ClientData& CD = GetClientData(this);
			CMat4Dfp32& ResultMat = *(CMat4Dfp32*)_Msg.m_Param1;
			if (CD.m_iRenderAttached != 0)
			{
				GetRenderAttachPos(m_pWServer, CD, ResultMat, 0.0f);
			}
			else
			{
				ResultMat = GetPositionMatrix();
			}

			return 1;
		}

	case OBJMSG_HOOK_GETCURRENTMATRIX:
		{
			CWO_Turret_ClientData& CD = GetClientData(this);
			CMat4Dfp32& ResultMat = *(CMat4Dfp32*)_Msg.m_Param0;
			fp32 IPTime = 0.0f;
			if (CD.m_iRenderAttached != 0)
			{
				GetRenderAttachPos(m_pWServer, CD, ResultMat, IPTime);
			}
			else
			{
				ResultMat = GetPositionMatrix();
			}

			uint iNode = _Msg.m_Param1;
			if (iNode)
			{
				CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
				if(!pModel)
					return false;
				CXR_Skeleton* pSkel = pModel->GetPhysSkeleton();
				if(!pSkel)
					return false;

				CXR_AnimState Anim;
				CXR_SkeletonInstance* pSkelInstance;
				OnGetAnimState(this, m_pWServer, pSkel, 1, ResultMat, IPTime, Anim);
				pSkelInstance = CD.m_spSkelInstance;
				if (pSkel && (iNode < pSkel->m_lNodes.Len()))
				{
					CVec3Dfp32 LocalPos = pSkel->m_lNodes[iNode].m_LocalCenter;
					if (pSkelInstance)
					{
						ResultMat = pSkelInstance->m_pBoneTransform[iNode];
						ResultMat.GetRow(3) = LocalPos * ResultMat;
						return 1;
					}
				}				
			}

			return 1;
		}
	case OBJMSG_TURRET_GETTURRETFLAGS:
		{
			CWO_Turret_ClientData& CD = GetClientData(this);
			return CD.m_TurretFlags;
		}
	default:
		return CWObject_TurretParent::OnMessage(_Msg);
	}
}

aint CWObject_Turret::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	/*case OBJMSG_CHAR_UPDATEMOUNTEDLOOK:
		{
			// Update look
			CWO_Turret_ClientData& CD = GetClientData(_pObj);
			CD.m_TurrentAngleZ = M_FMod(CD.m_TurrentAngleZ + _Msg.m_VecParam0[2] + 2.0f, 1.0f);
			MACRO_ADJUSTEDANGLE(CD.m_TurrentAngleZ);
			CD.m_TurrentAngleZ = Max(Min(CD.m_TurrentAngleZ > 0.5f ? CD.m_TurrentAngleZ - 1.0f : CD.m_TurrentAngleZ,CD.m_MaxTurrentAngleZ.m_Value),-CD.m_MaxTurrentAngleZ.m_Value);
			CD.m_TurrentAngleY = Min(CD.m_MaxTurrentAngleY.m_Value, Max(-CD.m_MaxTurrentAngleY.m_Value, CD.m_TurrentAngleY + _Msg.m_VecParam0[1]));
			return 1;
		}*/
	case OBJMSG_TURRET_GETMINMAXYZ:
		{
			CWO_Turret_ClientData& CD = GetClientData(_pObj);
			if (!_Msg.m_pData || _Msg.m_DataSize < sizeof(CVec3Dfp32))
				return 0;

			CVec3Dfp32* pVec = (CVec3Dfp32*)_Msg.m_pData;
			pVec->k[0] = CD.m_AimDamper;
			pVec->k[1] = CD.m_MaxTurrentAngleY;
			pVec->k[2] = CD.m_MaxTurrentAngleZ;
			return 1;
		}
	case OBJSYSMSG_GETCAMERA:
		{
			// Get camera
			CWO_Turret_ClientData& CD = GetClientData(_pObj);
			CXR_AnimState Anim;
			if ((CD.m_TurretFlags & TURRET_FLAG_NOCAMERA) || !_Msg.m_pData || _Msg.m_DataSize != sizeof(CMat4Dfp32)) return 0;

			CXR_Skeleton* pSkel;
			CXR_SkeletonInstance* pSkelInstance;
			if(!GetEvaluatedPhysSkeleton(_pObj, _pWClient, pSkelInstance, pSkel, Anim, _pWClient->GetRenderTickFrac(), NULL))
				return 0;

			if (pSkel && pSkelInstance && pSkelInstance->m_nBoneTransform > CD.m_iCameraRotTrack)
			{
				CMat4Dfp32& Camera = *(CMat4Dfp32*)_Msg.m_pData;
				Camera = pSkelInstance->m_pBoneTransform[CD.m_iCameraRotTrack];
				CVec3Dfp32 Temp = pSkel->m_lNodes[CD.m_iCameraRotTrack].m_LocalCenter;
				Temp += CD.m_CameraOffset;
				Temp *= Camera;
				Camera.GetRow(3) = Temp;
				//Camera.RotX_x_M(-0.25f);
				//Camera.RotY_x_M(0.25f);
				return 1;
			}
			return 0;
		}
	case OBJMSG_TURRET_GETPOSITION:
		{
			CWO_Turret_ClientData& CD = GetClientData(_pObj);
			if (!_Msg.m_pData || _Msg.m_DataSize != sizeof(CMat4Dfp32))
				return 0;
			CMat4Dfp32* pMat = (CMat4Dfp32*)_Msg.m_pData;

			if (CD.m_iRenderAttached != 0)
			{
				GetRenderAttachPos(_pWClient, CD, *pMat,_pWClient->GetRenderTickFrac());
			}
			else
				*pMat = _pObj->GetPositionMatrix();

			return 1;
		}
	case OBJMSG_TURRET_GETRELATIVEANGLES:
		{
			CVec2Dfp32* pVec = (CVec2Dfp32*)_Msg.m_pData;
			if (!pVec)
				return 0;
			CWO_Turret_ClientData& CD = GetClientData(_pObj);
			*pVec = CVec2Dfp32((_Msg.m_VecParam0.k[2] + CD.m_MaxTurrentAngleZ) / (CD.m_MaxTurrentAngleZ * 2.0f),
				(_Msg.m_VecParam0.k[1] + CD.m_MaxTurrentAngleY) / (CD.m_MaxTurrentAngleY * 2.0f));
			
			return 1;
		}
	case OBJMSG_TURRET_GETATTACHPOSITIONS:
		{
			CVec3Dfp32* pPos = (CVec3Dfp32*)_Msg.m_pData;
			if (pPos)
			{
				CWO_Turret_ClientData& CD = GetClientData(_pObj);
				CWObject_CoreData* pAttached = CWObject_Character::IsCharacter(CD.m_iRenderAttached,_pWClient);
				CWO_Character_ClientData* pAttachedCD = pAttached ? CWObject_Character::GetClientData(pAttached) : NULL;
				if (pAttachedCD && pAttachedCD->m_iMountedObject == _pObj->m_iObject)
					return 0;

				CXR_AnimState AnimState;
				CXR_Skeleton* pSkel;
				CXR_SkeletonInstance* pSkelInstance;
				if(!GetEvaluatedPhysSkeleton(_pObj, _pWClient, pSkelInstance, pSkel, AnimState, 0.0f, NULL))
					return 0;

				if (pSkel && pSkelInstance && CD.m_iShootBone0 > 0 && 
					pSkelInstance->m_nBoneTransform > CD.m_iShootBone0)
				{
					CMat4Dfp32 Temp,Temp2;
					Temp.Unit();
					CD.m_AttachLeftHand.m_Value.SetMatrixRow(Temp,3);
					
					Temp.Multiply(pSkelInstance->m_pBoneTransform[CD.m_iShootBone0],Temp2);
					pPos[0] = Temp2.GetRow(3);

					CD.m_AttachRightHand.m_Value.SetMatrixRow(Temp,3);
					Temp.Multiply(pSkelInstance->m_pBoneTransform[CD.m_iShootBone0],Temp2);
					pPos[1] = Temp2.GetRow(3);
				}
				return 1;
			}
			return 0;
		}
	case OBJMSG_HOOK_GETRENDERMATRIX:
		{
			CWO_Turret_ClientData& CD = GetClientData(_pObj);
			CMat4Dfp32& ResultMat = *(CMat4Dfp32*)_Msg.m_Param1;
			if (CD.m_iRenderAttached != 0)
			{
				GetRenderAttachPos(_pWClient, CD, ResultMat, _pWClient->GetRenderTickFrac());
			}
			else
			{
				Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), ResultMat, _pWClient->GetRenderTickFrac());
			}

			return 1;
		}

	case OBJMSG_HOOK_GETCURRENTMATRIX:
		{
			CWO_Turret_ClientData& CD = GetClientData(_pObj);
			CMat4Dfp32& ResultMat = *(CMat4Dfp32*)_Msg.m_Param0;
			fp32 IPTime = _pWClient->GetRenderTickFrac();
			if (CD.m_iRenderAttached != 0)
			{
				GetRenderAttachPos(_pWClient, CD, ResultMat, IPTime);
			}
			else
			{
				Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), ResultMat, IPTime);
			}

			uint iNode = _Msg.m_Param1;
			if (iNode)
			{
				CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
				if(!pModel)
					return false;
				CXR_Skeleton* pSkel = pModel->GetPhysSkeleton();
				if(!pSkel)
					return false;

				CXR_AnimState Anim;
				CXR_SkeletonInstance* pSkelInstance;
				OnGetAnimState(_pObj, _pWClient, pSkel, 1, ResultMat, IPTime, Anim);
				pSkelInstance = CD.m_spSkelInstance;
				if (pSkel && (iNode < pSkel->m_lNodes.Len()))
				{
					CVec3Dfp32 LocalPos = pSkel->m_lNodes[iNode].m_LocalCenter;
					if (pSkelInstance)
					{
						ResultMat = pSkelInstance->m_pBoneTransform[iNode];
						ResultMat.GetRow(3) = LocalPos * ResultMat;
						return 1;
					}
				}				
			}

			return 1;
		}
	case OBJMSG_TURRET_GETTURRETFLAGS:
		{
			CWO_Turret_ClientData& CD = GetClientData(_pObj);
			return CD.m_TurretFlags;
		}
	default:
		return CWObject_TurretParent::OnClientMessage(_pObj, _pWClient,_Msg);
	}	
}

int  CWObject_Turret::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const
{
	MSCOPE(CWObject_Turret::OnCreateClientUpdate, TENTACLES);

	uint8* pD = _pData;

	int Flags = CWO_CLIENTUPDATE_EXTRADATA;
	if ((_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT) & ~TURRET_AG2_DIRTYMASK)
		Flags |= CWO_CLIENTUPDATE_AUTOVAR;

	pD += CWObject_TurretParent::OnCreateClientUpdate(_iClient, _pClObjInfo, _pOld, _pData, Flags);
	if ((pD - _pData) == 0)
		return 0;

	const CWO_Turret_ClientData& CD = GetClientData(this);

	{ // Handle AutoVars, remove flag used for ag2)
		CD.AutoVar_Pack((_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT) & ~TURRET_AG2_DIRTYMASK, pD, m_pWServer->GetMapData(), 0);
	}

	{ // Handle AnimGraph
		static const CWAG2I DummyAGI;
		const CWAG2I* pMirrorAGI = &DummyAGI;
		if (_pOld->m_lspClientObj[0])
			pMirrorAGI = GetClientData(_pOld).m_AnimGraph2.m_spMirror->GetWAG2I(0);
		int8 AGPackType = (m_pWServer->GetPlayMode() == SERVER_PLAYMODE_DEMO) ? WAG2I_CLIENTUPDATE_NORMAL : WAG2I_CLIENTUPDATE_DIRECTACCESS;
		pD += CD.m_AnimGraph2.GetAG2I()->OnCreateClientUpdate2(pD, pMirrorAGI, AGPackType);
	}

	//	int nSize = (pD - _pData);
	//	M_TRACE("OnCreateClientUpdate, %d, %d, %d\n", m_iObject, m_iClass, nSize);
	return (pD - _pData);
}

int  CWObject_Turret::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	const uint8 *pD = _pData;
	pD += CWObject_TurretParent::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);
	if (_pObj->m_iClass == 0 || (pD - _pData) == 0)
		return 0;

	CWO_Turret_ClientData& CD = GetClientData(_pObj);

	if (_pObj->m_bAutoVarDirty)
	{ // Handle AutoVars
		CD.AutoVar_Unpack(pD, _pWClient->GetMapData(), 0);
	}

	{ // Handle AnimGraph
		CWAG2I_Context AG2IContext(_pObj, _pWClient, _pWClient->GetGameTime());
		CWAG2I* pMirrorAGI = CD.m_AnimGraph2.m_spMirror->GetWAG2I(0);
		CWAG2I* pAGI = ((_pWClient->GetClientMode() & WCLIENT_MODE_MIRROR) == 0) ? CD.m_AnimGraph2.GetAG2I() : NULL;
		pD += CWAG2I::OnClientUpdate2(&AG2IContext, pMirrorAGI, pAGI, pD);
	}

	//	int nSize = (pD - _pData);
	//	M_TRACE("OnClientUpdate, %d, %d, %d\n", _pObj->m_iObject, _pObj->m_iClass, nSize);
	return (pD - _pData);
}

void CWObject_Turret::OnRefresh()
{
	//...
	CWO_Turret_ClientData& CD = GetClientData(this);
	CWAG2I_Context Context(this,m_pWServer,m_pWServer->GetGameTime());
	CD.m_AnimGraph2.GetAG2I()->Refresh(&Context);

	OnRefresh_WeaponLights(this, m_pWServer, GetPositionMatrix(), 0.0f, NULL);
	UpdateVisibilityFlag();

	// Meh..
	int32 AGDirty = 0;//TURRET_AG2_DIRTYMASK;
	m_DirtyMask |= (CD.AutoVar_RefreshDirtyMask() | AGDirty) << CWO_DIRTYMASK_USERSHIFT;
	CWObject_TurretParent::OnRefresh();
}

void CWObject_Turret::UpdateVisibilityFlag()
{
	CWO_Turret_ClientData& CD = GetClientData(this);

	if ((CD.m_MuzzleFlashTick >= m_pWServer->GetGameTick() - 3) || (CD.m_ItemFlags & RPG_ITEM_FLAGS_FLASHLIGHT))
	{
		if (!(m_ClientFlags & CWO_CLIENTFLAGS_VISIBILITY))
			ClientFlags() |= CWO_CLIENTFLAGS_VISIBILITY;
	}
	else
	{
		if (m_ClientFlags & CWO_CLIENTFLAGS_VISIBILITY)
			ClientFlags() &= ~CWO_CLIENTFLAGS_VISIBILITY;
	}
}

bool CWObject_Turret::OnRefresh_WeaponLights(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, 
											const CMat4Dfp32& _Pos, fp32 _IPTime, CMat4Dfp32* _pMat)
{
	CWO_Turret_ClientData& CD = GetClientData(_pObj);
	CXR_SceneGraphInstance* pSGI = _pWPhysState->World_GetSceneGraphInstance();
	if (!pSGI)
		return false;

	bool bRet = false;

	const bool bFlashLight = (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_FLASHLIGHT || CD.m_ItemFlags & RPG_ITEM_FLAGS_FLASHLIGHT);
	const bool bMuzzleFlash0 = CD.m_MuzzleFlashTick > _pWPhysState->GetGameTick() - 4;

	CXR_AnimState Anim;
	CXR_Skeleton* pSkel = NULL;
	CXR_SkeletonInstance* pSkelInstance = NULL;

	const int iBaseIndex = _pObj->m_iObject * 5;
	if ((!bFlashLight && !bMuzzleFlash0) || 
		(!GetEvaluatedPhysSkeleton(_pObj, _pWPhysState, pSkelInstance, pSkel, Anim, _IPTime, &_Pos)))
	{
		// Either we have no lights, or something went wrong, so time to unlink lights
		int iIndex = iBaseIndex + 0x4002;
		const int nNum = iIndex + 4;
		for(; iIndex < nNum; iIndex++)
		{
			const int iLight = pSGI->SceneGraph_Light_GetIndex(iIndex);
			if(iLight)
				pSGI->SceneGraph_Light_Unlink(iLight);
		}
	}

	// Try to update weapon lights
	{
		CMat4Dfp32 Mat;
		if(!_pMat)
			_pMat = &Mat;

		if (pSkelInstance && pSkelInstance->m_nBoneTransform > CD.m_iCameraRotTrack)
		{
			CVec3Dfp32 Temp = _Pos.GetRow(3);
			*_pMat = pSkelInstance->m_pBoneTransform[CD.m_iCameraRotTrack];
			_pMat->GetRow(3) = Temp;
		}

		//CXR_Model* pModel;
		CMTime GameTime = CMTime::CreateFromTicks(_pWPhysState->GetGameTick(), _pWPhysState->GetGameTickTime(), _IPTime);

		// Update weapon lights
		bRet |= OnRefresh_WeaponLights_Model(_pObj, CD, iBaseIndex + 0x4002, _pWPhysState, pSGI, pSkelInstance, pSkel, GameTime, Anim, _pMat, _IPTime, bMuzzleFlash0, bFlashLight);
	}

	return bRet;
}

bool CWObject_Turret::OnRefresh_WeaponLights_Model(CWObject_CoreData* _pObj, CWO_Turret_ClientData& CD, const int& _iLight, CWorld_PhysState* _pWPhysState,
												CXR_SceneGraphInstance* _pSGI, CXR_SkeletonInstance* _pSkelInstance, CXR_Skeleton* _pSkel, const CMTime& _GameTime,
												CXR_AnimState& _AnimState, CMat4Dfp32* _pMat, const fp32& _IPTime, const bool& _bMuzzleFlash, const bool& _bFlashlight)
{
	if(!_bMuzzleFlash && !_bFlashlight)
		return false;

	// Fetch correct item model
	CXR_Model* pModel = _pWPhysState->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if(pModel)
	{
		int MuzzleFlashTick = CD.m_MuzzleFlashTick;
		int32 MuzzleLightColor = CD.m_MuzzleLightColor;
		int32 MuzzleLightRange = CD.m_MuzzleLightRange;

		if(_bMuzzleFlash)
		{
			CMat4Dfp32 FlashMat = *_pMat;
			CXR_Skeleton* pSkelItem = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
			if(pSkelItem)
			{
				const CXR_SkeletonAttachPoint *pAttach = pSkelItem->GetAttachPoint(1);
				if(pAttach)
					CVec3Dfp32::GetRow(FlashMat, 3) = pAttach->m_LocalPos.GetRow(3) * (FlashMat);
			}
			else if (_pSkelInstance && _pSkelInstance->m_nBoneTransform > CD.m_iCameraRotTrack)
			{
				// Muppiti, use camerarottrack thingie...
				CVec3Dfp32 Temp = _pSkel->m_lNodes[CD.m_iCameraRotTrack].m_LocalCenter;
				Temp *= FlashMat;
				FlashMat.GetRow(3) = Temp;
			}

			// Drag flash matrix a little bit above and "inwards" to get a nice lighting.
			FlashMat.GetRow(3) += (FlashMat.GetRow(2) * 5) + (FlashMat.GetRow(1) * 5);

			fp32 Duration = fp32(int(_pWPhysState->GetGameTick()) - MuzzleFlashTick) + _IPTime - 0.5f;
			Duration = MinMT(MaxMT(Duration, 0.0f), 3.0f);
			fp32 Amp = ((3.0f - Duration) / 3.0f) * 0.01f;
			if(Amp > 0 && Duration > 0)
			{
				CXR_Light Light(FlashMat, CVec3Dfp32(((MuzzleLightColor >> 16) & 0xff),
					((MuzzleLightColor >>  8) & 0xff),
					((MuzzleLightColor >>  0) & 0xff)) * Amp,
					MuzzleLightRange, 0, CXR_LIGHTTYPE_POINT);
				Light.m_LightGUID = _iLight;
				Light.m_iLight = 0;
				Light.m_Flags = CXR_LIGHT_NOSHADOWS | CXR_LIGHT_NOSPECULAR;
				_pSGI->SceneGraph_Light_LinkDynamic(Light);
			}
			else
			{
				// Unlink light
				const int iLight = _pSGI->SceneGraph_Light_GetIndex(_iLight);
				if(iLight)
					_pSGI->SceneGraph_Light_Unlink(iLight);
			}
		}

		// Willbo:	Flashlight dual wield is not really correct. It do add two lights if both items has one, but it only uses the color and range
		//			from the "primary" item.
		if(_bFlashlight)
		{
			CXR_Skeleton* pSkelItem = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
			if(pSkelItem)
			{
				const CXR_SkeletonAttachPoint *pAttach = pSkelItem->GetAttachPoint(3);
				if(pAttach)
					CVec3Dfp32::GetRow(*_pMat, 3) = pAttach->m_LocalPos.GetRow(3) * (*_pMat);
				else
				{
					CVec3Dfp32::GetRow(*_pMat, 3) += CVec3Dfp32::GetRow(*_pMat, 0) * 15.0f + CVec3Dfp32::GetRow(*_pMat, 2) * 15.0f;
				}
			}

			CMat4Dfp32 LightPos = *_pMat;
			CVec3Dfp32::GetRow(LightPos, 3) += CVec3Dfp32::GetRow(*_pMat, 0) * -3.0f;
			CXR_Light Light(LightPos, CVec3Dfp32(((CD.m_FlashLightColor.m_Value >> 16) & 0xff) / 255.0f,
				((CD.m_FlashLightColor.m_Value >> 8) & 0xff) / 255.0f,
				((CD.m_FlashLightColor.m_Value >> 0) & 0xff) / 255.0f) * 2,
				CD.m_FlashLightRange, 0, CXR_LIGHTTYPE_SPOT);
			Light.m_SpotHeight = M_Tan(30 * (_PI / 180.0f ));
			Light.m_SpotWidth = M_Tan(30 * (_PI / 180.0f ));
			Light.m_LightGUID = _iLight + 1;
			Light.m_iLight = 0;
			uint16 ExtraFlags = CD.m_TurretFlags & TURRET_FLAG_NOLIGHTSHADOW ? CXR_LIGHT_NOSHADOWS : 0;
			Light.m_Flags |= ExtraFlags | CXR_LIGHT_NOSPECULAR;
			_pSGI->SceneGraph_Light_LinkDynamic(Light);

			return true;
		}
	}

	return false;
}

void CWObject_Turret::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_Turret::OnClientRefresh);
	CWObject_TurretParent::OnClientRefresh(_pObj, _pWClient);
	CWO_Turret_ClientData& CD = GetClientData(_pObj);
	// if this is used it disturbs the rendering for some reason....
	if(CD.m_iRenderAttached != 0)
	{
		CMat4Dfp32 Mat;
		GetRenderAttachPos(_pWClient, CD, Mat,0.0f);
		CMat4Dfp32 MatCurrent = _pObj->GetPositionMatrix();
		Mat.GetRow(3).SetRow(MatCurrent,3);
		_pWClient->Object_ForcePosition_World(_pObj->m_iObject, MatCurrent);
	}
}

/*void RegPrint(CRegistry* _pReg, CStr _Tab)
{
	CStr Muppet = CStrF("%s%s - %s\n",_Tab.Str(),_pReg->GetThisName().Str(),_pReg->GetThisValue().Str());
	M_TRACE(Muppet.Str());
	CStr Tabb = _Tab;
	Tabb += CStrF("\t");
	for (int32 i = 0; i < _pReg->GetNumChildren(); i++)
	{
		RegPrint(_pReg->GetChild(i),Tabb);
	}
}*/
//void CWObject_Turret::OnIncludeClass(CMapData* _pWData, CWorld_Server*)
void CWObject_Turret::OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer)
{
	CWObject_TurretParent::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
	//RegPrint(_pReg,CStr());

	// Grr, won't get here...
	CRegistry *pAnimGraph2Reg = _pReg->FindChild("ANIMGRAPH2");
	if(pAnimGraph2Reg)
	{
		MSCOPE(GetResourceIndex_AnimGraph, CHARACTER);
		_pMapData->GetResourceIndex_AnimGraph2(pAnimGraph2Reg->GetThisValue());
	}
	CRPG_Object::IncludeRPGClassFromKey("WEAPON", _pReg, _pMapData, _pWServer, true);
}

//void CWObject_Turret::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
void CWObject_Turret::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	// .. render stuff..
	CWO_Turret_ClientData& CD = GetClientData(_pObj);
	if (CD.m_TurretFlags & TURRET_FLAG_WAITSPAWNED)
		return;

	// Get animstate..
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	CMat4Dfp32 MatIP, ParentMat;
	if (CD.m_iRenderAttached)
	{
		GetRenderAttachPos(_pWClient, CD, MatIP,_pWClient->GetRenderTickFrac());
	}
	else
		Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

	/*CXR_AnimLayer Layers[16];
	int nLayers = 16;
	CWAG2I_Context AGIContext(_pObj, _pWClient, _pWClient->GetRenderTime());
	CD.m_AnimGraph2.GetAG2I()->GetAnimLayers(&AGIContext, Layers, nLayers, 0);*/

	CXR_Model *pMainModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if (!pMainModel)
		return;

	CXR_AnimState AnimState;
	AnimState.m_iObject = _pObj->m_iObject;
	CXR_Skeleton* pSkel = pMainModel->GetSkeleton();
	if (pSkel)
	{
		AnimState.m_pSkeletonInst = CXR_VBMHelper::Alloc_SkeletonInst(_pEngine->GetVBM(),
			pSkel->m_lNodes.Len(), pSkel->m_nUsedRotations, pSkel->m_nUsedMovements);

		OnGetAnimState(_pObj,_pWClient,pSkel,0,MatIP,IPTime,AnimState);
	}
	AnimState.m_SurfaceOcclusionMask = ~CD.m_VisibilityMask;		
	AnimState.m_SurfaceShadowOcclusionMask = ~CD.m_VisibilityMask;	

	for (int32 i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
		if (pModel)
		{
			_pEngine->Render_AddModel(pModel, MatIP, AnimState);
			// Render extramodels
			if (i == 0)
			{
				CD.m_TurretEffect0.RenderExtraModel(_pWClient->GetMapData(),_pEngine,pModel,AnimState.m_pSkeletonInst,pSkel,MatIP,_pWClient->GetRenderTime(),_pObj->m_iObject);
				CD.m_TurretEffect1.RenderExtraModel(_pWClient->GetMapData(),_pEngine,pModel,AnimState.m_pSkeletonInst,pSkel,MatIP,_pWClient->GetRenderTime(),_pObj->m_iObject);
			}
		}
	}
}

void CWObject_Turret::OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, const CMat4Dfp32& _ParentMat)
{
	MSCOPESHORT(CWObject_Turret::OnClientRenderVis);
	// Add CWO_CLIENTFLAGS_VISIBILITY flag to turrets with lights
	CWO_Turret_ClientData& CD = GetClientData(_pObj);
	CXR_SceneGraphInstance* pSGI = _pWClient->World_GetSceneGraphInstance();

	bool bMuzzle = (CD.m_MuzzleFlashTick > _pWClient->GetGameTick() - 5);
	int bFlashLight = _pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_FLASHLIGHT || CD.m_ItemFlags & RPG_ITEM_FLAGS_FLASHLIGHT;
	if(bMuzzle || bFlashLight)
	{
		fp32 IPTime = _pWClient->GetRenderTickFrac();
		CMat4Dfp32 MatIP, ParentMat;

		aint bOk = false;
		if (CD.m_iRenderAttached)
		{
			GetRenderAttachPos(_pWClient, CD, MatIP,_pWClient->GetRenderTickFrac());
		}
		else
		{
			fp64 PeriodScale = _pWClient->GetModeratedFramePeriod() * _pWClient->GetGameTicksPerSecond();
			Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime / PeriodScale);
		}

		CMat4Dfp32 Mat;
		if(OnRefresh_WeaponLights(_pObj, _pWClient, MatIP, IPTime, &Mat))
		{
			/*bool bCutSceneView = (_pObj->m_ClientFlags & (PLAYER_CLIENTFLAGS_CUTSCENE | PLAYER_CLIENTFLAGS_DIALOGUE)) != 0;
			bool bThirdPerson = (pCD->m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON | PLAYER_EXTRAFLAGS_FORCETHIRPERSON)) != 0;
			int iPass = _pEngine->GetVCDepth();
			bool bFirstPerson = bLocalPlayer && !iPass && !bThirdPerson && !bCutSceneView;

			if (!bFirstPerson)
			{
				CXR_AnimState AnimState;
				CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(pCD->m_iLightCone);
				_pEngine->Render_AddModel(pModel, Mat, AnimState);
			}*/
		}
	}
}

void CWObject_Turret::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA];
	CWO_Turret_ClientData* pCD = TDynamicCast<CWO_Turret_ClientData>(pData);

	// Allocate clientdata
	if (!pCD || pCD->m_pObj != _pObj || pCD->m_pWPhysState != _pWPhysState)
	{
		pCD = MNew(CWO_Turret_ClientData);
		if (!pCD)
			Error_static("CWObject_Turret", "Could not allocate client data!");

		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = pCD;
		pCD->Clear(_pObj, _pWPhysState);
	}
}

enum
{
	TURRET_SAVEFLAGS_RENDERATTACHED		= M_Bit(0),
	TURRET_SAVEFLAGS_MOUNTED			= M_Bit(1),
	TURRET_SAVEFLAGS_PARENT				= M_Bit(2),
};
void CWObject_Turret::OnDeltaSave(CCFile* _pFile)
{
	CWObject_TurretParent::OnDeltaSave(_pFile);
	// Save things that might have changed
	CWO_Turret_ClientData& CD = GetClientData(this);
	int8 Flags = (CD.m_iRenderAttached != 0 ? TURRET_SAVEFLAGS_RENDERATTACHED : 0) |
		(CD.m_iMountedChar != 0 ? TURRET_SAVEFLAGS_MOUNTED : 0) |
		(GetParent() ? TURRET_SAVEFLAGS_PARENT : 0);
	_pFile->WriteLE(Flags);
	_pFile->WriteLE(CD.m_TurretFlags.m_Value);
	if (Flags & TURRET_SAVEFLAGS_RENDERATTACHED)
	{
		int32 GUID = m_pWServer->Object_GetGUID(CD.m_iRenderAttached);
		_pFile->WriteLE(GUID);
	}
	if (Flags & TURRET_SAVEFLAGS_MOUNTED)
	{
		int32 GUID = m_pWServer->Object_GetGUID(CD.m_iMountedChar);
		_pFile->WriteLE(GUID);
	}
	// Check parenting
	if (Flags & TURRET_SAVEFLAGS_PARENT)
	{
		int32 ParentGUID = m_pWServer->Object_GetGUID(GetParent());
		_pFile->WriteLE(ParentGUID);
		_pFile->WriteLE(int8(m_iParentAttach));
		GetLocalPositionMatrix().Write(_pFile);
		GetPositionMatrix().Write(_pFile);
		GetLastPositionMatrix().Write(_pFile);
	}
}

void CWObject_Turret::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	CWObject_TurretParent::OnDeltaLoad(_pFile, _Flags);
	// Load things that might have changed
	CWO_Turret_ClientData& CD = GetClientData(this);
	int8 Flags;
	_pFile->ReadLE(Flags);
	uint8 TurretFlags;
	_pFile->ReadLE(TurretFlags);
	CD.m_TurretFlags = TurretFlags;
	if (Flags & TURRET_SAVEFLAGS_RENDERATTACHED)
	{
		int32 GUID;
		_pFile->ReadLE(GUID);
		CD.m_iRenderAttached = m_pWServer->Object_GetIndex(GUID);
	}
	
	if (Flags & TURRET_SAVEFLAGS_MOUNTED)
	{
		_pFile->ReadLE(m_TempGUID);
	}

	if (Flags & TURRET_SAVEFLAGS_PARENT)
	{
		int32 iObjParentGUID;
		_pFile->ReadLE(iObjParentGUID);
		int8 Temp;
		_pFile->ReadLE(Temp);
		m_iParentAttach = Temp;
		// parent away!
		m_pWServer->Object_AddChild(m_pWServer->Object_GetIndex(iObjParentGUID), m_iObject);
		InternalState_GetLocalPositionMatrix().Read(_pFile);
		InternalState_GetPositionMatrix().Read(_pFile);
		InternalState_GetLastPositionMatrix().Read(_pFile);
	}
}

void CWObject_Turret::OnFinishDeltaLoad()
{
	CWObject_TurretParent::OnFinishDeltaLoad();
	CWO_Turret_ClientData& CD = GetClientData(this);
	if (m_TempGUID)
	{
		CD.m_iMountedChar = m_pWServer->Object_GetIndex(m_TempGUID);
		m_TempGUID = 0;
	}
}

const CWObject_Turret::CWO_Turret_ClientData& CWObject_Turret::GetClientData(const CWObject_CoreData* _pObj)
{
	const CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who deleted my client data?!");
	return *safe_cast<const CWO_Turret_ClientData>(pData);
}


CWObject_Turret::CWO_Turret_ClientData& CWObject_Turret::GetClientData(CWObject_CoreData* _pObj)
{
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who deleted my client data?!");
	return *safe_cast<CWO_Turret_ClientData>(pData);
}


CWObject_Turret::CWO_Turret_ClientData::CWO_Turret_ClientData()
{
	m_pObj = NULL;
	m_pWPhysState = NULL;
	m_iTurretRotTrack = 1;
	m_VisibilityMask = 0xffff;
}

void CWObject_Turret::CWO_Turret_ClientData::Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	m_TurretType = 0;
	m_pObj = _pObj;
	m_pWPhysState = _pWPhysState;
	m_AnimGraph2.Clear();
	m_AnimGraph2.SetAG2I(m_AnimGraph2.GetAG2I());
	m_AnimGraph2.GetAG2I()->SetEvaluator(&m_AnimGraph2);
	m_MaxTurrentAngleZ = 0.25;
	m_MaxTurrentAngleY = 0.25;
	m_TurrentAngleZ = 0.0f;
	m_TurrentAngleY = 0.0f;
	m_AimDamper = 1.0f;
	m_LastSkelInstanceFrac = 0.0f;
	m_LastSkelInstanceTick = 0;
	m_CameraOffset = 0.0f;
	m_iCameraRotTrack = 0;
	m_iRenderAttachBone = -1;
	m_iRenderAttachPoint = -1;
	m_iTurretRotTrack = 1;
	m_FlashLightColor = 0xff7f7f7f;
	m_MuzzleLightColor = 0xff7f7f7f;
	m_FlashLightRange = 200;
	m_MuzzleLightRange = 200;
	

	CWAG2I_Context AG2Context(_pObj, _pWPhysState, CMTime());
	m_AnimGraph2.SetInitialProperties(&AG2Context);
}

void CWObject_Turret::CWO_Turret_ClientData::AddAimModifier(CXR_SkeletonInstance* _pSkelInst, CWorld_PhysState* _pWPhys)
{
	switch (m_TurretType)
	{
	case CWObject_Turret::TURRET_TYPE_MOUNTEDGUN:
		{
			CWObject_CoreData* pObj = _pWPhys->Object_GetCD(m_iMountedChar);
			CWO_Character_ClientData* pCD = pObj ? CWObject_Character::GetClientData(pObj) : NULL;
			if (pCD)
			{
				CQuatfp32 Rotation;
				CMat4Dfp32 Mat;
				CVec3Dfp32 Look(0.0f,pCD->m_ActionCutSceneCameraOffsetY,pCD->m_ActionCutSceneCameraOffsetX);
				Look.CreateMatrixFromAngles(0, Mat);
				Rotation.Create(Mat);

				CWObject_Character::RotateBoneAbsolute(Rotation,_pSkelInst,m_iTurretRotTrack,0);
			}
			break;
		}
	case TURRET_TYPE_FLOODLIGHT:
		{
			// Modify whole base (for now...
			CWObject_CoreData* pObj = _pWPhys->Object_GetCD(m_iMountedChar);
			CWO_Character_ClientData* pCD = pObj ? CWObject_Character::GetClientData(pObj) : NULL;
			if (pCD)
			{
				CQuatfp32 Rotation;
				CMat4Dfp32 Mat;
				CVec3Dfp32 Look(0.0f,pCD->m_ActionCutSceneCameraOffsetY,pCD->m_ActionCutSceneCameraOffsetX);
				Look.CreateMatrixFromAngles(0, Mat);
				Rotation.Create(Mat);

				CWObject_Character::RotateBoneAbsolute(Rotation,_pSkelInst,0,0);
			}
			break;
		}
	case TURRET_TYPE_HELI:
		{
			CWObject_CoreData* pObj = _pWPhys->Object_GetCD(m_iMountedChar);
			CWO_Character_ClientData* pCD = pObj ? CWObject_Character::GetClientData(pObj) : NULL;
			if (pCD)
			{
				CQuatfp32 Rotation;
				CMat4Dfp32 Mat;
				CVec3Dfp32 Look(0.0f,0.0f,pCD->m_ActionCutSceneCameraOffsetX);
				Look.CreateMatrixFromAngles(0, Mat);
				Rotation.Create(Mat);
				CWObject_Character::RotateBoneAbsolute(Rotation,_pSkelInst,8,0);
				Look = CVec3Dfp32(0.0f, pCD->m_ActionCutSceneCameraOffsetY - m_MaxTurrentAngleY * 0.5f,0.0f);
				Look.CreateMatrixFromAngles(0,Mat);
				Rotation.Create(Mat);
				CWObject_Character::RotateBoneAbsolute(Rotation,_pSkelInst,12,8);
			}
			break;
		}
	case TURRET_TYPE_OWTANK:
		{
			CWObject_CoreData* pObj = _pWPhys->Object_GetCD(m_iMountedChar);
			CWO_Character_ClientData* pCD = pObj ? CWObject_Character::GetClientData(pObj) : NULL;
			if (pCD)
			{
				CQuatfp32 Rotation;
				CMat4Dfp32 Mat;
				CVec3Dfp32 Look(0.0f,0.0f,pCD->m_ActionCutSceneCameraOffsetX);
				Look.CreateMatrixFromAngles(0, Mat);
				Rotation.Create(Mat);
				CWObject_Character::RotateBoneAbsolute(Rotation,_pSkelInst,41,0);
				Look = CVec3Dfp32(0.0f,pCD->m_ActionCutSceneCameraOffsetY,0.0f);
				Look.CreateMatrixFromAngles(0,Mat);
				Rotation.Create(Mat);
				CWObject_Character::RotateBoneAbsolute(Rotation,_pSkelInst,1,0);
				CWObject_Character::RotateBoneAbsolute(Rotation,_pSkelInst,5,0);
			}
		}
	default:
		break;
	}
}

void CWObject_Turret::CWO_Turret_ClientData::OnRefresh()
{
	//...
}

CWObject_Turret::CWO_Turret_ClientData::CTurretAnimGraph::CTurretAnimGraph()
{
	// Create animgraph
	m_spAGI = MNew(CWAG2I);
	m_spMirror = MNew1(CWAG2I_Mirror, 1);
}

void CWObject_Turret::CWO_Turret_ClientData::CTurretAnimGraph::SetInitialProperties(const CWAG2I_Context* _pContext)
{
	SetNumProperties(0, 30, 30);
	//	SetPropertyInt(TENTACLE_AG2_PROPERTY_INT_DEVOUR, TENTACLE_AG2_DEVOUR_UNDEFINED);
	SetPropertyBool(PROPERTY_BOOL_ALWAYSTRUE, true);
	SetPropertyBool(PROPERTY_BOOL_ISSERVER, _pContext->m_pWPhysState->IsServer());
}

void CWObject_Turret::CWO_Turret_ClientData::CTurretAnimGraph::UpdateImpulseState(const CWAG2I_Context* _pContext)
{

}

void CWObject_Turret::CWO_Turret_ClientData::CTurretAnimGraph::AG2_OnEnterState(const CWAG2I_Context* _pContext, CAG2TokenID _TokenID, CAG2StateIndex _iState, CAG2AnimGraphID _iAnimGraph, CAG2ActionIndex _iEnterAction)
{

}

CWObject_Turret::CAutoVar_TurretEffect::CAutoVar_TurretEffect()
{
	Clear();
}

void CWObject_Turret::CAutoVar_TurretEffect::Clear()
{
	m_ExtraModelStartTime.Reset();
	m_ExtraModelDuration = 0.0f;
	m_iExtraModel = 0;
	m_iExtraModelAttachPoint = 0;
	m_ExtraModelSeed = 0;
}

void CWObject_Turret::CAutoVar_TurretEffect::SetExtraModel(CMapData* _pMapData, CMTime _EffectTime, fp32 _Duration, uint16 _iExtraModel, uint8 _iAttach, uint8 _iRotTrack, int16 _Seed, bool _bCreateInstance)
{
	uint16 iOldModel = m_iExtraModel;
	m_ExtraModelStartTime = _EffectTime;
	m_ExtraModelDuration = _Duration;
	m_iExtraModel = _iExtraModel;
	m_iExtraModelAttachPoint = _iAttach;
	m_iExtraModelRotTrack = _iRotTrack;
	m_ExtraModelSeed = _Seed;
	if (_bCreateInstance && (iOldModel != m_iExtraModel || !m_spExtraModelInstance))
	{
		CXR_Model *pModel = _pMapData->GetResource_Model(m_iExtraModel);
		if (pModel)
		{
			m_spExtraModelInstance = pModel->CreateModelInstance();
			if (m_spExtraModelInstance != NULL)
				m_spExtraModelInstance->Create(pModel, CXR_ModelInstanceContext(NULL, NULL));
		}
		else
			m_spExtraModelInstance = NULL;
	}
}

CMTime CWObject_Turret::CAutoVar_TurretEffect::GetTime(const CMTime& _Time)
{
	if (_Time.Compare(m_ExtraModelStartTime) < 0)
		return CMTime::CreateFromSeconds(0.0f);

	return _Time - m_ExtraModelStartTime;
}

void CWObject_Turret::CAutoVar_TurretEffect::RenderExtraModel(CMapData *_pMapData, CXR_Engine* _pEngine, CXR_Model* _pBaseModel,
															   CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, const CMat4Dfp32 &_Mat, const CMTime& _Time, uint16 _iObject)
{
	if(!_pBaseModel || !_pSkelInstance || !_pSkel)
		return;

	CXR_AnimState AnimState;
	AnimState.m_iObject = _iObject;
	AnimState.m_pContext = (CXR_Skeleton*)_pBaseModel->GetParam(MODEL_PARAM_SKELETON);
	
	if(m_iExtraModel != 0)
	{
		CXR_Model *pModel = _pMapData->GetResource_Model(m_iExtraModel);
		if(pModel)
		{
			if(pModel->GetParam(CXR_MODEL_PARAM_TIMEMODE) == CXR_MODEL_TIMEMODE_CONTINUOUS)
			{
				AnimState.m_AnimTime0 = _Time;
				AnimState.m_AnimTime1.Reset();
			}
			else
			{
				AnimState.m_AnimTime0 = GetTime(_Time);
				AnimState.m_AnimTime1 = _Time;

				// If we're over duration, just return...
				if (AnimState.m_AnimTime0.GetTime() > m_ExtraModelDuration)
					return;
			}

			CMat4Dfp32 Mat;
			if (_pSkelInstance->m_nBoneTransform > m_iExtraModelRotTrack)
				Mat = _pSkelInstance->m_pBoneTransform[m_iExtraModelRotTrack];
			else
				Mat = _Mat;
			
			const CXR_SkeletonAttachPoint *pAttach = _pSkel->GetAttachPoint(m_iExtraModelAttachPoint);
			if(pAttach)
			{
				CVec3Dfp32::GetRow(Mat, 3) = pAttach->m_LocalPos.GetRow(3) * Mat;
			}
			else
			{
				CVec3Dfp32 Temp = _pSkel->m_lNodes[m_iExtraModelRotTrack].m_LocalCenter;
				Temp *= Mat;
				Mat.GetRow(3) = Temp;
			}

			AnimState.m_iObject = _iObject;
			AnimState.m_pContext = (CXR_Skeleton*)_pBaseModel->GetParam(MODEL_PARAM_SKELETON);
			AnimState.m_Anim0 = (2 << 8) | 3;
			AnimState.m_pModelInstance = m_spExtraModelInstance;
			AnimState.m_Anim0 = 0;//m_AC_RandSeed & 0x7fff;
			AnimState.m_Anim0 = m_ExtraModelSeed;
			AnimState.m_Data[3] = 0;//m_ModelFlags[i];
			_pEngine->Render_AddModel(pModel, Mat, AnimState);
		}
	}
}

void CWObject_Turret::CAutoVar_TurretEffect::CopyFrom(const CWObject_Turret::CAutoVar_TurretEffect& _From)
{
	m_ExtraModelStartTime = _From.m_ExtraModelStartTime;
	m_ExtraModelDuration = _From.m_ExtraModelDuration;
	m_iExtraModel = _From.m_iExtraModel;
	m_iExtraModelAttachPoint = _From.m_iExtraModelAttachPoint;
	m_iExtraModelRotTrack = _From.m_iExtraModelRotTrack;
	m_ExtraModelSeed = _From.m_ExtraModelSeed;
}

void CWObject_Turret::CAutoVar_TurretEffect::Pack(uint8 *&_pD, CMapData* _pMapData) const
{
	uint8 &Mask = _pD[0];
	_pD++;
	Mask = 0;
	uint8 CurBit = 1;
	if (m_iExtraModel != 0)
	{
		Mask |= CurBit;
		CurBit <<= 1;
		PTR_PUTCMTIME(_pD,m_ExtraModelStartTime);
		PTR_PUTFP32(_pD,m_ExtraModelDuration);
		PTR_PUTUINT16(_pD,m_iExtraModel);
		PTR_PUTINT16(_pD,m_ExtraModelSeed);
		PTR_PUTUINT8(_pD,m_iExtraModelAttachPoint);
		PTR_PUTUINT8(_pD,m_iExtraModelRotTrack);
	}
}

void CWObject_Turret::CAutoVar_TurretEffect::Unpack(const uint8 *&_pD, CMapData* _pMapData)
{
	const uint8 &Mask = _pD[0];
	_pD++;
	uint8 CurBit = 1;
	if (Mask & CurBit)
	{
		PTR_GETCMTIME(_pD,m_ExtraModelStartTime);
		PTR_GETFP32(_pD,m_ExtraModelDuration);
		// Unpack model
		uint16 iOldModel = m_iExtraModel;
		PTR_GETUINT16(_pD,m_iExtraModel);
		if (iOldModel != m_iExtraModel)
		{
			CXR_Model *pModel = _pMapData->GetResource_Model(m_iExtraModel);
			if (pModel)
			{
				m_spExtraModelInstance = pModel->CreateModelInstance();
				if (m_spExtraModelInstance != NULL)
					m_spExtraModelInstance->Create(pModel, CXR_ModelInstanceContext(NULL, NULL));
			}
			else
				m_spExtraModelInstance = NULL;
		}
		PTR_GETINT16(_pD,m_ExtraModelSeed);
		PTR_GETINT8(_pD, m_iExtraModelAttachPoint);
		PTR_GETUINT8(_pD,m_iExtraModelRotTrack);
	}
	else
	{
		// Clear model
		m_ExtraModelStartTime.Reset();
		m_iExtraModel = 0;
		m_iExtraModelAttachPoint = 0;
		m_iExtraModelRotTrack = 0;
		m_spExtraModelInstance = NULL;
		m_ExtraModelSeed = 0;
	}
}
