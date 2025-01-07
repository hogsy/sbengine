/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_DarklingSpawn.cpp

	Author:			Olle Rosenquist

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_DarklingSpawn

	Comments:		

	History:		
		0511xx:		Created File
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_DarklingSpawn.h"
#include "../WObj_Char.h"
#include "../WObj_Char/WObj_CharDarkling.h"
#include "../WObj_Misc/WObj_Room.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_DarklingSpawn, CWObject_DarklingSpawn_Parent, 0x0100);

//#pragma optimize("", off)
//#pragma inline_depth(0)

enum
{
	class_Sound = MHASH5('CWOb','ject','_','Soun','d'),
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObj_ObjLightMeter
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObj_ObjLightMeter::CWObj_ObjLightMeter()
{
	m_LastMeasureTick = 0;
	for (int32 i = 0; i < CWObj_ObjLightMeter::MaxLights; i++)
	{
		m_lpLights[i] = NULL;
		m_lpLightIntensities[i] = -1;	// -1 means invalid unmeasured light
	}
	m_nLights = 0;
	m_LightMapIntensity = 0.0f;
	m_PrevLightIntensity = 0.5;
	m_LightIntensity = 0.5;
	m_bIntensityValid = false;
	m_iOwnerFlashLight = 0;
	m_FlashlightIntensity = 0.0f;

	m_MeasureInterval = 20;

	m_Slop = 4.0f;

	INVALIDATE_POS(m_Pos);
}

CWObj_ObjLightMeter::~CWObj_ObjLightMeter()
{
}

void CWObj_ObjLightMeter::Init(CWorld_Server* _pWServer, const fp32& _MeasureInterval)
{
	m_MeasureInterval = TruncToInt(_pWServer->GetGameTicksPerSecond() * _MeasureInterval);
	m_LastMeasureTick = _pWServer->GetGameTick() - m_MeasureInterval;
}


// MeasureFakePointLight() measures the light intersecting a box at _Pos with dimensions _Width, _Height
// _Light is the point light not yet added to the world that should work as a fake light for measurement
// Returns the intensity of the fake light
fp32 CWObj_ObjLightMeter::MeasureFakePointLight(CWorld_Server* _pWServer, const CVec3Dfp32& _BoxPos, const CVec3Dfp32& _MeasurePos, fp32 _Width, fp32 _Height, const CXR_Light& _Light, const bool _TraceLine, const int _iExclude0, const int _iExclude1)
{
	MSCOPESHORT(CWObj_ObjLightMeter::MeasureFakePointLight);

	// Measure point light
	const CVec4Dfp32 Intensity = _Light.GetIntensityv();
	const fp32 IntensFactor = Max3(Intensity.k[0], Intensity.k[1], Intensity.k[2]);
	
	const CVec3Dfp32 LightPos = _Light.GetPosition();
	const CVec3Dfp32 Delta = LightPos - _MeasurePos;
	const fp32 DeltaSqr = Delta.LengthSqr();
	const int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
	const int32 Mediums = XW_MEDIUM_SOLID;

	if (DeltaSqr > Sqr(_Light.m_Range))
		return 0.0f;

	if ((_TraceLine) && _pWServer->Phys_IntersectLine(_MeasurePos, LightPos, ObjectFlags, ObjectFlags, Mediums, _iExclude0, _iExclude1))
	{
		#ifndef M_RTM
		if (_pWServer->Debug_GetWireContainer())
		{
			const int Color = 0xffff0000 | (255 << 16) | (0 << 8) | (0);
			_pWServer->Debug_RenderWire(LightPos, _MeasurePos, Color, 1.0f);
		}
		#endif

		return 0.0f;
	}

	const fp32 RangeFactor  = (_Light.m_Range - M_Sqrt(DeltaSqr)) * _Light.m_RangeInv;
		
	// Draw debug
	#ifndef M_RTM
	if (_pWServer->Debug_GetWireContainer())
	{
		const fp32 Intens = RangeFactor * IntensFactor;
		if ((_TraceLine))
		{
			const int R = MinMT(int(Intens * Intensity.k[0] * 255.0f), 255);
			const int G = MinMT(int(Intens * Intensity.k[1] * 255.0f), 255);
			const int B = MinMT(int(Intens * Intensity.k[2] * 255.0f), 255);
			const int Color = 0xff000000 | (R << 16) | (G << 8) | (B);
			_pWServer->Debug_RenderWire(LightPos, _MeasurePos, Color, 1.0f);
		}

		return Intens;
	}
	else
	#endif

	return RangeFactor * IntensFactor;
}


// Measure() measures the light intersecting a box at _Pos with dimensions _Width, _Height
// The _nLights tells us the number of lights to sample this call (-1 will sample all lights)
// Returns true if all lights have been sampled at least once and false if not
bool CWObj_ObjLightMeter::Measure(CWorld_Server* _pWServer, const bool& _bConsiderLightmap, const CVec3Dfp32& _BoxPos, const CVec3Dfp32& _MeasurePos, fp32 _Width, fp32 _Height, int _nLights, bool _TraceLines, int _iSkipObj)
{
	MSCOPESHORT(CWObj_ObjLightMeter::Measure);

	// Check if it's time to measure light intensity
	if ((m_LastMeasureTick + m_MeasureInterval) > _pWServer->GetGameTick())
		return false;
	
	fp32 halfWidth = _Width * 0.5f;
	fp32 halfHeight = _Height * 0.5f;
	if ((_nLights < 0)||(_TraceLines)||(m_Pos.DistanceSqr(_BoxPos) > Sqr(m_Slop)))
	{	// Position has moved too much, we must resample lights
		CBox3Dfp32 Box;
		Box.m_Max = CVec3Dfp32( halfWidth, halfWidth, halfHeight);
		Box.m_Min = -Box.m_Max;
		Box.m_Max += _BoxPos;
		Box.m_Min += _BoxPos;

		CXR_SceneGraphInstance* pSceneGraph = _pWServer->World_GetSceneGraphInstance();
		if (!pSceneGraph)
			return false;

		const CXR_Light* lpLights[CWObj_ObjLightMeter::MaxLights];
		int nLights = pSceneGraph->SceneGraph_Light_Enum(Box, lpLights, CWObj_ObjLightMeter::MaxLights);
		for (int i = 0; i < CWObj_ObjLightMeter::MaxLights; i++)
		{
			m_lpLightIntensities[i] = -1;
			m_bIntensityValid = false;
		}
		m_FlashlightIntensity = 0.0f;
		m_iOwnerFlashLight = 0;
		memcpy(m_lpLights,lpLights,nLights * sizeof(const CXR_Light*));
		m_nLights = nLights;

		if (VALID_POS(m_Pos))
		{
			if (m_Pos.DistanceSqr(_BoxPos) > Sqr(m_Slop))
			{	// Check lightmap values
				//CXR_Model* pModel = m_pAI->m_pServer->World_GetModel();
				CXR_Model* pModel = _pWServer->World_GetModel();
				m_LightMapIntensity = 0;
				if (pModel)
				{
					CXR_LightVolume* pLightVolume = (CXR_LightVolume*)pModel->GetParam(CXR_MODEL_PARAM_LIGHTVOLUME);
					if (pLightVolume)
					{
						CVec4Dfp32 lLightDirs[6];
						fp32 lightmapIntensity = 0;
						pLightVolume->Light_EvalPoint(m_Pos,lLightDirs);
						for (int i = 0; i < 6; i++)
						{
							lightmapIntensity = Max3(lLightDirs[i].k[0],lLightDirs[i].k[1],lLightDirs[i].k[2]);
							m_LightMapIntensity = Max(m_LightMapIntensity,lightmapIntensity);
						}
						// *** DEBUG: Ugly fix until Light_EvalPoint() data is correct, 2.0 seems about OK ***
						m_LightMapIntensity *= 2.0f; 
					}
				}
			}
		}

		m_Pos = _BoxPos;
	}

	if (m_nLights == 0)
	{
		// No lights to worry about
		m_LightIntensity = (_bConsiderLightmap) ? m_LightMapIntensity : 0.0f;
		m_bIntensityValid = true;
		return(m_bIntensityValid);
	}

	for (int32 curLight = 0; curLight < m_nLights; curLight++)
	{
		const CXR_Light *pLight = m_lpLights[curLight];
		if(!pLight) // safety check
		{
			continue;
		}

		CVec4Dfp32 Intensity = pLight->GetIntensityv();
		fp32 IntensFactor = Max3(Intensity.k[0], Intensity.k[1], Intensity.k[2]);
		if (IntensFactor < 0.01f)
		{
			continue;
		}

		// Light hit us, let's calculate its intensity
		CVec3Dfp32 LightPos = pLight->GetPosition();
		CVec3Dfp32 Delta = LightPos - _MeasurePos;
		fp32 DeltaSqr = Delta.LengthSqr(); // basic info
		if (DeltaSqr > Sqr(pLight->m_Range))
		{
			m_lpLightIntensities[curLight] = 0;
			continue;
		}

		int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT; 
		int32 Mediums = XW_MEDIUM_SOLID /*| XW_MEDIUM_PHYSSOLID | XW_MEDIUM_PLAYERSOLID | XW_MEDIUM_GLASS | XW_MEDIUM_CAMERASOLID*/;
		CCollisionInfo Info;
		Info.Clear();
		if ((_TraceLines) && (_pWServer->Phys_IntersectLine(_MeasurePos,pLight->GetPosition(),ObjectFlags,ObjectFlags,Mediums,NULL,_iSkipObj)))
		{
			m_lpLightIntensities[curLight] = 0;
			continue;
		}

		fp32 RangeFactor = (pLight->m_Range-M_Sqrt(DeltaSqr)) * pLight->m_RangeInv;

		if (pLight->m_Type == CXR_LIGHTTYPE_SPOT)
		{	// For spotlights we must also determine if we are inside the lightcone and handle
			// attenuation as we get closer to the edge of the lightcone
			// spot light
			if (pLight->m_LightGUID & 0x4002)
			{
				m_FlashlightIntensity = 0.0f;
				m_iOwnerFlashLight = (pLight->m_LightGUID & ~0x4002) / 5;
			}

			if (pLight->m_SpotWidth * pLight->m_SpotHeight <= _FP32_EPSILON)
			{
				m_lpLightIntensities[curLight] = 0;
				continue;
			}

#ifdef DEBUG_DRAW_LIGHTMETER
			if ((m_pAI->DebugRender())&&(_TraceLines))
			{
				m_pAI->Debug_RenderWire(LightPos,_MeasurePos,kColorRed,1.0f);
			}
#endif
			// Measure lightray closest distance from capsule (cylinder with rounded caps)
			/*
			CVec3Dfp32 MinPoint;
			fp32 MinRange = SqrDistanceToSegment(const CVec3Dfp32& _P,const CVec3Dfp32& _B,const CVec3Dfp32& _M,fp32* _pT = NULL,CVec3Dfp32* _pDiff = NULL
			*/

			CVec3Dfp32 LightToPosDir = Delta.Normalize();
			CVec3Dfp32 LightDir = pLight->GetDirection();
//			fp32 AngleDelta = LightToPosDir * LightDir; // * = Dot Product

			CMat4Dfp32 LightPosInv;
			CVec3Dfp32 LightVec;
			pLight->m_Pos.InverseOrthogonal(LightPosInv);
			m_Pos.MultiplyMatrix(LightPosInv, LightVec);

			if(LightVec[0] < _FP32_EPSILON) // skip light, the char is behind the spot light
			{
				m_lpLightIntensities[curLight] = 0;
				continue;
			}

			fp32 xProj = LightVec[1] / (LightVec[0] * pLight->m_SpotWidth);
			fp32 yProj = LightVec[2] / (LightVec[0] * pLight->m_SpotHeight);
			fp32 rProj = Length2(xProj, yProj);

			if ((rProj < 1.0f)||(DeltaSqr <= Sqr(halfWidth * 2.0f)))
			{
				if (pLight->m_LightGUID & 0x4002)
				{
					m_FlashlightIntensity = RangeFactor * IntensFactor;
				}
			}
			else
			{
				m_lpLightIntensities[curLight] = 0.0f;
				continue;
			}
		}

		fp32 Intens = RangeFactor * IntensFactor;
		m_lpLightIntensities[curLight] = Intens;

		#ifndef M_RTM
		if (_pWServer->Debug_GetWireContainer())
		{
			CVec3Dfp32 LightPos = pLight->GetPosition();
			int Color = 0xff000000;
			int R = Min(int(Intens * Intensity[0] * 255.0f),255);
			int G = Min(int(Intens * Intensity[1] * 255.0f),255);
			int B = Min(int(Intens * Intensity[2] * 255.0f),255);
			Color |= (R << 16) | (G << 8) | (B);
			_pWServer->Debug_RenderWire(LightPos, _MeasurePos, Color, 1.0f);
		}
		#endif
	}

	#ifndef M_RTM
	{
		if (_pWServer->Debug_GetWireContainer())
			_pWServer->Debug_RenderVertex(m_Pos, kColorWhite, 1.0f);
	}
	#endif

	m_LastMeasureTick = _pWServer->GetGameTick();
	UpdateIntensity(_bConsiderLightmap);

	return(m_bIntensityValid);
}

fp32 CWObj_ObjLightMeter::GetIntensity(int _iObj)
{
	fp32 rIntesity = m_LightIntensity;
	if (_iObj == m_iOwnerFlashLight)
	{
		rIntesity -= m_FlashlightIntensity;
		if (rIntesity < 0.0f)
		{
			rIntesity = 0.0f;
		}
	}
	
	return(rIntesity);
};

void CWObj_ObjLightMeter::UpdateIntensity(const bool& _bConsiderLightmap)
{
	m_PrevLightIntensity = m_LightIntensity;
	m_LightIntensity = 0;
	m_bIntensityValid = true;
	for (int i = 0; i < m_nLights; i++)
	{
		if (m_lpLightIntensities[i] >= 0)
		{
			m_LightIntensity += m_lpLightIntensities[i];
		}
		else
		{
			m_bIntensityValid = false;
		}
	}

	// Add lightmap intensity
	if(_bConsiderLightmap)
		m_LightIntensity += m_LightMapIntensity;

	if (m_LightIntensity > 1.0f)	
	{
		m_LightIntensity = 1.0f;
	}	
};

void CWObj_ObjLightMeter::MakeMeasure(const int& _MeasureTick)
{
	// Set last measure tick so we can do an update at a specific time.
	m_LastMeasureTick = (_MeasureTick - m_MeasureInterval);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_DarklingSpawn
|__________________________________________________________________________________________________
\*************************************************************************************************/
TArray<int32> CWObject_DarklingSpawn::m_slSpawnedDarklings;

void CWObject_DarklingSpawn::OnCreate()
{
	//m_DarklingSpawnFlags = 0;
	//m_DarklingSpawnType.Clear();
	m_DarklingSpawnTypeChoice = -1;
	m_spLightMeter = NULL;
	m_LightThreshold = 0.3f;
	m_TickNoRefresh = 0;
	m_TickDestroyWallmark = 0;
	m_TickSpawnDarkling = 0;
	m_bCanSpawn = false;
	m_UsableRadius = 256.0f;
	m_LastIntensity = 1.0f;
	m_iPlayerInRange = 0;
	m_iSoundActive = 0;
	m_iSoundTurnActive = 0;
	m_iSoundNearby = 0;
	m_ModeFlags = 0;
	m_iDarkling = 0;
	m_lSpawnStrings.SetLen(DARKLINGSPAWN_SPAWNSTRING_SLOT_NUMSPAWNSTRINGS);
	m_Data[0] = (32 << 20) | (4 << 12);
}

void CWObject_DarklingSpawn::OnDestroy()
{
	m_spLightMeter = NULL;
}

void CWObject_DarklingSpawn::OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer)
{
	CWObject_Model::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
	// Include sounds
	CRegistry* pChildReg = _pReg->FindChild("SOUND_ACTIVE");
	if(pChildReg)
		_pMapData->GetResourceIndex_Sound(pChildReg->GetThisValue());
	
	pChildReg = _pReg->FindChild("SOUND_TURNACTIVE");
	if(pChildReg)
		_pMapData->GetResourceIndex_Sound(pChildReg->GetThisValue());
	
	pChildReg = _pReg->FindChild("SOUND_NEARBY");
	if(pChildReg)
		_pMapData->GetResourceIndex_Sound(pChildReg->GetThisValue());
}

void CWObject_DarklingSpawn::OnIncludeClass(CMapData* _pWData, CWorld_Server* _pWServer)
{
	CWObject_DarklingSpawn_Parent::OnIncludeClass(_pWData, _pWServer);
	
	// If we are not running an RTM we allow some errors in object setup and try to setup a default
	{
		_pWData->GetResourceIndex_Template("DarknessEffect_DarklingSpawn");
		_pWData->GetResourceIndex_Model("EffectSystem:FXEffect,fx_spawnpoint_light");
		_pWData->GetResourceIndex_Model("EffectSystem:FXEffect,fx_spawnpoint_shadow");
		_pWData->GetResourceIndex_Model("EffectSystem:FXEffect,darklingfx_burn");
//		_pWData->GetResourceIndex_Surface("darkling-spawn-symbol");
//		_pWData->GetResourceIndex_Surface("darkling-spawn-select");
	}
}

bool CWObject_DarklingSpawn::CheckCanPlayerSpawn(int32 _iPlayer)
{
	// Get player darkling info
//	CWObject_Message Msg(OBJMSG_CHAR_DARKLINGAVAILABLE);
//	Msg.m_pData = m_DarklingSpawnType.GetStr();
	aint CanSpawn = 1;

	if (m_Data[0] & DARKLINGSPAWN_FLAGS_ALWAYSENABLED)
	{
		m_bCanSpawn = true;
		return true;
	}

	//Should check what darklings the player has here, berserker, gunner, lightkiller, kamikaze
	//However the system for this isn't done yet and the data doesn't exist yet
	//So for now m_bCanSpawn is always true

/*	if (_iPlayer != -1)
	{
		if (!(m_Data[0] & DARKLINGSPAWN_FLAGS_GOTCHARACTERINFO))
			Data(0) |= DARKLINGSPAWN_FLAGS_GOTCHARACTERINFO;
		CanSpawn |= m_pWServer->Message_SendToObject(Msg,_iPlayer);
	}
	else
	{
		CWObject_Game *pGame = m_pWServer->Game_GetObject();
		// Send to all players, if it's available on any set that we can spawn
		for (int i = 0; i < pGame->Player_GetNum(); i++)
		{
			CWObject* pChar = pGame->Player_GetObject(i);
			if (pChar)
			{
				if (!(m_Data[0] & DARKLINGSPAWN_FLAGS_GOTCHARACTERINFO))
					Data(0) |= DARKLINGSPAWN_FLAGS_GOTCHARACTERINFO;
				CanSpawn |= m_pWServer->Message_SendToObject(Msg,pChar->m_iObject);
			}
		}
	}*/
	m_bCanSpawn = CanSpawn != 0;

	return m_bCanSpawn;
}

void CWObject_DarklingSpawn::SetEnabled(bool _bEnable)
{
	if (_bEnable && m_bCanSpawn)
	{
		// Enable this spawnpoint if possible
		Data(0) &= ~(DARKLINGSPAWN_FLAGS_WAITSPAWNED|DARKLINGSPAWN_FLAGS_DISABLED);
		ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
		//m_PhysState.m_ObjectFlags |= OBJECT_FLAGS_PICKUP;
	}
	else if (!(m_Data[0] & DARKLINGSPAWN_FLAGS_DISABLED))
	{
		// Disable this spawnpoint
		m_Data[0] |= DARKLINGSPAWN_FLAGS_DISABLED;
		//m_PhysState.m_ObjectFlags &= ~OBJECT_FLAGS_PICKUP;

		// Disable refresh after one second
		m_TickNoRefresh = m_pWServer->GetGameTick() + TruncToInt(m_pWServer->GetGameTicksPerSecond() * 1.0f);
		m_TickDestroyWallmark = m_pWServer->GetGameTick() + (TruncToInt(m_pWServer->GetGameTicksPerSecond() * 1.75f) - 1);
	}
}

bool CWObject_DarklingSpawn::CheckPrefabLights()
{
	// Check lights
	bool bLampsOk = true;
	int32 iLastEffect = 0;
	CWObject_Message MsgOn(OBJMSG_DARKLINGSPAWN_SENDDARKLIGHTS,1);
	CWObject_Message MsgOff(OBJMSG_DARKLINGSPAWN_SENDDARKLIGHTS);
	CWObject_Message Msg(OBJMSG_LIGHT_GETSTATUS);
	TAP_RCD<struct sPrefabIndices> lEffects = m_lDarklingSpawnEffects;
	TAP_RCD<struct sPrefabIndices> lLights = m_liLinkedLamps;
	for (int32 i = 0; i < lLights.Len(); i++)
	{
		if (m_pWServer->Message_SendToObject(Msg,lLights[i].m_iObject))
		{
			bLampsOk = false;
			// Lamp is on, turn off "dark" light
			if (lLights[i].m_iPrefab != -1)
			{
				for (; iLastEffect < lEffects.Len(); iLastEffect++)
				{
					if (lEffects[iLastEffect].m_iPrefab < 0)
						continue;
					if (lLights[i].m_iPrefab < lEffects[iLastEffect].m_iPrefab)
						break;
					m_pWServer->Message_SendToObject(MsgOff,lEffects[iLastEffect].m_iObject);
				}
			}
		}
		else
		{
			// Check against prefab objects
			// Lamp is on, turn off "dark" light
			if (lLights[i].m_iPrefab != -1)
			{
				for (; iLastEffect < lEffects.Len(); iLastEffect++)
				{
					if (lEffects[iLastEffect].m_iPrefab < 0)
						continue;
					if (lLights[i].m_iPrefab < lEffects[iLastEffect].m_iPrefab)
						break;
					m_pWServer->Message_SendToObject(MsgOn,lEffects[iLastEffect].m_iObject);
				}
			}
		}
	}

	return bLampsOk;
}

void CWObject_DarklingSpawn::SyncDarklingSpawn(int32 _DarklingSpawnTypeChoice)
{
	// Spawn darklingtype at this position (directly for now..)
	CMat4Dfp32 SpawnPlace = GetPositionMatrix();
	// Spawn 16 units up (?)
	//SpawnPlace.GetRow(3) += SpawnPlace.GetRow(2) * 16.0f;

	/*
	int iObj = m_pWServer->Object_Create(m_DarklingSpawnType, SpawnPlace);
	m_pWServer->Object_SetName(iObj, m_DarklingSpawnType);

	CWObject_CharDarkling* pDarkling = (CWObject_CharDarkling*)m_pWServer->Object_Get(iObj);
	if (pDarkling)
		pDarkling->Char_PrepareSpawn();
	*/

	m_TickSpawnDarkling = m_pWServer->GetGameTick();// + (TruncToInt(m_pWServer->GetGameTicksPerSecond() * 1.0f) - 1);

	// Spawn darkling spawn effect (This object is self destruct, so it will remove itself after complete.)
	int iObj = m_pWServer->Object_Create(m_SpawnEffect, SpawnPlace);
	CWObject* pObject = m_pWServer->Object_Get(iObj);
	if (pObject)
		pObject->SetAnimTick(m_pWServer, 0, 0.0f);

	//m_PhysState.m_ObjectFlags &= ~OBJECT_FLAGS_PICKUP;
	if (m_Data[0] & DARKLINGSPAWN_FLAGS_DISABLEONUSE)
	{
		m_Data[0] |= DARKLINGSPAWN_FLAGS_DISABLED | DARKLINGSPAWN_FLAGS_INACTIVE;
		
		// Disable refresh and wallmark after one second
		m_TickNoRefresh = m_pWServer->GetGameTick() + TruncToInt(m_pWServer->GetGameTicksPerSecond() * 1.0f);
		//ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
		SetEnabled(false);
		UpdateSpawnEffects(false);
	}

	m_TickDestroyWallmark = m_pWServer->GetGameTick() + (TruncToInt(m_pWServer->GetGameTicksPerSecond() * 1.75f) - 1);

	m_DarklingSpawnTypeChoice = _DarklingSpawnTypeChoice;
}


void CWObject_DarklingSpawn::OnRefresh()
{
	// Refresh parent
	CWObject_DarklingSpawn_Parent::OnRefresh();
	
	if (!(m_Data[0] & DARKLINGSPAWN_FLAGS_GOTCHARACTERINFO))
	{
		CheckCanPlayerSpawn();
		SetEnabled((m_Data[0] & DARKLINGSPAWN_FLAGS_SHOULDBEENABLED) != 0);
	}

	// Measure light intensity of object
	if (!(m_Data[0] & DARKLINGSPAWN_FLAGS_LAMPSMUSTBEDESTROYED))
	{
		if (m_spLightMeter)
		{
			const fp32 OldIntensity = m_spLightMeter->GetIntensity(0);
			MeasureLightIntensity();
			const fp32 NewIntensity = m_spLightMeter->GetIntensity(0);
			m_LastIntensity = NewIntensity;

			// Check if we need to do anything about the render state
			if (OldIntensity < DARKLINGSPAWN_MAX_LIGHTINTENSITY && NewIntensity >= DARKLINGSPAWN_MAX_LIGHTINTENSITY)
			{
				// Switch from activationable effect to unactivationable
				Data(0) = Data(0) | DARKLINGSPAWN_FLAGS_CREATEWALLMARK;
				Data(2) = Data(2) & ~DARKLINGSPAWN_MODEL_USEVARIATION;
			}

			else if (OldIntensity >= DARKLINGSPAWN_MAX_LIGHTINTENSITY && NewIntensity < DARKLINGSPAWN_MAX_LIGHTINTENSITY)
			{
				// Switch from unactivationable to activationable effect
				Data(0) = Data(0) | DARKLINGSPAWN_FLAGS_CREATEWALLMARK;
				Data(2) = Data(2) | DARKLINGSPAWN_MODEL_USEVARIATION;
			}
		}
		else
		{
			// Create lightmeter and setup intensity
			MeasureLightIntensity();
			const fp32 NewIntensity = m_spLightMeter->GetIntensity(0);
			m_LastIntensity = NewIntensity;

			// Determine if we are to render a usable or unusable spawnpoint
			if (NewIntensity >= DARKLINGSPAWN_MAX_LIGHTINTENSITY)
			{
				Data(0) = Data(0) | DARKLINGSPAWN_FLAGS_CREATEWALLMARK;
				Data(2) = Data(2) & ~DARKLINGSPAWN_MODEL_USEVARIATION;
			}
			else if (NewIntensity < DARKLINGSPAWN_MAX_LIGHTINTENSITY)
			{
				Data(0) = Data(0) | DARKLINGSPAWN_FLAGS_CREATEWALLMARK;
				Data(2) = Data(2) | DARKLINGSPAWN_MODEL_USEVARIATION;
			}
			else if (Data(0) & DARKLINGSPAWN_FLAGS_CREATEWALLMARK)
				Data(0) = Data(0) & ~DARKLINGSPAWN_FLAGS_CREATEWALLMARK;
		}
	}

	// Check if we are to disable this spawnpoint
	if (!(m_Data[0] & DARKLINGSPAWN_FLAGS_DISABLED))
	{
		int iDarkling = m_pWServer->Selection_GetSingleTarget(m_DarklingSpawnName);
        if ((iDarkling > 0 || m_TickSpawnDarkling != 0) && !(m_Data[0] & DARKLINGSPAWN_FLAGS_DISABLERENDER))
		{
			// Disable due to other spawnpoint used
			if (m_TickDestroyWallmark == 0)
			{
				Data(0) |= DARKLINGSPAWN_FLAGS_DISABLERENDER;
				m_TickDestroyWallmark = m_pWServer->GetGameTick();
			}

            //m_PhysState.m_ObjectFlags &= ~OBJECT_FLAGS_PICKUP;
			//m_TickDestroyWallmark = m_pWServer->GetGameTick() + (TruncToInt(m_pWServer->GetGameTicksPerSecond() * 1.0f) - 1);
		}
		else if (iDarkling <= 0 && (m_Data[0] & DARKLINGSPAWN_FLAGS_DISABLERENDER))
		{
			// No darkling found, enable spawnpoint again.
			Data(0) &= ~DARKLINGSPAWN_FLAGS_DISABLERENDER;
			//m_PhysState.m_ObjectFlags |= OBJECT_FLAGS_PICKUP;
			//Data(0) |= DARKLINGSPAWN_FLAGS_CREATEWALLMARK;
		}
	}

	// Time to spawn darkling?
	if (m_TickSpawnDarkling > 0 && m_TickSpawnDarkling < m_pWServer->GetGameTick())
	{
		// Spawn darklingtype at this position (directly for now..)
		CMat4Dfp32 SpawnPlace = GetPositionMatrix();
		// Spawn 16 units up (?)
		//SpawnPlace.GetRow(3) += SpawnPlace.GetRow(2) * 16.0f;

//		CWObject_Message Msg;
//		Msg.m_pData = (void *)m_DarklingSpawnType.Str();
//		Msg.m_Msg = OBJMSG_CHAR_GETDARKLINGLEVEL;
		 
//		int Level = m_pWServer->Message_SendToObject(Msg, m_iPlayerInRange);
		CStr SpawnType;
		if(m_Data[5] & DARKLINGSPAWN_FLAGS2_KAMIKAZE)
			SpawnType = "ai_darkling_kamikaze";
		else if(m_Data[5] & DARKLINGSPAWN_FLAGS2_LIGHTKILLER)
			SpawnType = "ai_darkling_lightkiller";
		else if(m_Data[5] & DARKLINGSPAWN_FLAGS2_BERSERKER)
			SpawnType = "ai_darkling_berserker";
		else if(m_Data[5] & DARKLINGSPAWN_FLAGS2_GUNNER)
			SpawnType = "ai_darkling_gunner";
//		CStr SpawnTypeWithLevel = SpawnType + CStrF("_lvl%d", Level) ;
		M_TRACEALWAYS(CStrF("Spawning darkling type: %s\n", SpawnType.Str()));

		CWObject *pObj = m_pWServer->Object_Get(m_Data[4]);
		if(!pObj)
			return;
		CWObject_Character *pChar = safe_cast<CWObject_Character>(pObj);
		SpawnType = pChar->Char_GetSpawnDarkling(SpawnType);
		if(SpawnType.IsEmpty())
			return;

		m_iDarkling = m_pWServer->Object_Create(SpawnType, SpawnPlace);

		CStr DarklingName = m_DarklingSpawnName;
		if(!m_DarklingSpawnName.Len())
			DarklingName = SpawnType;

		m_pWServer->Object_SetName(m_iDarkling, DarklingName);

		// Send onspawn messages
		TAP<const CSimpleMessageDT> pMessages = m_lMsg_OnDarklingSpawn;
		for (uint i = 0; i < pMessages.Len(); i++)
		{
			if (pMessages[i].m_TypeMask & m_Data[5])
				pMessages[i].SendMessage(m_iObject, m_iDarkling, m_pWServer);
		}

		CWObject_Message OnSpawn(OBJMSG_DARKLINGSPAWN_SENDDARKLINGSPAWN);
		TAP_RCD<struct sPrefabIndices> lDarklingEffects = m_lDarklingSpawnEffects;
		for (int32 i = 0; i < lDarklingEffects.Len(); i++)
			m_pWServer->Message_SendToObject(OnSpawn,lDarklingEffects[i].m_iObject);

		CWObject_CharDarkling* pDarkling = (CWObject_CharDarkling*)m_pWServer->Object_Get(m_iDarkling);
		if (pDarkling)
		{
			// Parse the extra keys (after spawn for now....)
			if (m_spDarklingExtraKeys)
			{
				for (int32 i = 0; i < m_spDarklingExtraKeys->GetNumChildren(); i++)
				{
					const CRegistry* pKey = m_spDarklingExtraKeys->GetChild(i);
					uint32 KeyHash = pKey->GetThisNameHash();
					pDarkling->OnEvalKey(KeyHash,pKey);
				}
			}
			pDarkling->Char_PrepareSpawn();
		}

		m_TickSpawnDarkling = 0;

		m_bCanSpawn = false;
	}

	if(m_iDarkling)
	{	//Is darkling still alive?
		CWObject_CharDarkling* pDarkling = TDynamicCast<CWObject_CharDarkling>(m_pWServer->Object_Get(m_iDarkling));
		if(!pDarkling || CWObject_Character::Char_GetPhysType(pDarkling) == PLAYER_PHYS_DEAD)
		{
			m_iDarkling = 0;
			m_bCanSpawn = true;
		}
	}

	// Destroy wallmark ?
	if (m_TickDestroyWallmark == 0)
	{
		if (Data(0) & DARKLINGSPAWN_FLAGS_DESTROYWALLMARK)
			Data(0) &= ~DARKLINGSPAWN_FLAGS_DESTROYWALLMARK;
	}
	else if (m_TickDestroyWallmark <= m_pWServer->GetGameTick())
	{
		Data(0) |= (DARKLINGSPAWN_FLAGS_DESTROYWALLMARK | DARKLINGSPAWN_FLAGS_DISABLERENDER);
		m_TickDestroyWallmark = 0;
	}

	if (m_iPlayerInRange)
	{
		int iDarkling = m_pWServer->Selection_GetSingleTarget(m_DarklingSpawnName);
		if (m_bCanSpawn && !(m_Data[0] & DARKLINGSPAWN_FLAGS_DISABLED) && iDarkling <= 0)
		{
			bool bLampsOk = CheckPrefabLights();
			if (!(m_Data[0] & DARKLINGSPAWN_FLAGS_LAMPSMUSTBEDESTROYED))
			{
				bLampsOk = m_LastIntensity < m_LightThreshold;
			}
			// Check if we should be nearby or active
			if (bLampsOk)
			{
				if (!(m_ModeFlags & DARKLINGSPAWN_MODE_ACTIVE))
				{
					TAP_RCD<struct sPrefabIndices> lEffects = m_lDarklingSpawnEffects;
					if (m_ModeFlags & DARKLINGSPAWN_MODE_NEARBY)
					{
						m_ModeFlags &= ~DARKLINGSPAWN_MODE_NEARBY;
						// If in range if player send leaveactive and enter active messages
						// Send OnTurnActive messages
						TAP_RCD<CWO_SimpleMessage> lMessage = m_lMsg_OnTurnActive;
						for (int32 i = 0; i < lMessage.Len(); i++)
							lMessage[i].SendMessage(m_iObject, m_iObject, m_pWServer);

						// Skip leave nearby at the request of scripters
						/*lMessage = m_lMsg_OnLeaveNearby;
						for (int32 i = 0; i < lMessage.Len(); i++)
							lMessage[i].SendMessage(m_iObject, m_iObject, m_pWServer);*/
						
						// Disable "inactive" effects and enable active effects + turn active effects..
						CWObject_Message Msg(OBJMSG_IMPULSE,CWObject_DarklingSpawnEffect::DARKLINGSPAWNEFFECT_IMPULSE_DISABLENEARBY);
						CWObject_Message LeaveNearby(OBJMSG_DARKLINGSPAWN_SENDLEAVENEARBYSOUNDS);
						CWObject_Message TurnActive(OBJMSG_DARKLINGSPAWN_SENDTURNACTIVE);
						for (int32 i = 0; i < lEffects.Len(); i++)
						{
							m_pWServer->Phys_Message_SendToObject(Msg,lEffects[i].m_iObject);
							m_pWServer->Phys_Message_SendToObject(LeaveNearby,lEffects[i].m_iObject);
							m_pWServer->Phys_Message_SendToObject(TurnActive,lEffects[i].m_iObject);
						}
						Msg.m_Param0 = CWObject_DarklingSpawnEffect::DARKLINGSPAWNEFFECT_IMPULSE_ENABLETURNACTIVE;
						Msg.m_Param1 = m_iObject; // Link with this spawn point
						for (int32 i = 0; i < lEffects.Len(); i++)
							m_pWServer->Phys_Message_SendToObject(Msg,lEffects[i].m_iObject);

						// Play turnactive sound
						m_pWServer->Sound_At(GetPosition(),m_iSoundTurnActive,0);
					}
					TAP_RCD<CWO_SimpleMessage> lMessage = m_lMsg_OnEnterActive;
					for (int32 i = 0; i < lMessage.Len(); i++)
						lMessage[i].SendMessage(m_iObject, m_iObject, m_pWServer);
					CWObject_Message OnEnterActive(OBJMSG_DARKLINGSPAWN_SENDENTERACTIVE);
					for (int32 i = 0; i < lEffects.Len(); i++)
						m_pWServer->Message_SendToObject(OnEnterActive,lEffects[i].m_iObject);

					m_ModeFlags |= DARKLINGSPAWN_MODE_ACTIVE;
					UpdateSpawnEffects(true);
				}
			}
			else if (!(m_ModeFlags & DARKLINGSPAWN_MODE_NEARBY))
			{
				TAP_RCD<struct sPrefabIndices> lEffects = m_lDarklingSpawnEffects;
				if (m_ModeFlags & DARKLINGSPAWN_MODE_ACTIVE)
				{
					m_ModeFlags &= ~DARKLINGSPAWN_MODE_ACTIVE;
					// Send leave active messages, and turn off active effects
					TAP_RCD<CWO_SimpleMessage> lMessage = m_lMsg_OnLeaveActive;
					for (int32 i = 0; i < lMessage.Len(); i++)
						lMessage[i].SendMessage(m_iObject, m_iObject, m_pWServer);

					// Disable "inactive" effects and enable active effects + turn active effects..
					CWObject_Message Msg(OBJMSG_IMPULSE,CWObject_DarklingSpawnEffect::DARKLINGSPAWNEFFECT_IMPULSE_DISABLEACTIVE);
					CWObject_Message OnLeaveActive(OBJMSG_DARKLINGSPAWN_SENDLEAVEACTIVE);
					for (int32 i = 0; i < lEffects.Len(); i++)
					{
						m_pWServer->Phys_Message_SendToObject(OnLeaveActive,lEffects[i].m_iObject);
						m_pWServer->Phys_Message_SendToObject(Msg,lEffects[i].m_iObject);
					}
				}
				TAP_RCD<CWO_SimpleMessage> lMessage = m_lMsg_OnEnterNearby;
				for (int32 i = 0; i < lMessage.Len(); i++)
					lMessage[i].SendMessage(m_iObject, m_iObject, m_pWServer);
				
				CWObject_Message OnEnterNearby(OBJMSG_DARKLINGSPAWN_SENDENTERNEARBY);
				for (int32 i = 0; i < lEffects.Len(); i++)
					m_pWServer->Message_SendToObject(OnEnterNearby,lEffects[i].m_iObject);
				
				m_ModeFlags |= DARKLINGSPAWN_MODE_NEARBY;
				UpdateSpawnEffects(true);
				SetEnabled(true);
			}

			// Check if we should leave
			bool bLeave = true;
			CWObject_CoreData* pChar = m_pWServer->Object_GetCD(m_iPlayerInRange);
			if (pChar)
			{
				CVec3Dfp32 TestPos = pChar->GetPosition();
				TestPos.k[2] += 30.0f;
				fp32 DistSqr = (GetPosition() - TestPos).LengthSqr();
				if (DistSqr <= Sqr(m_UsableRadius))
				{
					bLeave = false;
					m_TickNoRefresh = m_TickNoRefresh ? Max(m_TickNoRefresh,(int32)m_pWServer->GetGameTick() + 1) : 0;
				}
			}
			if (bLeave)
			{
				TAP_RCD<struct sPrefabIndices> lDarklingEffects = m_lDarklingSpawnEffects;
				if (m_ModeFlags & DARKLINGSPAWN_MODE_ACTIVE)
				{
					for (int32 i = 0; i < m_lMsg_OnLeaveActive.Len(); i++)
						m_lMsg_OnLeaveActive[i].SendMessage(m_iObject, m_iObject, m_pWServer);
					CWObject_Message OnLeaveNearby(OBJMSG_DARKLINGSPAWN_SENDLEAVEACTIVE);
					for (int32 i = 0; i < lDarklingEffects.Len(); i++)
						m_pWServer->Message_SendToObject(OnLeaveNearby,lDarklingEffects[i].m_iObject);
				}
				else
				{
					for (int32 i = 0; i < m_lMsg_OnLeaveNearby.Len(); i++)
						m_lMsg_OnLeaveNearby[i].SendMessage(m_iObject, m_iObject, m_pWServer);
					CWObject_Message OnLeaveNearby(OBJMSG_DARKLINGSPAWN_SENDLEAVENEARBY);
					for (int32 i = 0; i < lDarklingEffects.Len(); i++)
						m_pWServer->Message_SendToObject(OnLeaveNearby,lDarklingEffects[i].m_iObject);
				}
				// Remove player
				m_iPlayerInRange = 0;
				// Turn everything off
				// Turn off all effects...
				UpdateSpawnEffects(false);
				m_ModeFlags &= ~(DARKLINGSPAWN_MODE_ACTIVE|DARKLINGSPAWN_MODE_NEARBY);
			}
		}
		else if (m_ModeFlags & (DARKLINGSPAWN_MODE_NEARBY | DARKLINGSPAWN_MODE_ACTIVE))
		{
			// Turn off all effects
			UpdateSpawnEffects(false);
			// Turn off all lights
			CWObject_Message MsgOff(OBJMSG_DARKLINGSPAWN_SENDDARKLIGHTS);
			TAP_RCD<struct sPrefabIndices> lEffects = m_lDarklingSpawnEffects;
			for (int32 i = 0; i < lEffects.Len(); i++)
				m_pWServer->Message_SendToObject(MsgOff,lEffects[i].m_iObject);
			m_ModeFlags &= ~(DARKLINGSPAWN_MODE_ACTIVE|DARKLINGSPAWN_MODE_NEARBY);
		}
	}

	// Do we want to disable refresh
	if (m_TickNoRefresh > 0 && m_TickNoRefresh <= m_pWServer->GetGameTick() && !m_iDarkling)
	{
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
		// If we're inactive, delete ourselves
		if (m_Data[0] & DARKLINGSPAWN_FLAGS_INACTIVE)
			m_pWServer->Object_Destroy(m_iObject);
	}
}

void CWObject_DarklingSpawn::UpdateSpawnEffects(bool _bOn)
{
	if (_bOn)
	{
		// Send flicker messages to linked lights
		CWObject_Message MsgSet(OBJMSG_LIGHT_IMPULSE,CWObject_Light::MSG_FLICKER,1);
		TAP_RCD<struct sPrefabIndices> liLinkedLamps(m_liLinkedLamps);
		for (int32 i = 0; i < liLinkedLamps.Len(); i++)
			m_pWServer->Message_SendToObject(MsgSet,liLinkedLamps[i].m_iObject);

		// Send "on" to related objects
		int16 SoundID;
		int32 EffectType;
		if (m_ModeFlags & DARKLINGSPAWN_MODE_NEARBY)
		{
			SoundID = m_iSoundNearby;
			EffectType = CWObject_DarklingSpawnEffect::DARKLINGSPAWNEFFECT_IMPULSE_ENABLENEARBY;
		}
		else
		{
			SoundID = m_iSoundActive;
			EffectType = CWObject_DarklingSpawnEffect::DARKLINGSPAWNEFFECT_IMPULSE_ENABLEACTIVE;
		}
		if (iSound(0) != SoundID)
			iSound(0) = SoundID;
		CWObject_Message Msg(OBJMSG_IMPULSE, EffectType, m_iObject);
		TAP_RCD<struct sPrefabIndices> lObjects = m_lDarklingSpawnEffects;
		for (int32 i = 0; i < lObjects.Len(); i++)
			m_pWServer->Phys_Message_SendToObject(Msg,lObjects[i].m_iObject);
	}
	else
	{
		// Turn off all effects...
		CWObject_Message MsgSet(OBJMSG_LIGHT_IMPULSE,CWObject_Light::MSG_FLICKER,0);
		TAP_RCD<struct sPrefabIndices> liLinkedLamps(m_liLinkedLamps);
		for (int32 i = 0; i < liLinkedLamps.Len(); i++)
			m_pWServer->Message_SendToObject(MsgSet,liLinkedLamps[i].m_iObject);

		// Send "off" to related objects
		CWObject_Message Msg(OBJMSG_IMPULSE,CWObject_DarklingSpawnEffect::DARKLINGSPAWNEFFECT_IMPULSE_DISABLEACTIVE);
		TAP_RCD<struct sPrefabIndices> lObjects = m_lDarklingSpawnEffects;
		for (int32 i = 0; i < lObjects.Len(); i++)
			m_pWServer->Phys_Message_SendToObject(Msg,lObjects[i].m_iObject);
		Msg.m_Param0 = CWObject_DarklingSpawnEffect::DARKLINGSPAWNEFFECT_IMPULSE_DISABLENEARBY;
			for (int32 i = 0; i < lObjects.Len(); i++)
				m_pWServer->Phys_Message_SendToObject(Msg,lObjects[i].m_iObject);

		// Disable sounds
		if (iSound(0) != 0)
			iSound(0) = 0;
	}
}

fp32 CWObject_DarklingSpawn::MeasureLightIntensity(bool _bForce)
{
	// Create lightmeter if it doesn't exist
	if(!m_spLightMeter)
	{
		m_spLightMeter = MNew(CWObj_ObjLightMeter);
		
		if (!m_spLightMeter)
		{
			ConOutL("§cf00CWObject_DarklingSpawn: Failed to create DarklingSpawn point light meter!!");
			return 0.0f;
		}

		// Initalize light meter measure
		m_spLightMeter->Init(m_pWServer, 2.0f);
	}

	// Force light meter measure?
	if(_bForce)
		m_spLightMeter->MakeMeasure(m_pWServer->GetGameTick());

	// Create box and measure positions
	CVec3Dfp32 BoxPos = GetPosition();
	CVec3Dfp32 MeasurePos = BoxPos + (GetPositionMatrix().GetRow(2) * 6.0f);

	// Measure point and return
	m_spLightMeter->Measure(m_pWServer, (Data(0) & DARKLINGSPAWN_FLAGS_USELIGHTMAPS) != 0, BoxPos, MeasurePos, 8.0f, 8.0f, 10, true, m_iObject);
	return m_spLightMeter->GetIntensity(0);
}

void CWObject_DarklingSpawn::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	// Only 1 type for now...
	const CStr& KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH3('USAB','LERA','DIUS'):	// "USABLERADIUS"
		{
			m_UsableRadius = _pKey->GetThisValuef();
			return;
		}
	case MHASH3('SOUN','D_AC','TIVE'): // "SOUND_ACTIVE"
		{
			m_iSoundActive = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			return;
		}
	case MHASH4('SOUN','D_TU','RNAC','TIVE'): // "SOUND_TURNACTIVE"
		{
			m_iSoundTurnActive = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			return;
		}
	case MHASH3('SOUN','D_NE','ARBY'): // "SOUND_NEARBY"
		{
			m_iSoundNearby = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			return;
		}
	case MHASH5('DARK','LING','SPAW','NNAM','E'): // "DARKINGSPAWNNAME"
		{
			m_DarklingSpawnName = KeyValue.LowerCase();;
			return;
		}
	case MHASH5('DARK','LING','SPAW','NTYP','E'): // "DARKINGSPAWNTYPE"
		{
			// Make sure this matches the DARKLING_TYPE_XXX flags
			// We do not allow more than one default spawn type (they are flags for other reasons... :P)
			static const char * TranslateSpawnType[] =
			{
				"ai_darkling_berserker", "ai_darkling_lightkiller", "ai_darkling_kamikaze", "ai_darkling_gunner", NULL
			};
			// Default to berserker if no match
			int DefaultSpawnType =  Max(0, KeyValue.LowerCase().TranslateInt(TranslateSpawnType));
			m_DarklingSpawnTypeDefault = M_BitD(DefaultSpawnType);
			// m_DarklingSpawnType = KeyValue.LowerCase();
			return;
		}
	default:
		break;
	};
	if (KeyName.Find("DARKLINGSPAWNFLAGS") != -1 || KeyName.Find("FLAGS") != -1)
	{
		static const char *FlagsTranslate[] =
		{
			"disabled","disableonuse","disablerender","waitspawned","createwallmark","destroywallmark","gotcharacterinfo","shouldbeenabled","alwaysenabled","inactive","uselightmaps","lampsmustbedestroyed",NULL
		};

		Data(0) = Data(0) | KeyValue.TranslateFlags(FlagsTranslate);
	}
	else if (KeyName.Find("LIGHTTHRESHOLD") != -1)
	{
		m_LightThreshold = (fp32)KeyValue.Val_fp64();
	}
	else if (KeyName.Find("LINKEDLIGHTS") != -1)
	{
		m_lSpawnStrings[DARKLINGSPAWN_SPAWNSTRING_SLOT_LINKEDLAMPS].Add(KeyValue);
	}
	else if (KeyName.Find("DARKLINGEFFECTS") != -1)
	{
		m_lSpawnStrings[DARKLINGSPAWN_SPAWNSTRING_SLOT_DARKLINGEFFECTS].Add(KeyValue);
	}
	else if (KeyName.Find("PREFABOBJECTS") != -1)
	{
		m_lSpawnStrings[DARKLINGSPAWN_SPAWNSTRING_SLOT_PREFABOBJECTS].Add(KeyValue);
	}
	else if (KeyName.Find("WALLMARKSIZE") != -1)
	{
		Data(0) = (Data(0) & ~(DARKLINGSPAWN_WALLMARK_SIZEMASK << 20)) | ((KeyValue.Val_int() & DARKLINGSPAWN_WALLMARK_SIZEMASK) << 20);
	}
	else if (KeyName.Find("WALLMARKTOLERANCE") != -1)
	{
		Data(0) = (Data(0) & ~(DARKLINGSPAWN_WALLMARK_TOLERANCEMASK << 12)) | ((KeyValue.Val_int() & DARKLINGSPAWN_WALLMARK_TOLERANCEMASK) << 12);
	}
	else if (KeyName.Find("WALLMARKSURFACE") != -1)
	{
		m_pWServer->GetMapData()->GetResourceIndex_Surface(KeyValue);
	}
	else if (KeyName.Find("SPAWNMODELLIGHT") != -1)
	{
		iModel(0) = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
	}
	else if (KeyName.Find("SPAWNMODELSHADOWS") != -1)
	{
		iModel(1) = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
	}
	else if (KeyName.Find("SPAWNMODELEXTRA") != -1)
	{
		iModel(2) = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
	}
	else if (KeyName.Find("SPAWNEFFECT") != -1)
	{
		m_SpawnEffect = KeyValue;
		int iTemplate = m_pWServer->GetMapData()->GetResourceIndex_Template(KeyValue);
	}
	else if (KeyName.Find("MSG_ONSPAWN") != -1)
	{
		int iSlot = atoi(KeyName.Str() + 11);
		m_lMsg_OnDarklingSpawn.SetMinLen(iSlot + 1);
		m_lMsg_OnDarklingSpawn[iSlot].Parse(KeyValue, m_pWServer);
	}
	else if (KeyName.Find("MSG_ONENTERNEARBY") != -1)
	{
		//
		m_lMsg_OnEnterNearby.Add(CWO_SimpleMessage());
		m_lMsg_OnEnterNearby[m_lMsg_OnEnterNearby.Len()-1].Parse(KeyValue,m_pWServer);
	}
	else if (KeyName.Find("MSG_ONLEAVENEARBY") != -1)
	{
		//
		m_lMsg_OnLeaveNearby.Add(CWO_SimpleMessage());
		m_lMsg_OnLeaveNearby[m_lMsg_OnLeaveNearby.Len()-1].Parse(KeyValue,m_pWServer);
	}
	else if (KeyName.Find("MSG_ONENTERACTIVE") != -1)
	{
		//
		m_lMsg_OnEnterActive.Add(CWO_SimpleMessage());
		m_lMsg_OnEnterActive[m_lMsg_OnEnterActive.Len()-1].Parse(KeyValue,m_pWServer);
	}
	else if (KeyName.Find("MSG_ONLEAVEACTIVE") != -1)
	{
		//
		m_lMsg_OnLeaveActive.Add(CWO_SimpleMessage());
		m_lMsg_OnLeaveActive[m_lMsg_OnLeaveActive.Len()-1].Parse(KeyValue,m_pWServer);
	}
	else if (KeyName.Find("MSG_ONTURNACTIVE") != -1)
	{
		//
		m_lMsg_OnTurnActive.Add(CWO_SimpleMessage());
		m_lMsg_OnTurnActive[m_lMsg_OnTurnActive.Len()-1].Parse(KeyValue,m_pWServer);
	}
	else if (KeyName.Find("DARKLINGEXTRAKEYS") != -1)
	{
		if (!m_spDarklingExtraKeys)
			m_spDarklingExtraKeys = REGISTRY_CREATE;
		if (m_spDarklingExtraKeys)
		{
			CStr SubKeyName = KeyValue.GetStrSep(",");
			m_spDarklingExtraKeys->AddKey(SubKeyName,KeyValue);
		}
	}
	else
		CWObject_DarklingSpawn_Parent::OnEvalKey(_KeyHash, _pKey);
}

