#include "PCH.h"
#include "WObj_System.h"

#include "../../../XR/XR.h"
#include "../../../XR/XREngineVar.h"
#include "../../../XR/XREngineImp.h"
#include "../../../XRModels/Model_BSP4Glass/WBSP4Glass.h"
#include "WObj_Game.h"

#include "MFloat.h"

// -------------------------------------------------------------------
//  INFO_PLAYER_START
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Info_Player_Start, CWObject, 0x0100);

void CWObject_Info_Player_Start::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Info_Player_Start_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH2('FLAG','S'): // "FLAGS"
		{
			static const char *FlagsTranslate[] =
			{
				"resetdirection", "defaultstart", "startcrouched", "clearplayer", "nightvision", "relativepos", "clearsave", "clearmissions", NULL
			};
			m_iAnim0 = KeyValue.TranslateFlags(FlagsTranslate);
			break;
		}
	case MHASH1('TEAM'): // "TEAM"
		{
			m_Data[0] = _pKey->GetThisValuei();
			break;
			
		}
	default:
		{
			if (KeyName.CompareSubStr("MSG_ONSPAWN") == 0)
			{
				uint iSlot = atoi(KeyName.Str() + 11);
				m_lMsg_OnSpawn.SetMinLen(iSlot + 1);
				m_lMsg_OnSpawn[iSlot].Parse(KeyValue, m_pWServer);
			}
			else
			{
				CWObject::OnEvalKey(_KeyHash, _pKey);
			}
		
			break;
		}
	}
}

void CWObject_Info_Player_Start::OnCreate()
{
	MAUTOSTRIP(CWObject_Info_Player_Start_OnCreate, MAUTOSTRIP_VOID);

	m_ClientFlags |= CWO_CLIENTFLAGS_NOUPDATE | CWO_CLIENTFLAGS_NOREFRESH;
	m_Data[0] = -1;
}

void CWObject_Info_Player_Start::OnSpawnWorld()
{
	for(int i = 0; i < m_lMsg_OnSpawn.Len(); i++)
		m_lMsg_OnSpawn[i].SendPrecache(m_iObject, m_pWServer);
}

#ifdef M_Profile

// -------------------------------------------------------------------
//  INFO_INTERMISSION
// -------------------------------------------------------------------

void CWObject_Info_Intermission::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_Info_Intermission_OnFinishEvalKeys, MAUTOSTRIP_VOID);

	CWObject_Info_Player_Start::OnFinishEvalKeys();
	m_ClientFlags &= ~CWO_CLIENTFLAGS_NOUPDATE;
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Info_Intermission, CWObject_Info_Player_Start, 0x0100);
#endif

#ifndef M_DISABLE_TODELETE
// -------------------------------------------------------------------
//  INFO_TELEPORT_DESTINATION
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Info_Teleport_Destination, CWObject, 0x0100);

CWObject_Info_Teleport_Destination::CWObject_Info_Teleport_Destination()
{
	MAUTOSTRIP(CWObject_Info_Teleport_Destination_ctor, MAUTOSTRIP_VOID);

	m_ClientFlags |= CWO_CLIENTFLAGS_NOREFRESH;
}

// -------------------------------------------------------------------
//  INFO_CHANGEWORLD_DESTINATION
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Info_ChangeWorld_Destination, CWObject, 0x0100);

CWObject_Info_ChangeWorld_Destination::CWObject_Info_ChangeWorld_Destination()
{
	MAUTOSTRIP(CWObject_Info_ChangeWorld_Destination_ctor, MAUTOSTRIP_VOID);
	
	m_ClientFlags |= CWO_CLIENTFLAGS_NOREFRESH;
}
#endif

// -------------------------------------------------------------------
//  MODEL
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Model, CWObject, 0x0100);

CWObject_Model::CWObject_Model()
{
	MAUTOSTRIP(CWObject_Model_ctor, MAUTOSTRIP_VOID);
}

void CWObject_Model::Model_SetPhys(int _iModel, bool _bAdd, int _ObjectFlags, int _PhysFlags, bool _bNoPhysReport)
{
	MAUTOSTRIP(CWObject_Model_Model_SetPhys, MAUTOSTRIP_VOID);

	CXR_Model* pM = m_pWServer->GetMapData()->GetResource_Model(_iModel);
	if (pM)
	{
		CXR_PhysicsModel* pPhysModel = pM->Phys_GetInterface();
		if (pPhysModel)
		{
			// Setup physics
			CWO_PhysicsState Phys = GetPhysState();

			if (!_bAdd) Phys.m_nPrim = 0;
			if (Phys.m_nPrim >= CWO_MAXPHYSPRIM)
				Error("Model_SetPhys", "Too many physics-primitives.");
			Phys.m_Prim[Phys.m_nPrim++].Create(OBJECT_PRIMTYPE_PHYSMODEL, _iModel, 0, 0);

			Phys.m_PhysFlags = _PhysFlags;
			Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_ROTATION;
			Phys.m_ObjectFlags = _ObjectFlags;
			if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
				LogFile("§cf80WARNING: Unable to set model physics state.");
		}
		else if(_bNoPhysReport)
			ConOutL("§cf80WARNING (CWObject_Model::Model_SetPhys): Model was not a physics-model.");
	}
	else
		ConOutL("§cf80WARNING (CWObject_Model::Model_SetPhys): Invalid model-index.");
}

void CWObject_Model::Model_Set(int _iPos, int _iModel, bool _bAutoSetPhysics)
{
	MAUTOSTRIP(CWObject_Model_Model_Set_int_int_bool, MAUTOSTRIP_VOID);

	iModel(_iPos) = _iModel;

	if (m_iModel[_iPos] /*&& (m_pWServer->GetMapData()->GetResourceClass(m_iModel[_iPos]) != 3)*/)
	{
		CXR_Model* pM = m_pWServer->GetMapData()->GetResource_Model(m_iModel[_iPos]);

		if (_bAutoSetPhysics && pM)
		{
			CXR_AnimState Tmp;
			pM = pM->OnResolveVariationProxy(Tmp, Tmp);

			if (pM && strcmp(pM->MRTC_ClassName(), "CXR_Model_TriangleMesh") )
			{
				CXR_PhysicsModel* pPhysModel = pM->Phys_GetInterface();
				if (pPhysModel)
					Model_SetPhys(m_iModel[_iPos]);
			}
		}

/*		if (pM)
		{
			// Update vis box.
			CBox3Dfp32 Box, CurrentBox;
			pM->GetBound_Box(Box);
			GetVisBoundBox(CurrentBox);
			Box.Expand(CurrentBox);
			m_pWServer->Object_SetVisBox(m_iObject, Box.m_Min, Box.m_Max);

			if (pM->GetParam(CXR_MODEL_PARAM_ISSHADOWCASTER))
				ClientFlags() |= CWO_CLIENTFLAGS_SHADOWCASTER;
		}*/
	}
}

void CWObject_Model::Model_Set(int _iPos, const char* _pName, bool _bAutoSetPhysics)
{
	MAUTOSTRIP(CWObject_Model_Model_Set_int_char_bool, MAUTOSTRIP_VOID);

	Model_Set(_iPos, m_pWServer->GetMapData()->GetResourceIndex_Model(_pName), _bAutoSetPhysics);
}

void CWObject_Model::Sound(int _iSound)
{ 
	if (_iSound != 0) 
		m_pWServer->Sound_At(GetPosition(), _iSound, WCLIENT_ATTENUATION_3D); 
}

void CWObject_Model::SoundLRP(int _iSound, CVec3Dfp32 _P1, CVec3Dfp32 _P2)
{
	if (_iSound != 0) 
		m_pWServer->Sound_At(_P1, _iSound, WCLIENT_ATTENUATION_LRP, 0, 1.0f, -1, _P2 - _P1); 
}



void CWObject_Model::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server*)
{
	MAUTOSTRIP(CWObject_Model_OnIncludeTemplate, MAUTOSTRIP_VOID);

	IncludeModelFromKey("MODEL", _pReg, _pMapData);
	IncludeModelFromKey("MODEL0", _pReg, _pMapData);
	IncludeModelFromKey("MODEL1", _pReg, _pMapData);
	IncludeModelFromKey("MODEL2", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND0", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND1", _pReg, _pMapData);
}

void CWObject_Model::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Model_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
		{
			CStr ModelName = CStrF("$WORLD:%d", _Value.Val_int());

	/*		int iModel = 0;
			int XRMode = m_pWServer->Registry_GetGame()->GetValuei("XR_MODE", 0, 0);
			if (XRMode == 1)
				iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel2(ModelName);
			else if (XRMode == 2)
				iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel3(ModelName);
			else
				iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
	*/
			int iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
			if (m_iModel[0])
				Model_SetPhys(iModel, false, 
					OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT,
					OBJECT_PHYSFLAGS_PUSHER | OBJECT_PHYSFLAGS_ROTATION);
			else
				Model_Set(0, iModel);
			break;
		}
	case MHASH3('PHYS','MODE','L'): // "PHYSMODEL"
		{
	/*
			int iModel = 0;
			int XRMode = m_pWServer->Registry_GetGame()->GetValuei("XR_MODE", 0, 0);
			if (XRMode == 1)
				iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel2(_Value);
			else if (XRMode == 2)
				iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel3(_Value);
			else
				iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(_Value);
	*/
			int iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(_Value);
			Model_SetPhys(iModel);
			break;
		}
	case MHASH2('MODE','L'): // "MODEL"
	case MHASH2('MODE','L0'): // "MODEL0"
		{
			Model_Set(0, _Value);
			break;
		}
	case MHASH2('MODE','L1'): // "MODEL1"
		{
			Model_Set(1, _Value);
			break;
		}
	case MHASH2('MODE','L2'): // "MODEL2"
		{
			Model_Set(2, _Value);
			break;
		}
	case MHASH2('SOUN','D'): // "SOUND"
	case MHASH2('SOUN','D0'): // "SOUND0"
		{
			m_iSound[0] = m_pWServer->GetMapData()->GetResourceIndex_Sound(_Value);
			break;
		}
	case MHASH2('SOUN','D1'): // "SOUND1"
		{
			m_iSound[1] = m_pWServer->GetMapData()->GetResourceIndex_Sound(_Value);
			break;
		}
	default:
		{
			CWObject::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Model::OnFinishEvalKeys()
{
	CWObject::OnFinishEvalKeys();
	
	UpdateVisibility();
}

aint CWObject_Model::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Model_OnMessage, 0);

	switch (_Msg.m_Msg)
	{
		case OBJMSG_MODEL_SETFLAGS:
			Data(3) = ~(_Msg.m_Param0);
			return 1;

		case OBJMSG_HIDEMODEL:
			{
				if (_Msg.m_Param0)
					ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;
				else
					ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
			}
			return 1;
	}
	
	return CWObject::OnMessage(_Msg);
}

void CWObject_Model::OnRefresh()
{
	MAUTOSTRIP(CWObject_Model_OnRefresh, MAUTOSTRIP_VOID);
}

void CWObject_Model::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	MAUTOSTRIP(CWObject_Model_OnDeltaLoad, MAUTOSTRIP_VOID);

	CWObject::OnDeltaLoad(_pFile, _Flags);
	int8 Flags;
	_pFile->ReadLE(Flags);

	if(Flags & 1)
	{
		for(int iData = 0; iData<CWO_NUMDATA; iData++)
		{
			int32 Temp;
			_pFile->ReadLE(Temp);
			Data(iData) = Temp;
		}
	}
	else
	{
		for(int iData = 0; iData<CWO_NUMDATA; iData++)
			Data(iData) = 0;
	}
	if(Flags & 2)
	{
		for(int i = 0; i< CWO_NUMMODELINDICES; i++)
		{
			int16 Temp;
			_pFile->ReadLE(Temp);
			iModel(i) = Temp;
		}
	}
	else
		for(int i = 0; i< CWO_NUMMODELINDICES; i++)
			iModel(i) = 0;

	if(Flags & 4)
	{
		for(int i = 0; i< CWO_NUMSOUNDINDICES; i++)
		{
			int16 Temp;
			_pFile->ReadLE(Temp);
			iSound(i) = Temp;
		}
	}
	else
		for(int i = 0; i< CWO_NUMSOUNDINDICES; i++)
			iSound(i) = 0;

	UpdateVisibility();
}

void CWObject_Model::OnDeltaSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Model_OnDeltaSave, MAUTOSTRIP_VOID);

	CWObject::OnDeltaSave(_pFile);
	int i;
	int8 Flags = 0;
	for(i = 0; i < CWO_NUMDATA; i++)
		if(m_Data[i] != 0)
			Flags |= 1;

	for(i = 0; i < CWO_NUMMODELINDICES; i++)
		if(m_iModel[i] != 0)
			Flags |= 2;

	for(i = 0; i < CWO_NUMSOUNDINDICES; i++)
		if(m_iSound[i] != 0)
			Flags |= 4;

	_pFile->WriteLE(Flags);
	if(Flags & 1)
	{
		for(int iData=0; iData<CWO_NUMDATA; iData++)
			_pFile->WriteLE(m_Data[iData]);
	}
	if(Flags & 2) 
	{
		for(int i = 0; i< CWO_NUMMODELINDICES; i++)
			_pFile->WriteLE(m_iModel[i]);
	}
	if(Flags & 4)
	{
		for(int i = 0; i< CWO_NUMSOUNDINDICES; i++)
			_pFile->WriteLE(m_iSound[i]);
	}
}

void CWObject_Model::RefreshStandardModelInstances(CWObject_Client* _pObj, CWorld_Client* _pWClient, int _ModelFlags)
{
	MAUTOSTRIP(CWObject_Model_RefreshStandardModelInstances, MAUTOSTRIP_VOID);

//	int AnimTick = int(_pWClient->GetGameTick() - _pObj->m_CreationGameTick);
	for(int i = 0; i < CWO_NUMCLIENTOBJ; i++)
	{
		if(_pObj->m_lModelInstances[i] != NULL)
		{
			CXR_ModelInstanceContext Context(_pObj->GetAnimTick(_pWClient), _pWClient->GetGameTickTime(), _pObj, _pWClient);
			_pObj->m_lModelInstances[i]->OnRefresh(Context, &_pObj->GetPositionMatrix(), 1, _ModelFlags);
		}
	}
}

void CWObject_Model::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Model_OnClientRefresh, MAUTOSTRIP_VOID);
	MSCOPESHORT( CWObject_Model::OnClientRefresh );

	int ModelFlags = ~(_pObj->m_Data[3]);
	if (ModelFlags == 0)
		ModelFlags = 0;
	RefreshStandardModelInstances(_pObj, _pWClient, ModelFlags);
}

M_FORCEINLINE void M_VMatLrp(const CMat4Dfp32& _Pos, const CMat4Dfp32& _Pos2, CMat4Dfp32& _Dest, fp32& _Time)		// fp32& because we want the option to avoid LHS.
{
	vec128 p0x = _Pos.r[0];
	vec128 p0y = _Pos.r[1];
	vec128 p0z = _Pos.r[2];
	vec128 p0w = _Pos.r[3];
	vec128 p1x = _Pos2.r[0];
	vec128 p1y = _Pos2.r[1];
	vec128 p1z = _Pos2.r[2];
	vec128 p1w = _Pos2.r[3];
	vec128 t = M_VLdScalar(_Time);
	vec128 z = M_VZero();

	vec128 dstw = M_VLrp(p0w, p1w, t);
	vec128 dstx = M_VLrp(p0x, p1x, t);
	vec128 dstytmp = M_VLrp(p0y, p1y, t);
	vec128 dstz = M_VXpd(dstytmp, dstx);
	vec128 dsty = M_VXpd(dstz, dstx);
	M_VNrm3x4(dstx, dsty, dstz, z);

	_Dest.r[0] = dstx;
	_Dest.r[1] = dsty;
	_Dest.r[2] = dstz;
	_Dest.r[3] = dstw;

/*	if (CVec3Dfp32::GetMatrixRow(_Pos, 3).DistanceSqr(CVec3Dfp32::GetMatrixRow(_Pos2, 3)) > Sqr(256.0f))
	{
		_Dest = _Pos;
	}
	else if(!memcmp(&_Pos, &_Pos2, sizeof(_Pos)))
	{
		_Dest = _Pos;
	}
	else
	{
		fp32 tpos = ClampRange(t, 2.5f);
		t = ClampRange(t, 1.5f);
		_Dest.UnitNot3x3();
		CVec3Dfp32 v3 = CVec3Dfp32::GetMatrixRow(_Pos2, 3) - CVec3Dfp32::GetMatrixRow(_Pos, 3);
		CVec3Dfp32::GetMatrixRow(_Dest, 3) = CVec3Dfp32::GetMatrixRow(_Pos, 3) + v3*tpos;

		CVec3Dfp32 v0 = CVec3Dfp32::GetMatrixRow(_Pos2, 0) - CVec3Dfp32::GetMatrixRow(_Pos, 0);
		CVec3Dfp32::GetMatrixRow(_Dest, 0) = (CVec3Dfp32::GetMatrixRow(_Pos, 0) + v0*t).Normalize();
		CVec3Dfp32 v1 = CVec3Dfp32::GetMatrixRow(_Pos2, 1) - CVec3Dfp32::GetMatrixRow(_Pos, 1);
		CVec3Dfp32::GetMatrixRow(_Dest, 2) = -((CVec3Dfp32::GetMatrixRow(_Pos, 1) + v1*t) / CVec3Dfp32::GetMatrixRow(_Dest, 0)).Normalize();
		CVec3Dfp32::GetMatrixRow(_Dest, 1) = CVec3Dfp32::GetMatrixRow(_Dest, 2) / CVec3Dfp32::GetMatrixRow(_Dest, 0);
	}
	*/
}

void CWObject_Model::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Model_OnClientRender, MAUTOSTRIP_VOID);
	MSCOPESHORT( CWObject_Model::OnClientRender );

	aint bAnything = 0;
	CXR_Model* lpModels[CWO_NUMMODELINDICES];
	for (uint i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
		lpModels[i] = pModel;
		bAnything |= aint(pModel);
	}

	if (bAnything)
	{
		CMat4Dfp32 MatIP;
		fp32 IPTime = _pWClient->GetRenderTickFrac();
		Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

		for (uint i = 0; i < CWO_NUMMODELINDICES; i++)
		{
			if (lpModels[i])
			{
				CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient, i);
				AnimState.m_Data[3] = ~(_pObj->m_Data[3]);
				_pEngine->Render_AddModel(lpModels[i], MatIP, AnimState);
			}
		}
	}
}

// -------------------------------------------------------------------
//  ANIMMODEL
// -------------------------------------------------------------------
//#ifdef M_Profile
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_AnimModel, CWObject_Model, 0x0100);

CWObject_AnimModel::CWObject_AnimModel()
{
	MAUTOSTRIP(CWObject_AnimModel_ctor, MAUTOSTRIP_VOID);

	m_iAnim2 = -1;
}

