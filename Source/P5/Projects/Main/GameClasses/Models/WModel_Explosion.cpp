#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "CEnvelope.h"
#include "CDynamicVB.h"
#include "CSurfaceKey.h"

//----------------------------------------------------------------------

//#define MAXNUMPARTICLES					(400)
#define MAXNUMPARTICLES					(150)

#define CENTER_OFFSET					(0.15f)

#define QUICKSTART_TIME					(0.5f)

#define CLOUD_DURATION					(1.0f)
#define CLOUD_RADIUS					(0.8f)
#define CLOUD_SIZE						(0.3f)
#define CLOUD_ROTATION					(0.3f)
#define CLOUD_ALPHA						(0.8f)
#define CLOUD_TIMESHIFT					(0.25f)
//#define NUM_CLOUD_PARTICLES				(50)
#define NUM_CLOUD_PARTICLES				(40)

#define DEBRIS_DURATION					(0.3f)
#define DEBRIS_RADIUS_START				(0.4f)
#define DEBRIS_RADIUS					(3.0f)
#define DEBRIS_GRAVITY					(2.0f)
#define DEBRIS_SIZE_MIN					(0.02f)
#define DEBRIS_SIZE_MAX					(0.08f)
#define DEBRIS_ROTATION					(1.0f)
#define DEBRIS_ALPHA					(0.8f)
#define DEBRIS_FADETIME					(0.2f)
#define DEBRIS_SHARPNESS				(0.8f)
//#define NUM_DEBRIS_SAMPLES				(40)
#define NUM_DEBRIS_SAMPLES				(10)

#define MAX_NUM_DEBRISTRAILS			(10)
#define MAX_NUM_DEBRISTRAIL_KEYS		(10)
#define DEBRIS_KEYDURATION				(0.2f)

#define RING_RADIUS						(1.5f)
#define RING_SIZE						(0.1f)

#define FLASH_DURATION					(0.08f)
#define FLASH_RADIUS					(1.5f)
#define FLASH_ALPHA						(0.7f)

#define NUM_X_FRAMES					(4) // Number of frames that fit texture in x direction (counting empty slots).
#define NUM_Y_FRAMES					(8) // Number of frames that fit texture in y direction (counting empty slots).
#define NUM_FRAMES						(32) // Total number of frames in texture (only continuously used frame slots).
#define FRAME_X_SIZE					(1.0f / (fp32)NUM_X_FRAMES)
#define FRAME_Y_SIZE					(1.0f / (fp32)NUM_Y_FRAMES)

#define FIRE_ALPHA						(1.0f)
#define SMOKE_ALPHA						(1.0f)

#define FIRE_FADEOUT_START_ANIMTIME		(0.2f)
#define FIRE_FADEOUT_END_ANIMTIME		(0.4f)
#define SMOKE_FADEIN_START_ANIMTIME		(0.3f)
#define SMOKE_FADEIN_END_ANIMTIME		(0.5f)

//----------------------------------------------------------------------
// CXR_Model_Explosion
//----------------------------------------------------------------------

class CXR_Model_Explosion : public CXR_Model_Custom
{

	MRTC_DECLARE;

	//----------------------------------------------------------------------
	
private:

	int m_iSurface_Fire;
	int m_iSurface_Smoke;
	int m_iSurface_Flash;

public:

	//----------------------------------------------------------------------
	
	fp32
	GetRand()
	{
		MAUTOSTRIP(CXR_Model_Explosion_GetRand, 0);
		return MFloat_GetRand(m_Randseed++);
	}

	//----------------------------------------------------------------------
	
	void
	AnglesToVector(fp32 a, fp32 b, CVec3Dfp32& v)
	{
		MAUTOSTRIP(CXR_Model_Explosion_AnglesToVector, MAUTOSTRIP_VOID);
		a *= 2.0f * _PI;
		b *= 2.0f * _PI;
		v.k[0] = M_Cos(a) * M_Cos(b);
		v.k[1] = M_Sin(a) * M_Cos(b);
		v.k[2] = M_Sin(b);
	}

