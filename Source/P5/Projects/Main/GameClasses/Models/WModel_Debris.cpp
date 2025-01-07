
#include "PCH.h"

#include "WModel_Debris.h"

#include "MFloat.h"
#include "CModelHistory2.h"


#include "../../../../Shared/MOS/Classes/GameWorld/Client/WClient.h"

// -------------------------------------------------------------------
//  CDebrisTrails
// -------------------------------------------------------------------
void CDebrisTrails::Create(uint32 _iRand, int _NumDebris, fp32 _Duration, const CVec3Dfp32 &_Dir, fp32 _Speed, fp32 _Scatter, fp32 _SamplePeriod, int _MaxCollisions, const CMat4Dfp32& _WMat, CWorld_Client *_pWClient)
{
	MAUTOSTRIP(CDebrisTrails_Create, MAUTOSTRIP_VOID);
	m_SampleRate = 1.0f / _SamplePeriod;
	int NumSamples = (int)(_Duration / _SamplePeriod);
	const float GravityForce = 20 * _SamplePeriod;
	
	if (NumSamples > MAXSAMPLES)
	{
#ifdef PLATFORM_DOLPHIN
		OSReport("WARNING: CDebrisTrails::Create, NumSamples = %d, MAXSAMPLES = %d\n", NumSamples, MAXSAMPLES);
#endif
//		Error("Create", "Maxsamples is too low");
		NumSamples = MAXSAMPLES;
	}
	
	CMat4Dfp32 IWMat;
	_WMat.InverseOrthogonal(IWMat);
	
	m_Trails.SetLen(_NumDebris);
	
	for(int t = 0; t < _NumDebris; t++)
	{
	/*		CVec3Dfp32 V(_Speed * (0.5 * MFloat_GetRand(_iRand++) + 0.5),
	_Speed * (0.5 * (4 * MFloat_GetRand(_iRand++) - 1)),
		_Speed * (0.5 * (4 * MFloat_GetRand(_iRand++) - 1)));*/
		CVec3Dfp32 V(_Dir[0] * _Speed + _Scatter * (2.0f*(MFloat_GetRand(_iRand + 0) - 0.5f)),
					_Dir[1] * _Speed + _Scatter * (2.0f*(MFloat_GetRand(_iRand + 1) - 0.5f)),
					_Dir[2] * _Speed + _Scatter * (2.0f*(MFloat_GetRand(_iRand + 2) - 0.5f)));
		CVec3Dfp32 Gravity = -CVec3Dfp32::GetMatrixRow(IWMat, 2) * GravityForce;
		_iRand += 3;
		
		V *= (0.8f + 0.2f*MFloat_GetRand(_iRand++));
		
		int Collisions = 0;
		CVec3Dfp32 A = 0;
		CVec3Dfp32 VA = CVec3Dfp32(MFloat_GetRand(_iRand + 0) * 0.2f, MFloat_GetRand(_iRand + 1) * 0.2f, MFloat_GetRand(_iRand + 2) * 0.2f);
		_iRand += 3;
		
		CVec3Dfp32 Pos(0);
		int i;
		for(i = 0; i < NumSamples; i++)
		{
			m_Trails[t].m_SamplePos[i] = Pos;
			m_Trails[t].m_Angles[i] = A;
			
			if(Collisions < _MaxCollisions)
			{
				A += VA;

				CVec3Dfp32 New = Pos + V;
				CCollisionInfo Info;
				Info.m_bIsValid = false;
				//Info.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION);
				if(_pWClient->Phys_IntersectLine(Pos * _WMat, New * _WMat, 0, OBJECT_FLAGS_PHYSMODEL, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &Info))
				{
					Collisions++;
					if(Info.m_bIsValid)
					{
/*						if(Info.m_Plane.n * V < 0.5f && V.Length() > 20 && i < NumSamples - 2)
						{
							i++;
							m_Trails[t].m_SamplePos[i] = (Info.m_Pos + Info.m_Plane.n * 1) * IWMat;
							m_Trails[t].m_Angles[i] = A;
							i++;
							m_Trails[t].m_SamplePos[i] = (Info.m_Pos + Info.m_Plane.n * 1) * IWMat;
							m_Trails[t].m_Angles[i] = A;
							break;
						}
						else*/
						{
							CVec3Dfp32 Res;
							CVec3Dfp32 Plane = Info.m_Plane.n;
							Plane.MultiplyMatrix3x3(IWMat);
							V.Reflect(Plane, Res);
							V = Res * 0.4f;
							New = (Info.m_Pos + Info.m_Plane.n * 0.1f) * IWMat;
							VA = CVec3Dfp32(MFloat_GetRand(_iRand + 0) * 0.1f, MFloat_GetRand(_iRand + 1) * 0.1f, MFloat_GetRand(_iRand + 2) * 0.1f);
							_iRand += 3;
						}
					}
					else
						ConOut(CStrF("(CDebrisTrails::Create) No CInfo.  %d, %d", t, i));
				}
				
				Pos = New;			
				V += Gravity;
			}
		}
		m_Trails[t].m_NumSamples = i;
	}
}

