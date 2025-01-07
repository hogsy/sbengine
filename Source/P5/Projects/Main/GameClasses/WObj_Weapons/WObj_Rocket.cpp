#include "PCH.h"
#include "WObj_Rocket.h"

#include "../WObj_Misc/WObj_RigidBody.h"
#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_LightIntens.h"

/*
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_SmartRocket, CWObject_Damage, 0x0100);

void CWObject_SmartRocket::OnCreate()
{
	CWObject_ProjectileCluster::OnCreate();

	m_TurnInertia = 0.5f;
	m_CurrentDestination.k[0] = -96.0f;
	m_CurrentDestination.k[1] = -64.0f;
	m_CurrentDestination.k[2] = 32.0f;

	m_SpawnTick = m_pWServer->GetGameTick();
}

void CWObject_SmartRocket::OnRefresh()
{
	if (!m_bFirstOnRefresh)
	{
		CWO_ProjectileInstance* pAllProjectiles = m_lProjectileInstance.GetBasePtr();
		int nProjectiles = m_lProjectileInstance.Len();
		int iProjectile;
		for (iProjectile=0; iProjectile<nProjectiles; iProjectile++)
		{
			CWO_ProjectileInstance* pProjectile = &pAllProjectiles[iProjectile];

			if (pProjectile->m_PendingDeath == -1)
			{
				CMat4Dfp32 Apa;
				CVec3Dfp32 StartPos = CVec3Dfp32::GetMatrixRow(pProjectile->m_Position, 3);
				Apa.Unit();
				StartPos.SetMatrixRow(Apa, 3);

				CVec3Dfp32 Dir = m_CurrentDestination - StartPos;
				Dir.SetMatrixRow(Apa, 0);
				Apa.RecreateMatrix(0, 2);


				CQuatfp32 Temp, Kamp;
				Temp.Create(Apa);
				Kamp.Create(pProjectile->m_Position);

//				float _t = 0.5f;
//				if (Kamp.DotProd(Temp) < 0.0f)
//				{
//					for(int i = 0; i < 4; i++)
//						Temp.k[i] = -Kamp.k[i] + (Temp.k[i] + Kamp.k[i])*_t;
//				}
//				else
//				{
//					for(int i = 0; i < 4; i++)
//						Temp.k[i] = Kamp.k[i] + (Temp.k[i] - Kamp.k[i])*_t;
//				}
//				Temp.Normalize();


				// Note that this quartenion is (should :) also sent to the client
				Temp.CreateMatrix(Apa);

				CVec3Dfp32 NewVelocity = CVec3Dfp32::GetMatrixRow(Apa, 0) * m_ProjectileVelocity;
				pProjectile->m_Velocity = NewVelocity;

//				m_pWServer->Debug_RenderWire(CVec3Dfp32::GetMatrixRow(pProjectile->m_Position, 3), m_CurrentDestination);
				CNetMsg VelMsg(NETMSG_PCLU_PROJECTILEINSTANCE_SETNEWVELOCITY);
				VelMsg.AddInt8(iProjectile);
				VelMsg.Addfp32(NewVelocity.k[0]);
				VelMsg.Addfp32(NewVelocity.k[1]);
				VelMsg.Addfp32(NewVelocity.k[2]);
				m_pWServer->NetMsg_SendToObject(VelMsg, m_iObject);

				CNetMsg DirMsg(NETMSG_PCLU_PROJECTILEINSTANCE_SETNEWDIRECTION);
				DirMsg.AddInt8(iProjectile);
				DirMsg.Addfp32(Temp.k[0]);
				DirMsg.Addfp32(Temp.k[1]);
				DirMsg.Addfp32(Temp.k[2]);
				DirMsg.Addfp32(Temp.k[3]);
				m_pWServer->NetMsg_SendToObject(DirMsg, m_iObject);
			};
		};
	};

	CWObject_ProjectileCluster::OnRefresh();

	int CurrentTick = m_pWServer->GetGameTick();
	int TickDiff = CurrentTick - m_SpawnTick;
	if (TickDiff > 3*20)
	{
		// Has travelled for to long, kill all projectiles (should only be one)

		CWO_ProjectileInstance* pAllProjectiles = m_lProjectileInstance.GetBasePtr();
		int nProjectiles = m_lProjectileInstance.Len();
		int iProjectile;
		for (iProjectile=0; iProjectile<nProjectiles; iProjectile++)
		{
			CWO_ProjectileInstance* pProjectile = &pAllProjectiles[iProjectile];
			if (pProjectile->m_PendingDeath == -1)
				OnImpact(iProjectile, CVec3Dfp32::GetMatrixRow(pProjectile->m_Position, 3));
		}
	}
}

void CWObject_SmartRocket::SetNewDestination(const CVec3Dfp32& _NewDestination)
{
	m_CurrentDestination = _NewDestination;
}

int CWObject_SmartRocket::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
		case OBJMSG_SMARTROCKET_SETNEWTARGET:
			SetNewDestination(_Msg.m_VecParam0);
			return 1;
	};

	return CWObject_ProjectileCluster::OnMessage(_Msg);
}

void CWObject_SmartRocket::OnImpact(int _iProjectile, const CVec3Dfp32& _Pos, uint16 _iObject, CCollisionInfo* _pCInfo)
{
	CWObject_ProjectileCluster::OnImpact(_iProjectile, _Pos, _iObject, _pCInfo);

	if (m_Shockwave.IsValid())
		m_Shockwave.Send(_Pos, NULL, 0, m_iOwner, m_pWServer);
};

void CWObject_SmartRocket::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	CWObject_ProjectileCluster::OnDeltaLoad(_pFile);
	_pFile->ReadLE(m_SpawnTick);
	m_CurrentDestination.Read(_pFile);
};

void CWObject_SmartRocket::OnDeltaSave(CCFile* _pFile)
{
	CWObject_ProjectileCluster::OnDeltaSave(_pFile);
	_pFile->WriteLE(m_SpawnTick);
	m_CurrentDestination.Write(_pFile);
}*/


