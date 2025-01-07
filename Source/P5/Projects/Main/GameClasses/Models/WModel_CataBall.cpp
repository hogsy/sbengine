#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "CDynamicVB.h"
#include "CSurfaceKey.h"
#include "CModelHistory.h"
#include "CSinRandTable.h"
/*
//----------------------------------------------------------------------

#define MAXNUMPARTICLES					(100)

#define PARTICLE_DURATION				(2.0f) // Duration of one flame particle (in seconds).
#define PARTICLE_FADEINTIME				(0.1f) // 
#define PARTICLE_RAISE					(70.0f) // Height a particle will raise during it's full duration (in units).

#define PARTICLE_SIZE_START				(20.0f) // Size of particle when it spawns (in units).
#define PARTICLE_SIZE_END				(5.0f) // Size of particle when it dies (in units).
#define PARTICLE_SIZE_NOISE				(3.0f) // 

#define PARTICLE_POSFLUCT_START			(5.0f) // Size of particle position fluctuation (in units).
#define PARTICLE_POSFLUCT_END			(10.0f) // Size of particle position fluctuation (in units).
#define PARTICLE_POSFLUCT_TIMESCALE		(0.2f) // Timescale of position fluctuation SinRand

#define	PARTICLE_ROTFLUCT_START			(0.1f) // How many full turns a particle rotates during it's full duration.
#define	PARTICLE_ROTFLUCT_END			(0.6f) // How many full turns a particle rotates during it's full duration.
#define	PARTICLE_ROTFLUCT_TIMESCALE		(0.3f) // Timescale of position fluctuation SinRand

#define PARTICLE_ALPHA_START			(0.5f) // Transparency of particle when it spawns.
#define PARTICLE_ALPHA_END				(0.0f) // Transparency of particle when it dies.
#define PARTICLE_ALPHA_NOISE			(0.0f) // 
#define PARTICLE_ALPHA_QUICKFADE		(0.5f)
#define PARTICLE_COLOR					(0x808080)

#define PARTICLE_EMISSION_PROBABILITY	(0.8f) // Probability of emitting a particle during a time cell.
#define PARTICLE_TIMECELL_DURATION		(0.2f) // Minimum delay between two particle emissions (in seconds).
#define PARTICLES_PER_TIMECELL			(1) // Minimum delay between two particle emissions (in seconds).

#define PARTICLE_ANIMATION_LOOPS		(0.5f) // Number of loops played during a full particle duration.

#define PARTICLE_WIDTH_SHARPNESS		(0.6f)
#define PARTICLE_WIDTH_SCALE			(1.0f)
#define PARTICLE_HEIGHT_SCALE			(3.0f)
#define PARTICLE_LENGTH_SCALE			(0.4f)
#define PARTICLE_LENGTH_FADEINTIME		(0.6f)
#define PARTICLE_LENGTH_FADEINTIME_SLOW	(1.0f)
#define PARTICLE_SLOWSPEED_SCALE		(1.5f)

#define SINRAND_TIMEOFFSET				(0.8f)

#define SLOWSTART_FACTOR				(1.0)

#define NUM_X_FRAMES					(8) // Number of frames that fit texture in x direction (counting empty slots).
#define NUM_Y_FRAMES					(4) // Number of frames that fit texture in y direction (counting empty slots).
#define NUM_FRAMES						(32) // Total number of frames in texture (only continuously used frame slots).
#define FRAME_X_SIZE					(1.0f / (fp32)NUM_X_FRAMES)
#define FRAME_Y_SIZE					(1.0f / (fp32)NUM_Y_FRAMES)

#define FIRE_ALPHA						(1.0f)
#define SMOKE_ALPHA						(1.0f)

#define FIRE_FADEOUT_START_ANIMTIME		(1.0f)
#define FIRE_FADEOUT_END_ANIMTIME		(1.001f)
#define SMOKE_FADEIN_START_ANIMTIME		(1.0f)
#define SMOKE_FADEIN_END_ANIMTIME		(1.001f)

#define HISTORY_LENGTH					(20)
#define HISTORY_ENTRYDELAY				(0.2f)

//----------------------------------------------------------------------

#define LERP(a,b,t) ((a) + ((b) - (a))*t)
#define FRAC(a) ((a) - floor(a))

#define getRand()		(MFloat_GetRand(m_Randseed++))
#define getSign(x)		(Sign(x))
#define getAbs(x)		(((x) < 0) ? -(x) : (x))

//----------------------------------------------------------------------

typedef CModelHistory<HISTORY_LENGTH> CMHistory;
typedef CSinRandTable<1024, 8> CSinRandTable1024x8;

//----------------------------------------------------------------------
// CXR_Model_CataBall
//----------------------------------------------------------------------

class CXR_Model_CataBall : public CXR_Model_Custom
{

	MRTC_DECLARE;

public:

	enum { SinRandTableSize = 1024 };

	enum { NumTrailSteps = HISTORY_LENGTH };

private:

	//----------------------------------------------------------------------

	fp32					m_Time; // Total lifetime of model.
	fp32					m_Distance; // Distance from camera to model origin.
	int32				m_Randseed, m_RandseedBase;

//	fp32					m_TrailDuration; // Total available duration of trail.

	fp32					m_SinRandTable[SinRandTableSize];
	fp32					m_SinRandTimeOffset;

	fp32					m_TrailDuration;
	CMat4Dfp32			m_LocalToWorld[NumTrailSteps];
	CVec3Dfp32			m_TrailPos[NumTrailSteps]; // Positions of the trails.
	CVec3Dfp32			m_TrailSide[NumTrailSteps]; // World space side vector of trail towards camera, uhm =).

	CVec3Dfp32			m_CameraFwd, m_CameraLeft, m_CameraUp;
	CVec3Dfp32			m_CameraPos;

	CXR_Particle2*		m_Particles;
	int32				m_NumParticles, m_MaxNumParticles;

	int					m_NumCellsToBacktrace;
	int32				m_NumTotalCells;

	CDynamicVB			m_DVBFire, m_DVBSmoke;
	CSurfaceKey			m_SKFire, m_SKSmoke;

	CXR_VertexBuffer*	m_pVBFlare;
	CSurfaceKey			m_SKFlare;

	int					m_iSurface_Fire, m_iSurface_Smoke, m_iSurface_Flare;

	CWireContainer*		m_pWC;

	//----------------------------------------------------------------------

	void
	createSinRandTable()
	{
		fp32 pi2 = 2.0f * _PI;
		fp32 x, y, dy;

		for (int i = 0; i < SinRandTableSize; i++) {
			x = (fp32)i / (fp32)SinRandTableSize;
			y = 0;

			dy = sin(3.0f * pi2 * (x + 0.0f));
			if (dy < -1.0f) dy = -1.0f;
			if (dy > +1.0f) dy = +1.0f;
			y += 0.5f * dy;

			dy = sin(7.0f * pi2 * (x + 0.3f));
			if (dy < -1.0f) dy = -1.0f;
			if (dy > +1.0f) dy = +1.0f;
			y += 0.25f * dy;

			dy = sin(13.0f * pi2 * (x + 0.7f));
			if (dy < -1.0f) dy = -1.0f;
			if (dy > +1.0f) dy = +1.0f;
			y += 0.125f * dy;

			m_SinRandTable[i] = 0.5f + 0.5f * (y / 0.875f);
		}
	}

	//----------------------------------------------------------------------

	fp32
	getSinRandTable(fp32 timescale, fp32 timeoffset = 0.0f)
	{
		fp32 y, y1, y2;
		int32 x, xi, xf;

		x = RoundToInt(1023.0f * 255.0f * (m_Time + SINRAND_TIMEOFFSET * getRand() + timeoffset) * timescale);

		xi = (x >> 8) & 0x3FF;
		xf = x & 0xFF;

		y1 = m_SinRandTable[xi];
		y2 = m_SinRandTable[(xi + 1) & 0x3FF];

		y = y1 + (y2 - y1) * (fp32)xf / 255.0f;

		return y;
	}

	//----------------------------------------------------------------------

	// Get history from clientdata.
	// If no clientdata present, create new one and set it.
	CMHistory*
	getHistory(const CXR_AnimState* pAnimState)
	{
		CMHistory* history;

		if(pAnimState->m_pspClientData == NULL)
			return NULL;

		history = (CMHistory*)(CReferenceCount*)(*pAnimState->m_pspClientData);

		if (history == NULL) {
			*pAnimState->m_pspClientData = DNew(CMHistory) CMHistory(HISTORY_ENTRYDELAY);
			if (*pAnimState->m_pspClientData == NULL) return NULL;
			history = (CMHistory*)(CReferenceCount*)(*pAnimState->m_pspClientData);
//			ConOutL(CStrF("Created History(%X), RandAnim0 = %d", history, pAnimState->m_Anim0));
		} else {
//			ConOutL(CStrF("Found History(%X), RandAnim0 = %d", history, pAnimState->m_Anim0));
		}

		return history;
	}

	//----------------------------------------------------------------------

	// Updates history and matrices.
	// Returns true for success, false for failure.
	bool
	updateHistory(const CXR_AnimState* pAnimState, const CMat4Dfp32& LocalToWorld)
	{
		CMHistory* history = getHistory(pAnimState);

		if (history == NULL)
			return false;

		// add current matrix to history.
		history->AddEntry(LocalToWorld, m_Time);

		// compute new interpolated matrices from history.
		m_TrailDuration = m_Time - history->GetOldestTime();
		if (m_TrailDuration > PARTICLE_DURATION)
			m_TrailDuration = PARTICLE_DURATION;

		fp32 time, fraction, speed;

		for (int i = 0; i < NumTrailSteps; i++)
		{
			fraction = (fp32)i / (fp32)(NumTrailSteps - 1);
			time = m_Time - m_TrailDuration * fraction;
			if (!history->GetInterpolatedMatrix(time, m_LocalToWorld[i], speed))
				return false;

			if (m_pWC && false)
				m_pWC->RenderMatrix(m_LocalToWorld[i], 1.0f);

			m_TrailPos[i] = CVec3Dfp32::GetMatrixRow(m_LocalToWorld[i], 3);
			m_TrailPos[i].k[2] += PARTICLE_RAISE * fraction;

			if (m_pWC)
				m_pWC->RenderVertex(m_TrailPos[i], 0x80008000, 0.001f);
		}

		return true;
	}

	//----------------------------------------------------------------------

	void
	addParticle(CXR_Particle2& particle)
	{
		if (m_NumParticles >= m_MaxNumParticles)
			return;

		m_Particles[m_NumParticles] = particle;
		m_NumParticles++;
	}

	//----------------------------------------------------------------------

	void
	RenderParticle(CXR_Particle2& _Particle, CVec3Dfp32 _Left, CVec3Dfp32 _Up, fp32 _Length, fp32 _LengthBlend, fp32 AnimTimeFraction)
	{
		CVec3Dfp32 pos = _Particle.m_Pos;

		AnimTimeFraction = M_FMod(AnimTimeFraction * PARTICLE_ANIMATION_LOOPS, 1.0f);

		int frame = (int)(AnimTimeFraction * (fp32)(NUM_FRAMES - 1));

		fp32 tu = (fp32)(frame % NUM_X_FRAMES) * FRAME_X_SIZE;
		fp32 tv = (fp32)(frame / NUM_X_FRAMES) * FRAME_Y_SIZE;

		CVec3Dfp32 ViewVector = _Particle.m_Pos - m_CameraPos;
		ViewVector.Normalize();

		fp32 ViewAngleScale;
		ViewAngleScale = fabs(_Up * ViewVector);

		ViewVector.CrossProd(_Left, _Up);

//		CVec3Dfp32 w = m_CameraLeft * cos(_Particle.m_Angle) - m_CameraUp * sin(_Particle.m_Angle);
//		CVec3Dfp32 h = m_CameraLeft * sin(_Particle.m_Angle) + m_CameraUp * cos(_Particle.m_Angle);
		CVec3Dfp32 w = _Left * cos(_Particle.m_Angle) - _Up * sin(_Particle.m_Angle);
		CVec3Dfp32 h = _Left * sin(_Particle.m_Angle) + _Up * cos(_Particle.m_Angle);

		_Length *= PARTICLE_LENGTH_SCALE;
		h = h * LERP(_Length, 1.0f, LERP(1.0f, ViewAngleScale, _LengthBlend));

		// FIXME: Den skall skala höjden med slowspeed scale när längden är(närmar sig) 0, typ.
		//PARTICLE_SLOWSPEED_SCALE

		w *= _Particle.m_Size * 0.5f * PARTICLE_WIDTH_SCALE;
		h *= _Particle.m_Size * 0.5f * PARTICLE_HEIGHT_SCALE;

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

			fp32 sharpness = LERP((1.0f - PARTICLE_WIDTH_SHARPNESS), 1.0f, ViewAngleScale);

			m_DVBFire.AddTriangle(0, 1, 2);
			m_DVBFire.AddTriangle(0, 2, 3);
			m_DVBFire.AddVertex(pos - w * sharpness - h, tu, tv, firecolor);
			m_DVBFire.AddVertex(pos + w * sharpness - h, tu + FRAME_X_SIZE, tv, firecolor);
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
	generateParticle(fp32 _Time)
	{
		CXR_Particle2 particle;

		if (_Time > m_TrailDuration)
			return;

		fp32 offset = _Time / PARTICLE_DURATION;

		if ((offset < 0.0f) || (offset >= 1.0f))
			return;

		fp32 animtime = offset;

		fp32 trailfraction = LERP(offset, Sqr(offset), SLOWSTART_FACTOR); // Slow start...

		// Since offset will never reach 1.0f, iTrail0 will never reach (NumStrailSteps - 1).
		// So, at most iTrail0 and iTrail1 will span the last trailstep.
		int iTrail0, iTrail1;
		iTrail0 = TruncToInt(trailfraction * (fp32)(NumTrailSteps - 1));
		iTrail1 = iTrail0 + 1;

		fp32 stepfraction = (trailfraction * (fp32)(NumTrailSteps - 1)) - (fp32)iTrail0;

		fp32 alpha = LERP(PARTICLE_ALPHA_START, PARTICLE_ALPHA_END, LERP(trailfraction, InvSqrInv(trailfraction), PARTICLE_ALPHA_QUICKFADE)); 

		alpha += PARTICLE_ALPHA_NOISE * 2.0f * (getRand() - 0.5f);
		if (alpha < 0.0f) alpha = 0.0f;
		if (alpha > 1.0f) alpha = 1.0f;

		if (_Time < PARTICLE_FADEINTIME)
			alpha *= _Time / PARTICLE_FADEINTIME;

		CVec3Dfp32 dpos;
		dpos.k[0] = (getSinRandTable(PARTICLE_POSFLUCT_TIMESCALE, -trailfraction) - 0.5f);
		dpos.k[1] = (getSinRandTable(PARTICLE_POSFLUCT_TIMESCALE, -trailfraction) - 0.5f);
		dpos.k[2] = (getSinRandTable(PARTICLE_POSFLUCT_TIMESCALE, -trailfraction) - 0.5f);
		dpos *= LERP(PARTICLE_POSFLUCT_START, PARTICLE_POSFLUCT_END, trailfraction);

		particle.m_Pos = LERP(m_TrailPos[iTrail0], m_TrailPos[iTrail1], stepfraction) + dpos;
		
		particle.m_Size = LERP(PARTICLE_SIZE_START, PARTICLE_SIZE_END, trailfraction);
		particle.m_Size += PARTICLE_SIZE_NOISE * 2.0f * (getRand() - 0.5f);
		if (particle.m_Size < 0.0f)
			particle.m_Size = 0.0f;

		particle.m_Color = PARTICLE_COLOR | ((int32)(alpha * 255.0f) << 24);

		particle.m_Angle = 2.0f * (getSinRandTable(PARTICLE_ROTFLUCT_TIMESCALE, -trailfraction) - 0.5f);
		particle.m_Angle *= LERP(PARTICLE_ROTFLUCT_START, PARTICLE_ROTFLUCT_END, trailfraction);

		fp32 ip = stepfraction;

		CVec3Dfp32 Left, Up;
		fp32 Length;
		
		if (ip < 0.5f)
		{
			if (iTrail0 == 0)
			{
				Up = m_TrailPos[1] - m_TrailPos[0];
				Length = Up.Length(); Up *= 1.0f / Length;
			}
			else
			{
				ip += 0.5f;
				CVec3Dfp32 Up0 = m_TrailPos[iTrail0] - m_TrailPos[iTrail0 - 1];
				CVec3Dfp32 Up1 = m_TrailPos[iTrail1] - m_TrailPos[iTrail0];
				fp32 Length0 = Up0.Length();
				fp32 Length1 = Up1.Length();
				Length = LERP(Length0, Length1, ip);
				Up = LERP(Up0, Up1, ip);
//				Length = Up.Length(); Up *= 1.0f / Length;
				Up.Normalize();
			}
		}
		else 
		{
			if (iTrail1 == (NumTrailSteps - 1))
			{
				Up = m_TrailPos[iTrail1] - m_TrailPos[iTrail0];
				Length = Up.Length(); Up *= 1.0f / Length;
			}
			else
			{
				ip -= 0.5f;
				CVec3Dfp32 Up0 = m_TrailPos[iTrail1] - m_TrailPos[iTrail0];
				CVec3Dfp32 Up1 = m_TrailPos[iTrail1 + 1] - m_TrailPos[iTrail1];
				fp32 Length0 = Up0.Length();
				fp32 Length1 = Up1.Length();
				Length = LERP(Length0, Length1, ip);
				Up = LERP(Up0, Up1, ip);
//				Length = Up.Length(); Up *= 1.0f / Length;
				Up.Normalize();
			}
		}

		Up.CrossProd(m_CameraFwd, Left);
		Left.Normalize();

		fp32 lengthblend = 1.0f;
		if (_Time < PARTICLE_LENGTH_FADEINTIME)
		{
			lengthblend *= _Time / PARTICLE_LENGTH_FADEINTIME;
			lengthblend = LERP(lengthblend, Sqr(lengthblend), PARTICLE_LENGTH_FADEINTIME_SLOW);
		}

		RenderParticle(particle, Left, Up, Length, lengthblend, animtime);
	}

	//----------------------------------------------------------------------

	void
	generateBlade()
	{
		int CellRandseedBase = m_RandseedBase + m_NumTotalCells;

		for (int iCell = 0; iCell < m_NumCellsToBacktrace; iCell++) {
			for (int iParticle = 0; iParticle < PARTICLES_PER_TIMECELL; iParticle++)
			{
				m_Randseed = CellRandseedBase - iCell + iParticle;
				if (getRand() <= PARTICLE_EMISSION_PROBABILITY) {
					m_Randseed = CellRandseedBase - iCell + iParticle;
					fp32 separation_time = getRand() * PARTICLE_TIMECELL_DURATION;
					generateParticle(m_Time - fp32(m_NumTotalCells - iCell) * PARTICLE_TIMECELL_DURATION + separation_time);
				}
			}
		}
	}
	
	//----------------------------------------------------------------------

	bool
	fillVB(const CXR_AnimState* _pAnimState, 
		   const CMat4Dfp32& _LocalToWorld, const CMat4Dfp32& _WorldToCamera,
		   CXR_Particle2* _Particles, int32& _NumParticles)
	{
		m_Time = _pAnimState->m_AnimTime0;
		m_RandseedBase = _pAnimState->m_Anim0;
		m_Randseed = m_RandseedBase;

		m_BoundRadius = PARTICLE_RAISE * 1.5f;

		CMat4Dfp32 InvWorldToCamera;
		_WorldToCamera.Inverse(InvWorldToCamera);

		CMat4Dfp32 LocalToCamera;
		_LocalToWorld.Multiply(InvWorldToCamera, LocalToCamera);

		m_CameraLeft = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 0);
		m_CameraUp = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 1);
		m_CameraFwd = -CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 2);
		m_CameraPos = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 3);

		m_Distance = LocalToCamera.k[3][2];

		if (!m_DVBFire.Create(this, m_pVBM, MAXNUMPARTICLES * 4, MAXNUMPARTICLES * 2))
			return false;

		if (!m_DVBSmoke.Create(this, m_pVBM, MAXNUMPARTICLES * 4, MAXNUMPARTICLES * 2))
			return false;

		m_pVBFlare = AllocVB();
		if (m_pVBFlare == NULL)
			return false;

		m_SKFire.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface_Fire);
		m_SKSmoke.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface_Smoke);
		m_SKFlare.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface_Flare);

		if (!updateHistory(_pAnimState, _LocalToWorld))
			return false;

		m_Particles = _Particles;
		m_MaxNumParticles = _NumParticles;
		m_NumParticles = 0;

		// Calculate the number of time cells needed to cover the whole duration of a particle.
		// Each time cell is PARTICLE_MIN_EMISSION_DELAY long in time.
		m_NumCellsToBacktrace = TruncToInt(PARTICLE_DURATION / PARTICLE_TIMECELL_DURATION);

		// Calculate the total number of cell that has been passed during full lifetime of this model.
		m_NumTotalCells = TruncToInt(m_Time / PARTICLE_TIMECELL_DURATION);

		generateBlade();

		_NumParticles = m_NumParticles;

		return true;
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *surface)
	{
		m_iSurface_Fire = GetSurfaceID("FlameFire03");
		m_iSurface_Smoke = GetSurfaceID("FlameSmoke01");
		m_iSurface_Flare = GetSurfaceID("FlameFlare01");

		createSinRandTable();
	}

	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat)
	{
		m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));

		CXR_Particle2	particles[CXR_Util::MAXPARTICLES];
		int32			numParticles = CXR_Util::MAXPARTICLES;

		if (fillVB(_pAnimState, _WMat, _VMat, particles, numParticles))
		{
			if (m_DVBFire.IsValid())
			{
				m_DVBFire.Render(_VMat);
				m_SKFire.Render(m_DVBFire.GetVB(), m_pVBM, m_pEngine);
			}

			if (m_DVBSmoke.IsValid())
			{
				m_DVBSmoke.Render(_VMat);
				m_SKSmoke.Render(m_DVBSmoke.GetVB(), m_pVBM, m_pEngine);
			}

			//m_SKFlare.Render(m_pVBFlare, m_pVBM, m_pEngine);
		}
	}
};

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_CataBall, CXR_Model_Custom);
*/
//----------------------------------------------------------------------