void CWObject_AnimModel::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_AnimModel_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH1('ANIM'): // "ANIM"
		{
			m_iAnim2 = m_pWServer->GetMapData()->GetResourceIndex_Anim(_Value);
			break;
		}
	case MHASH1('ZROT'): // "ZROT"
		{
			m_Data[0] = int32((fp32)_pKey->GetThisValuef() * 1024.0f);
			break;
		}

	default:
		{
			CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

#define XR_WALLMARK_FADECOLOR 2
//CXR_WallmarkDesc CreateWallMarkDesc(int _Flags, fp32 _Size, const char* _pSurfName, int _TextureID = 0);

void CWObject_AnimModel::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_AnimModel_OnClientRender, MAUTOSTRIP_VOID);

	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

	CVec3Dfp32 v = CVec3Dfp32::GetMatrixRow(MatIP, 0);
	fp32 Angle = CVec3Dfp32::AngleFromVector(v.k[0], v.k[1]);

	CMat4Dfp32 Mat;
	Mat.Unit();
	CMTime Time = CMTime::CreateFromTicks(_pObj->GetAnimTick(_pWClient), _pWClient->GetGameTickTime(), _pWClient->GetRenderTickFrac() - _pObj->m_CreationGameTickFraction);
	Mat.SetZRotation3x3(Angle + Time.GetTimeModulusScaled((fp32(_pObj->m_Data[0]) / 1024), _PI2));
	CVec3Dfp32::GetMatrixRow(MatIP, 3).SetMatrixRow(Mat, 3);

	CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if(!pModel)
		return;

	CXR_AnimState Anim = _pObj->GetDefaultAnimState(_pWClient);

	if (!_pObj->m_lspClientObj[0])
	{
		MRTC_SAFECREATEOBJECT_NOEX(spSkel, "CXR_SkeletonInstance", CXR_SkeletonInstance);
		_pObj->m_lspClientObj[0] = spSkel;
		if (!_pObj->m_lspClientObj[0]) return;
	}
	CXR_SkeletonInstance* pSkelInstance = (CXR_SkeletonInstance*)(CReferenceCount*)_pObj->m_lspClientObj[0];
	CXR_Skeleton* pSkel = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
	if(pSkel)
		pSkelInstance->Create(pSkel->m_lNodes.Len());

//	_pWClient->Debug_RenderMatrix(Mat, 0.05f, false);
	
	CXR_Anim_Base *pAnim = _pWClient->GetMapData()->GetResource_Anim(_pObj->m_iAnim2);
	if(pSkel && pAnim)
	{
		// Eval animation
		CXR_AnimLayer Layer;
		spCXR_Anim_SequenceData spSeq = pAnim->GetSequence(Anim.m_Anim0);
		Layer.Create3(spSeq, spSeq->GetLoopedTime(Anim.m_AnimTime0), 1.0f, 1.0f, 0);

		// TODO: EvalAnim should return the evaulated models position?
		// Now something completely different is return, and models are incorrectly culled - JA
		CMat4Dfp32 Src = Mat;
		Anim.m_pSkeletonInst = pSkelInstance;
		pSkel->EvalAnim(&Layer, 1, Anim.m_pSkeletonInst, Src);
	}
	
	_pEngine->Render_AddModel(pModel, Mat, Anim);

#if 0
	if(_pEngine->GetVCDepth() == 0)
	{
		CMat4Dfp32 WallMarkPos;
		WallMarkPos.Unit();
		CVec3Dfp32::GetRow(WallMarkPos, 2) = CVec3Dfp32(0.20f, 0.20f, 1.0f);
		CVec3Dfp32::GetRow(WallMarkPos, 0) = CVec3Dfp32(1, 0, 0);
		WallMarkPos.RecreateMatrix(2, 0);
		CVec3Dfp32::GetRow(WallMarkPos, 3) = _pObj->GetPosition();

		CTextureContainer_ShadowDecals* pTC = safe_cast<CTextureContainer_ShadowDecals>(_pEngine->GetInterface(XR_ENGINE_TCSHADOWDECALS));
		if (pTC)
		{
			fp32 WMSize = 96.0f;
//			WMSize = 330;
			CMat4Dfp32 Camera = WallMarkPos;

			CVec3Dfp32::GetRow(Camera, 2) = -CVec3Dfp32::GetRow(Camera, 2);
			CVec3Dfp32::GetRow(Camera, 1) = -CVec3Dfp32::GetRow(Camera, 1);

			int hShadow = pTC->Shadow_Begin(Camera, WMSize, _pObj->m_iObject, 2);
			if (hShadow)
			{
				pTC->Shadow_AddModel(hShadow, pModel, Mat, Anim);
				pTC->Shadow_End(hShadow);
				int TextureID = pTC->Shadow_GetTextureID(hShadow);

				CreateTempWallmark(_pWClient, CreateWallMarkDesc(XR_WALLMARK_FADECOLOR, WMSize, "S_SHADOWDECAL", TextureID), WallMarkPos, WMSize);
			}
		}
	}
#endif
}
//#endif

CWObject_Sound::CWObject_Sound()
{
}


void CWObject_Sound::OnCreate()
{
	for(int i = 0; i < MAXRANDOMSOUNDS; i++)
	{
		m_RandomSound_iSound[i] = 0;
		m_RandomSound_MaxTick[i] = int(60 * m_pWServer->GetGameTicksPerSecond());
		m_RandomSound_MinTick[i] = int(30 * m_pWServer->GetGameTicksPerSecond());
	}
	m_bWaitSpawn = false;
	m_iRealSound[0] = 0;
	m_iRealSound[1] = 0;
}

void CWObject_Sound::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Sound_OnEvalKey, MAUTOSTRIP_VOID);

	fp32 TicksPerSecond = m_pWServer->GetGameTicksPerSecond();
	const fp32 Valuef = (fp32)_pKey->GetThisValuef();

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH5('RAND','OMSO','UND0','_SOU','ND'): // "RANDOMSOUND0_SOUND"
		{
			m_RandomSound_iSound[0] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	
	case MHASH5('RAND','OMSO','UND0','_MAX','TIME'): // "RANDOMSOUND0_MAXTIME"
		{
	 		m_RandomSound_MaxTick[0] = int(Valuef * TicksPerSecond);
			break;
		}
	
	case MHASH5('RAND','OMSO','UND0','_MIN','TIME'): // "RANDOMSOUND0_MINTIME"
		{
			m_RandomSound_MinTick[0] = int(Valuef * TicksPerSecond);
			break;
		}
	
	case MHASH5('RAND','OMSO','UND1','_SOU','ND'): // "RANDOMSOUND1_SOUND"
		{
			m_RandomSound_iSound[1] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	
	case MHASH5('RAND','OMSO','UND1','_MAX','TIME'): // "RANDOMSOUND1_MAXTIME"
		{
			m_RandomSound_MaxTick[1] = int(Valuef * TicksPerSecond);
			break;
		}
	
	case MHASH5('RAND','OMSO','UND1','_MIN','TIME'): // "RANDOMSOUND1_MINTIME"
		{
			m_RandomSound_MinTick[1] = int(Valuef * TicksPerSecond);
			break;
		}
	
	case MHASH5('RAND','OMSO','UND2','_SOU','ND'): // "RANDOMSOUND2_SOUND"
		{
			m_RandomSound_iSound[2] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	
	case MHASH5('RAND','OMSO','UND2','_MAX','TIME'): // "RANDOMSOUND2_MAXTIME"
		{
			m_RandomSound_MaxTick[2] = int(Valuef * TicksPerSecond);
			break;
		}
	
	case MHASH5('RAND','OMSO','UND2','_MIN','TIME'): // "RANDOMSOUND2_MINTIME"
		{
			m_RandomSound_MinTick[2] = int(Valuef * TicksPerSecond);
			break;
		}

	case MHASH2('SOUN','D'): // "SOUND"
	case MHASH2('SOUN','D0'): // "SOUND0"
		{
			m_iRealSound[0] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	
	case MHASH2('SOUN','D1'): // "SOUND1"
		{
			m_iRealSound[1] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	case MHASH3('WAIT','SPAW','N'): // "WAITSPAWN"
		{
			m_bWaitSpawn = _pKey->GetThisValuei() != 0;
			break;
		}

	default:
		{
			CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Sound::Spawn()
{
	MAUTOSTRIP(CWObject_Sound_Spawn, MAUTOSTRIP_VOID);

	m_bWaitSpawn = false;
	bool bRandom = false;
	for(int i = 0; i < MAXRANDOMSOUNDS; i++)
	{
		if(m_RandomSound_iSound[i] != 0)
		{
			//Some idiot-proof checks
			if(m_RandomSound_MinTick[i] <= 0)
				m_RandomSound_MinTick[i] = 1;
			if(m_RandomSound_MaxTick[i] < m_RandomSound_MinTick[i])
				m_RandomSound_MaxTick[i] = m_RandomSound_MinTick[i];
			
			//Calculate first tick that the sound should be played on. Add one for safety.
			m_RandomSound_NextTick[i] = int(m_pWServer->GetGameTick() + Random * m_RandomSound_MaxTick[i] + 1);
			bRandom = true;
		}
	}

	m_iSound[0] = m_iRealSound[0];
	m_iSound[1] = m_iRealSound[1];
	
	if(bRandom)
	{
		ClientFlags() &= ~(CWO_CLIENTFLAGS_INVISIBLE | CWO_CLIENTFLAGS_NOREFRESH);
		m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);
	}
	else if(m_iSound[0] != 0 || m_iSound[1] != 0)
	{		
		ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
		m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);
	}
}

void CWObject_Sound::Unspawn()
{
	m_bWaitSpawn = true;

	// WHYYYY? (if the sound is waitspawned this will remove them....)
	//m_iRealSound[0] = m_iSound[0];
	//m_iRealSound[1] = m_iSound[1];

	iSound(0) = 0;
	iSound(1) = 0;

	ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE | CWO_CLIENTFLAGS_NOREFRESH;
}

void CWObject_Sound::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_Sound_OnFinishEvalKeys, MAUTOSTRIP_VOID);

	if(!m_bWaitSpawn)
		Spawn();
	else
		Unspawn();
}

void CWObject_Sound::OnRefresh()
{
	MAUTOSTRIP(CWObject_Sound_OnRefresh, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Sound::OnRefresh);

	for(int i = 0; i < MAXRANDOMSOUNDS; i++)
		if(m_RandomSound_iSound[i] != 0)
		{
			if(m_pWServer->GetGameTick() >= m_RandomSound_NextTick[i])
			{
				m_pWServer->Sound_At(GetPosition(), m_RandomSound_iSound[i], 0);
				int Delta = m_RandomSound_MaxTick[i] - m_RandomSound_MinTick[i];
				m_RandomSound_NextTick[i] = int(m_pWServer->GetGameTick() + m_RandomSound_MinTick[i] + Random * Delta);
			}
		}
}

aint CWObject_Sound::OnMessage(const CWObject_Message &_Msg)
{
	MAUTOSTRIP(CWObject_Sound_OnMessage, 0);

	switch(_Msg.m_Msg)
	{

	case OBJMSG_GAME_SPAWN:
		if (m_bWaitSpawn && (_Msg.m_Param0 == 0))
		{
			Spawn();
			return 1;
		}
		else if (!m_bWaitSpawn && (_Msg.m_Param0 > 0))
		{
			Unspawn();
			return 1;
		}
		return 0;

#ifndef M_RTM
	case OBJSYSMSG_GETDEBUGSTRING:
		if(m_bWaitSpawn && _Msg.m_DataSize == sizeof(CStr *))
		{
			CStr *pSt = (CStr *)_Msg.m_pData;
			CWObject_Model::OnMessage(_Msg);
			*pSt += "Waiting for spawn";
			return 1;
		}
#endif
	}
	return CWObject_Model::OnMessage(_Msg);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Sound, CWObject_Model, 0x0100);

// -------------------------------------------------------------------
//  STATIC
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Static, CWObject_Model, 0x0100);

CWObject_Static::CWObject_Static()
{
	MAUTOSTRIP(CWObject_Static_ctor, MAUTOSTRIP_VOID);

	m_iBSPModelIndex = 0;
	m_iPhysModel = 0;
	m_iRealModel[0] = 0;
	m_iRealModel[1] = 0;
	m_iRealModel[2] = 0;
	m_iRealSound[0] = 0;
	m_iRealSound[1] = 0;
	m_bWaitSpawn = false;

	ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
}

void CWObject_Static::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Static_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH3('WAIT','SPAW','N'): // "WAITSPAWN"
		{
			m_bWaitSpawn = true;
			break;
		}

	case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
		{
			CStr ModelName = CStrF("$WORLD:%d", _pKey->GetThisValuei());
	/*
			int XRMode = m_pWServer->Registry_GetGame()->GetValuei("XR_MODE", 0, 0);
		
			if (XRMode == 1)
				m_iBSPModelIndex = m_pWServer->GetMapData()->GetResourceIndex_BSPModel2(ModelName);
			else if (XRMode == 2)
				m_iBSPModelIndex = m_pWServer->GetMapData()->GetResourceIndex_BSPModel3(ModelName);
			else
				m_iBSPModelIndex = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
	*/
			m_iBSPModelIndex = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
			break;
		}

	case MHASH3('PHYS','MODE','L'): // "PHYSMODEL"
		{
	/*		int XRMode = m_pWServer->Registry_GetGame()->GetValuei("XR_MODE", 0, 0);

			if( XRMode == 1 )
				m_iPhysModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel2(KeyValue);
			else if( XRMode == 2 )
				m_iPhysModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel3(KeyValue);
			else
				m_iPhysModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(KeyValue);
	*/
			m_iPhysModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(KeyValue);
			break;
		}
	case MHASH2('MODE','L'): // "MODEL"
	case MHASH2('MODE','L0'): // "MODEL0"
		{
			m_iRealModel[0] = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
			break;
		}

	case MHASH2('MODE','L1'): // "MODEL1"
		{
			m_iRealModel[1] = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
			break;
		}

	case MHASH2('MODE','L2'): // "MODEL2"
		{
			m_iRealModel[2] = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
			break;
		}

	case MHASH2('SOUN','D'): // "SOUND"
	case MHASH2('SOUN','D0'): // "SOUND0"
		{
			m_iRealSound[0] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	case MHASH2('SOUN','D1'): // "SOUND1"
		{
			m_iRealSound[1] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	case MHASH3('LIGH','TING','MODE'): // "LIGHTINGMODE"
		{
			iAnim1() = _pKey->GetThisValuei();
			break;
		}
	
	default:
		{
			CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Static::Spawn()
{
	MAUTOSTRIP(CWObject_Static_Spawn, MAUTOSTRIP_VOID);

	m_bWaitSpawn = false;

	if(m_iBSPModelIndex)
	{
		if (m_iModel[0])
			Model_SetPhys(m_iBSPModelIndex, false, 
				OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_NAVIGATION,
				OBJECT_PHYSFLAGS_PUSHER | OBJECT_PHYSFLAGS_ROTATION);
		else
			Model_Set(0, m_iBSPModelIndex);
	}
	if(m_iPhysModel)
		Model_SetPhys(m_iPhysModel);
	int i;
	for(i = 0; i < 3; i++)
		if(m_iRealModel[i])
			Model_Set(i, m_iRealModel[i]);
	for(i = 0; i < 2; i++)
		if(m_iRealSound[i])
			iSound(m_iRealSound[i]);

	ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;

	m_pWServer->Phys_InsertPosition(m_iObject, this);
	m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);

	UpdateVisibility();
}

void CWObject_Static::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_Static_OnFinishEvalKeys, MAUTOSTRIP_VOID);

	if(!m_bWaitSpawn)
		Spawn();
	else
		ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;
}

aint CWObject_Static::OnMessage(const CWObject_Message &_Msg)
{
	MAUTOSTRIP(CWObject_Static_OnMessage, 0);

	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAME_SPAWN:
		if(m_bWaitSpawn)
		{
			Spawn();
			return 1;
		}
		return 0;
#ifndef M_RTM
		case OBJSYSMSG_GETDEBUGSTRING:
			if(m_bWaitSpawn && _Msg.m_DataSize == sizeof(CStr *))
			{
				CStr *pSt = (CStr *)_Msg.m_pData;
				CWObject_Model::OnMessage(_Msg);
				*pSt += "Waiting for spawn";
				return 1;
			}
#endif
	}
	return CWObject_Model::OnMessage(_Msg);
}

void CWObject_Static::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	uint8 bWaitSpawn;
	_pFile->ReadLE(bWaitSpawn);
	if(!bWaitSpawn && m_bWaitSpawn)
		Spawn();
}

void CWObject_Static::OnDeltaSave(CCFile* _pFile)
{
	uint8 bWaitSpawn = m_bWaitSpawn ? 1 : 0;
	_pFile->WriteLE(bWaitSpawn);
}

void CWObject_Static::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Static_OnClientRender, MAUTOSTRIP_VOID);

	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

//	const CMat4Dfp32& MatIP = _pObj->GetPositionMatrix();
	
	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		if(_pObj->m_iModel[i] > 0)
		{
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
			if(pModel)
			{
				CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
				AnimState.m_Data[3] = ~(_pObj->m_Data[3]);

				switch(_pObj->m_iAnim1)
				{
				case 1: AnimState.m_AnimAttr0 = 0; break;
				case 2: AnimState.m_AnimAttr0 = 1; break;
				}

				_pEngine->Render_AddModel(pModel, MatIP, AnimState);
			}
		}
	}
}


// -------------------------------------------------------------------
//  EXT_STATIC
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Ext_Static, CWObject_Static, 0x0100);

CWO_Ext_Static_ClientData::CWO_Ext_Static_ClientData()
	: m_pObj(NULL)
{
	m_SurfaceResourceID = 0;
	m_pObj = NULL;
}

void CWO_Ext_Static_ClientData::Clear(CWObject_CoreData* _pObj)
{
	m_pObj = _pObj;
	m_SurfaceResourceID = 0;
}


CWObject_Ext_Static::CWObject_Ext_Static()
	: CWObject_Static()
{
	MAUTOSTRIP(CWObject_Ext_Static_ctor, MAUTOSTRIP_VOID);

	ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
}

void CWObject_Ext_Static::OnIncludeClass(CMapData* _pWData, CWorld_Server* _pWServer)
{
	CWObject_Static::OnIncludeClass(_pWData, _pWServer);

	_pWData->GetResourceIndex_Class("STATIC");
}

void CWObject_Ext_Static::OnCreate()
{
	CWObject_Static::OnCreate();
}

void CWObject_Ext_Static::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Ext_Static_OnEvalKey, MAUTOSTRIP_VOID);

	CMapData* pMapData = m_pWServer->GetMapData();

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	CWO_Ext_Static_ClientData& CD = GetClientData(this);

	switch (_KeyHash)
	{
	case MHASH7('EXT_','STAT','IC_D','EFAU','LT_S','URFA','CE'): // "EXT_STATIC_DEFAULT_SURFACE"
		{
			CD.m_SurfaceResourceID = pMapData->GetResourceIndex_Surface(KeyValue);
			m_DirtyMask |= CD.AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
			break;
		}

	default:
		{
			CWObject_Static::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Ext_Static::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_Ext_Static_OnFinishEvalKeys, MAUTOSTRIP_VOID);

	CWObject_Static::OnFinishEvalKeys();
}

void CWObject_Ext_Static::OnClientPrecache(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	CWObject_Static::OnClientPrecache(_pObj, _pWClient, _pEngine);

	CWO_Ext_Static_ClientData& CD = GetClientData(_pObj);
	int iRc = CD.m_SurfaceResourceID;
	if(iRc)
	{
		CXW_Surface* pSurface = _pWClient->GetMapData()->GetResource_Surface(iRc);
		if(pSurface)
		{
			pSurface = pSurface->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
			pSurface->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		}
	}
}