CVec3Dfp32 CDebrisTrails::GetTrailPos(int _iTrail, fp32 _Time, CVec3Dfp32 *_pAngles, fp32 *_pSpeedFactor)
{
	MAUTOSTRIP(CDebrisTrails_GetTrailPos, CVec3Dfp32());
	const fp32 Index = _Time * m_SampleRate;
	const int iIndex = TruncToInt(Index);
	if(iIndex >= m_Trails[_iTrail].m_NumSamples - 1)
	{
		if(_pAngles)
			*_pAngles = m_Trails[_iTrail].m_Angles[m_Trails[_iTrail].m_NumSamples - 1];
		if(_pSpeedFactor)
			*_pSpeedFactor = 0;
		return m_Trails[_iTrail].m_SamplePos[m_Trails[_iTrail].m_NumSamples - 1];
	}
	else
	{
		if(_pAngles)
		{
			CVec3Dfp32 A0 = m_Trails[_iTrail].m_Angles[iIndex];
			CVec3Dfp32 A1 = m_Trails[_iTrail].m_Angles[iIndex + 1];
			*_pAngles = A0 + (A1 - A0) * (Index - iIndex);
		}
		CVec3Dfp32 P0 = m_Trails[_iTrail].m_SamplePos[iIndex];
		CVec3Dfp32 P1 = m_Trails[_iTrail].m_SamplePos[iIndex + 1];
		if(_pSpeedFactor)
			*_pSpeedFactor = (P1 - P0).LengthSqr();
		return P0 + (P1 - P0) * (Index - iIndex);
	}
}

int CDebrisTrails::GetNumSamples(int _iTrail)
{
	return m_Trails[_iTrail].m_NumSamples;
}

int CDebrisTrails::GetIndexFromTime(int _iTrail, fp32 _Time, fp32 &_Frac)
{
	MAUTOSTRIP(CDebrisTrails_GetIndexFromTime, 0);
	const fp32 Index = _Time * m_SampleRate;
	const int iIndex = Min(m_Trails[_iTrail].m_NumSamples - 1, TruncToInt(Index));
	_Frac = Index - fp32(iIndex);
	return iIndex;
}

TPtr<CXR_ModelInstance> CDebrisTrails::Duplicate() const
{
	return NULL;
}

void CDebrisTrails::operator= (const CXR_ModelInstance &_Instance)
{
}


// -------------------------------------------------------------------
//  CXR_Model_Debris
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Debris, CXR_Model_Custom);

CXR_Model_Debris::CXR_Model_Debris()
{
	MAUTOSTRIP(CXR_Model_Debris_ctor, MAUTOSTRIP_VOID);
	m_Duration = 4;
	m_Scatter = 16.0f;
	m_Speed = 8;
	m_SamplePeriod = 0.05f;
	m_NumDebris = 8;
	m_MaxCollisions = 10;
	m_bUseDirection = false;
}

spCDebrisTrails CXR_Model_Debris::Compile(int _iRand, const CMat4Dfp32& _WMat, CWorld_Client *_pWClient)
{
	MAUTOSTRIP(CXR_Model_Debris_Compile, NULL);
	spCDebrisTrails spTrails = MNew(CDebrisTrails);
	if(!spTrails)
		return NULL;

	CVec3Dfp32 Dir;
	if(m_bUseDirection)
		Dir = CVec3Dfp32(1, 0, 0);
	else
		Dir = 0;

	spTrails->Create(_iRand, m_NumDebris, m_Duration, Dir, m_Speed, m_Scatter, m_SamplePeriod, m_MaxCollisions, _WMat, _pWClient);
	
	return spTrails;
}


#ifndef M_DISABLE_TODELETE
// -------------------------------------------------------------------
//  CXR_Model_DebrisTest
// -------------------------------------------------------------------
void CXR_Model_DebrisTest::Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
{
	MSCOPESHORT(CXR_Model_DebrisTest::Render);
	MAUTOSTRIP(CXR_Model_DebrisTest_Render, MAUTOSTRIP_VOID);
	if(!_pAnimState->m_pModelInstance)
		return;
	
	if(_pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/ > m_Duration)
		return;
	
	CDebrisTrails *pTrails = (CDebrisTrails *)_pAnimState->m_pModelInstance;
	if(!pTrails)
		return;
	
	CMat4Dfp32 L2V;
	_WMat.Multiply(_VMat, L2V);
	
	for(int i = 0; i < pTrails->GetNumTrails(); i++)
	{
		CMat4Dfp32 Move, Res;
		Move.Unit();
		pTrails->GetTrailPos(i, _pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/).SetMatrixRow(Move, 3);
		Move.Multiply(L2V, Res);

		CXR_VertexBuffer *pVB = CXR_Util::Create_Star(_pRenderParams->m_pVBM, Res, 8, 0xffffffff);
		if (!pVB)
			return;
		_pRenderParams->m_pVBM->AddVB();
	}
	return;
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_DebrisTest, CXR_Model_Debris);
#endif

