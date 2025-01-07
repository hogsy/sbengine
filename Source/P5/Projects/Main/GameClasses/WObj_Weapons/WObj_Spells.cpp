#include "PCH.h"

#include "../WObj_Char.h" // To declare OBJMSG_CHAR_SHAKECAMERA and check if an object is a bot
#include "WObj_Spells.h"
#include "../Models/WModel_Debris.h"
#include "../WRPG/WRPGChar.h"
#include "../WRPG/WRPGWeapon.h"
#include "../WRPG/WRPGInitParams.h"

//-------------------------------------------------------------------

#define RENDEROFFSET_MAX 127.0f
#define RENDEROFFSET_BLENDTIME_SUBPRECISION 8.0f
#define RENDEROFFSET_BLENDTIME_MAX 1.5f

//-------------------------------------------------------------------
#ifndef M_DISABLE_CURRENTPROJECT

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ClientModel, CWObject_Model, 0x0100);
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ClientModel_Ext, CWObject_Model, 0x0100);
#endif

//-------------------------------------------------------------------
/*
static int16 GetObjFlags(CWObject_CoreData* _pObj)
{
	MAUTOSTRIP(GetObjFlags, 0);
	if (_pObj == NULL)
		return 0;

	return _pObj->GetPhysState().m_ObjectFlags;
}

static int16 GetObjFlags(int _iObj, CWorld_PhysState* _pWPhysState)
{
	MAUTOSTRIP(GetObjFlags_2, 0);
	if (_pWPhysState == NULL)
		return 0;

	return GetObjFlags(_pWPhysState->Object_GetCD(_iObj));
}
*/
//-------------------------------------------------------------------
// Ext_Model
//-------------------------------------------------------------------

CWObject_Ext_Model::CWObject_Ext_Model()
{
	MAUTOSTRIP(CWObject_Ext_Model_ctor, MAUTOSTRIP_VOID);
	m_Timeout = -1;
	m_MovingSpawnDuration = -1;

	m_iSoundSpawn = 0;
	m_iSoundLoop0 = 0;
	m_iSoundLoop1 = 0;

	m_bPendingDestroy = false;
	m_DestroyDelayTicks = 0;
}

void CWObject_Ext_Model::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CWObject_Ext_Model_OnIncludeTemplate, MAUTOSTRIP_VOID);
	CWObject_Model::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

	IncludeSoundFromKey("SOUND_SPAWN", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_LOOP0", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_LOOP1", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_TIMEOUT", _pReg, _pMapData);

	if(_pReg)
	{
		int nKeys = _pReg->GetNumChildren();
		for(int i = 0; i < nKeys; i++)
		{
			CRegistry *pChild = _pReg->GetChild(i);
			if(pChild->GetThisName().Copy(0, 9) == "RANDMODEL")
			{
				CStr model = pChild->GetThisValue();
				int iRand = model.GetStrSep(",").Val_int();
				model.Trim();
				
				for(int i = 1; i <= iRand; i++)
					_pMapData->GetResourceIndex_Model(model + CStrF("%i", i));
			}
		}
	}
}

void CWObject_Ext_Model::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Ext_Model_OnEvalKey, MAUTOSTRIP_VOID);
	
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	int KeyValuei = KeyValue.Val_int();
	
	switch (_KeyHash)
	{
	case MHASH2('DURA','TION'): // "DURATION"
		{
			fp32 MinDuration, MaxDuration;
			CFStr str = KeyValue;
			MinDuration = str.GetStrSep("-").Val_fp64();
			if (str != "")
				MaxDuration = str.Val_fp64();
			else
				MaxDuration = MinDuration;

			fp32 Duration = LERP(MinDuration, MaxDuration, (MRTC_RAND() & 255) / 255.0f);

			SetTimeout((int)(Duration * m_pWServer->GetGameTicksPerSecond()));
			break;
		}

	case MHASH3('DEST','ROYD','ELAY'): // "DESTROYDELAY"
		{
			m_DestroyDelayTicks = (int)(_pKey->GetThisValuef() * m_pWServer->GetGameTicksPerSecond());
			break;
		}

	case MHASH5('DURA','TION','_MOV','INGS','PAWN'): // "DURATION_MOVINGSPAWN"
		{
			m_MovingSpawnDuration = _pKey->GetThisValuef();
			break;
		}

	case MHASH2('COLO','RS0'): // "COLORS0"
		{
			m_Data[0] = ~KeyValuei;
			break;
		}

	case MHASH2('COLO','RS1'): // "COLORS1"
		{
			m_Data[1] = ~KeyValuei;
			break;
		}
	
	case MHASH2('COLO','RS2'): // "COLORS2"
		{
			m_Data[2] = ~KeyValuei;
			break;
		}
	
	case MHASH2('COLO','RS3'): // "COLORS3"
		{
			m_Data[3] = ~KeyValuei;
			break;
		}
	
	case MHASH3('ANIM','ATTR','0'): // "ANIMATTR0"
		{
			fp32 f = _pKey->GetThisValuef();
			fp32 zero = 0;
			m_Data[4] = *((int *)&f) ^ *((int *)&zero);
			break;
		}		
	case MHASH3('ANIM','ATTR','1'): // "ANIMATTR1"
		{
			fp32 f = _pKey->GetThisValuef();
			fp32 zero = 0;
			m_Data[5] = *((int *)&f) ^ *((int *)&zero);
			break;
		}

	case MHASH3('REND','ERFL','AGS'): // "RENDERFLAGS"
		{
			ClientFlags() &= ~CLIENTFLAGS_RENDER_NOROT;
			ClientFlags() &= ~CLIENTFLAGS_RENDER_ZROT;

			static const char* slpRenderFlags[] = { "NoRot", "ZRot", NULL };
			CFStr Value = KeyValue;
			int RenderFlags = Value.TranslateFlags(slpRenderFlags);
			if (RenderFlags & 1)
				ClientFlags() |= CLIENTFLAGS_RENDER_NOROT;
			if (RenderFlags & 2)
				ClientFlags() |= CLIENTFLAGS_RENDER_ZROT;
			break;
		}

	case MHASH3('RAND','MODE','L'): // "RANDMODEL"
	case MHASH3('RAND','MODE','L0'): // "RANDMODEL0"
		{
			CFStr model = KeyValue;
			int iRand = model.GetStrSep(",").Val_int();
			model.Trim();
		
			model += CFStrF("%i", int(Random * iRand) + 1);
			Model_Set(0, model);
			break;
		}

	case MHASH3('RAND','MODE','L1'): // "RANDMODEL1"
		{
			CFStr model = KeyValue;
			int iRand = model.GetStrSep(",").Val_int();
			model.Trim();
		
			model += CFStrF("%i", int(Random * iRand) + 1);
			Model_Set(1, model);
			break;
		}
	
	case MHASH3('RAND','MODE','L2'): // "RANDMODEL2"
		{
			CFStr model = KeyValue;
			int iRand = model.GetStrSep(",").Val_int();
			model.Trim();
		
			model += CFStrF("%i", int(Random * iRand) + 1);
			Model_Set(2, model);
			break;
		}

	case MHASH3('RAND','ANIM','0'): // "RANDANIM0"
		{
			m_iAnim0 = MRTC_RAND();
			break;
		}
	
	case MHASH3('RAND','PERM','ODEL'): // "RANDPERMODEL"
		{
			m_ClientFlags |= CLIENTFLAGS_RANDPERMODEL;
			m_iAnim0 = MRTC_RAND();
			break;
		}
	
	case MHASH3('SOUN','D_SP','AWN'): // "SOUND_SPAWN"
		{
			m_iSoundSpawn = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_LO','OP0'): // "SOUND_LOOP0"
		{
			m_iSoundLoop0 = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_LO','OP1'): // "SOUND_LOOP1"
		{
			m_iSoundLoop1 = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	default:
		{
			CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Ext_Model::OnInitInstance(const aint* _pParam, int _nParam)
{
	MAUTOSTRIP(CWObject_Ext_Model_OnInitInstance, MAUTOSTRIP_VOID);
	Sound(m_iSoundSpawn);
	m_iSound[0] = m_iSoundLoop0;
	m_iSound[1] = m_iSoundLoop1;
}

void CWObject_Ext_Model::OnRefresh()
{
	MAUTOSTRIP(CWObject_Ext_Model_OnRefresh, MAUTOSTRIP_VOID);
	if (m_bPendingDestroy)
	{
		if (m_DestroyDelayTicks > 0)
		{
			m_DestroyDelayTicks--;
		}
		else
		{
			m_bPendingDestroy = false;
			Destroy();
		}
	}

	if ((GetTimeout() >= 0) && (GetAnimTick(m_pWServer) > GetTimeout()))
	{
		OnTimeout();
		m_Timeout = -1; // Turn timeout off, since object destruction might be delayed a short while.
		return;
	}
}

void CWObject_Ext_Model::OnLoad(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Ext_Model_OnLoad, MAUTOSTRIP_VOID);
	CWObject_Model::OnLoad(_pFile);
	_pFile->ReadLE(m_Timeout);

}

void CWObject_Ext_Model::OnSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Ext_Model_OnSave, MAUTOSTRIP_VOID);
	CWObject_Model::OnSave(_pFile);
	_pFile->WriteLE(m_Timeout);
}

void CWObject_Ext_Model::GetAnimTime(CWObject_Client* _pObj, CWorld_Client* _pWClient, CMTime& _AnimTime, fp32& _TickFrac)
{
	MAUTOSTRIP(CWObject_Ext_Model_GetAnimTime, MAUTOSTRIP_VOID);
	_TickFrac = _pWClient->GetRenderTickFrac();
	_AnimTime = CMTime::CreateFromTicks(_pObj->GetAnimTick(_pWClient), _pWClient->GetGameTickTime(), _TickFrac);
}

bool CWObject_Ext_Model::GetRenderMatrix(CWObject_Client* _pObj, CWorld_Client* _pWClient, CMat4Dfp32& _Matrix, fp32 _TickFrac)
{
	MAUTOSTRIP(CWObject_Ext_Model_GetRenderMatrix, false);
	CWObject_Message Msg(OBJMSG_SPELLS_GETRENDERMATRIX);
	Msg.m_pData = &_Matrix;
	Msg.m_VecParam0[0] = _TickFrac;
	int result = _pObj->m_pRTC->m_pfnOnClientMessage(_pObj, _pWClient, Msg);
	if (result != 0)
	{
		if (_pObj->m_ClientFlags & CLIENTFLAGS_RENDER_NOROT)
			_Matrix.Unit3x3();
		else if (_pObj->m_ClientFlags & CLIENTFLAGS_RENDER_ZROT)
		{
			CVec3Dfp32(0, 0, 1).SetMatrixRow(_Matrix, 2);
			_Matrix.RecreateMatrix(2, 1);
		}

		return true;
	}
	return false;
}

bool CWObject_Ext_Model::GetAnimState(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_AnimState& _AnimState, CMat4Dfp32& _Matrix, const CMTime& _AnimTime, fp32 _TickFrac, int _iModel)
{
	MAUTOSTRIP(CWObject_Ext_Model_GetAnimState, false);
	CWObject_Message Msg(OBJMSG_SPELLS_GETANIMSTATE);
	void* pData[2] = { &_AnimState, &_Matrix };
	Msg.m_pData = pData;
	Msg.m_DataSize = sizeof(void*) * 2;
	Msg.m_VecParam0[0] = _TickFrac;
	Msg.m_Param0 = _iModel;
	Msg.m_Param1 = (aint)&_AnimTime;
	return (_pObj->m_pRTC->m_pfnOnClientMessage(_pObj, _pWClient, Msg) != 0);
}

void CWObject_Ext_Model::GetDefaultAnimState(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Ext_Model_GetDefaultAnimState, MAUTOSTRIP_VOID);
	CXR_AnimState* pAnimState = (CXR_AnimState*)(((void**)_Msg.m_pData)[0]);
	//CMat4Dfp32* pMatrix = (CMat4Dfp32*)(((void**)_Msg.m_pData)[1]);
	CMTime *pAnimTime = (CMTime *)_Msg.m_Param1;
	//fp32 TickFrac = _Msg.m_VecParam0[1];
	int iModel = _Msg.m_Param0;

	GetDefaultAnimState(_pObj, _pWClient, *pAnimState, *pAnimTime, iModel);
}

void CWObject_Ext_Model::GetDefaultAnimState(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_AnimState& _AnimState, const CMTime& _AnimTime, int _iModel)
{
	MAUTOSTRIP(CWObject_Ext_Model_GetDefaultAnimState_2, MAUTOSTRIP_VOID);
	int iRPM = (_pObj->m_ClientFlags & CLIENTFLAGS_RANDPERMODEL) ? MRTC_RAND() : 0;

	_AnimState.m_AnimTime0 = _AnimTime;
	_AnimState.m_AnimTime1 = PHYSSTATE_TICKS_TO_TIME((uint16)_pObj->m_iAnim2, _pWClient);

	_AnimState.m_Anim0 = _pObj->m_iAnim0 + iRPM;
	_AnimState.m_Anim1 = _pObj->m_iAnim1;

	_AnimState.m_iObject = _pObj->m_iObject;

	for(int j = 0; j < 4; j++)
		_AnimState.m_Data[j] = ~_pObj->m_Data[j];

	fp32 zero = 0;
	int data4 = _pObj->m_Data[4] ^ *((int*)&zero);
	int data5 = _pObj->m_Data[5] ^ *((int*)&zero);
	_AnimState.m_AnimAttr0 = *((fp32*)&data4);
	_AnimState.m_AnimAttr1 = *((fp32*)&data5);
	
	if ((_iModel < 0) || (_iModel >= CWO_NUMMODELINDICES))
	{
		ConOutL(CStrF("CWObject_Ext_Model::GetDefaultAnimState() - Invalid model index %d (iObj %d).", _iModel, _pObj->m_iObject));
		return;
	}

	_AnimState.m_pModelInstance = _pObj->m_lModelInstances[_iModel];
}

void CWObject_Ext_Model::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Ext_Model_OnClientRefresh, MAUTOSTRIP_VOID);
	MSCOPESHORT( CWObject_Ext_Model::OnClientRefresh );
	CMat4Dfp32 Matrix;
	if (!GetRenderMatrix(_pObj, _pWClient, Matrix, 1.0f))
		return;

	int ModelFlags = ~(_pObj->m_Data[3]);

	int AnimTick = _pWClient->GetGameTick() - _pObj->m_CreationGameTick;
	CXR_ModelInstanceContext RefreshContext(AnimTick, _pWClient->GetGameTickTime(), _pObj, _pWClient);

	for(int i = 0; i < CWO_NUMCLIENTOBJ; i++)
	{
		if (_pObj->m_lModelInstances[i] != NULL)
			_pObj->m_lModelInstances[i]->OnRefresh(RefreshContext, &Matrix, 1, ModelFlags);
	}
}

void CWObject_Ext_Model::RenderModelsDefault(const CMTime& _AnimTime, fp32 _TickFrac, CMat4Dfp32& _Matrix, CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CWObject_Ext_Model_RenderModelsDefault, MAUTOSTRIP_VOID);
	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
		if (pModel)
		{
			CXR_AnimState AnimState;
			if (GetAnimState(_pObj, _pWClient, AnimState, _Matrix, _AnimTime, _TickFrac, i))
				_pEngine->Render_AddModel(pModel, _Matrix, AnimState);
		}
	}
}

void CWObject_Ext_Model::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Ext_Model_OnClientRender, MAUTOSTRIP_VOID);
	CMTime AnimTime;
	fp32 TickFrac;
	GetAnimTime(_pObj, _pWClient, AnimTime, TickFrac);

	CMat4Dfp32 Matrix;
	if (!GetRenderMatrix(_pObj, _pWClient, Matrix, TickFrac))
		return;

	RenderModelsDefault(AnimTime, TickFrac, Matrix, _pObj, _pWClient, _pEngine);
}

aint CWObject_Ext_Model::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Ext_Model_OnClientMessage, 0);
	switch (_Msg.m_Msg)
	{
		case OBJMSG_SPELLS_GETRENDERMATRIX:
		{
			CMat4Dfp32* pMatrix = (CMat4Dfp32*)_Msg.m_pData;
			fp32 TickFrac = _Msg.m_VecParam0[0];

			if (TickFrac == 1.0f)
				*pMatrix = _pObj->GetPositionMatrix();
			else
				Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), *pMatrix, TickFrac);

			return 1;
		}
		case OBJMSG_SPELLS_GETANIMSTATE:
		{
			GetDefaultAnimState(_pObj, _pWClient, _Msg);
			return 1;
		}
	}

	return CWObject_Model::OnClientMessage(_pObj, _pWClient, _Msg);
}

aint CWObject_Ext_Model::OnMessage(const CWObject_Message &_Msg)
{
	MAUTOSTRIP(CWObject_Ext_Model_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
		case OBJSYSMSG_PHYSICS_PREINTERSECTION:
			return OnPreIntersection(_Msg.m_Param0, (CCollisionInfo *)_Msg.m_pData);

		case OBJSYSMSG_NOTIFY_INTERSECTION:
			return OnNotifyIntersection(_Msg.m_Param0);

//		case OBJMSG_SPELLS_SUMMONINIT:
//			return OnSummonInit(*(CSummonInit *)_Msg.m_Param0);
	}
	
	return CWObject_Model::OnMessage(_Msg);
}

int CWObject_Ext_Model::IsInLiquid(CVec3Dfp32 _Pos)
{
	MAUTOSTRIP(CWObject_Ext_Model_IsInLiquid, 0);
	int Medium = m_pWServer->Phys_GetMedium(_Pos, NULL);
	if (Medium & XW_MEDIUM_LIQUID)
		return 1;
	else
		return 0;
}