void CWObject_DarklingSpawn::PrecacheDarkling(const CStr& _Darkling)
{
	// Create/destroy darkling (to precache)
	if (_Darkling.Len())
	{
		// Check if this darkling has already been spawned
		int32 NameHash = StringToHash(_Darkling.LowerCase());
		TAP_RCD<int32> lSpawnedDarklings = m_slSpawnedDarklings;
		for (int32 i = 0; i < lSpawnedDarklings.Len(); i++)
		{
			if (lSpawnedDarklings[i] == NameHash)
				return;
		}
		m_slSpawnedDarklings.Add(NameHash);
		CMat4Dfp32 Mat;
		Mat.Unit();
		Mat.k[3][2] = -100000;
		int iObj = m_pWServer->Object_Create(_Darkling,Mat);
		CWObject* pObj = m_pWServer->Object_Get(iObj);
		if (pObj)
		{
			CWObject_Character* pChar = safe_cast<CWObject_Character>(pObj);
			if (pChar)
			{	// Clear the list of teamnames
				pChar->m_lTeamFromKeys.Clear();
			}
			pObj->OnSpawnWorld();
			pObj->OnSpawnWorld2();
		}
		m_pWServer->Object_DestroyInstant(iObj);
	}
}

void CWObject_DarklingSpawn::OnFinishEvalKeys()
{
	CWObject_DarklingSpawn_Parent::OnFinishEvalKeys();
	
	// Waitspawned object, disable refresh
	if (m_Data[0] & DARKLINGSPAWN_FLAGS_WAITSPAWNED)
	{
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
	}
	else
	{
		Data(0) |= DARKLINGSPAWN_FLAGS_SHOULDBEENABLED;
	}
	// Set default disabled
	SetEnabled(false);
	Data(0) |= DARKLINGSPAWN_FLAGS_DISABLED;

	// Add pickup flag so "findstuff" can find it
	if (!(m_Data[0] & DARKLINGSPAWN_FLAGS_WAITSPAWNED))
	{
//		m_PhysState.m_ObjectFlags |= OBJECT_FLAGS_PICKUP;
		m_pWServer->Object_SetPhysics_ObjectFlags(m_iObject, GetPhysState().m_ObjectFlags | OBJECT_FLAGS_PICKUP);
	}
	
	// Special precache.
	// For all 4 darklings
	PrecacheDarkling("ai_darkling_berserker");
	PrecacheDarkling("ai_darkling_gunner");
	PrecacheDarkling("ai_darkling_lightkiller");
	PrecacheDarkling("ai_darkling_kamikaze");

	// Make sure some effects exist if none has been specified. This is not valid in RTM!!
	{
		if (!m_SpawnEffect.Len())
		{
			m_SpawnEffect = "DarknessEffect_DarklingSpawn";
			int iTemplate = m_pWServer->GetMapData()->GetResourceIndex_Template("DarknessEffect_DarklingSpawn");
			//ConOutL("§cf00CWObject_DarklingSpawn: Invalid spawnpoint! No spawneffect specified. (Handled outside RTM only!)");
		}

		/*if (!iModel(0))
		{
			iModel(0) = m_pWServer->GetMapData()->GetResourceIndex_Model("EffectSystem:FXEffect,fx_spawnpoint_light");
			ConOutL("§cf00CWObject_DarklingSpawn: Invalid spawnpoint! No model set for light areas. (Handled outside RTM only!)");
		}

		if (!iModel(1))
		{
			iModel(1) = m_pWServer->GetMapData()->GetResourceIndex_Model("EffectSystem:FXEffect,fx_spawnpoint_shadow");
			ConOutL("§cf00CWObject_DarklingSpawn: Invalid spawnpoint! No model set for dark areas. (Handled outside RTM only!)");
		}*/
	}
	
	// Set models
	CWObject_Model::Model_Set(0, iModel(0), false);
	CWObject_Model::Model_Set(1, iModel(1), false);
	CWObject_Model::Model_Set(2, iModel(2), false);
}