#ifndef M_DISABLE_TODELETE

// -------------------------------------------------------------------
//  CXR_Model_DebrisTest
// -------------------------------------------------------------------
void CXR_Model_RocketDebris::Create(const char* _pParam)
{
	MAUTOSTRIP(CXR_Model_RocketDebris_Create, MAUTOSTRIP_VOID);
	CXR_Model_Debris::Create(_pParam);
	
	MRTC_SAFECREATEOBJECT_NOEX(spTSK, "CXW_SurfaceKeyFrame", CXW_SurfaceKeyFrame);
	if (!spTSK) Error("OnCreate", "No CXW_SurfaceKeyFrame available.");
	m_spTmpSurfKey = spTSK;
	
	m_iDefSurface = 0;
	m_iDefTrailSurface = 0;
	if (_pParam)
	{
		CStr Param(_pParam);
		
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
		if (!pSC) Error("Create", "No surface-context available.");
		m_iDefSurface = pSC->GetSurfaceID(Param.GetStrSep(","));
		
		//			if (Param.Len())
		//				m_DefSize = Param.Getfp64Sep(",");
		//			ConOutL(CStrF("(CXR_Model_RocketDebris::Create) Params %s, iDefSurface %d, DefSize %f", _pParam, m_iDefSurface, m_DefSize));
	}
	else
	{
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
		if (!pSC) Error("Create", "No surface-context available.");
		m_iDefTrailSurface = pSC->GetSurfaceID("S_DEBRISTRAIL");
		m_iDefSurface = pSC->GetSurfaceID("S_DEBRISPARTICLE");
	}
	
	m_Duration = 1.2f;
	m_Speed = 35;
	m_SamplePeriod = 0.05f;
	m_NumDebris = 12;
	m_Scatter = 10.0f;
	m_BoundRadius = 128;
}

void CXR_Model_RocketDebris::OnCreate(const char *_pParams)
{
	MAUTOSTRIP(CXR_Model_RocketDebris_OnCreate, MAUTOSTRIP_VOID);
	m_iTexture = GetTextureID("B_SMOKE01");
}

void CXR_Model_RocketDebris::Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
{
	MAUTOSTRIP(CXR_Model_RocketDebris_Render, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_RocketDebris::Render);
	if(!_pAnimState->m_pModelInstance)
		return;
	
	if(_pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/ > m_Duration)
		return;
	
	CDebrisTrails *pTrails = (CDebrisTrails *)_pAnimState->m_pModelInstance;
	if(!pTrails)
		return;
	
	const float BeamTimeStep = 0.1f;
	
	CMat4Dfp32 L2V;
	_WMat.Multiply(_VMat, L2V);
	
	CXR_BeamStrip lBeams[CXR_Util::MAXBEAMS];
	
	fp32 TimeSpan = 0.5f;
	fp32 TimeSpanInv = 1.0f / TimeSpan;
	fp32 TStart = Max(fp32(TruncToInt((_pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/ - TimeSpan) / BeamTimeStep))*BeamTimeStep, 0.0f);
	
	CXR_Particle2 lParticles[128];
	if (m_NumDebris > 128) return;
	
	CXW_Surface* pSurf = _pAnimState->m_lpSurfaces[0];
	if (!pSurf)
	{
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
		if (!pSC) Error("Create", "No surface-context available.");
		pSurf = pSC->GetSurface(m_iDefTrailSurface);
		if (!pSurf)
		{
			if (!pSurf)
				ConOut("(CXR_Model_RocketDebris::Render) No Surface.");
			return;
		}
	}
	
	CMTime SurfTime = pSurf->m_iController ? _pAnimState->m_AnimTime1 : _pAnimState->m_AnimTime0;
	CXW_SurfaceKeyFrame* pSurfKey = pSurf->GetFrame(
		pSurf->m_iController ? _pAnimState->m_Anim1 : _pAnimState->m_Anim0, 
		SurfTime, *m_spTmpSurfKey);
	
	for(int i = 0; i < pTrails->GetNumTrails(); i++)
	{
		int iBeam = 0;
		float Time = TStart;
		while(Time < _pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/)
		{
			fp32 Age = (_pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/ - Time) * TimeSpanInv;
			lBeams[iBeam].m_Pos = pTrails->GetTrailPos(i, Time);
			int Col = Clamp01(1.0f-Age)*255.0f;
			lBeams[iBeam].m_Color = Col + (Col << 8) + (Col << 16) + (Col << 24);
			lBeams[iBeam].m_Width = 2.5f + Age*4.0f;
			lBeams[iBeam].m_TextureYOfs = 0;
			iBeam++;
			Time += BeamTimeStep;
		}
		
		lBeams[iBeam].m_Pos = pTrails->GetTrailPos(i, _pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/);
		lBeams[iBeam].m_Color = 0x00ffffff;
		lBeams[iBeam].m_Width = 2.5f;
		lBeams[iBeam].m_TextureYOfs = 0;
		iBeam++;
		
		lParticles[i].m_Pos = lBeams[iBeam].m_Pos;
		lParticles[i].m_Pos *= _WMat;
		lParticles[i].m_Color = 0xffffffff;
		lParticles[i].m_Angle = 0;
		lParticles[i].m_Size = 24.0f;
		
		/*			if(iBeam > 0)
		{
		lBeams[0].m_Color = 0x00ffffff;
		lBeams[iBeam - 1].m_Color = 0x00ffffff;
	}*/
		
		//	ConOut(CStrF("Trail %d, BeamLen %d", i, iBeam));
		
		CXR_VertexBuffer *pVB = AllocVB();
		if(!pVB)
			return;
		pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
		pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_CULL);
		pVB->m_Priority = _pRenderParams->m_RenderInfo.m_BasePriority_Transparent;
		if(CXR_Util::Render_BeamStrip(_pRenderParams->m_pVBM, pVB, _WMat, _VMat, lBeams, iBeam))
		{
			//				m_pVBM->AddVB(pVB);
			int Flags = 0;//RENDERSURFACE_VERTEXFOG*0;
			const CMat4Dfp32* pM2V = pVB->m_pTransform[0];
			CXR_Util::Render_Surface(Flags, SurfTime, pSurf, pSurfKey, _pRenderParams->m_pEngine, _pRenderParams->m_pVBM, pM2V, NULL, (CMat4Dfp32*)NULL, pVB, 
				_pRenderParams->m_RenderInfo.m_BasePriority_Transparent, 0.0001f, 0);
		}
		//				m_pVBM->AddVB(pVB);
	}
	