bool CWObject_Ext_Model::SetSphereProjectile(fp32 _Radius, int _Intersectflags, int _iExclude)
{
	MAUTOSTRIP(CWObject_Ext_Model_SetSphereProjectile, false);
	CWO_PhysicsState Phys;
	Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_SPHERE, -1, CVec3Dfp32(_Radius, 0, 0), 0);
	Phys.m_nPrim = 1;

	Phys.m_ObjectFlags = OBJECT_FLAGS_PROJECTILE;
	Phys.m_ObjectIntersectFlags = _Intersectflags;
	Phys.m_PhysFlags = OBJECT_PHYSFLAGS_PHYSMOVEMENT;
	Phys.m_iExclude = _iExclude == -1 ? m_iOwner : _iExclude;
	if(!m_pWServer->Object_SetPhysics(m_iObject, Phys))
		return false;
	else
		return true;
}
/*
void CWObject_Ext_Model::SendShockwave(CVec3Dfp32 _Pos, fp32 _Radius, int _Force, int _Damage, uint32 _DamageType, int _iExclude)
{
	MAUTOSTRIP(CWObject_Ext_Model_SendShockwave, MAUTOSTRIP_VOID);
	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pWServer->Selection_AddBoundSphere(Selection, OBJECT_FLAGS_PHYSOBJECT, _Pos, _Radius);

	if (_iExclude != 0)
		m_pWServer->Selection_RemoveOnIndex(Selection, _iExclude);

	CWObject_Message Msg(OBJMSG_PHYSICS_RADIALSHOCKWAVE);

	Msg.m_VecParam0 = _Pos;
	Msg.m_VecParam1[0] = _Radius;
	Msg.m_VecParam1[1] = _Force;
	Msg.m_VecParam1[2] = _Damage;
	Msg.m_Reason = _DamageType;
	Msg.m_iSender = m_iOwner;

//	Msg = CWObject_Message(OBJMSG_PHYSICS_RADIALSHOCKWAVE, _Radius, _Damage, m_iOwner, _DamageType, GetPosition());

	m_pWServer->Message_SendToSelection(Msg, iSel);
}
*/
/*
int CWObject_Ext_Model::SendDamage(int _iObject, const CVec3Dfp32& _Pos, int _iSender, int _Damage, uint32 _DamageType, int _SurfaceType, int _SplashDmg, fp32 _Radius, const CVec3Dfp32& _Force, CVec3Dfp32 *_pSplatterDir)
{
	MAUTOSTRIP(CWObject_Ext_Model_SendDamage, 0);
	CWObject_Message_Damage Msg;
	Msg.m_pPosition = &_Pos;
	Msg.m_Damage = _Damage;
	Msg.m_DamageType = _DamageType;
	Msg.m_SplashDamage = _SplashDmg;
	Msg.m_SplashForce = _SplashDmg;
	Msg.m_SplashRadius = _Radius;
	Msg.m_pForce = &_Force;
	Msg.m_pSplatterDirection = _pSplatterDir;
	Msg.m_SurfaceType = _SurfaceType;
	return Msg.Send(m_pWServer, _iObject, _iSender);
}
*/
/*
void CWObject_Ext_Model::SendDamage(int _iObject, CVec3Dfp32 _Pos, int _Damage, uint32 _DamageType, int _SplashDmg, fp32 _Radius, const CVec3Dfp32& _Force)
{
	MAUTOSTRIP(CWObject_Ext_Model_SendDamage_2, MAUTOSTRIP_VOID);
	// Sends shockwave to all surrounding objects, except from the one that was hit.

	if(_SplashDmg)
	{
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddBoundSphere(Selection, OBJECT_FLAGS_PHYSOBJECT, _Pos, _Radius);
		if(_Damage && _iObject > 0)
			m_pWServer->Selection_RemoveOnIndex(Selection, _iObject);

		CWObject_Message Msg(OBJMSG_PHYSICS_RADIALSHOCKWAVE);

		Msg.m_VecParam0 = _Pos;
		Msg.m_VecParam1[0] = _Radius;
		Msg.m_VecParam1[1] = _SplashDmg;
		Msg.m_VecParam1[2] = _SplashDmg;
		Msg.m_Reason = _DamageType;
		Msg.m_iSender = m_iOwner;

//		m_pWServer->Message_SendToSelection(CWObject_Message(OBJMSG_PHYSICS_RADIALSHOCKWAVE, _Radius, _SplashDmg, m_iOwner, _DamageType, GetPosition()), iSel);
		m_pWServer->Message_SendToSelection(Msg, iSel);

		if(_Damage)
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_PHYSICS_DAMAGE, _Damage, _DamageType, m_iOwner, 0, _Force, _Pos), _iObject);
	}
	else
	{
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_PHYSICS_DAMAGE, _Damage, _DamageType, m_iOwner, 0, _Force, _Pos), _iObject);
	}
}
*/
CMat4Dfp32 CWObject_Ext_Model::GetReflection(CCollisionInfo *_pInfo)
{
	MAUTOSTRIP(CWObject_Ext_Model_GetReflection, CMat4Dfp32());
	CMat4Dfp32 Mat;

	if(_pInfo && _pInfo->m_bIsValid)
	{
		Mat.Unit();
		CVec3Dfp32 Fwd = CVec3Dfp32::GetMatrixRow(GetPositionMatrix(), 0);
		CVec3Dfp32 Res;
		Fwd.Reflect(_pInfo->m_Plane.n, Res);
		Res.SetMatrixRow(Mat, 0);
		CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 2);
		Mat.RecreateMatrix(0, 2);
//		CVec3Dfp32::GetMatrixRow(Mat, 3) = GetPosition() + _pInfo->m_Plane.n;
		CVec3Dfp32::GetMatrixRow(Mat, 3) = _pInfo->m_Pos + _pInfo->m_Plane.n;
//		ConOut("(CWObject_Ext_Model::GetReflection) " + CVec3Dfp32::GetMatrixRow(Mat, 3).GetString());
	}
	else
	{
//		ConOut("(CWObject_Ext_Model::GetReflection) No collision info.");
		Mat = GetPositionMatrix();
	}

	return Mat;
}

/*
bool CWObject_Ext_Model::TraceRay(float _Range, CCollisionInfo &_Info)
{
	MAUTOSTRIP(CWObject_Ext_Model_TraceRay, false);
	bool Res = m_pWServer->Phys_IntersectLine(GetPosition(), GetPosition() + GetForward() * _Range, 0, OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_CHARACTER, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &_Info);
	if(!Res)
		return false;
	return _Info.m_bIsValid != 0;
}
*/

void CWObject_Ext_Model::Line(const CVec3Dfp32& _Pos0, const CVec3Dfp32& _Pos1, int _Col)
{
	MAUTOSTRIP(CWObject_Ext_Model_Line, MAUTOSTRIP_VOID);
	CMat4Dfp32 Mat;
	Mat.Unit();
	_Pos0.SetMatrixRow(Mat, 3);
	int iObj = m_pWServer->Object_Create("Line", Mat);
	if(iObj >= 0)
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_PRIMITIVES_LINE, _Col, 0, -1, 0 , _Pos0, _Pos1), iObj);
}

bool CWObject_Ext_Model::InitializeDebris(CWObject_Client* _pObj, CWorld_Client* _pWClient, int _iSlot)
{
	MAUTOSTRIP(CWObject_Ext_Model_InitializeDebris, false);
	if(!_pObj->m_lspClientObj[_iSlot])
	{
		for(int i = 0; i < CWO_NUMMODELINDICES; i++)
		{
			//JK-FIX: Don't do this with TDynamicCast
			CXR_Model_Debris *pModel = TDynamicCast<CXR_Model_Debris >(_pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]));
			if(pModel)
			{
				// This is the first tick, and Model[i] is a debris model.
				// Compile debris trail
				_pObj->m_lspClientObj[_iSlot] = pModel->Compile(MRTC_RAND(), _pObj->GetPositionMatrix(), _pWClient);
				return true;
			}

		}
	}
	return false;
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Ext_Model, CWObject_Model, 0x0100);

#ifndef M_DISABLE_TODELETE
// -------------------------------------------------------------------
//  CWObject_Ext_ModelEmitter
// -------------------------------------------------------------------
CWObject_Ext_ModelEmitter::CWObject_Ext_ModelEmitter()
{
	MAUTOSTRIP(CWObject_Ext_ModelEmitter_ctor, MAUTOSTRIP_VOID);	
	m_CreationGameTick = 10000; //Don't "trigger" the effect at startup
	m_CreationGameTickFraction = 0.0f;
	m_Freq = 0;
	m_MinDuration = 2;
}

void CWObject_Ext_ModelEmitter::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Ext_ModelEmitter_OnEvalKey, MAUTOSTRIP_VOID);
	
	const CStr KeyName = _pKey->GetThisName();
	
	switch (_KeyHash)
	{
	case MHASH3('FREQ','UENC','Y'): // "FREQUENCY"
		{
			m_Freq = 1.0f / (_pKey->GetThisValuef() * SERVER_TICKSPERSECOND);
			break;
		}
	
	case MHASH3('MIND','URAT','ION'): // "MINDURATION"
		{
			m_MinDuration = _pKey->GetThisValuef() * SERVER_TICKSPERSECOND;
			break;
		}
	
	default:
		{
			CWObject_Ext_Model::OnEvalKey(_pKey);
			break;
		}
	}
	return;
}

void CWObject_Ext_ModelEmitter::OnRefresh()
{
	MAUTOSTRIP(CWObject_Ext_ModelEmitter_OnRefresh, MAUTOSTRIP_VOID);
	if (GetAnimTick(m_pWServer) > m_MinDuration)
	{
		if(Random < m_Freq)	
			SetAnimTick(m_pWServer, 0, 0);
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Ext_ModelEmitter, CWObject_Ext_Model, 0x0100);
#endif

//-------------------------------------------------------------------
//- CWObject_Damage -------------------------------------------------
//-------------------------------------------------------------------

void CWObject_Damage::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Damage_OnEvalKey, MAUTOSTRIP_VOID);
	
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	switch (_KeyHash)
	{
	case MHASH2('DAMA','GE'): // "DAMAGE"
		{
			m_Damage.m_Damage = KeyValue.Val_int();
			break;
		}
	case MHASH3('DAMA','GETY','PE'): // "DAMAGETYPE"
		{	// Syntax: *DAMAGETYPE <list of types>[,<>]
			CFStr Str = KeyValue;
			if (Str != "")
			{
				CFStr Str2 = Str.GetStrSep(","); Str2.Trim();
				m_Damage.m_DamageType = (uint32)Str2.TranslateFlags(CRPG_Object::ms_DamageTypeStr);
				if (Str != "")
				{
					m_Damage.m_DeliverDelay = (fp32)Str.GetStrSep(",").Val_fp64();
				}
			}
			break;
		}
	case MHASH4('DAMA','GE_E','FFEC','T'): // "DAMAGE_EFFECT"
		{
			m_DamageEffect = KeyValue;
			break;
		}
	case MHASH3('SHOC','KWAV','E'): // "SHOCKWAVE"
		{
			m_Shockwave.Parse(KeyValue, CRPG_Object::ms_DamageTypeStr);
			break;
		}
	default:
		{
			CWObject_Damage_Parent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

//-------------------------------------------------------------------

void CWObject_Damage::OnInitInstance(const aint* _pParam, int _nParam)
{
	MAUTOSTRIP(CWObject_Damage_OnInitInstance, MAUTOSTRIP_VOID);
	CWObject_Damage_Parent::OnInitInstance(_pParam, _nParam);
	
	if (_nParam == 0)
		return;

	CRPG_InitParams* pInitParams = (CRPG_InitParams*)_pParam[0];

	if (pInitParams == NULL)
		return;

	// Add any specific damage delivery flags to the damagetype
	if (pInitParams->m_DamageDeliveryFlags)
	{
		m_Damage.m_DamageType |= pInitParams->m_DamageDeliveryFlags;
	}

	// Merge weapon and projectile damage/shockwave.
	if (pInitParams->m_pDamage != NULL)
		CWO_Damage::Merge(m_Damage, *pInitParams->m_pDamage, m_Damage);

	if (pInitParams->m_pShockwave != NULL)
		CWO_Shockwave::Merge(m_Shockwave, *pInitParams->m_pShockwave, m_Shockwave);

	// Set weapon damage effect only if projectile doesn't specify one of it's own.
	if ((m_DamageEffect == "") && (pInitParams->m_pDamageEffect != NULL))
		m_DamageEffect = pInitParams->m_pDamageEffect;

/*
	// Boost damage based on skill.
	if ((pInitParams->m_pRPGCreator != NULL) && (pInitParams->m_pRPGCreatorItem != NULL))
	{
		CRPG_Object_Char* pCreatorChar = TDynamicCast<CRPG_Object_Char>(pInitParams->m_pRPGCreator);
		if (pCreatorChar != NULL)
		{
			fp32 DamageBoost = 1.0f;

			// Skill Boost
			DamageBoost *= (fp32)(pCreatorChar->GetDamageBoost(pInitParams->m_pRPGCreatorItem->m_iSkill)) / 255.0f;

			// Good Aiming Boost (Inside targetring for quite a while)
			CWObject_Character* pChar = TDynamicCast<CWObject_Character>(m_pWServer->Object_Get(pInitParams->m_iCreator));
			if (pChar != NULL)
			{
				CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pChar);
				if (pCD != NULL)
				{
					if (pCD->m_InsideRingTime > PLAYER_TARGETRING_BOOST_THRESHOLD)
						DamageBoost *= PLAYER_TARGETRING_BOOST_FACTOR;
				}
			}

			m_Damage.m_Damage *= DamageBoost;
		}
	}
*/

	m_iOwner = pInitParams->m_iCreator;
}

aint CWObject_Damage::OnMessage(const CWObject_Message &_Msg)
{
	MAUTOSTRIP(CWObject_Damage_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAME_NOTIFYCUTSCENE:
		Destroy();
		return 1;
	}
	
	return CWObject_Damage_Parent::OnMessage(_Msg);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Damage, CWObject_Damage_Parent, 0x0100);


#ifndef M_DISABLE_TODELETE
//-------------------------------------------------------------------
//- CWObject_PainBox ------------------------------------------------
//-------------------------------------------------------------------

void CWObject_PainBox::OnCreate()
{
	MAUTOSTRIP(CWObject_PainBox_OnCreate, MAUTOSTRIP_VOID);
	CWObject_PainBox_Parent::OnCreate();

	for (int i = 0; i < NUMDAMAGEDOBJECTS; i++)
	{
		m_DamagedObject[i] = 0;
		m_DamageDeliveredi] = 0.0f;
	}
}

//-------------------------------------------------------------------

void CWObject_PainBox::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_PainBox_OnEvalKey, MAUTOSTRIP_VOID);
	CWObject_PainBox_Parent::OnEvalKey(_pKey);
}

//-------------------------------------------------------------------

void CWObject_PainBox::OnRefresh()
{
	MAUTOSTRIP(CWObject_PainBox_OnRefresh, MAUTOSTRIP_VOID);
	bool bLog = false;
	bool bObjectsKnown = false;
	
	// Increase number of ticks since damage last was delivered.
	for (int i = 0; i < NUMDAMAGEDOBJECTS; i++)
		if (m_DamagedObject[i] != 0)
		{
			m_DamageDelivered[i] += m_pWServer->GetGameTickTime();
			bObjectsKnown = true;
		}

	if (bLog && bObjectsKnown)
	{
		ConOutL(CStrF("PainBox - Updating; iObj/Delay = { <%d,%.1f>, <%d,%.1f>, <%d,%.1f>, <%d,%.1f>, <%d,%.1f> }",
					 m_DamagedObject[0], m_DamageDelivered[0],
					 m_DamagedObject[1], m_DamageDelivered[1],
					 m_DamagedObject[2], m_DamageDelivered[2],
					 m_DamagedObject[3], m_DamageDelivered[3],
					 m_DamagedObject[4], m_DamageDelivered[4]));
	}

	CWObject_PainBox_Parent::OnRefresh();
}

//-------------------------------------------------------------------

int CWObject_PainBox::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_PainBox_OnMessage, 0);
	bool bLog = false;

	switch(_Msg.m_Msg)
	{
		case OBJSYSMSG_NOTIFY_INTERSECTION:
		{
			if (m_Damage.m_DeliverDelay <= 0)
				return 0;

			int i;
			int iIntersectingObject = _Msg.m_Param0;

			// Find previously damaged object
			for (i = 0; i < NUMDAMAGEDOBJECTS; i++)
			{
				if (m_DamagedObject[i] == iIntersectingObject)
				{
					if (m_DamageDelivered[i] >= m_Damage.m_DeliverDelay)
					{
						if (bLog) ConOutL(CStrF("PainBox - Delivering; iObj = %d, Delay = %.1f", m_DamagedObject[i], m_DamageDelivered[i]));

						m_DamageDelivered[i] = 0.0f;
						return OnIntersectingObject(iIntersectingObject, false);
					}
					else
					{
						if (bLog) ConOutL(CStrF("PainBox - Ignoring; iObj = %d, Delay = %.1f", m_DamagedObject[i], m_DamageDelivered[i]));
						return 0;
					}
				}
			}

			// Object wasn't recently damaged.
			// Find LRU object and replace it.
			int iOldestDamagedObject = 0; // Not system wide object id, merely local array index.
			fp32 OldestDelivery = 0.0f;
			for (i = 0; i < NUMDAMAGEDOBJECTS; i++)
			{
				if (m_DamageDelivered[i] > OldestDelivery)
				{
					iOldestDamagedObject = i;
					OldestDelivery = m_DamageDelivered[i];
				}
			}

			m_DamagedObject[iOldestDamagedObject] = iIntersectingObject;
			m_DamageDelivered[iOldestDamagedObject] = 0.0f;

			if (bLog) ConOutL(CStrF("PainBox - Delivering; iObj = %d (ADDED)", m_DamagedObject[iOldestDamagedObject]));

			return OnIntersectingObject(iIntersectingObject, true);
		}
		break;
	}

	return CWObject_PainBox_Parent::OnMessage(_Msg);
}

//-------------------------------------------------------------------

int CWObject_PainBox::OnIntersectingObject(int _iObj, bool _bFirstTime)
{
	MAUTOSTRIP(CWObject_PainBox_OnIntersectingObject, 0);
	if (m_Damage.IsValid())
		if (_bFirstTime)
			m_Damage.Send(_iObj, m_iOwner, m_pWServer, GetPosition(), m_DamageEffect);
		else
			m_Damage.Send(_iObj, m_iOwner, m_pWServer, GetPosition());

	return 1;
}