	//----------------------------------------------------------------------

private:
	//----------------------------------------------------------------------

	class CExplosionDebris: public CReferenceCount
	{
		private:

			class CTrailKey
			{
				public:

					CVec3Dfp32	m_Pos;
					fp32			m_Time;
			};

			class CTrail
			{
				public:

					CTrailKey		m_Key[MAX_NUM_DEBRISTRAIL_KEYS];
					int				m_NumKeys;
			};

			CTrail				m_Trail[MAX_NUM_DEBRISTRAILS];
			int					m_NumTrails;

		public:

			CExplosionDebris()
			{
				m_NumTrails = 0;
			}

			void
			Create(CXR_Model_Explosion* _pExplosionModel, CVec3Dfp32 m_Center)
			{
				MAUTOSTRIP(CXR_Model_Explosion_CExplosionDebris_Create, MAUTOSTRIP_VOID);
				m_NumTrails = (int)((0.8f + 0.2f * _pExplosionModel->GetRand()) * (fp32)MAX_NUM_DEBRISTRAILS);

				fp32 duration = _pExplosionModel->m_Duration * DEBRIS_DURATION;
				fp32 speed = _pExplosionModel->m_Radius * (DEBRIS_RADIUS - DEBRIS_RADIUS_START) / duration;
				fp32 gravity = _pExplosionModel->m_Radius * DEBRIS_GRAVITY / duration;
				
				for (int iTrail = 0; iTrail < m_NumTrails; iTrail++)
				{
					CVec3Dfp32 dir;
					_pExplosionModel->AnglesToVector(_pExplosionModel->GetRand(), _pExplosionModel->GetRand() * 0.5f, dir);
//					dir.k[0] *= dir.k[2];
//					dir.k[1] *= dir.k[2];

					m_Trail[iTrail].m_Key[0].m_Pos = m_Center + dir * _pExplosionModel->m_Radius * DEBRIS_RADIUS_START;
					m_Trail[iTrail].m_Key[0].m_Time = 0.0f;

					dir *= speed;

					int iKey;
					for (iKey = 1; iKey < MAX_NUM_DEBRISTRAIL_KEYS; iKey++)
					{
						fp32 dt = duration / MAX_NUM_DEBRISTRAIL_KEYS;

						m_Trail[iTrail].m_Key[iKey].m_Pos = m_Trail[iTrail].m_Key[iKey - 1].m_Pos + dir * dt;
						m_Trail[iTrail].m_Key[iKey].m_Time = m_Trail[iTrail].m_Key[iKey - 1].m_Time + dt;

						dir.k[2] -= gravity * dt;
					}

					m_Trail[iTrail].m_NumKeys = iKey;
				}
			}

			int
			GetNumTrails()
			{
				MAUTOSTRIP(CXR_Model_Explosion_CExplosionDebris_GetNumTrails, 0);
				return m_NumTrails;
			}

			bool
			GetTrailPos(int _iTrail, fp32 _time, CVec3Dfp32& _pos)
			{
				MAUTOSTRIP(CXR_Model_Explosion_CExplosionDebris_GetTrailPos, false);
				if (_time < 0.0f)
					return false;

				if (_time > m_Trail[_iTrail].m_Key[m_Trail[_iTrail].m_NumKeys - 1].m_Time)
					return false;

				int iKey = 0;

				while (_time > m_Trail[_iTrail].m_Key[iKey + 1].m_Time)
					iKey++;

				fp32 timefraction = (_time - m_Trail[_iTrail].m_Key[iKey].m_Time) / (m_Trail[_iTrail].m_Key[iKey + 1].m_Time - m_Trail[_iTrail].m_Key[iKey].m_Time);

				_pos = m_Trail[_iTrail].m_Key[iKey].m_Pos + (m_Trail[_iTrail].m_Key[iKey + 1].m_Pos - m_Trail[_iTrail].m_Key[iKey].m_Pos) * timefraction;

				return true;
			}


		

	};
	