#ifdef NEVER
	pSurf = _pAnimState->m_lpSurfaces[1];
	if (!pSurf)
	{
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
		if (!pSC) Error("Create", "No surface-context available.");
		pSurf = pSC->GetSurface(m_iDefSurface);
		if (!pSurf)
		{
			if (!pSurf)
				ConOut("(CXR_Model_RocketDebris::Render) No Surface.");
			return false;
		}
	}
	
	CXR_VertexBuffer* pVB = _pRenderParams->m_pVBM->Alloc_VB();
	if (!pVB) return false;
	if (CXR_Util::Render_Particles(_pRenderParams->m_pVBM, pVB, _VMat, lParticles, m_NumDebris, NULL, CXR_PARTICLETYPE_QUAD | CXR_PARTICLETYPE_ANGLE))
	{
		pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;

		CMTime SurfTime = pSurf->m_iController ? _pAnimState->m_AnimTime1 : _pAnimState->m_AnimTime0;
		CXW_SurfaceKeyFrame* pSurfKey = pSurf->GetFrame(
			pSurf->m_iController ? _pAnimState->m_Anim1 : _pAnimState->m_Anim0, 
			SurfTime, *m_spTmpSurfKey);
		
		int Flags = RENDERSURFACE_VERTEXFOG;
		CXR_Util::Render_Surface(Flags, SurfTime, pSurf, pSurfKey, m_pEngine, m_pVBM, NULL, NULL, NULL, pVB, 
			m_RenderInfo.m_BasePriority_Transparent, 0.0001f, 0);
	}
#endif
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_RocketDebris, CXR_Model_Debris);
#endif

#ifndef M_DISABLE_TODELETE
// -------------------------------------------------------------------
//  CXR_Model_SurfaceDebris
// -------------------------------------------------------------------
class CXR_Model_SurfaceDebris : public CXR_Model_Debris, public CXR_ViewClipInterface
{
	MRTC_DECLARE;

private:

	int32	m_iSurface;

public:
	void Create(const char* _pParam)
	{
		MAUTOSTRIP(CXR_Model_SurfaceDebris_Create, MAUTOSTRIP_VOID);
		m_NumDebris = 4;
		m_Speed = 8;
		m_Scatter = 8;
		m_bUseDirection = true;

		m_BoundRadius = 512;
		
		CXR_Model_Debris::Create(_pParam);

		m_iSurface = GetSurfaceID("DebrisSparks01");
	}

	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
	{
		MAUTOSTRIP(CXR_Model_SurfaceDebris_Render, MAUTOSTRIP_VOID);
		MSCOPESHORT(CXR_Model_SurfaceDebris::Render);
		if(!_pAnimState->m_pModelInstance)
			return;
		
		if(_pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/ > m_Duration)
			return;
		
		renderSparks(_pAnimState, _WMat, _VMat);

		CXW_Surface *pSurf = (CXW_Surface *)(*(int *)&_pAnimState->m_AnimAttr0);
		if(!pSurf)
			return;

		CDebrisTrails *pTrails = (CDebrisTrails *)_pAnimState->m_pModelInstance;
		if(!pTrails)
			return;

		int iBaseRand = _pAnimState->m_Anim0;

		CXR_AnimState AnimState(_pAnimState->m_AnimTime0, CMTime(), 0, 0, 0, 0);
		
		AnimState.m_lpSurfaces[0] = pSurf;
		for(int i = 0; i < pTrails->GetNumTrails(); i++)
		{
			if(_pAnimState->m_Colors[i] && _pAnimState->m_Colors[i] != -1)
			{
				CXR_Model *pModel = (CXR_Model *)_pAnimState->m_Colors[i];
				if(pModel && int(pModel) != -1)
				{
					CVec3Dfp32 Angles;
					CVec3Dfp32 Pos = pTrails->GetTrailPos(i, _pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/, &Angles);
					CMat4Dfp32 MAngles, Mat;
					Angles.CreateMatrixFromAngles(0, MAngles);
					Pos.SetMatrixRow(MAngles, 3);
					MAngles.Multiply(_WMat, Mat);
					pModel->OnRender(m_pEngine, m_pRC, m_pVBM, this, m_pWLS, &AnimState, Mat, _VMat);
				}

			}
			iBaseRand += MAXSAMPLES * 3;
		}
	}

