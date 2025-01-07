//--------------------------------------------------------------------------------

#include "PCH.h"

#include "WObj_CharWaterEffect.h"
#include "WObj_Char.h"
#include "../../../Shared/MOS/Classes/GameWorld/Client/WClient_Core.h"

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

CWaterEffectEntry::CWaterEffectEntry(CVec3Dfp32& _Pos, CVec3Dfp32& _Fwd, fp32 _Fade, fp32 _Time, uint8 _Type)
{
	m_PosX = FloatPack32(_Pos[0], 60000);
	m_PosY = FloatPack32(_Pos[1], 60000);
	m_PosZ = FloatPack16(_Pos[2], 2000);
	m_Fwd = FloatPack8(CVec3Dfp32::AngleFromVector(_Fwd[0], _Fwd[1]), 1);
	m_Time = _Time;
	m_FadeAndType = FloatPack(_Fade, 1, 6) | ((_Type & 0x3) << 6);
}

//--------------------------------------------------------------------------------

CVec3Dfp32 CWaterEffectEntry::GetPos()
{
	fp32 PosX = FloatUnpack32(m_PosX, 60000);
	fp32 PosY = FloatUnpack32(m_PosY, 60000);
	fp32 PosZ = FloatUnpack16(m_PosZ, 2000);
	return CVec3Dfp32(PosX, PosY, PosZ);
}

//--------------------------------------------------------------------------------

CVec3Dfp32 CWaterEffectEntry::GetFwd()
{
	fp32 c, s;
	QSinCosUnit(FloatUnpack8(m_Fwd, 1), s, c);
	return CVec3Dfp32(c, s, 0);
}

//--------------------------------------------------------------------------------

fp32 CWaterEffectEntry::GetFade()
{
	return Clamp01(FloatUnpack(m_FadeAndType, 1, 6));
}

//--------------------------------------------------------------------------------

fp32 CWaterEffectEntry::GetTime()
{
	return m_Time;
}

//--------------------------------------------------------------------------------