void CWObject_Ext_Static::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	CWObject_Static::OnInitClientObjects(_pObj, _pWPhysState);

	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	CWO_Ext_Static_ClientData* pCD = TDynamicCast<CWO_Ext_Static_ClientData>(pData);

	// Allocate clientdata
	if(!pCD || pCD->m_pObj != _pObj)
	{
		pCD = MNew(CWO_Ext_Static_ClientData);
		if(!pCD)
			Error_static("CWObject_Ext_Static_ClientData", "Could not allocate client data!");

		_pObj->m_lspClientObj[0] = pCD;
		pCD->Clear(_pObj);
	}
}

aint CWObject_Ext_Static::OnMessage(const CWObject_Message &_Msg)
{
	MAUTOSTRIP(CWObject_Ext_Static_OnMessage, 0);

	CWO_Ext_Static_ClientData& CD = GetClientData(this);

	switch(_Msg.m_Msg)
	{
		case OBJMSG_EXT_STATIC_IMPULSE:
		{
			CD.m_SurfaceResourceID = m_pWServer->GetMapData()->GetResourceIndex_Surface((char *)_Msg.m_pData);
			return 1;
		}
		break;

		case OBJSYSMSG_PRECACHEMESSAGE:
		{
			if(_Msg.m_DataSize == sizeof(CWObject_Message))
			{
				CWObject_Message* pMsg = (CWObject_Message *)_Msg.m_pData;
				if(pMsg->m_Msg == OBJMSG_EXT_STATIC_IMPULSE)
					m_pWServer->GetMapData()->GetResourceIndex_Surface((char*)pMsg->m_pData);
			}
		}
		break;
	}

	return CWObject_Static::OnMessage(_Msg);
}

void CWObject_Ext_Static::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	CWObject_Static::OnDeltaLoad(_pFile, _Flags);
}

void CWObject_Ext_Static::OnDeltaSave(CCFile* _pFile)
{
	CWObject_Static::OnDeltaSave(_pFile);
}

const CWO_Ext_Static_ClientData& CWObject_Ext_Static::GetClientData(const CWObject_CoreData* _pObj)
{
	const CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who deleted this client data?!");
	return *safe_cast<const CWO_Ext_Static_ClientData>(pData);
}

CWO_Ext_Static_ClientData& CWObject_Ext_Static::GetClientData(      CWObject_CoreData* _pObj)
{
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who deleted this client data?!");
	return *safe_cast<CWO_Ext_Static_ClientData>(pData);
}

void CWObject_Ext_Static::OnRefresh()
{
	CWO_Ext_Static_ClientData& CD = GetClientData(this);
	m_DirtyMask |= CD.AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}

int CWObject_Ext_Static::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const
{
	uint8* pD = _pData;

	int Flags = CWO_CLIENTUPDATE_EXTRADATA;
	if (_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
		Flags |= CWO_CLIENTUPDATE_AUTOVAR;

	pD += CWObject_Static::OnCreateClientUpdate(_iClient, _pClObjInfo, _pOld, _pData, Flags);
	if ((pD - _pData) == 0)
		return 0;

	const CWO_Ext_Static_ClientData& CD = GetClientData(this);
	CD.AutoVar_Pack(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT, pD, m_pWServer->GetMapData(), 0);

	return (pD - _pData);
}

int CWObject_Ext_Static::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	const uint8 *pD = _pData;
	pD += CWObject_Static::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);
	if (_pObj->m_iClass == 0 || (pD - _pData) == 0)
		return 0;

	CWO_Ext_Static_ClientData& CD = GetClientData(_pObj);

	if (_pObj->m_bAutoVarDirty)
		CD.AutoVar_Unpack(pD, _pWClient->GetMapData(), 0);

	return (pD - _pData);
}

void CWObject_Ext_Static::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Ext_Static_OnClientRender, MAUTOSTRIP_VOID);

	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

	CWO_Ext_Static_ClientData& CD = GetClientData(_pObj);

	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		if(_pObj->m_iModel[i] > 0)
		{
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
			if(pModel)
			{
				CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);

				if(CD.m_SurfaceResourceID > 0)
				{
					MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
					
					CXW_Surface* pSurface = _pWClient->GetMapData()->GetResource_Surface(CD.m_SurfaceResourceID);
					int SurfaceID = (pSurface) ? pSC->GetSurfaceID(pSurface->m_Name) : 0;
					pSurface = pSC->GetSurface(SurfaceID);

					if(pSurface)
						pSurface = pSurface->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);

					AnimState.m_lpSurfaces[0] = pSurface;
				}

				AnimState.m_Data[3] = ~(_pObj->m_Data[3]);

				switch(_pObj->m_iAnim1)
				{
				case 1: AnimState.m_AnimAttr0 = 0; break;
				case 2: AnimState.m_AnimAttr0 = 1; break;
				}

				_pEngine->Render_AddModel(pModel, MatIP, AnimState);
			}
		}
	}
}


// -------------------------------------------------------------------
//  NULL
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Null, CWObject, 0x0100);

void CWObject_Null::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Null_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
		{
	/*		CStr ModelName = CStrF("$WORLD:%d", _Value.Val_int());
			int iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
			if (!iModel) Error("OnEvalKey", "Failed to acquire world-model.");

			m_iModel[0] = iModel;

			// Setup physics
			CWO_PhysicsState Phys;
			Phys.m_PhysFlags = 0;
			Phys.m_ObjectFlags = 0;
			if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
				ConOutL("§cf80WARNING: Unable to set BSP-model physics state.");

			// Set bound-box.
			{
				CBox3Dfp32 Box(GetPosition(), GetPosition());
				CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
				if (pModel) pModel->GetBound_Box(Box);
				m_pWServer->Object_SetVisBox(m_iObject, Box.m_Min, Box.m_Max);
			}*/
			break;
		}
	default:
		{
			CWObject::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

// -------------------------------------------------------------------
//  WORLDSPAWN
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_WorldSpawn, CWObject_Model, 0x0100);

CWObject_WorldSpawn::CWObject_WorldSpawn()
{
	MAUTOSTRIP(CWObject_WorldSpawn_ctor, MAUTOSTRIP_VOID);

	m_ClientFlags |= CWO_CLIENTFLAGS_LINKINFINITE | CWO_CLIENTFLAGS_VISIBILITY;
	m_XR_Mode = 0;
	m_Data[1] = 0;
	m_Data[2] = 0;
	m_Data[3] = 0;
	m_ModelIndex = -1;
}

void CWObject_WorldSpawn::OnCreate()
{
	MAUTOSTRIP(CWObject_WorldSpawn_OnCreate, MAUTOSTRIP_VOID);

	CWObject_Model::OnCreate();
}

void CWObject_WorldSpawn::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_WorldSpawn_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH3('RESO','URCE','S'): // "RESOURCES"
	case MHASH2('PLAY','ERS'): // "PLAYERS"
	case MHASH2('AUTH','OR'): // "AUTHOR"
		{
			// /ignore
			break;
		}
	case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
		{
			m_ModelIndex = _Value.Val_int();
			break;
		}
	case MHASH5('HASP','LAYE','RPHY','SMOD','EL'): // "HASPLAYERPHYSMODEL"
		{
			m_Data[2] = _pKey->GetThisValuei();
			break;
		}
	case MHASH4('HASG','LASS','MODE','L'): // "HASGLASSMODEL"
		{
			m_Data[3] = _pKey->GetThisValuei();
			break;
		}
/*	
	case MHASH4('XR_F','OGCU','LLOF','FSET'): // "XR_FOGCULLOFFSET"
	{
		m_Data[1] = _Value.Val_int();
		break;
	}
*/
	case MHASH2('XR_M','ODE'): // "XR_MODE"
		{
			m_XR_Mode = _Value.Val_int();
			if( m_XR_Mode == 3 )
			{
				// This model holds data for both BSP2 and BSP3... choose which to load somehow
#if defined(PLATFORM_PS2)
				m_XR_Mode = 2;
#elif defined(PLATFORM_XBOX)
				m_XR_Mode = 1;
#else
				MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
				CRegistry* pReg = (pSys) ? pSys->GetEnvironment() : NULL;
				if( pReg )
				{
					m_XR_Mode = pReg->GetValuei( "XR_MODEOVERRIDE", 1 );
				}
				else
					m_XR_Mode = 1;
#endif
			}
			break;
		}
	default:
		{
			CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}


/*CXR_WallmarkDesc CreateWallMarkDesc(int _Flags, fp32 _Size, const char* _pSurfName, int _TextureID)
{
	MAUTOSTRIP(CreateWallMarkDesc, CXR_WallmarkDesc());

	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	
	CXR_WallmarkDesc WM;
	WM.m_Flags = _Flags;
//	WM.m_Tolerance = _Tolerance;
	WM.m_Size = _Size;
	WM.m_SurfaceParam_TextureID[0] = _TextureID;
	if (pSC) WM.m_SurfaceID = pSC->GetSurfaceID(_pSurfName);

	return WM;
}*/

void CWObject_WorldSpawn::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CWObject_WorldSpawn_OnIncludeClass, MAUTOSTRIP_VOID);

//	_pWData->GetResourceIndex_Class("Player");
//	_pWData->GetResourceIndex_Class("Character");
	_pWData->GetResourceIndex_Class("DynamicLight2");

#if defined(PLATFORM_DOLPHIN ) || defined(PLATFORM_PS2)
#if	!defined(M_RTM)
	_pWData->GetResourceIndex_Class("Line");
	_pWData->GetResourceIndex_Class("Coordsys");
	_pWData->GetResourceIndex_Class("CoordinateSystem");
#endif
#else
	_pWData->GetResourceIndex_Class("Line");
	_pWData->GetResourceIndex_Class("Coordsys");
	_pWData->GetResourceIndex_Class("CoordinateSystem");
#endif


//	_pWData->GetResourceIndex_Class("NavigationProbe");
	_pWData->GetResourceIndex_Class("Info_Player_Start");

/*	_pWData->GetResourceIndex_Model("PARTICLES_TORCHFIRE");
	_pWData->GetResourceIndex_Model("PARTICLES_FIREWALL");
	_pWData->GetResourceIndex_Model("PARTICLES_SMALLFIRE");
//	_pWData->GetResourceIndex_Model("PARTICLES_GENERIC");
	_pWData->GetResourceIndex_Model("PARTICLES_TELEPORTER");*/
	_pWData->GetResourceIndex_Model("FLARE");
//	_pWData->GetResourceIndex_Model("FOGVOLUME");
//	_pWData->GetResourceIndex_Model("SKY");
//	_pWData->GetResourceIndex_Model("WATERTILE");

//	_pWData->GetResourceIndex_Model("CURVESOLID");

//	_pWData->GetResourceIndex_Font("FONT1");
//	_pWData->GetResourceIndex_Font("MENU");

//	_pWData->GetResourceIndex_Model("Items\\Viewmodels\\Viewmodel");

/*	_pWData->GetResourceIndex_Sound("PICKUPHEALTH");
	_pWData->GetResourceIndex_Sound("PICKUPHEALTH2");
	_pWData->GetResourceIndex_Sound("PICKUPMANA0");
	_pWData->GetResourceIndex_Sound("PICKUPMANA1");
	_pWData->GetResourceIndex_Sound("PICKUPMANA2");
	_pWData->GetResourceIndex_Sound("PICKUPDIVINE");
	_pWData->GetResourceIndex_Sound("PICKUPSCROLL");
	_pWData->GetResourceIndex_Sound("GOREGIBBED");
	_pWData->GetResourceIndex_Sound("TELEPORT1");
	_pWData->GetResourceIndex_Sound("TELEPORT2");
	_pWData->GetResourceIndex_Sound("DOOR1START");
	_pWData->GetResourceIndex_Sound("DOOR1STOP");
	_pWData->GetResourceIndex_Sound("DOOR1SLIDE");
	_pWData->GetResourceIndex_Sound("DOOR2START");
	_pWData->GetResourceIndex_Sound("DOOR2STOP");
	_pWData->GetResourceIndex_Sound("DOOR2SLIDE");*/
}

void CWObject_WorldSpawn::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_WorldSpawn_OnFinishEvalKeys, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_WorldSpawn::OnFinishEvalKeys, WOBJ_SYSTEM);

	m_pWServer->Registry_GetGame()->SetValuei("XR_MODE", m_XR_Mode);
	m_Data[0] = m_XR_Mode;

	if (m_ModelIndex >= 0)
	{
		CStr ModelName = CStrF("$WORLD:%d:%d", m_ModelIndex, m_Data[3]);
/*
		int iModel;
		if (m_XR_Mode == 1)
			iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel2(ModelName);
		else if (m_XR_Mode == 2)
			iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel3(ModelName);
		else
			iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
*/
		int iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
		if (!iModel) Error("OnEvalKey", "Failed to acquire world-model.");

		CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(iModel);
		if (!pModel) Error("OnEvalKey", "Resource is not a CXR_Model.");

		m_iModel[0] = iModel;

		// Setup physics
		CWO_PhysicsState Phys;
		Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_PHYSMODEL, m_iModel[0], 0, 0);
		Phys.m_nPrim = 1;

		Phys.m_PhysFlags = 0;
		Phys.m_ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
		if (!m_Data[2])
			Phys.m_ObjectFlags |= OBJECT_FLAGS_PLAYERPHYSMODEL;

		if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
			Error("OnEvalKey", "Unable to set world physics state.");

		// Set bound-box.
		{
/*			CBox3Dfp32 Box;
			pModel->GetBound_Box(Box);
			m_pWServer->Object_SetVisBox(m_iObject, Box.m_Min, Box.m_Max);*/
/*			CVec3Dfp32 p; fp32 r;
			m_pWServer->GetMapData()->GetResource_BSPModel(m_iModel[0])->GetBound_Sphere(p, r);
			m_pWServer->Object_SetVisBox(m_iObject, p + CVec3Dfp32(-r), p + CVec3Dfp32(r));*/
		}
	}

	m_pWServer->World_SetModel(m_iModel[0]);
	m_pWServer->Object_SetWorldspawnIndex(m_iObject);

	CWObject_Model::OnFinishEvalKeys();
}

void CWObject_WorldSpawn::OnRefresh()
{
	MAUTOSTRIP(CWObject_WorldSpawn_OnRefresh, MAUTOSTRIP_VOID);
}

void CWObject_WorldSpawn::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_WorldSpawn_OnClientRefresh, MAUTOSTRIP_VOID);
	MSCOPESHORT( CWObject_WorldSpawn::OnClientRefresh );
	_pWClient->Object_SetWorldspawnIndex(_pObj->m_iObject);
}

void CWObject_WorldSpawn::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_WorldSpawn_OnClientRender, MAUTOSTRIP_VOID);
}

void CWObject_WorldSpawn::OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_WorldSpawn_OnClientRenderVis, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_WorldSpawn::OnClientRenderVis);

	_pEngine->SetVar(XR_ENGINE_MODE, _pObj->m_Data[0]);
//	_pEngine->SetVar(XR_ENGINE_FOGCULLOFFSET, _pObj->m_Data[1]);

	CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if (!pModel) ConOut(CStrF("(CWObject_WorldSpawn::OnClientRender) No world-model. (Rc %d)", _pObj->m_iModel[0]));
	_pEngine->Render_AddModel(pModel, _pObj->GetPositionMatrix(), _pObj->GetDefaultAnimState(_pWClient), XR_MODEL_WORLD);
}

int CWObject_WorldSpawn::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	MAUTOSTRIP(CWObject_WorldSpawn_OnClientUpdate, 0);
	MSCOPESHORT(CWObject_WorldSpawn::OnClientUpdate);

	int Ret = CWObject_Model::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);
	_pWClient->World_SetModel(_pObj->m_iModel[0]);
	return Ret;
}

void CWObject_WorldSpawn::OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	MAUTOSTRIP(CWObject_WorldSpawn_OnClientLoad, MAUTOSTRIP_VOID);

	CWObject_Model::OnClientLoad(_pObj, _pWorld, _pFile, _pWData, _Flags);

	// Questionable if we should do this here, but it's the only transparent solution I can think of ATM.
	_pWorld->World_SetModel(_pObj->m_iModel[0]);
}


aint CWObject_WorldSpawn::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_SPAWN_WALLMARK:
		{
			const CXR_WallmarkDesc* pWMD = (const CXR_WallmarkDesc*)_Msg.m_Param0;
			const CMat4Dfp32* pMat = (const CMat4Dfp32*)_Msg.m_Param1;
			_pWClient->Wallmark_Create(*pWMD, *pMat, 4, XR_WALLMARK_SCANFOROVERLAP);
		}
		return 0;

	default:
		return CWObject_Model::OnClientMessage(_pObj, _pWClient, _Msg);
	}
}



// -------------------------------------------------------------------
//  WORLDPLAYERPHYS
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_WorldPlayerPhys, CWObject_Model, 0x0100);

CWObject_WorldPlayerPhys::CWObject_WorldPlayerPhys()
{
	MAUTOSTRIP(CWObject_WorldPlayerPhys_ctor, MAUTOSTRIP_VOID);

	m_ClientFlags |= CWO_CLIENTFLAGS_LINKINFINITE;
	m_ModelIndex = -1;
}

void CWObject_WorldPlayerPhys::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_WorldPlayerPhys_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();


	switch (_KeyHash)
	{
	case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
		{
			m_ModelIndex = _Value.Val_int();
			break;
		}
	default:
		{
			CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_WorldPlayerPhys::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_WorldPlayerPhys_OnFinishEvalKeys, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_WorldPlayerPhys::OnFinishEvalKeys, WOBJ_SYSTEM);

	if (m_ModelIndex >= 0)
	{
		CStr ModelName = CStrF("XW4:%s:%d", m_pWServer->GetMapData()->GetWorld().Str(), m_ModelIndex);
		int iModel = m_pWServer->GetMapData()->GetResourceIndex(ModelName);
		if (!iModel) Error("OnEvalKey", "Failed to acquire world-model.");

		CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(iModel);
		if (!pModel) Error("OnEvalKey", "Resource is not a CXR_Model.");

		m_iModel[0] = iModel;

		// Setup physics
		CWO_PhysicsState Phys;
		Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_PHYSMODEL, m_iModel[0], 0, 0);
		Phys.m_nPrim = 1;
		Phys.m_PhysFlags = 0;
		Phys.m_ObjectFlags = OBJECT_FLAGS_PLAYERPHYSMODEL;
		Phys.m_ObjectIntersectFlags = 0;

		if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
			Error("OnEvalKey", "Unable to set world physics state.");

		// Set bound-box.
/*		{
			CBox3Dfp32 Box;
			pModel->GetBound_Box(Box);
			m_pWServer->Object_SetVisBox(m_iObject, Box.m_Min, Box.m_Max);
		}*/
	}

	CWObject_Model::OnFinishEvalKeys();
}

void CWObject_WorldPlayerPhys::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_WorldPlayerPhys_OnClientRender, MAUTOSTRIP_VOID);