//-------------------------------------------------------------------

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_PainBox, CWObject_PainBox_Parent, 0x0100);
#endif

//-------------------------------------------------------------------
// Projectile
//-------------------------------------------------------------------
/*
void CWObject_Projectile::OnCreate()
{
	MAUTOSTRIP(CWObject_Projectile_OnCreate, MAUTOSTRIP_VOID);
	CWObject_Projectile_Parent::OnCreate();

	m_PhysAttrib.m_Elasticy = 0;
//	m_TrailFreq = SERVER_TICKSPERSECOND;
	m_ExplosionType = 0;
	m_ExplosionAlignAxis = 0;

	m_ImpactForce = 0;

	m_RenderOffset_MaxBlendTime = RENDEROFFSET_BLENDTIME_MAX;

	m_MinVelocity = 0;
	m_MaxVelocity = 0;
	m_Velocity = 0;
	m_RotVelocity = 0;
	m_AttachObjType = 0;
	m_iAttachObject = 0;
	m_bHit = 0;

	m_bAttachAligned = false;
	m_iAttachAlignedAxis = 0;
	m_AttachOnObjectFlags = 0;

	m_CollisionObjects = 0;

	m_TimeOutEffectFrontOffset = 0;
	m_TimeOutEffectInheritVelocity = 0;

	m_Data[3] = ~1; // Inverted ModelFlags == 1.
	m_Data[6] = CVec3Dfp32(0).Pack24(RENDEROFFSET_MAX); // RenderOffset
	m_Data[7] = 0; // Local Attach Node

	m_iSoundHit = 0;
	m_iSoundHitWorld = 0;
	m_iSoundHitChar = 0;

	m_Flags = 0;
	m_RemoveAttachedModel = 0;
}

void CWObject_Projectile::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CWObject_Projectile_OnIncludeTemplate, MAUTOSTRIP_VOID);
	CWObject_Projectile_Parent::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

	IncludeClassFromKey("ATTACHCLASS", _pReg, _pMapData);
	IncludeClassFromKey("ATTACHCLASSWORLD", _pReg, _pMapData);
	IncludeClassFromKey("ATTACHCLASSCHAR", _pReg, _pMapData);

	if (_pReg != NULL)
	{
		CRegistry *pChild = _pReg->FindChild("TIMEOUT_EFFECT");
		if (pChild != NULL)
		{
			CFStr Value = pChild->GetThisValue();
			CFStr TimeOutEffect = Value.GetStrSep(",");
			_pMapData->GetResourceIndex_Class(TimeOutEffect);
		}
	}

	IncludeClassFromKey("EXPLOSION", _pReg, _pMapData);
	IncludeClassFromKey("EXPLOSIONUW", _pReg, _pMapData);
	IncludeClassFromKey("TRAIL", _pReg, _pMapData);
	IncludeClassFromKey("TRAILUW", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_LRP", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_HIT", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_HITCHAR", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_HITWORLD", _pReg, _pMapData);
}

void CWObject_Projectile::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Projectile_OnEvalKey, MAUTOSTRIP_VOID);
	
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	int KeyValuei = KeyValue.Val_int();
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();
	
	switch (_KeyHash)
	{
	case MHASH2('VELO','CITY'): // "VELOCITY"
		{
			CFStr Str = KeyValue;

			m_MinVelocity = Str.GetStrSep("-").Val_fp64();

			if (Str != "")
				m_MaxVelocity = Str.Val_fp64();
			else
				m_MaxVelocity = m_MinVelocity;

			m_Velocity = LERP(m_MinVelocity, m_MaxVelocity, (MRTC_RAND() & 255) / 255.0f);
			break;
		}

	case MHASH3('ROTV','ELOC','ITY'): // "ROTVELOCITY"
		{
			m_RotVelocity.ParseString(KeyValue);
			break;
		}

	case MHASH4('ROTV','ELCC','ITYR','AND'): // "ROTVELCCITYRAND"
		{
			m_RotVelocity.ParseString(KeyValue);
			m_RotVelocity[1] *= 2 * (Random - 0.5f);
			m_RotVelocity[2] *= 2 * (Random - 0.5f);
			m_RotVelocity[3] *= 2 * (Random - 0.5f);
			break;
		}

	case MHASH6('MAXR','ENDE','ROFF','SETB','LEND','TIME'): // "MAXRENDEROFFSETBLENDTIME"
		{
			m_RenderOffset_MaxBlendTime = KeyValuef;
			break;
		}
	
	case MHASH3('IMPA','CTFO','RCE'): // "IMPACTFORCE"
		{
			m_ImpactForce = KeyValuef;
			break;
		}

	case MHASH4('TIME','OUT_','EFFE','CT'): // "TIMEOUT_EFFECT"
		{
			CFStr Value = KeyValue;
		
			m_TimeOutEffect = Value.GetStrSep(",");
			if (Value != "")
			{
				m_TimeOutEffectFrontOffset = Value.GetStrSep(",").Val_fp64();
				if (Value != "")
				{
					m_TimeOutEffectInheritVelocity = Value.Val_fp64();
				}
			}
			break;
		}

	// ATTACH ----------------------------------------------------------

	case MHASH3('ATTA','CHCL','ASS'): // "ATTACHCLASS"
		{
			m_AttachClassWorld = KeyValue;
			m_AttachClassChar = KeyValue;
			break;
		}

	case MHASH4('ATTA','CHCL','ASSW','ORLD'): // "ATTACHCLASSWORLD"
		{
			m_AttachClassWorld = KeyValue;
			break;
		}

	case MHASH4('ATTA','CHCL','ASSC','HAR'): // "ATTACHCLASSCHAR"
		{
			m_AttachClassChar = KeyValue;
			break;
		}

	case MHASH4('ATTA','CHAL','IGNE','D'): // "ATTACHALIGNED"
		{
			m_bAttachAligned = KeyValuei != 0;
			break;
		}

	case MHASH5('ATTA','CHAL','IGNE','DAXI','S'): // "ATTACHALIGNEDAXIS"
		{
			m_iAttachAlignedAxis = KeyValuei;
			break;
		}

	switch (_KeyHash)
	{
	case MHASH4('ATTA','CHON','OBJE','CTS'): // "ATTACHONOBJECTS"
		{
			const char* lpObjFlags[] = { "CHARACTERS", "WORLD", "ALL", "NOTCHARACTERS", NULL };
			int AttachObjFlags = KeyValue.TranslateFlags(lpObjFlags);
			if (AttachObjFlags & 1) m_AttachOnObjectFlags |= OBJECT_FLAGS_CHARACTER;
			if (AttachObjFlags & 2) m_AttachOnObjectFlags |= OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
			if (AttachObjFlags & 4) m_AttachOnObjectFlags |= -1;
			if (AttachObjFlags & 8) m_AttachOnObjectFlags &= ~OBJECT_FLAGS_CHARACTER;
			break;
		}

	switch (_KeyHash)
	{
	case MHASH4('COLL','ISIO','NOBJ','ECTS'): // "COLLISIONOBJECTS"
		{
			const char* lpObjFlags[] = { "CHARACTERS", "WORLD", "ALL", "NOTCHARACTERS", NULL };
			int ObjFlags = KeyValue.TranslateFlags(lpObjFlags);
			if (ObjFlags & 1) m_CollisionObjects |= OBJECT_FLAGS_CHARACTER;
			if (ObjFlags & 2) m_CollisionObjects |= OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
			if (ObjFlags & 4) m_CollisionObjects |= -1;
			if (ObjFlags & 8) m_CollisionObjects &= ~OBJECT_FLAGS_CHARACTER;
			break;
		}

	// EXPLOSION -------------------------------------------------------

	case MHASH3('EXPL','OSIO','N'): // "EXPLOSION"
		{
			m_Explosion = KeyValue;
			break;
		}

	case MHASH3('EXPL','OSIO','NUW'): // "EXPLOSIONUW"
		{
			m_ExplosionUW = KeyValue;
			break;
		}

	case MHASH4('EXPL','OSIO','NTYP','E'): // "EXPLOSIONTYPE"
		{		static const char* lpExplosionTypeFlags[] = { "REFLECTED", "ALIGNED", NULL };
			m_ExplosionType = KeyValue.TranslateFlags(lpExplosionTypeFlags);
			if (m_ExplosionType == 1)
				m_ExplosionType = 0;
			else if (m_ExplosionType == 2)
				m_ExplosionType = 1;
			else if (m_ExplosionType == 0)
				m_ExplosionType = KeyValuei;
			else
				m_ExplosionType = 1;
			break;
		}
	case MHASH5('EXPL','OSIO','NALI','GNAX','IS'): // "EXPLOSIONALIGNAXIS"
		{
			m_ExplosionAlignAxis = KeyValuei;
			break;
		}


	case MHASH2('FLAG','S'): // "FLAGS"
		{
			static const char *FlagsTranslate[] =
			{
				"NODESTROY", "NOATTACH", "BOUNCE", "NODAMAGEONIMPACT", "EXPLODEONTIMEOUT", "KEEPLOOPING", "TRUEOFFSET", NULL
			};

			m_Flags = KeyValue.TranslateFlags(FlagsTranslate);
			break;
		}

	// Added by Mondelore.
	case MHASH5('REMO','VEAT','TACH','EDMO','DEL'): // "REMOVEATTACHEDMODEL"
		{
			static const char *ModelIndexTranslate[] = { "MODEL0", "MODEL1", "MODEL2", "MODEL3", NULL };
			m_RemoveAttachedModel = KeyValue.TranslateFlags(ModelIndexTranslate);
			break;
		}

	case MHASH3('SHOC','KWAV','E'): // "SHOCKWAVE"
		{
			m_Shockwave.Parse(KeyValue, CRPG_Object::ms_DamageTypeStr);
			break;
		}

	case MHASH3('SOUN','D_HI','T'): // "SOUND_HIT"
		{
			m_iSoundHit = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_HI','TWOR','LD'): // "SOUND_HITWORLD"
		{
			m_iSoundHitWorld = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_HI','TCHA','R'): // "SOUND_HITCHAR"
		{
			m_iSoundHitChar = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	default:
		{
			CWObject_Projectile_Parent::OnEvalKey(_pKey);
			break;
		}
	}
}

void CWObject_Projectile::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_Projectile_OnFinishEvalKeys, MAUTOSTRIP_VOID);
	CWObject_Projectile_Parent::OnFinishEvalKeys();
}

//-------------------------------------------------------------------

void CWObject_Projectile::OnInitInstance(const int32* _pParam, int _nParam)
{
	MAUTOSTRIP(CWObject_Projectile_OnInitInstance, MAUTOSTRIP_VOID);
	CWObject_Projectile_Parent::OnInitInstance(_pParam, _nParam);

	if (_nParam == 0)
		return;

	CRPG_InitParams* pInitParams = (CRPG_InitParams*)_pParam[0];

	if (pInitParams != NULL)
	{
		if (pInitParams->m_VelocityType == CRPG_InitParams::VelocityAbsolute)
			m_Velocity = pInitParams->m_Velocity;
		else if (pInitParams->m_VelocityType == CRPG_InitParams::VelocityFraction)
		{
			m_Velocity = LERP(m_MinVelocity, m_MaxVelocity, pInitParams->m_Velocity);
			//ConOut(CStrF("Projectile %s released with %3.2f charge.", GetName(), pInitParams->m_Velocity * 100.0f));
		}

		if (pInitParams->m_pRPGCreatorItem != NULL)
		{
			if (pInitParams->m_pRPGCreatorItem->m_Flags & RPG_ITEM_FLAGS_AUTOAIM)
			{
				CMat4Dfp32 AutoAimMatrix;
				CWObject* pCreatorChar = m_pWServer->Object_Get(pInitParams->m_iCreator);
				CWObject_Character* pChar = CWObject_Character::IsCharacter(pCreatorChar);
				if ((pChar != NULL) && (!pChar->IsBot()))
				{
					pChar->GetActivatePosition(AutoAimMatrix, true, m_Damage.m_DamageType);
//					CWObject_Character::GetActivatePosition(pCreatorChar, m_pWServer, &AutoAimMatrix, true, m_Damage.m_DamageType);
					SetPosition(AutoAimMatrix);
				}
			}
				
			if (m_Flags & FLAGS_TRUEOFFSET)
			{
				// Extrapolation needed by release summon (Bomb). Not really sure why though.
				CMat4Dfp32 EffectMatrix; pInitParams->m_pRPGCreatorItem->GetCurEffectMatrix(EffectMatrix, -1, 2*SERVER_TIMEPERFRAME);
				SetPosition(CVec3Dfp32::GetMatrixRow(EffectMatrix, 3));
			}
			else
			{
				// FIXME: Do we need extrapolation here too?!
				CMat4Dfp32 EffectMatrix; pInitParams->m_pRPGCreatorItem->GetCurEffectMatrix(EffectMatrix);
				CVec3Dfp32 FakeEmitPos = CVec3Dfp32::GetMatrixRow(EffectMatrix, 3);
				CVec3Dfp32 EmitPos = GetPosition();
				CVec3Dfp32 RenderOffset = FakeEmitPos - EmitPos;
				m_Data[6] = RenderOffset.Pack24(RENDEROFFSET_MAX);
		
				fp32 BlendTime;
				if (m_Velocity > 0)
				{
					BlendTime = 2.0f * RENDEROFFSET_BLENDTIME_SUBPRECISION * RENDEROFFSET_BLENDTIME_MAX / m_Velocity;
	//				BlendTime = Min(RENDEROFFSET_BLENDTIME_MAX, BlendTime);
					BlendTime = Min(m_RenderOffset_MaxBlendTime, BlendTime);
				}
				else
					BlendTime = 0;

				m_Data[6] |= (uint8(BlendTime * RENDEROFFSET_BLENDTIME_SUBPRECISION) << 24);

				Data(6) = m_Data[6];
			}
		}
	}

	{
		SetVelocity(GetForward() * m_Velocity);
	//	SetRotVelocity(CAxisRotfp32(GetForward(), m_RotVelocity));
		if (m_RotVelocity != 0)
		{
			CMat4Dfp32 RotationMatrix;
			RotationMatrix.Unit();
			RotationMatrix.M_x_RotZ(m_RotVelocity[2]);
			RotationMatrix.M_x_RotY(m_RotVelocity[1]);
			RotationMatrix.M_x_RotX(m_RotVelocity[0]);
			m_pWServer->Object_SetRotVelocity(m_iObject, CAxisRotfp32(RotationMatrix));
			//m_ClientFlags |= CWO_CLIENTFLAGS_ROTVELOCITY;
		}
	}

//	m_pWServer->Debug_RenderVector(CVec3Dfp32::GetMatrixRow(GetPositionMatrix(), 3), GetMoveVelocity(), 0xFF00FFFF, 20);
}

//-------------------------------------------------------------------

void CWObject_Projectile::OnRefreshAttached()
{
	MAUTOSTRIP(CWObject_Projectile_OnRefreshAttached, MAUTOSTRIP_VOID);
	if (GetMoveVelocity().LengthSqr() > 0.001f)
	{
		SetVelocity(0);
		SetRotVelocity(CAxisRotfp32(GetForward(), 0));
		SetDirty(-1);
	}
}

void CWObject_Projectile::OnRefresh()
{
	MAUTOSTRIP(CWObject_Projectile_OnRefresh, MAUTOSTRIP_VOID);
	if (m_ClientFlags & CLIENTFLAGS_ATTACHED)
	{
		OnRefreshAttached();
		return;
	}

	OnRefreshVelocity();

	if(!(m_ClientFlags & CLIENTFLAGS_NOTMOVING))
		m_pWServer->Object_MovePhysical(m_iObject);

	CWObject_Ext_Model::OnRefresh();
}

const char* CWObject_Projectile::GetExplosion(CVec3Dfp32 _Pos)
{
	MAUTOSTRIP(CWObject_Projectile_GetExplosion, NULL);
	if (IsInLiquid(_Pos) && m_ExplosionUW != "")
		return m_ExplosionUW;
	else
		return m_Explosion;
}

void CWObject_Projectile::OnTimeout()
{
	MAUTOSTRIP(CWObject_Projectile_OnTimeout, MAUTOSTRIP_VOID);
	if (m_TimeOutEffect != "")
	{
		CMat4Dfp32 Matrix = GetPositionMatrix();

		if (m_TimeOutEffectFrontOffset > 0)
			(GetMoveVelocity() * m_TimeOutEffectFrontOffset).AddMatrixRow(Matrix, 3);
		int iObj = m_pWServer->Object_Create(m_TimeOutEffect, Matrix, m_iOwner);

		Matrix = m_pWServer->Object_GetPositionMatrix(iObj);
//		m_pWServer->Debug_RenderMatrix(Matrix, 20.0f);
//		m_pWServer->Debug_RenderWire(0, CVec3Dfp32::GetMatrixRow(Matrix, 3), 0xFF00FF00, 20.0f);

		if (m_TimeOutEffectInheritVelocity > 0.0f)
			m_pWServer->Object_SetVelocity(iObj, GetMoveVelocity() * m_TimeOutEffectInheritVelocity);

		m_TimeOutEffect.Clear(); // We have to clear this, since it's the only way this code does not run again (so far).
	}

	m_bPendingDestroy = (m_DestroyDelayTicks > 0);
	if (m_Flags & FLAGS_EXPLODEONTIMEOUT)
	{
		Explode(0, GetPosition(), NULL, !m_bPendingDestroy);
	}
	else
	{
		if (!m_bPendingDestroy)
			Destroy();
	}
}

void CWObject_Projectile::PlayHitSounds(CCollisionInfo* _pCInfo, bool _bScaleByVelocity)
{
	MAUTOSTRIP(CWObject_Projectile_PlayHitSounds, MAUTOSTRIP_VOID);
	fp32 ImpactVelocity;

	if (_bScaleByVelocity)
	{
		CVec3Dfp32 Velocity = GetMoveVelocity();
		CVec3Dfp32 Normal = _pCInfo->m_Plane.n;
		ImpactVelocity = -(Velocity * Normal);
	}
	else
		ImpactVelocity = 1.0f;
	
	if (ImpactVelocity < 1.0f)
	{
		if (ImpactVelocity > 0.1f)
		{
			fp32 ImpactFraction = (ImpactVelocity - 0.1f) / (1.0f - 0.1f);
			m_pWServer->Sound_At(_pCInfo->m_Pos, m_iSoundHit, 0, 0, ImpactFraction);
			if (GetObjFlags(_pCInfo->m_iObject, m_pWServer) & OBJECT_FLAGS_CHARACTER)
				m_pWServer->Sound_At(_pCInfo->m_Pos, m_iSoundHitChar, 0, 0, ImpactFraction);
			else
				m_pWServer->Sound_At(_pCInfo->m_Pos, m_iSoundHitWorld, 0, 0, ImpactFraction);
		}
	}
	else
	{
		m_pWServer->Sound_At(_pCInfo->m_Pos, m_iSoundHit, 0);
		if (GetObjFlags(_pCInfo->m_iObject, m_pWServer) & OBJECT_FLAGS_CHARACTER)
			m_pWServer->Sound_At(_pCInfo->m_Pos, m_iSoundHitChar, 0);
		else
			m_pWServer->Sound_At(_pCInfo->m_Pos, m_iSoundHitWorld, 0);
	}
}

void CWObject_Projectile::StopLoopingSounds()
{
	MAUTOSTRIP(CWObject_Projectile_StopLoopingSounds, MAUTOSTRIP_VOID);
	// Stop playing looping sounds.
	if (!(m_Flags & FLAGS_KEEPLOOPING))
	{
		m_iSound[0] = 0;
		m_iSound[1] = 0;
	}
}

void CWObject_Projectile::Attach(CCollisionInfo* _pCInfo)
{
	MAUTOSTRIP(CWObject_Projectile_Attach, MAUTOSTRIP_VOID);
	int ObjFlags = GetObjFlags(_pCInfo->m_iObject, m_pWServer);
	if (!(ObjFlags & m_AttachOnObjectFlags))
		return;
	
	CFStr AttachClass;
	if (ObjFlags & OBJECT_FLAGS_CHARACTER)
		AttachClass = (const char*)((m_AttachClassChar != "") ? m_AttachClassChar : m_AttachClassWorld);
	else
		AttachClass = (const char*)((m_AttachClassWorld != "") ? m_AttachClassWorld : m_AttachClassChar);

	if (AttachClass != "")
	{
		CMat4Dfp32 AttachMatrix = GetPositionMatrix();
		int iAttachObj = CWObject_AttachModel::CreateAndAttach(AttachClass, m_pWServer, m_iOwner, AttachMatrix, *_pCInfo, m_bAttachAligned, m_iAttachAlignedAxis);
	}
}

void CWObject_Projectile::Explode(int _iObject, CVec3Dfp32 _Pos, CCollisionInfo *_pInfo, bool _bDestroy)
{
	MAUTOSTRIP(CWObject_Projectile_Explode, MAUTOSTRIP_VOID);
//	const char* pExplName = GetExplosion(_Pos);

	if (m_Explosion != "")
	{
		CMat4Dfp32 Mat;

		if (_pInfo && _pInfo->m_bIsValid)
		{
			if (m_ExplosionType == 0)
			{
				Mat = GetReflection(_pInfo);
			}
			else if (m_ExplosionType == 1)
			{
				CVec3Dfp32::GetMatrixRow(Mat, 3) = _pInfo->m_Pos + _pInfo->m_Plane.n * 5.0f;

				switch (m_ExplosionAlignAxis)
				{
					case 0:
					{
						_pInfo->m_Plane.n.SetMatrixRow(Mat, 0);
						if(Abs(_pInfo->m_Plane.n * CVec3Dfp32(0, 1, 0)) < 0.95f)
							CVec3Dfp32(0, 1, 1).SetMatrixRow(Mat, 1);
						else
							CVec3Dfp32(1, 0, 0).SetMatrixRow(Mat, 1);
						Mat.RecreateMatrix(0, 1);
					}
					break;

					case 1:
					{
						_pInfo->m_Plane.n.SetMatrixRow(Mat, 1);
						if(Abs(_pInfo->m_Plane.n * CVec3Dfp32(1, 0, 0)) < 0.95f)
							CVec3Dfp32(1, 0, 0).SetMatrixRow(Mat, 0);
						else
							CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 0);
						Mat.RecreateMatrix(1, 0);
					}
					break;

					case 2:
					{
						_pInfo->m_Plane.n.SetMatrixRow(Mat, 2);
						if(Abs(_pInfo->m_Plane.n * CVec3Dfp32(0, 0, 1)) < 0.95f)
							CVec3Dfp32(1, 0, 0).SetMatrixRow(Mat, 0);
						else
							CVec3Dfp32(0, 1, 0).SetMatrixRow(Mat, 0);
						Mat.RecreateMatrix(2, 0);
					}
					break;
				}
				
			}
		}
		else
		{
			Mat.Unit3x3();
			_Pos.SetMatrixRow(Mat, 3);
		}

		int iExplosionObj = m_pWServer->Object_Create(m_Explosion, Mat, m_iOwner);

		if (iExplosionObj != 0)
		{
			CWObject* pObj = m_pWServer->Object_Get(_iObject);
			if ((pObj != NULL) && (pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER))
			{
				CWObject_Ext_Model* pExplosionObj = safe_cast<CWObject_Ext_Model>(m_pWServer->Object_Get(iExplosionObj));
				if ((pExplosionObj != NULL) && (pExplosionObj->m_MovingSpawnDuration >= 0))
				{
					pExplosionObj->m_Timeout = pExplosionObj->m_MovingSpawnDuration * SERVER_TICKSPERSECOND;
					pExplosionObj->iAnim2() = pExplosionObj->m_Timeout;
				}
			}
		}
		else
		{
			ConOut("(CWObject_Projectile::Explode) Failed to create explosion.");
		}
	}

//	SendShockwave(_Pos, m_ShockwaveRange, m_ShockwaveForce, m_ShockwaveDamage, m_ShockwaveDamageType, 0);
//	SendDamage(_iObject, _Pos, m_Damage, m_DamageType, 0, 0, );
//	SendDamage(_iObject, _Pos, m_iOwner, m_Damage, m_DamageType, _pInfo ? _pInfo->m_SurfaceType : 0, m_ShockwaveDamage, m_ShockwaveRange, GetForward() * m_ImpactForce, NULL);

	if (_iObject && m_Damage.IsValid())
	{
		CVec3Dfp32 Force = GetForward() * m_ImpactForce;
		CVec3Dfp32* pSplatterDir = (_pInfo != NULL) ? &(_pInfo->m_Plane.n) : NULL;
		m_Damage.SendExt(_iObject, m_iOwner, m_pWServer, _pInfo, &Force, pSplatterDir, m_DamageEffect);
	}

	if (m_Shockwave.IsValid())
		m_Shockwave.Send(_Pos, &m_iObject, 1, m_iOwner, m_pWServer);

	// This should not be needed.
	StopLoopingSounds();
	
	if (_bDestroy)
		Destroy();
}

bool CWObject_Projectile::TraceRay(CVec3Dfp32 _Pos1, CVec3Dfp32 _Pos2, CCollisionInfo* _pCInfo, int _CollisionObjects, bool _bExcludeOwner, bool _bDebug)
{
	MAUTOSTRIP(CWObject_Projectile_TraceRay, false);
	bool bDebugRender = _bDebug;
	fp32 DebugRenderDuration = 20.0f;
	
	_pCInfo->m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
	int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
	int32 ObjectFlags = (_CollisionObjects != 0) ? _CollisionObjects: (OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER);
	int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
	int32 iExclude = _bExcludeOwner ? m_iOwner : 0;
	bool bHit = m_pWServer->Phys_IntersectLine(_Pos1, _Pos2, OwnFlags, ObjectFlags, MediumFlags, _pCInfo, iExclude);
	if (bHit)
	{
		if (bDebugRender) m_pWServer->Debug_RenderWire(_Pos1, _pCInfo->m_Pos, 0xFF00FF00, DebugRenderDuration);

		if (_pCInfo->m_bIsValid)
			return true;
		else
			return false;
	}
	else
	{
		if (bDebugRender) m_pWServer->Debug_RenderWire(_Pos1, _Pos2, 0xFFFF0000, DebugRenderDuration);
		return false;
	}
}

int CWObject_Projectile::OnPreIntersection(int _iObject, CCollisionInfo *_pInfo)
{
	MAUTOSTRIP(CWObject_Projectile_OnPreIntersection, 0);
	if (m_bHit) return SERVER_PHYS_DEFAULTHANDLER;
	m_bHit = 1;

	if(!(m_Flags & FLAGS_NODAMAGEONIMPACT) && (m_Flags & FLAGS_NODESTROY))
	{
		CVec3Dfp32 Dir = GetMoveVelocity();
		Dir.Normalize();
//		SendDamage(_iObject, GetPosition(), m_Damage, m_DamageType, m_ShockwaveDamage, m_ShockwaveRange, Dir * m_ImpactForce);
//		SendDamage(_iObject, GetPosition(), m_iOwner, m_Damage, m_DamageType, _pInfo ? _pInfo->m_SurfaceType : 0, m_ShockwaveDamage, m_ShockwaveRange, Dir * m_ImpactForce, NULL);
	}

	if(!(m_Flags & FLAGS_NODESTROY))
	{
		m_bPendingDestroy = (m_DestroyDelayTicks > 0);
		Explode(_iObject, GetPosition(), _pInfo, !m_bPendingDestroy);
		return SERVER_PHYS_ABORT;
	}
	else if (!(m_Flags & FLAGS_NOATTACH))
	{
		if (_pInfo && _pInfo->m_bIsValid)
		{
			CWObject* pObj = m_pWServer->Object_Get(_pInfo->m_iObject);
			if(pObj && pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
			{
				Destroy();
				return SERVER_PHYS_ABORT;
			}

			if (pObj && !(pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_WORLD))
			{
				m_iAttachObject = _pInfo->m_iObject;
				if (CWO_NUMDATA <= 7) Error("OnPreIntersection", "Internal error.");
				m_Data[7] = _pInfo->m_LocalNode;
				if (_pInfo->m_LocalNode)
				{
					m_pWServer->Object_AddChild(m_iAttachObject, m_iObject);

					CMat4Dfp32 Pos;
					GetLocalPositionMatrix().Multiply(_pInfo->m_LocalNodePos, Pos);
					_pInfo->m_LocalNodePos.Multiply(GetLocalPositionMatrix(), Pos);
					SetPosition(Pos);

				ConOutL("LocalNodePos " + _pInfo->m_LocalNodePos.GetString());
					SetPosition(_pInfo->m_LocalNodePos);
				}
				else
				{
					m_pWServer->Object_AddChild(m_iAttachObject, m_iObject);
					SetPosition(_pInfo->m_LocalPos);
				}

				ConOut(CStrF("Attach %d->%d", m_iObject, m_iAttachObject));
			}

			// Set object-position close to impact pos and remove any physics-primitives it might have.
			CMat4Dfp32 Mat = GetPositionMatrix();
			CVec3Dfp32::GetMatrixRow(Mat, 3) = _pInfo->m_Pos + _pInfo->m_Plane.n*1.0f;
			CWO_PhysicsState Phys = GetPhysState();
			Phys.m_nPrim = 0;
			m_pWServer->Object_SetPhysics(m_iObject, Phys);
			m_pWServer->Object_SetPosition(m_iObject, Mat);
		}
		ClientFlags() |= CLIENTFLAGS_ATTACHED;
		
		// Added by Mondelore.
		for (int iModel = 0; iModel < CWO_NUMMODELINDICES; iModel++)
		{
			if ((m_RemoveAttachedModel & (1 << iModel)) != 0)
				m_iModel[iModel] = 0;
		}

		return SERVER_PHYS_ABORT;
	}

	return SERVER_PHYS_DEFAULTHANDLER;
}

CVec3Dfp32 CWObject_Projectile::GetRenderOffset(CWObject_Client* _pObj, fp32 _AnimTime)
{
	MAUTOSTRIP(CWObject_Projectile_GetRenderOffset, CVec3Dfp32());
	fp32 BlendTime = fp32((_pObj->m_Data[6] & 0xff000000) >> 24) / RENDEROFFSET_BLENDTIME_SUBPRECISION;
	fp32 BlendFraction = (BlendTime > 0.0f) ? (1.0f - Max(0.0f, Min(1.0f, _AnimTime / BlendTime))) : 1.0f;

	CVec3Dfp32 RenderOffset;
	RenderOffset.Unpack24(_pObj->m_Data[6], RENDEROFFSET_MAX);
	RenderOffset *= LERP(BlendFraction, Sqr(BlendFraction), 0.75f);

	return RenderOffset;
}

void CWObject_Projectile::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Projectile_OnClientRefresh, MAUTOSTRIP_VOID);
	int ModelFlags = ~(_pObj->m_Data[3]);
	if (ModelFlags == 0)
		ModelFlags = 0;

	CMat4Dfp32 Matrix;
	if (!GetRenderMatrix(_pObj, _pWClient, Matrix, 1.0f))
		return;

	CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(Matrix, 3);

	for(int i = 0; i < CWO_NUMCLIENTOBJ; i++)
	{
		if(_pObj->m_lModelInstances[i] != NULL)
		{
			int AnimTick = _pWClient->GetGameTick() - _pObj->m_AnimTime;
			fp32 AnimTime = AnimTick * SERVER_TIMEPERFRAME;
			(Pos + GetRenderOffset(_pObj, AnimTime)).SetMatrixRow(Matrix, 3);

			_pObj->m_lModelInstances[i]->OnRefresh(_pObj, _pWClient, AnimTick, &Matrix, 1, ModelFlags);
		}
	}
	
	if(!(_pObj->m_ClientFlags & CLIENTFLAGS_ATTACHED))
		_pWClient->Object_ForcePosition(_pObj->m_iObject, _pObj->GetPosition() + _pObj->GetMoveVelocity());
}

void CWObject_Projectile::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Projectile_OnClientRender, MAUTOSTRIP_VOID);
	fp32 AnimTime, TickFrac;
	GetAnimTime(_pObj, _pWClient, AnimTime, TickFrac);

	CMat4Dfp32 Matrix;
	if (!GetRenderMatrix(_pObj, _pWClient, Matrix, TickFrac))
		return;

	// These two lines are the only differance from CWObject_Ext_Model::OnClientRender!
	CVec3Dfp32 RenderOffset = GetRenderOffset(_pObj, AnimTime);
	RenderOffset.AddMatrixRow(Matrix, 3);

	RenderModelsDefault(AnimTime, TickFrac, Matrix, _pObj, _pWClient, _pEngine);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Projectile, CWObject_Ext_Model, 0x0100);


//-------------------------------------------------------------------
// Projectile_Tracer
//-------------------------------------------------------------------
CWObject_Projectile_Tracer::CWObject_Projectile_Tracer()
{
	MAUTOSTRIP(CWObject_Projectile_Tracer_ctor, MAUTOSTRIP_VOID);
//	m_TraceObjFlags = OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
//	m_TraceObjFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
	m_TraceObjFlags = OBJECT_FLAGS_PROJECTILE;
}

void CWObject_Projectile_Tracer::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Projectile_Tracer_OnEvalKey, MAUTOSTRIP_VOID);
	switch (_KeyHash)
	{
	case MHASH4('TRAC','EOBJ','FLAG','S'): // "TRACEOBJFLAGS"
		{
			m_TraceObjFlags = Phys_TranslateObjectFlags(_pKey->GetThisValue());
			break;
		}
	default:
		{
			CWObject_Projectile::OnEvalKey(_pKey);
			break;
		}
	}
}

bool CWObject_Projectile_Tracer::OnTracerIntersection(CCollisionInfo* _pCInfo)
{
	MAUTOSTRIP(CWObject_Projectile_Tracer_OnTracerIntersection, false);
	return true;
}

void CWObject_Projectile_Tracer::OnRefresh()
{
	MAUTOSTRIP(CWObject_Projectile_Tracer_OnRefresh, MAUTOSTRIP_VOID);
	if (m_ClientFlags & CLIENTFLAGS_ATTACHED)
	{
		OnRefreshAttached();
		
		CWObject_Ext_Model::OnRefresh();
		return;
	}

	OnRefreshVelocity();

	if (!(m_ClientFlags & CLIENTFLAGS_NOTMOVING))
	{
		if (GetPhysState().m_nPrim == 0)
		{
			CVec3Dfp32 Dest = GetPosition() + GetMoveVelocity();

			if(m_ClientFlags & CLIENTFLAGS_INLIMBO)
			{
				CXR_MediumDesc Desc;
				m_pWServer->Phys_GetMedium(Dest, &Desc);
				if(!(Desc.m_MediumFlags & XW_MEDIUM_SKY))
				{
					//We have left limbo
					ClientFlags() &= ~CLIENTFLAGS_INLIMBO;

					if(Desc.m_MediumFlags & XW_MEDIUM_SOLID || Desc.m_MediumFlags & XW_MEDIUM_PHYSSOLID)
					{
						//and directly into a wall
						ClientFlags() |= CLIENTFLAGS_NOTMOVING;
						ConOutD("(CWObject_Projectile_Tracer::OnRefresh) Stepped into wall");
					}
					else
					{
						//Trace backwards to check if we went through a wall
						CCollisionInfo CInfo;
						if (TraceRay(Dest, GetPosition(), &CInfo))
						{
							if(CInfo.m_bIsValid && CInfo.m_pSurface && (CInfo.m_pSurface->GetBaseFrame()->m_Medium.m_MediumFlags & XW_MEDIUM_SKY))
							{
								//The way back to limbo is clear. Valid path.
	//							ConOut("(CWObject_Projectile_Tracer::OnRefresh) Left limbo");
								SetPosition(Dest);
							}
							else
							{
								//Couldn't trace back into limbo. The way is probably blocked.
								ConOutD("(CWObject_Projectile_Tracer::OnRefresh) Trace-check failed");
								ClientFlags() |= CLIENTFLAGS_NOTMOVING;
							}
						}
						else
						{
							ConOutD("(CWObject_Projectile_Tracer::OnRefresh) No collision when leaving limbo");
							SetPosition(Dest);
						}
					}
				}
				else
					//Still in limbo
					SetPosition(Dest);
			}
			else
			{
				CCollisionInfo CInfo;
				if (!TraceRay(GetPosition(), Dest, &CInfo, m_CollisionObjects, true, true))
				{
					SetPosition(Dest);
				}
				else
				{
					if (CInfo.m_bIsValid && CInfo.m_pSurface && (CInfo.m_pSurface->GetBaseFrame()->m_Medium.m_MediumFlags & XW_MEDIUM_SKY))
					{
						//We have hit limbo. Lets do some strange stuff.
						ClientFlags() |= CWO_CLIENTFLAGS_LINKINFINITE;
						SetPosition(Dest);

						CXR_MediumDesc Desc;
						m_pWServer->Phys_GetMedium(Dest, &Desc);

						//Check that we haven't already passed through
						if(Desc.m_MediumFlags & XW_MEDIUM_SKY)
						{
							ClientFlags() |= CLIENTFLAGS_INLIMBO;
						}

					}
					else
					{
						if (CInfo.m_bIsValid)
							GetPosition().Lerp(Dest, CInfo.m_Time, Dest);
						else
							Dest = GetPosition();

						SetPosition(Dest);

						bool bDestroy = OnTracerIntersection(&CInfo);
						if (bDestroy)
						{
							PlayHitSounds(&CInfo, true);
							Attach(&CInfo);
							StopLoopingSounds();

							ClientFlags() |= CLIENTFLAGS_NOTMOVING;

							if (!(m_Flags & FLAGS_NOATTACH))
							{
								if (CInfo.m_bIsValid)
								{
									CWObject* pObj = m_pWServer->Object_Get(CInfo.m_iObject);
									if (pObj && pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
									{
										Destroy();
										return;
									}
								ClientFlags() |= CLIENTFLAGS_ATTACHED;

								// Added by Mondelore.
								for (int iModel = 0; iModel < CWO_NUMMODELINDICES; iModel++)
								{
									if ((m_RemoveAttachedModel & (1 << iModel)) != 0)
										m_iModel[iModel] = 0;
								}
							}

							if (!(m_Flags & FLAGS_NODESTROY))
							{
								m_bPendingDestroy = (m_DestroyDelayTicks > 0);
								Explode(CInfo.m_iObject, GetPosition(), &CInfo, !m_bPendingDestroy);
							}
							else if(m_Damage.IsValid())
							{
								CVec3Dfp32 Force = GetForward() * m_ImpactForce;
								CVec3Dfp32* pSplattDir = &(CInfo.m_Plane.n);
								m_Damage.SendExt(CInfo.m_iObject, m_iOwner, m_pWServer, &CInfo, &Force, pSplattDir, m_DamageEffect);
							}
						}
					}
				}
			}
		}
		else
		{
			if (true)
			{
				m_pWServer->Object_MovePhysical(m_iObject);
			}
			else
			{
				CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(GetPositionMatrix(), 3);
				CVec3Dfp32 Velocity = GetMoveVelocity();

				fp32 Scale = 0.5f;
				m_pWServer->Debug_RenderVector(Pos - Velocity * Scale, Velocity * Scale, 0xFFFF0000, 20);

				m_pWServer->Object_MovePhysical(m_iObject);

				CVec3Dfp32 NewVelocity = GetMoveVelocity();
				m_pWServer->Debug_RenderVector(Pos, NewVelocity * Scale, 0xFF00FF00, 20);
			}
		}
	}

	CWObject_Ext_Model::OnRefresh();
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Projectile_Tracer, CWObject_Projectile, 0x0100);

//-------------------------------------------------------------------
// Projectile_Bouncer (By Mondelore)
//-------------------------------------------------------------------

void CWObject_Projectile_Bouncer::OnCreate()
{
	MAUTOSTRIP(CWObject_Projectile_Bouncer_OnCreate, MAUTOSTRIP_VOID);
	CWObject_Projectile_Bouncer_Parent::OnCreate();

	m_AbsorbingObjects = 0;
	m_bExcludeOwner = false;
	m_NumBouncesLeft = -1;
	m_BounceAlignmentLimit = 1.0f;
//	m_BounceEffect = "";
	m_BounceImpactForce = 0;
	m_BounceElasticy = 1.0f;
	m_BounceElasticyVerticalFactor = 1.0f;
	m_BounceFriction = 0.0f;
	m_Gravity = 0;
	m_iBounceSound = 0;
}

void CWObject_Projectile_Bouncer::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CWObject_Projectile_Bouncer_OnIncludeTemplate, MAUTOSTRIP_VOID);
	IncludeClassFromKey("BOUNCE_EFFECT", _pReg, _pMapData);
	IncludeSoundFromKey("BOUNCE_SOUND", _pReg, _pMapData);
	CWObject_Projectile_Bouncer_Parent::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
}

void CWObject_Projectile_Bouncer::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Projectile_Bouncer_OnEvalKey, MAUTOSTRIP_VOID);
	
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	int KeyValuei = KeyValue.Val_int();
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();
	
	switch (_KeyHash)
	{
	case MHASH4('ABSO','RBIN','GOBJ','ECTS'): // "ABSORBINGOBJECTS"
		{
			const char* lpAbsObjFlags[] = { "CHARACTERS", "TRIGGERS", "WORLD", "NOTCHARACTERS", "ALL", NULL };
			int AbsObjFlags= KeyValue.TranslateFlags(lpAbsObjFlags);
			if (AbsObjFlags & 1) m_AbsorbingObjects |= OBJECT_FLAGS_CHARACTER;
			if (AbsObjFlags & 2) m_AbsorbingObjects |= OBJECT_FLAGS_TRIGGER;
			if (AbsObjFlags & 4) m_AbsorbingObjects |= OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
			if (AbsObjFlags & 8) m_AbsorbingObjects &= ~OBJECT_FLAGS_CHARACTER;
			if (AbsObjFlags & 16) m_AbsorbingObjects |= -1;
			break;
		}
	case MHASH3('EXCL','UDEO','WNER'): // "EXCLUDEOWNER"
		{
			m_bExcludeOwner = (KeyValuei != 0);
			break;
		}
	case MHASH3('NUMB','OUNC','ES'): // "NUMBOUNCES"
		{
			m_NumBouncesLeft = KeyValuei;
			break;
		}
	case MHASH6('BOUN','CE_A','LIGN','MENT','LIMI','T'): // "BOUNCE_ALIGNMENTLIMIT"
		{
			m_BounceAlignmentLimit = KeyValuef;
			break;
		}
	case MHASH4('BOUN','CE_E','FFEC','T'): // "BOUNCE_EFFECT"
		{
			m_BounceEffect = KeyValue;
			break;
		}
	case MHASH5('BOUN','CE_E','FFEC','TOFF','SET'): // "BOUNCE_EFFECTOFFSET"
		{
			m_BounceEffectOffset = KeyValuef;
			break;
		}
	case MHASH4('BOUN','CE_D','AMAG','E'): // "BOUNCE_DAMAGE"
		{
			m_BounceDamage.Parse(KeyValue, CRPG_Object::ms_DamageTypeStr);
			break;
		}
	case MHASH5('BOUN','CE_I','MPAC','TFOR','CE'): // "BOUNCE_IMPACTFORCE"
		{
			m_BounceImpactForce = KeyValuef;
			break;
		}
	case MHASH5('BOUN','CE_E','LAST','ICIT','Y'): // "BOUNCE_ELASTICITY"
		{
			m_BounceElasticy = KeyValuef;
			break;
		}
	case MHASH5('BOUN','CE_E','LAST','ICIT','Y_VF'): // "BOUNCE_ELASTICITY_VF"
		{
			m_BounceElasticyVerticalFactor = KeyValuef;
			break;
		}
	case MHASH4('BOUN','CE_F','RICT','ION'): // "BOUNCE_FRICTION"
		{
			m_BounceFriction = KeyValuef;
			break;
		}
	case MHASH2('GRAV','ITY'): // "GRAVITY"
		{
			m_Gravity = KeyValuef;
			break;
		}
	case MHASH3('BOUN','CE_S','OUND'): // "BOUNCE_SOUND"
		{
			m_iBounceSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	default:
		{
			CWObject_Projectile_Bouncer_Parent::OnEvalKey(_pKey);
			break;
		}
	}
}

void CWObject_Projectile_Bouncer::OnInitInstance(const int32* _pParam, int _nParam)
{
	MAUTOSTRIP(CWObject_Projectile_Bouncer_OnInitInstance, MAUTOSTRIP_VOID);
	CWObject_Projectile_Bouncer_Parent::OnInitInstance(_pParam, _nParam);

	m_PhysAttrib.m_Elasticy = m_BounceElasticy;
}

void CWObject_Projectile_Bouncer::OnRefresh()
{
	MAUTOSTRIP(CWObject_Projectile_Bouncer_OnRefresh, MAUTOSTRIP_VOID);
	CWObject_Projectile_Bouncer_Parent::OnRefresh();
}

void CWObject_Projectile_Bouncer::OnRefreshVelocity()
{
	MAUTOSTRIP(CWObject_Projectile_Bouncer_OnRefreshVelocity, MAUTOSTRIP_VOID);
	if (GetPhysState().m_nPrim > 0)
		return;

	if (m_Gravity != 0)
	{
		CVec3Dfp32 Velocity = GetMoveVelocity();
		Velocity[2] -= m_Gravity;
		SetVelocity(Velocity);
	}
}

bool CWObject_Projectile_Bouncer::OnTracerIntersection(CCollisionInfo* _pCInfo)
{
	MAUTOSTRIP(CWObject_Projectile_Bouncer_OnTracerIntersection, false);
//	Kolla på CInfo objectet, om det är en gubbe eller om det är världen.
//	Är det inte världen så returneras bara true, dvs explodera skiten.
//	Är det världen så skall det bouncas.
//	Skapa bounce effekt objectet mha CInfo position och normal.
//	Bounca velocity vectorn mha CInfo normalen.
//	returnera false.
//
//	Allt det här skall givetvis flaggas och parameteriseras genom nyklar.
//
//	Största vinkeln innan bouncen anses vara en direkt splatt träff.
//	Vilka object typer den studsar mot och vilka den absorberas av.
//	Ger den skada när den studsar, eller det får ligga i bounce_effekt objectet?.Näe, det här objectet vet om träffat object direkt.

	if (m_bExcludeOwner && (_pCInfo->m_iObject == m_iOwner))
		return false;
	
	CWObject* pObj = m_pWServer->Object_Get(_pCInfo->m_iObject);

	if (pObj == NULL)
		return false;

	int16 ObjFlags = pObj->GetPhysState().m_ObjectFlags;

	if (ObjFlags & m_AbsorbingObjects)
		return true;
	
	if (m_NumBouncesLeft == 0)
		return true;

//	m_PhysAttrib.m_Elasticy = m_BounceElasticy;

	CVec3Dfp32 Pos, Normal;
	Pos = _pCInfo->m_Pos;
	Normal = _pCInfo->m_Plane.n;
	Pos += Normal * m_BounceEffectOffset;

	CVec3Dfp32 Velocity = GetMoveVelocity();
	CVec3Dfp32 VelocityDir = Velocity; VelocityDir.Normalize();
	fp32 BounceAlignment = (VelocityDir * Normal);
	fp32 ImpactVelocity = -(Velocity * Normal);

	if (BounceAlignment > m_BounceAlignmentLimit)
		return true;

	if ((m_BounceEffect.Len() > 0) && (ImpactVelocity > 0.1f))
	{
		CMat4Dfp32 BounceEffectMatrix;

		if (!_pCInfo->m_bIsValid)
			_pCInfo->m_bIsValid = true;

		Pos.SetMatrixRow(BounceEffectMatrix, 3);
		Normal.SetMatrixRow(BounceEffectMatrix, 0);
		if (M_Fabs(Normal[1]) < 0.75f)
			CVec3Dfp32(0, 1, 0).SetMatrixRow(BounceEffectMatrix, 1);
		else
			CVec3Dfp32(1, 0, 0).SetMatrixRow(BounceEffectMatrix, 1);
		BounceEffectMatrix.RecreateMatrix(0, 1);

//		m_pWServer->Debug_RenderMatrix(BounceEffectMatrix, 20);
		
		int iObj = m_pWServer->Object_Create(m_BounceEffect, BounceEffectMatrix);
		if (iObj == 0)
			iObj = -1;

//		ConOut(CStrF("Pos = <%3.2f, %3.2f, %3.2f>, Normal = <%3.2f, %3.2f, %3.2f>, iObj = %d", Pos[0], Pos[1], Pos[2], Normal[0], Normal[1], Normal[2], iObj));
	}

	if(_pCInfo && m_BounceDamage.IsValid())
	{
		CVec3Dfp32 Force = VelocityDir * BounceAlignment * m_BounceImpactForce;
		m_BounceDamage.SendExt(_pCInfo->m_iObject, m_iOwner, m_pWServer, _pCInfo, &Force);
	}
	
	PlayHitSounds(_pCInfo, true);

	if (GetPhysState().m_nPrim == 0)
	{
		CVec3Dfp32 NormalVelocity = Normal * (Velocity * Normal);
		CVec3Dfp32 AlignedVelocity = Velocity - NormalVelocity;
		CVec3Dfp32 ReflectedVelocity = (AlignedVelocity * (1.0f - m_BounceFriction)) - (NormalVelocity * m_BounceElasticy);
		SetVelocity(ReflectedVelocity);

		if (false)
		{
			fp32 Scale = 0.5f;
			m_pWServer->Debug_RenderVector(Pos - Velocity * Scale, Velocity * Scale, 0xFFFF0000, 20);
			m_pWServer->Debug_RenderVector(Pos, ReflectedVelocity * Scale, 0xFF00FF00, 20);
		}
		
		{
			CMat4Dfp32 Matrix = GetPositionMatrix();
			ReflectedVelocity.SetMatrixRow(Matrix, 0);
			Matrix.RecreateMatrix(0, 1);
			SetPosition(Matrix);
		}
	}
	else
	{
		CVec3Dfp32 NormalVelocity = Normal * (Velocity * Normal);
		CVec3Dfp32 AlignedVelocity = Velocity - NormalVelocity;
//		CVec3Dfp32 ReflectedVelocity = (AlignedVelocity * (1.0f - m_BounceFriction)) + (NormalVelocity * m_BounceElasticy);
		CVec3Dfp32 ReflectedVelocity = (AlignedVelocity * (1.0f - m_BounceFriction)) + NormalVelocity;

		ReflectedVelocity *= LERP(M_Fabs(Normal[2]), 1.0f, m_BounceElasticyVerticalFactor);

		SetVelocity(ReflectedVelocity);
	}

	if (m_NumBouncesLeft > 0)
		m_NumBouncesLeft--;

	return false;
}

int CWObject_Projectile_Bouncer::OnPreIntersection(int _iObject, CCollisionInfo *_pCInfo)
{
	MAUTOSTRIP(CWObject_Projectile_Bouncer_OnPreIntersection, 0);
	return SERVER_PHYS_DEFAULTHANDLER;
}

int CWObject_Projectile_Bouncer::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWObject_Projectile_Bouncer_OnPhysicsEvent, 0);
	//JK-FIX: Don't do this with a TDynamicCast
	CWObject_Projectile_Bouncer* pBouncer = TDynamicCast<CWObject_Projectile_Bouncer>(_pObj);
	if (pBouncer == NULL)
		return SERVER_PHYS_DEFAULTHANDLER;

	switch (_Event)
	{
		case CWO_PHYSEVENT_GETACCELERATION:
			{
				if (_pMat == NULL)
					return SERVER_PHYS_DEFAULTHANDLER;

				_pMat->Unit();
				CVec3Dfp32(0, 0, -pBouncer->m_Gravity).SetMatrixRow(*_pMat, 3);
				return SERVER_PHYS_DEFAULTHANDLER;
			}

		case CWO_PHYSEVENT_IMPACT:
			{
				if ((_pCollisionInfo == NULL) || (!_pCollisionInfo->m_bIsValid)) return SERVER_PHYS_ABORT;

				bool bExplode = pBouncer->OnTracerIntersection(_pCollisionInfo);

				if (bExplode)
				{
					if (!(pBouncer->m_Flags & FLAGS_NODESTROY))
					{
						pBouncer->m_bPendingDestroy = (pBouncer->m_DestroyDelayTicks > 0);
						pBouncer->Explode(0, _pObj->GetPosition(), NULL, !pBouncer->m_bPendingDestroy);
					}
				}

				if (bExplode)
					return SERVER_PHYS_ABORT;
				else
					return SERVER_PHYS_DEFAULTHANDLER;
//					return SERVER_PHYS_HANDLED;
			}
		default:
			return CWObject_Projectile_Bouncer_Parent::OnPhysicsEvent(_pObj, _pObjOther, _pPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Projectile_Bouncer, CWObject_Projectile_Bouncer_Parent, 0x0100);

//-------------------------------------------------------------------
// Projectile_Bounce
//-------------------------------------------------------------------
void CWObject_Projectile_Bounce::OnCreate()
{
	MAUTOSTRIP(CWObject_Projectile_Bounce_OnCreate, MAUTOSTRIP_VOID);
	CWObject_Projectile::OnCreate();
	m_nBounce = -1;
	m_PhysAttrib.m_Elasticy = 1;
	m_Elasticity = 1.0f;
	m_BounceAngle = 1.0f;
}

void CWObject_Projectile_Bounce::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CWObject_Projectile_Bounce_OnIncludeTemplate, MAUTOSTRIP_VOID);
	CWObject_Projectile::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

	IncludeSoundFromKey("SOUND_BOUNCE", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_STUCK", _pReg, _pMapData);
}

void CWObject_Projectile_Bounce::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Projectile_Bounce_OnEvalKey, MAUTOSTRIP_VOID);
	
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();
	
	switch (_KeyHash)
	{
	case MHASH3('NUMB','OUNC','E'): // "NUMBOUNCE"
		{
			m_nBounce = _pKey->GetThisValuei();
			break;
		}

	case MHASH3('SOUN','D_BO','UNCE'): // "SOUND_BOUNCE"
		{
			m_Sound_Bounce = KeyValue;
			break;
		}

	case MHASH3('SOUN','D_ST','UCK'): // "SOUND_STUCK"
		{
			m_Sound_Stuck = KeyValue;
			break;
		}

	case MHASH5('BOUN','CE_E','LAST','ICIT','Y'): // "BOUNCE_ELASTICITY"
		{
			m_Elasticity = KeyValuef;
			break;
		}

	case MHASH3('BOUN','CE_A','NGLE'): // "BOUNCE_ANGLE"
		{
			m_BounceAngle = KeyValuef;
			break;
		}

	default:
		{
			CWObject_Projectile::OnEvalKey(_pKey);
			break;
		}
	}
}

int CWObject_Projectile_Bounce::OnPreIntersection(int _iObject, CCollisionInfo *_pInfo)
{
	MAUTOSTRIP(CWObject_Projectile_Bounce_OnPreIntersection, 0);
	if (m_PendingDeath) return SERVER_PHYS_DEFAULTHANDLER;

	if(m_nBounce > 0 || m_nBounce == -1)
	{
		CVec3Dfp32 Dir(GetMoveVelocity());
		Dir.Normalize();

		if(!(m_Flags & FLAGS_NODAMAGEONIMPACT))
		{
//			SendDamage(_iObject, GetPosition(), m_Damage, m_DamageType, m_ShockwaveDamage, m_ShockwaveRange, Dir * m_ImpactForce);
//			SendDamage(_iObject, GetPosition(), m_iOwner, m_Damage, m_DamageType, _pInfo ? _pInfo->m_SurfaceType : 0, m_ShockwaveDamage, m_ShockwaveRange, Dir * m_ImpactForce, NULL);
		}

		CWObject *pObj = m_pWServer->Object_Get(_iObject);
		if(pObj && (pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PHYSMODEL))
		{
			if(_pInfo && _pInfo->m_bIsValid)
			{
				if(Abs(Dir * _pInfo->m_Plane.n) < m_BounceAngle)
				{
//					if(m_Sound_Bounce.Len())
//						Sound(m_Sound_Bounce);

					// Previously SERVER_PHYS_CONTINUE was returned since the physics handled bouncing
					CVec3Dfp32 Move = GetMoveVelocity();
					CVec3Dfp32 Res;
					Move.Reflect(_pInfo->m_Plane.n, Res);
					SetVelocity(Res * m_Elasticity);

					CMat4Dfp32 Mat = GetPositionMatrix();
					Res.SetMatrixRow(Mat, 0);
					Mat.RecreateMatrix(0, 2);
					SetPosition(Mat);

					if(m_nBounce != -1)
						m_nBounce--;
					
					return SERVER_PHYS_HANDLED;
				}
			}

//			if(m_Sound_Stuck.Len())
//				Sound(m_Sound_Stuck);

			ClientFlags() |= CLIENTFLAGS_NOTMOVING;
			m_PendingDeath = 1;
			m_PhysAttrib.m_Elasticy = 0;	// To prevent bouncing so that the object is moved the remaining distance before impact.

			return SERVER_PHYS_HANDLED;	// Continue, to let the object move the remaining distance before impact.
		}
		else
		{
//			if(m_Sound_Stuck.Len())
//				Sound(m_Sound_Stuck);
			Destroy();
			return SERVER_PHYS_ABORT;
		}
	}
	return 0;
}

void CWObject_Projectile_Bounce::OnRefresh()
{
	MAUTOSTRIP(CWObject_Projectile_Bounce_OnRefresh, MAUTOSTRIP_VOID);
	if(m_nBounce > 0 || m_nBounce == -1)
		CWObject_Projectile::OnRefresh();
	else
		CWObject_Ext_Model::OnRefresh();

	if (m_PendingDeath)
	{
		SetVelocity(0);		// Velocity should already be zero since the object has collided, but let's set it just to be sure.
		m_iSound[0] = 0;
	}
}

void CWObject_Projectile_Bounce::OnLoad(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Projectile_Bounce_OnLoad, MAUTOSTRIP_VOID);
	CWObject_Model::OnLoad(_pFile);
	_pFile->ReadLE(m_nBounce);
}

void CWObject_Projectile_Bounce::OnSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Projectile_Bounce_OnSave, MAUTOSTRIP_VOID);
	CWObject_Model::OnSave(_pFile);
	_pFile->WriteLE(m_nBounce);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Projectile_Bounce, CWObject_Projectile, 0x0100);

//-------------------------------------------------------------------
// Projectile_Instant
//-------------------------------------------------------------------

void CWObject_Projectile_Instant::OnCreate()
{
	MAUTOSTRIP(CWObject_Projectile_Instant_OnCreate, MAUTOSTRIP_VOID);
	CWObject_Projectile_Instant_Parent::OnCreate();

	// TrailDirection
	m_Data[0] = 0;
	m_Data[1] = 0;
	m_Data[2] = 0;

	// ExplosionIndexBits
	m_Data[3] = 0;
	m_Data[4] = 0;

	// Offset explosion away from surface, along surface normal.
	m_MoveOut = 0;

	// Offset starting position of trail, so that it does not intersect a running player, etc.
	m_StartOffset = 0;

	m_iSoundLRP = 0;
}

//-------------------------------------------------------------------

void CWObject_Projectile_Instant::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Projectile_Instant_OnEvalKey, MAUTOSTRIP_VOID);
	static const char *ModelIndexTranslate[] = { "MODEL0", "MODEL1", "MODEL2", "MODEL3", NULL };
	
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();
	
	switch (_KeyHash)
	{
	case MHASH5('ALIG','NED_','EXPL','OSIO','N'): // "ALIGNED_EXPLOSION"
		{
			m_Data[3] = KeyValue.TranslateFlags(ModelIndexTranslate);
			break;
		}
	case MHASH5('REFL','ECT_','EXPL','OSIO','N'): // "REFLECT_EXPLOSION"
		{
			m_Data[4] = KeyValue.TranslateFlags(ModelIndexTranslate);
			break;
		}
	case MHASH2('MOVE','OUT'): // "MOVEOUT"
		{
			m_MoveOut = KeyValuef;
			break;
		}
	case MHASH3('STAR','TOFF','SET'): // "STARTOFFSET"
		{
			m_StartOffset = KeyValuef;
			break;
		}
	case MHASH5('VELO','CITY','OFFS','ETFA','CTOR'): // "VELOCITYOFFSETFACTOR"
		{
			m_VelocityOffsetFactor = KeyValuef;
			break;
		}

	case MHASH3('SOUN','D_LR','P'): // "SOUND_LRP"
		{
			m_iSoundLRP = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	default:
		{
			CWObject_Projectile_Instant_Parent::OnEvalKey(_pKey);
			break;
		}
	}
}

//-------------------------------------------------------------------

void CWObject_Projectile_Instant::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CWObject_Projectile_Instant_OnIncludeTemplate, MAUTOSTRIP_VOID);
	CWObject_Projectile_Instant_Parent::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
}

//-------------------------------------------------------------------

void CWObject_Projectile_Instant::OnInitInstance(const int32* _pParam, int _nParam)
{
	MAUTOSTRIP(CWObject_Projectile_Instant_OnInitInstance, MAUTOSTRIP_VOID);
	CWObject_Projectile_Instant_Parent::OnInitInstance(_pParam, _nParam);

	fp32 Range = 500;

	CRPG_InitParams* pInitParams = NULL;
	if (_nParam > 0)
		pInitParams = (CRPG_InitParams*)_pParam[0];


	if (pInitParams != NULL)
	{
		CWObject_Character* pChar = safe_cast<CWObject_Character>(m_pWServer->Object_Get(pInitParams->m_iCreator));
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pChar);
		if (pCD != NULL)
		{
			Range = pCD->m_BackPlane;
		}
	}

	fp32 SubPrecision = 10;

	CVec3Dfp32 Dir = GetForward();

	CVec3Dfp32 Pos1 = GetPosition();
	CVec3Dfp32 Pos2 = Pos1 + Dir * Range;
	CCollisionInfo CInfo;

//	m_pWServer->Debug_RenderWire(Pos1, Pos2, 0xFF00FF00, 10.0f, false);

	if (TraceRay(Pos1, Pos2, &CInfo))
	{
		CVec3Dfp32 HitPos = CInfo.m_Pos;
		CVec3Dfp32 FakeEmitPos = Pos1;

		fp32 AlignedOwnerVelocity = Max(0.0f, m_pWServer->Object_GetVelocity(m_iOwner) * Dir);
		fp32 PerpOwnerVelocity = (m_pWServer->Object_GetVelocity(m_iOwner) - Dir * AlignedOwnerVelocity).Length();
		AlignedOwnerVelocity *= 1.0f - Min((PerpOwnerVelocity / (AlignedOwnerVelocity * 0.1f)), 1.0f);

		if ((pInitParams != NULL) && (pInitParams->m_pRPGCreatorItem != NULL))
		{
			CMat4Dfp32 EffectMatrix;
			pInitParams->m_pRPGCreatorItem->GetCurEffectMatrix(EffectMatrix);
			FakeEmitPos = CVec3Dfp32::GetMatrixRow(EffectMatrix, 3);
		}

		Dir = HitPos - FakeEmitPos; Dir.Normalize();

//		m_pWServer->Debug_RenderWire(FakeEmitPos, HitPos, -1, 10.0f, false);

		fp32 Length = (HitPos - FakeEmitPos).Length() - m_StartOffset - AlignedOwnerVelocity * m_VelocityOffsetFactor;
		iAnim1() = Min(32767, (int)((Length) * SubPrecision));

		CMat4Dfp32 Matrix = GetPositionMatrix();
		CInfo.m_Plane.n.SetMatrixRow(Matrix, 0);
		Matrix.RecreateMatrix(0, 1);
		CInfo.m_Pos.SetMatrixRow(Matrix, 3);
		(CInfo.m_Plane.n * m_MoveOut).AddMatrixRow(Matrix, 3);

		CFStr AttachClass;

		if (GetObjFlags(CInfo.m_iObject, m_pWServer) & OBJECT_FLAGS_CHARACTER)
			AttachClass = (const char*)((m_AttachClassChar != "") ? m_AttachClassChar : m_AttachClassWorld);
		else
			AttachClass = (const char*)((m_AttachClassWorld != "") ? m_AttachClassWorld : m_AttachClassChar);

		if (AttachClass != "")
		{
			if (m_AttachOnObjectFlags & GetObjFlags(CInfo.m_iObject, m_pWServer))
			{
				CMat4Dfp32 AttachMatrix = Matrix;
				Dir.SetMatrixRow(AttachMatrix, 0);
				CVec3Dfp32::GetMatrixRow(GetPositionMatrix(), 1).SetMatrixRow(AttachMatrix, 1);
	
				AttachMatrix.RecreateMatrix(0, 1);

				int iAttachObj = CWObject_AttachModel::CreateAndAttach(AttachClass, m_pWServer, m_iOwner, AttachMatrix, CInfo, m_bAttachAligned);
				CWObject* pAttachObj = m_pWServer->Object_Get(iAttachObj);
				if (pAttachObj != NULL)
				{
//					pAttachObj->m_iOwner = m_iOwner;
				}
			}
		}

		m_bPendingDestroy = (m_DestroyDelayTicks > 0);
		Explode(CInfo.m_iObject, CInfo.m_Pos, &CInfo, !m_bPendingDestroy);

		SetPosition(Matrix);

		SoundLRP(m_iSoundLRP, FakeEmitPos, HitPos);
		PlayHitSounds(&CInfo, false);
	}
	else
	{
		CVec3Dfp32 HitPos = Pos2;
		CVec3Dfp32 FakeEmitPos = Pos1;

		fp32 AlignedOwnerVelocity = Max(0.0f, m_pWServer->Object_GetVelocity(m_iOwner) * Dir);
		fp32 PerpOwnerVelocity = (m_pWServer->Object_GetVelocity(m_iOwner) - Dir * AlignedOwnerVelocity).Length();
		AlignedOwnerVelocity *= 1.0f - Min((PerpOwnerVelocity / (AlignedOwnerVelocity * 0.1f)), 1.0f);

		if (_nParam > 0)
		{
			CRPG_InitParams* pInitParams = (CRPG_InitParams*)_pParam[0];

			if ((pInitParams != NULL) && (pInitParams->m_pRPGCreatorItem != NULL))
			{
				CMat4Dfp32 EffectMatrix;
				pInitParams->m_pRPGCreatorItem->GetCurEffectMatrix(EffectMatrix);
				FakeEmitPos = CVec3Dfp32::GetMatrixRow(EffectMatrix, 3);
//				m_pWServer->Debug_RenderMatrix(EffectMatrix, 10.0f, false);
			}
		}

		Dir = HitPos - FakeEmitPos; Dir.Normalize();

//		m_pWServer->Debug_RenderWire(FakeEmitPos, HitPos, -1, 10.0f, false);

		fp32 Length = (HitPos - FakeEmitPos).Length() - m_StartOffset - AlignedOwnerVelocity * m_VelocityOffsetFactor;
		iAnim1() = Min(32767, (int)((Length) * SubPrecision));

		SetPosition(HitPos);

		SoundLRP(m_iSoundLRP, FakeEmitPos, HitPos);

		for (int iModel = 0; iModel < CWO_NUMMODELINDICES; iModel++)
			if ((m_Data[3] & (1 << iModel)) || (m_Data[4] & (1 << iModel)))
				m_iModel[iModel] = 0;

		m_bPendingDestroy = (m_DestroyDelayTicks > 0);
	}

	m_Data[0] = *(int32*)(&Dir[0]);
	m_Data[1] = *(int32*)(&Dir[1]);
	m_Data[2] = *(int32*)(&Dir[2]);
}

//-------------------------------------------------------------------

void CWObject_Projectile_Instant::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Projectile_Instant_OnClientRender, MAUTOSTRIP_VOID);
	fp32 SubPrecision = 100;
	CMat4Dfp32 AlignMat, ReflectMat, TrailMat;
	CVec3Dfp32 Dir;
	
	AlignMat = _pObj->GetPositionMatrix();

	Dir[0] = *(fp32*)(&_pObj->m_Data[0]);
	Dir[1] = *(fp32*)(&_pObj->m_Data[1]);
	Dir[2] = *(fp32*)(&_pObj->m_Data[2]);

	ReflectMat = AlignMat;
	CVec3Dfp32 Normal, Reflection;
	Normal = CVec3Dfp32::GetMatrixRow(AlignMat, 0);
	Dir.Reflect(Normal, Reflection);
	Reflection.SetMatrixRow(ReflectMat, 0);
	ReflectMat.RecreateMatrix(0, 1);

	TrailMat = AlignMat;
	Dir.SetMatrixRow(TrailMat, 0);
	TrailMat.RecreateMatrix(0, 1);

	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
		if (pModel)
		{
			if (_pObj->m_Data[3] & (1 << i))
				_pEngine->Render_AddModel(pModel, AlignMat, _pObj->GetDefaultAnimState(_pWClient, i));
			else if (_pObj->m_Data[4] & (1 << i))
				_pEngine->Render_AddModel(pModel, ReflectMat, _pObj->GetDefaultAnimState(_pWClient, i));
			else
				_pEngine->Render_AddModel(pModel, TrailMat, _pObj->GetDefaultAnimState(_pWClient, i));
		}
	}
}

//-------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Projectile_Instant, CWObject_Projectile, 0x0100);
//-------------------------------------------------------------------

void CWObject_Projectile::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWObject_Projectile_OnClientNetMsg, MAUTOSTRIP_VOID);
	switch(_Msg.m_MsgType)
	{
		case NETMSG_SPAWN_CLIENTMODEL:
		{
			if(_pWClient->GetClientMode() == WCLIENT_MODE_MIRROR)
				return;

			int iMsgPos = 0;
			CVec3Dfp32 Pos;
			CQuatfp32 Rot;
			int iModel[3];
			int DurationTicks;
			//int iParentObj;
			//int iParentNode;
			CVec3Dfp32 LocalNodePos;
			CQuatfp32 LocalNodeRot;

			Pos[0] = _Msg.Getfp32(iMsgPos);
			Pos[1] = _Msg.Getfp32(iMsgPos);
			Pos[2] = _Msg.Getfp32(iMsgPos);

			Rot.k[0] = _Msg.Getfp32(iMsgPos);
			Rot.k[1] = _Msg.Getfp32(iMsgPos);
			Rot.k[2] = _Msg.Getfp32(iMsgPos);
			Rot.k[3] = _Msg.Getfp32(iMsgPos);

			iModel[0] = _Msg.GetInt32(iMsgPos);
			iModel[1] = _Msg.GetInt32(iMsgPos);
			iModel[2] = _Msg.GetInt32(iMsgPos);

			DurationTicks = _Msg.GetInt32(iMsgPos);
			CMat4Dfp32 Matrix;
			Rot.CreateMatrix(Matrix);
			Pos.SetMatrixRow(Matrix, 3);
			
			int iObj = _pWClient->ClientObject_Create("ClientModel", Matrix);
			if (iObj > 0)
			{
				CWObject_Client* pObj = _pWClient->ClientObject_Get(iObj);
				if (pObj != NULL)
				{
					for (int i = 0; i < CWO_NUMMODELINDICES; i++)
						pObj->m_iModel[i] = iModel[i];

					pObj->m_iAnim2 = DurationTicks;

					//pObj->ClientFlags() |= CLIENTFLAGS_ATTACHED;
				}
			}
		}
	}
}
*/
//-------------------------------------------------------------------
//- AttachModel -----------------------------------------------------
//-------------------------------------------------------------------