#if !defined(M_DISABLE_TODELETE) || 1
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Rocket, CWObject_Explosion, 0x0100);


void CWObject_Rocket::OnCreate()
{
	CWObject_Explosion::OnCreate();

	m_Flags |= FLAGS_DEBRIS | FLAGS_SHOCKWAVE | FLAGS_CAMERASHAKE | FLAGS_DYNAMICLIGHT | FLAGS_RUMBLE;
	m_iSound[0] = m_pWServer->GetMapData()->GetResourceIndex_Sound("Wp_Rokt_Move01");
	Sound(m_pWServer->GetMapData()->GetResourceIndex_Sound("Wp_Rokt_Fire01"));
	m_iExplosionSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("Wp_Rokt_Deto01");

	m_Data[3] = m_pWServer->GetMapData()->GetResourceIndex_Model("weapons\\missile");
	m_Data[4] = m_pWServer->GetMapData()->GetResourceIndex_Model("rockettrail");
	m_Data[5] = m_pWServer->GetGameTick();

	m_iModel[0] = m_pWServer->GetMapData()->GetResourceIndex_Model("particles:MP=25,SU=p_fireblob05#2,CO=0x9FFFaF9F,CON=0x00004040,DU=1.5,DUN=0.5,FI=0.2,VE=120,VEN=25,LO=0 0 20,AX=0 0 7,AXN=0 0 5,SZ0=50,SZ1=80,SZ2=90,SZ3=200,RT0=1,RT1=-1,RT2=-1,RT3=1.5,RT4=0.5,ES=0.25,SMT=0.4,FL=sm+nl");
	m_iModel[1] = m_pWServer->GetMapData()->GetResourceIndex_Model("particles:MP=25,SU=p_smokethin02#4,CO=0xFF1F1F1F,DU=1.8,DUN=0.8,FI=0.5,AL0=0.7,VE=150,VEN=100,AX=0 0 10,DI=sphere,DIS=10 10 10 ,SZ0=25,SZ1=50,SZ2=80,SZ3=120,RT3=3,TC=0.024,ES=0.6,SMT=0.6,SD=5,FL=sd+nl");
	m_iModel[2] = m_pWServer->GetMapData()->GetResourceIndex_Model("particles:MP=1,SU=explosion03,DU=2.2,AX=0 0 14,SZ0=100,SZ1=100,SZ2=320,SZ3=320,SPB=0,TC=0.1,ES=0.1,SMT=2,FL=sm+nl");

	ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
}