	//----------------------------------------------------------------------
	
public:

	fp32					m_Time;
	fp32					m_Duration;
	fp32					m_TimeFraction;

	fp32					m_Radius;

private:

	fp32					m_CloudRadius, m_CloudRadiusNoise, m_CloudSize, m_CloudSizeNoise;
	fp32					m_CloudAlpha, m_CloudAlphaNoise;
	fp32					m_DebrisAlpha;
	fp32					m_FlashRadius, m_FlashAlpha;
	fp32					m_RingRadius, m_RingRadiusNoise, m_RingSize, m_RingSizeNoise;

	CVec3Dfp32			m_Center;
	CVec3Dfp32			m_Front, m_Left, m_Up;
	fp32					m_Distance;

	int32				m_RandseedBase, m_Randseed;
	int32				m_ColorRGB_Fire, m_ColorAlpha_Fire;
	int32				m_ColorRGB_Smoke, m_ColorAlpha_Smoke;

	CDynamicVB			m_DVBFire, m_DVBSmoke;
	CXR_VertexBuffer*	m_pVBFlash;
	CSurfaceKey			m_SKFire, m_SKSmoke, m_SKFlash;

	CExplosionDebris*	m_DebrisTrails;

	//----------------------------------------------------------------------

private:

	void
	CalculateEnvelopes()
	{
		MAUTOSTRIP(CXR_Model_Explosion_CalculateEnvelopes, MAUTOSTRIP_VOID);

		CEnvelope envelope;

		#define Envelope(value, duration, method)											\
		{																					\
			envelope.Create(EnvKeys, sizeof(EnvKeys) / sizeof(EnvKey), CEnvelope::method);	\
			value = envelope.getValue(m_Time / (m_Duration * duration));					\
		}																					\

		typedef CEnvelope::Key	EnvKey;

		{
			EnvKey EnvKeys[] = {
				EnvKey(0.00f, 0.00f, 0, 0, 0), 
				EnvKey(0.05f, 0.40f, 0, 0, 0), 
				EnvKey(0.10f, 0.70f, 0, 0, 0), 
				EnvKey(0.20f, 1.00f, 0, 0, 0), 
				EnvKey(0.50f, 1.50f, 0, 0, 0),
				EnvKey(1.00f, 2.00f, 0, 0, 0)
//				EnvKey(0.00f, 1.00f, 0, 0, 0)
			};
			Envelope(m_CloudRadius, CLOUD_DURATION, IPMethod_TCBSpline);
//			envelope.Render(m_pWC, 100, 0xFF00FF00, 0xFF404040, 0.1f);
		}
		{
			EnvKey EnvKeys[] = {
				EnvKey(0.00f, 0.20f, 0, 0, 0),
				EnvKey(1.00f, 0.50f, 0, 0, 0)
			};
			Envelope(m_CloudRadiusNoise, CLOUD_DURATION, IPMethod_Linear);
		}
		{
			EnvKey EnvKeys[] = {
				EnvKey(0.00f, 0.00f, 0, 0, 0), 
				EnvKey(0.05f, 0.40f, 0, 0, 0), 
				EnvKey(0.10f, 0.70f, 0, 0, 0), 
				EnvKey(0.20f, 0.95f, 0, 0, 0), 
				EnvKey(0.50f, 1.20f, 0, 0, 0),
				EnvKey(1.00f, 1.50f, 0, 0, 0)
//				EnvKey(0.00f, 1.00f, 0, 0, 0)
			};
			Envelope(m_CloudSize, CLOUD_DURATION, IPMethod_TCBSpline);
		}
		{
			EnvKey EnvKeys[] = {
				EnvKey(0.00f, 0.30f, 0, 0, 0), 
				EnvKey(1.00f, 0.40f, 0, 0, 0)
			};
			Envelope(m_CloudSizeNoise, CLOUD_DURATION, IPMethod_Linear);
		}
		{
			EnvKey EnvKeys[] = {
				EnvKey(0.00f, 1.00f, 0, 0, 0), 
				EnvKey(0.40f, 0.60f, 0, 0, 0),
				EnvKey(0.60f, 0.25f, 0, 0, 0),
				EnvKey(0.80f, 0.10f, 0, 0, 0),
				EnvKey(1.00f, 0.00f, 0, 0, 0)
//				EnvKey(0.00f, 1.00f, 0, 0, 0)
			};
			Envelope(m_CloudAlpha, CLOUD_DURATION, IPMethod_Linear);
		}
		{
			EnvKey EnvKeys[] = {
				EnvKey(0.80f, 0.30f, 0, 0, 0), 
				EnvKey(1.00f, 0.00f, 0, 0, 0)
			};
			Envelope(m_CloudAlphaNoise, CLOUD_DURATION, IPMethod_Linear);
		}
		{
			EnvKey EnvKeys[] = {
				EnvKey(0.00f, 1.00f, 0, 0, 0), 
				EnvKey(0.60f, 0.40f, 0, 0, 0), 
				EnvKey(1.00f, 0.00f, 0, 0, 0), 
			};
			Envelope(m_DebrisAlpha, DEBRIS_DURATION, IPMethod_Linear);
		}
		{
			EnvKey EnvKeys[] = {
				EnvKey(0.00f, 0.00f, 0, 0, 0), 
				EnvKey(0.50f, 1.00f, 0, 0, 0), 
				EnvKey(1.00f, 0.00f, 0, 0, 0), 
			};
			Envelope(m_FlashRadius, FLASH_DURATION, IPMethod_Linear);
		}
		{
			EnvKey EnvKeys[] = {
				EnvKey(0.00f, 1.00f, 0, 0, 0), 
				EnvKey(0.50f, 0.80f, 0, 0, 0), 
				EnvKey(1.00f, 0.00f, 0, 0, 0), 
			};
			Envelope(m_FlashAlpha, FLASH_DURATION, IPMethod_Linear);
		}
	}
	