	void
	renderSparks(const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
	{
		MAUTOSTRIP(CXR_Model_SurfaceDebris_renderSparks, MAUTOSTRIP_VOID);
		CXR_VertexBuffer* pVB = AllocVB();
		if (pVB == NULL)
			return;

		int32			numSparks = 10;
		int32			numBeamStripsPerSpark = 4;
		int32			numBeamStrips = numSparks * numBeamStripsPerSpark;
		CXR_BeamStrip	beamstrips[100];

		int32 randseed = _pAnimState->m_Anim0;

		CMat4Dfp32 LocalToCamera;
		_WMat.Multiply(_VMat, LocalToCamera);

//		fp32 duration = _pAnimState->m_AnimTime1;
		fp32 duration = 0.3f;
		fp32 timefraction = _pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/ / duration;

		if (timefraction > 1.0f)
			return;

		fp32 distance = LocalToCamera.k[3][2];
		fp32 width = 0.5f + distance / 300.0f;
		fp32 minLength = 10.0f;
		fp32 maxLength = 20.0f;
		fp32 maxTravelLength = 30.0f;

		fp32 alpha = 255.0f * (1.0f - timefraction);

		for (int32 i = 0; i < numSparks; i++)
		{
			CVec3Dfp32 dir;
			dir.k[0] = (MFloat_GetRand(randseed++) - 0.2f);
			dir.k[1] = 0.4f * (MFloat_GetRand(randseed++) - 0.5f);
			dir.k[2] = (MFloat_GetRand(randseed++) - 0.5f);
			dir.Normalize();
			
			for (int32 j = 0; j < numBeamStripsPerSpark; j++) {
				fp32 sparkfraction = (fp32)j / (fp32)(numBeamStripsPerSpark - 1);
				fp32 length = minLength + (maxLength - minLength) * MFloat_GetRand(randseed++);
				fp32 pos = maxTravelLength * timefraction - sparkfraction * length;
				if (pos < 0.0f) pos = 0.0f;

				beamstrips[i * numBeamStripsPerSpark + j].m_Pos = dir * pos;
				beamstrips[i * numBeamStripsPerSpark + j].m_Width = width * M_Sin(sparkfraction * _PI);
				beamstrips[i * numBeamStripsPerSpark + j].m_TextureYOfs = (fp32)j;
				beamstrips[i * numBeamStripsPerSpark + j].m_Color = 0x00FFFFFF + ((int32)alpha << 24);
				beamstrips[i * numBeamStripsPerSpark + j].m_Flags = (j == 0) ? CXR_BEAMFLAGS_BEGINCHAIN : 0;
			}
		}

		if (CXR_Util::Render_BeamStrip2(m_pVBM, pVB, _WMat, _VMat, beamstrips, numBeamStrips))
			Render_Surface(m_iSurface, pVB, _pAnimState->m_AnimTime0);
	}

	virtual void View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0)
	{
	}

	virtual void View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CVec3Dfp32* _pVPortal, int _nVPortal,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0)
	{
	}

	virtual void View_SetCurrent(int _iView, CXR_SceneGraphInstance* _pSGI)
	{
	}

	virtual bool View_GetClip_Sphere(CVec3Dfp32 _v0, fp32 _Radius, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo)
	{
		MAUTOSTRIP(CXR_Model_SurfaceDebris_View_GetClip_Sphere, false);
		if(_pRenderInfo)
			*_pRenderInfo = m_RenderInfo;
		return true;
	}

	virtual bool View_GetClip_Box(CVec3Dfp32 _min, CVec3Dfp32 _max, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo)
	{
		MAUTOSTRIP(CXR_Model_SurfaceDebris_View_GetClip_Box, true);
		if(_pRenderInfo)
			*_pRenderInfo = m_RenderInfo;
		return true;
	}
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_SurfaceDebris, CXR_Model_Debris);
#endif

// -------------------------------------------------------------------
//  CXR_Model_Impact
// -------------------------------------------------------------------
class CXR_Model_Impact : public CXR_Model_Custom
{
	MRTC_DECLARE;

public:
	int m_iSurface[10];
	fp32 m_Size[10];
	fp32 m_RenderTime[10];
	fp32 m_TexScale[10];
	fp32 m_Dot;
	int m_nSurfaces;