#ifndef M_RTM
	if (_pWClient->Registry_GetUser()->GetValuei("SHOWPLAYERPHYS"))
	{
		CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
		if (!pModel)
			ConOut(CStrF("(CWObject_WorldPlayerPhys::OnClientRender) No model. (Rc %d)", _pObj->m_iModel[0]));
		_pEngine->Render_AddModel(pModel, _pObj->GetPositionMatrix(), _pObj->GetDefaultAnimState(_pWClient));
	}
#endif
}

// -------------------------------------------------------------------
//  WORLDGLASSSPAWN
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_WorldGlassSpawn, CWObject_Model, 0x0100);


static const char* g_lpTranslateCrushType[] = { "Point", "Cube", "Sphere", NULL };


CWObject_WorldGlassSpawn::CWO_GlassClientData::CWO_GlassClientData()
{
	m_spModelInstance = NULL;
}


CWObject_WorldGlassSpawn::CWO_GlassClientData::~CWO_GlassClientData()
{
	m_spModelInstance = NULL;
}


CWObject_WorldGlassSpawn::CWObject_WorldGlassSpawn()
{
	MAUTOSTRIP(CWObject_WorldGlassSpawn, MAUTOSTRIP_VOID);

	m_ClientFlags |= CWO_CLIENTFLAGS_LINKINFINITE;
	m_ModelIndex = -1;
}


void CWObject_WorldGlassSpawn::OnIncludeClass(CMapData* _pMapData, CWorld_Server* _pWServer)
{
	CWObject_Model::OnIncludeClass(_pMapData, _pWServer);

	_pMapData->GetResourceIndex_Sound("env_glass_wndwmed_crack_01");
	_pMapData->GetResourceIndex_Sound("env_glass_wndwmed_01");
	_pMapData->GetResourceIndex_Sound("env_glass_wndwmed_loose_01");
}


void CWObject_WorldGlassSpawn::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_WorldGlassSpawn_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();


	switch (_KeyHash)
	{
	case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
		{
			m_ModelIndex = _Value.Val_int();
		
			CStr ModelName = CStrF("XGL:%s:%d", m_pWServer->GetMapData()->GetWorld().Str(), m_ModelIndex);
			m_Data[GLASS_MODEL_INDEX] = m_pWServer->GetMapData()->GetResourceIndex(ModelName);
			break;
		}
	default:
		{
			CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}


void CWObject_WorldGlassSpawn::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_WorldGlassSpawn_OnFinishEvalKeys, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_WorldGlassSpawn::OnFinishEvalKeys, WOBJ_SYSTEM);

	if (!(m_ClientFlags & CWO_GLASS_CLIENTFLAGS_DYNAMIC))
		m_pWServer->Object_SetName(m_iObject, "WORLDGLASSSPAWN");

	if (m_ModelIndex >= 0)
	{
		//CStr ModelName = CStrF("XGL:%s:%d", m_pWServer->GetMapData()->GetWorld().Str(), m_ModelIndex);
		//int iModel = m_pWServer->GetMapData()->GetResourceIndex(ModelName);
		int iModel = m_Data[GLASS_MODEL_INDEX];
		if (!iModel) Error("OnEvalKey", "Failed to acquire glass world-model.");

		CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(iModel);
		if (!pModel) Error("OnFinishEvalKey", "Resource is not a CXR_Model.");

		m_iModel[0] = iModel;

		// Setup physics
		CWO_PhysicsState Phys;
		Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_PHYSMODEL, m_iModel[0], 0, 0);
		Phys.m_nPrim = 1;
		Phys.m_PhysFlags = 0;
		Phys.m_ObjectFlags = OBJECT_FLAGS_PLAYERPHYSMODEL | OBJECT_FLAGS_PHYSMODEL;//OBJECT_FLAGS_PLAYERPHYSMODEL;
		Phys.m_ObjectIntersectFlags = 0;

		if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
			Error("OnFinnishKey", "Unable to set world glass state.");

		CXR_Model_BSP4Glass* pGlassModel = safe_cast<CXR_Model_BSP4Glass>(pModel);
		if(!pGlassModel) Error("OnFinishEvalKey", "Resource is not a CXR_Model_BSP4Glass.");

		// Create link context for glass model if we want
		if (!(m_ClientFlags & CWO_GLASS_CLIENTFLAGS_NOLINK))
			pGlassModel->CreateLinkContext(GetPositionMatrix());
	}

	#ifndef M_RTM
		// Precache fonts
		CRC_Font* pFont = m_pWServer->GetMapData()->GetResource_Font(m_pWServer->GetMapData()->GetResourceIndex_Font("MONOPRO"));
		MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
		if(pFont && pTC)
		{
			int nFontTex = pFont->m_spTC->GetNumLocal();
			for (int i = 0; i < nFontTex; i++)
				pTC->SetTextureParam(pFont->m_spTC->GetTextureID(i), CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		}
	#endif

	CWObject_Model::OnFinishEvalKeys();
}


void CWObject_WorldGlassSpawn::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	CWO_GlassClientData* pCD = TDynamicCast<CWO_GlassClientData>(pData);

	// Allocate clientdata
	//if (!pCD || pCD->m_pObj != _pObj || pCD->m_pWPhysState != _pWPhysState)
	if(!pCD)
	{
		pCD = MNew(CWO_GlassClientData);
		if (!pCD)
			Error_static("CWObject_WorldGlassSpawn", "Could not allocate client data!");

		_pObj->m_lspClientObj[0] = pCD;
	}
}


CWObject_WorldGlassSpawn::CWO_GlassClientData& CWObject_WorldGlassSpawn::GetClientData(CWObject_CoreData* _pObj)
{
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who wiped my client data?!");
	return *safe_cast<CWO_GlassClientData>(pData);
}


const CWObject_WorldGlassSpawn::CWO_GlassClientData& CWObject_WorldGlassSpawn::GetClientData(const CWObject_CoreData* _pObj)
{
	const CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "This client data is scrap");
	return *safe_cast<const CWO_GlassClientData>(pData);
}


CXR_Model_BSP4Glass_Instance* CWObject_WorldGlassSpawn::GetGlassModelInstance(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	return safe_cast<CXR_Model_BSP4Glass_Instance>(GetModelInstance(GetClientData(_pObj), _pObj, _pWPhysState));
}


CXR_ModelInstance* CWObject_WorldGlassSpawn::GetModelInstance(CWObject_WorldGlassSpawn::CWO_GlassClientData& _CD, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Create model instance if it doesn't exist.
	if(!_CD.m_spModelInstance)
	{
		CXR_Model* pModel = _pWPhysState->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
		if(pModel)
		{
			_CD.m_spModelInstance = pModel->CreateModelInstance();
			if(_CD.m_spModelInstance)
			{
				CXR_ModelInstanceContext ModelInstanceContext(_pWPhysState->GetGameTick(), _pWPhysState->GetGameTickTime(), _pObj);
				_CD.m_spModelInstance->Create(pModel, ModelInstanceContext);
			}
		}
	}

	if(!_pObj->GetPhysState().m_pModelInstance)
		_pObj->SetPhysStateModelInstance(_CD.m_spModelInstance);

	return _CD.m_spModelInstance;
}


void CWObject_WorldGlassSpawn::OnRefresh()
{
	MSCOPESHORT(CWObject_WorldGlassSpawn::OnRefresh);

	// Refresh server model instance, so collision can be done properly
	CWO_GlassClientData& CD = GetClientData(this);
	CWorld_PhysState* pWPhysState = safe_cast<CWorld_PhysState>(m_pWServer);
	CXR_ModelInstance* pModelInstance = GetModelInstance(CD, this, pWPhysState);
	CXR_Model_BSP4Glass_Instance* pGlassInstance = GetGlassModelInstance(this, pWPhysState);
	if(pGlassInstance)
	{
		CXR_AnimState AnimState(CMTime::CreateFromTicks(GetAnimTick(m_pWServer), m_pWServer->GetGameTickTime(), 0.0f),
								CMTime::CreateFromTicks(m_iAnim2, m_pWServer->GetGameTickTime()),
								m_iAnim0, m_iAnim1, pModelInstance, m_iObject);

		// Wirerendering
#ifndef M_RTM
		bool bWire = false;
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (pSys)
			bWire = (pSys->GetEnvironment()->GetValuei("SV_GLASSWIRE", 0) != 0);

		if (bWire)
		{
			// Render debug wires
			CWireContainer* pWire = (CWireContainer*)m_pWServer->Debug_GetWireContainer();
			if (pWire)
				pGlassInstance->Debug_RenderWire(pWire, CPixel32(0,0,255,255), GetPositionMatrix());
		}
#endif

		CXR_ModelInstanceContext ModelInstanceContext(m_pWServer->GetGameTick(), m_pWServer->GetGameTickTime(), this, NULL, 0, &AnimState);
		pGlassInstance->OnRefresh(ModelInstanceContext, &GetPositionMatrix(), 1);
	}
}


void CWObject_WorldGlassSpawn::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_WorldGlassSpawn::OnClientRefresh);
	
	// Refresh model instance
	//CWO_GlassClientData& CD = GetClientData(_pObj);
	CWorld_PhysState* pWPhysState = safe_cast<CWorld_PhysState>(_pWClient);
	CXR_Model_BSP4Glass_Instance* pGlassInstance = GetGlassModelInstance(_pObj, pWPhysState);
	if(pGlassInstance)
	{
		CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
		CXR_ModelInstanceContext ModelInstanceContext(_pWClient->GetGameTick(), _pWClient->GetGameTickTime(), _pObj, _pWClient, 0, &AnimState);
		pGlassInstance->OnRefresh(ModelInstanceContext, &_pObj->GetPositionMatrix(), 1);
	}
}


int CWObject_WorldGlassSpawn::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	const uint8 *pD = _pData;
	pD += CWObject_Model::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);

	if (_pObj->m_iClass == 0 || (pD - _pData) == 0)
		return (pD - _pData);

	CWO_GlassClientData& CD = GetClientData(_pObj);
	//if(_pObj->m_bAutoVarDirty) {}

	if(_pObj->m_lModelInstances[0])
		CD.m_spModelInstance = _pObj->m_lModelInstances[0];

	// Make sure instance has been created
	CWorld_PhysState* pWPhysState = safe_cast<CWorld_PhysState>(_pWClient);
	GetModelInstance(CD, _pObj, pWPhysState);

	return(pD - _pData);
}


void CWObject_WorldGlassSpawn::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_WorldGlassSpawn_OnClientRender, MAUTOSTRIP_VOID);

	OnClientRenderGlass(_pObj, _pWClient, _pEngine, _ParentMat, _pObj->GetPositionMatrix());
}


void CWObject_WorldGlassSpawn::OnClientRenderGlass(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat, const CMat4Dfp32& _WMat)
{
	MAUTOSTRIP(CWObject_WorldGlassSpawn_OnClientRender_Glass, MAUTOSTRIP_VOID);

#ifndef M_RTM
	if (_pWClient->Registry_GetUser()->GetValuei("SHOWPLAYERPHYS"))
	{
		CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);

		if (!pModel)
			ConOut(CStrF("(CWObject_WorldGlassSpawn::OnClientRender) No model. (Rc %d)", _pObj->m_iModel[0]));
		else
			_pEngine->Render_AddModel(pModel, _WMat, _pObj->GetDefaultAnimState(_pWClient));
	}

	{
		CWO_GlassClientData& CD = GetClientData(_pObj);
		CWorld_PhysState* pWPhysState = safe_cast<CWorld_PhysState>(_pWClient);
		CXR_Model_BSP4Glass_Instance* pGlassModelInstance = GetGlassModelInstance(_pObj, pWPhysState);
		if(pGlassModelInstance)
			pGlassModelInstance->m_pDebug_Font = _pWClient->GetMapData()->GetResource_Font(_pWClient->GetMapData()->GetResourceIndex_Font("MONOPRO"));
	}
#endif

	// Render model instance
	{
		CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
		if(pModel)
		{
			CWorld_PhysState* pWPhysState = safe_cast<CWorld_PhysState>(_pWClient);
			CWO_GlassClientData& CD = GetClientData(_pObj);

			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
			AnimState.m_pModelInstance = GetModelInstance(CD, _pObj, pWPhysState);
			AnimState.m_AnimAttr0 = _pWClient->GetGameTickTime();
			AnimState.m_AnimAttr1 = _pWClient->GetRenderTickFrac();
			AnimState.m_Data[GLASS_DATA_GAMETICK] = _pWClient->GetGameTick();
			_pEngine->Render_AddModel(pModel, _WMat, AnimState);
		}
	}
}


int CWObject_WorldGlassSpawn::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pWPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	if (!_pWPhysState->IsServer())
		return CWObject_Model::OnPhysicsEvent(_pObj, _pObjOther, _pWPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);

	switch (_Event)
	{
	case CWO_PHYSEVENT_DYNAMICS_COLLISION:
	case CWO_PHYSEVENT_IMPACT:
		{
			// Implement ?
			break;
		}

	case CWO_PHYSEVENT_IMPACTOTHER:
		{
			CXR_Model_BSP4Glass_Instance* pGlassInstance = GetGlassModelInstance(_pObj, _pWPhysState);
			if (_pCollisionInfo && pGlassInstance && pGlassInstance->GetAttrib_NoPhys(_pCollisionInfo->m_LocalNode))
			{
				CMat4Dfp32 ObjMatInv, ObjOtherMat;
				CBox3Dfp32 BoxSize = *_pObjOther->GetAbsBoundBox();
				_pObj->GetLocalPositionMatrix().InverseOrthogonal(ObjMatInv);
				_pObjOther->GetLocalPositionMatrix().Multiply(ObjMatInv, ObjOtherMat);

				CWObject_Message CrushMsg(OBJMSG_GLASS_CRUSH);
				CWO_Glass_MsgData MsgData;
				MsgData.m_iInstance = _pCollisionInfo->m_LocalNode;
				MsgData.m_CrushType = GLASS_CRUSH_CUBE;
				MsgData.m_ForceDir = ObjOtherMat.GetRow(0);
				MsgData.m_LocalPosition = ObjOtherMat.GetRow(3);
				MsgData.m_BoxSize = BoxSize.m_Max - BoxSize.m_Min;
				
				CWO_Glass_MsgData::SetData(CrushMsg, MsgData);
				_pWPhysState->Phys_Message_SendToObject(CrushMsg, _pObj->m_iObject);
			}
			return SERVER_PHYS_DEFAULTHANDLER;
		}
	}

	return CWObject_Model::OnPhysicsEvent(_pObj, _pObjOther, _pWPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);
}


void CWObject_WorldGlassSpawn::Impulse(uint _Impulse, const char* _pImpulseData, int16 _iSender)
{
	//if (_pImpulseData)
	{
		CFStr ImpulseData(_pImpulseData);

		CWO_GlassClientData& CD = GetClientData(this);
		CXR_Model_BSP4Glass_Instance* pGlassModelInstance = GetGlassModelInstance(this, m_pWServer);
		
		// Find instance if needed
		uint16 iInstance = GLASS_MAX_INSTANCE;
		CStr GlassName = ImpulseData.GetStrSep(",");
		if (_Impulse >= GLASS_IMPULSE_NEEDINSTANCE && _Impulse < GLASS_IMPULSE_NOINSTANCE)
		{
			CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
			CXR_Model_BSP4Glass* pGlassModel = safe_cast<CXR_Model_BSP4Glass>(pModel);
			if(pGlassModel)
			{
				uint32 GlassHash = uint32(StringToHash(GlassName));
				iInstance = pGlassModel->GetInstanceFromHash(GlassHash);
			}

			if (iInstance == GLASS_MAX_INSTANCE)
			{
				ConOutL(CStrF("CWObject_WorldGlassSpawn::Impulse: Invalid glass target name! (%s)", GlassName.Str()));
				return;
			}

			if (!pGlassModelInstance)
			{
				ConOutL("CWObject_WorldGlassSpawn::Impulse: No valid glass instance data!");
				return;
			}
		}

		// Handle impulse
		switch (_Impulse)
		{
		// -----------------------------------------------------------------------------------------
		//  PER INSTANCE MESSAGES
		// -----------------------------------------------------------------------------------------
		case GLASS_IMPULSE_RESTORE:
			{
				// Restore server data
				pGlassModelInstance->Server_Restore(iInstance);

				// Send message to clients for restoration
				CNetMsg NetMsg(GLASS_MSG_RESTORE);
				NetMsg.AddInt16(iInstance);
				m_pWServer->NetMsg_SendToObject(NetMsg, m_iObject);
				break;
			}

		case GLASS_IMPULSE_CRUSH:
			{
				// Read impulse message
				CVec3Dfp32 BoxSize;
				CFStr XYStr = ImpulseData.GetStrSep(",");
				fp32 x = fp32(XYStr.GetStrSep(" ").Val_fp64());
				fp32 y = fp32(XYStr.GetStrSep(" ").Val_fp64());
				CFStr ForceDirObj = ImpulseData.GetStrSep(",");
				fp32 ForceScale = fp32(ImpulseData.GetStrSep(",").Val_fp64());
				bool bRandForceRange = ImpulseData.GetStrSep(",").Val_int() != 0;
				uint8 CrushType = uint8(ImpulseData.GetStrSep(",").TranslateInt(g_lpTranslateCrushType));
				CFStr SizeStr = ImpulseData.GetStrSep(",");				
				fp32 Radius = fp32(SizeStr.GetStrSep(" ").Val_fp64());
				BoxSize.k[0] = Radius;
				BoxSize.k[1] = fp32(SizeStr.GetStrSep(" ").Val_fp64());
				BoxSize.k[2] = fp32(SizeStr.GetStrSep(" ").Val_fp64());
				
				// Create from impulse data
				uint8 MsgType = TranslateCrushType(CrushType);
				CVec3Dfp32 Force = 0.0f;
				int iObj = _iSender;
				
				if (ForceDirObj.CompareNoCase("$ACTIVATOR") != 0)
				{
					int iForceDirObj = m_pWServer->Selection_GetSingleTarget(ForceDirObj);
					if (iForceDirObj > 0)
						iObj = iForceDirObj;
				}
				CWObject* pObj = m_pWServer->Object_Get(iObj);
				if (pObj)
					Force = pObj->GetPositionMatrix().GetRow(0);

				// Fetch game tick and local position
				int32 GameTick = m_pWServer->GetGameTick();
				CVec3Dfp32 LocalPos = pGlassModelInstance->GetLocalPosFrom01(iInstance, x, y);

				if (pGlassModelInstance->Server_CrushGlassSurface(CrushType, iInstance, LocalPos, Radius, BoxSize, GameTick, GetPositionMatrix(), 0))
				{
					// Create crush net msg with CInfo
					CNetMsg NetMsg(MsgType);
					NetMsg.AddInt8(1);

					// Add seed, instance and crush flags
					NetMsg.AddInt32(GameTick);
					NetMsg.AddInt32(iInstance);
					NetMsg.AddInt8(0);

					// Add local position
					NetMsg.Addfp32(LocalPos.k[0]);
					NetMsg.Addfp32(LocalPos.k[1]);
					NetMsg.Addfp32(LocalPos.k[2]);

					// Add force and scale
					NetMsg.Addfp32(Force.k[0]);
					NetMsg.Addfp32(Force.k[1]);
					NetMsg.Addfp32(Force.k[2]);
					NetMsg.Addfp32((bRandForceRange ? MFloat_GetRand(GameTick) : 1.0f) * ForceScale);

					if (CrushType == GLASS_CRUSH_CUBE)
					{
						NetMsg.Addfp32(BoxSize.k[0]);
						NetMsg.Addfp32(BoxSize.k[1]);
						NetMsg.Addfp32(BoxSize.k[2]);
					}
					else if (CrushType == GLASS_CRUSH_SPHERE)
					{
						NetMsg.Addfp32(Radius);
					}

					// Send message to crush window on client
					m_pWServer->NetMsg_SendToObject(NetMsg, m_iObject);
				}
				break;
			}

		case GLASS_IMPULSE_ACTIVE:
			{
				int8 Inactive = (ImpulseData.GetStrSep(",").Val_int() == 0) ? 1 : 0;
				
				pGlassModelInstance->Server_Inactive(iInstance, (Inactive != 0));

				CNetMsg NetMsg(GLASS_MSG_ACTIVE);
				NetMsg.AddInt16(iInstance);
				NetMsg.AddInt8(Inactive);
				m_pWServer->NetMsg_SendToObject(NetMsg, m_iObject);
				break;
			}

		// -----------------------------------------------------------------------------------------
		//  NONE INSTANCE MESSAGES
		// -----------------------------------------------------------------------------------------
		case GLASS_IMPULSE_GLOBAL_RESTORE:
			{
				// Restore server data
				pGlassModelInstance->Server_RestoreAll();

				// Send message to clients for restoration
				CNetMsg NetMsg(GLASS_MSG_RESTORE_ALL);
				m_pWServer->NetMsg_SendToObject(NetMsg, m_iObject);
				break;
			}

		default:
			break;
		}
	}
}