	//----------------------------------------------------------------------

	void
	RenderParticle(CXR_Particle2& _Particle, fp32 AnimTimeFraction)
	{
		MAUTOSTRIP(CXR_Model_Explosion_RenderParticle, MAUTOSTRIP_VOID);
		CVec3Dfp32 pos = _Particle.m_Pos;

		int frame = (int)(AnimTimeFraction * (fp32)(NUM_FRAMES - 1));

		fp32 tu = (fp32)(frame % NUM_X_FRAMES) * FRAME_X_SIZE;
		fp32 tv = (fp32)(frame / NUM_X_FRAMES) * FRAME_Y_SIZE;

		CVec3Dfp32 w = m_Left * M_Cos(_Particle.m_Angle) - m_Up * M_Sin(_Particle.m_Angle);
		CVec3Dfp32 h = m_Left * M_Sin(_Particle.m_Angle) + m_Up * M_Cos(_Particle.m_Angle);

		w *= _Particle.m_Size * 0.5f;
		h *= _Particle.m_Size * 0.5f;

		fp32 alpha = (fp32)((_Particle.m_Color >> 24) & 0xFF) / 255.0f;

		if (AnimTimeFraction < FIRE_FADEOUT_END_ANIMTIME)
		{
			fp32 firealpha = (fp32)m_SKFire.m_ColorAlpha * alpha * FIRE_ALPHA;
			int32 firecolor;
			if (AnimTimeFraction > FIRE_FADEOUT_START_ANIMTIME)
			{
				fp32 fade = 1.0f - (AnimTimeFraction - FIRE_FADEOUT_START_ANIMTIME) / (FIRE_FADEOUT_END_ANIMTIME - FIRE_FADEOUT_START_ANIMTIME);
				firecolor = m_SKFire.m_ColorRGB | ((int32)(firealpha * fade) << 24);
			}
			else
			{
				firecolor = m_SKFire.m_ColorRGB | ((int32)(firealpha) << 24);
			}

			m_DVBFire.AddTriangle(0, 1, 2);
			m_DVBFire.AddTriangle(0, 2, 3);
			m_DVBFire.AddVertex(pos - w - h, tu, tv, firecolor);
			m_DVBFire.AddVertex(pos + w - h, tu + FRAME_X_SIZE, tv, firecolor);
			m_DVBFire.AddVertex(pos + w + h, tu + FRAME_X_SIZE, tv + FRAME_Y_SIZE, firecolor);
			m_DVBFire.AddVertex(pos - w + h, tu, tv + FRAME_Y_SIZE, firecolor);
		}

		if (AnimTimeFraction > SMOKE_FADEIN_START_ANIMTIME)
		{
			fp32 smokealpha = (fp32)m_SKSmoke.m_ColorAlpha * alpha;
			int32 smokecolor;
			if (AnimTimeFraction < SMOKE_FADEIN_END_ANIMTIME)
			{
				fp32 fade = (AnimTimeFraction - SMOKE_FADEIN_START_ANIMTIME) / (SMOKE_FADEIN_END_ANIMTIME - SMOKE_FADEIN_START_ANIMTIME);
				smokecolor = m_SKSmoke.m_ColorRGB | ((int32)(smokealpha * fade) << 24);
			}
			else
			{
				smokecolor = m_SKSmoke.m_ColorRGB | ((int32)(smokealpha) << 24);
			}

			m_DVBSmoke.AddTriangle(0, 1, 2);
			m_DVBSmoke.AddTriangle(0, 2, 3);
			m_DVBSmoke.AddVertex(pos - w - h, tu, tv, smokecolor);
			m_DVBSmoke.AddVertex(pos + w - h, tu + FRAME_X_SIZE, tv, smokecolor);
			m_DVBSmoke.AddVertex(pos + w + h, tu + FRAME_X_SIZE, tv + FRAME_Y_SIZE, smokecolor);
			m_DVBSmoke.AddVertex(pos - w + h, tu, tv + FRAME_Y_SIZE, smokecolor);
		}
	}
	