void CWObject_Rocket::OnFinishEvalKeys()
{
	CWObject_Explosion::OnFinishEvalKeys();

	const CVec3Dfp32 &From = GetPosition();
	CVec3Dfp32 Dest = From + GetForward() * 2048;
	CCollisionInfo Info;
	if(m_pWServer->Phys_IntersectLine(From, Dest, 0, OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_CHARACTER, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &Info) && Info.m_bIsValid)
		m_Destiation = Info.m_Pos + GetForward() * 2;
	else
		m_Destiation = Dest;

	CMat4Dfp32 Pos;
	(CVec3Dfp32(0, 0, 1) + GetForward()).SetRow(Pos, 0);
	CVec3Dfp32(1, 0, 0).SetRow(Pos, 1);
	Pos.RecreateMatrix(0, 1);
	GetPosition().SetRow(Pos, 3);
	SetPosition(Pos);
}

void CWObject_Rocket::OnIncludeClass(CMapData *_pWData, CWorld_Server *_pWServer)
{
	CWObject_Explosion::OnIncludeClass(_pWData, _pWServer);

	_pWData->GetResourceIndex_Model("explosiondebris");
	_pWData->GetResourceIndex_Model("rockettrail");
	_pWData->GetResourceIndex_Model("weapons\\missile");

	_pWData->GetResourceIndex_Sound("Wp_Rokt_Fire01");
	_pWData->GetResourceIndex_Sound("Wp_Rokt_Move01");
	_pWData->GetResourceIndex_Sound("Wp_Rokt_Deto01");

	_pWData->GetResourceIndex_Model("particles:MP=25,SU=p_fireblob05#2,CO=0x9FFFaF9F,CON=0x00004040,DU=1.5,DUN=0.5,FI=0.2,VE=120,VEN=25,LO=0 0 20,AX=0 0 7,AXN=0 0 5,SZ0=50,SZ1=80,SZ2=90,SZ3=200,RT0=1,RT1=-1,RT2=-1,RT3=1.5,RT4=0.5,ES=0.25,SMT=0.4,FL=sm+nl");
	_pWData->GetResourceIndex_Model("particles:MP=25,SU=p_smokethin02#4,CO=0xFF1F1F1F,DU=1.8,DUN=0.8,FI=0.5,AL0=0.7,VE=150,VEN=100,AX=0 0 10,DI=sphere,DIS=10 10 10 ,SZ0=25,SZ1=50,SZ2=80,SZ3=120,RT3=3,TC=0.024,ES=0.6,SMT=0.6,SD=5,FL=sd+nl");
	_pWData->GetResourceIndex_Model("particles:MP=1,SU=explosion03,DU=2.2,AX=0 0 14,SZ0=100,SZ1=100,SZ2=320,SZ3=320,SPB=0,TC=0.1,ES=0.1,SMT=2,FL=sm+nl");
}