aint CWObject_WorldGlassSpawn::OnMessage(const CWObject_Message& _Msg)
{
	static bool bNotifyReturnTrue = true;

	switch(_Msg.m_Msg)
	{
	case OBJSYSMSG_NOTIFY_INTERSECTION:
		{
			M_TRACEALWAYS("CWObject_WorldGlassSpawn::OnMessage(OBJSYSMSG_NOTIFY_INTERSECTION)\n");
			if (bNotifyReturnTrue)
				return 1;
		}
		return 0;

	case OBJMSG_GLASS_IMPULSE:
		{
			Impulse(_Msg.m_Param0, (const char*)_Msg.m_pData, _Msg.m_iSender);
		}
		return 0;

	case OBJMSG_GLASS_CRUSH:
		{
			const CWO_Glass_MsgData* pMsgData = CWO_Glass_MsgData::GetSafe(_Msg);
			if (!pMsgData)
				return 0;

			CVec3Dfp32 BoxSize = pMsgData->m_BoxSize;
			CVec3Dfp32 LocalPosition = pMsgData->m_LocalPosition;
			CVec3Dfp32 Force = pMsgData->m_ForceDir;
			CVec2Dfp32 ForceRange = pMsgData->m_ForceRange;
			fp32 ForceScale = pMsgData->m_ForceScale;
			fp32 Radius = pMsgData->m_Radius;
			uint16 iInstance = pMsgData->m_iInstance;
			uint8 CrushType = pMsgData->m_CrushType;
			int32 GameTick = m_pWServer->GetGameTick();
			
			// Find instances we might have hit
			if (iInstance == 0xFFFF)
			{
				CWO_PhysicsState PhysState;
				PhysState.Clear();
				PhysState.m_MediumFlags = XW_MEDIUM_GLASS;
				PhysState.m_ObjectFlags = OBJECT_FLAGS_PROJECTILE;
				PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_PHYSMODEL;
				if (CrushType == GLASS_CRUSH_POINT)
					PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_POINT, 0, CVec3Dfp32(Radius), 0));
				else if (CrushType == GLASS_CRUSH_SPHERE)
					PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_SPHERE, 0, CVec3Dfp32(Radius), 0));
				else if (CrushType == GLASS_CRUSH_CUBE)
					PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_BOX, 0, BoxSize, 0));

				CCollisionInfo CInfo;
				CMat4Dfp32 Origin, Dest;
				Origin.Unit();
				Origin.GetRow(3) = LocalPosition * GetLocalPositionMatrix();
				Dest = Origin;
				Dest.GetRow(3) += CVec3Dfp32(0.0f, 0.0f, 0.0001f);

				TSelection<CSelection::SMALL_BUFFER> Selection;
				Selection.AddData(m_iObject);
				if (m_pWServer->Phys_IntersectWorld(&Selection, PhysState, Origin, Dest, _Msg.m_iSender, &CInfo) && CInfo.m_bIsValid)
					iInstance = CInfo.m_LocalNode;
			}

			CWorld_PhysState* pWPhysState = safe_cast<CWorld_PhysState>(m_pWServer);
			CXR_Model_BSP4Glass_Instance* pGlassModelInstance = GetGlassModelInstance(this, m_pWServer);
			if (pGlassModelInstance->Server_CrushGlassSurface(CrushType, iInstance, LocalPosition, Radius, BoxSize, GameTick, GetPositionMatrix(), 0))
			{
				// Create crush net msg with CInfo
				uint8 MsgType = TranslateCrushType(CrushType);
				CNetMsg NetMsg(MsgType);
				NetMsg.AddInt8(1);

				// Add seed, instance and crush flags
				NetMsg.AddInt32(GameTick);
				NetMsg.AddInt32(iInstance);
				NetMsg.AddInt8(0);

				// Add local position
				NetMsg.Addfp32(LocalPosition.k[0]);
				NetMsg.Addfp32(LocalPosition.k[1]);
				NetMsg.Addfp32(LocalPosition.k[2]);

				// Add force and scale
				fp32 RandForce = ForceRange.k[0] + (MFloat_GetRand(GameTick) * (ForceRange.k[1] - ForceRange.k[0]));
				NetMsg.Addfp32(Force.k[0]);
				NetMsg.Addfp32(Force.k[1]);
				NetMsg.Addfp32(Force.k[2]);
				NetMsg.Addfp32(RandForce * ForceScale);

				if (CrushType == GLASS_CRUSH_CUBE)
				{
					NetMsg.Addfp32(BoxSize.k[0]);
					NetMsg.Addfp32(BoxSize.k[1]);
					NetMsg.Addfp32(BoxSize.k[2]);
				}
				else if (CrushType == GLASS_CRUSH_SPHERE)
					NetMsg.Addfp32(Radius);

				// Send message to crush window on client
				m_pWServer->NetMsg_SendToObject(NetMsg, m_iObject);
			}
		}
		return 1;

	case OBJMSG_DAMAGE:
		{
			const CWO_DamageMsg *pMsg = CWO_DamageMsg::GetSafe(_Msg);
			if(pMsg)
			{
				int8 CInfo = (pMsg->m_pCInfo) ? 1 : 0;
				uint8 MsgType = GLASS_MSG_NA;

				// Get model instance
				CWorld_PhysState* pWPhysState = safe_cast<CWorld_PhysState>(m_pWServer);

				// Send server relating damage/crushing
				CXR_Model_BSP4Glass_Instance* pGlassInstance = GetGlassModelInstance(this, pWPhysState);
				if(CInfo && pGlassInstance)
				{
					const int32 LocalNode = pMsg->m_pCInfo->m_LocalNode;
					MsgType = pGlassInstance->Server_OnDamage(LocalNode, pMsg->m_Damage);
				}

				// Initialize message type
				if(MsgType == GLASS_MSG_CRUSH_POINT)
				{
					CNetMsg NetMsg(MsgType);
					NetMsg.AddInt8(CInfo);

					if(CInfo)
					{
						// Get instance hit and pick a seeding value from server game tick
						const int32 LocalNode = pMsg->m_pCInfo->m_LocalNode;
						const int32 Seed = m_pWServer->GetGameTick();
						const uint8 CrushFlags = EvaluateDamageType(pMsg->m_DamageType);
						
						// Add seed and instance
						NetMsg.AddInt32(Seed);
						NetMsg.AddInt32(LocalNode);
						NetMsg.AddInt8(CrushFlags);
						
						// Add collisions local position
						NetMsg.Addfp32(pMsg->m_pCInfo->m_LocalPos.k[0]);
						NetMsg.Addfp32(pMsg->m_pCInfo->m_LocalPos.k[1]);
						NetMsg.Addfp32(pMsg->m_pCInfo->m_LocalPos.k[2]);

						// Add force vector
						NetMsg.Addfp32(pMsg->m_Force.k[0]);
						NetMsg.Addfp32(pMsg->m_Force.k[1]);
						NetMsg.Addfp32(pMsg->m_Force.k[2]);
						NetMsg.Addfp32(MFloat_GetRand(m_pWServer->GetGameTick()) * 1.5f);

						pGlassInstance->Server_CrushGlassSurface(GLASS_CRUSH_POINT, LocalNode, pMsg->m_pCInfo->m_LocalPos, 0.0f, 0.0f, Seed, GetPositionMatrix(), CrushFlags);
					}

					// Send message to ourself so we can crush glass on clients
					m_pWServer->NetMsg_SendToObject(NetMsg, m_iObject);
				}
			}

			return 0;
		}
	}

	return CWObject_Model::OnMessage(_Msg);
}


uint8 CWObject_WorldGlassSpawn::EvaluateDamageType(uint32 _DamageType)
{
	uint8 CrushFlags = 0;
	switch (_DamageType)
	{
	case DAMAGETYPE_BLAST:
		{
			CrushFlags |= GLASS_CRUSHFLAGS_FULLBREAK;
			break;
		}
	}

	return CrushFlags;
}


aint CWObject_WorldGlassSpawn::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_SPAWN_WALLMARK:
		{
			// Get model instance
			CWO_GlassClientData& CD = GetClientData(_pObj);
			CXR_Model_BSP4Glass_Instance* pGlassInstance = GetGlassModelInstance(_pObj, _pWClient);
			CXR_Model* pModel = (_pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]));
			CXR_Model_BSP4Glass* pModelGlass = (pModel) ? safe_cast<CXR_Model_BSP4Glass>(pModel) : NULL;
			if(pGlassInstance && pModelGlass)
			{
				const CXR_WallmarkDesc* pWMD = (const CXR_WallmarkDesc*)_Msg.m_Param0;
				const CMat4Dfp32* pMat = (const CMat4Dfp32*)_Msg.m_Param1;

				pGlassInstance->Wallmark_Create(pModelGlass, _pObj->GetPositionMatrix(), *pWMD, *pMat, 4, XR_WALLMARK_SCANFOROVERLAP, 0);
			}
		}
		return 0;

	default:
		return CWObject_Model::OnClientMessage(_pObj, _pWClient, _Msg);
	}
}


void CWObject_WorldGlassSpawn::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	switch(_Msg.m_MsgType)
	{
	// Break glass, or at least try to
	case GLASS_MSG_CRUSH_POINT:
	case GLASS_MSG_CRUSH_CUBE:
	case GLASS_MSG_CRUSH_SPHERE:
		{
			int iPos = 0;
			
			// Do we have collision info?
			int8 CInfo = _Msg.GetInt8(iPos);

			if(CInfo)
			{
				CVec3Dfp32 BoxSize = 0.0f;
				fp32 Radius = 0.0f;
				fp32 ForceScale;
				CVec3Dfp32 LocalPosition, Force;

				// Get crush properties
				int32 Seed = _Msg.GetInt32(iPos);
				int32 iInstance = _Msg.GetInt32(iPos);
				uint8 CrushFlags = _Msg.GetInt8(iPos);

				// Get local collision point
				LocalPosition.k[0] = _Msg.Getfp32(iPos);
				LocalPosition.k[1] = _Msg.Getfp32(iPos);
				LocalPosition.k[2] = _Msg.Getfp32(iPos);

				// Get force
				Force.k[0] = _Msg.Getfp32(iPos);
				Force.k[1] = _Msg.Getfp32(iPos);
				Force.k[2] = _Msg.Getfp32(iPos);
				ForceScale = _Msg.Getfp32(iPos);

				uint8 CrushType = TranslateCrushMsg(_Msg.m_MsgType);
				if (CrushType == GLASS_CRUSH_CUBE)
				{
					BoxSize.k[0] = _Msg.Getfp32(iPos);
					BoxSize.k[1] = _Msg.Getfp32(iPos);
					BoxSize.k[2] = _Msg.Getfp32(iPos);
				}
				else if (CrushType == GLASS_CRUSH_SPHERE)
				{
					Radius = _Msg.Getfp32(iPos);
				}

				// Get model instance
				CXR_Model_BSP4Glass_Instance* pGlassInstance = GetGlassModelInstance(_pObj, _pWClient);
				if(pGlassInstance)
				{
					const int8 iPlaySound = pGlassInstance->Client_CrushGlassSurface(CrushType, _pObj->GetPositionMatrix(), Seed, iInstance, Seed, LocalPosition, Force, ForceScale, Radius, BoxSize, CrushFlags);
					int iSound = -1;
					switch(iPlaySound)
					{
					case 0:
						iSound = _pWClient->GetMapData()->GetResourceIndex_Sound("env_glass_wndwmed_crack_01");
						break;

					case 1:
						iSound = _pWClient->GetMapData()->GetResourceIndex_Sound("env_glass_wndwmed_01");
						break;
					
					case 2:
						iSound = _pWClient->GetMapData()->GetResourceIndex_Sound("env_glass_wndwmed_loose_01");
						break;
					}

					// Play sound
					if(iSound >= 0)
					{
						CVec3Dfp32 SoundPos = LocalPosition * _pObj->GetPositionMatrix();
						_pWClient->Sound_At(WCLIENT_CHANNEL_SFX, SoundPos, iSound, WCLIENT_ATTENUATION_3D, 0);
					}
				}
			}
		}
		break;

	// Restore glass instace
	case GLASS_MSG_RESTORE:
		{
			// Get model instance
			CWO_GlassClientData& CD = GetClientData(_pObj);
			CXR_Model_BSP4Glass_Instance* pGlassInstance = GetGlassModelInstance(_pObj, _pWClient);

			int iPos = 0;
			uint16 iInstance = _Msg.GetInt16(iPos);
			if(pGlassInstance)
			{
				pGlassInstance->Client_Restore(iInstance);
			}
		}
		break;

	// Restore all glass instances
	case GLASS_MSG_RESTORE_ALL:
		{
			// Get model instance
			CXR_Model_BSP4Glass_Instance* pGlassInstance = GetGlassModelInstance(_pObj, _pWClient);
			if (pGlassInstance)
				pGlassInstance->Client_RestoreAll();
		}
		break;

	case GLASS_MSG_ACTIVE:
		{
			int iPos = 0;
			uint16 iInstance = _Msg.GetInt16(iPos);
			bool bInactive = (_Msg.GetInt8(iPos) != 0);

			// Get model instance
			CXR_Model_BSP4Glass_Instance* pGlassInstance = GetGlassModelInstance(_pObj, _pWClient);
			if (pGlassInstance)
				pGlassInstance->Client_Inactive(iInstance, bInactive);

			break;
		}

	default:
		break;
	}
}


uint8 CWObject_WorldGlassSpawn::TranslateCrushMsg(uint8 _CrushMsg)
{
	uint8 CrushType = GLASS_CRUSH_POINT;
	switch (_CrushMsg)
	{
	case GLASS_MSG_CRUSH_CUBE:
		CrushType = GLASS_CRUSH_CUBE;
		break;

	case GLASS_MSG_CRUSH_POINT:
		CrushType = GLASS_CRUSH_POINT;
		break;

	case GLASS_MSG_CRUSH_SPHERE:
		CrushType = GLASS_CRUSH_SPHERE;
		break;
	}

	return CrushType;
}


uint8 CWObject_WorldGlassSpawn::TranslateCrushType(uint8 _CrushType)
{
	uint8 CrushMsg = GLASS_MSG_CRUSH_POINT;
	switch (_CrushType)
	{
	case GLASS_CRUSH_CUBE:
		CrushMsg = GLASS_MSG_CRUSH_CUBE;
		break;

	case GLASS_CRUSH_POINT:
		CrushMsg = GLASS_MSG_CRUSH_POINT;
		break;

	case GLASS_CRUSH_SPHERE:
		CrushMsg = GLASS_MSG_CRUSH_SPHERE;
		break;
	}

	return CrushMsg;
}


bool CWObject_WorldGlassSpawn::OnIntersectLine(CWO_OnIntersectLineContext& _Context, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWObject_OnIntersectLine, false);
	CWObject_CoreData* _pObj = _Context.m_pObj;
	if (!_pObj) return false;
//	if (!((pObj->GetPhysState().m_ObjectFlags | pObj->GetPhysState().m_ObjectIntersectFlags) & _ObjectFlags)) return false;
//	if (!(pObj->GetPhysState().m_ObjectFlags & _ObjectFlags)) return false;

	CWorld_PhysState* _pWPhysState = _Context.m_pPhysState;

//	CWObject_WorldGlassSpawn* pGlassObj = (CWObject_WorldGlassSpawn*)_pObj;

	bool bImpact = false;

	const CWO_PhysicsState& PhysState = _pObj->GetPhysState();

	for(int iPrim = 0; iPrim < PhysState.m_nPrim; iPrim++)
	{
		const CWO_PhysicsPrim& PhysPrim = PhysState.m_Prim[iPrim];

		CXR_PhysicsModel* pPhysModel = NULL;

		const CMat4Dfp32* pPos = &_pObj->GetPositionMatrix();
		CMat4Dfp32 Pos;
		if (PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_OFFSET)
		{
			Pos = *pPos;
			CVec3Dfp32 Ofs = PhysPrim.GetOffset();
			if (PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION)
				Ofs.MultiplyMatrix3x3(*pPos);
			CVec3Dfp32::GetMatrixRow(Pos, 3) += Ofs;
			pPos = &Pos;
		}

		CXR_PhysicsContext PhysContext;
		switch(PhysPrim.m_PrimType)
		{
		case OBJECT_PRIMTYPE_PHYSMODEL :
			{
				CWO_GlassClientData& CD = GetClientData(_pObj);
				CXR_ModelInstance* pModelInstance = GetModelInstance(CD, _pObj, _pWPhysState);
				CXR_Model* pModel = _pWPhysState->GetMapData()->GetResource_Model(PhysPrim.m_iPhysModel);
				if (!pModel) continue;
				pPhysModel = pModel->Phys_GetInterface();

				CXR_AnimState Anim(CMTime::CreateFromTicks(_pWPhysState->GetGameTick() - _pObj->m_CreationGameTick, _pWPhysState->GetGameTickTime(), -_pObj->m_CreationGameTickFraction), CMTime(), _pObj->m_iAnim0, _pObj->m_iAnim1, pModelInstance, 0);
				Anim.m_Data[GLASS_DATA_GAMETICK] = _pWPhysState->GetGameTick() - _pObj->m_CreationGameTick;
				PhysContext = CXR_PhysicsContext(*pPos, &Anim);
				PhysContext.m_PhysGroupMaskThis = PhysPrim.m_PhysModelMask;
				pPhysModel->Phys_Init(&PhysContext);
			}
			break;

		case OBJECT_PRIMTYPE_SPHERE :
		case OBJECT_PRIMTYPE_BOX :
			return CWObject::OnIntersectLine(_Context, _pCollisionInfo);

		/*
		case OBJECT_PRIMTYPE_SPHERE :
			{
				_pWPhysState->m_PhysModel_Sphere.Phys_SetDimensions(pObj->GetPhysState().m_Prim[iPrim].m_Dimensions[0]);
				PhysContext = CXR_PhysicsContext(*pPos);
				_pWPhysState->m_PhysModel_Sphere.Phys_Init(&PhysContext);
				pPhysModel = &_pWPhysState->m_PhysModel_Sphere;
			}
			break;

		case OBJECT_PRIMTYPE_BOX :
			{
				_pWPhysState->m_PhysModel_Box.Phys_SetDimensions(pObj->GetPhysState().m_Prim[iPrim].GetDim());
				PhysContext = CXR_PhysicsContext(*pPos);
				_pWPhysState->m_PhysModel_Box.Phys_Init(&PhysContext);
				pPhysModel = &_pWPhysState->m_PhysModel_Box;
			}
			break;
		*/

		default :
			break;
		}

		if (pPhysModel)
		{
			if(pPhysModel->Phys_IntersectLine(&PhysContext, _Context.m_p0, _Context.m_p1, _Context.m_MediumFlags, _pCollisionInfo))
			{
				if (!_pCollisionInfo)
					return true;
				_pCollisionInfo->m_iObject = _pObj->m_iObject;
				if (_pCollisionInfo->IsComplete())
					return false;

#ifdef _DEBUG
				if (_pCollisionInfo->m_bIsValid && (_pCollisionInfo->m_Time < -0.01f || _pCollisionInfo->m_Time > 1.01f))
				{
					ConOut(CStrF("(CWorld_PhysState::Object_IntersectLine) Entity %d,  Phys_IntersectLine returned T = %f", _pObj->m_iObject, _pCollisionInfo->m_Time));
//					pPhysModel->Phys_IntersectLine(_p0, _p1, _MediumFlags, (_pCollisionInfo) ? &CInfo : NULL);
				}
#endif
				bImpact = true;
			}
		}
	}

	return bImpact;
}