uint8 CWaterEffectEntry::GetType()
{
	return ((m_FadeAndType >> 6) & 0x3);
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CWaterEffectManager::Clear()
{
	m_iEntry = -1;
	m_nEntries = 0;
	m_EntryDuration = WATEREFFECT_RIPPLEDURATION;
	m_WaterTimer = 0;
/*
	if ((m_WaterSplashClass != "") && (m_WaterSplashClass != "Cleared"))
		m_WaterSplashClass = "JIPPI!!!";

	m_WaterSplashClass = "Cleared";
*/
}

//--------------------------------------------------------------------------------

void CWaterEffectManager::CopyFrom(const CWaterEffectManager& _WEM)
{
	m_iEntry = _WEM.m_iEntry;
	m_nEntries = _WEM.m_nEntries;
	for (int iEntry = 0; iEntry < MAXNUMWATEREFFECTENTRIES; iEntry++)
		m_lEntries[iEntry] = _WEM.m_lEntries[iEntry];

	m_WaterSplashClass = _WEM.m_WaterSplashClass;
	m_WaterTimer = _WEM.m_WaterTimer;
/*
	if ((_WEM.m_WaterSplashClass != "") && (_WEM.m_WaterSplashClass != "Cleared"))
		m_WaterSplashClass = "JIPPI!!!";

	m_WaterSplashClass = _WEM.m_WaterSplashClass;
*/
}

//--------------------------------------------------------------------------------

void CWaterEffectManager::AddEntry(CVec3Dfp32& _Pos, CVec3Dfp32& _Fwd, fp32 _Fade, fp32 _Time, uint8 _Type)
{
	if (m_nEntries < MAXNUMWATEREFFECTENTRIES)
		m_nEntries++;

	m_iEntry++;
	if (m_iEntry >= MAXNUMWATEREFFECTENTRIES)
		m_iEntry -= MAXNUMWATEREFFECTENTRIES;

	m_lEntries[m_iEntry] = CWaterEffectEntry(_Pos, _Fwd, _Fade, _Time, _Type);
}

//--------------------------------------------------------------------------------

void CWaterEffectManager::Refresh(fp32 _GameTime)
{
}

//--------------------------------------------------------------------------------

bool CWaterEffectManager::RemapEntryIndex(int& _iEntry)
{
	if ((_iEntry < 0) || (_iEntry >= m_nEntries))
		return false;

	_iEntry = m_iEntry - _iEntry;
	if (_iEntry < 0)
		_iEntry += MAXNUMWATEREFFECTENTRIES;

	return true;
}

//--------------------------------------------------------------------------------

CWaterEffectEntry* CWaterEffectManager::GetEntry(int _iEntry)
{
	if (!RemapEntryIndex(_iEntry))
		return NULL;

	return &(m_lEntries[_iEntry]);
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

uint32 CWaterEffectManager::GetNumInstances()
{
	return m_nEntries;
}

//--------------------------------------------------------------------------------

bool CWaterEffectManager::GetInstanceAnimState(uint32 _iInstance, fp32 _GameTime, CXR_AnimState* _pAnimState)
{
	if (_pAnimState == NULL)
		return false;

	CWaterEffectEntry* pEntry = GetEntry(_iInstance);
	if (pEntry == NULL)
		return false;

	_pAnimState->m_Colors[0] = CPixel32(0, 0, 0, pEntry->GetFade() * 255);
	_pAnimState->m_AnimTime0 = _GameTime - pEntry->GetTime();
	return true;
}

//--------------------------------------------------------------------------------

bool CWaterEffectManager::GetInstanceWorldMatrix(uint32 _iInstance, CMat43fp32* _pMatrix)
{
	if (_pMatrix == NULL)
		return false;

	CWaterEffectEntry* pEntry = GetEntry(_iInstance);
	if (pEntry == NULL)
		return false;

	_pMatrix->Unit();
	pEntry->GetPos().SetMatrixRow(*_pMatrix, 3);
	pEntry->GetFwd().SetMatrixRow(*_pMatrix, 0);
	CVec3Dfp32(0, 0, 1).SetMatrixRow(*_pMatrix, 2);
	_pMatrix->RecreateMatrix(0, 2);
	return true;
}

//--------------------------------------------------------------------------------

bool CWaterEffectManager::GetInstance(uint32 _iInstance, fp32 _GameTime, CXR_AnimState* _pAnimState, CMat43fp32* _pMatrix)
{
	if (_pAnimState == NULL)
		return false;

	if (_pMatrix == NULL)
		return false;

	CWaterEffectEntry* pEntry = GetEntry(_iInstance);
	if (pEntry == NULL)
		return false;

	_pAnimState->m_Colors[0] = CPixel32(255, 255, 255, pEntry->GetFade() * 255);
	_pAnimState->m_AnimTime0 = _GameTime - pEntry->GetTime();

	_pMatrix->Unit();
	pEntry->GetPos().SetMatrixRow(*_pMatrix, 3);
	pEntry->GetFwd().SetMatrixRow(*_pMatrix, 0);
	CVec3Dfp32(0, 0, 1).SetMatrixRow(*_pMatrix, 2);
	_pMatrix->RecreateMatrix(0, 2);

/*
	if (pEntry->GetTime() == 0)
		ConOut("ERROR");

	int iEntry = _iInstance;
	if (!RemapEntryIndex(iEntry))
		iEntry = -1;
	CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(*_pMatrix, 3);

	ConOutL(CStrF("WaterEffects: GetInstance(%d) - SpawnTime=%3.3f, iEntry=%d, AnimTime=%3.3f, Colors=%X, Pos=<%3.3f, %3.3f, %3.3f>", _iInstance, pEntry->GetTime(), iEntry, _pAnimState->m_AnimTime0, _pAnimState->m_Colors[0], Pos[0], Pos[1], Pos[2]));
*/
	return true;
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

int CWObject_Character::FindWaterLevel(CVec3Dfp32& _Pos1, CVec3Dfp32& _Pos2, CWorld_PhysState* _pWPhysState, CVec3Dfp32& _SurfacePos)
{
/*
	* _Pos1, _Pos2	: Line to sample.
	* _SurfacePos	: Watersurface position along sampleline.
	* Returns		: MediumFlags if no intersection/watersurface was found, else 0.
*/
#ifndef M_RTM
	fp32 DebugRenderDuration = 0.1f;
#endif
	
	fp32 MaxSampleDistance = 1.0f;
	const int nMediumSamples = 8;
	CVec3Dfp32 MediumSamplePos[nMediumSamples];
	CXR_MediumDesc MediumSample[nMediumSamples];

	CVec3Dfp32 Delta = _Pos2 - _Pos1;
	fp32 TotalDistance = Delta.Length();
	fp32 SampleDistance = TotalDistance / fp32(nMediumSamples);

	int iSample;
	
	for (iSample = 0; iSample < nMediumSamples; iSample++)
	{
		MediumSamplePos[iSample] = _Pos1 + Delta * (fp32(iSample) / fp32(nMediumSamples-1));
#ifndef M_RTM
		_pWPhysState->Debug_RenderVertex(MediumSamplePos[iSample], 0xFF2020F0, DebugRenderDuration);
#endif
	}
	
	_pWPhysState->Phys_GetMediums(MediumSamplePos, nMediumSamples, MediumSample);
	
	for (iSample = 0; iSample < nMediumSamples; iSample++)
	{
		if ((MediumSample[iSample].m_MediumFlags & XW_MEDIUM_WATER) != 0)
		{
			if (iSample == 0)
			{
				return XW_MEDIUM_WATER;
			}
			else
			{
				if (SampleDistance <= MaxSampleDistance)
				{
					_SurfacePos = MediumSamplePos[iSample];
#ifndef M_RTM
					_pWPhysState->Debug_RenderVertex(_SurfacePos, 0xFF20F020, 50);
#endif
					return 0;
				}
				else
				{
					return FindWaterLevel(MediumSamplePos[iSample-1], MediumSamplePos[iSample], _pWPhysState, _SurfacePos);
				}
			}
		}
	}

	return MediumSample[0].m_MediumFlags;
}

//--------------------------------------------------------------------------------

void CWObject_Character::AddWaterEffect(CMat43fp32& _Pos, fp32 _Fade, int _Type, CWObject_CoreData* _pObj, CWorld_Client* _pWClient, fp32 _GameTime)
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if (!pCD) return;

	if (pCD->m_bDoOneTimeEffects)
	{
		int iSound = 0;
		switch (_Type)
		{
			case WATEREFFECTTYPE_RIPPLE: iSound = _pWClient->GetMapData()->GetResourceIndex_Sound(WATEREFFECT_RIPPLE_SOUND); break;
			case WATEREFFECTTYPE_SPLASH: iSound = _pWClient->GetMapData()->GetResourceIndex_Sound(WATEREFFECT_SPLASH_SOUND); break;
		}
		_pWClient->Sound_At(WCLIENT_CHANNEL_SFX, CVec3Dfp32::GetMatrixRow(_Pos, 3), iSound, 0);
	}
	pCD->m_WaterEffectManager.AddEntry(CVec3Dfp32::GetMatrixRow(_Pos, 3), CVec3Dfp32::GetMatrixRow(_Pos, 0), _Fade, _GameTime, _Type);

/*
	if (_Type == WATEREFFECTTYPE_RIPPLE)
	{
		if (pCD->m_bDoOneTimeEffects)
		{
			int iSound = _pWClient->GetMapData()->GetResourceIndex_Sound(WATEREFFECT_RIPPLE_SOUND);
			_pWClient->Sound_At(WCLIENT_CHANNEL_SFX, CVec3Dfp32::GetMatrixRow(_Pos, 3), iSound, 0);
		}
		pCD->m_WaterEffectManager.AddEntry(CVec3Dfp32::GetMatrixRow(_Pos, 3), CVec3Dfp32::GetMatrixRow(_Pos, 0), _Fade, _GameTime, _Type);
	}
	else
	{
		_pWClient->ClientObject_Create(pCD->m_WaterEffectManager.m_WaterSplashClass, _Pos);
	}
*/
}

//--------------------------------------------------------------------------------

void CWObject_Character::OnClientRefresh_WaterEffect(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, fp32 _GameTime)
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if (!pCD) return;

	pCD->m_WaterEffectManager.Refresh(_GameTime);

	if (!IsAlive(_pObj))
		return;

	CVec3Dfp32 FeetPos = _pObj->GetPosition() + CVec3Dfp32(0, 0, pCD->m_Phys_Height * 0.1f);
	CVec3Dfp32 HeadPos = _pObj->GetPosition() + CVec3Dfp32(0, 0, pCD->m_Phys_Height * 2.0f);
	CVec3Dfp32 SurfacePos;
	int SampleMedium = FindWaterLevel(HeadPos, FeetPos, _pWClient, SurfacePos);
	if (SampleMedium == 0)
	{
		CVec3Dfp32 ObjPos = _pWClient->Object_GetPosition(_pObj->m_iObject);
		CVec3Dfp32 ObjFwd = CVec3Dfp32::GetMatrixRow(_pWClient->Object_GetPositionMatrix(_pObj->m_iObject), 0);
		CVec3Dfp32 ObjVelo = _pWClient->Object_GetVelocity(_pObj->m_iObject);
		fp32 ObjHVelo = ObjVelo[2]; ObjVelo[2] = 0;
		fp32 ObjVeloScalar = ObjVelo.Length();
		CVec3Dfp32 EffectFwd = LERP(ObjFwd, ObjVelo, Clamp01(ObjVeloScalar / 2.0f));

		CMat43fp32 EffectPos;
		EffectPos.Unit();
		EffectFwd.SetMatrixRow(EffectPos, 0);
		CVec3Dfp32(0, 0, 1).SetMatrixRow(EffectPos, 2);
		EffectPos.RecreateMatrix(2, 0);
		SurfacePos[0] = ObjPos[0];
		SurfacePos[1] = ObjPos[1];
		SurfacePos.SetMatrixRow(EffectPos, 3);

		fp32 Fade = Clamp01(0.3f + (ObjVeloScalar / 2.0f));

		if (pCD->m_WaterEffectManager.m_WaterTimer >= 0.05f)
		{
			AddWaterEffect(EffectPos, Fade, WATEREFFECTTYPE_RIPPLE, _pObj, _pWClient, _GameTime);
/*
			if (ObjVeloScalar > 0)
			{
				(ObjVelo * 0.5f).AddMatrixRow(EffectPos, 3);
				AddWaterEffect(EffectPos, Fade, WATEREFFECTTYPE_RIPPLE, _pObj, _pWClient, _GameTime);
			}
*/
			pCD->m_WaterEffectManager.m_WaterTimer = 0;
		}
		else if ((ObjHVelo < -0.5f) && (pCD->m_WaterEffectManager.m_WaterTimer < -0.0f))
		{
			AddWaterEffect(EffectPos, 1, WATEREFFECTTYPE_SPLASH, _pObj, _pWClient, _GameTime);
			pCD->m_WaterEffectManager.m_WaterTimer = 0;
		}
		else if (pCD->m_WaterEffectManager.m_WaterTimer < 0)
			pCD->m_WaterEffectManager.m_WaterTimer = 0;
		else
			pCD->m_WaterEffectManager.m_WaterTimer += SERVER_TIMEPERFRAME;
	}
	else
	{
		if (pCD->m_WaterEffectManager.m_WaterTimer > 0)
			pCD->m_WaterEffectManager.m_WaterTimer = 0;
		else
			pCD->m_WaterEffectManager.m_WaterTimer -= SERVER_TIMEPERFRAME;
	}
}

//--------------------------------------------------------------------------------

void CWObject_Character::OnClientRender_WaterEffects(CWObject_Client* _pObj, CWorld_Client* _pWClient, fp32 _GameTime, CXR_Engine* _pEngine)
{
	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if (pCD == NULL) return;

	CMat43fp32 EffectMatrix; EffectMatrix.Unit();

	CXR_AnimState EffectAnimState;
	int iEffectModel;
	CXR_Model* pEffectModel;
	
	for (int iEntry = 0; iEntry < pCD->m_WaterEffectManager.GetNumEntries(); iEntry++)
	{
		CWaterEffectEntry* pEntry = pCD->m_WaterEffectManager.GetEntry(iEntry);
		if( pEntry == NULL )
			continue;

		uint8 EntryType = pEntry->GetType();

		if( pCD->m_WaterEffectManager.GetInstance(iEntry, _GameTime, &EffectAnimState, &EffectMatrix) )
		{
			if (EntryType == WATEREFFECTTYPE_SPLASH)
			{
				if (WATEREFFECT_SPLASH_MODEL0)
				{
					iEffectModel = _pWClient->GetMapData()->GetResourceIndex_Model(WATEREFFECT_SPLASH_MODEL0);
					pEffectModel = _pWClient->GetMapData()->GetResource_Model(iEffectModel);
					if (pEffectModel != NULL)
						_pEngine->Render_AddModel(pEffectModel, EffectMatrix, EffectAnimState);
				}

				if (WATEREFFECT_SPLASH_MODEL1)
				{
					iEffectModel = _pWClient->GetMapData()->GetResourceIndex_Model(WATEREFFECT_SPLASH_MODEL1);
					pEffectModel = _pWClient->GetMapData()->GetResource_Model(iEffectModel);
					if (pEffectModel != NULL)
						_pEngine->Render_AddModel(pEffectModel, EffectMatrix, EffectAnimState);
				}

				if (WATEREFFECT_SPLASH_MODEL2)
				{
					iEffectModel = _pWClient->GetMapData()->GetResourceIndex_Model(WATEREFFECT_SPLASH_MODEL2);
					pEffectModel = _pWClient->GetMapData()->GetResource_Model(iEffectModel);
					if (pEffectModel != NULL)
						_pEngine->Render_AddModel(pEffectModel, EffectMatrix, EffectAnimState);
				}
			}

		}
	}

	if (pCD->m_WaterEffectManager.GetNumEntries() > 0)
	{
/*		if (pCD->m_WaterEffectManager.GetNumEntries() < MAXNUMWATEREFFECTENTRIES)
		{
			ConOut("WaterEffects: nEntries < MaxEntries");
		}*/
		EffectMatrix.Unit();
		EffectAnimState.Clear();
		EffectAnimState.m_AnimTime0 = _GameTime;
		EffectAnimState.m_pContext = &(pCD->m_WaterEffectManager);
		iEffectModel = _pWClient->GetMapData()->GetResourceIndex_Model(WATEREFFECT_RIPPLE_MODEL0);
		pEffectModel = _pWClient->GetMapData()->GetResource_Model(iEffectModel);
		if (pEffectModel != NULL)
			_pEngine->Render_AddModel(pEffectModel, EffectMatrix, EffectAnimState);
	}
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