void CWObject_DarklingSpawn::OnSpawnWorld()
{
	CWObject_DarklingSpawn_Parent::OnSpawnWorld();
	for (int i = 0; i < m_lMsg_OnDarklingSpawn.Len(); i++)
		m_lMsg_OnDarklingSpawn[i].SendPrecache(m_iObject, m_pWServer);
	for (int i = 0; i < m_lMsg_OnEnterNearby.Len(); i++)
		m_lMsg_OnEnterNearby[i].SendPrecache(m_iObject, m_pWServer);
	for (int i = 0; i < m_lMsg_OnLeaveNearby.Len(); i++)
		m_lMsg_OnLeaveNearby[i].SendPrecache(m_iObject, m_pWServer);
	for (int i = 0; i < m_lMsg_OnEnterActive.Len(); i++)
		m_lMsg_OnEnterActive[i].SendPrecache(m_iObject, m_pWServer);
	for (int i = 0; i < m_lMsg_OnLeaveActive.Len(); i++)
		m_lMsg_OnLeaveActive[i].SendPrecache(m_iObject, m_pWServer);
	for (int i = 0; i < m_lMsg_OnTurnActive.Len(); i++)
		m_lMsg_OnTurnActive[i].SendPrecache(m_iObject, m_pWServer);
	
	// Clear spawned darkling list
	m_slSpawnedDarklings.Clear();

	// Find the linked lights
	TAP_RCD<CStr> lObjects(m_lSpawnStrings[DARKLINGSPAWN_SPAWNSTRING_SLOT_LINKEDLAMPS]);
	for (int i = 0; i < lObjects.Len(); i++)
	{
		int iLamp = m_pWServer->Selection_GetSingleTarget(lObjects[i]);
		if (iLamp > 0)
		{
			struct sPrefabIndices Prefab;
			Prefab.m_iObject = iLamp;
			Prefab.m_iPrefab = -1;
			m_liLinkedLamps.Add(Prefab);
		}
	}
	lObjects = m_lSpawnStrings[DARKLINGSPAWN_SPAWNSTRING_SLOT_DARKLINGEFFECTS];
	for (int i = 0; i < lObjects.Len(); i++)
	{
		int iEffect = m_pWServer->Selection_GetSingleTarget(lObjects[i]);
		if (iEffect > 0)
		{
			struct sPrefabIndices Prefab;
			Prefab.m_iObject = iEffect;
			Prefab.m_iPrefab = -1;
			m_lDarklingSpawnEffects.Add(Prefab);
		}
	}
	lObjects = m_lSpawnStrings[DARKLINGSPAWN_SPAWNSTRING_SLOT_PREFABOBJECTS];
	// Add all lamps and darklingspawn effects
	if (lObjects.Len())
	{
		TSelection<CSelection::LARGE_BUFFER> Selection;
		int32 iClassLamp = m_pWServer->GetMapData()->GetResourceIndex_Class("Object_Lamp");
		int32 iClassSpawnEffect = m_pWServer->GetMapData()->GetResourceIndex_Class("DarklingSpawnEffect");
		m_pWServer->Selection_AddClass(Selection,iClassLamp);
		m_pWServer->Selection_AddClass(Selection,iClassSpawnEffect);
		const int16* pSel;
		int nSel = m_pWServer->Selection_Get(Selection,&pSel);
		for (int i = 0; i < lObjects.Len(); i++)
		{
			// Okiedokie then, find all objects that have the given name as a prefix
			CStr SearchStr = lObjects[i];
			SearchStr.MakeLowerCase();
			int32 SearchStrLen = SearchStr.Len();
			for (int iSel = 0; iSel < nSel; iSel++)
			{
				CWObject* pObj = m_pWServer->Object_Get(pSel[iSel]);
				CStr ObjName = pObj->GetName();
				ObjName.MakeLowerCase();
				if (ObjName.Len() >= SearchStrLen && !ObjName.CompareSubStr(SearchStr))
				{
					// WOOT found a match, add it to lamp or spawneffect arrays
					struct sPrefabIndices Prefab;
					Prefab.m_iObject = pSel[iSel];
					Prefab.m_iPrefab = i;
					if (iClassLamp == pObj->m_iClass)
					{
						int iInsertPos = 0;
						for (iInsertPos = 0; iInsertPos < m_liLinkedLamps.Len(); iInsertPos++)
						{
							if (m_liLinkedLamps[iInsertPos].m_iPrefab > Prefab.m_iPrefab)
								break;
						}

						m_liLinkedLamps.Insert(iInsertPos,Prefab);
					}
					else if (iClassSpawnEffect == pObj->m_iClass)
					{
						int iInsertPos = 0;
						for (iInsertPos = 0; iInsertPos < m_lDarklingSpawnEffects.Len(); iInsertPos++)
						{
							if (m_lDarklingSpawnEffects[iInsertPos].m_iPrefab > Prefab.m_iPrefab)
								break;
						}
						m_lDarklingSpawnEffects.Insert(iInsertPos, Prefab);
					}
				}
			}
		}
	}

	// Touch spawn effects
	CWObject_Message Msg(OBJMSG_DARKLINGSPAWN_TOUCHEFFECT);
	for (int32 i = 0; i < m_lDarklingSpawnEffects.Len(); i++)
	{
		m_pWServer->Message_SendToObject(Msg,m_lDarklingSpawnEffects[i].m_iObject);
	}

	// Find prefab objects and link the types in there appropriately
	// Clear spawn info
	m_lSpawnStrings.Clear();
}