void CWObject_WorldGlassSpawn::OnLoad(CCFile* _pFile)
{
	CWObject_Model::OnLoad(_pFile);
}


void CWObject_WorldGlassSpawn::OnSave(CCFile* _pFile)
{
	CWObject_Model::OnSave(_pFile);
}


void CWObject_WorldGlassSpawn::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	// Load glass instance data (disabled, have to fix instance loading)
	CXR_Model_BSP4Glass_Instance* pGlassModelInstance = GetGlassModelInstance(this, m_pWServer);
	//pGlassModelInstance->OnDeltaLoad(_pFile);

	CWObject_Model::OnDeltaLoad(_pFile, _Flags);
}


void CWObject_WorldGlassSpawn::OnDeltaSave(CCFile* _pFile)
{
	// Save glass instance data (disabled, have to fix instance saving)
	CXR_Model_BSP4Glass_Instance* pGlassModelInstance = GetGlassModelInstance(this, m_pWServer);
	//pGlassModelInstance->OnDeltaSave(_pFile);

	CWObject_Model::OnDeltaSave(_pFile);
}


void CWObject_WorldGlassSpawn::OnFinishDeltaLoad()
{
	CWObject_Model::OnFinishDeltaLoad();
}


// -------------------------------------------------------------------
//  DYNAMIC GLASS
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Glass_Dynamic, CWObject_WorldGlassSpawn, 0x0100);
CWObject_Glass_Dynamic::CWObject_Glass_Dynamic()
	: CWObject_WorldGlassSpawn()
{
	m_ClientFlags |= (CWO_GLASS_CLIENTFLAGS_NOLINK | CWO_GLASS_CLIENTFLAGS_DYNAMIC);
	m_ClientFlags &= ~CWO_CLIENTFLAGS_LINKINFINITE;
}


void CWObject_Glass_Dynamic::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_Glass_Dynamic_OnFinishEvalKeys, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_Glass_Dynamic::OnFinishEvalKeys, WOBJ_SYSTEM);

	CWObject_WorldGlassSpawn::OnFinishEvalKeys();

	m_pWServer->Phys_InsertPosition(m_iObject, this);
	m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);

	UpdateVisibility();
}


void CWObject_Glass_Dynamic::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Glass_Dynamic_OnClientRender, MAUTOSTRIP_VOID);

	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

	OnClientRenderGlass(_pObj, _pWClient, _pEngine, _ParentMat, MatIP);
}


// -------------------------------------------------------------------
//  WORLDSKY
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_WorldSky, CWObject_Model, 0x0100);

CWObject_WorldSky::CWObject_WorldSky()
{
	MAUTOSTRIP(CWObject_WorldSky_ctor, MAUTOSTRIP_VOID);

	m_ClientFlags |= CWO_CLIENTFLAGS_LINKINFINITE | CWO_CLIENTFLAGS_VISIBILITY;
	m_Data[0] = 65535;
	m_Data[1] = 100000;
	m_Data[2] = 100010;
//	m_Data[3] = 0xff3f3f4f;
	m_Data[3] = 0;

	m_Data[4] = 65535;
	m_Data[5] = 0;
	m_Data[6] = 500;
	m_Data[7] = 0xff2c3236;

	m_iAnim1 = 0;
	m_iAnim2 = 0;
}

void CWObject_WorldSky::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_WorldSky_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();

	const fp32 Valuef = _Value.Val_fp64();

	switch (_KeyHash)
	{
	case MHASH3('FOG_','SEAL','EVEL'): // "FOG_SEALEVEL"
		{
	//		m_Data[0] = _Value.Val_fp64();
			break;
		}
	case MHASH3('FOG_','DISA','BLE'): // "FOG_DISABLE"
		{
			m_iAnim0 |= 1;
			break;
		}

	case MHASH4('FOG_','DEPT','HNEA','R'): // "FOG_DEPTHNEAR"
		{
			m_Data[1] = int32(Valuef);
			break;
		}
	case MHASH3('FOG_','DEPT','HFAR'): // "FOG_DEPTHFAR"
		{
			m_Data[2] = int32(Valuef);
			break;
		}
	case MHASH4('FOG_','DEPT','HCOL','OR'): // "FOG_DEPTHCOLOR"
		{
			CPixel32 Color;
			Color.Parse(_Value);
			m_Data[3] = Color;
			break;
		}
	case MHASH4('FOG_','DEPT','HDEN','SITY'): // "FOG_DEPTHDENSITY"
		{
			fp32 Density = Clamp01(Valuef);
			m_Data[0] = int32(Density*65535);
			break;
		}

	case MHASH5('FOGW','ATER','_DEP','THNE','AR'): // "FOGWATER_DEPTHNEAR"
		{
			m_Data[5] = int32(Valuef);
			break;
		}
	case MHASH5('FOGW','ATER','_DEP','THFA','R'): // "FOGWATER_DEPTHFAR"
		{
			m_Data[6] = int32(Valuef);
			break;
		}
	case MHASH5('FOGW','ATER','_DEP','THCO','LOR'): // "FOGWATER_DEPTHCOLOR"
		{
			CPixel32 Color;
			Color.Parse(_Value);
			m_Data[7] = Color;
			break;
		}
	case MHASH6('FOGW','ATER','_DEP','THDE','NSIT','Y'): // "FOGWATER_DEPTHDENSITY"
		{
			fp32 Density = Clamp01(Valuef);
			m_Data[4] = int32(Density*65535);
			break;
		}

	case MHASH4('XR_F','OGCU','LLOF','FSET'): // "XR_FOGCULLOFFSET"
		{
			m_iAnim2 = _Value.Val_int();
			break;
		}

	case MHASH3('SKY_','INDO','OR'): // "SKY_INDOOR"
		{
			m_iAnim0 |= 2;
			break;
		}
	case MHASH2('SKY_','NAME'): // "SKY_NAME"
		{
			m_iModel[0] = m_pWServer->GetMapData()->GetResourceIndex("CMF:Sky:Skies\\" + _Value + ".xrg");
			break;
		}
	case MHASH3('VP_B','ACKP','LANE'): // "VP_BACKPLANE"
		{
			m_iAnim1 = _Value.Val_int();
			break;
		}
	default:
		{
			CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_WorldSky::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_WorldSky_OnFinishEvalKeys, MAUTOSTRIP_VOID);

	if(!m_iModel[0])
		m_iModel[0] = m_pWServer->GetMapData()->GetResourceIndex("CMF:Sky:Skies\\Sky_Default.xrg");

	CWObject_Model::OnFinishEvalKeys();
}

void CWObject_WorldSky::OnCreate()
{
	MAUTOSTRIP(CWObject_WorldSky_OnCreate, MAUTOSTRIP_VOID);
}

void CWObject_WorldSky::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CWObject_WorldSky_OnIncludeClass, MAUTOSTRIP_VOID);
//	_pWData->GetResourceIndex_Model("SKY");
}

void CWObject_WorldSky::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_WorldSky_OnClientRender, MAUTOSTRIP_VOID);
}

void CWObject_WorldSky::OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_WorldSky_OnClientRenderVis, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_WorldSky::OnClientRenderVis);

	CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);

	CMat4Dfp32 Cam;
	_pWClient->Render_GetLastRenderCamera(Cam);
	CXR_MediumDesc Medium;
	_pWClient->Phys_GetMedium(CVec3Dfp32::GetRow(Cam, 3), &Medium);

// ConOut(CStrF("Medium %.8x, FogEnd %f, FogColor %.8x", Medium.m_MediumFlags, 1.0f / Medium.m_FogAttenuation, Medium.m_FogColor));

	CRC_Viewport VP;
	_pWClient->Render_GetLastRenderViewport(VP);

	if (_pObj->m_iAnim0 & 1)
	{
		fp32 FogEnd = VP.GetBackPlane();
		int iFogData = (Medium.m_MediumFlags & XW_MEDIUM_LIQUID) ? 4 : 0;

		if (Medium.m_MediumFlags & XW_MEDIUM_DEPTHFOG)
		{
			FogEnd = 1.0f / Medium.m_FogAttenuation;
			uint32 FogColor = Medium.m_FogColor;
			fp32 FogDensity = Medium.m_FogDensity;
			_pEngine->GetFogState()->DepthFog_Init(0, FogEnd, FogColor, FogDensity);
		}
		else
		{
			CDepthFog Fog;
			CWObject_Message Msg(OBJMSG_DEPTHFOG_GET);
			Msg.m_pData = &Fog;
			CPixel32 Color;
			if(Medium.m_MediumFlags & XW_MEDIUM_LIQUID || !_pWClient->ClientMessage_SendToObject(Msg, _pWClient->Player_GetLocalObject()))
			{
				Fog.m_Near = _pObj->m_Data[iFogData + 1];
				FogEnd = _pObj->m_Data[iFogData + 2];
				Color = _pObj->m_Data[iFogData + 3];
				Fog.m_Density = _pObj->m_Data[iFogData + 0] >> 1;
			}
			else
			{
				Color = (Fog.m_Color[0] << 17) | (Fog.m_Color[1] << 9) | (Fog.m_Color[2] << 1);
				FogEnd = Fog.m_Far;
			}

			_pEngine->GetFogState()->DepthFog_Init(Fog.m_Near, FogEnd, Color, fp32(Fog.m_Density) * (1.0f / 32627.0f));
		}

		fp32 FogCullOffset = _pObj->m_iAnim2;
		if (iFogData || Medium.m_MediumFlags & XW_MEDIUM_DEPTHFOG)
			FogCullOffset += Max(0.0f, VP.GetBackPlane() - FogEnd);

		_pEngine->SetVarf(XR_ENGINE_FOGCULLOFFSET, FogCullOffset);

	}
	else
	{
		CPixel32 Color(_pObj->m_Data[3]);
//		Color *= 0.5f;
		_pEngine->GetFogState()->VertexFog_Init(_pEngine->GetVC()->m_CameraWMat, 4096, 1024, Color, _pObj->m_Data[0]);
		_pEngine->GetFogState()->DepthFog_Init(100000, 100010, 0xffffffff);
	}

//	fp32 IPTime = _pObj->GetAnimTick(_pWClient)*SERVER_TIMEPERFRAME + _pWClient->GetInterpolateTime();
//	if (_pObj->m_iAnim0 & 2)
	{
		_pEngine->Render_AddModel(
			pModel, 
			_pObj->GetPositionMatrix(), 
			_pObj->GetDefaultAnimState(_pWClient),
			XR_MODEL_SKY
		);
	}
/*	else
	{
		_pEngine->Render_AddModel(pModel, _pObj->GetPositionMatrix(), 
			CXR_AnimState(_pObj->AnimTick(_pWClient) + IPTime, _pObj->m_iAnim0, 0, &_pObj->m_lspClientObj[0]),
			XR_MODEL_SKY);
//		_pEngine->Render_AddModel(pModel, _pObj->GetPositionMatrix(), 
//			CXR_AnimState(_pObj->AnimTick(_pWClient) + IPTime, _pObj->m_iAnim0, 1, &_pObj->m_lspClientObj[0]));
	}*/
}

void CWObject_WorldSky::CDepthFog::Parse(const char *_pSt)
{
	CFStr St = _pSt;
	m_Near = St.GetStrSep(",").Val_int();
	m_Far = St.GetStrSep(",").Val_int();
	int Color = St.GetStrSep(",").Val_int();
	m_Color[0] = (Color >> 17) & 0x7f;
	m_Color[1] = (Color >> 9) & 0x7f;
	m_Color[2] = (Color >> 1) & 0x7f;
	m_Density = int16(St.GetStrSep(",").Val_fp64() * 32767.0f);
}

bool CWObject_WorldSky::CDepthFog::operator !=(const CDepthFog &_Fog) const
{
	return m_Near != _Fog.m_Near || m_Far != _Fog.m_Far || m_Color != _Fog.m_Color || m_Density != _Fog.m_Density;
}

void CWObject_WorldSky::CDepthFog::operator =(int _Val)
{
	// Setting default
	m_Near = 100000;
	m_Far = 100010;
	m_Color = 0;
	m_Density = 32767;
}

CWObject_WorldSky::CDepthFog CWObject_WorldSky::CDepthFog::operator +(const CDepthFog &_Fog) const
{
	CDepthFog Fog;
	Fog.m_Near = m_Near + _Fog.m_Near;
	Fog.m_Far = m_Far + _Fog.m_Far;
	Fog.m_Color = m_Color + _Fog.m_Color;
	Fog.m_Density = m_Density + _Fog.m_Density;
	return Fog;
}

CWObject_WorldSky::CDepthFog CWObject_WorldSky::CDepthFog::operator -(const CDepthFog &_Fog) const
{
	CDepthFog Fog;
	Fog.m_Near = m_Near - _Fog.m_Near;
	Fog.m_Far = m_Far - _Fog.m_Far;
	Fog.m_Color = m_Color - _Fog.m_Color;
	Fog.m_Density = m_Density - _Fog.m_Density;
	return Fog;
}

CWObject_WorldSky::CDepthFog CWObject_WorldSky::CDepthFog::operator *(fp32 _Val) const
{
	CDepthFog Fog;
	Fog.m_Near = int32(m_Near * _Val);
	Fog.m_Far = int32(m_Far * _Val);
	Fog.m_Color[0] = RoundToInt(m_Color[0] * _Val);
	Fog.m_Color[1] = RoundToInt(m_Color[1] * _Val);
	Fog.m_Color[2] = RoundToInt(m_Color[2] * _Val);
	Fog.m_Density = int16(m_Density * _Val);
	return Fog;
}

void CWObject_WorldSky::CDepthFog::Pack(uint8*& _pD) const
{
	TAutoVar_Pack(m_Near, _pD);
	TAutoVar_Pack(m_Far, _pD);
	TAutoVar_Pack(m_Color, _pD);
	TAutoVar_Pack(m_Density, _pD);
}

void CWObject_WorldSky::CDepthFog::Unpack(const uint8*& _pD)
{
	TAutoVar_Unpack(m_Near, _pD);
	TAutoVar_Unpack(m_Far, _pD);
	TAutoVar_Unpack(m_Color, _pD);
	TAutoVar_Unpack(m_Density, _pD);
}

// -------------------------------------------------------------------
//  SIDESCENE
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_SideScene, CWObject_Model, 0x0100);

void CWObject_SideScene::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Trigger_OnEvalKey, MAUTOSTRIP_VOID);

	switch (_KeyHash)
	{
	case MHASH2('MODE','L'): // "MODEL"
	case MHASH2('MODE','L0'): // "MODEL0"
		{
			Model_Set(0, _pKey->GetThisValue(), false);
			break;
		}
	default:
		{
			CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_SideScene::OnFinishEvalKeys()
{
	CWObject_Model::OnFinishEvalKeys();

	ClientFlags() |= CWO_CLIENTFLAGS_LINKINFINITE | CWO_CLIENTFLAGS_NOREFRESH;
}

void CWObject_SideScene::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if (pModel)
	{
		CXR_AnimState AnimState(_pObj->GetDefaultAnimState(_pWClient));
		_pEngine->Render_AddModel(pModel, _pObj->GetPositionMatrix(), AnimState, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_NODYNAMICLIGHT);
	}
}

// -------------------------------------------------------------------
//  FUNC_USERPORTAL
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Func_UserPortal, CWObject, 0x0100);

CWObject_Func_UserPortal::CWObject_Func_UserPortal()
{
	MAUTOSTRIP(CWObject_Func_UserPortal_ctor, MAUTOSTRIP_VOID);

	m_iAnim2 = 1;		// Start open
	m_iAnim0 = -1;		// Invalid portal index
	m_iAnim1 = 32767;	// Closing distance
	m_ClientFlags |= CWO_CLIENTFLAGS_VISIBILITY;
	m_NumControllers = 0;
}

void CWObject_Func_UserPortal::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Func_UserPortal_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();
	
	const int Valuei = _Value.Val_int();
	const fp32 Valuef = _Value.Val_fp64();
	
	switch (_KeyHash)
	{
	case MHASH3('USER','PORT','AL'): // "USERPORTAL"
		{
			m_iAnim0 = Valuei;
			break;
		}
	case MHASH3('PORT','ALST','ATE'): // "PORTALSTATE"
		{
			m_iAnim2 = Valuei;
			break;
		}
	case MHASH5('PORT','ALCL','OSED','ISTA','NCE'): // "PORTALCLOSEDISTANCE"
		{
			m_iAnim1 = int16(Valuef);
			break;
		}
	default:
		{
			CWObject::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

aint CWObject_Func_UserPortal::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Func_UserPortal_OnMessage, 0);

	switch(_Msg.m_Msg)
	{
	case OBJMSG_FUNCPORTAL_SETCONTROLLER:
		{
			if(m_NumControllers < MAXCONTROLLERS)
			{
				m_lControllers[m_NumControllers] = _Msg.m_iSender;
				m_lControllerState[m_NumControllers++] = _Msg.m_Param0;
			}
			// No break. Update state
//			break;
		}

	case OBJMSG_FUNCPORTAL_SETSTATE :
	case OBJMSG_IMPULSE:
		{
			if(m_NumControllers != 0)
			{
				// Controller mode
				uint8 Open = 0;
				for(int i = 0; i < m_NumControllers; i++)
				{
					if(_Msg.m_iSender == m_lControllers[i])
						m_lControllerState[i] = _Msg.m_Param0 & 1;

					Open |= m_lControllerState[i];
				}

				iAnim2() = Open;
			}
			else
				// Normal mode
				iAnim2() = _Msg.m_Param0 & 1;
			return 1;
		}

	default :
		return CWObject::OnMessage(_Msg);
	}
}