#ifndef M_DISABLE_TODELETE
const char* CWObject_AttachModel::ms_lpAttachTypeStr[] = { "harmless", "painfull", "leathal", NULL};

//-------------------------------------------------------------------

void CWObject_AttachModel::Attach(int _iParentObj, int _iBone, CMat4Dfp32* _pWorldToBone, bool _bGenerateWTB)
{
	MAUTOSTRIP(CWObject_AttachModel_Attach, MAUTOSTRIP_VOID);
	ClientFlags() |= CLIENTFLAGS_ATTACHED;

	if (_iParentObj != 0)
	{
		m_Data[7] = _iBone;

		if (_iBone == 0)
		{
			m_pWServer->Object_AddChild(_iParentObj, m_iObject);
		}
		else
		{
			if (GetObjFlags(_iParentObj, m_pWServer) & OBJECT_FLAGS_CHARACTER)
			{
				if (!m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ISVALIDATTACH, _iBone, m_iAttachType), _iParentObj))
				{
					m_DestroyDelayTicks = 0;
					m_bPendingDestroy = true;
				}
			}

			CMat4Dfp32 WorldToBone;

			if (_bGenerateWTB && (_pWorldToBone == NULL))
			{
				CXR_AnimState AnimState;
				CWObject_Message Msg(OBJSYSMSG_GETANIMSTATE);
				Msg.m_pData = &AnimState;
				Msg.m_DataSize = sizeof(AnimState);
				Msg.m_VecParam0[0] = 1.0f;
				if (m_pWServer->Message_SendToObject(Msg, _iParentObj))
				{
					CXR_SkeletonInstance* pSkelInstance = AnimState.m_pSkeletonInst;
					if (pSkelInstance && pSkelInstance->m_lBoneTransform.ValidPos(_iBone))
					{
						/* Debug Render
						CWObject_Client* pParentObj = _pWClient->Object_Get(_pObj->GetParent());
						CXR_Model *pCharModel = _pWClient->GetMapData()->GetResource_Model(pParentObj->m_iModel[0]);
						CXR_Skeleton *pCharSkeleton = (CXR_Skeleton*)pCharModel->GetParam(MODEL_PARAM_SKELETON);
						CXR_SkeletonInstance* pSkeletonInstance = AnimState.m_pSkeletonInst;
						_pWClient->Debug_RenderSkeleton(pCharSkeleton, pSkeletonInstance, 0xFFFFFF00, 1.0f);
						*/

						pSkelInstance->GetBoneTransform(_iBone).InverseOrthogonal(WorldToBone);
						_pWorldToBone = &WorldToBone;

//						m_pWServer->Debug_RenderMatrix(BoneWorldMatrix, 20);
					}
				}
			}

			if (_pWorldToBone != NULL)
			{
				CMat4Dfp32 LocalMatrix;
				GetPositionMatrix().Multiply(*_pWorldToBone, LocalMatrix);
				m_pWServer->Object_AddChild(_iParentObj, m_iObject);
				SetPosition(LocalMatrix);
			}
			else
			{
				CMat4Dfp32 LocalMatrix;
				LocalMatrix.Unit();
				m_pWServer->Object_AddChild(_iParentObj, m_iObject);
				SetPosition(LocalMatrix);
			}
		}