	virtual void Create(const char* _pParam)
	{
		CXR_Model_Custom::Create(_pParam);
		
        m_nSurfaces = 0;
		CFStr St = _pParam;

		m_Dot = St.GetStrSep(",").Val_fp64();
		while(St != "")
		{
			if(St[0] == '#')
			{
				St = St.Copy(1, St.Len());
				m_RenderTime[m_nSurfaces] = St.GetStrSep(",").Val_fp64();
				m_TexScale[m_nSurfaces] = St.GetStrSep(",").Val_fp64();
			}
			else
			{
				m_RenderTime[m_nSurfaces] = 0;
				m_TexScale[m_nSurfaces] = 1.5f;
			}

			//St.GetStrSep(",")
			m_iSurface[m_nSurfaces] = GetSurfaceID(St.GetStrSep(","));
			m_Size[m_nSurfaces] = St.GetStrSep(",").Val_fp64();
			m_nSurfaces++;
		}
	}

	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
	{
		MSCOPESHORT(CXR_Model_Impact::Render);
		int iSurf = _pAnimState->m_Anim0 % m_nSurfaces;
		
		// If we have render time extensions, check it against incoming time.
		if(m_RenderTime[iSurf] != 0 && _pAnimState->m_AnimTime0.GetTime() > m_RenderTime[iSurf])
			return;

		CVec3Dfp32 Edge[2];
		Edge[0] = 0;
		Edge[1] = CVec3Dfp32(0, m_Size[iSurf], m_Size[iSurf] * m_Dot);
		CPixel32 Colors[2];
		Colors[0] = -1;
		Colors[1] = -1;
		CMat4Dfp32 WMat2, Mat, L2V;
		Mat.SetYRotation(0.25f);
		Mat.Multiply(_WMat, WMat2);
		WMat2.Multiply(_VMat, L2V);
		CXR_VertexBuffer *pVB = CXR_Util::Create_SOR(_pRenderParams->m_pVBM, L2V, Edge, 2, 8, fp32(_pAnimState->m_Anim0) / 32767.0f, 1, m_Size[iSurf] / m_TexScale[iSurf]);
		if(!pVB)
			return;
		Render_Surface(_pRenderParams, m_iSurface[iSurf], pVB, _pAnimState->m_AnimTime0);
	}
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Impact, CXR_Model_Custom);

class IModel_ParticleTrails
{
public:
	IModel_ParticleTrails()
	{
		m_nParticles1 = 0;
		m_nParticles2 = 0;
	}

	CXR_Particle3 *m_lParticles1;
	CXR_Particle3 *m_lParticles2;
	int m_nParticles1;
	int m_nParticles2;

	fp32 m_TransitionStart;
	fp32 m_TransitionEnd;
	fp32 m_TransitionDuration;
	fp32 m_FadeOutStart;
	fp32 m_FadeOutEnd;
	fp32 m_FadeOutDuration;

	void AddParticleTrails(CVec3Dfp32 *_pPos, fp32 *_pSpeed, fp32 *_pAge, int _nParticles, int _iRand)
	{
		for(int i = 0; i < _nParticles; i++)
		{
			fp32 Age = _pAge[i];
			_iRand += 4;
			if(Age < m_FadeOutEnd)
			{
				int Alpha;
				CVec3Dfp32 Pos = _pPos[i];
				Pos += CVec3Dfp32(MFloat_GetRand(_iRand + 0) - 0.5f, MFloat_GetRand(_iRand + 1) - 0.5f, MFloat_GetRand(_iRand + 2) - 0.5f) * 4;
				Pos[2] += Age * MFloat_GetRand(_iRand + 3) * 32 * 2;
				if(Age < m_TransitionEnd)
				{
					if(Age > m_TransitionStart)
						Alpha = RoundToInt(255.0f * (Age - m_TransitionStart) / m_TransitionDuration);
					else
						Alpha = 255;

					Alpha = (int)(Alpha * Min(_pSpeed[i], 100.0f) / 100.0f);

					if(m_nParticles1 < 512)
					{
						m_lParticles1[m_nParticles1].m_Pos = Pos;
						m_lParticles1[m_nParticles1].m_Color = 0x808080 | (Alpha << 24);
						m_lParticles1[m_nParticles1].m_Angle = 0;
						m_lParticles1[m_nParticles1].m_Size = 24.0f;
						m_lParticles1[m_nParticles1].m_Time = Age;
						m_nParticles1++;
					}
				}
				if(Age > m_TransitionStart)
				{
					if(Age > m_FadeOutStart)
						Alpha = RoundToInt(255.0f * (Age - m_FadeOutStart) / m_FadeOutDuration);
					else if(Age > m_TransitionEnd)
						Alpha = 255;
					else
						Alpha = 255 - RoundToInt(255.0f * (Age - m_TransitionStart) / m_TransitionDuration);

					Alpha = (int)(Alpha * Min(_pSpeed[i], 100.0f) / 100.0f);

					if(m_nParticles2 < 512)
					{
						m_lParticles2[m_nParticles2].m_Pos = Pos;
						m_lParticles2[m_nParticles2].m_Color = 0x808080 | (Alpha << 24);
						m_lParticles2[m_nParticles2].m_Angle = 0;
						m_lParticles2[m_nParticles2].m_Size = 24.0f;
						m_lParticles2[m_nParticles2].m_Time = Age;
						m_nParticles2++;
					}
				}
			}
		}
	}