void CWObject_Func_UserPortal::OnDeltaSave(CCFile* _pFile)
{
	int i;
	for (i=0; i<MAXCONTROLLERS; i++)
	{
		_pFile->WriteLE(m_lControllers[i]);
		_pFile->WriteLE(m_lControllerState[i]);
	}
	int32 t = iAnim2();
	_pFile->WriteLE(t);
}

void CWObject_Func_UserPortal::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	int i;
	for (i=0; i<MAXCONTROLLERS; i++)
	{
		_pFile->ReadLE(m_lControllers[i]);
		_pFile->ReadLE(m_lControllerState[i]);
	}
	int32 t;
	_pFile->ReadLE(t);
	iAnim2() = t;
}

void CWObject_Func_UserPortal::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Func_UserPortal_OnClientRefresh, MAUTOSTRIP_VOID);
}

void CWObject_Func_UserPortal::OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Func_UserPortal_OnClientRenderVis, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Func_UserPortal::OnClientRenderVis);
//ConOut(CStrF("(CWObject_Func_UserPortal::OnClientRender) %d, %d", _pObj->m_iAnim0, _pObj->m_iAnim2));

	// Do we have a portal ID?
	if (_pObj->m_iAnim0 > 0) 
	{
		CXR_ViewClipInterface* pViewClip = _pEngine->GetVC()->m_pViewClip;
		if (!pViewClip) return;

		int State = _pObj->m_iAnim2;

		// Is this portal limited by distance, and is it open?
		if (_pObj->m_iAnim1 < 32767 && State)
		{
			// Check distance
			fp32 dSqr = CVec3Dfp32::GetMatrixRow(_pEngine->GetVC()->m_CameraWMat, 3).DistanceSqr(_pObj->GetPosition());
			if (dSqr > Sqr(fp32(_pObj->m_iAnim1))) State = 0;
		}

		// Set portal state using the current viewclip interface, also taking the recursion depth into account.
//ConOut(CStrF("Client PSet %d, %d", _pObj->m_iAnim0, State));
		pViewClip->View_SetState(_pEngine->GetVCDepth(), XR_VIEWCLIPSTATE_PORTAL, _pObj->m_iAnim0 + (State << 16));
	}
}

#ifndef M_DISABLE_CWOBJECT_FUNC_PORTAL
// -------------------------------------------------------------------
//  FUNC_PORTAL
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Func_Portal, CWObject, 0x0100);

CWObject_Func_Portal::CWObject_Func_Portal()
{
	MAUTOSTRIP(CWObject_Func_Portal_ctor, MAUTOSTRIP_VOID);

	m_iAnim0 = 48;
	m_iAnim1 = 64;
//	m_iAnim2 = 0x4448;
}

void CWObject_Func_Portal::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Func_Portal_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();

	const int Valuei = _Value.Val_int();
	const fp32 Valuef = _Value.Val_fp64();
	
	switch (_KeyHash)
	{
	case MHASH3('PORT','AL_W','IDTH'): // "PORTAL_WIDTH"
		{
			m_iAnim0 = int16(Valuef);
			break;
		}
	case MHASH4('PORT','AL_H','EIGH','T'): // "PORTAL_HEIGHT"
		{
			m_iAnim1 = int16(Valuef);
			break;
		}
	case MHASH3('PORT','AL_C','OLOR'): // "PORTAL_COLOR"
		{
	//		m_iAnim2 = CImage::ConvToRGBA4444(_Value.Val_int());
			break;
		}
	case MHASH3('PORT','AL_F','LIP'): // "PORTAL_FLIP"
		{
			m_Data[0] = Valuei;
			break;
		}
	case MHASH4('PORT','AL_S','URFA','CE'): // "PORTAL_SURFACE"
		{
	//		m_iModel[0] = m_pWServer->m_pWData->GetResource_SurfaceIndex(_pKey->.GetValue());
			break;
		}
	case MHASH2('TARG','ET'): // "TARGET"
		{
			m_Target = m_pWServer->World_MangleTargetName(_pKey->GetThisValue());
			break;
		}
	default:
		{
			CWObject::OnEvalKey(_pKey);
			break;
		}
	}
}

aint CWObject_Func_Portal::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Func_Portal_OnMessage, 0);

	return CWObject::OnMessage(_Msg);
}

void CWObject_Func_Portal::OnRefresh()
{
	MAUTOSTRIP(CWObject_Func_Portal_OnRefresh, MAUTOSTRIP_VOID);

	if (m_iAnim2)
	{
		m_ClientFlags |= CWO_CLIENTFLAGS_NOREFRESH;
		return;
	}

	TSelection<CSelection::LARGE_BUFFER> Selection;

		m_pWServer->Selection_AddTarget(Selection, m_Target);
		const int16* pRetList;
		int nSel = m_pWServer->Selection_Get(Selection, &pRetList);
		m_iAnim2 = (nSel) ? pRetList[0] : 0;

}

void CWObject_Func_Portal::OnLoad(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Func_Portal_OnLoad, MAUTOSTRIP_VOID);

	CWObject::OnLoad(_pFile);
	int16 nVPortals;
	_pFile->ReadLE(nVPortals);
	m_lVPortal.SetLen(nVPortals);
	for(int p = 0; p < nVPortals; p++)
		m_lVPortal[p].Read(_pFile);
}

void CWObject_Func_Portal::OnSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Func_Portal_OnSave, MAUTOSTRIP_VOID);

	CWObject::OnSave(_pFile);
	int16 nVPortals = m_lVPortal.Len();
	_pFile->WriteLE(nVPortals);
	for(int p = 0; p < nVPortals; p++)
		m_lVPortal[p].Write(_pFile);
}

void CWObject_Func_Portal::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Func_Portal_OnClientRefresh, MAUTOSTRIP_VOID);
}

// -------------------------------------------------------------------
// Move to CVec3Dfp32 or CPlane3Dfp32!
// -------------------------------------------------------------------
/*static void ReflectMatrix3x3(const CPlane3Dfp32& _Plane, const CMat4Dfp32& _Mat, CMat4Dfp32& _RefMat)
{
	MAUTOSTRIP(ReflectMatrix3x3, MAUTOSTRIP_VOID);

	// Reflects _Mat in _Plane and puts the result in _RefMat
	_RefMat.Unit();
	CVec3Dfp32::GetMatrixRow(_Mat, 0).Reflect(_Plane.n, CVec3Dfp32::GetMatrixRow(_RefMat, 0));
	CVec3Dfp32::GetMatrixRow(_Mat, 1).Reflect(_Plane.n, CVec3Dfp32::GetMatrixRow(_RefMat, 1));
	CVec3Dfp32::GetMatrixRow(_Mat, 2).Reflect(_Plane.n, CVec3Dfp32::GetMatrixRow(_RefMat, 2));
}

static void ReflectMatrix(const CPlane3Dfp32& _Plane, const CMat4Dfp32& _Mat, CMat4Dfp32& _RefMat)
{
	MAUTOSTRIP(ReflectMatrix, MAUTOSTRIP_VOID);

	// Reflects _Mat in _Plane and puts the result in _RefMat
	_RefMat.Unit();
	CVec3Dfp32::GetMatrixRow(_Mat, 0).Reflect(_Plane.n, CVec3Dfp32::GetMatrixRow(_RefMat, 0));
	CVec3Dfp32::GetMatrixRow(_Mat, 1).Reflect(_Plane.n, CVec3Dfp32::GetMatrixRow(_RefMat, 1));
	CVec3Dfp32::GetMatrixRow(_Mat, 2).Reflect(_Plane.n, CVec3Dfp32::GetMatrixRow(_RefMat, 2));
	CVec3Dfp32 Pos(CVec3Dfp32::GetMatrixRow(_Mat, 3));
	Pos -= _Plane.n*(2.0f*_Plane.Distance(Pos));
	CVec3Dfp32::GetMatrixRow(_RefMat, 3) = Pos;
}*/


void CWObject_Func_Portal::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Func_Portal_OnClientRender, MAUTOSTRIP_VOID);

	CWObject_Client* pDest = _pWClient->Object_Get(_pObj->m_iAnim2);

	if (pDest)
	{
		CVec3Dfp32 lVPortal[4];
		lVPortal[3] = CVec3Dfp32(0, -_pObj->m_iAnim0, _pObj->m_iAnim1);
		lVPortal[2] = CVec3Dfp32(0, -_pObj->m_iAnim0, -_pObj->m_iAnim1);
		lVPortal[1] = CVec3Dfp32(0, _pObj->m_iAnim0, -_pObj->m_iAnim1);
		lVPortal[0] = CVec3Dfp32(0, _pObj->m_iAnim0, _pObj->m_iAnim1);

		CMat4Dfp32 Dest = pDest->GetPositionMatrix();
		CMat4Dfp32 Src = _pObj->GetPositionMatrix();

		// Flip X
		if (_pObj->m_Data[0])
		{
			CPlane3Dfp32 Plane;
			Plane.Create(lVPortal[0], lVPortal[2], lVPortal[1]);

			CMat4Dfp32 DestFlip;
			Plane.ReflectMatrix3x3(DestFlip, Dest); //ReflectMatrix3x3(Plane, Dest, DestFlip);
			
			Dest = DestFlip;

//			Swap(lVPortal[0], lVPortal[3]);
//			Swap(lVPortal[1], lVPortal[2]);
		}


		CMat4Dfp32 dCam, SrcInv, PortalCam;
		Src.InverseOrthogonal(SrcInv);
		SrcInv.Multiply(Dest, dCam);
//		CXR_ViewContext* pVC = _pEngine->GetVC();
//		pVC->m_CameraWMat.Multiply(dCam, PortalCam);

//		PortalCam = pVC->m_CameraWMat;
//		PortalCam.k[3][0] += 192.0f;

		CMat4Dfp32 Rot; Rot.Unit();
		Rot = _pObj->GetPositionMatrix();

		CVec3Dfp32 Portal[32];
		CVec3Dfp32::MultiplyMatrix(lVPortal, Portal, Rot, 4);

//		CPixel32 Col32 = CImage::ConvFromRGBA4444(_pObj->m_iAnim2);
//		_pEngine->Render_AddPortal(Portal, 4, dCam, CPixel32(0x003f3f6f));
		// FIXME WMat and VMat needs to be supplied
	}
}
#endif

// -------------------------------------------------------------------
//  FUNC_MIRROR
// -------------------------------------------------------------------
#ifndef M_DISABLE_CWOBJECT_FUNC_MIRROR
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Func_Mirror, CWObject, 0x0100);

void CWObject_Func_Mirror::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Func_Mirror_OnClientRender, MAUTOSTRIP_VOID);

//	TArray<CVec3Dfp32> m_lVPortal;
	CVec3Dfp32 m_lVPortal[4];
//	m_lVPortal.SetLen(4);
/*	m_lVPortal[4] = CVec3Dfp32(-128, 128, 0);
	m_lVPortal[3] = CVec3Dfp32(-128, -128, 0);
	m_lVPortal[2] = CVec3Dfp32(0, -256, 0);
	m_lVPortal[1] = CVec3Dfp32(128, -128, 0);
	m_lVPortal[0] = CVec3Dfp32(128, 128, 0);
*/
	m_lVPortal[3] = CVec3Dfp32(0, -128, 128);
	m_lVPortal[2] = CVec3Dfp32(0, -128, -128);
	m_lVPortal[1] = CVec3Dfp32(0, 128, -128);
	m_lVPortal[0] = CVec3Dfp32(0, 128, 128);

	CMat4Dfp32 Rot;
	Rot.Unit();
//	CMTime t = (CMTime::GetCPU() + CMTime::CreateFromSeconds(_pObj->m_iObject*4.33f));
	CMTime t;
	t.Snapshot();
	t	+= CMTime::CreateFromSeconds(_pObj->m_iObject*4.33f);
	CVec3Dfp32 Rotv(0.22f*M_Sin(t.GetTimeModulusScaled(0.0333f, _PI2)), 0.25f*M_Sin(t.GetTimeModulusScaled(0.0133f, _PI2)) ,t.GetTimeModulusScaled(0.02f, 1.0f));
	Rotv.CreateMatrixFromAngles(0, Rot);

	Rot = _pObj->GetPositionMatrix();

//	_pObj->GetPosition().SetMatrixRow(Rot, 3);

	CVec3Dfp32 Portal[32];
	CVec3Dfp32::MultiplyMatrix(m_lVPortal, Portal, Rot, 4);

//	_pEngine->Render_AddMirror(Portal, 4, 0x3f3f7f6f);
	// FIXME WMat and VMat needs to be supplied
}
#endif

// -------------------------------------------------------------------
//  FUNC_LOD
// -------------------------------------------------------------------
#ifndef M_DISABLE_CWOBJECT_FUNC_LOD

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Func_LOD, CWObject_Model, 0x0100);

CWObject_Func_LOD::CWObject_Func_LOD()
{
	MAUTOSTRIP(CWObject_Func_LOD_ctor, MAUTOSTRIP_VOID);

	m_Data[0] = 0;
	m_Data[1] = Sqr(32768);
}

void CWObject_Func_LOD::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Func_LOD_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();

	const fp32 Valuef = _Value.Val_fp64();
	
	switch (_KeyHash)
	{
	case MHASH3('MIND','ISTA','NCE'): // "MINDISTANCE"
		{
			m_Data[0] = int32(Sqr(Valuef));
			break;
		}
	case MHASH3('MAXD','ISTA','NCE'): // "MAXDISTANCE"
		{
			m_Data[1] = int32(Sqr(Valuef));
			break;
		}
	default:
		{
			CWObject_Model::OnEvalKey(_pKey);
			break;
		}
	}
}

void CWObject_Func_LOD::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Func_LOD_OnClientRender, MAUTOSTRIP_VOID);

	fp32 DistSqr = CVec3Dfp32::GetMatrixRow(_pEngine->GetVC()->m_CameraWMat, 3).DistanceSqr(_pObj->GetPosition());
	if (DistSqr < _pObj->m_Data[0]) return;
	if (DistSqr > _pObj->m_Data[1]) return;

	CWObject_Model::OnClientRender(_pObj, _pWClient, _pEngine);
}

#endif

// -------------------------------------------------------------------
//  FOGVOLUME
// -------------------------------------------------------------------

#ifndef M_DISABLE_TODELETE
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_FogVolume, CWObject, 0x0100);

CWObject_FogVolume::CWObject_FogVolume()
{
	MAUTOSTRIP(CWObject_FogVolume_ctor, MAUTOSTRIP_VOID);

	m_Radius = 256.0f;
	m_Color = 0x7f808080;
	m_Thickness = 0.5f;
}

void CWObject_FogVolume::OnCreate()
{
	MAUTOSTRIP(CWObject_FogVolume_OnCreate, MAUTOSTRIP_VOID);

	m_iModel[0] = m_pWServer->GetMapData()->GetResourceIndex_Model("FOGVOLUME");
}

void CWObject_FogVolume::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_FogVolume_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();
	const int Valuei = _Value.Val_int();
	const fp32 Valuef = _Value.Val_fp64();
	
	switch (_KeyHash)
	{
	case MHASH2('RADI','US'): // "RADIUS"
		{
			m_Radius = Valuef;
			break;
		}
	case MHASH2('COLO','R'): // "COLOR"
		{
			m_Color.Parse(_Value);
			break;
		}
	case MHASH3('THIC','KNES','S'): // "THICKNESS"
		{
			m_Thickness = Valuei;
			break;
		}
	default:
		{
			CWObject::OnEvalKey(_pKey);
			break;
		}
	}
}

void CWObject_FogVolume::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_FogVolume_OnFinishEvalKeys, MAUTOSTRIP_VOID);

	m_iAnim0 = 
		(int(m_Radius) >> 4) + 
		(int(m_Thickness*16.0f) << 8);
	m_iAnim1 = CImage::ConvToRGBA4444(m_Color);
}

void CWObject_FogVolume::OnLoad(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_FogVolume_OnLoad, MAUTOSTRIP_VOID);

	CWObject::OnLoad(_pFile);
	_pFile->ReadLE(m_Radius);
	_pFile->ReadLE(m_Thickness);
	uint32 c;
	_pFile->ReadLE(c);
	m_Color = c;
}

void CWObject_FogVolume::OnSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_FogVolume_OnSave, MAUTOSTRIP_VOID);

	CWObject::OnSave(_pFile);
	_pFile->WriteLE(m_Radius);
	_pFile->WriteLE(m_Thickness);
	_pFile->WriteLE(m_Color);
}

void CWObject_FogVolume::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_FogVolume_OnClientRender, MAUTOSTRIP_VOID);

	CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	_pEngine->Render_AddModel(pModel, _pObj->GetPositionMatrix(), _pObj->GetDefaultAnimState(_pWClient));
}
#endif

// -------------------------------------------------------------------
//  FLARE
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Flare, CWObject, 0x0100);

CWObject_Flare::CWObject_Flare()
{
	MAUTOSTRIP(CWObject_Flare_ctor, MAUTOSTRIP_VOID);

	m_ClientFlags |= CWO_CLIENTFLAGS_NOREFRESH;
	m_iAnim0 = CImage::ConvToRGBA4444(0xff808080);
	m_iAnim1 = 0;
}

void CWObject_Flare::OnCreate()
{
	MAUTOSTRIP(CWObject_Flare_OnCreate, MAUTOSTRIP_VOID);

	m_iModel[0] = m_pWServer->GetMapData()->GetResourceIndex_Model("FLARE");
}

void CWObject_Flare::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Flare_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH2('COLO','R'): // "COLOR"
		{
			CPixel32 Col;
			Col.Parse(_Value);
			m_iAnim0 = CImage::ConvToRGBA4444(Col);
			break;
		} 
	case MHASH3('FLAR','ETYP','E'): // "FLARETYPE"
		{
			m_iAnim1 = _Value.Val_int();
			break;
		}
	default:
		{
			CWObject::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Flare::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Flare_OnClientRender, MAUTOSTRIP_VOID);

//	CWObject_Model::OnClientRender(_pObj, _pWorld, _pEngine);
//	CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
//	_pEngine->Render_AddModel(pModel, _pObj->GetPositionMatrix(), CXR_AnimState(_pObj->m_AnimTime, _pObj->m_iAnim0, _pObj->m_iAnim1), 2);
}

// -------------------------------------------------------------------
//  TRIGGER
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger, CWObject, 0x0100);

CWObject_Trigger::CWObject_Trigger()
{
	MAUTOSTRIP(CWObject_Trigger_ctor, MAUTOSTRIP_VOID);

	m_ClientFlags = CWO_CLIENTFLAGS_NOUPDATE;
	m_bActive = false;
	m_Mode = 0;
	m_TriggerObjFlags = OBJECT_FLAGS_CHARACTER;
	m_IntersectNotifyFlags = m_TriggerObjFlags;
	m_InitObjectIntersectFlags = 0;
	m_InitObjectFlags = OBJECT_FLAGS_TRIGGER;
}

void CWObject_Trigger::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Trigger_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();

	const int Valuei = _Value.Val_int();
