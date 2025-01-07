#include "PCH.h"
#include "MFloat.h"
#include "../WObj_Weapons/WObj_Spells.h"
#include "../WRPG/WRPGSpell.h"
#include "WObj_Explosion.h"
#include "../WObj_Char.h"
#include "../Models/WModel_Debris.h"

//-------------------------------------------------------------------
// Explosion
//-------------------------------------------------------------------
const char * CWObject_Explosion::ms_FlagsTranslate[] =
{
	"Instant", "Shockwave", "CameraShake", "StaticLight", "DynamicLight", "Wallmark", "Rumble", "Debris", "ShowFirstModel", "NoSelfDamage", NULL
};


void CWObject_Explosion::OnCreate()
{
	MAUTOSTRIP(CWObject_Explosion_OnCreate, MAUTOSTRIP_VOID);
	CWObject_Model::OnCreate();
	m_CameraShake.Clear();
	m_CameraShake.m_bNoRumble = true;
	ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;

	m_Flags = 0;
	m_iExplosionSound = 0;
	m_DurationTicks = (uint16)(4 * m_pWServer->GetGameTicksPerSecond());

	m_Wallmark_iSurface = 0;
	m_Wallmark_Radius = 512;
	m_Rumble_Range = 512;
	m_Rumble = "Explo_01_2sek";

	m_iAnim1 = 512;													// Light range
	m_Data[1] = (int32)(2 * m_pWServer->GetGameTicksPerSecond());	// Light duration
	m_Data[0] = 0xffffffff;											// Light color

	m_Shockwave.m_Damage.m_Damage = 100;
	m_Shockwave.m_Damage.m_DamageType = DAMAGETYPE_BLAST;
	m_Shockwave.m_Force = 100;
	m_Shockwave.m_ObjectRange = 256;
	m_Shockwave.m_CharacterRange = 512;
	m_Shockwave.m_Mass = 1000;
	m_Shockwave_IntersectionFlags = OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_TRIGGER;

	m_CameraShake.m_Duration = 2;
	m_CameraShake.m_Magnitude = 10;
	m_CameraShake.m_Speed = 5;
	m_CameraShake.m_Range = 1024;
}