	void RenderParticleTrails(CXR_VBManager *_pVBM, CXR_VertexBuffer*_pVB1, CXR_VertexBuffer*_pVB2, const CMat4Dfp32 &_VMat, int _iTexture0, int _iTexture1)
	{
		if(m_nParticles1 > 0)
		{
			if(CXR_Util::Render_Particles(_pVBM, _pVB1, _VMat, m_lParticles1, m_nParticles1, 8, 8, 64, m_FadeOutEnd))
			{
				_pVB1->m_pAttrib->Attrib_TextureID(0, _iTexture0);
				_pVB1->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_CULL);
				_pVB1->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
				_pVBM->AddVB(_pVB1);
			}
		}
		if(m_nParticles2 > 0)
		{
			if(CXR_Util::Render_Particles(_pVBM, _pVB2, _VMat, m_lParticles2, m_nParticles2, 8, 8, 64, m_FadeOutEnd))
			{
				_pVB2->m_pAttrib->Attrib_TextureID(0, _iTexture1);
				_pVB2->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_CULL);
				_pVB2->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
				_pVBM->AddVB(_pVB2);
			}
		}
	}
};

// -------------------------------------------------------------------
//  CXR_Model_ExplosionDebris
// -------------------------------------------------------------------
class CXR_Model_ExplosionDebris : public CXR_Model_Debris, public IModel_ParticleTrails
{
	MRTC_DECLARE;

public:
	int m_iDefSurface;

	virtual void Create(const char* _pParam)
	{
		CXR_Model_Custom::Create(_pParam);

		m_Speed = 56;
		m_SamplePeriod = 0.05f;
		m_NumDebris = 12;
		m_Scatter = 25.0f;
		m_BoundRadius = 128;
		m_iDefSurface = 0;

		CFStr St = _pParam;
		if(St != "")
			m_iDefSurface = GetSurfaceID(St.GetStrSep(","));
		else
			m_iDefSurface = GetSurfaceID("EXPLOSION03");

		const fp32 TimeScale = 0.3f;
		m_Duration = 3.0f;
		m_TransitionStart = 0.2f * TimeScale;
		m_TransitionEnd = 0.6f * TimeScale;
		m_FadeOutStart = 1.2f * TimeScale;
		m_FadeOutEnd = 1.92f * TimeScale;

		m_TransitionDuration = m_TransitionEnd - m_TransitionStart;
		m_FadeOutDuration = m_FadeOutEnd - m_FadeOutStart;
	}

	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
	{
		CXR_Model_Debris::OnRender(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS, _pAnimState, _WMat, _VMat, _Flags);

/*		CDebrisTrails *pTrails = (CDebrisTrails *)_pAnimState->m_pModelInstance;
		if(!pTrails)
			return;
		for(int j = 0; j < pTrails->GetNumTrails(); j++)
		{
			if(pTrails->GetNumSamples(j) < 40)
			{
				CXR_Model *pModel = (CXR_Model *)_pAnimState->m_Colors[j & 3];
				if(pModel)
				{
					CVec3Dfp32 Angles;
					CVec3Dfp32 Pos = pTrails->GetTrailPos(j, _pAnimState->m_AnimTime0.GetTime(), &Angles);
					CMat4Dfp32 Mat, Res;
					Angles.CreateMatrixFromAngles(0, Mat);
					Pos.SetRow(Mat, 3);
					Mat.Multiply(_WMat, Res);
					pModel->OnRender(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS, _pAnimState, Res, _VMat, _Flags);
				}
			}
		}*/

	}

	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
	{
		MSCOPESHORT(CXR_Model_ExplosionDebris::Render);
		if(!_pAnimState->m_pModelInstance || _pAnimState->m_AnimTime0.Compare(m_Duration) > 0)
			return;

		CDebrisTrails *pTrails = (CDebrisTrails *)_pAnimState->m_pModelInstance;
		if(!pTrails)
			return;

		CMat4Dfp32 L2V;
		_WMat.Multiply(_VMat, L2V);

		const fp32 TimeStep = 0.013f;

		CXR_Particle3 lParticles1[512];
		CXR_Particle3 lParticles2[512];
		m_lParticles1 = lParticles1;
		m_lParticles2 = lParticles2;
		m_nParticles1 = 0;
		m_nParticles2 = 0;
		CXW_Surface *pSurf = GetSurfaceContext()->GetSurface(m_iDefSurface);
		CXW_SurfaceKeyFrame *pKey = pSurf ? pSurf->GetBaseFrame() : NULL;
		if(!pKey || pKey->m_lTextures.Len() != 4)
			return;

		fp32 MaxTime = Min(_pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/, m_Duration - m_FadeOutStart);
		for(int i = 0; i < pTrails->GetNumTrails(); i++)
		{
			CVec3Dfp32 Pos[128];
			fp32 Speed[128];
			fp32 Age[128];
			int nParticles = 0;

			float Time = MaxMT(_pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/ - m_FadeOutEnd, 0.0f);
			int iRand = i * 0x100 + 4 * TruncToInt(Time / TimeStep);
			while(Time < MaxTime)
			{
				Pos[nParticles] = pTrails->GetTrailPos(i, Time, NULL, &Speed[nParticles]) * _WMat;
				Age[nParticles] = _pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/ - Time;
				nParticles++;
				Time += TimeStep;
			}
			AddParticleTrails(Pos, Speed, Age, nParticles, iRand);
		}

		CXR_VertexBuffer *pVB1 = AllocVB(_pRenderParams);
		CXR_VertexBuffer *pVB2 = AllocVB(_pRenderParams);
		RenderParticleTrails(_pRenderParams->m_pVBM, pVB1, pVB2, _VMat, pKey->m_lTextures[1].m_TextureID, pKey->m_lTextures[3].m_TextureID);
	}
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_ExplosionDebris, CXR_Model_Debris);