void CWObject_Rocket::OnRefresh()
{
	CWObject_Explosion::OnRefresh();

	if(m_iAnim0 != 0)
		return;

	const CVec3Dfp32 &Fwd = GetForward();
	CMat4Dfp32 Pos = GetPositionMatrix();
	CVec3Dfp32::GetRow(Pos, 3) += Fwd * 15;
		
	CCollisionInfo Info;
	if(m_pWServer->Phys_IntersectLine(GetPosition(), CVec3Dfp32::GetRow(Pos, 3), 0, OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_CHARACTER, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &Info) && Info.m_bIsValid)
	{
		CVec3Dfp32::GetRow(Pos, 3) += Info.m_Plane.n * 4;
		m_pWServer->Object_SetPosition(m_iObject, Pos);
		SpawnExplosion(m_iOwner);
		return;
	}
		
	fp32 Duration = (m_pWServer->GetGameTick() - m_CreationGameTick) - m_CreationGameTickFraction;
	fp32 LockDuration = 1.5f * m_pWServer->GetGameTicksPerSecond();
	if(Duration < LockDuration)
	{
		fp32 DirForce = Sqr(Duration / LockDuration) * 0.9f + 0.1f;
		CQuatfp32 QCur, QDest, QRes;
		QCur.Create(Pos);

		CVec3Dfp32 Dir = m_Destiation - CVec3Dfp32::GetRow(Pos, 3);
		Dir.SetRow(Pos, 0);
		Pos.RecreateMatrix(0, 2);

		QDest.Create(Pos);
		QCur.Lerp(QDest, DirForce, QRes);

		QRes.CreateMatrix3x3(Pos);
	}

	SetPosition(Pos);
}

CXR_ModelInstance *CWObject_Rocket::GetRocketModelInstance(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_Data[4]);
	if(!pModel)
		return NULL;

	CClientData *pCD = GetClientData(_pObj);
	if(pCD->m_spInstance1 == NULL)
	{
		pCD->m_spInstance1 = pModel->CreateModelInstance();
//		if(pCD->m_spInstance1 == NULL)
//			Error_static("CWObject_Rocket::GetRocketModelInstance", "Out of memory");
		if(pCD->m_spInstance1 != NULL)
		{
			CXR_ModelInstanceContext Context(_pWClient->GetGameTick(), _pWClient->GetGameTickTime(), _pObj, _pWClient);
			pCD->m_spInstance1->Create(pModel, Context);
		}
	}

	return pCD->m_spInstance1;
}

void CWObject_Rocket::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_Rocket::OnClientRefresh);
	CWObject_Explosion::OnClientRefresh(_pObj, _pWClient);

	CMat4Dfp32 Mat = _pObj->GetPositionMatrix();
	CXR_ModelInstance* pInstance = GetRocketModelInstance(_pObj, _pWClient);
	if (pInstance)
	{
		int RefreshTick = _pWClient->GetGameTick() - _pObj->m_Data[5];
		CXR_ModelInstanceContext Context(RefreshTick, _pWClient->GetGameTickTime(), _pObj, _pWClient);
		pInstance->OnRefresh(Context, &Mat, 1, 0);
	}
}

void CWObject_Rocket::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	CWObject_Explosion::OnClientRender(_pObj, _pWClient, _pEngine, _ParentMat);

	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

	if(_pObj->m_iAnim0 == 0 && _pObj->m_Data[3])
	{
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_Data[3]);
		if(pModel)
			_pEngine->Render_AddModel(pModel, MatIP, _pObj->GetDefaultAnimState(_pWClient));
	}

	if(_pObj->m_Data[4])
	{
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_Data[4]);
		if(pModel)
		{
			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
			AnimState.m_pModelInstance = GetRocketModelInstance(_pObj, _pWClient);
			_pEngine->Render_AddModel(pModel, MatIP, AnimState);
		}
	}
}
#endif