void CWObject_Explosion::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Explosion_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr Val = _pKey->GetThisValue();
	fp32 Valf = _pKey->GetThisValuef();
	int Vali = _pKey->GetThisValuei();

	switch (_KeyHash)
	{
	case MHASH2('FLAG','S'): // "FLAGS"
		{
			m_Flags = Val.TranslateFlags(ms_FlagsTranslate);
			break;
		}

	case MHASH6('SHOC','KWAV','E_OB','JECT','_RAN','GE'): // "SHOCKWAVE_OBJECT_RANGE"
		{
			m_Shockwave.m_ObjectRange = Valf;
			break;
		}

	case MHASH7('SHOC','KWAV','E_CH','ARAC','TER_','RANG','E'): // "SHOCKWAVE_CHARACTER_RANGE"
		{
			m_Shockwave.m_CharacterRange = Valf;
			break;
		}

	case MHASH4('SHOC','KWAV','E_SP','EED'): // "SHOCKWAVE_SPEED"
		{
			m_Shockwave.m_Force = Valf;
			break;
		}

	case MHASH4('SHOC','KWAV','E_WE','IGHT'): // "SHOCKWAVE_WEIGHT"
		{
			m_Shockwave.m_Mass = Vali;
			break;
		}

	case MHASH4('SHOC','KWAV','E_DA','MAGE'): // "SHOCKWAVE_DAMAGE"
		{
			m_Shockwave.m_Damage.m_Damage = Vali;
			break;
		}

	case MHASH5('SHOC','KWAV','E_DA','MAGE','TYPE'): // "SHOCKWAVE_DAMAGETYPE"
		{
			m_Shockwave.m_Damage.m_DamageType = Val.TranslateFlags(CRPG_Object::ms_DamageTypeStr);
			break;
		}

	case MHASH7('SHOC','KWAV','E_IN','TERS','ECTI','ONFL','AGS'): // "SHOCKWAVE_INTERSECTIONFLAGS"
		{
			m_Shockwave_IntersectionFlags = Val.TranslateFlags(ms_ObjectFlagsTranslate);
			break;
		}

	case MHASH5('CAME','RASH','AKE_','RANG','E'): // "CAMERASHAKE_RANGE"
		{
			m_CameraShake.m_Range = Valf;
			break;
		}

	case MHASH6('CAME','RASH','AKE_','MAGN','ITUD','E'): // "CAMERASHAKE_MAGNITUDE"
		{
			m_CameraShake.m_Magnitude = Valf;
			break;
		}

	case MHASH5('CAME','RASH','AKE_','DURA','TION'): // "CAMERASHAKE_DURATION"
		{
			m_CameraShake.m_Duration = Valf;
			break;
		}

	case MHASH5('CAME','RASH','AKE_','SPEE','D'): // "CAMERASHAKE_SPEED"
		{
			m_CameraShake.m_Speed = Valf;
			break;
		}

	case MHASH3('LIGH','T_ST','ATIC'): // "LIGHT_STATIC"
		{
			m_Light_Target = Val;
			break;
		}

	case MHASH3('LIGH','T_CO','LOR'): // "LIGHT_COLOR"
		{
			Data(0) = Vali;
			break;
		}

	case MHASH3('LIGH','T_RA','NGE'): // "LIGHT_RANGE"
		{
			iAnim1() = Vali;
			break;
		}

	case MHASH4('LIGH','T_DU','RATI','ON'): // "LIGHT_DURATION"
		{
			Data(1) = RoundToInt(Valf * m_pWServer->GetGameTicksPerSecond());
			break;
		}

	case MHASH4('WALL','MARK','_SUR','FACE'): // "WALLMARK_SURFACE"
		{
			m_Wallmark_iSurface = m_pWServer->GetMapData()->GetResourceIndex_Surface(Val);
			break;
		}

	case MHASH4('WALL','MARK','_RAD','UIS'): // "WALLMARK_RADUIS"
		{
			m_Wallmark_Radius = Vali;
			break;
		}

	case MHASH2('SOUN','D'): // "SOUND"
		{
			m_iExplosionSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(Val);
			break;
		}

	case MHASH2('DURA','TION'): // "DURATION"
		{
			m_DurationTicks = (uint16)(Valf * m_pWServer->GetGameTicksPerSecond());
			break;
		}
	
	case MHASH2('RUMB','LE'): // "RUMBLE"
		{
			m_Rumble = Val;
			break;
		}

	case MHASH3('RUMB','LE_R','ANGE'): // "RUMBLE_RANGE"
		{
			m_Rumble_Range = Vali;
			break;
		}

	case MHASH3('DEBR','IS_M','ODEL'): // "DEBRIS_MODEL"
		{
			m_Data[2] = m_pWServer->GetMapData()->GetResourceIndex_Model(Val);
			break;
		}

	default:
		{
			if (KeyName.CompareSubStr("MESSAGE_EXPLODE") == 0)
			{
				int iSlot = atoi(KeyName.Str()+15);
				m_lMessages_Explode.SetMinLen(iSlot + 1);
				m_lMessages_Explode[iSlot].Parse(Val, m_pWServer);
			}
			else
				CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Explosion::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_Explosion_OnFinishEvalKeys, MAUTOSTRIP_VOID);
	CWObject_Model::OnFinishEvalKeys();

	if(m_Flags & FLAGS_DEBRIS)
	{
		Data(2) = m_pWServer->GetMapData()->GetResourceIndex_Model("ExplosionDebris");
		/*Data(6) = m_pWServer->GetMapData()->GetResourceIndex_Model("misc\\debris02b") | (m_pWServer->GetMapData()->GetResourceIndex_Model("misc\\debris04b") << 16);
		Data(7) = m_pWServer->GetMapData()->GetResourceIndex_Model("misc\\debris05b") | (m_pWServer->GetMapData()->GetResourceIndex_Model("misc\\debris06b") << 16);*/
	}
}

void CWObject_Explosion::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	CWObject_Model::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

	IncludeSurfaceFromKey("WALLMARK_SURFACE", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND", _pReg, _pMapData);
	IncludeModelFromKey("DEBRIS_MODEL", _pReg, _pMapData);

	CRegistry* pReg = _pReg->Find("FLAGS");
	if (pReg)
	{
		int Flags = pReg->GetThisValue().TranslateFlags(ms_FlagsTranslate);
		if(Flags & FLAGS_DEBRIS)
		{
			_pMapData->GetResourceIndex_Model("ExplosionDebris");
		}
	}

}


void CWObject_Explosion::OnSpawnWorld()
{
	CWObject_Model::OnSpawnWorld();

	for(int i = 0; i < m_lMessages_Explode.Len(); i++)
		m_lMessages_Explode[i].SendPrecache(m_iObject, m_pWServer);
}

void CWObject_Explosion::OnRefresh()
{
	MAUTOSTRIP(CWObject_Explosion_OnRefresh, MAUTOSTRIP_VOID);
	CWObject_Model::OnRefresh();

	if (m_Flags & FLAGS_INSTANT)
	{
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
		SpawnExplosion(m_iOwner);
	}

	if(m_iAnim0 == 1)
	{
		if((m_pWServer->GetGameTick() - m_CreationGameTick) > m_DurationTicks)
			Destroy();
	}
}