//		pAttachObj->m_iOwner = iParentObj;
	}
}

//-------------------------------------------------------------------

void CWObject_AttachModel::Attach(CCollisionInfo& _CInfo, bool _bForceAligned, int _iAlignAxis)
{
	MAUTOSTRIP(CWObject_AttachModel_Attach_2, MAUTOSTRIP_VOID);
	if (_bForceAligned && _CInfo.m_bIsValid)
	{
		CMat4Dfp32 Matrix = GetPositionMatrix();
		if (_iAlignAxis == 0)
		{
			_CInfo.m_Plane.n.SetMatrixRow(Matrix, 0);
			CVec3Dfp32(0, 0, 1).SetMatrixRow(Matrix, 2);
			Matrix.RecreateMatrix(0, 2);
		}
		else if (_iAlignAxis == 2)
		{
			_CInfo.m_Plane.n.SetMatrixRow(Matrix, 2);
			CVec3Dfp32(1, 0, 0).SetMatrixRow(Matrix, 0);
			Matrix.RecreateMatrix(2, 0);
		}
		SetPosition(Matrix);
	}

	CWObject* pParentObj = m_pWServer->Object_Get(_CInfo.m_iObject);
	if ((pParentObj != NULL) && !(pParentObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_WORLD))
		Attach(_CInfo.m_iObject, _CInfo.m_LocalNode, &_CInfo.m_LocalNodePos);
	else
		Attach();
}