	//----------------------------------------------------------------------
	
	void
	GenerateCloudParticle()
	{
		MAUTOSTRIP(CXR_Model_Explosion_GenerateCloudParticle, MAUTOSTRIP_VOID);
		CXR_Particle2 particle;

		fp32 tf = m_Time / (m_Duration * CLOUD_DURATION);

//		if ((tf < 0.0f) || (tf > 1.0f))
//			return;

		CVec3Dfp32 dir;
		AnglesToVector(GetRand(), GetRand() * 0.5f, dir);
//		dir.k[0] *= dir.k[2];
//		dir.k[1] *= dir.k[2];

		fp32 alpha = CLOUD_ALPHA * (m_CloudAlpha - m_CloudAlphaNoise * GetRand());
		if (alpha < 0.0f)
			alpha = 0.0f;
		if (alpha > 1.0f)
			alpha = 1.0f;
		particle.m_Color = ((int32)(alpha * 255.0f) & 0xFF) << 24;

		fp32 radius = m_Radius * (CLOUD_RADIUS - 1.0f * CLOUD_SIZE) * (m_CloudRadius + m_CloudRadiusNoise * 2.0f * (GetRand() - 0.5f));
		if (radius < 0.0f)
			radius = 0.0f;
		particle.m_Pos = m_Center + dir * radius;

		particle.m_Size = 2.0f * m_Radius * CLOUD_SIZE * (m_CloudSize + m_CloudSizeNoise * 2.0f * (GetRand() - 0.5f));
		if (particle.m_Size < 0.0f) 
			particle.m_Size = 0.0f;

		fp32 pi2 = 2.0f * _PI;

		particle.m_Angle = (GetRand() + tf * CLOUD_ROTATION * (GetRand() - 0.5f)) * pi2;

		tf += CLOUD_TIMESHIFT * 2.0f * (GetRand() - 0.5f);
		if (tf < 0.0f)
			tf = 0.0f;
		if (tf > 1.0f)
			tf = 1.0f;

		tf = InvSqrInv(tf);
		
		RenderParticle(particle, tf);
	}