void CWObject_Explosion::SpawnExplosion(int _iSender, const aint* _pParam, int _nParam)
{
	m_Flags &= ~FLAGS_INSTANT;

	int nExclude = 0;
	int32 liExclude[2];
	if(m_Flags & FLAGS_NOSELFDAMAGE)
	{
		liExclude[0] = m_iObject;
		nExclude++;
		if(_nParam > 0)
		{
			CRPG_InitParams *pParams = (CRPG_InitParams *)_pParam[0];
			if(pParams->m_liExcludeObj[0])
			{
				liExclude[0] = pParams->m_liExcludeObj[0];
				if(pParams->m_liExcludeObj[1])
				{
					liExclude[1] = pParams->m_liExcludeObj[1];
					nExclude = 2;
				}
			}
		}
	}

	if(m_Flags & FLAGS_SHOCKWAVE && m_Shockwave.IsValid())
		m_Shockwave.Send(GetPosition(), liExclude, nExclude, _iSender, m_pWServer, m_Shockwave_IntersectionFlags);

	if(m_Flags & FLAGS_CAMERASHAKE && m_CameraShake.IsValid())
		m_CameraShake.Send(GetPosition(), _iSender, m_pWServer);

	if(m_iExplosionSound != 0)
		Sound(m_iExplosionSound);
	iSound(0) = 0;

	for(int i = 0; i < m_lMessages_Explode.Len(); i++)
		m_lMessages_Explode[i].SendMessage(m_iObject, _iSender, m_pWServer);

	if(m_Flags & FLAGS_RUMBLE && m_Rumble != "")
	{
		CWObject_Message Msg(OBJMSG_CHAR_RUMBLE);
		Msg.m_pData = (void *)m_Rumble.Str();
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddBoundSphere(Selection, OBJECT_FLAGS_PLAYER, GetPosition(), m_Rumble_Range);
		const int16 *pSel;
		int nSel = m_pWServer->Selection_Get(Selection, &pSel);
		for(int i = 0; i < nSel; i++)
		{
//			CWObject *pObj = m_pWServer->Object_Get(pSel[i]);
			m_pWServer->Message_SendToObject(Msg, pSel[i]);
		}
	}

	if(m_Flags & FLAGS_DYNAMICLIGHT)
	{
		ClientFlags() |= CWO_CLIENTFLAGS_VISIBILITY;
	}

	m_CreationGameTick = m_pWServer->GetGameTick();
	m_CreationGameTickFraction = 0;
	iAnim0() = 1;
	ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
	UpdateVisibility();
}

void CWObject_Explosion::OnInitInstance(const aint* _pParam, int _nParam)
{
	MAUTOSTRIP(CWObject_Explosion_OnInitInstance, MAUTOSTRIP_VOID);
	CWObject_Model::OnInitInstance(_pParam, _nParam);

	if (m_Flags & FLAGS_INSTANT)
	{
		if (_pParam)
		{
			// Spawn now (yuck)
			SpawnExplosion(m_iOwner, _pParam, _nParam);
		}
		else
		{
			// Spawn from OnRefresh
			ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
		}
	}
}

aint CWObject_Explosion::OnMessage(const CWObject_Message &_Msg)
{
	if(_Msg.m_Msg == OBJMSG_IMPULSE && m_iAnim0 == 0)
	{
		SpawnExplosion(_Msg.m_iSender);
		return 1;
	}

	return CWObject_Model::OnMessage(_Msg);
}

void CWObject_Explosion::OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Explosion_OnClientRenderVis, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Explosion::OnClientRenderVis);

	if(_pObj->m_iAnim0 == 1)
	{
		fp32 Tick = _pObj->GetAnimTick(_pWClient) + _pWClient->GetRenderTickFrac() - _pObj->m_CreationGameTickFraction;
		if(Tick < _pObj->m_Data[1])
		{
			CMat4Dfp32 Mat = _pObj->GetPositionMatrix();
			CVec3Dfp32 Col(fp32((_pObj->m_Data[0] >> 16) & 0xff) / 255.0f,
						  fp32((_pObj->m_Data[0] >> 8) & 0xff) / 255.0f,
						  fp32((_pObj->m_Data[0] >> 0) & 0xff) / 255.0f);
			Col *= 1.0f - (Tick / _pObj->m_Data[1]);
			CXR_Light Light(Mat, Col, _pObj->m_iAnim1, 0, CXR_LIGHTTYPE_POINT);

			static int g_ExplosionGUID = 0;

			Light.m_LightGUID = 0x7100 + (g_ExplosionGUID & 0xff);
			g_ExplosionGUID++;

//			Light.m_LightGUID = MRTC_RAND() + 0x4000;	// wtf?!
			Light.m_iLight = 0;
			Light.m_Flags = CXR_LIGHT_NOSHADOWS | CXR_LIGHT_NOSPECULAR;
			CXR_SceneGraphInstance* pSGI = _pWClient->World_GetSceneGraphInstance();
			if(pSGI)
				pSGI->SceneGraph_Light_LinkDynamic(Light);
		}
	}
}