//-------------------------------------------------------------------

int CWObject_AttachModel::CreateAndAttach(const char* _Class, CWorld_Server* _pWServer, int _iOwner, CMat4Dfp32& _Matrix, int _iParentObj, int _iBone, CMat4Dfp32* _pWorldToBone, bool _bGenerateWTB)
{
	MAUTOSTRIP(CWObject_AttachModel_CreateAndAttach, 0);
	int iAttachObj = _pWServer->Object_Create(_Class, _Matrix, _iOwner);
	CWObject_AttachModel* pAttachObj = safe_cast<CWObject_AttachModel>(_pWServer->Object_Get(iAttachObj));
	if (pAttachObj != NULL)
	{
		pAttachObj->Attach(_iParentObj, _iBone, _pWorldToBone, _bGenerateWTB);
		return iAttachObj;
	}

	return 0;
}

//-------------------------------------------------------------------

int CWObject_AttachModel::CreateAndAttach(const char* _Class, CWorld_Server* _pWServer, int _iOwner, CMat4Dfp32& _Matrix, CCollisionInfo& _CInfo, bool _bForceAligned, int _iAlignAxis)
{
	MAUTOSTRIP(CWObject_AttachModel_CreateAndAttach_2, 0);
	if (_bForceAligned && _CInfo.m_bIsValid)
	{
		if (_iAlignAxis == 0)
		{
			_CInfo.m_Plane.n.SetMatrixRow(_Matrix, 0);
			CVec3Dfp32(0, 0, 1).SetMatrixRow(_Matrix, 2);
			_Matrix.RecreateMatrix(0, 2);
		}
		else if (_iAlignAxis == 2)
		{
			_CInfo.m_Plane.n.SetMatrixRow(_Matrix, 2);
			CVec3Dfp32(1, 0, 0).SetMatrixRow(_Matrix, 0);
			_Matrix.RecreateMatrix(2, 0);
		}
	}

	CWObject* pParentObj = _pWServer->Object_Get(_CInfo.m_iObject);
	if ((pParentObj != NULL) && !(pParentObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_WORLD))
		return CreateAndAttach(_Class, _pWServer, _iOwner, _Matrix, _CInfo.m_iObject, _CInfo.m_LocalNode, &_CInfo.m_LocalNodePos);
	else
		return CreateAndAttach(_Class, _pWServer, _iOwner, _Matrix);
}