//	const fp32 Valuef = _Value.Val_fp64();
	
	switch (_KeyHash)
	{
	case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
		{
			CStr ModelName = CStrF("$WORLD:%d", Valuei);
	/*
			int XRMode = m_pWServer->Registry_GetGame()->GetValuei("XR_MODE", 0, 0);
			int iModel;
			if( XRMode == 1 )
				iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel2(ModelName);
			else if( XRMode == 2 )
				iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel3(ModelName);
			else
				iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
	*/
			int iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
			if (!iModel) Error("OnEvalKey", "Failed to acquire world-model.");
			if (!m_iModel[0]) m_iModel[0] = iModel;

			// Setup physics
			CWO_PhysicsState Phys;
			Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_PHYSMODEL, iModel, 0, 0);
			Phys.m_nPrim = 1;

			Phys.m_PhysFlags = OBJECT_PHYSFLAGS_ROTATION;
			Phys.m_ObjectIntersectFlags = m_InitObjectIntersectFlags;
			Phys.m_ObjectFlags = m_InitObjectFlags;
			if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
				ConOutL("§cf80WARNING: Unable to set trigger physics state.");

			// Set bound-box.
			{
				CBox3Dfp32 Box;
				m_pWServer->GetMapData()->GetResource_Model(m_iModel[0])->GetBound_Box(Box);
				m_pWServer->Object_SetVisBox(m_iObject, Box.m_Min, Box.m_Max);
			}
			break;
		}
	case MHASH2('MODE','L'): // "MODEL"
	case MHASH3('PHYS','MODE','L'): // "PHYSMODEL"
		{
			int iModel = m_pWServer->GetMapData()->GetResourceIndex_Model(_Value);
			CXR_Model* pM = m_pWServer->GetMapData()->GetResource_Model(iModel);
			if (pM)
			{
				CXR_PhysicsModel* pPhysModel = pM->Phys_GetInterface();
				if (pPhysModel)
				{
					// Setup physics
					CWO_PhysicsState Phys = GetPhysState();

					Phys.m_nPrim = 0;
					if (Phys.m_nPrim >= CWO_MAXPHYSPRIM)
						Error("Model_SetPhys", "Too many physics-primitives.");
					Phys.m_Prim[Phys.m_nPrim++].Create(OBJECT_PRIMTYPE_PHYSMODEL, iModel, 0, 0);

					Phys.m_PhysFlags = 0;
					Phys.m_ObjectFlags = m_InitObjectFlags;
					if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
						ConOutL("§cf80WARNING: Unable to set model physics state.");

					// Set bound-box.
					{
						CBox3Dfp32 Box;
						pM->GetBound_Box(Box);
						m_pWServer->Object_SetVisBox(m_iObject, Box.m_Min, Box.m_Max);
					}
				}
				else
					ConOutL("§cf80WARNING (CWObject_Model::Model_SetPhys): Model was not a physics-model.");
			}
			else
				ConOutL("§cf80WARNING (CWObject_Model::Model_SetPhys): Invalid model-index.");
			break;
		}
	case MHASH4('TRIG','GERO','BJEC','TS'): // "TRIGGEROBJECTS"
		{
			m_TriggerObjFlags = _Value.TranslateFlags(ms_ObjectFlagsTranslate);
			m_IntersectNotifyFlags = m_TriggerObjFlags;
			break;
		}
	case MHASH1('MODE'): // "MODE"
		{
			m_Mode = Valuei;
			break;
		}
	case MHASH2('ACTI','VE'): // "ACTIVE"
		{
			m_bActive = Valuei;
			break;
		}
	case MHASH2('TARG','ET'): // "TARGET"
		{
			m_Target = m_pWServer->World_MangleTargetName(_pKey->GetThisValue());
			break;
		}
	default:
		{
			CWObject::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Trigger::Trigger(int _iSender)
{
	MAUTOSTRIP(CWObject_Trigger_Trigger, MAUTOSTRIP_VOID);

	if(m_Target != "")
	{
		CWObject_Message Msg(OBJMSG_IMPULSE, m_Mode);
		Msg.m_iSender = _iSender;
		m_pWServer->Message_SendToTarget(Msg, m_Target);
	}
}

aint CWObject_Trigger::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Trigger_OnMessage, 0);

	switch(_Msg.m_Msg)
	{
	case OBJSYSMSG_NOTIFY_INTERSECTION : 
	case OBJMSG_TRIGGER_INTERSECTION :
//		ConOut("Trigger intersect.");
			Trigger(_Msg.m_Param0);
			return 0;
	default :
		return CWObject::OnMessage(_Msg);
	}
}

void CWObject_Trigger::OnRefresh()
{
	MAUTOSTRIP(CWObject_Trigger_OnRefresh, MAUTOSTRIP_VOID);

	if (m_bActive)
	{
		CWO_PhysicsState Phys = GetPhysState();
		Phys.m_ObjectIntersectFlags = m_TriggerObjFlags;

		int iObj = m_iObject;
		CWorld_Server* pServer = m_pWServer;
		TSelection<CSelection::LARGE_BUFFER> Selection;
		{
			pServer->Selection_AddIntersection(Selection, GetPositionMatrix(), Phys);
			const int16* pSel = NULL;
			int nSel = pServer->Selection_Get(Selection, &pSel);
			for(int i = 0; i < nSel; i++)
			{
				pServer->Message_SendToObject(CWObject_Message(OBJMSG_TRIGGER_INTERSECTION, pSel[i]), m_iObject);
				if (!pServer->Object_Get(iObj)) break;
			}
		}
	}
	else
		m_ClientFlags |= CWO_CLIENTFLAGS_NOREFRESH;
}

void CWObject_Trigger::OnLoad(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Trigger_OnLoad, MAUTOSTRIP_VOID);

	CWObject::OnLoad(_pFile);
	_pFile->ReadLE(m_TriggerObjFlags);
	_pFile->ReadLE(m_Mode);
	_pFile->ReadLE(m_bActive);
}

void CWObject_Trigger::OnSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Trigger_OnSave, MAUTOSTRIP_VOID);

	CWObject::OnSave(_pFile);
	_pFile->WriteLE(m_TriggerObjFlags);
	_pFile->WriteLE(m_Mode);
	_pFile->WriteLE(m_bActive);
}

#ifndef M_DISABLE_CWOBJECT_TRIGGER_TELEPORT
// -------------------------------------------------------------------
//  TRIGGER_TELEPORT
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_Teleport, CWObject_Trigger, 0x0100);

void CWObject_Trigger_Teleport::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Trigger_Teleport_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	switch (_KeyHash)
	{
	case MHASH4('TELE','PORT','_SIL','ENT'): // "TELEPORT_SILENT"
		{
			m_State |= 1;
			break;
		}
	default:
		{
			CWObject_Trigger::OnEvalKey(_pKey);
			break;
		}
	}
}

void CWObject_Trigger_Teleport::OnCreate()
{
	MAUTOSTRIP(CWObject_Trigger_Teleport_OnCreate, MAUTOSTRIP_VOID);

	CWObject_Trigger::OnCreate();

	m_State = 0;
}

bool CWObject_Trigger_Teleport::Teleport(int _iObj, const CMat4Dfp32& _Pos)
{
	MAUTOSTRIP(CWObject_Trigger_Teleport_Teleport, false);

	CWObject* pObj = m_pWServer->Object_Get(_iObj);
	if (pObj == NULL)
		return true;

	bool result;

	if (!m_pWServer->Object_SetPosition(_iObj, _Pos))
	{
		// Try telefragging everthing obstructing the teleport
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddIntersection(Selection, _Pos, pObj->GetPhysState());
		m_pWServer->Message_SendToSelection(CWObject_Message(OBJMSG_PHYSICS_KILL, 0, 0, _iObj), selection);

		result = m_pWServer->Object_SetPosition(_iObj, _Pos);
	}
	else
		result = true;

	if (result)
	{
		CWObject_Message Msg(OBJMSG_CREATETELEPORTEFFECT);
		m_pWServer->Message_SendToObject(Msg, _iObj);
	}

	return result;
}

int CWObject_Trigger_Teleport::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Trigger_Teleport_OnMessage, 0);

	switch(_Msg.m_Msg)
	{
	case OBJSYSMSG_NOTIFY_INTERSECTION : 
	case OBJMSG_TRIGGER_INTERSECTION :
//	case OBJSYSMSG_PHYSICS_PREINTERSECTION:
		{
			CWObject* pObj = m_pWServer->Object_Get(_Msg.m_Param0);
			if (!pObj) return 0;
			if (!m_pWServer->Phys_IntersectStates(pObj->GetPhysState(), GetPhysState(), 
				pObj->GetPositionMatrix(), pObj->GetPositionMatrix(), 
				GetPositionMatrix(), GetPositionMatrix(), NULL)) return 0;

			TSelection<CSelection::LARGE_BUFFER> Selection;
			{
				m_pWServer->Selection_AddTarget(Selection, m_Target);
				const int16* pSel = NULL;
				int nSel = m_pWServer->Selection_Get(Selection, &pSel);

				if (nSel)
				{
					int iSel = Min(int(Random*nSel), nSel-1);
	//				for(int i = 0; i < nSel; i++)
					{
						if (!(m_State & 1))
						{
							m_pWServer->Object_Create("Explosion2", m_pWServer->Object_GetPosition(_Msg.m_Param0));
							m_pWServer->Sound_At(GetPosition(), m_pWServer->GetMapData()->GetResourceIndex_Sound("TELEPORT1"), 0);
						}

						CWObject* pObjDst = m_pWServer->Object_Get(pSel[iSel]);
						if (pObj && pObjDst)
						{
							CMat4Dfp32 ObjInv, TPInv, Obj2TP, TP2Dst, Tmp, NewPos;
							GetPositionMatrix().InverseOrthogonal(TPInv);
							pObj->GetPositionMatrix().InverseOrthogonal(ObjInv);
							GetPositionMatrix().Multiply(ObjInv, Obj2TP);
//							pObjDst->GetPositionMatrix().Multiply(TPInv, TP2Dst);

							Obj2TP.InverseOrthogonal(Tmp);
							Tmp.Multiply(pObjDst->GetPositionMatrix(), NewPos);

/*ConOutL(CStrF("Frame %d", m_pWServer->GetFrame()));
ConOutL("Before: " + pObj->GetPositionMatrix().GetString());
ConOutL("After: " + NewPos.GetString());*/

							if (!Teleport(_Msg.m_Param0, NewPos))
							{
								NewPos.k[3][2] += 1.0f;
								if (!Teleport(_Msg.m_Param0, NewPos))
								{
									CMat4Dfp32 AbsDestPos(pObjDst->GetPositionMatrix());
									AbsDestPos.k[3][2] += 1.0f;
									ConOut(CStrF("§cf80WARNING: Unable to teleport %d to %s.", pObj->m_iObject, CVec3Dfp32::GetRow(NewPos, 3).GetString().Str()));
									if (!Teleport(_Msg.m_Param0, AbsDestPos))
										ConOut(CStrF("§cf80WARNING: Unable to teleport %d to %s.", pObj->m_iObject, CVec3Dfp32::GetRow(AbsDestPos, 3).GetString().Str()));
								}
							}

							if (!(m_State & 1))
								m_pWServer->Sound_At(pObj->GetPosition(), m_pWServer->GetMapData()->GetResourceIndex_Sound("TELEPORT2"), 0);
						}
					}
				}
			}
			return 0;
		}
	default :
		return CWObject_Trigger::OnMessage(_Msg);
	}
}

void CWObject_Trigger_Teleport::OnRefresh()
{
	MAUTOSTRIP(CWObject_Trigger_Teleport_OnRefresh, MAUTOSTRIP_VOID);

	CWObject_Trigger::OnRefresh();
}

void CWObject_Trigger_Teleport::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_Trigger_Teleport_OnFinishEvalKeys, MAUTOSTRIP_VOID);

	CWObject_Trigger::OnFinishEvalKeys();
	if (!(m_State & 1))
		m_iSound[0] = m_pWServer->GetMapData()->GetResourceIndex_Sound("TELEPORT3");

}

void CWObject_Trigger_Teleport::OnLoad(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Trigger_Teleport_OnLoad, MAUTOSTRIP_VOID);

	CWObject_Trigger::OnLoad(_pFile);
	_pFile->ReadLE(m_State);
}

void CWObject_Trigger_Teleport::OnSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Trigger_Teleport_OnSave, MAUTOSTRIP_VOID);

	CWObject_Trigger::OnSave(_pFile);
	_pFile->WriteLE(m_State);
}

void CWObject_Trigger_Teleport::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CWObject_Trigger_Teleport_OnIncludeClass, MAUTOSTRIP_VOID);

	CWObject_Trigger::OnIncludeClass(_pWData, _pWServer);

	_pWData->GetResourceIndex_Class("Explosion2");
/*	_pWData->GetResourceIndex_Sound("TELEPORT1");
	_pWData->GetResourceIndex_Sound("TELEPORT2");
	_pWData->GetResourceIndex_Sound("TELEPORT3");*/
}
#endif

// -------------------------------------------------------------------
//  TRIGGER_TRANSITZONE
// -------------------------------------------------------------------
/*MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_TransitZone, CWObject_Trigger, 0x0100);

CWObject_Trigger_TransitZone::CWObject_Trigger_TransitZone()
{
	MAUTOSTRIP(CWObject_Trigger_TransitZone_ctor, MAUTOSTRIP_VOID);

	m_TriggerObjFlags = 0;
	m_IntersectNotifyFlags = 0;
}

void CWObject_Trigger_TransitZone::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Trigger_TransitZone_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH4('TRAN','SITZ','ONE_','NAME'): // "TRANSITZONE_NAME"
		{
			m_TZName = _Value;
			m_pWServer->Object_SetName(m_iObject, m_TZName);
			break;
		}
	case MHASH4('TRAN','SITZ','ONE_','USE'): // "TRANSITZONE_USE"
		{
			m_TZName = _Value;
			m_pWServer->Object_SetName(m_iObject, m_TZName);
			break;
		}
	default:
		{
			CWObject_Trigger::OnEvalKey(_pKey);
			break;
		}
	}
}

void CWObject_Trigger_TransitZone::OnLoad(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Trigger_TransitZone_OnLoad, MAUTOSTRIP_VOID);

	CWObject_Trigger::OnLoad(_pFile);
	m_TZName.Read(_pFile);
}

void CWObject_Trigger_TransitZone::OnSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Trigger_TransitZone_OnSave, MAUTOSTRIP_VOID);

	CWObject_Trigger::OnSave(_pFile);
	m_TZName.Write(_pFile);
}

void CWObject_Trigger_TransitZone::OnSpawnWorld()
{
	MAUTOSTRIP(CWObject_Trigger_TransitZone_OnSpawnWorld, MAUTOSTRIP_VOID);

	if (m_TZName != "") m_pWServer->World_SpawnTransitZone(m_TZName, GetPositionMatrix());
	CWObject_Trigger::OnSpawnWorld();
}*/

// -------------------------------------------------------------------
//  TRIGGER_CHANGEWORLD
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_ChangeWorld, CWObject_Trigger, 0x0100);

CWObject_Trigger_ChangeWorld::CWObject_Trigger_ChangeWorld()
{
	MAUTOSTRIP(CWObject_Trigger_ChangeWorld_ctor, MAUTOSTRIP_VOID);

	m_bActive = true;
	m_TriggerObjFlags = OBJECT_FLAGS_PLAYER;
	m_IntersectNotifyFlags = m_TriggerObjFlags;
	m_Flags = 0;
}


void CWObject_Trigger_ChangeWorld::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Trigger_ChangeWorld_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH2('WORL','D'): // "WORLD"
		{
			m_World = _Value;
			break;
		}
	case MHASH2('FLAG','S'): // "FLAGS"
		{
			static const char *TriggerFlagsStr[] = { "Instant", "WaitSpawn", NULL };
			m_Flags |= _Value.TranslateFlags(TriggerFlagsStr);
			break;
		}
	default:
		{
			CWObject_Trigger::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}


void CWObject_Trigger_ChangeWorld::OnLoad(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Trigger_ChangeWorld_OnLoad, MAUTOSTRIP_VOID);

	CWObject_Trigger::OnLoad(_pFile);
	m_World.Read(_pFile);

	Spawn( !(m_Flags & FLAGS_WAITSPAWN) );
}


void CWObject_Trigger_ChangeWorld::OnSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Trigger_ChangeWorld_OnSave, MAUTOSTRIP_VOID);

	CWObject_Trigger::OnSave(_pFile);
	m_World.Write(_pFile);
}


void CWObject_Trigger_ChangeWorld::OnFinishEvalKeys()
{
	CWObject_Trigger::OnFinishEvalKeys();
	Spawn( !(m_Flags & FLAGS_WAITSPAWN) );
}


aint CWObject_Trigger_ChangeWorld::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_GAME_SPAWN:
		Spawn( (_Msg.m_Param0 <= 0) );
		return 0;
	}
	return CWObject_Trigger::OnMessage(_Msg);
}


void CWObject_Trigger_ChangeWorld::Trigger(int _iSender)
{
	MAUTOSTRIP(CWObject_Trigger_ChangeWorld_Trigger, MAUTOSTRIP_VOID);

	if (m_World == "")
	{
		ConOut("(CWObject_Trigger_ChangeWorld::OnMessage) No world-name specified.");
		return;
	}
	
	if (GetName() == "")
	{
		ConOut("(CWObject_Trigger_ChangeWorld::OnMessage) No targetname specified.");
		return;
	}
	
	if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_NEVERTRIGGER, 2), _iSender) == 1)
		return;

	CWObject_Message Msg(OBJMSG_GAME_CHANGEWORLD, m_Flags ^ 1, 0, m_iObject);
	Msg.m_pData = (void *)m_World.Str();
	m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
}


void CWObject_Trigger_ChangeWorld::Spawn(bool _bSpawn)
{
	if (_bSpawn)
	{
		m_Flags &= ~FLAGS_WAITSPAWN;
		m_IntersectNotifyFlags = m_TriggerObjFlags;
		m_bActive = true;  // I don't want this! but I dare not change the old behaviour
	}
	else
	{
		m_Flags |= FLAGS_WAITSPAWN;
		m_IntersectNotifyFlags = 0;
		m_bActive = false;
	}
}


#ifndef M_DISABLE_CWOBJECT_SYSTEM
/////////////////////////////////////////////////////////////////////////////
// CWObject_System
void CWObject_System::OnCreate()
{
	MAUTOSTRIP(CWObject_System_OnCreate, MAUTOSTRIP_VOID);
}

int CWObject_System::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_System_OnMessage, 0);
	
	switch(_Msg.m_Msg)
	{
	case OBJMSG_IMPULSE:
	case OBJMSG_GAME_SPAWN:
	case OBJSYSMSG_SETMODEL:
		{
			int iObj = GetFirstChild();
			while(iObj)
			{
				m_pWServer->Message_SendToObject(_Msg, iObj);

				CWObject *pObj = m_pWServer->Object_Get(iObj);
				if(!pObj)
					iObj = 0;
				else
					iObj = pObj->GetNextChild();
			}

			return 1;
		}

	default:
		return CWObject::OnMessage(_Msg);
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_System, CWObject, 0x0100);
#endif