CWObject_Explosion::CClientData *CWObject_Explosion::GetClientData(CWObject_CoreData* _pObj)
{
	if(!_pObj->m_lspClientObj[0])
	{
		_pObj->m_lspClientObj[0] = MNew(CClientData);
		if(!_pObj->m_lspClientObj[0])
			Error_static("CWObject_Explosion::GetClientData", "Out of memory");
	}

	return (CClientData *)(CReferenceCount *)_pObj->m_lspClientObj[0];
}

void CWObject_Explosion::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Explosion_OnClientRender, MAUTOSTRIP_VOID);

	if(_pObj->m_iAnim0 == 1)
	{
		for(int i = 0; i < CWO_NUMMODELINDICES; i++)
		{
			if(_pObj->m_iModel[i] > 0)
			{
				CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
				if(pModel)
					_pEngine->Render_AddModel(pModel, _pObj->GetPositionMatrix(), _pObj->GetDefaultAnimState(_pWClient, i));
			}
		}
		if(_pObj->m_Data[2])
		{
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_Data[2]);
			if(pModel)
			{
				CClientData *pCD = GetClientData(_pObj);
				if(pCD->m_spDebrisInstance == NULL)
				{
					CXR_Model_Debris *pDebris = safe_cast<CXR_Model_Debris >(pModel);
					pCD->m_spDebrisInstance = pDebris->Compile(MRTC_RAND(), _pObj->GetPositionMatrix(), _pWClient);
				}
				CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
				AnimState.m_pModelInstance = pCD->m_spDebrisInstance;
				/*AnimState.m_Colors[0] = int(_pWClient->GetMapData()->GetResource_Model(_pObj->m_Data[6] >> 16));
				AnimState.m_Colors[1] = int(_pWClient->GetMapData()->GetResource_Model(_pObj->m_Data[6] & 0xffff));
				AnimState.m_Colors[2] = int(_pWClient->GetMapData()->GetResource_Model(_pObj->m_Data[7] >> 16));
				AnimState.m_Colors[3] = int(_pWClient->GetMapData()->GetResource_Model(_pObj->m_Data[7] & 0xffff));*/
				_pEngine->Render_AddModel(pModel, _pObj->GetPositionMatrix(), AnimState);
			}
		}
	}
	
/*	fp32 AnimTime, TickFrac;
	GetAnimTime(_pObj, _pWClient, AnimTime, TickFrac);

	CMat4Dfp32 Matrix;
	if(!GetRenderMatrix(_pObj, _pWClient, Matrix, TickFrac))
		return;

	RenderModelsDefault(AnimTime, TickFrac, Matrix, _pObj, _pWClient, _pEngine);

	fp32 AnimDuration = fp32((uint16)_pObj->m_iAnim2) * fp32(SERVER_TIMEPERFRAME);

	fp32 TimeFraction = AnimTime / AnimDuration;
	fp32 InvTimeFraction = 1.0f - TimeFraction;
	fp32 InvTimeFraction2 = Sqr(InvTimeFraction);
	fp32 InvTimeFraction4 = Sqr(InvTimeFraction2);

	fp32 Radius = _pObj->m_Data[0];
	CFStr WallmarkSurface = "S_DROPSHADOW";
	int iWallmarkSurface = 0; // m_iData[1];
	CPixel32 LightColor32 = ~_pObj->m_Data[2];

	CVec3Dfp32 LightColor = CVec3Dfp32(LightColor32.GetR(), LightColor32.GetG(), LightColor32.GetB()) * LightColor32.GetA() * InvTimeFraction4;
	fp32 LightRadius = Radius * InvTimeFraction4;

	if((_pObj->m_ClientData[0] == 0) && (iWallmarkSurface != 0))
	{
		CMat4Dfp32 WallMarkPos;
		WallMarkPos.Unit();
		CVec3Dfp32::GetRow(WallMarkPos, 2) = CVec3Dfp32(0, 0, 1);
		CVec3Dfp32::GetRow(WallMarkPos, 0) = CVec3Dfp32(1, 0, 0);
		WallMarkPos.RecreateMatrix(2, 0);
		CVec3Dfp32::GetRow(WallMarkPos, 3) = CVec3Dfp32::GetMatrixRow(Matrix, 3);

		_pWClient->Wallmark_Create(WallMarkPos, Radius, Radius, _pWClient->GetRenderTime(), WallmarkSurface, 0);

		_pObj->m_ClientData[0] = 1;
	}*/

	//	if(LightColor != 0)
	//		_pEngine->Render_Light_AddDynamic(CVec3Dfp32::GetMatrixRow(Matrix, 3), LightColor, LightRadius, 0, 0);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Explosion, CWObject_Model, 0x0100);