void CWObject_DarklingSpawn::OnSpawnWorld2()
{
	CWObject_DarklingSpawn_Parent::OnSpawnWorld2();
}

aint CWObject_DarklingSpawn::OnMessage(const CWObject_Message &_Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_CANUSE:
	{
		if (!(m_Data[0] & DARKLINGSPAWN_FLAGS_DISABLED) && m_bCanSpawn)
		{
			// Make sure we don't already have a darkling spawned of this type.
//			int iDarkling = m_pWServer->Selection_GetSingleTarget(m_DarklingSpawnName);
//			if (iDarkling > 0 || m_TickSpawnDarkling != 0)
//				return 0;

			int return_value = DARKLINGSPAWN_FLAGS2_BERSERKER | DARKLINGSPAWN_FLAGS2_GUNNER | DARKLINGSPAWN_FLAGS2_LIGHTKILLER | DARKLINGSPAWN_FLAGS2_KAMIKAZE;
			
			TSelection<CSelection::LARGE_BUFFER> Selection;
			m_pWServer->Selection_AddClass(Selection, "CharDarkling");
			const int16* pRet;
			int nRet = m_pWServer->Selection_Get(Selection, &pRet);
			for(int i = 0; i < nRet; i++)
			{
				CWObject *pObj = m_pWServer->Object_Get(pRet[i]);
				if(pObj)
				{
					CStr TemplateName = pObj->GetTemplateName();
					if(!TemplateName.Compare("ai_darkling_berserker"))
						return_value &= ~DARKLINGSPAWN_FLAGS2_BERSERKER;
					else if(!TemplateName.Compare("ai_darkling_gunner"))
						return_value &= ~DARKLINGSPAWN_FLAGS2_GUNNER;
					else if(!TemplateName.Compare("ai_darkling_lightkiller"))
						return_value &= ~DARKLINGSPAWN_FLAGS2_LIGHTKILLER;
					else if(!TemplateName.Compare("ai_darkling_kamikaze"))
						return_value &= ~DARKLINGSPAWN_FLAGS2_KAMIKAZE;
				}
			}

			CWObject_Message Msg(OBJMSG_CHAR_DARKLINGAVAILABLE);
			Msg.m_pData = (void*) "ai_darkling_berserker";
			if(!m_pWServer->Message_SendToObject(Msg, _Msg.m_Param0))
				return_value &= ~DARKLINGSPAWN_FLAGS2_BERSERKER;

			Msg.m_pData = (void*) "ai_darkling_gunner";
			if(!m_pWServer->Message_SendToObject(Msg, _Msg.m_Param0))
				return_value &= ~DARKLINGSPAWN_FLAGS2_GUNNER;

			Msg.m_pData = (void*) "ai_darkling_lightkiller";
			if(!m_pWServer->Message_SendToObject(Msg, _Msg.m_Param0))
				return_value &= ~DARKLINGSPAWN_FLAGS2_LIGHTKILLER;

			Msg.m_pData = (void*) "ai_darkling_kamikaze";
			if(!m_pWServer->Message_SendToObject(Msg, _Msg.m_Param0))
				return_value &= ~DARKLINGSPAWN_FLAGS2_KAMIKAZE;

			m_Data[4] = _Msg.m_Param0;


/*			CMat4Dfp32 Mat = GetPositionMatrix();
			CVec3Dfp32 BerserkPos, GunnerPos, LightKillerPos, KamikazePos, Right, Forward;
			GetSpawnEffectDirections(m_pWServer, this, _Msg.m_Param0, Forward, Right);

			if(return_value & DARKLINGSPAWN_FLAGS2_BERSERKER)
				BerserkPos = Mat.GetRow(3) + (Forward * DARKLINGSPAWN_CENTER_TO_CHOICE_OFFSET);
			if(return_value & DARKLINGSPAWN_FLAGS2_KAMIKAZE)
				KamikazePos = Mat.GetRow(3) - Forward * DARKLINGSPAWN_CENTER_TO_CHOICE_OFFSET;
			if(return_value & DARKLINGSPAWN_FLAGS2_LIGHTKILLER)
				LightKillerPos = Mat.GetRow(3) - Right * DARKLINGSPAWN_CENTER_TO_CHOICE_OFFSET;
			if(return_value & DARKLINGSPAWN_FLAGS2_GUNNER)
				GunnerPos = Mat.GetRow(3) + Right * DARKLINGSPAWN_CENTER_TO_CHOICE_OFFSET;;

			CWObject_Character *pChar = safe_cast<CWObject_Character>(m_pWServer->Object_Get(_Msg.m_Param0));
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(pChar);
			CMat4Dfp32 Pos;
			pChar->GetCameraAimMatrix(pChar, m_pWServer, &Pos);
			CVec3Dfp32 CharToSpawn = GetPosition() - Pos.GetRow(3);
			CharToSpawn.Normalize();
			fp32 d = CharToSpawn * Pos.GetRow(0);
			if(d > 0.98f)
			{	//We are looking directly at it, kinda
				d = -1.0f;
				int32 iChoice = 0;
				if(return_value & DARKLINGSPAWN_FLAGS2_BERSERKER)
				{
					CVec3Dfp32 CharToBerserker = BerserkPos - Pos.GetRow(3);
					CharToBerserker.Normalize();
					d = CharToBerserker * Pos.GetRow(0);
					iChoice = DARKLINGSPAWN_FLAGS2_BERSERKER;
					Mat.GetRow(3) = BerserkPos;
				}
				if(return_value & DARKLINGSPAWN_FLAGS2_GUNNER)
				{
					CVec3Dfp32 CharToGunner = GunnerPos - Pos.GetRow(3);
					CharToGunner.Normalize();
					fp32 d2 = CharToGunner * Pos.GetRow(0);
					if(d2 > d)
					{
						d = d2;
						iChoice = DARKLINGSPAWN_FLAGS2_GUNNER;
						Mat.GetRow(3) = GunnerPos;
					}
				}
				if(return_value & DARKLINGSPAWN_FLAGS2_LIGHTKILLER)
				{
					CVec3Dfp32 CharToLightKiller = LightKillerPos - Pos.GetRow(3);
					CharToLightKiller.Normalize();
					fp32 d2 = CharToLightKiller * Pos.GetRow(0);
					if(d2 > d)
					{
						d = d2;
						iChoice = DARKLINGSPAWN_FLAGS2_LIGHTKILLER;
						Mat.GetRow(3) = LightKillerPos;
					}
				}
				if(return_value & DARKLINGSPAWN_FLAGS2_KAMIKAZE)
				{
					CVec3Dfp32 CharToKamikaze = KamikazePos - Pos.GetRow(3);
					CharToKamikaze.Normalize();
					fp32 d2 = CharToKamikaze * Pos.GetRow(0);
					if(d2 > d)
					{
						d = d2;
						iChoice = DARKLINGSPAWN_FLAGS2_KAMIKAZE;
						Mat.GetRow(3) = KamikazePos;
					}
				}
				if(iChoice)
				{
					m_Data[5] &= ~return_value;
					m_Data[5] |= iChoice;
				}
			}
			else
			{	//Not looking on it, use last control movement
				if(M_Fabs(pCD->m_Control_DeltaLook.k[1]) > M_Fabs(pCD->m_Control_DeltaLook.k[2]))
				{ 
					if(M_Fabs(pCD->m_Control_DeltaLook.k[1]) > 0.003f) //a bit of deadzone
					{
						m_Data[5] &= ~return_value;
						if(pCD->m_Control_DeltaLook.k[1] > 0.0f)
							m_Data[5] |= DARKLINGSPAWN_FLAGS2_BERSERKER;
						else
							m_Data[5] |= DARKLINGSPAWN_FLAGS2_KAMIKAZE;
					}
				}
				else
				{
					if(M_Fabs(pCD->m_Control_DeltaLook.k[2]) > 0.003f) //a bit of deadzone
					{
						m_Data[5] &= ~return_value;
						if(pCD->m_Control_DeltaLook.k[2] > 0.0f)
							m_Data[5] |= DARKLINGSPAWN_FLAGS2_LIGHTKILLER;
						else
							m_Data[5] |= DARKLINGSPAWN_FLAGS2_GUNNER;
					}
				}
			}*/

			bool bLampsOk = true;
			if (m_Data[0] & DARKLINGSPAWN_FLAGS_LAMPSMUSTBEDESTROYED)
			{
				// Check so that all linked lights are destroyed
				bLampsOk = true;
				CWObject_Message Msg(OBJMSG_LIGHT_GETSTATUS);
				TAP_RCD<struct sPrefabIndices> lLinkedLamps = m_liLinkedLamps;
				for (int32 i = 0; i < lLinkedLamps.Len(); i++)
				{
					if (m_pWServer->Message_SendToObject(Msg,lLinkedLamps[i].m_iObject))
					{
						bLampsOk = false;
						break;
					}
				}
			}
			else if (m_spLightMeter)
			{
				// Fetch light intensity and make sure it's low enough
				const fp32 LightIntensity = m_spLightMeter->GetIntensity(0);
				const fp32 LightmapIntensity = m_spLightMeter->GetLightmapIntensity();
				bLampsOk = (LightIntensity < m_LightThreshold);
			}

			if(!bLampsOk)
			{
				return 0;
			}

			m_Data[6] = m_pWServer->GetGameTick();

			return return_value;
		}
		return 0;
	}

	//OBJMSG_CANUSE must be called before since it checks what darkling to spawn
	case OBJMSG_USE:
	{
		if (!(m_Data[0] & DARKLINGSPAWN_FLAGS_DISABLED) && m_bCanSpawn)
		{
			m_Data[5] = _Msg.m_Param0;
			// Make sure we don't already have a darkling spawned of this type.
			int iDarkling = m_pWServer->Selection_GetSingleTarget(m_DarklingSpawnName);
			if (iDarkling > 0 || m_TickSpawnDarkling != 0)
				return 0;

			bool bLampsOk;
			if (m_Data[0] & DARKLINGSPAWN_FLAGS_LAMPSMUSTBEDESTROYED)
			{
				// Check so that all linked lights are destroyed
				bLampsOk = true;
				CWObject_Message Msg(OBJMSG_LIGHT_GETSTATUS);
				TAP_RCD<struct sPrefabIndices> lLinkedLamps = m_liLinkedLamps;
				for (int32 i = 0; i < lLinkedLamps.Len(); i++)
				{
					if (m_pWServer->Message_SendToObject(Msg,lLinkedLamps[i].m_iObject))
					{
						bLampsOk = false;
						break;
					}
				}
			}
			else
			{
				// Fetch light intensity and make sure it's low enough
				const fp32 LightIntensity = m_spLightMeter->GetIntensity(0);
				const fp32 LightmapIntensity = m_spLightMeter->GetLightmapIntensity();
				bLampsOk = (LightIntensity < m_LightThreshold);
			}

			if (bLampsOk)
			{
				// Go ahead and spawn (next tick)
				SyncDarklingSpawn(_Msg.m_Param0);
				return 1;
			}
		}
		return 0;
	}
	case OBJMSG_IMPULSE:
	{
		if (_Msg.m_Param0 == -1)
		{
			// Spawn default darkling next tick, regardless of normal restrictions
			m_Data[4] = _Msg.m_iSender;
			m_Data[5] = m_DarklingSpawnTypeDefault;
			SyncDarklingSpawn(m_DarklingSpawnTypeDefault);
		}
		else if (_Msg.m_Param0 == 0)
		{
			Data(0) &= ~DARKLINGSPAWN_FLAGS_SHOULDBEENABLED;
			SetEnabled(false);
		}
		else if (_Msg.m_Param0 == 1)
		{
			Data(0) |= DARKLINGSPAWN_FLAGS_SHOULDBEENABLED;
			SetEnabled(true);
		}
		else if (_Msg.m_Param0 == 2)
		{
			// Destroy ourselves
			m_pWServer->Object_Destroy(m_iObject);
		}
		else if (_Msg.m_Param0 == 3)
		{
			// Force light measure update now!
			MeasureLightIntensity(true);
		}
		return 1;
	}
	case OBJMSG_GAME_SPAWN:
	{
		if (!_Msg.m_Param0 && (m_Data[0] & DARKLINGSPAWN_FLAGS_WAITSPAWNED))
		{
			Data(0) |= DARKLINGSPAWN_FLAGS_SHOULDBEENABLED;
			SetEnabled(true);
		}
		else if (_Msg.m_Param0 > 0)
		{
			Data(0) &= ~DARKLINGSPAWN_FLAGS_SHOULDBEENABLED;
			SetEnabled(false);
		}

		if (!(m_Data[0] & DARKLINGSPAWN_FLAGS_DISABLED))
		{
			int iDarkling = m_pWServer->Selection_GetSingleTarget(m_DarklingSpawnName);
			if (iDarkling > 0 || m_TickSpawnDarkling != 0)// && !(m_Data[0] & DARKLINGSPAWN_FLAGS_DISABLERENDER))
			{
				Data(0) |= DARKLINGSPAWN_FLAGS_DISABLERENDER;
//				m_PhysState.m_ObjectFlags &= ~OBJECT_FLAGS_PICKUP;
				m_pWServer->Object_SetPhysics_ObjectFlags(m_iObject, GetPhysState().m_ObjectFlags & ~OBJECT_FLAGS_PICKUP);
			}
			else if (iDarkling <= 0)// && (m_Data[0] & DARKLINGSPAWN_FLAGS_DISABLERENDER))
			{
				Data(0) &= ~DARKLINGSPAWN_FLAGS_DISABLERENDER;
//				m_PhysState.m_ObjectFlags |= OBJECT_FLAGS_PICKUP;
				m_pWServer->Object_SetPhysics_ObjectFlags(m_iObject, GetPhysState().m_ObjectFlags | OBJECT_FLAGS_PICKUP);
			}
		}
		return 1;
	}
	case OBJMSG_DARKLINGSPAWN_DOSPAWNCHECK:
	{
		// Check if we can spawn, and spawn if we should
		if (_Msg.m_pData)
		{
			// Check towards given type
//			CStr SpawnType = CStr((char*)_Msg.m_pData);
//			SpawnType = SpawnType.GetStrSep("_lvl");
//			if (m_DarklingSpawnType == SpawnType)

			//If the player has any darklings available he can use the spawn point

			{
				if (!(m_Data[0] & DARKLINGSPAWN_FLAGS_GOTCHARACTERINFO))
					Data(0) |= DARKLINGSPAWN_FLAGS_GOTCHARACTERINFO;
				m_bCanSpawn = true;
			}
		}
		else
		{
			// Check all stuff
			CheckCanPlayerSpawn(_Msg.m_iSender);
		}
		if (m_bCanSpawn && (m_Data[0] & DARKLINGSPAWN_FLAGS_SHOULDBEENABLED))
			SetEnabled(true);
		
		return m_bCanSpawn;
	}
	case OBJMSG_DARKLINGSPAWN_ISCLOSESPAWN:
		{
			// Do some effect thingy?
			fp32 DistSqr = (GetPosition() - _Msg.m_VecParam0).LengthSqr();
			if (DistSqr <= Sqr(m_UsableRadius))
			{
				if (m_bCanSpawn)
				{
					if (!m_iPlayerInRange)
					{
						//TAP_RCD<struct sPrefabIndices> lDarklingEffects = m_lDarklingSpawnEffects;
						/*if (m_Data[0] & DARKLINGSPAWN_FLAGS_DISABLED)
						{
							for (int32 i = 0; i < m_lMsg_OnEnterNearby.Len(); i++)
								m_lMsg_OnEnterNearby[i].SendMessage(m_iObject, m_iObject, m_pWServer);

							CWObject_Message OnNearby(OBJMSG_DARKLINGSPAWN_SENDENTERNEARBY);
							for (int32 i = 0; i < lDarklingEffects.Len(); i++)
								m_pWServer->Message_SendToObject(OnNearby,lDarklingEffects[i]);
						}
						else
						{
							for (int32 i = 0; i < m_lMsg_OnEnterActive.Len(); i++)
								m_lMsg_OnEnterActive[i].SendMessage(m_iObject, m_iObject, m_pWServer);
							CWObject_Message OnEnterActive(OBJMSG_DARKLINGSPAWN_SENDENTERACTIVE);
							for (int32 i = 0; i < lDarklingEffects.Len(); i++)
								m_pWServer->Message_SendToObject(OnEnterActive,lDarklingEffects[i]);
						}*/
						m_iPlayerInRange = _Msg.m_iSender;
					}
					if (ClientFlags() & CWO_CLIENTFLAGS_NOREFRESH)
					{
						ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
						int32 GameTick = m_pWServer->GetGameTick();
						m_TickNoRefresh = m_TickNoRefresh ? Max(m_TickNoRefresh,GameTick + 2) : GameTick + 2;
					}
					// Update spawn effects
					//UpdateCanSpawnEffects(true);
					int iDarkling = m_pWServer->Selection_GetSingleTarget(m_DarklingSpawnName);
					if (iDarkling > 0 || m_TickSpawnDarkling != 0)
						return 0;
				}
				return m_bCanSpawn;
			}
			return 0;
		}
	default:
		return CWObject_DarklingSpawn_Parent::OnMessage(_Msg);
	}
}