	//----------------------------------------------------------------------
	
	void GenerateCloud()
	{
		MAUTOSTRIP(CXR_Model_Explosion_GenerateCloud, MAUTOSTRIP_VOID);
		for (int32 i = 0; i < NUM_CLOUD_PARTICLES; i++) {
			m_Randseed = m_RandseedBase + i;
			GenerateCloudParticle();
		}
	}

	//----------------------------------------------------------------------

	void GenerateDebris()
	{
		MAUTOSTRIP(CXR_Model_Explosion_GenerateDebris, MAUTOSTRIP_VOID);
		fp32 tf = m_Time / (m_Duration * DEBRIS_DURATION);

		CXR_Particle2 particle;

		for (int iSample = 0; iSample < NUM_DEBRIS_SAMPLES; iSample++)
		{
			fp32 angle = GetRand();
			fp32 rotation = 2.0f * (GetRand() - 0.5f);
			for (int iTrail = 0; iTrail < m_DebrisTrails->GetNumTrails(); iTrail++)
			{
				fp32 trailfraction = ((fp32)iSample / (fp32)NUM_DEBRIS_SAMPLES);
				fp32 t = m_Time - trailfraction * DEBRIS_FADETIME;

				if (m_DebrisTrails->GetTrailPos(iTrail, t, particle.m_Pos))
				{
					fp32 alpha =  DEBRIS_ALPHA * m_DebrisAlpha * Sqr(1.0f - trailfraction);
					if (alpha < 0.0f)
						alpha = 0.0f;
					if (alpha > 1.0f)
						alpha = 1.0f;

					particle.m_Size = 2.0f * m_Radius * (DEBRIS_SIZE_MIN + GetRand() * (DEBRIS_SIZE_MAX - DEBRIS_SIZE_MIN)) * ((1.0f - DEBRIS_SHARPNESS) + DEBRIS_SHARPNESS * trailfraction);
					particle.m_Angle = (angle + t * rotation * DEBRIS_ROTATION) * (2.0f * _PI);
					particle.m_Color = 0x00FFFFFF | (int32)(alpha * 255.0f) << 24;

					//trailfraction = InvSqrInv(trailfraction);
					//trailfraction = InvSqrInv(trailfraction);
					RenderParticle(particle, trailfraction);
				}
			}
		}
	}

	//----------------------------------------------------------------------

	void
	GenerateFlash(const CMat43fp32& _mat)
	{
		MAUTOSTRIP(CXR_Model_Explosion_GenerateFlash, MAUTOSTRIP_VOID);
		CXR_Particle2 particle;

		particle.m_Pos = m_Center;
		particle.m_Color = 0x00FFFFFF | ((int32)(m_FlashAlpha * FLASH_ALPHA * 255.0f) << 24);
		particle.m_Size = 2.0f * m_Radius * m_FlashRadius * FLASH_RADIUS;
		particle.m_Angle = 0.0f;

		m_pVBFlash->m_Priority = CXR_VBPRIORITY_FLARE;

		CXR_Util::Render_Flares(m_pRC, m_pVBM, m_pVBFlash, _mat, &particle, 1, 0.0f, 0.0f, 3, true);
	}
	
	//----------------------------------------------------------------------
	