//-------------------------------------------------------------------

void CWObject_AttachModel::OnCreate()
{
	MAUTOSTRIP(CWObject_AttachModel_OnCreate, MAUTOSTRIP_VOID);
	m_DamageDeliveredDelay = 0.0f;
	m_iAttachType = AttachType_Harmless;
	m_bAllowAttachedOnly = false;
	
	CWObject_AttachModel_Parent::OnCreate();
}

//-------------------------------------------------------------------

void CWObject_AttachModel::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_AttachModel_OnEvalKey, MAUTOSTRIP_VOID);
	
	const CStr KeyName = _pKey->GetThisName();
	
	switch (_KeyHash)
	{
	case MHASH3('ATTA','CHTY','PE'): // "ATTACHTYPE"
		{
			m_iAttachType = _pKey->GetThisValue().TranslateInt(ms_lpAttachTypeStr);
			break;
		}
	case MHASH5('ALLO','WATT','ACHE','DONL','Y'): // "ALLOWATTACHEDONLY"
		{
			m_bAllowAttachedOnly = _pKey->GetThisValuei() != 0;
			break;
		}
	default:
		{
			CWObject_AttachModel_Parent::OnEvalKey(_pKey);
			break;
		}
	}
}

//-------------------------------------------------------------------

void CWObject_AttachModel::OnInitInstance(const int32* _pParam, int _nParam)
{
	MAUTOSTRIP(CWObject_AttachModel_OnInitInstance, MAUTOSTRIP_VOID);
	CWObject_AttachModel_Parent::OnInitInstance(_pParam, _nParam);
	
	if (_nParam == 0)
		return;

	CRPG_InitParams* pInitParams = (CRPG_InitParams*)_pParam[0];

	if (pInitParams == NULL)
		return;

	if ((pInitParams->m_pCInfo != NULL) && (pInitParams->m_pCInfo->m_bIsValid))
	{
		Attach(*pInitParams->m_pCInfo);
	}
	else
	{
		// No collision info available. Object can't be attached.
		if (m_bAllowAttachedOnly)
		{
			// Destroy object.
			m_DestroyDelayTicks = 0;
			m_bPendingDestroy = true;
		}
	}
}

//-------------------------------------------------------------------

void CWObject_AttachModel::OnRefresh()
{
	MAUTOSTRIP(CWObject_AttachModel_OnRefresh, MAUTOSTRIP_VOID);
	if (GetParent() != 0)
	{
		if (m_Damage.IsValid())
		{
			if ((m_Damage.m_DeliverDelay > 0.0f) && (m_DamageDeliveredDelay <= 0.0f))
			{
				// FIXME: Send damage effect only once, at first time damage is sent.
				m_Damage.Send(GetParent(), m_iOwner, m_pWServer, GetPosition(), m_DamageEffect);
				m_DamageDeliveredDelay = m_Damage.m_DeliverDelay;
			}
			else
			{
				m_DamageDeliveredDelay -= m_pWServer->GetGameTickTime();
			}
		}
	}

	CWObject_AttachModel_Parent::OnRefresh();
}

//-------------------------------------------------------------------

aint CWObject_AttachModel::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_AttachModel_OnClientMessage, 0);
	switch (_Msg.m_Msg)
	{
		case OBJMSG_SPELLS_GETRENDERMATRIX:
		{
			fp32 TickFrac = _Msg.m_VecParam0[0];
			CMat4Dfp32* pMatrix = (CMat4Dfp32*)_Msg.m_pData;

			{
				if (_pObj->GetParent() != 0)
				{
					int iLocalNode = _pObj->m_Data[7];
					
					if (iLocalNode > 0)
					{
						CXR_AnimState AnimState;

						CWObject_Message Msg(OBJSYSMSG_GETANIMSTATE);
						Msg.m_pData = &AnimState;
						Msg.m_DataSize = sizeof(AnimState);
						Msg.m_VecParam0[0] = TickFrac;
						if (_pWClient->ClientMessage_SendToObject(Msg, _pObj->GetParent()))
						{
							CXR_SkeletonInstance* pSkelInstance = AnimState.m_pSkeletonInst;
							if (pSkelInstance && pSkelInstance->m_lBoneTransform.ValidPos(iLocalNode))
							{
								CWObject_Client* pParentObj = _pWClient->Object_Get(_pObj->GetParent());
								CXR_Model *pCharModel = _pWClient->GetMapData()->GetResource_Model(pParentObj->m_iModel[0]);
								if(!pCharModel)
									return 0;

								CMat4Dfp32 BoneWorldMatrix;
								pSkelInstance->GetBoneTransform(iLocalNode, &BoneWorldMatrix);

								CXR_Skeleton *pCharSkeleton = (CXR_Skeleton*)pCharModel->GetParam(MODEL_PARAM_SKELETON);

								// (False funkar minst för pilar från sniper & crossbow(!aimassist) på berserker & lich)
								// (True funkar för wingshielden, men den sitter i bone origin.
								CMat4Dfp32 Unit; Unit.Unit();
								if (_pObj->GetLocalPositionMatrix().AlmostEqual(Unit, _FP32_EPSILON))
								{
									const CXR_SkeletonNode& Node = pCharSkeleton->m_lNodes[iLocalNode];
									CVec3Dfp32 Pos = Node.m_LocalCenter;
									Pos *= BoneWorldMatrix;
									Pos.SetMatrixRow(BoneWorldMatrix, 3);
								}

								_pObj->GetLocalPositionMatrix().Multiply(BoneWorldMatrix, *pMatrix);

//								_pWClient->Debug_RenderSkeleton(pCharSkeleton, pSkelInstance, 0xFFFFFF00, 20);
//								_pWClient->Debug_RenderMatrix(BoneWorldMatrix, 20);
//								_pWClient->Debug_RenderMatrix(*pMatrix, 20);

							}
							else
							{
								return 0; // Abort render if there's no skeleton to use (rewrite to use some default parent position?)
							}
						}
					}
					else
					{
						CMat4Dfp32 ParentMatrix;

						CWObject_Message Msg(OBJMSG_CHAR_GETRENDERMATRIX);
						Msg.m_VecParam0[0] = TickFrac; // Not needed, but just to keep the convention of the message.
						Msg.m_pData = &ParentMatrix;
						Msg.m_DataSize = sizeof(CMat4Dfp32);

						if (!_pWClient->ClientMessage_SendToObject(Msg, _pObj->GetParent()))
							return 0;

						ParentMatrix.Unit3x3();
						_pObj->GetLocalPositionMatrix().Multiply(ParentMatrix, *pMatrix);
						return 1;

/*
						// FutureFix: May have to fix anither TickFrac for non predicted characters (GetAnimState msg does this properly).
						CWObject_Client* pParentObj = _pWClient->Object_Get(_pObj->GetParent());
						if (pParentObj != NULL)
						{
							CMat4Dfp32 ParentMatrix;
							Interpolate2(pParentObj->GetLastPositionMatrix(), pParentObj->GetPositionMatrix(), ParentMatrix, TickFrac);
							ParentMatrix.Unit3x3();
							_pObj->GetLocalPositionMatrix().Multiply(ParentMatrix, *pMatrix);
						}
						else
						{
							return 0;
						}
*/

					}
					return 1;
				}
			}
		}
		break;
	}

	return CWObject_AttachModel_Parent::OnClientMessage(_pObj, _pWClient, _Msg);
}

//-------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_AttachModel, CWObject_AttachModel_Parent, 0x0100);
//-------------------------------------------------------------------