void CWObject_DarklingSpawn::GetSpawnEffectDirections(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int _iObj, CVec3Dfp32 &_Forward, CVec3Dfp32 &_Right)
{
	CWObject_CoreData *pChar = _pWPhys->Object_GetCD(_iObj);
	CMat4Dfp32 Pos;
	CWObject_Character::GetCameraAimMatrix(pChar, _pWPhys, &Pos);
	CVec3Dfp32 CharToSpawn = _pObj->GetPosition() - Pos.GetRow(3);
	CharToSpawn.Normalize();

	CMat4Dfp32 Mat = _pObj->GetPositionMatrix();
	CVec3Dfp32 BerserkPos, GunnerPos, LightKillerPos, KamikazePos;

	CVec3Dfp32 SpawnUp = Mat.GetRow(2);
	CVec3Dfp32 Up(0.0f, 0.0f, 1.0f);
	CVec3Dfp32 SpawnRelRight;
	Up.CrossProd(CharToSpawn, SpawnRelRight);
	SpawnRelRight.Normalize();
	fp32 dot = 0.0f;
	dot = SpawnUp * SpawnRelRight;
	if(dot < -0.01f || dot > 0.01f)
	{
		CVec3Dfp32 ProjUp = SpawnUp * dot;
		_Right = (SpawnRelRight - ProjUp);
		_Right.Normalize();
	}
	else
		_Right = SpawnRelRight;

	_Right.CrossProd(SpawnUp, _Forward);
	_Forward.Normalize();
}