	bool
	Init(const CXR_AnimState* _pAnimState, const CMat43fp32& _LocalToWorld, const CMat43fp32& _WorldToCamera, const CMat43fp32& _LocalToCamera)
	{
		MAUTOSTRIP(CXR_Model_Explosion_Init, false);
		m_Time = _pAnimState->m_AnimTime0;
		m_Duration = _pAnimState->m_AnimTime1;

		m_TimeFraction = m_Time / m_Duration;

		m_TimeFraction = QUICKSTART_TIME * InvSqrInv(m_TimeFraction) + (1.0f - QUICKSTART_TIME) * m_TimeFraction;
		m_Time = m_TimeFraction * m_Duration;

		m_Radius = 0.5f * _pAnimState->m_Anim1;
		m_RandseedBase = _pAnimState->m_Anim0;
		m_Randseed = m_RandseedBase;
		m_BoundRadius = m_Radius / 1.1f;

		m_Center = CVec3Dfp32::GetMatrixRow(_LocalToWorld, 3);
		m_Center.k[2] += m_Radius * CENTER_OFFSET;

		CMat43fp32 InvWorldToCamera;
		_WorldToCamera.InverseOrthogonal(InvWorldToCamera);

		m_Left = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 0);
		m_Up = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 1);
		m_Front = -CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 2);
		m_Distance = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 3).k[0];

		CalculateEnvelopes();

		if (!m_DVBFire.Create(this, m_pVBM, MAXNUMPARTICLES * 4, MAXNUMPARTICLES * 2))
			return false;

		if (!m_DVBSmoke.Create(this, m_pVBM, MAXNUMPARTICLES * 4, MAXNUMPARTICLES * 2))
			return false;

		m_pVBFlash = AllocVB();
		if (m_pVBFlash == NULL)
			return false;

		m_SKFire.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface_Fire);
		m_SKSmoke.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface_Smoke);
		m_SKFlash.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface_Flash);

		return true;
	}

	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_Explosion_Render, MAUTOSTRIP_VOID);
		CMat43fp32 LocalToCamera;
		_WMat.Multiply(_VMat, LocalToCamera);

		if (!Init(_pAnimState, _WMat, _VMat, LocalToCamera))
			return;

		// Temporary storage fix of CExplosionDebris. Should later be put in clientdata.
		CExplosionDebris	debristrails;
		debristrails.Create(this, m_Center);
		m_DebrisTrails = &debristrails;

		GenerateCloud();
		GenerateDebris();
		GenerateFlash(_VMat);

		m_DVBFire.Render(_VMat);
		m_DVBSmoke.Render(_VMat);

		m_SKFire.Render(m_DVBFire.GetVB(), m_pVBM, m_pEngine);
		m_SKSmoke.Render(m_DVBSmoke.GetVB(), m_pVBM, m_pEngine);
		m_SKFlash.Render(m_pVBFlash, m_pVBM, m_pEngine);
	}

	//----------------------------------------------------------------------
	
	virtual
	void
	OnCreate(const char *)
	{
		MAUTOSTRIP(CXR_Model_Explosion_OnCreate, MAUTOSTRIP_VOID);
		m_iSurface_Fire = GetSurfaceID("ExplosionFire");
		m_iSurface_Smoke = GetSurfaceID("ExplosionSmoke");
		m_iSurface_Flash = GetSurfaceID("ExplosionFlash");
	}

};

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Explosion, CXR_Model_Custom);

//----------------------------------------------------------------------
	
/*
m_pEngine->m_pRender->Attrib_GlobalGetVar(CRC_GLOBALVAR_NUMTEXTURES)
CPixel32 GetCurrentFog(const CMat4Dfp32 &_WMat);
GetFogFade(_WMat);
// Sist...
m_pEngine->GetFogState()->SetDepthFog(pVB->m_pAttrib);
*/
/*	
fp32 alpha = 1.0f;

pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ADD);
pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
pVB->m_pAttrib->Attrib_TexEnvMode(0, CRC_TEXENVMODE_MULTIPLY);
pVB->m_pAttrib->Attrib_TexEnvMode(1, CRC_TEXENVMODE_MULTIPLY);
pVB->m_pAttrib->Attrib_TexEnvColor(0, 0x00FFFFFF | ((int32)alpha << 24));
pVB->m_pAttrib->Attrib_TexEnvColor(1, 0x00FFFFFF | ((int32)(1.0f - alpha) << 24));
*/