class CWObject_Throwable : public CWObject_RigidBody
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	int32 m_DestroyTick;
	CStr m_Spawn_Destroy;

	virtual void OnCreate()
	{
		CWObject_RigidBody::OnCreate();

		m_DestroyTick = 0;
	}

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
	{
		switch (_KeyHash)
		{
		case MHASH2('DURA','TION'): // "DURATION"
			{
				int Duration = RoundToInt(_pKey->GetThisValuef() * m_pWServer->GetGameTicksPerSecond());
				m_DestroyTick = m_pWServer->GetGameTick() + Duration;
				iAnim2() = Duration;
				break;
			}

		case MHASH4('SPAW','N_DE','STRO','Y'): // "SPAWN_DESTROY"
			{
				m_Spawn_Destroy = _pKey->GetThisValue();
				break;
			}

		case MHASH3('LIGH','T_CO','LOR'): // "LIGHT_COLOR"
			{
				Data(0) = _pKey->GetThisValuei();
				break;
			}

		case MHASH3('LIGH','T_RA','NGE'): // "LIGHT_RANGE"
			{
				iAnim1() = _pKey->GetThisValuei();
				ClientFlags() |= CWO_CLIENTFLAGS_VISIBILITY;
				break;
			}

		default:
			{
				CWObject_RigidBody::OnEvalKey(_KeyHash, _pKey);
				break;
			}
		}
	}

	virtual void OnFinishEvalKeys()
	{
		CWObject_RigidBody::OnFinishEvalKeys();

		m_ClientFlags &= ~CWO_CLIENTFLAGS_NOREFRESH;
	}

	static void OnIncludeClass(CMapData *_pWData, CWorld_Server *_pWServer)
	{
		CWObject_RigidBody::OnIncludeClass(_pWData, _pWServer);

		_pWData->GetResourceIndex_Surface("projmap_0006");
	}

	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
	{
		CWObject_RigidBody::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

		IncludeClassFromKey("SPAWN_DESTROY", _pReg, _pMapData);
	}

	virtual void OnRefresh()
	{
		CWObject_RigidBody::OnRefresh();
		m_ClientFlags &= ~CWO_CLIENTFLAGS_NOREFRESH;

		if(m_DestroyTick != 0 && m_pWServer->GetGameTick() == m_DestroyTick)
		{
			Destroy();
			if(m_Spawn_Destroy != "")
			{
				CMat4Dfp32 Mat = GetPositionMatrix();
				Mat.k[3][2] += 25.0f;
				m_pWServer->Object_Create(m_Spawn_Destroy, Mat, m_iOwner);
			}
		}
	}

	static void OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, const CMat4Dfp32& _ParentMat)
	{
//		fp32 Tick = _pObj->GetAnimTick(_pWClient) + _pWClient->GetRenderTickFrac() - _pObj->m_CreationGameTickFraction;
//		if(Tick < _pObj->m_Data[1])
		if(_pObj->m_iAnim1 > 0)
		{
			CMat4Dfp32 MatIP;
			fp32 IPTime = _pWClient->GetRenderTickFrac();
			Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

			MatIP.k[3][2] += 10;
			CVec3Dfp32::GetRow(MatIP, 3) += CVec3Dfp32::GetRow(MatIP, 2) * 10;
			CVec3Dfp32 Col(fp32((_pObj->m_Data[0] >> 16) & 0xff) / 255.0f,
				fp32((_pObj->m_Data[0] >> 8) & 0xff) / 255.0f,
				fp32((_pObj->m_Data[0] >> 0) & 0xff) / 255.0f);
			fp32 Time = (_pObj->GetAnimTick(_pWClient) + _pWClient->GetRenderTickFrac());
			const fp32 FadeTicks = 3.0f * _pWClient->GetGameTicksPerSecond();
			const fp32 FadeDuration = 1.0f * _pWClient->GetGameTicksPerSecond();
			if(Time > _pObj->m_iAnim2 - FadeTicks)
				Col *= Max((_pObj->m_iAnim2 - Time - (FadeTicks - FadeDuration)) / FadeDuration, 0.0f);
			Col *= CLightIntens::GetIntens(Time * _pWClient->GetGameTickTime(), 1, 8, 33.0f / 256);
//			Col *= 1.0f - (Tick / _pObj->m_Data[1]);
			CXR_Light Light(MatIP, Col, _pObj->m_iAnim1, 0, CXR_LIGHTTYPE_POINT);
			Light.m_LightGUID = _pObj->m_iObject*5 + 0x4000;
			Light.m_iLight = 0;
			Light.m_Flags = CXR_LIGHT_NOSHADOWS | CXR_LIGHT_NOSPECULAR;
			Light.SetProjectionMap(_pEngine->m_pTC->GetTextureID("ProjMap_0006_00"));
			CXR_SceneGraphInstance* pSGI = _pWClient->World_GetSceneGraphInstance();
			if(pSGI)
				pSGI->SceneGraph_Light_LinkDynamic(Light);
		}
	}

	//static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine);
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Throwable, CWObject_RigidBody, 0x0100);