void CWObject_DarklingSpawn::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_DarklingSpawn::OnClientRefresh);
	if (_pObj->m_Data[0] & DARKLINGSPAWN_FLAGS_DESTROYWALLMARK)
	{
		_pObj->m_Data[0] = _pObj->m_Data[0] & ~DARKLINGSPAWN_FLAGS_DESTROYWALLMARK;

		if (_pObj->m_ClientData[0])
		{
			//_pWClient->Wallmark_Destroy(_pObj->m_ClientData[0]);
		}
		_pObj->m_ClientData[0] = 0;
	}

//	if (_pObj->m_Data[0] & (DARKLINGSPAWN_FLAGS_DISABLED|DARKLINGSPAWN_FLAGS_DISABLERENDER))
//		return;
}

void CWObject_DarklingSpawn::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_DarklingSpawn_OnClientRender, MAUTOSTRIP_VOID);
	MSCOPESHORT( CWObject_DarklingSpawn::OnClientRender );

	if (_pObj->m_Data[0] & (DARKLINGSPAWN_FLAGS_DISABLED|DARKLINGSPAWN_FLAGS_DISABLERENDER))
		return;

	if ((_pObj->m_ClientData[0] == 0 || _pObj->m_Data[0] & DARKLINGSPAWN_FLAGS_CREATEWALLMARK) && _pObj->m_Data[6] >= (_pWClient->GetGameTick() - 5))
	{
		// Destroy previous wallmarks if any exist
		if (_pObj->m_ClientData[0] != 0)
		{
			//_pWClient->Wallmark_Destroy(_pObj->m_ClientData[0]);
			_pObj->m_ClientData[0] = 0;
		}

		// Fetch surface and wallmark properties and try to create it
		int iWMSurface = (int)_pObj->m_Data[1];
		CXW_Surface* pSurface = _pWClient->GetMapData()->GetResource_Surface(iWMSurface);

		// If no surface was specified, do a safe fallback outside RTM
/*		if (!pSurface)
		{
			iWMSurface = _pWClient->GetMapData()->GetResourceIndex_Surface("darkling-spawn-symbol");
			pSurface = _pWClient->GetMapData()->GetResource_Surface(iWMSurface);
		}

		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
		if (pSC && pSurface)
		{
			int iObj = _pObj->m_Data[4];
			CVec3Dfp32 Forward, Right;
			GetSpawnEffectDirections(_pWClient, _pObj, iObj, Forward, Right);
			CMat4Dfp32 WMMat = _pObj->GetPositionMatrix();
			WMMat.GetRow(0) = Forward;
			WMMat.GetRow(1) = Right;
			WMMat.GetRow(0).CrossProd(WMMat.GetRow(1), WMMat.GetRow(2));
			int WMSurfaceID = pSC->GetSurfaceID("darkling-spawn-symbol");
			fp32 WMSize = fp32((_pObj->m_Data[0] >> 20) & DARKLINGSPAWN_WALLMARK_SIZEMASK);
			fp32 WMTolerance = fp32((_pObj->m_Data[0] >> 12) & DARKLINGSPAWN_WALLMARK_TOLERANCEMASK);

			if(WMTolerance == 0)
				WMTolerance = WMSize;

			CXR_WallmarkDesc WMD;
			WMD.m_SpawnTime = _pWClient->GetRenderTime();
			WMD.m_SurfaceID = WMSurfaceID;
			WMD.m_Size = WMSize;

			_pWClient->Wallmark_Create(WMD, WMMat, WMTolerance, XR_WALLMARK_TEMPORARY);
		}

		iWMSurface = _pWClient->GetMapData()->GetResourceIndex_Surface("darkling-spawn-select");
		pSurface = _pWClient->GetMapData()->GetResource_Surface(iWMSurface);
		if (pSC && pSurface)
		{
			int iObj = _pObj->m_Data[4];
			CVec3Dfp32 Forward, Right;
			GetSpawnEffectDirections(_pWClient, _pObj, iObj, Forward, Right);
			CMat4Dfp32 WMMat = _pObj->GetPositionMatrix();
			WMMat.GetRow(0) = Forward;
			WMMat.GetRow(1) = Right;
			WMMat.GetRow(0).CrossProd(WMMat.GetRow(1), WMMat.GetRow(2));
			int WMSurfaceID = pSC->GetSurfaceID("darkling-spawn-select");
			fp32 WMSize = fp32((_pObj->m_Data[0] >> 20) & DARKLINGSPAWN_WALLMARK_SIZEMASK);
			fp32 WMTolerance = fp32((_pObj->m_Data[0] >> 12) & DARKLINGSPAWN_WALLMARK_TOLERANCEMASK);

			if(WMTolerance == 0)
				WMTolerance = WMSize;

			CXR_WallmarkDesc WMD;
			WMD.m_SpawnTime = _pWClient->GetRenderTime();
			WMD.m_SurfaceID = WMSurfaceID;
			WMD.m_Size = 8.0f;

			if(_pObj->m_Data[5] & DARKLINGSPAWN_FLAGS2_BERSERKER)
				WMMat.GetRow(3) += Forward * DARKLINGSPAWN_CENTER_TO_CHOICE_OFFSET;
			else if(_pObj->m_Data[5] & DARKLINGSPAWN_FLAGS2_KAMIKAZE)
				WMMat.GetRow(3) -= Forward * DARKLINGSPAWN_CENTER_TO_CHOICE_OFFSET;
			else if(_pObj->m_Data[5] & DARKLINGSPAWN_FLAGS2_LIGHTKILLER)
				WMMat.GetRow(3) -= Right * DARKLINGSPAWN_CENTER_TO_CHOICE_OFFSET;
			else if(_pObj->m_Data[5] & DARKLINGSPAWN_FLAGS2_GUNNER)
				WMMat.GetRow(3) += Right * DARKLINGSPAWN_CENTER_TO_CHOICE_OFFSET;
			_pWClient->Wallmark_Create(WMD, WMMat, WMTolerance, XR_WALLMARK_TEMPORARY);
		}*/
	}

	CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
	AnimState.m_Data[3] = ~(_pObj->m_Data[3]);

	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);
	
	// Choose between one of two models to use for rendering
	const int iModel = (_pObj->m_Data[2] & DARKLINGSPAWN_MODEL_USEVARIATION) ? _pObj->m_iModel[1] : _pObj->m_iModel[0];
	CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(iModel);

	if (pModel)
		_pEngine->Render_AddModel(pModel, MatIP, AnimState);

	// Offset matrix upwards/outwards
	MatIP.GetRow(3) += MatIP.GetRow(2) * 5.0f;
	pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[2]);
	if (pModel)
		_pEngine->Render_AddModel(pModel, MatIP, AnimState);
}

void CWObject_DarklingSpawnEffect::OnCreate()
{
	CWObject_Model::OnCreate();
	// Default disabled..
	Data(0) = 0;
	m_DurationNearby = 0;
	m_DurationActive = 0;
	m_DurationTurnActive = 0;
	m_lSpawnStrings.SetLen(5);
	m_TurnActiveTick = 0;
	m_bGotTouched = false;
}

