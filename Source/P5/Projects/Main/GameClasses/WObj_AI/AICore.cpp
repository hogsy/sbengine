 #include "PCH.h"
#include "AICore.h"
#include "../../../../shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
#include "../../../../shared/MOS/Classes/GameWorld/WObjects/WObj_PhysCluster.h"
#include "../../../../shared/MOS/XR/XRBlockNav.h"
#include "../WObj_Game/WObj_GameMod.h"
#include "WObj_Aux/WObj_AreaInfo.h"
#include "WObj_Aux/WObj_Team.h"
#include "../WObj_CharMsg.h"
#include "../WObj_Char.h"
#include "../CConstraintSystem.h"
#include "../WRPG/WRPGChar.h"
#include "../WRPG/WRPGAmmo.h"
#include "AI_ResourceHandler.h"
#include "../WObj_Char/WObj_CharDarkling.h"
#include "../WObj_Misc/WObj_ScenePoint.h"
#include "../WObj_Misc/WObj_Turret.h"
#include "../WAG2_ClientData_Game.h"
#include "../WObj_Misc/WObj_AnimEventListener.h"

//Custom AIs
#include "AI_Custom/AICore_Turret.h"
#include "AI_Custom/AICore_Darkling.h"

//#pragma optimize("",off)
//#pragma inline_depth(0)

enum
{
	class_CWObject_CharNPC =	MHASH5('CWOb','ject','_','Char','NPC'),
	class_CAI_Core_Darkling =	MHASH5('CAI_','Core','_','Dark','ling'),
};

MRTC_IMPLEMENT(CAI_Core, CReferenceCount);

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObj_LightMeter
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObj_LightMeter::CWObj_LightMeter(CAI_Core* _pAI)
{
	m_pAI = _pAI;
	m_LastMeasureTick = 0;
	for (int32 i = 0; i < LIGHTMETER_MAXLIGHTS; i++)
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

	INVALIDATE_POS(m_Pos);
}

CWObj_LightMeter::~CWObj_LightMeter()
{
}

#ifndef M_RTM
#define DEBUG_DRAW_LIGHTMETER
#endif

// Measures the squared distance from _p to segment _M + t_ * _B, returning the param t.
static inline fp32 SqrDistanceToSegment(const CVec3Dfp32& _P,const CVec3Dfp32& _B,const CVec3Dfp32& _M,fp32* _pT = NULL,CVec3Dfp32* _pDiff = NULL)
{
	CVec3Dfp32 diff;
	fp32 t;

	diff = _P - _B;
	t = _M * diff;
	if (t > 0.0f)
	{
		fp32 dotMM = _M * _M;
		if (t < dotMM)
		{
			t = t / dotMM;
			diff = diff - _M * t;
		}
		else
		{
			t = 1.0f;
			diff = diff - _M;
		}
	}
	else
	{
		t = 0.0f;
	}

	if (_pDiff)
	{
		*_pDiff = diff;
	}
	if (_pT)
	{
		*_pT = t;
	}
	return(diff * diff);
};


// MeasureFakePointLight() measures the light intersecting a box at _Pos with dimensions _Width, _Height
// _Light is the point light not yet added to the world that should work as a fake light for measurement
// Returns the intensity of the fake light
fp32 CWObj_LightMeter::MeasureFakePointLight(const CVec3Dfp32& _BoxPos, const CVec3Dfp32& _MeasurePos, fp32 _Width, fp32 _Height, const CXR_Light& _Light, const bool _TraceLine, const int _iExclude0, const int _iExclude1)
{
	MSCOPESHORT(CWObj_LightMeter::MeasureFakePointLight);
	if (!m_pAI || !m_pAI->m_pGameObject)
		return 0.0f;

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

	if ((_TraceLine) && (m_pAI->m_pServer->Phys_IntersectLine(_MeasurePos, LightPos, ObjectFlags, ObjectFlags, Mediums, _iExclude0, _iExclude1)))
	{
		// Draw a red debug line showing how we where aiming at target point with the light measure
		if (m_pAI->DebugRender())
		{
			const int Color = 0xffff0000 | (255 << 16) | (0 << 8) | (0);
			m_pAI->Debug_RenderWire(LightPos, _MeasurePos, Color, 1.0f);
		}
		return 0.0f;
	}

	const fp32 RangeFactor  = (_Light.m_Range - M_Sqrt(DeltaSqr)) * _Light.m_RangeInv;
		
	// Draw debug
	#ifdef DEBUG_DRAW_LIGHTMETER
	{
		const fp32 Intens = RangeFactor * IntensFactor;
		if ((m_pAI->DebugRender()) && (_TraceLine))
		{
			const int R = MinMT(int(Intens * Intensity.k[0] * 255.0f), 255);
			const int G = MinMT(int(Intens * Intensity.k[1] * 255.0f), 255);
			const int B = MinMT(int(Intens * Intensity.k[2] * 255.0f), 255);
			const int Color = 0xff000000 | (R << 16) | (G << 8) | (B);
			m_pAI->Debug_RenderWire(LightPos, _MeasurePos, Color, 1.0f);
			//m_pAU->Debug_RenderVertex(m_Pos, kColorWhite, 1.0f);
		}

		return Intens;
	}
	#else
		return RangeFactor * IntensFactor;
	#endif
}


// Measure() measures the light intersecting a box at _Pos with dimensions _Width, _Height
// The _nLights tells us the number of lights to sample this call (-1 will sample all lights)
// Returns true if all lights have been sampled at least once and false if not
bool CWObj_LightMeter::Measure(const CVec3Dfp32& _BoxPos,const CVec3Dfp32& _MeasurePos, fp32 _Width, fp32 _Height, int _nLights,bool _TraceLines,int _iSkipObj)
{
	MSCOPESHORT(CWObj_LightMeter::Measure);
	if ((!m_pAI)||(!m_pAI->m_pGameObject))
	{
		return(false);
	}

	fp32 halfWidth = _Width * 0.5f;
	fp32 halfHeight = _Height * 0.5f;
	if ((_nLights < 0)||(_TraceLines)||(m_Pos.DistanceSqr(_BoxPos) > Sqr(LIGHTMETER_SLOP)))
	{	// Position has moved too much, we must resample lights
		CBox3Dfp32 Box;
		Box.m_Max = CVec3Dfp32( halfWidth, halfWidth, halfHeight);
		Box.m_Min = -Box.m_Max;
		Box.m_Max += _BoxPos;
		Box.m_Min += _BoxPos;

		CXR_SceneGraphInstance* pSceneGraph = m_pAI->m_pServer->World_GetSceneGraphInstance();
		if (!pSceneGraph)
			return false;

		const CXR_Light* lpLights[LIGHTMETER_MAXLIGHTS];
		int nLights = pSceneGraph->SceneGraph_Light_Enum(Box, lpLights, LIGHTMETER_MAXLIGHTS);
		for (int i = 0; i < LIGHTMETER_MAXLIGHTS; i++)
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
			if (m_Pos.DistanceSqr(_BoxPos) > Sqr(LIGHTMETER_SLOP))
			{	// Check lightmap values
				CXR_Model* pModel = m_pAI->m_pServer->World_GetModel();
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
	{	// No lights to worry about
		m_LastMeasureTick = m_pAI->GetAITick();
		m_PrevLightIntensity = m_LightIntensity;
		m_LightIntensity = m_LightMapIntensity;
		// Nonlinear lightmeter
		m_LightIntensity = M_Sqrt(m_LightIntensity);
		if (m_LightIntensity > 1.0f)	
		{
			m_LightIntensity = 1.0f;
		}	
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
		fp32 IntensFactor = Max3(Intensity.k[0],Intensity.k[1],Intensity.k[2]);
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
		if ((_TraceLines)&&(m_pAI->m_pServer->Phys_IntersectLine(_MeasurePos,pLight->GetPosition(),ObjectFlags,ObjectFlags,Mediums,NULL,_iSkipObj)))
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

#ifdef DEBUG_DRAW_LIGHTMETER
		// We draw a line from source to measured spot (in appropriate color)
		if ((m_pAI->DebugRender())&&(_TraceLines))
		{
			CVec3Dfp32 LightPos = pLight->GetPosition();
			int Color = 0xff000000;
			int R = Min(int(Intens * Intensity[0] * 255.0f),255);
			int G = Min(int(Intens * Intensity[1] * 255.0f),255);
			int B = Min(int(Intens * Intensity[2] * 255.0f),255);
			Color |= (R << 16) | (G << 8) | (B);
			m_pAI->Debug_RenderWire(LightPos,_MeasurePos,Color,1.0f);
		}
#endif
	}
#ifdef DEBUG_DRAW_LIGHTMETER
	m_pAI->m_pServer->Debug_RenderVertex(m_Pos,kColorWhite,1.0f);
#endif

	if (m_pAI)
	{
		m_LastMeasureTick = m_pAI->GetAITick();
	}
	UpdateIntensity();

	return(m_bIntensityValid);
}

fp32 CWObj_LightMeter::GetIntensity(int _iObj)
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

void CWObj_LightMeter::UpdateIntensity()
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
	m_LightIntensity += m_LightMapIntensity;

	// Nonlinear lightmeter
	m_LightIntensity = M_Sqrt(m_LightIntensity);

	if (m_LightIntensity > 1.0f)	
	{
		m_LightIntensity = 1.0f;
	}	
};

// ========================================================================================
// CAI_Core
// ========================================================================================
static const char * TranslateUseFlags[] =
{
	"clumsy", "flyatpathspeed", "unspotfast", "saveplayerinfo", 
	"switchweapon", "alwaysblock", "scriptmute", "alwaysaggressive", 
	"nopathfinding", "noroomselect", "doorsps", "stairs", "noturnin", 
	"drawammo", "softlookpitch", "teleporter", NULL
};

//Methods for getting the maximum speed of the character in the given directions
fp32 CAI_Core::GetMaxSpeedForward()
{
	if (!m_pGameObject || !m_pServer)
		return 0;

	return m_pGameObject->AI_GetMaxSpeed(AIQUERY_GETMAXSPEED_FORWARD);
};

fp32 CAI_Core::GetMaxSpeedSideStep()
{
	if (!m_pGameObject || !m_pServer)
		return 0;

	return m_pGameObject->AI_GetMaxSpeed(AIQUERY_GETMAXSPEED_SIDESTEP);
};

fp32 CAI_Core::GetMaxSpeedUp()
{
	if (!m_pGameObject || !m_pServer)
		return 0;

	return m_pGameObject->AI_GetMaxSpeed(AIQUERY_GETMAXSPEED_UP);
};

fp32 CAI_Core::GetMaxSpeedJump()
{
	if (!m_pGameObject || !m_pServer)
		return 0;

	return m_pGameObject->AI_GetMaxSpeed(AIQUERY_GETMAXSPEED_JUMP);
};

fp32 CAI_Core::GetMaxSpeedWalk()
{
	if (!m_pGameObject || !m_pServer)
		return 0;
	// Fudge the walkspeed to be 0.3 of runspeed
	return(m_pGameObject->AI_GetMaxSpeed(AIQUERY_GETMAXSPEED_FORWARD)* 0.3f);
}


//These methods aren't properly implemented, since if they we're 
//there would be a lot of problems right now FIX!
fp32 CAI_Core::GetBaseSize()
{
	return(m_PF_BaseSize);
	// return 24;
};

fp32 CAI_Core::GetHeight()
{
	return(m_PF_Height);
	//return 64;
};

fp32 CAI_Core::GetCrouchHeight()
{
	return(m_PF_HeightCrouch);
	//return 32;
};

fp32 CAI_Core::GetCurHeight()
{
	return(m_CurHeight);
	/*
	if (CWObject_Character::Char_GetPhysType(m_pGameObject) == PLAYER_PHYS_CROUCH)
	{
		return(GetCrouchHeight());
	}
	else
	{
		return(GetHeight());
	}
	*/
};

fp32 CAI_Core::GetStepHeight()
{
	return(m_PF_StepHeight);
	//return 12;
};

fp32 CAI_Core::GetJumpHeight()
{
	return(m_PF_JumpHeight);
	//return 48;
};

fp32 CAI_Core::GetWalkStepLength()
{
	return(m_PF_WalkStepLength);
	//return 24;
};

fp32 CAI_Core::GetRunStepLength()
{
	return(m_PF_RunStepLength);
	//return 48;
};

void CAI_Core::SetBaseSize(fp32 _Val)
{
	m_PF_BaseSize = (int16)_Val;
}

void CAI_Core::SetHeight(fp32 _Val)
{
	m_PF_Height = (int16)_Val;
}

void CAI_Core::SetCrouchHeight(fp32 _Val)
{
	m_PF_HeightCrouch = (int16)_Val;
}

void CAI_Core::SetStepHeight(fp32 _Val)
{
	m_PF_StepHeight = (int16)_Val;
}

void CAI_Core::SetJumpHeight(fp32 _Val)
{
	m_PF_JumpHeight = (int16)_Val;
}

void CAI_Core::SetWalkStepLength(fp32 _Val)
{
	m_PF_WalkStepLength = (int16)_Val;
}

void CAI_Core::SetRunStepLength(fp32 _Val)
{
	m_PF_RunStepLength = (int16)_Val;
}

void CAI_Core::MeasureLightIntensity(bool _bTracelines)
{
	if ((m_UseLightmeter < 0)||(!m_pGameObject))
		return;

	if (!m_spLightMeter)
	{
		m_spLightMeter = MNew1(CWObj_LightMeter,this);
	}

	fp32 height = GetCurHeight();
	fp32 width = GetBaseSize();
	CVec3Dfp32 BoxPos = GetBasePos();
	CVec3Dfp32 MeasurePos = BoxPos;
	int iSkipObject = GetObjectID();
	if (IsPlayer())
	{
/*
		if (m_bDarknessActive)
		{	// Measure light at Creeping Dark head
			if (GetDarknessPos(&BoxPos))
			{
				MeasurePos = BoxPos;
				CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
				if (pCD)
				{
					iSkipObject = pCD->m_iCreepingDark;
				}
			}
		}
*/
		MeasurePos[0] += width * (Random-0.5f) * 0.5f;
		MeasurePos[1] += width * (Random-0.5f) * 0.5f;
		MeasurePos[2] += height * 0.5f;
	}
	else
	{
		// Somewhat above
		MeasurePos[2] += 6;
	}
	m_spLightMeter->Measure(BoxPos,MeasurePos,width,height,10,_bTracelines,iSkipObject);
};

fp32 CAI_Core::MeasureLightIntensityAt(const CVec3Dfp32& _Pos,bool _bTracelines,int _iSkipObj)
{
	if ((m_UseLightmeter < 0)||(!m_pGameObject))
	{	// If we don't know the value we treat it as bright
		return(1.0f);
	}

	if (!m_spLightMeter)
	{
		m_spLightMeter = MNew1(CWObj_LightMeter,this);
	}

	fp32 height,width;
	if (CWObject_Character::Char_GetPhysType(m_pGameObject) == PLAYER_PHYS_CROUCH)
	{
		height = GetCrouchHeight();
	}
	else
	{
		height = GetHeight();
	}
	width = GetBaseSize();
	CVec3Dfp32 BoxPos = _Pos;
	CVec3Dfp32 MeasurePos = BoxPos;
	if (IsPlayer())
	{
		MeasurePos[0] += width * (Random-0.5f) * 0.5f;
		MeasurePos[1] += width * (Random-0.5f) * 0.5f;
		MeasurePos[2] += height * 0.5f;
		// MeasurePos[2] += height * Random * 0.5f;
	}
	else
	{
		// Somewhat above
		MeasurePos[2] += 6;
	}
	m_spLightMeter->Measure(BoxPos,MeasurePos,width,height,10,_bTracelines,_iSkipObj);
	// We supply GetIntensity() with iObj 0 as we don't want 'this' flashlight to interfere with our measurement
	return(m_spLightMeter->GetIntensity(0));
};

fp32 CAI_Core::GetCorpseLightIntensity(int _Interval)
{
	fp32 rIntensity;
	if (m_spLightMeter)
	{
//		int Tick = m_spLightMeter->m_LastMeasureTick;
 		if (GetAITick() >= m_spLightMeter->m_LastMeasureTick + _Interval)
		{	// Yes, we can afford the expensive lightmeter as we only do this every ???
			MeasureLightIntensity(true);
		}
		rIntensity = m_spLightMeter->GetIntensity(0);

		//Clamp intensity at 1 (otherwise we count as having a muzzle flash when in bright light)
		if (rIntensity > 1.0f)
			rIntensity = 1.0f;
	}
	else
	{
		rIntensity = 0.5f;
	}

	return(rIntensity);
};

fp32 CAI_Core::GetLightIntensity(int _iObj)
{
	fp32 rIntensity;
	if (m_spLightMeter)
	{
		rIntensity = m_spLightMeter->GetIntensity(_iObj);
		
		//Clamp intensity at 1 (otherwise we count as having a muzzle flash whe in bright light)
		if (rIntensity > 1.0f)
			rIntensity = 1.0f;

		if ((rIntensity < LIGHTMETER_CORPSE_MIN)&&(CWObject_Character::Char_GetPhysType(m_pGameObject) == PLAYER_PHYS_DEAD))
		{
			rIntensity = LIGHTMETER_CORPSE_MIN;
		}
	}
	else
	{
		rIntensity = 0.0f;
	}

	return(rIntensity);
};

fp32 CAI_Core::GetLightmapIntensity()
{
	fp32 rIntensity = 0.0f;
	if (m_spLightMeter)
	{
		rIntensity = m_spLightMeter->GetLightmapIntensity();
		return(rIntensity);
	}

	return(rIntensity);
};

fp32 CAI_Core::GetLightIntensityDelta()
{
	fp32 rDiff;
	if (m_spLightMeter)
	{
		rDiff = m_spLightMeter->GetIntensityDelta();
	}
	else
	{
		rDiff = 0.0f;
	}
	return(rDiff);
};

bool CAI_Core::UseFlash(int _Duration,int _Prio)
{
	CWObject_Game *pGame = m_pServer->Game_GetObject();
	if ((!pGame)||(!m_pGameObject))
	{
		return false;
	}

	int32 playerID = pGame->Player_GetObjectIndex(0);	// There is only one player in PB right?
	uint8 Prio = _Prio;

	fp32 SqrDist = SqrDistanceToUs(playerID);
	if (SqrDist < Sqr(512.0f))
	{
		Prio++;
		if (SqrDist < Sqr(320.0f))
		{
			Prio++;
		}
	}

	if (m_pAIResources->ms_FlashlightUsers.Poll(m_pGameObject->m_iObject,Prio,GetGlobalAITick(),_Duration))
	{
		return(true);
	}
	else
	{
		return(false);
	}
};

void CAI_Core::HandleFlashlight(int _Duration)
{
	fp32 Intensity,Diff;

	if ((!m_spLightMeter)||(m_UseLightmeter == 0)||(!m_pGameObject))
	{
		return;
	}

	if ((m_StunTimeout)||(m_CurCB.m_iBehaviour)||(m_Script.IsPaused()))
	{	// We're stunned, switch off the lightmeter
		if (m_pGameObject->ClientFlags() & PLAYER_CLIENTFLAGS_FLASHLIGHT)
		{
			m_pGameObject->ClientFlags() &= ~PLAYER_CLIENTFLAGS_FLASHLIGHT;
			CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
			if (pChar)
			{
				pChar->UpdateVisibilityFlag();
			}
		}
		return;
	}

	int32 playerID = GetClosestPlayer();
	uint8 Prio = 1;

	Intensity = GetLightIntensity(m_pGameObject->m_iObject);
	Diff = GetLightIntensityDelta();

	// Flashlight enabling/disabling
	if (Intensity < LIGHTMETER_SNEAK_THRESHOLD)
	{
		Prio++;
	}

	fp32 SqrDist = SqrDistanceToUs(playerID);
	if (SqrDist < Sqr(512.0f))
	{
		Prio++;
		if (SqrDist < Sqr(320.0f))
		{
			Prio++;
			if ((Intensity > LIGHTMETER_SNEAK_THRESHOLD)&&(SqrDist < Sqr(128.0f)))
			{	// We're pretty close to the player; use his lightmeter
				CAI_Core* pPlayerAI = GetAI(playerID);
				if (pPlayerAI)
				{
					// if (CosHeadingDiff(m_pGameObject,pPlayerAI->m_pGameObject) > 0.0f)
					{
						fp32 PlayerIntensity = pPlayerAI->GetLightIntensity(m_pGameObject->m_iObject);
						if (PlayerIntensity < Intensity)
						{
							Prio++;
							Intensity = PlayerIntensity;
							Diff = 0.0f;
						}
					}
				}
			}
		}
	}

	// AI_FLASH_LIGHT_THRESHOLD
	fp32 MaxIntensityForFlash = LIGHTMETER_SNEAK_THRESHOLD;
	if (m_FlashThresholdOverride > MaxIntensityForFlash)
	{
		MaxIntensityForFlash = m_FlashThresholdOverride;
		Diff = 0.01f;
	}

	if (m_Timer > m_FlashThresholdOverrideTimeout)
	{
		m_FlashThresholdOverrideTimeout = -1;
		m_FlashThresholdOverride = 0.0f;
	}


	if (m_pGameObject->ClientFlags() & PLAYER_CLIENTFLAGS_FLASHLIGHT)
	{	// Dead characters don't use their flashlight
		// Note, we secure the semaphore for 10 ticks as that is the periodicity
		if ((CWObject_Character::Char_GetPhysType(m_pGameObject) == PLAYER_PHYS_DEAD)||
			(!m_pAIResources->ms_FlashlightUsers.Poll(m_pGameObject->m_iObject,Prio,GetGlobalAITick(),_Duration))||
			((Intensity >= MaxIntensityForFlash)&&(Diff >= 0.0f)))
		{	// Either someone nibbed our handle or the light went up a notch
			// Switch flashlight off
			m_pGameObject->ClientFlags() &= ~PLAYER_CLIENTFLAGS_FLASHLIGHT;
			CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
			if (pChar)
			{
				pChar->UpdateVisibilityFlag();
			}
		}
	}
	else
	{	// Should we turn on the flashlight?
		// Darkness
		if ((m_DeviceStance.GetIdleStance() >= CAI_Device_Stance::IDLESTANCE_HOSTILE)&&
			((Intensity < MaxIntensityForFlash))&&
			(m_pAIResources->ms_FlashlightUsers.Poll(m_pGameObject->m_iObject,Prio,GetGlobalAITick(),_Duration)))
		{
			// Force a new measurement if flash switches ON
			if (!(m_pGameObject->ClientFlags() & PLAYER_CLIENTFLAGS_FLASHLIGHT))
			{
				if (m_UseLightmeter == 2)
				{
					MeasureLightIntensity(true);
				}
				else
				{
					MeasureLightIntensity(false);
				}
			}
			m_pGameObject->ClientFlags() |= PLAYER_CLIENTFLAGS_FLASHLIGHT;
			CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
			if (pChar)
			{
				pChar->UpdateVisibilityFlag();
			}
		}
	}
};

void CAI_Core::HandleBrokenLights(int _Duration)
{
	CVec3Dfp32 soundPos = m_KB.GetBrokenLightPos(true);
	if ((INVALID_POS(soundPos))||(!m_pGameObject)||(m_StunTimeout)||(m_Script.IsPaused()))
	{
		return;
	}

	// Get the index of the player
	int iPlayer = GetClosestPlayer();
	CAI_AgentInfo* pInfo = m_KB.GetAgentInfo(iPlayer);
	if (!pInfo)
	{	//Character is not in character list, add him as spotted
		pInfo = m_KB.AddAgent(iPlayer,CAI_AgentInfo::INFOFLAGS_NONE,CAI_AgentInfo::NONE);
	}
	if (!pInfo)
	{	// We bail out
		// (We return true as we are the only one that could possibly understand this impulse
		return;
	}

	CVec3Dfp32 listenerPos = GetBasePos();
	fp32 DistSqr = listenerPos.DistanceSqr(soundPos);
	if ((DistSqr <= Sqr(m_HearingRange))&&(pInfo->GetCurAwareness() <= CAI_AgentInfo::NOTICED))
	{	// Inhibit regular investigate noises a while
		m_DeviceSound.PauseType(CAI_Device_Sound::SEARCH_START,15.0f);
		if (pInfo->GetCurAwareness() < CAI_AgentInfo::HOSTILE)
		{
			UseRandom(CStr("Broken light"),CAI_Device_Sound::SUSPICIOUS_SOUND,CAI_Action::PRIO_ALERT);
		}
		pInfo->SetAwareness(CAI_AgentInfo::NOTICED,true,true);
		pInfo->SetSuspectedPosition(soundPos);
	}
}

void CAI_Core::OverrideFlashThreshold(fp32 _Threshold,int _Duration)
{
 	if ((m_bFlashlight)&&(m_UseLightmeter > 0))
	{
		if (_Threshold > m_FlashThresholdOverride)
		{
			if (m_UseLightmeter == 2)
			{
				MeasureLightIntensity(true);
			}
			else
			{
				MeasureLightIntensity(false);
			}
		}
		if (_Threshold > 0.0f)
		{
			HandleFlashlight(20);
			m_FlashThresholdOverrideTimeout = m_Timer + _Duration;
		}
		m_FlashThresholdOverride = _Threshold;
	}
};

// Returns the vision factor (0..1) visavi pInfo
// That is, how well WE see _iObj
fp32 CAI_Core::GetVisionFactor(int _iObj)
{
	if (!m_pGameObject || !m_pServer)
	{
		return(0.0f);
	}

	fp32 rFactor = 1.0f;
	fp32 Vis = 1.0f;
	CAI_Core* pTargetAI = GetAI(_iObj);
	if (!pTargetAI)
	{	// Handle object light
		if (m_PerceptionFlags & PERC_NIGHTVISION_FLAG)
		{
			return(1.0f);
		}
		return(0.0f);
	}

	if (!(m_PerceptionFlags & PERC_NIGHTVISION_FLAG))
	{
		Vis = m_KB.GetVisibility(pTargetAI->GetObject());
		// Darkness: No total blackness
		Vis = Max(Vis,PERCEPTIONFACTOR_MIN_LIGHT);

		// Keep this 0.0 part despite setting Min to 0.1 above, we may wish to revert this
		if (Vis == 0.0f)
		{	// Sneak or total darkness
			return(0.0f);
		}
		else if (Vis > 1.0f)
		{	// Muzzle flash etc, we ignore everything else
			return(Vis);
		}

		rFactor *= Vis;
		/*
		else if (Vis <= LIGHTMETER_SNEAK_THRESHOLD)
		{
			//rFactor *= PERCEPTIONFACTOR_SIGHT_DARKNESS;
			rFactor *= Vis;
		}
		*/

		// We break out as early as possible to reduce CPU load
		if (rFactor == 0.0f)
		{
			return(rFactor);
		}
	}

	// Target movemode
	int MoveMode = pTargetAI->GetMoveMode();
	switch(MoveMode)
	{
	case MOVEMODE_CROUCH:
		/*
		if ((Vis <= LIGHTMETER_SNEAK_THRESHOLD)&&(pTargetAI->IsPlayer()))
		{	// Furyan magical sneak mode!
			rFactor = 0.0f;
		}
		else
		*/
		{
			rFactor *= PERCEPTIONFACTOR_SIGHT_CROUCH;
		}
		break;

	case MOVEMODE_STAND:
	case MOVEMODE_LEDGE:
	case MOVEMODE_HANGRAIL:
		rFactor *= PERCEPTIONFACTOR_SIGHT_STAND;
		break;

	case MOVEMODE_WALK:
	case MOVEMODE_LADDER:
		rFactor *= PERCEPTIONFACTOR_SIGHT_WALK;
		break;

	case MOVEMODE_RUN:
		rFactor *= PERCEPTIONFACTOR_SIGHT_RUN;
		break;

	default:
		ConOut("CAI_Core::GetVisionFactor: Unknown movemode");
		break;
	}

	// We break out as early as possible to reduce CPU load
	if(rFactor == 0.0f) {return(rFactor);}
	rFactor *= pTargetAI->m_PerceptionFactor;
	// Our own movemode
	MoveMode = GetMoveMode();
	switch(MoveMode)
	{
	case MOVEMODE_CROUCH:
		rFactor *= 1.0f;
		break;

	case MOVEMODE_STAND:
		rFactor *= PERCEPTIONFACTOR_VIEW_STAND;
		break;

	case MOVEMODE_WALK:
	case MOVEMODE_LEDGE:
	case MOVEMODE_HANGRAIL:
	case MOVEMODE_LADDER:
		rFactor *= PERCEPTIONFACTOR_VIEW_WALK;
		break;

	case MOVEMODE_RUN:
		rFactor *= PERCEPTIONFACTOR_VIEW_RUN;
		break;

	default:
		ConOut("CAI_Core::GetVisionFactor: Unknown movemode");
		break;
	}

	// Modify factor for own alertness
	int Alertness = m_KB.GetAlertness();
	switch(Alertness)
	{
	case CAI_KnowledgeBase::ALERTNESS_INVALID:
	case CAI_KnowledgeBase::ALERTNESS_OBLIVIOUS:
		rFactor *= 0.0f;
		break;
	case CAI_KnowledgeBase::ALERTNESS_SLEEPING:
		rFactor *= PERCEPTIONFACTOR_VIEW_SLEEPING;
		break;
	case CAI_KnowledgeBase::ALERTNESS_DROWSY:
		rFactor *= PERCEPTIONFACTOR_VIEW_DROWSY;
		break;
	case CAI_KnowledgeBase::ALERTNESS_IDLE:
		rFactor *= PERCEPTIONFACTOR_VIEW_IDLE;
		break;
	case CAI_KnowledgeBase::ALERTNESS_WATCHFUL:
		rFactor *= PERCEPTIONFACTOR_VIEW_WATCHFUL;
		break;
	case CAI_KnowledgeBase::ALERTNESS_JUMPY:
		rFactor *= PERCEPTIONFACTOR_VIEW_WATCHFUL;
		break;
	}

	return(rFactor);
}

// Returns the hearing  factor (0..1) visavi _iObj
// That is, how well WE hear _iObj
fp32 CAI_Core::GetHearingFactor(int _iObj)
{
	if (!m_pGameObject || !m_pServer)
	{
		return(0.0f);
	}

	CAI_Core* pTargetAI = GetAI(_iObj);
	if (!pTargetAI)
	{
		return(0.0f);
	}

	fp32 rFactor = 1.0f;

	// Target movemode
	int MoveMode = pTargetAI->GetMoveMode();
	fp32 moveFactor = 0.0f;
	switch(MoveMode)
	{
	case MOVEMODE_LEDGE:
	case MOVEMODE_HANGRAIL:
	case MOVEMODE_LADDER:
	case MOVEMODE_CROUCH:
		moveFactor = PERCEPTIONFACTOR_SOUND_CROUCH;
		break;

	case MOVEMODE_STAND:
		moveFactor = PERCEPTIONFACTOR_SOUND_STAND;
		break;

	case MOVEMODE_WALK:
		moveFactor = PERCEPTIONFACTOR_SOUND_WALK;
		break;

	case MOVEMODE_RUN:
		moveFactor = PERCEPTIONFACTOR_SOUND_RUN;
		break;

	default:
		ConOut("CAI_Core::GetHearingFactor: Unknown movemode");
		break;
	}
	// Target noise?
	fp32 TargetNoise = pTargetAI->m_KB.GetNoise();
	if (TargetNoise > 1.0f)
	{	// Noise above 1.0 can only come from guns
		return(TargetNoise);
	}

	if ((!IsPlayer())&&(m_KB.GetNoise() > Max(TargetNoise,1.0f)))
	{	// We hear nothing if gun's noise is too loud
		return(0.0f);
	}
	rFactor = Max(moveFactor,TargetNoise);
	// We break out as early as possible to reduce CPU load
	if(rFactor == 0.0f) {return(rFactor);}
	rFactor *= pTargetAI->m_PerceptionFactor;

	// Our own movemode
	MoveMode = GetMoveMode();
	switch(MoveMode)
	{
	case MOVEMODE_LEDGE:
	case MOVEMODE_HANGRAIL:
	case MOVEMODE_LADDER:
	case MOVEMODE_CROUCH:
		rFactor *= 1.0f;	// No hearing reduction for crouching/ladder etc
		break;

	case MOVEMODE_STAND:
		rFactor *= PERCEPTIONFACTOR_LISTEN_STAND;
		break;

	case MOVEMODE_WALK:
		rFactor *= PERCEPTIONFACTOR_LISTEN_WALK;
		rFactor -= m_LoudMovement * 0.5f;
		if (rFactor < 0.0f) {rFactor = 0.0f;}
		break;

	case MOVEMODE_RUN:
		rFactor *= PERCEPTIONFACTOR_LISTEN_RUN;
		rFactor -= m_LoudMovement;
		if (rFactor < 0.0f) {rFactor = 0.0f;}
		break;

	default:
		ConOut("CAI_Core::GetHearingFactor: Unknown movemode");
		break;
	}
	// We break out as early as possible to reduce CPU load
	if(rFactor == 0.0f) {return(rFactor);}

	// Modify factor for own alertness
	int Alertness = m_KB.GetAlertness();
	switch(Alertness)
	{
	case CAI_KnowledgeBase::ALERTNESS_INVALID:
	case CAI_KnowledgeBase::ALERTNESS_OBLIVIOUS:
		rFactor *= 0.0f;
		break;
	case CAI_KnowledgeBase::ALERTNESS_SLEEPING:
		rFactor *= PERCEPTIONFACTOR_LISTEN_SLEEPING;
		break;
	case CAI_KnowledgeBase::ALERTNESS_DROWSY:
		rFactor *= PERCEPTIONFACTOR_LISTEN_DROWSY;
		break;
	case CAI_KnowledgeBase::ALERTNESS_IDLE:
		rFactor *= PERCEPTIONFACTOR_LISTEN_IDLE;
		break;
	case CAI_KnowledgeBase::ALERTNESS_WATCHFUL:
		rFactor *= PERCEPTIONFACTOR_LISTEN_WATCHFUL;
		break;
	case CAI_KnowledgeBase::ALERTNESS_JUMPY:
		rFactor *= PERCEPTIONFACTOR_LISTEN_WATCHFUL;
		break;
	}

	// We break out as early as possible to reduce CPU load
	if(rFactor == 0.0f) {return(rFactor);}

	// LOS?
	CAI_AgentInfo* pInfo = m_KB.GetAgentInfo(_iObj);
	if (!(m_PerceptionFlags & PERC_HEARTHROUGHWALLS_FLAG))
	{
		if (!FastCheckLOS(pInfo->GetObject()))
		{
			rFactor *= PERCEPTIONFACTOR_SOUND_LOS_BLOCKED;
		}
	}

	return(rFactor);
};

// Sets/clears inter character collisions
void CAI_Core::SetCharacterCollisions(bool _bSet)
{
	CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
	if (pChar)
	{	// Anton promises pCD will always be available so no check (Yahoo!)
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pChar);
		if ((pCD->m_iPlayer == -1)&&((!_bSet)||(_bSet != m_bCharColl)))
		{
			if (_bSet)
			{
				pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~PLAYER_PHYSFLAGS_NOCHARACTERCOLL;
				CWObject_Character::Char_SetPhysics(pChar,m_pServer,m_pServer,CWObject_Character::Char_GetPhysType(pChar),false,true);
			}
			else
			{
				pCD->m_Phys_Flags = pCD->m_Phys_Flags | PLAYER_PHYSFLAGS_NOCHARACTERCOLL;
				CWObject_Character::Char_SetPhysics(pChar,m_pServer,m_pServer,CWObject_Character::Char_GetPhysType(pChar),false,true);
			}
			m_bCharColl = _bSet; 
		}
	}
};

bool CAI_Core::CheckCollisions()
{
	bool rBool = false;

	if ((IsPlayer())||(!m_pGameObject)||(!m_pGameObject->AI_IsAlive()))
	{
		return(rBool);
	}

	if (CheckNavGridStuck())
	{
		m_bCollAvoidanceBlocked = true;
		return(rBool);
	}

	rBool = m_KB.CheckCollisions();

	return(rBool);
};

void CAI_Core::SetObjColl(bool _bFlag)
{
	if (_bFlag)
	{	// Turn ON obj collisions
		m_bObjCollRequested = true;
	}
	else
	{
		m_bObjCollRequested = false;
		CWObject_Message Msg(OBJMSG_CHAR_SETANIMPHYS,1);
		m_pServer->Message_SendToObject(Msg,GetObjectID());
	}
};

// Checks if we're stuck from lack of navgrid and if that is the case handles us moving out of the navgrid
// Returns true if we really was stuck and false otherwise, called from within CheckCollisions() above
bool CAI_Core::CheckNavGridStuck()
{
	if ((m_bBehaviourRunning)||(!m_bObjCollRequested))
	{
		return(false);
	}
#ifndef M_RTM
	if (DebugTarget())
	{
		bool wtf = true;
	}
#endif
	CVec3Dfp32 Pos = GetBasePos();
	CVec3Dfp32 PathPos = m_PathFinder.GetPathPosition(Pos,1);
	if (INVALID_POS(PathPos))
	{
		m_DeviceStance.Crouch(false);
		m_DeviceMove.Free();
		CVec3Dfp32 OurWantedMove = Pos + GetWantedWorldMove() * 8;	// One grid box
		PathPos = m_PathFinder.GetPathPosition(OurWantedMove,1);
		if (VALID_POS(PathPos))
		{	// Go there without checking nav
			AddMoveTowardsPositionCmd(PathPos,m_ReleaseIdleSpeed);
		}
		else
		{	
#ifndef M_RTM
			if (DebugTarget())
			{
				DebugDrawNavgrid(GetObjectID());
			}
#endif
			// Hold stick in a particular direction for a while
			fp32 SqrDistanceToLastValid = Pos.DistanceSqr(m_LastValidGridPos);
			if ((VALID_POS(m_LastValidGridPos))&&(SqrDistanceToLastValid <= Sqr(48.0f))&&(SqrDistanceToLastValid >= Sqr(8.0f)))
			{
				AddMoveTowardsPositionCmd(m_LastValidGridPos,m_ReleaseIdleSpeed);
			}
			else
			{
				CVec3Dfp32 lastMove = m_DeviceMove.GetLastMove();
				if ((VALID_POS(lastMove))&&(lastMove.LengthSqr() > 0.1f)&&(m_Timer % GetAITicksPerSecond() != 0))
				{
					m_DeviceMove.UseLastMove();
				}
				else
				{
					if (Random < 0.66f)
					{
						lastMove[0] = 0.5f;
					}
					else
					{
						lastMove[0] = -0.5f;
					}
					lastMove[1] = Random-0.5f;
					lastMove[2] = 0.0f;
					m_DeviceMove.Use(lastMove);
				}
			}
		}
		return(true);
	}
	else
	{
		m_LastValidGridPos = PathPos;
		return(false);
	}
};

bool CAI_Core::MoveEffectAG2()
{
	// First we test AG for sidesteps and bumps
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
	if (pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_MOVEEFFECTACTIVE)
	{	// Stick fwd until false
		ResetStuckInfo();
		ResetPathSearch();
		m_DeviceMove.Use(CVec3Dfp32(0.5f,0.0f,0.0f));
		INVALIDATE_POS(m_CollAvoidancePos);
		return(true);	
	}

	return(false);
};

bool CAI_Core::MoveCollision()
{
	if (VALID_POS(m_CollAvoidancePos))
	{	// Go there
		// Clear character 
		ResetStuckInfo();
		// Free the move device thus overriding any previous use.
		m_DeviceMove.Free();
		SetDestination(m_CollAvoidancePos);
		if (SqrDistanceToUs(m_CollAvoidancePos) > Sqr(8.0f))
		{
			if (m_AH.GetCurPrioClass() >= CAI_Action::PRIO_DANGER)
			{
				AddMoveTowardsPositionCmd(m_CollAvoidancePos,m_ReleaseMaxSpeed);
			}
			else
			{
				AddMoveTowardsPositionCmd(m_CollAvoidancePos,m_ReleaseIdleSpeed);
			}
		}
		m_DeviceMove.Use();
#ifndef M_RTM
		CVec3Dfp32 OurPos = GetBasePos();
		CVec3Dfp32 Off = CVec3Dfp32(0,0,32);
		Debug_RenderWire(OurPos+Off,m_CollAvoidancePos+Off,kColorRed,0.1f);
#endif
		if (m_Timer > m_CollisionAvoidanceCount)
		{
			INVALIDATE_POS(m_CollAvoidancePos);
			m_CollisionAvoidanceCount = 0;
			return(false);
		}
		return(true);
	}

	return(false);
};

// Returns true if we should avoid _iCollider
// Rules (descending priority)
// Our collision rank is 0:					Don't avoid
// Our collision rank is -1:				Avoid (yes, -1 coll rank avoid each other)
// Either one is dead:						Don't avoid
// Enemy relation with him:					Don't avoid
// He is running a behaviour:				Avoid
// We are running a behaviour:				Don't avoid
// We are scripted
//		He is not scripted:					Don't avoid
//		He is scripted:						Avoid
// We are crouching:						Don't avoid
// He is crouching							Avoid (ie if both crouch nobody avoids)
// He is angrier than we:					Avoid
// We are angrier than he:					Don't avoid
// We have higher collision rank than he:	Don't avoid
// We have equal collision rank
//		He has higher obj nbr:				Avoid
// Nothing									Avoid
bool CAI_Core::ShouldAvoid(int _iCollider)
{
	if (m_CollisionRank)
	{
		if (m_CollisionRank == -1)
		{
			return(true);
		}

		CAI_AgentInfo* pAgent = m_KB.GetAgentInfo(_iCollider);
		CAI_Core* pHisAI = GetAI(_iCollider);
		if ((!pHisAI)||(!m_pGameObject)||(!pHisAI->m_pGameObject)||((m_CharacterClass != CLASS_DARKLING)&&(pHisAI->IsPlayer())))
		{
			return(false);
		}
		// NO! Never collision avoid unspawned dudes
		if (!pHisAI->m_pGameObject->AI_IsSpawned())
		{
			return(false);
		}
		if ((m_CharacterClass == CLASS_CIV)&&(!pHisAI->m_pGameObject->AI_IsAlive()))
		{	// Avoid dead guys
			return(true);
		}
		if ((!m_pGameObject->AI_IsAlive())||
			(!pHisAI->m_pGameObject->AI_IsAlive())||
			(!pHisAI->IsConscious()))
		{
			return(false);
		}
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
		if (!pCD || (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE))
		{
			return(false);
		}
		CWO_Character_ClientData* pHisCD = CWObject_Character::GetClientData(pHisAI->m_pGameObject);
		if (!pHisCD || (pHisCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE))
		{
			return(true);
		}

		if ((pAgent)&&(pAgent->GetCurRelation() >= CAI_AgentInfo::ENEMY))
		{
			return(false);
		}
		// Door scenepoints take precedence
		if ((m_Script.GetDoorScenepoint())&&(!pHisAI->m_Script.GetDoorScenepoint()))
		{
			return(false);
		}
		else
		{
			if (pHisAI->m_Script.GetDoorScenepoint())
			{
				return(true);
			}
		}
		if ((pHisAI->m_bBehaviourRunning)||(pHisAI->m_CurCB.m_iBehaviour))
		{
			return(true);
		}
		if ((m_bBehaviourRunning)||(m_CurCB.m_iBehaviour))
		{
			return(false);
		}

		if (m_Script.IsValid())
		{
			if (!pHisAI->m_Script.IsValid())
			{
				return(false);
			}
		}
		else
		{
			if (pHisAI->m_Script.IsValid())
			{
				return(true);
			}
		}
		if (m_DeviceStance.IsCrouching())
		{
			return(false);
		}
		else if (pHisAI->m_DeviceStance.IsCrouching())
		{
			return(true);
		}
		// Darklings avoid each other
		if ((m_CharacterClass == CLASS_DARKLING)&&(pHisAI->m_CharacterClass == CLASS_DARKLING))
		{
			return(true);
		}
		if ((m_CharacterClass == CLASS_DARKLING)&&(pHisAI->IsPlayer()))
		{
			SetCharacterCollisions(false);
			pHisAI->SetCharacterCollisions(false);
			return(true);
		}
		if ((pHisAI->m_bCollAvoidanceBlocked)||(pHisAI->m_bIsStuck))
		{
			return(true);
		}
		if (pHisAI->m_DeviceStance.GetIdleStance() > pHisAI->m_DeviceStance.GetIdleStance())
		{
			return(true);
		}
		else if (pHisAI->m_DeviceStance.GetIdleStance() < pHisAI->m_DeviceStance.GetIdleStance())
		{
			return(false);
		}
		if (m_CollisionRank > pHisAI->m_CollisionRank)
		{
			return(false);
		}
		else if (m_CollisionRank < pHisAI->m_CollisionRank)
		{
			return(true);
		}

		if (m_CollisionRank == pHisAI->m_CollisionRank)
		{
			if (pHisCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOLOOK)
			{
				return(true);
			}
			if (pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOLOOK)
			{
				return(false);
			}
			if (m_pGameObject->m_iObject < pHisAI->m_pGameObject->m_iObject)
			{
				return(false);
			}
		}
		return(true);
	}
	else
	{
		return(false);
	}
};

#define AI_COLL_AVOID_TICKS		20
bool CAI_Core::HandleCollision(uint16 _iColl,uint16 _iColl2,fp32 _RangeSqr)
{
	CAI_Core* pHisAI = GetAI(_iColl);
	CAI_Core* pHisAI2 = GetAI(_iColl2);

	if ((!m_CollisionRank)||(!pHisAI)||(!m_pGameObject)||(!pHisAI->m_pGameObject))
	{
		INVALIDATE_POS(m_CollAvoidancePos);
		m_CollisionAvoidanceCount = 0;
		m_bCollAvoidanceBlocked = false;
		m_bCollCWBlocked = false;
		m_bCollCCWBlocked = false;
		return(false);
	}

	// Are we close enough to risk colliding?
	CWObject_Character* pOurChar = CWObject_Character::IsCharacter(m_pGameObject);
	if (!pOurChar)
	{	
		INVALIDATE_POS(m_CollAvoidancePos);
		m_CollisionAvoidanceCount = 0;
		m_bCollAvoidanceBlocked = false;
		m_bCollCWBlocked = false;
		m_bCollCCWBlocked = false;
		return(false);
	}

	CWO_Character_ClientData* pOurCD = CWObject_Character::GetClientData(pOurChar);
	bool bHeIsDead = false;
	CWObject_Character* pHisChar = CWObject_Character::IsCharacter(pHisAI->m_pGameObject);
	if (!pHisChar)
	{	
		if ((!pHisAI->m_pGameObject->AI_IsAlive())&&(pHisAI->m_pGameObject->IsClass(class_CWObject_CharNPC)))
		{	// Dead? We still avoid the bastard
			bHeIsDead = true;
			pHisAI->m_CollisionRank = 0;
			pHisChar = safe_cast<CWObject_Character>(pHisAI->m_pGameObject);
		}
		else
		{
			INVALIDATE_POS(m_CollAvoidancePos);
			m_CollisionAvoidanceCount = 0;
			m_bCollAvoidanceBlocked = false;
			m_bCollCWBlocked = false;
			m_bCollCCWBlocked = false;
			return(false);
		}
	}
	CWO_Character_ClientData* pHisCD = CWObject_Character::GetClientData(pHisChar);

	// NOTE
	// We measure offset from HisPos to OurPos ie he is at origo
	CVec3Dfp32 OurPos = GetBasePos();
	CVec3Dfp32 HisPos = pHisAI->GetBasePos();
	if (!m_bCollAvoidanceBlocked)
	{	// Don't predict if he is avoiding us too
		HisPos += pHisAI->GetPrevVelocity() * 5.0f;
	}
	fp32 OurRadius = (fp32)pOurCD->m_Phys_Width;
	fp32 HisRadius = (fp32)pHisCD->m_Phys_Width;
	if (bHeIsDead)
	{
		HisRadius *= 2.0f;
	}
	fp32 MinRange = (OurRadius + HisRadius) * 1.25f;
	fp32 MidRange = (OurRadius + HisRadius) * 2.0f;
	fp32 MaxRange = (OurRadius + HisRadius) * 2.5f;
	if (pHisAI->m_bBehaviourRunning)
	{
		MinRange = (OurRadius + HisRadius) * 1.25f;
		MidRange = (OurRadius + HisRadius) * 1.75f;
		MaxRange = (OurRadius + HisRadius) * 2.0f;
	}

	CVec3Dfp32 Offset = OurPos - HisPos;
	Offset[2] = 0.0f;
	CVec3Dfp32 OffN = Offset;

	fp32 Range = Offset.Length();
	if (Range > MaxRange)
	{	// Too far away
		INVALIDATE_POS(m_CollAvoidancePos);
		m_CollisionAvoidanceCount = 0;
		m_bCollAvoidanceBlocked = false;
		m_bCollCWBlocked = false;
		m_bCollCCWBlocked = false;
		return(false);
	}
	else
	{
		SetCharacterCollisions(false);
		pHisAI->SetCharacterCollisions(false);
		if (!pHisAI->m_KB.GetAgentInfo(GetObjectID()))
		{
			pHisAI->m_KB.AddAgent(GetObjectID(),CAI_AgentInfo::INFOFLAGS_NONE,CAI_AgentInfo::SPOTTED);
		}
	}

	// We ARE colliding with someone
	if (m_KB.DidCollAgentChange())
	{
		m_DeviceLook.Free();
		OnTrackObj(_iColl,10,false,true);
	}

	if (_iColl2)
	{
		CVec3Dfp32 Coll2Pos = GetBasePos(_iColl2);
		if ((VALID_POS(Coll2Pos))&&(HisPos.DistanceSqr(Coll2Pos) <= Sqr(MaxRange))&&(SqrDistanceToUs(Coll2Pos) < Sqr(MaxRange)))
		{	// Is Coll2Pos to left or right of OffN?
			m_bCollAvoidanceBlocked = true;
			CVec3Dfp32 OffLeftN(OffN[1],-OffN[0],0.0f);
			if ((Coll2Pos-HisPos) * OffLeftN >= 0.0f)
			{
				if (!m_bCollCCWBlocked)
				{
					m_bCollCWBlocked = true;
				}
			}
			else
			{
				if (!m_bCollCWBlocked)
				{
					m_bCollCCWBlocked = true;
				}
			}
		}
	}

	if ((m_bCollCWBlocked)&&(m_bCollCCWBlocked))
	{
		m_bCollAvoidanceBlocked = true;
		if (Range < MinRange)
		{
			m_CollAvoidancePos = HisPos + OffN * MidRange;
			m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
			if ((OurPos.DistanceSqr(m_CollAvoidancePos) <= Sqr(8.0f)))
			{
				m_CollAvoidancePos = HisPos + OffN * MaxRange;
				m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
			}
		}
		else
		{
			// We try to squeeze in
			if ((INVALID_POS(m_CollAvoidancePos))&&(!m_CollisionAvoidanceCount))
			{
				INVALIDATE_POS(m_CollAvoidancePos);
				m_CollisionAvoidanceCount = 0;
				m_bCollAvoidanceBlocked = true;
				return(true);
			}
			m_bCollAvoidanceBlocked = true;
		}
	}

	OffN.Normalize();


	// Determine goal positions for us and him
	// First we take the m_SubDest and if that is invalid we use m_PathDestination
	// If none of these are available we use prevvelocity and if that is unavailable or zero we
	// give up and use separation as our velocities
	CVec3Dfp32 OurWantedMove = GetWantedWorldMove();
	CVec3Dfp32 OurV = CVec3Dfp32(0.0f,0.0f,0.0f);
	CVec3Dfp32 OurDest = HisPos + OffN * MidRange;
	int OurClosest8 = GetOctantFromOff(OffN);
	int OurDest8 = OurClosest8;
	// We're moving somewhere
	if ((OurWantedMove.LengthSqr() > Sqr(0.1f))&&(VALID_POS(m_SubDest))&&(m_Path.IsValid()))
	{	// Our subdest is beyond coll or our pathdestination is
		OurDest = m_Path.PropelPathToFirstNodeOutsideRange(HisPos,MidRange,true);
		if (VALID_POS(OurDest))
		{
			OurV = OurDest - OurPos;
			OurDest8 = GetOctantFromOff(OurDest-HisPos);
			if (Abs(OurDest8-OurClosest8) <= 1)
			{	// Move straight away
				ResetStuckInfo();
				INVALIDATE_POS(m_CollAvoidancePos);
				m_CollisionAvoidanceCount = 0;
				return(true);
			}
		}
		else
		{	// Our actual destination is inside MaxRange, we cannot do much but stay put
			INVALIDATE_POS(m_CollAvoidancePos);
			m_CollisionAvoidanceCount = 0;
			m_bCollAvoidanceBlocked = true;
			m_bCollCWBlocked = false;
			m_bCollCCWBlocked = false;
			//return(true);
		}
	}
	if (OurV.LengthSqr() <= Sqr(8.0f))
	{
		OurV = OurWantedMove;
		OurDest8 = GetOctantFromOff(OurV);
	}
	if (OurV.LengthSqr() <= Sqr(0.1f))
	{
		OurV = OffN;
		OurDest8 = GetOctantFromOff(OffN);
	}
	OurV.Normalize();
	OurV = OurV * GetMaxSpeedForward();

	CVec3Dfp32 HisV = CVec3Dfp32(0.0f,0.0f,0.0f);
	int HisDest8 = 7 - OurDest8;
	if ((VALID_POS(pHisAI->m_SubDest))&&(pHisAI->GetPrevSpeed() > 0.1f))
	{
		HisV = pHisAI->m_SubDest - HisPos;
		HisDest8 = GetOctantFromOff(pHisAI->m_SubDest-HisPos);
	}
	if (HisV.LengthSqr() <= Sqr(8.0f))
	{
		HisV = pHisAI->GetPrevVelocity();
		HisDest8 = GetOctantFromOff(HisV);
	}
	if (HisV.LengthSqr() <= Sqr(0.1f))
	{
		// HisV = OffN;
		HisDest8 = GetOctantFromOff(OffN);
	}
	else
	{
		HisV.Normalize();
		HisV = HisV * pHisAI->GetMaxSpeedForward();
	}

#ifndef M_RTM
	if ((DebugTarget())||(!DebugTargetSet()))
	{	// Draw a circle
		CVec3Dfp32 Up = CVec3Dfp32(0.0f,0.0f,8.0f);
		CVec3Dfp32 OldDraw, NewDraw, Off;
		fp32 CircleRadius = MaxRange;
		NewDraw = HisPos;
		Off = GetOffFromOctant(7);
		NewDraw += Off * CircleRadius;
		NewDraw += Up;
		int32 Color = kColorBlue;
		if (Range <= MinRange)
		{
			Color = kColorRed;
		}
		for (int i = 0; i < 8; i++)
		{
			OldDraw = NewDraw;
			NewDraw = HisPos;
			Off = GetOffFromOctant(i);
			NewDraw += Off * CircleRadius;
			NewDraw += Up;
			Debug_RenderWire(OldDraw,NewDraw,Color,2.0f);
		}
		if ((VALID_POS(m_CollAvoidancePos))&&(m_CollisionAvoidanceCount))
		{	
			Debug_RenderVertex(m_CollAvoidancePos, Color,0.5f);
			Debug_RenderWire(OurPos,m_CollAvoidancePos,kColorRed,0.5f);
		}
	}
#endif

	if ((OurV.LengthSqr() < Sqr(0.1f))&&(HisV.LengthSqr() < Sqr(0.1f)))
	{	// Standing still
		if (DebugTarget())
		{
			ConOutL("We and he are standing still");
		}
		INVALIDATE_POS(m_CollAvoidancePos);
		m_CollisionAvoidanceCount = 0;
		m_bCollAvoidanceBlocked = false;
		m_bCollCWBlocked = false;
		m_bCollCCWBlocked = false;
		return(false);
	}

	OurPos = m_PathFinder.SafeGetPathPosition(OurPos,2,1);
	HisPos = m_PathFinder.SafeGetPathPosition(HisPos,2,1);
	m_bCollAvoidanceBlocked = false;

	// We don't want to move on our own so OurDest8 means nuthin'
	if (OurWantedMove.LengthSqr() <= Sqr(0.1f))
	{	// We want to stand still, just get straight outta there laddie
		if (Range < MinRange)
		{
			m_CollAvoidancePos = HisPos + OffN * MidRange;
			m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
			if ((OurPos.DistanceSqr(m_CollAvoidancePos) <= Sqr(8.0f)))
			{
				m_CollAvoidancePos = HisPos + OffN * MaxRange;
				m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
			}
		}
		else
		{
			if (!bHeIsDead)
			{
				m_DeviceMove.Free();
				m_DeviceMove.Use();
				ResetStuckInfo();
			}
			INVALIDATE_POS(m_CollAvoidancePos);
			m_CollisionAvoidanceCount = 0;
			m_bCollAvoidanceBlocked = false;
			m_bCollCWBlocked = false;
			m_bCollCCWBlocked = false;
			return(false);
		}
	}

	if (DebugTarget())
	{
		bool wtf = true;
	}

	// Bad, move straight away and if blocked we tell him so
	if (Range <= MinRange)
	{
		m_CollAvoidancePos = HisPos + OffN * MaxRange;
		m_CollAvoidancePos = m_PathFinder.SafeGetPathPosition(m_CollAvoidancePos,2,1);
		CVec3Dfp32 PosTemp = m_CollAvoidancePos;
		if (!m_PathFinder.GroundTraversable(OurPos,m_CollAvoidancePos,&PosTemp))
		{
			if (HisPos.DistanceSqr(PosTemp) >= Sqr(MidRange * 0.5f))
			{
				m_CollAvoidancePos = PosTemp;
			}
			else
			{
				// We cheat here by not pushing him away (if the below line is commented out that is)
				// m_bCollAvoidanceBlocked = true;
				if (!bHeIsDead)
				{
					m_DeviceMove.Free();
					m_DeviceMove.Use();
					ResetStuckInfo();
				}
				INVALIDATE_POS(m_CollAvoidancePos);
				m_CollisionAvoidanceCount = 0;
				return(true);
			}
		}
		m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
		ResetStuckInfo();
	}

	// TODO: Look at this and do some OurWantedMove tangent magic
	if ((Range > MinRange)&&((VALID_POS(m_PathDestination))&&(HisPos.DistanceSqr(m_PathDestination) < Sqr(MaxRange))))
	{	// Our goal is inside collision radius
		if (DebugTarget())
		{
			ConOutL("m_SubDest inside coll radius, stay put");
		}
		if (!bHeIsDead)
		{
			m_DeviceMove.Free();
			m_DeviceMove.Use();
			ResetStuckInfo();
		}
		INVALIDATE_POS(m_CollAvoidancePos);
		m_CollisionAvoidanceCount = 0;
		return(true);
	}

	// If we're moving away from him just keep on trucking
	if ((!m_CollisionAvoidanceCount)&&(OurWantedMove * OffN > 0.0f))
	{
		if (DebugTarget())
		{
			ConOutL("Moving away, just continue");
		}
		ResetStuckInfo();
		INVALIDATE_POS(m_CollAvoidancePos);
		m_CollisionAvoidanceCount = 0;
		m_bCollAvoidanceBlocked = false;
		return(true);
	}

	if ((INVALID_POS(m_CollAvoidancePos))||(!m_CollisionAvoidanceCount)||(OurPos.DistanceSqr(m_CollAvoidancePos) <= Sqr(8.0f)))
	{	// Beyond MinRange
		if ((VALID_POS(m_PathDestination))&&(HisPos.DistanceSqr(m_PathDestination) <= Sqr(MinRange))&&(Range > MinRange))
		{	// Stop and do nothing when our goal is inside the other
			if (DebugTarget())
			{
				ConOutL("m_PathDestination inside coll radius, stay put");
			}
			if (!bHeIsDead)
			{
				m_DeviceMove.Free();
				m_DeviceMove.Use();
				ResetStuckInfo();
			}
			INVALIDATE_POS(m_CollAvoidancePos);
			m_CollisionAvoidanceCount = 0;
			return(true);
		}

		if ((OurDest8 == OurClosest8)&&(true))
		{	// Do nothing
			INVALIDATE_POS(m_CollAvoidancePos);
			m_CollisionAvoidanceCount = 0;
			m_bCollAvoidanceBlocked = false;
			m_bCollCWBlocked = false;
			m_bCollCCWBlocked = false;
			return(false);
		}

		// Which way is the closest to our goal? OurClosest8+1 or OurClosest8-1?
		CVec3Dfp32 CCW = HisPos + GetOffFromOctant((OurClosest8+1) % 8) * MidRange;
		CCW = m_PathFinder.SafeGetPathPosition(CCW,2,1);
		CVec3Dfp32 CW = HisPos + GetOffFromOctant((8+OurClosest8-1) % 8) * MidRange;
		CW = m_PathFinder.SafeGetPathPosition(CW,2,1);
		if ((!m_bCollCCWBlocked)&&((m_bCollCWBlocked)||(OurDest.DistanceSqr(CCW) <= OurDest.DistanceSqr(CW))))
		{
			if (DebugTarget())
			{
				ConOutL("Go CCW");
			}
			if (m_PathFinder.GroundTraversable(OurPos,CCW,&CCW))
			{
				m_CollAvoidancePos = CCW;
				m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
				m_bCollAvoidanceBlocked = false;
				m_bCollCCWBlocked = false;
				ResetStuckInfo();
			}
			else
			{	// Stay put
				m_bCollCCWBlocked = true;
				if ((!m_bCollCWBlocked)&&(m_PathFinder.GroundTraversable(OurPos,CW,&CW)))
				{
					m_CollAvoidancePos = CW;
					m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
					m_bCollAvoidanceBlocked = false;
					m_bCollCWBlocked = false;
					ResetPathSearch();
				}
				else
				{
					if (!bHeIsDead)
					{
						m_DeviceMove.Free();
						m_DeviceMove.Use();
						ResetStuckInfo();
					}
					INVALIDATE_POS(m_CollAvoidancePos);
					m_CollisionAvoidanceCount = 0;
				}
			}
		}
		else
		{
			if (DebugTarget())
			{
				ConOutL("Go CW");
			}
			if (m_PathFinder.GroundTraversable(OurPos,CW,&CW))
			{
				m_CollAvoidancePos = CW;
				m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
				m_bCollAvoidanceBlocked = false;
				m_bCollCWBlocked = false;
				ResetPathSearch();
			}
			else
			{	// Stay put
				m_bCollCWBlocked = true;
				if ((!m_bCollCCWBlocked)&&(m_PathFinder.GroundTraversable(OurPos,CCW,&CCW)))
				{
					m_CollAvoidancePos = CCW;
					m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
					m_bCollAvoidanceBlocked = false;
					m_bCollCCWBlocked = false;
					ResetStuckInfo();
				}
				else
				{
					if (!bHeIsDead)
					{
						m_DeviceMove.Free();
						m_DeviceMove.Use();
						ResetStuckInfo();
					}
					INVALIDATE_POS(m_CollAvoidancePos);
					m_CollisionAvoidanceCount = 0;
				}
			}
		}
	}

	// Always finish our avoidance move
	if ((VALID_POS(m_CollAvoidancePos))&&(m_CollisionAvoidanceCount))
	{
		if (SqrDistanceToUs(m_CollAvoidancePos) <= Sqr(8.0f))
		{	// Stay put
			m_DeviceMove.Free();
			m_DeviceMove.Use();
			ResetStuckInfo();
			return(true);
		}
		else
		{	// Go there
			ResetStuckInfo();
			m_DeviceMove.Free();
			AddMoveTowardsPositionCmd(m_CollAvoidancePos,m_ReleaseIdleSpeed);
			ResetPathSearch();
		}
		m_CollisionAvoidanceCount--;
		if (m_CollisionAvoidanceCount <= 0)
		{
			INVALIDATE_POS(m_CollAvoidancePos);
			m_CollisionAvoidanceCount = 0;
			m_bCollAvoidanceBlocked = false;
		}
	}

	return(true);
}

// Helper, returns the octnat of _Off
int CAI_Core::GetOctantFromOff(CVec3Dfp32 _Off)
{
	int Octant = 0;
	if (_Off[0] >= 0.0f)
	{
		if (_Off[1] >= 0.0f)
		{
			if (Abs(_Off[0]) >= Abs(_Off[1]))
			{
				Octant = 0;
			}
			else
			{
				Octant = 1;
			}
		}
		else
		{
			if (Abs(_Off[0]) >= Abs(_Off[1]))
			{
				Octant = 7;
			}
			else
			{
				Octant = 6;
			}
		}

	}
	else
	{
		if (_Off[1] >= 0.0f)
		{
			if (Abs(_Off[0]) >= Abs(_Off[1]))
			{
				Octant = 3;
			}
			else
			{
				Octant = 2;
			}
		}
		else
		{
			if (Abs(_Off[0]) >= Abs(_Off[1]))
			{
				Octant = 4;
			}
			else
			{
				Octant = 5;
			}
		}
	}

	return(Octant);
};
// Helper, returns the octnat of _Off
CVec3Dfp32 CAI_Core::GetOffFromOctant(int _iOctant)
{
#define SIN_225 0.3826834324f
#define COS_225 0.9238795325f

	CVec3Dfp32 Off;
	Off[2] = 0.0f;
	switch(_iOctant)
	{
	case 0:
		Off[0] = COS_225;
		Off[1] = SIN_225;
		break;
	case 1:
		Off[0] = SIN_225;
		Off[1] = COS_225;
		break;
	case 2:
		Off[0] = -SIN_225;
		Off[1] = COS_225;
		break;
	case 3:
		Off[0] = -COS_225;
		Off[1] = SIN_225;
		break;
	case 4:
		Off[0] = -COS_225;
		Off[1] = -SIN_225;
		break;
	case 5:
		Off[0] = -SIN_225;
		Off[1] = -COS_225;
		break;
	case 6:
		Off[0] = SIN_225;
		Off[1] = -COS_225;
		break;
	case 7:
		Off[0] = COS_225;
		Off[1] = -SIN_225;
		break;
	default:
		INVALIDATE_POS(Off);
		break;
	}

	return(Off);
};

bool CAI_Core::HandleCollisionNew(uint16 _iColl,uint16 _iColl2,fp32 _RangeSqr)
{
	CAI_Core* pHisAI = GetAI(_iColl);
	CAI_Core* pHisAI2 = GetAI(_iColl2);

	if ((!m_CollisionRank)||(!pHisAI)||(!m_pGameObject)||(!pHisAI->m_pGameObject))
	{
		INVALIDATE_POS(m_CollAvoidancePos);
		m_CollisionAvoidanceCount = 0;
		m_bCollAvoidanceBlocked = false;
		m_bCollCWBlocked = false;
		m_bCollCCWBlocked = false;
		return(false);
	}

	// Are we close enough to risk colliding?
	CWObject_Character* pOurChar = CWObject_Character::IsCharacter(m_pGameObject);
	if (!pOurChar)
	{	
		INVALIDATE_POS(m_CollAvoidancePos);
		m_CollisionAvoidanceCount = 0;
		m_bCollAvoidanceBlocked = false;
		m_bCollCWBlocked = false;
		m_bCollCCWBlocked = false;
		return(false);
	}

	CWO_Character_ClientData* pOurCD = CWObject_Character::GetClientData(pOurChar);
	bool bHeIsDead = false;
	CWObject_Character* pHisChar = CWObject_Character::IsCharacter(pHisAI->m_pGameObject);
	if (!pHisChar)
	{	
		if ((!pHisAI->m_pGameObject->AI_IsAlive())&&(pHisAI->m_pGameObject->IsClass(class_CWObject_CharNPC)))
		{	// Dead? We still avoid the bastard
			bHeIsDead = true;
			pHisAI->m_CollisionRank = 0;
			pHisChar = safe_cast<CWObject_Character>(pHisAI->m_pGameObject);
		}
		else
		{
			INVALIDATE_POS(m_CollAvoidancePos);
			m_CollisionAvoidanceCount = 0;
			m_bCollAvoidanceBlocked = false;
			m_bCollCWBlocked = false;
			m_bCollCCWBlocked = false;
			return(false);
		}
	}
	CWO_Character_ClientData* pHisCD = CWObject_Character::GetClientData(pHisChar);

	// NOTE
	// We measure offset from HisPos to OurPos ie he is at origo
	CVec3Dfp32 OurPos = GetBasePos();
	CVec3Dfp32 HisPos = pHisAI->GetBasePos();
	if (!m_bCollAvoidanceBlocked)
	{	// Don't predict if he is avoiding us too
		HisPos += pHisAI->GetPrevVelocity() * 5.0f;
	}
	fp32 OurRadius = (fp32)pOurCD->m_Phys_Width;
	fp32 HisRadius = (fp32)pHisCD->m_Phys_Width;
	if (bHeIsDead)
	{
		HisRadius *= 2.0f;
	}
	fp32 MinRange = (OurRadius + HisRadius) * 1.25f;
	fp32 MidRange = (OurRadius + HisRadius) * 2.0f;
	fp32 MaxRange = (OurRadius + HisRadius) * 2.5f;
	if (pHisAI->m_bBehaviourRunning)/*||(pHisAI->GetPrevSpeed() < 0.1f))*/
	{
		MinRange = (OurRadius + HisRadius) * 1.25f;
		MidRange = (OurRadius + HisRadius) * 1.75f;
		MaxRange = (OurRadius + HisRadius) * 2.0f;
	}

	CVec3Dfp32 Offset = OurPos - HisPos;
	Offset[2] = 0.0f;
	CVec3Dfp32 OffN = Offset;

	fp32 Range = Offset.Length();
	if (Range > MaxRange)
	{	// Too far away
		INVALIDATE_POS(m_CollAvoidancePos);
		m_CollisionAvoidanceCount = 0;
		m_bCollAvoidanceBlocked = false;
		m_bCollCWBlocked = false;
		m_bCollCCWBlocked = false;
		return(false);
	}
	else
	{
		SetCharacterCollisions(false);
		pHisAI->SetCharacterCollisions(false);
		if (!pHisAI->m_KB.GetAgentInfo(GetObjectID()))
		{
			pHisAI->m_KB.AddAgent(GetObjectID(),CAI_AgentInfo::INFOFLAGS_NONE,CAI_AgentInfo::SPOTTED);
		}
	}

	// We ARE colliding with someone
	if (m_KB.DidCollAgentChange())
	{
		m_DeviceLook.Free();
		OnTrackObj(_iColl,10,false,true);
	}

	if (DebugTarget())
	{
		bool wtf = true;
	}

	if (_iColl2)
	{
		CVec3Dfp32 Coll2Pos = GetBasePos(_iColl2);
		if ((VALID_POS(Coll2Pos))&&(HisPos.DistanceSqr(Coll2Pos) <= Sqr(MaxRange))&&(SqrDistanceToUs(Coll2Pos) < Sqr(MaxRange)))
		{	// Is Coll2Pos to left or right of OffN?
			m_bCollAvoidanceBlocked = true;
			CVec3Dfp32 OffLeftN(OffN[1],-OffN[0],0.0f);
			if ((Coll2Pos-HisPos) * OffLeftN >= 0.0f)
			{
				if (!m_bCollCCWBlocked)
				{
					m_bCollCWBlocked = true;
				}
			}
			else
			{
				if (!m_bCollCWBlocked)
				{
					m_bCollCCWBlocked = true;
				}
			}
		}
	}

	if ((m_bCollCWBlocked)&&(m_bCollCCWBlocked))
	{
		m_bCollAvoidanceBlocked = true;
		if (Range < MinRange)
		{
			m_CollAvoidancePos = HisPos + OffN * MidRange;
			m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
			if ((OurPos.DistanceSqr(m_CollAvoidancePos) <= Sqr(8.0f)))
			{
				m_CollAvoidancePos = HisPos + OffN * MaxRange;
				m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
			}
			// We may even suggest he turn his behaviour off!
			if ((pHisAI->m_bBehaviourRunning)&&(pHisAI->m_BehaviourPrioClass <= CAI_Action::PRIO_IDLE))
			{
				pHisAI->StopBehaviour(BEHAVIOUR_STOP_NORMAL,CAI_Action::PRIO_IDLE);
			}
		}
		else
		{
			// We try to squeeze in
			if ((INVALID_POS(m_CollAvoidancePos))&&(!m_CollisionAvoidanceCount))
			{
				INVALIDATE_POS(m_CollAvoidancePos);
				m_CollisionAvoidanceCount = 0;
				m_bCollAvoidanceBlocked = true;
				return(true);
			}
			m_bCollAvoidanceBlocked = true;
		}
	}

	OffN.Normalize();


	// Determine goal positions for us and him
	// First we take the m_SubDest and if that is invalid we use m_PathDestination
	// If none of these are available we use prevvelocity and if that is unavailable or zero we
	// give up and use separation as our velocities
	CVec3Dfp32 OurWantedMove = GetWantedWorldMove();
	CVec3Dfp32 OurV = CVec3Dfp32(0.0f,0.0f,0.0f);
	CVec3Dfp32 OurDest = HisPos + OffN * MidRange;
	int OurClosest8 = GetOctantFromOff(OffN);
	int OurDest8 = OurClosest8;
	// We're moving somewhere
	if ((OurWantedMove.LengthSqr() > Sqr(0.1f))&&(VALID_POS(m_SubDest))&&(m_Path.IsValid()))
	{	// Our subdest is beyond coll or our pathdestination is
		OurDest = m_Path.PropelPathToFirstNodeOutsideRange(HisPos,MidRange,true);
		if (VALID_POS(OurDest))
		{
			OurV = OurDest - OurPos;
			OurDest8 = GetOctantFromOff(OurDest-HisPos);
			if (Abs(OurDest8-OurClosest8) <= 2)
			{	// Move straight away
				ResetStuckInfo();
				INVALIDATE_POS(m_CollAvoidancePos);
				m_CollisionAvoidanceCount = 0;
				return(true);
			}
		}
		else
		{	// Our actual destination is inside MaxRange, we cannot do much but stay put
			if (DebugTarget())
			{
				ConOutL("Dest inside coll radius");
			}
			//INVALIDATE_POS(m_CollAvoidancePos);
			//m_CollisionAvoidanceCount = 0;
			m_bCollAvoidanceBlocked = true;
			//m_bCollCWBlocked = false;
			//m_bCollCCWBlocked = false;
			return(true);
		}
	}
	if (OurV.LengthSqr() <= Sqr(8.0f))
	{
		OurV = OurWantedMove;
		OurDest8 = GetOctantFromOff(OurV);
	}
	if (OurV.LengthSqr() <= Sqr(0.1f))
	{
		OurV = OffN;
		OurDest8 = GetOctantFromOff(OffN);
	}
	OurV.Normalize();
	OurV = OurV * GetMaxSpeedForward();

	CVec3Dfp32 HisV = CVec3Dfp32(0.0f,0.0f,0.0f);
	int HisDest8 = 7 - OurDest8;
	if ((VALID_POS(pHisAI->m_SubDest))&&(pHisAI->GetPrevSpeed() > 0.1f))
	{
		HisV = pHisAI->m_SubDest - HisPos;
		HisDest8 = GetOctantFromOff(pHisAI->m_SubDest-HisPos);
	}
	if (HisV.LengthSqr() <= Sqr(8.0f))
	{
		HisV = pHisAI->GetPrevVelocity();
		HisDest8 = GetOctantFromOff(HisV);
	}
	if (HisV.LengthSqr() <= Sqr(0.1f))
	{
		// HisV = OffN;
		HisDest8 = GetOctantFromOff(OffN);
	}
	else
	{
		HisV.Normalize();
		HisV = HisV * pHisAI->GetMaxSpeedForward();
	}

#ifndef M_RTM
	if ((DebugTarget())||(!DebugTargetSet()))
	{	// Draw a circle
		CVec3Dfp32 Up = CVec3Dfp32(0.0f,0.0f,8.0f);
		CVec3Dfp32 OldDraw, NewDraw, Off;
		fp32 CircleRadius = MaxRange;
		NewDraw = HisPos;
		Off = GetOffFromOctant(7);
		NewDraw += Off * CircleRadius;
		NewDraw += Up;
		int32 Color = kColorBlue;
		if (Range <= MinRange)
		{
			Color = kColorRed;
		}
		for (int i = 0; i < 8; i++)
		{
			OldDraw = NewDraw;
			NewDraw = HisPos;
			Off = GetOffFromOctant(i);
			NewDraw += Off * CircleRadius;
			NewDraw += Up;
			Debug_RenderWire(OldDraw,NewDraw,Color,2.0f);
		}
		if ((VALID_POS(m_CollAvoidancePos))&&(m_CollisionAvoidanceCount))
		{	
			Debug_RenderVertex(m_CollAvoidancePos, Color,0.5f);
			Debug_RenderWire(OurPos,m_CollAvoidancePos,kColorRed,0.5f);
		}
	}
#endif

	if ((OurV.LengthSqr() < Sqr(0.1f))&&(HisV.LengthSqr() < Sqr(0.1f)))
	{	// Standing still
		if (DebugTarget())
		{
			ConOutL("We and he are standing still");
		}
		INVALIDATE_POS(m_CollAvoidancePos);
		m_CollisionAvoidanceCount = 0;
		m_bCollAvoidanceBlocked = false;
		m_bCollCWBlocked = false;
		m_bCollCCWBlocked = false;
		return(false);
	}

	OurPos = m_PathFinder.SafeGetPathPosition(OurPos,2,1);
	HisPos = m_PathFinder.SafeGetPathPosition(HisPos,2,1);
	m_bCollAvoidanceBlocked = false;

	// We don't want to move on our own so OurDest8 means nuthin'
	if (OurWantedMove.LengthSqr() <= Sqr(0.1f))
	{	// We want to stand still, just get straight outta there laddie
		if (Range < MinRange)
		{
			m_CollAvoidancePos = HisPos + OffN * MidRange;
			m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
			if ((OurPos.DistanceSqr(m_CollAvoidancePos) <= Sqr(8.0f)))
			{
				m_CollAvoidancePos = HisPos + OffN * MaxRange;
				m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
			}
		}
		else
		{
			if (!bHeIsDead)
			{
				m_DeviceMove.Free();
				m_DeviceMove.Use();
				ResetStuckInfo();
			}
			INVALIDATE_POS(m_CollAvoidancePos);
			m_CollisionAvoidanceCount = 0;
			m_bCollAvoidanceBlocked = false;
			m_bCollCWBlocked = false;
			m_bCollCCWBlocked = false;
			return(false);
		}
	}

	if (DebugTarget())
	{
		bool wtf = true;
	}

	// Bad, move straight away and if blocked we tell him so
	if (Range <= MinRange)
	{
		m_CollAvoidancePos = HisPos + OffN * MaxRange;
		m_CollAvoidancePos = m_PathFinder.SafeGetPathPosition(m_CollAvoidancePos,2,1);
		CVec3Dfp32 AwayPos = OurPos;
		if (!m_PathFinder.GroundTraversable(OurPos,m_CollAvoidancePos,&AwayPos))
		{	// We cannot move straight away from him (the bastard!)
			// Try a few other directions: OurClosest8+1 and OurClosest8-1
			CVec3Dfp32 CCW = HisPos + GetOffFromOctant((OurClosest8+1) % 8) * MaxRange;
			CCW = m_PathFinder.SafeGetPathPosition(CCW,2,1);
			CVec3Dfp32 CW = HisPos + GetOffFromOctant((8+OurClosest8-1) % 8) * MaxRange;
			CW = m_PathFinder.SafeGetPathPosition(CW,2,1);
			CVec3Dfp32 CCWPos = OurPos;
			CVec3Dfp32 CWPos = OurPos;
			if (HisPos.DistanceSqr(CCW) <= HisPos.DistanceSqr(CW))
			{	// Check CCW first
				if (!m_PathFinder.GroundTraversable(OurPos,CCW,&CCWPos))
				{
					m_PathFinder.GroundTraversable(OurPos,CW,&CWPos);
				}
			}
			else
			{	// Check CCW first
				if (!m_PathFinder.GroundTraversable(OurPos,CW,&CWPos))
				{
					m_PathFinder.GroundTraversable(OurPos,CCW,&CCWPos);
				}
			}
			
			// OK, which one is best?
			if ((HisPos.DistanceSqr(CCWPos) >= HisPos.DistanceSqr(AwayPos))&&(HisPos.DistanceSqr(CCWPos) >= HisPos.DistanceSqr(CWPos)))
			{
				AwayPos = CCWPos;
			}
			else if ((HisPos.DistanceSqr(CWPos) >= HisPos.DistanceSqr(AwayPos))&&(HisPos.DistanceSqr(CWPos) >= HisPos.DistanceSqr(CCWPos)))
			{
				AwayPos = CWPos;
			}
			if (HisPos.DistanceSqr(AwayPos) >= Sqr(MidRange))
			{
				m_CollAvoidancePos = AwayPos;
			}
			else
			{
				m_bCollAvoidanceBlocked = true;
				if (!bHeIsDead)
				{
					m_DeviceMove.Free();
					m_DeviceMove.Use();
					ResetStuckInfo();
				}
				INVALIDATE_POS(m_CollAvoidancePos);
				m_CollisionAvoidanceCount = 0;
				return(true);
			}
		}
		m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
		ResetStuckInfo();
	}

	// TODO: Look at this and do some OurWantedMove tangent magic
	if ((Range > MinRange)&&((VALID_POS(m_PathDestination))&&(HisPos.DistanceSqr(m_PathDestination) < Sqr(MidRange))))
	{	// Our goal is inside collision radius
		if (DebugTarget())
		{
			ConOutL("m_PathDestination inside MidRange, stay put");
		}
		if (!bHeIsDead)
		{
			m_DeviceMove.Free();
			m_DeviceMove.Use();
			ResetStuckInfo();
		}
		INVALIDATE_POS(m_CollAvoidancePos);
		m_CollisionAvoidanceCount = 0;
		return(true);
	}

	// If we're moving away from him just keep on trucking
	if ((!m_CollisionAvoidanceCount)&&(OurWantedMove * OffN > 0.0f))
	{
		if (DebugTarget())
		{
			ConOutL("Moving away, just continue");
		}
		ResetStuckInfo();
		INVALIDATE_POS(m_CollAvoidancePos);
		m_CollisionAvoidanceCount = 0;
		m_bCollAvoidanceBlocked = false;
		return(true);
	}

	if ((INVALID_POS(m_CollAvoidancePos))||(!m_CollisionAvoidanceCount)||(OurPos.DistanceSqr(m_CollAvoidancePos) <= Sqr(8.0f)))
	{	// Beyond MinRange
		if ((VALID_POS(m_PathDestination))&&(HisPos.DistanceSqr(m_PathDestination) <= Sqr(MinRange))&&(Range > MinRange))
		{	// Stop and do nothing when our goal is inside the other
			if (DebugTarget())
			{
				ConOutL("m_PathDestination inside coll radius, stay put");
			}
			if (!bHeIsDead)
			{
				m_DeviceMove.Free();
				m_DeviceMove.Use();
				ResetStuckInfo();
			}
			INVALIDATE_POS(m_CollAvoidancePos);
			m_CollisionAvoidanceCount = 0;
			return(true);
		}

		if ((OurDest8 == OurClosest8)&&(true))
		{	// Do nothing
			INVALIDATE_POS(m_CollAvoidancePos);
			m_CollisionAvoidanceCount = 0;
			m_bCollAvoidanceBlocked = false;
			m_bCollCWBlocked = false;
			m_bCollCCWBlocked = false;
			return(false);
		}

		// Which way is the closest to our goal? OurClosest8+1 or OurClosest8-1?
		CVec3Dfp32 CCW = HisPos + GetOffFromOctant((OurClosest8+1) % 8) * MidRange;
		CCW = m_PathFinder.SafeGetPathPosition(CCW,2,1);
		CVec3Dfp32 CW = HisPos + GetOffFromOctant((8+OurClosest8-1) % 8) * MidRange;
		CW = m_PathFinder.SafeGetPathPosition(CW,2,1);
		if ((!m_bCollCCWBlocked)&&((m_bCollCWBlocked)||(OurDest.DistanceSqr(CCW) <= OurDest.DistanceSqr(CW))))
		{
			if (DebugTarget())
			{
				ConOutL("Go CCW");
			}
			if (m_PathFinder.GroundTraversable(OurPos,CCW,&CCW))
			{
				m_CollAvoidancePos = CCW;
				m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
				m_bCollAvoidanceBlocked = false;
				m_bCollCCWBlocked = false;
				ResetStuckInfo();
			}
			else
			{	// Stay put
				m_bCollCCWBlocked = true;
				if ((!m_bCollCWBlocked)&&(m_PathFinder.GroundTraversable(OurPos,CW,&CW)))
				{
					m_CollAvoidancePos = CW;
					m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
					m_bCollAvoidanceBlocked = false;
					m_bCollCWBlocked = false;
					ResetPathSearch();
				}
				else
				{
					if (!bHeIsDead)
					{
						m_DeviceMove.Free();
						m_DeviceMove.Use();
						ResetStuckInfo();
					}
					INVALIDATE_POS(m_CollAvoidancePos);
					m_CollisionAvoidanceCount = 0;
				}
			}
		}
		else
		{
			if (DebugTarget())
			{
				ConOutL("Go CW");
			}
			if (m_PathFinder.GroundTraversable(OurPos,CW,&CW))
			{
				m_CollAvoidancePos = CW;
				m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
				m_bCollAvoidanceBlocked = false;
				m_bCollCWBlocked = false;
				ResetPathSearch();
			}
			else
			{	// Stay put
				m_bCollCWBlocked = true;
				if ((!m_bCollCCWBlocked)&&(m_PathFinder.GroundTraversable(OurPos,CCW,&CCW)))
				{
					m_CollAvoidancePos = CCW;
					m_CollisionAvoidanceCount = AI_COLL_AVOID_TICKS;
					m_bCollAvoidanceBlocked = false;
					m_bCollCCWBlocked = false;
					ResetStuckInfo();
				}
				else
				{
					if (!bHeIsDead)
					{
						m_DeviceMove.Free();
						m_DeviceMove.Use();
						ResetStuckInfo();
					}
					INVALIDATE_POS(m_CollAvoidancePos);
					m_CollisionAvoidanceCount = 0;
				}
			}
		}
	}

	// Always finish our avoidance move
	if ((VALID_POS(m_CollAvoidancePos))&&(m_CollisionAvoidanceCount))
	{
		if (SqrDistanceToUs(m_CollAvoidancePos) <= Sqr(8.0f))
		{	// Stay put
			m_DeviceMove.Free();
			m_DeviceMove.Use();
			ResetStuckInfo();
			return(true);
		}
		else
		{	// Go there
			ResetStuckInfo();
			m_DeviceMove.Free();
			AddMoveTowardsPositionCmd(m_CollAvoidancePos,m_ReleaseIdleSpeed);
			ResetPathSearch();
		}
		m_CollisionAvoidanceCount--;
		if (m_CollisionAvoidanceCount <= 0)
		{
			INVALIDATE_POS(m_CollAvoidancePos);
			m_CollisionAvoidanceCount = 0;
			m_bCollAvoidanceBlocked = false;
		}
	}

	return(true);
}

int CAI_Core::GetCollider()
{
	if (VALID_POS(m_CollAvoidancePos))
	{
		return(m_KB.GetCollider());
	}
	else
	{
		return(0);
	}
};

bool CAI_Core::SetWallclimb(bool _bActivate)
{
	if (_bActivate)
	{
		if ((m_bCanWallClimb)&&(!m_bWallClimb))
		{
			m_pGameObject->AI_EnableWallClimbing(true);
			m_bWallClimb = true;
			return(true);
		}
	}
	else if (m_bWallClimb)
	{
		m_pGameObject->AI_EnableWallClimbing(false);
		m_bWallClimb = false;
	}

	return(false);
};

bool CAI_Core::TempEnableWallClimb(int32 _DurationTicks)
{
	if ((m_bCanWallClimb)&&(!m_ClimbDisableTicks))
	{
		if (m_ClimbEnableTicks < _DurationTicks)
		{
			m_ClimbEnableTicks = _DurationTicks;
		}
		SetWallclimb(true);
		return(true);
	}
	return(false);
};

bool CAI_Core::TempDisableWallClimb(int32 _DurationTicks)
{
	if ((m_bCanWallClimb)&&(m_ClimbDisableTicks < _DurationTicks))
	{
		m_ClimbDisableTicks = _DurationTicks;
		m_ClimbEnableTicks = 0;
		SetWallclimb(false);
		return(true);
	}
	return(false);
};

bool CAI_Core::TempDisableJump(int32 _DurationTicks)
{
	if ((m_bCanJump)&&(m_JumpDisableTicks < _DurationTicks))
	{
		m_JumpDisableTicks = _DurationTicks;
		return(true);
	}
	return(false);
};

// Set velocity to given amount in given direction, if possible
void CAI_Core::Push(fp32 _Speed, const CVec3Dfp32& _Direction)
{
	//Bots velocity is set to the given amount, in the given direction
	if (!(m_pGameObject->AI_GetPhysFlags() & PLAYER_PHYSFLAGS_IMMOBILE))
	{
		CVec3Dfp32 PushVel = _Direction;
		PushVel.Normalize();
		PushVel *= _Speed;
		
		CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject); 
		if (!pChar)
		{
			// IsCharacter fails for dead characters... This hack will take care of some stuff, but we really should fix IsCharacter instead.
			static const uint32 class_charNPC = MHASH4('CWOb','ject','_Cha','rNPC');
			static const uint32 class_charPlayer = MHASH5('CWOb','ject','_Cha','rPla','yer');
			if (m_pGameObject->IsClass(class_charPlayer) || m_pGameObject->IsClass(class_charNPC))
				pChar = (CWObject_Character*)m_pGameObject;
		}

		if (pChar)
		{
			// If ragdoll is running we push it instead
#ifdef INCLUDE_OLD_RAGDOLL
			if (pChar->m_spRagdoll)
				pChar->m_spRagdoll->AddImpulse(CConstraintSystem::BODY_ALL,PushVel);
			else
#endif // INCLUDE_OLD_RAGDOLL
			if (pChar->m_pPhysCluster)
			{
				TAP<CWPhys_ClusterObject> pPO = pChar->m_pPhysCluster->m_lObjects;
				for(uint i = 0;i < pPO.Len();i++)
				{
					m_pServer->Phys_AddForce(pPO[i].m_pRB,PushVel * RAGDOLL_FORCE_MULTIPLIER);
				}
			}
			else
			{
				pChar->Physics_AddVelocityImpulse(PushVel);
			}
		}
		else
		{
			m_pServer->Object_SetVelocity(m_pGameObject->m_iObject, PushVel);
		}
	};
}


//Perception check of awareness, Returns agent info awareness value.
int CAI_Core::Perception_Awareness(int _iObj)
{
	CVec3Dfp32 ObjPos = GetBasePos(_iObj);
	if (INVALID_POS(ObjPos))
		return CAI_AgentInfo::AWARENESS_INVALID;

	//A bot is aware of any objects within it's awareness radius, and any such objects 
	//are automatically spotted
	if ((GetBasePos()).DistanceSqr(ObjPos) < Sqr(m_Awareness))
	{
		return CAI_AgentInfo::SPOTTED;
	}
	else 
		return CAI_AgentInfo::NONE;
};

// Returns CAI_AgentInfo::SPOTTED when inside vision range and FOV
// Returns CAI_AgentInfo::NOTICED when inside vision range and wide FOV
// Returns CAI_AgentInfo::NONE otherwise
int32 CAI_Core::InsideFOV(CVec3Dfp32& _Pos)
{
	if (INVALID_POS(_Pos))
	{
		return CAI_AgentInfo::NONE;
	}

	CVec3Dfp32 viewerPos, sightPos;
	viewerPos = GetLookPos();
	sightPos = _Pos;
	
	// We exaggerate vertical diff by ms_aPerceptionFactors[VERTICAL_MULTIPLIER]
	fp32 DistSqr = Sqr(sightPos[0] - viewerPos[0])+ Sqr(sightPos[1] - viewerPos[1]);
	if (sightPos[2] - viewerPos[2] >= 0.0f)
	{	// Only use vertical multiplier upwards
		DistSqr += Sqr(PERCEPTIONFACTOR_VERTICAL_MULTIPLIER * (sightPos[2] - viewerPos[2]));
	}
	else
	{
		DistSqr += Sqr(sightPos[2] - viewerPos[2]);
	}

	if (DistSqr > Sqr(m_SightRange))
	{
		return CAI_AgentInfo::NONE;
	}

	if ((sightPos[2] - viewerPos[2] >= 0.0f)&&((sightPos[2] - viewerPos[2]) > M_Sqrt(Sqr(sightPos[0] - viewerPos[0])+ Sqr(sightPos[1] - viewerPos[1]))))
	{	// Above vertical FOV
		return CAI_AgentInfo::NONE;
	}
	else
	{	// New version, works better with weird ai:s such as the rails
		fp32 AngleDiff = AngleOff(sightPos,true);
		if (AngleDiff > m_FOVWide * 0.5f)
		{
			return CAI_AgentInfo::NONE;
		}
		else if (AngleDiff > m_FOV * 0.5f)
		{
			return CAI_AgentInfo::NOTICED;
		}
		else
		{
			return CAI_AgentInfo::SPOTTED;
		}
	}

	return CAI_AgentInfo::NONE;
};

// Perception check of sight, Returns agent info awareness value.
// New method uses ms_aPerceptionFactors for calculation
int CAI_Core::Perception_Sight(int _iObj, bool _bObjectOnly, fp32 _Vis)
{
	MSCOPESHORT(CAI_Core::Perception_Sight);

	if (!m_pGameObject || !m_pServer)
	{
		return CAI_AgentInfo::AWARENESS_INVALID;
	}

	CVec3Dfp32 viewerPos,sightPos;
	viewerPos = GetLookPos();
	CAI_AgentInfo* pInfo = NULL;
	CWObject* pObj = NULL;
	if (!_bObjectOnly)
	{
		// NOTE: We don't use pInfo->GetBasePos() here as we want the real actual pos
		pInfo = m_KB.GetAgentInfo(_iObj);
		if (!pInfo)
		{
			return CAI_AgentInfo::AWARENESS_INVALID;
		}
	}
	pObj = m_pServer->Object_Get(_iObj);
	sightPos = GetHeadPos(pObj);
	if (INVALID_POS(sightPos))
	{
		return CAI_AgentInfo::AWARENESS_INVALID;
	}

#ifndef M_RTM
	// *** Debug stop ***
	if (DebugTarget())
	{
		bool wtf = true;
	}
	// ***
#endif

	// We exaggerate vertical diff by ms_aPerceptionFactors[VERTICAL_MULTIPLIER]
	fp32 DistSqr = Sqr(sightPos[0] - viewerPos[0])+ Sqr(sightPos[1] - viewerPos[1]);
	if (sightPos[2] - viewerPos[2] >= 0.0f)
	{	// Only use vertical multiplier upwards
		DistSqr += Sqr(PERCEPTIONFACTOR_VERTICAL_MULTIPLIER * (sightPos[2] - viewerPos[2]));
	}
	else
	{
		DistSqr += Sqr(sightPos[2] - viewerPos[2]);
	}

	if (DistSqr > Sqr(m_SightRange))
	{
		if (pInfo)
		{
			pInfo->SetLOS(false);
		}
		return CAI_AgentInfo::NONE;
	}

	// We don't check FOV when factor is too high
	fp32 Factor = _Vis;
	if (_Vis < 0.0f)
	{
		Factor = GetVisionFactor(_iObj);
	}
	bool bInsideMinSneak = false;
	bool bMuzzleFlash = false;
	if (Factor > 1.0f)
	{
		bMuzzleFlash = true;
	}

	// Check FOV
	// Horisontal FOV is limited by m_FOV
	// Vertical FOV is unmlimited down, and 45 degree up
	if ((!bMuzzleFlash)&&(sightPos[2] - viewerPos[2] >= 0.0f)&&((sightPos[2] - viewerPos[2]) > M_Sqrt(Sqr(sightPos[0] - viewerPos[0])+ Sqr(sightPos[1] - viewerPos[1]))))
	{	// Above vertical FOV
		if (pInfo)
		{
			pInfo->SetLOS(false);
		}
		return CAI_AgentInfo::NONE;
	}
	else
	{
		// New version, works better with weird ai:s such as the rails
		fp32 AngleDiff = AngleOff(sightPos,true);
		// A fairly ugly hack to allow civs to see behind their backs when panicked
		if ((m_CharacterClass == CLASS_CIV)&&(pInfo)&&(pInfo->GetCurRelation() >= CAI_AgentInfo::ENEMY))
		{
			AngleDiff = 0.0f;
		}
		if ((bMuzzleFlash)||(AngleDiff < m_FOVWide * 0.5f))
		{
			if (m_PerceptionFlags & PERC_XRAYVISION_FLAG)
			{
				if (pInfo)
				{
					pInfo->SetLOS(true);
				}
			}
			else
			{
				if (FastCheckLOS(pObj))
				{
					if (pInfo)
					{
						pInfo->SetLOS(true);
					}
				}
				else
				{
					if (pInfo)
					{
						pInfo->SetLOS(false);
					}
					return CAI_AgentInfo::NONE;
				}
			}
			
			if (DistSqr < Sqr(PERCEPTIONFACTOR_MIN_SNEAK_RANGE))
			{	// Within MIN_SNEAK_RANGE
				if (Factor < 1.0f)
				{
					Factor = 1.0f;
				}
				bInsideMinSneak = true;
			}

			if ((bMuzzleFlash)||(AngleDiff <= m_FOV * 0.5f))
			{	// Inside central view or muzzleflash
				if ((m_FakeFOVLight > 0.0f)&&(Sqr(m_FakeFOVLight) > DistSqr))
				{
					Factor = Max(Factor,1.0f);
				}

				// We keep spot all the way out to sightrange if we spotted previously
				if ((bInsideMinSneak)||((pInfo)&&(pInfo->GetCurAwareness() >= CAI_AgentInfo::SPOTTED)))
				{	
					if (DistSqr < Sqr(m_SightRange))
					{	// Inside THRESHOLD_SPOT
						if (pInfo)
						{
							pInfo->TouchLastSpotted();
							pInfo->SetCorrectSuspectedPosition();
						}
						return CAI_AgentInfo::SPOTTED;
					}
				}

				// Is he so dark or so far away as we merely notice him?
				fp32 ModifiedSightRange = m_SightRange * Factor;
				if (DistSqr > Sqr(ModifiedSightRange))
				{	// Outside of modified sightrange
					if ((ModifiedSightRange > 0.0f)&&(DistSqr <= Sqr(m_SightRange)))
					{	// NOTICED Chance tapers off at twice detection range
						fp32 Fraction = (M_Sqrt(DistSqr) / (ModifiedSightRange)) - 1.0f;
						if (Random > Fraction)
						{
							if (pInfo)
							{
								pInfo->SetCorrectSuspectedPosition();
							}
							return CAI_AgentInfo::NOTICED;
						}
					}
					return CAI_AgentInfo::NONE;
				}

				if (DistSqr < Sqr(m_SightRange * Factor))
				{	// Inside THRESHOLD_SPOT
					if (pInfo)
					{
						pInfo->TouchLastSpotted();
						pInfo->SetCorrectSuspectedPosition();
					}
					return CAI_AgentInfo::SPOTTED;
				}
				else
				{	// Outside THRESHOLD_SPOT
					if (pInfo)
					{
						pInfo->TouchLastSpotted();
						pInfo->SetCorrectSuspectedPosition();
					}
					return CAI_AgentInfo::DETECTED;
				}
			}
			else
			{	// Outside central view but inside peripheral vision
				if (DistSqr < Sqr(m_SightRange * Factor * PERCEPTIONFACTOR_VIEW_WIDE_FOV))
				{	// Peripheral vision NOTICED when Ilde or less and DETECTED when Watchful
					if (pInfo)
					{
						pInfo->SetCorrectSuspectedPosition();
					}
					if (m_KB.GetAlertness() >= CAI_KnowledgeBase::ALERTNESS_WATCHFUL)
					{
						return CAI_AgentInfo::DETECTED;
					}
					return CAI_AgentInfo::NOTICED;
				}
			}
		}
		else
		{	// Outside horisontal FOV
			if (pInfo)
			{
				pInfo->SetLOS(false);
			}
			return CAI_AgentInfo::NONE;
		}
	}
	if (pInfo)
	{
		pInfo->SetLOS(false);
	}
	return CAI_AgentInfo::NONE;
}

// Perception check of hearing, Returns agent info awareness value.
// New method uses ms_aPerceptionFactors for calculation
int CAI_Core::Perception_Hearing(int _iObj, int _OwnLoudness, bool _bInArea)
{
	MSCOPESHORT(CAI_Core::Perception_Hearing);
	if (!m_pGameObject || !m_pServer)
		return CAI_AgentInfo::AWARENESS_INVALID;

	CAI_AgentInfo* pInfo = m_KB.GetAgentInfo(_iObj);
	if (!pInfo)
		return CAI_AgentInfo::AWARENESS_INVALID;

	// Measure basic range to target
	CVec3Dfp32 listenerPos,soundPos;
	listenerPos = GetHeadPos();
	soundPos = pInfo->GetObject()->GetPosition();
	// We exaggerate vertical diff by ms_aPerceptionFactors[VERTICAL_MULTIPLIER]
	fp32 DistSqr = Sqr(soundPos[0] - listenerPos[0])+ Sqr(soundPos[1] - listenerPos[1]);
	if (soundPos[2] - listenerPos[2] >= 0.0f)
	{	// Only use vertical multiplier upwards
		DistSqr += Sqr(PERCEPTIONFACTOR_VERTICAL_MULTIPLIER * (soundPos[2] - listenerPos[2]));
	}
	else
	{
		DistSqr += Sqr(soundPos[2] - listenerPos[2]);
	}


	if (DistSqr > Sqr(m_HearingRange * 2.0f))
	{
		return CAI_AgentInfo::NONE;
	}

	fp32 Factor = GetHearingFactor(_iObj);
	int Relation = pInfo->GetCurRelation();
	int Awareness = pInfo->GetCurAwareness();
	if ((Factor > 1.0f)&&(IsPlayer(_iObj))&&(Relation >= CAI_AgentInfo::UNFRIENDLY))
	{
		fp32 RangeMulti = Factor * 0.5f;
		if (Awareness < CAI_AgentInfo::SPOTTED)
		{
			if (!(m_PerceptionFlags & PERC_HEARTHROUGHWALLS_FLAG))
			{	
				RangeMulti *= PERCEPTIONFACTOR_SOUND_LOS_BLOCKED_GUN;
			}
		}
		if (DistSqr < Sqr(RangeMulti * m_HearingRange))
		{	// Wake up!!!
			if (m_DeviceStance.GetIdleStance() < CAI_Device_Stance::IDLESTANCE_COMBAT)
			{
				m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_SURPRISED,60);
			}
			int Alertness = m_KB.GetAlertness();
			if (Alertness < CAI_KnowledgeBase::ALERTNESS_WATCHFUL)
			{
				m_KB.SetAlertness(CAI_KnowledgeBase::ALERTNESS_WATCHFUL);
			}
			if ((pInfo->GetGetLastSpottedTicks() <= 30 * AI_TICKS_PER_SECOND)||(m_CharacterClass == CLASS_CIV))
			{	// We know who did it!
				pInfo->TouchLastSpotted();
				if (Relation < CAI_AgentInfo::ENEMY)
				{
					pInfo->SetRelation(CAI_AgentInfo::ENEMY);
				}
				RaiseAlarm(_iObj,CAI_AgentInfo::ENEMY,true);
				pInfo->SetCorrectSuspectedPosition();
				SetMinStealthTension(TENSION_COMBAT);
				m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
				if (m_bBehaviourRunning)
				{
					StopBehaviour(BEHAVIOUR_STOP_FAST,CAI_Action::PRIO_IDLE);
				}
			}
			else
			{
				if (Relation < CAI_AgentInfo::HOSTILE_2)
				{
					pInfo->SetRelation(CAI_AgentInfo::HOSTILE_2);
				}
				RaiseAlarm(_iObj,CAI_AgentInfo::HOSTILE_2,true);
				pInfo->SetCorrectSuspectedPosition();
				SetMinStealthTension(TENSION_HOSTILE);
				if (m_DeviceStance.GetIdleStance() < CAI_Device_Stance::IDLESTANCE_HOSTILE)
				{
					m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
				}
				if (m_bBehaviourRunning)
				{
					StopBehaviour(BEHAVIOUR_STOP_NORMAL,CAI_Action::PRIO_IDLE);
				}
			}
			
		}
		else
		{
			if (Relation < CAI_AgentInfo::ENEMY)
			{
				if (Awareness < CAI_AgentInfo::NOTICED)
				{
					if (GetStealthTension(false) < TENSION_COMBAT)
					{
						m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_SURPRISED,60);
						UseRandom("Dangerous sound",CAI_Device_Sound::DANGEROUS_SOUND,CAI_Action::PRIO_DANGER);
					}
				}
				pInfo->SetRelation(CAI_AgentInfo::HOSTILE_2);
				if (m_bBehaviourRunning)
				{
					StopBehaviour(BEHAVIOUR_STOP_NORMAL,CAI_Action::PRIO_IDLE);
				}
			}
		}

		if (Awareness < CAI_AgentInfo::DETECTED)
		{
			pInfo->SetAwareness(CAI_AgentInfo::DETECTED,true,true);
		}

		return CAI_AgentInfo::DETECTED;
	}

	// Nothing but guns heard beyond m_HearingRange
	if (DistSqr > Sqr(m_HearingRange))
	{
		return CAI_AgentInfo::NONE;
	}

	if (_OwnLoudness * 0.01f > Factor)
	{
		Factor = 0.0f;
	}
	if (DistSqr > Sqr(m_HearingRange * Factor))
	{	// Outside of modified sightrange
		return CAI_AgentInfo::NONE;
	}
	else
	{	// We can "never" get better result than NOTICED from sound alone
		if (pInfo->GetCurAwareness() < CAI_AgentInfo::NOTICED)
		{	// We haven't heard the agent before
			if (Relation >= CAI_AgentInfo::HOSTILE)
			{	// Bad sound
				m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_ATTENTIVE,60);
				UseRandom("Suspicious sound",CAI_Device_Sound::SUSPICIOUS_SOUND,CAI_Action::PRIO_ALERT);
			}
		}

		pInfo->SetCorrectSuspectedPosition();
		return CAI_AgentInfo::NOTICED;
	}
}

CVec3Dfp32 CAI_Core::GetBasePos()
{
	return(m_CurBaseMat.GetRow(3));
};

CVec3Dfp32 CAI_Core::GetLastBasePos()
{
	return(m_LastBaseMat.GetRow(3));
};

CVec3Dfp32 CAI_Core::GetHeadPos()
{
	return(m_CurHeadMat.GetRow(3));
};

CVec3Dfp32 CAI_Core::GetLastHeadPos()
{
	return(m_LastHeadMat.GetRow(3));
};

CVec3Dfp32 CAI_Core::GetHeadPos(int _iObj)
{
	CWObject* pObj = m_pServer->Object_Get(_iObj);
	return(GetHeadPos(pObj));
};

void CAI_Core::GetHeadMat(CMat4Dfp32& _RetValue)
{
	_RetValue = m_CurHeadMat;
};

CVec3Dfp32 CAI_Core::GetLookDir()
{
	return(m_CurLookDir);
}

CVec3Dfp32 CAI_Core::GetLookDir(CWObject* _pObj)
{
	if (_pObj)
	{
		CAI_Core* pObjAI = GetAI(_pObj->m_iObject);
		if (pObjAI)
		{
			return(pObjAI->GetLookDir());
		}
		else
		{

			return(_pObj->GetPositionMatrix().GetRow(0));
		}
	}
	else
	{
		return(CVec3Dfp32(_FP32_MAX));
	}
};

CVec3Dfp32 CAI_Core::GetLookPos()
{
	return(m_CurLookPos);
}

CVec3Dfp32 CAI_Core::GetBasePos(CWObject* _pObj)
{
	if (_pObj)
	{
		CAI_Core* pObjAI = GetAI(_pObj->m_iObject);
		if (pObjAI)
		{
			return(pObjAI->GetBasePos());
		}
		else
		{
			return(_pObj->GetPosition());
		}
	}
	else
	{
		return(CVec3Dfp32(_FP32_MAX));
	}
	
};

CVec3Dfp32 CAI_Core::GetBasePos(int _iObj)
{
	CAI_Core* pObjAI = GetAI(_iObj);
	if (pObjAI)
	{
		return(pObjAI->GetBasePos());
	}
	else
	{
		return(GetBasePos(m_pServer->Object_Get(_iObj)));
	}
};

void CAI_Core::GetBaseMat(CMat4Dfp32& _RetValue)
{
	_RetValue = m_CurBaseMat;
};

CVec3Dfp32 CAI_Core::GetBaseDir()
{
	return(m_CurBaseMat.GetRow(0));
};

CVec3Dfp32 CAI_Core::GetUp()
{
	return(m_CurBaseMat.GetRow(2));
};

CVec3Dfp32 CAI_Core::GetLastUp()
{
	return(m_LastBaseMat.GetRow(2));
};

CVec3Dfp32 CAI_Core::GetSideDir()
{
	return(m_CurBaseMat.GetRow(1));
};

CVec3Dfp32 CAI_Core::GetLastSideDir()
{
	return(m_LastBaseMat.GetRow(1));
};

CVec3Dfp32 CAI_Core::GetForwardDir()
{
	// Base matrix fwd, not head fwd. Dunno if there is a difference, but I've added 
	// this method  for completeness with the Up and Side methods above.
	return(m_CurBaseMat.GetRow(0));
};

CVec3Dfp32 CAI_Core::GetLastForwardDir()
{
	return(m_LastBaseMat.GetRow(0));
};


void CAI_Core::OnRefreshPosMats()
{
	MSCOPESHORT(CAI_Core::OnRefreshPosMats);

	m_LastBaseMat = m_CurBaseMat;
	m_LastHeadMat = m_CurHeadMat;
	m_LastLookPos = m_CurLookPos; 
	if (m_pGameObject)
	{
		m_pGameObject->AI_GetBaseMat(m_CurBaseMat);
		m_CurHeadMat = m_pGameObject->GetPositionMatrix();
		// Only refresh headpos when we update perception

		// Postanim system needs the headlook direction at all times /J.E.		
		if (IsPlayer() || (m_bFirstRefresh)||((m_Timer % AI_KB_REFRESH_PERIOD == 0)&&(!m_StunTimeout)&&(!m_Script.IsPaused())&&(m_pGameObject->AI_IsAlive())))
		{
			m_CurHeadMat.GetRow(3) = m_pGameObject->AI_GetHeadPos();
			m_HeadOff = m_CurHeadMat.GetRow(3) - m_CurBaseMat.GetRow(3);
			m_WeaponOff = m_pGameObject->AI_GetWeaponPos() - m_CurBaseMat.GetRow(3);
		}
		else
		{
			m_CurHeadMat.GetRow(3) += m_HeadOff;
		}
	}

	// Update cur height
	if (m_CharacterClass == CLASS_DARKLING)
	{
		m_CurHeadMat.GetRow(3) = m_CurBaseMat.GetRow(3) + m_CurBaseMat.GetRow(2) * 24.0f;
	}
	m_CurHeight = (m_CurHeadMat.GetRow(3) - m_CurBaseMat.GetRow(3)).Length();

	// Update m_InvBaseMat
	m_CurBaseMat.InverseOrthogonal(m_InvBaseMat);
	m_InvBaseMat.UnitNot3x3();

	// Update m_CurLookPos
	// TODO: Hmm, is vertical bobbing troublesome? In that case we must fall back to using
	// GetHeight() along Up vector like we did in oldentimes.
	CVec3Dfp32 Up = m_CurBaseMat.GetRow(2);
	m_CurLookPos = m_CurHeadMat.GetRow(3);
	m_CurLookDir = m_CurHeadMat.GetRow(0);

	// Scripted & mounted characters trace from look 
	CMat4Dfp32 Mat;
	if (IsMounted(Mat))
	{
		m_CurLookPos = Mat.GetRow(3);
		m_CurLookDir = Mat.GetRow(0);
		m_CurLookPos += Mat.GetRow(0) * 64.0f;
#ifndef M_RTM
		if (DebugTarget())
		{
			Mat.GetRow(3) = m_CurLookPos;
			m_pServer->Debug_RenderMatrix(Mat,1.0f,true);
		}
#endif
	}
	else
	{
		if (m_CharacterClass != CLASS_DARKLING)
		{	// Eyes 2 units up and 2 fwd
			CVec3Dfp32 Fwd = m_CurBaseMat.GetRow(0);
			m_CurLookPos += Up * 2.0f;
			m_CurLookPos += Fwd * 2.0f;
		}
	}

	// Object collisions
	if (m_bObjCollRequested)
	{
		// Can we stand here?
		CVec3Dfp32 Pos = m_PathFinder.GetPathPosition(m_CurBaseMat.GetRow(3),0,0);;
		if (VALID_POS(Pos))
		{
			CWObject_Message Msg(OBJMSG_CHAR_SETANIMPHYS,0);
			m_pServer->Message_SendToObject(Msg,GetObjectID());
			m_bObjCollRequested = false;
		}
	}

	// Wall climbing
	if (m_CharacterClass == CLASS_DARKLING)
	{	// *** Hmm, what about lifting up the basemat of the darkling?
		m_CurBaseMat.GetRow(3) += Up * DARKLING_PF_OFFSET;
	}
};

void CAI_Core::OnRefreshClimb()
{
	if (m_ClimbEnableTicks > 0)
	{
		m_ClimbEnableTicks--;
		if (!m_ClimbEnableTicks)
		{
			SetWallclimb(false);
			ResetPathSearch();
		}
	}
	if (m_ClimbDisableTicks > 0)
	{
		m_ClimbDisableTicks--;
	}
	if (m_JumpDisableTicks > 0)
	{
		m_JumpDisableTicks--;
	}
};

void CAI_Core::OnRefreshBehaviours()
{	
#ifndef M_RTM
	if (DebugTarget())
	{
		bool wtf = true;
	}
#endif

	m_iPrevBehaviour = m_iCurBehaviour;
	m_bBehaviourEntering = false;
	m_bBehaviourExiting = false;
	m_iCurBehaviour = 0;

	// Why do we keep refreshing dead AIs???
	if ((m_pGameObject)&&(!m_pGameObject->AI_IsAlive()))
	{
		return;
	}

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
	const CWAG2I_Token* pToken = pCD->m_AnimGraph2.GetAG2I()->GetTokenFromID(0);
	if (pToken && pToken->GetGraphBlock() != -1 && pToken->GetAnimGraphIndex() != -1)
	{
		const CXRAG2_GraphBlock* pGraphBlock = pCD->m_AnimGraph2.GetAG2I()->GetGraphBlock(pToken->GetGraphBlock(),pToken->GetAnimGraphIndex());
		if (pGraphBlock && (pGraphBlock->m_Condition.m_ImpulseType == AG2_IMPULSETYPE_BEHAVIOR))
		{
			m_iCurBehaviour = pGraphBlock->m_Condition.m_ImpulseValue;
		}
	}

	if ((m_iCurBehaviour >= BEHAVIOUR_COMBAT_MIN)&&(m_iCurBehaviour <= BEHAVIOUR_COMBAT_MAX))
	{
		CAI_Weapon* pWeapon = m_Weapon.GetWielded();
		if (pWeapon)
		{
			CRPG_Object_Item* pItem = pWeapon->GetItem();
			if ((pItem)&&(pItem->m_AnimType == AG2_IMPULSEVALUE_WEAPONTYPE_RIFLE))
			{
				m_iCurBehaviour -= BEHAVIOUR_COMBAT_RIFLE_OFFSET;
			}
		}
		if (!m_pBehaviourScenePoint)
		{	// Combat behaviour but no SP?
			StopBehaviour(BEHAVIOUR_STOP_NORMAL,m_BehaviourPrioClass);
		}
	}

	if ((m_AgonyBehaviour)&&(m_iPrevBehaviour == m_AgonyBehaviour)&&(m_iCurBehaviour == 0))
	{	// Agony finished
		m_StunTimeout = 0;
	}

	if (pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_BEHAVIORENTER)
	{
		m_bBehaviourEntering = true;
	}
	else if (pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_BEHAVIOREXIT)
	{
		m_bBehaviourExiting = true;
	}

	if (m_iCurBehaviour == BEHAVIOUR_NBR_TURNING)
	{
		m_iCurBehaviour = m_iBehaviourRunning;
		m_bBehaviourEntering = true;
	}

	if ((!m_bBehaviourEntering)&&(!m_bBehaviourExiting))
	{
		if (m_iCurBehaviour != m_iCurMainBehaviour)
		{	// Trigger events
			if (m_iCurBehaviour != BEHAVIOUR_NBR_STOPPED)
			{
				if ((!m_bBehaviourMainRunning)&&(m_iCurBehaviour))
				{
					OnBehaviourMainStarted(m_iCurBehaviour);
				}
			}
			else if (m_iPrevBehaviour != BEHAVIOUR_NBR_STOPPED)
			{
				OnBehaviourFinished(m_iPrevBehaviour);
			}
		}
		m_iCurMainBehaviour = m_iCurBehaviour;
		if (m_BehaviourStopIssued)
		{
			if (m_iCurBehaviour)
			{
				m_bBehaviourRunning = true;
				m_iBehaviourRunning = m_iCurBehaviour;
				StopBehaviour(BEHAVIOUR_STOP_NORMAL,m_BehaviourPrioClass,m_StopDir);
			}
			else
			{
				m_BehaviourStopIssued = BEHAVIOUR_STOP_NONE;
				m_StopDir = 0;
			}
		}
		if ((m_bBehaviourRunning)&&(!m_iCurBehaviour))
		{
			ResetBehaviour();
		}
	}

	if ((!m_iCurBehaviour)&&(m_iPendingBehaviour))
	{
		SetWantedBehaviour2(m_iPendingBehaviour,m_PendingBehaviourPrioClass,0,0);
		m_iPendingBehaviour = 0;
		m_PendingBehaviourPrioClass = CAI_Action::PRIO_MIN;
	}

	// Refresh gestures
	if (m_GestureTimer)
	{
		m_GestureTimer--;
	}
};

// Returns true if character is scripted with TURRET_MOUNT
// (With the turret mount's current matrix in _Mat)
bool CAI_Core::IsMounted(CMat4Dfp32& _Mat)
{
	if (m_iMountParam)
	{
		CWObject* pObj = m_pServer->Object_Get(m_iMountParam);
		if (pObj)
		{	// Get the current matrix
			CWObject_Message Msg(OBJSYSMSG_GETCAMERA);
			Msg.m_pData = &_Mat;
			Msg.m_DataSize = sizeof(_Mat);
			if (m_pServer->Message_SendToObject(Msg, m_iMountParam))
			{
				return(true);
			}
			else
			{
				// *** Clear m_liAction[LOOK]? ***
				m_iMountParam = 0;
				return(false);
			}
		}
		else
		{
			// *** Clear m_liAction[LOOK]? ***
			m_iMountParam = 0;
			return(false);
		}
	}
	else
	{	// No, here we should NOT clear m_liAction[LOOK]
		m_iMountParam = 0;
		return(false);
	}
};

bool CAI_Core::RefreshTicks()
{
	// *** Questionable: Maybe we base our timing on GetGlobalAITick() instead? ***
	m_ServerTickFraction += m_pServer->GetGameTickTime();
	if (m_ServerTickFraction >= AI_TICK_DURATION)
	{
		m_Timer++;
		m_ServerTick++;
		m_ServerTickFraction -= AI_TICK_DURATION;
		m_bServerTickAdvanced = true;
		return(true);
	}
	else
	{
		m_bServerTickAdvanced = false;
		return(false);
	}
};

//Get type of AI
int CAI_Core::GetType()
{
	return TYPE_CHARACTER;
};

CAI_Core* CAI_Core::GetAI(int _iObj, CWorld_Server* _pServer)
{
	if (_pServer)
	{
		CWObject* pObj = _pServer->Object_Get(_iObj);
		if (pObj)
		{
			CWObject_Interface_AI *pObjAI = pObj->GetInterface_AI();
			if (pObjAI)
				return pObjAI->AI_GetAI();
		}
	}
	return NULL;
};

//Get pointer to the AI of an object, or NULL if there isn't any
CAI_Core* CAI_Core::GetAI(int _iObj)
{
	if (!m_pServer)
		return NULL;

	CWObject* pObj = m_pServer->Object_Get(_iObj);
	if (pObj)
	{
		CWObject_Interface_AI *pObjAI = pObj->GetInterface_AI();
		if (pObjAI)
			return pObjAI->AI_GetAI();
	}
	return NULL;
};

void CAI_Core::UpdatePrevSpeed()
{
	m_PrevMoveDir = m_CurBaseMat.GetRow(3) - m_LastBaseMat.GetRow(3);
	fp32 Len = m_PrevMoveDir.Length();
	if (Len > 0.01f)
	{
		m_PrevMoveDir *= (1.0f/Len);
	}
	else
	{
		m_PrevMoveDir = CVec3Dfp32(0.0f,0.0f,0.0f);
		Len = 0.0f;
	}
	m_PrevSpeed = Len;
	m_PrevMoveDir = Proj2Base(m_PrevMoveDir);
};

fp32 CAI_Core::GetPrevSpeed()
{
	if (m_PrevSpeed < 0.0f) { UpdatePrevSpeed(); }
	return(m_PrevSpeed);
};

CVec3Dfp32 CAI_Core::GetPrevMoveDir()
{
	if (m_PrevSpeed < 0.0f) { UpdatePrevSpeed(); }
	return(m_PrevMoveDir);
};

CVec3Dfp32 CAI_Core::GetPrevVelocity()
{
	if (m_PrevSpeed < 0.0f) { UpdatePrevSpeed(); }
	return(m_PrevMoveDir * m_PrevSpeed);
}

// Returns the current move in worldspace
CVec3Dfp32 CAI_Core::GetWantedWorldMove()
{
	CVec3Dfp32 MoveStick = m_DeviceMove.GetNextMove();
	if (MoveStick.LengthSqr() > 0.01f)
	{
		CMat4Dfp32 PosMat;
		GetBaseMat(PosMat);
		PosMat.UnitNot3x3();
		MoveStick = MoveStick * PosMat;
	}
	return(MoveStick);
}


//Release search instance
void CAI_Core::ReleasePathSearch()
{
	//StartBM("ReleasePathSearch");
	m_PathFinder.Reset();
	m_Path.Reset();
	//StopBM();
};

// Returns the unit angle diff from our look at _Pos
// If _bBelowValid then we only measure angle horizontally if _Pos is below
fp32 CAI_Core::AngleOff(const CVec3Dfp32& _Pos,bool _bValidBelow)
{	
	CVec3Dfp32 Diff = _Pos - GetLookPos();
	if ((_bValidBelow)&&(Diff[2] < 0.0f))
	{
		Diff[2] = 0.0f;
	}
	Diff.Normalize();
	fp32 Dot = GetLookDir() * Diff;
	Dot = Clamp(Dot,-1.0f,1.0f);
	fp32 Angle = M_ACos(Dot);
	Angle /= _PI2;

	return(Angle);
};

//Returns the horizontal angle of the line between the given "from"- and "to"-positions, relative 
//to the x coordinate axis or the given _Angle. If the _Interval argument is a value between 0 and 1, 
//the result will be given in the interval [_Interval-1.._Interval]
fp32 CAI_Core::HeadingToPosition(const CVec3Dfp32& _FromPos, const CVec3Dfp32& _ToPos, fp32 _Interval, fp32 _Angle)
{
	//StartBM("HeadingToPosition");
	//Get angle in radians
	CVec3Dfp32 RelPos = _ToPos - _FromPos;
	RelPos[2] = 0; 
	RelPos.Normalize();
	RelPos[0] = Clamp(RelPos[0],-1.0f,1.0f);
	fp32 Angle = M_ACos(RelPos[0]);
	if (RelPos[1] > 0) 
		Angle = - Angle;
	//Convert angle to fractions
	Angle /= _PI2;
	//Change "absolute" angle to relative angle
	Angle -= _Angle;

	//Should we force the value to a specified interval?
	if ( (_Interval <= 1.0f) &&
		(_Interval >= 0.0f) )
	{
		//Force angle to interval -1..1
		Angle -= (int)Angle;
		//Force angle to _Interval-1.._Interval
		if (Angle < (_Interval - 1.0f))
			Angle = Angle + 1;
		else if (Angle > _Interval)	
			Angle = Angle - 1;
	};

	// ***
	/*
	fp32 Test = HeadingToPosition2(_FromPos,_ToPos);
	Test += _Angle;
	return(Test);
	*/
	// ***

	//StopBM();
	return Angle;
};

// Returns the local coordinate fract based heading to _ToPos
fp32 CAI_Core::HeadingToPosition2(const CVec3Dfp32& _FromPos, const CVec3Dfp32& _ToPos)
{
	CVec3Dfp32 RelPos = _ToPos - _FromPos;
	RelPos = (_ToPos - _FromPos) * m_InvBaseMat;

	// Confine to horizontal plane
	RelPos[2] = 0; 
	RelPos.Normalize();
	RelPos[0] = Clamp(RelPos[0],-1.0f,1.0f);
	fp32 Angle = M_ACos(RelPos[0]);
	if (RelPos[1] > 0) 
		Angle = - Angle;

	//Convert angle to fractions
	Angle /= _PI2;

	return Angle;
};

//Returns the vertical angle of the line between the given "from"- and "to"-positions, relative 
//to the x coordinate axis or the given _Angle. If the _Interval argument is a value between 0 and 1, 
//the result will be given in the interval [_Interval-1.._Interval]
fp32 CAI_Core::PitchToPosition(const CVec3Dfp32& _FromPos, const CVec3Dfp32& _ToPos, fp32 _Interval, fp32 _Angle)
{
	//StartBM("PitchToPosition");
	//Get angle in radians
	fp32 Angle;
	CVec3Dfp32 RelPos = _ToPos - _FromPos;
	fp32 l = RelPos.Length();

	Angle = M_ASin( M_FSel(-l, 0.0f, RelPos.k[2] / l) );
	//Convert to fractions
	Angle /= _PI2;

	//Change "absolute" angle to relative angle
	Angle -= _Angle;

	//Should we force the value to a specified interval?
	if ( (_Interval <= 1.0f) &&
		(_Interval >= 0.0f) )
	{
		//Force angle to interval -1..1
		Angle -= (int)Angle;
		//Force angle to _Interval-1.._Interval
		if (Angle < (_Interval - 1.0f))
			Angle = Angle + 1;
		else if (Angle > _Interval)	
			Angle = Angle - 1;
	};

	// ***
	/*
	fp32 Test = PitchToPosition2(_FromPos,_ToPos);
	Test += _Angle;
	return Test;
	*/
	// ***

	return Angle;
};

// Returns the local coordinate fract based pitch to _ToPos
fp32 CAI_Core::PitchToPosition2(const CVec3Dfp32& _FromPos, const CVec3Dfp32& _ToPos)
{
	fp32 Angle;
	CVec3Dfp32 RelPos = _ToPos - _FromPos;
	RelPos = (_ToPos - _FromPos) * m_InvBaseMat;

	// Confine to vertical plane
	RelPos[1] = 0; 
	RelPos.Normalize();
	Angle = M_ASin(RelPos[2]/RelPos.Length());
	//Convert to fractions
	Angle /= _PI2;

	return Angle;
};

//Current heading of given object in fractions
fp32 CAI_Core::GetHeading()
{
	return GetLookAngles()[2];
};

//Current heading of given object in fractions
fp32 CAI_Core::GetHeading(CWObject * _pObj)
{
	if (!_pObj)
		return 0;

	return GetLookAngles(_pObj)[2];
};

//Get cosine of first objects heading and it's heading to second object
fp32 CAI_Core::CosHeadingDiff(int32 _iTarget, bool _bHeadingOnly)
{
	if (!_iTarget)
		return 0;

	//Get our look
	CVec3Dfp32 Look = GetLookDir();

	//Get normalized heading vector from first to second object
	CVec3Dfp32 To = GetHeadPos(_iTarget) - GetHeadPos();

	if (_bHeadingOnly)
	{	// Ignore vertical component
		Look[2] = 0.0f;
		To[2] = 0.0f;
	}
	Look.Normalize();
	To.Normalize();

	//Return scalar product of vectors (i.e. cos of angle between them)
	return Look * To;
};

//Get cosine of first objects heading and it's heading to second object
fp32 CAI_Core::CosHeadingDiff(CWObject* _pObj1, CWObject* _pObj2, bool _bHeadingOnly)
{
	if (!_pObj1 || !_pObj2)
		return 0;

	//Get normalized heading vector (x,y,0) of first object
	CVec3Dfp32 Look = GetLookDir(_pObj1);
	

	//Get normalized heading vector from first to second object
	CVec3Dfp32 To = _pObj2->GetPosition() - _pObj1->GetPosition();
	

	if (_bHeadingOnly)
	{	// Ignore vertical component
		Look[2] = 0.0f;
		Look.Normalize();
		To[2] = 0.0f;
	}
	To.Normalize();

	//Return scalar product of vectors (i.e. cos of angle between them)
	return Look * To;
};


//Checks if the position is path-traversable for the bot
bool CAI_Core::IsTraversable(const CVec3Dfp32& _Pos)
{
	if (!m_pServer || !m_pGameObject || !m_PathFinder.GridPF())
		return false;

	//Check navgrid
	return m_PathFinder.GridPF()->GetBlockNav()->IsTraversable(m_PathFinder.GridPF()->GetBlockNav()->GetGridPosition(_Pos), (int)GetBaseSize(), (int)GetHeight(), m_bWallClimb);
};
/*
fp32 CAI_Core::ms_aPerceptionFactors[NUM_PERCEPTION_FACTORS] = 
{
	{0.0f}, // PERCEPTIONFACTOR_SOUND_CROUCH
	{0.0f}, // PERCEPTIONFACTOR_SOUND_STAND
	{0.0f}, // PERCEPTIONFACTOR_SOUND_WALK
	{1.0f}, // PERCEPTIONFACTOR_SOUND_RUN
	{0.5f}, // PERCEPTIONFACTOR_SOUND_LOS_BLOCKED
	{1.0f}, // PERCEPTIONFACTOR_LISTEN_STAND
	{1.0f}, // PERCEPTIONFACTOR_LISTEN_WALK
	{0.5f}, // PERCEPTIONFACTOR_LISTEN_RUN
	{0.25f},// PERCEPTIONFACTOR_LISTEN_SLEEPING
	{0.5f}, // PERCEPTIONFACTOR_LISTEN_DROWSY
	{0.75f},// PERCEPTIONFACTOR_LISTEN_IDLE
	{1.0f}, // PERCEPTIONFACTOR_LISTEN_WATCHFUL
	{0.75f}, //PERCEPTIONFACTOR_SIGHT_CROUCH
	{0.75f},// PERCEPTIONFACTOR_SIGHT_STAND
	{1.0f}, // PERCEPTIONFACTOR_SIGHT_WALK
	{1.0f}, // PERCEPTIONFACTOR_SIGHT_RUN
	{0.5f}, // PERCEPTIONFACTOR_SIGHT_DARKNESS
	{0.0f}, // PERCEPTIONFACTOR_SIGHT_LOS_BLOCKED
	{1.0f}, // PERCEPTIONFACTOR_VIEW_STAND
	{1.0f}, // PERCEPTIONFACTOR_VIEW_WALK
	{0.5f}, // PERCEPTIONFACTOR_VIEW_RUN
	{0.0f}, // PERCEPTIONFACTOR_VIEW_SLEEPING
	{0.5f}, // PERCEPTIONFACTOR_VIEW_DROWSY
	{0.75f},// PERCEPTIONFACTOR_VIEW_IDLE
	{1.0f}, // PERCEPTIONFACTOR_VIEW_WATCHFUL
	{0.25f},// PERCEPTIONFACTOR_VIEW_WIDE_FOV
	{0.75f},// PERCEPTIONFACTOR_THRESHOLD_SPOT
	{4.0f}, // PERCEPTIONFACTOR_VERTICAL_MULTIPLIER
	{40.0f} // PERCEPTIONFACTOR_MIN_SNEAK_RANGE
};
*/

//Check LOS between given positions. Succeeds if we hit something other than optionally excluded object.
//If given, the optional collision info is set appropriately.
bool CAI_Core::IntersectLOS(const CVec3Dfp32& _Start, const CVec3Dfp32& _End, int _iExclude, CCollisionInfo * _pInfo)
{
	if (!m_pServer)
		return false;

	return m_pServer->Phys_IntersectLine(_Start, 
		_End,
		// ObjectFlags
		0,
		// ObjectIntersectFlags, Intersect with physobjects, characters...
		OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER,
		//...and solids
		XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID,
		_pInfo,
		_iExclude);
};


//Checks if there is LOS from the given position to the target. The positions actually checked for LOS are
//those specified in the target offsets array added to the targets position. If an object is to be excluded 
//from the intersection checking, the server index to that object should be given. Returns the first position 
//found where there is LOS, or CVec3Dfp32(_FP32_MAX) if there is no LOS. 
CVec3Dfp32 CAI_Core::CheckLOS(const CVec3Dfp32& _Pos, CWObject* _pTarget, const CVec3Dfp32*_TargetOffsets, int _nTargets, int _iExclude)
{
	if (!_pTarget)
		return CVec3Dfp32(_FP32_MAX);

	//StartBM("CheckLOS");
	CCollisionInfo Info;
	//Info.m_CollisionType = CXR_COLLISIONTYPE_BOUND;

	CVec3Dfp32 TargetPos;

	//Check positions
	for (int i = 0; i < _nTargets; i++)
	{
		//Check for intersections of the line between _Pos and the current target position
		TargetPos = _pTarget->GetPosition() + _TargetOffsets[i];

		//Safety check for distance
		CVec3Dfp32 TargetVec;
		TargetPos.Sub(_Pos, TargetVec);
		fp32 DistSqr = TargetVec.LengthSqr();
		if (DistSqr > 8192.0f*8192.0f)
			break;

		const CWO_PhysicsState& PhysState = _pTarget->GetPhysState();

		// Make sure target has a regular AABB as physics primitive
		if ((PhysState.m_nPrim == 1) && 
			(PhysState.m_Prim[0].m_PrimType == OBJECT_PRIMTYPE_BOX) &&
			(PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION) == 0)
		{
			// Find collision point on box and then trace from _Pos to this point.
			// This way the ray will never hit the target so we don't need to
			// call IntersectLine with a collisioninfo to see what object was hit.
			// 

			const CWO_PhysicsPrim& Prim = PhysState.m_Prim[0];
			CBox3Dfp32 TargetPhysBox;
			CVec3Dfp32 PrimPos = _pTarget->GetPosition() + Prim.GetOffset();
			CVec3Dfp32 Dim = Prim.GetDim();
			TargetPhysBox.m_Min = PrimPos - Dim;
			TargetPhysBox.m_Max = PrimPos + Dim;

			CVec3Dfp32 HitPos;
			if (TargetPhysBox.IntersectLine(_Pos, TargetPos, HitPos))
			{
				// Move the hitpos 0.001f towards _Pos to make sure we don't hit the box.
				fp32 DistRecp = 1.0f / M_Sqrt(DistSqr);
				HitPos.Combine(TargetVec, -0.001f * DistRecp, HitPos);
				if (!IntersectLOS(_Pos, HitPos, _iExclude, NULL))
					return TargetPos;
			}
			else
			{
				goto RegularLOS;	// Don't think this will ever happen.
			}
		}
		else
		{
RegularLOS:
			//Check LOS
			bool Hit = IntersectLOS(_Pos, TargetPos, _iExclude, &Info);
			if (Hit && Info.m_bIsValid)
			{
				//We hit something, check if it was target 
				if ((Info.m_iObject == _pTarget->m_iObject))
				{
					//Debug_RenderWire(TargetPos, _Pos, 0xff00ff00, 1);//DEBUG
					//Found target position
					//StopBM("CheckLOS (succeeded)");
					return TargetPos;
				}
			}
		}
		//Debug_RenderWire(TargetPos, _Pos, 0xffff0000, 1);//DEBUG
	};

	//There is no LOS to any of the target points, so the target is considered to be out of LOS
	//StopBM("CheckLOS (failed)");
	return CVec3Dfp32(_FP32_MAX);
}; 



//Default LOS check, with object pointer as argument. Checks LOS from character to target, with a few offsets.
CVec3Dfp32 CAI_Core::CheckLOS(CWObject * _pTarget)
{
	if ((!_pTarget)||(!m_pGameObject))
		return CVec3Dfp32(_FP32_MAX);

	//Set up target offset points array
	CVec3Dfp32 Offsets[2];

	// We check the head first...
	CVec3Dfp32 Base = GetBasePos(_pTarget);
	Offsets[0] = GetTorsoPosition(_pTarget) - Base;
	// ...and then the Torso pos.
	Offsets[1] = GetHeadPos(_pTarget) - Base;

	return CheckLOS(GetHeadPos(),_pTarget,Offsets,2,GetObjectID());
};


//Default LOS check, with object server index as argument. Checks LOS from character to target, with a few offsets.
CVec3Dfp32 CAI_Core::CheckLOS(int _iTarget)
{
	//Get object pointer and check LOS
	return CheckLOS(m_pServer->Object_Get(_iTarget));
};

// Faster version of CheckLOS that doesn't bother with where the LOS was stopped
// It basically returns true if _Start can see _End
bool CAI_Core::FastCheckLOS(const CVec3Dfp32& _Start, const CVec3Dfp32& _End, int _iExclude0, int _iExclude1)
{
	if (!m_pServer)
		return true;


	bool Hit = m_pServer->Phys_IntersectLine(_Start, _End, 0,
		// ObjectIntersectFlags, Intersect with physobjects, characters...
		OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER,
		//...and solids
		XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID,
		_iExclude0, _iExclude1);

	return(!Hit);
}

// Faster version of CheckLOS that doesn't bother with where the LOS was stopped
// It basically returns true if _Start can see _End
bool CAI_Core::FastCheckLOS(CWObject* _pTarget)
{
	if ((_pTarget)&&(m_pGameObject))
	{
		CVec3Dfp32 Start = GetLookPos();
		CVec3Dfp32 End = GetHeadPos(_pTarget);
		if (m_iMountParam)
		{
			return FastCheckLOS(Start,End,m_iMountParam,_pTarget->m_iObject);
		}
		else
		{
			return FastCheckLOS(Start,End,GetObjectID(),_pTarget->m_iObject);
		}
	}

    return false;
}

// Faster version of CheckLOS that doesn't bother with where the LOS was stopped
// It basically returns true if _Start can see _End
bool CAI_Core::FastCheckLOS(int _iObj)
{
	//Get object pointer and check LOS
	return FastCheckLOS(m_pServer->Object_Get(_iObj));
}


//A bit above center position of bounding box (center position is often between the legs of characters)
CVec3Dfp32 CAI_Core::GetTorsoPosition(CWObject * _pObj)
{
	CVec3Dfp32 Base = GetBasePos(_pObj);
	if (INVALID_POS(Base))
	{
		return(Base);
	}
	CVec3Dfp32 Head = GetHeadPos(_pObj);
	if (INVALID_POS(Head))
	{
		return(Head);
	}
	Base[2] += (Head[2] - Base[2]) * 0.6;

	return Base;
};

//Get believed torso position
CVec3Dfp32 CAI_Core::GetTorsoPosition(CAI_AgentInfo* _pInfo)
{
	if (!_pInfo)
	{
		return(CVec3Dfp32(_FP32_MAX));
	}

	CVec3Dfp32 Base = _pInfo->GetBasePos();
	if (INVALID_POS(Base))
	{
		return(Base);
	}
	CVec3Dfp32 Head = _pInfo->GetHeadPos();
	if (INVALID_POS(Head))
	{
		return(Base);
	}
	Base[2] += (Head[2] - Base[2]) * 0.6;

	return Base;
};

CVec3Dfp32 CAI_Core::GetHeadPos(CWObject* _pObj)
{
	if (!_pObj)
		return CVec3Dfp32(_FP32_MAX);

	// Try cheaper one first
	CAI_Core* pTargetAI = GetAI(_pObj->m_iObject);
	if (pTargetAI)
	{
		return(pTargetAI->GetHeadPos());
	}

	CWObject_Character* pChar = CWObject_Character::IsCharacter(_pObj);
	if (pChar)
	{
		return(pChar->AI_GetHeadPos());
	}
	
	// Get position and add 90% aof bbox height
	CVec3Dfp32 HeadPos = _pObj->GetPosition();
	const CBox3Dfp32* pBox = _pObj->GetAbsBoundBox();
	if (pBox)
	{
		CVec3Dfp32 Diff = pBox->m_Max - pBox->m_Min;
		HeadPos[2] += Diff[2];
	}
	else
	{
		HeadPos[2] += 48;	// 1.5m high
	}

	return(HeadPos);
}

//Face position or a bit below aim position
CVec3Dfp32 CAI_Core::GetVulnerablePosition(CWObject * _pObj)
{
	if (!_pObj)
		return CVec3Dfp32(_FP32_MAX);

	CVec3Dfp32 Res;

	if (IsCharacter(_pObj->m_iObject))
	{
		CVec3Dfp32 FacePos;
		CWObject_Message Msg(OBJMSG_AIQUERY_GETVULNERABLEPOS);
		Msg.m_pData = &FacePos;
		if (m_pServer->Message_SendToObject(Msg, _pObj->m_iObject))
		{
			//Face position found, set offset
			Res = FacePos;
		}
		else
		{
			//Couldn't get face attach point, use point just below aim position
			Res = GetAimPosition(_pObj);
			Res[2] -= 4;
		}
		//StopBM();
	}
	else
	{
		//Not character, use base position
		Res = _pObj->GetPosition();
	}

	return Res;
};



//The activate-position of the character
CVec3Dfp32 CAI_Core::GetTrueAimPosition()
{
	CVec3Dfp32 WeaponPos = m_pGameObject->AI_GetWeaponPos();
	m_WeaponOff = WeaponPos - m_CurBaseMat.GetRow(3);
	return(WeaponPos);
};

CVec3Dfp32 CAI_Core::GetAimPosition()
{
	return(m_CurBaseMat.GetRow(3) + m_WeaponOff);
};

CVec3Dfp32 CAI_Core::GetAimPosition(uint16 _iObj)
{
	CAI_Core* pCharAI = GetAI(_iObj);
	if (pCharAI)
	{
		return(pCharAI->m_CurBaseMat.GetRow(3) + pCharAI->m_WeaponOff);
	}
	else
	{
		CWObject* pObj = m_pServer->Object_Get(_iObj);
		return GetCenterPosition(pObj);
	}
};

CVec3Dfp32 CAI_Core::GetAimPosition(CAI_AgentInfo* _pAgent)
{
	if (_pAgent)
	{
		return(GetAimPosition(_pAgent->GetObjectID()));
	}
	else
	{
		return(CVec3Dfp32(_FP32_MAX));
	}
};

CVec3Dfp32 CAI_Core::GetAimPosition(CWObject * _pChar)
{
	if (!m_pServer || !m_pGameObject || !_pChar)
		return CVec3Dfp32(_FP32_MAX);

	CAI_Core* pCharAI = GetAI(_pChar->m_iObject);
	if (pCharAI)
	{
		return(m_CurBaseMat.GetRow(3) + m_WeaponOff);
	}
	else
	{
		return GetCenterPosition(_pChar);
	}
};

//Gets the transformation matrix of the engine path of the given server id at the given time, or CMat4Dfp32(_FP32_MAX) if the object is not an engine path
CMat4Dfp32 CAI_Core::GetEnginePathMatrix(int _iPath, int _Time)
{
	if (!m_pServer)
		return CMat4Dfp32(_FP32_MAX, _FP32_MAX, _FP32_MAX, _FP32_MAX,
		                 _FP32_MAX, _FP32_MAX, _FP32_MAX, _FP32_MAX,
		                 _FP32_MAX, _FP32_MAX, _FP32_MAX, _FP32_MAX,
		                 _FP32_MAX, _FP32_MAX, _FP32_MAX, _FP32_MAX);

	CMTime CTime;
	CTime.CreateFromTicks(_Time, m_pServer->GetGameTickTime());
	if (_Time < 0)
	{
		CWObject_Message TimeMsg(OBJMSG_HOOK_GETTIME);
		TimeMsg.m_pData = &CTime;
		TimeMsg.m_DataSize = sizeof(CMTime);
		m_pServer->Message_SendToObject(TimeMsg, _iPath);
	}

	CMat4Dfp32 Res;
	if (m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX,(aint)(&CTime), (aint)(&Res)), _iPath) == 1)
		return Res;
	else
		return CMat4Dfp32(_FP32_MAX, _FP32_MAX, _FP32_MAX, _FP32_MAX,
		                 _FP32_MAX, _FP32_MAX, _FP32_MAX, _FP32_MAX,
		                 _FP32_MAX, _FP32_MAX, _FP32_MAX, _FP32_MAX,
		                 _FP32_MAX, _FP32_MAX, _FP32_MAX, _FP32_MAX);
};


//Gets the position of the engine path of the given server id at the given time, or CVec3Dfp32(_FP32_MAX) if the object is not an engine path
CVec3Dfp32 CAI_Core::GetEnginePathPosition(int _iPath, int _Time)
{
	if (!m_pServer)
		return CVec3Dfp32(_FP32_MAX);

	return CVec3Dfp32::GetRow(GetEnginePathMatrix(_iPath, _Time), 3);	
};


//Gets the center position of the objects bounding box, or CVec3Dfp32(_FP32_MAX) if this fails
CVec3Dfp32 CAI_Core::GetCenterPosition(CWObject * _pObj)
{
	if (!_pObj)
		return CVec3Dfp32(_FP32_MAX);

	if (_pObj->GetAbsBoundBox())
	{
		CVec3Dfp32 Pos;
		(_pObj->GetAbsBoundBox())->GetCenter(Pos);
		return Pos;
	}
	else
		return CVec3Dfp32(_FP32_MAX);
};

//Gets the dimensions of the objects bounding box
CVec3Dfp32 CAI_Core::GetBoundBoxDimensions(CWObject * _pObj)
{
	if (!_pObj)
		return CVec3Dfp32(0);

	const CBox3Dfp32 * pBox = _pObj->GetAbsBoundBox();
	if (pBox)
		return pBox->m_Max - pBox->m_Min;
	else
		return CVec3Dfp32(0);
};

//Gets the position to focus on, which is activateposition for characters and center of bounding box for other objects
CVec3Dfp32 CAI_Core::GetFocusPosition(CWObject* _pObj)
{
	if (!_pObj)
		return CVec3Dfp32(_FP32_MAX);

	//Is this a character?
	if (IsCharacter(_pObj->m_iObject))
	{	//Jupp, use aim position!
		return GetTorsoPosition(_pObj);
	}
	else
	{
		//Nope, use center of bounding box if there is one
		CVec3Dfp32 Pos = GetCenterPosition(_pObj);
		return (VALID_POS(Pos)) ? Pos : _pObj->GetPosition();
	}
};				


//Get believed focus position
CVec3Dfp32 CAI_Core::GetFocusPosition(CAI_AgentInfo* _pInfo)
{
	return GetTorsoPosition(_pInfo);
};


//Get health of given agent
int CAI_Core::Health(int _iObj)
{
	return m_pGameObject->AI_GetRPGAttribute(AIQUERY_RPGATTRIBUTE_HEALTH);
};
int CAI_Core::Health()
{
	return Health(GetObjectID());
}


//Get maximum health of given agent
int CAI_Core::MaxHealth(int _iObj)
{
	return m_pGameObject->AI_GetRPGAttribute(AIQUERY_RPGATTRIBUTE_MAXHEALTH);
};
int CAI_Core::MaxHealth()
{
	return MaxHealth(GetObjectID());
}


//Check if we're below the given fraction of our maximum health
bool CAI_Core::IsHurt(fp32 _Threshold)
{
	if (!IsValid())
		return false;

	fp32 Frac = GetHealthFrac();

	if (Frac < Min(1.0f, _Threshold))
		return true;
	else
		return false;
};

fp32 CAI_Core::GetHealthFrac()
{
	if (!IsValid())
	{
		return(0.0f);
	}

	int HealthV = Health(GetObjectID());
	int MaxHealthV = MaxHealth(GetObjectID());

	if (MaxHealthV == 0)
	{
		return(0.0f);
	}

	fp32 rFrac = (fp32)HealthV / (fp32)MaxHealthV;

	return(rFrac);
};

//Check if given character is an enemy
bool CAI_Core::IsHostileOrWorse(CWObject * _pChar,bool _DeadOK)
{
	if ((_pChar)&&((_DeadOK)||(IsValidAgent(_pChar))))
	{
		CAI_AgentInfo * pInfo = m_KB.GetAgentInfo(_pChar->m_iObject);
		if (pInfo && pInfo->GetCurRelation() >= CAI_AgentInfo::HOSTILE)
			return true;
		else
			return false;
	}
	else
		//Invalid character is not enemy
		return false;
}


//Checks if character pointed at by given server index is an enemy. Returns character pointer if so, else NULL. 
CWObject* CAI_Core::IsHostileOrWorse(int _iObj,bool _DeadOK)
{
	CWObject* pObj = m_pServer->Object_Get(_iObj);
	if (IsHostileOrWorse(pObj))
		return pObj;
	else
		return NULL;
};

//Check if given character is an enemy
bool CAI_Core::IsEnemy(CWObject * _pChar,bool _DeadOK)
{
	if ((_pChar)&&((_DeadOK)||(IsValidAgent(_pChar))))
	{
		CAI_AgentInfo * pInfo = m_KB.GetAgentInfo(_pChar->m_iObject);
		if (pInfo && pInfo->GetRelation() >= CAI_AgentInfo::ENEMY)
			return true;
		else
			return false;
	}
	else
		//Invalid character is not enemy
		return false;
}


//Checks if character pointed at by given server index is an enemy. Returns character pointer if so, else NULL. 
CWObject* CAI_Core::IsEnemy(int _iObj,bool _DeadOK)
{
	CWObject* pObj = m_pServer->Object_Get(_iObj);
	if (IsEnemy(pObj))
		return pObj;
	else
		return NULL;
};


//Check if given character is an ally
bool CAI_Core::IsFriend(CWObject * _pChar)
{
	if ( _pChar &&
		IsValidAgent(_pChar))
	{
		CAI_AgentInfo * pInfo = m_KB.GetAgentInfo(_pChar->m_iObject);
		if (pInfo && pInfo->GetRelation() == CAI_AgentInfo::FRIENDLY)
			return true;
		else
			return false;
	}
	else
		//Invalid character is not enemy
		return false;
};


//Checks if character pointed at by given server index is an ally.. 
//Returns character pointer if so, else NULL. 
CWObject * CAI_Core::IsFriend(int _iObj)
{
	CWObject * pObj = m_pServer->Object_Get(_iObj);
	if (IsFriend(pObj))
		return pObj;
	else
		return NULL;
};



#ifndef M_DISABLE_TODELETE
//Check if the given position is in the given area info
bool CAI_Core::IsInArea(int _iArea, const CVec3Dfp32& _Pos)
{
	bool bRes;
	CWObject_Message Msg;
	Msg.m_Msg = CWObject_AreaInfo::OBJMSG_POSITION_IN_AREA;
	Msg.m_Param0 = (int)(&bRes);
	Msg.m_VecParam0 = _Pos;
	if (m_pServer->Message_SendToObject(Msg, _iArea))
		return bRes;
	else
		return false;
};
#endif


//Check if the given position is within the given objects bounding box
bool CAI_Core::IsInBoundBox(CWObject * _pObj, const CVec3Dfp32& _Pos)
{
	if (!_pObj && !_pObj->GetAbsBoundBox())
		//Invalid object
		return false;

	for (int i = 0; i < 3; i++)
	{
		if ( (_Pos[i] < _pObj->GetAbsBoundBox()->m_Min[i]) ||
			(_Pos[i] > _pObj->GetAbsBoundBox()->m_Max[i]) )
			return false;
	};
	return true;
};

//Resets path stuff
void CAI_Core::ResetPathSearch()
{
	ReleasePathSearch();
	ResetStuckInfo();
	m_bReachedPathDestination = false;
	m_bDirectPath = false;
	m_bJumpCheat = false;
	m_SearchStatus = SEARCH_NONE;
};

//Check if we are perfectly placed at given xy-position. Optionally we can demand that look is 
//perfect as well and change the height diff tolerancy
bool CAI_Core::IsPerfectlyPlaced(CVec3Dfp32 _Pos, CVec3Dfp32 _LookVec, fp32 _HeightDiffToleracy)
{
	//Check xy-distance diff
	if (XYDistanceSqr(m_pGameObject->GetPosition(), _Pos) > Sqr(0.01f))
		return false;

	//Check height diff
	if (Abs(_Pos[2] - m_pGameObject->GetPosition()[2]) > _HeightDiffToleracy)
		return false;

	//Check look
	if (VALID_POS(_LookVec))
	{
		fp32 LookLenSqr = _LookVec.LengthSqr();
		if (LookLenSqr != 1.0f)
		{
			//Safety check prior to normalizing
			if (LookLenSqr == 0)
				return false;
			_LookVec.Normalize();
		}

		//Check cos of angle
		if (_LookVec * CVec3Dfp32::GetRow(m_pGameObject->GetPositionMatrix(), 0) < 0.999999f)
			return false;
	}

	//All checks passed
	return true;
}


// Returns MOVEMODE_CROUCH, MOVEMODE_STAND, MOVEMODE_WALK or MOVEMODE_RUN
int CAI_Core::GetMoveMode()
{
	if (CWObject_Character::Char_GetPhysType(m_pGameObject) == PLAYER_PHYS_CROUCH)
	{	// If crouching we don't care if we're moving or not, we assume that
		// the move speed is slow enough not to be a factor of visibility.
		return(MOVEMODE_CROUCH);
	}
	int iControlMode = CWObject_Character::Char_GetControlMode(m_pGameObject);
	switch(iControlMode)
	{
		case PLAYER_CONTROLMODE_LEDGE:
		case PLAYER_CONTROLMODE_LEDGE2:
			return(MOVEMODE_LEDGE);
			break;
		case PLAYER_CONTROLMODE_LADDER:
			return(MOVEMODE_LADDER);
			break;
		case PLAYER_CONTROLMODE_HANGRAIL:
			return(MOVEMODE_HANGRAIL);
			break;
	}

	// We zero vertical component if negative to silence
	// falls but not jumps.
	CVec3Dfp32 PrevSpeed = m_pGameObject->GetPosition() - m_pGameObject->GetLastPosition();
	if (PrevSpeed[2] < 0.0f)
	{
		PrevSpeed[2] = 0.0f;
	}

	fp32 CurSpeedSqr = PrevSpeed.LengthSqr();
	if (CurSpeedSqr < Sqr(GetMaxSpeedWalk() / 2))
	{	// Half or less of walk speed: We're standing still
		return(MOVEMODE_STAND);
	}

	if (CurSpeedSqr >= Sqr(GetMaxSpeedForward()))
	{
		return(MOVEMODE_RUN);
	}

	return(MOVEMODE_WALK);
}

//Check if given object is a character
bool CAI_Core::IsCharacter(int _iObj)
{
	if (!m_pServer)
		return false;

	return CWObject_Character::IsCharacter(_iObj, m_pServer) != NULL;
};


//Checks if the given agent is "alive" and not the bot himself
bool CAI_Core::IsValidAgent(CWObject * _pObj)
{
	if (_pObj && (_pObj != m_pGameObject))
	{
		if (CWObject_Character::Char_GetPhysType(m_pGameObject) == PLAYER_PHYS_NOCLIP)
		{
			return true;
		}
		else
		{
			CWObject_Interface_AI* pObjAI = _pObj->GetInterface_AI();
			if (pObjAI)
				return pObjAI->AI_IsAlive();
		}
	}
	return false;
};

bool CAI_Core::IsValidTarget(int _iTarget)
{
	CAI_Core* pTargetAI = GetAI(_iTarget);
	if (pTargetAI)
	{
		if (!pTargetAI->IsConscious())
		{
			return(false);
		}
	}
	else
	{	// We dare attack inanimate objects (those lacking ai ;)
		if (m_pServer->Object_Get(_iTarget))
		{
			return(true);
		}
		else
		{
			return(false);
		}		
	}

	if (m_AH.m_iEnemyOverride == _iTarget)
	{
		return(true);
	}

	// Compare the weapontypes of us and it to see if our bravery allows us to attack it
	if (m_Bravery == BRAVERY_ALWAYS)
	{	// We attack anything
		return(true);
	}
	else if (m_Bravery == BRAVERY_NEVER)
	{	// We attack nothing
		return(false);
	}
	else
	{
		int OurArmsClass = m_Weapon.GetWieldedArmsClass();
		int TargetArmsClass = pTargetAI->m_Weapon.GetWieldedArmsClass();
		if (m_Bravery == BRAVERY_BETTER)
		{	// OurArmsClass must be better than TargetArmsClass
			if (OurArmsClass > TargetArmsClass)
			{
				return(true);
			}
			else
			{
				return(false);
			}
		}
		else if (m_Bravery == BRAVERY_EQUAL)	// m_Bravery == BRAVERY_EQUAL
		{	// OurArmsClass must be at least equal to TargetArmsClass
			if (OurArmsClass >= TargetArmsClass)
			{
				return(true);
			}
			else
			{
				return(false);
			}
		}
		else	// BRAVERY_ONE_LESS
		{
			if (OurArmsClass+1 >= TargetArmsClass)
			{
				return(true);
			}
			else
			{
				return(false);
			}
		}
	}
}

//Checks if the given server index points to an agent that is "alive", not no-clipping and
//not character self. Returns a pointer to the object if so, NULL otherwise
CWObject * CAI_Core::IsValidAgent(int _iObj)
{
	CWObject * pObj;
	if ( m_pServer &&
		(pObj = m_pServer->Object_Get(_iObj)) &&
		IsValidAgent(pObj)) 
		return pObj;
	else
		return NULL;
};

// Returns, as a fractional value, the nbr of mags fired (0.0 means gun is full)
fp32 CAI_Core::GetMagsFired()
{
	fp32 rValue = 0.0f;
	if (m_Weapon.GetWieldedArmsClass() >= CAI_WeaponHandler::AI_ARMSCLASS_GUN)
	{
		CAI_Weapon* pAIWeapon = m_Weapon.GetWielded();
		if ((pAIWeapon)&&(pAIWeapon->GetItem()))
		{
			CRPG_Object_Item* pItem = pAIWeapon->GetItem();
			if (pItem)
			{
				if (pItem->m_MaxAmmo > 0)
				{
					rValue = fp32(pItem->m_ShotsFired) / fp32(pItem->m_MaxAmmo);
				}
			}
		}	
	}

	return(rValue);
}

// Tries to do a reload action, retuns true if successful
// _MagFrac is the (fractional) nbr of mags fired for a reload to occurs
bool CAI_Core::Reload(fp32 _MagFrac)
{
	if ((m_Weapon.GetWieldedArmsClass() >= CAI_WeaponHandler::AI_ARMSCLASS_GUN)&&
		(m_DeviceStance.GetIdleStance() > CAI_Device_Stance::IDLESTANCE_HOSTILE)&&
		(m_DeviceWeapon.IsAvailable()))
	{
		if (GetMagsFired() >= _MagFrac)
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
			if (pCD->m_AnimGraph2.GetStateFlagsLoCombined() & (AG2_STATEFLAG_EQUIPPING | AG2_STATEFLAG_HURTACTIVE))
			{
				return(false);
			}

			CAI_Weapon* pAIWeapon = m_Weapon.GetWielded();
			if (pAIWeapon)
			{
				CRPG_Object_Weapon* pWeapon = safe_cast<CRPG_Object_Weapon>(pAIWeapon->GetItem());
				if (pWeapon)
				{
					CWAG2I_Context AGContext(m_pGameObject,m_pServer,pCD->m_GameTime,m_pServer->GetGameTickTime());
					if (pCD->m_AnimGraph2.SendAttackImpulse(&AGContext,CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_RELOAD)))
					{
	#ifndef M_RTM
						if (DebugTarget())
						{
							CStr Name = m_pGameObject->GetName();
							ConOutL(Name+CStr(": Reloading ")+CStrF("%d Tick ",GetAITick()));
						}
	#endif
						// Set facial
						m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_IRRITATED,20);
						pWeapon->m_ShotsFired = 0;
						return(true);
					}
				}
			}
		}
	}
	return(false);
};

bool CAI_Core::CheckScenepoint(CWO_ScenePoint* _pSP)
{
	return(true);
};

//Check if the given object is AI-controlled
bool CAI_Core::IsBot(int _iObj)
{
	if (!m_pServer)
		return false;

	CWObject *pObj = m_pServer->Object_Get(_iObj);

	if(!pObj)
		return false;

	CWObject_Interface_AI *pInterface = pObj->GetInterface_AI();
	if(!pInterface)
		return false;

	return (pInterface->AI_IsBot() != 0);
};

bool CAI_Core::CanAddAgent()
{
	return(m_bAgent);
};

//Checks if the object of the given server index is a player
bool CAI_Core::IsPlayer(int _iObj)
{
	if (!m_pServer)
		return false;


	CAI_Core* pObjAI = GetAI(_iObj);
	if (pObjAI)
	{
		return(pObjAI->IsPlayer());
	}
	else
	{
		return(true);
	}
		/*
	CWObject_Game *pGame = m_pServer->Game_GetObject();
	for (int i = 0; i < pGame->Player_GetNum(); i++)
	{
		if (pGame->Player_GetObjectIndex(i) == _iObj)
			return true;
	}
	return false;
	*/
};


//Checks if 'this' is a player
bool CAI_Core::IsPlayer()
{
	if ((m_pGameObject)&&(m_pGameObject->AI_IsBot()))
	{
		return(false);
	}
	else
	{
		return(true);
	}
	/*
	if (!m_pServer)
		return false;

	CWObject_Game *pGame = m_pServer->Game_GetObject();
	for (int i = 0; i < pGame->Player_GetNum(); i++)
	{
		if (pGame->Player_GetObjectIndex(i) == GetObjectID())
			return true;
	}
	return false;
	*/
};

//Empty constructor
CAI_Core::CAI_Core()
{
	m_pGameObject = NULL;
	m_bWeightless = false;;
	m_pServer = NULL;
	m_pAIResources = NULL;
	m_ActionFromKey = -1;
	m_lActionParamKeys.Clear();

	m_AH.SetAI(this);
	m_bAgent = true;
	m_Awareness = -1;
	m_SightRange = -1;
	m_HearingRange = -1;
	m_FOV = -1; 
	m_FOVWide = -1;
	m_FakeFOVLight = 0.0f;
	m_TurnPerTick = -1;
	m_PerceptionFlags = -1;
	m_PerceptionFactor = -1;
	m_LoudMovement = 0.0f;
	m_Reasoning = -1;
	m_RoamMinRange = -1;
	m_RoamMaxRange = -1;
	m_SearchMinRange = -1;
	m_SearchMaxRange = -1;
	m_RangedMinRange = -1;
	m_RangedMaxRange = -1;
	m_MeleeMaxRange = -1;
	m_CloseMinRange = -1;
	m_CloseMaxRange = -1;
	m_CombatMinRange = -1;
	m_CombatMaxRange = -1;
	m_CombatTargetMinRange = -1;
	m_CombatTargetMaxRange = -1;
	m_CoverMinRange = -1;
	m_CoverMaxRange = -1;
	m_CoverTargetMinRange = -1;
	m_CoverTargetMaxRange = -1;
	m_JumpMinRange = -1;	// No jumping as a default
	m_JumpMaxRange = -1;	// No jumping as a default
	m_DefaultBravery = -1;
	m_Bravery = -1;
	m_Order = ORDER_NONE;
	m_OrderTicksRemaining = 0;
	m_iDragObject = 0;
	m_MeleeRelationIncrease = 0;
	m_RelationChangeAwareness = CAI_AgentInfo::NOTICED;
	m_Skill = -1;
	m_Patience = -1;
	m_HitRatio = -1;
	m_FriendlyFireFactor = 0.25f;	// 25 % damage from friendly fire
	m_SniperPenalty = -1;
	m_StationaryBonus = -1;
	m_Reaction = -1;
	m_PressDuration = 0;
	m_ShootPeriod = 20;
	m_bNextOfKin = false;
	m_AgonyBehaviour = 0;
	m_SecondaryWeaponAnimType = 0;

	m_UndeadSleepTicks = AI_UNDEAD_SLEEPTIME;
	m_UndeadDieBehaviour = 0;

	m_StealthTension = -1;
	m_StealthTensionTime = 0;
	m_SecurityTolerance = -1;

	m_bWallClimb = false;
	m_bCanWallClimb = false;
	m_ClimbEnableTicks = 0;
	m_ClimbDisableTicks = 0;
	m_bWorldspaceMovement = false;

	m_bCanJump = false;			// When TRUE bot can jump whenever m_bJump and m_JumpFlags allow it
	m_bJump = false;
	m_JumpFlags = 0;			// Flags for under which circumstances the bot can jump, still given by m_bCanJump above
	m_JumpDisableTicks = 0;		// Nbr of ticks of no jumping

	m_SeenDarknessPowers = 0;
	m_bFearDarkness = false;
	m_CharacterClass = CLASS_DEFAULT;
	m_Importance = IMPORTANCE_NORMAL;

	m_PF_BaseSize = -1;
	m_PF_Height = -1;
	m_PF_HeightCrouch = -1;
	m_PF_StepHeight = -1;
	m_PF_JumpHeight = -1;
	m_PF_WalkStepLength = -1;	
	m_PF_RunStepLength = -1;

	m_bWalkStarts = false;
	m_LastStairsType = AG2_IMPULSEVALUE_WALKSTAIR_STOP;
	m_iMoveVariant = 0;
	m_iFacialGroup = CAI_Device_Facial::FACIALGROUP_INVALID;
	m_iMountParam = 0;

	m_IdleMaxSpeed = -1;
	m_MaxEvadeSpeed = -1;
	m_ReleaseMaxSpeed = -1;
	m_ReleaseIdleSpeed = -1;
	m_ReleaseFlightSpeed = -1;
	m_ReleaseFlightSpeed = 1.0f;
	m_MinPropelStep = -1;
	m_FallenBehindRangeSqr = -1;
	m_CaughtUpRangeSqr = -1;
	m_StuckThreshold = 0;
	m_CollisionRank = 0;
	INVALIDATE_POS(m_CollAvoidancePos);
	m_CollisionAvoidanceCount = 0;
	m_bCollAvoidanceBlocked = false;
	m_bCollCWBlocked = false;
	m_bCollCCWBlocked = false;
	m_bCharColl = true;

	m_ActivateRangeSqr = -1;
	m_ActivationState = STATE_INACTIVE;
	m_DestroyOnDeactivationRangeSqr = -1.0f;
	m_ActivationTime = 100;
	m_ActivityScore = 0;
	m_ActivityLevel = ACTIVITY_FULL;
	m_ActivityTick = 0;
	
	m_bIgnoreCutscenes = false;
	m_StunTimeout = 0;

	m_LastMeleeTarget = 0;
	m_LastMeleeAttackTick = -10 * AI_KB_REFRESH_PERIOD;
	m_LastHurtTimer = -10 * AI_KB_REFRESH_PERIOD;

	m_bHostileFromBlowDmg = false;
	m_bPlayerOnlyMeleeDmg = false;
	m_DamageFromPlayerFactor = 1.0f;
	m_bDeathspots = false;

	m_LifeTimer = -1;
	m_LifeDrainAmount = 0;
	m_LifeDrainInterval = -1;
	m_MinHealth = 0;
	m_DestroyTimer = -1;
	m_PauseActions = false;
	m_iDialoguePartner = 0;
	m_bLookPartner = false;
	m_iCrowdFocus = 0;
	m_CurPriorityClass = CAI_Action::PRIO_MIN;

	m_UseFlags = USE_NONE;

	m_UseLightmeter = -1;
	m_spLightMeter = NULL;	// Don't allocate unless needed (yet, may change)
	m_bFlashlight = false;
	m_FlashThresholdOverride = 0.0f;
	m_FlashThresholdOverrideTimeout = -1;

	m_Script.SetAI(this);
	m_Script.Clear();
	m_EventHandler.Init(this);
	m_Controlhandler.SetAI(this);
	m_KB.SetAI(this);

	m_DeviceLook.SetAI(this);
	m_DeviceMove.SetAI(this);
	m_DeviceJump.SetAI(this);
	m_DeviceSound.SetAI(this);
	m_DeviceWeapon.SetAI(this);
	m_DeviceItem.SetAI(this);
	m_DeviceStance.SetAI(this);
	m_DeviceAnimation.SetAI(this);
	m_DeviceFacial.SetAI(this);
	m_DeviceDarkness.SetAI(this);

	m_lDevices[CAI_Device::LOOK] = &m_DeviceLook;
	m_lDevices[CAI_Device::MOVE] = &m_DeviceMove;
	m_lDevices[CAI_Device::JUMP] = &m_DeviceJump;
	m_lDevices[CAI_Device::SOUND] = &m_DeviceSound;
	m_lDevices[CAI_Device::WEAPON] = &m_DeviceWeapon;
	m_lDevices[CAI_Device::ITEM] = &m_DeviceItem;
	m_lDevices[CAI_Device::STANCE] = &m_DeviceStance;
	m_lDevices[CAI_Device::ANIMATION] = &m_DeviceAnimation;
	m_lDevices[CAI_Device::MELEE] = &m_DeviceMelee;
	m_lDevices[CAI_Device::FACIAL] = &m_DeviceFacial;
	m_lDevices[CAI_Device::DARKNESS] = &m_DeviceDarkness;
};


//Copy constructor
CAI_Core::CAI_Core(CAI_Core* _pAI_Core)
{
	if (!_pAI_Core)
	{	// CAI_Core()::CAI_Core() already implicitly called
		return;
	}

	//Iterator
	int i;

	m_bDirectPath = _pAI_Core->m_bDirectPath;
	m_PrevPos = _pAI_Core->m_PrevPos;

	m_pGameObject = _pAI_Core->m_pGameObject;
	m_bWeightless = _pAI_Core->m_bWeightless;
	m_pServer = _pAI_Core->m_pServer;
	m_pAIResources = _pAI_Core->m_pAIResources;
	m_SearchStatus = _pAI_Core->m_SearchStatus;
	m_bReachedPathDestination = _pAI_Core->m_bReachedPathDestination;
	m_ActionFlags = _pAI_Core->m_ActionFlags;

	m_BehaviourStopIssued = _pAI_Core->m_BehaviourStopIssued;
	m_StopDir = _pAI_Core->m_StopDir;
	m_bBehaviourRunning = _pAI_Core->m_bBehaviourRunning;
	m_bBehaviourMainRunning = _pAI_Core->m_bBehaviourMainRunning;
	m_bBehaviourLooping = _pAI_Core->m_bBehaviourLooping;
	m_iBehaviourRunning = _pAI_Core->m_iBehaviourRunning;
	m_BehaviourPrioClass = _pAI_Core->m_BehaviourPrioClass;
	m_iPendingBehaviour = _pAI_Core->m_iPendingBehaviour;
	m_PendingBehaviourPrioClass = _pAI_Core->m_PendingBehaviourPrioClass;
	m_BehaviourExpirationTick = _pAI_Core->m_BehaviourExpirationTick;
	m_pBehaviourScenePoint = _pAI_Core->m_pBehaviourScenePoint;

	m_iCurBehaviour = _pAI_Core->m_iCurBehaviour;
	m_iCurMainBehaviour = _pAI_Core->m_iCurMainBehaviour;
	m_iPrevBehaviour = _pAI_Core->m_iPrevBehaviour;
	m_bBehaviourEntering = _pAI_Core->m_bBehaviourEntering;
	m_bBehaviourExiting = _pAI_Core->m_bBehaviourExiting;
	m_GestureTimer = _pAI_Core->m_GestureTimer;

	m_Weapon = _pAI_Core->m_Weapon; 
	m_Item = _pAI_Core->m_Item; 
	m_PathFinder = _pAI_Core->m_PathFinder;
	m_Path = _pAI_Core->m_Path;
	m_ActionFromKey = _pAI_Core->m_ActionFromKey;
	m_lActionParamKeys = _pAI_Core->m_lActionParamKeys;
	m_Personality.SetPersonality(_pAI_Core->m_Personality.GetPersonality());
	m_AH = _pAI_Core->m_AH;
	m_bFirstRefresh = _pAI_Core->m_bFirstRefresh;
	m_bCanDeactivateAG = _pAI_Core->m_bCanDeactivateAG;
	m_Awareness = _pAI_Core->m_Awareness;
	m_bAgent = _pAI_Core->m_bAgent;
	m_SightRange = _pAI_Core->m_SightRange;
	m_HearingRange = _pAI_Core->m_HearingRange;
	m_FOV = _pAI_Core->m_FOV;
	m_FOVWide = _pAI_Core->m_FOVWide;
	m_FakeFOVLight = _pAI_Core->m_FakeFOVLight;
	m_TurnPerTick = _pAI_Core->m_TurnPerTick;
	m_PerceptionFlags = _pAI_Core->m_PerceptionFlags;
	m_PerceptionFactor = _pAI_Core->m_PerceptionFactor;
	m_LoudMovement = _pAI_Core->m_LoudMovement;
	m_Reasoning = _pAI_Core->m_Reasoning;

	m_RoamMinRange = _pAI_Core->m_RoamMinRange;
	m_RoamMaxRange = _pAI_Core->m_RoamMaxRange;
	m_SearchMinRange = _pAI_Core->m_SearchMinRange;
	m_SearchMaxRange = _pAI_Core->m_SearchMaxRange;;
	m_RangedMinRange = _pAI_Core->m_RangedMinRange;
	m_RangedMaxRange = _pAI_Core->m_RangedMaxRange;
	m_MeleeMaxRange = _pAI_Core->m_MeleeMaxRange;
	m_CloseMinRange = _pAI_Core->m_CloseMinRange;
	m_CloseMaxRange = _pAI_Core->m_CloseMaxRange;
	m_CombatMinRange = _pAI_Core->m_CombatMinRange;
	m_CombatMaxRange = _pAI_Core->m_CombatMaxRange;
	m_CombatTargetMinRange = _pAI_Core->m_CombatTargetMinRange;
	m_CombatTargetMaxRange = _pAI_Core->m_CombatTargetMaxRange;
	m_CoverMinRange = _pAI_Core->m_CoverMinRange;
	m_CoverMaxRange = _pAI_Core->m_CoverMaxRange;
	m_CoverTargetMinRange = _pAI_Core->m_CoverTargetMinRange;
	m_CoverTargetMaxRange = _pAI_Core->m_CoverTargetMaxRange;
	m_JumpMinRange = _pAI_Core->m_JumpMinRange;
	m_JumpMaxRange = _pAI_Core->m_JumpMaxRange;
	m_DefaultBravery = _pAI_Core->m_DefaultBravery;
	m_Bravery = _pAI_Core->m_Bravery;
	m_Order = _pAI_Core->m_Order;
	m_OrderTicksRemaining = _pAI_Core->m_OrderTicksRemaining;
	m_iDragObject = _pAI_Core->m_iDragObject;
	m_MeleeRelationIncrease = _pAI_Core->m_MeleeRelationIncrease;
	m_RelationChangeAwareness = _pAI_Core->m_RelationChangeAwareness;
	m_Skill = _pAI_Core->m_Skill;
	m_Patience = _pAI_Core->m_Patience;
	m_HitRatio = _pAI_Core->m_HitRatio;
	m_FriendlyFireFactor = _pAI_Core->m_FriendlyFireFactor;

	m_SniperPenalty = _pAI_Core->m_SniperPenalty;
	m_StationaryBonus = _pAI_Core->m_StationaryBonus;
	m_Reaction = _pAI_Core->m_Reaction;
	m_bNextOfKin = _pAI_Core->m_bNextOfKin;
	m_AgonyBehaviour = _pAI_Core->m_AgonyBehaviour;
	m_SecondaryWeaponAnimType = _pAI_Core->m_SecondaryWeaponAnimType;

	m_UndeadSleepTicks = _pAI_Core->m_UndeadSleepTicks;
	m_UndeadDieBehaviour = _pAI_Core->m_UndeadDieBehaviour;

	m_StealthTension = _pAI_Core->m_StealthTension;
	m_StealthTensionTime = _pAI_Core->m_StealthTensionTime;
	m_SecurityTolerance = _pAI_Core->m_SecurityTolerance;

	m_bWallClimb = _pAI_Core->m_bWallClimb;
	m_bCanWallClimb = _pAI_Core->m_bCanWallClimb;
	m_ClimbEnableTicks = _pAI_Core->m_ClimbEnableTicks;
	m_ClimbDisableTicks = _pAI_Core->m_ClimbDisableTicks;
	m_bWorldspaceMovement = _pAI_Core->m_bWorldspaceMovement;

	m_bCanJump = _pAI_Core->m_bCanJump;						// When TRUE bot can jump whenever m_bJump and m_JumpFlags allow it
	m_bJump = _pAI_Core->m_bJump;							// 
	m_JumpFlags = _pAI_Core->m_JumpFlags;					// Flags for under which circumstances the bot can jump, still given by m_bCanJump above
	m_JumpDisableTicks = _pAI_Core->m_JumpDisableTicks;		// Nbr of ticks of no jumping

	m_SeenDarknessPowers = _pAI_Core->m_SeenDarknessPowers;
	m_bFearDarkness = _pAI_Core->m_bFearDarkness;

	m_CharacterClass = _pAI_Core->m_CharacterClass;
	m_Importance = _pAI_Core->m_Importance;

	m_LookAngles = _pAI_Core->m_LookAngles;
	m_LookDir = _pAI_Core->m_LookDir;
	m_LookTime = _pAI_Core->m_LookTime;
	m_iLookAtObject = _pAI_Core->m_iLookAtObject;
	m_bLookSoft = _pAI_Core->m_bLookSoft;
	
	m_Timer = _pAI_Core->m_Timer;
	m_ServerTick = _pAI_Core->m_ServerTick;
	m_ServerTickFraction = _pAI_Core->m_ServerTickFraction;
	m_bServerTickAdvanced = _pAI_Core->m_bServerTickAdvanced;

	m_HoldPosition = _pAI_Core->m_HoldPosition;
	m_HoldLookDir = _pAI_Core->m_HoldLookDir;
	m_PendingAlarmTime = _pAI_Core->m_PendingAlarmTime;
	m_PendingAlarmMsg = _pAI_Core->m_PendingAlarmMsg;

	m_PathPrio = _pAI_Core->m_PathPrio;

	m_iHostile = _pAI_Core->m_iHostile;
	m_bNewHostility = _pAI_Core->m_bNewHostility;
	m_PathDestination = _pAI_Core->m_PathDestination;
	m_SubDest = _pAI_Core->m_SubDest;

	m_bWasAlive = _pAI_Core->m_bWasAlive;
	m_bWasValid = _pAI_Core->m_bWasValid;

	m_PF_BaseSize = _pAI_Core->m_PF_BaseSize;
	m_PF_Height = _pAI_Core->m_PF_Height;
	m_PF_HeightCrouch = _pAI_Core->m_PF_HeightCrouch;
	m_PF_StepHeight = _pAI_Core->m_PF_StepHeight;
	m_PF_JumpHeight = _pAI_Core->m_PF_JumpHeight;
	m_PF_WalkStepLength = _pAI_Core->m_PF_WalkStepLength;	
	m_PF_RunStepLength = _pAI_Core->m_PF_RunStepLength;

	m_bMDestDirty = _pAI_Core->m_bMDestDirty;
	m_MDestState = _pAI_Core->m_MDestState;
	m_MDest = _pAI_Core->m_MDest;
	m_MDestSpeed = _pAI_Core->m_MDestSpeed;

	m_HeadOff = _pAI_Core->m_HeadOff;
	m_LastBaseMat = _pAI_Core->m_LastBaseMat;
	m_CurBaseMat = _pAI_Core->m_CurBaseMat;
	m_InvBaseMat = _pAI_Core->m_InvBaseMat;
	m_LastHeadMat = _pAI_Core->m_LastHeadMat;
	m_CurHeadMat = _pAI_Core->m_CurHeadMat;
	m_LastLookPos = _pAI_Core->m_LastLookPos;
	m_CurLookPos = _pAI_Core->m_CurLookPos;
	m_CurLookDir = _pAI_Core->m_CurLookDir;
	m_iMountParam = _pAI_Core->m_iMountParam;
	m_CurHeight = _pAI_Core->m_CurHeight;
	m_bObjColl = _pAI_Core->m_bObjColl;
	m_bObjCollRequested = _pAI_Core->m_bObjCollRequested;
	m_LastValidGridPos = _pAI_Core->m_LastValidGridPos;

	m_bWalkStarts = _pAI_Core->m_bWalkStarts;
	m_LastStairsType = _pAI_Core->m_LastStairsType;

	m_iFacialGroup  = _pAI_Core->m_iFacialGroup;
	m_iMoveVariant = _pAI_Core->m_iMoveVariant;
	m_PrevSpeed = _pAI_Core->m_PrevSpeed;
	m_PrevMoveDir = _pAI_Core->m_PrevMoveDir;
	m_IdleMaxSpeed = _pAI_Core->m_IdleMaxSpeed;
	m_MaxEvadeSpeed = _pAI_Core->m_MaxEvadeSpeed;
	m_ReleaseMaxSpeed = _pAI_Core->m_ReleaseMaxSpeed;
	m_ReleaseIdleSpeed = _pAI_Core->m_ReleaseIdleSpeed;
	m_MaxSpeed = _pAI_Core->m_MaxSpeed;
	m_ReleaseFlightSpeed = _pAI_Core->m_ReleaseFlightSpeed;
	m_ReleaseFlightSpeed = _pAI_Core->m_ReleaseFlightSpeed;
	m_MinPropelStep = _pAI_Core->m_MinPropelStep;
	m_FallenBehindRangeSqr = _pAI_Core->m_FallenBehindRangeSqr;
	m_CaughtUpRangeSqr = _pAI_Core->m_CaughtUpRangeSqr;
	m_CollisionRank = _pAI_Core->m_CollisionRank;
	m_CollAvoidancePos = _pAI_Core->m_CollAvoidancePos;
	m_bCollAvoidanceBlocked = _pAI_Core->m_bCollAvoidanceBlocked;
	m_bCollCWBlocked = _pAI_Core->m_bCollCWBlocked;
	m_bCollCCWBlocked = _pAI_Core->m_bCollCCWBlocked;
	m_CollisionAvoidanceCount = _pAI_Core->m_CollisionAvoidanceCount;
	m_bCharColl = _pAI_Core->m_bCharColl;

	m_ActivateRangeSqr = _pAI_Core->m_ActivateRangeSqr;
	m_ActivationState = _pAI_Core->m_ActivationState;
	m_DestroyOnDeactivationRangeSqr = _pAI_Core->m_DestroyOnDeactivationRangeSqr;
	m_ActivationTime = _pAI_Core->m_ActivationTime;
	m_ActivationTick = _pAI_Core->m_ActivationTick;
	m_ActivityScore = _pAI_Core->m_ActivityScore;
	m_ActivityLevel = _pAI_Core->m_ActivityLevel;
	m_ActivityTick = _pAI_Core->m_ActivityTick;
	m_bIgnoreCutscenes = _pAI_Core->m_bIgnoreCutscenes;
	m_StunTimeout = _pAI_Core->m_StunTimeout;

	m_LastMeleeTarget = _pAI_Core->m_LastMeleeTarget;
	m_LastMeleeAttackTick = _pAI_Core->m_LastMeleeAttackTick;
	m_LastHurtTimer = _pAI_Core->m_LastHurtTimer;
	m_bHostileFromBlowDmg = _pAI_Core->m_bHostileFromBlowDmg;
	m_bPlayerOnlyMeleeDmg = _pAI_Core->m_bPlayerOnlyMeleeDmg;
	m_DamageFromPlayerFactor = _pAI_Core->m_DamageFromPlayerFactor;

	m_bDeathspots = _pAI_Core->m_bDeathspots;

	m_bNoEscapeSeqMoves = _pAI_Core->m_bNoEscapeSeqMoves;
	m_EscSeqPos = _pAI_Core->m_EscSeqPos;
	m_bIsStuck = _pAI_Core->m_bIsStuck;
	m_StuckThreshold = _pAI_Core->m_StuckThreshold; 
	m_StuckCounter = _pAI_Core->m_StuckCounter; 
	m_bJumpCheat = _pAI_Core->m_bJumpCheat;
	m_CurDestDistSqr = _pAI_Core->m_CurDestDistSqr;
	m_UseLightmeter = _pAI_Core->m_UseLightmeter;
	m_bFlashlight = _pAI_Core->m_bFlashlight;
	m_LifeTimer = _pAI_Core->m_LifeTimer;
	m_LifeDrainAmount = _pAI_Core->m_LifeDrainAmount;
	m_LifeDrainInterval = _pAI_Core->m_LifeDrainInterval;
	m_MinHealth = _pAI_Core->m_MinHealth;
	m_DestroyTimer = _pAI_Core->m_DestroyTimer;
	m_PauseActions = _pAI_Core->m_PauseActions;
	m_iDialoguePartner = _pAI_Core->m_iDialoguePartner;
	m_CurPriorityClass = _pAI_Core->m_CurPriorityClass;
	m_bLookPartner = _pAI_Core->m_bLookPartner;
	m_iCrowdFocus = _pAI_Core->m_iCrowdFocus;
	m_UseFlags = _pAI_Core->m_UseFlags;
	m_CurCBTimeout = 0;
	m_CurCBTarget = 0;
	m_ScenepointCBTimeout = 0;
	m_CurCB.Clear();
	m_CurCB = _pAI_Core->m_CurCB;
	m_PrevCB.Clear();
	m_PrevCB = _pAI_Core->m_PrevCB;
	m_lCombatBehaviour.Clear();
	for (i = 0; i < _pAI_Core->m_lCombatBehaviour.Len(); i++)
	{
		m_lCombatBehaviour.Add(_pAI_Core->m_lCombatBehaviour[i]);
	}

	m_Script = _pAI_Core->m_Script;
	m_EventHandler = _pAI_Core->m_EventHandler;
	m_Controlhandler = _pAI_Core->m_Controlhandler;
	m_KB = _pAI_Core->m_KB;

	m_DeviceLook = _pAI_Core->m_DeviceLook;
	m_DeviceMove = _pAI_Core->m_DeviceMove;
	m_DeviceJump = _pAI_Core->m_DeviceJump;
	m_DeviceSound = _pAI_Core->m_DeviceSound;
	m_DeviceWeapon = _pAI_Core->m_DeviceWeapon;
	m_DeviceItem = _pAI_Core->m_DeviceItem;
	m_DeviceStance = _pAI_Core->m_DeviceStance;
	m_DeviceAnimation = _pAI_Core->m_DeviceAnimation;
	m_DeviceMelee = _pAI_Core->m_DeviceMelee;
	m_DeviceDarkness = _pAI_Core->m_DeviceDarkness;

#ifndef M_RTM
	m_bAIOut = _pAI_Core->m_bAIOut;
	m_bRenderFOV = _pAI_Core->m_bRenderFOV;
	Time = _pAI_Core->Time;
	cmFrameTime = _pAI_Core->cmFrameTime;
	Clear = _pAI_Core->Clear;
	BotCoord = _pAI_Core->BotCoord;
#endif

	m_lDevices[CAI_Device::MOVE] = &m_DeviceMove;
	m_lDevices[CAI_Device::LOOK] = &m_DeviceLook;
	m_lDevices[CAI_Device::JUMP] = &m_DeviceJump;
	m_lDevices[CAI_Device::SOUND] = &m_DeviceSound;
	m_lDevices[CAI_Device::WEAPON] = &m_DeviceWeapon;
	m_lDevices[CAI_Device::ITEM] = &m_DeviceItem;
	m_lDevices[CAI_Device::STANCE] = &m_DeviceStance;
	m_lDevices[CAI_Device::ANIMATION] = &m_DeviceAnimation;
	m_lDevices[CAI_Device::FACIAL] = &m_DeviceFacial;
	m_lDevices[CAI_Device::MELEE] = &m_DeviceMelee;
	m_lDevices[CAI_Device::DARKNESS] = &m_DeviceDarkness;

	for (i = 0; i < CAI_Device::NUM_DEVICE; i++)
		m_lDevices[i]->ReInit(this);
	m_AH.ReInit(this);
	m_EventHandler.ReInit(this);
	m_Weapon.ReInit(this);
	m_Item.ReInit(this);
	m_Personality.ReInit(this);
	m_Script.ReInit(this);
	m_Controlhandler.ReInit(this);
	m_KB.ReInit(this);
	m_PathFinder.ReInit(this);
};


//Destructor 
CAI_Core::~CAI_Core()
{
	Destroy(false);
};

//Initializes the AI, setting it up for action...
void CAI_Core::Init(CWObject* _pObj, CWorld_Server* _pServer)
{
	MSCOPE(CAI_Core::Init, CHARACTER);

	//StartBM("Init");

#ifndef M_RTM
	Clear = false;//DEBUG
	INVALIDATE_POS(BotCoord);//DEBUG
	Time.Reset(); //DEBUG
	m_bAIOut = false; //DEBUG
	m_bRenderFOV = false; //DEBUG
	cmFrameTime.Reset(); //DEBUG
	m_bJustStarted = false;//DEBUG
	m_fPFTime = 0;//DEBUG
#endif

	m_pGameObject = _pObj->GetInterface_AI();
	m_pServer = _pServer;
	m_pAIResources = GetAIResources();

	m_Weapon = CAI_WeaponHandler(this);
	m_Item = CAI_ItemHandler(this);

	m_ActionFlags = 0;
	m_BehaviourStopIssued = BEHAVIOUR_STOP_NONE;
	m_StopDir = 0;
	m_bBehaviourRunning = false;
	m_bBehaviourMainRunning = false;
	m_bBehaviourLooping = false;
	m_iBehaviourRunning = BEHAVIOUR_NBR_STOPPED;
	m_iPendingBehaviour = BEHAVIOUR_NBR_STOPPED;
	m_BehaviourPrioClass = CAI_Action::PRIO_MIN;
	m_PendingBehaviourPrioClass = CAI_Action::PRIO_MIN;
	m_BehaviourExpirationTick = -1;
	m_pBehaviourScenePoint = NULL;

	m_iCurBehaviour = BEHAVIOUR_NBR_STOPPED;
	m_iCurMainBehaviour = BEHAVIOUR_NBR_STOPPED;
	m_iPrevBehaviour = BEHAVIOUR_NBR_STOPPED;
	m_bBehaviourEntering = false;
	m_bBehaviourExiting = false;
	m_GestureTimer = 0;

	m_CurCBTimeout = 0;
	m_CurCBTarget = 0;
	m_ScenepointCBTimeout = 0;
	m_CurCB.Clear();
	m_PrevCB.Clear();

	if (m_pGameObject)
		m_PrevPos = m_pGameObject->GetPosition();
	else
		INVALIDATE_POS(m_PrevPos);

	if (m_ActionFromKey == -1)
	{	// Hold is the default action (should be Explore but we have to live with our decisions)
		m_ActionFromKey = CAI_Core::HOLD;
	}

	m_bFirstRefresh = true;
	m_bCanDeactivateAG = 0;
	m_Timer = RoundToInt(Random * 100);
	m_ServerTick = 0;
	m_ServerTickFraction = 0.0f;
	m_bServerTickAdvanced = false;
	m_PendingAlarmTime = -1;

	if (m_Awareness == -1)
		m_Awareness = 48.0f;
	if (m_SightRange == -1)
		m_SightRange = 400.0f;
	if (m_HearingRange == -1)
		m_HearingRange = 200.0f;
	if (m_FOV == -1)
		m_FOV = 0.25f;// 90 degrees FOV   375f; //135 degrees FOV
	if (m_FOVWide == -1)
		m_FOVWide = 0.5f;// 180 degrees widefield
	if (m_PerceptionFlags == -1)
		m_PerceptionFlags = 0;	// No perception specials as default
	if (m_PerceptionFactor == -1)
		m_PerceptionFactor = 1.0f;
	if (m_Reasoning == -1)
		m_Reasoning = 50;

	if (m_RoamMinRange == -1)
		m_RoamMinRange = 96.0f;
	if (m_RoamMaxRange == -1)
		m_RoamMaxRange = 512.0;
	if (m_SearchMinRange == -1)
		m_SearchMinRange = 96.0f;
	if (m_SearchMaxRange == -1)
		m_SearchMaxRange = 512.0;
	if (m_RangedMinRange == -1)
		m_RangedMinRange = 32.0;
	if (m_RangedMaxRange == -1)
		m_RangedMaxRange = 640.0;
	if (m_MeleeMaxRange == -1)
		m_MeleeMaxRange = 64.0;

	if (m_CloseMinRange == -1)
		m_CloseMinRange = 32.0f;
	if (m_CloseMaxRange == -1)
		m_CloseMaxRange = 1024.0f;

	if (m_CombatMinRange == -1)
		m_CombatMinRange = 160.0f;
	if (m_CombatMaxRange == -1)
		m_CombatMaxRange = 640.0f;
	if (m_CombatTargetMinRange == -1)
		m_CombatTargetMinRange = 96.0f;
	if (m_CombatTargetMaxRange == -1)
		m_CombatTargetMaxRange = 1024.0f;

	if (m_CoverMinRange == -1)
		m_CoverMinRange = 160.0f;
	if (m_CoverMaxRange == -1)
		m_CoverMaxRange = 640.0f;
	if (m_CoverTargetMinRange == -1)
		m_CoverTargetMinRange  = 96.0f;
	if (m_CoverTargetMaxRange == -1)
		m_CoverTargetMaxRange = 1024.0f;

	if (m_DefaultBravery == -1)
	{
		m_DefaultBravery = BRAVERY_ALWAYS;
	}
	m_Bravery = m_DefaultBravery;
	if (m_Skill == -1)
		m_Skill = 50;
	if (m_Patience == -1)
		m_Patience = 10 * GetAITicksPerSecond();	// 10 seconds default patience
	if (m_Reaction == -1)
		m_Reaction = RoundToInt(0.1f * GetAITicksPerSecond()); //Default reaction time is 0.1 seconds
	if (m_StealthTension == -1)
	{
		m_StealthTension = TENSION_NONE;
		m_StealthTensionTime = 0;
	}
	if (m_HitRatio == -1)
		m_HitRatio = 50;
	if (m_SniperPenalty == -1)
		m_SniperPenalty = 0;
	if (m_StationaryBonus == -1)
		m_StationaryBonus = 1;

	INVALIDATE_POS(m_LookAngles);
	INVALIDATE_POS(m_LookDir);
	m_LookTime = 0;
	m_iLookAtObject = 0;
	m_bLookSoft = false;

	INVALIDATE_POS(m_HoldPosition);
	INVALIDATE_POS(m_HoldLookDir);

	// ResetThreat();
	m_iHostile = 0;
	m_bNewHostility = false;

	m_bWasAlive = false;
	m_bWasValid = false;

	//Make sure activation range is greater or equal to all of the sensory ranges
	if (m_ActivateRangeSqr == -1)
		m_ActivateRangeSqr = 2048*2048;
	m_ActivateRangeSqr = Max(m_ActivateRangeSqr, Max(Sqr(m_SightRange), Max(Sqr(m_HearingRange), Sqr(m_Awareness))));
	m_ActivationTick = 0;
	if (IsPlayer())
	{
		m_ActivationState = STATE_ALWAYSACTIVE;
		m_DestroyOnDeactivationRangeSqr = -1;
	};

	m_LastMeleeTarget = 0;
	m_LastMeleeAttackTick = -10 * AI_KB_REFRESH_PERIOD;
	m_LastHurtTimer = -10 * AI_KB_REFRESH_PERIOD;

	if (m_StuckThreshold == 0)
		m_StuckThreshold = 10;

	// Try getting the stuff from the char
	// Fill in defaults at all values that still are at -1
	if (m_PF_BaseSize == -1)
	{
		m_PF_BaseSize = AI_DEFAULT_BASESIZE;
	}
	if (m_PF_Height == -1)
	{
		m_PF_Height = AI_DEFAULT_HEIGHT;
	}
	if (m_PF_HeightCrouch == -1)
	{
		m_PF_HeightCrouch = AI_DEFAULT_HEIGHT_CROUCH;
	}
	if (m_PF_StepHeight == -1)
	{
		m_PF_StepHeight = AI_DEFAULT_STEP_HEIGHT;
	}
	if (m_PF_JumpHeight == -1)
	{
		m_PF_JumpHeight = AI_DEFAULT_JUMP_HEIGHT;
	}
	if (m_PF_WalkStepLength == -1)
	{
		m_PF_WalkStepLength = AI_DEFAULT_WALK_STEPLENGTH;
	}
	if (m_PF_RunStepLength == -1)
	{
		m_PF_RunStepLength = AI_DEFAULT_RUN_STEPLENGTH;
	}

	m_bMDestDirty = true;
	m_MDestState = DEST_NONE,
	m_MDest.Unit();
	m_MDestSpeed = 0.0f;

	m_CurHeight = 64.0f;
	m_HeadOff = CVec3Dfp32(0.0f,0.0f,m_CurHeight);
	m_LastBaseMat.Unit();
	m_CurBaseMat.Unit();
	m_InvBaseMat.Unit();
	m_LastHeadMat.Unit();
	m_CurHeadMat.Unit();
	m_LastLookPos = CVec3Dfp32(0.0f,0.0f,0.0f);
	m_CurLookPos = CVec3Dfp32(0.0f,0.0f,0.0f);
	m_CurLookDir = CVec3Dfp32(1,0,0);

	m_bObjColl = true;				// By default, obj coll are on
	m_bObjCollRequested = false;	// Set to true when we try turning on obj coll but have not yet found a valid pos
	INVALIDATE_POS(m_LastValidGridPos); 

	m_PrevSpeed = -1.0f;
	if (m_IdleMaxSpeed == -1)
		m_IdleMaxSpeed = GetMaxSpeedForward();
	if (m_MaxEvadeSpeed == -1)
		m_MaxEvadeSpeed = GetMaxSpeedForward();
	if (m_ReleaseMaxSpeed == -1)
		m_ReleaseMaxSpeed = GetMaxSpeedForward();
	if (m_ReleaseIdleSpeed == -1)
		m_ReleaseIdleSpeed = GetMaxSpeedWalk();
	m_MaxSpeed = GetMaxSpeedForward();
	if (m_ReleaseFlightSpeed == -1)
	{
		m_ReleaseFlightSpeed = m_ReleaseMaxSpeed;
	}
	if (m_MinPropelStep == -1)
		m_MinPropelStep = 0;
	if (m_FallenBehindRangeSqr == -1)
		m_FallenBehindRangeSqr = 32*32;
	if (m_CaughtUpRangeSqr == -1)
		m_CaughtUpRangeSqr = 16*16;

	m_PathPrio = -1;
	if (m_pServer)
	{	// *** Node expansions should be 64, not 256
		m_bWeightless = m_pGameObject->AI_IsWeightLess() != 0;
		if (m_pAIResources)
		{
			m_PathFinder.Init(this, m_pServer->Path_GetNavGraph(), m_pServer->Path_GetBlockNavSearcher(), &(m_pAIResources->ms_PFResource_Graph), &(m_pAIResources->ms_PFResource_Grid), 
				(int)GetBaseSize(), (int)GetCrouchHeight(), (int)GetHeight(), (int)GetStepHeight(), (int)GetJumpHeight(), 256, AI_PATHFIND_MICROSECONDS);
		}
		else
		{
			M_ASSERTHANDLER(!m_pAIResources, "No AI resources!",);
			m_PathFinder.Init(this, m_pServer->Path_GetNavGraph(), m_pServer->Path_GetBlockNavSearcher(), NULL, NULL, 
				(int)GetBaseSize(), (int)GetCrouchHeight(), (int)GetHeight(), (int)GetStepHeight(), (int)GetJumpHeight(), 256, AI_PATHFIND_MICROSECONDS);
		}
	}
	INVALIDATE_POS(m_PathDestination);
	INVALIDATE_POS(m_SubDest);
	ResetPathSearch();

	m_bNoEscapeSeqMoves = false;
	INVALIDATE_POS(m_EscSeqPos);
	m_bIsStuck = false;
	ResetStuckInfo();
	m_bJumpCheat = false;

	//StopBM();
};



//Destroys the AI, releasing any held resources
void CAI_Core::Destroy(bool _bRaiseOndie)
{
	CWObject_Game *pGame = m_pServer->Game_GetObject();
	if (!pGame)
		return; // prevent crashes during shutdown

	if (IsPlayer())
	{	// Reset dialogue starts in the unlikely (hah) event that the player dies without a save
		m_DeviceSound.ResetDialogueStartTicks();
	}
	ReleasePathSearch();

	if ((m_bWasAlive)&&(m_pGameObject))
	{
		OnDie(_bRaiseOndie);
	};

	//Remove self from global list of agents
	m_KB.Global_RemoveAgent(GetObjectID());
};


//Argh! Oh what a cruel world! Call when dying normally (i.e. not when script-destroyed)
void CAI_Core::OnDie(bool _bRaiseOndie)
{
	// Clear debugtarget when we die
	if (DebugTarget())
	{
		SetDebugTarget(0);
	}
	// Turn of the flashlight if need be
	if (m_pGameObject->ClientFlags() & PLAYER_CLIENTFLAGS_FLASHLIGHT)
	{
		m_pGameObject->ClientFlags() &= ~PLAYER_CLIENTFLAGS_FLASHLIGHT;
		CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
		if (pChar)
		{
			pChar->UpdateVisibilityFlag();
		}
	}

	// And shut the F*#% up!
	CWObject_Message Msg(OBJMSG_CHAR_DESTROYCAUSUALDIALOGUE);
	Msg.m_Param0 = CAI_Action::PRIO_FORCED;
	m_pServer->Message_SendToObject(Msg,m_pGameObject->m_iObject);

	//Raise OnDie event
	m_EventHandler.RaiseEvent(CAI_EventHandler::ON_DIE,0);
	m_bWasAlive = false;

	//Clear scripting
	m_Script.Clear();

	m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_DEAD);
};

CVec3Dfp32 CAI_Core::GetLookAngles(bool _Reverse)
{
	CVec3Dfp32 VFwd = GetLookDir();
	if (_Reverse)
	{
		VFwd = -VFwd;
	}
	CVec3Dfp32 Look;
	Look[0] = 0;	// No roll
	fp32 l = Length2(VFwd[0], VFwd[1]);
	Look[1] = CVec3Dfp32::AngleFromVector(l, VFwd[2]);
	if (Look[1] > 0.5f) Look[1] -= 1.0f;

	//Handle straight up/down look
	if ((M_Fabs(VFwd[0]) < _FP32_EPSILON) && (M_Fabs(VFwd[1]) < _FP32_EPSILON))
		Look[2] = 0.0f;
	else
		Look[2] = 1.0f - CVec3Dfp32::AngleFromVector(VFwd[0], VFwd[1]);

	return Look;
};

//Get given object's (tilt, pitch, heading) vector in fractions 
CVec3Dfp32 CAI_Core::GetLookAngles(CWObject* _pObj,bool _Reverse)
{
	if (!_pObj)
		return CVec3Dfp32(_FP32_MAX);

	CAI_Core* pObjAI = GetAI(_pObj->m_iObject);
	if (pObjAI)
	{
		return(pObjAI->GetLookAngles(_Reverse));
	}
	else
	{	// No AI
		CVec3Dfp32 VFwd = CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(), 0);
		if (_Reverse)
		{
			VFwd = -VFwd;
		}
		CVec3Dfp32 Look;
		Look[0] = 0;
		fp32 l = Length2(VFwd[0], VFwd[1]);
		Look[1] = CVec3Dfp32::AngleFromVector(l, VFwd[2]);
		if (Look[1] > 0.5f) Look[1] -= 1.0f;

		//Handle straight up/down look
		if ((M_Fabs(VFwd[0]) < _FP32_EPSILON) && (M_Fabs(VFwd[1]) < _FP32_EPSILON))
			Look[2] = 0.0f;
		else
			Look[2] = 1.0f - CVec3Dfp32::AngleFromVector(VFwd[0], VFwd[1]);

		return Look;
	}
};

// Converts Lookangles to direction _Look[Tilt,Pitch,Heading] to [x,y,z] ignoring tilt
CVec3Dfp32 CAI_Core::LookAngles2LookDir(CVec3Dfp32 _Look)
{
	CVec3Dfp32 Dir;

	Dir[0] = M_Cos(_Look[2]) * M_Cos(_Look[1]);
	Dir[1] = -M_Sin(_Look[2]) * M_Cos(_Look[1]);
	Dir[2] = M_Sin(_Look[1]);

	return(Dir);
}

//Returns the absolute difference in fractions between two angles. Return value is in the interval -0.5..0.5
fp32 CAI_Core::AngleDiff(fp32 _a1, fp32 _a2)
{
	fp32 Diff = _a1 - _a2;

	//Force difference to interval -1..1
	Diff = Diff - (int)Diff;
	//Force difference to interval -0.5..0.5
	if (Diff > 0.5f)
		Diff -= 1;
	else if (Diff < -0.5f)
		Diff += 1;

	return Diff;
};


//Returns the absolute difference in fractions between two angles. Return value is in the interval 0..0.5, of course
fp32 CAI_Core::AbsAngleDiff(fp32 _a1, fp32 _a2)
{
	fp32 Diff = _a1 - _a2;
	//Force difference to interval 0..1
	Diff = M_Fabs(Diff - (int)Diff);
	//Force difference to interval 0..0.5
	if (Diff > 0.5f)
		Diff = 1 - Diff;

	return Diff;
};


//Get pointer to NonZeroList with all spawned agents indices
CSimpleIntList* CAI_Core::GetAgents()
{
	return &(m_pAIResources->m_KBGlobals.ms_lAgents);
};

// Returns the relation the bot has with the player and various flags are set in _pioFlags if supplied
// and the corresponding flags are set.
int CAI_Core::GetPlayerRelation(int* _pioFlags)
{
	int iPlayer = GetClosestPlayer();
	CAI_AgentInfo* pPlayer = m_KB.GetAgentInfo(iPlayer);
	CAI_Core* pPlayerAI = GetAI(iPlayer);
	if ((iPlayer)&&(pPlayer)&&(pPlayerAI)&&(m_pGameObject)&&(m_pGameObject->m_iObject != iPlayer))
	{
		int Relation = pPlayer->GetCurRelation();
		if (_pioFlags)
		{
			int Flags = 0;
			if ((*_pioFlags & PLAYER_RELATION_SEENDARKNESS)&&(m_SeenDarknessPowers))
			{
				Flags |= PLAYER_RELATION_SEENDARKNESS; 
			}
			if ((*_pioFlags & PLAYER_RELATION_PANICKED)&&(m_CharacterClass == CLASS_CIV)&&(Relation >= CAI_AgentInfo::ENEMY))
			{
				Flags |= PLAYER_RELATION_PANICKED;
			}
			else if ((*_pioFlags & PLAYER_RELATION_SCARED)&&(m_CharacterClass == CLASS_CIV)&&(Relation >= CAI_AgentInfo::HOSTILE))
			{
				Flags |= PLAYER_RELATION_SCARED;
			}
			if ((*_pioFlags & PLAYER_RELATION_COULDATTACK)&&IsValidTarget(iPlayer))
			{
				Flags |= PLAYER_RELATION_COULDATTACK;
			}
			if ((*_pioFlags & PLAYER_RELATION_IN_FOV)&&IsPlayerLookWithinAngle(30))
			{
				Flags |= PLAYER_RELATION_IN_FOV;
			}
			else if ((*_pioFlags & PLAYER_RELATION_WAYBEYOND_FOV)&&(!IsPlayerLookWithinAngle(90)))
			{	// No need to calculate PLAYER_RELATION_WAYBEYOND_FOV if within PLAYER_RELATION_IN_FOV
				Flags |= PLAYER_RELATION_WAYBEYOND_FOV;
			}
			if (*_pioFlags & (PLAYER_RELATION_BEYOND_RANGE | PLAYER_RELATION_WAYBEYOND_RANGE))
			{
				fp32 RangeSqr = SqrDistanceToUs(iPlayer);
				if (RangeSqr > Sqr(AI_BEYOND_DIALOGUERANGE))
				{
					Flags |= PLAYER_RELATION_BEYOND_RANGE;
					if (RangeSqr > Sqr(AI_WAYBEYOND_DIALOGUERANGE))
					{
						Flags |= PLAYER_RELATION_WAYBEYOND_RANGE;
					}
				}
			}
			if (*_pioFlags & PLAYER_RELATION_GUNOUT)
			{
				if (pPlayerAI->m_Weapon.GetWieldedArmsClass() >= CAI_WeaponHandler::AI_ARMSCLASS_GUN)
				{
					Flags |= PLAYER_RELATION_GUNOUT;
					if (*_pioFlags & PLAYER_RELATION_GUNAIM)
					{
						if (IsPlayerLookWithinAngle(15))
						{
							Flags |= PLAYER_RELATION_GUNAIM;
						}
					}
				}
			}

			*_pioFlags = Flags;
		}
		return(Relation);
	}
	else
	{
		return(CAI_AgentInfo::RELATION_INVALID);
	}
};

int CAI_Core::GetCurrentPriorityClass()
{
	if (m_Script.IsValid())
	{
		m_CurPriorityClass = CAI_Action::PRIO_IDLE;
		return(m_CurPriorityClass);
	}

	if ((m_pGameObject)&&((!m_pGameObject->AI_IsAlive())||(m_Script.IsPaused())||(m_StunTimeout != 0)))
	{
		return CAI_Action::PRIO_MAX;
	}

	int Tension = GetStealthTension();
	if (Tension > TENSION_HOSTILE)
	{
		return CAI_Action::PRIO_DANGER;
	}

	m_CurPriorityClass = m_AH.GetCurrentPriorityClass();
	if (m_CurPriorityClass >= CAI_Action::PRIO_DANGER)
	{
		return CAI_Action::PRIO_DANGER;
	}

	return(m_CurPriorityClass);
};


//Gets all object server indices with the specified target name on the specified server and 
//puts them into the given array
void CAI_Core::GetTargetIndices(CStr _sTargetName, CWorld_Server * _pServer, TArray<int16>& _lResult)
{ 
	//Find all objects with the given name
	const int16* piObj;
	int nObj;

	//Set up selection
	TSelection<CSelection::LARGE_BUFFER> Selection;

	//Add objects and shut down selection
	_pServer->Selection_AddTarget(Selection, _sTargetName);
	nObj = _pServer->Selection_Get(Selection, &piObj);

//	const char * s = _sTargetName.Str();

	//Append result to array
	_lResult.Insertx(_lResult.Len(), piObj, nObj); 
};


//Add indices of all human players to given array
void CAI_Core::AddPlayers(CWorld_Server * _pServer, TArray<int>& _lResult)
{
	CWObject * pChar;
	CWObject_Game *pGame = _pServer->Game_GetObject();
	for (int i = 0; i < pGame->Player_GetNum(); i++)
	{
		if ((pChar = pGame->Player_GetObject(i)) &&
			!(IsBot(pChar->m_iObject)))
		{
			_lResult.Add(pChar->m_iObject);
		};
	};
};


//Add indices of all human players to given array (int16)
void CAI_Core::AddPlayers(CWorld_Server * _pServer, TArray<int16>& _lResult)
{
	CWObject * pChar;
	CWObject_Game *pGame = _pServer->Game_GetObject();
	for (int i = 0; i < pGame->Player_GetNum(); i++)
	{
		if ((pChar = pGame->Player_GetObject(i)) &&
			!(IsBot(pChar->m_iObject)))
		{
			_lResult.Add(pChar->m_iObject);
		};
	};
};


//The distance in the horizontal plane only between two points
fp32 CAI_Core::XYDistance(const CVec3Dfp32& _p1, const CVec3Dfp32& _p2)
{
	return CVec3Dfp32(_p1[0] - _p2[0], _p1[1] - _p2[1], 0).Length();
};

//The squared distance in the horizontal plane only between two points
fp32 CAI_Core::XYDistanceSqr(const CVec3Dfp32& _p1, const CVec3Dfp32& _p2)
{
	return CVec3Dfp32(_p1[0] - _p2[0], _p1[1] - _p2[1], 0).LengthSqr();
};


//Adds commands that changes the characters look direction to look at the given position.
//This can change both the heading and pitch of the characters look direction.
// This change of direction will occur over _Time ticks
void CAI_Core::AddAimAtPositionCmd(const CVec3Dfp32& _Pos, int _Time)
{
	//Check that look device is free
	if (!m_DeviceLook.IsAvailable())
		return;

	OnTrackPos(_Pos,_Time,false,false);
};


//Adds commands that changes the movement direction of the character to 
//head towards the given position.
void CAI_Core::AddFacePositionCmd(const CVec3Dfp32& _Pos,int _Time)
{
	if (m_bWallClimb)
	{
		OnTrackPos(_Pos,_Time,true,false);
	}
	else
	{
		OnTrackPos(_Pos,_Time,true,false);
	}
};


//Adds commands to move towards the given position.
bool CAI_Core::AddMoveTowardsPositionCmd(const CVec3Dfp32& _Pos, fp32 _Speed)
{
	// We should never isssue any move commands during that crucial time when a behaviour is firing up
	if (m_bBehaviourRunning)
	{
#ifndef M_RTM
		ConOutL(CStrF("Movecommand during behaviour: %s", m_pGameObject->GetName()));
#endif
		return true;
	}

#ifndef M_RTM
	if (DebugTarget())
	{
 		m_pServer->Debug_RenderVertex(_Pos+CVec3Dfp32(0,0,8),kColorRed,1.0f);
		bool wtf = true;
	}
#endif
	// This is very important for wallclimbers, don't remove!
	if ((m_pGameObject->AI_IsForceRotated())||(GetUp() * GetLastUp() < 0.95f))
	{
#ifndef M_RTM
		if (DebugRender())
		{
			m_pServer->Debug_RenderOBB(m_CurBaseMat,CVec3Dfp32(16,16,16),kColorGreen,0.0f);
		}
#endif
		m_DeviceLook.Free();
		m_DeviceLook.Use();
		m_DeviceMove.UseLastMove(false);
		ResetStuckInfo();
		return true;
	}

	CVec3Dfp32 VelocityVector;
	//Speed is expected movement distance to next frame, which is limited by characters maximum speed
	//This is scaled down to the interval 0..1 when calculating the actual movement command.
	if (_Speed == -1.0f || _Speed > m_MaxSpeed)
	{	//Use maximum speed
		_Speed = 1.0;
	}
	else
	{
		_Speed /= m_MaxSpeed;
		if (_Speed < 0.15f)
		{
			_Speed = 0.15f;
		}
	}

	// If we're turn locked (perhaps due to a walkstart or walkstop) we propel our path forward
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
	if (pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOLOOK)
	{
		m_Path.FindNextClosestNode(GetBasePos(),true,48.0f);
		m_DeviceLook.Free();
		m_DeviceLook.Use();
		m_DeviceMove.Use();
		ResetStuckInfo();
		return true;
	}

	//We should track along movement heading if look device is available or if look target 
	//is bad (i.e. we shouldn't try to look in that direction given current movement)
	bool bLook = m_DeviceLook.IsAvailable();
	if (!bLook && m_DeviceLook.BadLookTarget(_Pos - GetBasePos()))
	{	//Current look target is bad, lets set new look
		m_DeviceLook.Free();
		bLook = true;
	}

	// Project _Pos onto our local plane
	CVec3Dfp32 OwnPos = GetBasePos();
	CVec3Dfp32 GoalPos = Proj2Base(_Pos);
	CVec3Dfp32 Diff = GoalPos - OwnPos;
	CVec3Dfp32 Up = GetUp() * 0.4f + GetLastUp() * 0.6f;
	Up.Normalize();

	if ((m_bWallClimb)&&(Diff.LengthSqr() < Sqr(m_PF_BaseSize * 0.5f)))
	{
		bLook = false;
	}

	// If Diff is nearly vertical relative our Up we should go with the last move command
	if ((m_bWallClimb)&&(!m_bWorldspaceMovement))
	{
		CVec3Dfp32 DiffNorm = _Pos - OwnPos;
		DiffNorm.Normalize();
		fp32 Dot = Up * DiffNorm;
		if (Abs(Dot) > 0.707f)
		{
			CVec3Dfp32 LastMove = m_DeviceMove.GetLastMove();
			if (LastMove.LengthSqr() > Sqr(0.1f))
			{
				m_DeviceMove.UseLastMove(false);
			}
			else
			{
				m_DeviceMove.Use(CVec3Dfp32(1,0,0)*_Speed);
			}
			return true;
		}
	}

	if ((bLook)&&(GetPrevSpeed() > 0.05f))	// Moving faster than 1 unit per second
	{	//Look device available, look in movement direction and move forward
		int32 lookTicks = 20;
		if (_Speed >= 0.75f)
		{
			lookTicks = 6;
		}

		OnTrackDir(GoalPos - OwnPos,0,lookTicks,true,false);
	}
	
	if (m_bWorldspaceMovement)
	{
		m_DeviceMove.Use((Diff.Normalize()) * _Speed);
		//We're moving (at least)
		SetActivityScore(CAI_ActivityCounter::MOVING);
		if ((m_DeviceLook.IsAvailable())&&(GetPrevSpeed() > 0.05f))
		{
			OnTrackDir(GoalPos - OwnPos,0,10,true,true);
		}
		return true;
	}
	
	// Handle stairs
	if ((m_CharacterClass != CLASS_DARKLING)&&(m_UseFlags & USE_STAIRS)&&(!m_bWallClimb))
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
		CWAG2I_Context Context(m_pGameObject,m_pServer,pCD->m_GameTime);
		int8 StairsType = pCD->m_PostAnimSystem.GetStairType();
		if (StairsType != m_LastStairsType)
		{	// TODO: Check for fwd movement
			if (((m_DeviceStance.GetIdleStance() <= CAI_Device_Stance::IDLESTANCE_IDLE)&&(_Speed < AI_MOVE_THRESHOLD_RUN)))
			{
				if (StairsType == CPostAnimSystem::FOOT_MOVEMENT_STAIR_UP)
				{	// Going up
					pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_WALKSTAIR,AG2_IMPULSEVALUE_WALKSTAIR_WALKUP),0);
				}
				else if (StairsType == CPostAnimSystem::FOOT_MOVEMENT_STAIR_DOWN)
				{	// Going down
					pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_WALKSTAIR,AG2_IMPULSEVALUE_WALKSTAIR_WALKDOWN),0);
				}
				else
				{	// Level
					pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_WALKSTAIR,AG2_IMPULSEVALUE_WALKSTAIR_STOP),0);
				}
				m_LastStairsType = StairsType;
			}
			else
			{
				pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_WALKSTAIR,AG2_IMPULSEVALUE_WALKSTAIR_STOP),0);
				m_LastStairsType = StairsType;
			}
		}
	}

	{
		//Calculate desired heading
		if (m_bWallClimb)
		{
			CVec3Dfp32 RotPos = m_pGameObject->GetPosition();
			CVec3Dfp32 DirWorld, DirObj;
			DirWorld = GoalPos - OwnPos;
#ifndef M_RTM
			m_pServer->Debug_RenderVertex(GoalPos,CPixel32(kColorRed));
#endif
			CMat4Dfp32 PosMat;
			GetBaseMat(PosMat);
			PosMat.UnitNot3x3();
			DirObj = DirWorld * m_InvBaseMat;
			VelocityVector = DirObj;
			VelocityVector[2] = 0.0f;
			VelocityVector.Normalize();
			VelocityVector *= _Speed;
			m_DeviceMove.Use(VelocityVector,false);
			// TempEnableWallClimb(CAI_Core::AI_TICKS_PER_SECOND);
			SetActivityScore(CAI_ActivityCounter::MOVING);
			return true;
		}
		else
		{
			CVec3Dfp32 LookAngles = m_DeviceLook.GetNextLook();
			if (INVALID_POS(LookAngles))
			{
				LookAngles = GetLookAngles();
			}
			fp32 Heading = HeadingToPosition(OwnPos, GoalPos, 0.5f, m_DeviceLook.GetNextLook()[2]);
			VelocityVector = CVec3Dfp32(_Speed * QCos(Heading * _PI2), _Speed * QSin(-Heading * _PI2), 0);
		}
		
		//Move!
		m_DeviceMove.Use(VelocityVector);
	}

	//We're moving (at least)
	SetActivityScore(CAI_ActivityCounter::MOVING);
	return true;
};


//Adds commands to jump towards the given position.
bool CAI_Core::AddJumpTowardsPositionCmd(const CMat4Dfp32* _pMat, int _Reason, int _Flags)
{
	if ((m_bCanWallClimb)&&(m_bWallClimb)&&(m_bCanJump)&&(_Reason & m_JumpFlags)&&(!m_bBehaviourRunning)&&(!m_JumpDisableTicks))
	{	// Check global resource
		if ((!m_pAIResources)||((_Reason != JUMPREASON_SCRIPT)&&(GetGlobalAITick() <= m_pAIResources->m_KBGlobals.ms_JumpTick)))
		{
			ResetStuckInfo();
			return(false);
		}

		if ((!(_Flags & JUMP_TO_AIR))&&(_pMat))
		{
			CVec3Dfp32 Diff = _pMat->GetRow(3) - GetBasePos();
			if ((Diff.LengthSqr() < Sqr(m_JumpMinRange))||(Diff.LengthSqr() > Sqr(m_JumpMaxRange)))
			{
				ResetStuckInfo();
				return(false);
			}
		}

		if (m_pGameObject->AI_CanJump(_Flags & JUMP_FROM_AIR))
		{
			if (m_pGameObject->AI_Jump(_pMat, _Flags))
			{	// Say something
				if (m_CharacterClass == CLASS_DARKLING)
				{
					switch(_Reason)
					{
					case JUMPREASON_IDLE:
					case JUMPREASON_STUCK:
					case JUMPREASON_NOGRID:
						{
							if ((_pMat)&&(SqrDistanceToUs(_pMat->GetRow(3)) > Sqr((m_JumpMinRange+m_JumpMaxRange)*0.5f)))
							{	
								UseRandom(CStr("Darkling idle jump"),CAI_Device_Sound::IDLE_LONGJUMP,CAI_Action::PRIO_COMBAT);
							}
							else
							{
								UseRandom(CStr("Darkling idle jump"),CAI_Device_Sound::IDLE_SHORTJUMP,CAI_Action::PRIO_COMBAT);
							}
						}
						TempDisableJump(m_Patience * 2);
						break;
					case JUMPREASON_SCRIPT:
						{
							if ((_pMat)&&(SqrDistanceToUs(_pMat->GetRow(3)) > Sqr((m_JumpMinRange+m_JumpMaxRange)*0.5f)))
							{	
								UseRandom(CStr("Darkling idle jump"),CAI_Device_Sound::IDLE_LONGJUMP,CAI_Action::PRIO_COMBAT);
							}
							else
							{
								UseRandom(CStr("Darkling idle jump"),CAI_Device_Sound::IDLE_SHORTJUMP,CAI_Action::PRIO_COMBAT);
							}
						}
						break;
					case JUMPREASON_COMBAT:
						{
							UseRandom(CStr("Darkling combat jump"),CAI_Device_Sound::COMBAT_ATTACKJUMP,CAI_Action::PRIO_COMBAT);
						}
						TempDisableJump(m_Patience * 2);
						break;
					case JUMPREASON_AVOID:
						{
							UseRandom(CStr("Darkling avoid jump"),CAI_Device_Sound::IDLE_AVOIDJUMP,CAI_Action::PRIO_DANGER);
						}
						TempDisableJump(m_Patience * 2);
						break;
					case JUMPREASON_RETARGET:
						{
							if ((_pMat)&&(SqrDistanceToUs(_pMat->GetRow(3)) > Sqr((m_JumpMinRange+m_JumpMaxRange)*0.5f)))
							{	
								UseRandom(CStr("Darkling retarget jump"),CAI_Device_Sound::IDLE_LONGJUMP,CAI_Action::PRIO_IDLE);
							}
							else
							{
								UseRandom(CStr("Darkling retarget jump"),CAI_Device_Sound::IDLE_SHORTJUMP,CAI_Action::PRIO_IDLE);
							}
						}
						TempDisableJump(m_Patience * 2);
						break;
					default:
						if (_Reason & JUMPREASON_COMBAT)
						{	// Scripted jumps may have JUMPREASON_SCRIPT and JUMPREASON_COMBAT OR:ed together
							UseRandom(CStr("Darkling combat jump"),CAI_Device_Sound::COMBAT_ATTACKJUMP,CAI_Action::PRIO_COMBAT);
						}
						else
						{
							if ((_pMat)&&(SqrDistanceToUs(_pMat->GetRow(3)) > Sqr((m_JumpMinRange+m_JumpMaxRange)*0.5f)))
							{	
								UseRandom(CStr("Darkling idle jump"),CAI_Device_Sound::IDLE_LONGJUMP,CAI_Action::PRIO_COMBAT);
							}
							else
							{
								UseRandom(CStr("Darkling idle jump"),CAI_Device_Sound::IDLE_SHORTJUMP,CAI_Action::PRIO_COMBAT);
							}
						}
						TempDisableJump(m_Patience * 2);
						break;
					}
				}
				if (m_pAIResources)
				{
					m_pAIResources->m_KBGlobals.ms_JumpTick = GetGlobalAITick() + m_Patience;
				}
				// TempEnableWallClimb(m_Patience);
				ResetStuckInfo();
				return(true);
			}
		}
		// No stuck while jumping
		ResetStuckInfo();
		return(false);
	}
	return(false);
};


void CAI_Core::SetPathDestination(const CVec3Dfp32& _Pos)
{
	if (!&_Pos)
		return;

	m_PathDestination = _Pos;
};									


//Checks if we've gotten stuck, assuming that we are moving along a path.
//Tightly tied to OnFollowPath and OnEscapeSequenceMove
bool CAI_Core::IsStuck(const CVec3Dfp32& _Dest)
{
	//We're never actually stuck if move device is unavailable
	//or if we're running a behaviour (should check for an animgraph flag instead, 
	//since there are other AG states when we never should consider ourselves stuck)
	if (!m_DeviceMove.IsAvailable() ||
		m_bBehaviourRunning)
	{
		ResetStuckInfo();
		return false;
	}

	//We consider ourselves to have gotten stuck if we fail to get closer to our current destination
	//during m_StuckThreshold consecutive frames time.
	fp32 DistSqr = ((INVALID_POS(_Dest)) ? _FP32_MAX : GetBasePos().DistanceSqr(_Dest));

	//Have we gotten closer or have we just struck out for a new destination?
	if ((m_CurDestDistSqr == -1)||
		(DistSqr < (m_CurDestDistSqr - 0.1f)))
	{
		//Jupp, reset stuck counter and set new min distance 
		ResetStuckInfo();
		m_CurDestDistSqr = DistSqr;
	}
	else
	{	//Nope, we might be stuck
		m_StuckCounter++;
	};

	//We're stuck if counter exceeds a certain limit
	if (m_StuckCounter > m_StuckThreshold)
	{
		int32 iBehaviour = GetCurrentBehaviour();
		if (iBehaviour)
		{
			m_bBehaviourRunning = true;
			m_bBehaviourMainRunning = true;
			m_iBehaviourRunning = iBehaviour;
			StopBehaviour(BEHAVIOUR_STOP_NORMAL,m_BehaviourPrioClass);
		}
		return(true);
	}
	else
	{
		return(false);
	}
};

void CAI_Core::ResetStuckInfo()
{
	m_StuckCounter = 0;
	m_CurDestDistSqr = -1;
};



//Add commands to follow the path at given speed , or maximum speed by default. 
void CAI_Core::OnFollowPath(fp32 _Speed)
{	
	//StartBM("OnFollowPath");
	fp32 GoalRange = _Speed / 2;
	if (GoalRange < 8.0f)
	{
		GoalRange = 8.0f;
	}

	//Set sub destination if necessary
	if (INVALID_POS(m_SubDest))
	{
		m_SubDest = m_PathDestination;
	}

	//Are we at path destination?
	CVec3Dfp32 ProjDest = m_PathDestination;
	CVec3Dfp32 BasePos = GetBasePos();
	if (m_bWallClimb)
	{
		ProjDest = Proj2Base(m_PathDestination);
	}
	fp32 DistSqr = BasePos.DistanceSqr(ProjDest);
	if (DistSqr < Sqr(GoalRange))
	{
		//We have reached the destination! (or at least as close as we can get)
		m_bReachedPathDestination = true;
		ResetStuckInfo();
		INVALIDATE_POS(m_SubDest);
		ResetPathSearch();
		OnIdle();
	}
	//Are we trying to move, but have gotten stuck?
	else if ((!m_bNoEscapeSeqMoves)&&
		(m_bIsStuck || IsStuck(m_SubDest)))
	{
		//Escape sequence move
		OnEscapeSequenceMove(_Speed);
	}
	else
	{
		//Normal path following
		int ActionMode = CAI_Path::NORMAL;
		int ActionFlags = CAI_Path::NONE;
		bool bDone = false;
		CVec3Dfp32 OldSubDest = m_SubDest;

		//DEBUG
#ifndef M_RTM
		CMTime PFT;
		TStart(PFT);
		if (CAI_Core::ms_bPFReset)
		{
			CAI_Core::ms_nPFFrames++;
			CAI_Core::ms_bPFReset = false;
		}
		//StartBM("GetCurDestination");
#endif
		//DEBUG
		CVec3Dfp32 PrevDest = m_SubDest;
		CVec3Dfp32 LookPos = CVec3Dfp32(_FP32_MAX);
		if (m_bWallClimb) // *** Maybe this should always be done
		{	// *** Note the long steplength
			m_SubDest = m_Path.GetCurDestination(GetBasePos(), ActionMode, ActionFlags, bDone, 32.0f, 512);
			m_SubDest = Proj2Base(m_SubDest);
			LookPos = m_Path.PropelPath(GetBasePos(),64.0f,false);
		}
		else
		{
			m_SubDest = m_Path.GetCurDestination(GetBasePos(), ActionMode, ActionFlags, bDone, GoalRange, 512);
			// Lookahead
			LookPos = m_Path.PropelPath(GetBasePos(),64.0f,false);
		}
		if ((INVALID_POS(LookPos))&&(SqrDistanceToUs(m_SubDest) >= Sqr(48.0f)))
		{
			LookPos = m_SubDest;
		}
		if ((VALID_POS(LookPos))&&(SqrDistanceToUs(LookPos) >= Sqr(32.0f))&&(SqrDistanceToUs(LookPos) <= Sqr(160.0f)))
		{
			LookPos[2] += GetCurHeight() * 0.75f;
			OnTrackPos(LookPos,20,false,false);
#ifndef M_RTM
			if (DebugTarget())
			{
				Debug_RenderVertex(LookPos,kColorYellow,1.0f);
			}
#endif
		}

		if ((ActionMode == CAI_Path::PAUSE)||(ActionMode == CAI_Path::CAREFUL))
		{
			ResetStuckInfo();
		}

		//DEBUG
#ifndef M_RTM
		//StopBM();
		TStop(PFT);
		ms_PFTime += PFT;
		if (PFT.GetTime() > ms_fMaxPFTimePerFrame)
			ms_fMaxPFTimePerFrame = PFT.GetTime(); 
		//if (PFT/GetCPUFrequency() > 0.01f)
		//	ConOutL(CStrF("Expensive pathfind: %f", PFT/GetCPUFrequency() * 1000));
#endif
		//DEBUG

		//When following path we always count as pathfinding even if we really aren't
		SetActivityScore(CAI_ActivityCounter::PATHFINDING);

		if (INVALID_POS(m_SubDest))
		{	//Invalid destination
			m_DeviceMove.Use();
			ResetPathSearch();
			//StopBM("OnFollowPath (invalid destination)");
			return;			
		}
		
		// Some smoothing
		if (VALID_POS(PrevDest))
		{

#ifndef M_RTM	//DEBUG
			// ***
			if (DebugTarget())
			{
				int Color = 0xffff0000;
				switch(m_Path.m_DebugReason)
				{
				case CAI_Path::DIRECT_PATH:
					Color = 0xff008800;	// DkGreen
					break;

				case CAI_Path::DIRECTNEXT_PATH:
					Color = 0xff00ff00;	// Green
					break;

				case CAI_Path::SWITCHING_PATH:
					Color = 0xffff0000;
					break;

				case CAI_Path::SWITCHINGSTEPS_PATH:
					Color = 0xffff0088;
					break;

				case CAI_Path::SWITCHINGACROSS_PATH:
					Color = 0xffff00ff;
					break;

				default:
					Color = 0xffffffff;
					break;
				}

				if (Color)
				{
					Debug_RenderWire(GetBasePos(),m_SubDest,Color);
					Debug_RenderWire(m_SubDest,m_SubDest + CVec3Dfp32(0,0,30),Color);
				}
			}
#endif	
		}

		//Reset stuck info if we've changed sub-destination
		if (m_SubDest != OldSubDest)
		{
#ifndef M_RTM
			if (ms_bMovementVerbose)
 				ConOutL(CStrF("AIMV (GTime: %3.2f)", m_pServer->GetGameTime().GetTime()) + GetDebugName(m_pGameObject) + CStr(" OnFollowPath changed Subdestination."));
#endif
			ResetStuckInfo();
		}

		//DEBUG
#ifndef M_RTM
  		if (ms_bMovementVerbose)
		{
			if (GetBasePos().DistanceSqr(m_pGameObject->GetLastPosition()) < 0.1f*0.1f)
				ConOutL(CStrF("AIMV (GTime: %3.2f)", m_pServer->GetGameTime().GetTime()) + GetDebugName(m_pGameObject) + CStr(" Possibly stuck."));
		}
#endif
		//DEBUG

		//Are we done?
		if (bDone)
		{
			m_bReachedPathDestination = true;
			ResetStuckInfo();
			INVALIDATE_POS(m_SubDest);
			m_DeviceMove.Use();
		}
		//Not done, we must move
		else if (!m_DeviceMove.IsAvailable())
		{
			//Move device locked! Can't change movement
			ResetStuckInfo();
			m_DeviceMove.Use();
		}
		else
		{
			//Time to move! 
			//Crouch if when appropriate.
			/*
			if (ActionFlags & CAI_Path::CROUCH)
			{
				m_DeviceStance.Use(CAI_Device_Stance::CROUCHING);
			}
			*/

			//How should we move?
			if (ActionMode == CAI_Path::PAUSE)
			{
				//Stand still or inch closer if really close
				m_DeviceMove.Use();

				//Although we are standing still this is quite normal
				ResetStuckInfo();
				m_bJumpCheat = false;
			}
			else if (ActionMode == CAI_Path::CAREFUL)
			{
				//Very careful ground move (or should we perhaps stop?)
				fp32 Speed = 0.2f * Min(_Speed, XYDistance(m_SubDest, GetBasePos()));
				AddMoveTowardsPositionCmd(m_SubDest, Speed); 

				//Although we might stand still this is quite normal
				ResetStuckInfo();
				m_bJumpCheat = false;
			}
			else if (ActionMode == CAI_Path::AIRBORNE)
			{
				//Fall!
				if (m_bWallClimb)
				{
					AddMoveTowardsPositionCmd(m_SubDest, _Speed);
				}
				else
				{
					fp32 Speed = XYDistance(m_SubDest, GetBasePos());
					AddMoveTowardsPositionCmd(m_SubDest, Speed); 
					//Continue any started jump cheat if still far from destination
					if (m_bJumpCheat && (Speed > 12))
					{
						CVec3Dfp32 CheatVel = m_SubDest - GetBasePos();
						CheatVel[2] = 0;
						CheatVel.Normalize();
						m_pServer->Object_AddVelocity(m_pGameObject->m_iObject, CheatVel);
					}
					else
					{
						m_bJumpCheat = false;
					}
				}
			}
	 		else if (ActionMode == CAI_Path::STOP)
			{
				//Careful ground move
				fp32 Speed = Min(_Speed, XYDistance(m_SubDest, GetBasePos()));
				AddMoveTowardsPositionCmd(m_SubDest, Speed); 
				m_bJumpCheat = false;
			}
			else
			{
				//Normal ground move
				AddMoveTowardsPositionCmd(m_SubDest, _Speed);
				m_bJumpCheat = false;
			}
		}
	}

	//StopBM();
}




//Make moves to escape from a "stuck" position
void CAI_Core::OnEscapeSequenceMove(fp32 _Speed)
{
//	bool bFirstStuck = false;

	if ((m_bCanWallClimb)&&(!m_bIsStuck))
	{
		m_bIsStuck = true;
		TempEnableWallClimb(AI_TICKS_PER_SECOND);
		if (m_bWallClimb)
		{
			// Jump straight up!
			// (Yes,ye old OnFrustration() strikes again in a slightly different form)
			CMat4Dfp32 Mat;
			GetBaseMat(Mat);
			CVec3Dfp32 Pos = GetBasePos();
			CVec3Dfp32 Up = GetUp();
			Pos += Up;
			Pos += Mat.GetRow(0) * (0.5f - Random);
			Pos += Mat.GetRow(1) * (0.5f - Random);
			Pos.SetRow(Mat,3);
			if (AddJumpTowardsPositionCmd(&Mat,CAI_Core::JUMPREASON_STUCK,CAI_Core::JUMP_FROM_AIR | CAI_Core::JUMP_TO_AIR))
			{
				ResetStuckInfo();
				return;
			}
		}
	}

	if (!m_bIsStuck)
	{
		//Init escape sequence
		// Disable wallclimbing for 3 seconds
		// *** TempDisableWallClimb(60);
		//We're never stuck with a brand new escape sequence!
		ResetStuckInfo();

		//Get initial escape position
		CVec3Dfp32 CurDest;
		if (VALID_POS(m_SubDest))
			CurDest = m_SubDest;
		else if (VALID_POS(m_PathDestination))
			CurDest = m_PathDestination;
		else
			CurDest = m_pGameObject->GetPosition() + CVec3Dfp32::GetMatrixRow(m_pGameObject->GetPositionMatrix(), 0) * 100;
		fp32 EscHeading = ((Random < 0.5f) ? 1 : -1) * (Random * 0.1f + 0.2f) + HeadingToPosition(m_pGameObject->GetPosition(), CurDest); 

		m_EscSeqPos = FindGroundPosition(EscHeading, 64, 16, 0.125f, 0.25f);

		if (INVALID_POS(m_EscSeqPos))
			m_EscSeqPos = FindGroundPosition(EscHeading + 0.5f, 64, 16, 0.125f, 0.25f);
		if (VALID_POS(m_EscSeqPos))
		{
			SetDestination(m_EscSeqPos);
		}
		m_bIsStuck = true;
	}

#ifndef M_RTM	//DEBUG
	if (VALID_POS(m_EscSeqPos))
	{
		Debug_RenderWire(m_EscSeqPos, m_EscSeqPos + CVec3Dfp32(0,0,100));//DEBUG
		Debug_RenderWire(m_EscSeqPos, m_pGameObject->GetPosition(), 0xffff0000);//DEBUG
		Debug_RenderWire(m_pGameObject->GetPosition(), m_pGameObject->GetPosition() + CVec3Dfp32(0,0,100), 0xffff0000);//DEBUG
	}
#endif			//DEBUG

	//Check if we've gotten stuck while escaping
	if (IsStuck(m_EscSeqPos))
	{	//We've gotten stuck again, get new escape position in random direction
		m_EscSeqPos = FindGroundPosition(Random, 64, 16, 0.25f);
		SetDestination(m_EscSeqPos);
		ResetStuckInfo();
	};

	// Close to m_EscSeqPos or small random chance that we abandon m_EscSeqPos
	if (((VALID_POS(m_EscSeqPos))&&(GetBasePos().DistanceSqr(m_EscSeqPos) < Sqr(16.0f)))||
		((m_Timer % 10 == 0)&&(Random < 0.1f)))
	{
		m_bIsStuck = false;
		ResetPathSearch();

		//If we've got a path destination, strike out anew!
		if (VALID_POS(m_PathDestination))
		{
			int32 moveResult = OnMove(m_PathDestination,_Speed,false,false,NULL);
			if (CheckMoveresult(moveResult,NULL))
			{	// Move action succeeded
				return;
			}
			else
			{	// Move action failed
				ConOutL("OnEscapeSequenceMove failed");
				INVALIDATE_POS(m_PathDestination);
			}
		}
	}

	if (VALID_POS(m_EscSeqPos))
	{
		AddMoveTowardsPositionCmd(m_EscSeqPos, Min(_Speed, XYDistance(m_pGameObject->GetPosition(), m_EscSeqPos)));
	}
	else
	{	//Should we get frustrated?
		OnFrustration();
		StopNow();
		m_DeviceMove.Use();
		m_bIsStuck = false;
	}
};

//Causes the bot to move towards it's current destination or the new destination if 
//such is given, at given speed or maximum speed as default
int32 CAI_Core::OnMove(const CVec3Dfp32& _NewDestination, fp32 _Speed, bool _bFollowPartial, bool _bPerfectPlacement, CWO_ScenePoint* _pScenePoint)
{
	if (m_pGameObject->AI_IsJumping())
	{
		ResetPathSearch();
		ResetStuckInfo();
		return MOVE_DEVICE_BUSY;
	}

	//Check that move device is avaliable
	if (!m_DeviceMove.IsAvailable())
	{
		return MOVE_DEVICE_BUSY;
	}

	// Teleporters try to move to the new positions instantly and blithely assumes they can.
	// Immobile handling etc TBD by Backman ;)
	if (m_UseFlags & USE_TELEPORTER)
	{
		m_pServer->Object_SetPosition(m_pGameObject->m_iObject, _NewDestination);		
		m_DeviceMove.Use();
		return MOVE_OK;
	}

	//Move straight towards position if we don't use pathfinding
	if (m_UseFlags & USE_NOPATHFINDING)
	{
		AddMoveTowardsPositionCmd(_NewDestination, _Speed);
		SetDestination();
		/* *** Don't dare use this just yet ***
		if (OnNudge(_NewDestination,_Speed))
		{
			return MOVE_DONE;
		}
		else
		{
			return MOVE_OK;
		}
		*/
	}

	//Full speed?
	if (_Speed < -1.0f)
	{
		_Speed = GetMaxSpeedForward();
	}
	
	/* *** Don't dare use this just yet ***
	// Nudge us the last part
	if (_pScenePoint)
	{
		if (OnNudge(_pScenePoint,_Speed,true))
		{
			return MOVE_DONE;
		}
	}
	else
	{
		if (OnNudge(_NewDestination,_Speed))
		{
			return MOVE_DONE;
		}
	}
	*/

	// ***
	if (DebugTarget())
	{
		bool wtf = true;
	}
	// ***

	//Check that we've got a valid pathfinder
	if (!m_PathFinder.IsValid())
	{	//Nope, do nothing
		m_DeviceMove.Use();
		if (DebugTarget())
		{
			CStr Name = m_pGameObject->GetName();
			ConOut(Name + CStr(": No pathfinder"));
		}
		if (m_Timer % 20 == 0)
		{
			SetWantedGesture(GESTURE_HUMAN_CHECK_LEFTRIGHT);
		}
		return MOVE_WAIT_NO_PATHFINDER;
	};

	//If we have been given a new destination, set the path destination to this 
	//and discard any old path and ongoing search
	if ((VALID_POS(_NewDestination)) && (_NewDestination.DistanceSqr(m_PathDestination) > 1*1))
	{
		m_PathDestination = _NewDestination;
		SetDestination(m_PathDestination);
		ResetPathSearch();
	};

	//Do we have a destination?
	if (INVALID_POS(m_PathDestination))
	{	//Nope, reset path stuff and return
		ResetPathSearch();
		m_DeviceMove.Use();
		if (DebugRender())
		{
			CStr Name = m_pGameObject->GetName();
			ConOut(Name + CStr(": No path destination"));
		}
		return MOVE_DST_INVALID;
	} 

	// We have a path destination
	// If there isn't an ongoing search or found path, create search
	if ((m_SearchStatus != CAI_Pathfinder::IN_PROGRESS)&&
		(m_SearchStatus != CAI_Pathfinder::PAUSED)&&
		(m_SearchStatus != CAI_Pathfinder::FOUND_PATH)&&
		!(_bFollowPartial && (m_SearchStatus == SEARCH_FOUND_PARTIAL_PATH)))
	{
		//Free any previous path search
		ResetPathSearch();

		CVec3Dfp32 PathPos = m_PathFinder.GetPathPosition(m_pGameObject);
		if (INVALID_POS(PathPos))
		{	//We must be airborne or something, check up and down and hope we find a good spot.
			if (m_bWallClimb)
			{	// We could be on a wall as well
				PathPos = m_PathFinder.GetPathPosition(GetBasePos(), 3, 3);
			}
			else
			{
				PathPos = m_PathFinder.GetPathPosition(GetBasePos(), 20, 0);
			}
			if (INVALID_POS(PathPos))
			{
				//We might be in a corner or other inaccessible spot, try a little wider
				PathPos = m_PathFinder.GetPathPosition(GetBasePos(), 5, 5);
#ifndef M_RTM		
				DebugDrawNavgrid(GetObjectID());
#endif
			}
		}

		if (INVALID_POS(PathPos))
		{
#ifndef M_RTM
			if (DebugRender())
			{
				m_pServer->Debug_RenderVertex(GetBasePos(),kColorRed, 5.0f, true);
				CStr Name = m_pGameObject->GetName();
				ConOut(Name + CStr(": GetPathPosition failed"));
			}
#endif
			ResetPathSearch();
			PathfindFailed(true);
			return(MOVE_SRC_INVALID);
		}

		SetActivityScore(CAI_ActivityCounter::PATHFINDING);
		m_SearchStatus = m_PathFinder.FindNewPath(PathPos,m_PathDestination,GetPathPrio(),_bFollowPartial,m_bWallClimb);
		SetDestination(m_PathDestination);
		ResetStuckInfo();
		// We do NOT break out here as the pathfinder may actually finish the pathfind on a single frame
		// return(true);
	}
	else if ((m_SearchStatus == CAI_Pathfinder::IN_PROGRESS)||
			 (m_SearchStatus == CAI_Pathfinder::PAUSED))
	{
		// Continue searching
		SetActivityScore(CAI_ActivityCounter::PATHFINDING);
		m_SearchStatus = m_PathFinder.FindPath();
	}
	
	
	if ((m_SearchStatus == CAI_Pathfinder::IN_PROGRESS)||
		(m_SearchStatus == CAI_Pathfinder::PAUSED))
	{	
		//Still pathfinding. We must wait for completion.
		ResetStuckInfo();
		m_DeviceMove.Use();
		if (m_Timer % 20 == 0)
		{
			SetWantedGesture(GESTURE_HUMAN_CHECK_LEFTRIGHT);
		}
		return(MOVE_WAIT_PATHFINDING);
	}

	if (m_SearchStatus == CAI_Pathfinder::INVALID)
	{
		if (DebugRender())
		{
			CStr Name = m_pGameObject->GetName();
			ConOut(Name + CStr(": m_SearchStatus == CAI_Pathfinder::INVALID"));
		}
		PathfindFailed(true);
		return(MOVE_PATHFIND_FAILURE);
	}

	//Check if we might have found a (partial) path
	if ((m_SearchStatus == CAI_Pathfinder::FOUND_PATH) ||
		(_bFollowPartial &&	((m_SearchStatus == CAI_Pathfinder::NO_PATH) || (m_SearchStatus == SEARCH_FOUND_PARTIAL_PATH))))
	{
		if (!m_Path.IsValid())
		{
			//Try to get current path
			m_PathFinder.GetPath(&m_Path);

			if (!m_Path.IsValid())
			{	// Path is bad, we bail out
				if ((!DebugTargetSet())||(DebugTarget()))
				{
					CStr Name = m_pGameObject->GetName();
					ConOut(Name + CStr(": Found path but it was invalid"));
				}
				PathfindFailed(true);
				return(MOVE_PATHFIND_FAILURE);
			}
			else if (m_SearchStatus == CAI_Pathfinder::NO_PATH)
			{	//We could recover a partial path
				m_SearchStatus = SEARCH_FOUND_PARTIAL_PATH;
			}
			else
			{	// Ah this is the magic part where we start walking
				// TODO: PropelPath 1-2 meters and see if we can go straight there to skip the first ugly kink
				int ActionMode = 0;
				int ActionFlags = 0;
				bool bDone = false;
				CVec3Dfp32 SubDest = m_Path.GetCurDestination(GetBasePos(), ActionMode, ActionFlags, bDone, 32.0f, 512);
				if (VALID_POS(SubDest))
				{	// Don't really care wether we succeed or not here
					// *** m_Path.CheckShortcut(48.0f,&SubDest); ***
					CheckWalkStart(SubDest);
				}
			}
		}

		// Path is valid yipee!!
		OnFollowPath(_Speed);
		return MOVE_OK;
	}

	if (m_SearchStatus == CAI_Pathfinder::NO_PATH)
	{
		return MOVE_PATHFIND_FAILURE;
	}

	if ((!DebugTargetSet())||(DebugTarget()))
	{
		CStr Name = m_pGameObject->GetName();
		ConOut(Name + CStrF(": Dunno how pathfinder got here, m_SearchStatus %d",m_SearchStatus));
	}

	// Trilla ned
	PathfindFailed(true);

	return(false);
}

// Starndardized handling of movement errcodes
bool CAI_Core::CheckMoveresult(int32 _MoveResult,CWO_ScenePoint* _pScenePoint)
{	
	if ((_pScenePoint)&&((_MoveResult == MOVE_DST_INVALID)||(_MoveResult == MOVE_PATHFIND_FAILURE)))
	{
#ifndef M_RTM
		if ((!DebugTargetSet())||(DebugTarget()))
		{
			CVec3Dfp32 Start = m_PathFinder.SafeGetPathPosition(GetBasePos(),2,1);
			CVec3Dfp32 End = m_PathFinder.SafeGetPathPosition(_pScenePoint->GetPosition(),2,1);
			ConOutL(CStrF("§c3f0WARNING: Unreachable ScenePoint! (User: %s(%d) Start: %d, %d, %d;  End:  %d, %d, %d; Sp: %s)",
				m_pGameObject->GetName(),
				GetObjectID(),
				TruncToInt(Start[0]), TruncToInt(Start[1]), TruncToInt(Start[2]),
				TruncToInt(End[0]), TruncToInt(End[1]), TruncToInt(End[2]),
				_pScenePoint->GetName().Str()));
		}
#endif
		_pScenePoint->InvalidateScenepoint(m_pServer,200);
	}
	
	if ((_pScenePoint)&&(_pScenePoint->GetType() & CWO_ScenePoint::ROAM)&&((_MoveResult == MOVE_WAIT_PATHFINDING)||(_MoveResult == MOVE_WAIT_NO_PATHFINDER)))
	{	// Handle waypoints
		CWO_ScenePoint* pWayPoint = m_AH.GetPrevScenepoint(0);
		if ((pWayPoint)&&(pWayPoint->IsWaypoint()))
		{	// Move towards the scenepoint
			AddMoveTowardsPositionCmd(_pScenePoint->GetPosition(),m_IdleMaxSpeed);
		}
	}

#ifndef M_RTM
	if (DebugTarget())
	{
		CStr Name = m_pGameObject->GetName();
		switch(_MoveResult)
		{
		case MOVE_DONE:
			// We're there dude!
			break;

		case MOVE_OK:
			// We're chugging along nicely, nothing to report
			break;

		case MOVE_WAIT_PATHFINDING:
			// Pathfinder is still busy
			break;

		case MOVE_WAIT_NO_PATHFINDER:
			// Waiting for free pathfinder
			break;

		case MOVE_DEVICE_BUSY:
			// Someone else has already used the move device
			ConOut(Name + CStr(": Move device busy"));
			break;

		case MOVE_DST_INVALID:
			// Startposition invalid: How the hell did we get here?
			ConOut(Name + CStr(": Move dest invalid"));
			break;

		case MOVE_SRC_INVALID:
			// Endposition invalid: Only SitFlag SPs should result in this
			ConOut(Name + CStr(": Move src invalid"));
			break;

		case MOVE_PATHFIND_FAILURE:
			// Other pathfind failure (timeout, missing pathfinder etc)
			ConOut(Name + CStr(": MOVE_PATHFIND_FAILURE"));
			break;

		default:
			break;
		}
	}
#endif



	if (_MoveResult >= MOVE_WAIT_NO_PATHFINDER)
	{
		return(true);
	}
	else
	{
		return(false);
	}
};

void CAI_Core::PathfindFailed(bool _bJump)
{
	if ((_bJump)&&(m_bCanWallClimb))
	{
		if (m_StuckCounter > m_StuckThreshold)
		{
			// Jump straight up!
			// (Yes,ye old OnFrustration() strikes again in a slightly different form)
			CMat4Dfp32 Mat;
			GetBaseMat(Mat);
			CVec3Dfp32 Pos = GetBasePos();
			CVec3Dfp32 Up = GetUp();
			Up += Mat.GetRow(0) * (0.5f - Random) * 1.5f;
			Up += Mat.GetRow(1) * (0.5f - Random) * 1.5f;
			Pos += Up;
			Pos.SetRow(Mat,3);

			//Jump!
			fp32 RangeSqr = SqrDistanceToUs(Pos);
			if (RangeSqr <= Sqr(m_JumpMaxRange))
			{
				TempEnableWallClimb(1);
				if (AddJumpTowardsPositionCmd(&Mat,CAI_Core::JUMPREASON_STUCK,0))
				{	// Hoppa upp
					TempEnableWallClimb(m_Patience);
					ResetPathSearch();
					m_DeviceMove.Use();
					m_AH.ExpireActions(CAI_Action::INVALID);
					return;
				}
			}
			ResetStuckInfo();
		}
		else
		{
			m_StuckCounter++;
		}
	}

	// Expire all actions, maybe we will choose more wisely next time around
	m_AH.ExpireActions(CAI_Action::INVALID);
	ResetPathSearch();
	m_DeviceMove.Use();
};

bool CAI_Core::CheckWalkStop(CVec3Dfp32 _Dest, fp32 _Range)
{
	// AG2_STATEFLAG_NOLOOK dudes are probably already walkstarting/walkstopping 
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
	if ((!pCD)||(pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOLOOK))
	{
		return(false);
	}

	if ((INVALID_POS(_Dest))||(!m_bWalkStarts)||(m_KB.GetCollider())||(VALID_POS(m_CollAvoidancePos)))
	{
		return(false);
	}

	CVec3Dfp32 OurPos = GetBasePos();
	fp32 RangeSquare = OurPos.DistanceSqr(_Dest);
	if ((RangeSquare <= Sqr(1.5f * _Range))&&(RangeSquare >= Sqr(_Range)))
	{
		CVec3Dfp32 ToDestN = OurPos - _Dest;
		ToDestN.Normalize();
		// *** Until we have walkstops in all directions we must make sure that we only walkstop when the eenemy is to the front
		CAI_AgentInfo* pTarget = m_AH.AcquireTarget();
		if ((pTarget)&&(pTarget->GetCurRelation() >= CAI_AgentInfo::HOSTILE))
		{
			if ((m_CharacterClass == CLASS_BADGUY)||(m_CharacterClass == CLASS_UNDEAD))
			{
				if ((pTarget->GetCurRelation() >= CAI_AgentInfo::ENEMY)&&(pTarget->GetCurAwareness() >= CAI_AgentInfo::SPOTTED))
				{
					return(false);
				}
			}

			CVec3Dfp32 ToTargetN = pTarget->GetSuspectedPosition() - GetBasePos();
			ToTargetN.Normalize();
			// Plus/minus 45 degrees
			if (ToDestN * ToTargetN <= 0.71f)
			{
				return(false);
			}
		}
		// ***

		if (ToDestN * GetBaseDir() > 0.71f)
		{
			if (m_PathFinder.GroundTraversable(GetBasePos(),_Dest))
			{
				ResetStuckInfo();
				CWAG2I_Context Context2(m_pGameObject,m_pServer,pCD->m_GameTime);
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WALKSTOPTYPE,AG2_WALKSTOP_NORMAL);
				pCD->m_AnimGraph2.SendWalkStopImpulse(&Context2);
				return(true);
			}
		}
		
	}

	return(false);
};

bool CAI_Core::CheckWalkStart(CVec3Dfp32 _Dest, fp32 _Range)
{
	CVec3Dfp32 BasePos = GetBasePos();

	if ((!m_bWalkStarts)||(m_KB.GetCollider())||(VALID_POS(m_CollAvoidancePos)))
	{
		return(false);
	}

	// AG2_STATEFLAG_NOLOOK dudes are probably already walkstarting/walkstopping 
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
	if (pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOLOOK)
	{
		return(false);
	}

	if (m_CharacterClass == CLASS_DARKLING)
	{	// *** Reinstate when Darklings move well again ***
		return(false);
	}

	// We must really be stationary to walkstart
	if ((m_DeviceMove.GetLastMove().LengthSqr() >= Sqr(0.1f))||(GetPrevSpeed() >= 0.1f))
	{
		return(false);
	}

	// *** Until we have walkstops in all directions we must make sure that we only walkstop when the eenemy is to the front
	CAI_AgentInfo* pTarget = m_AH.AcquireTarget();
	if ((pTarget)&&(pTarget->GetCurRelation() >= CAI_AgentInfo::HOSTILE))
	{
		if ((m_CharacterClass == CLASS_BADGUY)||(m_CharacterClass == CLASS_UNDEAD))
		{
			if ((pTarget->GetCurRelation() >= CAI_AgentInfo::ENEMY)&&(pTarget->GetCurAwareness() >= CAI_AgentInfo::SPOTTED))
			{
				return(false);
			}
		}

		CVec3Dfp32 ToTarget = pTarget->GetSuspectedPosition() - GetBasePos();
		CVec3Dfp32 ToDest = _Dest - GetBasePos();
		ToTarget.Normalize();
		ToDest.Normalize();
		// Plus/minus 45 degrees
		if (ToDest * ToTarget <= 0.71f)
		{
			return(false);
		}
	}
	// ***

	// Propel the path forward a bit
	CVec3Dfp32 Dest = _Dest;
	fp32 Range = _Range;
	Dest = m_Path.PropelPath(_Range,false);
	if (INVALID_POS(Dest))
	{
		Dest = _Dest;
	}
	int32 iDir = m_PathFinder.GetDir6(Dest);
	CVec3Dfp32 Stop;
	// Ugly hack to compensate the fact that BACK is a bit longer than usual
	if ((iDir == CAI_Pathfinder::BACK)&&(Range < 64.0f))
	{
		Range = 64.0f;
	}
	// Now we do this ass backwarrs thing where we
	// 1 Start the walkstart animation
	// 2 Retrieve the animation distance
	// 3 Calculate our wanted destination from that
	// 4 Set perfect origin there to have the animation gradually smoothing it to there
	// if (m_PathFinder.GroundTraversableDirection(iDir, &Range,&Stop))
	CVec3Dfp32 Dir = (Dest-BasePos).Normalize();
	if (m_PathFinder.GroundTraversable(BasePos,BasePos+Dir*(Range+8.0f),&Stop))
	{	// We can walk there: Walkstarts are go
		int32 iImpulse = -1;
		switch(iDir)
		{
		case CAI_Pathfinder::FRONT:
			{
				iImpulse = AG2_WALKSTARTDIRECTION_FWD;
			}
			break;

		case CAI_Pathfinder::RIGHT:
			{
				iImpulse = AG2_WALKSTARTDIRECTION_RIGHT90;
			}
			break;
		case CAI_Pathfinder::BACK:
			{	// Left or right?
				if ((_Dest - m_CurBaseMat.GetRow(3)) * m_CurBaseMat.GetRow(1) >= 0.0f)
				{	// Left
					iImpulse = AG2_WALKSTARTDIRECTION_LEFT180;
				}
				else
				{	// Right
					iImpulse = AG2_WALKSTARTDIRECTION_RIGHT180;
				}
			}
			break;
		case CAI_Pathfinder::LEFT:
			{
				iImpulse = AG2_WALKSTARTDIRECTION_LEFT90;
			}
			break;
		case CAI_Pathfinder::RIGHT_45:
			{
				iImpulse = AG2_WALKSTARTDIRECTION_RIGHT45;
			}
			break;
		case CAI_Pathfinder::LEFT_45:
			{
				iImpulse = AG2_WALKSTARTDIRECTION_LEFT45;
			}
			break;
		default:
			iImpulse = -1;	// Bad value, no walkstart is made
			break;
		}

		if (iImpulse >= 0)
		{
			ResetStuckInfo();
			CWAG2I_Context Context2(m_pGameObject,m_pServer,pCD->m_GameTime);
			pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WALKSTARTTYPE,iImpulse);
			pCD->m_AnimGraph2.SendWalkStartImpulse(&Context2);

			m_Path.PropelPath(Range,true);
			m_DeviceMove.Use();
			/*
			CVec3Dfp32 Offset = CVec3Dfp32(Range,0.0f,0.0f);
			CQuatfp32 Rot;
			CMat4Dfp32 BaseMat,DestMat;
			GetBaseMat(BaseMat);
			DestMat.Unit();
			pCD->m_AnimGraph2.GetAG2I()->CWAG2I::GetTopLayerTotalAnimOffset(&Context2, Offset, Rot, 0);
			fp32 OffLen = Offset.Length();
			Dest = m_Path.PropelPath(OffLen,true);
			if (VALID_POS(Dest))
			{
				Offset.MultiplyMatrix3x3(BaseMat);
				DestMat.GetRow(0) = DestMat.GetRow(3)-BaseMat.GetRow(3);
				DestMat.RecreateMatrix(2,0);
				DestMat.GetRow(3) = Dest;
				// TODO: What to do with Rot?
				SetPerfectOrigin(&DestMat);
				// Then propel some more
				m_Path.PropelPath(128.0f,true);
				m_DeviceMove.Use();
			}
			*/
			return(true);
		}
	}
	return(false);
};

void CAI_Core::SetDestination(CVec3Dfp32 _Dest, fp32 _Velocity)
{
	m_bMDestDirty = true;
	if (VALID_POS(_Dest))
	{
		m_MDestState = DEST_NORMAL;
		m_MDest.GetRow(3) = _Dest;
	}
	else
	{
		m_MDestState = DEST_NONE;
		INVALIDATE_POS(m_MDest.GetRow(3));
	}
	m_MDestSpeed = _Velocity;
	if (m_CharacterClass == CLASS_DARKLING)
	{
		m_MDest.GetRow(3) += GetUp() * 16.0f;
	}
};

void CAI_Core::SetPerfectPlacement(const CMat4Dfp32* _pMDest,fp32 _Velocity)
{
	m_bMDestDirty = true;
	if (_pMDest)
	{
		m_MDestState = DEST_PP;
		m_MDest = *_pMDest;
		m_MDestSpeed = _Velocity;
		if (m_CharacterClass == CLASS_DARKLING)
		{
			m_MDest.GetRow(3) += m_MDest.GetRow(2) * 16.0f;
		}
	}
	else
	{
		m_MDestState = DEST_NONE;
		INVALIDATE_POS(m_MDest.GetRow(3));
	}
};

void CAI_Core::SetPerfectPlacement(CWO_ScenePoint* _pScenePoint, bool _bReversedLook)
{
	m_bMDestDirty = true;
	if (_pScenePoint)
	{
		m_MDestState = DEST_PP;
		m_MDest = _pScenePoint->GetPositionMatrix();
		m_MDestSpeed = 0.0f;
		if (m_CharacterClass == CLASS_DARKLING)
		{
			m_MDest.GetRow(3) += m_MDest.GetRow(2) * 16.0f;
		}
		if (_bReversedLook)
		{
			CVec3Dfp32 temp = m_MDest.GetRow(0);
			(-temp).SetRow(m_MDest,0);
			temp = m_MDest.GetRow(1);
			(-temp).SetRow(m_MDest,1);
			// No need to reconstruct matrix
		}
	}
	else
	{
		NoStops();
	}
};

void CAI_Core::SetPerfectOrigin(const CMat4Dfp32* _pMDest)
{
	m_bMDestDirty = true;
	if (_pMDest)
	{
		m_MDestState = DEST_PO;
		m_MDest = *_pMDest;
		if (m_CharacterClass == CLASS_DARKLING)
		{
			m_MDest.GetRow(3) += m_MDest.GetRow(2) * 16.0f;
		}
	}
	else
	{
		m_MDestState = DEST_NONE;
		INVALIDATE_POS(m_MDest.GetRow(3));
	}
};

void CAI_Core::SetPerfectOrigin(CWO_ScenePoint* _pScenePoint)
{
	m_bMDestDirty = true;
	if (_pScenePoint)
	{
		m_MDestState = DEST_PO;
		m_MDest = _pScenePoint->GetPositionMatrix();
		if (m_CharacterClass == CLASS_DARKLING)
		{
			m_MDest.GetRow(3) += m_MDest.GetRow(2) * 16.0f;
		}
	}
	else
	{
		m_MDestState = DEST_NONE;
		INVALIDATE_POS(m_MDest.GetRow(3));
	}
};

void CAI_Core::ClearPerfectOrigin()
{
	m_bMDestDirty = true;
	m_MDestState = DEST_NONE;
	INVALIDATE_POS(m_MDest.GetRow(3));
};

void CAI_Core::NoStops()
{
	if (m_MDestState != DEST_NONE)
	{
		m_bMDestDirty = true;
		m_MDestState = DEST_NONE;
		INVALIDATE_POS(m_MDest.GetRow(3));
	}
};

void CAI_Core::NoPerfectPlacement()
{
	if ((m_MDestState == DEST_PP)||(m_MDestState == DEST_PO))
	{
		m_bMDestDirty = true;
		m_MDestState = DEST_NONE;
		INVALIDATE_POS(m_MDest.GetRow(3));
	}
};

void CAI_Core::StopNow()
{	// We cannot use GetBaseMat as it is shifted up 8 units in Darklings
	m_bMDestDirty = true;
	m_MDestState = DEST_STOP;
	m_MDest = m_pGameObject->GetPositionMatrix();
};

void CAI_Core::OnRefreshDestination()
{
	// Tell AG where our destination is
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
	if ((pCD)&&(m_bMDestDirty))
	{
		bool bPerfectPlacement = false;
		bool bWalkstops = false;
		bool bWalkstarts = false;
		if (GetStealthTension() < TENSION_HOSTILE)
		{
			bWalkstarts = true;
		}
		else
		{
			bWalkstarts = false;
		}
		switch(m_MDestState)
		{
		case DEST_NORMAL:
			if (VALID_POS(m_MDest.GetRow(3)))
			{
				CVec3Dfp32 Stop;
				/*
				if (m_PathFinder.GroundTraversable(GetBasePos(),m_MDest.GetRow(3),&Stop))
				{
					bPerfectPlacement = false;
					bWalkstops = true;
				}
				else
				*/
				{
					bPerfectPlacement = false;
					bWalkstarts = false;
					m_MDestState = DEST_NONE;
					INVALIDATE_POS(m_MDest.GetRow(3));
				}
			}
			else
			{
				bPerfectPlacement = false;
				m_MDestState = DEST_NONE;
				INVALIDATE_POS(m_MDest.GetRow(3));
			}
			break;

		case DEST_PP:
			if (VALID_POS(m_MDest.GetRow(3)))
			{
				bPerfectPlacement = true;
			}
			else
			{
				bPerfectPlacement = false;
				m_MDestState = DEST_NONE;
				INVALIDATE_POS(m_MDest.GetRow(3));
			}
			break;

		case DEST_PO:
			if (VALID_POS(m_MDest.GetRow(3)))
			{
				bPerfectPlacement = false;
			}
			else
			{
				bPerfectPlacement = false;
				m_MDestState = DEST_NONE;
				INVALIDATE_POS(m_MDest.GetRow(3));
			}
			break;

		case DEST_STOP:
			bPerfectPlacement = false;
			bWalkstops = true;
			m_MDest.GetRow(3) = m_pGameObject->GetPosition();
			if (m_CharacterClass == CLASS_DARKLING)
			{
				m_MDest.GetRow(3) += m_MDest.GetRow(2) * 16.0f;
			}
			break;

		default:	// DEST_NONE
			bPerfectPlacement = false;
			INVALIDATE_POS(m_MDest.GetRow(3));
			m_MDestSpeed = 0.0f;
			break;
		}
		
		if (bPerfectPlacement)
		{
			if (!(pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_DISABLESETDEST))
			{
				pCD->m_AnimGraph2.SetDestination(m_MDest,m_MDestSpeed);
				pCD->m_AnimGraph2.SetUpVector(m_MDest.GetRow(2));
			}
			pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_PERFECTPLACEMENT,true);
		}
		else
		{
			pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_PERFECTPLACEMENT,false);
			if (!(pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_DISABLESETDEST))
			{
				pCD->m_AnimGraph2.SetDestination(m_MDest,m_MDestSpeed);
				pCD->m_AnimGraph2.SetUpVector(m_MDest.GetRow(2));
			}
		}

	}
	m_bMDestDirty = false;

#ifndef M_RTM
	if (DebugTarget())
	{
		CMat4Dfp32 Mat = m_MDest;
		Mat.GetRow(3) += CVec3Dfp32(0,0,10);
		switch(m_MDestState)
		{
		case DEST_NORMAL:
			Debug_RenderVertex(Mat.GetRow(3),kColorGreen,0.0f);
			break;

		case DEST_PP:
			m_pServer->Debug_RenderMatrix(Mat,CPixel32(kColorGreen),0.0f,true);
			break;

		case DEST_STOP:
			Debug_RenderVertex(Mat.GetRow(3),kColorRed,0.0f);
			break;

		default:
			// Debug render nothing man
			break;
		}
	}
#endif
};

void CAI_Core::RefreshInteresting()
{
	m_AH.m_iTarget = RefreshInteresting(m_AH.m_iTarget,INTERESTING_ENEMY);
	m_AH.m_iTarget2 = RefreshInteresting(m_AH.m_iTarget2,INTERESTING_ENEMY);
	m_AH.m_iHostile = RefreshInteresting(m_AH.m_iHostile,INTERESTING_HOSTILE);
	m_AH.m_iHostile2 = RefreshInteresting(m_AH.m_iHostile2,INTERESTING_HOSTILE);
	m_AH.m_iFriend = RefreshInteresting(m_AH.m_iFriend,INTERESTING_FRIEND);
	m_AH.m_iFriend2 = RefreshInteresting(m_AH.m_iFriend2,INTERESTING_FRIEND);
};

int32 CAI_Core::RefreshInteresting(int32 _iObject,int32 _Type)
{
	if (_iObject)
	{
		CAI_AgentInfo* pInfo = m_KB.GetAgentInfo(_iObject);
		CAI_Core* pAI = GetAI(_iObject);
		if ((!pInfo)||(!pAI))
		{
			return(0);
		}
		if ((pInfo->GetCurAwareness() < CAI_AgentInfo::DETECTED)||(!pAI->IsConscious()))
		{	// Should we really downgrade 
			return(0);
		}

		switch(_Type)
		{
		case INTERESTING_FRIEND:
			if (pInfo->GetCurRelation() > CAI_AgentInfo::FRIENDLY)
			{
				return(0);
			}
			break;
		case INTERESTING_HOSTILE:
			if ((pInfo->GetCurRelation() < CAI_AgentInfo::HOSTILE)||(pInfo->GetCurRelation() >= CAI_AgentInfo::ENEMY))
			{
				return(0);
			}
			break;
		case INTERESTING_ENEMY:
			if (pInfo->GetCurRelation() < CAI_AgentInfo::ENEMY)
			{
				return(0);
			}
			break;
		default:
			return(0);
		}
	}

	return(_iObject);
};

// *** TODO: Move the methods to ActionHandler ***
int32 CAI_Core::GetInterestingObjects(int32& _Type, CVec3Dfp32& _Pos, CVec3Dfp32& _Pos2)
{
	if ((!m_pGameObject)||(CWObject_Character::Char_GetPhysType(m_pGameObject) == PLAYER_PHYS_NOCLIP))
	{
		return(0);
	}

	CAI_AgentInfo* pInfo;
	if ((m_AH.m_iTarget)&&(pInfo = m_KB.GetAgentInfo(m_AH.m_iTarget)))
	{
		_Type = INTERESTING_ENEMY;
		_Pos = pInfo->GetSuspectedHeadPosition();
		if ((m_AH.m_iTarget2)&&(pInfo = m_KB.GetAgentInfo(m_AH.m_iTarget2)))
		{
			_Pos2 = pInfo->GetSuspectedHeadPosition();
			return(2);
		}
		else
		{
			return(1);
		}
	}

	if ((m_AH.m_iHostile)&&(pInfo = m_KB.GetAgentInfo(m_AH.m_iHostile)))
	{
		_Type = INTERESTING_HOSTILE;
		_Pos = pInfo->GetSuspectedHeadPosition();
		if ((m_AH.m_iHostile2)&&(pInfo = m_KB.GetAgentInfo(m_AH.m_iHostile2)))
		{
			_Pos2 = pInfo->GetSuspectedHeadPosition();	
			return(2);
		}
		else
		{
			return(1);
		}
	}

	if ((m_AH.m_iFriend)&&(pInfo = m_KB.GetAgentInfo(m_AH.m_iFriend)))
	{
		_Type = INTERESTING_FRIEND;
		_Pos = pInfo->GetSuspectedHeadPosition();
		if ((m_AH.m_iFriend2)&&(pInfo = m_KB.GetAgentInfo(m_AH.m_iFriend2)))
		{
			_Pos2 = pInfo->GetSuspectedHeadPosition();
			return(2);
		}
		else
		{
			return(1);
		}
	}

	// Looki looki
	if (IsPlayer())
	{
		int Flags = CAI_ActionHandler::SP_PREFER_LIGHT | CAI_ActionHandler::SP_RECENT_FORBIDDEN;
		CVec3Dfp32 OurPos = GetBasePos();
		CMat4Dfp32 OurMat;
		GetBaseMat(OurMat);
		
		if (m_Timer > m_AH.m_InterestingScenepointTimeout)
		{
			m_AH.m_pInterestingScenepoint = m_AH.GetBestScenePoint(CWO_ScenePoint::LOOK | CWO_ScenePoint::DYNAMIC | CWO_ScenePoint::ROAM,Flags,32,m_SightRange,OurMat,OurPos,0.0f,0);
			if (m_AH.m_pInterestingScenepoint)
			{
				m_AH.ActivateScenePoint(m_AH.m_pInterestingScenepoint,CAI_Action::PRIO_IDLE);
				int StayTicks = (int)(m_AH.m_pInterestingScenepoint->GetBehaviourDuration() * GetAITicksPerSecond());
				m_AH.m_InterestingScenepointTimeout = m_Timer + StayTicks;
			}
			else
			{
				m_AH.m_InterestingScenepointTimeout = 0;
			}
		}
		if (m_AH.m_pInterestingScenepoint)
		{
			_Type = INTERESTING_LOOK;
			_Pos = m_AH.m_pInterestingScenepoint->GetPosition();
			return(1);
		}
	}

	return(0);
};

bool CAI_Core::SwitchTarget(int32 _iTarget,bool _bReplace)
{
	if (m_AH.m_iTarget == _iTarget)
	{
		return(false);
	}

	if (!m_AH.m_iTarget)
	{
		m_AH.m_iTarget = _iTarget;
		return(true);
	}
	else
	{
		if (_bReplace)
		{
			m_AH.m_iTarget2 = m_AH.m_iTarget;
			m_AH.m_iTarget = _iTarget;
			return(true);
		}

		if (!RefreshInteresting(_iTarget,INTERESTING_ENEMY))
		{
			return(false);
		}
		if (!RefreshInteresting(m_AH.m_iTarget,INTERESTING_ENEMY))
		{
			m_AH.m_iTarget = _iTarget;
			return(true);
		}

		// Switch target if _iTarget is Darkling
		CAI_Core* pOrgAI = GetAI(m_AH.m_iTarget);
		CAI_AgentInfo* pOrgInfo = m_KB.GetAgentInfo(m_AH.m_iTarget);
		CAI_Core* pNewAI = GetAI(_iTarget);
		CAI_AgentInfo* pNewInfo = m_KB.GetAgentInfo(_iTarget);
		if ((!pNewAI)||(!pNewInfo))
		{
			return(false);
		}
		if ((!pOrgAI)||(!pOrgInfo))
		{
			m_AH.m_iTarget = _iTarget;
			m_AH.m_iTarget2 = 0;
			return(false);
		}

		// Expensive, checks consciousness, bravery and enemy override
		if (!IsValidTarget(_iTarget))
		{
			return(false);
		}

		// Measure ranges
		fp32 OrgRangeSqr = SqrDistanceToUs(m_AH.m_iTarget);
		fp32 NewRangeSqr = SqrDistanceToUs(_iTarget);

		// Prefer player if close, even if the other target has better awareness
		if ((NewRangeSqr <= Sqr(96.0f))&&(IsPlayer(_iTarget)))
		{	// New target is player and close
			m_AH.m_iTarget2 = m_AH.m_iTarget;
			m_AH.m_iTarget = _iTarget;
			return(true);
		}
		if ((OrgRangeSqr <= Sqr(96.0f))&&(IsPlayer(m_AH.m_iTarget)))
		{	// Old target is player and close
			return(false);
		}
		if ((NewRangeSqr <= Sqr(96.0f))&&(IsPlayer(_iTarget)))
		{	// New target is player and close
			m_AH.m_iTarget = _iTarget;
			m_AH.m_iTarget2 = m_AH.m_iTarget;
			return(true);
		}

		// Both the old and the new AI are valid, choose wisely
		if (pOrgAI->m_CharacterClass != pNewAI->m_CharacterClass)
		{
			if (pNewAI->m_CharacterClass == CLASS_DARKLING)
			{	// Prefer darklings
				m_AH.m_iTarget2 = m_AH.m_iTarget;
				m_AH.m_iTarget = _iTarget;
				return(true);
			}
			else if ((pNewAI->m_CharacterClass == CLASS_DEFAULT)&&(pOrgAI->m_CharacterClass != CLASS_DARKLING))
			{	// Prefer player
				m_AH.m_iTarget2 = m_AH.m_iTarget;
				m_AH.m_iTarget = _iTarget;
				return(true);
			}
		}
		else	// Classes are the same
		{
			if (pNewInfo->GetCurAwareness() > pOrgInfo->GetCurAwareness())
			{	// Prefer better awareness
				m_AH.m_iTarget2 = m_AH.m_iTarget;
				m_AH.m_iTarget = _iTarget;
				return(true);
			}

			// Same awareness, class etc: Switch if _iTarget is substantially closer to us
			
			if (NewRangeSqr < OrgRangeSqr * 0.5f)
			{
				m_AH.m_iTarget2 = m_AH.m_iTarget;
				m_AH.m_iTarget = _iTarget;
				return(true);
			}
		}
	}

	return(false);
};

bool CAI_Core::SwitchHostile(int32 _iHostile, bool _bReplace)
{
	if (m_AH.m_iHostile == _iHostile)
	{
		return(false);
	}

	if (!m_AH.m_iHostile)
	{
		m_AH.m_iHostile = _iHostile;
		return(true);
	}
	else
	{
		if (_bReplace)
		{
			m_AH.m_iHostile2 = m_AH.m_iHostile;
			m_AH.m_iHostile = _iHostile;
			return(true);
		}

		if (!RefreshInteresting(_iHostile,INTERESTING_HOSTILE))
		{
			return(false);
		}
		if (!RefreshInteresting(m_AH.m_iHostile,INTERESTING_HOSTILE))
		{
			m_AH.m_iHostile = _iHostile;
			return(true);
		}

		// Switch target if _iTarget is Darkling
		CAI_Core* pOrgAI = GetAI(m_AH.m_iHostile);
		CAI_AgentInfo* pOrgInfo = m_KB.GetAgentInfo(m_AH.m_iHostile);
		CAI_Core* pNewAI = GetAI(_iHostile);
		CAI_AgentInfo* pNewInfo = m_KB.GetAgentInfo(_iHostile);
		if ((!pNewAI)||(!pNewInfo))
		{
			return(false);
		}
		if ((!pOrgAI)||(!pOrgInfo))
		{
			m_AH.m_iHostile = _iHostile;
			m_AH.m_iHostile2 = 0;
			return(false);
		}
		// Both the old and the new AI are valid, choose wisely
		if (pOrgAI->m_CharacterClass != pNewAI->m_CharacterClass)
		{
			if (pNewAI->m_CharacterClass == CLASS_DARKLING)
			{	// Prefer darklings
				m_AH.m_iHostile2 = m_AH.m_iHostile;
				m_AH.m_iHostile = _iHostile;
				return(true);
			}
			else if ((pNewAI->m_CharacterClass == CLASS_DEFAULT)&&(pOrgAI->m_CharacterClass != CLASS_DARKLING))
			{	// Prefer player
				m_AH.m_iHostile2 = m_AH.m_iHostile;
				m_AH.m_iHostile = _iHostile;
				return(true);
			}
		}
		else	// Classes are the same
		{
			if (pNewInfo->GetCurAwareness() > pOrgInfo->GetCurAwareness())
			{	// Prefer better awareness
				m_AH.m_iHostile2 = m_AH.m_iHostile;
				m_AH.m_iHostile = _iHostile;
				return(true);
			}

			// Same awareness, class etc: Switch if _iTarget is substantially closer to us
			fp32 OrgRangeSqr = SqrDistanceToUs(m_AH.m_iHostile);
			fp32 NewRangeSqr = SqrDistanceToUs(_iHostile);
			if (NewRangeSqr < OrgRangeSqr * 0.5f)
			{
				m_AH.m_iHostile2 = m_AH.m_iHostile;
				m_AH.m_iHostile = _iHostile;
				return(true);
			}
		}
	}

	return(false);
};

bool CAI_Core::SwitchInvestigate(int32 _iInvestigate, bool _bReplace)
{
	if (m_AH.m_iInvestigate == _iInvestigate)
	{
		return(false);
	}

	if (!m_AH.m_iInvestigate)
	{
		m_AH.m_iInvestigate = _iInvestigate;
		return(true);
	}
	else
	{
		if (_bReplace)
		{
			m_AH.m_iInvestigate2 = m_AH.m_iInvestigate;
			m_AH.m_iInvestigate = _iInvestigate;
			return(true);
		}

		// Switch target if _iTarget is Darkling
		CAI_Core* pOrgAI = GetAI(m_AH.m_iInvestigate);
		CAI_AgentInfo* pOrgInfo = m_KB.GetAgentInfo(m_AH.m_iInvestigate);
		CAI_Core* pNewAI = GetAI(_iInvestigate);
		CAI_AgentInfo* pNewInfo = m_KB.GetAgentInfo(_iInvestigate);
		if ((!pNewAI)||(!pNewInfo))
		{
			return(false);
		}
		if ((!pOrgAI)||(!pOrgInfo))
		{
			m_AH.m_iInvestigate = _iInvestigate;
			m_AH.m_iInvestigate2 = 0;
			return(false);
		}

		// Both the old and the new AI are valid, choose wisely
		if (pOrgAI->m_CharacterClass != pNewAI->m_CharacterClass)
		{
			if (pNewAI->m_CharacterClass == CLASS_DARKLING)
			{	// Prefer darklings
				m_AH.m_iInvestigate2 = m_AH.m_iInvestigate;
				m_AH.m_iInvestigate = _iInvestigate;
				return(true);
			}
			else if ((pNewAI->m_CharacterClass == CLASS_DEFAULT)&&(pOrgAI->m_CharacterClass != CLASS_DARKLING))
			{	// Prefer player
				m_AH.m_iInvestigate2 = m_AH.m_iInvestigate;
				m_AH.m_iInvestigate = _iInvestigate;
				return(true);
			}
		}
		else	// Classes are the same
		{
			if (pNewInfo->GetCurAwareness() > pOrgInfo->GetCurAwareness())
			{	// Prefer better awareness
				m_AH.m_iInvestigate2 = m_AH.m_iInvestigate;
				m_AH.m_iInvestigate = _iInvestigate;
				return(true);
			}

			// Same awareness, class etc: Switch if _iTarget is substantially closer to us
			fp32 OrgRangeSqr = SqrDistanceToUs(m_AH.m_iInvestigate);
			fp32 NewRangeSqr = SqrDistanceToUs(_iInvestigate);
			if (NewRangeSqr < OrgRangeSqr * 0.5f)
			{
				m_AH.m_iInvestigate2 = m_AH.m_iInvestigate;
				m_AH.m_iInvestigate = _iInvestigate;
				return(true);
			}
		}
	}

	return(false);
};

bool CAI_Core::SwitchFriendly(int32 _iFriend,bool _bReplace)
{
	if (m_AH.m_iFriend == _iFriend)
	{
		return(false);
	}

	if (!m_AH.m_iFriend)
	{
		m_AH.m_iFriend = _iFriend;
		return(true);
	}
	else
	{
		if (_bReplace)
		{
			m_AH.m_iFriend2 = m_AH.m_iFriend;
			m_AH.m_iFriend = _iFriend;
			return(true);
		}

		if (!RefreshInteresting(_iFriend,INTERESTING_FRIEND))
		{
			return(false);
		}
		if (!RefreshInteresting(m_AH.m_iFriend,INTERESTING_FRIEND))
		{
			m_AH.m_iFriend = _iFriend;
			return(true);
		}

		// Switch target if _iTarget is Darkling
		CAI_Core* pOrgAI = GetAI(m_AH.m_iFriend);
		CAI_AgentInfo* pOrgInfo = m_KB.GetAgentInfo(m_AH.m_iFriend);
		CAI_Core* pNewAI = GetAI(_iFriend);
		CAI_AgentInfo* pNewInfo = m_KB.GetAgentInfo(_iFriend);

		// Both the old and the new AI are valid, choose wisely
		if (pOrgAI->m_CharacterClass != pNewAI->m_CharacterClass)
		{
			if (pNewAI->m_CharacterClass == CLASS_DARKLING)
			{	// Prefer darklings
				m_AH.m_iFriend2 = m_AH.m_iFriend;
				m_AH.m_iFriend = _iFriend;
				return(true);
			}
			else if ((pNewAI->m_CharacterClass == CLASS_DEFAULT)&&(pOrgAI->m_CharacterClass != CLASS_DARKLING))
			{	// Prefer player
				m_AH.m_iFriend2 = m_AH.m_iFriend;
				m_AH.m_iFriend = _iFriend;
				return(true);
			}
		}
		else	// Classes are the same
		{
			if (pNewInfo->GetCurAwareness() > pOrgInfo->GetCurAwareness())
			{	// Prefer better awareness
				m_AH.m_iFriend2 = m_AH.m_iFriend;
				m_AH.m_iFriend = _iFriend;
				return(true);
			}

			// Same awareness, class etc: Switch if _iTarget is substantially closer to us
			fp32 OrgRangeSqr = SqrDistanceToUs(m_AH.m_iFriend);
			fp32 NewRangeSqr = SqrDistanceToUs(_iFriend);
			if (NewRangeSqr < OrgRangeSqr * 0.5f)
			{
				m_AH.m_iFriend2 = m_AH.m_iFriend;
				m_AH.m_iFriend = _iFriend;
				return(true);
			}
		}
	}

	return(false);
};

// *** Temporary solution until the action and objective systems are fully implemented ***
// Returns true if we can attack the target ie attack angles and possibly ranges are OK
// Behaviour drive bots will always return true
bool CAI_Core::CanAttack(CAI_AgentInfo* _pObj)
{
	// Action driven bots must check their m_AH if any of the combat actions are valid
	// for the target and returns true if one is.
	return(m_AH.CanAttack(_pObj));
}

//Causes the bot to attack the given target.
void CAI_Core::OnAttack(CAI_AgentInfo * _pObj)
{
	//Attack if item device is available (we might fail to aim properly if look device is unavailable, though)
	if (m_DeviceWeapon.IsAvailable())
	{	//Attack!
		OnTrackAgent(_pObj,5,false,false);
		CAI_Weapon* pWeapon = m_Weapon.GetWielded();
		if (pWeapon)
		{
			m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_AIMGUN,20);
			pWeapon->OnUse(_pObj);
			if (pWeapon->GetType() == CAI_Weapon::RANGED_RAPIDFIRE)
			{
				m_DeviceWeapon.Use();
			}
			else
			{
				m_DeviceWeapon.UsePeriodic();
			}
		}
	};
};


//Causes the bot to aim at the given target
void CAI_Core::OnAim(CAI_AgentInfo * _pObj)
{
	//Aim if look device is available
	if (m_DeviceLook.IsAvailable())
	{
		//Aim
		m_Weapon.GetWielded()->OnAim(_pObj);
	};
};

// Handles turret tracking etc
bool CAI_Core::HandleMount(uint32 _Flags,int32 _iPrimary,CVec3Dfp32* _pTargetPos)
{
	if (m_iMountParam)
	{	// Track and possibly even fire
		// Handle being mounted
		CWObject* pObj = m_pServer->Object_Get(m_iMountParam); 
		if (pObj)
		{	// Get the turret matrix
			CMat4Dfp32 Mat;
			CWObject_Message Msg(OBJSYSMSG_GETCAMERA);
			Msg.m_pData = &Mat;
			Msg.m_DataSize = sizeof(Mat);
			m_pServer->Message_SendToObject(Msg, m_iMountParam);
			CVec3Dfp32 MountPos = Mat.GetRow(3);
			// OK, now we should somehow track enemies and hostiles using this information
#ifndef M_RTM
			if (DebugTarget())
			{
				m_pServer->Debug_RenderMatrix(Mat,1.0f,true);
			}
#endif
			m_DeviceMove.Use();
			bool bEnemy = false;
			int iTarget = 0;

			INVALIDATE_POS(*_pTargetPos);
			if (m_Script.m_iTargetParam)
			{
				iTarget = m_Script.m_iTargetParam;
				bEnemy = true;
			}
			else if (_iPrimary)
			{
				iTarget = _iPrimary;
				bEnemy = true;
			}
			else if (m_AH.m_iTarget)
			{
				iTarget = m_AH.m_iTarget;
				bEnemy = true;
			}
			else if (m_AH.m_iTarget2)
			{
				iTarget = m_AH.m_iTarget2;
				bEnemy = true;
			}
			else if (m_AH.m_iHostile)
			{
				iTarget = m_AH.m_iHostile;
			}
			else if (m_AH.m_iHostile2)
			{
				iTarget = m_AH.m_iHostile2;
			}
			else if (m_AH.m_iInvestigateObj)
			{
				iTarget = m_AH.m_iInvestigateObj;
			}
			else if (m_AH.m_iInvestigate)
			{
				iTarget = m_AH.m_iInvestigate;
			}
			else if (m_AH.m_iInvestigate2)
			{
				iTarget = m_AH.m_iInvestigate2;
			}

			if (iTarget)
			{
				CAI_AgentInfo* pTarget = m_KB.GetAgentInfo(iTarget);
				if (pTarget)
				{	// Maybe we should put the stopfire delay into an action-param?
					*_pTargetPos = pTarget->GetSuspectedTorsoPosition();
					if ((bEnemy)&&(pTarget->GetCurAwareness() < CAI_AgentInfo::SPOTTED)&&(pTarget->GetGetLastSpottedTicks() > 60))
					{
						bEnemy = false;
					}
				}
				else
				{
					*_pTargetPos = GetBasePos(iTarget);
				}

				if (VALID_POS((*_pTargetPos)))
				{	// Track target
					
					// We update the m_TargetDir slowly, but track it fast
					CVec3Dfp32 LookPos = Mat.GetRow(3);
					CVec3Dfp32 TargetDir = *_pTargetPos - LookPos;
					TargetDir.Normalize();
					CVec3Dfp32 LookDir = TargetDir;
					LookDir.Normalize();
					int32 Ticks = 20;
					if (_Flags & MOUNTFLAGS_WORLDTRACKING)
					{	// Blend between the two
						fp32 Weight = 0.5f;
						LookDir = Mat.GetRow(0) * (1.0f - Weight) + TargetDir * Weight;
						Ticks = 1;
					}
					OnTrackDir(LookDir,iTarget,Ticks,false,false);
#ifndef M_RTM
					if (DebugTarget())
					{
						m_pServer->Debug_RenderWire(LookPos,LookPos+LookDir*100.0f);
					}
#endif

					// Fire at target
					if (bEnemy)
					{
						if (_Flags & MOUNTFLAGS_BURST)
						{
							m_DeviceWeapon.UsePeriodic();
						}
						else
						{
							m_DeviceWeapon.Use();
						}
					}
				}
			}
		}
		return(true);
	}
	else
	{
		return(false);
	}
};

//Get random traversable position given position matrix (which defines the search origin and direction)
//and constraints on distance, density of search positions and max deviation
CVec3Dfp32 CAI_Core::FindRandomPosition(const CMat4Dfp32& _Mat, fp32 _Distance, fp32 _Deviation, fp32 _Density, fp32 _MaxDev)
{
	//Not implemented... fix?
	return CVec3Dfp32(_FP32_MAX);
};

//Convenient overload; uses characters current position matrix
CVec3Dfp32 CAI_Core::FindRandomPosition(fp32 _Distance, fp32 _Deviation, fp32 _Density, fp32 _MaxDev)
{
	if (!m_pGameObject)
		return CVec3Dfp32(_FP32_MAX);

	return FindRandomPosition(m_pGameObject->GetPositionMatrix(), _Distance, _Deviation, _Density, _MaxDev);
};



//Attempts to find a ground-traversable position (i.e. one that can be reached by travelling in a straight line 
//along the ground) in the given heading at the given distance. If this fails, it will try to find the closest 
//position at that distance or closer, trying new heading with the given angle interval (90 degrees as default). 
//If the _angle argument is specified it will only try to find positions within the arc of that angle, centered 
//around the heading and if the _MinDist argument is given it will only try to find closer positions down to 
//this minimum distance. Returns CVec3Dfp32(_FP32_MAX) if no position is found. If the nGroundCells argument is specified 
//the position must also have at least that many cround cells.
CVec3Dfp32 CAI_Core::FindGroundPosition(fp32 _Heading, fp32 _Distance, fp32 _MinDist, fp32 _Interval, fp32 _Angle, int nGroundCells)
{
	//StartBM("FindGroundPosition");

	if (m_PathFinder.GridPF())
	{
		//Ugly, but it works...
		fp32 dHeading = 0;
		fp32 Heading;
		CVec3Dfp32 Pos;
		CVec3Dfp32 Partial;
		//The direction we primarily search is randomly determined
		int8 iDir = (Random > 0.5f) ? 1 : -1;
		bool bIncr = true;
		while (dHeading <= _Angle)
		{
			Heading = _Heading + (iDir * dHeading);
			Pos = m_PathFinder.GetPathPosition(GetBasePos() + (CVec3Dfp32(M_Cos(Heading*_PI2) , M_Sin(-Heading*_PI2), 0) * _Distance), 5, 0);

			if (m_PathFinder.GridPF()->GetBlockNav()->IsGroundTraversable(m_PathFinder.GetPathPosition(m_pGameObject), Pos, (int)GetBaseSize(), (int)GetCrouchHeight(), false, (int)GetStepHeight(), (int)GetJumpHeight(), &Partial))
			{
				//Found position!
				//Debug_RenderWire(Pos, Pos + CVec3Dfp32(0,0,100), 0xffffffff, 10);//DEBUG
				//StopBM();
				return Pos;
			}
			else
			{
				//Was the ground traversable position closest to the wanted position far away enough?
				if ( !_MinDist &&
					((GetBasePos()).DistanceSqr(Partial) >= Sqr(_MinDist)) )
				{
					//Found closer position
					//Debug_RenderWire(Partial, Partial + CVec3Dfp32(0,0,100), 0xffffffff, 10);//DEBUG
					//StopBM();
					return Partial;
				}
			};

			//If angle-delta is 0.5f, then there is no point in checking the other direction, so we're done
			if (dHeading == 0.5f)
				break;

			//Did not find any position in this pass, set up next pass
			//Toggle direction
			iDir = -iDir;
			//Should we increase angle-delta? 
			if (bIncr)
				dHeading += _Interval;
			bIncr = !bIncr;
		};

		//No position can be found
		//StopBM();
		return CVec3Dfp32(_FP32_MAX);
	}
	else
	{
		//We're effectively blind without a navgrid, so just use position _Distance units away in specified _Heading
		//StopBM();
		return m_PathFinder.GetPathPosition(GetBasePos() + (CVec3Dfp32(M_Cos(_Heading*_PI2), M_Sin(-_Heading*_PI2), 0) * _Distance), 10, 0) ;
	}
};

//Bot makes a normal move to follow the target, no evasion etc
void CAI_Core::OnBasicFollowTarget(CAI_AgentInfo* _pTarget,fp32 _Speed)
{
	//Valid target?
	if (!_pTarget || !_pTarget->GetObject())
	{
		OnIdle();
		return;
	}

	CVec3Dfp32 TargetPos = _pTarget->GetObject()->GetPosition();
	CVec3Dfp32 TargetBelievedPos = _pTarget->GetBasePos();
	CVec3Dfp32 OurPos = GetBasePos();

	//Don't move if at low activity
	if (m_ActivityLevel <= ACTIVITY_LOW)
	{
		m_DeviceMove.Use();
		return;
	}

	if (SqrDistanceToUs(TargetBelievedPos) >= Sqr(48))
	{	//Always track the target
		OnTrackAgent(_pTarget,6);
	}

	if ((!m_DeviceMove.IsAvailable())||(m_bBehaviourRunning)||(m_pGameObject->AI_IsJumping()))
	{
		return;
	}

	{
		//Always go to current path destination except if there isn't one, if we're almost there 
		//or every once in a while when the target has moved much relative to the distance between us
		if ((INVALID_POS(m_PathDestination)) ||
			(OurPos.DistanceSqr(m_PathDestination) < 64*64) ||
			((m_Timer % 50 == 0)&&(_pTarget->GetObject()->GetPosition()).DistanceSqr(m_PathDestination) > Max(Sqr(64.0f), Sqr(0.2f) * m_pGameObject->GetPosition().DistanceSqr(TargetBelievedPos))))
		{
			//Stop if we're close to target
			if (m_pGameObject->GetPosition().DistanceSqr(TargetBelievedPos) < 48*48)
			{
				OnIdle();
			}
			else
			{
				//Move to current target position
				TargetBelievedPos = m_PathFinder.SafeGetPathPosition(TargetBelievedPos, 2, 1);
				SetDestination(TargetBelievedPos);
				int32 moveResult = OnMove(TargetBelievedPos,_Speed,true,false,NULL); //Go to last position if target is airborne
				if (!CheckMoveresult(moveResult,NULL))
				{	// Move action failed
					ConOutL("OnBasicFollowTarget failed");
					m_DeviceMove.Use();
				};
			}
		}
		else
		{
			//If we've reached path destination but been pushed away or is otherwise not close to target
			//we should reset path search and go to targets current position
			if (m_bReachedPathDestination &&
				((m_pGameObject->GetPosition()).DistanceSqr(TargetBelievedPos) > 48*48))
			{
				INVALIDATE_POS(m_PathDestination);					
				ResetPathSearch();
				TargetBelievedPos = m_PathFinder.SafeGetPathPosition(TargetBelievedPos, 2, 1);
				SetDestination(TargetBelievedPos);
				int32 moveResult = OnMove(TargetBelievedPos,_Speed,false,false,NULL); //Don't move if target is airborne
				if (!CheckMoveresult(moveResult,NULL))
				{	// Move action failed
					ConOutL("OnBasicFollowTarget failed");
					m_DeviceMove.Use();
				};
			} 
			//If previous pathfinding failed follow partial path for a while
			else if (m_SearchStatus == SEARCH_FOUND_PARTIAL_PATH)
			{
				//Follow partial path (if any)
				if (m_bReachedPathDestination)
				{
					//Try to find path again
					INVALIDATE_POS(m_PathDestination);					
					ResetPathSearch();
					TargetBelievedPos = m_PathFinder.SafeGetPathPosition(TargetBelievedPos, 2, 1);
					SetDestination(TargetBelievedPos);
					int32 moveResult = OnMove(TargetBelievedPos,_Speed,true,false,NULL); //Don't move if target is airborne
					if (!CheckMoveresult(moveResult,NULL))
					{	// Move action failed
						ConOutL("OnBasicFollowTarget failed");
						m_DeviceMove.Use();
					};
				}
				else
				{
					OnFollowPath(GetMaxSpeedForward());
				}
			}
			else
			{
				//No special circumstances; just move towards current destination. 
				int32 moveResult = OnMove(m_PathDestination,_Speed,true,false,NULL);
				if (!CheckMoveresult(moveResult,NULL))
				{	// Move action failed
					ConOutL("OnBasicFollowTarget failed");
					m_DeviceMove.Use();
				};
			}
		}

		//Were we unable to go to this position?
		if ((m_SearchStatus == CAI_Pathfinder::NO_PATH)||
			(m_SearchStatus == CAI_Pathfinder::INVALID))
		{
			// Pathfinder failed to move to the target
			ResetPathSearch();
			m_PathDestination = TargetBelievedPos;
			SetActivationState(STATE_SUGGEST_DEACTIVATION);
		}
	}
};

//Bot makes a normal move to follow the target, no evasion etc
void CAI_Core::OnBasicInvestigatePos(CVec3Dfp32& _Pos,fp32 _Speed,fp32 _GoalRange)
{
	//Always track the phenomenon (48 units (1.5m) above ground)
	CVec3Dfp32 Diff = _Pos - m_pGameObject->GetPosition();
	fp32 hDiffSqr = Sqr(Diff[0]) + Sqr(Diff[1]);
	if ((hDiffSqr < Sqr(160.0f))&&((Diff[2] >= 0.0f)||(hDiffSqr > Sqr(2 * Diff[2]))))
	{
		OnTrackPos(_Pos+CVec3Dfp32(0.0f,0.0f,48.0f),10,false,false);
	}

	//Don't move if at low activity
	if (m_ActivityLevel <= ACTIVITY_LOW)
	{
		m_DeviceMove.Use();
		return;
	}

	if ((!m_DeviceMove.IsAvailable())||(m_bBehaviourRunning)||(m_pGameObject->AI_IsJumping()))
	{
		return;
	}

	{
		//Always go to current path destination except if there isn't one, if we're almost there 
		//or every once in a while when the target has moved much relative to the distance between us
		if ((INVALID_POS(m_PathDestination))||
			(m_pGameObject->GetPosition().DistanceSqr(m_PathDestination) < 64*64)||
			((m_Timer % 50 == 0) && (_Pos).DistanceSqr(m_PathDestination) > Max(Sqr(64.0f), Sqr(0.2f) * m_pGameObject->GetPosition().DistanceSqr(_Pos))))
		{
			//Stop if we're close to target
			if (m_pGameObject->GetPosition().DistanceSqr(_Pos) < Sqr(_GoalRange))
			{
				m_DeviceMove.Use();
			}
			else
			{
				//Move to current target position
				CVec3Dfp32 PathPos = m_PathFinder.SafeGetPathPosition(_Pos, 2, 1);
				SetDestination(PathPos);
				OnMove(PathPos,_Speed,true,false,NULL); //Go to last position if target is airborne
			}
		}
		else
		{
			//If we've reached path destination but been pushed away or is otherwise not close to target
			//we should reset path search and go to targets current position
			if (m_bReachedPathDestination &&
				((m_pGameObject->GetPosition()).DistanceSqr(_Pos) > Sqr(_GoalRange)))
			{
				INVALIDATE_POS(m_PathDestination);					
				ResetPathSearch();
				CVec3Dfp32 PathPos = m_PathFinder.SafeGetPathPosition(_Pos, 2, 1);
				SetDestination(PathPos);
				OnMove(PathPos,_Speed,true,false,NULL); //Don't move if target is airborne
				//OnSpeak("New move");
			} 
			//If previous pathfinding failed follow partial path for a while
			else if (m_SearchStatus == SEARCH_FOUND_PARTIAL_PATH)
			{
				//Follow partial path (if any)
				if (m_bReachedPathDestination)
				{	//Try to find path again
					INVALIDATE_POS(m_PathDestination);					
					ResetPathSearch();
					CVec3Dfp32 PathPos = m_PathFinder.SafeGetPathPosition(_Pos, 2, 1);
					SetDestination(PathPos);
					OnMove(PathPos,_Speed,true,false,NULL); //Don't move if target is airborne
					//OnSpeak("New move");
				}
				else
				{	// Let's mosey over and check it out
					OnFollowPath(_Speed);
				}
			}
			else
			{
				//No special circumstances; just move towards current destination.
				SetDestination(_Pos);
				OnMove(_Pos,_Speed,true,false,NULL);
			}
		}

		//Were we unable to go to this position?
		if ((m_SearchStatus == CAI_Pathfinder::NO_PATH) ||
			(m_SearchStatus == CAI_Pathfinder::INVALID) )
		{
			// Pathfinder failed to move to the target
			ResetPathSearch();
			m_PathDestination = _Pos;
			SetActivationState(STATE_SUGGEST_DEACTIVATION);
		}
	}
};

// All MakeNoise et al calls goes through these method first
// So all CAI_Core subclasses can override sound behaviours
// The methods mirror those in CAI_Device_Sound with the param _Reason added:
// _Reason tell us why the particular sound should be emitted
// Returns the variant used if a sound was actually played and -1 if none was played
int CAI_Core::UseRandom(CStr _Reason, int _iType, int _Prio, int _iVariant)
{
	if ((!IsPlayer())&&(!m_Script.IsValid())&&(m_pGameObject)&&(m_pGameObject->AI_IsAlive()))
	{
		int iVariant = m_DeviceSound.UseRandom(_iType,_Prio,_iVariant);
#ifndef M_RTM
		if ((iVariant > -1)&&(DebugTarget()))
		{
			DebugPrintSoundReason(_Reason,_iType,iVariant);
		}
#endif
		return(iVariant);
	}
	else
	{
		return(-1);
	}
}

int CAI_Core::UseRandom(CStr _Reason, int _iType, int _Prio)
{
	if ((!IsPlayer())&&(!m_Script.IsValid())&&(m_pGameObject)&&(m_pGameObject->AI_IsAlive()))
	{
		// Nono, not yet!
		if ((_iType == CAI_Device_Sound::COMBAT_SPOTTED)||(_iType == CAI_Device_Sound::COMBAT_DETECTED))
		{	// Are we alone?
			fp32 Range = 0.0f;
			if (!GetClosestTeammember(false,0,&Range))
			{	// Nobody knows but Jesus!
				if (_iType == CAI_Device_Sound::COMBAT_SPOTTED)
				{	// Only use the alone voices when not seeing Jackie
					return(-1);
				}
				_Reason += "(alone)";
				_iType = CAI_Device_Sound::COMBAT_ALONE;
			}
		}
		int iVariant = m_DeviceSound.UseRandom(_iType,_Prio);
#ifndef M_RTM
		if ((iVariant > -1)&&(DebugTarget()))
		{
			DebugPrintSoundReason(_Reason,_iType);
		}
#endif
		return(iVariant);
	}
	else
	{
		return(-1);
	}
}


void CAI_Core::UseRandomDelayed(CStr _Reason, int _iType, int _Prio, int _DelayTicks)
{
	if ((!IsPlayer())&&(!m_Script.IsValid())&&(m_pGameObject)&&(m_pGameObject->AI_IsAlive()))
	{
		// Nono, not yet!
		if ((_iType == CAI_Device_Sound::COMBAT_SPOTTED)||(_iType == CAI_Device_Sound::COMBAT_DETECTED))
		{	// Are we alone?
			fp32 Range = 0.0f;
			if (!GetClosestTeammember(false,0,&Range))
			{	// Nobody knows but Jesus!
				_Reason += "(alone)";
				_iType = CAI_Device_Sound::COMBAT_ALONE;
			}
		}
		m_DeviceSound.UseRandomDelayed(_iType,_Prio,_DelayTicks);
#ifndef M_RTM
		if (DebugTarget())
		{
			DebugPrintSoundReason(_Reason,_iType,-1);
		}
#endif
	}
}


#ifndef M_RTM
void CAI_Core::DebugPrintSoundReason(CStr _Reason, int _iType,int _iVariant)
{
	if (!DebugRender())
		return;

	CStr msg = m_pGameObject->GetName();
	msg += " says: ";
	if ((_iType > CAI_Device_Sound::INVALID_SOUND)&&(_iType < CAI_Device_Sound::MAX_SOUNDS))
	{
		msg += CAI_Device_Sound::ms_lTranslateSpeech[_iType];
	}
	else
	{
		msg += "unkown";
	}
	if (_iVariant > -1)
	{
		msg += CStrF(" %d",_iVariant);
	}
	else
	{
		msg += " delayed";
	}
	msg += " Reason: " + _Reason;

	ConOut(msg);
}
#endif


//Raise an alarm about the given object. Optionally you can specify alarm information level and 
//the relation recipients of the alarm should assume versus the cause
void CAI_Core::RaiseAlarm(int _iCause, int _Relation, bool _RangeBased)
{
	if (GetStealthTension(false) >= TENSION_COMBAT)
	{
		return;
	}

	if ((m_PendingAlarmTime == -1)||
		((m_PendingAlarmMsg.m_Msg != OBJMSG_TEAM_ALARM)&&
		(m_PendingAlarmMsg.m_Msg != OBJMSG_TEAM_FOUND_CORPSE)))
	{
		if ((!m_StunTimeout)&&(!m_Script.IsPaused())&&(_Relation >= CAI_AgentInfo::ENEMY))
		{
			m_EventHandler.RaiseEvent(CAI_EventHandler::ON_ALARM,_iCause);
			if (IsPlayer(_iCause))
			{
				UseRandom("Raise alarm",CAI_Device_Sound::ENEMY_SPOT_PLAYER,CAI_Action::PRIO_COMBAT);
			}
			else
			{
				UseRandom("Raise alarm",CAI_Device_Sound::COMBAT_SPOTTED,CAI_Action::PRIO_COMBAT);
			}
		}

		m_PendingAlarmMsg.m_Msg = OBJMSG_TEAM_ALARM;
		if(_RangeBased)
			m_PendingAlarmMsg.m_Msg = OBJMSG_TEAM_RANGE_ALARM;

		m_PendingAlarmMsg.m_Param0 = _iCause;
		m_PendingAlarmMsg.m_Reason = _Relation;
		m_PendingAlarmMsg.m_iSender = m_pGameObject->m_iObject;

		// I don't particularly like hardcoded numbers, what about these?
		m_PendingAlarmTime = (int)(GetAITick() + (0.5f + Random) * GetAITicksPerSecond());
	}
};

void CAI_Core::RaiseFoundCorpse(int _iPerp, int _iVictim, CVec3Dfp32 _Pos)
{
	if (GetStealthTension(false) >= TENSION_COMBAT)
	{
		return;
	}

	if ((m_PendingAlarmTime == -1)||
		((m_PendingAlarmMsg.m_Msg != OBJMSG_TEAM_ALARM)&&
		(m_PendingAlarmMsg.m_Msg != OBJMSG_TEAM_FOUND_CORPSE)))
	{
		m_PendingAlarmMsg.m_Msg = OBJMSG_TEAM_FOUND_CORPSE;
		m_PendingAlarmMsg.m_Param0 = _iPerp;
		m_PendingAlarmMsg.m_Param1 = _iVictim;
		m_PendingAlarmMsg.m_iSender = m_pGameObject->m_iObject;
		m_PendingAlarmMsg.m_VecParam0 = _Pos;

		// I don't particularly like hardcoded numbers, what about these?
		m_PendingAlarmTime = (int)(GetAITick() + (0.5f+Random) * GetAITicksPerSecond());
	}
};

//Raise an alarm about the given object. Optionally you can specify alarm information level and 
//the relation recipients of the alarm should assume versus the cause
void CAI_Core::RaiseFight(int _iCause, int _Relation)
{
	CWObject_Message Msg;

	Msg.m_Msg = OBJMSG_TEAM_FIGHT;
	Msg.m_Param0 = _iCause;
	Msg.m_Param1 = 0;
	Msg.m_iSender = m_pGameObject->m_iObject;
	Msg.m_Reason = _Relation;
	SendMessage(CAI_Core::MSGMODE_ALARM,Msg);
};

void CAI_Core::RaiseSearchOff(int _iSuspect)
{
	CWObject_Message Msg;

	if (!m_KB.CanSpeakTeammate())
	{
		return;
	}

	if ((m_PendingAlarmTime == -1)&&
		(m_PendingAlarmMsg.m_Msg != OBJMSG_TEAM_ALARM)&&
		(m_PendingAlarmMsg.m_Msg != OBJMSG_TEAM_FOUND_CORPSE)&&
		(m_PendingAlarmMsg.m_Msg != OBJMSG_TEAM_SEARCH_OFF))
	{
		m_PendingAlarmMsg.m_Msg = OBJMSG_TEAM_SEARCH_OFF;
		m_PendingAlarmMsg.m_Param0 = _iSuspect;
		m_PendingAlarmMsg.m_iSender = m_pGameObject->m_iObject;
		m_PendingAlarmTime = (int)(GetAITick() + (0.5f+Random) * GetAITicksPerSecond());
	}
};

//Raise awareness about the given object.
void CAI_Core::RaiseAwareness(int _iCause, CVec3Dfp32 _Pos,int _Awareness,int _Relation)
{
	if ((m_PendingAlarmTime == -1)&&
		(m_PendingAlarmMsg.m_Msg != OBJMSG_TEAM_ALARM)&&
		(m_PendingAlarmMsg.m_Msg != OBJMSG_TEAM_FOUND_CORPSE)&&
		(m_PendingAlarmMsg.m_Msg != OBJMSG_TEAM_SEARCH_OFF))
	{
		switch(_Awareness)
		{
		case CAI_AgentInfo::NOTICED:
			break;

		case CAI_AgentInfo::DETECTED:
			break;

		case CAI_AgentInfo::SPOTTED:
			break;

		default:
			ConOut("CAI_Core::RaiseAwareness, unknown _Awareness");
			return;
			break;
		}
		m_PendingAlarmMsg.m_Msg = OBJMSG_TEAM_AWARE;
		m_PendingAlarmMsg.m_Param0 = _iCause;
		m_PendingAlarmMsg.m_Param1 = _Awareness;
		m_PendingAlarmMsg.m_Reason = _Relation;
		m_PendingAlarmMsg.m_iSender = m_pGameObject->m_iObject;
		m_PendingAlarmMsg.m_VecParam0 = _Pos;

		m_PendingAlarmTime = (int)(GetAITick() + (0.5f+Random) * GetAITicksPerSecond());
	}
	else if ((m_PendingAlarmMsg.m_Msg == OBJMSG_TEAM_AWARE)&&
		(m_PendingAlarmTime > GetAITick())&&
		(_Awareness > m_PendingAlarmMsg.m_Param1)&&
		(m_PendingAlarmMsg.m_Param0 == _iCause))
	{	// There is already a pending OBJMSG_TEAM_AWARE for the same object
		// We just improved the awareness before sending
		m_PendingAlarmMsg.m_Param1 = _Awareness;
		m_PendingAlarmMsg.m_VecParam0 = _Pos;
	}
};

int CAI_Core::GetBestCommunication(int _iObject)
{
	int rComm = -1;

	uint16 liTeams[16];
	uint nTeams = m_pGameObject->AI_GetTeams(liTeams, sizeof_buffer(liTeams));
	for (int i = 0; i < nTeams; i++)
	{
		int iTeam = liTeams[i];
		CWObject_Message IsTeamMember(OBJMSG_TEAM_BELONGTOTEAM,iTeam);
		if (m_pServer->Message_SendToObject(IsTeamMember,_iObject))
		{	// _iObject is a teammember
			int CurComm = m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_TEAM_COMMUNICATIONS),iTeam);
			if (CurComm > rComm)
			{
				rComm = CurComm;
			}
		}
	}

	return(rComm);
};

//Sends a message to an object, all friends, all allies, all enemies or all characters.
void CAI_Core::SendMessage(int _iMode, const CWObject_Message& _Msg, int _iObject)
{
	switch (_iMode)
	{
	case MSGMODE_ALARM:
		{
			//Send alarm through teams
			uint16 liTeams[16];
			uint nTeams = m_pGameObject->AI_GetTeams(liTeams, sizeof_buffer(liTeams));
			for (int i = 0; i < nTeams; i++)
			{
				int iTeam = liTeams[i];
				m_pServer->Message_SendToObject(_Msg, iTeam);
			}
		};
		break;

	case MSGMODE_ALL:
		{
			//Send message to all characters in character list
			int iChar;
			CSimpleIntList* plAgents = &(m_pAIResources->m_KBGlobals.ms_lAgents);
			for (int i = 0; i < plAgents->Length(); i++)
				if ( plAgents->IsValid(i) &&
					(iChar = plAgents->Get(i)) &&
					(iChar != m_pGameObject->m_iObject) )
					m_pServer->Message_SendToObject(_Msg, iChar);
		};
		break;

	};
};

#ifndef M_RTM
CVec3Dfp32 CAI_Core::GetLookAtPos(int _iPlayer)
{
	CVec3Dfp32 rPos;
	INVALIDATE_POS(rPos);
	
	CCollisionInfo Info;
	Info.Clear();
	CAI_Core* pLookerAI = GetAI(_iPlayer);
	if (pLookerAI)
	{
		CVec3Dfp32 LookDir = pLookerAI->GetLookDir();
		CVec3Dfp32 EyePos = pLookerAI->GetLookPos();
		if (IntersectLOS(EyePos,EyePos + LookDir * 512.0f,_iPlayer,&Info))
		{
			if (Info.m_bIsValid)
			{
				rPos = Info.m_Pos;
			}
		}
	}

	return(rPos);
};

// Returns the bot object ID that is nearest FOV centerline and closest according to the following heuristic: TBD
int32 CAI_Core::FindBotInFOV(CVec3Dfp32* _pLook, CVec3Dfp32* _pPos)
{
	if (!IsPlayer())
	{
		return(0);
	}

	int32 iBestBot = 0;
	int32 iChar;
	fp32 bestFitness = 1000000.0f;
	fp32 curFitness;
	CSimpleIntList* plAgents = &(m_pAIResources->m_KBGlobals.ms_lAgents);
	CVec3Dfp32 LookPos, LookDir;
	if ((_pLook)&&(_pPos))
	{
		LookDir = *_pLook;
		LookPos = *_pPos;
	}
	else
	{
		LookDir = GetLookDir();
		LookPos = GetHeadPos();
	}
	
	for (int i = 0; i < plAgents->Length(); i++)
	{
		if ((plAgents->IsValid(i))&&(iChar = plAgents->Get(i))&&(iChar != m_pGameObject->m_iObject))
		{
			CAI_Core* pAI = GetAI(iChar);
			if ((pAI)&&(pAI->m_pGameObject->AI_IsAlive()))
			{
				CVec3Dfp32 Diff = pAI->GetHeadPos() - LookPos;
				fp32 Range = Diff.Length();
				Diff.Normalize();
				fp32 Dot = LookDir * Diff;
				if (Dot <= 0.95f) { continue; }
				curFitness = (1.0f - (LookDir * Diff)) * Range;
				if (curFitness < bestFitness)
				{
					bestFitness = curFitness;
					iBestBot = iChar;
				}
			}
		}
	}

	return(iBestBot);
};

void CAI_Core::DebugDrawNavgrid(int32 _iObject)
{
	CVec3Dint16 Pos;
	CVec3Dint16 Pos2;
	int Val,ValBelow,ValAbove;

	if (!m_PathFinder.GridPF())
	{
		ConOutL("DebugDrawNavgrid: No navgrid!");
		return;
	}

	if (_iObject)
	{
		Pos = m_PathFinder.GetBlockNav()->GetGridPosition(GetBasePos(_iObject));
	}
	else
	{
		Pos = m_PathFinder.GetBlockNav()->GetGridPosition(GetBasePos());
	}
	
	for (int x = -5; x < 6; x++)
	{
		for (int y = -5; y < 6; y++)
		{
			for (int z = -5; z < 6; z++)
			{
				Pos2 = Pos + CVec3Dint16(x,y,z);
				Val = m_PathFinder.GetBlockNav()->GetCellValue(Pos2[0],Pos2[1],Pos2[2]);
				ValBelow = m_PathFinder.GetBlockNav()->GetCellValue(Pos2[0],Pos2[1],Pos2[2]-1);
				ValAbove = m_PathFinder.GetBlockNav()->GetCellValue(Pos2[0],Pos2[1],Pos2[2]+1);
				DebugRenderCell(Val,ValBelow,ValAbove,m_PathFinder.GetBlockNav()->GetWorldPosition(Pos2), m_pServer,20);	
			}
		}
	}
};

void CAI_Core::DebugDrawCheckDead(CVec3Dfp32& _ViewPos,CVec3Dfp32& _VictimPos,int _Awareness)
{
	if (DebugRender())
	{
		// Wireframe what we saw
		// One line between looker and victim and a cross 32 units above
		int Color = kColorWhite;
		if (_Awareness >= CAI_AgentInfo::DETECTED)
		{	// Red
			Color = kColorRed;
		}
		else if (_Awareness >= CAI_AgentInfo::DETECTED)
		{	// Green
			Color = kColorGreen;
		}
		else
		{	// Blue
			Color = kColorBlue;
		}
		
		// LOS trace
		Debug_RenderWire(_ViewPos,_VictimPos,Color,1.0f);
		// Cross
		Debug_RenderWire(_VictimPos+CVec3Dfp32(0,0,32),_VictimPos+CVec3Dfp32(0,0,64),Color,3.0f);
		Debug_RenderWire(_VictimPos+CVec3Dfp32(0,-16,56),_VictimPos+CVec3Dfp32(0,16,56),Color,3.0f);
	}
};

// Draws a tetraeder that shows the bots knowledge about the player
// BLUE is NONE, GREEN is NOTICED, YELLOW is DETECTED and RED is SPOTTED
// A cross will be drawn where the bots thinks the player is in the same color
// The tetraeder will be upside down if the bot consider the player an enemy
// A vertical line will be drawn if the bot runs a behaviour or is scripted
// A white cross that denotes the light intensity on the bot will be drawn.
// It will be at the bottom of the pyramid when intensity = 0.0 and at the top when 1.0
void CAI_Core::DebugDrawKnowledge()
{
	if ((m_pServer)&&(m_pServer->Debug_GetWireContainer()))
	{
//		CWObject_Game *pGame = m_pServer->Game_GetObject();
		int32 targetID = 0;
		CAI_AgentInfo* pTarget = NULL;
		if (DebugTargetSet())
		{
			int iPlayer = GetClosestPlayer();
			CAI_Core* pPlayerAI = GetAI(iPlayer);
			if ((pPlayerAI)&&(pPlayerAI->m_pGameObject)&&(CWObject_Character::Char_GetPhysType(pPlayerAI->m_pGameObject) == PLAYER_PHYS_NOCLIP))
			{	// Player Noclips:
				// Debugdraw knowledge visavi the debugtarget
				targetID = m_pAIResources->ms_iDebugRenderTarget;
				pTarget = m_KB.GetAgentInfo(targetID);
			}
		}
		if (!pTarget)
		{
			targetID = GetClosestPlayer();
			pTarget = m_KB.GetAgentInfo(targetID);
		}

		m_KB.DefaultRelation(targetID);
		int32 color = 0xffffffff;	// White
		if (pTarget)
		{
			int32 CurAwareness = pTarget->GetCurAwareness();

			// Draw a tetraeder above the head of the target
			#define AI_SHAPEDIM	16
			CVec3Dfp32 targetPos = pTarget->GetBasePos();
			CVec3Dfp32 headPos = GetLookPos();
			CVec3Dfp32 base0,base1,base2,top;
			base0 = headPos;
			base1 = headPos;
			base2 = headPos;
			top = headPos;

			base0[1] += AI_SHAPEDIM;
			base1[0] += AI_SHAPEDIM;
			base1[1] -= AI_SHAPEDIM;
			base2[0] -= AI_SHAPEDIM;
			base2[1] -= AI_SHAPEDIM;

			int CurRelation = pTarget->GetCurRelation();
			switch(CurRelation)
			{
			case CAI_AgentInfo::ENEMY:
				base0[2] += AI_SHAPEDIM;
				base1[2] += AI_SHAPEDIM;
				base2[2] += AI_SHAPEDIM;
				switch(CurAwareness)
				{
				case CAI_AgentInfo::NOTICED:
					color = kColorGreen;	// Green
					break;
				case CAI_AgentInfo::DETECTED:
					color = kColorYellow;	// Yellow
					break;
				case CAI_AgentInfo::SPOTTED:
					color = kColorRed;	// Red
					break;
				}
				break;

			case CAI_AgentInfo::HOSTILE_2:
				top[2] += AI_SHAPEDIM - 12;
				switch(CurAwareness)
				{
				case CAI_AgentInfo::NOTICED:
					color = kColorGreen;	// Green
					break;
				case CAI_AgentInfo::DETECTED:
					color = kColorYellow;	// Yellow
					break;
				case CAI_AgentInfo::SPOTTED:
					color = kColorRed;	// Red
					break;
				}
				break;

			case CAI_AgentInfo::HOSTILE_1:
				top[2] += AI_SHAPEDIM - 8;
				switch(CurAwareness)
				{
				case CAI_AgentInfo::NOTICED:
					color = kColorGreen;	// Green
					break;
				case CAI_AgentInfo::DETECTED:
					color = kColorYellow;	// Yellow
					break;
				case CAI_AgentInfo::SPOTTED:
					color = kColorRed;	// Red
					break;
				}
				break;

			case CAI_AgentInfo::HOSTILE:
				top[2] += AI_SHAPEDIM - 4;
				switch(CurAwareness)
				{
				case CAI_AgentInfo::NOTICED:
					color = kColorGreen;	// Green
					break;
				case CAI_AgentInfo::DETECTED:
					color = kColorYellow;	// Yellow
					break;
				case CAI_AgentInfo::SPOTTED:
					color = kColorRed;	// Red
					break;
				}
				break;

			case CAI_AgentInfo::UNFRIENDLY:
				// UNFRIENDLY
				top[2] += AI_SHAPEDIM;
				switch(CurAwareness)
				{
				case CAI_AgentInfo::NOTICED:
					color = kColorGreen;	// Green
					break;
				case CAI_AgentInfo::DETECTED:
					color = kColorYellow;	// Yellow
					break;
				case CAI_AgentInfo::SPOTTED:
					color = kColorRed;	// Red
					break;
				}
				break;

			case CAI_AgentInfo::NEUTRAL:
				// Brighter
				top[2] += AI_SHAPEDIM + 8;
				switch(CurAwareness)
				{
				case CAI_AgentInfo::NOTICED:
					color = kColorGreen;	// Green
					break;
				case CAI_AgentInfo::DETECTED:
					color = kColorYellow;	// Yellow
					break;
				case CAI_AgentInfo::SPOTTED:
					color = kColorRed;	// Red
					break;
				}
				break;

			case CAI_AgentInfo::FRIENDLY:
				// Brighter still!
				top[2] += AI_SHAPEDIM;
				switch(CurAwareness)
				{
				case CAI_AgentInfo::NOTICED:
					color = kColorGreen;	// Green
					break;
				case CAI_AgentInfo::DETECTED:
					color = kColorYellow;	// Yellow
					break;
				case CAI_AgentInfo::SPOTTED:
					color = kColorRed;	// Red
					break;
				}
				break;

			default:
				break;
			}

			CWireContainer* pWC = m_pServer->Debug_GetWireContainer();
			if ((pWC)&&(color != 0))
			{
				pWC->RenderWire(base0,base1,color, 0.0f, false);
				pWC->RenderWire(base1,base2,color, 0.0f, false);
				pWC->RenderWire(base2,base0,color, 0.0f, false);
				pWC->RenderWire(top,base0,color, 0.0f, false);
				pWC->RenderWire(top,base1,color, 0.0f, false);
				pWC->RenderWire(top,base2,color, 0.0f, false);
				if (DebugTarget())
				{
					CVec3Dfp32 midBase = top;
					midBase[2] = (top[2]+base0[2]) * 0.5f;
					pWC->RenderWire(base0,midBase,color, 0.0f, false);
					pWC->RenderWire(base1,midBase,color, 0.0f, false);
					pWC->RenderWire(base2,midBase,color, 0.0f, false);
				}
				if ((pTarget)&&(!IsPlayer(pTarget->GetObjectID())))
				{
					pWC->RenderWire(headPos,targetPos,color, 0.0f, false);
				}

				// Render a vertical line if scripted or running a behaviour
				if (m_Script.IsValid())
				{
					pWC->RenderWire(headPos,headPos + CVec3Dfp32(0,0,AI_SHAPEDIM),color, 0.0f, false);
				}
				else if (m_bBehaviourRunning)
				{
					pWC->RenderWire(headPos,headPos + CVec3Dfp32(0,0,AI_SHAPEDIM / 2),color, 0.0f, false);
				}

				if ((m_bCanWallClimb)&&(m_bWallClimb))
				{
					CVec3Dfp32 Pos = headPos+CVec3Dfp32(0,0,AI_SHAPEDIM);
					CVec3Dfp32 Up = GetUp();
					if (m_pGameObject->AI_IsOnWall())
					{
						pWC->RenderWire(Pos,Pos+Up*AI_SHAPEDIM,kColorRed, 0.0f, false);
					}
					else
					{
						pWC->RenderWire(Pos,Pos+Up*AI_SHAPEDIM,kColorGreen, 0.0f, false);
					}
				}
			}

			if (color != 0)
			{
				// Also draw the suspected pos of the player
				m_pServer->Debug_RenderVertex(targetPos+CVec3Dfp32(0,0,10),color,0.0f,false);
			}
		}
	}
};

void CAI_Core::DebugDrawLight()
{
	CWireContainer* pWC = m_pServer->Debug_GetWireContainer();
	CVec3Dfp32 headPos = GetLookPos();
	int32 color = kColorBlue;	// Blue

	if (pWC)
	{	// Draw the light intensity
		// Min/max bars
		pWC->RenderWire(headPos,headPos+CVec3Dfp32(AI_SHAPEDIM,0,0),kColorRed, 0.0f, false);
		pWC->RenderWire(headPos+CVec3Dfp32(0,0,AI_SHAPEDIM),headPos+CVec3Dfp32(AI_SHAPEDIM,0,AI_SHAPEDIM),kColorWhite, 0.0f, false);
		fp32 LightIntensity = GetLightIntensity(0);
		if (LightIntensity > LIGHTMETER_SNEAK_THRESHOLD)
		{	// White
			color = kColorWhite;		// White that will not blow up in nightvision
		}
		else
		{
			color = kColorBlue;		// Blue that will not blow up in nightvision
		}
		m_pServer->Debug_RenderVertex(headPos+CVec3Dfp32(AI_SHAPEDIM*0.5f,0,AI_SHAPEDIM*LightIntensity),color, 0.0f, false);
		// *** DEBUG Draw lightmap separate
		// LightIntensity = GetLightmapIntensity();
		// m_pServer->Debug_RenderVertex(headPos+CVec3Dfp32(AI_SHAPEDIM*0.5f,0,AI_SHAPEDIM*LightIntensity),kColorRed, 0.0f, false);
		// ***
	}
};
#endif

bool CAI_Core::HandleDialoguePartner()
{
	if (m_iDialoguePartner)
	{
		CVec3Dfp32 OurEyes = GetLookPos();
		CAI_Core* pHisAI = GetAI(m_iDialoguePartner);
		if (pHisAI)
		{
			CVec3Dfp32 HisEyes,Dir;
			OurEyes = GetLookPos();
			HisEyes = pHisAI->GetLookPos();
			Dir = HisEyes-OurEyes;
			Dir.Normalize();
			if ((IsPlayer())||(!m_bLookPartner))
			{
				m_pGameObject->AI_SetEyeLookDir(Dir,(uint16)m_iDialoguePartner);
			}
			else
			{
				OnTrackDir(Dir,m_iDialoguePartner,10,false,false);
			}	
		}
		else
		{
			OnTrackObj(m_iDialoguePartner,10,false,false);
		}
		
#ifndef M_RTM
		CVec3Dfp32 HisGroin;
		CWObject* pObj = m_pServer->Object_Get(m_iDialoguePartner);
		if (pObj)
		{
			HisGroin = pObj->GetPosition() + CVec3Dfp32(0,0,1);
			Debug_RenderWire(OurEyes,HisGroin,kColorYellow,0.55f);
		}
#endif
		return(true);
	}
	else
	{
		{	// Reset vacuous look
			// m_pGameObject->AI_SetEyeLookDir();
		}
		return(false);
	}
};

//Chooses the actions the bot should take at the current time.
void CAI_Core::OnTakeAction()
{
 	MSCOPESHORT(CAI_Core::OnTakeAction);

	bool bScriptValid = m_Script.IsValid();

	// Break out if we're deactivated
	if ((!IsActive())&&(!bScriptValid))
	{	
		return;
	}

#ifndef M_RTM
	DebugDrawKnowledge();
#endif

#ifndef M_RTM
	if (DebugTarget())
	{	// *** Put debug code here! ***
		bool wtf = true;
	}
#endif

	//Check if we're under script control
	if (bScriptValid)
	{
		ActivateAnimgraph();
		//Don't use escape sequences in general
		//m_bNoEscapeSeqMoves = true;

		//Don't use ai max speed
		m_MaxSpeed = GetMaxSpeedForward();

		// Clear any redirects (Darklings cannot be redirected while scripted and shouldn't remember any during scripting)
		INVALIDATE_POS(m_AH.m_RetargetPos);
		INVALIDATE_POS(m_AH.m_RetargetUp);

		//Script on!
		m_Script.OnTakeAction();
		if (m_CollisionRank != 0)
		{
			CheckCollisions();
		}
	}
	else if ((!m_PauseActions))
	{
		//Should all agents pause non-scripted actions and we conform?
		if (!m_bIgnoreCutscenes && !m_pAIResources->AllowActions())
		{
			HandleDialoguePartner();
			StopNow();
			m_DeviceLook.Use();
			m_DeviceMove.Use();
			m_AH.OnRefreshScenePointStatus();
			return;
		}

		// Don't do anything here when oblivious
		if (m_KB.GetAlertness() == CAI_KnowledgeBase::ALERTNESS_OBLIVIOUS)
		{
			if (m_bBehaviourRunning)
			{	// Stops the behaviour fast!
				StopBehaviour(BEHAVIOUR_STOP_FAST,CAI_Action::PRIO_IDLE);
			}
			m_AH.OnRefreshScenePointStatus();
			return;
		}

		// Panicked CIVS should do absolutely nothing but stay put
		if ((m_CharacterClass == CLASS_CIV)&&(m_DeviceStance.IsCrouching()))
		{
			CAI_AgentInfo* pEnemy = m_KB.GetAgentInfo(m_AH.m_iTarget);
			if ((pEnemy)&&(pEnemy->GetCurRelation() >= CAI_AgentInfo::ENEMY)&&(pEnemy->GetCurAwareness() >= CAI_AgentInfo::DETECTED))
			{
				m_DeviceLook.Use();
				m_DeviceMove.Use();
				HandleDialoguePartner();
				StopNow();
				m_AH.OnRefreshScenePointStatus();
				return;
			}
		}

		//Use escape sequences and normal max speed
		m_bNoEscapeSeqMoves = false;
		m_MaxSpeed = m_ReleaseMaxSpeed;

		if (IsActive()&&(m_ActivityLevel != ACTIVITY_NONE)&&(!m_StunTimeout)&&(!m_Script.IsPaused()))
		{
			HandleDialoguePartner();
			if (m_bBehaviourRunning)
			{
				// ***
				if (DebugTarget())
				{
					bool wtf = true;
				}
				// ***

				HandleCombatBehaviours();
				/*
				if ((m_bWallClimb)&&(m_pGameObject->AI_IsOnWall()))
				{
					TempEnableWallClimb(AI_TICKS_PER_SECOND);
				}
				*/
				int32 PrioClass = CAI_Action::PRIO_IDLE;
				int32 PrioClassToBeat = CAI_Action::PRIO_IDLE;
				PrioClassToBeat = Max(PrioClassToBeat,m_BehaviourPrioClass);
				if (m_CurCB.m_iBehaviour)
				{
					PrioClassToBeat = Max(PrioClassToBeat,(int32)CAI_Action::PRIO_COMBAT);
				}
				if ((m_pBehaviourScenePoint)&&(m_AH.RequestScenePoint(m_pBehaviourScenePoint)))
				{
#ifndef M_RTM
					m_AH.DebugDrawScenePoint(m_pBehaviourScenePoint,true);
#endif
					CAI_AgentInfo* pInfo = m_KB.GetAgentInfo(m_AH.m_iTarget);
					if ((pInfo)&&(m_pBehaviourScenePoint->GetBehaviour() == m_iCurBehaviour)&&(!m_pBehaviourScenePoint->GetTacFlags())&&(m_pBehaviourScenePoint->GetType() & (CWO_ScenePoint::TACTICAL | CWO_ScenePoint::COVER)))
					{	// An enemy, no tacflags, behaviour running, TACTICAL or COVER; expire if outside arc
						if (!m_pBehaviourScenePoint->InFrontArc(pInfo->GetSuspectedPosition()))
						{	// Drop SP
							m_BehaviourExpirationTick = 1;
							m_BehaviourPrioClass = 0;
						}
					}
				}
				if (((m_pBehaviourScenePoint != NULL)&&(!m_AH.RequestScenePoint(m_pBehaviourScenePoint)))||
					((m_BehaviourExpirationTick > 0)&&(m_Timer >= m_BehaviourExpirationTick))||
					(m_AH.TestTakeActions(PrioClassToBeat,&PrioClass)))
				{	// Stop any ongoing casual dialogue if new Prio is PRIO_ALERT+
					if (PrioClass > CAI_Action::PRIO_IDLE)
					{
						CWObject_Message Msg(OBJMSG_CHAR_DESTROYCAUSUALDIALOGUE);
						Msg.m_Param0 = (PrioClass & CAI_Action::MASK_PRIO_ONLY);
						m_pServer->Message_SendToObject(Msg,m_pGameObject->m_iObject);
					}
					// Tell animgraph we want to stop
					ActivateAnimgraph();
					if ((PrioClass > m_BehaviourPrioClass)||((m_BehaviourExpirationTick > 0)&&(m_Timer >= m_BehaviourExpirationTick)))
					{
						if (m_Timer >= m_BehaviourExpirationTick)
						{
							if (PrioClass >= CAI_Action::PRIO_COMBAT)
							{	// Stops the behaviour fast!
								StopBehaviour(BEHAVIOUR_STOP_FAST,m_BehaviourPrioClass);
							}
							else
							{	// Stops the behaviour in a more sedate way
								StopBehaviour(BEHAVIOUR_STOP_NORMAL,m_BehaviourPrioClass);
							}
							if (PrioClass > CAI_Action::PRIO_IDLE)
							{
								m_AH.ExpireActions(CAI_Action::ROAM);
							}
						}
						else
						{
							if (PrioClass >= CAI_Action::PRIO_COMBAT)
							{	// Stops the behaviour fast!
								StopBehaviour(BEHAVIOUR_STOP_FAST,PrioClass);
							}
							else
							{	// Stops the behaviour in a more sedate way
								StopBehaviour(BEHAVIOUR_STOP_NORMAL,PrioClass);
							}
							if (PrioClass > CAI_Action::PRIO_IDLE)
							{
								m_AH.ExpireActions(CAI_Action::ROAM);
							}
						}
					}
				}
				// If we're still running a behaviour
				if (m_bBehaviourRunning)
				{
					m_AH.OnServiceBehaviourActions();
				}
			}
			else	// m_bBehaviourRunning is false ie we're NOT running a behaviour
			{
				if ((IsActive())&&(m_ActivityLevel != ACTIVITY_NONE)&&(!m_pGameObject->AI_IsJumping()))
				{
					int32 Prio = CAI_Action::PRIO_IDLE;
					if ((!m_iDialoguePartner)||(m_AH.TestTakeActions(CAI_Action::PRIO_ALERT,&Prio)))
					{	// Do actions and check collisions
						// If the actions actually taken is of lower prio than PRIO_ALERT
						// we may check for continuing dialogue
						// Make a collision check just prior to taking action to hog move/look if needed
						if (m_iDialoguePartner)
						{
							ShutUp(Prio);
						}

						Prio = m_AH.OnTakeActions();
						if (Prio >= CAI_Action::PRIO_ALERT)
						{
							SetActivationState(STATE_ACTIVE);
						}
						else
						{	// Check for continuing dialogue
							CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
							if ((pChar)&&(!HighTension(100))&&(pChar->CheckContinueDialogue()))
							{
								SetActivationState(STATE_ACTIVE);
								m_iCrowdFocus = 0;
							}
							else
							{
								SetActivationState(STATE_SUGGEST_DEACTIVATION);
							}
						}

					}
					else
					{
						HandleDialoguePartner();
						SetActivationState(STATE_ACTIVE);
					}

					// Handle collisons last
					// (It's OK; we free up the move device to override anything previous
					if (m_CollisionRank != 0)
					{
						CheckCollisions();
					}
				}
				else
				{
					OnIdle();
				}
			}
		}
	}
	else
	{	//Not script controlled, and behaviour paused. Do nothing.
		//Except look at the speaker if any
		HandleDialoguePartner();
		if (!m_bBehaviourRunning)
		{
			StopNow();
			m_DeviceLook.Use();
			m_DeviceMove.Use();
		}
	}

	// Keep tracking
	if ((m_ActionFlags & LOOKING)&&(!m_bBehaviourRunning))
	{	// Default params will update the look some
		RefreshLook();
		m_DeviceLook.Use();
 	}

	//Refresh aux stuff
	m_AH.OnRefreshScenePointStatus();
};

bool CAI_Core::ShutUp(int _Prio)
{
	CWObject_Message Msg(OBJMSG_CHAR_DESTROYCAUSUALDIALOGUE);
	Msg.m_Param0 = _Prio;
	CWObject_Character* pChar = NULL;
	CWO_Character_ClientData* pCD = NULL;
	bool bResult = false;
	pCD = CWObject_Character::GetClientData(m_pGameObject);
	if (pCD)
	{
		if (_Prio > pCD->m_DialogueInstance.m_Priority)
		{
			m_pServer->Message_SendToObject(Msg,GetObjectID());
			bResult = true;
		}
	}
	
	
	if (m_iDialoguePartner)
	{
		CWObject* pObj = m_pServer->Object_Get(m_iDialoguePartner);
		pChar = CWObject_Character::IsCharacter(pObj);
		if (pChar)
		{
			pCD = CWObject_Character::GetClientData(pChar);
			if (pCD)
			{
				if (_Prio > pCD->m_DialogueInstance.m_Priority)
				{
					m_pServer->Message_SendToObject(Msg,m_iDialoguePartner);
					m_bLookPartner = false;
					m_iDialoguePartner = 0;
					m_pGameObject->AI_SetEyeLookDir();
					bResult = true;
				}
			}
		}
	}

	if (m_iCrowdFocus)
	{
		CWObject* pObj = m_pServer->Object_Get(m_iCrowdFocus);
		pChar = CWObject_Character::IsCharacter(pObj);
		if (pChar)
		{
			pCD = CWObject_Character::GetClientData(pChar);
			if (pCD)
			{
				if (_Prio > pCD->m_DialogueInstance.m_Priority)
				{
					m_pServer->Message_SendToObject(Msg,m_iCrowdFocus);
					m_bLookPartner = false;
					m_iCrowdFocus = 0;
					bResult = true;
				}
			}
		}
	}

	return(bResult);
};

void CAI_Core::OnBumped(bool _bWeAreBumping,int _iObj,CVec3Dfp32& CollPos)
{
	if (!_bWeAreBumping)
	{
		OnBumpedAG2(_iObj);
	}
};

void CAI_Core::OnBumpedAG2(int _iAttacker, CVec3Dfp32 _Dir)
{
	CVec3Dfp32 ForceDir;
	if (INVALID_POS(_Dir))
	{	// Calculate direction from _iAttacker
		CAI_Core* pAttackerAI = GetAI(_iAttacker);
		if (!pAttackerAI)
		{
			return;
		}

		ForceDir = pAttackerAI->GetBasePos() - GetBasePos();
		ForceDir[2] = 0.0f;
		ForceDir.Normalize();
	}
	else
	{	// Use _Dir
		ForceDir = _Dir;
		ForceDir.Normalize();
	}
	// Calculate what relative direction
	int32 hitDirection;
	CMat4Dfp32 Mat;
	GetBaseMat(Mat);	
	CVec3Dfp32 FwdDir = Mat.GetRow(0);
	FwdDir[2] = 0.0f;
	FwdDir.Normalize();
	CVec3Dfp32 LeftDir = Mat.GetRow(1);
	LeftDir[2] = 0.0f;
	LeftDir.Normalize();
	if (ForceDir * FwdDir >= _SIN45)
	{
		hitDirection = AG2_IMPULSEVALUE_BUMP_FRONT;
	}
	else if (ForceDir * FwdDir <= -_SIN45)
	{
		hitDirection = AG2_IMPULSEVALUE_BUMP_BACK;
	}
	else if (ForceDir * LeftDir >= _SIN45)
	{
		hitDirection = AG2_IMPULSEVALUE_BUMP_LEFT;
	}
	else if (ForceDir * LeftDir <= -_SIN45)
	{
		hitDirection = AG2_IMPULSEVALUE_BUMP_RIGHT;
	}
	else
	{
		hitDirection = AG2_IMPULSEVALUE_BUMP_FRONT;
	}

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
	if (pCD)
	{
		CWAG2I_Context AG2Context(m_pGameObject,m_pServer,pCD->m_GameTime);
		pCD->m_AnimGraph2.SendHurtImpulse(&AG2Context, CXRAG2_Impulse(AG2_IMPULSETYPE_BUMP,hitDirection),0);
	}
};

void CAI_Core::OnTakeDamageAG2(int _iAttacker, int _Damage, uint32 _iDamageType, int _Hitloc)
{
	fp32 PrevHealthFrac = Health() / MaxHealth();
	fp32 CurHealthFract = (Health() - (fp32)_Damage) / MaxHealth();

	// Modify healthfracs by game difficulty (for hurts anims only)
	if (m_pAIResources->ms_GameDifficulty == DIFFICULTYLEVEL_EASY)
	{
		PrevHealthFrac *= 0.5f; 
		CurHealthFract *= 0.5f;
	}
	else if (m_pAIResources->ms_GameDifficulty == DIFFICULTYLEVEL_HARD)
	{
		PrevHealthFrac -= (1.0f - PrevHealthFrac) * 0.5f;
		CurHealthFract -= (1.0f - CurHealthFract) * 0.5f;
	}
	
	// Hurt by the player perhaps?
	if (m_CharacterClass == CLASS_DARKLING)
	{
		int32 iPlayer = GetClosestPlayer();
		if (iPlayer == _iAttacker)
		{	// Show hurt and sadness as response to our masters cruelty
			if (m_KB.GetDetectedEnemyCount() < 1)
			{
				if (Random > 0.5f)
				{
					UseRandom(CStr("Darkling shot by Jackie: Ouch!"),CAI_Device_Sound::IDLE_CALL_RESPONSE,CAI_Action::PRIO_ALERT,3);
				}
				else
				{
					UseRandom(CStr("Darkling shot by Jackie: Quit it!"),CAI_Device_Sound::IDLE_CALL_RESPONSE,CAI_Action::PRIO_ALERT,0);
				}

				if (Random > 0.5f)
				{
					SetWantedBehaviour2(BEHAVIOUR_DARKLING_JACKIESHOOT_SHORT,CAI_Action::PRIO_ALERT,0,-1,NULL);
				}
				else
				{
					SetWantedBehaviour2(BEHAVIOUR_DARKLING_JACKIESHOOT,CAI_Action::PRIO_ALERT,0,-1,NULL);
				}
				// Should we return here?
				return;
			}
		}

		if (_iDamageType & DAMAGETYPE_LIGHT)
		{	// Voice and smoke should be enouch!
			return;
		}
	}

	if ((PrevHealthFrac >= 0.5f)&&(CurHealthFract < 0.5f))
	{	// Stop fighting a bit if we have Cover action
		CAI_Action* pAction;
		if (m_AH.GetAction(CAI_Action::COVER,&pAction))
		{
			if (m_AH.GetAction(CAI_Action::COMBAT,&pAction))
			{
				pAction->SetExpirationExpired();
			}
			if (m_AH.GetAction(CAI_Action::CLOSE,&pAction))
			{
				pAction->SetExpirationExpired();
			}
		}
	}

	if (m_pGameObject)
	{
		CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
		if (pChar)
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pChar);
			if (pCD)
			{
				int hurtType = AG2_IMPULSEVALUE_HURTTYPE_SHORT;
				int hitDirection = AG2_IMPACTDIRECTION_NONE;
				int staggerDirection = AG2_IMPACTDIRECTION_NONE;
				int staggerRange = AG2_STAGGER_RANGE_LONG;
				int hitlocation = AG2_IMPACTLOCATION_MID;
				int iAttackDir = CAI_Pathfinder::FRONT;
				int iDir = iAttackDir;
				if (m_CurCB.m_iBehaviour == 0)
				{	// No combat behaviour: Do fancy hurt calculations
					CWObject* pObj = m_pServer->Object_Get(_iAttacker);
					if (pObj)
					{
						CVec3Dfp32 AttackerPos = pObj->GetPosition();
						CVec3Dfp32 ForceDir;
						INVALIDATE_POS(ForceDir);
						iAttackDir = m_PathFinder.GetDir4(AttackerPos);
						iDir = iAttackDir;

						CMat4Dfp32 Mat;
						GetBaseMat(Mat);	
						CVec3Dfp32 FwdDir = Mat.GetRow(0);
						FwdDir[2] = 0.0f;
						FwdDir.Normalize();
						CVec3Dfp32 LeftDir = Mat.GetRow(1);
						LeftDir[2] = 0.0f;
						LeftDir.Normalize();
						ForceDir = (AttackerPos - GetBasePos()).Normalize();
						int DieRoll = int32(Random * 6.0f);
						if (ForceDir * FwdDir >= _SIN45)
						{
							hitDirection = AG2_IMPACTDIRECTION_FRONT;
							staggerDirection = AG2_STAGGERDIRECTION_BACK;
							if (Random > 0.75f)
							{
								if (Random > 0.5f)
								{
									staggerDirection = AG2_STAGGERDIRECTION_LEFT;
								}
								else
								{
									staggerDirection = AG2_STAGGERDIRECTION_RIGHT;
								}
							}
						}
						else if (ForceDir * FwdDir <= -_SIN45)
						{
							hitDirection = AG2_IMPACTDIRECTION_BACK;
							staggerDirection = AG2_STAGGERDIRECTION_FORWARD;
							if (Random > 0.75f)
							{
								if (Random > 0.5f)
								{
									staggerDirection = AG2_STAGGERDIRECTION_LEFT;
								}
								else
								{
									staggerDirection = AG2_STAGGERDIRECTION_RIGHT;
								}
							}
						}
						else if (ForceDir * LeftDir >= _SIN45)
						{
							hitDirection = AG2_IMPACTDIRECTION_LEFT;
							staggerDirection = AG2_STAGGERDIRECTION_RIGHT;
							if (Random > 0.75f)
							{
								if (Random > 0.5f)
								{
									staggerDirection = AG2_STAGGERDIRECTION_FORWARD;
								}
								else
								{
									staggerDirection = AG2_STAGGERDIRECTION_BACK;
								}
							}
						}
						else if (ForceDir * LeftDir <= -_SIN45)
						{
							hitDirection = AG2_IMPACTDIRECTION_RIGHT;
							staggerDirection = AG2_STAGGERDIRECTION_LEFT;
							if (Random > 0.75f)
							{
								if (Random > 0.5f)
								{
									staggerDirection = AG2_STAGGERDIRECTION_FORWARD;
								}
								else
								{
									staggerDirection = AG2_STAGGERDIRECTION_BACK;
								}
							}
						}
						else
						{
							hitDirection = AG2_IMPACTDIRECTION_NONE;
							staggerDirection = AG2_STAGGERDIRECTION_BACK;
							if (Random > 0.5f)
							{
								if (Random > 0.5f)
								{
									staggerDirection = AG2_STAGGERDIRECTION_LEFT;
								}
								else
								{
									staggerDirection = AG2_STAGGERDIRECTION_RIGHT;
								}
							}
						}
					}

					// Determine weather the hit was low/mid/high
					if (_Hitloc == CWObject_Character::SHitloc::HITLOC_HIGH)
					{
						hitlocation = AG2_IMPACTLOCATION_HIGH;
					}
					else if (_Hitloc == CWObject_Character::SHitloc::HITLOC_LOW)
					{
						hitlocation = AG2_IMPACTLOCATION_LOW;
					}
					// Stagger range:
					// We reduce the range based on CurHealthFract
					// We also reduce range based on available staggerspace
					staggerRange = AG2_STAGGER_RANGE_LONG;
					// We almost always play the long hurt when circumstances are right
					CAI_Core* pAttackerAI = GetAI(_iAttacker);
					int32 iPlayer = GetClosestPlayer();

					if (_iDamageType == DAMAGETYPE_PHYSICS)
					{	
						if ((_iAttacker == iPlayer)&&(_Damage > 0))
						{	// Player hurls something at us, overreact!
							if (Random <= 0.5f)
							{
								hurtType = AG2_IMPULSEVALUE_HURTTYPE_LONG;
							}
							else
							{
								hurtType = AG2_IMPULSEVALUE_HURTTYPE_EXTREME;
							}
							staggerRange = AG2_STAGGER_RANGE_LONG;
						}
						else
						{
							if (_Damage == 0)
							{
								OnBumpedAG2(_iAttacker);
								return;
							}
							else
							{
								hurtType = AG2_IMPULSEVALUE_HURTTYPE_LONG;
								staggerRange = AG2_STAGGER_RANGE_LONG;
							}
						}
					}
					else
					{
						if (CurHealthFract >= 0.5f)
						{
							hurtType = AG2_IMPULSEVALUE_HURTTYPE_SHORT;
							staggerRange = AG2_STAGGER_RANGE_NONE;
						}
						else
						{
							hurtType = AG2_IMPULSEVALUE_HURTTYPE_LONG;
							staggerRange = AG2_STAGGER_RANGE_LONG;
							// Am I the last enemy of the attacker?
							if (pAttackerAI)
							{
								if (pAttackerAI->m_KB.GetDetectedEnemyCount() <= 1)
								{
									hurtType = AG2_IMPULSEVALUE_HURTTYPE_EXTREME;
									staggerRange = AG2_STAGGER_RANGE_LONG;
								}
							}
						}
					}

					if ((iAttackDir == CAI_Pathfinder::FRONT)||(iAttackDir == CAI_Pathfinder::BACK))
					{	// Fifty fifty left/right
						if (Random > 0.5f)
						{
							staggerDirection = AG2_STAGGERDIRECTION_LEFT;
							iDir = CAI_Pathfinder::LEFT;
						}
						else
						{
							staggerDirection = AG2_STAGGERDIRECTION_RIGHT;
							iDir = CAI_Pathfinder::RIGHT;
						}
					}
					else	// Left or right attackdir
					{
						if (Random > 0.5f)
						{
							staggerDirection = AG2_STAGGERDIRECTION_FORWARD;
							iDir = CAI_Pathfinder::FRONT;
						}
						else
						{
							staggerDirection = AG2_STAGGERDIRECTION_BACK;
							iDir = CAI_Pathfinder::BACK;
						}
					}
					/*
					fp32 range = 32.0f * staggerRange;
					if (!m_PathFinder.GroundTraversableDirection(iDir,&range))
					{
						// The other side?
						switch(iDir) 
						{
						case CAI_Pathfinder::FRONT:
							staggerDirection = AG2_STAGGERDIRECTION_BACK;
							iDir = CAI_Pathfinder::BACK;
							break;
						case CAI_Pathfinder::LEFT:
							staggerDirection = AG2_STAGGERDIRECTION_RIGHT;
							iDir = CAI_Pathfinder::RIGHT;
							break;
						case CAI_Pathfinder::BACK:
							staggerDirection = AG2_STAGGERDIRECTION_FORWARD;
							iDir = CAI_Pathfinder::FRONT;
							break;
						case CAI_Pathfinder::RIGHT:
							staggerDirection = AG2_STAGGERDIRECTION_LEFT;
							iDir = CAI_Pathfinder::LEFT;
							break;
						}

						range = 32.0f * staggerRange;
						if (!m_PathFinder.GroundTraversableDirection(iDir,&range))
						{
							if (range < 64.0f)
							{
								if (range >= 32.0f)
								{
									staggerRange = AG2_STAGGER_RANGE_SHORT;
								}
								else
								{
									staggerRange = AG2_STAGGER_RANGE_NONE;
								}
							}
							range = 32.0f * staggerRange;
						}
					}
					*/
					// Attacks from Darklings always produce long hurts
					if (_iDamageType & DAMAGETYPE_DARKNESS)
					{
						hurtType = AG2_IMPULSEVALUE_HURTTYPE_LONG;
					}
				}
				else
				{	
					hurtType = AG2_IMPULSEVALUE_HURTTYPE_LONG;
					hitDirection = AG2_IMPACTDIRECTION_FRONT;
					staggerDirection = AG2_STAGGERDIRECTION_BACK;
					staggerRange = AG2_STAGGER_RANGE_ONPLACE;
					hitlocation = AG2_IMPACTLOCATION_MID;
				}
	
				if (m_CharacterClass == CLASS_UNDEAD)
				{	// Zombie dudes get the long hurt, all the time pal.
					hurtType = AG2_IMPULSEVALUE_HURTTYPE_EXTREME;
					staggerRange = AG2_STAGGER_RANGE_LONG;
				}

				// Depends on CurHealthFract but 
				// Setup the AG properties first
				if (DebugTarget())
				{
					CStr Hurt = m_pGameObject->GetName();
					Hurt+= " Hurt: ";
					switch(hitDirection)
					{
					case AG2_IMPACTDIRECTION_NONE:
						Hurt = Hurt+CStr("HITDIR: None, ");
						break;
					case AG2_IMPACTDIRECTION_FRONT:
						Hurt = Hurt+CStr("HITDIR: Front, ");
						break;
					case AG2_IMPACTDIRECTION_BACK:
						Hurt = Hurt+CStr("HITDIR: Back, ");
						break;
					case AG2_IMPACTDIRECTION_LEFT:
						Hurt = Hurt+CStr("HITDIR: Left, ");
						break;
					case AG2_IMPACTDIRECTION_RIGHT:
						Hurt = Hurt+CStr("HITDIR: Right, ");
						break;
					}

					switch(staggerDirection)
					{
					case AG2_STAGGERDIRECTION_FORWARD:
						Hurt = Hurt+CStr("STAG_DIR: Fwd, ");
						break;
					case AG2_STAGGERDIRECTION_BACK:
						Hurt = Hurt+CStr("STAG_DIR: Bwd, ");
						break;
					case AG2_STAGGERDIRECTION_LEFT:
						Hurt = Hurt+CStr("STAG_DIR: Left, ");
						break;
					case AG2_STAGGERDIRECTION_RIGHT:
						Hurt = Hurt+CStr("STAG_DIR: Right, ");
						break;
					}

					switch(staggerRange)
					{
					case AG2_STAGGER_RANGE_NONE:
						Hurt = Hurt+CStr("STAG_RNG: None, ");
						break;
					case AG2_STAGGER_RANGE_ONPLACE:
						Hurt = Hurt+CStr("STAG_RNG: Inplace, ");
						break;
					case AG2_STAGGER_RANGE_SHORT:
						Hurt = Hurt+CStr("STAG_RNG: Short, ");
						break;
					case AG2_STAGGER_RANGE_LONG:
						Hurt = Hurt+CStr("STAG_RNG: Long, ");
						break;
					}

					switch(hitlocation)
					{
					case AG2_IMPACTLOCATION_NONE:
						Hurt = Hurt+CStr("HITLOC: None, ");
						break;
					case AG2_IMPACTLOCATION_HIGH:
						Hurt = Hurt+CStr("HITLOC: High, ");
						break;
					case AG2_IMPACTLOCATION_MID:
						Hurt = Hurt+CStr("HITLOC: Mid, ");
						break;
					case AG2_IMPACTLOCATION_LOW:
						Hurt = Hurt+CStr("HITLOC: Low, ");
						break;
					}

					switch(hurtType)
					{
					case AG2_IMPULSEVALUE_HURTTYPE_SHORT:
						Hurt = Hurt+CStr("HURT: Short");
						break;
					case AG2_IMPULSEVALUE_HURTTYPE_LONG:
						Hurt = Hurt+CStr("HURT: Long");
						break;
					case AG2_IMPULSEVALUE_HURTTYPE_EXTREME:
						Hurt = Hurt+CStr("HURT: Extreme");
						break;
					case AG2_IMPULSEVALUE_HURTTYPE_DEAD:
						Hurt = Hurt+CStr("HURT: Dead");
						break;
					}
					ConOutL(Hurt);
				}
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_HURT_IMPACTDIRECTION,hitDirection);
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_HURT_STAGGERDIRECTION,staggerDirection);
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_HURT_RANGE,staggerRange);
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_HURT_IMPACTLOCATION,hitlocation);

				CWAG2I_Context AG2Context(m_pGameObject,m_pServer,pCD->m_GameTime);
				pCD->m_AnimGraph2.SendHurtImpulse(&AG2Context, CXRAG2_Impulse(AG2_IMPULSETYPE_HURT,hurtType),0);
			}
		}
	}
};

//Handles the event of us taking damage for some reason
void CAI_Core::OnTakeDamage(int _iAttacker, int _Damage, uint32 _iDamageType, int _Hitloc)
{
	MSCOPESHORT(CAI_Core::OnTakeDamage);

	//Raise surprise event (this won't actually get raised unless this is the first time we take damage and there's no spotted enemies)
	m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SURPRISED,0);

	fp32 CurHealthFract = (Health() - (fp32)_Damage) / MaxHealth();

	if (_Damage > 0)
	{     // Shut the fuck up!
		ShutUp(CAI_Action::PRIO_COMBAT);
		//Raise injury events
		m_LastHurtTimer = m_Timer;
		m_EventHandler.RaiseEvent(CAI_EventHandler::ON_INJURED,_iAttacker,_iDamageType);

		if (IsPlayer(_iAttacker))
		{	// Player is the attacker
			if (m_DamageFromPlayerFactor != 1.0f)
			{     // Add/subtract health by factors difference to 1.0
				int AddHealth = RoundToInt((1.0f - m_DamageFromPlayerFactor) * _Damage);
				m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADDHEALTH,AddHealth), m_pGameObject->m_iObject);
			}

			// Shoot the Kamikaze and he'll explode!
			if ((m_CharacterClass == CLASS_DARKLING)&&/*(m_KB.GetDetectedEnemyCount() > 0)&&*/(MRTC_ISKINDOF(this,CAI_Core_Darkling)))
			{
				CAI_Core_Darkling* pDK = safe_cast<CAI_Core_Darkling>(this);
				if ((pDK)&&(pDK->m_DK_Special == CAI_Core_Darkling::DK_SPECIAL_KAMIKAZE))
				{
					CAI_Action* pAction;
					if (m_AH.GetAction(CAI_Action::DARKLING_CLOSE,&pAction))
					{
						CAI_Action_DarklingClose* pAttackAction = safe_cast<CAI_Action_DarklingClose>(pAction);
						pAttackAction->AnimationEvent(GetObjectID(),1);
					}
				}
			}
		}
		else
		{	// Player is not the attacker
			if ((m_bPlayerOnlyMeleeDmg)&&(_iDamageType & DAMAGETYPE_BLOW))
			{     //Boost health to avoid falling below limit
				m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADDHEALTH,_Damage), m_pGameObject->m_iObject);
			}
		}

		

		//If immortal, make sure we never fall below minhealth health
		if (m_MinHealth > 0)
		{
			if (Health() - _Damage < m_MinHealth)
				//Boost health to avoid falling below limit
				m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADDHEALTH, m_MinHealth + _Damage - Health(m_pGameObject->m_iObject)), m_pGameObject->m_iObject);
		}

		// Ragadolly when seriously hurt while running behaviour other than CB
		// Recalculate 'coz immune etc
		CurHealthFract = (Health() - (fp32)_Damage) / MaxHealth();
		if ((m_UndeadDieBehaviour)&&(m_CharacterClass == CLASS_BADGUY)&&(!(_iDamageType & DAMAGETYPE_BLACKHOLE))&&(m_iCurBehaviour != BEHAVIOUR_COMBAT_BLACKHOLE_STAND)&&
			(CurHealthFract <= 0.8f)&&(CurHealthFract > 0.0f)&&
			(m_iCurBehaviour)&&(m_CurCB.m_iBehaviour == 0)&&
			(m_BehaviourStopIssued == BEHAVIOUR_STOP_NONE)&&
			(m_BehaviourPrioClass < CAI_Action::PRIO_FORCED))
		{	// Ragdoll us and returneth
			if (SetUndeadStun(m_UndeadSleepTicks,true))
			{
				m_EventHandler.RaiseEvent(CAI_EventHandler::ON_STUNNED,_iAttacker,_iDamageType);
				m_KB.OnTakeDamage(_iAttacker, _Damage, _iDamageType);
				return;
			}
		}

		if ((m_AgonyBehaviour)&&(!m_CurCB.m_iBehaviour)&&(CurHealthFract > 0.0f)&&(CurHealthFract <= 0.5f))
		{
			int iPlayer = GetClosestPlayer();
			CAI_Core* pPlayerAI = GetAI(iPlayer);
			if ((pPlayerAI)&&(pPlayerAI->m_KB.GetDetectedEnemyCount() <= 1)&&(Random < 0.5f))
			{	// Last man?
				if (SetAgonyStun())
				{
					m_EventHandler.RaiseEvent(CAI_EventHandler::ON_STUNNED,_iAttacker,_iDamageType);
					m_KB.OnTakeDamage(_iAttacker, _Damage, _iDamageType);
					return;
				}
			}
		}

		if ((m_CharacterClass == CLASS_UNDEAD)&&(!(_iDamageType & DAMAGETYPE_DARKNESS_GUN)))
		{
			if (m_iBehaviourRunning == m_UndeadDieBehaviour)
			{
				bool bAlreadyStunned = (m_StunTimeout != 0)? true : false;
				if ((SetUndeadStun(m_UndeadSleepTicks))&&(!bAlreadyStunned))
				{
					m_EventHandler.RaiseEvent(CAI_EventHandler::ON_STUNNED,_iAttacker,_iDamageType);
				}

				m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_DEAD,m_UndeadSleepTicks);
				//Activate!
				SetActivationState(STATE_ACTIVE);
				SetActivityLevel(ACTIVITY_FULL, true);
				m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADDHEALTH,1+_Damage - Health(m_pGameObject->m_iObject)), m_pGameObject->m_iObject);
				_Damage = 0;
				return;
			}
			else if (Health() - _Damage <= 0)
			{
				bool bAlreadyStunned = (m_StunTimeout != 0)? true : false;
				if ((SetUndeadStun(m_UndeadSleepTicks))&&(!bAlreadyStunned))
				{
					m_EventHandler.RaiseEvent(CAI_EventHandler::ON_STUNNED,_iAttacker,_iDamageType);
				}
				m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_DEAD,m_UndeadSleepTicks);
				//Activate!
				SetActivationState(STATE_ACTIVE);
				SetActivityLevel(ACTIVITY_FULL, true);
				m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADDHEALTH,1+_Damage - Health(m_pGameObject->m_iObject)), m_pGameObject->m_iObject);
				return;
			}
		}

		if ((m_iBehaviourRunning)&&(m_CurCB.m_iBehaviour == 0)&&(!m_StunTimeout)&&(!m_Script.IsPaused()))
		{
			if ((m_iBehaviourRunning != BEHAVIOUR_COMBAT_BLACKHOLE_STAND)&&(m_iBehaviourRunning != BEHAVIOUR_COMBAT_BLACKHOLE_SUCKEDIN))
			{
				StopBehaviour(BEHAVIOUR_STOP_FAST,CAI_Action::PRIO_FORCED);
			}
		}

		// If we died, we should inform our team friends that we died and where
		if (Health() - _Damage <= 0)
		{     // We died!
			StopBehaviour(BEHAVIOUR_STOP_FAST,CAI_Action::PRIO_FORCED);
			m_StunTimeout = 80;
			if (m_bNextOfKin)
			{
				int8 CauseOfDeath = SDead::DEATHCAUSE_UNKNOWN;
				switch(_iDamageType & DAMAGETYPE_MASK)
				{
				case DAMAGETYPE_BLOW:
				case DAMAGETYPE_TRANQUILLIZER:
				case DAMAGETYPE_FALL:
				case DAMAGETYPE_BLAST:
				case DAMAGETYPE_PHYSICS:
					CauseOfDeath = SDead::DEATHCAUSE_UNKNOWN;
					break;

				case DAMAGETYPE_PIERCE:
					CauseOfDeath = SDead::DEATHCAUSE_CUT;
					break;

				case DAMAGETYPE_PISTOL:
				case DAMAGETYPE_RIFLE:
				case DAMAGETYPE_AP:
					CauseOfDeath = SDead::DEATHCAUSE_SHOT;
					break;      

				case DAMAGETYPE_DARKNESS:
				case DAMAGETYPE_DARKNESS_GUN:
				case DAMAGETYPE_BLACKHOLE:
					CauseOfDeath = SDead::DEATHCAUSE_DARKNESS;
					break;
				}
				m_KB.Global_ReportDeath(_iAttacker,m_pGameObject->m_iObject,m_pGameObject->GetPosition(),CauseOfDeath);
			}
			SetAnimgraphDeactivation(true);
		}
		if (!(DAMAGETYPE_LIGHT & _iDamageType))
		{
			OnTakeDamageAG2(_iAttacker,_Damage,_iDamageType,_Hitloc);
			if (CurHealthFract >= 0.5f)
			{
				m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_HURT,60);
			}
			else
			{
				m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_INPAIN,100);
			}
		}
	}
	else
	{
		//Raise injury events
		if (_iDamageType & DAMAGETYPE_TRANQUILLIZER)
		{     // We consider tranq hits somewhat like damaging hits
			if (!m_StunTimeout)
			{
				m_EventHandler.RaiseEvent(CAI_EventHandler::ON_STUNNED,_iAttacker,_iDamageType);
			}
			StopBehaviour(BEHAVIOUR_STOP_FAST,CAI_Action::PRIO_COMBAT);
			CWObject_Message Msg(OBJMSG_CHAR_DESTROYCAUSUALDIALOGUE);
			Msg.m_Param0 = CAI_Action::PRIO_COMBAT;
			m_pServer->Message_SendToObject(Msg,m_pGameObject->m_iObject);
			if ((_iDamageType & DAMAGETYPE_TRANQUILLIZER)&&(m_StunTimeout < 100)&&(m_StunTimeout > -1))
			{
				m_StunTimeout = 100;
			}
		}
		else
		{     // Just a scratch,or a bump
			if (_iDamageType & DAMAGETYPE_UNDEFINED)
			{     // Physics bump
				OnBumpedAG2(_iAttacker);
			}
			else
			{
				m_EventHandler.RaiseEvent(CAI_EventHandler::ON_HIT_UNINJURED,_iAttacker);
			}
		}
	}

	//Activate!
	SetActivationState(STATE_ACTIVE);
	SetActivityLevel(ACTIVITY_FULL, true);


	//Notify knowledge base if the attack didn't kill us
	if ((Health() - _Damage > 0)&&(!m_StunTimeout))
	{
		m_KB.OnTakeDamage(_iAttacker, _Damage, _iDamageType);
	}
};

bool CAI_Core::OnAlign(CWO_ScenePoint* _pScenePoint, int _iObj, int _Time, bool _bReverseDir)
{
	if (_pScenePoint)
	{
		CVec3Dfp32 SPDir = _pScenePoint->GetDirection();
		if (_bReverseDir)
		{
			SPDir = -SPDir;
		}
		CVec3Dfp32 Dir = GetLookDir();
		OnTrackDir(SPDir,_iObj, _Time, false, false);
		if (_pScenePoint->GetSqrRadius() < Sqr(8.0f))
		{	// Perfect placement type
			// Start doing perfect placement until we are pretty close to what we want
			if (SPDir * Dir > 0.999f)
			{	// Within 2.5 degrees
				SetPerfectPlacement(_pScenePoint);
				return(true);
			}
			else if (SPDir * Dir > 0.99f)
			{	// Within 8 degrees
				SetPerfectPlacement(_pScenePoint);
			}
		}
		else
		{
			if (_pScenePoint->IsAligned(Dir,_bReverseDir))
			{
				return(true);
			}
		}
	}
	return(false);
};

bool CAI_Core::OnAlign(const CMat4Dfp32* _pMDest, int _iObj, int _Time)
{
	if (_pMDest)
	{
		CVec3Dfp32 SPDir = _pMDest->GetRow(0);
		CVec3Dfp32 Dir = GetLookDir();
		OnTrackDir(SPDir,_iObj, _Time, false, false);
		// Perfect placement type
		// Start doing perfect placement until we are pretty close to what we want
		if (SPDir * Dir > 0.999f)
		{	// Within 2.5 degrees
			SetPerfectPlacement(_pMDest);
			return(true);
		}
		else if (SPDir * Dir > 0.99f)
		{	// Within 8 degrees
			SetPerfectPlacement(_pMDest);
		}
	}
	return(false);
};

// Returns true when we are sufficiently close to do PP
bool CAI_Core::OnNudge(CWO_ScenePoint* _pScenePoint, fp32 _Speed, bool _bUseOffset)
{
	if ((_pScenePoint)&&(_Speed >= AI_MOVE_THRESHOLD_WALKSLOW))
	{
		CVec3Dfp32 SPPos = _pScenePoint->GetPosition(_bUseOffset);
		if ((_pScenePoint->GetSitFlag())&&(SqrDistanceToUs(SPPos) <= Sqr(32.0f)))
		{
			SetObjColl(false);
		}
		else
		{
			SetObjColl(true);
		}

		return(OnNudge(SPPos, _Speed));
	}
	else
	{
		return(false);
	}
};

bool CAI_Core::OnNudge(CVec3Dfp32 _Pos, fp32 _Speed)
{
	if ((VALID_POS(_Pos))&&(_Speed >= AI_MOVE_THRESHOLD_WALKSLOW))
	{
		CVec3Dfp32 Diff = _Pos - GetBasePos();
		if (Abs(Diff[2]) < 16.0f)
		{
			Diff[2] = 0.0f;
		}
		if (Diff.LengthSqr() < Sqr(16.0f))
		{
			if (Sqr(_Speed) > Diff.LengthSqr())
			{
				_Speed = Diff.Length();
			}
			AddMoveTowardsPositionCmd(_Pos,_Speed);
		}
		if (Diff.LengthSqr() <= 8.0f)
		{
			return(true);
		}
	}

	return(false);
};

//Change the direction (heading and pitch) the bot looks at. The change will take place gradually 
//over the course of the given number of frames.
// This is the ONLY call that actually communicates anything to m_DeviceLook
void CAI_Core::OnTrackDir(CVec3Dfp32 _Dir, int _iObj, int _Time, bool _bHeadingOnly, bool _bLookSoft)
{
	//Check that look device is free
	if (!m_DeviceLook.IsAvailable())
		return;

	if ((_bLookSoft)||(m_DeviceLook.IsSoftLook(true)))
	{
		_Dir.Normalize();
		if (m_DeviceLook.BadLookDir(_Dir))
		{
			m_pGameObject->AI_SetEyeLookDir(CVec3Dfp32(_FP32_MAX), _iObj);
			return;
		}
	}

	// Difficulty handling
	if (_Time == -1)
	{	// Tracking time is handled by
		fp32 lookScale = 2.0f - (GetLookDir() * _Dir);
		if (m_pAIResources->ms_GameDifficulty == DIFFICULTYLEVEL_EASY)
		{
			_Time = TruncToInt(lookScale * 0.25 * 2.0f * GetAITicksPerSecond());
		}
		else if (m_pAIResources->ms_GameDifficulty == DIFFICULTYLEVEL_HARD)
		{
			_Time = TruncToInt(lookScale * 0.25 * 0.5f * GetAITicksPerSecond());
		}
		else	// DIFFICULTYLEVEL_NORMAL
		{
			_Time = TruncToInt(lookScale * 0.25 * 1.0f * GetAITicksPerSecond());
		}
	}
	if ((_Time < 3)&&(_Time != 1)) 
	{
		_Time = 3;
	}

	CVec3Dfp32 HeadPos = GetLookPos();
	CVec3Dfp32 Look;
	if (m_bWallClimb)
	{
		// Take variant up vector into account
		fp32 MyHeading, MyPitch;
		MyHeading = MyPitch = 0.0f;
		// ***
		if (DebugTarget())
		{
			m_pServer->Debug_RenderMatrix(m_CurHeadMat,1.0f,true);
		}
		// ***
		_Dir.Normalize();
		// No! Don't you try using Up = GetUp(); we want local up, not absolute up
		CVec3Dfp32 Fwd = m_CurHeadMat.GetRow(0);
		CVec3Dfp32 Left = m_CurHeadMat.GetRow(1);
		CVec3Dfp32 Up = m_CurHeadMat.GetRow(2);
		if (Fwd * _Dir > 0.999f)
		{	// Already aligned
			//We are done looking. 
			m_ActionFlags &= ~LOOKING;
			m_DeviceLook.Use(_bLookSoft, _Dir);
			m_LookTime = 0;
			return;
		}

		m_LookDir = _Dir;
		m_LookTime = _Time;
		m_ActionFlags |= LOOKING;
		// Calculate MyHeading first
		// Project Dir onto Fwd,Left plane
		CVec3Dfp32 DirHorizontal = _Dir - Up*(_Dir*Up);
		DirHorizontal.Normalize();
		fp32 CosAngle = DirHorizontal * Fwd;
		fp32 SinAngle = DirHorizontal * Left;
		if (Abs(CosAngle) < kEpsilon)
		{
			if (SinAngle >= 0)
			{	// 90 degrees CCW
				MyHeading = _PIHALF;
			}
			else
			{	// 90 degrees CW
				MyHeading = -_PIHALF;
			}
		}
		else
		{
			MyHeading = atan2f(SinAngle,CosAngle);
		}
		// Calculate MyPitch
		// Project Dir onto Fwd,Up plane
		CVec3Dfp32 DirVertical = _Dir - Left*(_Dir*Left);
		DirVertical.Normalize();
		CosAngle = Abs(DirVertical * Fwd);
		SinAngle = DirVertical * Up;
		if (Abs(CosAngle) < kEpsilon)
		{
			if (SinAngle >= 0)
			{	// 90 degrees pitch up
				MyPitch = _PIHALF;
			}
			else
			{	// 90 degrees pitch down
				MyPitch = -_PIHALF;
			}
		}
		else
		{
			MyPitch = atan2f(SinAngle,CosAngle);
		}
		MyHeading /= _PI2;
		MyPitch /= _PI2;

		// Limit pitch etc
		if (_bHeadingOnly)
		{
			MyPitch = 0.0f;
		}
		// Clamp _Pitch to +45 -60 degrees
		if (MyPitch > 0.125f)
		{
			MyPitch = 0.125f;
		}
		else if (MyPitch < -0.167f)
		{
			MyPitch = -0.167f;
		}
		// Limit _Time by m_TurnPerTick
		if (m_TurnPerTick > 0.0f)
		{
			if (Abs(MyPitch) > m_TurnPerTick)
			{
				MyPitch = Sign(MyPitch) * m_TurnPerTick;
			}
			if (Abs(MyHeading) > m_TurnPerTick)
			{
				MyHeading = Sign(MyHeading) * m_TurnPerTick;
			}
		}

		if (Abs(MyPitch) < 0.01f)
		{
			MyPitch = 0.0f;
		}
		if (Abs(MyHeading) < 0.01f)
		{
			MyHeading = 0.0f;
		}
		m_LookTime = _Time;
		Look[0] = 0.0f;
		Look[1] = MyPitch / m_LookTime;
		Look[2] = -MyHeading / m_LookTime;
		m_DeviceLook.Use(Look, _bLookSoft, _Dir);
		_Dir.Normalize();
		m_pGameObject->AI_SetEyeLookDir(_Dir, _iObj);
	}
	else
	{
		CVec3Dfp32 Pos = HeadPos + _Dir * 100.0f;
		fp32 Heading = HeadingToPosition(HeadPos,Pos);
		fp32 Pitch = 0;
		if (!_bHeadingOnly)
		{
			Pitch = PitchToPosition(HeadPos, Pos);
		}
		OnLook(Heading, Pitch, _Time, _bLookSoft);
		_Dir.Normalize();
		m_pGameObject->AI_SetEyeLookDir(_Dir, _iObj);
#ifndef M_RTM
		if (DebugTarget())
		{
			Debug_RenderWire(HeadPos,HeadPos+_Dir*100,kColorRed,fp32(_Time) / fp32(GetAITicksPerSecond()));
		}
#endif
	}
};

//Change the direction (heading and pitch) the bot looks at. The change will take place gradually 
//over the course of the given number of frames.
void CAI_Core::OnLook(fp32 _Heading, fp32 _Pitch, int _Time, bool _bLookSoft)
{
	MSCOPESHORT(CAI_Core::OnLook);

	//Check that look device is free
	if (!m_DeviceLook.IsAvailable())
		return;

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
	if (pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOLOOK)
	{
		return;
	}

	if ((_Time < 3)&&(_Time != 1))
	{
		_Time = 3;
	}

	CVec3Dfp32 CurLook = GetLookAngles();

	//Check if this is new look direction
	if ((_Heading != _FP32_MAX)&&(_Pitch != _FP32_MAX)&&
		((Abs(_Heading - m_LookAngles[2]) > AI_LOOK_ANGLE_THRESHOLD)||
		(Abs(_Pitch - m_LookAngles[1]) > AI_LOOK_ANGLE_THRESHOLD)))
	{
		// Clamp _Pitch to +45 -60 degrees
		if (_Pitch > 0.125f)
		{
			_Pitch = 0.125f;
		}
		else if (_Pitch < -0.167f)
		{
			_Pitch = -0.167f;
		}

		// Limit _Time by m_TurnPerTick
		if (m_TurnPerTick > 0.0f)
		{
			fp32 TurnPerTick = Abs(CurLook[2]-_Heading) / _Time;
			if (TurnPerTick > m_TurnPerTick)
			{
				_Time = RoundToInt((TurnPerTick / m_TurnPerTick) * _Time);
			}
		}

		//New look destination
		m_LookAngles[2] = _Heading;
		m_LookAngles[1] = _Pitch;
		m_LookAngles[0] = 0;
		m_LookTime = _Time;
	}

	//Set target direction if necessary
	CVec3Dfp32 TargetDir = CVec3Dfp32(_FP32_MAX);
	if (_bLookSoft || m_DeviceLook.IsSoftLook(true))
	{
		fp32 HRadians = _Heading * _PI2;
		if (m_UseFlags & USE_SOFTLOOKPITCH)
		{
			fp32 PRadians = _Pitch * _PI2;
			TargetDir = LookAngles2LookDir(CVec3Dfp32(0, PRadians, HRadians));
		}
		else
		{
			// Heading only
			TargetDir = CVec3Dfp32(M_Cos(HRadians), M_Sin(-HRadians), 0);
		}
	}


	//Do we lack a direction to look at, or do we already look in that direction?
	if ((m_LookAngles[2] == _FP32_MAX) ||
		(m_LookAngles[1] == _FP32_MAX) ||
		((m_LookAngles[2] == CurLook[2]) &&
		(m_LookAngles[1] == CurLook[1])))
	{
		//We are done looking. 
		m_ActionFlags &= ~LOOKING;
		m_DeviceLook.Use(_bLookSoft, TargetDir);
	} 
	else
	{
		//We are looking
		CVec3Dfp32 dLook;
		//Never change tilt
		dLook[0] = 0;

		//Set pitch change... This value should always be between 0.5 and -0.5
		dLook[1] = (m_LookAngles[1] - CurLook[1]);
		dLook[1] -= (int)dLook[1];
		if (dLook[1] < -0.5f)
			dLook[1] = 1 + dLook[1];
		else if (dLook[1] > 0.5f)
			dLook[1] = dLook[1] - 1;

		//Set heading change
		dLook[2] = (m_LookAngles[2] - CurLook[2]);
		dLook[2] -= (int)dLook[2];
		if (dLook[2] < -0.5f)
			dLook[2] = 1 + dLook[2];
		else if (dLook[2] > 0.5f)
			dLook[2] = dLook[2] - 1;

		// Moderate turn by m_TurnPerTick
		if (m_LookTime > 0)
		{
			dLook[1] /= m_LookTime;
			dLook[2] /= m_LookTime;
			m_ActionFlags |= LOOKING;
			m_DeviceLook.Use(dLook, _bLookSoft, TargetDir);
			m_LookTime--;
		}
		else
		{	// Final look
			m_LookTime = 0;
			m_ActionFlags |= LOOKING;
			m_DeviceLook.Use(dLook, _bLookSoft, TargetDir);
		}
	};
};

void CAI_Core::RefreshLook()
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
	if ((pCD)&&(pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOLOOK))
	{
		m_LookAngles = GetLookAngles();
		m_LookAngles[0] = 0;
		m_LookTime = 0;
		return;
	}

	if (m_DeviceLook.IsAvailable())
	{
		if (m_LookTime > 0)
		{
			if (VALID_POS(m_LookDir))
			{
				fp32 MyHeading;
				fp32 MyPitch;
				CVec3Dfp32 Fwd = m_CurHeadMat.GetRow(0);
				CVec3Dfp32 Left = m_CurHeadMat.GetRow(1);
				CVec3Dfp32 Up = m_CurHeadMat.GetRow(2);
		
				// Calculate MyHeading first
				// Project m_LookDir onto Fwd,Left plane
				CVec3Dfp32 DirHorizontal = m_LookDir - Up*(m_LookDir*Up);
				DirHorizontal.Normalize();
				fp32 CosAngle = DirHorizontal * Fwd;
				fp32 SinAngle = DirHorizontal * Left;
				if (Abs(CosAngle) < kEpsilon)
				{
					if (SinAngle >= 0)
					{	// 90 degrees CCW
						MyHeading = _PIHALF;
					}
					else
					{	// 90 degrees CW
						MyHeading = -_PIHALF;
					}
				}
				else
				{
					MyHeading = atan2f(SinAngle, CosAngle);
				}
				// Calculate MyPitch
				// Project m_LookDir onto Fwd,Up plane
				CVec3Dfp32 DirVertical = m_LookDir - Left*(m_LookDir*Left);
				DirVertical.Normalize();
				CosAngle = Abs(DirVertical * Fwd);
				SinAngle = DirVertical * Up;
				if (Abs(CosAngle) < kEpsilon)
				{
					if (SinAngle >= 0)
					{	// 90 degrees pitch up
						MyPitch = _PIHALF;
					}
					else
					{	// 90 degrees pitch down
						MyPitch = -_PIHALF;
					}
				}
				else
				{
					MyPitch = atan2f(SinAngle,CosAngle);
				}
				MyHeading /= _PI2;
				MyPitch /= _PI2;

				// Clamp _Pitch to +45 -60 degrees
				if (MyPitch > 0.125f)
				{
					MyPitch = 0.125f;
				}
				else if (MyPitch < -0.167f)
				{
					MyPitch = -0.167f;
				}
				// Limit _Time by m_TurnPerTick
				if (m_TurnPerTick > 0.0f)
				{
					if (Abs(MyPitch) > m_TurnPerTick)
					{
						MyPitch = Sign(MyPitch) * m_TurnPerTick;
					}
					if (Abs(MyHeading) > m_TurnPerTick)
					{
						MyHeading = Sign(MyHeading) * m_TurnPerTick;
					}
				}

				if (Abs(MyPitch) < 0.01f)
				{
					MyPitch = 0.0f;
				}
				if (Abs(MyHeading) < 0.01f)
				{
					MyHeading = 0.0f;
				}

				CVec3Dfp32 Look;
				Look[0] = 0.0f;
				Look[1] = MyPitch / m_LookTime;
				Look[2] = -MyHeading / m_LookTime;
				m_DeviceLook.Use(Look, false, m_LookDir);
				m_LookTime--;
			}
			else if VALID_POS(m_LookAngles)
			{
				m_LookTime--;
			}
			else
			{
				m_LookTime = 0;
			}
		}
		else
		{
			INVALIDATE_POS(m_LookDir);
			INVALIDATE_POS(m_LookAngles);
		}
	}
};

void CAI_Core::OnTrackPos(const CVec3Dfp32& _Pos, int _Time, bool _bHeadingOnly, bool _bLookSoft)
{
	CVec3Dfp32 HeadPos = GetLookPos();
	CVec3Dfp32 Dir = _Pos - HeadPos;
	OnTrackDir(Dir,0,_Time,_bHeadingOnly,_bLookSoft);
	return;
}

//Overload with target position as param
void CAI_Core::OnTrackObj(int _iObj, int _Time, bool _bHeadingOnly, bool _bLookSoft)
{
#ifndef M_RTM
	if (_iObj == GetObjectID())
	{
		CStr Name = m_pGameObject->GetName();
		ConOutL(Name+CStr(": OnTrackObj() called on self ")+CStrF("%d Tick ",GetAITick()));
		return;
	}
#endif
	CVec3Dfp32 Pos = GetHeadPos(_iObj);
	if (VALID_POS(Pos))
	{
		CVec3Dfp32 Dir = Pos - GetLookPos();
		OnTrackDir(Dir,_iObj,_Time,_bHeadingOnly,_bLookSoft);
	}
}

//Overload with target position as param
void CAI_Core::OnTrackAgent(CAI_AgentInfo* pAgent,int _Time,bool bHeadingOnly, bool _bLookSoft)
{
	if (pAgent)
	{
		int32 iTrackObj = pAgent->GetObjectID();
		if (iTrackObj == m_KB.GetPlayerDarkness())
		{
			m_KB.GetCreepingDark(&iTrackObj);
			OnTrackObj(iTrackObj,_Time,bHeadingOnly,_bLookSoft);
		}
		else if (pAgent->GetCurAwareness() >= CAI_AgentInfo::SPOTTED)
		{
			OnTrackObj(pAgent->GetObjectID(),_Time,bHeadingOnly,_bLookSoft);
		}
		else
		{
			OnTrackPos(pAgent->GetSuspectedHeadPosition(),_Time,bHeadingOnly,_bLookSoft);
		}
	}
};

CVec3Dfp32 CAI_Core::OnWeaponTrackHead(CAI_AgentInfo* _pTarget,int _TrackTicks,fp32 _RetreatRng, bool _bTrack)
{
	if (!_pTarget)
	{
		return(GetLookDir());
	}

	int iTarget = _pTarget->GetObjectID();
	CAI_Core* pTargetAI = GetAI(iTarget);
	if (!pTargetAI)
	{
		return(GetLookDir());
	}
	if (_TrackTicks <= 0)
	{
		_TrackTicks = 5;
	}

	CVec3Dfp32 OurUp = GetUp();
	CVec3Dfp32 OurLookDir = GetLookDir();
	CVec3Dfp32 TrackPos = GetTorsoPosition(_pTarget);
	int32 iTrackObj = iTarget;
	if (iTarget == m_KB.GetPlayerDarkness())
	{
		if (m_KB.GetCreepingDark(&iTrackObj))
		{
			m_KB.GetDarknessPos(&TrackPos);
		}
	}
	CVec3Dfp32 PosDiff = TrackPos - GetBasePos();
	fp32 RangeSqr = PosDiff.LengthSqr();
	PosDiff.Normalize();
	if (Abs(PosDiff * OurUp) >= 0.867f)
	{
		if (_bTrack)
		{
			OnTrackDir(OurLookDir,iTrackObj,_TrackTicks,false,false);
		}
		return(OurLookDir);
	}
	CVec3Dfp32 OurLook = GetLookPos();
	CVec3Dfp32 OurAim = GetAimPosition(m_pGameObject);
	
	CVec3Dfp32 Diff = OurLook - OurAim;	// Diff between eyes and gun
	CVec3Dfp32 WantedLookDir = TrackPos-OurAim;
	if (pTargetAI->m_CharacterClass == CLASS_DARKLING)
	{	// BasePos of Darkling are already lifted 8 units
		TrackPos = pTargetAI->GetBasePos();
		WantedLookDir = TrackPos - OurAim;
	}
	if (RangeSqr <= Sqr(_RetreatRng))
	{
		NoStops();
		OurAim = GetBasePos();
		OurAim += OurUp * GetCurHeight() * 0.75f;
		WantedLookDir = TrackPos - OurAim;
		CVec3Dfp32 Move = CVec3Dfp32(-1.0f,0.0f,0.0f);
		m_DeviceMove.Use(Move * 0.25f);
	}
	if (_bTrack)
	{
		OnTrackDir(WantedLookDir,iTrackObj,_TrackTicks,false,false);
	}

	return(WantedLookDir.Normalize());
};




//Scratching our butt...
void CAI_Core::OnIdle()
{
	//Look without body turn is not supported anymore. Must restor this when it is
	if (m_DeviceLook.IsAvailable())
	{
		//Just stay put and look around lazily.
		if ((VALID_POS(m_HoldLookDir))&&(!(m_ActionFlags & LOOKING)&&(Random < 0.1f)))
		{	//Sweep gaze to another angle within 45 degrees of the default heading and reset pitch
			CVec3Dfp32 NewLook = m_HoldLookDir + CVec3Dfp32(Random-0.5f,Random-0.5f,0.5f*(Random-0.5f));
			NewLook.Normalize();
			OnTrackDir(NewLook, 0, RoundToInt(25 + Random * 50),true,true);
		}
	};
};


//Act frustrated
void CAI_Core::OnFrustration()
{
	//Just jump for now
	//m_DeviceJump.Use();
};


//Sets action from given behaviour type and parameter target
bool CAI_Core::SetAction(int _iAction, CStr _sParamName, fp32 _DistParam, fp32 _MaxDistParam, int _iActivator)
{
	bool result = m_AH.SetActionOverride(_iAction, _sParamName, _DistParam, _MaxDistParam, _iActivator);
	if (result)
	{
		return(result);
	}
	else
	{
		CStr Name = m_pGameObject->GetName();
		ConOut(CStr("Unsupported behaviour request by ") + Name);
		return(result);
	}
};


//Sets behaviour from previously given key values
void CAI_Core::SetActionFromKeys()
{
	//Don't do anything if we've already got action overrides. If so then we've either got action
	//override messages before we spawned or have set action overrides when loading.
	if (m_AH.HasActionOverrides())
		return;
    
	//Get correct behaviour parameters (the explore and survive behaviours do not take any parameters)
	CStr sParam = "";
	fp32 DistParam = -1.0f;
	fp32 MaxDistParam = -1.0f;
	
	if (m_ActionFromKey == CAI_Core::EXPLORE)
	{
		//Get first position from behaviour parameters
		int i = 0;
		while ( (i < m_lActionParamKeys.Len()) &&
			(m_lActionParamKeys[i].m_Name != "AI_POSITION") )
			i++;
		if (i < m_lActionParamKeys.Len())
			//Found position
			sParam = m_lActionParamKeys[i].m_Value;
	}
	else if (m_ActionFromKey == CAI_Core::ENGAGE)
	{
		//Get first target from behaviour parameters
		int i = 0;
		while ( (i < m_lActionParamKeys.Len()) &&
			(m_lActionParamKeys[i].m_Name != "AI_TARGET") )
			i++;
		if (i < m_lActionParamKeys.Len())
			//Found target
			sParam = m_lActionParamKeys[i].m_Value;
	}
	else if (m_ActionFromKey == CAI_Core::PATROL)
	{
		m_AH.SetActionOverride(m_ActionFromKey,m_lActionParamKeys);
		return;
	}
	else if (m_ActionFromKey == CAI_Core::HOLD)
	{
		m_AH.SetActionOverride(m_ActionFromKey,m_lActionParamKeys);
		return;
	}
	else if (m_ActionFromKey == CAI_Core::BEHAVIOUR)
	{
		m_AH.SetActionOverride(m_ActionFromKey,m_lActionParamKeys);
		return;
	};

	SetAction(m_ActionFromKey, sParam, DistParam, MaxDistParam);
};		


// Returns the closest players eyepos and eyedirection
bool CAI_Core::GetPlayerLookAndPos(CVec3Dfp32& _LookDir,CVec3Dfp32& _EyePos)
{
	int32 playerID = GetClosestPlayer();
	if (playerID == m_pGameObject->m_iObject)
	{
		return(false);
	}
	CAI_Core* pPlayerAI = GetAI(playerID);
	if ((!pPlayerAI)||(!pPlayerAI->m_pGameObject))
	{
		return(false);
	}

	_LookDir = pPlayerAI->GetLookDir();
	_EyePos = pPlayerAI->GetLookPos();

	return(true);
};

bool CAI_Core::IsLookWithinAngle(fp32 _Angle,int _iBot)
{
	if (_iBot == m_pGameObject->m_iObject)
	{
		return(false);
	}

	CAI_Core* pBotAI = GetAI(_iBot);
	if ((!pBotAI)||(!pBotAI->m_pGameObject))
	{
		return(false);
	}

	CVec3Dfp32 LookDir = pBotAI->GetLookDir();
	CVec3Dfp32 EyePos = pBotAI->GetLookPos();
	fp32 CosAngle = M_Cos(_Angle * _PI2 / 360.0f);
	CVec3Dfp32 PosDir = (GetLookPos() - EyePos).Normalize();
	if (PosDir * LookDir > CosAngle)
	{
		return(true);
	}
	else
	{
		return(false);
	}
}

bool CAI_Core::IsPlayerLookWithinCosAngle(fp32 _CosAngle)
{
	CVec3Dfp32 LookDir, EyePos;
	
	if (GetPlayerLookAndPos(LookDir,EyePos))
	{
		CMat4Dfp32 BaseMat;
		GetBaseMat(BaseMat);
		CVec3Dfp32 LookPos = BaseMat.GetRow(3) + BaseMat.GetRow(2) * GetHeight() * 0.5f;
		if (_CosAngle <= LookDir * (LookPos-EyePos).Normalize())
		{
			return(true);
		}

	}
	return(false);
};

bool CAI_Core::IsPlayerLookWithinAngle(fp32 _Angle)
{
	fp32 CosAngle = M_Cos(_Angle * _PI2 / 360.0f);
	return(IsPlayerLookWithinCosAngle(CosAngle));
};

bool CAI_Core::IsPlayerLookWithinAngle(fp32 _Angle,const CVec3Dfp32& _TargetPos)
{
	fp32 CosAngle = M_Cos(_Angle * _PI2 / 360.0f);
	CVec3Dfp32 LookDir,EyePos,PosDir;
	if (GetPlayerLookAndPos(LookDir,EyePos))
	{
		PosDir = (_TargetPos - EyePos).Normalize();
		fp32 LookCosAngle = PosDir * LookDir;
		if (LookCosAngle >= CosAngle)
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}
	return(true);
};

//Is bot active?
bool CAI_Core::IsActive()
{
	// ***
	if (m_pGameObject)
	{
		const char* pName = m_pGameObject->GetName();
		if ((CStrBase::CompareNoCase(pName, "TELEPHONEREG") == 0)||
			(CStrBase::CompareNoCase(pName, "RADIO") == 0))
		{
			return(false);
		}
	}
	// ***
	return m_ActivationState >= STATE_ACTIVE;
};


//Set activation state to given state if possible
bool CAI_Core::SetActivationState(int _State)
{
	if (m_ActivationState == STATE_ALWAYSACTIVE)
	{
		//Never switch state
		return false;
	}
	else
	{
		//Set activation tick if we're being activated or when we switch to suggested deactivation
		if ((_State == STATE_ACTIVE) || 
			((_State == STATE_SUGGEST_DEACTIVATION) && (m_ActivationState != STATE_SUGGEST_DEACTIVATION)))
			m_ActivationTick = GetAITick();

		//Always switch to any state
		m_ActivationState = _State;
		return true;
	}
};


//Checks if the bot should be activated, and sets activate flag if so
bool CAI_Core::OnActivationCheck()
{
	if (IsActive()) 
		return true;

	//A scripted bot is always active, although it may be inactivated 
	if (m_Script.IsValid())
		return true;

	if ((!m_pServer)||((m_pServer->GetGameTick()+m_Timer-m_ServerTick) % 10 != 0))

	if ((!m_pServer)||(m_Timer % 10 != 0))
		return false;

	//Check if this bot has been deactivated or not
	if (m_ActivationState == STATE_DEACTIVATED)
	{
		//This bot has been deactivated. Only activate if in LOS of player, or explicitly when hurt etc
		//Make sure we don't have LOS to a player
		int32 playerID = GetClosestPlayer();
		CAI_Core* pPlayerAI = GetAI(playerID);
		if (!pPlayerAI)
		{
			return(false);
		}

		//Check LOS to player
		CVec3Dfp32 Start = pPlayerAI->GetAimPosition();
		CVec3Dfp32 End = GetAimPosition();
		fp32 RangeSqrToPlayer = Start.DistanceSqr(End);
		if (RangeSqrToPlayer < Sqr(100.0f))
		{
			SetActivationState(STATE_ACTIVE);
			if (DebugRender())
			{
				CStr Name = m_pGameObject->GetName();
				ConOut(Name + CStrF(" range activated %d",m_pGameObject->m_iObject));
			}
			return true;
		}

		//Check LOS from player
		int Tick = m_pGameObject->GetLastTickInClientPVS();
		if (Tick + 10 >= m_pServer->GetGameTick())
		{	//Have LOS, activate
			SetActivationState(STATE_ACTIVE);
			if (DebugRender())
			{
				CStr Name = m_pGameObject->GetName();
				ConOut(Name + CStrF(" LOS activated %d",m_pGameObject->m_iObject));
			}
			return true;
		};
	}
	else
	{
		//This bot has never been active.
		//A player should always be active. 
		if (IsPlayer(m_pGameObject->m_iObject))
		{
			m_ActivationState = STATE_ALWAYSACTIVE;
			return true;
		};

		//Activate bot if any living player (human or bot) comes within activation range.
		//Note that a no-clipping player will also trigger activation.
		//Bot may also be activated by certain events, such as when taking damage, or when receiving
		//a release or change behaviour impulse.
		CWObject_Game *pGame = m_pServer->Game_GetObject();
		for (int i = 0; i < pGame->Player_GetNum(); i++)
		{
			CWObject * pChar;
			CWObject_Interface_AI* pObjAI;

			if ( (pChar = pGame->Player_GetObject(i)) && 
			    (pObjAI = pChar->GetInterface_AI()) &&
		//		IsPlayer(pChar->m_iObject) &&   -- can't see why this is needed (it compares to the same list we're currently iterating)
		//		pObjAI->AI_IsAlive() &&	        -- disabled, since it treats 'noclip' as 'dead'
				(m_pGameObject->GetPosition().DistanceSqr(pChar->GetPosition()) < m_ActivateRangeSqr) )
			{
				SetActivationState(STATE_ACTIVE);
				return true;
			};
		};
	}

	// We can be deactivated
	return false;
};


//Checks if the bot should be deactivated
bool CAI_Core::OnDeactivationCheck()
{	
	//Only check every tenth frame
	if (m_Timer % 10 != 0)
		return false;

	//Never deactivate if always active
	if (m_ActivationState == STATE_ALWAYSACTIVE)
		return false;

	// If we're in a dialog we cannot deactivate
	if (m_iDialoguePartner)
	{
		return(false);
	}
	CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
	if (pChar)
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pChar);
		if (pCD && pCD->m_DialogueInstance.IsValid())
		{
			return(false);
		}
	}

	//Only deactivate if so suggested, we've been active more than the given activationtime 
	//(or we're at lower than full activity) and we're not in LOS of a player. Note that every 
	//time we're hurt or when we're otherwise explicitly activated, the activationtick is reset.
	if ((m_ActivationState == STATE_SUGGEST_DEACTIVATION)&&
		((GetAITick() > m_ActivationTick + m_ActivationTime)||(m_ActivityLevel < ACTIVITY_FULL)))
	{
		if (m_DestroyOnDeactivationRangeSqr <= 0.0f)
		{	// We don't care if AH is busy
			if (!m_AH.CanDeactivate())
			{
				return(false);
			}

			// Don't deactivate when running behaviours or waiting for behaviours to start
			if ((m_bBehaviourRunning)||(m_iPendingBehaviour != 0))
			{
				return(false);
			}
		}
		
		//Make sure we don't have LOS to a player
		int32 playerID = GetClosestPlayer();
		CAI_Core* pPlayerAI = GetAI(playerID);
		if (!pPlayerAI)
		{
			return(false);
		}

		//Check LOS from player
		CVec3Dfp32 Start = pPlayerAI->GetAimPosition();
		CVec3Dfp32 End = GetAimPosition();
		fp32 RangeSqrToPlayer = Start.DistanceSqr(End);
		int Tick = m_pGameObject->GetLastTickInClientPVS();
		if ((RangeSqrToPlayer < Sqr(100.0f))||(Tick + 10 >= GetAITick()))
		{	//Have LOS, don't deactivate
			return false;
		}
		else
		{	//LOS deactivated
			Debug_RenderWire(Start,End,0xff0000ff,1.0f);
		}

		//No LOS to any player, deactivate
		SetActivationState(STATE_DEACTIVATED);
		ResetPathSearch();
		DeactivateAnimgraph();
		if (DebugRender())
		{
			CStr Name = m_pGameObject->GetName();
			ConOut(Name + CStrF(" deactivated %d",m_pGameObject->m_iObject));
		}
		if ((m_DestroyOnDeactivationRangeSqr > 0.0f)&&(RangeSqrToPlayer > m_DestroyOnDeactivationRangeSqr))
		{
			m_DestroyTimer = 0;
			if (DebugRender())
			{
				CStr Name = m_pGameObject->GetName();
				ConOut(Name + CStrF(" deactivation destroyed %d",m_pGameObject->m_iObject));
			}
		}
		return true;
	}
	
	//Failed some criterion above, don't deactivate
	return false;
};


//Pause behavior-controlled bot. Scripting will still work
void CAI_Core::PauseBehaviour()
{
	m_PauseActions = true;
};

//Release bot with paused behaviour
void CAI_Core::ReleaseBehaviour()
{
	m_PauseActions = false;
};

void CAI_Core::PauseScripts()
{
	m_Script.SetPause(true);
};

void CAI_Core::ReleaseScripts()
{
	m_Script.SetPause(false);
};


void CAI_Core::SetActivityScore(int _Score)
{
	if (_Score > m_ActivityScore)
		m_ActivityScore = _Score;
};

void CAI_Core::SetActivityLevel(int _Level, bool _bForce)
{
	if (!m_pServer)
		return;

	if ((m_ActivityLevel != _Level) &&
		((_bForce || (GetAITick() > m_ActivityTick))))
	{
		//Change activity level and set activity tick anew
		m_ActivityLevel = _Level;

		int Tick = GetAITick();
		m_ActivityTick = RoundToInt(Tick + 10 + Random * 50);
		switch (_Level)
		{
		case ACTIVITY_FULL:
			m_ActivityTick = RoundToInt(Tick + 100 + Random * 100);
			break;
		case ACTIVITY_HIGH:
			m_ActivityTick = RoundToInt(Tick + 50 + Random * 50);
			break;
		case ACTIVITY_LOW:
			m_ActivityTick = RoundToInt(Tick + 20 + Random * 80);
			break;
		}
	}
};

void CAI_Core::OnActivityCheck()
{
	if (!m_pServer)
		return;

	//Set activity level every ten frames when past activity tick
	if ((GetAITick() > m_ActivityTick) &&
		(m_Timer % 10 == 0))
	{
		int ActivityScore = m_pAIResources->ms_ActivityCounter.GetScore();
		int Level;
		if (ActivityScore < 21)
			Level = ACTIVITY_FULL;
		else if (ActivityScore < 41)
			Level = ACTIVITY_HIGH;
		else if (ActivityScore < 61)
			Level = ACTIVITY_LOW;
		else 
			Level = ACTIVITY_NONE;

		//Player allies are always at one activity level higher than normal
		if (m_KB.IsPlayerAlly() && (Level < ACTIVITY_FULL))
			Level++;

		SetActivityLevel(Level);				
	}
}


//Calculate pathfinding prio
uint8 CAI_Core::GetPathPrio()
{
	//Only calculate new path prio if it's invalid, or every ten frames
	if ((m_PathPrio > CAI_Resource_Pathfinding::PRIO_MIN) && 
		(m_PathPrio < CAI_Resource_Pathfinding::PRIO_MAX) &&
		(m_Timer % 10))
		return m_PathPrio;

	//Calculate new path prio
	m_PathPrio = CAI_Resource_Pathfinding::PRIO_MIN;
	if (!IsValid(true) || !IsActive())
		return m_PathPrio;

	//Scripted bots should always have top prio, but I don't dare fix this now
	if (m_Script.IsValid())
	{
		m_PathPrio = CAI_Resource_Pathfinding::PRIO_MAX;
		return m_PathPrio;
	}

	//Calculate "camera score" (closeness to human player cameras)
	CWObject * pObj;
	CWObject_Interface_AI * pAgent;
	CVec3Dfp32 CamPos;
	int nCams = 0;
	fp32 DistSqr;
	fp32 CamScore = 0;
	CWObject_Game *pGame = m_pServer->Game_GetObject();
	for (int i = 0; i < pGame->Player_GetNum(); i++)
	{
		pObj = IsValidAgent(pGame->Player_GetObjectIndex(i));
		pAgent = (pObj) ? pObj->GetInterface_AI() : NULL;
		if (pAgent && !pAgent->AI_IsBot())
		{
			CamPos = pAgent->AI_GetLastCameraPos();
			nCams++;
			DistSqr = m_pGameObject->GetPosition().DistanceSqr(CamPos);
			if (DistSqr < 1000*1000)
			{
				CamScore += 1 - (DistSqr * (1.0f / 1000000.0f) );
			};
		};
	};
	if (nCams > 1)
		CamScore /= (fp32)nCams;

	//Get score from behaviour
	// *** m_PathPrio = Max((int)CAI_Resource_Pathfinding::PRIO_MIN, Min((int)CAI_Resource_Pathfinding::PRIO_MAX, (int)m_spBehaviour->GetPathPrio(CamScore)));
	m_PathPrio = (int)CAI_Resource_Pathfinding::PRIO_MAX;

	//DEBUG
	//OnSpeak(CStrF("Pathprio: %d", iRes));
	//if (iRes > 256)
	//	ConOut("High pathprio");
	//DEBUG

	//StopBM();
	return m_PathPrio;
};


//Forces the bots' body direction to align towards it's look direction 
//at the given fraction of the direction change needed 
void CAI_Core::ForceAlignBody(fp32 _Amount)
{
	fp32 BodyAngle;
	CWObject_Message Msg(OBJMSG_AIQUERY_GETBODYANGLEZ);
	Msg.m_pData = &BodyAngle;
	if (m_pGameObject && m_pServer &&
		m_pServer->Message_SendToObject(Msg, m_pGameObject->m_iObject))
	{
		//Force angle difference onto interval -0.5..0.5
		fp32 Diff = GetLookAngles()[2] - BodyAngle;
		Diff -= (int)Diff;
		if (Diff < -0.5f)
			Diff += 1;
		else if (Diff > 0.5f)
			Diff -= 1;

		BodyAngle = BodyAngle + Diff * _Amount;
		Msg = CWObject_Message(OBJMSG_CHAR_SETBODYANGLEZ);
		Msg.m_pData = &BodyAngle;
		m_pServer->Message_SendToObject(Msg, m_pGameObject->m_iObject);
	};
};

//Check if body is aligned with look direction, within a given fraction
bool CAI_Core::IsBodyAligned(fp32 _Epsilon)
{
	fp32 BodyAngle;
	CWObject_Message Msg(OBJMSG_AIQUERY_GETBODYANGLEZ);
	Msg.m_pData = &BodyAngle;
	if (m_pGameObject && m_pServer &&
		m_pServer->Message_SendToObject(Msg, m_pGameObject->m_iObject))
		return (M_Fabs(GetLookAngles()[2] - BodyAngle) <= _Epsilon);
	else
		return false;
};


//How long until an item or weapon can be activated?
int CAI_Core::Wait()
{
	if (!m_pGameObject)
		return -1;

	return Wait(m_pGameObject->m_iObject);
};
int CAI_Core::Wait(int _iObj)
{
	if (!m_pServer)
		return -1;

	return m_pGameObject->AI_GetRPGAttribute(AIQUERY_RPGATTRIBUTE_WAIT);
};


//Propel an engine path given distance etc. If given, result is the new position of the engine path.
void CAI_Core::PropelPath(int _iPath, fp32 _Distance, bool _bAllowStops, bool _bXYOnly, bool _bPropel, CMat4Dfp32 * _pRes)
{	
	if (m_pServer)
	{
		aint Flags = 0;
		if (_bAllowStops) { Flags |= CWObject_Engine_Path::EPFLAGS_ALLOW_STOPS;};
		if (_bXYOnly) { Flags |= CWObject_Engine_Path::EPFLAGS_XYONLY;};
		if (_bPropel) { Flags |= CWObject_Engine_Path::EPFLAGS_PROPEL;};

		CWObject_Message Propel(OBJMSG_ENGINEPATH_PROPELPATH);
		Propel.m_VecParam0[0] = _Distance;
		Propel.m_Param0 = Flags;
		Propel.m_pData = _pRes;
		m_pServer->Message_SendToObject(Propel, _iPath);
	}
}

//***TODO***Sets the time of _iPath to _Time, sending messagges if _bSendMessages is true, returns pos in _pRes if supplied
void CAI_Core::SetTimePath(int _iPath, fp32 _Time, bool _bSendMessages, CMat4Dfp32 * _pRes)
{	
	if (m_pServer)
	{
		CWObject_Message TimeMsg(OBJMSG_ENGINEPATH_SETTIME);
		TimeMsg.m_VecParam0[0] = _Time;
		TimeMsg.m_Param0 = _bSendMessages; 
		TimeMsg.m_pData = _pRes;
		m_pServer->Message_SendToObject(TimeMsg, _iPath);
	}
}

// Returns the path
// *** TODO: Rewrite with desiatnce instead of time, or add params to PropelPath ***
bool CAI_Core::GetTimeAheadPath(int _iPath, fp32 _TimeAhead, CMat4Dfp32 * _pRes)
{	
	if ((m_pServer)&&(_pRes))
	{
		CMTime Time;
		CWObject_Message TimeMsg(OBJMSG_HOOK_GETTIME);
		TimeMsg.m_pData = &Time;
		TimeMsg.m_DataSize = sizeof(Time);
		if (m_pServer->Message_SendToObject(TimeMsg, _iPath))
		{	// Time now holds ep current time
			Time += CMTime::CreateFromSeconds(_TimeAhead);
			m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (aint)(&Time), (aint)(_pRes), GetObjectID()), _iPath);
			return true;
		}
	}
	return false;
}


void CAI_Core::DebugDrawPath(int32 _iPath, int32 _Steps, fp32 _Duration)
{
#ifndef M_RTM

	if ((!_iPath)||(_Steps <= 0))
	{
		return;
	}

	if ((DebugRender())&&(m_pServer))
	{	// Get ep duration
		fp32 Duration;
		CWObject_Message Msg(OBJMSG_HOOK_GETDURATION);
		Msg.m_Param0 = 0;
		Msg.m_Param1 = (aint)&Duration;
		Msg.m_DataSize = sizeof(fp32);
		m_pServer->Message_SendToObject(Msg,_iPath);

		CMTime EpTime;
		Msg.m_Msg = OBJMSG_HOOK_GETTIME;
		Msg.m_pData = &EpTime;
		Msg.m_DataSize = sizeof(EpTime);
		m_pServer->Message_SendToObject(Msg, _iPath);
		fp32 fEpTime = EpTime.GetTime();

		fp32 StepLength = Duration / _Steps;
		fp32 CurTime = 0.0f;
		CMat4Dfp32 PrevMat,CurMat;
		CurMat.Unit();
		CVec3Dfp32 Off = CVec3Dfp32(0.0f,0.0f,4.0f);
		for (int i = 0; i < _Steps; i++)
		{	// Draw the path one
			PrevMat = CurMat;
			CMTime Time;
			Msg.m_Msg = OBJMSG_HOOK_GETTIME;
			Msg.m_pData = &Time;
			Msg.m_DataSize = sizeof(Time);
			CurTime += StepLength;
			Time += CMTime::CreateFromSeconds(CurTime);
			m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (aint)(&Time),(aint)(&CurMat),GetObjectID()), _iPath);
			if (i > 0)
			{
				if (CurTime >= fEpTime)
				{
					Debug_RenderWire(PrevMat.GetRow(3)+Off,CurMat.GetRow(3)+Off,kColorRed,_Duration);
				}
				else
				{
					Debug_RenderWire(PrevMat.GetRow(3)+Off,CurMat.GetRow(3)+Off,kColorBlue,_Duration);
				}
			}
		}
	}
#endif	
};

//Is given agent using a weapon?
bool CAI_Core::UsingWeapon(int _iObj)
{
	//Just assume he's using weapon if there's a wait
	return Wait(_iObj) != 0;
};

bool CAI_Core::IsTeammember(int _iObj)
{
	if ((!m_pServer)||(!m_pGameObject))
	{
		return false;
	}

	if (m_pGameObject->m_iObject == _iObj)
	{
		return(false);
	}

	uint16 liTeams[16];
	uint nTeams = m_pGameObject->AI_GetTeams(liTeams, sizeof_buffer(liTeams));
	for (int i = 0; i < nTeams; i++)
	{
		int iTeam = liTeams[i];
		int Comm = (m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_TEAM_COMMUNICATIONS),iTeam));
		if (Comm == CWObject_Team::NONE)
		{
			continue;
		}
		CWObject_Message IsTeamMember(OBJMSG_TEAM_BELONGTOTEAM, iTeam);
		if (m_pServer->Message_SendToObject(IsTeamMember,_iObj))
		{
			return(true);
		}
	}

	return(false);
};

// Returns the index of the nearest teammember to us
// If _bComm is true only teams with voice or direct communications are considered
// _RangeSqr will hold the squared range to the returned member 
int CAI_Core::GetClosestTeammember(bool _bComm,int _iIgnore,fp32* _RangeSqr)
{
	if (!m_pServer)
		return 0;

	fp32 BestRangeSqr = _FP32_MAX;
	int iBestObj = 0;

	CSimpleIntList* plAgents = &(m_pAIResources->m_KBGlobals.ms_lAgents);
	uint16 liTeams[16];
	uint nTeams = m_pGameObject->AI_GetTeams(liTeams, sizeof_buffer(liTeams));
	for (int i = 0; i < nTeams; i++)
	{
		int iTeam = liTeams[i];
		if (_bComm)
		{
			int Comm = (m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_TEAM_COMMUNICATIONS),iTeam));
			if (Comm == CWObject_Team::NONE)
			{
				continue;
			}
		}
		CWObject_Message IsTeamMember(OBJMSG_TEAM_BELONGTOTEAM, iTeam);
		for (int i = 0; i < plAgents->Length(); i++)
		{
			if (plAgents->IsValid(i))
			{
				int iCurObj = plAgents->Get(i);
				if ((_iIgnore != iCurObj)&& //Ignore _iIgnore
					(m_pGameObject->m_iObject != iCurObj)&& //Don't communicate with self
					m_pServer->Message_SendToObject(IsTeamMember,iCurObj))
				{	//Team member found
					// Is he alive? Conscious? Spawned?
					CAI_Core* pMemberAI = GetAI(iCurObj);
					if ((pMemberAI)&&(pMemberAI->m_pGameObject)&&(pMemberAI->m_pGameObject->AI_IsSpawned())&&(pMemberAI->m_pGameObject->AI_IsAlive()))
					{
						fp32 RangeSqr = SqrDistanceToUs(iCurObj);
						if (RangeSqr < BestRangeSqr)
						{
							BestRangeSqr = RangeSqr;
							iBestObj = iCurObj;
						}
					}
				}
			}
		}
	}

	if ((iBestObj)&&(BestRangeSqr != _FP32_MAX))
	{
		*_RangeSqr = BestRangeSqr;
	}

	return(iBestObj);
}

//Get best security clearance of any team given agent belongs to
int CAI_Core::GetSecurityClearance(int _iObj)
{
	if (!m_pServer)
		return 0;

	int BestLevel = 0;
	CWObject * pObj = m_pServer->Object_Get(_iObj);
	CWObject_Interface_AI * pAIObj = (pObj) ? pObj->GetInterface_AI() : NULL;
	if (!pAIObj)
		return 0;
	uint16 liTeams[16];
	uint nTeams = m_pGameObject->AI_GetTeams(liTeams, sizeof_buffer(liTeams));
	for (int i = 0; i < nTeams; i++)
	{
		int iTeam = liTeams[i];
		int Level = (m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_TEAM_CLEARANCE), iTeam));
		if (Level > BestLevel)
		{
			BestLevel = Level;
		}
	}

	// Ask this character if he has any special items that give hm higher clearance level
	int cl = m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETGIVENCLEARANCELEVEL), _iObj);
	if (cl > BestLevel)
		BestLevel = cl;

	return BestLevel;
};

//Get positions/objects squared distance to us
fp32 CAI_Core::GridSqrDistanceToUs(const CVec3Dfp32& _Pos)
{
	if ((m_pGameObject)&&(m_PathFinder.GridPF()))
	{
		CVec3Dfp32 BasePos = GetBasePos();
		CVec3Dfp32 Off = GetUp() * m_PathFinder.GridPF()->GetBlockNav()->UnitsPerCell();
		fp32 DistSqr = Min3(_Pos.DistanceSqr(BasePos),_Pos.DistanceSqr(BasePos+Off),_Pos.DistanceSqr(BasePos-Off));
		return(DistSqr);
	}
	else
	{
		return(_FP32_MAX);
	}
};

//Get positions/objects squared distance to us
fp32 CAI_Core::SqrDistanceToUs(const CVec3Dfp32& _Pos)
{
	if (m_pGameObject)
	{
		return(_Pos.DistanceSqr(m_pGameObject->GetPosition()));
	}
	else
	{
		return(_FP32_MAX);
	} 
};

fp32 CAI_Core::SqrDistanceToUs(CWObject * _pObj)
{
	if (m_pGameObject)
	{
		return _pObj->GetPosition().DistanceSqr(m_pGameObject->GetPosition());
	}
	else
	{
		return(_FP32_MAX);
	}
};

fp32 CAI_Core::SqrDistanceToUs(int _iObj)
{
	CWObject* pObj = m_pServer->Object_Get(_iObj);
	if (pObj)
	{
		return(SqrDistanceToUs(pObj));
	}
	else
	{
		return(_FP32_MAX);
	}
};

// Projects _Pos onto the AI:s baseplane
// (baseplane of basepos and up normal)
CVec3Dfp32 CAI_Core::Proj2Base(CVec3Dfp32 _Pos)
{
	CVec3Dfp32 rPos;
	if (m_bWallClimb)
	{
		CVec3Dfp32 Up = m_CurBaseMat.GetRow(2);
		rPos = _Pos - Up * (Up * (_Pos - m_CurBaseMat.GetRow(3)));
	}
	else
	{
		rPos = _Pos;
		rPos[2] = m_CurBaseMat.GetRow(3)[2];
	}

	return(rPos);
};

fp32 CAI_Core::SqrFlatDistance(CVec3Dfp32 _Pos)
{
	return(m_CurBaseMat.GetRow(3).DistanceSqr(Proj2Base(_Pos)));
};

//Get AI resource handler
CAI_ResourceHandler * CAI_Core::GetAIResources()
{
	CWObject_Message Msg(OBJMSG_GAME_GETAIRESOURCES);
	return (CAI_ResourceHandler *)m_pServer->Message_SendToObject(Msg, m_pServer->Game_GetObjectIndex());
};


//Get scenepoint manager
CWO_ScenePointManager * CAI_Core::GetScenePointManager()
{
	CWObject_Message Msg(OBJMSG_GAME_GETSCENEPOINTMANAGER);
	return (CWO_ScenePointManager *)m_pServer->Message_SendToObject(Msg, m_pServer->Game_GetObjectIndex());
};


//Is the AI valid, i.e. does it have a server, character, character RPG object, behaviour, personality etc.
//If so specified the AI is not considered valid if it's dead, noclipping or immobile, also.
bool CAI_Core::IsValid(bool _bStrict)
{
	return (m_pServer && m_pGameObject && 
		(!_bStrict || m_pGameObject->AI_IsAlive()));
};

// Returns true if the characters is alive and not stunned nor oblivious
bool CAI_Core::IsConscious()
{
	return ((m_pServer)&&(m_pGameObject)&&(m_StunTimeout == 0)&&(m_pGameObject->AI_IsAlive()));
};

// If _StunTime == -1 the undead will be stunned forever
// If _StunTime > 0 the undead will be stunned _StunTicks ticks
// If _StunTime == 0 the undead will wake up next tick
// Returns true if the call was valid for the bot
bool CAI_Core::SetUndeadStun(int _StunTicks, bool _bDirect)
{
	if (m_UndeadDieBehaviour)
	{
		int Flags = BEHAVIOUR_FLAGS_LOOP;
		if (_bDirect)
		{
			Flags |= BEHAVIOUR_FLAGS_DIRECT;
		}
		if (_StunTicks == -1)
		{
			if (m_StunTimeout == 0)
			{
				m_bBehaviourRunning = false;
				m_BehaviourStopIssued = BEHAVIOUR_STOP_NONE;
				CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
				CWAG2I_Context Context(m_pGameObject,m_pServer,pCD->m_GameTime);
				m_DeviceStance.SetTargetInFOV(false);
				m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
				pCD->m_AnimGraph2.ForceStance(&Context);
				SetWantedBehaviour2(m_UndeadDieBehaviour,CAI_Action::PRIO_FORCED,Flags,0,NULL);
			}
			m_StunTimeout = -1;
		}
		else if (_StunTicks == 0)
		{
			if (m_StunTimeout != 0)
			{	// This will be handled next OnRefresh()
				m_StunTimeout = 1;
			}
		}
		else
		{
			if (m_StunTimeout == 0)
			{
				m_bBehaviourRunning = false;
				m_BehaviourStopIssued = BEHAVIOUR_STOP_NONE;
				CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
				CWAG2I_Context Context(m_pGameObject,m_pServer,pCD->m_GameTime);
				m_DeviceStance.SetTargetInFOV(false);
				m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
				pCD->m_AnimGraph2.ForceStance(&Context);
				if (SetWantedBehaviour2(m_UndeadDieBehaviour,CAI_Action::PRIO_FORCED,Flags,_StunTicks,NULL))
				{
					m_StunTimeout = _StunTicks;
				}
			}
		}	
		return(true);
	}

	return(false);
}

bool CAI_Core::SetAgonyStun()
{
	if ((m_AgonyBehaviour)&&(m_iCurBehaviour != m_AgonyBehaviour))
	{
		SetWantedBehaviour2(m_AgonyBehaviour,CAI_Action::PRIO_FORCED,0,0,NULL);
		m_StunTimeout = -1;
		return(true);
	}

	return(false);
}

//Updates the bot one frame. If the _bPassive flag is true, no actions are considered or generated, 
//but information is gathered.
void CAI_Core::OnRefresh(bool _bPassive)
{
	MSCOPE(CAI_Core::OnRefresh, CHARACTER);
	if(!m_pGameObject || !m_pServer)
		//Lacking either game object or server, no point refreshing
		return;

	if (!m_pAIResources)
	{
		m_pAIResources = GetAIResources();
		if (!m_pAIResources)
		{	// Lacking global resources we bail
			return;
		}
	}

	const int PhysType = CWObject_Character::Char_GetPhysType(m_pGameObject);

#ifndef PLATFORM_CONSOLE
#ifndef M_RTM
	//	try 
	{
		//DebugRender() = true;
		// Auto-activate debug rendering

		m_DebugInfo = "";//DEBUG
		OnDebugStuff();//DEBUG :)
		//DEBUG
		TStart(Time);
		//ConOut(CStrF("ACTIVITY: %d", ms_ActivityCounter.GetScore()));//DEBUG
#endif
#endif

		//Set some "character" data 
		m_bWeightless = (m_pGameObject->m_ClientFlags & PLAYER_CLIENTFLAGS_WEIGHTLESS) != 0;
		//Reset activity
		m_ActivityScore = CAI_ActivityCounter::INACTIVE;

		//Don't do anything if the AI is invalid
		// Store old matrices and get new ones
		OnRefreshPosMats();
		OnRefreshClimb();
		OnRefreshBehaviours();
		RefreshTicks();
		if (IsPlayer())
		{
			OnRefreshDarkness();
		}
		// *** RefreshCBExits(); // Still needed ? (superseded by OnRefreshBehaviours())

		if (!IsValid(true))
		{	//Did we just die?
			if  ((m_bWasAlive)&&(m_pGameObject))
			{
				OnDie();
			};

			//Did we just become invalid?
			if (m_bWasValid)
			{//Destroy and initialize the character 
				Destroy();
				m_bWasValid = false;
				// Init(m_pGameObject, m_pServer); //Init sets m_bWasValid to false...
			};

			// Draw corpse light
			if (m_bNextOfKin)
			{
				GetCorpseLightIntensity(RoundToInt(5.0f * GetAITicksPerSecond()));
			}
#ifndef M_RTM
			DebugDrawLight();
#endif
			return;
		};

		//StopBM();

		//Is this the first refresh, then do some "enter game" stuff
		m_bCanDeactivateAG = -1;
		if (m_bFirstRefresh)
		{
	
#ifndef M_RTM
			// An simple way to set the debugtarget at compiletime, for tricky targets that bug out at spawn
			if ((m_pAIResources)&&(m_pAIResources->ms_iDebugRenderTarget == GetObjectID()))
			{	// (re)set debugtarget to ensure that AF controls are properly drawn
				m_pAIResources->ms_bDebugRender = true;
				SetDebugTarget(GetObjectID());
			}
#endif

			//Initialize knowledge base
			m_KB.Init();

			//Raise OnSpawn event
			m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPAWN,0);

			m_bWasAlive = true;
			m_bWasValid = true;

			// Players are always active
			if (IsPlayer())
			{
				m_ActivationState = STATE_ALWAYSACTIVE;
			}
			else
			{
				OnActivationCheck();
				//Set the initial behaviour
				SetActionFromKeys();
			}

			// Store the spawned position/look if we don't already have one
			if (INVALID_POS(m_HoldPosition))
				m_HoldPosition = m_pGameObject->GetPosition();
			if (INVALID_POS(m_HoldLookDir))
				m_HoldLookDir = GetLookDir();

			

			//Send a "I'm aliiive!" message to all characters in char list
			SendMessage(MSGMODE_ALL, CWObject_Message(OBJMSG_IM_ALIIIVE, 0, 0, m_pGameObject->m_iObject));
			m_pAIResources->ms_Gamestyle = m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETGAMESTYLE), m_pServer->Game_GetObjectIndex());
			m_pAIResources->ms_GameDifficulty = m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETDIFFICULTYLEVEL), m_pServer->Game_GetObjectIndex());
			m_DeviceSound.SetupDialogueIndices();
			m_DeviceFacial.SetFacialGroup(m_iFacialGroup);

			// Unlimited ammo for AI. Note that this only gives unlimited ammo to the currently equipped item.
			// To enable AIs to have several weapons we should handle this for all initial weapons and whenever we add a weapon.
			if ((!IsPlayer())&&(!(m_UseFlags & USE_DRAWAMMO)))
			{
				m_Weapon.GetWieldedArmsClass();
				CAI_Weapon* pWeapon = m_Weapon.GetWielded();
				if (pWeapon)
				{
					// AI have unlimited ammo
					CRPG_Object_Item* pItem = pWeapon->GetItem();
					if (pItem)
					{
						pItem->m_Flags2 |= RPG_ITEM_FLAGS2_NOAMMODRAW;
					}
				}
			}
			int shootPeriod = 20;
			if (m_pAIResources->ms_GameDifficulty == DIFFICULTYLEVEL_HARD)
			{
				shootPeriod = 5;
			}
			else if (m_pAIResources->ms_GameDifficulty == DIFFICULTYLEVEL_NORMAL)
			{
				shootPeriod = 10;
			}
			else	// Easy and Commentary
			{
				shootPeriod = 20;
				
			}
			// *** Shotgun firerate hack ***
			if ((!IsPlayer())&&(m_Weapon.GetWieldedWeaponType() == CAI_WeaponHandler::AI_WEAPONTYPE_SHOTGUN))
			{
				shootPeriod += 10;
			}
			// ***
			m_PressDuration = 0;
			m_ShootPeriod = shootPeriod;
			m_DeviceWeapon.SetPressDuration(m_PressDuration);
			m_DeviceWeapon.SetPeriod(m_ShootPeriod);
			m_DeviceLook.UpdateNextLook();
			// Reflurg the frombotzer!
			m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_IDLE);
			if (m_UseLightmeter >= 0)
			{
				m_KB.Global_SetCareAboutBrokenLights();
			}

			// Tell animgraph what move variant we use
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
			if (pCD)
			{
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_MOVEVARIATION,m_iMoveVariant);
			}

			if (m_bWallClimb)
			{
				m_pGameObject->AI_EnableWallClimbing(true);
			}
			else
			{
				m_pGameObject->AI_EnableWallClimbing(false);
			}
			if (m_StunTimeout == -1)
			{
				SetUndeadStun(m_StunTimeout,false);
			}
			// End of 'if (m_bFirstRefresh)'
		};

		bool bIsBraindead = false;
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
		if ((pCD)&&(pCD->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_ISSLEEPING))||(m_Script.IsPaused())||(!m_pGameObject->AI_IsAlive()))
		{
			bIsBraindead = true;
		}

		// Orders
		if (m_bServerTickAdvanced)
		{
			if (m_OrderTicksRemaining > 0)
			{
				m_OrderTicksRemaining--;
			}
			if (m_OrderTicksRemaining == 0)
			{
				m_Order = ORDER_NONE;
			}
		}

		if (m_bServerTickAdvanced)
		{
			if (m_StunTimeout)
			{
				if (m_iMountParam)
				{	// Mounted?
					CWObject_Message Msg(OBJMSG_CHAR_MOUNT);
					Msg.m_pData = NULL;
					m_pServer->Message_SendToObject(Msg, GetObjectID());
					m_DeviceWeapon.SetPressDuration(m_PressDuration);
					m_DeviceWeapon.SetPeriod(m_ShootPeriod);
					m_Script.SetAction(CAI_ScriptHandler::LOOK_AT, 0);
					m_iMountParam = 0;
				}
				ResetPathSearch();
				ResetStuckInfo();
				CAI_AgentInfo* pTarget = m_KB.GetAgentInfo(m_AH.m_iTarget);
				if (pTarget)
				{
					pTarget->Touch();
					pTarget->TouchRelation();
					pTarget->TouchLastSpotted();
				}
				if (m_StunTimeout > 0)	{m_StunTimeout--;}
				if ((m_StunTimeout == 0)&&(!bIsBraindead))
				{	// If we can stand at our pos we stop, otherwise we continue being stunned
					CVec3Dfp32 Pos = m_PathFinder.GetPathPosition(GetBasePos(),4,4);
					if (VALID_POS(Pos))
					{
						StopBehaviour(BEHAVIOUR_STOP_NORMAL,CAI_Action::PRIO_FORCED);
						if (pTarget)
						{
							pTarget->OnRefresh();
							if (pTarget->GetCurAwareness() < CAI_AgentInfo::DETECTED)
							{
								pTarget->SetAwareness(CAI_AgentInfo::DETECTED,true,true);
							}
							OnTrackAgent(pTarget,6,false, false);
						}
					}
					else
					{	// Stunned for another 4 secs or so
						m_StunTimeout = 80;
					}
				}
			}
			/*
			if ((bIsBraindead)&&(m_StunTimeout >= 0))
			{
				m_StunTimeout = 1;
			}
			*/

#ifndef M_RTM
			if (DebugTarget())
			{
				bool wtf = true;
			}
#endif
		}

		//The AI is ready for (or in the middle of) action!
		//Refresh knowledge base (only every 10th frame for passive refresh)
		if (m_bServerTickAdvanced)
		{
			if ((IsActive())&&(IsAlive(PhysType)))
			{
				m_CurPriorityClass = CAI_Action::PRIO_MIN;
				if ((m_Timer % AI_KB_REFRESH_PERIOD == 0)||(m_bFirstRefresh))
				{
					if ((IsPlayer())&&(m_Timer % (2 * AI_KB_REFRESH_PERIOD) == 0))
					{	// Check changes in gamestyle periodically
						m_pAIResources->ms_Gamestyle = m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETGAMESTYLE), m_pServer->Game_GetObjectIndex());
					}

					
					if (IsPlayer())
					{
						m_KB.OnRefresh();
					}
					else if (!_bPassive)// Not the player
					{
						if ((!bIsBraindead)&&(!m_StunTimeout))
						{
							m_KB.OnRefresh();
						}
						else
						{
							// Set awareness of player to no more than DETECTED (ie not updating with player moves)
							CAI_AgentInfo* pPlayerInfo = m_KB.GetAgentInfo(GetClosestPlayer());
							if ((pPlayerInfo)&&(pPlayerInfo->GetCurAwareness() >= CAI_AgentInfo::NOTICED))
							{
								pPlayerInfo->SetAwareness(CAI_AgentInfo::NOTICED,false,true);	
							}
						}
					}
					m_Weapon.UpdateWieldedArmsClass();
				}
				else if ((m_UseLightmeter > -1)&&((m_Timer+2) % 5 == 0))
				{	// Measure the light impinging on the player twice a second
					if (IsPlayer())
					{
						MeasureLightIntensity(true);
					}
					else
					{	// We're not a player and should at least check for broken lights
						if (m_UseLightmeter == 2)
						{	// Full light measurers every 2 seconds
							if ((m_Timer+2) % 40 == 0)
							{
								MeasureLightIntensity(true);
								if (m_bFlashlight)
								{
									HandleFlashlight(40);
								}
							}
						}
						else if (m_UseLightmeter == 1)
						{	// Basic light measurers every second
							if ((m_Timer+2) % 20 == 0)
							{
								MeasureLightIntensity(false);
								if (m_bFlashlight)
								{
									HandleFlashlight(20);
								}
							}
						}

						if (m_UseLightmeter >= 0)
						{	// Detect broken lights every 2 seconds or so
							if ((m_Timer+2) % 40 == 0)
							{
								HandleBrokenLights(40);
							}
						}
					}
				}

				// Draw lightmeter
	#ifndef M_RTM
				DebugDrawLight();
	#endif

				// Check m_iCrowdFocus and m_iDialoguePartner for death
				if (m_iDialoguePartner)
				{
					CWObject* pPartner = m_pServer->Object_Get(m_iDialoguePartner);
					if (pPartner)
					{
						CWObject_Interface_AI* pPartnerInterfaceAI = pPartner->GetInterface_AI();
						if ((pPartnerInterfaceAI)&&(!pPartnerInterfaceAI->AI_IsAlive()))
						{
							m_iDialoguePartner = 0;
							m_pGameObject->AI_SetEyeLookDir();
						}
					}
				}
				if (m_iCrowdFocus)
				{
					CWObject* pPartner = m_pServer->Object_Get(m_iCrowdFocus);
					if (pPartner)
					{
						CWObject_Interface_AI* pPartnerInterfaceAI = pPartner->GetInterface_AI();
						if ((pPartnerInterfaceAI)&&(!pPartnerInterfaceAI->AI_IsAlive()))
						{
							m_iCrowdFocus = 0;
						}
					}
				}

				// Don't do this at the same time as m_KB.OnRefresh() above
				if ((m_Timer + 7) % 20 == 0)
				{
					GetStealthTension(true);
				}
			}
		}

		//The below stuff is for non-passive bots only
		if (m_bServerTickAdvanced)
		{
			if (!_bPassive)
			{
				if (!IsPlayer())
				{
					OnActivityCheck();
					//Should we activate inactive bot? Only check for activation every ten frames.
					if (OnActivationCheck())
						//Check if we perhaps should deactivate active bot
						OnDeactivationCheck();
				}

				if (INVALID_POS(m_HoldLookDir))
				{	
					m_HoldLookDir = GetLookDir();
				}

				//Refresh devices
				for (int i = 0; i < CAI_Device::NUM_DEVICE; i++)
					m_lDevices[i]->OnRefresh();

				//Lock devices that should be unavailable 
				if (m_pGameObject->AI_GetPhysFlags() & PLAYER_PHYSFLAGS_IMMOBILE)
					m_DeviceMove.Lock();

				//Refresh item handlers
				if (!IsPlayer())
				{
					m_Weapon.OnRefresh();
					m_Item.OnRefresh();
					m_EventHandler.OnRefresh();
					if ((m_StunTimeout == 0)&&(!bIsBraindead))
					{
						OnTakeAction();
					}

					// Maybe we should raise the alarm now (if we're still alive)
					if ((m_PendingAlarmTime > -1)&&
						(IsAlive(PhysType))&&
						(m_KB.GetAlertness() >= CAI_KnowledgeBase::ALERTNESS_DROWSY)&&
						(GetAITick() >= m_PendingAlarmTime))
					{
						m_KB.AddLoudness(100);	// We're screamin some kind of alarm - right!
						SendMessage(CAI_Core::MSGMODE_ALARM,m_PendingAlarmMsg);
						if (m_PendingAlarmMsg.m_Msg == OBJMSG_TEAM_ALARM)
						{
							m_EventHandler.RaiseEvent(CAI_EventHandler::ON_ALARM,m_PendingAlarmMsg.m_Param0);
						}

						m_PendingAlarmMsg.m_Msg = 0; 
						m_PendingAlarmTime = -1;
					}
				}
				if ((!m_bBehaviourRunning)&&(!m_iCurBehaviour))
				{
					OnRefreshDestination();
				}
				//PostActionsRefresh devices
				for (int i = 0; i < CAI_Device::NUM_DEVICE; i++)
					m_lDevices[i]->OnPostActionsRefresh();

				// Bots cannot move while crouching in P5
				if ((m_DeviceStance.IsCrouching())&&(m_DeviceMove.GetNextMove().LengthSqr() >= Sqr(0.15f)))
				{
	#ifndef M_RTM
					CStr name = m_pGameObject->GetName();
					ConOutL(name + CStr(" forced stop Crouch"));
	#endif
					m_DeviceStance.Crouch(false);
				}

	#ifndef M_RTM
				if (DebugTarget())
				{
					m_PathFinder.Debug_Render(m_pServer);
					m_Path.Debug_Render(m_pGameObject->GetPosition(), m_pServer);
				}
	#endif
				//Activity is at least idle
				SetActivityScore(CAI_ActivityCounter::IDLE);
			}
		}

		// Handle player look
		if (IsPlayer())
		{
			HandleDialoguePartner();
		}
		
		if ((m_bCanDeactivateAG == 1)&&(m_bBehaviourRunning == false))
		{
			// Someone has called DeactivateAnimgraph() and none has called ActivateAnimgraph()
			SetAnimgraphDeactivation(true);
		}
		else if (m_bCanDeactivateAG == 0)
		{	// Someone has called ActivateAnimgraph()
			SetAnimgraphDeactivation(false);
		}

		m_bFirstRefresh = false;

		//Reset previous speed
		m_PrevSpeed = -1.0f;

		//Destroy timer stuff
		if (m_bServerTickAdvanced)
		{
			if (m_DestroyTimer == 0)
			{
				m_EventHandler.RaiseEvent(CAI_EventHandler::ON_DIE,0);
				Destroy();
				m_pServer->Object_Destroy(m_pGameObject->m_iObject);
			}
			else if (m_DestroyTimer > 0)
				m_DestroyTimer--;
			//Life timer stuff
			if ((m_LifeTimer >= 0) &&
				(m_LifeDrainAmount != 0))
			{
				if (m_LifeTimer == 0)
				{
					//Lose (or gain) some health at specified intervals
					if (m_Timer % Max(1, m_LifeDrainInterval) == 0)
					{
						if (m_LifeDrainAmount > 0)
						{
							//Apply damage and die if necessary
							m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_APPLYDAMAGE, m_LifeDrainAmount), m_pGameObject->m_iObject);
							if (Health(m_pGameObject->m_iObject) <= 0)
								m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_PHYSICS_KILL, DAMAGETYPE_UNDEFINED), m_pGameObject->m_iObject);
						}
						else
							m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADDHEALTH, -m_LifeDrainAmount), m_pGameObject->m_iObject);
					};
				}
				else
					m_LifeTimer--; 
			}
		}

#ifndef PLATFORM_CONSOLE
#ifndef M_RTM
		OnDebugStuff2();//DEBUG :)
#endif
#endif

		//Report activity score if not player
		if (!IsPlayer(m_pGameObject->m_iObject))
			m_pAIResources->ms_ActivityCounter.Report(m_ActivityScore, m_pAIResources->GetAITick());	

#ifndef PLATFORM_CONSOLE
#ifndef M_RTM
		//StopBM();

		//DEBUG
		//Benchmarking stuff (DEBUG)
		TStop(Time);
		//T_Stop(T); //DEBUG
		//AITime += T;//DEBUG
		//Time = T;
		//if (AITime > MaxTime)//DEBUG
		//	MaxTime = AITime;//DEBUG
		//DEBUG
	}
	//catch(...)
	{
		//	ConOutL("Unhandled AI error! Go whack Anders over the head with appropriate blunt object.");
	};
#endif
#endif
};


//Helper: creates a CVec3Dfp32 from a string containing three numbers separated by the given char, or CVec3Dfp(0) otherwise
CVec3Dfp32 CAI_Core::GetVectorFromString(CStr _Str, char _cSeparator)
{
	int iStart = 0;
	int iEnd = 0;
	int iLen = _Str.Len();
	CVec3Dfp32 Res;

	for (int i = 0; i < 3; i++)
	{
		//Scan to separator
		while ((iEnd < iLen) && (_Str[iEnd] != _cSeparator)) iEnd++;
		//Get float (I hope this is zero for 0 length strings, or strings with no numbers in them)
		if (iEnd < iLen)
			Res[i] = (_Str.Copy(iStart, iEnd - iStart)).Val_fp64();
		else
			Res[i] = 0;
		//Move on to next float
		iEnd++;
		iStart = iEnd;
	};
	return Res;
};


//Helper to OnImpulse; gets single random object to use as parameter to some impulses. Will prefer valid 
//characters before other objects.
CWObject* CAI_Core::GetSingleTarget(int _iObject, CStr _pName, int _iSender)
{
	CWObject * pObject = NULL;

	// AIImpulse targets doesn't default to $this, they default to 0 ie no object
	if (_pName.CompareNoCase("") == 0)
	{
		return(NULL);
	}

	//If we have reserved name always use the corresponding object
	int iSpecialTarget = CWO_SimpleMessage::ResolveSpecialTargetName(_pName);
	if (iSpecialTarget >= 0)
	{
		if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ACTIVATOR)
		{
			//Use activator, which should be the supplied onject ID
			return m_pServer->Object_Get(_iObject);
		}
		else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_THIS)
		{
			//Use sender
			return m_pServer->Object_Get(_iSender);
		}
		else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_PLAYER)
		{
			CWObject* pMinChar = m_pServer->Object_Get(GetClosestPlayer());
			return pMinChar;
		}
		else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ALLPLAYERS)
		{
			//Get random player
			CWObject * pChar;
			CWObject_Game *pGame = m_pServer->Game_GetObject();
			int iRnd = TruncToInt(Random * 0.999f * pGame->Player_GetNum());
			for (int i = 0; i < pGame->Player_GetNum(); i++)
			{
				if ((pChar = pGame->Player_GetObject((i + iRnd) % pGame->Player_GetNum())))
				{
					return pChar;
				};
			};
			return NULL;
		}
		else
		{
			//Can't handle this name
			return NULL;
		};
	}
	//Otherwise use target name if this is valid
	else if (_pName.Str())
	{
		//Find all objects with the given name
		const int16* piObj;
		int nObj;

		//Set up selection
		TSelection<CSelection::LARGE_BUFFER> Selection;

		//Add objects
		m_pServer->Selection_AddTarget(Selection, _pName);
		nObj = m_pServer->Selection_Get(Selection, &piObj);

		//Shuffle through objects until best random candidate is found
		if (nObj == 0)
			return NULL;
		else if (nObj == 1)
			return m_pServer->Object_Get(piObj[0]);
		else
		{
			//More than one object, settle for the first valid character that is found by a randomized
			//search or the first valid object otherwise

			//Set up check array
			TArray<bool> lbChecked;
			lbChecked.QuickSetLen(nObj);
			int i;
			for (i = 0; i < nObj; i++) lbChecked[i] = false;

			//Start searching
			CWObject * pChar = NULL;
			int Rnd;
			int j = 0;
			for (i = nObj; i > 0; i--)
			{
				//Find random non-checked object index
				Rnd = TruncToInt(Random * i * 0.9999f);
				j = 0;
				while (j <= Rnd)
				{
					if (lbChecked[j])
						Rnd++;
					j++;
				};

				//Check if we've got a valid character
				if ((pChar = IsValidAgent(piObj[Rnd])))
				{
					//First valid character found, we're done!
					return pChar;
				};

				//Should we get the object?
				if (!pObject)
					pObject = m_pServer->Object_Get(piObj[Rnd]);
			};
			return pObject;
		};
	}
	//Try given object ID
	else 
		return m_pServer->Object_Get(_iObject);
};


//Get index of closest human player
int CAI_Core::GetClosestPlayer()
{
	//Find closest human player
	int iObj = 0;
	fp32 DistSqr;
	fp32 MinDistSqr = _FP32_MAX;
	CWObject* pChar;
	CWObject_Game *pGame = m_pServer->Game_GetObject();
	for (int i = 0; i < pGame->Player_GetNum(); i++)
	{
		if ((pChar = pGame->Player_GetObject(i)) &&
			!IsBot(pChar->m_iObject) &&
			((DistSqr = m_pGameObject->GetPosition().DistanceSqr(pChar->GetPosition())) < MinDistSqr))
		{
			iObj = pChar->m_iObject;
			MinDistSqr = DistSqr;
		};
	};
	return iObj;
};

//Get index of closest teammate with _ActionType within _MaxRange
int CAI_Core::GetClosestActionUser(int _ActionType,fp32 _MaxRange, bool _bActive)
{
	fp32 DistSqr;
	fp32 MinDistSqr = _FP32_MAX;
	if (_MaxRange < _FP32_MAX)
	{
		MinDistSqr = Sqr(_MaxRange);
	}
	int iActionUser = 0;
	//Send message to all characters in character list
	int iChar;
	CSimpleIntList* plAgents = &(m_pAIResources->m_KBGlobals.ms_lAgents);
	CVec3Dfp32 OurPos = m_pGameObject->GetPosition();
	for (int i = 0; i < plAgents->Length(); i++)
	{
		if ((plAgents->IsValid(i))&&
			(iChar = plAgents->Get(i))&&
			(iChar != m_pGameObject->m_iObject)&&
			(IsBot(iChar)))
		{
			DistSqr = SqrDistanceToUs(iChar);
			if ((DistSqr < MinDistSqr)&&(IsTeammember(iChar)))
			{
				CAI_Core* pAI = GetAI(iChar);
				if ((pAI)&&(pAI->IsConscious())&&(pAI->m_AH.HasAction(_ActionType,_bActive)))
				{
					MinDistSqr = DistSqr;
					iActionUser = iChar;
				}
			}
		}
	}

	return iActionUser;
};

int CAI_Core::GetClosestActionUserWithSkiplist(int _ActionType,fp32 _MaxRange, bool _bActive, int* _pSkipArray, int _nArraySize)
{
	fp32 DistSqr;
	fp32 MinDistSqr = _FP32_MAX;

	if (!_pSkipArray)
	{
		return(0);
	}

	if (_MaxRange < _FP32_MAX)
	{
		MinDistSqr = Sqr(_MaxRange);
	}
	int iActionUser = 0;
	//Send message to all characters in character list
	int iChar;
	CSimpleIntList* plAgents = &(m_pAIResources->m_KBGlobals.ms_lAgents);
	CVec3Dfp32 OurPos = m_pGameObject->GetPosition();
	for (int i = 0; i < plAgents->Length(); i++)
	{
		if ((plAgents->IsValid(i))&&
			(iChar = plAgents->Get(i))&&
			(iChar != m_pGameObject->m_iObject)&&
			(IsBot(iChar)))
		{
			bool bOK = true;
			for (int j = 0; j < _nArraySize; j++)
			{
				if (_pSkipArray[j] == iChar)
				{
					bOK = false;
				}
			}

			if (bOK)
			{
				DistSqr = SqrDistanceToUs(iChar);
				if ((DistSqr < MinDistSqr)&&(IsTeammember(iChar)))
				{
					CAI_Core* pAI = GetAI(iChar);
					if ((pAI)&&(pAI->IsConscious())&&(pAI->m_AH.HasAction(_ActionType,_bActive)))
					{
						MinDistSqr = DistSqr;
						iActionUser = iChar;
					}
				}
			}
		}
	}

	return iActionUser;
};

//Gets animation resource index from the string on the format <FILEHANDLE>;<ANIM ID>
int CAI_Core::ResolveAnimHandle(const char *_pSequence)
{
	if (!m_pServer)
		return -1;

	CWObject_Message Msg(OBJMSG_GAME_RESOLVEANIMHANDLE);
	Msg.m_pData = (void *)_pSequence;
	return m_pServer->Message_SendToObject(Msg, m_pServer->Selection_GetSingleTarget(WSERVER_GAMEOBJNAME));
}

void CAI_Core::ActivateAnimgraph()
{
	m_bCanDeactivateAG = 0;
}

void CAI_Core::DeactivateAnimgraph()
{
	if (m_bCanDeactivateAG == -1)
	{
		m_bCanDeactivateAG = 1;
	}
}

void CAI_Core::SetAnimgraphDeactivation(bool _Deactivate)
{
	CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
	if (pChar)
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pChar);
		if (pCD)
		{
			pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_DISABLEREFRESH,_Deactivate);
		}
	}
}

// Sets _iGesture 
bool CAI_Core::SetWantedGesture(uint32 _iGesture, int32 _DurationTicks)
{
	if (!m_GestureTimer)
	{
		if (_iGesture)
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
			CWAG2I_Context Context(m_pGameObject,m_pServer,pCD->m_GameTime);
			pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_SHORTGESTURE,_iGesture),0);
		}
		m_GestureTimer = Max(int32(AI_TICKS_PER_SECOND * 2l), _DurationTicks);
	}
	
	return(true);
};

// Sets _iGesture 
bool CAI_Core::SetWantedMove(uint32 _iMove, int32 _DurationTicks)
{
	if (!m_GestureTimer)
	{
		if (_iMove)
		{
			CAI_Weapon* pWeapon = m_Weapon.GetWielded();
			if (pWeapon)
			{
				CRPG_Object_Item* pItem = pWeapon->GetItem();
				if ((pItem)&&(pItem->m_AnimType == AG2_IMPULSEVALUE_WEAPONTYPE_RIFLE))
				{
					_iMove += GESTURE_COMBAT_RIFLE_OFFSET;
				}
			}

			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
			CWAG2I_Context Context(m_pGameObject,m_pServer,pCD->m_GameTime);
			pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_SHOOTGESTURE,_iMove),0);
		}
		m_GestureTimer = Max(int32(AI_TICKS_PER_SECOND * 2l), _DurationTicks);
	}

	return(true);
};

bool CAI_Core::IsBehaviourSupported(int _iBehaviour)
{
	if ((_iBehaviour == BEHAVIOUR_NBR_STOPPED)||
		(_iBehaviour == BEHAVIOUR_NBR_STOP)||
		(_iBehaviour == BEHAVIOUR_NBR_STOP_FAST)||
		(_iBehaviour == BEHAVIOUR_NBR_STOP_DIRECT))
	{
		return(false);
	}
	return(true);
};

int32 CAI_Core::GetCurrentBehaviour()
{
	return(m_iCurBehaviour);
};

// _iBehaviour:		The behaviour nbr to be played
// _Prio:			Priority of the behavior
// _Flags:
// BEHAVIOUR_FLAGS_LEFT		= 0,		// For completeness
// BEHAVIOUR_FLAGS_FAST		= 1,		// Fast transition to/from behaviour main
// BEHAVIOUR_FLAGS_DIRECT	= 2,		// Go to/from behaviour main, "...do not collect any money"	
// BEHAVIOUR_FLAGS_RIGHT	= 4,		// Right version behaviour, also used as second version in two behaviours
// BEHAVIOUR_FLAGS_TOP		= 8,		// Top/fwd version of behaviour
// BEHAVIOUR_FLAGS_BOTTOM	= 16,		// Bottom/rear version of behaviour
// BEHAVIOUR_FLAGS_LOOP		= 32,		// Loop
// BEHAVIOUR_FLAGS_EXIT_MH	= 64,		// Exit to MH
// BEHAVIOUR_FLAGS_PP		= 128,		// Set perfect placement
// BEHAVIOUR_FLAGS_PO		= 256,		// Set perfect origin
// _Duration:		Duration of behaviour, i <= 0 the behaviour is looping
// _pScenePoint:	Optional scenepoint to play the behaviour at
// _pMPerfectPlacement:	Perfect placement matrix when _pScenePoint was not given
bool CAI_Core::SetWantedBehaviour2(uint32 _iBehaviour,
								   uint32 _Prio,
								   uint32 _Flags,
								   int32 _Duration,
								   CWO_ScenePoint* _pScenePoint,
								   CMat4Dfp32* _pMPerfectPlacement)
{
	if ((_iBehaviour == BEHAVIOUR_NBR_STOPPED)||(_iBehaviour == BEHAVIOUR_NBR_STOP)||(_iBehaviour == BEHAVIOUR_NBR_STOP_FAST)||
		(m_BehaviourStopIssued != BEHAVIOUR_STOP_NONE))
	{
		return(false);
	}

	int PrioClass = _Prio & CAI_Action::MASK_PRIO_ONLY;

	if ((!(_Flags & BEHAVIOUR_FLAGS_JUMPOK))&&(m_pGameObject->AI_IsJumping()))
	{	// Cannot initiate behaviours while jumping
		return(false);
	}

	if ((_iBehaviour >= BEHAVIOUR_BERZERKER_MIN)&&(_iBehaviour <= BEHAVIOUR_BERZERKER_MAX)&&(GetUp() * CVec3Dfp32(0.0f,0.0f,1.0f) < 0.99f))
	{	// Must be upright to do these behaviours
		TempDisableWallClimb(1);
		return(false);
	}

	// We must be able to get animgraph. Currently only characters have animgraph.
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
	if (pCD)
	{
		if (m_bBehaviourRunning)
		{
			if (!(_Flags & BEHAVIOUR_FLAGS_DIRECT))
			{
				// We don't restart the behaviour if it is already running, unless we want to go directly to main
				if (m_iCurBehaviour == _iBehaviour)
				{
					return(true);
				}
				if (PrioClass >= m_BehaviourPrioClass)
				{	
					if (!((_Flags & BEHAVIOUR_FLAGS_EXIT_MH)||(pCD->m_AnimGraph2.GetStateFlagsHiCombined() & AG2_STATEFLAGHI_BEHAVIORISMH)))
					{	// Current behaviour is not a MH
						if (PrioClass > m_BehaviourPrioClass)
						{
							if (!(_iBehaviour & AG2_WANTEDBEHAVIORFLAG_MASKTYPE))
							{
								m_iPendingBehaviour = _iBehaviour;
								m_PendingBehaviourPrioClass = _Prio;
								StopBehaviour(BEHAVIOUR_STOP_NORMAL,PrioClass);
							}
						}
						return(false);
					}
					// AG2_STATEFLAGHI_BEHAVIORISMH set and EQUAL or higher prio, just set the damned behaviour
				}
				else // PrioClass < m_BehaviourPrioClass
				{
					return(false);
				}
			}
		}

		// Reloading or hurt, postpone behaviour
		if (pCD->m_AnimGraph2.GetStateFlagsLoCombined() & (AG2_STATEFLAG_EQUIPPING | AG2_STATEFLAG_HURTACTIVE))
		{
			if (_Prio < CAI_Action::PRIO_FORCED)
			{
				return(false);
			}
			if (m_CurCB.m_iBehaviour == 0)
			{
				m_iPendingBehaviour = _iBehaviour;
				m_PendingBehaviourPrioClass = _Prio;
			}
		}

		int iBehaviour = _iBehaviour & ~AG2_WANTEDBEHAVIORFLAG_MASKTYPE;
		if (IsBehaviourSupported(iBehaviour))
		{
			ActivateAnimgraph();
			// We don't check for m_bGotAnimGraph2!
			CWAG2I_Context AGContext(m_pGameObject,m_pServer,pCD->m_GameTime,m_pServer->GetGameTickTime());
			// Set enter/exit mode
			if (_Flags & BEHAVIOUR_FLAGS_DIRECT)
			{
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_ENTEREXITTYPE,AG2_BEHAVIOR_ENTEREXIT_DIRECT);
			}
			else if (_Flags & BEHAVIOUR_FLAGS_FAST)
			{
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_ENTEREXITTYPE,AG2_BEHAVIOR_ENTEREXIT_FAST);
			}
			else	// AG2_BEHAVIOR_ENTEREXIT_NORMAL
			{
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_ENTEREXITTYPE,AG2_BEHAVIOR_ENTEREXIT_NORMAL);
			}
			//Set loopmode;
			if ((_pScenePoint)&&(_Duration == -1))
			{
				_Duration = RoundToInt(_pScenePoint->GetBehaviourDuration() *  GetAITicksPerSecond());
			}
			if (_Flags & BEHAVIOUR_FLAGS_LOOP)
			{
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_LOOPMODE,AG2_BEHAVIOR_LOOPMODE_CONTINUOUS);
			}
			else
			{
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_LOOPMODE,AG2_BEHAVIOR_LOOPMODE_ONCE);
			}
			// Set general property (typically used for left/right decisions
			int generalProperty = AG2_IMPULSEVALUE_BEHAVIOR_GENERAL_LEFT;
			if (m_CurCB.m_iBehaviour == iBehaviour)
			{	// Handle left/right flags etc
				if (m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_RIGHT)
				{
					generalProperty = AG2_IMPULSEVALUE_BEHAVIOR_GENERAL_RIGHT;
				}
				else if ((m_CurCB.m_TacFlags  & CWO_ScenePoint::TACFLAGS_POPUP)&&!(m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_LEFTRIGHT))
				{
					generalProperty = AG2_IMPULSEVALUE_BEHAVIOR_GENERAL_TOP;
				}
			}
			else if (_Flags & BEHAVIOUR_FLAGS_RIGHT)
			{
				generalProperty = AG2_IMPULSEVALUE_BEHAVIOR_GENERAL_RIGHT;
			}
			else if (_Flags & BEHAVIOUR_FLAGS_TOP)
			{
				generalProperty = AG2_IMPULSEVALUE_BEHAVIOR_GENERAL_TOP;
			}
			else if (_Flags & BEHAVIOUR_FLAGS_BOTTOM)
			{
				generalProperty = AG2_IMPULSEVALUE_BEHAVIOR_GENERAL_BOTTOM;
			}

			// Set perfect placement property
			if (_Flags & BEHAVIOUR_FLAGS_PP)
			{
				if (_pScenePoint)
				{	
					SetPerfectPlacement(_pScenePoint);
				}
				else if (_pMPerfectPlacement)
				{
					SetPerfectPlacement(_pMPerfectPlacement);
				}
				else
				{
					_Flags &= ~BEHAVIOUR_FLAGS_PP;
				}
			}

			int WeaponOffset = 0;
			if ((iBehaviour >= BEHAVIOUR_COMBAT_MIN)&&(iBehaviour <= BEHAVIOUR_COMBAT_MAX))
			{
				CAI_Weapon* pWeapon = m_Weapon.GetWielded();
				if (pWeapon)
				{
					CRPG_Object_Item* pItem = pWeapon->GetItem();
					if ((pItem)&&(pItem->m_AnimType == AG2_IMPULSEVALUE_WEAPONTYPE_RIFLE))
					{
						WeaponOffset = BEHAVIOUR_COMBAT_RIFLE_OFFSET;
					}
				}
			}
			
			if (!(_Flags & BEHAVIOUR_FLAGS_PP))
			{	// No more perfect placement
				NoPerfectPlacement();
			}
			if (_Flags & BEHAVIOUR_FLAGS_PO)
			{
				if (_pMPerfectPlacement)
				{
					SetPerfectOrigin(_pMPerfectPlacement);
				}
				else if (_pScenePoint)
				{
					SetPerfectOrigin(_pScenePoint);
				}
			}

			bool bPP = false;
			if (_Flags & BEHAVIOUR_FLAGS_PP)
			{
				bPP = true;
			}
			if (_Flags & BEHAVIOUR_FLAGS_EXIT_MH)
			{
				if ((m_CurCB.m_iBehaviour == iBehaviour)&&(m_PrevCB.m_iBehaviour)&&!(m_CurCB.m_TacFlags & (CWO_ScenePoint::TACFLAGS_LEFTRIGHT | CWO_ScenePoint::TACFLAGS_POPUP)))
				{	// remember from where we came
					if (m_PrevCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_RIGHT)
					{
						generalProperty = AG2_IMPULSEVALUE_BEHAVIOR_GENERAL_RIGHT;
					}
					else if ((m_PrevCB.m_TacFlags  & CWO_ScenePoint::TACFLAGS_POPUP)&&!(m_PrevCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_LEFTRIGHT))
					{
						generalProperty = AG2_IMPULSEVALUE_BEHAVIOR_GENERAL_TOP;
					}
				}
				OnRefreshDestination();
				pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_PERFECTPLACEMENT,bPP);
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_GENERAL,generalProperty);
#ifndef M_RTM
				if (DebugTarget())
				{
					CStr name = m_pGameObject->GetName();
					if (m_CurCB.m_iBehaviour == iBehaviour)
					{
						ConOutL(name+CStr(": Exit to ")+m_CurCB.m_DbgName+CStrF(" %d iBeh ",_iBehaviour) + CStrF("%d Tick ",GetAITick()));
					}
					else
					{
						ConOutL(name+CStr(": Exit to behaviour ")+CStrF("%d iBeh ",_iBehaviour) + CStrF("%d Tick ",GetAITick()));
					}
				}
#endif
				bool bResult = pCD->m_AnimGraph2.SendBehaviorImpulse(&AGContext,AG2_IMPULSEVALUE_BEHAVIOR_EXITMH,false);
				// WTF you may think!
				// Behaviours that don't support AG2_IMPULSEVALUE_BEHAVIOR_EXITMH return false and Olle promises
				// that there will NEVER be other kinds of failures from that call
				// so I just assume everything went peachy when false is returned.
				bResult = true;
				if (!bResult)
				{
#ifndef M_RTM
					ConOutL("Exit to behaviour failed!");
					CStr name = m_pGameObject->GetName();
					ConOutL(name+CStr(": Exit to behaviour failed!")+CStrF("%d Tick ",GetAITick()));
#endif
					return(false);
				}
			}
			else
			{
				OnRefreshDestination();
				pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_PERFECTPLACEMENT,bPP);
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_GENERAL,generalProperty);
#ifndef M_RTM
				if (DebugTarget())
				{
					CStr name = m_pGameObject->GetName();
					if (m_CurCB.m_iBehaviour == iBehaviour)
					{
						ConOutL(name+CStr(": Start ")+m_CurCB.m_DbgName+CStrF(" %d iBeh ",_iBehaviour) + CStrF("%d Tick ",GetAITick()));
					}
					else
					{
						ConOutL(name+CStr(": Start behaviour ")+CStrF("%d iBeh ",_iBehaviour) + CStrF("%d Tick ",GetAITick()));
					}
				}
#endif
				bool bResult = false;
				if ((PrioClass < CAI_Action::PRIO_ALERT)&&(!(m_UseFlags & USE_NO_TURN_IN))&&(_Flags & (BEHAVIOUR_FLAGS_PP |BEHAVIOUR_FLAGS_PO))&&(m_CharacterClass != CLASS_DARKLING))
				{
					bResult = pCD->m_AnimGraph2.SendBehaviorImpulse(&AGContext,iBehaviour+WeaponOffset,true);
				}
				else
				{
					bResult = pCD->m_AnimGraph2.SendBehaviorImpulse(&AGContext,iBehaviour+WeaponOffset,false);
				}
				// Clear PP
				pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_PERFECTPLACEMENT,false);
				if (!bResult)
				{
#ifndef M_RTM
					CStr name = m_pGameObject->GetName();
					ConOutL(name+CStr(": Start behaviour failed! ")+CStrF("%d iBeh ",_iBehaviour) + CStrF("%d Tick ",GetAITick()));
#endif
					return(false);
				}
			}
	
			m_iBehaviourRunning = _iBehaviour;
			m_BehaviourPrioClass = PrioClass;
			m_iPendingBehaviour = 0;
			m_PendingBehaviourPrioClass = CAI_Action::PRIO_MIN;
			if (_Flags & BEHAVIOUR_FLAGS_LOOP)
			{
				m_bBehaviourLooping = true;
			}
			else
			{
				m_bBehaviourLooping = false;
			}
			if (_Duration <= 0)
			{
				m_BehaviourExpirationTick = 0;
			}
			else
			{
				m_BehaviourExpirationTick = m_Timer + _Duration;
			}

			m_pBehaviourScenePoint = _pScenePoint;
			m_bBehaviourRunning = true;
			m_bBehaviourMainRunning = false;
			
			return(true);
		}
		else
		{
			return(false);
		}
	}

	return(false);
}

// Tries to stop a running behaviour
// _HowFast: Can be any of BEHAVIOUR_STOP_NORMAL, BEHAVIOUR_STOP_FAST or BEHAVIOUR_STOP_DIRECT
// _Prio: Is the prio of the stop, prio must be EQUAL or higher to stop a running behaviour
void CAI_Core::StopBehaviour(int32 _HowFast, uint32 _Prio, int16 _StopDir)
{
	if ((!m_bBehaviourRunning)||(_Prio < m_BehaviourPrioClass)||
		(m_iBehaviourRunning == BEHAVIOUR_NBR_STOP)||
		(m_iBehaviourRunning == BEHAVIOUR_NBR_STOP_FAST)||
		(m_iBehaviourRunning == BEHAVIOUR_NBR_STOP_DIRECT)||
		(m_iBehaviourRunning == BEHAVIOUR_NBR_STOPPED))
	{
		return;
	}

#ifndef M_RTM
	if (DebugTarget())
	{
		CStr name = m_pGameObject->GetName();
		ConOutL(name + CStr(" Stop behaviour ") + CStrF("%d iBeh ",m_iBehaviourRunning) + CStrF("%d Tick ",GetAITick()));
	}
#endif

	m_CurCB.Clear();
	m_PrevCB.Clear();
	m_CurCBTarget = 0;
	m_CurCBTimeout = 0;
	m_ScenepointCBTimeout = 0;
	m_BehaviourStopIssued = _HowFast;
	m_StopDir = _StopDir;

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
	CWAG2I_Context AGContext(m_pGameObject,m_pServer,pCD->m_GameTime,m_pServer->GetGameTickTime());
	// Left/right/mid?
	if (pCD->m_AnimGraph2.GetStateFlagsHiCombined() & AG2_STATEFLAGHI_BEHAVIORISMH)
	{
		if (_StopDir == 1)
		{
			pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_GENERAL,AG2_IMPULSEVALUE_BEHAVIOR_GENERAL_LEFT);
		}
		else if (_StopDir == -1)
		{
			pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_GENERAL,AG2_IMPULSEVALUE_BEHAVIOR_GENERAL_RIGHT);
		}
		else
		{	// BEHAVIOUR_FLAGS_TOP
			pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_GENERAL,AG2_IMPULSEVALUE_BEHAVIOR_GENERAL_TOP);
		}
	}
	
	if (pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_BEHAVIORACTIVE)
	{	// Behaviour running
		if ((_HowFast == BEHAVIOUR_STOP_NORMAL)||(_HowFast == BEHAVIOUR_STOP_NONE))
		{
			pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_ENTEREXITTYPE,AG2_BEHAVIOR_ENTEREXIT_NORMAL);
			m_iBehaviourRunning = BEHAVIOUR_NBR_STOP;
			m_BehaviourPrioClass = CAI_Action::PRIO_MIN;
		}
		else if (_HowFast == BEHAVIOUR_STOP_FAST)
		{
			pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_ENTEREXITTYPE,AG2_BEHAVIOR_ENTEREXIT_FAST);
			m_iBehaviourRunning = BEHAVIOUR_NBR_STOP_FAST;
			m_BehaviourPrioClass = CAI_Action::PRIO_MIN;
		}
		else
		{
			pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_ENTEREXITTYPE,AG2_BEHAVIOR_ENTEREXIT_DIRECT);
			m_iBehaviourRunning = BEHAVIOUR_NBR_STOP_DIRECT;
			m_BehaviourPrioClass = CAI_Action::PRIO_MIN;
		}
		pCD->m_AnimGraph2.SendBehaviorImpulse(&AGContext,AG2_IMPULSEVALUE_BEHAVIOR_EXIT);
	}
	else
	{	// Suspenders and belt
		pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_ENTEREXITTYPE,AG2_BEHAVIOR_ENTEREXIT_NORMAL);
		pCD->m_AnimGraph2.SendBehaviorImpulse(&AGContext,AG2_IMPULSEVALUE_BEHAVIOR_EXIT);
		ResetBehaviour();
	}
};

// The main part of the behaviour has (finally) started
void CAI_Core::OnBehaviourMainStarted(int _iBehaviour)
{
	m_bBehaviourMainRunning = true;

	if ((m_pBehaviourScenePoint)&&((!m_CurCB.m_iBehaviour)||(!m_CurCBTimeout)))
	{	// The new behaviour may possibly be a combat behaviour
		// We need an actual enemy for this to work
		CAI_AgentInfo* pTarget = m_AH.AcquireTarget();
		// if ((m_pBehaviourScenePoint->GetNoTargetTacFlag())||(pTarget))
		if (m_AH.CheckScenePointTarget(m_pBehaviourScenePoint,pTarget))
		{
			int NPicks = 0;
			for (int i = 0; i < m_lCombatBehaviour.Len(); i++)
			{
				S_CombatBehaviour cur = m_lCombatBehaviour[i];
				if (cur.m_iBehaviour != _iBehaviour) {continue;}
				if (m_lCombatBehaviour[i].m_iPrev == 0)
				{	
					uint16 CBTacFlags = m_lCombatBehaviour[i].m_TacFlags;
					uint16 SPTacFlags = m_pBehaviourScenePoint->GetTacFlags();
					if (CBTacFlags & CWO_ScenePoint::TACFLAGS_EXIT)
					{
						continue;
					}
					if (((CBTacFlags & CWO_ScenePoint::TACFLAGS_STAND)&&(SPTacFlags & CWO_ScenePoint::TACFLAGS_STAND))||
						((CBTacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)&&(SPTacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)))
					{
						m_li_CBChoices[NPicks] = i;
						NPicks++;
					}
				}
			}

			if (NPicks > 0)
			{	// Pick one among them
				int iPick = TruncToInt(NPicks * Random * 0.999f);
				iPick = Min(iPick,MAX_CB_PICKCOUNT-1);
				m_PrevCB.Clear();	// This is our very first CB behaviour this time around
				m_CurCB = m_lCombatBehaviour[m_li_CBChoices[iPick]];

				// Duration 0 lasts forever
				if (m_pBehaviourScenePoint->GetBehaviourDuration() <= 0.0f)
				{	// Set to one second from now; this will be prolonged as necessary
					m_ScenepointCBTimeout = m_Timer + GetAITicksPerSecond();
				}
				else
				{
					// Set timeout to now + scenepoint duration
					m_ScenepointCBTimeout = (int32)(m_Timer + m_pBehaviourScenePoint->GetBehaviourDuration() * GetAITicksPerSecond());
				}

				if (m_CurCB.m_Duration <= 0)
				{
					m_CurCBTimeout = RoundToInt(m_Timer + (0.5f + Random * 0.5f) * m_Patience);
				}
				else
				{
					m_CurCBTimeout = RoundToInt(m_Timer + (0.5f + Random * 0.5f) * m_CurCB.m_Duration);
				}
				if (pTarget)
				{
					m_CurCBTarget = pTarget->GetObjectID();
				}
				else
				{
					m_CurCBTarget = 0;
				}
				m_BehaviourExpirationTick = 0;
			}
		}
	}
}

void CAI_Core::OnBehaviourFinished(int _iBehaviour)
{
#ifndef M_RTM
	if (DebugTarget())
	{
		CStr name = m_pGameObject->GetName();
		ConOutL(name+CStr(": Behaviour finished ")+CStrF("%d iBeh ",_iBehaviour) + CStrF("%d Tick ",GetAITick()));
	}
#endif
	ResetBehaviour();
};

//Reset behaviour stuff. Should be called when we know we're not in behaviour state anymore.
void CAI_Core::ResetBehaviour()
{
	m_BehaviourExpirationTick = -1;
	m_bBehaviourRunning = false;
	m_bBehaviourMainRunning = false;
	m_iBehaviourRunning = BEHAVIOUR_NBR_STOPPED;
	m_BehaviourPrioClass = CAI_Action::PRIO_MIN;
	m_pBehaviourScenePoint = NULL;
	
	m_CurCB.Clear();
	m_PrevCB.Clear();
	m_CurCBTarget = 0;
	m_CurCBTimeout = 0;
	m_ScenepointCBTimeout = 0;
};

bool CAI_Core::InitCombatBehaviours(CWO_ScenePoint* _pScenePoint,int32 _Prio, int32 _iTarget)
{
	if (!_pScenePoint)
	{
		return(false);
	}

	if ((m_CurCB.m_iBehaviour)||(m_BehaviourPrioClass >= (_Prio & CAI_Action::MASK_PRIO_ONLY)))
	{
		return(true);
	}

	// Find the behaviour starting behaviour
	int NPicks = 0;
	for (int i = 0; i < m_lCombatBehaviour.Len(); i++)
	{
		S_CombatBehaviour cur = m_lCombatBehaviour[i];
		if (m_lCombatBehaviour[i].m_iPrev == 0)
		{
			uint16 CBTacFlags = m_lCombatBehaviour[i].m_TacFlags;
			uint16 SPTacFlags = _pScenePoint->GetTacFlags();
			if (CBTacFlags & CWO_ScenePoint::TACFLAGS_EXIT)
			{
				continue;
			}
			if (((CBTacFlags & CWO_ScenePoint::TACFLAGS_STAND)&&(SPTacFlags & CWO_ScenePoint::TACFLAGS_STAND))||
				((CBTacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)&&(SPTacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)))
			{
				m_li_CBChoices[NPicks] = i;
				NPicks++;
			}
		}
	}

	// Lock the look device and flush any issued looks
	m_DeviceLook.Free();
	m_DeviceLook.Use();

	if (NPicks > 0)
	{	// Pick one among them
		int iPick = TruncToInt(NPicks * Random * 0.999f);
		iPick = Min(iPick,MAX_CB_PICKCOUNT-1);
		m_PrevCB.Clear();	// This is our very first CB behaviour this time around
		m_CurCB = m_lCombatBehaviour[m_li_CBChoices[iPick]];
		m_CurCB.m_Duration = 1;	// Immideatelt pick a new one
		if (_pScenePoint->GetBehaviour())
		{	// We enter using the scenepoints normal behaviour
			m_CurCB.m_iBehaviour = _pScenePoint->GetBehaviour();
		}
		if (SetWantedBehaviour2(m_CurCB.m_iBehaviour,_Prio,BEHAVIOUR_FLAGS_PP | BEHAVIOUR_FLAGS_LOOP,m_CurCB.m_Duration,_pScenePoint))
		{
			// Duration 0 lasts forever
			if (m_pBehaviourScenePoint->GetBehaviourDuration() == 0.0f)
			{
				// Set to one second from now; this will be prolonged as necessary
				m_ScenepointCBTimeout = m_Timer + GetAITicksPerSecond();
			}
			else
			{
				// Set timeout to now + scenepoint duration
				m_ScenepointCBTimeout = (int32)(m_Timer + _pScenePoint->GetBehaviourDuration() * GetAITicksPerSecond());
			}

			if (m_CurCB.m_Duration <= 0)
			{
				m_CurCBTimeout = RoundToInt(m_Timer + (0.5f + Random * 0.5f) * m_Patience);
			}
			else
			{
				m_CurCBTimeout = RoundToInt(m_Timer + (0.5f + Random * 0.5f) * m_CurCB.m_Duration);
			}
			m_CurCBTarget = _iTarget;
			m_BehaviourExpirationTick = 0;
			return(true);
		}
		else
		{
			m_CurCB.Clear();
			m_PrevCB.Clear();
			m_CurCBTarget = 0;
			return(false);
		}
	}
	else
	{
		return(false);
	}

	return(true);
};

bool CAI_Core::GiveOrder(int16 _Order, int _DurationTicks)
{
	if ((_Order != ORDER_NONE)&&(_Order == m_Order)&&(m_OrderTicksRemaining > 0))
	{	// Prolong the duration of the order
		m_OrderTicksRemaining = Max(m_OrderTicksRemaining,_DurationTicks);
		return(true);
	}
	else
	{
		if ((_Order == ORDER_NONE)||(_DurationTicks == 0))
		{	// Clear order
			m_Order = ORDER_NONE;
			m_OrderTicksRemaining = 0;
		}

		switch(_Order)
		{
		case ORDER_CHARGE:
			{	// Expire COMBAT for a while
				// Expire any held CBSPs
				// Shout charge (unless alone)
				if (m_DeviceSound.SoundAvailable(CAI_Device_Sound::LEADER_COMBAT_FWD))
				{
					fp32 Range;
					if (GetClosestTeammember(true,0,&Range))
					{
						UseRandom("Got IMPULSE_CHARGE",CAI_Device_Sound::LEADER_COMBAT_FWD,CAI_Action::PRIO_COMBAT);
						// *** TODO: Play appropriate behaviour (don't forget to precache)
					}
				}
				m_Order = ORDER_CHARGE;
				m_OrderTicksRemaining = _DurationTicks;
				// Touch target to make us fire/run
				CAI_AgentInfo* pTarget = m_AH.AcquireTarget();
				if (pTarget)
				{
					pTarget->TouchLastSpotted();
				}
				m_AH.ReleaseSPs(2);
				m_AH.PauseAction(CAI_Action::COMBAT,_DurationTicks);
				StopBehaviour(BEHAVIOUR_STOP_NORMAL,m_BehaviourPrioClass);
				ClearCombatBehaviours();
				// Make sure CLOSE kicks in right away
				CAI_Action* pAction = NULL;
				if ((m_AH.GetAction(CAI_Action::CLOSE,&pAction))&&(!pAction->IsTaken()))
				{
					int Estimate = m_Personality.EstimateAction(pAction);
					if (Estimate > 0)
					{	
						pAction->OnStart();
					}
				}
				if ((m_AH.GetAction(CAI_Action::DARKLING_CLOSE,&pAction))&&(!pAction->IsTaken()))
				{
					int Estimate = m_Personality.EstimateAction(pAction);
					if (Estimate > 0)
					{	
						pAction->OnStart();
					}
				}
				return true;
			}
			break;

		case ORDER_COVER:
			{	// Expire CLOSE for a while
				// Expire any held CBSPs
				// Shout cover (unless alone)
				if (m_Weapon.GetWieldedArmsClass() >= CAI_WeaponHandler::AI_ARMSCLASS_GUN)
				{	// No cover action for knifers
					if (m_DeviceSound.SoundAvailable(CAI_Device_Sound::LEADER_COMBAT_COVER))
					{
						fp32 Range;
						if (GetClosestTeammember(true,0,&Range))
						{
							UseRandom("Got IMPULSE_COVER",CAI_Device_Sound::LEADER_COMBAT_COVER,CAI_Action::PRIO_COMBAT);
							// *** TODO: Play appropriate behaviour (don't forget to precache)
						}
					}
					m_Order = ORDER_COVER;
					m_OrderTicksRemaining = _DurationTicks;
					m_AH.ReleaseSPs(2);
					m_AH.PauseAction(CAI_Action::CLOSE,_DurationTicks);
					StopBehaviour(BEHAVIOUR_STOP_NORMAL,m_BehaviourPrioClass);
					ClearCombatBehaviours();
					m_AH.ReportEvent(CAI_EventHandler::ON_INJURED,m_AH.m_iTarget,DAMAGETYPE_MASK);
					// Make sure COVER runs
					CAI_Action* pAction = NULL;
					if ((m_AH.GetAction(CAI_Action::COVER,&pAction))&&(!pAction->IsTaken()))
					{
						int Estimate = m_Personality.EstimateAction(pAction);
						if (Estimate > 0)
						{	
							pAction->OnStart();
						}
					}
				}
				return true;
			}
			break;

		case ORDER_FLANK:
			{	// Expire any held CBSPs
				if (m_DeviceSound.SoundAvailable(CAI_Device_Sound::LEADER_COMBAT_FLANK))
				{
					fp32 Range;
					if (GetClosestTeammember(true,0,&Range))
					{
						UseRandom("Got IMPULSE_COVER",CAI_Device_Sound::LEADER_COMBAT_FLANK,CAI_Action::PRIO_COMBAT);
						// *** TODO: Play appropriate behaviour (don't forget to precache)
					}
				}
				m_Order = ORDER_FLANK;
				m_OrderTicksRemaining = _DurationTicks;
				m_AH.ReleaseSPs(2);
				StopBehaviour(BEHAVIOUR_STOP_NORMAL,m_BehaviourPrioClass);
				ClearCombatBehaviours();
				return true;
			}
			break;

		case ORDER_ESCALATE:
			m_Order = ORDER_ESCALATE;
			m_OrderTicksRemaining = _DurationTicks;
			m_AH.ReleaseSPs(2);
			StopBehaviour(BEHAVIOUR_STOP_NORMAL,m_BehaviourPrioClass);
			ClearCombatBehaviours();
			break;

		default:
			// Clear order
			m_Order = ORDER_NONE;
			m_OrderTicksRemaining = 0;
			break;
		}
	}

	return(false);
};

void CAI_Core::ClearCombatBehaviours(int16 _StopDir)
{
	if ((m_CurCB.m_iBehaviour)&&(m_bBehaviourRunning))
	{	// Fast CB exit if flagged
		if (m_CurCB.m_UseFlags & CBFLAG_FASTEXIT)
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
			CWAG2I_Context Context(m_pGameObject,m_pServer,pCD->m_GameTime);
			m_DeviceStance.SetTargetInFOV(false);
			m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
			pCD->m_AnimGraph2.ForceStance(&Context);
			ResetBehaviour();
			m_BehaviourStopIssued = BEHAVIOUR_STOP_NONE;
		}
		else
		{	// Regular plain vanilla exit
			if (_StopDir != 0)
			{	// Exit left/right
				m_DeviceStance.SetTargetInFOV(true);
			}
			StopBehaviour(BEHAVIOUR_STOP_NORMAL,CAI_Action::PRIO_COMBAT,_StopDir);
		}

	}
	if ((m_pBehaviourScenePoint)&&(m_pBehaviourScenePoint->GetTacFlags())&&(!m_pBehaviourScenePoint->GetBehaviour()))
	{
		m_pBehaviourScenePoint = NULL;
		NoPerfectPlacement();
	}
	m_CurCB.Clear();
	m_PrevCB.Clear();
	m_CurCBTarget = 0;
	m_CurCBTimeout = 0;
	m_ScenepointCBTimeout = 0;
	
};

bool CAI_Core::HandleCombatBehaviours()
{
	if (!m_pBehaviourScenePoint)
	{
		ClearCombatBehaviours();
		return(false);
	}

	if ((!m_CurCB.m_iBehaviour)||(!m_CurCBTimeout))
	{
		ClearCombatBehaviours();
		return(false);
	}

	int32 iCreepDark = 0;
	if (m_KB.GetCreepingDark(&iCreepDark))
	{
		ClearCombatBehaviours();
		return(false);
	}

	// Put yer debugcode here
#ifndef M_RTM
	if (DebugTarget())
	{
		bool wtf = true;
	}
#endif

	bool bSPTimeout = false;
	if ((!m_AH.RequestScenePoint(m_pBehaviourScenePoint))||(m_Timer >= m_ScenepointCBTimeout))
	{	// SP timeout, we must pick an exit or MH to exit the next time
		// If cur CB was exit or MH we exit as soon as m_Timer >= m_CurCBTimeout
		bSPTimeout = true;
	}
	else
	{	// Duration 0 lasts forever
		if (m_pBehaviourScenePoint->GetBehaviourDuration() == 0.0f)
		{
			m_ScenepointCBTimeout = m_Timer + GetAITicksPerSecond();
		}
	}
	
	bool bCalmFight = false;
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
	CAI_AgentInfo* pTarget = NULL;
	pTarget = m_KB.GetAgentInfo(m_CurCBTarget);
	if ((!pTarget)||(pTarget->GetCurRelation() < CAI_AgentInfo::ENEMY)||(pTarget->GetCurAwareness() < CAI_AgentInfo::DETECTED))
	{
		pTarget = m_AH.AcquireTarget();
		if (pTarget)
		{
			m_CurCBTarget = pTarget->GetObjectID();
		}
	}

	if (!m_AH.CheckScenePointTarget(m_pBehaviourScenePoint,pTarget))
	{
		if (!pTarget)
		{	// Dunno where he is, just exit straight ahead I guess
			ClearCombatBehaviours(0);
			return(false);
		}

		// Determine if we should exit left, right or straight ahead
		OnTrackAgent(pTarget, 6, false, false);
		CMat4Dfp32 OurMat;
		GetBaseMat(OurMat);
		CVec3Dfp32 Diff = pTarget->GetCorrectPosition() - OurMat.GetRow(3);
		Diff.Normalize();
		fp32 Dot = m_pBehaviourScenePoint->GetDirection() * Diff;
		if (Abs(Dot) >= 0.93f)
		{	// Less than 20 degrees off
			ClearCombatBehaviours(0);
		}
		else if (OurMat.GetRow(1) * Diff >= 0.0f)
		{	// Left
			ClearCombatBehaviours(1);
		}
		else
		{	// Right
			ClearCombatBehaviours(-1);
		}
		return(false);
	}
	

	if (pTarget)
	{
		CVec3Dfp32 TargetPos = pTarget->GetSuspectedPosition();

		// Handle bCalmFight
		if (pTarget->GetGetLastSpottedTicks() >= m_Patience * 2)
		{	// We haven't seen the target for quite some time
			bCalmFight = true;
		}

		// Handle look constraining
		CMat4Dfp32 SPMat = m_pBehaviourScenePoint->GetPositionMatrix();
		CVec3Dfp32 HeadPos = GetLookPos();
		CVec3Dfp32 TargetHead = pTarget->GetSuspectedHeadPosition();
		CVec3Dfp32 Dir = TargetHead - HeadPos;
		m_DeviceLook.Free();
		m_DeviceLook.Use();
		Dir.Normalize();
		int iObj = pTarget->GetObjectID();
		int WireColor = kColorGreen;
		if (pTarget->GetCurAwareness() < CAI_AgentInfo::SPOTTED)
		{
			iObj = -1;
		}
		if ((m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_LEFT)&&(Dir * SPMat.GetRow(1) < 0.0f))
		{
			Dir = SPMat.GetRow(0);
			iObj = 0;
			WireColor = kColorRed;
		}
		else if ((m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_RIGHT)&&(Dir * SPMat.GetRow(1) > 0.0f))
		{
			Dir = SPMat.GetRow(0);
			iObj = 0;
			WireColor = kColorRed;
		}
		// *** Should be -1 instead of 0 I guess? ***
		m_pGameObject->AI_SetEyeLookDir(Dir,iObj);
		
		m_DeviceLook.Use();
#ifndef M_RTM
		if (DebugTarget())
		{
			Debug_RenderWire(HeadPos,HeadPos+Dir * 100.0f, WireColor);
		}
#endif
	}
	else
	{	// No target but still valid
		bCalmFight = true;
	}

	// Fight is always calm when we're covering
	if (HasOrder(CAI_Core::ORDER_COVER))
	{
		bCalmFight = true;
	}
		
	// Reload when in MH and main has started
	if ((m_CurCB.m_iPrev == 0)&&(m_bBehaviourMainRunning))
	{
		if (bCalmFight)
		{
			Reload(0.5f);
		}
		else
		{
			Reload(1.0f);
		}
	}

	// ***
	if (m_CurCB.m_iBehaviour)
	{
		if (true)
		{	// MH
			CMat4Dfp32 Mat;
			GetBaseMat(Mat);
			CVec3Dfp32 BasePos = Mat.GetRow(3);
			CVec3Dfp32 EyePos = GetHeadPos();
			CVec3Dfp32 GunPos = GetAimPosition();
			Mat.GetRow(3) = CVec3Dfp32(0);
			CVec3Dfp32 LocalEyePos = (EyePos-BasePos) * m_InvBaseMat;
			CVec3Dfp32 LocalGunPos = (GunPos-BasePos) * m_InvBaseMat;

			Debug_RenderWire(EyePos,EyePos + Mat.GetRow(0) * 16.0f, kColorRed, 2.0f);
			Debug_RenderWire(GunPos,GunPos + Mat.GetRow(0) * 16.0f, kColorBlue, 2.0f);
		}
	}
	// ***

	// If the fight is not calm and SP has EXIT/CHARGE and we are in MH and the SP is about to time out (1 sec?) and we have at least half a mag
	// We set back m_CurCBTimeout and set bSPTimeout to try to force an exit/charge.
	if ((!bCalmFight)&&(m_CurCB.m_iPrev == 0)&&(m_Timer+GetAITicksPerSecond() > m_ScenepointCBTimeout)&&
		(m_pBehaviourScenePoint->GetTacFlags() & (CWO_ScenePoint::TACFLAGS_EXIT | CWO_ScenePoint::TACFLAGS_CHARGE))&&
		(GetMagsFired() <= 0.5f))
	{	// MH and near timeout and SP has EXIT
		bSPTimeout = true;
		m_CurCBTimeout = m_Timer;
		if (m_Timer < m_ScenepointCBTimeout)
		{
			m_ScenepointCBTimeout = m_Timer;
		}
	}

	// Was hit while leaning out, get back in!
	if ((m_KB.GetLastHurtTicks() <= 1)&&(m_CurCB.m_TacFlags & (CWO_ScenePoint::TACFLAGS_LEFTRIGHT | CWO_ScenePoint::TACFLAGS_POPUP | CWO_ScenePoint::TACFLAGS_COWARD)))
	{
		m_CurCBTimeout = m_Timer;
	}

	// Let hurts and reloads finish gracefully
	if (pCD->m_AnimGraph2.GetStateFlagsLoCombined() & (AG2_STATEFLAG_EQUIPPING | AG2_STATEFLAG_HURTACTIVE))
	{
		m_CurCBTimeout = m_Timer+1;
	}
	if ((m_Timer >= m_CurCBTimeout)&&(m_bBehaviourMainRunning))
	{	// What if the moron scripter ONLY checked TACFLAGS_STAND or TACFLAGS_CROUCH?
		if (!(m_pBehaviourScenePoint->GetTacFlags() & 
			(CWO_ScenePoint::TACFLAGS_LEFTRIGHT | CWO_ScenePoint::TACFLAGS_POPUP | CWO_ScenePoint::TACFLAGS_COWARD)))
		{
			m_CurCBTimeout = m_Timer+1;
		}
	}

	m_DeviceStance.SetTargetInFOV(true);
	if ((m_Timer >= m_CurCBTimeout)&&(m_bBehaviourMainRunning))
	{
		if (m_CurCB.m_TacFlags & (CWO_ScenePoint::TACFLAGS_EXIT | CWO_ScenePoint::TACFLAGS_CHARGE))
		{
			ClearCombatBehaviours();
			if (m_pBehaviourScenePoint)
			{
				m_pBehaviourScenePoint->InvalidateScenepoint(m_pServer,60);
			}
			return(false);
		}

		int NPicks = 0;
		for (int i = 0; i < m_lCombatBehaviour.Len(); i++)
		{
			S_CombatBehaviour cur = m_lCombatBehaviour[i];
			if (m_CurCB.m_iBehaviour == m_lCombatBehaviour[i].m_iBehaviour)
			{	// Never the same behaviour twice
				continue;
			}
			if ((m_CurCB.m_iPrev != 0)&&(m_lCombatBehaviour[i].m_iPrev != 0))
			{	// Cur none-MH requires next to be MH
				continue;
			}
			if ((m_CurCB.m_iPrev == 0)&&(m_lCombatBehaviour[i].m_iPrev == 0))
			{	// Cur MH requires next to be none-MH
				continue;
			}

			uint16 CBTacFlags = m_lCombatBehaviour[i].m_TacFlags;
			uint16 SPTacFlags = m_pBehaviourScenePoint->GetTacFlags();
			if (bCalmFight)
			{	// Mask out TACFLAGS_EXIT | TACFLAGS_CHARGE | TACFLAGS_FIRE in calm fights
				SPTacFlags &= ~(CWO_ScenePoint::TACFLAGS_EXIT | CWO_ScenePoint::TACFLAGS_CHARGE | CWO_ScenePoint::TACFLAGS_FIRE);
			}
			else
			{	// Add CWO_ScenePoint::TACFLAGS_FIRE when fight is not calm
				if (CBTacFlags & CWO_ScenePoint::TACFLAGS_FIRE)
				{
					SPTacFlags |= CWO_ScenePoint::TACFLAGS_FIRE;
				}
			}

			// CB has both left and right flags and scenepoint has left and/or right
			if (((CBTacFlags & CWO_ScenePoint::TACFLAGS_LEFTRIGHT) == CWO_ScenePoint::TACFLAGS_LEFTRIGHT)&&
				(SPTacFlags & CWO_ScenePoint::TACFLAGS_LEFTRIGHT))
			{	
				SPTacFlags |= CWO_ScenePoint::TACFLAGS_LEFTRIGHT;
			}

			if (bCalmFight)
			{	// No TACFLAGS_EXIT,TACFLAGS_CHARGE,TACFLAGS_FIRE
				if (CBTacFlags & (CWO_ScenePoint::TACFLAGS_EXIT | CWO_ScenePoint::TACFLAGS_CHARGE | CWO_ScenePoint::TACFLAGS_FIRE))
				{
					continue;
				}
			}

			// Scenepoints with both crouch and stand cannot allow popup that is neither left nor right
			if ((SPTacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)&&(SPTacFlags & CWO_ScenePoint::TACFLAGS_STAND))
			{
				if ((CBTacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)&&(CBTacFlags & (CWO_ScenePoint::TACFLAGS_POPUP | CWO_ScenePoint::TACFLAGS_FIRE))&&(!(CBTacFlags & CWO_ScenePoint::TACFLAGS_LEFTRIGHT)))
				{
					continue;
				}
			}

			// Transitions from stand to crouch and vice verse doesn't work that well, we skip 'em
			if (((CBTacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)&&(m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_STAND))||
				((CBTacFlags & CWO_ScenePoint::TACFLAGS_STAND)&&(m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)))
			{
				continue;
			}

			if (CBTacFlags & CWO_ScenePoint::TACFLAGS_CHANGEPOSTURE)
			{
				continue;
			}

			/*
			// Handle transitions between crouch and standing
			if (((CBTacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)&&(m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_STAND))||
				((CBTacFlags & CWO_ScenePoint::TACFLAGS_STAND)&&(m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)))
			{
				if ((CBTacFlags & CWO_ScenePoint::TACFLAGS_CHANGEPOSTURE)
					&&(SPTacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)&&(SPTacFlags & CWO_ScenePoint::TACFLAGS_STAND))
				{	// No timeout and Movinhold
					if ((!bSPTimeout)&&(m_CurCB.m_iPrev == 0))
					{
						m_li_CBChoices[NPicks] = i;
						NPicks++;
					}
					else
					{
						continue;
					}
				}
				else
				{
					continue;
				}
			}
			*/

			if (bSPTimeout)
			{
				if (m_lCombatBehaviour[i].m_iPrev == 0)
				{	// MH
					if ((SPTacFlags & CBTacFlags) == CBTacFlags)
					{
						m_li_CBChoices[NPicks] = i;
						NPicks++;
					}
				}
				else if (CBTacFlags & (CWO_ScenePoint::TACFLAGS_EXIT | CWO_ScenePoint::TACFLAGS_CHARGE))
				{	// Only TACFLAGS_EXIT,TACFLAGS_CHARGE are legitimate
					if ((SPTacFlags & CBTacFlags) == CBTacFlags)
					{
						m_li_CBChoices[NPicks] = i;
						NPicks++;
					}
				}
			}
			else
			{
				if (!(CBTacFlags & (CWO_ScenePoint::TACFLAGS_EXIT | CWO_ScenePoint::TACFLAGS_CHARGE)))
				{	// TACFLAGS_EXIT,TACFLAGS_CHARGE are illegitimate
					if ((SPTacFlags & CBTacFlags) == CBTacFlags)
					{
						m_li_CBChoices[NPicks] = i;
						NPicks++;
					}
				}
			}
		}

		// Pick one among them
		if (NPicks > 0)
		{
			m_PrevCB = m_CurCB;
			int iPick = TruncToInt(NPicks * Random * 0.999f);
			iPick = Min(iPick,MAX_CB_PICKCOUNT-1);
			if ((!bCalmFight)&&(!(m_lCombatBehaviour[m_li_CBChoices[iPick]].m_TacFlags & CWO_ScenePoint::TACFLAGS_FIRE)))
			{	// Pick again if we are fighting and first choice wasn't a fight one
				iPick = TruncToInt(NPicks * Random * 0.999f);
			}
			if ((!bCalmFight)&&(!(m_lCombatBehaviour[m_li_CBChoices[iPick]].m_TacFlags & CWO_ScenePoint::TACFLAGS_FIRE)))
			{	// Pick again if we are fighting and first choice wasn't a fight one
				iPick = TruncToInt(NPicks * Random * 0.999f);
			}
			if ((!bCalmFight)&&(!(m_lCombatBehaviour[m_li_CBChoices[iPick]].m_TacFlags & CWO_ScenePoint::TACFLAGS_FIRE)))
			{	// Pick again if we are fighting and first choice wasn't a fight one
				iPick = TruncToInt(NPicks * Random * 0.999f);
			}
			m_CurCB = m_lCombatBehaviour[m_li_CBChoices[iPick]];
			m_BehaviourPrioClass = CAI_Action::PRIO_COMBAT;
			int OurArmsClass = m_Weapon.GetWieldedArmsClass();
			// Set appropriate facial
			if (m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_FIRE)
			{
				if (m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_COWARD)
				{
					m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_STRAINED,m_CurCB.m_Duration+10);
				}
				else
				{
					m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_AIMGUN,m_CurCB.m_Duration+10);
				}
			}
			// *** Ugliest hack in a looong time ***
			if ((m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_FIRE)&&(m_Weapon.GetWieldedWeaponType() == CAI_WeaponHandler::AI_WEAPONTYPE_SHOTGUN))
			{	// Shorten duration for shotgunners
				m_CurCB.m_Duration = 40;
			}
			// ***
		}
		else
		{
			if (bSPTimeout)
			{	// No exits
				ClearCombatBehaviours();
				if (m_pBehaviourScenePoint)
				{
					m_pBehaviourScenePoint->InvalidateScenepoint(m_pServer,60);
				}
				return(false);
			}
			else
			{	// Keep old behaviour
				m_BehaviourPrioClass = CAI_Action::PRIO_COMBAT;

			}
		}

		int32 Duration = 0;
		int iCurBehaviour = GetCurrentBehaviour();
		bool bResult = false;
		if (iCurBehaviour == m_CurCB.m_iBehaviour)
		{
			m_bBehaviourRunning = true;
			m_iBehaviourRunning = iCurBehaviour;
			bResult = true;
		}
		else
		{
			int32 Flags = 0;
			int32 Duration = 0;
			if (m_CurCB.m_TacFlags & (CWO_ScenePoint::TACFLAGS_EXIT | CWO_ScenePoint::TACFLAGS_CHARGE))
			{
				if (m_CurCB.m_Duration > 0.0f)
				{
					Duration = m_CurCB.m_Duration;
				}
				else
				{
					Duration = RoundToInt(GetAITicksPerSecond() * 5.0f);
				}
			}
			else
			{
				Flags |= BEHAVIOUR_FLAGS_LOOP;
			}
			if (m_CurCB.m_iPrev == 0)
			{
				Flags |= BEHAVIOUR_FLAGS_EXIT_MH;
			}

			// Before we actually commit ourselves we should check if we have a clear LOS (!MH) or not (MH)
			// NOTE: Should we store our investigations so we don't check the same dir twice?
			if ((m_pBehaviourScenePoint->GetType() & CWO_ScenePoint::DYNAMIC)&&
				(!(m_CurCB.m_TacFlags & (CWO_ScenePoint::TACFLAGS_EXIT | CWO_ScenePoint::TACFLAGS_CHARGE))))
			{
				CMat4Dfp32 SPMat = m_pBehaviourScenePoint->GetPositionMatrix();
				CVec3Dfp32 Pos = SPMat.GetRow(3);
				CVec3Dfp32 Fwd = SPMat.GetRow(0);
				CVec3Dfp32 Left = SPMat.GetRow(1);
				CVec3Dfp32 Up = SPMat.GetRow(2);
				CVec3Dfp32 Start, End;
				if (Flags & BEHAVIOUR_FLAGS_EXIT_MH)
				{	// We must be in cover
					if (m_pBehaviourScenePoint->GetTacFlags())
					if (m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)
					{
						Start = Pos + Up * CB_OFF_CROUCH;
					}
					else
					{
						Start = Pos + Up * CB_OFF_STAND;
					}
					// Q: Should we use GetSqrt(m_pBehaviourScenePoint->GetSqrArcMaxRange()) here?
					// A: Probably not. Evil scripters may set it to extreme ranges which results in really expensive raycasts
					End = Start + Fwd * 64.0f;
					if (FastCheckLOS(Start,End,GetObjectID(),m_CurCBTarget))
					{	// Bad, we are NOT protected; bail out
						// We're done: Clear out
						// Invalidate scenepoint for 30 secs
#ifndef M_RTM
						// if (DebugTarget())
						{
							m_AH.DebugDrawScenePoint(m_pBehaviourScenePoint,true,kColorWhite,30.0f);
							Debug_RenderWire(Start,End,kColorWhite,30.0f);
						}
#endif
						m_pBehaviourScenePoint->InvalidateScenepoint(m_pServer,GetAITicksPerSecond() * 30l);
						ClearCombatBehaviours();
						return(true);
					}

				}
				else
				{	// We must have clear LOS
					if (m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)
					{
						Start = Pos + Up * CB_OFF_CROUCH;
					}
					else
					{
						Start = Pos + Up * CB_OFF_STAND;
					}

					if (m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_LEFT)
					{
						Start += Left * CB_OFF_LEAN;
					}
					else if (m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_RIGHT)
					{
						Start -= Left * CB_OFF_LEAN;
					}
					else
					{	// Popup
						Start = Pos + Up * CB_OFF_POPUP;
					}
					End = Start + Fwd * 64.0f;
					if (!FastCheckLOS(Start,End,GetObjectID(),m_CurCBTarget))
					{	// Bad, LOS is blocked; bail out
						// We're done: Clear out
						// Invalidate scenepoint for 30 secs
#ifndef M_RTM
						// if (DebugTarget())
						{
							m_AH.DebugDrawScenePoint(m_pBehaviourScenePoint,true,kColorWhite,30.0f);
							Debug_RenderWire(Start,End,kColorWhite,30.0f);
						}
#endif
						m_pBehaviourScenePoint->InvalidateScenepoint(m_pServer,GetAITicksPerSecond() * 30l);
						ClearCombatBehaviours();
						return(true);
					}
				}
			}


			bResult = SetWantedBehaviour2(m_CurCB.m_iBehaviour,m_BehaviourPrioClass,Flags,Duration,m_pBehaviourScenePoint);
			if ((bResult)&&(m_CurCB.m_TacFlags & (CWO_ScenePoint::TACFLAGS_EXIT | CWO_ScenePoint::TACFLAGS_CHARGE)))
			{	// We're done: Clear out
				m_CurCB.Clear();
				m_PrevCB.Clear();
				m_CurCBTarget = 0;
				m_CurCBTimeout = 0;
				m_ScenepointCBTimeout = 0;
				return(true);
			}
		}

		if (bResult)
		{
			if (m_CurCB.m_Duration <= 0)
			{
				m_CurCBTimeout = RoundToInt(m_Timer + (0.5f + Random * 0.5f) * m_Patience);
			}
			else
			{
				m_CurCBTimeout = RoundToInt(m_Timer + (0.5f + Random * 0.5f) * m_CurCB.m_Duration);
			}
			m_BehaviourExpirationTick = 0;
			return(true);
		}
		else
		{
			ClearCombatBehaviours();
			if (m_pBehaviourScenePoint)
			{
				m_pBehaviourScenePoint->InvalidateScenepoint(m_pServer,60);
			}
			return(false);
		}
	}
	else	// 'if ((m_Timer >= m_CurCBTimeout)&&(m_bBehaviourMainRunning))'
	{	// Postpone expiration until m_bBehaviourMainRunning is received
		if ((m_Timer >= m_CurCBTimeout)&&(m_BehaviourExpirationTick))
		{
			m_BehaviourExpirationTick = m_Timer + 1;
		}
	}
	
	return(true);
};

//Drag the given object, if possible. Currently only usable on dead ragdoll characters.
void CAI_Core::DragObject(int _iObj)
{
	//Drag to center position, offset by a bit in look direction.
	//This should really be defined somehow in template instead, for example "always drag with right hand" etc
	CVec3Dfp32 Pos = GetCenterPosition(m_pGameObject);
	if (INVALID_POS(Pos))
		return;
	Pos += CVec3Dfp32::GetRow(m_pGameObject->GetPositionMatrix(), 0) * - 12.0f;
    
	CWObject_Message Drag(OBJMSG_CHAR_DRAG);
	Drag.m_Param0 = 2;
	Drag.m_VecParam0 = Pos;
	m_pServer->Message_SendToObject(Drag, _iObj);
}; 


//Handles impulses
bool CAI_Core::OnImpulse(int _iImpulse, int _iSender, int _iObject, const CVec3Dfp32& _VecParam, void * _pData)
{
	//Check for server and character
 	if (!m_pServer || !m_pGameObject)
		return false;

	//ConOutL(CStrF("%s(%s) received impulse %d(%s)", m_pGameObject->GetTemplateName(), m_pGameObject->GetName(), _iImpulse, (char*)_pData));//DEBUG

	//Always activate bots when they get impulses
 	SetActivationState(STATE_ACTIVE);

	if ((_iImpulse >= ACTION_OVERRIDE_ENUM_BASE) &&
		(_iImpulse < PERSONALITY_ENUM_BASE))
	{
		//Bot will switch behaviour according to impulse. 
		//This frees the bot from any current script control

		//Assume pData is a char * and try to set behaviour
		SetAction(_iImpulse, (char *)_pData, -1, -1, _iObject);
		m_Script.Clear();
		return true; 
	}
	else switch (_iImpulse)
	{
	case IMPULSE_FOLLOW_ME:
		{
			//Bot will switch to script-mode and go to the position of the sender. 
			m_Script.SetPath(_iSender);
			return true;
		};
	case IMPULSE_FORCE_FOLLOW_ME:
		{
			//Bot will switch to script-mode and move towards the sender. 
			m_Script.SetPath(_iSender, CAI_ScriptHandler::FOLLOW_FORCE);
			return true;
		};
	case IMPULSE_FLY_FOLLOW:
		{
			//Bot moves normally to position of engine path, then jumps and flies along the engine path
			m_Script.SetPath(_iSender, CAI_ScriptHandler::FOLLOW_FLY);
			return true;
		};
	case IMPULSE_FLY_FOLLOW_TRUE:
		{
			//Bot moves to path position at top speed, then starts following the path as exactly as possible
			m_Script.SetPath(_iSender, CAI_ScriptHandler::FOLLOW_FLY | CAI_ScriptHandler::FOLLOW_TRUE);
			return true;
		};
	case IMPULSE_TELEPORT_FOLLOW:
		{
			//Bot moves to path position every frame
			m_Script.SetPath(_iSender, CAI_ScriptHandler::FOLLOW_TELEPORT);
			return true;
		};
	case IMPULSE_RELATIVE_FORCE_FOLLOW:
		{
			//Bot moves in the same fashion as the engine path, but ignores the exakt position of the path
			m_Script.SetPath(_iSender, CAI_ScriptHandler::FOLLOW_RELATIVE | CAI_ScriptHandler::FOLLOW_FORCE);
			return true;
		};
	case IMPULSE_FOLLOW_PATH:
		{	//Bot will switch to script-mode and go to the position of the given path, if any, or pause otherwise
			CWObject * pObj = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pObj)
				m_Script.SetPath(pObj->m_iObject);
			else
				m_Script.SetAction(CAI_ScriptHandler::CONTINUOUS_MOVE, CVec3Dfp32(0));
			return true;
		};
	case IMPULSE_ANIMATION:
		{
			//Bot will switch to script control and perform the given full body animation. 
			//If the argument is -1, then the current animation is aborted
			m_Script.SetAnimation(ResolveAnimHandle((char *)_pData));
			return true;
		};
	case IMPULSE_TORSO_ANIMATION:
		{
			//Bot will switch to script control and perform the given torso animation
			//If the argument is -1, then the current torso animation is aborted
			m_Script.SetAnimation(ResolveAnimHandle((char *)_pData), true);
			return true;
		};
	case IMPULSE_RUN_BEHAVIOUR:
		{	
			// We mask out the MSB (reserved for IMPULSE_FORCE_BEHAVIOUR)
			int iBehaviour = ((int)CStr((char *)_pData).Val_int()) & ~AG2_WANTEDBEHAVIORFLAG_MASKTYPE;
			SetWantedBehaviour2(iBehaviour,CAI_Action::PRIO_IDLE,0,-1,NULL);
			return true;
		};
	case IMPULSE_LOOP_BEHAVIOUR:
		{	
			// We mask out the MSB (reserved for IMPULSE_FORCE_BEHAVIOUR)
			int iBehaviour = ((int)CStr((char *)_pData).Val_int()) & ~AG2_WANTEDBEHAVIORFLAG_MASKTYPE;
			SetWantedBehaviour2(iBehaviour,CAI_Action::PRIO_IDLE,BEHAVIOUR_FLAGS_LOOP,-1,NULL);
			return true;
		};
		/*
	case IMPULSE_FORCE_BEHAVIOUR:
		{	// We add the MSB (telling AG to start the behaviour main anim right away)
			int iBehaviour = ((int)CStr((char *)_pData).Val_int()) | AG2_WANTEDBEHAVIORFLAG_MASKTYPE;
			SetWantedBehaviour2(iBehaviour,CAI_Action::PRIO_IDLE,0,-1,NULL);
			return true;
		};
		*/

	case IMPULSE_STOP_BEHAVIOUR:
		{
			if (_pData)
			{
				int iData = CStr((char *)_pData).Val_int();
				if (iData)
				{
					StopBehaviour(BEHAVIOUR_STOP_DIRECT,CAI_Action::PRIO_FORCED);
				}
				else
				{
					StopBehaviour(BEHAVIOUR_STOP_NORMAL,CAI_Action::PRIO_FORCED);
				}
			}
			else
			{	// Default is false
				StopBehaviour(BEHAVIOUR_STOP_NORMAL,CAI_Action::PRIO_FORCED);
			}
			return true;
		};
	case IMPULSE_SECURITY_TOLERANCE:
		{
			if (_pData)
			{
				int iData = CStr((char *)_pData).Val_int();
				m_SecurityTolerance = iData;
			}
			return true;
		};
	case IMPULSE_DAMAGE:
		{
			//Bot receives the given amount of damage, or health, if amount is negative
			int iDam = CStr((char *)_pData).Val_int();
			if (iDam > 0)
			{
				// RPG attributes are signed 16 bit integers
				if (iDam > 20000)
					iDam = 20000;
				CWO_DamageMsg Msg(iDam, DAMAGETYPE_UNDEFINED);
				Msg.Send(m_pGameObject->m_iObject, 0, m_pServer);
				OnTakeDamage(0,iDam,DAMAGETYPE_UNDEFINED);
			}
			else if (iDam < 0)
			{
				//Increase health with negative damage
				m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADDHEALTH, -iDam), m_pGameObject->m_iObject);
			};
			return true;
		};
	
	case IMPULSE_DIE:
		{	// The character dies, perhaps
			CStr Data = (char *)_pData;
			Data = Data.LowerCase();
			static const char* TranslateDieFlags[] =
			{
				"player","civs","toughguys","badguys","darklings","undead","all",NULL
			};
			bool bKill = false;
			int KillFlags = Data.TranslateFlags(TranslateDieFlags);
			if (KillFlags & DIE_FLAG_ALL)
			{
				bKill = true;
			}
			else
			{
				if (m_CharacterClass == CLASS_DEFAULT)
				{	// Player
					if (KillFlags & DIE_FLAG_PLAYER)
					{
						bKill = true;
					}
				}
				else if (m_CharacterClass == CLASS_CIV)
				{
					if (KillFlags & DIE_FLAG_CIV)
					{
						bKill = true;
					}
				}
				else if (m_CharacterClass == CLASS_TOUGHGUY)
				{
					if (KillFlags & DIE_FLAG_TOUGHGUY)
					{
						bKill = true;
					}
				}
				else if (m_CharacterClass == CLASS_BADGUY)
				{
					if (KillFlags & DIE_FLAG_BADGUY)
					{
						bKill = true;
					}
				}
				else if (m_CharacterClass == CLASS_DARKLING)
				{
					if (KillFlags & DIE_FLAG_DARKLING)
					{
						bKill = true;
					}
				}
				else if (m_CharacterClass == CLASS_UNDEAD)
				{
					if (KillFlags & DIE_FLAG_UNDEAD)
					{
						bKill = true;
					}
				}
			}
			
			if (bKill)
			{	// Handle character being immune
				/*
				int OrgMinHealth = m_MinHealth;
				m_MinHealth = 0;
				int32 Damage = MaxHealth()+1;
				CWO_DamageMsg Msg(Damage, DAMAGETYPE_UNDEFINED);
				Msg.Send(GetObjectID(), 0, m_pServer);
				OnDie();
				m_MinHealth = OrgMinHealth;
				*/
				int OrgMinHealth = m_MinHealth;
				m_MinHealth = 0;
				m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_ADDHEALTH,-2 * MaxHealth()));				
				OnTakeDamage(_iSender,MaxHealth(),DAMAGETYPE_UNDEFINED);
				m_MinHealth = OrgMinHealth;
			}

			return 1;
		};
	case IMPULSE_USEFLAGS:
		{	//These flags must match the order of the USE_XXX enums
			CStr Data = (char *)_pData;
			Data = Data.LowerCase();
			m_UseFlags = Data.TranslateFlags(TranslateUseFlags);
		};
		return 1;
	case IMPULSE_WALLCLIMB:
		{
			if (GetType() == TYPE_DARKLING)
			{
				fp32 climbTime = CStr((char *)_pData).Val_fp64();
				if (climbTime == 0)
				{
					//m_bCanWallClimb = false;
					SetWallclimb(false);
				}
				else
				{
					//m_bCanWallClimb = true;
					if (climbTime == -1)
					{	// Wallclimb forever
						SetWallclimb(true);
					}
					else
					{	// Wallclimb for a while
						TempEnableWallClimb((int)(climbTime * GetAITicksPerSecond()));
					}
				}
			}
			return 1;
		};

	case IMPULSE_UNDEAD_STUN:
		{
			fp32 stunTime = CStr((char *)_pData).Val_fp64();
			if (stunTime != -1.0f)
			{
				if (stunTime == 0)
				{
					SetUndeadStun(0);
				}
				else
				{
					m_StunTimeout = (int)(stunTime * GetAITicksPerSecond());
				}
			}
			else
			{
				SetUndeadStun(-1,false);
				m_StunTimeout = -1;
			}
			return 1;
		};

	case IMPULSE_PUSH:
		{	
			// Push in direction of sending object or given object if any.
			CStr Data = (char*)_pData;
			// Scale from 20hz
			fp32 Speed = (fp32)Data.GetStrSep(",").Val_fp64() * 20.0f * m_pServer->GetGameTickTime();
			int32 Type = Data.GetStrSep(",").Val_int();
			CStr Pusher = Data.GetStrSep(",");
			CWObject * pPusher = (Pusher != "") ? GetSingleTarget(_iObject, Pusher, _iSender) : m_pServer->Object_Get(_iSender);
			if (pPusher)
			{
				CVec3Dfp32 Dir;
				if (Type == PUSH_AWAY)
				{
					Dir = m_pGameObject->GetPosition() - pPusher->GetPosition();
				}
				else
				{
					Dir = CVec3Dfp32::GetRow(pPusher->GetPositionMatrix(), 0);
				}
				Push(Speed, Dir);
			}
			return true;
		};

	case IMPULSE_GHOSTMODE:	
		{
			//Bot becomes a ghost, physics-wise, or switches back to normal if param is not 1
			m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_GHOST, CStr((char *)_pData).Val_int()));
			return true;
		};

	case IMPULSE_AI_VOICE:
		{
			int iSound = CStr((char *)_pData).TranslateInt(CAI_Device_Sound::ms_lTranslateSpeech);
			int Prio = CAI_Action::PRIO_COMBAT;
			if (iSound != -1)
			{
				UseRandom("IMPULSE_AI_VOICE",iSound,Prio);
			}
			else
			{
				iSound = CStr((char *)_pData).Val_int();
				UseRandom("IMPULSE_AI_VOICE",iSound,Prio);
			}
			return true;
		};

	case IMPULSE_DETECT:
		{
			//Bot spots the given objects
			TArray<int16> liObjs;

			//Add any given targets
			if (_pData)
			{
				//Reserved target names must be handled differently
				int iSpecialTarget = CWO_SimpleMessage::ResolveSpecialTargetName((const char *)_pData);
				if (iSpecialTarget >= 0)
				{
					if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ACTIVATOR)
					{
						//Spot activator, which should be the supplied onject ID
						liObjs.Add(_iObject);			
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_THIS)
					{
						//Spot sender
						liObjs.Add(_iSender);			
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_PLAYER)
					{
						liObjs.Add(GetClosestPlayer());
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ALLPLAYERS)
					{
						AddPlayers(m_pServer, liObjs);
					}
					else
					{
						//Can't handle this reserved name
						return true;
					};
				}
				else
				{
					//Spot targets
					GetTargetIndices((char *)_pData, m_pServer, liObjs); 
				};
			}
			//Add any given object index if there were no target name specified
			else if (_iObject)
			{
				liObjs.Add(_iObject);
			};

			//Spot all characters found
			for (int i = 0; i < liObjs.Len(); i++)
			{
				if (IsValidAgent(liObjs[i]))
				{
					CAI_AgentInfo* pInfo = m_KB.GetAgentInfo(liObjs[i]);
					if (!pInfo)
					{	// Add agent
						pInfo = m_KB.AddAgent(liObjs[i]);
					}
					if (pInfo)
					{	//Character is in character list
						//Spot with no position uncertainty
						pInfo->SetCorrectSuspectedPosition();
						pInfo->SetAwareness(CAI_AgentInfo::DETECTED,true,true);
						pInfo->TouchLastSpotted();
					}
				};
			};
			return true;
		};
	case IMPULSE_UNSPOT_ALL:
		{
			//Bot clears awareness of all agents
			CAI_AgentInfo * pInfo;
			bool orgPauseActions = m_PauseActions;
			m_PauseActions = false;
			for (int i = 0; i < m_KB.NumAgentInfo(); i++)
			{
				pInfo = m_KB.IterateAgentInfo(i);
				if (pInfo)
					pInfo->SetAwareness(CAI_AgentInfo::NONE,false,true);
			}
			m_PauseActions = orgPauseActions;
			SetStealthTension(TENSION_NONE);
			m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_IDLE,TruncToInt(Random * 6.999f));
		}
		return true;

	case IMPULSE_NOTICE_PLAYER_AT:
		{
			//Bot spots player at iLocationObj pos
			int iLocationObj = 0;

			//Add any given targets
			if (_pData)
			{
				//Reserved target names must be handled differently
				int iSpecialTarget = CWO_SimpleMessage::ResolveSpecialTargetName((const char *)_pData);
				if (iSpecialTarget >= 0)
				{
					if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ACTIVATOR)
					{
						//Spot at activator, which should be the supplied onject ID
						iLocationObj = _iObject;			
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_THIS)
					{
						//Spot sender
						iLocationObj = _iSender;
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_PLAYER)
					{
						//Spot player
						iLocationObj = GetClosestPlayer();
					}
					else
					{
						//Can't handle this reserved name
						return true;
					};
				}
				else
				{
					TArray<int16> liObjects;
					GetTargetIndices((char *)_pData, m_pServer,liObjects);
					if (liObjects.Len())
					{
						iLocationObj = liObjects[(int)(Random * 0.999f * liObjects.Len())];
					}
				};
			}

			// Get the index of the player
			int iPlayer = GetClosestPlayer();
			CAI_AgentInfo* pInfo = m_KB.GetAgentInfo(iPlayer);
			if (!pInfo)
			{
				//Character is not in character list, add him as spotted
				pInfo = m_KB.AddAgent(iPlayer,CAI_AgentInfo::INFOFLAGS_NONE,CAI_AgentInfo::NONE);
			}
			if (!pInfo)
			{	// We bail out
				// (We return true as we are the only one that could possibly understand this impulse
				return(true);
			}

			if (iLocationObj)
			{	// Find the position of iLocationObj
				CWObject* pObj = m_pServer->Object_Get(iLocationObj);
				if (!pObj)
				{
					return(true);
				}

				CVec3Dfp32 listenerPos = m_pGameObject->GetPosition();
				CVec3Dfp32 soundPos = pObj->GetPosition();
				// NOTE: No vertical exaggeration for NOTICE_PLAYER_AT
				if ((listenerPos.DistanceSqr(soundPos) <= Sqr(m_HearingRange))&&(pInfo->GetAwareness() <= CAI_AgentInfo::NOTICED))
				{
					pInfo->SetAwareness(CAI_AgentInfo::NOTICED,true,true);
					pInfo->SetSuspectedPosition(soundPos);
					OnTrackPos(soundPos,10,false,false);
				}
			}
			return true;
		};

	case IMPULSE_INVESTIGATE_OBJ:
		{	// Find the object
			CWObject * pObj = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pObj)
			{
				m_AH.SetInvestigateObject(pObj->m_iObject);
			}
			else
			{
				m_AH.SetInvestigateObject(0);
			}
			return true;
		};

	case IMPULSE_SET_ENEMY_RELATION:
		{
			//Bot becomes an enemy of the given objects
			TArray<int16> liObjs;

			//Add any given targets
			if (_pData)
			{
				//Reserved target names must be handled differently
				int iSpecialTarget = CWO_SimpleMessage::ResolveSpecialTargetName((const char *)_pData);
				if (iSpecialTarget >= 0)
				{
					if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ACTIVATOR)
					{
						//Spot activator, which should be the supplied onject ID
						liObjs.Add(_iObject);			
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_THIS)
					{
						//Spot sender
						liObjs.Add(_iSender);			
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_PLAYER)
					{
						liObjs.Add(GetClosestPlayer());
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ALLPLAYERS)
					{
						AddPlayers(m_pServer, liObjs);
					}
					else
					{
						//Can't handle this reserved name
						return true;
					};
				}
				else
				{
					//Spot targets
					GetTargetIndices((char *)_pData, m_pServer, liObjs); 
				};
			}
			//Add any given object index if there were no target name specified
			else if (_iObject)
			{
				liObjs.Add(_iObject);
			};

			//Spot all characters found
			for (int i = 0; i < liObjs.Len(); i++)
			{
				if (IsValidAgent(liObjs[i]))
				{
					CAI_AgentInfo* pInfo = m_KB.GetAgentInfo(liObjs[i]);
					if (!pInfo)
					{	// Add agent
						pInfo = m_KB.AddAgent(liObjs[i]);
					}
					if (pInfo)
					{	
						pInfo->SetRelation(CAI_AgentInfo::ENEMY);
					}
				};
			};
			return true;
		};
	case IMPULSE_SET_FRIEND_RELATION:
		{
			//Bot becomes an enemy of the given objects
			TArray<int16> liObjs;

			//Add any given targets
			if (_pData)
			{
				//Reserved target names must be handled differently
				int iSpecialTarget = CWO_SimpleMessage::ResolveSpecialTargetName((const char *)_pData);
				if (iSpecialTarget >= 0)
				{
					if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ACTIVATOR)
					{
						//Spot activator, which should be the supplied onject ID
						liObjs.Add(_iObject);			
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_THIS)
					{
						//Spot sender
						liObjs.Add(_iSender);			
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_PLAYER)
					{
						liObjs.Add(GetClosestPlayer());
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ALLPLAYERS)
					{
						AddPlayers(m_pServer, liObjs);
					}
					else
					{
						//Can't handle this reserved name
						return true;
					};
				}
				else
				{
					//Spot targets
					GetTargetIndices((char *)_pData, m_pServer, liObjs); 
				};
			}
			//Add any given object index if there were no target name specified
			else if (_iObject)
			{
				liObjs.Add(_iObject);
			};

			//Spot all characters found
			for (int i = 0; i < liObjs.Len(); i++)
			{
				if (IsValidAgent(liObjs[i]))
				{
					CAI_AgentInfo* pInfo = m_KB.GetAgentInfo(liObjs[i]);
					if (!pInfo)
					{	// Add agent
						pInfo = m_KB.AddAgent(liObjs[i]);
					}
					if (pInfo)
					{	
						pInfo->SetRelation(CAI_AgentInfo::FRIENDLY);
					}
				};
			};
			return true;
		};
	case IMPULSE_SET_DEFAULT_RELATION:
		{
			//Bot becomes an enemy of the given objects
			TArray<int16> liObjs;

			//Add any given targets
			if (_pData)
			{
				//Reserved target names must be handled differently
				int iSpecialTarget = CWO_SimpleMessage::ResolveSpecialTargetName((const char *)_pData);
				if (iSpecialTarget >= 0)
				{
					if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ACTIVATOR)
					{
						//Spot activator, which should be the supplied onject ID
						liObjs.Add(_iObject);			
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_THIS)
					{
						//Spot sender
						liObjs.Add(_iSender);			
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_PLAYER)
					{
						liObjs.Add(GetClosestPlayer());
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ALLPLAYERS)
					{
						AddPlayers(m_pServer, liObjs);
					}
					else
					{
						//Can't handle this reserved name
						return true;
					};
				}
				else
				{
					//Spot targets
					GetTargetIndices((char *)_pData, m_pServer, liObjs); 
				};
			}
			//Add any given object index if there were no target name specified
			else if (_iObject)
			{
				liObjs.Add(_iObject);
			};

			//Spot all characters found
			for (int i = 0; i < liObjs.Len(); i++)
			{
				if (IsValidAgent(liObjs[i]))
				{
					CAI_AgentInfo* pInfo = m_KB.GetAgentInfo(liObjs[i]);
					if (!pInfo)
					{	// Add agent
						pInfo = m_KB.AddAgent(liObjs[i]);
					}
					if (pInfo)
					{
						int DefaultRelation = m_KB.DefaultRelation(liObjs[i]);
						pInfo->SetRelation(DefaultRelation);
					}
				};
			};
			return true;
		};
	case IMPULSE_HOSTILE_RELATION:
		{
			//Bot becomes an enemy of the given objects
			TArray<int16> liObjs;

			//Add any given targets
			if (_pData)
			{
				//Reserved target names must be handled differently
				int iSpecialTarget = CWO_SimpleMessage::ResolveSpecialTargetName((const char *)_pData);
				if (iSpecialTarget >= 0)
				{
					if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ACTIVATOR)
					{
						//Spot activator, which should be the supplied onject ID
						liObjs.Add(_iObject);			
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_THIS)
					{
						//Spot sender
						liObjs.Add(_iSender);			
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_PLAYER)
					{
						liObjs.Add(GetClosestPlayer());
					}
					else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ALLPLAYERS)
					{
						AddPlayers(m_pServer, liObjs);
					}
					else
					{
						//Can't handle this reserved name
						return true;
					};
				}
				else
				{
					//Spot targets
					GetTargetIndices((char *)_pData, m_pServer, liObjs); 
				};
			}
			//Add any given object index if there were no target name specified
			else if (_iObject)
			{
				liObjs.Add(_iObject);
			};

			//Spot all characters found
			for (int i = 0; i < liObjs.Len(); i++)
			{
				if (IsValidAgent(liObjs[i]))
				{
					CAI_AgentInfo* pInfo = m_KB.GetAgentInfo(liObjs[i]);
					if (!pInfo)
					{	// Add agent
						pInfo = m_KB.AddAgent(liObjs[i]);
					}
					if (pInfo)
					{
						int CurRelation = pInfo->GetRelation();
						if ((CurRelation < CAI_AgentInfo::HOSTILE)&&(CurRelation >= CAI_AgentInfo::UNFRIENDLY))
						{
							pInfo->SetRelation(CAI_AgentInfo::HOSTILE);
						}
					}
				};
			};
			return true;
		};

	case IMPULSE_SET_MIN_STANCE:
		{
			CStr Value = (char *)_pData;
			int Valuei = Value.Val_int();
			if (Value.CompareNoCase("IDLE") == 0)
			{
				int rnd = TruncToInt(Random * 6.999f);
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_IDLE,false);
				m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_IDLE,rnd);
				
			}
			else if (Value.CompareNoCase("HOSTILE") == 0)
			{
				m_DeviceSound.PauseType(CAI_Device_Sound::IDLE_TO_WARY,5.0f);
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_HOSTILE,true);
			}
			else if (Value.CompareNoCase("COMBAT") == 0)
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_COMBAT,true);
			}
			else if (Valuei == 0)
			{
				int rnd = TruncToInt(Random * 6.999f);
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_IDLE,false);
				m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_IDLE,rnd);
			}
			else if (Valuei == 1)
			{
				m_DeviceSound.PauseType(CAI_Device_Sound::IDLE_TO_WARY,5.0f);
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_HOSTILE,true);
			}
			else if (Valuei == 2)
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_COMBAT,true);
			}
			else
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_IDLE,true);
			}
			return true;
		}
	case IMPULSE_SET_MAX_STANCE:
		{
			CStr Value = (char *)_pData;
			int Valuei = Value.Val_int();
			if (Value.CompareNoCase("IDLE") == 0)
			{
				m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_IDLE,true);
			}
			else if (Value.CompareNoCase("HOSTILE") == 0)
			{
				m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_HOSTILE,true);
			}
			else if (Value.CompareNoCase("COMBAT") == 0)
			{
				m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_COMBAT,true);
			}
			else if (Valuei == 0)
			{
				m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_IDLE,true);
			}
			else if (Valuei == 1)
			{
				m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_HOSTILE,true);
			}
			else if (Valuei == 2)
			{
				m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_COMBAT,true);
			}
			else
			{
				m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_IDLE,true);
			}
			return true;
		}
	case IMPULSE_SET_DRAGOBJECT:
		{
			//Bot looks continuosly at object
			CWObject * pObj = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pObj)
			{
				m_iDragObject = pObj->m_iObject;
			}
			else
			{
				//Reset
				m_iDragObject = 0;
			}
			return true;
		};
	case IMPULSE_SET_NEXTOFKIN:
		{
			int nextOfKin = CStr((char *)_pData).Val_int();
			if (nextOfKin)
			{
				m_bNextOfKin = true;
			}
			else
			{
				m_bNextOfKin = false;
			}
			return true;
		};
	case IMPULSE_RELEASE_SP:
		{
			// Expires all currently held scenepoints
			// Ignore if  _pData is true and we are the sender
			int iData = CStr((char *)_pData).Val_int();
			if ((iData)||(_iSender != GetObjectID()))
			{	// Tell AM to invalidate all held scenepoints for 1 tick
				m_AH.ReleaseSPs(1);
			}
			return true;
		};
	case IMPULSE_TELEPORT_TO_SP:
		{
			// PSEUDO
			// Find SP name from _pData and if several hits, pick one at random.
			// Then teleport the bot to the gridpos nearest the SP pos.
			CWO_ScenePointManager* pSPM = GetScenePointManager();
			if (pSPM)
			{
				CWO_ScenePoint* pSP = pSPM->FindRandom(m_pGameObject->m_iObject,(char *)_pData);
				if (pSP)
				{
					// *** NOTE This is a temp fix until ANton fixes the regular TELEPORT message ***
					CMat4Dfp32 PosMat = pSP->GetPositionMatrix();
					CVec3Dfp32 Pos;
					if (pSP->GetSqrRadius() <= Sqr(8.0f))
					{
						Pos = PosMat.GetRow(3);
					}
					else
					{
						Pos = pSP->GetPathPosition(m_pServer);
					}
					if (m_CharacterClass == CLASS_DARKLING)
					{	// Up a bit (up in the sense of the scenepoint)
						Pos += PosMat.GetRow(2) * 16.0f;
					}
					Pos.SetRow(PosMat,3);
					
					// Teleport there
					CWObject_Message Msg(OBJSYSMSG_TELEPORT);
					Msg.m_Param0 = 3;	// Param0 == 3 means we supply the matrix as a simple ptr
					Msg.m_pData = &PosMat;
					Msg.m_DataSize = sizeof(PosMat);
					m_pServer->Message_SendToObject(Msg,GetObjectID());

					//Teleport! Stop regular movement though
					// Do NOT try to force m_iObject smeowhere near the SP, if the teleport cannot
					// put m_iObject on the SP the SP is badly placed and the call should fail
					m_AH.ExpireActions(CAI_Action::INVALID);
					m_DeviceMove.Use(0);
					StopNow();
					ResetPathSearch();
					m_CurBaseMat = PosMat;
					m_CurHeadMat = m_CurBaseMat;
					m_CurHeadMat.GetRow(3) += CVec3Dfp32(0.0f,0.0f,GetCurHeight());
					m_CurLookPos = m_CurHeadMat.GetRow(3);
					m_CurLookDir = m_CurHeadMat.GetRow(0);	
					m_AH.HandleSitFlag(pSP);
					m_AH.ActivateScenePoint(pSP,CAI_Action::PRIO_FORCED);
					ResetStuckInfo();
				}
			}
			return true;
		};
	case IMPULSE_JUMP_TO_SP:
		{
			// PSEUDO
			// Find SP name from _pData and if several hits, pick one at random.
			// Then teleport the bot to the gridpos nearest the SP pos.
			CWO_ScenePointManager* pSPM = GetScenePointManager();
			if (pSPM)
			{
				CWO_ScenePoint* pSP = pSPM->FindRandom(GetObjectID(),(char *)_pData);
				if ((pSP)&&(pSP->PeekRequest(GetObjectID(),m_pServer)))
				{
					CMat4Dfp32 PosMat = pSP->GetPositionMatrix();
					//Jump!
					if ((m_bCanJump)&&(JUMPREASON_SCRIPT & m_JumpFlags)&&
						(SqrDistanceToUs(PosMat.GetRow(3)) <= Sqr(m_JumpMaxRange))&&
						(SqrDistanceToUs(PosMat.GetRow(3)) >= Sqr(m_JumpMinRange)))
					{
						TempEnableWallClimb(m_Patience);
						int32 Flags = JUMP_CHECK_COLL | JUMP_FROM_AIR;
						if (pSP->GetType() & CWO_ScenePoint::JUMP_CLIMB)
						{// Check behaviour too?
							Flags |= JUMP_TO_AIR;
						}
						int _Reason = CAI_Core::JUMPREASON_SCRIPT;
						if (pSP->GetType() & CWO_ScenePoint::TACTICAL)
						{	// If this voice works it will silence the script voice from AddJumpTowardsPositionCmd
							UseRandom(CStr("Darkling combat jump"),CAI_Device_Sound::COMBAT_ATTACKJUMP,CAI_Action::PRIO_COMBAT);
						}
						if (AddJumpTowardsPositionCmd(&PosMat,_Reason,Flags))
						{
#ifndef M_RTM
							m_AH.DebugDrawScenePoint(pSP,true,0,5.0f);
#endif
							m_AH.ExpireActions(CAI_Action::INVALID);
							ResetPathSearch();
							m_AH.m_pRetargetSP = pSP;
							m_AH.m_RetargetPos = m_AH.m_pRetargetSP->GetPosition();
							m_AH.m_RetargetUp = m_AH.m_pRetargetSP->GetUp();
						}
						else
						{
#ifndef M_RTM
							if (DebugTarget())
							{
								CStr UserName = m_pGameObject->GetName();
								CStr SpName = (char *)_pData;
								ConOut(CStr(UserName+CStr(": JumpToScenepoint failed. Collided ")+ SpName));
							}
#endif
						}
					}
					else
					{
#ifndef M_RTM
						if (DebugTarget())
						{
							CStr UserName = m_pGameObject->GetName();
							CStr SpName = (char *)_pData;
							if ((!m_bCanJump)||(!(JUMPREASON_SCRIPT & m_JumpFlags)))
							{
								ConOut(CStr(UserName+": JumpToScenepoint failed. Cannot jump "+ SpName));
							}
							else
							{
								if (SqrDistanceToUs(PosMat.GetRow(3)) > Sqr(m_JumpMaxRange))
								{
									ConOut(CStr(UserName+": JumpToScenepoint failed. Too far "+ SpName));
								}
								else
								{
									ConOut(CStr(UserName+": JumpToScenepoint failed. Too close "+ SpName));
								}
							}
						}
#endif
					}
				}
				else
				{
#ifndef M_RTM
					if (DebugTarget())
					{
						CStr UserName = m_pGameObject->GetName();
						CStr SpName = (char *)_pData;
						ConOut(CStr(UserName+CStr(": JumpToScenepoint failed. Failed to find SP: ")+ SpName));
					}
#endif
				}

			}
			return true;
		};
	case IMPULSE_RELEASE:
		{
			//Bot will be released from script control
			m_Script.Clear();
			SetActivationState(STATE_ACTIVE);
			return true;
		};
	case IMPULSE_PAUSE:
		{	//Bot will stop doing anything until released or otherwise ordered to do something
			m_Script.Clear();
			m_Script.SetPause(true);
			return true;
		};
	case IMPULSE_UNPAUSE:
		{	//Bot moves according to given move-delta until a new move impulse is sent or bot is released
			m_Script.SetPause(false);
			return true;
		};
	case IMPULSE_LOOK:
		{
			//Bot looks continuosly at object
			CWObject* pObj = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pObj)
			{
				m_iLookAtObject = pObj->m_iObject;
				m_bLookSoft = false;	
			}
			else
			{
				//Reset
				m_iLookAtObject = 0;
				m_bLookSoft = false;	
			}
			return true;
		};
	case IMPULSE_SOFTLOOK:
		{
			//Bot looks continuosly at object even when taking some actions
			//Bot should only look as far as movement direction allows without having to strafe or move backwards
			CWObject * pObj = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pObj)
			{
				m_iLookAtObject = pObj->m_iObject;
				m_bLookSoft = true;	
			}
			else
			{
				//Reset
				m_iLookAtObject = 0;
				m_bLookSoft = false;	
			}
			return true;
		};
	case IMPULSE_CROUCH:
		{
			// Bot crouches if pData is nonzero and stops crouching if zero
			CWObject_Interface_AI* pAI = m_pGameObject->GetInterface_AI();
			if((pAI)&&(pAI->AI_IsAlive()))
			{
				if (_pData)
				{
					int iData = CStr((char *)_pData).Val_int();
					if (iData)
					{
						m_DeviceStance.Crouch(true);
					}
					else
					{
						m_DeviceStance.Crouch(false);
					}
				}
				else
				{	// Default is true
					m_DeviceStance.Crouch(true); 
				}
			}
			return true;
		};
	case IMPULSE_JUMP:
		{
			//Bot jumps once
			m_Script.SetAction(CAI_ScriptHandler::SINGLE_JUMP);
			return true;
		};
	case IMPULSE_SWITCH_WEAPON:
		{
			//Bot switches to given weapon or to next weapon if no weapon index is given
			m_Script.SetAction(CAI_ScriptHandler::SINGLE_SWITCH_WEAPON, CStr((char *)_pData).Val_int());
			return true;
		};
	case IMPULSE_SWITCH_ITEM:
		{
			//Bot switches to given item or to next item if no ai item type string is given
			//Note that param should be a string with the ai_type string, -1 (previous) or 0 (next)
			int Id; 
			CStr ItemId = CStr((char *)_pData);
			if (ItemId == "0")
				//Switch to next
				Id = 0;
			else if (ItemId == "-1")
				//Switch to previous
				Id = -1;
			else
			{
				//Switch to given item type
				Id = m_Item.GetItemType(ItemId);
				if (Id == CAI_Item::UNKNOWN)
					//Don't switch weapon
					return true;
			}
			m_Script.SetAction(CAI_ScriptHandler::SINGLE_SWITCH_ITEM, Id);
			return true;
		};
	case IMPULSE_USE_WEAPON:
		{
			//Bot uses wepon once
			int UseTime = (int)(CStr((char *)_pData).Val_fp64() * GetAITicksPerSecond());
			m_Script.SetAction(CAI_ScriptHandler::SINGLE_USE_WEAPON, UseTime);
			return true;
		};
	case IMPULSE_USE_ITEM:
		{
			//Bot uses item once
			int UseTime = (int)(CStr((char *)_pData).Val_fp64() * GetAITicksPerSecond());
			m_Script.SetAction(CAI_ScriptHandler::SINGLE_USE_ITEM, UseTime);
			return true;
		};
	case IMPULSE_MOVE_TO:
		{
			//Bot navigates to position of given object
			//Note that giving jump or crouch impulses while conducting this move might cause the movement system to fail in a few cases.
			CWObject * pObj = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pObj)
				m_Script.SetAction(CAI_ScriptHandler::MOVE_TO, pObj->GetPosition());
			return true;
		};
	case IMPULSE_MOVE_TO_SCENEPOINT:
		{
			m_Script.SetAction(CAI_ScriptHandler::MOVE_TO_SCENEPOINT,(char *)_pData);
			return true;
		};
	case IMPULSE_LOOK_AT:
		{
			//Bot tracks given object's focus position
			CWObject * pObj = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pObj)
				m_Script.SetAction(CAI_ScriptHandler::LOOK_AT, pObj->m_iObject);
			else
				m_Script.SetAction(CAI_ScriptHandler::LOOK_AT, 0);	
			return true;
		};
	case IMPULSE_SNAP_LOOK_AT:
		{
			//Bot instantly looks at given objects focus position
			CWObject * pObj = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pObj)
				m_Script.SetAction(CAI_ScriptHandler::SNAP_LOOK_AT, pObj->m_iObject);
			else
				m_Script.SetAction(CAI_ScriptHandler::SNAP_LOOK_AT, 0);	
			return true;
		};
	case IMPULSE_TARGET:
		{
			CWObject * pObj = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pObj)
			{
				m_AH.SetEnemyOverride(pObj->m_iObject);
			}
			else
			{
				m_AH.SetEnemyOverride(0);
			}
		}
		break;
	case IMPULSE_ATTACK:
		{
			//Bot aims at given objects focus position and makes a single attack (with possible deviation 
			//due to lack of precision) when weapon is ready for use. 
			CWObject * pObj = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pObj)
				m_Script.SetAction(CAI_ScriptHandler::ATTACK, pObj->m_iObject);
			else
				m_Script.SetAction(CAI_ScriptHandler::ATTACK, 0);
			return true;
		};
	case IMPULSE_AIM:
		{
			//Bot continuosly aims at given objects focus position, gaining precision as normal
			CWObject * pObj = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pObj)
				m_Script.SetAction(CAI_ScriptHandler::AIM, pObj->m_iObject);
			else
				m_Script.SetAction(CAI_ScriptHandler::AIM, 0);
			return true;
		};
/*
	case IMPULSE_FORCE_USE_WEAPON:
		{
			//Bot uses weapon immediately regardless of any normal restrictions such as rate of fire
			int UseTime = (int)(CStr((char *)_pData).Val_fp64() * GetAITicksPerSecond());
			m_Script.ForcedAction(CAI_ScriptHandler::FORCE_USE_WEAPON, UseTime);
			return true;
		};
*/
	case IMPULSE_FORCE_SHOOT:
		{
			//Bot makes an attack immediately regardless of any normal restrictions towards the given target without changing heading. This is only effective for ranged weapons, of course.
			CWObject * pTarget = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pTarget)
				m_Script.ForcedAction(CAI_ScriptHandler::FORCE_SHOOT, pTarget->m_iObject);
			return true;
		};

	case IMPULSE_AGGRESSIVE:
		{
			//Bot becomes aggressive and will attack the given target, or any valid target, without moving
			CWObject* pTarget = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pTarget)
			{
				m_Script.SetAction(CAI_ScriptHandler::AGGRESSIVE,pTarget->m_iObject);
			}
			else
			{
				m_Script.SetAction(CAI_ScriptHandler::AGGRESSIVE,0);
			}
			return true;
		};
	case IMPULSE_PROJ_INVULNERABLE:
		{	// Bot is invulnerable to projectile attacks
			if (CStr((char *)_pData).Val_int())
			{	// Set flag
				m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETFLAGS,PLAYER_FLAGS_PROJECTILEINVULNERABLE),m_pGameObject->m_iObject);
			}
			else
			{	// Clear flag (Yes, the bizarre way of setting clearing flags is correct, I didn't dream it up!)
				m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETFLAGS,0,PLAYER_FLAGS_PROJECTILEINVULNERABLE),m_pGameObject->m_iObject);
			}
		};
	case IMPULSE_ALLOW_ATTACKS:
		{
			if (CStr((char *)_pData).Val_int())
			{
				m_Script.SetHoldAttacks(false);
			}
			else
			{
				m_Script.SetHoldAttacks(true);
			}
			return true;
		};
	case IMPULSE_CHARGE:
		{	// Expire COMBAT for a while
			// Expire any held CBSPs
			int durationTicks = int32(fp32(CStr((char *)_pData).Val_fp64()) * GetAITicksPerSecond());
			if (durationTicks < 0) { durationTicks = -1;}
			GiveOrder(ORDER_CHARGE,durationTicks);
			return true;
		};
	case IMPULSE_COVER:
		{
			// Expire WEAPONATTACKRANGED for a while
			// Expire any held CBSPs
			int durationTicks = int32(fp32(CStr((char *)_pData).Val_fp64()) * GetAITicksPerSecond());
			if (durationTicks < 0) { durationTicks = -1;}
			GiveOrder(ORDER_COVER,durationTicks);
			return true;
		};
	case IMPULSE_FLANK:
		{
			// Expire any held CBSPs
			int durationTicks = int32(fp32(CStr((char *)_pData).Val_fp64()) * GetAITicksPerSecond());
			if (durationTicks < 0) { durationTicks = -1;}
			GiveOrder(ORDER_FLANK,durationTicks);
			return true;
		};
	case IMPULSE_ESCALATE:
		{
			// Expire COMBAT for a while
			// Expire any held CBSPs
			int durationTicks = int32(fp32(CStr((char *)_pData).Val_fp64()) * GetAITicksPerSecond());
			if (durationTicks < 0) { durationTicks = -1;}
			GiveOrder(ORDER_ESCALATE,durationTicks);
			return true;
		};
	case IMPULSE_TURRET_MOUNT:
		{	// Bot is either mounted to a turret or released from same
			if (!IsPlayer())
			{
				CWObject * pObj = m_pServer->Object_Get(_iObject);
				if (pObj)
				{	// Set turret mount
					m_Script.SetAction(CAI_ScriptHandler::TURRET_MOUNT,pObj->m_iObject);
				}
				else
				{	// Release turret mount
					m_Script.SetAction(CAI_ScriptHandler::TURRET_MOUNT,0);
				}
			}
			return true;
		};
	case IMPULSE_SCENEPOINTOVERRIDE:
		{
			//Bot will try to take given scenepoint next time a scenepoint of matching type is required./
			m_AH.SetOverrideScenePoint(CStr((char *)_pData));
			return true;
		}
	case IMPULSE_ADD_TEAM:
		{
			//Add team. Note that other characters may remain unaware that the agent 
			//has joined another team until agent does something to get their attention.
			CWObject * pTeam = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pTeam)
			{
				m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_TEAM_JOINTEAM, pTeam->m_iObject),m_pGameObject->m_iObject);
#ifndef M_RTM
				if (DebugTarget())
				{
					CStr Name = CStr("§c3f0") +  m_pGameObject->GetName();
					ConOutL(Name+CStr(" team added ")+pTeam->GetName());
				}
#endif
			}

			return true;
		};
	case IMPULSE_REMOVE_TEAM:
		{
			//Note that other characters may remain unaware that the agent 
			//has left team until agent does something to get their attention.
			CWObject * pTeam = GetSingleTarget(_iObject, (char *)_pData, _iSender);
			if (pTeam)
			{
				if (m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_TEAM_LEAVETEAM, pTeam->m_iObject),m_pGameObject->m_iObject))
				{
					m_AH.OnRemoveTeam(pTeam->m_iObject);
#ifndef M_RTM
					if (DebugTarget())
					{
						CStr Name = CStr("§c3f0") +  m_pGameObject->GetName();
						ConOutL(Name+CStr(" team removed ")+pTeam->GetName());
					}
#endif
				}
			}
			return true;
		};
	case IMPULSE_AWARENESS:
		{
			//Bot changes awareness
			m_Awareness = Max(0.0f, (fp32)CStr((char *)_pData).Val_fp64());
			return true;
		};
	case IMPULSE_SIGHTRANGE:
		{
			//Bot changes sight range
			m_SightRange = Max(0.0f, (fp32)CStr((char *)_pData).Val_fp64());
			return true;
		};
	case IMPULSE_HEARINGRANGE:
		{
			//Bot changes hearing range
			m_HearingRange = Max(0.0f, (fp32)CStr((char *)_pData).Val_fp64());
			return true;
		};
	case IMPULSE_DEBUG:
		{
			// *** We hijack this unused IMPULSE for testing purposes ***
			if ((!m_pGameObject->AI_IsSpawned()||(!m_pGameObject->AI_IsAlive())))
			{
				return true;
			}
			/*AG2_IMPACTDIRECTION_NONE = 0,
			AG2_IMPACTDIRECTION_FRONT = 1,
			AG2_IMPACTDIRECTION_BACK = 2,
			AG2_IMPACTDIRECTION_LEFT = 3,
			AG2_IMPACTDIRECTION_RIGHT = 4,*/
			static int32 hitDirection = AG2_IMPACTDIRECTION_NONE;

			/*AG2_STAGGERDIRECTION_FORWARD = 0,
			AG2_STAGGERDIRECTION_BACK = 1,
			AG2_STAGGERDIRECTION_LEFT = 2,
			AG2_STAGGERDIRECTION_RIGHT = 3,*/
			static int32 staggerDirection = AG2_STAGGERDIRECTION_FORWARD;

			/*AG2_STAGGER_RANGE_NONE = 0,
			AG2_STAGGER_RANGE_ONPLACE = 1,
			AG2_STAGGER_RANGE_SHORT = 2,
			AG2_STAGGER_RANGE_LONG = 3,*/
			static int32 staggerRange = AG2_STAGGER_RANGE_NONE;

			/*AG2_IMPACTLOCATION_NONE = 0,
			AG2_IMPACTLOCATION_HIGH = 1,
			AG2_IMPACTLOCATION_MID = 2,
			AG2_IMPACTLOCATION_LOW = 3,*/
			static int32 hitLocation = AG2_IMPACTLOCATION_NONE;

			/*AG2_IMPULSEVALUE_HURTTYPE_SHORT = 1,
			AG2_IMPULSEVALUE_HURTTYPE_LONG = 2,
			AG2_IMPULSEVALUE_HURTTYPE_EXTREME = 3,
			AG2_IMPULSEVALUE_HURTTYPE_DEAD = 4,*/
			static int32 hurtType = AG2_IMPULSEVALUE_HURTTYPE_SHORT;
			
			// Increment stuff
			int incrementWhat = int32(CStr((char *)_pData).Val_int());
			switch(incrementWhat)
			{
				case 1:
				{
					hitDirection++;
					if (hitDirection > AG2_IMPACTDIRECTION_RIGHT)
					{
						hitDirection = AG2_IMPACTDIRECTION_NONE;
					}
				}
				break;

				case 2:
				{
					staggerDirection++;
					if (staggerDirection > AG2_STAGGERDIRECTION_RIGHT)
					{
						staggerDirection = AG2_STAGGERDIRECTION_FORWARD;
					}
				}
				break;

				case 3:
				{
					staggerRange++;
					if (staggerRange > AG2_STAGGER_RANGE_LONG)
					{
						staggerRange = AG2_STAGGER_RANGE_NONE;
					}
				}
				break;

				case 4:
				{
					hitLocation++;
					if (hitLocation > AG2_IMPACTLOCATION_LOW)
					{
						hitLocation = AG2_IMPACTLOCATION_NONE;
					}
				}
				break;

				case 5:
				{
					hurtType++;
					if (hurtType > AG2_IMPULSEVALUE_HURTTYPE_DEAD)
					{
						hurtType = AG2_IMPULSEVALUE_HURTTYPE_SHORT;
					}
				}
				break;

			default:
				break;
			}


			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
			if (pCD)
			{
				if (!incrementWhat)
				{
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_HURT_IMPACTDIRECTION,hitDirection);
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_HURT_STAGGERDIRECTION,staggerDirection);
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_HURT_RANGE,staggerRange);
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_HURT_IMPACTLOCATION,hitLocation);

					CWAG2I_Context AG2Context(m_pGameObject,m_pServer,pCD->m_GameTime);
					pCD->m_AnimGraph2.SendHurtImpulse(&AG2Context, CXRAG2_Impulse(AG2_IMPULSETYPE_HURT,hurtType),0);
				}

				CStr Hurt;
				switch(hitDirection)
				{
				case AG2_IMPACTDIRECTION_NONE:
					Hurt = Hurt+CStr("HITDIR: None, ");
					break;
				case AG2_IMPACTDIRECTION_FRONT:
					Hurt = Hurt+CStr("HITDIR: Front, ");
					break;
				case AG2_IMPACTDIRECTION_BACK:
					Hurt = Hurt+CStr("HITDIR: Back, ");
					break;
				case AG2_IMPACTDIRECTION_LEFT:
					Hurt = Hurt+CStr("HITDIR: Left, ");
					break;
				case AG2_IMPACTDIRECTION_RIGHT:
					Hurt = Hurt+CStr("HITDIR: Right, ");
					break;
				}

				switch(staggerDirection)
				{
				case AG2_STAGGERDIRECTION_FORWARD:
					Hurt = Hurt+CStr("STAG_DIR: Fwd, ");
					break;
				case AG2_STAGGERDIRECTION_BACK:
					Hurt = Hurt+CStr("STAG_DIR: Bwd, ");
					break;
				case AG2_STAGGERDIRECTION_LEFT:
					Hurt = Hurt+CStr("STAG_DIR: Left, ");
					break;
				case AG2_STAGGERDIRECTION_RIGHT:
					Hurt = Hurt+CStr("STAG_DIR: Right, ");
					break;
				}

				switch(staggerRange)
				{
				case AG2_STAGGER_RANGE_NONE:
					Hurt = Hurt+CStr("STAG_RNG: None, ");
					break;
				case AG2_STAGGER_RANGE_ONPLACE:
					Hurt = Hurt+CStr("STAG_RNG: Inplace, ");
					break;
				case AG2_STAGGER_RANGE_SHORT:
					Hurt = Hurt+CStr("STAG_RNG: Short, ");
					break;
				case AG2_STAGGER_RANGE_LONG:
					Hurt = Hurt+CStr("STAG_RNG: Long, ");
					break;
				}

				switch(hitLocation)
				{
				case AG2_IMPACTLOCATION_NONE:
					Hurt = Hurt+CStr("HITLOC: None, ");
					break;
				case AG2_IMPACTLOCATION_HIGH:
					Hurt = Hurt+CStr("HITLOC: High, ");
					break;
				case AG2_IMPACTLOCATION_MID:
					Hurt = Hurt+CStr("HITLOC: Mid, ");
					break;
				case AG2_IMPACTLOCATION_LOW:
					Hurt = Hurt+CStr("HITLOC: Low, ");
					break;
				}

				switch(hurtType)
				{
				case AG2_IMPULSEVALUE_HURTTYPE_SHORT:
					Hurt = Hurt+CStr("HURT: Short");
					break;
				case AG2_IMPULSEVALUE_HURTTYPE_LONG:
					Hurt = Hurt+CStr("HURT: Long");
					break;
				case AG2_IMPULSEVALUE_HURTTYPE_EXTREME:
					Hurt = Hurt+CStr("HURT: Extreme");
					break;
				case AG2_IMPULSEVALUE_HURTTYPE_DEAD:
					Hurt = Hurt+CStr("HURT: Dead");
					break;
				}
				ConOutL(Hurt);
			}
			
			return true;
		};
	case IMPULSE_RESTRICT_ACTIVE:
		{
			if (m_AH.m_spRestriction == NULL)
			{
				m_AH.m_spRestriction = MNew1(CAI_Action_Restrict, &m_AH);
			}
			m_AH.m_spRestriction->SetParameter(CAI_Action_Restrict::PARAM_ACTIVE,CStr((char *)_pData));
			return true;
		};
	case IMPULSE_RESTRICT_RANGE:
		{
			if (m_AH.m_spRestriction == NULL)
			{
				m_AH.m_spRestriction = MNew1(CAI_Action_Restrict, &m_AH);
			}
			m_AH.m_spRestriction->SetParameter(CAI_Action_Restrict::PARAM_RANGE,CStr((char *)_pData));
			return true;
		};
	case IMPULSE_RESTRICT_OBJECT:
		{
			if (m_AH.m_spRestriction == NULL)
			{
				m_AH.m_spRestriction = MNew1(CAI_Action_Restrict, &m_AH);
			}
			m_AH.m_spRestriction->SetParameter(CAI_Action_Restrict::PARAM_OBJECT,CStr((char *)_pData));
			return true;
		};
	case IMPULSE_HITRATIO:
		{
			//Bot changes hit ratio
			m_HitRatio = Max(0, Min(100, CStr((char *)_pData).Val_int()));
			return true;
		};
	case IMPULSE_ACTIVATERANGE:
		{
			//Bot changes activate range
			m_ActivateRangeSqr = Sqr(Max(0.0f, (fp32)CStr((char *)_pData).Val_fp64()));
			return true;
		};
	case IMPULSE_IDLEMAXSPEED:
		{	//Bot changes idle max speed
			if (_pData)
			{
				fp32 Valuef = (fp32)CStr((char *)_pData).Val_fp64();
				CStr SpeedStr = CStr((char *)_pData).UpperCase();
				if (Valuef != 0)
				{
					m_IdleMaxSpeed = Valuef;
					m_ReleaseIdleSpeed = m_IdleMaxSpeed;
				}
				else if (SpeedStr.CompareNoCase("WALK SLOW") == 0)
				{
					m_IdleMaxSpeed = GetMaxSpeedForward() * AI_MOVE_THRESHOLD_WALKSLOW;
					m_ReleaseIdleSpeed = m_IdleMaxSpeed;
				}
				else if (SpeedStr.CompareNoCase("WALK") == 0)
				{
					m_IdleMaxSpeed = GetMaxSpeedForward() * AI_MOVE_THRESHOLD_WALK;
					m_ReleaseIdleSpeed = m_IdleMaxSpeed;
				}
				else if (SpeedStr.CompareNoCase("WALK FAST") == 0)
				{
					m_IdleMaxSpeed = GetMaxSpeedForward() * AI_MOVE_THRESHOLD_WALKFAST;
					m_ReleaseIdleSpeed = m_IdleMaxSpeed;
				}
				else	// Run
				{
					m_IdleMaxSpeed = GetMaxSpeedForward();
					m_ReleaseIdleSpeed = m_IdleMaxSpeed;
				}
			}
			return true;
		};
	case IMPULSE_HEALTH: 
		{
			//Bot changes health (without actually taking damage)
			int HealthV = CStr((char *)_pData).Val_int();
			if (HealthV <= 0)
				m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_PHYSICS_KILL, DAMAGETYPE_UNDEFINED), m_pGameObject->m_iObject);
			else
				//Set health (this caps health at max health value, and assumes bot doesn't die)
				m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADDHEALTH, HealthV - Health(m_pGameObject->m_iObject)), m_pGameObject->m_iObject);
			return true;
		};
	case IMPULSE_ALERTNESS:
		{
			//Bot changes alertness level
			m_KB.SetAlertness(CStr((char *)_pData), true);
			return true;
		}
	case IMPULSE_DARKNESSPOWER:
		{
			CStr Params = CStr((char *)_pData).LowerCase();

			// Make sure these matches with the CAI_Device_Darkness::POWER_XXX enums.
			static const char* TranslatePower[] = {"CreepingDark", "DemonArm", "BlackHole", NULL}; 
			CStr sAction = Params.GetStrSep(":");
			int Power = sAction.TranslateInt(TranslatePower);
			int Usage;
			if (Power != -1)
			{
				// We have a power. Drain and activate.
				Usage = CAI_Device_Darkness::USAGE_ACTIVATE | CAI_Device_Darkness::USAGE_DRAIN;
			}
			else
			{
				Power = CAI_Device_Darkness::POWER_INVALID;

				// Check if we should perform other darkness actions
				if (sAction.CompareNoCase("DrainJuice") == 0)
				{
					Usage = CAI_Device_Darkness::USAGE_DRAIN;
				}
				else if (sAction.CompareNoCase("RemoveAllJuice") == 0)
				{
					// Remove all drained darkness juice immediately and stop darkness usage
					m_DeviceDarkness.RemoveAllDarknessJuice();
					Usage = CAI_Device_Darkness::USAGE_NONE;
				}
				else if (sAction.Find("Select") == 0)
				{
					Usage = CAI_Device_Darkness::USAGE_SELECT;
					sAction = sAction.SubStr(6,sAction.Len() - 6); // Trim the "Select" prefix
					Power = sAction.TranslateInt(TranslatePower);
				}
				else
				{
					// Unrecognized actions, such as "StopUsingPower" will stop any current darkness usage
					Usage = CAI_Device_Darkness::USAGE_NONE;
				}
			}
			
			CStr PrimaryTarget = Params.GetStrSep(":");
			CWObject * pPrimaryTarget = GetSingleTarget(_iObject, PrimaryTarget, _iSender);
			int iPrimaryTarget = (pPrimaryTarget ? pPrimaryTarget->m_iObject : 0);
			
			CStr SecondaryTarget = Params.GetStrSep(":");
			CWObject * pSecondaryTarget = GetSingleTarget(_iObject, SecondaryTarget, _iSender);
			int iSecondaryTarget = (pSecondaryTarget ? pSecondaryTarget->m_iObject : 0);
			
			m_Script.SetAction(CAI_ScriptHandler::USE_DARKNESS, Power, CVec3Dfp32(iPrimaryTarget, iSecondaryTarget, Usage));
			return true;
		}
	case IMPULSE_SPECIAL:
		{
			// For any special (usually hack) purposes
			return true;
		}
	};

	//Impulse was incorrect
	return false;
};


//Handles incoming messages
aint CAI_Core::OnMessage(const CWObject_Message& _Msg)
{
	// Don't handle messages when dead
	switch(_Msg.m_Msg)
	{
	case OBJSYSMSG_GAME_POSTWORLDLOAD:
		{
			OnPostWorldLoad();
			return 1;
		};
	case OBJMSG_GAME_SPAWN:
		{
			if (_Msg.m_Param0 > 0)
			{	// Unspawn: Yes the parameters are all backwards
				m_EventHandler.RaiseEvent(CAI_EventHandler::ON_UNSPAWN,0);
			}
			// m_Param0 == 0: Spawn, m_Param0 < 0: Tryspawn
			// Spawn is handled under m_bFirstRefresh in CAI_Core::OnRefresh
			return 0;
		};
	case OBJMSG_BROKEN_LIGHT:
		{
			m_KB.Global_ReportBrokenLight(_Msg.m_Param0);
			return 1;
		};
	case OBJMSG_DAMAGE:
		{	// Only use this to update last hurt tick of player
			if ((m_pGameObject)&&(!IsBot(m_pGameObject->m_iObject)))
			{
				m_KB.UpdateLastHurtTick();
			}
			//Let char recieve this message as well
			return 0;
		};
	case OBJMSG_GAME_DAMAGE:
		{
			//We got hurt! 
			OnTakeDamage(_Msg.m_iSender,_Msg.m_Param0,_Msg.m_Param1);

			//Let char recieve this message as well
			return 0;
		};
	case OBJMSG_CHAR_RAISEVISIBILITY:
		{
			m_KB.AddLight(_Msg.m_Param0);

			//Char must handle this message as well
			return 0;
		};
	case OBJMSG_CHAR_RAISENOISELEVEL:
		{
			m_KB.AddLoudness(_Msg.m_Param0);

			//Char must handle this message as well
			return 0;
		};
	case OBJMSG_CHAR_AIRESTRICT:
		{
			// Extract the separate parameters (object,range,active flag) and send them to AH
			// IMPULSE_RESTRICT_OBJECT
			// IMPULSE_RESTRICT_RANGE
			// IMPULSE_RESTRICT_ACTIVE
			CStr Data = (const char*)_Msg.m_pData;
			CStr Target = Data.GetStrMSep(",;");
			CStr Range = Data.GetStrMSep(",;");
			CStr Activation = Data.GetStrMSep(",;");
			// Extract the object part of _Msg
			OnImpulse(IMPULSE_RESTRICT_OBJECT,m_pGameObject->m_iObject,0,0,Target.GetStr());
			// Extract the range part of _Msg
			OnImpulse(IMPULSE_RESTRICT_RANGE,m_pGameObject->m_iObject,0,0,Range.GetStr());
			// Extract the active part of _Msg
			OnImpulse(IMPULSE_RESTRICT_ACTIVE,m_pGameObject->m_iObject,0,0,Activation.GetStr());
			return 1;
		};
	case OBJMSG_CHAR_AI_PAUSEACTION:
		{
			// Extract the separate parameters (action, duration)
			CStr Data = (char*)_Msg.m_pData;
			CStr Action = Data.GetStrMSep(",;");
			CStr Duration = Data.GetStrMSep(",;");
			int32 iAction = CAI_Action::StrToType(Action);
			if ((iAction == 0)&&(Action == "ALL"))
			{
				iAction = -1;
			}
			int32 DurationTicks = int(fp32(Duration.Val_fp64()) * GetAITicksPerSecond());
			if (DurationTicks < 0) { DurationTicks = -1;}
			m_AH.PauseAction(iAction,DurationTicks);
			return 1;
		};
	case OBJMSG_CHAR_PLAYANIM:
		{	// We do NOT eat this message as char needs it as well (even more than us actually)
			CFStr Str = (char *)_Msg.m_pData;
			if ((Str.CompareNoCase("") == 0)&&(Str.Val_int() == 0L))
			{	// Play anim (ward off evil sign) == 0
				OnImpulse(IMPULSE_RELEASE);
			}
			else
			{	// Play anim (ward off evil sign) != 0
				OnImpulse(IMPULSE_PAUSE);
				// m_Script.SetAction(CAI_ScriptHandler::CONTINUOUS_MOVE, CVec3Dfp32(0));
			}
			// I repeat; We do NOT eat this message
			return 0;
		};
	case OBJMSG_CHAR_AIGETACIVESTATE:
		{
			return m_ActivationState;
		}
	case OBJMSG_CHAR_AISETACTIVESTATE:
		{
			m_ActivationState = _Msg.m_Param0;
			return 1;
		}
	case OBJMSG_CHAR_PICKFIGHT:
		{
			//If we're aware of sender, treat this as if we've been attacked by sender, but don't raise injury event
			CAI_AgentInfo * pInfo = m_KB.GetAgentInfo(_Msg.m_iSender);
			if (pInfo && (pInfo->GetAwareness() >= CAI_AgentInfo::SPOTTED))
			{
				m_EventHandler.LockEvent(CAI_EventHandler::ON_INJURED);
				OnTakeDamage(_Msg.m_iSender);			
				m_EventHandler.ReleaseEvent(CAI_EventHandler::ON_INJURED);
			}
			//Let game object handle as well
			return 0;
		}
	case OBJMSG_BLOCKED_ATTACK:
		{
			//We blocked the attack of the object with param0 index
			CAI_Item_Shield * pShield = CAI_Item_Shield::GetShield(m_Item.GetWielded());
			if (pShield)
				pShield->Block(_Msg.m_Param0);
			return 1;
		};
	case OBJMSG_CHAR_AICANOPENDOOR:
		{	// Tell caller wether we want to open the door ourselves or not
			// Return 1 iw we DON'T want the door to auto-open
			// Return 0 if we DO want the door to auto-open
			// 1a We will not even go through the door
			// 1b We want to open the door through its scenepoints
			// 1c We want to kick and trick the door to open when the anim event happens in our kick
			// 0a: We play a kick and let the door auto-open

			if (!(m_UseFlags & USE_DOORSPS))
			{
				return 0;
			}
			if ((m_DeviceStance.GetIdleStance() > CAI_Device_Stance::IDLESTANCE_IDLE)||(GetStealthTension() > TENSION_NONE))
			{	// Cannot open doors in combat, may kick it in though
				return 0;
			}
			CWO_ScenePointManager* pSPM = GetScenePointManager();
			CWO_ScenePoint* pSP0 = NULL;
			if (pSPM)
			{
				pSP0 = pSPM->GetScenePointFromIndex(_Msg.m_Param0);
				if (!pSP0)
				{	// No scenepoint
					CWO_ScenePoint* pDoorSP = m_Script.GetDoorScenepoint();
					if (pDoorSP)
					{
						int iBehaviour = pDoorSP->GetBehaviour();
						if (GetCurrentBehaviour() == iBehaviour)
						{
							StopBehaviour(BEHAVIOUR_STOP_NORMAL,CAI_Action::PRIO_COMBAT);
						}
						m_Script.ClearDoorScenepoint();
						ResetPathSearch();
					}
					// We DID handle the message didn't we?
					return 1;			
				}
			}
			else
			{
				return 0;				
			}
			
			if (pSP0)
			{
				if (pSP0->PeekRequest(GetObjectID(),m_pServer))
				{
					m_Script.SetAction(CAI_ScriptHandler::MOVE_TO_SCENEPOINT,pSP0);
					return 1;
				}
			}
			return 0;
		};
	case OBJMSG_SCRIPTIMPULSE:
		{
			return OnImpulse(_Msg.m_Param0, _Msg.m_iSender, _Msg.m_Param1, _Msg.m_VecParam0, _Msg.m_pData);
		};
	case CWObject_AnimEventListener::OBJECT_ANIMEVENTLISTENER_MESSAGE_ONEVENT:
		{
			if ((m_pBehaviourScenePoint)&&(m_bBehaviourRunning))
			{	// Activate the scenepoints animation event messages if any
				CWO_ScenePointManager * pSPM = GetScenePointManager();
				if (pSPM)
				{
					m_pBehaviourScenePoint->AnimationEvent(GetObjectID(),_Msg.m_Param0,m_pServer,pSPM);
				}
			}
			else if ((m_Script.GetMoveToScenepoint())&&(m_bBehaviourRunning))
			{	// Activate the scenepoints animation event messages if any
				CWO_ScenePointManager * pSPM = GetScenePointManager();
				if (pSPM)
				{
					m_Script.GetMoveToScenepoint()->AnimationEvent(GetObjectID(),_Msg.m_Param0,m_pServer,pSPM);
				}
			}
			// *** Note: Maybe the scenepoint animation event hanlers should return bools and we only call this
			// when both of the above fail? ***
			m_AH.AnimationEvent(GetObjectID(),_Msg.m_Param0);
			return 1;
		};
	case OBJMSG_TEAM_FIGHT:
		{	//Incoming alarm, let knowledge base handle this
			m_KB.OnFight(_Msg.m_Param0, _Msg.m_iSender, _Msg.m_Reason);
			return 1;
		}
	case OBJMSG_TEAM_ALARM:
		{	//Incoming alarm, let knowledge base handle this
			m_KB.OnAlarm(_Msg.m_Param0,_Msg.m_iSender,_Msg.m_Reason);
			return 1;
		}
	case OBJMSG_TEAM_AWARE:
		{	//Incoming NOTICE,DETECT or SPOT, let knowledge base handle this
			m_KB.OnAware(_Msg.m_Param0, _Msg.m_iSender, _Msg.m_Param1,_Msg.m_VecParam0,_Msg.m_Reason);
			return 1;
		}
	case OBJMSG_TEAM_FOUND_CORPSE:
		{
			if (m_StunTimeout)
			{
				return 1;
			}
			// Perp, victim, where
			m_KB.FoundCorpse(_Msg.m_Param0,_Msg.m_Param1,_Msg.m_VecParam0);
			return 1;
		}
	case OBJMSG_TEAM_RANGE_ALARM:
		{	// Measure range to _Msg.m_Param0
			CAI_AgentInfo* pInfo = m_KB.GetAgentInfo(_Msg.m_Param0);
			if (!pInfo)
			{
				pInfo = m_KB.AddAgent(_Msg.m_Param0);
			}
			if ((pInfo)&&(pInfo->GetObject()))
			{	// Q: Why don't we use pInfo->GetPosition()?
				// A: We want the objects true position as we measure range for picking up the alarm
				if (m_pGameObject->GetPosition().DistanceSqr(pInfo->GetObject()->GetPosition()) <= Sqr(m_HearingRange))
				{
					m_EventHandler.RaiseEvent(CAI_EventHandler::ON_ALARM,_Msg.m_Param0);
					m_KB.OnAlarm(_Msg.m_Param0, _Msg.m_iSender, _Msg.m_Reason);
				}
			}
			return 1;
		}
	case OBJMSG_TEAM_SEARCH_OFF:
		{
			if ((m_StunTimeout)||(m_Script.IsPaused()))
			{
				return 1;
			}
			if (GetStealthTension() <= TENSION_HOSTILE)
			{
				CAI_AgentInfo* pSuspect = m_KB.GetAgentInfo(_Msg.m_Param0);
				if (pSuspect)
				{
					pSuspect->SetAwareness(CAI_AgentInfo::NONE,true);
				}
				SetStealthTension(TENSION_NONE);
				m_AH.ExpireActions(CAI_Action::INVESTIGATE);
				if (m_AH.m_iInvestigate == _Msg.m_Param0) { m_AH.m_iInvestigate = 0; };
				if (m_AH.m_iInvestigate2 == _Msg.m_Param0) { m_AH.m_iInvestigate2 = 0; };
				if (m_AH.m_iInvestigateObj == _Msg.m_Param0) { m_AH.m_iInvestigateObj = 0; }
				UseRandom("Got search off",CAI_Device_Sound::SEARCH_STOP_RESPONSE,CAI_Action::PRIO_ALERT);
			}
			return 1;
		};
	case OBJMSG_AIEFFECT_RETARGET:
		{	// AI should consider a new target
			if ((!m_pGameObject->AI_IsSpawned())||(m_Script.IsValid()))
			{
				return 1;
			}
			if ((m_AH.m_spRestriction)&&(m_AH.m_spRestriction->IsRestricted(_Msg.m_VecParam0)))
			{
				return 1;
			}
			m_AH.ExpireActions(CAI_Action::INVALID);
			m_AH.m_iTarget = 0;
			if (SqrDistanceToUs(_Msg.m_VecParam0) < Sqr(Max3(m_RoamMaxRange,m_CloseMaxRange,m_CombatMaxRange)))
			{
				m_AH.m_RetargetPos = _Msg.m_VecParam0;
				m_AH.m_RetargetUp =  _Msg.m_VecParam1;
			}
			m_AH.m_RetargetPos = m_PathFinder.SafeGetPathPosition(m_AH.m_RetargetPos,4,2);
			m_AH.m_RetargetTimeout = 0;
			return 1;
		};
	case OBJMSG_IM_ALIIIVE:
		{
			//Add any newly spawned interesting agent
			m_KB.NewAgent(_Msg.m_iSender);
			return 1;
		};
	case OBJMSG_CHAR_DIALOGUE_SPEAKER:	// Dialogue master begin
		{	// Param0 is the recipient of the dialogue
			CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
			if ((_Msg.m_Param0 != 0)&&
				(!(_Msg.m_Param1 & PLAYER_DIALOGUE_AIFLAGS_KEEPBEHAVIOURSPEAKER))&&
				((!pChar)||(!(pChar->m_Flags & PLAYER_FLAGS_NODIALOGUETURN))))
			{
				StopBehaviour();
			}
			// Check for setting m_iDialoguePartner to self
			if (_Msg.m_Param0 != GetObjectID())
			{
/*			DIALOGUE DEBUG
				if (_Msg.m_Param0)
				{
					CStr Name = m_pGameObject->GetName();
					ConOutL(Name+CStr(" speaker to ")+CStrF("%d",_Msg.m_Param0));
				}
				else
				{
					ConOut("No speaker");
				}
*/
				m_iDialoguePartner = _Msg.m_Param0;
				m_pGameObject->AI_SetEyeLookDir();
			}
#ifndef M_RTM
			else
			{
				CStr Name = m_pGameObject->GetName();
				ConOutL(Name+CStr(": OBJMSG_CHAR_DIALOGUE_SPEAKER sent to self ")+CStrF("%d Tick ",GetAITick()));
			}
#endif
			m_iCrowdFocus = 0;
			m_bLookPartner = true;
			if ((m_iDialoguePartner != 0)&&(_Msg.m_Param1 & PLAYER_DIALOGUE_AIFLAGS_NOLOOKSPEAKER))
			{
				m_bLookPartner = false;
			}
			else if (m_pGameObject)
			{
				if ((pChar)&&(pChar->m_Flags &PLAYER_FLAGS_NODIALOGUETURN))
				{
					m_bLookPartner = false;
				}
			}
			if ((_Msg.m_Param0 != 0)&&(!(_Msg.m_Param1 & PLAYER_DIALOGUE_AIFLAGS_NOCROWD)))
			{
				// Tell nearby characters that they should look at me if neutral or better(?)
				// We iterate through all characters and report any dead ones
				TSelection<CSelection::LARGE_BUFFER> Selection;
				m_pServer->Selection_AddOnFlagsSet(Selection, OBJECT_FLAGS_CHARACTER);
				const int16* pSel = NULL;
				int nSel = m_pServer->Selection_Get(Selection, &pSel);
				// Iterate through 'em all (this sure will generate a spike :)
				for (int k = 0; k < nSel; k++)
				{
					int iCur = pSel[k];
					if ((m_pGameObject)&&(iCur == m_pGameObject->m_iObject))
					{
						continue;
					}
					if (SqrDistanceToUs(iCur) <= Sqr(MAX_DIALOGUE_CROWD_RANGE))
					{
						m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DIALOGUE_CROWD_FOCUS,m_pGameObject->m_iObject),iCur);
					}
				}
			}
			return 1;
		};
	case OBJMSG_CHAR_DIALOGUE_LISTENER:	// Dialogue listener begin
		{	// Param0 is the speaker
			CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
			if ((_Msg.m_Param0 != 0)&&
				(!(_Msg.m_Param1 & PLAYER_DIALOGUE_AIFLAGS_KEEPBEHAVIOURLISTENER))&&
				((!pChar)||(!(pChar->m_Flags & PLAYER_FLAGS_NODIALOGUETURN))))
			{
				StopBehaviour();
			}
			// Check for setting m_iDialoguePartner to self
			if (_Msg.m_Param0 != GetObjectID())
			{
				m_iDialoguePartner = _Msg.m_Param0;
				m_pGameObject->AI_SetEyeLookDir();
			}
#ifndef M_RTM
			else
			{
				CStr Name = m_pGameObject->GetName();
				ConOutL(Name+CStr(": OBJMSG_CHAR_DIALOGUE_LISTENER sent to self ")+CStrF("%d Tick ",GetAITick()));
			}
#endif
/*			DIALOGUE DEBUG
			if (_Msg.m_Param0)
			{
				CStr Name = m_pGameObject->GetName();
				ConOutL(Name+CStr(" listener to ")+CStrF("%d",_Msg.m_Param0));
			}
			else
			{
				ConOut("No listener");
			}
*/

			m_iCrowdFocus = 0;
			m_bLookPartner = true;
			if ((m_iDialoguePartner != 0)&&(_Msg.m_Param1 & PLAYER_DIALOGUE_AIFLAGS_NOLOOKSPEAKER))
			{
				m_bLookPartner = false;
			}
			else if (m_pGameObject)
			{
				if ((pChar)&&(pChar->m_Flags & PLAYER_FLAGS_NODIALOGUETURN))
				{
					m_bLookPartner = false;
				}
			}
			return 1;
		};
	case OBJMSG_CHAR_DIALOGUE_CROWD_FOCUS:
		{
			// Check for setting m_iDialoguePartner to self
			if (_Msg.m_Param0 != GetObjectID())
			{
				if (_Msg.m_Param0)
				{
					CAI_AgentInfo* pSpeaker = m_KB.GetAgentInfo(_Msg.m_Param0);
					if ((pSpeaker)&&(pSpeaker->GetCurRelation() < CAI_AgentInfo::UNFRIENDLY))
					{
						m_iCrowdFocus = _Msg.m_Param0;
						CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
						if ((pChar)&&(pChar->m_Flags & PLAYER_FLAGS_NODIALOGUETURN))
						{
							m_iCrowdFocus = 0;
						}
						else
						{
							StopBehaviour();
						}
					}
				}
				else
				{
					m_iCrowdFocus = 0;
				}
			}
#ifndef M_RTM
			else
			{
				CStr Name = m_pGameObject->GetName();
				ConOutL(Name+CStr(": OBJMSG_CHAR_DIALOGUE_CROWD_FOCUS sent to self ")+CStrF("%d Tick ",GetAITick()));
			}
#endif
			return 1;
		};
	case OBJSYSMSG_PRECACHEMESSAGE:
		{
			// Resolve all animation handles, so that the animation can be precached.
			if(_Msg.m_DataSize == sizeof(CWObject_Message))
			{
				CWObject_Message *pMsg = (CWObject_Message *)_Msg.m_pData;
				if (pMsg->m_Msg == OBJMSG_SCRIPTIMPULSE)
				{
					if (((pMsg->m_Param0 == IMPULSE_ANIMATION) ||
						(pMsg->m_Param0 == IMPULSE_TORSO_ANIMATION))
						&& pMsg->m_pData)
					{
						ResolveAnimHandle((char *)pMsg->m_pData);
					}
					else if ((pMsg->m_Param0 == IMPULSE_RUN_BEHAVIOUR)||(pMsg->m_Param0 == IMPULSE_FORCE_BEHAVIOUR) || 
						(pMsg->m_Param0 == IMPULSE_LOOP_BEHAVIOUR))
					{
						int iBehaviour;
						if ((pMsg->m_Param0 == IMPULSE_RUN_BEHAVIOUR) || (pMsg->m_Param0 == IMPULSE_LOOP_BEHAVIOUR))
						{
							iBehaviour = ((int)CStr((char *)pMsg->m_pData).Val_int()) & ~AG2_WANTEDBEHAVIORFLAG_MASKTYPE;
						}
						else
						{
							iBehaviour = ((int)CStr((char *)pMsg->m_pData).Val_int()) | AG2_WANTEDBEHAVIORFLAG_MASKTYPE;
						}

						if (iBehaviour)
						{
							CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
							if ((pCD)&&(m_pGameObject))
							{	// Tell AG we use iBehaviour as well
								CWAG2I_Context Context2(m_pGameObject,m_pServer,pCD->m_GameTime);
								TArray<CXRAG2_Impulse> lImpulses;
								lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR,iBehaviour));
								pCD->m_AnimGraph2.GetAG2I()->TagAnimSetFromImpulses(&Context2,m_pServer->GetMapData(),m_pServer->m_spWData,lImpulses);
							}
						}
					}
					else if (pMsg->m_Param0 == IMPULSE_TURRET_MOUNT)
					{	// Precache turret anims
						CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
						if ((pCD)&&(m_pGameObject))
						{	// Tell AG we use iBehaviour as well
							CWAG2I_Context Context2(m_pGameObject,m_pServer,pCD->m_GameTime);
							TArray<CXRAG2_Impulse> lImpulses;
							lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,AG2_IMPULSEVALUE_GAMEPLAY_TURRET_STANDING));
							lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,AG2_IMPULSEVALUE_GAMEPLAY_TURRET_LOW));
							pCD->m_AnimGraph2.GetAG2I()->TagAnimSetFromImpulses(&Context2,m_pServer->GetMapData(),m_pServer->m_spWData,lImpulses);
						}
					}
					else if ((pMsg->m_Param0 == IMPULSE_MOVE_TO_SCENEPOINT)||(pMsg->m_Param0 == IMPULSE_TELEPORT_TO_SP)||
						pMsg->m_Param0 == IMPULSE_JUMP_TO_SP)
					{	// TODO: Precache all messages from the given scenepoint (somehow?)
						
					}
				}
				else if (pMsg->m_Msg == OBJMSG_CHAR_AI_RUNBEHAVIOUR)
				{
					CStr Data = (char*)pMsg->m_pData;
					CStr PrioStr = Data.GetStrMSep(",;");
					CStr BehaviourStr = Data.GetStrMSep(",;");
					int32 iBehaviour = BehaviourStr.Val_int();
					if (iBehaviour)
					{
						CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
						if ((pCD)&&(m_pGameObject))
						{	// Tell AG we use iBehaviour as well
							CWAG2I_Context Context2(m_pGameObject,m_pServer,pCD->m_GameTime);
							TArray<CXRAG2_Impulse> lImpulses;
							lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR,iBehaviour));
							pCD->m_AnimGraph2.GetAG2I()->TagAnimSetFromImpulses(&Context2,m_pServer->GetMapData(),m_pServer->m_spWData,lImpulses);
						}
					}
				}
			};
			

			// CWObject must be able to handle the message too.
			return 0;
		}
	case OBJMSG_RELEASEPATH:
		{
			return (m_PathFinder.OnMessage(_Msg) || m_Path.OnMessage(_Msg));
		}
	case OBJMSG_DIE:
		{
			//Raise OnDie event
			m_EventHandler.RaiseEvent(CAI_EventHandler::ON_DIE,0);

			//Clear scripting
			m_Script.Clear();

			//Remove from static character list
			CSimpleIntList* plAgents = &(m_pAIResources->m_KBGlobals.ms_lAgents);
			int i = plAgents->Find(m_pGameObject->m_iObject);
			if (i != -1)
			{
				plAgents->Remove(i);
			}

			//Reinit
			m_bWasAlive = false;
			Destroy();
			Init(m_pGameObject, m_pServer);

			//Allow character to handle as well.. Don't know if this is necessary, but wtf
			return 0;
		}
	case OBJMSG_TEAM_HEARVOICE:
		{
			//Assume team voice messages make pretty much noise
			// We add the agent wether he hears us or not as getting this message should mean
			// the sender is interesting to us.
			// NOTE It is somehow odd that we do not already know about this agent. We belong to his
			// team yet still do not know about him?
			CAI_AgentInfo * pInfo = m_KB.GetAgentInfo(_Msg.m_iSender);
			if (!pInfo)
			{
				m_KB.AddAgent(_Msg.m_iSender);
			}
			if (Perception_Hearing(_Msg.m_iSender,0,true) >= CAI_AgentInfo::NOTICED)
				return 1;
			else
				return 0;
		}

	case OBJMSG_AIEFFECT_SETHURT:
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
			if (pCD)
			{
				CStr Data = (char*)_Msg.m_pData;
				int rnd;
				/*AG2_IMPULSEVALUE_HURTTYPE_SHORT = 1,
				AG2_IMPULSEVALUE_HURTTYPE_LONG = 2,
				AG2_IMPULSEVALUE_HURTTYPE_EXTREME = 3,
				AG2_IMPULSEVALUE_HURTTYPE_DEAD = 4,*/
				int32 hurtType =  Data.GetStrMSep(",;").Val_int();
				if (hurtType == -1)
				{	// Random hurt
					rnd = TruncToInt(Random * 2.999f);
					switch(rnd)
					{
					case 0:
						hurtType = AG2_IMPULSEVALUE_HURTTYPE_SHORT;
						break;
					case 1:
						hurtType = AG2_IMPULSEVALUE_HURTTYPE_LONG;
						break;
					case 2:
						hurtType = AG2_IMPULSEVALUE_HURTTYPE_EXTREME;
						break;
					}
				}

				/*AG2_IMPACTLOCATION_NONE = 0,
				AG2_IMPACTLOCATION_HIGH = 1,
				AG2_IMPACTLOCATION_MID = 2,
				AG2_IMPACTLOCATION_LOW = 3,*/
				int32 hitLocation = Data.GetStrMSep(",;").Val_int();
				if (hitLocation == -1)
				{	// Random hurt
					rnd = TruncToInt(Random * 2.999f);
					switch(rnd)
					{
					case 0:
						hitLocation = AG2_IMPACTLOCATION_NONE;
						break;
					case 1:
						hitLocation = AG2_IMPACTLOCATION_MID;
						break;
					case 2:
						hitLocation = AG2_IMPACTLOCATION_LOW;
						break;
					}
				}

				// Extract the various parameters
				/*AG2_IMPACTDIRECTION_NONE = 0,
				AG2_IMPACTDIRECTION_FRONT = 1,
				AG2_IMPACTDIRECTION_BACK = 2,
				AG2_IMPACTDIRECTION_LEFT = 3,
				AG2_IMPACTDIRECTION_RIGHT = 4,*/
				int32 hitDirection = Data.GetStrMSep(",;").Val_int();
				if (hitDirection == -1)
				{	// Random hurt
					rnd = TruncToInt(Random * 3.999f);
					switch(rnd)
					{
					case 0:
						hitDirection = AG2_IMPACTDIRECTION_FRONT;
						break;
					case 1:
						hitDirection = AG2_IMPACTDIRECTION_BACK;
						break;
					case 2:
						hitDirection = AG2_IMPACTDIRECTION_LEFT;
						break;
					case 3:
						hitDirection = AG2_IMPACTDIRECTION_RIGHT;
						break;
					}
				}

				/*AG2_STAGGERDIRECTION_FORWARD = 0,
				AG2_STAGGERDIRECTION_BACK = 1,
				AG2_STAGGERDIRECTION_LEFT = 2,
				AG2_STAGGERDIRECTION_RIGHT = 3,*/
				int32 staggerDirection = Data.GetStrMSep(",;").Val_int();
				if (staggerDirection == -1)
				{	// Random hurt
					rnd = TruncToInt(Random * 3.999f);
					switch(rnd)
					{
					case 0:
						staggerDirection = AG2_STAGGERDIRECTION_FORWARD;
						break;
					case 1:
						staggerDirection = AG2_STAGGERDIRECTION_BACK;
						break;
					case 2:
						staggerDirection = AG2_STAGGERDIRECTION_LEFT;
						break;
					case 3:
						staggerDirection = AG2_STAGGERDIRECTION_RIGHT;
						break;
					}
				}

				/*AG2_STAGGER_RANGE_NONE = 0,
				AG2_STAGGER_RANGE_ONPLACE = 1,
				AG2_STAGGER_RANGE_SHORT = 2,
				AG2_STAGGER_RANGE_LONG = 3,*/
				int32 staggerRange = Data.GetStrMSep(",;").Val_int();
				if (staggerRange == -1)
				{	// Random hurt
					rnd = TruncToInt(Random * 2.999f);
					switch(rnd)
					{
					case 0:
						staggerRange = AG2_STAGGERDIRECTION_FORWARD;
						break;
					case 1:
						staggerRange = AG2_STAGGERDIRECTION_BACK;
						break;
					case 2:
						staggerRange = AG2_STAGGERDIRECTION_LEFT;
						break;
					}
				}

				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_HURT_IMPACTDIRECTION,hitDirection);
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_HURT_STAGGERDIRECTION,staggerDirection);
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_HURT_RANGE,staggerRange);
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_HURT_IMPACTLOCATION,hitLocation);

				CWAG2I_Context AG2Context(m_pGameObject,m_pServer,pCD->m_GameTime);
				pCD->m_AnimGraph2.SendHurtImpulse(&AG2Context, CXRAG2_Impulse(AG2_IMPULSETYPE_HURT,hurtType),0);
			}
			return 1;
		}
		break;

	case OBJMSG_AIEFFECT_STARTOFBEHAVIOR:
		{
#ifndef M_RTM
			// ***
			if (DebugTarget())
			{
				bool wtf = true;
			}
			// ***
#endif
			int32 iBehaviour = _Msg.m_Param0;
			if ((iBehaviour >= BEHAVIOUR_COMBAT_MIN)&&(iBehaviour <= BEHAVIOUR_COMBAT_MAX))
			{
				CAI_Weapon* pWeapon = m_Weapon.GetWielded();
				if (pWeapon)
				{
					CRPG_Object_Item* pItem = pWeapon->GetItem();
					if ((pItem)&&(pItem->m_AnimType == AG2_IMPULSEVALUE_WEAPONTYPE_RIFLE))
					{
						iBehaviour -= BEHAVIOUR_COMBAT_RIFLE_OFFSET;
					}
				}
			}

			int32 timeOut = 0;
			if (m_CurCB.m_iBehaviour == iBehaviour)
			{
				if (m_CurCB.m_Duration <= 0)
				{
					m_CurCBTimeout = Max(m_CurCBTimeout,(int32)(m_Timer + (0.5f + Random * 0.5f) * m_Patience));
				}
				else
				{
					m_CurCBTimeout = Max(m_CurCBTimeout,(int32)(m_Timer + (0.5f + Random * 0.5f) * m_CurCB.m_Duration));
				}
				m_BehaviourExpirationTick = 0;
			}
			else if (m_CurCB.m_iNext == iBehaviour)
			{
				for (int i = 0; i < m_lCombatBehaviour.Len(); i++)
				{
					if (m_lCombatBehaviour[i].m_iBehaviour == iBehaviour)
					{
						m_CurCB = m_lCombatBehaviour[i];
						if (m_CurCB.m_Duration <= 0)
						{
							m_CurCBTimeout = Max(m_CurCBTimeout,(int32)(m_Timer + (0.5f + Random * 0.5f) * m_Patience));
						}
						else
						{
							m_CurCBTimeout = Max(m_CurCBTimeout,(int32)(m_Timer + (0.5f + Random * 0.5f) * m_CurCB.m_Duration));
						}
						m_BehaviourExpirationTick = 0;
						break;
					}
				}
			}

			if ((m_bBehaviourRunning)&&((iBehaviour == m_iBehaviourRunning)||(iBehaviour == m_CurCB.m_iBehaviour)||(iBehaviour == m_CurCB.m_iPrev)))
			{
				OnBehaviourMainStarted(iBehaviour);
			}
			else if ((iBehaviour != BEHAVIOUR_NBR_STOP)&&(iBehaviour != BEHAVIOUR_NBR_STOP_FAST)&&(!m_BehaviourStopIssued))
			{
				if (!m_BehaviourStopIssued)
				{
					OnBehaviourMainStarted(iBehaviour);
				}
			}
			return(true);
		}
		break;

	case OBJMSG_AIEFFECT_ENDOFBEHAVIOR:
		{	// NOTE: 'm_Timer > m_CurCBTimeout', 'not m_Timer >= m_CurCBTimeout' as we want HandleCombatBehaviours() a chance
			// to remedy things on 'm_Timer == m_CurCBTimeout'.
#ifndef M_RTM
			// ***
			if (DebugTarget())
			{
				bool wtf = true;
			}
			// ***
#endif

			int32 iBehaviour = _Msg.m_Param0;
			if ((iBehaviour >= BEHAVIOUR_COMBAT_MIN)&&(iBehaviour <= BEHAVIOUR_COMBAT_MAX))
			{
				CAI_Weapon* pWeapon = m_Weapon.GetWielded();
				if (pWeapon)
				{
					CRPG_Object_Item* pItem = pWeapon->GetItem();
					if ((pItem)&&(pItem->m_AnimType == AG2_IMPULSEVALUE_WEAPONTYPE_RIFLE))
					{
						iBehaviour -= BEHAVIOUR_COMBAT_RIFLE_OFFSET;
					}
				}
			}

			/*
			if ((m_bBehaviourMainRunning)||(m_PrevCB.m_iBehaviour != iBehaviour))
			{	// Either no CB or we're exiting from a CBMH
				if (m_BehaviourStopIssued)
				{
					if ((m_CurCB.m_iBehaviour == 0)||((m_CurCB.m_iPrev == 0)&&(m_Timer > m_CurCBTimeout)))
					{
						OnBehaviourFinished(iBehaviour);
						m_BehaviourStoppedTimer = m_Timer;
					}
				}
			}
			*/

			return(true);
		}
		break;


	case OBJMSG_AIEFFECT_SETFACIAL:
		{
			// Extract the separate parameters (action, duration)
			CStr Data = (char*)_Msg.m_pData;
			CStr Facial = Data.GetStrMSep(",;");
			CStr Duration = Data.GetStrMSep(",;");
			int32 iFacial = Facial.Val_int();
			int32 DurationTicks = int(fp32(Duration.Val_fp64()) * GetAITicksPerSecond());
			if (DurationTicks < 0) { DurationTicks = -1;}
			m_DeviceFacial.Use(iFacial,DurationTicks);
		}
		break;

	case OBJMSG_CHAR_ANGELUS_AURA_SETSTATE:
		{	// Param0 holds the flashbangstate
			CAI_Action* pAction;
			if (m_AH.GetAction(CAI_Action::ANGELUS_ATTACK,&pAction))
			{
				CAI_Action_AngelusAttack* pAttackAction = safe_cast<CAI_Action_AngelusAttack>(pAction);
				pAttackAction->m_FlashBangState = _Msg.m_Param0;
			}
			switch(_Msg.m_Param0)
			{
			case ANGELUS_AURASTATE_IDLE:
			case ANGELUS_AURASTATE_RAMPING:
			case ANGELUS_AURASTATE_USING:
				{	// Strong and mighty
					m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETFLAGS,PLAYER_FLAGS_PROJECTILEINVULNERABLE),GetObjectID());
				}
				break;
			case ANGELUS_AURASTATE_RESTING:
				{	// Weak and puny
					m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETFLAGS,0,PLAYER_FLAGS_PROJECTILEINVULNERABLE),GetObjectID());
				}
				break;	

			default:
				break;
			}
			// Let the character handle this event as well
			return 0;
		}
	case OBJMSG_CHAR_ANGELUS_TENTACLE_SETSTATE:
		{	// Param0 holds the flashbangstate
			CAI_Action* pAction;
			if (m_AH.GetAction(CAI_Action::ANGELUS_ATTACK,&pAction))
			{
				CAI_Action_AngelusAttack* pAttackAction = safe_cast<CAI_Action_AngelusAttack>(pAction);
				pAttackAction->SetTentacleState(_Msg.m_Param0);
			}
			// Let the character handle this event as well
			return 0;
		}

	case OBJMSG_CHAR_ANIMEVENT_DEATH:
		{	// A Berserker victim or a Blackhole victim snuffs it
			if (m_iCurBehaviour)
			{
				switch (m_iCurBehaviour)
				{
					// Bersereker victim
				case BEHAVIOUR_DARKLING_BERZERKER_AXE:
				case BEHAVIOUR_DARKLING_BERZERKER_AXE_SHORT:
				case BEHAVIOUR_DARKLING_BERZERKER_BAT:
				case BEHAVIOUR_DARKLING_BERZERKER_BAT_SHORT:
				case BEHAVIOUR_DARKLING_BERZERKER_HAMMER:
				case BEHAVIOUR_DARKLING_BERZERKER_HAMMER_SHORT:
				case BEHAVIOUR_DARKLING_BERZERKER_MACHETE:
				case BEHAVIOUR_DARKLING_BERZERKER_MACHETE_SHORT:
				case BEHAVIOUR_DARKLING_BERZERKER_PISTOL:
				case BEHAVIOUR_DARKLING_BERZERKER_SAW:
				case BEHAVIOUR_DARKLING_BERZERKER_SAW_SHORT:
				case BEHAVIOUR_DARKLING_BERZERKER_SLEDGEHAMMER:
				case BEHAVIOUR_DARKLING_BERZERKER_SLEDGEHAMMER_SHORT:
				case BEHAVIOUR_DARKLING_BERZERKER_TEETH:
				case BEHAVIOUR_DARKLING_BERZERKER_TEETH_SHORT:
					{
						if (m_CharacterClass == CLASS_UNDEAD)
						{
							if (m_BehaviourPrioClass < CAI_Action::PRIO_FORCED)
							{
								StopBehaviour(BEHAVIOUR_STOP_NORMAL,CAI_Action::PRIO_FORCED);
							}
							int32 Damage = MaxHealth()+1;
							CWO_DamageMsg Msg(Damage, DAMAGETYPE_PIERCE);
							Msg.Send(GetObjectID(), 0, m_pServer);
						}
						else
						{
							if (m_BehaviourPrioClass < CAI_Action::PRIO_FORCED)
							{
								StopBehaviour(BEHAVIOUR_STOP_NORMAL,CAI_Action::PRIO_FORCED);
							}
							int32 Damage = MaxHealth()+1;
							CWO_DamageMsg Msg(Damage, DAMAGETYPE_DARKNESS);
							Msg.Send(GetObjectID(), 0, m_pServer);
							OnDie();
						}
					}
					break;

					// Black hole victim
				case BEHAVIOUR_COMBAT_BLACKHOLE_SUCKEDIN:
					{
						if (m_BehaviourPrioClass < CAI_Action::PRIO_FORCED)
						{
							StopBehaviour(BEHAVIOUR_STOP_NORMAL,CAI_Action::PRIO_FORCED);
						}
						int32 Damage = MaxHealth()+1;
						CWO_DamageMsg Msg(Damage, DAMAGETYPE_DARKNESS);
						Msg.Send(GetObjectID(), 0, m_pServer);
						OnDie();
					}
					break;
	
				default:
					{
						int32 Damage = MaxHealth()+1;
						CWO_DamageMsg Msg(Damage, DAMAGETYPE_UNDEFINED);
						Msg.Send(GetObjectID(), 0, m_pServer);
						OnDie();
					}
					break;
				}
			}
			else
			{
				int32 Damage = MaxHealth()+1;
				CWO_DamageMsg Msg(Damage, DAMAGETYPE_UNDEFINED);
				Msg.Send(GetObjectID(), 0, m_pServer);
				OnDie();
			}
			return 1;
		}

	case OBJMSG_CHAR_GRABBED_BY_DEMONARM:
		{
			if (_Msg.m_Param0)
			{	// Grabbed, Shut up
				ShutUp();
				int iVoice = SPOT_PLAYER_DARKNESS_1;
				m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_RPG_SPEAK_OLD, iVoice),GetObjectID());
				m_StunTimeout = -1;
			}
			else
			{	// Released
				m_StunTimeout = 1;
			}
		};
		return 1;
	case OBJMSG_CHAR_EXPOSEDTOBLACKHOLE:
		{
			CVec3Dfp32 Pos = _Msg.m_VecParam0;
			int32 Param = _Msg.m_Param1;
			int32 iPlayer = GetClosestPlayer();
			CAI_AgentInfo* pDarknessWielder = m_KB.GetAgentInfo(iPlayer);
			if ((m_bFearDarkness)&&(pDarknessWielder)&&(!(m_SeenDarknessPowers & PLAYER_DARKNESSMODE_POWER_BLACKHOLE)))
			{
				pDarknessWielder->SetRelation(CAI_AgentInfo::ENEMY,0);
				m_SeenDarknessPowers |= PLAYER_DARKNESSMODE_POWER_BLACKHOLE;
				UseRandom(CStr("View darkness"),CAI_Device_Sound::SPOT_PLAYER_DARKNESS,CAI_Action::PRIO_COMBAT);
				RaiseAlarm(iPlayer,CAI_AgentInfo::ENEMY,true);
			}

			if (Param != BLACKHOLE_STATE_IDLE)
			{
				if (m_iBehaviourRunning != BEHAVIOUR_COMBAT_BLACKHOLE_SUCKEDIN)
				{	// Give damage 4 times per second
					if (m_Timer % 5 == 0)
					{
						fp32 F = *(fp32*)_Msg.m_Param0;
						int32 dmg = Max(1,RoundToInt(10 * F));
						if (Health() > dmg)
						{	// F is a factor from 0.0 to 1.0
							CWO_DamageMsg Msg(dmg, DAMAGETYPE_BLACKHOLE);
							Msg.Send(GetObjectID(), 0, m_pServer);
						}
						else
						{	// Die
							if ((m_bBehaviourRunning)&&(m_iBehaviourRunning == BEHAVIOUR_COMBAT_BLACKHOLE_STAND))
							{
								CWO_DamageMsg Msg(dmg, DAMAGETYPE_BLACKHOLE);
								Msg.Send(GetObjectID(), 0, m_pServer);
							}
							else
							{
								Param = BLACKHOLE_STATE_KILL;
							}
						}
					}
				}
			}
			
			if (m_BehaviourPrioClass < CAI_Action::PRIO_FORCED)
			{
				switch(Param)
				{
				case BLACKHOLE_STATE_IDLE:
					{
						if (m_BehaviourPrioClass < CAI_Action::PRIO_FORCED)
						{
							StopBehaviour(BEHAVIOUR_STOP_NORMAL,CAI_Action::PRIO_FORCED);
						}
						m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_STRAINED,60);
					}
					break;
				case BLACKHOLE_STATE_PULL:
					{
						if ((m_BehaviourPrioClass < CAI_Action::PRIO_FORCED)&&(m_iBehaviourRunning != BEHAVIOUR_COMBAT_BLACKHOLE_STAND))
						{
							int pullDir = m_PathFinder.GetDir4(Pos);
							int behaviourFlags = BEHAVIOUR_FLAGS_LOOP;
							switch(pullDir)
							{
							case CAI_Pathfinder::FRONT:
								{
									behaviourFlags |= BEHAVIOUR_FLAGS_TOP;	
								}
								break;
							case CAI_Pathfinder::RIGHT:
								{
									behaviourFlags |= BEHAVIOUR_FLAGS_RIGHT;
								}
								break;
							case CAI_Pathfinder::BACK:
								{
									behaviourFlags |= BEHAVIOUR_FLAGS_BOTTOM;
								}
								break;
							case CAI_Pathfinder::LEFT:
								{
									behaviourFlags |= BEHAVIOUR_FLAGS_LEFT;
								}
								break;

							}																			
							SetWantedBehaviour2(BEHAVIOUR_COMBAT_BLACKHOLE_STAND,CAI_Action::PRIO_FORCED-1,behaviourFlags,0,NULL,NULL);
							m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_SCARED,60);
						}
					}
					break;
				case BLACKHOLE_STATE_KILL:
					{
						if ((m_BehaviourPrioClass < CAI_Action::PRIO_FORCED)&&(m_iBehaviourRunning != BEHAVIOUR_COMBAT_BLACKHOLE_SUCKEDIN))
						{
							int pullDir = m_PathFinder.GetDir4(Pos);
							int behaviourFlags = BEHAVIOUR_FLAGS_LOOP;
							switch(pullDir)
							{
							case CAI_Pathfinder::FRONT:
								{
									behaviourFlags |= BEHAVIOUR_FLAGS_TOP;	
								}
								break;
							case CAI_Pathfinder::RIGHT:
								{
									behaviourFlags |= BEHAVIOUR_FLAGS_RIGHT;
								}
								break;
							case CAI_Pathfinder::BACK:
								{
									behaviourFlags |= BEHAVIOUR_FLAGS_BOTTOM;
								}
								break;
							case CAI_Pathfinder::LEFT:
								{
									behaviourFlags |= BEHAVIOUR_FLAGS_LEFT;
								}
								break;

							}
							StopBehaviour(BEHAVIOUR_STOP_FAST,CAI_Action::PRIO_FORCED);
							SetWantedBehaviour2(BEHAVIOUR_COMBAT_BLACKHOLE_SUCKEDIN,CAI_Action::PRIO_FORCED,behaviourFlags,0,NULL,NULL);
						}
					}
					break;
				}
			};
		}

		case OBJMSG_CHAR_AI_RUNBEHAVIOUR:
			{
				// Extract the separate parameters (object,range,active flag) and send them to AH
				CStr Data = (char*)_Msg.m_pData;
				CStr PrioStr = Data.GetStrMSep(",;");
				CStr BehaviourStr = Data.GetStrMSep(",;");
				CStr FlagsStr = Data.GetStrMSep(",;");
				int32 Prio = CAI_Action::StrToPrio(PrioStr);
				int32 iBehaviour = BehaviourStr.Val_int();
				static const char * TranslateRunBehaviourFlags[] =
				{	// Yes, there will be more flags i the future
					"Loop","PerfectPlacement","Direct",NULL
				};
				int32 Flags = FlagsStr.TranslateFlags(TranslateRunBehaviourFlags);
				// Do it properly when more than one flag exists
				int32 behaviourFlags = 0;
				if (Flags & M_Bit(0))
				{
					behaviourFlags |= BEHAVIOUR_FLAGS_LOOP;
				}
				if (Flags & M_Bit(2))
				{
					behaviourFlags |= BEHAVIOUR_FLAGS_DIRECT;
				}
				if (Prio <= 0) { Prio = CAI_Action::PRIO_IDLE; }
				if ((Prio >= CAI_Action::PRIO_FORCED)&&(Prio >= m_BehaviourPrioClass))
				{
					m_CurCB.Clear();
					CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
					CWAG2I_Context Context(m_pGameObject,m_pServer,pCD->m_GameTime);
					pCD->m_AnimGraph2.ForceStance(&Context);
					ResetBehaviour();
					m_BehaviourStopIssued = BEHAVIOUR_STOP_NONE;
				}
				if (Flags & M_Bit(1))
				{
					behaviourFlags |= BEHAVIOUR_FLAGS_PP;
					CMat4Dfp32 Mat;
					GetBaseMat(Mat);
					SetWantedBehaviour2(iBehaviour,Prio,behaviourFlags,-1,NULL,&Mat);
				}
				else
				{
					SetWantedBehaviour2(iBehaviour,Prio,behaviourFlags,-1,NULL,NULL);
				}
				return 1;
			}
			break;
		case OBJMSG_CHAR_AI_STOPBEHAVIOUR:
			{
				//"MSG_SPAWN1" "0x101f;$this;0;5,Direct"
				CStr Data = (char*)_Msg.m_pData;
				CStr PrioStr = Data.GetStrMSep(",;");
				CStr HowFastStr = Data.GetStrMSep(",;");
				int32 Prio = CAI_Action::StrToPrio(PrioStr);
				int32 HowFast = HowFastStr.Val_int();
				if (Prio <= 0) { Prio = CAI_Action::PRIO_IDLE; }
				if (HowFast < BEHAVIOUR_STOP_NORMAL) { HowFast = BEHAVIOUR_STOP_NORMAL; }
				StopBehaviour(HowFast,Prio);
			}
			break;
	};

	//Message not handled
	return 0;
};


//Handles any registry keys of a character that are AI-related
bool CAI_Core::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	if (!_pKey)
		return false;	

	const CStr KeyName = _pKey->GetThisName();
	CStr Value = _pKey->GetThisValue().UpperCase();
	uint32 ValueHash = StringToHash(Value);
	const fp32 Valuef = _pKey->GetThisValuef();
	const int Valuei = _pKey->GetThisValuei();

	//Misc///////////////////////////////////////////////////////////////////
	switch (_KeyHash)
	{
	case MHASH4('AI_D','ESTR','OYTI','MER'): // "AI_DESTROYTIMER"
		{
			//Set destroy timer
			m_DestroyTimer = RoundToInt(Valuef * GetAITicksPerSecond()); 
			break;
		}
	case MHASH3('AI_L','IFET','IMER'): // "AI_LIFETIMER"
		{
			//Set life timer
			m_LifeTimer = RoundToInt(Valuef * GetAITicksPerSecond()); 
			break;
		}
	case MHASH3('AI_L','IFED','RAIN'): // "AI_LIFEDRAIN"
		{
			//Set life drain interval and amount
			fp32 DrainPerFrame = Valuef * GetAITicksPerSecond();
			if (DrainPerFrame != 0)
			{
				if (M_Fabs(DrainPerFrame) > 1)
				{
					m_LifeDrainInterval = 1; 
					m_LifeDrainAmount = RoundToInt(DrainPerFrame);
				}
				else
				{
					m_LifeDrainInterval = RoundToInt(1.0f / M_Fabs(DrainPerFrame));
					m_LifeDrainAmount = (RoundToInt(DrainPerFrame) > 0) ? 1 : -1;
				};
			}
			else
			{
				m_LifeDrainInterval = 1; 
				m_LifeDrainAmount = 0;
			};
			break;
		}
	case MHASH3('AI_I','MMOR','TAL'): // "AI_IMMORTAL"
		{
			//Set health bottom
			m_MinHealth = Valuei;
			break;
		}
	case MHASH5('AI_A','CTIV','ATIO','NRAN','GE'): // "AI_ACTIVATIONRANGE"
		{
			m_ActivateRangeSqr = Sqr(Valuef);
			break;
		}
	case MHASH8('AI_D','ESTR','OYON','DEAC','TIVA','TION','RANG','E'): // "AI_DESTROYONDEACTIVATIONRANGE"
		{
			if (Valuef > 0.0f)
			{
				m_DestroyOnDeactivationRangeSqr = Sqr(Valuef);
			}
			else
			{
				m_DestroyOnDeactivationRangeSqr = -1;
			}
			break;
		}
	case MHASH3('AI_A','CTIV','ATED'): // "AI_ACTIVATED"
		{
			if (Valuei)
				SetActivationState(STATE_ALWAYSACTIVE);
			break;
		}
	case MHASH5('AI_A','CTIV','ATIO','NTIM','E'): // "AI_ACTIVATIONTIME"
		{
			m_ActivationTime = RoundToInt(Valuef * GetAITicksPerSecond());
			break;
		}
	case MHASH3('AI_U','SEFL','AGS'): // "AI_USEFLAGS"
		{
			m_UseFlags = Value.TranslateFlags(TranslateUseFlags);
			break;
		}
	case MHASH3('AI_S','TUCK','TIME'): // "AI_STUCKTIME"
		{
			m_StuckThreshold = (uint8)(Valuef * GetAITicksPerSecond());
			break;
		}
	case MHASH5('AI_S','OFTL','OOKT','HRES','HOLD'): // "AI_SOFTLOOKTHRESHOLD"
		{
			//Given value is degrees
			fp32 Frac = Valuef / 360.0f;
			m_DeviceLook.SetSoftLookThreshold(Frac);
			break;
		}
	case MHASH5('AI_A','LWAY','SSOF','TLOO','K'): // "AI_ALWAYSSOFTLOOK"
		{
			if (Valuei)
			{
				m_DeviceLook.SetAlwaysLookSoft(true);
			}
			else
			{
				m_DeviceLook.SetAlwaysLookSoft(false);
			}
			break;
		}
	// =================================================================
	// Physical ai values here
	// NOTE The input values are NOT similar to *phys_xxx
	// *** When will(?) the *phys_xxx values get straightened out?
	// (Come on - height given from center!)
	// =================================================================
	case MHASH3('AI_B','ASES','IZE'): // "AI_BASESIZE"
		{
			SetBaseSize(Valuef);
			break;
		}
	case MHASH3('AI_H','EIGH','T'): // "AI_HEIGHT"
		{
			SetHeight(Valuef);
			break;
		}
	case MHASH4('AI_H','EIGH','T_CR','OUCH'): // "AI_HEIGHT_CROUCH"
		{
			SetCrouchHeight(Valuef);
			break;
		}
	case MHASH4('AI_S','TEP_','HEIG','HT'): // "AI_STEP_HEIGHT"
		{
			SetStepHeight(Valuef);
			break;
		}
	case MHASH4('AI_J','UMP_','HEIG','HT'): // "AI_JUMP_HEIGHT"
		{
			SetJumpHeight(Valuef);
			break;
		}
	case MHASH5('AI_W','ALK_','STEP','LENG','TH'): // "AI_WALK_STEPLENGTH"
		{
			SetWalkStepLength(Valuef);
			break;
		}
	case MHASH5('AI_R','UN_S','TEPL','ENGT','H'): // "AI_RUN_STEPLENGTH"
		{
			SetRunStepLength(Valuef);
			break;
		}
	// =================================================================
	case MHASH3('AI_A','LERT','NESS'): // "AI_ALERTNESS"
		{
			m_KB.SetAlertness(Value, true);	
			break;
		}
	case MHASH5('AI_A','LERT','NESS','RANG','E'): // "AI_ALERTNESSRANGE"
		{
			m_KB.SetAlertnessRange(Valuef);	
			break;
		}
	case MHASH5('AI_I','NITI','ALAL','ERTN','ESS'): // "AI_INITIALALERTNESS"
		{
			m_KB.SetInitialAlertness(Value);
			break;
		}
	case MHASH3('AI_S','ECUR','ITY'): // "AI_SECURITY"
		{
			m_SecurityTolerance = Valuei;
			break;
		}
	case MHASH3('AI_W','ALLC','LIMB'): // "AI_WALLCLIMB"
		{
			if (Valuei)
			{
				m_bCanWallClimb = true;
				SetWallclimb(true);
			}
			else
			{
				m_bCanWallClimb = false;
				SetWallclimb(false);
			}
			break;
		}
	case MHASH4('AI_S','EEN_','DARK','NESS'): // "AI_SEEN_DARKNESS"
		{
			m_SeenDarknessPowers = Valuei & PLAYER_DARKNESSMODE_AIDARKNESSMASK;
			break;
		}
	case MHASH4('AI_F','EAR_','DARK','NESS'): // "AI_FEAR_DARKNESS"
		{
			if (Valuei)
			{
				m_bFearDarkness = true;
			}
			else
			{
				m_bFearDarkness = false;
			}
			break;
		}
	case MHASH5('AI_C','HARA','CTER','_CLA','SS'): // "AI_CHARACTER_CLASS"
		{
			if (ValueHash == MHASH4('CLAS','S_DE','FAUL','T'))
			{
				m_CharacterClass = CLASS_DEFAULT;
			}
			else if (ValueHash == MHASH3('CLAS','S_CI','V'))
			{
				m_CharacterClass = CLASS_CIV;
			}
			else if (ValueHash == MHASH4('CLAS','S_TO','UGHG','UY'))
			{
				m_CharacterClass = CLASS_TOUGHGUY;
			}
			else if (ValueHash == MHASH3('CLAS','S_BA','DGUY'))
			{
				m_CharacterClass = CLASS_BADGUY;
			}
			else if (ValueHash == MHASH4('CLAS','S_DA','RKLI','NG'))
			{
				m_CharacterClass = CLASS_DARKLING;
			}
			else if (ValueHash == MHASH3('CLAS','S_UN','DEAD'))
			{
				m_CharacterClass = CLASS_UNDEAD;
			}
			break;
		}
	case MHASH4('AI_I','MPOR','TANC','E'): // "AI_IMPORTANCE"
		{
			if (ValueHash == MHASH3('UNIM','PORT','ANT'))
			{
				m_Importance = IMPORTANCE_UNIMPORTANT;
			}
			else if (ValueHash == MHASH2('NORM','AL'))
			{
				m_Importance = IMPORTANCE_NORMAL;
			}
			else if (ValueHash == MHASH3('IMPO','RTAN','T'))
			{
				m_Importance = IMPORTANCE_IMPORTANT;
			}
			else if (ValueHash == MHASH2('CRIT','ICAL'))
			{
				m_Importance = IMPORTANCE_CRITICAL;
			}
			break;
		}
	////////////////////////////////////////////////////////////////////////

	//Bot stats//////////////////////////////////////////////////////////////
	case MHASH4('AI_L','IGHT','METE','R'): // "AI_LIGHTMETER"
		{
			m_UseLightmeter = Valuei;
			break;
		}
	case MHASH2('AI_A','GENT'): // "AI_AGENT"
		{
			if (Valuei)
			{
				m_bAgent = true;
			}
			else
			{
				m_bAgent = false;
			}
			break;
		}
	case MHASH3('AI_A','WARE','NESS'): // "AI_AWARENESS"
		{
			//Set awareness
			m_Awareness = Valuef;
			break;
		}
	case MHASH4('AI_S','IGHT','RANG','E'): // "AI_SIGHTRANGE"
		{
			//Set sight range
			m_SightRange = Valuef;
			break;
		}
	case MHASH4('AI_H','EARI','NGRA','NGE'): // "AI_HEARINGRANGE"
		{
			//Set hearing range
			m_HearingRange = Valuef;
			break;
		}
	case MHASH2('AI_F','OV'): // "AI_FOV"
		{
			//Set field of view
			m_FOV = Valuef  * ( 1.0f / 360.0f );
			m_FOVWide = m_FOV * 2.0f;
			if (m_FOVWide > 1.0f)
			{
				m_FOVWide  = 1.0f;
			}
			break;
		}
	case MHASH3('AI_F','OVWI','DE'): // "AI_FOVWIDE"
		{
			//Set the wide field of view
			fp32 FOVWide = Valuef  * ( 1.0f / 360.0f );
			if (FOVWide < m_FOV)
			{
				FOVWide = m_FOV;
			}
			m_FOVWide = FOVWide;
			if (m_FOVWide > 1.0f)
			{
				m_FOVWide  = 1.0f;
			}
			break;
		}
	case MHASH4('AI_F','AKEF','OVLI','GHT'): // "AI_FAKEFOVLIGHT"
		{	// Set the range of fake light in FOV (100% brightness)
			m_FakeFOVLight = Valuef;
			break;
		}
	case MHASH3('AI_T','URNR','ATE'): // "AI_TURNRATE"
		{	// Convert to turns per tick from turns per secons
			m_TurnPerTick = Valuef * GetAITickDuration();
			break;
		}
	case MHASH5('AI_P','ERCE','PTIO','N_FA','CTOR'): // "AI_PERCEPTION_FACTOR"
		{
			m_PerceptionFactor = Valuef;
			break;
		}
	case MHASH4('AI_L','OUDM','OVEM','ENT'): // "AI_LOUDMOVEMENT"
		{
			m_LoudMovement = Valuef;
			break;
		}
	case MHASH4('AI_N','IGHT','VISI','ON'): // "AI_NIGHTVISION"
		{
			if (Valuei == 0)
			{
				m_PerceptionFlags &= ~PERC_NIGHTVISION_FLAG;
			}
			else
			{
				m_PerceptionFlags |= PERC_NIGHTVISION_FLAG;
			}
			break;
		}
	case MHASH4('AI_X','RAYV','ISIO','N'): // "AI_XRAYVISION"
		{
			if (Valuei == 0)
			{
				m_PerceptionFlags &= ~PERC_XRAYVISION_FLAG;
			}
			else
			{
				m_PerceptionFlags |= PERC_XRAYVISION_FLAG;
			}
			break;
		}
	case MHASH5('AI_H','EART','HROU','GHWA','LLS'): // "AI_HEARTHROUGHWALLS"
		{
			if (Valuei == 0)
			{
				m_PerceptionFlags &= ~PERC_HEARTHROUGHWALLS_FLAG;
			}
			else
			{
				m_PerceptionFlags |= PERC_HEARTHROUGHWALLS_FLAG;
			}
			break;
		}
	case MHASH3('AI_R','EASO','NING'): // "AI_REASONING"
		{
			//Set reasoning
			m_Reasoning = Max(0, Min(100,Valuei));
			break;
		}
	case MHASH4('AI_R','OAM_','MINR','ANGE'): // "AI_ROAM_MINRANGE"
		{
			m_RoamMinRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH4('AI_R','OAM_','MAXR','ANGE'): // "AI_ROAM_MAXRANGE"
		{
			m_RoamMaxRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH5('AI_S','EARC','H_MI','NRAN','GE'): // "AI_SEARCH_MINRANGE"
		{
			m_SearchMinRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH5('AI_S','EARC','H_MA','XRAN','GE'): // "AI_SEARCH_MAXRANGE"
		{
			m_SearchMaxRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH5('AI_R','ANGE','D_MI','NRAN','GE'): // "AI_RANGED_MINRANGE"
		{
			m_RangedMinRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH5('AI_R','ANGE','D_MA','XRAN','GE'): // "AI_RANGED_MAXRANGE"
		{
			m_RangedMaxRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH5('AI_M','ELEE','_MAX','RANG','E'): // "AI_MELEE_MAXRANGE"
		{
			m_MeleeMaxRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH5('AI_C','LOSE','_MIN','RANG','E'): // "AI_CLOSE_MINRANGE"
		{
			m_CloseMinRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH5('AI_C','LOSE','_MAX','RANG','E'): // "AI_CLOSE_MAXRANGE"
		{
			m_CloseMaxRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH5('AI_C','OMBA','T_MI','NRAN','GE'): // "AI_COMBAT_MINRANGE"
		{
			m_CombatMinRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH5('AI_C','OMBA','T_MA','XRAN','GE'): // "AI_COMBAT_MAXRANGE"
		{
			m_CombatMaxRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH7('AI_C','OMBA','T_TA','RGET','_MIN','RANG','E'): // "AI_COMBAT_TARGET_MINRANGE"
		{
			m_CombatTargetMinRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH7('AI_C','OMBA','T_TA','RGET','_MAX','RANG','E'): // "AI_COMBAT_TARGET_MAXRANGE"
		{
			m_CombatTargetMaxRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH5('AI_C','OVER','_MIN','RANG','E'): // "AI_COVER_MINRANGE"
		{
			m_CoverMinRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH5('AI_C','OVER','_MAX','RANG','E'): // "AI_COVER_MAXRANGE"
		{
			m_CoverMaxRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH6('AI_C','OVER','_TAR','GET_','MINR','ANGE'): // "AI_COVER_TARGET_MINRANGE"
		{
			m_CoverTargetMinRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH6('AI_C','OVER','_TAR','GET_','MAXR','ANGE'): // "AI_COVER_TARGET_MAXRANGE"
		{
			m_CoverTargetMaxRange = Max(0.0f,(fp32)Valuef);
			break;
		}
	case MHASH2('AI_S','KILL'): // "AI_SKILL"
		{
			//Set skill
			m_Skill = Max(0, Min(100,Valuei));
			break;
		}
	case MHASH3('AI_B','RAVE','RY'): // "AI_BRAVERY"
		{	//Set bravery
			if (ValueHash == MHASH2('NEVE','R'))
			{
				m_Bravery = BRAVERY_NEVER;
			}
			else if (ValueHash == MHASH2('BETT','ER'))
			{
				m_Bravery = BRAVERY_BETTER;
			}
			else if (ValueHash == MHASH2('EQUA','L'))
			{
				m_Bravery = BRAVERY_EQUAL;
			}
			else if (ValueHash == MHASH2('ONE_','LESS'))
			{
				m_Bravery = BRAVERY_ONE_LESS;
			}
			else if (ValueHash == MHASH2('ALWA','YS'))
			{
				m_Bravery = BRAVERY_ALWAYS;
			}
			else
			{
				m_Bravery = Max(0, Min(4,Valuei));
			}
			m_DefaultBravery = m_Bravery;
			break;
		}
	case MHASH5('AI_M','ELEE','_SEN','SITI','VE'): // "AI_MELEE_SENSITIVE"
		{
			m_MeleeRelationIncrease = Valuei;
			break;
		}
	case MHASH3('AI_P','ATIE','NCE'): // "AI_PATIENCE"
		{
			//Set patience
			// Patience is entered in seconds from the template but stored as
			// ticks for engine convenience.
			m_Patience = Max(int(0),int(Valuei * GetAITicksPerSecond()));
			break;
		}
	case MHASH7('AI_R','ELAT','ION_','CHAN','GE_A','WARE','NESS'): // "AI_RELATION_CHANGE_AWARENESS"
		{
			// Minimum awareness for relation checks (default NOTICED!)
			m_RelationChangeAwareness = Value.TranslateInt(CAI_AgentInfo::ms_TranslateAwareness);
			break;
		}
	case MHASH3('AI_R','EACT','ION'): // "AI_REACTION"
		{
			//Set reaction time
			m_Reaction = Max(int(0),int(Valuei * GetAITicksPerSecond()));
			break;
		}
	case MHASH3('AI_H','ITRA','TIO'): // "AI_HITRATIO"
		{
			//Set hit ratio
			m_HitRatio = Max(0, Min(100,Valuei));
			break;
		}
	case MHASH3('AI_A','IMBO','NUS'): // "AI_AIMBONUS"
		{
			m_StationaryBonus = Valuef * GetAITicksPerSecond();
			break;
		}
	case MHASH4('AI_S','HOTI','NTER','VAL'): // "AI_SHOTINTERVAL"
		{
			m_DeviceWeapon.SetPeriod((Valuei * GetAITicksPerSecond()));
			break;
		}
	case MHASH4('AI_F','RIEN','DLYF','IRE'): // "AI_FRIENDLYFIRE"
		{
			if (Valuef >= 0.0f)
			{
				m_FriendlyFireFactor = Valuef;
			}
			break;
		}
	case MHASH3('AI_N','EXTO','FKIN'): // "AI_NEXTOFKIN"
		{
			if (Valuei != 0)
			{
				m_bNextOfKin = true;
			}
			else
			{
				m_bNextOfKin = false;
			}
			break;
		}
	case MHASH6('AI_2','ND_W','EAPO','N_AN','IMTY','PE'): // "AI_2ND_WEAPON_ANIMTYPE"
		{
			if (Valuei > 0)
			{
				m_SecondaryWeaponAnimType = Valuei;
			}
			else
			{
				m_SecondaryWeaponAnimType = 0;
			}
			break;
		}
	case MHASH5('AI_A','GONY','BEHA','VIOU','R'): // "AI_AGONYBEHAVIOUR"
		{
			if (Valuei > 0)
			{
				m_AgonyBehaviour = Valuei;
			}
			else
			{
				m_AgonyBehaviour = 0;
			}
			break;
		}
	case MHASH5('AI_U','NDEA','D_SL','EEPT','IME'): // "AI_UNDEAD_SLEEPTIME"
		{
			if (Valuei == -1)
			{
				m_UndeadSleepTicks = -1;
			}
			else
			{
				m_UndeadSleepTicks = (Valuei * GetAITicksPerSecond());
			}
			break;
		}
	case MHASH5('AI_U','NDEA','D_BE','HAVI','OUR'): // "AI_UNDEAD_BEHAVIOUR"
		{
			m_UndeadDieBehaviour = Valuei;
			break;
		}
	////////////////////////////////////////////////////////////////////////
	//Personality///////////////////////////////////////////////////////////
	case MHASH4('AI_P','ERSO','NALI','TY'): // "AI_PERSONALITY"
		{
			m_Personality.SetPersonality(CAI_Personality::NORMAL);
			/*
			//Set personality
			if (Value.CompareNoCase("NORMAL") == 0)
			{
				m_Personality.SetPersonality(CAI_Personality::NORMAL);
			}
			else if (Value.CompareNoCase("BERSERKER") == 0)
			{
				m_Personality.SetPersonality(CAI_Personality::NORMAL);
			}
			else if (Value.CompareNoCase("COWARD") == 0)
			{
				m_Personality.SetPersonality(CAI_Personality::NORMAL);
			}
			else if (Value.CompareNoCase("SNEAKER") == 0)
			{
				m_Personality.SetPersonality(CAI_Personality::NORMAL);
			}
			else if (Value.CompareNoCase("PASSIVE") == 0)
			{
				m_Personality.SetPersonality(CAI_Personality::NORMAL);
			}
			*/
			break;
		}
	////////////////////////////////////////////////////////////////////////

	//Behaviour/////////////////////////////////////////////////////////////
	case MHASH3('AI_B','EHAV','IOUR'): // "AI_BEHAVIOUR"
		{
			//Set behaviour to be created
			if (Value.CompareNoCase("PATROL") == 0)
				m_ActionFromKey = CAI_Core::PATROL;
			else if (Value.CompareNoCase("BEHAVIOUR") == 0)
				m_ActionFromKey = CAI_Core::BEHAVIOUR;
			else if (Value.CompareNoCase("EXPLORE") == 0)
				m_ActionFromKey = CAI_Core::EXPLORE;
			else if (Value.CompareNoCase("HOLD") == 0)
				m_ActionFromKey = CAI_Core::HOLD;
			else if (Value.CompareNoCase("ENGAGE") == 0)
				m_ActionFromKey = CAI_Core::ENGAGE;
			else
			{
				ConOut(CStr("Unkown default action"));
				return false;
			}
			break;
		}
	case MHASH4('AI_D','EBUG','TARG','ET'): // "AI_DEBUGTARGET"
		{
			if (Valuei)
			{
				SetDebugTarget(GetObjectID());
			}
			break;
		}
	case MHASH5('AI_H','OSTI','LE_F','ROM_','BLOW'): // "AI_HOSTILE_FROM_BLOW"
		{
			if (Valuei)
			{
				m_bHostileFromBlowDmg = true;
			}
			else
			{
				m_bHostileFromBlowDmg = false;
			}
			break;
		}
	case MHASH7('AI_P','LAYE','RONL','Y_ME','LEED','AMAG','E'): // "AI_PLAYERONLY_MELEEDAMAGE"
		{
			if (Valuei)
			{
				m_bPlayerOnlyMeleeDmg = true;
			}
			else
			{
				m_bPlayerOnlyMeleeDmg = false;
			}
			break;
		}
	case MHASH5('AI_D','AMAG','EFRO','MPLA','YER'): // "AI_DAMAGEFROMPLAYER"
		{
			m_DamageFromPlayerFactor = Valuef;
			break;
		}
	case MHASH4('AI_D','EATH','SPOT','S'): // "AI_DEATHSPOTS"
		{
			if (Valuei)
			{
				m_bDeathspots = true;
			}
			else
			{
				m_bDeathspots = false;
			}
			break;
		}
	case MHASH4('AI_','WALK','STAR','TS'):	// "AI_WALKSTARTS"
		{
			if (Valuei)
			{
				m_bWalkStarts = true;
			}
			else
			{
				m_bWalkStarts = false;
			}
			break;
		}
	case MHASH4('AI_M','OVEV','ARIA','NT'): // "AI_MOVEVARIANT"
		{
			if (Valuef != 0)
			{
				m_iMoveVariant = MOVEVARIATION_NORMAL;
			}
			else if (Value.CompareNoCase("NORMAL") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_NORMAL;
			}
			else if (Value.CompareNoCase("ALKY") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_ALKY;
			}
			else if (Value.CompareNoCase("FEMALE1_1") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_FEMALE1_1;
			}
			else if (Value.CompareNoCase("FEMALE1_2") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_FEMALE1_2;
			}
			else if (Value.CompareNoCase("FEMALE2_1") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_FEMALE2_1;
			}
			else if (Value.CompareNoCase("FEMALE2_2") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_FEMALE2_2;
			}
			else if (Value.CompareNoCase("FEMALE3_1") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_FEMALE3_1;
			}
			else if (Value.CompareNoCase("FEMALE3_2") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_FEMALE3_2;
			}
			else if (Value.CompareNoCase("FEMALE4_1") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_FEMALE4_1;
			}
			else if (Value.CompareNoCase("FEMALE4_2") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_FEMALE4_2;
			}
			else if (Value.CompareNoCase("FEMALE5_1") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_FEMALE5_1;
			}
			else if (Value.CompareNoCase("FEMALE5_2") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_FEMALE5_2;
			}
			else if (Value.CompareNoCase("GANGSTER1") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_GANGSTER1;
			}
			else if (Value.CompareNoCase("GANGSTER2") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_GANGSTER2;
			}
			else if (Value.CompareNoCase("GANGSTER3") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_GANGSTER3;
			}
			else if (Value.CompareNoCase("HURT") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_HURT;
			}
			else if (Value.CompareNoCase("MALE1_1") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_MALE1_1;
			}
			else if (Value.CompareNoCase("MALE1_2") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_MALE1_2;
			}
			else if (Value.CompareNoCase("MALE2_1") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_MALE2_1;
			}
			else if (Value.CompareNoCase("MALE2_2") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_MALE2_2;
			}
			else if (Value.CompareNoCase("MALE3_1") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_MALE3_1;
			}
			else if (Value.CompareNoCase("MALE3_2") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_MALE3_2;
			}
			else if (Value.CompareNoCase("MALE4_1") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_MALE4_1;
			}
			else if (Value.CompareNoCase("MALE4_2") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_MALE4_2;
			}
			else if (Value.CompareNoCase("MALE5_1") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_MALE5_1;
			}
			else if (Value.CompareNoCase("MALE5_2") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_MALE5_2;
			}
			else if (Value.CompareNoCase("MALE6_1") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_MALE6_1;
			}
			else if (Value.CompareNoCase("MALE6_2") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_MALE6_2;
			}
			else if (Value.CompareNoCase("OLDGUY1") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_OLDGUY1;
			}
			else if (Value.CompareNoCase("OLDGUY2") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_OLDGUY2;
			}
			else if (Value.CompareNoCase("OLDLADY1") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_OLDLADY1;
			}
			else if (Value.CompareNoCase("OLDLADY2") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_OLDLADY2;
			}
			else if (Value.CompareNoCase("HORSESLEEP") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_HORSESLEEP;
			}
			else if (Value.CompareNoCase("LIMP") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_LIMP;
			}
			else if (Value.CompareNoCase("BUTCHER") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_BUTCHER;
			}
			else if (Value.CompareNoCase("SWAT") == 0)
			{
				m_iMoveVariant = MOVEVARIATION_SWAT;
			}
			break;
		}
	case MHASH3('AI_I','DLES','PEED'): // "AI_IDLESPEED"
		{
			if (Valuef != 0)
			{
				m_IdleMaxSpeed = Valuef;
				m_ReleaseIdleSpeed = m_IdleMaxSpeed;
			}
			else if (Value.CompareNoCase("WALK SLOW") == 0)
			{
				m_IdleMaxSpeed = GetMaxSpeedForward() * AI_MOVE_THRESHOLD_WALKSLOW;
				m_ReleaseIdleSpeed = m_IdleMaxSpeed;
			}
			else if (Value.CompareNoCase("WALK") == 0)
			{
				m_IdleMaxSpeed = GetMaxSpeedForward() * AI_MOVE_THRESHOLD_WALK;
				m_ReleaseIdleSpeed = m_IdleMaxSpeed;
			}
			else if (Value.CompareNoCase("WALK FAST") == 0)
			{
				m_IdleMaxSpeed = GetMaxSpeedForward() * AI_MOVE_THRESHOLD_WALKFAST;
				m_ReleaseIdleSpeed = m_IdleMaxSpeed;
			}
			else	// Run
			{
				m_IdleMaxSpeed = GetMaxSpeedForward();
				m_ReleaseIdleSpeed = m_IdleMaxSpeed;
			}
			break;
		}
	case MHASH3('AI_S','TANC','E'): // "AI_STANCE"
		{
			// NOTE: I cannot really set the stance at this stage as we don't yet have a character and therefore the
			// SetDefaultStance call will fail.
			if (Value.CompareNoCase("IDLE") == 0)
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_IDLE,false);
			}
			else if (Value.CompareNoCase("HOSTILE") == 0)
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_HOSTILE,false);
			}
			else if (Value.CompareNoCase("COMBAT") == 0)
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_COMBAT,false);
			}
			else if (Valuei == 0)
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_IDLE,false);
			}
			else if (Valuei == 1)
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_HOSTILE,false);
			}
			else if (Valuei == 2)
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_COMBAT,false);
			}
			else
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_IDLE,false);
			}
			break;
		}
	case MHASH5('AI_D','EFAU','LT_S','TANC','E'): // "AI_DEFAULT_STANCE"
		{
			// NOTE: I cannot really set the stance at this stage as we don't yet have a character and therefore the
			// SetDefaultStance call will fail.
			if (Value.CompareNoCase("IDLE") == 0)
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_IDLE,false);
			}
			else if (Value.CompareNoCase("HOSTILE") == 0)
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_HOSTILE,false);
			}
			else if (Value.CompareNoCase("COMBAT") == 0)
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_COMBAT,false);
			}
			else if (Valuei == 0)
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_IDLE,false);
			}
			else if (Valuei == 1)
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_HOSTILE,false);
			}
			else if (Valuei == 2)
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_COMBAT,false);
			}
			else
			{
				m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_IDLE,false);
			}
			break;
		}
	case MHASH4('AI_M','AX_S','TANC','E'): // "AI_MAX_STANCE"
		{
			// NOTE: I cannot really set the stance at this stage as we don't yet have a character and therefore the
			// SetDefaultStance call will fail.
			if (Value.CompareNoCase("IDLE") == 0)
			{
				m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_IDLE,false);
			}
			else if (Value.CompareNoCase("HOSTILE") == 0)
			{
				m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_HOSTILE,false);
			}
			else if (Value.CompareNoCase("COMBAT") == 0)
			{
				m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_COMBAT,false);
			}
			else if (Valuei == 0)
			{
				m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_IDLE,false);
			}
			else if (Valuei == 1)
			{
				m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_HOSTILE,false);
			}
			else if (Valuei == 2)
			{
				m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_COMBAT,false);
			}
			else
			{
				m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_IDLE,false);
			}
			break;
		}
	case MHASH4('AI_F','ACIA','LGRO','UP'): // "AI_FACIALGROUP"
		{
			if (Valuef != 0)
			{
				m_iFacialGroup = CAI_Device_Facial::FACIALGROUP_INVALID;
			}
			else if (Value.CompareNoCase("NONE") == 0)
			{
				m_iFacialGroup = CAI_Device_Facial::FACIALGROUP_INVALID;
			}
			else if (Value.CompareNoCase("HUNTER") == 0)
			{
				m_iFacialGroup = CAI_Device_Facial::FACIALGROUP_HUNTER;
			}
			else if (Value.CompareNoCase("BUTCHER") == 0)
			{
				m_iFacialGroup = CAI_Device_Facial::FACIALGROUP_BUTCHER;
			}
			else if (Value.CompareNoCase("JENNY") == 0)
			{
				m_iFacialGroup = CAI_Device_Facial::FACIALGROUP_JENNY;
			}
			else if (Value.CompareNoCase("SARAH") == 0)
			{
				m_iFacialGroup = CAI_Device_Facial::FACIALGROUP_SARAH;
			}
			else if (Value.CompareNoCase("CIVMALE3") == 0)
			{
				m_iFacialGroup = CAI_Device_Facial::FACIALGROUP_CIVMALE3;
			}
			else if (Value.CompareNoCase("Extreme") == 0)
			{
				m_iFacialGroup = CAI_Device_Facial::FACIALGROUP_EXTREME;
			}
			break;
		}
	case MHASH4('AI_F','LIGH','TSPE','ED'): // "AI_FLIGHTSPEED"
		{
			m_ReleaseFlightSpeed = Valuef;
			break;
		}
	case MHASH5('AI_F','LIGH','TSPE','EDFA','CTOR'): // "AI_FLIGHTSPEEDFACTOR"
		{
			m_CurFlightSpeedFactor = Valuef;
			break;
		}
	case MHASH4('AI_E','VADE','SPEE','D'): // "AI_EVADESPEED"
		{
			m_MaxEvadeSpeed = Valuef;
			break;
		}
	case MHASH3('AI_M','AXSP','EED'): // "AI_MAXSPEED"
		{
			m_ReleaseMaxSpeed = Valuef;
			break;
		}
	case MHASH4('AI_M','INPR','OPEL','STEP'): // "AI_MINPROPELSTEP"
		{
			m_MinPropelStep = Valuef;
			break;
		}
	case MHASH5('AI_F','ALLE','NBEH','INDR','ANGE'): // "AI_FALLENBEHINDRANGE"
		{
			m_FallenBehindRangeSqr = Sqr(Valuef);
			break;
		}
	case MHASH4('AI_C','AUGH','TUPR','ANGE'): // "AI_CAUGHTUPRANGE"
		{
			m_CaughtUpRangeSqr = Sqr(Valuef);
			break;
		}
	case MHASH4('AI_C','OLLI','SION','RANK'): // "AI_COLLISIONRANK"
		{
			if (Valuei >= -1)
			{
				m_CollisionRank = Valuei;
			}
			break;
		}
	////////////////////////////////////////////////////////////////////////

	//Events////////////////////////////////////////////////////////////////
	case MHASH4('AI_O','NSTA','RT_P','AUSE'): // "AI_ONSTART_PAUSE"
		{
			OnImpulse(IMPULSE_PAUSE);
			break;
		}
	case MHASH5('AI_O','NSTA','RT_I','MMUN','E'): // "AI_ONSTART_IMMUNE"
		{
			if (m_pGameObject)
				m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_IMMUNE, 1));
			break;
		}
	case MHASH6('AI_O','NSTA','RT_A','GGRE','SSIV','E'): // "AI_ONSTART_AGGRESSIVE"
		{
			OnImpulse(IMPULSE_AGGRESSIVE);
			break;
		}
	case MHASH5('AI_O','NALA','RM_R','ELEA','SE'): // "AI_ONALARM_RELEASE"
		{
			CWO_SimpleMessage Msg;
			Msg.m_Msg = OBJMSG_SCRIPTIMPULSE;
			Msg.m_Param0 = IMPULSE_RELEASE;
			Msg.m_iSpecialTarget = CWO_SimpleMessage::SPECIAL_TARGET_THIS;
			m_EventHandler.AddEventMessage(CAI_EventHandler::ON_ALARM,-1,Msg); 
			break;
		}
	case MHASH4('AI_O','NSPA','WN_P','USH'): // "AI_ONSPAWN_PUSH"
		{
			CWO_SimpleMessage Msg;
			Msg.m_Msg = OBJMSG_SCRIPTIMPULSE;
			Msg.m_Param0 = IMPULSE_PUSH;
			Msg.m_iSpecialTarget = CWO_SimpleMessage::SPECIAL_TARGET_THIS;
			Msg.m_StrParam = Value;
			m_EventHandler.AddEventMessage(CAI_EventHandler::ON_SPAWN,-1,Msg);
			break;
		}
	case MHASH4('AI_C','USTO','M_EV','ENTS'): // "AI_CUSTOM_EVENTS"
		{
			//Deprecated, only in use for backwards compatibility

			//Set the custom events which have been checked
			static const char * CustomEventsTranslate[] =
			{
				"OnEnemySpotted: Release", //Bit 1
					"OnStart: Pause", //Bit 2
					"OnStart: Immune", //Bit 3
					"OnStart: Aggressive", //Bit 4
					NULL
			};

			if (Value != "0")
			{
				int CustomEvents = Value.TranslateFlags(CustomEventsTranslate);

				//OnEnemySpotted: Release?
				if (CustomEvents & 1)
				{
					CWO_SimpleMessage Msg;
					Msg.m_Msg = OBJMSG_SCRIPTIMPULSE;
					Msg.m_Param0 = IMPULSE_RELEASE;
					Msg.m_iSpecialTarget = CWO_SimpleMessage::SPECIAL_TARGET_THIS;
					m_EventHandler.AddEventMessage(CAI_EventHandler::ON_SPOT_ENEMY,-1,Msg); 
				};

				//OnStart: Pause?
				if (CustomEvents & 2)
				{
					OnImpulse(IMPULSE_PAUSE);
				};

				//OnStart: Immune?
				if (CustomEvents & 4)
				{
					if (m_pGameObject)
						m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_IMMUNE, 1));
				};

				//OnStart: Aggressive?
				if (CustomEvents & 8)
				{
					OnImpulse(IMPULSE_AGGRESSIVE);
				};

			};

			return true;
			break;
		}
	default:
		{
			if (KeyName.CompareSubStr("CB_") == 0)
			{	// Syntax CB_XXX <iBehaviour> <iPrev> <iNext> <TacFlags> <Duration>
				// CombatBehaviourxx CrLeanRightMH CrMH TACFLAGS_RIGHT+TACFLAGS_CROUCH 60
				// NOTE: Replace xx with something to make the param unique
				S_CombatBehaviour newCB;
				int16 iBehaviour = Value.GetIntMSep(" ,;:");
				int16 iPrev = Value.GetIntMSep(" ,;:");
				int16 iNext = Value.GetIntMSep(" ,;:");
				CStr tempStr = Value.GetStrMSep(" ,;:");
				tempStr.MakeUpperCase();
				static const char *FlagsTranslate[] =
				{
					"LEFT", "RIGHT", "STAND", "CROUCH", "POPUP", "COWARD", "EXIT", "CHARGE", "FIRE", "POSTURE", NULL
				};
				int16 TacFlags = tempStr.TranslateFlags(FlagsTranslate);
				fp32 Temp = (fp32)Value.Getfp64MSep(" ,;:");
				int16 Duration = (int16)(Temp * GetAITicksPerSecond());
				static const char *UseFlagsTranslate[] =
				{
					"FastExit", "WideMove", NULL
				};
				int16 UseFlags = Value.TranslateFlags(UseFlagsTranslate);
				if ((iBehaviour)&&(TacFlags))
				{
					newCB.Set(iBehaviour,iPrev,iNext,TacFlags,Duration,UseFlags);
		#ifndef M_RTM
					newCB.m_DbgName = KeyName;
		#endif
					m_lCombatBehaviour.Add(newCB);
				}
			}
			else if (KeyName.CompareSubStr("AI_RESTRICT") == 0)
			{
				if (m_AH.m_spRestriction == NULL)
				{
					m_AH.m_spRestriction = MNew1(CAI_Action_Restrict, &m_AH);
				}

				switch (_KeyHash)
				{
				case MHASH4('AI_R','ESTR','ICTA','TTR'): // "AI_RESTRICTATTR"
					{
						int Flag = Value.Val_int();
						m_AH.m_spRestriction->SetParameter(CAI_Action_Restrict::PARAM_ACTIVE,Flag);
						break;
					}
				case MHASH5('AI_R','ESTR','ICT_','ACTI','VE'): // "AI_RESTRICT_ACTIVE"
					{
						int Flag = Value.Val_int();
						m_AH.m_spRestriction->SetParameter(CAI_Action_Restrict::PARAM_ACTIVE,Flag);
						break;
					}
				case MHASH5('AI_R','ESTR','ICT_','TARG','ET'): // "AI_RESTRICT_TARGET"
					{
						m_AH.m_spRestriction->m_sObjName = Value;
						break;
					}
				case MHASH6('AI_R','ESTR','ICT_','MAXD','ISTA','NCE'): // "AI_RESTRICT_MAXDISTANCE"
					{
						fp32 Range = Value.Val_fp64();
						m_AH.m_spRestriction->SetParameter(CAI_Action_Restrict::PARAM_RANGE,Range);
						break;
					}
				default:
					{
						break;
					}
				}
			}
			else if (KeyName.CompareSubStr("AI_ACTION") == 0)
			{
				//Handle *ai_actions and *ai_actionsets in actionhandler
				m_AH.OnEvalKey(_KeyHash, _pKey);
			}
			else if ((_KeyHash == MHASH2('AI_P','ATH'))||
				(_KeyHash == MHASH3('AI_L','EADE','R'))||
				(_KeyHash == MHASH3('AI_T','ARGE','T'))||
				(_KeyHash == MHASH3('AI_P','OSIT','ION'))||
				(_KeyHash == MHASH3('AI_D','ISTA','NCE'))||
				(_KeyHash == MHASH4('AI_M','AXDI','STAN','CE'))||
				(_KeyHash == MHASH2('AI_A','REA'))||
				(_KeyHash == MHASH3('AI_S','TANC','E'))||
				(_KeyHash == MHASH5('AI_B','EHAV','IOUR','_VAL','UE'))||
				(_KeyHash == MHASH6('AI_B','EHAV','IOUR','_INT','ERVA','L'))||
				(_KeyHash == MHASH5('AI_B','EHAV','IOUR','_GOA','L'))||
				(_KeyHash == MHASH5('AI_B','EHAV','IOUR','_RAN','DOM'))||
				(_KeyHash == MHASH5('AI_B','EHAV','IOUR','_RAN','GE'))||
				(_KeyHash == MHASH7('AI_B','EHAV','IOUR','_GOA','L_RE','LATI','ON'))||
				(_KeyHash == MHASH6('AI_B','EHAV','IOUR','_GOA','LLOO','K'))||
				(KeyName.CompareSubStr("AI_BHVR_PARAM") == 0))
			{	
				//Add this key to behaviour parameters
				m_lActionParamKeys.Add(CNameValue(KeyName, m_pServer->World_MangleTargetName(Value)));
			}
			else if (KeyName.CompareSubStr("AI_IMPULSE") == 0)
			{
				//Handle impulse
				OnImpulse(Valuei, 0);
			}
			else if (KeyName.CompareSubStr("AI_EVENT_") == 0)
			{
				m_EventHandler.AddEventMessage(_pKey);
			}
			//No key that we recognize, send back with failure
			else
				return false;
			break;
		}
	}

	return true;
};

void CAI_Core::OnFinishEvalKeys()
{
	// Set initial alertness if the object doesn't get an alertness from template
	int InitialAlertness = m_KB.GetInitialAlertness();
	if((m_KB.GetAlertness() == CAI_KnowledgeBase::ALERTNESS_INVALID)
		&& (InitialAlertness != CAI_KnowledgeBase::ALERTNESS_INVALID))
	{
		m_KB.SetAlertness(InitialAlertness, true);
	};


};

int CAI_Core::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	int32 nbrAdded = 0;
	if (m_CharacterClass != CLASS_DEFAULT)
	{	// *** NOTE: Come up with a way to precache target berserker behaviours
		// (If we could somehow have the darklings precache targets as well?)
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_AXE);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_AXE_SHORT);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_BAT);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_BAT_SHORT);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_HAMMER);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_HAMMER_SHORT);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_MACHETE);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_MACHETE_SHORT);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_PISTOL);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_SAW);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_SAW_SHORT);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_SLEDGEHAMMER);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_SLEDGEHAMMER_SHORT);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_TEETH);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BERZERKER_TEETH_SHORT);
	}
	
	if (m_AgonyBehaviour)
	{
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,m_AgonyBehaviour);
	}

	/*
	if ((m_CharacterClass == CLASS_BADGUY)&&(m_bFearDarkness))
	{
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_COMBAT_GESTURE_SCARED);
	}
	*/

	nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_COMBAT_BLACKHOLE_STAND);
	nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_COMBAT_BLACKHOLE_SUCKEDIN);

	if (m_UseFlags & USE_DOORSPS)
	{
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_OPENDOOR_IN);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_OPENDOOR_IN_KNOB);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_OPENDOOR_OUT);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_OPENDOOR_OUT_KNOB);
	}

	if (m_UndeadDieBehaviour)
	{
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,m_UndeadDieBehaviour);
	}

	if (m_lCombatBehaviour.Len() > 0)
	{
		for (int i = 0; i < m_lCombatBehaviour.Len(); i++)
		{
			nbrAdded += CAI_Action::AddUnique(_liBehaviours,m_lCombatBehaviour[i].m_iBehaviour);
		}
	}

	return(nbrAdded);
};

int CAI_Core::GetUsedGesturesAndMoves(TArray<int16>& _liGestures, TArray<int16>& _liMoves) const
{
	int32 nbrAdded = 0;

	// Add gestures here
	nbrAdded += CAI_Action::AddUnique(_liGestures,GESTURE_HUMAN_CHECK_LEFTRIGHT);

	return(nbrAdded);
};

int CAI_Core::GetUsedAcs(TArray<int16>& _liAcs) const
{
	int32 nbrAdded = 0;

	// Add gestures here
	return(nbrAdded);
};

//Should run when world has been spawned
void CAI_Core::OnSpawnWorld()
{
	//Precache event messages
	m_EventHandler.OnPrecacheMessages();
	m_Weapon.Init();
	m_Item.Init();
	m_AH.OnSpawn();

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
	if (pCD)
	{
		// Check for worldspace movement
		if (m_CharacterClass == CLASS_DARKLING)
		{
			CWO_CharDarkling_ClientData* pDarklingCD = safe_cast<CWO_CharDarkling_ClientData>(pCD);	
			if ((pDarklingCD)&&(pDarklingCD->m_Flags & DARKLING_FLAGS_WORLDSPACE))
			{
				m_bWorldspaceMovement = true;
			}
		}

		if (m_pGameObject)
		{	//Collect all behaviours and hand them to the AG
			TArray<int16> liBehaviours;
			TArray<int16> liGestures;
			TArray<int16> liMoves;
			TArray<int16> liAcss;
			GetUsedBehaviours(liBehaviours);
			GetUsedGesturesAndMoves(liGestures,liMoves);
			GetUsedAcs(liAcss);
			m_AH.GetUsedBehaviours(liBehaviours);
			m_AH.GetUsedGesturesAndMoves(liGestures,liMoves);
			m_AH.GetUsedAcs(liAcss);
			int WeaponType = 0;	// RPG_EQUIPPEDITEMCLASS_UNDEFINED
			if (true)
			{
				CAI_Weapon* pWeapon = m_Weapon.GetWielded();
				if (pWeapon)
				{
					CRPG_Object_Item* pItem = pWeapon->GetItem();
					if (pItem)
					{
						WeaponType = pItem->m_AnimType;
					}
				}
				CWAG2I_Context Context2(m_pGameObject,m_pServer,pCD->m_GameTime);
				TArray<CXRAG2_Impulse> lImpulses;
				lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED));
				lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,AG2_IMPULSEVALUE_WEAPONTYPE_UNARMEDCROUCH));
				lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,WeaponType + AG2_IMPULSEVALUE_STANCETYPEOFFSET * AG2_STANCETYPE_IDLE));
				if (m_CharacterClass == CLASS_CIV)
				{
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,WeaponType + AG2_IMPULSEVALUE_STANCETYPEOFFSET * AG2_STANCETYPE_PANIC));
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,WeaponType + AG2_IMPULSEVALUE_STANCETYPEOFFSET * AG2_STANCETYPE_WARY));
					// Precache crouch as well..
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,WeaponType + AG2_IMPULSEVALUE_STANCETYPEOFFSET * AG2_STANCETYPE_PANIC + AG2_IMPULSEVALUE_NUMWEAPONTYPES));
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,WeaponType + AG2_IMPULSEVALUE_STANCETYPEOFFSET * AG2_STANCETYPE_WARY + AG2_IMPULSEVALUE_NUMWEAPONTYPES));
				}
				else
				{
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,WeaponType + AG2_IMPULSEVALUE_STANCETYPEOFFSET * AG2_STANCETYPE_HOSTILE));
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,WeaponType + AG2_IMPULSEVALUE_STANCETYPEOFFSET * AG2_STANCETYPE_COMBAT));
					// Precache crouch as well.
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,WeaponType + AG2_IMPULSEVALUE_STANCETYPEOFFSET * AG2_STANCETYPE_HOSTILE + AG2_IMPULSEVALUE_NUMWEAPONTYPES));
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,WeaponType + AG2_IMPULSEVALUE_STANCETYPEOFFSET * AG2_STANCETYPE_COMBAT + AG2_IMPULSEVALUE_NUMWEAPONTYPES));
				}
				if ((m_SecondaryWeaponAnimType)&&(m_SecondaryWeaponAnimType != WeaponType))
				{
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,m_SecondaryWeaponAnimType + AG2_IMPULSEVALUE_STANCETYPEOFFSET * AG2_STANCETYPE_IDLE));
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,m_SecondaryWeaponAnimType + AG2_IMPULSEVALUE_STANCETYPEOFFSET * AG2_STANCETYPE_HOSTILE));
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,m_SecondaryWeaponAnimType + AG2_IMPULSEVALUE_STANCETYPEOFFSET * AG2_STANCETYPE_COMBAT));
					// Precache crouch as well.
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,m_SecondaryWeaponAnimType + AG2_IMPULSEVALUE_STANCETYPEOFFSET * AG2_STANCETYPE_HOSTILE + AG2_IMPULSEVALUE_NUMWEAPONTYPES));
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,m_SecondaryWeaponAnimType + AG2_IMPULSEVALUE_STANCETYPEOFFSET * AG2_STANCETYPE_COMBAT + AG2_IMPULSEVALUE_NUMWEAPONTYPES));
				}
				// Precache move variants
				TArray<CXRAG2_Impulse> lReactionImpulses;
				lReactionImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_MOVEVAR,AG2_IMPULSEVALUE_MOVEVAR_IDLE));
				lReactionImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_MOVEVAR,AG2_IMPULSEVALUE_MOVEVAR_WALKFWD));
				lReactionImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_MOVEVAR,AG2_IMPULSEVALUE_MOVEVAR_RUNFWD));
				TArray<int32> lMoveVariants;
				lMoveVariants.Add(m_iMoveVariant);
				pCD->m_AnimGraph2.GetAG2I()->TagAnimSetFromBlockReactionSwitchState(&Context2,m_pServer->GetMapData(),m_pServer->m_spWData,lImpulses,lReactionImpulses,lMoveVariants);
				for (int i = 0; i < liBehaviours.Len(); i++)
				{
					if ((liBehaviours[i] >= BEHAVIOUR_COMBAT_MIN)&&(liBehaviours[i] <= BEHAVIOUR_COMBAT_MAX))
					{	// 
						if (WeaponType == AG2_IMPULSEVALUE_WEAPONTYPE_RIFLE)
						{	// Rifle
							lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR,liBehaviours[i]+BEHAVIOUR_COMBAT_RIFLE_OFFSET));
						}
						else
						{	// Gun
							lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR,liBehaviours[i]));
						}
					}
					else
					{
						lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR,liBehaviours[i]));
					}
				}
				for (int i = 0; i < liGestures.Len(); i++)
				{
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_SHORTGESTURE,liGestures[i]));
				}
				for (int i = 0; i < liMoves.Len(); i++)
				{
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_SHOOTGESTURE,liMoves[i]));
				}
				// We need to apply these impulses on the player
				if (liAcss.Len())
				{
					CWObject_GameCore* pGame = safe_cast<CWObject_GameCore>(m_pServer->Game_GetObject());	
					if (pGame)
					{
						int32 iPlayer = pGame->Player_GetNum() && pGame->Player_GetObject(0) ? pGame->Player_GetObject(0)->m_iObject : pGame->m_iDummyPlayer;
						if (iPlayer > 0)
						{
							CWObject* pPlayer = m_pServer->Object_Get(iPlayer);
							CWO_Character_ClientData* pPlayerCD = CWObject_Character::GetClientData(pPlayer);
							if ((pPlayer)&&(pPlayerCD))
							{
								TArray<CXRAG2_Impulse> lAcsImpulses;
								for (int i = 0; i < liAcss.Len(); i++)
								{
									lAcsImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,liAcss[i]));
								}
								CWAG2I_Context PlayerContext(pPlayer,m_pServer,pPlayerCD->m_GameTime);
								pPlayerCD->m_AnimGraph2.GetAG2I()->TagAnimSetFromImpulses(&PlayerContext,m_pServer->GetMapData(),m_pServer->m_spWData,lAcsImpulses);
							}
						}
					}
				}

				if (m_iFacialGroup != CAI_Device_Facial::FACIALGROUP_INVALID)
				{
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_FACIAL,0));
				}
				pCD->m_AnimGraph2.GetAG2I()->TagAnimSetFromImpulses(&Context2,m_pServer->GetMapData(),m_pServer->m_spWData,lImpulses);
			}
		}
	}
};

#ifdef _DEBUG
#define MACRO_WRITEVERIFY {uint32 Apa = 0x81920467; _pFile->WriteLE(Apa);}
#define MACRO_READVERIFY {uint32 Apa; _pFile->ReadLE(Apa); M_ASSERT(Apa == 0x81920467, CStrF("Load/save mismatch in file '%s' on line %i", __FILE__, __LINE__)); };
#else
#define MACRO_WRITEVERIFY
#define MACRO_READVERIFY
#endif

enum {
	AI_OPTIONALSAVEFLAGS_BEHAVIOUR					= 0x1,
	AI_OPTIONALSAVEFLAGS_BEHAVIOURSCENEPOINT		= 0x2,
	AI_OPTIONALSAVEFLAGS_BEHAVIOUREXPIRATIONTICK	= 0x4,
	AI_OPTIONALSAVEFLAGS_BEHAVIOURSTOPISSUED		= 0x8,
};

void CAI_Core::OnDeltaLoad(CCFile* _pFile)
{
	static int Test = 0;
	ConOutL(CStrF("Loading AI %i", Test++));

	MACRO_READVERIFY;

	int32 Temp;
	fp32 TempFloat;
	int8 Temp8;
	uint16 Tempu16;
	_pFile->ReadLE(Temp); m_ActivationState = Temp;
	_pFile->ReadLE(Temp); m_ActivationTick = Temp;
	_pFile->ReadLE(Temp); m_ActivationTime = Temp;
	_pFile->ReadLE(Temp); m_ActivityScore = Temp;
	_pFile->ReadLE(Temp); m_ActivityLevel = Temp;
	_pFile->ReadLE(Temp); m_ActivityTick = Temp;
	_pFile->ReadLE(Temp); m_LifeTimer = Temp;
	_pFile->ReadLE(Temp); m_DestroyTimer = Temp;
	_pFile->ReadLE(Temp8); m_PauseActions = (Temp8 != 0);
	_pFile->ReadLE(Temp); m_StunTimeout = Temp;
	_pFile->ReadLE(Temp); m_iDialoguePartner = Temp;
	_pFile->ReadLE(Temp); m_iCrowdFocus = Temp;
	_pFile->ReadLE(Temp); m_iLookAtObject = Temp;
	_pFile->ReadLE(Temp8); m_bLookSoft = (Temp8 != 0);
	_pFile->ReadLE(Temp); m_iDragObject = Temp;
	_pFile->ReadLE(Tempu16); m_SeenDarknessPowers = Tempu16;

	m_HoldPosition.Read(_pFile);
	m_HoldLookDir.Read(_pFile);

	_pFile->ReadLE(Temp8); m_bObjColl = (Temp8 != 0);
	_pFile->ReadLE(Temp8); m_bObjCollRequested = (Temp8 != 0);

	//Attributes that can be set by messages
	_pFile->ReadLE(Temp); m_SecurityTolerance = Temp;
	_pFile->ReadLE(TempFloat); m_Awareness = TempFloat;
	_pFile->ReadLE(TempFloat); m_SightRange = TempFloat;
	_pFile->ReadLE(TempFloat); m_HearingRange = TempFloat;
	_pFile->ReadLE(TempFloat); m_FOV = TempFloat;
	_pFile->ReadLE(Temp); m_HitRatio = Temp;
	
	_pFile->ReadLE(Temp); m_Timer = Temp;
	_pFile->ReadLE(Temp); m_ServerTick = Temp; 
	_pFile->ReadLE(TempFloat); m_ServerTickFraction = TempFloat; 

	_pFile->ReadLE(TempFloat); m_ActivateRangeSqr = TempFloat;
	_pFile->ReadLE(TempFloat); m_IdleMaxSpeed = TempFloat;
	_pFile->ReadLE(Temp); m_ActionFromKey = Temp;

	_pFile->ReadLE(Temp); m_iPrevBehaviour = Temp;
	//Check if we have saved behaviour stuff
	int8 OptionalSaveFlags = 0;
	_pFile->ReadLE(OptionalSaveFlags);
	if (OptionalSaveFlags & AI_OPTIONALSAVEFLAGS_BEHAVIOUR)
	{
		//We have behaviour set, load stuff
		m_bBehaviourRunning = true;
		_pFile->ReadLE(Temp); m_iBehaviourRunning = Temp;

		//Should we load a behaviour scene point?
		m_pBehaviourScenePoint = NULL;
		if (OptionalSaveFlags & AI_OPTIONALSAVEFLAGS_BEHAVIOURSCENEPOINT)
		{
			int16 iSP = -1;
			_pFile->ReadLE(iSP);
			CWO_ScenePointManager * pSPM = GetScenePointManager();
			if (pSPM)
			{
				CWO_ScenePoint * pSP = pSPM->GetScenePointFromIndex(iSP);
				if (pSP)
				{
					//Found a scenepoint that was saved
					m_pBehaviourScenePoint = pSP;
				}
			}
		}
		//Should we load a behaviour expiration tick?
		m_BehaviourExpirationTick = -1;
		if (OptionalSaveFlags & AI_OPTIONALSAVEFLAGS_BEHAVIOUREXPIRATIONTICK)
		{
			_pFile->ReadLE(Temp); m_BehaviourExpirationTick = Temp + m_Timer;
		}

		// Should we save m_BehaviourStopIssued?
		if (OptionalSaveFlags & AI_OPTIONALSAVEFLAGS_BEHAVIOURSTOPISSUED)
		{
			_pFile->ReadLE(Temp); m_BehaviourStopIssued = Temp;
		}
	}
	else
	{
		//No behaviour
		ResetBehaviour();
	}

	MACRO_READVERIFY;

	//Set personality (can be changed by script)
	_pFile->ReadLE(Temp);
	if ((Temp >= PERSONALITY_ENUM_BASE) && (Temp <= CAI_Personality::PERSONALITY_MAX))
	{
		OnImpulse(Temp);
	}

	m_AH.OnDeltaLoad(_pFile);
	m_Script.OnDeltaLoad(_pFile);
	m_EventHandler.OnDeltaLoad(_pFile);

	MACRO_READVERIFY;

	for (int iDevice=0; iDevice<CAI_Device::NUM_DEVICE; iDevice++)
		m_lDevices[iDevice]->OnDeltaLoad(_pFile);

	MACRO_READVERIFY;

	m_KB.OnDeltaLoad(_pFile);

	MACRO_READVERIFY;
}

void CAI_Core::OnDeltaSave(CCFile* _pFile)
{
	MACRO_WRITEVERIFY;

	int32 Temp;
	fp32 TempFloat;
	int8 Temp8;
	uint16 Tempu16;
	Temp = m_ActivationState; _pFile->WriteLE(Temp);
	Temp = m_ActivationTick; _pFile->WriteLE(Temp);
	Temp = m_ActivationTime; _pFile->WriteLE(Temp);
	Temp = m_ActivityScore; _pFile->WriteLE(Temp);
	Temp = m_ActivityLevel; _pFile->WriteLE(Temp);
	Temp = m_ActivityTick; _pFile->WriteLE(Temp);
	Temp = m_LifeTimer; _pFile->WriteLE(Temp);
	Temp = m_DestroyTimer; _pFile->WriteLE(Temp);
	Temp8 = m_PauseActions; _pFile->WriteLE(Temp8);
	Temp = m_StunTimeout; _pFile->WriteLE(Temp);
	Temp = m_iDialoguePartner; _pFile->WriteLE(Temp);
	Temp = m_iCrowdFocus; _pFile->WriteLE(Temp);
	Temp = m_iLookAtObject; _pFile->WriteLE(Temp);
	Temp8 = m_bLookSoft; _pFile->WriteLE(Temp8);
	Temp = m_iDragObject; _pFile->WriteLE(Temp);
	Tempu16 = m_SeenDarknessPowers; _pFile->WriteLE(Tempu16);

	m_HoldPosition.Write(_pFile);
	m_HoldLookDir.Write(_pFile);

	Temp8 = m_bObjColl; _pFile->WriteLE(Temp8);
	Temp8 = m_bObjCollRequested; _pFile->WriteLE(Temp8);

	//Attributes that can be set by messages
	Temp = m_SecurityTolerance; _pFile->WriteLE(Temp);
	TempFloat = m_Awareness; _pFile->WriteLE(TempFloat);
	TempFloat = m_SightRange; _pFile->WriteLE(TempFloat);
	TempFloat = m_HearingRange; _pFile->WriteLE(TempFloat);
	TempFloat = m_FOV; _pFile->WriteLE(TempFloat);
	Temp = m_HitRatio; _pFile->WriteLE(Temp);

	Temp = m_Timer; _pFile->WriteLE(Temp);
	Temp = m_ServerTick; _pFile->WriteLE(Temp);
	TempFloat = m_ServerTickFraction; _pFile->WriteLE(TempFloat);

	TempFloat = m_ActivateRangeSqr; _pFile->WriteLE(TempFloat);
	TempFloat = m_IdleMaxSpeed; _pFile->WriteLE(TempFloat);
	Temp = m_ActionFromKey; _pFile->WriteLE(Temp);

	Temp = m_iPrevBehaviour; _pFile->WriteLE(Temp);
	//Check if we should save behaviour stuff
	int8 OptionalSaveFlags = 0;
	if (m_bBehaviourRunning)
	{
		OptionalSaveFlags |= AI_OPTIONALSAVEFLAGS_BEHAVIOUR;
		if (m_pBehaviourScenePoint)
			OptionalSaveFlags |= AI_OPTIONALSAVEFLAGS_BEHAVIOURSCENEPOINT;
		if (m_BehaviourExpirationTick != -1)
			OptionalSaveFlags |= AI_OPTIONALSAVEFLAGS_BEHAVIOUREXPIRATIONTICK;
		if (m_BehaviourStopIssued != BEHAVIOUR_STOP_NONE)
		{
			OptionalSaveFlags |= AI_OPTIONALSAVEFLAGS_BEHAVIOURSTOPISSUED;
		}
	}
	_pFile->WriteLE(OptionalSaveFlags);
	if (OptionalSaveFlags & AI_OPTIONALSAVEFLAGS_BEHAVIOUR)
	{
		//We have behaviour, save stuff
		Temp = m_iBehaviourRunning; _pFile->WriteLE(Temp);

		//Should we save a behaviour scene point?
		if (OptionalSaveFlags & AI_OPTIONALSAVEFLAGS_BEHAVIOURSCENEPOINT)
		{
			//Save any scenepoints we have held, so that we make sure 
			//they are released next refresh after load if unused
			CWO_ScenePointManager * pSPM = GetScenePointManager();
			int16 iSP = -1;
			if (pSPM)
			{
				iSP = pSPM->GetScenePointIndex(m_pBehaviourScenePoint);
			}
			_pFile->WriteLE(iSP);
		}
		//Should we save a behaviour expiration tick?
		if (OptionalSaveFlags & AI_OPTIONALSAVEFLAGS_BEHAVIOUREXPIRATIONTICK)
		{
			Temp = m_BehaviourExpirationTick-m_Timer; _pFile->WriteLE(Temp);
		}
		// Should we save m_BehaviourStopIssued?
		if (OptionalSaveFlags & AI_OPTIONALSAVEFLAGS_BEHAVIOURSTOPISSUED)
		{
			Temp = m_BehaviourStopIssued; _pFile->WriteLE(Temp);
		}
	}

	MACRO_WRITEVERIFY;

	//Save personality (could have been changed by script)
	Temp = m_Personality.GetPersonality(); _pFile->WriteLE(Temp);

	m_AH.OnDeltaSave(_pFile);
	m_Script.OnDeltaSave(_pFile);
	m_EventHandler.OnDeltaSave(_pFile);

	MACRO_WRITEVERIFY;

	for (int iDevice=0; iDevice<CAI_Device::NUM_DEVICE; iDevice++)
		m_lDevices[iDevice]->OnDeltaSave(_pFile);

	MACRO_WRITEVERIFY;

	m_KB.OnDeltaSave(_pFile);

	MACRO_WRITEVERIFY;
}

void CAI_Core::OnPostWorldLoad()
{
	m_KB.OnPostWorldLoad();
};

//Custom OnIncludeSound method, to take special sound syntax in consideration
void CAI_Core::IncludeSoundFromKey(CStr _Key, CRegistry * _pReg, CMapData * _pMapData)
{
	if(_pReg)
	{
		CRegistry *pChild = _pReg->FindChild(_Key);
		CStr Temp;
		if(pChild)
		{
			Temp = pChild->GetThisValue();
			_pMapData->GetResourceIndex_Sound(Temp.GetStrSep(":"));
		}
	}
}

//Should run when including a template
void CAI_Core::OnIncludeTemplate(CRegistry * _pReg, CMapData * _pMapData)
{
	//Include AI sounds; since these are of the format <sound>:<duration> they cannot be included via the usual
	//CWObject::IncludeSoundFromKey method 
	/*	IncludeSoundFromKey("AI_SOUND_IDLE0", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_IDLE1", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_IDLE2", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_DETECT0", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_DETECT1", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_DETECT2", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_SPOT0", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_SPOT1", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_SPOT2", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_CHARGE0", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_CHARGE1", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_CHARGE2", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_ANGRY0", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_ANGRY1", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_ANGRY2", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_AFRAID0", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_AFRAID1", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_AFRAID2", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_SURPRISED0", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_SURPRISED1", _pReg, _pMapData);
	IncludeSoundFromKey("AI_SOUND_SURPRISED2", _pReg, _pMapData);*/
};


//Construct a new AI core object, of the type denoted by the given string, from the given one
spCAI_Core CAI_Core::GetAIFromString(CStr _Str, CAI_Core * _pAI, CWorld_Server * pServer,  CWObject * _pChar)
{
	MSCOPE(CAI_Core::GetAIFromString(), CHARACTER);

	if (_Str.CompareNoCase("TURRET") == 0)
	{
		//Change to turret AI
		spCAI_Core spAI = MNew1(CAI_Core_Turret, _pAI);
		spAI->Init(_pChar, pServer);
		return spAI;
	}
	else if (_Str.CompareNoCase("DARKLING") == 0)
	{
		if (!MRTC_ISKINDOF(_pAI,CAI_Core_Darkling))
		{
			//Change to darkling AI
			spCAI_Core spAI = MNew1(CAI_Core_Darkling, _pAI);
			spAI->Init(_pChar, pServer);
			return spAI;
		}
		else
		{
			return NULL;
		}
	}
	else
		//No custom AI available with that string identifier, fail
		return NULL;
};


//Get pointer to the agent's current item of given type or NULL if agent doesn't have an item of that type
CRPG_Object_Item * CAI_Core::GetRPGItem(int _iObj, int _iType)
{
	CWObject *pObj = m_pServer->Object_Get(_iObj);
	if(!pObj)
		return NULL;

	CWObject_Interface_AI *pAI = pObj->GetInterface_AI();
	if(!pAI)
		return NULL;
	return pAI->AI_GetRPGItem(_iType);
};


//Get pointer to the agent's inventory of given type (RPG_CHAR_INVENTORY_WEAPONS etc) or NULL if agent doesn't have an inventory of that type
CRPG_Object_Inventory * CAI_Core::GetRPGInventory(int _iObj, int _iType)
{
	CWObject *pObj = m_pServer->Object_Get(_iObj);
	if(!pObj)
		return NULL;

	CWObject_Interface_AI *pAI = pObj->GetInterface_AI();
	if(!pAI)
		return NULL;
	return pAI->AI_GetRPGInventory(_iType);
};


//Gets the AI control frame, containing the commands that the bot should currently perform
const CControlFrame* CAI_Core::GetControlFrame()
{
	//Return the ai control frame
	return m_Controlhandler.GetControlFrame();
};

bool CAI_Core::HighTension(int _CalmTime)
{
	if ((m_pAIResources->ms_Tension != -1)&&(m_pAIResources->ms_Tension + _CalmTime>= GetAITick()))
	{
		return(true);
	}
	else
	{
		return(false);
	}
};

//The more "action" the player thinks there currently is, the higher value is returned. 
// 1.0f: Battle, one or more non civs are enemies with the player and has DETECTED or better awareness
// 0.5f: Tension(Search), one or more characters are enemies with the player and have NOTICED or better awareness
// 0.25f: Tension(Irked), one or more characters are hostile+ with the player and have DETECTED or better awareness
// 0.0f: Peace, none of the above conditions hold
// Jesse, I added the two flavours of Tension for my debugging and possibe future expansion
fp32 CAI_Core::GetTension()
{
	if (!IsValid())
	{
		m_pAIResources->ms_Tension = -1;
		return 0;
	}

	fp32 rValue = 0.0f;
	CSimpleIntList* plAgents = &(m_pAIResources->m_KBGlobals.ms_lAgents);
	if (plAgents)
	{
		CAI_Core * pOtherAI;
		CAI_AgentInfo * pSelfInfo;
		for (int i = 0; i < plAgents->Length(); i++)
		{
			if (!plAgents->IsValid(i)) {continue;}
			pOtherAI = GetAI(plAgents->Get(i));
			if ((pOtherAI)&&(!pOtherAI->m_bFirstRefresh))
			{
				pSelfInfo = pOtherAI->m_KB.GetAgentInfo(m_pGameObject->m_iObject);
				if (pSelfInfo)
				{	// Civs are no threat so they can never trigger battlemusic
					int Relation = pSelfInfo->GetCurRelation();
					int Awareness = pSelfInfo->GetCurAwareness();
					if (pOtherAI->m_CharacterClass == CLASS_CIV)
					{	// Civs don't raise tension much
						if (Relation >= CAI_AgentInfo::ENEMY)
						{
							if (Awareness >= CAI_AgentInfo::SPOTTED)
							{
								rValue = Max(rValue,GET_TENSION_TENSION_HOSTILE_2);
							}
							else if (Awareness >= CAI_AgentInfo::DETECTED)
							{
								rValue = Max(rValue,GET_TENSION_TENSION_HOSTILE_1);
							}
						}
						else if (Relation >= CAI_AgentInfo::HOSTILE)
						{
							if (Awareness >= CAI_AgentInfo::SPOTTED)
							{
								rValue = Max(rValue,GET_TENSION_TENSION_HOSTILE_1);
							}
						}
					}
					else
					{	// Badguys, cops, soldiers, meatfaces
						if (Relation >= CAI_AgentInfo::ENEMY)
						{
							if ((Awareness >= CAI_AgentInfo::SPOTTED)||
								((Awareness >= CAI_AgentInfo::DETECTED)&&(pSelfInfo->GetGetLastSpottedTicks() <= 200)))
							{
								m_pAIResources->ms_Tension = GetAITick();
								rValue = Max(rValue,GET_TENSION_BATTLE);
								return(rValue);
							}
							else if (Awareness >= CAI_AgentInfo::NOTICED)
							{
								rValue = Max(rValue,GET_TENSION_TENSION_HOSTILE_2);
							}
						}
						else if (Relation >= CAI_AgentInfo::HOSTILE)
						{
							rValue = Max(rValue,GET_TENSION_TENSION_HOSTILE_2);
						}

					}
				}	//	'if (pSelfInfo)'
			}	// 'if ((pOtherAI)&&(!pOtherAI->m_bFirstRefresh))'
		}	// 'for (int i = 0; i < plAgents->Length(); i++)'
	}

	//No agents have spotted us and considers us enemies.
	m_pAIResources->ms_Tension = -1;
	return rValue;
};


//Tension for stealth stuff; returns any of the following values:
// TENSION_NONE:		No alarming activity whatsoever
// TENSION_SCANNING:	We may have heard something, scan and investigate
// TENSION_SEARCHING:	We no longer see the mother***
// TENSION_PURSUING:	Take him down man!
// Only when the _UpdateFlag is true will there be an actual recalc of the value
int CAI_Core::GetStealthTension(bool _UpdateFlag)
{
	MSCOPESHORT(CAI_Core::GetStealthTension2);

	if ((IsPlayer())||(!IsValid()))
	{
		return(TENSION_NONE);
	}

	if (_UpdateFlag == false)
	{
		return(m_StealthTension);
	}

	// This is a way simpler system for determining stealth tension:
	// We check if the player has violated security, if so he becomes an enemy
	// (This is simply done by calling GetRelation on the player
	// Then we simply measure the worst awareness we have of an enemy or hostile
	// NOTICED -> TENSION_SEARCHING
	// DETECTED -> TENSION_WARY
	// SPOTTED -> TENSION_HOSTILE
	// As SPOTTED goes down to DETECTED pretty fast we must keep TENSION_PURSUING for m_Patience ticks
	//
	// We cap stealth tension by alertness:
	// DROWSY -> max TENSION_SEARCHING
	// IDLE -> max TENSION_WARY
	// Watchful+ -> max TENSION_HOSTILE
	
	int Tension = TENSION_NONE;
	int Alertness = m_KB.GetAlertness();

	// Sleeping or KO don't get much action
	if (Alertness <= CAI_KnowledgeBase::ALERTNESS_SLEEPING)
	{
		return(Tension);
	}

	int N = m_KB.NumAgentInfo();
	// Step through each agent, and if he is an enemy we check him further
	for (int agentIter = 0; agentIter < N; agentIter++)
	{
		CAI_AgentInfo* pAgent = m_KB.IterateAgentInfo(agentIter);
		if (!pAgent) continue;
		int Relation = pAgent->GetRelation();
		if (Relation >= CAI_AgentInfo::HOSTILE)
		{
			int CurTension = TENSION_NONE;
			int CurAwareness = pAgent->GetCurAwareness();
			switch(CurAwareness)
			{
			case CAI_AgentInfo::NONE:
				CurTension = TENSION_NONE;
				break;

			case CAI_AgentInfo::NOTICED:
				CurTension = TENSION_HOSTILE;
				break;

			case CAI_AgentInfo::DETECTED:
			case CAI_AgentInfo::SPOTTED:
				CurTension = TENSION_COMBAT;
				break;
			// No default; we set CurTension = TENSION_NONE; prior to switch call
			}

			// Hostiles cannot get more tension than TENSION_SEARCHING
			if ((CurTension > TENSION_HOSTILE)&&(Relation < CAI_AgentInfo::ENEMY))
			{
				CurTension = TENSION_HOSTILE;
			}
			// Detected+ enemies cannot get less tension than TENSION_PURSUING
			if ((CurAwareness >= CAI_AgentInfo::DETECTED)&&(CurTension < TENSION_COMBAT)&&(Relation >= CAI_AgentInfo::ENEMY))
			{
				CurTension = TENSION_COMBAT;
			}

			if (CurTension > Tension)
			{
				Tension = CurTension;
			}
		}
	}
	SetStealthTension(Tension);

	return(m_StealthTension);
}

// TODO: Store darkness type in globals, react appropriately with voices
void CAI_Core::SetPlayerDarkness(uint16 _DarknessType)
{
	if ((IsPlayer())&&(m_pAIResources))
	{
		if (_DarknessType != 0)
		{	// Should we add some ugly bugly visibility to make AI:s see us?
			m_KB.AddLight(50);
			m_pAIResources->m_KBGlobals.ms_DarknessPowers = _DarknessType;
			m_pAIResources->m_KBGlobals.ms_DarknessCount = 2 * AI_KB_REFRESH_PERIOD;
		}
	}
}

void CAI_Core::SetDarknessPos(CVec3Dfp32 _Pos)
{
	if ((IsPlayer())&&(m_pAIResources))
	{
		m_pAIResources->m_KBGlobals.ms_DarknessPos = _Pos;
		m_pAIResources->m_KBGlobals.ms_DarknessCount = 2 * AI_KB_REFRESH_PERIOD;
		m_pAIResources->m_KBGlobals.ms_DarknessPosCount = 2 * AI_KB_REFRESH_PERIOD;
	}
};

// Called on the player every tick to delay the clearing of Darkness globals
void CAI_Core::OnRefreshDarkness()
{
	if ((IsPlayer())&&(m_pAIResources))
	{
		if (m_pAIResources->m_KBGlobals.ms_DarknessCount > 0)
		{
			m_pAIResources->m_KBGlobals.ms_DarknessCount--;
		}
		if (m_pAIResources->m_KBGlobals.ms_DarknessCount > 0)
		{
			m_pAIResources->m_KBGlobals.ms_iDarknessPlayer = GetObjectID();
			// Is creepdark (or tentacle) out?
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
			if (pCD)
			{
				m_pAIResources->m_KBGlobals.ms_iCreepingDark = pCD->m_iCreepingDark;
			}
		}
		else
		{
			m_pAIResources->m_KBGlobals.ms_iDarknessPlayer = 0;
			INVALIDATE_POS(m_pAIResources->m_KBGlobals.ms_DarknessPos);
			m_pAIResources->m_KBGlobals.ms_iCreepingDark = 0;
		}

		if (m_pAIResources->m_KBGlobals.ms_DarknessPosCount > 0)
		{
			m_pAIResources->m_KBGlobals.ms_DarknessPosCount--;
		}
		m_pAIResources->m_KBGlobals.ms_DarknessPosCount = Min(m_pAIResources->m_KBGlobals.ms_DarknessPosCount,m_pAIResources->m_KBGlobals.ms_DarknessCount);
		if (m_pAIResources->m_KBGlobals.ms_DarknessPosCount <= 0)
		{
			INVALIDATE_POS(m_pAIResources->m_KBGlobals.ms_DarknessPos);
		}
	}
};


void CAI_Core::SetMinStealthTension(int _Tension, bool _bRightNow)
{
	if (GetStealthTension() < _Tension)
	{
		SetStealthTension(_Tension,_bRightNow);
	}
};

void CAI_Core::SetStealthTension(int _Tension, bool _bRightNow)
{
	if ((_Tension < TENSION_NONE)||(_Tension > TENSION_COMBAT))
	{
		return;
	}

	bool bTensionChanged = false;
	int CurTick = GetAITick();
	if (_Tension < m_StealthTension)
	{
		if ((_bRightNow)||(CurTick > m_StealthTensionTime + m_Patience * 2.0f))
		{
			if (m_StealthTension != _Tension)
			{
				bTensionChanged = true;
			}
			m_StealthTension = _Tension;
			m_StealthTensionTime = CurTick;
		}
	}
	else
	{
		if (m_StealthTension != _Tension)
		{
			bTensionChanged = true;
		}
		m_StealthTension = _Tension;
		m_StealthTensionTime = CurTick;
	}

	if ((bTensionChanged)&&(!m_StunTimeout))
	{	
		// Set stance based (somewhat on tension)
		if (m_StealthTension >= TENSION_COMBAT)
		{
			m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		}
		else if ((m_StealthTension <= TENSION_NONE)&&(m_DeviceStance.GetIdleStance() <= CAI_Device_Stance::IDLESTANCE_HOSTILE))
		{
			m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_IDLE);
		}
	}
}

//Get itemid of recommended weapon to start with.
int CAI_Core::GetRecommendedWeapon()
{
	return m_Weapon.GetRecommendedWeapon();
}

//Calculate the activateposition matrix for the bot. This is the character position matrix,
//possibly modified by the wielded weapon.
void CAI_Core::GetActivatePosition(CMat4Dfp32* _pMat)
{
	if (!_pMat || !m_pGameObject)
		return;

	CVec3Dfp32 OrgDir;
	CVec3Dfp32 AimPos;
	CVec3Dfp32 AimDir;
	if (m_CharacterClass == CLASS_DARKLING)
	{
		*_pMat = m_pGameObject->GetPositionMatrix();
		OrgDir = _pMat->GetRow(0);
		AimPos = GetAimPosition();
		m_pGameObject->AI_SetEyeLookDir(OrgDir,0);
#ifndef M_RTM
		if (DebugTarget())
		{
			Debug_RenderWire(AimPos,AimPos+OrgDir * 100, kColorWhite);
		}
#endif
	}
	else
	{
		//First set default activate position
		*_pMat = m_pGameObject->GetPositionMatrix();
		OrgDir = _pMat->GetRow(0);
		int32 WieldedWeaponType = m_Weapon.GetWieldedWeaponType();
		if ((!IsPlayer())&&
			((WieldedWeaponType == CAI_WeaponHandler::AI_WEAPONTYPE_ASSAULT)||(WieldedWeaponType == CAI_WeaponHandler::AI_WEAPONTYPE_SHOTGUN)))
		{	// Eating your precious CPU cycles, Magnus!
			AimPos = GetTrueAimPosition();
		}
		else
		{
			AimPos = GetAimPosition();
		}
		AimPos.SetMatrixRow(*_pMat, 3);
		// Handle some targeting for CombatBehaviour
		if ((!m_KB.GetPlayerDarkness())&&(m_CurCB.m_iBehaviour)&&(m_CurCBTarget)&&(m_pBehaviourScenePoint))
		{
			CAI_AgentInfo* pTarget = m_KB.GetAgentInfo(m_CurCBTarget);
			if (pTarget)
			{
				CVec3Dfp32 TargetPos;
				if (Random > 0.75f)
				{	// Headshot
					TargetPos = pTarget->GetHeadPos();
				}
				else
				{	// Bodyshot
					TargetPos = GetTorsoPosition(pTarget);
				}
				AimDir = TargetPos - AimPos;
				if (AimDir.LengthSqr() > 0.001f)
				{
					int iObj = pTarget->GetObjectID();
					if (pTarget->GetCurAwareness() < CAI_AgentInfo::SPOTTED)
					{
						iObj = -1;
					}
					AimDir.Normalize();
					// Handle look constraining
					int WireColor = kColorGreen;
					CMat4Dfp32 SPMat = m_pBehaviourScenePoint->GetPositionMatrix();
					if ((m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_LEFT)&&(AimDir * SPMat.GetRow(1) < -0.25f))
					{
						AimDir = SPMat.GetRow(0);
						WireColor = kColorRed;
						iObj = 0;
					}
					else if ((m_CurCB.m_TacFlags & CWO_ScenePoint::TACFLAGS_RIGHT)&&(AimDir * SPMat.GetRow(1) > 0.25f))
					{
						AimDir = SPMat.GetRow(0);
						WireColor = kColorRed;
						iObj = 0;
					}

					if (OrgDir * AimDir >= 0.87f)
					{	// Within 30 degrees of arc
						AimDir.SetMatrixRow(*_pMat, 0);
						_pMat->RecreateMatrix(0,1);
						WireColor = kColorGreen;
					}
					else
					{
						WireColor = kColorRed;
						iObj = 0;
					}
					m_pGameObject->AI_SetEyeLookDir(AimDir,iObj);
#ifndef M_RTM
					if (DebugTarget())
					{
						Debug_RenderWire(AimPos,AimPos+AimDir * 100, WireColor);
					}
#endif
				}
			}
		}
	}

	//Then let weapon override activateposition, and assume that we're attacking current target
	if (m_Weapon.GetWielded()->IsValid())
		m_Weapon.GetWielded()->GetActivatePosition(_pMat);

	//StopBM();
};


#ifndef M_RTM
//Get debug name of object
CStr CAI_Core::GetDebugName(CWObject * _pObj)
{
	if (!_pObj)
		return "WARNING! Non-existing object!";

	return CStrF("%s (%s):", _pObj->GetTemplateName(), _pObj->GetName());
};

CStr CAI_Core::GetDebugName(int _iObj)
{
	return GetDebugName(m_pServer->Object_Get(_iObj));
};


//Get some debug info about the bot
CStr CAI_Core::GetDebugString()
{
	if (!IsValid())
		return "WARNING! Invalid AI!";
	else
		return "***AI*** " + 
		m_AH.GetDebugString() +
		m_Script.GetDebugString() + 
		m_EventHandler.GetDebugString();
};
#endif


//DEBUG STUFF BELOW
#ifndef M_RTM

int CAI_Core::DebugMsg = 0;
CMTime CAI_Core::AITime;
CMTime CAI_Core::MaxTime;
bool CAI_Core::ms_bBenchMark = false;
bool CAI_Core::ms_bRemoveFunctionality = false;
bool CAI_Core::ms_bRenderNavGrid = false;
bool CAI_Core::ms_bMovementVerbose = false;
CMTime CAI_Core::ms_Pathfinds;
CMTime CAI_Core::ms_DirectMoves;
CMTime CAI_Core::ms_LengthFailedPathFinds;
CMTime CAI_Core::ms_PositionFailedPathFinds;
CMTime CAI_Core::ms_DelayedPathfinds;
CMTime CAI_Core::ms_SearchTimeouts;
CMTime CAI_Core::ms_MiscFailedPathFinds;
CMTime CAI_Core::ms_PFTime;
int CAI_Core::ms_nPFs = 0;
fp32 CAI_Core::ms_fMaxPFTime = 0;
fp32 CAI_Core::ms_fMinPFTime = _FP32_MAX;
TPtr<CWorld_Navgraph_Pathfinder> CAI_Core::ms_spGraphPF = NULL;
CVec3Dfp32 CAI_Core::ms_PathDest = CVec3Dfp32(_FP32_MAX);
bool CAI_Core::ms_bNewPath = false;
bool CAI_Core::ms_bPFReset = false; 
int CAI_Core::ms_nPFFrames = 0; 
fp32 CAI_Core::ms_fMaxPFTimePerFrame = 0;
fp32 CAI_Core::ms_fMinPFTimePerFrame = _FP32_MAX;
fp32 CAI_Core::ms_fFakeFrameTime = 33;

//Frame time in ms
fp32 CAI_Core::GetFrameTime()
{
	return ms_fFakeFrameTime;
};

#endif

//Debug renderwire if flag is set
void CAI_Core::Debug_RenderWire(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int32 _Color, fp32 _Duration)
{
	if (DebugRender() && m_pServer)
		m_pServer->Debug_RenderWire(_p0, _p1, _Color, _Duration);
};

//Debug rendervector if flag is set
void CAI_Core::Debug_RenderVector(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int32 _Color, fp32 _Duration)
{
	if (DebugRender() && m_pServer)
		m_pServer->Debug_RenderVector(_p0, _p1, _Color, _Duration);
};

//Debug render if flag is set
void CAI_Core::Debug_RenderVertex(const CVec3Dfp32& _p0, int32 _Color, fp32 _Duration)
{
	if (DebugRender() && m_pServer)
		m_pServer->Debug_RenderVertex(_p0, _Color, _Duration);
};

void CAI_Core::SetDebugMsg(int _Msg)
{
#ifndef M_RTM
	DebugMsg = _Msg;
#endif
};

#ifndef M_RTM
void CAI_Core::DebugRenderCell(int Val, int ValBelow, int ValAbove, CVec3Dfp32 Pos, CWorld_Server * pServer, fp32 Time)
{
	int32 Clr = 0xff000000;	// Black
	if  ((Val == 0)||(Val & XR_CELL_DIE))
	{
		if(Val == 0)
		Clr = kColorRed;	// Red == verboten
		else
			Clr = kColorYellow;	// Red == verboten
	}
	else
	{
		if (Val & (XR_CELL_AIR | XR_CELL_TRAVERSABLE))
		{
			if (ValBelow == 0)
			{
				Clr = kColorBlue;	// Blue
			}
			else if (Val & XR_CELL_WALL)
			{
				Clr = kColorGreen;	// Green
			}
			else
			{
				return;
			}
		}
		else
		{
			if (Val == XR_CELL_SLIP)
			{
				Clr = kColorDkPurple;	// DkBlue
			}
			else
			{	
				if (Val == (XR_CELL_SLIP | XR_CELL_WALL))
				{
					Clr = kColorDkGreen;
				}
				else
				{	// Weird combo of flags
					Clr = kColorWhite;
				}
			}
		}
	}

	pServer->Debug_RenderWire(Pos + CVec3Dfp32(0,0,4), Pos - CVec3Dfp32(0,0,4), Clr, Time); 
	pServer->Debug_RenderWire(Pos + CVec3Dfp32(0,4,0), Pos - CVec3Dfp32(0,4,0), Clr, Time); 
	pServer->Debug_RenderWire(Pos + CVec3Dfp32(4,0,0), Pos - CVec3Dfp32(4,0,0), Clr, Time); 
};


void CAI_Core::OnDebugStuff()
{
	//if (IsValid() && !IsBot(m_pGameObject->m_iObject) && s_nLOSChecks)
	//{
	//s_Tick++;
	//ConOut(CStrF("LosChecks: %f     .../tick: %f    .../Char: %f", (fp32)s_nLOSChecks, (fp32)s_nLOSChecks/s_Tick, (fp32)s_nLOSChecks/s_Tick/(m_lCharacters.Len() + 1)));
	//};

	if (!m_pGameObject || !m_pServer)
		return;

	TStop(cmFrameTime); //DEBUG

	//Reset statics stuff on first player refresh
	if (m_bFirstRefresh && !IsBot(m_pGameObject->m_iObject))
	{
		ms_bPFReset = false; 
		ms_nPFFrames = 0; 
		ms_PFTime.Reset();
		ms_fMaxPFTimePerFrame = 0;
		ms_fMinPFTimePerFrame = _FP32_MAX;
	}

	//Reset frame-count stuff flag on player refresh
	if (!IsBot(m_pGameObject->m_iObject))
	{
		ms_bPFReset = true;
	}

	/*	if (m_pServer && m_pGameObject && !IsBot(m_pGameObject->m_iObject))
	{
	//Rangemeter
	CVec3Dfp32 Fwd = CVec3Dfp32::GetRow(m_pGameObject->GetPositionMatrix(), 0);
	CVec3Dfp32 Pos = GetFocusPosition(m_pGameObject);
	CCollisionInfo Info;
	Info.m_CollisionType = CXR_COLLISIONTYPE_BOUND;
	bool Hit = m_pServer->Phys_IntersectLine(Pos, 
	Pos + Fwd * 4000,
	// ObjectFlags
	0, 
	// ObjectIntersectFlags, Intersect with physobjects, characters...
	OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER,
	//...and solids
	XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID,
	&Info,
	m_pGameObject->m_iObject);
	if(Hit && Info.m_bIsValid)
	{
	Debug_RenderWire(Pos, Info.m_Pos, 0xffff0000, 0.05f);
	AIOut(CStrF("Distance: %f", m_pGameObject->GetPosition().Distance(Info.m_Pos)));
	};
	}*/

	//Test area info stuff
	/*if (m_pServer && !IsBot(m_pGameObject->m_iObject))
	{
	CPixel32 Clr = 0xff00ff00;
	fp32 Time = 0.1f;
	bool bRes = false;
	CStr Res = "";
	m_pServer->Message_SendToTarget(CWObject_Message(CWObject_AreaInfo::OBJMSG_DEBUG_RENDER, (int)(&Clr), (int)(&Time)), "TestArea");
	m_pServer->Message_SendToTarget(CWObject_Message(CWObject_AreaInfo::OBJMSG_POSITION_IN_AREA, (int)(&bRes), 0, 0, 0, GetAimPosition()), "TestArea");
	Res += (bRes) ? "Position in area  " : "                  ";
	m_pServer->Message_SendToTarget(CWObject_Message(CWObject_AreaInfo::OBJMSG_OBJECT_IN_AREA, (int)(&bRes), m_pGameObject->m_iObject), "TestArea");
	Res += (bRes) ? "Object in area" : "";
	ConOut(Res);
	}*/



	if (Clear)
	{
		DebugMsg = 0;
		Clear = false;
	}
	else switch (DebugMsg)
	{
		case 1: 
			{
				//AIOut all characters in character list (make sure this is done once for each AI)
				ConOutL(CStrF("%s (%s) knows of:", m_pGameObject->GetTemplateName(), m_pGameObject->GetName()));
				CAI_AgentInfo * pInfo; 
				for (int i =0; i < m_KB.NumAgentInfo(); i++)
				{
					pInfo = m_KB.IterateAgentInfo(i);
					if (pInfo)
					{
						CWObject * pObj = pInfo->GetObject();
						if (pObj)
						{
							CStr Name = CStrF("%s (%s)", pObj->GetTemplateName(), pObj->GetName());
							CStr Str = "";

							int Awareness = pInfo->GetAwareness();
							if (Awareness >= CAI_AgentInfo::SPOTTED)
								Str += " SPOTTED";
							else if (Awareness == CAI_AgentInfo::DETECTED)
								Str += " DETECTED";
							else if (Awareness == CAI_AgentInfo::NOTICED)
								Str += " NOTICED";

							Str += CStr(" ") + CStr(CAI_AgentInfo::ms_TranslateRelation[pInfo->GetRelation()]);

							ConOutL("    " + Name + CStrF(" Index: %d/%d ", i, pInfo->GetObjectID()) + Str);
						}
					};
				}
				Clear = true;
			};
			break;
		case 2:
			{
				//Toggle AI printing for all bots
				m_bAIOut = !m_bAIOut;
				Clear = true;
			};
			break;
		case 3:
			{
				//Maximum and current total AI time per frame (once only)
				ConOutL("Current AI Time: " + CStrF("%.f ms", AITime.GetTime() * 1000.0f)); //DEBUG
				ConOutL("Maximum AI Time: " + CStrF("%.f ms", MaxTime.GetTime() * 1000.0f)); //DEBUG
				DebugMsg = 0;
			};
			break;
		case 4:
			{
				//Print current AI time for each bot
				ConOutL(CStrF("Refresh %s (%s): %.f micssec", m_pGameObject->GetTemplateName(), m_pGameObject->GetName(), Time.GetTime() * 1000000.0f)); //DEBUG
				Clear = true;
			};
			break;
		case 5:
			{
				//Print total frame time
				ConOutL("Frame Time" + CStrF(": %.f ms", cmFrameTime.GetTime() * 1000.0f)); //DEBUG
				DebugMsg = 0;
			};
			break;
		case 6:
			{	
				//Toggle FOV-rendering for all bots
				m_bRenderFOV = !m_bRenderFOV;
				Clear = true;
			};
			break;
		case 7:
			{
				//Toggle benchmark printing
				ms_bBenchMark = !ms_bBenchMark;
				DebugMsg = 0;
			};
			break;
		case 8:
			{
				if (IsPlayer())
				{
					int32 iDbgTarget = FindBotInFOV();
					CWObject* pObj = SetDebugTarget(iDbgTarget);
					if (pObj)
					{
						CAI_Core* pDebugTarget = GetAI(iDbgTarget);

						if (pDebugTarget)
						{
							CStr Stance;
							int32 iStance = pDebugTarget->m_DeviceStance.GetIdleStance();
							switch(iStance)
							{
							case CAI_Device_Stance::IDLESTANCE_IDLE:
								{
									Stance = " Idle stance";
								}
								break;
							case CAI_Device_Stance::IDLESTANCE_HOSTILE:
								{
									if (pDebugTarget->m_CharacterClass == CLASS_CIV)
									{
										Stance = " Wary stance";
									}
									else
									{
										Stance = " Hostile stance";
									}
								}
								break;
							case CAI_Device_Stance::IDLESTANCE_COMBAT:
								{
									if (pDebugTarget->m_CharacterClass == CLASS_CIV)
									{
										Stance = " Panic stance";
									}
									else
									{
										Stance = " Combat stance";
									}
								}
								break;
							default:
								{
									Stance = " Unknown stance";
								}
								break;
							}
							if (pDebugTarget->m_Script.IsValid())
							{
								Stance += pDebugTarget->m_Script.GetDebugString();
							}
							if (pDebugTarget->m_iCurBehaviour)
							{
								Stance += CStrF(", Behaviour %d",pDebugTarget->m_iCurBehaviour);
							}
							CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
							if (pCD)
							{
								if (pCD->m_AnimGraph2.GetDestinationLock())
								{
									Stance += CStrF("WARNING: PlayAnim not cleared");
								}
							}
							// ***
							CStr Teams = "Teams: ";
							uint16 liTeams[16];
							uint nTeams = pDebugTarget->m_pGameObject->AI_GetTeams(liTeams, sizeof_buffer(liTeams));
							for (int i = 0; i < nTeams; i++)
							{
								int iTeam = liTeams[i];
								CWObject* pTeam = m_pServer->Object_Get(iTeam);
								if (pTeam)
								{
									Teams += pTeam->GetName();
									Teams += ", ";
								}
							}
							// ***
							ConOut(CStrF("DebugTarget: %d ",pObj->m_iObject)+pObj->GetName()+Stance);
							CStr AHDebugSTring = pDebugTarget->m_AH.GetDebugString();
							ConOut(AHDebugSTring);
							ConOut(Teams);
						}
					}
					else
					{
						m_pServer->Registry_GetServer()->SetValue("dbgctrlobj",CStr("0"));
						m_pServer->Registry_GetServer()->SetValue("agdbgobj",CStr("0"));
						ConOut(CStr("DebugTarget: $player"));
					}
					DebugMsg = 0;
				}
			};
			break;
		case 9:
			{	//Toggle remove functionality flag
				ms_bRemoveFunctionality = !ms_bRemoveFunctionality;
				if (ms_bRemoveFunctionality)
					ConOut("Functionality off");
				else
					ConOut("Functionality on");
				DebugMsg = 0;
			};
			break;
		case 10:
			{
				//Toggling navgrid rendering
#ifndef M_RTM
				if (IsPlayer())
				{
					DebugDrawNavgrid();
					if ( (m_pAIResources)&&(m_pAIResources->ms_bDebugRender)&&(m_pAIResources->ms_iDebugRenderTarget))
					{
						DebugDrawNavgrid(m_pAIResources->ms_iDebugRenderTarget);
					}
					DebugMsg = 0;
				}
#endif
			};
			break;
		case 11:
			{
				//ConOutL path info stuff
				ConOutL("");
				ConOutL("PATHFINDING USAGE:");
				ConOutL(CStrF("Total Pathfinds: %lu", ms_Pathfinds.GetCycles())); //DEBUG
				ConOutL(CStrF("Slow reaction: %lu (%f %%)", ms_DelayedPathfinds.GetCycles(), (!ms_Pathfinds.IsReset()) ? (fp32)(100*ms_DelayedPathfinds.GetTime())/ms_Pathfinds.GetTime() : 0)); //DEBUG
				ConOutL(CStrF("Pathfind timeouts: %lu (%f %%)", ms_SearchTimeouts.GetCycles(), (!ms_Pathfinds.IsReset()) ? (fp32)(100*ms_SearchTimeouts.GetTime())/ms_Pathfinds.GetTime() : 0)); //DEBUG
				ConOutL(CStrF("Failed (length): %lu (%f %%)", ms_LengthFailedPathFinds.GetCycles(), (!ms_Pathfinds.IsReset()) ? (fp32)(100*ms_LengthFailedPathFinds.GetTime())/ms_Pathfinds.GetTime() : 0)); //DEBUG
				ConOutL(CStrF("Failed (position): %lu (%f %%)", ms_PositionFailedPathFinds.GetCycles(), (!ms_Pathfinds.IsReset()) ? (fp32)(100*ms_PositionFailedPathFinds.GetTime())/ms_Pathfinds.GetTime() : 0)); //DEBUG
				ConOutL(CStrF("Failed (other): %lu (%f %%)", ms_MiscFailedPathFinds.GetCycles(), (!ms_Pathfinds.IsReset()) ? (fp32)(100*ms_MiscFailedPathFinds.GetTime())/ms_Pathfinds.GetTime() : 0)); //DEBUG
				ConOutL(CStrF("Direct paths: %lu (%f %%)", ms_DirectMoves.GetCycles(), (!(ms_Pathfinds + ms_DirectMoves).IsReset()) ? (fp32)(100*ms_DirectMoves.GetTime())/(ms_Pathfinds + ms_DirectMoves).GetTime() : 0)); //DEBUG
				ConOutL("");
				ConOutL(CStrF("Average pathfinding time per frame: %.f ms", (ms_PFTime.GetTime() / (fp32)ms_nPFFrames) * 1000.0));
				ConOutL(CStrF("Maximum pathfinding time per framesearch: %.f ms", ms_fMaxPFTimePerFrame * 1000.0));
				ConOutL(CStrF("Minimum pathfinding time per framesearch: %.f ms", ms_fMinPFTimePerFrame * 1000.0));
				ConOutL("");
				DebugMsg = 0;
			};
			break;
		case 12:
			{
				m_pAIResources->ms_bDebugRender = true;
				DebugMsg = 0;
			}
			break;
		case 13:
			{
				// Won't worke here!
				if (DebugTargetSet())
				{
					if (DebugTarget())
					{
						int iPlayer = GetClosestPlayer();
						if (iPlayer)
						{
							CVec3Dfp32 Pos = GetLookAtPos(iPlayer);
							if (VALID_POS(Pos))
							{
								Pos = m_PathFinder.SafeGetPathPosition(Pos, 4, 1);
								m_Script.SetAction(CAI_ScriptHandler::MOVE_TO,0,Pos);
								Debug_RenderVertex(Pos,kColorCyan,5.0f);
							}
						}
						DebugMsg = 0;
					}
				}
				else
				{
					DebugMsg = 0;
				}
				/*
				//Build navgraph and save to file <worldname>.xng
				if (m_PathFinder.GridPF() && m_pServer && m_pGameObject && !IsBot(m_pGameObject->m_iObject))
				{
					m_PathFinder.GridPF()->BuildNavGraph(m_pGameObject->GetPosition(), 0);
					DebugMsg = 0;
				}
				*/
			}
			break;
		case 14:
			{
				//Debug render navgraph
				if (m_PathFinder.GridPF() && m_pServer && m_pGameObject && !IsBot(m_pGameObject->m_iObject))
				{
					if (m_pServer->Path_GetNavGraph() && m_pServer->Path_GetBlockNav())
					{
						m_pServer->Path_GetNavGraph()->DebugRenderGraph(m_pServer, m_pServer->Path_GetBlockNav()->GetGridPosition(m_pGameObject->GetPosition()),50,20);
					}
					else
					{
						ConOutL("No NavGrid and/or NavGraph!");
					}

					DebugMsg = 0;
				}
			}
			break;
		case 15:
			{
				if (m_pServer && m_PathFinder.IsValid())
				{
					m_Script.Clear();
					m_Script.SetAction(CAI_ScriptHandler::CONTINUOUS_MOVE, CVec3Dfp32(1,0,0));
					ResetPathSearch();
					if (!IsBot(m_pGameObject->m_iObject))
					{
						ms_bNewPath = true;
						ms_PathDest = m_PathFinder.SafeGetPathPosition(m_pGameObject->GetPosition(), 10, 10);
						Clear = true;
					}
				}
			}
			break;
		case 18:
			{
				//Stop all debug actions (and scripting)
				m_Script.Clear();
				ResetPathSearch();
				INVALIDATE_POS(m_PathDestination);
				ms_bRemoveFunctionality = false;
				INVALIDATE_POS(ms_PathDest);
				ms_bNewPath = false;
				Clear = true;
			}
			break;
		case 19:
			{
				//Send damage to any bot intersecting sight line of a non-bot
				if (IsPlayer())
				{
					int iTarget = FindBotInFOV();
					if ((iTarget)&&(IsValidAgent(iTarget)))
					{
						CWO_DamageMsg Damage;
						Damage.m_Damage = 1000;
						Damage.m_DamageType = DAMAGETYPE_UNDEFINED;
						m_pServer->Message_SendToObject(Damage.GetMessage(GetObjectID()), iTarget);	
					}
					DebugMsg = 0;
				}
				/*
 				if (!IsBot(m_pGameObject->m_iObject))
				{
		 			bool bHit = false;
					CCollisionInfo Info;
					CVec3Dfp32 AimAt = GetAimPosition() + CVec3Dfp32::GetRow(m_pGameObject->GetPositionMatrix(), 0) * 1000;
					bHit = IntersectLOS(GetAimPosition(), AimAt, m_pGameObject->m_iObject, &Info);
					if (bHit && Info.m_bIsValid && IsValidAgent(Info.m_iObject))
					{
						CWO_DamageMsg Damage(1000);
						m_pServer->Message_SendToObject(Damage.GetMessage(m_pGameObject->m_iObject), Info.m_iObject);	
					}
					Debug_RenderWire(GetAimPosition() + CVec3Dfp32(0,0,100), AimAt, 0xffff0000);
					DebugMsg = 0;
				}
				*/
			}
			break; 
		case 20:
			
			// Get a bunch of scenepoints
#ifdef M_Profile
			if (IsPlayer()) 
			{
				if (m_pAIResources->ms_iDebugRenderTarget)
				{
					CAI_Core* pAI = GetAI(m_pAIResources->ms_iDebugRenderTarget);
					if (pAI)
					{
						if (m_AH.m_spRestriction)
						{
							pAI->m_AH.DebugDrawScenepoints(true,512,m_AH.m_spRestriction->GetRoom());
						}
						else
						{
							pAI->m_AH.DebugDrawScenepoints(true,512,NULL);
						}
					}
				}
				else
				{
					m_AH.DebugDrawScenepoints(false,512,NULL);
				}
				DebugMsg = 0;
			}
#endif
			break;

		case 21:
			{
#ifdef M_Profile
				// Unused
#endif
			}
			break;

		case 22:
			{
				DebugMsg = 0;
			}
			break;

		case 23:
			//Debug render navgrid bounds
 			if (m_PathFinder.GetBlockNav() && m_pServer && m_pGameObject)
			{
				if (!IsBot(m_pGameObject->m_iObject))
				{
					CVec3Dint16	Dims = m_PathFinder.GetBlockNav()->GridDimensions() - CVec3Dint16(1,1,1);
					static const int s_lSides[12][2][3] = 
					{
						{{0,0,0},{0,0,1}},
						{{0,0,0},{0,1,0}},
						{{0,0,0},{1,0,0}},
						{{0,0,1},{0,1,1}},
						{{0,0,1},{1,0,1}},
						{{0,1,0},{0,1,1}},
						{{0,1,0},{1,1,0}},
						{{0,1,1},{1,1,1}},
						{{1,0,0},{1,0,1}},
						{{1,0,0},{1,1,0}},
						{{1,0,1},{1,1,1}},
						{{1,1,0},{1,1,1}},
					};
					for (int i = 0; i < 12; i++)
					{
						CVec3Dint16 From(s_lSides[i][0][0] * Dims[0], s_lSides[i][0][1] * Dims[1], s_lSides[i][0][2] * Dims[2]); 
						CVec3Dint16 To(s_lSides[i][1][0] * Dims[0], s_lSides[i][1][1] * Dims[1], s_lSides[i][1][2] * Dims[2]); 
						Debug_RenderWire(m_PathFinder.GetBlockNav()->GetWorldPosition(From), m_PathFinder.GetBlockNav()->GetWorldPosition(To), 0xff00ff00, 20);
					}
					DebugMsg = 0;
				}
			}
			else if (!m_PathFinder.GridPF())
			{
				ConOutL("No navgrid!");
			}
			Clear = true;
			break;
		case 24:
			{
				//Debug print all characters' current animgraph state
				// If we're in a dialog we cannot deactivate
				// ***AG1***
				/*
 				CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
 				if (pChar)
				{
					CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pChar);
					if (pCD)
					{
						CWAGI_Context AGContext(m_pGameObject, m_pServer, pCD->m_GameTime);
						CWAGI* pAGI = pCD->m_AnimGraph.GetAGI();
						CStr Str = CStr("AGSTATEINFO: ") + GetDebugName(m_pGameObject) + CStr(" ");
						Str += pAGI->DebugStateInfo(&AGContext);
					
						//String is formatted with | as newline indicator, since \n doesn't seem to work :/
						while (Str != "")
							ConOutL(Str.GetStrSep("|"));
					}
				}
				*/
			}
			Clear = true;
			break;
		case 25:
			{
				//Toggle movement verbose mode
				ms_bMovementVerbose = !ms_bMovementVerbose;
			}
			DebugMsg = 0;
			break;
#ifdef AG2_RECORDPROPERTYCHANGES
		case 26:
			{
				if (IsPlayer())
				{
					int32 iDbgTarget = FindBotInFOV();
					CWObject* pObj = SetDebugTarget(iDbgTarget);
					if (pObj)
					{
						CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pObj);
						if (pCD)
						{
							CWAG2I_Context Context(pObj,m_pServer,CMTime::CreateFromTicks(pCD->m_GameTick,m_pServer->GetGameTickTime()));
							if (pCD->m_AnimGraph2.m_PropertyRecorder.m_State == CXRAG2_PropertyRecorder::AG2_PROPERTYRECORDER_RECORDING)
							{
								pCD->m_AnimGraph2.m_PropertyRecorder.StopOperation();
								CCFile File;
								CStr FileName = CStrF("propertyrecorder_%s.agr",pObj->GetName());
								File.Open(FileName,CFILE_WRITE);
								pCD->m_AnimGraph2.m_PropertyRecorder.Write(File);
								ConOutL(CStrF("Stopped recording, saving to: %s", FileName.Str()));
							}
							else
							{
								pCD->m_AnimGraph2.m_PropertyRecorder.StartRecording(&Context,pCD->m_AnimGraph2.GetAG2I(),pCD->m_TurnCorrectionTargetAngle);
								ConOutL(CStrF("Started recording ag properties: %s", pObj->GetName()));
							}
						}
						CAI_Core* pDebugTarget = GetAI(iDbgTarget);
						if (pDebugTarget)
						{
							CStr Stance;
							int32 iStance = pDebugTarget->m_DeviceStance.GetIdleStance();
							switch(iStance)
							{
							case CAI_Device_Stance::IDLESTANCE_IDLE:
								{
									Stance = " Idle stance";
								}
								break;
							case CAI_Device_Stance::IDLESTANCE_HOSTILE:
								{
									if (pDebugTarget->m_CharacterClass == CLASS_CIV)
									{
										Stance = " Wary stance";
									}
									else
									{
										Stance = " Hostile stance";
									}
								}
								break;
							case CAI_Device_Stance::IDLESTANCE_COMBAT:
								{
									if (pDebugTarget->m_CharacterClass == CLASS_CIV)
									{
										Stance = " Panic stance";
									}
									else
									{
										Stance = " Combat stance";
									}
								}
								break;
							default:
								{
									Stance = " Unknown stance";
								}
								break;
							}
							ConOut(CStrF("DebugTarget: %d ",pObj->m_iObject)+pObj->GetName()+Stance);
							CStr AHDebugSTring = pDebugTarget->m_AH.GetDebugString();
							ConOut(AHDebugSTring);
						}
					}
					else
					{
						m_pServer->Registry_GetServer()->SetValue("dbgctrlobj",CStr("0"));
						m_pServer->Registry_GetServer()->SetValue("agdbgobj",CStr("0"));
						ConOut(CStr("DebugTarget: $player"));
					}
					DebugMsg = 0;
				}
				break;
			}
#endif
		case 27:
			{
				// Toggle script message debug printing
				m_pServer->m_bConsoleLogMessages = !m_pServer->m_bConsoleLogMessages;
				ConOutL(CStr("Script message debug printing is ") + (m_pServer->m_bConsoleLogMessages ? "ON" : "OFF"));
				DebugMsg = 0;
			}
			break;
	}

	if (m_bRenderFOV && IsBot(m_pGameObject->m_iObject) && IsValid(true))
	{
		// Don't do this on the player
		//DEBUG Render approximate FOV (should be curve at end of cone)
		fp32 Look = GetLookAngles()[2];
		Debug_RenderWire(m_pGameObject->GetPosition() + CVec3Dfp32(0,0,48), m_pGameObject->GetPosition() + CVec3Dfp32(m_SightRange * M_Cos(Look * _PI2),m_SightRange * M_Sin(-Look * 2 * _PI),48), 0xffff0000, 0.01f);//DEBUG
		Debug_RenderWire(m_pGameObject->GetPosition() + CVec3Dfp32(0,0,48), m_pGameObject->GetPosition() + CVec3Dfp32(m_SightRange * M_Cos((Look + m_FOV/2) * _PI2),m_SightRange * M_Sin(-(Look + m_FOV/2) * _PI2),48), 0xff00ff00, 0.0f);//DEBUG
		Debug_RenderWire(m_pGameObject->GetPosition() + CVec3Dfp32(0,0,48), m_pGameObject->GetPosition() + CVec3Dfp32(m_SightRange * M_Cos((Look - m_FOV/2) * _PI2),m_SightRange * M_Sin(-(Look - m_FOV/2) * _PI2),48), 0xff00ff00, 0.0f);//DEBUG
		for (int i = 0; i < 8; i++)
		{
			Debug_RenderWire(m_pGameObject->GetPosition() + CVec3Dfp32(m_SightRange * M_Cos((Look + (i + 1) * m_FOV/16) * _PI2),m_SightRange * M_Sin(-(Look + (i + 1) * m_FOV/16) * _PI2),48), m_pGameObject->GetPosition() + CVec3Dfp32(m_SightRange * M_Cos((Look + i * m_FOV/16) * _PI2),m_SightRange * M_Sin(-(Look + i * m_FOV/16) * _PI2),48), 0xff00ff00, 0.05f);//DEBUG
			Debug_RenderWire(m_pGameObject->GetPosition() + CVec3Dfp32(m_SightRange * M_Cos((Look - (i + 1) * m_FOV/16) * _PI2),m_SightRange * M_Sin(-(Look - (i + 1) * m_FOV/16) * _PI2),48), m_pGameObject->GetPosition() + CVec3Dfp32(m_SightRange * M_Cos((Look - i * m_FOV/16) * _PI2),m_SightRange * M_Sin(-(Look - i * m_FOV/16) * _PI2),48), 0xff00ff00, 0.05f);//DEBUG
		};

		// Render approx. hearingrange
		CVec3Dfp32 Pos = m_pGameObject->GetPosition();
		int nCircles = 4;
		int Circle;
		for (Circle=1; Circle<=nCircles; Circle++)
		{
			fp32 Segment;
			fp32 Radius = (m_HearingRange * Circle) / nCircles;
			fp32 nSegments = Radius / 10.0f;
			fp32 SegmentDelta = 1.0f / float(nSegments);

			Segment = 0;
			fp32 v0 = (SegmentDelta*Segment)*_PI2;
			fp32 v1 = ((SegmentDelta*Segment)+SegmentDelta)*_PI2;
			CVec3Dfp32 p0 = Pos + CVec3Dfp32(Radius*M_Cos(v0), Radius*M_Sin(v0), 48.0f);
			CVec3Dfp32 p1 = Pos + CVec3Dfp32(Radius*M_Cos(v1), Radius*M_Sin(v1), 48.0f);
			Debug_RenderWire(p0, p1, 0xc0204080, 0.1f);

			for (Segment=0; Segment<nSegments; Segment++)
			{
				p0 = p1;
				v1 = ((SegmentDelta*Segment)+SegmentDelta)*_PI2;
				p1 = Pos + CVec3Dfp32(Radius*M_Cos(v1), Radius*M_Sin(v1), 48.0f);
				Debug_RenderWire(p0, p1, 0xc0204080, 0.1f);
			};
		};
	};

	if (ms_bBenchMark)
	{
		//Print benchmark stuff
		//StartBM("BM Overhead");
		//StartBM("BM Overhead");
		//StartBM("BM Overhead");
		//StopBM();
		//StopBM();
		//StopBM();
//		fp32 NetTime = (Time - m_BMStack.m_TotalOverhead).GetTime();
		ConOutL(CStrF("\r\n%s (%s), total: %.f micssec \r\n", m_pGameObject->GetTemplateName(), m_pGameObject->GetName(), Time.GetTime() * 1000000.0f) +
			m_BenchmarkInfo);
		m_BenchmarkInfo = "";

		//We should have cleared benchmarking stack by now
		CMTime T; CStr S;	
		if (m_BMStack.Pop(T, S, T))
			ConOutL("*****************Benchmarking error; non-matching start and stop***************");
		m_BMStack.m_TotalOverhead.Reset();
	}
	else if ((DebugMsg == 100) && m_pServer && IsBot(m_pGameObject->m_iObject))
	{
		ConOutL(CStrF("\r\n%s (%s), total: %.f micssec \r\n", m_pGameObject->GetTemplateName(), m_pGameObject->GetName(), Time.GetTime() * 1000000.0f) + 
			m_PathFinder.m_DebugInfo + m_Path.m_DebugInfo);
		m_PathFinder.m_DebugInfo = "";
		m_Path.m_DebugInfo = "";
		Clear = true;
	}

	TStart(cmFrameTime);
};

//Debugstuff to be done at end of refresh
void CAI_Core::OnDebugStuff2()
{
	if (ms_bNewPath && IsBot(m_pGameObject->m_iObject))
	{
 		//Move bots to current path destination, but release them when there
		if (ms_PathDest.DistanceSqr(m_pGameObject->GetPosition()) < 100*100)
		{
			m_Script.Clear();
			ResetPathSearch();
			INVALIDATE_POS(m_PathDestination);
		}
		else
		{
			m_Script.Clear();
			m_Script.SetAction(CAI_ScriptHandler::CONTINUOUS_MOVE, CVec3Dfp32(1,0,0));

			m_DeviceMove.Free();
			m_bNoEscapeSeqMoves = false;
			OnMove(ms_PathDest,4.0f,true,false,NULL);
		}
	}
};

void CAI_Core::StartBM(CStr _Text)
{
	if (ms_bBenchMark)
	{
		CMTime OverHead;
		TStart(OverHead);
		CMTime T;
		TStart(T);
		m_BMStack.Push(T, _Text);				
		TStop(OverHead);
		m_BMStack.AddOverHead(OverHead);
	}
};

void CAI_Core::StopBM(CStr _Text)
{
	if (ms_bBenchMark)
	{
		CMTime OverHead;
		TStart(OverHead);
		CMTime T;
		CStr Text;
		CMTime PrevOH;
		int nDepth;
		if ((nDepth = m_BMStack.Pop(T, Text, PrevOH)))
		{
			if (_Text != "")
				Text = _Text;
			CStr Indent = "";
			for (int i = 1; i < nDepth; i++)
				Indent += " ";
			TStop(T);
			CMTime ExtraOH;
			TStop(OverHead);
			TStart(ExtraOH);
			m_BenchmarkInfo += Indent + Text + CStrF(": %.f micssec \r\n", (T - OverHead - PrevOH).GetTime() * 1000000.0f);
			TStop(ExtraOH);

			//Add accumulated overhead of this benchmarker to immediate stack parent
			m_BMStack.AddOverHead(OverHead + PrevOH + ExtraOH);
		}
		else
		{
			ConOutL("*****************Benchmarking error; non-matching start and stop***************");
		}
	}
};

void CAI_Core::SetPath(CAI_FollowPathData* _pFD, int _iPath, int _iFollowMode)
{
	MAUTOSTRIP(CAI_Core_SetPath, MAUTOSTRIP_VOID);
	if (!IsValid())
		return;

	//Don't do anything if we're already following the path under the exact same circumstances
	if ((_pFD->m_iPath == _iPath) &&
		(_pFD->m_iFollowMode == _iFollowMode))
		return;

	_pFD->m_iPath = _iPath;

	_pFD->m_iFollowMode = _iFollowMode;
	_pFD->m_bFlying = false;
	INVALIDATE_POS(_pFD->m_MovePosRel);

	//Turn on gravity and animcontrol if necessary
	if (_pFD->m_bTurnOnGravity  && !m_bWeightless)
	{
		m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETGRAVITY, 1), m_pGameObject->m_iObject);
		_pFD->m_bTurnOnGravity = false;
	};
	if (_pFD->m_bTurnOnAnimControl)
	{
		m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ANIMCONTROL, 1), m_pGameObject->m_iObject);
		_pFD->m_bTurnOnAnimControl = false;
	};

	//Check if we're at path or not, and notify path accordingly if we're not force following, relative following or teleport following
	if (!(_pFD->m_iFollowMode & (CAI_FollowPathData::FOLLOW_FORCE | CAI_FollowPathData::FOLLOW_RELATIVE | CAI_FollowPathData::FOLLOW_TELEPORT)))
	{
		//This is somewhat deprecated... FIX
		
		//Get path object
		CWObject * pPath;
		if (_pFD->m_iPath && 
			 (pPath = m_pServer->Object_Get(_pFD->m_iPath)) &&
			 (CAI_Core::XYDistanceSqr(m_pGameObject->GetPosition(), pPath->GetPosition()) < 16*16) )
		{
			//Jupp, we're at path
			_pFD->m_bFallenBehind = false;
	 		m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_pGameObject->m_iObject), _pFD->m_iPath);
		} 
		else 
		{
			//We're not at path.
			_pFD->m_bFallenBehind = true;
	 		m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, m_pGameObject->m_iObject), _pFD->m_iPath);
		};
	};
	
	//No matter if we're at path or not, don't set correct path position now.
	INVALIDATE_POS(_pFD->m_PathPosition);
	INVALIDATE_POS(_pFD->m_LookVector);
	_pFD->m_LastPropelTick = GetAITick();				
	_pFD->m_bStop = false;

	// *** These lines should follow CAI_ScriptHandler call to SetPath
	// *** If we already have a normal move action, interrupt it.
	// *** m_liAction[MOVE] = 0;
	// *** m_MoveParam = CVec3Dfp32(_FP32_MAX);

}

void CAI_Core::OnGroundFollowPath(CAI_FollowPathData* _pFD)
{
	MAUTOSTRIP(CAI_Core_OnGroundFollowPath, MAUTOSTRIP_VOID);
	//Check that at least move device is available
	if (!m_DeviceMove.IsAvailable())
		//Device unavailable, abort!
		return;

	//Get path object
	CWObject* pPath;
	if (!_pFD->m_iPath || 
		!(pPath = m_pServer->Object_Get(_pFD->m_iPath)))
	{
		//Can't get path, stay put and abort!
		OnIdle();
		_pFD->m_iPath = 0;

		//Turn on gravity and animcontrol if necessary (again?)
		if (_pFD->m_bTurnOnGravity && !m_bWeightless)
		{
			m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETGRAVITY, 1), m_pGameObject->m_iObject);
			_pFD->m_bTurnOnGravity = false;
		};
		if (_pFD->m_bTurnOnAnimControl)
		{
			m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ANIMCONTROL, 1), m_pGameObject->m_iObject);
			_pFD->m_bTurnOnAnimControl = false;
		};

		return;
	};

	//Should we follow the paths relative movement, but not it's actual position?
	if (_pFD->m_iFollowMode & CAI_FollowPathData::FOLLOW_RELATIVE)
	{
		//Should we force follow relative?
		if (_pFD->m_iFollowMode & CAI_FollowPathData::FOLLOW_FORCE)
		{
			//Look in the same direction as path
			AddAimAtPositionCmd(GetAimPosition() + CVec3Dfp32::GetMatrixRow(pPath->GetPositionMatrix(),0));

			//Move in the same fashion as path
			CVec3Dfp32 PathMove = pPath->GetPosition() - pPath->GetLastPosition(); 
			fp32 Speed = Min(m_IdleMaxSpeed, PathMove.Length());
			AddMoveTowardsPositionCmd(m_pGameObject->GetPosition() + PathMove, Speed);
		}
		else
		{
			// *** Non-force following not implemented	
		};
	}
	//Should we "force follow" path?
	else if (_pFD->m_iFollowMode & CAI_FollowPathData::FOLLOW_FORCE)
	{
		//Jupp, always move directly towards the sender and look in the same direction as it
		AddAimAtPositionCmd(GetAimPosition() + CVec3Dfp32::GetMatrixRow(pPath->GetPositionMatrix(), 0));

		//The speed at which to move might be tuned better, and perhaps be based on the XY-distance instead of full 3D-distance.
		fp32 Speed = Min(m_IdleMaxSpeed, Max((pPath->GetPosition()).Distance(pPath->GetLastPosition()), CAI_Core::XYDistance(m_pGameObject->GetPosition(), pPath->GetPosition())));
		AddMoveTowardsPositionCmd(pPath->GetPosition(), Speed);
	}
	else
	{
		if (!_pFD->m_bFallenBehind &&
			(INVALID_POS(_pFD->m_PathPosition)))
		{
			CMat4Dfp32 Mat = GetEnginePathMatrix(_pFD->m_iPath);
			_pFD->m_PathPosition = CVec3Dfp32::GetRow(Mat, 3);
			_pFD->m_LookVector = CVec3Dfp32::GetRow(Mat, 0);
		}
		
		//Check distance to path position
		fp32 DistSqr = CAI_Core::XYDistanceSqr(m_pGameObject->GetPosition(), _pFD->m_PathPosition);

		//CVec3Dfp32 CharVel = m_pAI->m_pGameObject->GetMoveVelocity();//DEBUG
		//fp32 CharSpeed = Length2(CharVel[0], CharVel[1]);
		Debug_RenderWire(m_pGameObject->GetPosition(), _pFD->m_PathPosition);//DEBUG
		Debug_RenderWire(_pFD->m_PathPosition, _pFD->m_PathPosition + CVec3Dfp32(0,0,100), 0xffffffff, 0);
		
		//Check step length and speed
		fp32 StepLength;
		fp32 Speed;

		// *** Don't like this, FIX something better
		if (m_IdleMaxSpeed < 0.75f * GetMaxSpeedForward())
		{
			StepLength = GetWalkStepLength();
			Speed = GetMaxSpeedWalk();
		}
		else
		{
			StepLength = GetRunStepLength();
			Speed = GetMaxSpeedForward();
		}

		//If we've fallen behind, check where we need to go, or if we've caught up with the path.
		if (_pFD->m_bFallenBehind)
		{
			//Are we still behind the path?
			if (DistSqr < Sqr(StepLength/2))
			{
				//We've caught up!
				_pFD->m_bFallenBehind = false;
				CMat4Dfp32 Mat = GetEnginePathMatrix(_pFD->m_iPath);
				_pFD->m_PathPosition = CVec3Dfp32::GetRow(Mat, 3);
				_pFD->m_LookVector = CVec3Dfp32::GetRow(Mat, 0);
				DistSqr = CAI_Core::XYDistanceSqr(m_pGameObject->GetPosition(), _pFD->m_PathPosition);

				//Allow some extra time before we start force-propelling path when we've just returned to path.
				_pFD->m_LastPropelTick = GetAITick() + 10;
				_pFD->m_bStop = false;
			}
		}
		else
		{
			//Have we fallen behind just now?
			if (DistSqr > Sqr(StepLength))
			{
				//Jupp, we must catch up with path. 
				_pFD->m_bFallenBehind = true;
				_pFD->m_PathPosition = m_PathFinder.GetPathPosition(_pFD->m_PathPosition, 10, 2); 
			};
		};
		
		//Should we follow path normally or are we behind and need to navigate our way to the path
		if (_pFD->m_bFallenBehind) 
		{
			//We need to navigate if possible
			if (INVALID_POS(_pFD->m_PathPosition))
			{
				// Current pathpos is invalid, we propel the path a bit
				// to (hopefully) get a position that is legit next frame
				CMat4Dfp32 Mat;
				PropelPath(_pFD->m_iPath, StepLength, true, true, true, &Mat);
				if(m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETCURRENTMATRIX, (aint)(&Mat)), _pFD->m_iPath))
					_pFD->m_PathPosition = m_PathFinder.GetPathPosition(CVec3Dfp32::GetRow(Mat, 3), 10, 3);
			}

			if (INVALID_POS(_pFD->m_PathPosition))
			{
				//Return position still invalid. Stay put.
				StopNow();
				m_DeviceMove.Use();
			}
			else
			{
				//Return position ok! Stop path, and allow escape sequences
	 			m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, m_pGameObject->m_iObject), _pFD->m_iPath);
				m_bNoEscapeSeqMoves = false;
				OnMove(_pFD->m_PathPosition, m_IdleMaxSpeed,false,false,NULL);
			}
		}
		else 
		{
			//We're following path normally. Propel path each frame when at stop, 
			//or propel to new position whenever we get close to path
			if (_pFD->m_bStop || (DistSqr < Sqr(2 * Speed)))
			{
				//Since we allow stops when propelling, path won't move when at a stop until it has 
				//been propelled the number of frames it should stop, regardless of distance
				CVec3Dfp32 PrevPos = _pFD->m_PathPosition;
				CMat4Dfp32 Mat;
				PropelPath(_pFD->m_iPath, 2 * Speed, true, true, true, &Mat);
				_pFD->m_PathPosition = CVec3Dfp32::GetRow(Mat, 3);
				_pFD->m_LookVector = CVec3Dfp32::GetRow(Mat, 0);
				_pFD->m_LastPropelTick = GetAITick();				

				//Check if (still) at stop
				if (_pFD->m_PathPosition.AlmostEqual(PrevPos, 0.1))
				{
					_pFD->m_bStop = true;
				}
				else
				{
					_pFD->m_bStop = false;
				}
			}
			//If we haven't propelled path for a while, we're probably stuck and must propel path 
			//past the obstacle and then use pathfinding
			else if (GetAITick() > _pFD->m_LastPropelTick + 20)
			{
				CMat4Dfp32 Mat;
				PropelPath(_pFD->m_iPath, StepLength/2, true, true, true, &Mat);
				_pFD->m_LookVector = CVec3Dfp32::GetRow(Mat, 0);
				_pFD->m_PathPosition = CVec3Dfp32::GetRow(Mat, 3);
			}

			//Should we stop?
			if (_pFD->m_bStop)
			{
				m_DeviceMove.Use();
			}
			else 
			{
				//Go to path position 
				AddMoveTowardsPositionCmd(_pFD->m_PathPosition, m_IdleMaxSpeed);
			}
		}
	}
}


//Testing routine, remove later...
//Adds commands according to the given bot control flags.
void CAI_Core::OnBotControl(CControlFrame& _Ctrl, int32 _iBotControl, CWObject * _pController)
{
	if (!IsValid())
		return;

	{//Change bot movement? 

		//Movement vector due to commands
		CVec3Dfp32 Move(0);

		if (_iBotControl & BOT_MOVE_FORWARD)
		{//Move forward!
			AIOut("Bot moving forward");
			Move[0] += 1.0f;	
		};
		if (_iBotControl & BOT_MOVE_BACKWARD)
		{//Move backward!
			AIOut("Bot moving backward");
			Move[0] -= 1.0f;	
		};
		if (_iBotControl & BOT_MOVE_LEFT)
		{//Move left!
			AIOut("Bot moving left");
			Move[1] += 1.0f;	
		};
		if (_iBotControl & BOT_MOVE_RIGHT)
		{//Move right!
			AIOut("Bot moving right");
			Move[1] -= 1.0f;	
		};
		if (_iBotControl & BOT_MOVE_UP)
		{//Move left!
			AIOut("Bot moving up");
			Move[2] += 1.0f;	
		};
		if (_iBotControl & BOT_MOVE_DOWN)
		{//Move right!
			AIOut("Bot moving down");
			Move[2] -= 1.0f;	
		};

		//m_DeviceMove.Use(Move);
	}


	{//Change bot facing?

		//Look vector, due to commands
		CVec3Dfp32 Look(0);
		Look[0] = 0;

		if (_iBotControl & BOT_LOOK_LEFT)
		{//Look left!
			AIOut("Bot looking left");
			Look[2] -= 1.0f;	
		};
		if (_iBotControl & BOT_LOOK_RIGHT)
		{//Look right!
			AIOut("Bot looking right");
			Look[2] += 1.0f;	
		};
		if (_iBotControl & BOT_LOOK_UP)
		{//Look up!
			AIOut("Bot looking up");
			Look[1] += 1.0f;	
		};
		if (_iBotControl & BOT_LOOK_DOWN)
		{//Look down!
			AIOut("Bot looking down");
			Look[1] -= 1.0f;	
		};

		m_DeviceLook.Use(Look * 0.05f);
	}

/*
	if (_iBotControl & BOT_JUMP)
		//Jump!
		m_DeviceJump.Use();

	if (_iBotControl & BOT_CROUCH)
		//Crouch!
		m_DeviceStance.Use(CAI_Device_Stance::CROUCHING);
*/

	if (_iBotControl & BOT_PRIMARY_ATTACK)
		//Attack!
		m_DeviceWeapon.Use();

	if (_iBotControl & BOT_SWITCH_WEAPON)
		//Switch weapon! Note that character will give switch weapon once in every update 
		//during which the switch weapon key is held down. Will fix.
		m_DeviceWeapon.Use(CAI_Device_Item::SWITCH_NEXT);


	if (_iBotControl & BOT_FOLLOW_PATH)
	{	//Move to the positon set by the owner
		
		ConOutL(CStr("BOT_FOLLOW_PATH used, despite being obsolete"));
		/*
		//Get the controllers bot path destination
		CVec3Dfp32 ControllerBotPathDestination(0);
		CAI_Core * pAI;
		if (_pController && (pAI = GetAI(_pController->m_iObject)))
			ControllerBotPathDestination = pAI->m_PathDestination;

		OnMove(_Ctrl, ControllerBotPathDestination);
		*/
	};


	if (_iBotControl & BOT_TEST_STUFF) 
	{
		_iBotControl = 0;
	};


	//Bot is freed from control
	if (_iBotControl & BOT_AI_CONTROL) 
	{
		OnRefresh();
		_Ctrl = *GetControlFrame();
	};
};

//ConOuts stuff if the ms_bAIOut variable is true
void CAI_Core::AIOut(CStr Str)
{
	if (m_bAIOut)
		ConOutL(Str);
};

#endif