#if !defined(M_DISABLE_TODELETE) || 1

// -------------------------------------------------------------------
//  CXR_Model_RocketTrail
// -------------------------------------------------------------------
class CXR_Model_RocketTrail : public CXR_Model_Custom, public IModel_ParticleTrails
{
	MRTC_DECLARE;

public:
	int m_iDefSurface;

	TPtr<CXR_ModelInstance> CreateModelInstance()
	{
		return MNew(CModelHistory2);
	}

	virtual void Create(const char* _pParam)
	{
		CXR_Model_Custom::Create(_pParam);

		CFStr St = _pParam;
		if(St != "")
			m_iDefSurface = GetSurfaceID(St.GetStrSep(","));
		else
			m_iDefSurface = GetSurfaceID("EXPLOSION03");

		const fp32 TimeScale = 0.3f;
		m_TransitionStart = 0.2f * TimeScale;
		m_TransitionEnd = 0.6f * TimeScale;
		m_FadeOutStart = 1.2f * TimeScale;
		m_FadeOutEnd = 1.92f * TimeScale;

		m_TransitionDuration = m_TransitionEnd - m_TransitionStart;
		m_FadeOutDuration = m_FadeOutEnd - m_FadeOutStart;
	}

	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
	{
		MSCOPESHORT(CXR_Model_RocketTrail::Render);
		if(!_pAnimState->m_pModelInstance)
			return;

		CXW_Surface *pSurf = GetSurfaceContext()->GetSurface(m_iDefSurface);
		CXW_SurfaceKeyFrame *pKey = pSurf ? pSurf->GetBaseFrame() : NULL;
		if(!pKey || pKey->m_lTextures.Len() != 4)
			return;

		CModelHistory2 *pHistory = safe_cast<CModelHistory2 >((CXR_ModelInstance *)_pAnimState->m_pModelInstance);
		pHistory->AddEntry(_WMat, _pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/, 0);

		const fp32 TimeStep = 0.013f;
		fp32 StartTime = MaxMT(_pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/ - m_FadeOutEnd, 0.0f);
		int nSteps = TruncToInt(StartTime / TimeStep);
		//nSteps = 0;
		fp32 Time = nSteps * TimeStep;

		CXR_Particle3 lParticles1[512];
		CXR_Particle3 lParticles2[512];
		m_lParticles1 = lParticles1;
		m_lParticles2 = lParticles2;
		m_nParticles1 = 0;
		m_nParticles2 = 0;

		CVec3Dfp32 Pos[128];
		fp32 Speed[128];
		fp32 Age[128];
		int nParticles = 0;
		int iRand = nSteps * 4;
		for(;Time < _pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/; Time += TimeStep)
		{
			CVec3Dfp32 RetSpeed;
			if(pHistory->GetInterpolatedPos(Time, Pos[nParticles], RetSpeed))
			{
				Speed[nParticles] = RetSpeed.LengthSqr();
				Age[nParticles] = _pAnimState->m_AnimTime0.GetTime() /*CMTIMEFIX*/ - Time;
				nParticles++;
				if(nParticles == 128)
					break;
			}
		}

		AddParticleTrails(Pos, Speed, Age, nParticles, iRand);
		CXR_VertexBuffer *pVB1 = AllocVB(_pRenderParams);
		CXR_VertexBuffer *pVB2 = AllocVB(_pRenderParams);
		RenderParticleTrails(_pRenderParams->m_pVBM, pVB1, pVB2, _VMat, pKey->m_lTextures[1].m_TextureID, pKey->m_lTextures[3].m_TextureID);
	}
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_RocketTrail, CXR_Model_Custom);
#endif