void CWObject_DarklingSpawnEffect::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH4('DURA','TION','NEAR','BY'):
		{
			m_DurationNearby = (int32)(m_pWServer->GetGameTicksPerSecond() * _pKey->GetThisValuef());
			return;
		}
	case MHASH4('DURA','TION','ACTI','VE'):
		{
			m_DurationActive = (int32)(m_pWServer->GetGameTicksPerSecond() * _pKey->GetThisValuef());
			return;
		}
	case MHASH5('DURA','TION','TURN','ACTI','VE'):
		{
			m_DurationTurnActive = (int32)(m_pWServer->GetGameTicksPerSecond() * _pKey->GetThisValuef());
			return;
		}
	case MHASH3('EFFE','CTFL','AGS'): // "EFFECTFLAGS"
		{
			static const char *FlagsTranslate[] =
			{
				"-","--","---","waitspawned",NULL
			};

			Data(0) = Data(0) | KeyValue.TranslateFlags(FlagsTranslate);
		}
	default:
		{
			//CWObject_Model::OnEvalKey(_KeyHash,_pKey);
			break;
		}
	};
	if (KeyName.Find("MSG_ONSPAWN") != -1)
	{
		//
		m_lMsg_OnDarklingSpawn.Add(CWO_SimpleMessage());
		m_lMsg_OnDarklingSpawn[m_lMsg_OnDarklingSpawn.Len()-1].Parse(KeyValue,m_pWServer);
	}
	else if (KeyName.Find("MSG_ONENTERNEARBY") != -1)
	{
		//
		m_lMsg_OnEnterNearby.Add(CWO_SimpleMessage());
		m_lMsg_OnEnterNearby[m_lMsg_OnEnterNearby.Len()-1].Parse(KeyValue,m_pWServer);
	}
	else if (KeyName.Find("MSG_ONLEAVENEARBY") != -1)
	{
		//
		m_lMsg_OnLeaveNearby.Add(CWO_SimpleMessage());
		m_lMsg_OnLeaveNearby[m_lMsg_OnLeaveNearby.Len()-1].Parse(KeyValue,m_pWServer);
	}
	else if (KeyName.Find("MSG_ONENTERACTIVE") != -1)
	{
		//
		m_lMsg_OnEnterActive.Add(CWO_SimpleMessage());
		m_lMsg_OnEnterActive[m_lMsg_OnEnterActive.Len()-1].Parse(KeyValue,m_pWServer);
	}
	else if (KeyName.Find("MSG_ONLEAVEACTIVE") != -1)
	{
		//
		m_lMsg_OnLeaveActive.Add(CWO_SimpleMessage());
		m_lMsg_OnLeaveActive[m_lMsg_OnLeaveActive.Len()-1].Parse(KeyValue,m_pWServer);
	}
	else if (KeyName.Find("MSG_ONTURNACTIVE") != -1)
	{
		//
		m_lMsg_OnTurnActive.Add(CWO_SimpleMessage());
		m_lMsg_OnTurnActive[m_lMsg_OnTurnActive.Len()-1].Parse(KeyValue,m_pWServer);
	}
	else if (KeyName.Find("LIGHTSDARK") != -1)
	{
		m_lSpawnStrings[0].Add(KeyValue);
	}
	else if (KeyName.Find("LIGHTSTURNACTIVE") != -1)
	{
		m_lSpawnStrings[1].Add(KeyValue);
	}
	else if (KeyName.Find("SOUNDACTIVE") != -1)
	{
		m_lSpawnStrings[2].Add(KeyValue);
	}
	else if (KeyName.Find("SOUNDTURNACTIVE") != -1)
	{
		m_lSpawnStrings[3].Add(KeyValue);
	}
	else if (KeyName.Find("SOUNDNEARBY") != -1)
	{
		m_lSpawnStrings[4].Add(KeyValue);
	}
	else
		CWObject_Model::OnEvalKey(_KeyHash,_pKey);
}

void CWObject_DarklingSpawnEffect::OnSpawnWorld()
{
	CWObject_Model::OnSpawnWorld();
	for (int i = 0; i < m_lMsg_OnDarklingSpawn.Len(); i++)
		m_lMsg_OnDarklingSpawn[i].SendPrecache(m_iObject, m_pWServer);
	for (int i = 0; i < m_lMsg_OnEnterNearby.Len(); i++)
		m_lMsg_OnEnterNearby[i].SendPrecache(m_iObject, m_pWServer);
	for (int i = 0; i < m_lMsg_OnLeaveNearby.Len(); i++)
		m_lMsg_OnLeaveNearby[i].SendPrecache(m_iObject, m_pWServer);
	for (int i = 0; i < m_lMsg_OnEnterActive.Len(); i++)
		m_lMsg_OnEnterActive[i].SendPrecache(m_iObject, m_pWServer);
	for (int i = 0; i < m_lMsg_OnLeaveActive.Len(); i++)
		m_lMsg_OnLeaveActive[i].SendPrecache(m_iObject, m_pWServer);
	for (int i = 0; i < m_lMsg_OnTurnActive.Len(); i++)
		m_lMsg_OnTurnActive[i].SendPrecache(m_iObject, m_pWServer);

	// Find linked lights
	int Len = m_lSpawnStrings[0].Len();
	m_liLinkedLampsDark.SetLen(Len);
	for (int i = 0; i < Len; i++)
		m_liLinkedLampsDark[i] = m_pWServer->Selection_GetSingleTarget(m_lSpawnStrings[0][i]);

	Len = m_lSpawnStrings[1].Len();
	m_liLinkedLampsTurnActive.SetLen(Len);
	for (int i = 0; i < Len; i++)
		m_liLinkedLampsTurnActive[i] = m_pWServer->Selection_GetSingleTarget(m_lSpawnStrings[1][i]);

	m_NumSoundActive = m_lSpawnStrings[2].Len();
	m_iSoundActive = 0;
	m_NumSoundTurnActive = m_lSpawnStrings[3].Len();
	m_iSoundTurnActive = m_NumSoundActive;
	m_NumSoundNearby = m_lSpawnStrings[4].Len();
	m_iSoundNearby = m_NumSoundActive;
	int32 iSound = 0;
	m_lSounds.SetLen(m_NumSoundActive + m_NumSoundTurnActive + m_NumSoundNearby);

	for (int i = 0; i < m_NumSoundActive; i++)
	{
		int32 iObj = m_pWServer->Selection_GetSingleTarget(m_lSpawnStrings[2][i]);
		CWObject_CoreData* pObj = m_pWServer->Object_GetCD(iObj);
		if (pObj && pObj->IsClass(class_Sound))
		{
			// Yay, new sound
			CWObject_Sound* pSound = (CWObject_Sound*)pObj;
			m_lSounds[iSound++].Init(pSound,m_pWServer,this);
		}
		m_pWServer->Object_DestroyInstant(iObj);
	}
	
	for (int i = 0; i < m_NumSoundTurnActive; i++)
	{
		int32 iObj = m_pWServer->Selection_GetSingleTarget(m_lSpawnStrings[3][i]);
		CWObject_CoreData* pObj = m_pWServer->Object_GetCD(iObj);
		if (pObj && pObj->IsClass(class_Sound))
		{
			// Yay, new sound
			CWObject_Sound* pSound = (CWObject_Sound*)pObj;
			m_lSounds[iSound++].Init(pSound,m_pWServer,this);
		}
		m_pWServer->Object_DestroyInstant(iObj);
	}
	
	for (int i = 0; i < m_NumSoundNearby; i++)
	{
		int32 iObj = m_pWServer->Selection_GetSingleTarget(m_lSpawnStrings[4][i]);
		CWObject_CoreData* pObj = m_pWServer->Object_GetCD(iObj);
		if (pObj && pObj->IsClass(class_Sound))
		{
			// Yay, new sound
			CWObject_Sound* pSound = (CWObject_Sound*)pObj;
			m_lSounds[iSound++].Init(pSound,m_pWServer,this);
		}
		m_pWServer->Object_DestroyInstant(iObj);
	}

	m_lSpawnStrings.Clear();
}

void CWObject_DarklingSpawnEffect::OnSpawnWorld2()
{
	CWObject_Model::OnSpawnWorld2();
	if (!m_bGotTouched)
	{
		// Remove linked lights as well
		for (int32 i = 0; i < m_liLinkedLampsDark.Len(); i++)
			m_pWServer->Object_DestroyInstant(m_liLinkedLampsDark[i]);
		for (int32 i = 0; i < m_liLinkedLampsTurnActive.Len(); i++)
			m_pWServer->Object_DestroyInstant(m_liLinkedLampsTurnActive[i]);
		// Destroy ourselves
		m_pWServer->Object_DestroyInstant(m_iObject);
	}
}

void CWObject_DarklingSpawnEffect::OnRefresh()
{
	CWObject_Model::OnRefresh();
	// Refresh sounds
	bool bSoundNeedRefresh = false;
	TAP_RCD<CSoundHelper> lSounds = m_lSounds;
	for (int32 i = 0; i < lSounds.Len(); i++)
	{
		if (lSounds[i].m_bRandom)
		{
			bSoundNeedRefresh = true;
			lSounds[i].OnRefresh(m_pWServer,this);
		}
	}

	if (m_TurnActiveTick > 0 && m_TurnActiveTick <= m_pWServer->GetGameTick())
	{
		m_TurnActiveTick = -1;
		// Turn off turn active lights
		CWObject_Message TurnOn(OBJMSG_LIGHT_IMPULSE,CWObject_Light::MSG_OFF);
		TAP_RCD<int16> liLights = m_liLinkedLampsTurnActive;
		for (int32 i = 0; i < liLights.Len(); i++)
			m_pWServer->Message_SendToObject(TurnOn,liLights[i]);
	}
	if (!bSoundNeedRefresh)
	{
		// Turn off "turnactive" lights again
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
	}
}

aint CWObject_DarklingSpawnEffect::OnMessage(const CWObject_Message &_Msg)
{
	if (_Msg.m_Msg == OBJMSG_IMPULSE)
	{
		switch (_Msg.m_Param0)
		{
		case DARKLINGSPAWNEFFECT_IMPULSE_DISABLETURNACTIVE:
			{
				// Turn off turnactive
				if (m_Data[0] & DARKLINGSPAWNEFFECT_FLAG_TURNACTIVEENABLED)
					Data(0) &= ~DARKLINGSPAWNEFFECT_FLAG_TURNACTIVEENABLED;
				return 1;
			}
		case DARKLINGSPAWNEFFECT_IMPULSE_DISABLEACTIVE:
			{
				// Turn off active
				if (m_Data[0] & DARKLINGSPAWNEFFECT_FLAG_ACTIVEENABLED)
					Data(0) &= ~DARKLINGSPAWNEFFECT_FLAG_ACTIVEENABLED;
				// Turn off looping active sound
				TAP_RCD<CSoundHelper> lActive = m_lSounds;
				int32 NumSounds = m_iSoundActive + m_NumSoundActive;
				for (int32 i = m_iSoundActive; i < NumSounds; i++)
				{
					lActive[i].UnSpawn(m_pWServer,this);
				}

				return 1;
			}
		case DARKLINGSPAWNEFFECT_IMPULSE_DISABLENEARBY:
			{
				// Turn off nearby
				if (m_Data[0] & DARKLINGSPAWNEFFECT_FLAG_NEARBYENABLED)
					Data(0) &= ~DARKLINGSPAWNEFFECT_FLAG_NEARBYENABLED;

				// Turn off looping nearby sound
				TAP_RCD<CSoundHelper> lNearby = m_lSounds;
				int32 NumSounds = m_iSoundNearby + m_NumSoundNearby;
				for (int32 i = m_iSoundNearby; i < NumSounds; i++)
				{
					lNearby[i].UnSpawn(m_pWServer,this);
				}
				return 1;
			}
		case DARKLINGSPAWNEFFECT_IMPULSE_ENABLENEARBY:
			{
				// Turn on nearby
				if (!(m_Data[0] & DARKLINGSPAWNEFFECT_FLAG_NEARBYENABLED))
				{
					Data(0) |= DARKLINGSPAWNEFFECT_FLAG_NEARBYENABLED;
				}
				Data(5) = m_DurationNearby ? m_pWServer->GetGameTick() + m_DurationNearby :  m_DurationNearby;
				// Turn on looping nearby sound
				TAP_RCD<CSoundHelper> lNearby = m_lSounds;
				int32 NumSounds = m_iSoundNearby + m_NumSoundNearby;
				for (int32 i = m_iSoundNearby; i < NumSounds; i++)
				{
					lNearby[i].Spawn(m_pWServer,this);
				}


				// Pack object index in the "upper" 16 bits
				//Data(0) = (Data(0) & 0xFFFF) | ((int32)_Msg.m_Param1 << 16);
				return 1;
			}
		case DARKLINGSPAWNEFFECT_IMPULSE_ENABLEACTIVE:
			{
				// Turn on nearby
				if (!(m_Data[0] & DARKLINGSPAWNEFFECT_FLAG_ACTIVEENABLED))
				{
					Data(0) |= DARKLINGSPAWNEFFECT_FLAG_ACTIVEENABLED;
				}
				Data(6) = m_DurationActive ? m_pWServer->GetGameTick() + m_DurationActive :  m_DurationActive;
				// Turn on looping active sound
				TAP_RCD<CSoundHelper> lActive = m_lSounds;
				int32 NumSounds = m_iSoundActive + m_NumSoundActive;
				for (int32 i = m_iSoundActive; i < NumSounds; i++)
				{
					lActive[i].Spawn(m_pWServer,this);
				}

				// Pack object index in the "upper" 16 bits
				//Data(0) = (Data(0) & 0xFFFF) | ((int32)_Msg.m_Param1 << 16);

				return 1;
			}
		case DARKLINGSPAWNEFFECT_IMPULSE_ENABLETURNACTIVE:
			{
				// Turn on nearby
				if (!(m_Data[0] & DARKLINGSPAWNEFFECT_FLAG_TURNACTIVEENABLED))
				{
					Data(0) |= DARKLINGSPAWNEFFECT_FLAG_TURNACTIVEENABLED;
				}
				Data(7) = m_DurationTurnActive ? m_pWServer->GetGameTick() + m_DurationTurnActive :  m_DurationTurnActive;
				
				m_TurnActiveTick = m_pWServer->GetGameTick() + 2;
				if (m_liLinkedLampsTurnActive.Len())
				{
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
					// Turn on turnactive lights
					CWObject_Message TurnOn(OBJMSG_LIGHT_IMPULSE,CWObject_Light::MSG_ON);
					TAP_RCD<int16> liLights = m_liLinkedLampsTurnActive;
					for (int32 i = 0; i < liLights.Len(); i++)
						m_pWServer->Message_SendToObject(TurnOn,liLights[i]);
				}
				// Turn on turnactive sound
				TAP_RCD<CSoundHelper> lTurnActive = m_lSounds;
				int32 NumSounds = m_iSoundTurnActive + m_NumSoundTurnActive;
				for (int32 i = m_iSoundTurnActive; i < NumSounds; i++)
				{
					lTurnActive[i].Spawn(m_pWServer,this);
				}
				// Pack object index in the "upper" 16 bits
				//Data(0) = (Data(0) & 0xFFFF) | ((int32)_Msg.m_Param1 << 16);
				return 1;
			}
		default:
			return 0;
		}
	}
	switch (_Msg.m_Msg)
	{
	case OBJMSG_GAME_SPAWN:
		{
			if (!_Msg.m_Param0)
			{
				// Enable
				if (m_Data[0] & DARKLINGSPAWNEFFECT_FLAG_WAITSPAWN)
					Data(0) &= ~DARKLINGSPAWNEFFECT_FLAG_WAITSPAWN;
			}
			else if (_Msg.m_Param0 > 0)
			{
				// Disable
				if (!(m_Data[0] & DARKLINGSPAWNEFFECT_FLAG_WAITSPAWN))
					Data(0) |= DARKLINGSPAWNEFFECT_FLAG_WAITSPAWN;
			}
			return 1;
		}
	case OBJMSG_DARKLINGSPAWN_SENDDARKLINGSPAWN:
		{
			TAP_RCD<CWO_SimpleMessage> lMessage = m_lMsg_OnDarklingSpawn;
			for (int32 i = 0; i < lMessage.Len(); i++)
				lMessage[i].SendMessage(m_iObject, m_iObject, m_pWServer);
			return 1;
		}
	case OBJMSG_DARKLINGSPAWN_SENDENTERNEARBY:
		{
			TAP_RCD<CWO_SimpleMessage> lMessage = m_lMsg_OnEnterNearby;
			for (int32 i = 0; i < lMessage.Len(); i++)
				lMessage[i].SendMessage(m_iObject, m_iObject, m_pWServer);

			return 1;
		}
	case OBJMSG_DARKLINGSPAWN_SENDLEAVENEARBY:
		{
			TAP_RCD<CWO_SimpleMessage> lMessage = m_lMsg_OnLeaveNearby;
			for (int32 i = 0; i < lMessage.Len(); i++)
				lMessage[i].SendMessage(m_iObject, m_iObject, m_pWServer);

			return 1;
		}
	case OBJMSG_DARKLINGSPAWN_SENDLEAVENEARBYSOUNDS:
		{
			// Turn off looping nearby sound
			TAP_RCD<CSoundHelper> lNearby = m_lSounds;
			int32 NumSounds = m_iSoundNearby + m_NumSoundNearby;
			for (int32 i = m_iSoundNearby; i < NumSounds; i++)
			{
				lNearby[i].UnSpawn(m_pWServer,this);
			}

			return 1;
		}
	case OBJMSG_DARKLINGSPAWN_SENDENTERACTIVE:
		{
			TAP_RCD<CWO_SimpleMessage> lMessage = m_lMsg_OnEnterActive;
			for (int32 i = 0; i < lMessage.Len(); i++)
				lMessage[i].SendMessage(m_iObject, m_iObject, m_pWServer);

			return 1;
		}
	case OBJMSG_DARKLINGSPAWN_SENDLEAVEACTIVE:
		{
			TAP_RCD<CWO_SimpleMessage> lMessage = m_lMsg_OnLeaveActive;
			for (int32 i = 0; i < lMessage.Len(); i++)
				lMessage[i].SendMessage(m_iObject, m_iObject, m_pWServer);

			return 1;
		}
	case OBJMSG_DARKLINGSPAWN_SENDTURNACTIVE:
		{
			TAP_RCD<CWO_SimpleMessage> lMessage = m_lMsg_OnTurnActive;
			for (int32 i = 0; i < lMessage.Len(); i++)
				lMessage[i].SendMessage(m_iObject, m_iObject, m_pWServer);

			return 1;
		}
	case OBJMSG_DARKLINGSPAWN_SENDDARKLIGHTS:
		{
			if (!_Msg.m_Param0)
			{
				// Turn off active lights
				CWObject_Message TurnOn(OBJMSG_LIGHT_IMPULSE,CWObject_Light::MSG_OFF);
				TAP_RCD<int16> liLights = m_liLinkedLampsDark;
				for (int32 i = 0; i < liLights.Len(); i++)
					m_pWServer->Message_SendToObject(TurnOn,liLights[i]);
			}
			else
			{
				// Turn on active lights
				CWObject_Message TurnOn(OBJMSG_LIGHT_IMPULSE,CWObject_Light::MSG_ON);
				TAP_RCD<int16> liLights = m_liLinkedLampsDark;
				for (int32 i = 0; i < liLights.Len(); i++)
					m_pWServer->Message_SendToObject(TurnOn,liLights[i]);
			}
			return 1;
		}
	case OBJMSG_DARKLINGSPAWN_TOUCHEFFECT:
		{
			m_bGotTouched = true;
			return 1;
		}
	default:
		return CWObject_Model::OnMessage(_Msg);
	}
}