#endif
//-------------------------------------------------------------------
//- Homing ----------------------------------------------------------
//-------------------------------------------------------------------
/*
bool CWObject_Projectile_Homing::SetTargetPos(CVec3Dfp32& _TargetPos)
{
	MAUTOSTRIP(CWObject_Projectile_Homing_SetTargetPos, false);
	if (m_Homing_Mode != HOMINGMODE_NOTARGET)
		return false;

	m_Homing_Mode = HOMINGMODE_TARGETPOS;
	m_Homing_TargetPos = _TargetPos;
	return true;
}

//-------------------------------------------------------------------

bool CWObject_Projectile_Homing::SetTargetObj(int _iTargetObj)
{
	MAUTOSTRIP(CWObject_Projectile_Homing_SetTargetObj, false);
	if (m_Homing_Mode == HOMINGMODE_TARGETOBJ)
		return false;

	m_Homing_Mode = HOMINGMODE_TARGETOBJ;
	m_Homing_TargetObjGUID = m_pWServer->Object_GetGUID(_iTargetObj);
	return true;
}

//-------------------------------------------------------------------

int CWObject_Projectile_Homing::FindTarget()
{
	MAUTOSTRIP(CWObject_Projectile_Homing_FindTarget, 0);
	return 0;
}

//-------------------------------------------------------------------

void CWObject_Projectile_Homing::OnCreate()
{
	MAUTOSTRIP(CWObject_Projectile_Homing_OnCreate, MAUTOSTRIP_VOID);
	CWObject_Projectile_Homing_Parent::OnCreate();

	m_Homing_MinVelocity = 0;
	m_Homing_MaxVelocity = 0;
	m_Homing_Acceleration = 0;
	m_Homing_bTurning = false;
	m_Homing_StartTurnDistance = 0;
	m_Homing_EndTurnDistance = 0;
	m_Homing_ContraTurnHelp = 0;

	m_Homing_bPitchControl = false;
	m_Homing_bBankControl = false;

	m_Homing_Banking = 0;
	m_Homing_MaxBanking = 0;
	m_Homing_MaxPitch = 0;
	m_Homing_BankReduction = 1;
	m_Homing_PitchReduction = 1;

	m_Homing_Mode = HOMINGMODE_NOTARGET;
	m_Homing_TargetPos = 0;
	m_Homing_TargetObjGUID = 0;

	m_Homing_TargetObjects = 0;
	
	m_Homing_StartDelayTicks = 0;
	m_Homing_StartFadeTicks = 0;
}

//-------------------------------------------------------------------

void CWObject_Projectile_Homing::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Projectile_Homing_OnEvalKey, MAUTOSTRIP_VOID);
	
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();
	
	switch (_KeyHash)
	{
	case MHASH5('HOMI','NG_M','INVE','LOCI','TY'): // "HOMING_MINVELOCITY"
		{
			m_Homing_MinVelocity = KeyValuef;
			break;
		}
	case MHASH5('HOMI','NG_M','AXVE','LOCI','TY'): // "HOMING_MAXVELOCITY"
		{
			m_Homing_MaxVelocity = KeyValuef;
			break;
		}
	case MHASH5('HOMI','NG_A','CCEL','ERAT','ION'): // "HOMING_ACCELERATION"
		{
			m_Homing_Acceleration = KeyValuef;
			break;
		}
	case MHASH6('HOMI','NG_S','TART','TURN','DIST','ANCE'): // "HOMING_STARTTURNDISTANCE"
		{
			m_Homing_StartTurnDistance = KeyValuef;
			break;
		}
	case MHASH6('HOMI','NG_E','NDTU','RNDI','STAN','CE'): // "HOMING_ENDTURNDISTANCE"
		{
			m_Homing_EndTurnDistance = KeyValuef;
			break;
		}
	case MHASH6('HOMI','NG_C','ONTR','ATUR','NHEL','P'): // "HOMING_CONTRATURNHELP"
		{
			m_Homing_ContraTurnHelp = KeyValuef;
			break;
		}
	case MHASH4('HOMI','NG_B','ANKI','NG'): // "HOMING_BANKING"
		{
			m_Homing_Banking = KeyValuef;
			break;
		}
	case MHASH5('HOMI','NG_M','AXBA','NKIN','G'): // "HOMING_MAXBANKING"
		{
			m_Homing_MaxBanking = KeyValuef;
			break;
		}
	case MHASH4('HOMI','NG_M','AXPI','TCH'): // "HOMING_MAXPITCH"
		{
			m_Homing_MaxPitch = KeyValuef;
			break;
		}
	case MHASH5('HOMI','NG_B','ANKR','EDUC','TION'): // "HOMING_BANKREDUCTION"
		{
			m_Homing_BankReduction = KeyValuef;
			break;
		}
	case MHASH6('HOMI','NG_P','ITCH','REDU','CTIO','N'): // "HOMING_PITCHREDUCTION"
		{
			m_Homing_PitchReduction = KeyValuef;
			break;
		}
	case MHASH5('HOMI','NG_T','ARGE','TOBJ','ECTS'): // "HOMING_TARGETOBJECTS"
		{
			const char* lpObjFlags[] = { "CHARACTERS", "WORLD", "ALL", "NOTCHARACTERS", NULL };
			int ObjFlags = KeyValue.TranslateFlags(lpObjFlags);
			if (ObjFlags & 1) m_Homing_TargetObjects |= OBJECT_FLAGS_CHARACTER;
			if (ObjFlags & 2) m_Homing_TargetObjects |= OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
			if (ObjFlags & 4) m_Homing_TargetObjects |= -1;
			if (ObjFlags & 8) m_Homing_TargetObjects &= ~OBJECT_FLAGS_CHARACTER;
			break;
		}
	case MHASH6('HOMI','NG_A','UTOS','ELEC','TTAR','GET'): // "HOMING_AUTOSELECTTARGET"
		{
			m_Homing_bAutoSelectTarget = _pKey->GetThisValuei() != 0;
			break;
		}
	case MHASH5('HOMI','NG_T','ARGE','TOBJ','ECT'): // "HOMING_TARGETOBJECT"
		{
			SetTargetObj(m_pWServer->Selection_GetSingleTarget(Selection));
			break;
		}
	case MHASH5('HOMI','NG_S','TART','DELA','Y'): // "HOMING_STARTDELAY"
		{
			m_Homing_StartDelayTicks = KeyValuef * SERVER_TICKSPERSECOND;
			break;
		}
	case MHASH4('HOMI','NG_S','TART','FADE'): // "HOMING_STARTFADE"
		{
			m_Homing_StartFadeTicks = KeyValuef * SERVER_TICKSPERSECOND;
			break;
		}
	default:
		{
			CWObject_Projectile_Homing_Parent::OnEvalKey(_pKey);
			break;
		}
	}
}

//-------------------------------------------------------------------

void CWObject_Projectile_Homing::OnInitInstance(const int32* _pParam, int _nParam)
{
	MAUTOSTRIP(CWObject_Projectile_Homing_OnInitInstance, MAUTOSTRIP_VOID);
	CWObject_Projectile_Homing_Parent::OnInitInstance(_pParam, _nParam);

	fp32 Range = 2000;

	m_Homing_bPitchControl = m_Homing_MaxPitch > 0;
	m_Homing_bBankControl = m_Homing_Banking > 0;

	CVec3Dfp32 Dir = GetForward();
	CVec3Dfp32 Pos1 = GetPosition();
	CVec3Dfp32 Pos2 = Pos1 + Dir * Range;

	CCollisionInfo CInfo;

	if (_nParam == 0)
		return;

	CRPG_InitParams* pInitParams = (CRPG_InitParams*)_pParam[0];
	if (pInitParams == NULL)
		return;

	if (TraceRay(Pos1, Pos2, &CInfo))
	{
		CWObject* pObj = m_pWServer->Object_Get(CInfo.m_iObject);
		if ((pObj != NULL) && (GetObjFlags(pObj) & m_Homing_TargetObjects))
		{
			SetTargetObj(CInfo.m_iObject);
		}
		else
		{
			if (pInitParams->m_iTargetObj != 0)
			{
				SetTargetObj(pInitParams->m_iTargetObj);
			}
			else
			{
			}
		}
	}
	else
	{
		if (pInitParams->m_iTargetObj != 0)
		{
			SetTargetObj(pInitParams->m_iTargetObj);
		}
	}
}

//-------------------------------------------------------------------

int CWObject_Projectile_Homing::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Projectile_Homing_OnMessage, 0);
	switch (_Msg.m_Msg)
	{
		case OBJMSG_SPELLS_SETTARGET:
			return (SetTargetObj(_Msg.m_Param0));
	}

	return CWObject_Projectile_Homing_Parent::OnMessage(_Msg);
}

//-------------------------------------------------------------------

void CWObject_Projectile_Homing::OnRefresh()
{
	MAUTOSTRIP(CWObject_Projectile_Homing_OnRefresh, MAUTOSTRIP_VOID);
	if (m_Homing_StartDelayTicks > 0)
	{
		m_Homing_StartDelayTicks--;
	}
	else if (!m_bPendingDestroy)
	{
		if (m_Homing_Mode == HOMINGMODE_NOTARGET)
		{
			if (m_Homing_bAutoSelectTarget)
			{
				int iObj = FindTarget();
				if (iObj > 0)
					SetTargetObj(iObj);
			}
		}
		else 
		{
			CVec3Dfp32 TargetPos;
			if (m_Homing_Mode == HOMINGMODE_TARGETPOS)
			{
				TargetPos = m_Homing_TargetPos;
			}
			else if (m_Homing_Mode == HOMINGMODE_TARGETOBJ)
			{
				CWObject* pTarget = m_pWServer->Object_GetWithGUID(m_Homing_TargetObjGUID);
				if ((pTarget == NULL) || (CWObject_Character::Char_GetPhysType(pTarget) == PLAYER_PHYS_DEAD))
				{
					m_Homing_Mode = HOMINGMODE_NOTARGET;
					m_Homing_TargetObjGUID = 0;
					OnRefresh();
					return;
				}

				//pTarget->GetAbsBoundBox()->GetCenter(TargetPos);
				TargetPos = CWObject_Character::GetCharacterCenter(pTarget, m_pWServer, false, 0.25f);
			}

	//		m_pWServer->Debug_RenderWire(GetPosition(), TargetPos, 0xFF00FF00, 0.1f);

			fp32 Fade;
			if (m_Homing_StartFadeTicks > 0)
				Fade = Clamp01((fp32)GetAnimTick(m_pWServer) / (fp32)m_Homing_StartFadeTicks);
			else
				Fade = 1.0f;

			CMat4Dfp32 Matrix = GetPositionMatrix();
			CVec3Dfp32 Left = CVec3Dfp32::GetMatrixRow(Matrix, 1);
			CVec3Dfp32 Up = CVec3Dfp32::GetMatrixRow(Matrix, 2);

			CVec3Dfp32 TargetDiff = TargetPos - GetPosition();
			fp32 TargetDistance = TargetDiff.Length();
			CVec3Dfp32 TargetDir = TargetDiff / TargetDistance;
			CVec3Dfp32 AccelerationVector = TargetDir * m_Homing_Acceleration * Fade;

			CVec3Dfp32 VelocityDir;
			CVec3Dfp32 VelocityVector = GetMoveVelocity();
			CVec3Dfp32 OldVelocityVector = VelocityVector;

			if (TargetDistance > m_Homing_StartTurnDistance)
				m_Homing_bTurning = true;

			if (TargetDistance < m_Homing_EndTurnDistance)
				m_Homing_bTurning = false;

			if (m_Homing_bTurning)
				VelocityVector += AccelerationVector;

			fp32 Velocity = VelocityVector.Length();
			if (Velocity > 0)
			{
				VelocityDir = VelocityVector * (1.0f / Velocity);

				if (((VelocityDir * TargetDir) < -0.8f) && m_Homing_bTurning)
				{
					CVec3Dfp32 WorldLeft;
					CVec3Dfp32(0, 0, 1).CrossProd(VelocityDir, WorldLeft);
					VelocityDir += WorldLeft * m_Homing_ContraTurnHelp / Velocity;
					VelocityDir.Normalize();
				}

				if (m_Homing_bPitchControl)
				{
					if (M_Fabs(VelocityDir[2]) > m_Homing_MaxPitch)
					{
						VelocityDir[2] *= m_Homing_MaxPitch / M_Fabs(VelocityDir[2]);

						fp32 VeloDirXYLenOld = M_Sqrt(Sqr(VelocityDir[0]) + Sqr(VelocityDir[1]));
						fp32 VeloDirXYLenNew = M_Sqrt(1.0f - Sqr(VelocityDir[2]));
						VelocityDir[0] *= VeloDirXYLenNew / VeloDirXYLenOld;
						VelocityDir[1] *= VeloDirXYLenNew / VeloDirXYLenOld;
						//fp32 DebugLen = VelocityDir.Length();
						//VelocityDir.Normalize();
					}
					else
					{
						VelocityDir[2] *= m_Homing_PitchReduction;

						fp32 VeloDirXYLenOld = M_Sqrt(Sqr(VelocityDir[0]) + Sqr(VelocityDir[1]));
						fp32 VeloDirXYLenNew = M_Sqrt(1.0f - Sqr(VelocityDir[2]));
						fp32 Factor;
						if (VeloDirXYLenOld != 0)
						{
							Factor = VeloDirXYLenNew / VeloDirXYLenOld;
							VelocityDir[0] *= Factor;
							VelocityDir[1] *= Factor;
						}
						//fp32 DebugLen = VelocityDir.Length();
						//VelocityDir.Normalize();
					}
				}
				
				Velocity = Max(m_Homing_MinVelocity, Min(m_Homing_MaxVelocity, Velocity));
				VelocityVector = VelocityDir * Velocity;
			}

//			m_pWServer->Debug_RenderVector(GetPosition(), VelocityDir * 20, 0xFF00FF00, 10.0f);

			SetVelocity(VelocityVector);

			if (m_Homing_bBankControl)
			{
				CVec3Dfp32 VelocityChange = VelocityVector - OldVelocityVector;
				fp32 Turning = VelocityChange * Left;
				Up += Left * Turning * m_Homing_Banking;
				CVec3Dfp32 UpXY(Up[0], Up[1], 0);
				fp32 UpXYLenSqr = UpXY.LengthSqr();
				if (UpXYLenSqr > Sqr(m_Homing_MaxBanking))
				{
					fp32 Scale = m_Homing_MaxBanking / M_Sqrt(UpXYLenSqr);
					Up[0] *= Scale;
					Up[1] *= Scale;
				}
				else
				{
					Up[0] *= m_Homing_BankReduction;
					Up[1] *= m_Homing_BankReduction;
				}

				fp32 MinUpZ = M_Sqrt(1.0f - Sqr(m_Homing_MaxBanking));		
				if (Up[2] < MinUpZ)
					Up[2] = MinUpZ;
			}

			VelocityDir.SetMatrixRow(Matrix, 0);
			Up.SetMatrixRow(Matrix, 2);

			Matrix.RecreateMatrix(0, 2);
			SetPosition(Matrix);
		}
	}

	CWObject_Projectile_Homing_Parent::OnRefresh();
}
//-------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Projectile_Homing, CWObject_Projectile_Homing_Parent, 0x0100);
//-------------------------------------------------------------------
*/

//-------------------------------------------------------------------
//- SkelAnim --------------------------------------------------------
//-------------------------------------------------------------------

void CSkeletonAnimated::OnCreate(CWObject* _pObj)
{
	MAUTOSTRIP(CSkeletonAnimated_OnCreate, MAUTOSTRIP_VOID);
	// Data[7] is only used for attach bone index in Projectile, which is overriden here, since we don't allow outselves to be attached.
	_pObj->m_Data[7] = 0; // Index of the animation to play.
}

//-------------------------------------------------------------------

void CSkeletonAnimated::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CSkeletonAnimated_OnIncludeTemplate, MAUTOSTRIP_VOID);
	CWObject::IncludeAnimFromKey("ANIMATION", _pReg, _pMapData);
}

//-------------------------------------------------------------------

void CSkeletonAnimated::OnEvalKey(CWObject* _pObj, CWorld_Server *_pWServer, const CRegistry* _pKey)
{
	MAUTOSTRIP(CSkeletonAnimated_OnEvalKey, MAUTOSTRIP_VOID);
	CStr KeyName = _pKey->GetThisName();
	uint32 KeyHash = StringToHash(KeyName);
	switch (KeyHash)
	{
	case MHASH3('ANIM','ATIO','N'): // "ANIMATION"
		{
			CWObject_Message Msg(OBJMSG_GAME_RESOLVEANIMHANDLE);
			Msg.m_pData = (void*)(_pKey->GetThisValue().Str());
			_pObj->m_Data[7] = _pWServer->Message_SendToObject(Msg, _pWServer->Selection_GetSingleTarget(WSERVER_GAMEOBJNAME));
			break;
		}
	default:
		{
			break;
		}
	}
}

//-------------------------------------------------------------------

aint CSkeletonAnimated::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg, bool& _bHandled)
{
	MAUTOSTRIP(CSkeletonAnimated_OnClientMessage, 0);
	if (_pObj == NULL)
	{
		ConOutL("CSkeletonAnimated::OnClientMessage() - Invalid object pointer (_pObj == NULL).");
		return 0;
	}

	_bHandled = false;
	switch (_Msg.m_Msg)
	{
		case OBJMSG_SPELLS_GETANIMSTATE:
		{
			if (_Msg.m_pData == NULL)
			{
				ConOutL("CSkeletonAnimated::OnClientMessage() - Invalid data pointer (_Msg.m_pData == NULL).");
				return 0;
			}
			
			_bHandled = true;
			CXR_AnimState* pAnimState = (CXR_AnimState*)(((void**)_Msg.m_pData)[0]);
			CMat4Dfp32* pMatrix = (CMat4Dfp32*)(((void**)_Msg.m_pData)[1]);
			CMTime *pAnimTime = (CMTime *)_Msg.m_Param1;
			int iModel = _Msg.m_Param0;

			if (pAnimState == NULL)
			{
				ConOutL(CStr("CSkeletonAnimated::OnClientMessage() - Invalid data pointer (pAnimState == NULL)."));
				return 0;
			}

			if (pMatrix == NULL)
			{
				ConOutL(CStr("CSkeletonAnimated::OnClientMessage() - Invalid data pointer (pMatrix == NULL)."));
				return 0;
			}

			if ((iModel < 0) || (iModel >= CWO_NUMMODELINDICES))
			{
				ConOutL(CStrF("CSkeletonAnimated::OnClientMessage() - Invalid model index %d (iObj %d).", iModel, _pObj->m_iObject));
				return 0;
			}

			CWObject_Ext_Model::GetDefaultAnimState(_pObj, _pWClient, *pAnimState, *pAnimTime, iModel);

			// BoneAnimated Specifics...
			{
				int iAnim = _pObj->m_Data[7];

				CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[iModel]);
				if (pModel == NULL)
					return 0;

				CXR_Skeleton* pSkeleton = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);

				// Return success, for we have a model, but it has no skeleton, and is thus not skeleton animated.
				// Though the AnimState is properly default initialised for a regular non-skeleton-animated model.
				if (pSkeleton == NULL)
					return 1; 

				CReferenceCount* RefCount = (CReferenceCount*)_pObj->m_lspClientObj[iModel];
				pAnimState->m_pSkeletonInst = safe_cast<CXR_SkeletonInstance>(RefCount);
				if (pAnimState->m_pSkeletonInst == NULL)
				{
					// Something was already allocated, but it wasn't a SkeletonInstance.
					// I think this should be treated as an error. Let's at least return 0.
					if (RefCount != NULL)
						return 0;

					MRTC_SAFECREATEOBJECT_NOEX(spSI, "CXR_SkeletonInstance", CXR_SkeletonInstance);
					pAnimState->m_pSkeletonInst = spSI;

					if (pAnimState->m_pSkeletonInst == NULL)
						return 0;

					_pObj->m_lspClientObj[iModel] = pAnimState->m_pSkeletonInst;

				}
				
				pAnimState->m_pSkeletonInst->Create(pSkeleton->m_lNodes.Len());
				
				CWObject_Message Msg(OBJMSG_GAME_GETANIMFROMHANDLE, iAnim);
				CXR_Anim_SequenceData* pAnim = (CXR_Anim_SequenceData*)_pWClient->Phys_Message_SendToObject(Msg, _pWClient->Game_GetObjectIndex());
				if (pAnim == NULL)
					return 0;

				CXR_AnimLayer Layer;
				Layer.Create3(pAnim, pAnimState->m_AnimTime0, 1.0f, 1.0f, 0, 0);

				CMat4Dfp32 MatrixEval = *pMatrix;
				pSkeleton->EvalAnim(&Layer, 1, pAnimState->m_pSkeletonInst, MatrixEval, 0);
			}

			return 1;
		}
		break;
	}
	return 0;
}

//-------------------------------------------------------------------
//- HomingSkelAnim --------------------------------------------------
//-------------------------------------------------------------------
/*
void CWObject_Projectile_HomingSkelAnim::OnCreate()
{
	MAUTOSTRIP(CWObject_Projectile_HomingSkelAnim_OnCreate, MAUTOSTRIP_VOID);
	CSkeletonAnimated::OnCreate(this);
	CWObject_Projectile_HomingSkelAnim_Parent::OnCreate();
}

//-------------------------------------------------------------------

void CWObject_Projectile_HomingSkelAnim::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CWObject_Projectile_HomingSkelAnim_OnIncludeTemplate, MAUTOSTRIP_VOID);
	CSkeletonAnimated::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
	CWObject_Projectile_HomingSkelAnim_Parent::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
}

//-------------------------------------------------------------------

void CWObject_Projectile_HomingSkelAnim::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Projectile_HomingSkelAnim_OnEvalKey, MAUTOSTRIP_VOID);
	CSkeletonAnimated::OnEvalKey(this, m_pWServer, _pKey);
	CWObject_Projectile_HomingSkelAnim_Parent::OnEvalKey(_pKey);
}

//-------------------------------------------------------------------

int CWObject_Projectile_HomingSkelAnim::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Projectile_HomingSkelAnim_OnClientMessage, 0);
	bool bHandled;
	int result = CSkeletonAnimated::OnClientMessage(_pObj, _pWClient, _Msg, bHandled);
	if (bHandled)
		return result;

	return CWObject_Projectile_HomingSkelAnim_Parent::OnClientMessage(_pObj, _pWClient, _Msg);
}

//-------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Projectile_HomingSkelAnim, CWObject_Projectile_HomingSkelAnim_Parent, 0x0100);
//-------------------------------------------------------------------
*/