void CWObject_DarklingSpawnEffect::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_DarklingSpawnEffect_OnClientRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_DarklingSpawnEffect::OnClientRender);
	
	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

	// Fetch object index from the hiword (same for all slots)
//	uint16 iOwner = (_pObj->m_Data[0] >> 16);
//	CWObject_Client* pOwner = (iOwner) ? _pWClient->Object_Get(iOwner) : NULL;
//	fp32 Length = 1.0f;
//	if(pOwner)
//	{
//		// Setup matrix to point at owner
//		MatIP.GetRow(2) = pOwner->GetPosition() - MatIP.GetRow(3);
//		MatIP.GetRow(2).Normalize();
//		MatIP.RecreateMatrix(2,0);
//		Length = (pOwner->GetPosition() - MatIP.GetRow(3)).Length();
//	}
	CDarklingSpawnEffectClientData* pCD = GetClientData(_pObj);
	for (int32 i = 0; i < 3; i++)
	{
		if (_pObj->m_iModel[i] && ((_pObj->m_Data[0] & M_BitD(i)) || (pCD->m_EndTick[i] != 0 && ((_pWClient->GetGameTick() - pCD->m_EndTick[i]) < _pWClient->GetGameTicksPerSecond()))) && 
			(_pObj->m_Data[5 + i] == 0 || _pObj->m_Data[5 + i] >= _pWClient->GetGameTick()))
		{
			// Add model
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
			if(pModel)
			{
				CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
				AnimState.m_AnimTime0 = CMTime::CreateFromTicks(_pWClient->GetGameTick() - pCD->m_StartTick[i], _pWClient->GetGameTickTime(), _pWClient->GetRenderTickFrac());
				if (_pObj->m_Data[0] & M_BitD(i))
				{
					int32 EndTick = Max((int32)0,_pWClient->GetGameTick() - pCD->m_StartTick[i] + (_pWClient->GetGameTick() - pCD->m_EndTick[i]) * 2 - pCD->m_Offset[i]);
					AnimState.m_AnimTime1 = pCD->m_EndTick[i] > 0 ? CMTime::CreateFromTicks(EndTick,_pWClient->GetGameTickTime(),_pWClient->GetRenderTickFrac()) : CMTime::CreateInvalid();
				}
				else
				{
					int32 EndTick = Max((int32)0,pCD->m_EndTick[i] - pCD->m_StartTick[i]);
					AnimState.m_AnimTime1 = pCD->m_EndTick[i] > 0 ? CMTime::CreateFromTicks(EndTick,_pWClient->GetGameTickTime(),-_pWClient->GetRenderTickFrac()) : CMTime::CreateInvalid();
				}
				
				AnimState.m_Data[3] = ~(_pObj->m_Data[3]);
//				AnimState.m_Data[0] = *((int32*)((fp32*)&Length));
				_pEngine->Render_AddModel(pModel, MatIP, AnimState);
			}
		}
	}
}

int CWObject_DarklingSpawnEffect::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	int32 Flags = _pObj->m_Data[0];
	int32 DurationTurnActive = _pObj->m_Data[7];
	int32 RetVal = CWObject_Model::OnClientUpdate(_pObj,_pWClient,_pData,_Flags);
	CDarklingSpawnEffectClientData* pCD = GetClientData(_pObj);
	if (!pCD)
		return RetVal;

	pCD->m_RealData = _pObj->m_Data[0];
	if (_pObj->m_Data[0] & DARKLINGSPAWNEFFECT_FLAG_WAITSPAWN)
	{
		_pObj->m_Data[0] = 0;
	}
	else
	{
		for (int32 i = 0; i < 3; i++)
		{
			if ((Flags ^ _pObj->m_Data[0]) & M_BitD(i))
			{
				if (Flags & M_BitD(i))
				{
					// Turned off nearby
					pCD->m_EndTick[i] = _pWClient->GetGameTick() + TruncToInt(_pWClient->GetGameTicksPerSecond());
				}
				else
				{
					// Turned on nearby
					int32 DiffVal = (_pWClient->GetGameTick() - pCD->m_EndTick[i]);
					if (DiffVal < TruncToInt(_pWClient->GetGameTicksPerSecond()))
					{
						pCD->m_StartTick[i] = pCD->m_StartTick[i];
						pCD->m_Offset[i] = DiffVal;
						pCD->m_EndTick[i] = _pWClient->GetGameTick();
					}
					else
					{
						pCD->m_StartTick[i] = _pWClient->GetGameTick();
						pCD->m_Offset[i] = 0;
						pCD->m_EndTick[i] = 0;
					}
				}
			}
			else if (i == 2 && DurationTurnActive != _pObj->m_Data[7])
			{
				pCD->m_StartTick[i] = _pWClient->GetGameTick();
				pCD->m_Offset[i] = 0;
				pCD->m_EndTick[i] = 0;
			}
		}
	}
	return RetVal;
}

CWObject_DarklingSpawnEffect::CDarklingSpawnEffectClientData* CWObject_DarklingSpawnEffect::GetClientData(CWObject_CoreData* _pObj)
{
	if(_pObj->m_lspClientObj[0] == NULL)
	{
		_pObj->m_lspClientObj[0] = MNew(CDarklingSpawnEffectClientData);
		if(!_pObj->m_lspClientObj[0])
			Error_static("CWObject_DarklingSpawnEffect::CDarklingSpawnEffectClientData", "Could not allocate ClientData.")
			CDarklingSpawnEffectClientData *pData = (CDarklingSpawnEffectClientData *)(CReferenceCount *)_pObj->m_lspClientObj[0];
		return pData;
	}
	else
		return (CDarklingSpawnEffectClientData *)(CReferenceCount *)_pObj->m_lspClientObj[0];
}

const CWObject_DarklingSpawnEffect::CDarklingSpawnEffectClientData* CWObject_DarklingSpawnEffect::GetClientData(const CWObject_CoreData* _pObj)
{
	return (const CDarklingSpawnEffectClientData *)(const CReferenceCount *)_pObj->m_lspClientObj[0];
}

CWObject_DarklingSpawnEffect::CSoundHelper::CSoundHelper()
{
	m_Offset = CVec3Dfp32(0.0f,0.0f,0.0f);
	m_iRealSound[0] = 0;
	m_iRealSound[1] = 0;
	m_bWaitSpawn = true;
	m_bRandom = false;

	for (int32 i = 0; i < CWObject_Sound::MAXRANDOMSOUNDS; i++)
	{
		m_RandomSound_iSound[i] = 0;
		m_RandomSound_MaxTick[i] = 0;
		m_RandomSound_MinTick[i] = 0;
		m_RandomSound_NextTick[i] = 0;
	}
}

void CWObject_DarklingSpawnEffect::CSoundHelper::Init(CWObject_Sound* _pSound, CWorld_Server* _pWServer, CWObject_DarklingSpawnEffect* _pEffect)
{
	if (!_pSound)
		return;
	m_Offset = _pSound->GetPosition() - _pEffect->GetPosition();
	m_iRealSound[0] = _pSound->m_iRealSound[0];
	m_iRealSound[1] = _pSound->m_iRealSound[1];
	m_bWaitSpawn = _pSound->m_bWaitSpawn;
	
	for (int32 i = 0; i < CWObject_Sound::MAXRANDOMSOUNDS; i++)
	{
		m_RandomSound_iSound[i] = _pSound->m_RandomSound_iSound[i];
		m_RandomSound_MaxTick[i] = _pSound->m_RandomSound_MaxTick[i];
		m_RandomSound_MinTick[i] = _pSound->m_RandomSound_MinTick[i];
		m_RandomSound_NextTick[i] = _pSound->m_RandomSound_NextTick[i];
	}
}

void CWObject_DarklingSpawnEffect::CSoundHelper::Spawn(CWorld_Server* _pWServer, CWObject_DarklingSpawnEffect* _pEffect)
{
	MAUTOSTRIP(CWObject_DarklingSpawnEffect::CSoundHelper::Spawn, MAUTOSTRIP_VOID);

	m_bWaitSpawn = false;
	m_bRandom = false;
	for(int i = 0; i < CWObject_Sound::MAXRANDOMSOUNDS; i++)
	{
		if(m_RandomSound_iSound[i] != 0)
		{
			//Some idiot-proof checks
			if(m_RandomSound_MinTick[i] <= 0)
				m_RandomSound_MinTick[i] = 1;
			if(m_RandomSound_MaxTick[i] < m_RandomSound_MinTick[i])
				m_RandomSound_MaxTick[i] = m_RandomSound_MinTick[i];

			//Calculate first tick that the sound should be played on. Add one for safety.
			m_RandomSound_NextTick[i] = int(_pWServer->GetGameTick() + Random * m_RandomSound_MaxTick[i] + 1);
			m_bRandom = true;
		}
	}
	if (m_bRandom)
	{
		_pEffect->ClientFlags() &= ~ CWO_CLIENTFLAGS_NOREFRESH;
	}

	if (m_iRealSound[0])
		_pWServer->Sound_On(_pEffect->m_iObject, m_iRealSound[0], WCLIENT_ATTENUATION_LOOP, 0, 1.0f, -1, 0, m_Offset);
	if (m_iRealSound[1])
		_pWServer->Sound_On(_pEffect->m_iObject, m_iRealSound[1], WCLIENT_ATTENUATION_LOOP, 0, 1.0f, -1, 0, m_Offset);
}

void CWObject_DarklingSpawnEffect::CSoundHelper::UnSpawn(CWorld_Server* _pWServer, CWObject_DarklingSpawnEffect* _pEffect)
{
	m_bWaitSpawn = true;
	m_bRandom = false;

	if (m_iRealSound[0])
		_pWServer->Sound_Off(_pEffect->m_iObject, m_iRealSound[0]);
	if (m_iRealSound[1])
		_pWServer->Sound_Off(_pEffect->m_iObject, m_iRealSound[1]);
}

void CWObject_DarklingSpawnEffect::CSoundHelper::OnRefresh(CWorld_Server* _pWServer, CWObject_DarklingSpawnEffect* _pEffect)
{
	CVec3Dfp32 Position = _pEffect->GetPosition() + m_Offset;
	for(int i = 0; i < CWObject_Sound::MAXRANDOMSOUNDS; i++)
	{
		if(m_RandomSound_iSound[i] != 0)
		{
			if(_pWServer->GetGameTick() >= m_RandomSound_NextTick[i])
			{
				_pWServer->Sound_At(Position, m_RandomSound_iSound[i], 0);
				int Delta = m_RandomSound_MaxTick[i] - m_RandomSound_MinTick[i];
				m_RandomSound_NextTick[i] = int(_pWServer->GetGameTick() + m_RandomSound_MinTick[i] + Random * Delta);
			}
		}
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_DarklingSpawnEffect, CWObject_Model, 0x0100);

