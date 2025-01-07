#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "CDynamicVB.h"
#include "CSurfaceKey.h"
#include "CModelHistory.h"
#include "CSinRandTable.h"
#include "CPropertyControl.h"
#include "ModelsMisc.h"

//----------------------------------------------------------------------

static
const char* lpSETTINGS[] = 
{
	"fire", 
	"water", 
	"rings", 
	"helix", 
	"glimmer", 
	"wraithtrail", 
	"deatheffect",
	NULL
};

static
const char* lpPRIMITIVE[] = { "point", "cube", "sphere", "tube", "cylinder", "pyramid", "tetraid", NULL };
enum
{
	PRIMITIVE_POINT				= 0x00,
	PRIMITIVE_CUBE				= 0x01,
	PRIMITIVE_SPHERE			= 0x02,
	PRIMITIVE_TUBE				= 0x03,
	PRIMITIVE_CYLINDER			= 0x04,
	PRIMITIVE_PYRAMID			= 0x05,
	PRIMITIVE_TETRAID			= 0x06,
};

static
const char* lpPARTICLE_FLAGS[] = { "fadestill",	"slowdown", "nohistory", "quads", "applyduration", NULL };
enum
{
	PARTICLE_FLAGS_FADESTILL		= 0x01, // Fade when model reaches a still position.
	PARTICLE_FLAGS_SLOWDOWN			= 0x02, // Slow down particle velocity to zero at full duration.
	PARTICLE_FLAGS_NOHISTORY		= 0x04, // Use only the latest history matrix, no older ones.
	PARTICLE_FLAGS_QUADS			= 0x08, // Render each particle using 2 triangles, as a quad, instead of one clamped triangle.
	PARTICLE_FLAGS_APPLYDURATION	= 0x10, // Clip and fade whole model using m_RingDuration.
};

//----------------------------------------------------------------------

#define HISTORY_LENGTH					(20)
#define MAXTIMEJUMP						(0.1f)

//----------------------------------------------------------------------

typedef CModelHistory CMHistory;

//----------------------------------------------------------------------
// CXR_Model_ExpandRing
//----------------------------------------------------------------------

class CXR_Model_ExpandRing : public CXR_Model_Custom
{

	MRTC_DECLARE;

public:

private:

	//----------------------------------------------------------------------

	fp32					m_Time; // Total lifetime of model.
	fp32					m_Duration;
	uint32				m_Randseed, m_RandseedBase;

	fp32					m_SinRandTimeSpread;

//	CMat4Dfp32			m_LocalToWorld[HISTORY_LENGTH];
	CMHistory*			m_pHistory;

	CVec3Dfp32			m_CameraLeft, m_CameraUp;

//	CXR_Particle2*		m_ExpandRing;
//	int32				m_NumParticles;

	int					m_NumCellsToBacktrace;
	int32				m_NumTotalCells;

//	CXR_VertexBuffer*	m_pVB;
	CDynamicVB			m_DVB;
	CSurfaceKey			m_SK;

	CWireContainer*		m_pWC;

	int					m_iSurface;
	fp32					m_RenderPriorityBias;

	CStr				m_Keys;

	fp32					m_RingDuration;
	fp32					m_RingRadiusStart, m_RingRadiusEnd;
	CVec3Dfp32			m_RingRotation, m_RingRotationOffset;
	fp32					m_RingParticleSizeScale;

	bool				m_bApplyRingRotMat;
	CMat43fp32			m_RingRotMat;

	int32				m_NumParticles;
	int					m_MaxNumParticles;
	fp32					m_ParticleDuration, m_ParticleDurationNoise;
	fp32					m_ParticleOffset, m_ParticleOffsetNoise;
	fp32					m_ParticleVelocity, m_ParticleVelocityNoise;

	fp32					m_ParticleSlowdownPower, m_InvParticleSlowdownPower;
	fp32					m_InvParticleSlowdownPowerPlusOne, m_InvInvParticleSlowdownPowerPlusOne;

	CVec3Dfp32			m_ParticleAcceleration, m_ParticleAccelerationNoise;
	fp32					m_AngleSpreadA, m_AngleSpreadB;
	fp32					m_AngleOffsetA, m_AngleOffsetB;
	fp32					m_AngleOffsetChangeA, m_AngleOffsetChangeB;
	int32				m_ParticleColor, m_ParticleColorRGB, m_ParticleColorAlpha;
	fp32					m_ParticleFadeInTime, m_InvParticleFadeInTime;
	fp32					m_ParticleFadeStillThreshold;
	fp32					m_DurationFade;

	CPropertyControl	m_MoveCtrl, m_RotCtrl, m_SizeCtrl, m_AlphaCtrl;

	fp32					m_MaxParticleDuration;
	fp32					m_HistoryDuration, m_InvHistoryDuration;
	int					m_Flags;
	int					m_Setting;

	fp32					m_ParticleEmissionProbability;
	fp32					m_ParticleTimecellDuration;
	int					m_NumParticlesPerTimecell;
	fp32					m_ParticleTimecellSpread;
	fp32					m_ParticleEmissionStop;

	fp32					m_HistoryEntryDelay;

	CMat43fp32			m_LocalToWorld;

	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------

	CMHistory* AllocHistory()
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_AllocHistory, NULL);
		return (DNew(CMHistory) CMHistory(m_HistoryEntryDelay, 1.0f));
	}

	//----------------------------------------------------------------------

	TPtr<CXR_ModelInstance> CreateModelInstance()
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_CreateModelInstance, NULL);
		if (m_Flags & PARTICLE_FLAGS_NOHISTORY)
			return NULL;
		else
			return AllocHistory();
	}

	//----------------------------------------------------------------------

	// Get history from clientdata.
	// If no clientdata present, create DNew one and set it.
	CMHistory*
	GetHistory(const CXR_AnimState* pAnimState)
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_GetHistory, NULL);
		if (pAnimState == NULL)
			return NULL;

		if (pAnimState->m_spModelInstance == NULL)
			return NULL;

		CMHistory* pHistory = (CMHistory*)safe_cast<const CMHistory>((const CXR_ModelInstance*)pAnimState->m_spModelInstance);
		return pHistory;
	}

	//----------------------------------------------------------------------

#ifndef M_RTM

	CMHistory*
	ForceGetHistory(const CXR_AnimState* pAnimState)
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_ForceGetHistory, NULL);
		if (pAnimState == NULL)
			return NULL;

		if (pAnimState->m_pspClientData == NULL)
			return NULL;

		// Check if the clientdata is used for something else...
		CReferenceCount* RefCount = (CReferenceCount*)(*pAnimState->m_pspClientData);
		if ((RefCount != NULL) && (TDynamicCast<CMHistory>(RefCount) == NULL))
			return NULL;

		// Correct type, use it...
		CMHistory* pHistory = (CMHistory*)RefCount;

		// No history was allocated, so allocate it now...
		if (pHistory == NULL) {
			*pAnimState->m_pspClientData = AllocHistory();
			if (*pAnimState->m_pspClientData == NULL) return NULL;
			pHistory = (CMHistory*)(CReferenceCount*)(*pAnimState->m_pspClientData);
		}

		return pHistory;
	}

#endif

	//----------------------------------------------------------------------

	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_GetBound_Box, MAUTOSTRIP_VOID);
		m_pHistory = GetHistory(_pAnimState);
		if (m_pHistory != NULL)
		{
			_Box = m_pHistory->GetStoredBoundBox();
		}
		else
		{
			fp32 MaxAcceleration = m_ParticleAcceleration.Length() + m_ParticleAcceleration.Length() * 0.5f;
			fp32 MaxVelocity = m_ParticleVelocity + m_ParticleVelocityNoise * 0.5f;
			fp32 MaxOffset = m_ParticleOffset + m_ParticleOffsetNoise * 0.5f;
			fp32 MaxSize = Max(m_SizeCtrl.m_StartMax, m_SizeCtrl.m_EndMax);
			fp32 MaxMove = Max(m_MoveCtrl.m_StartMax, m_MoveCtrl.m_EndMax);
			fp32 BoundRadius = MaxOffset + MaxVelocity * m_MaxParticleDuration + MaxAcceleration * Sqr(m_MaxParticleDuration) + MaxMove + MaxSize;
			_Box.m_Min = -BoundRadius;
			_Box.m_Max = BoundRadius;
		}
	}

	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------

	void RenderParticle(CXR_Particle2& _Particle)
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_RenderParticle, MAUTOSTRIP_VOID);
		fp32 c = M_Cos(_Particle.m_Angle);
		fp32 s = M_Sin(_Particle.m_Angle);

		CVec3Dfp32 w = m_CameraLeft * c - m_CameraUp * s;
		CVec3Dfp32 h = m_CameraLeft * s + m_CameraUp * c;

		w *= _Particle.m_Size * 0.5f;
		h *= _Particle.m_Size * 0.5f;

		if (m_Flags & PARTICLE_FLAGS_QUADS)
		{
			m_DVB.AddTriangle(0, 1, 2);
			m_DVB.AddTriangle(0, 2, 3);
			m_DVB.AddVertex(_Particle.m_Pos - w - h, 0, 0, _Particle.m_Color);
			m_DVB.AddVertex(_Particle.m_Pos + w - h, 1, 0, _Particle.m_Color);
			m_DVB.AddVertex(_Particle.m_Pos + w + h, 1, 1, _Particle.m_Color);
			m_DVB.AddVertex(_Particle.m_Pos - w + h, 0, 1, _Particle.m_Color);
		}
		else
		{
			m_DVB.AddTriangle(0, 1, 2);
			m_DVB.AddVertex(_Particle.m_Pos - w - h, 0, 0, _Particle.m_Color);
			m_DVB.AddVertex(_Particle.m_Pos + w * 3.0f - h, 2, 0, _Particle.m_Color);
			m_DVB.AddVertex(_Particle.m_Pos - w + h * 3.0f, 0, 2, _Particle.m_Color);
		}
	}

	//----------------------------------------------------------------------

	fp32 GetAdjustedTime(fp32 _TimeOffset)
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_GetAdjustedTime, 0);
		return (m_Time + m_SinRandTimeSpread * GetRand(m_Randseed) + _TimeOffset);
	}

	//----------------------------------------------------------------------

	void GenerateParticle(fp32 _ParticleTime)
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_GenerateParticle, MAUTOSTRIP_VOID);
		int OldRandseed = m_Randseed;

		fp32 ParticleDuration = m_ParticleDuration + m_ParticleDurationNoise * GetRandS(m_Randseed);

		if ((_ParticleTime < 0) || (_ParticleTime > ParticleDuration))
			return;

		fp32 TimeFraction = _ParticleTime / ParticleDuration;

		fp32 RingAngleA = GetRand(m_Randseed);
		CVec3Dfp32 RingDir;
		AnglesToVector(RingAngleA, 0, RingDir);

		fp32 RingRadius = LERP(m_RingRadiusStart, m_RingRadiusEnd, (m_Time - _ParticleTime) / m_RingDuration);

		fp32 AngleA = RingAngleA + m_AngleOffsetA + m_AngleOffsetChangeA * (m_Time - _ParticleTime) + m_AngleSpreadA * GetRandS(m_Randseed);
		fp32 AngleB = m_AngleOffsetB + m_AngleOffsetChangeB * (m_Time - _ParticleTime) + m_AngleSpreadB * GetRandS(m_Randseed);
		CVec3Dfp32 Dir;
		AnglesToVector(AngleA, AngleB, Dir);

		fp32 Offset = m_ParticleOffset + m_ParticleOffsetNoise * GetRandS(m_Randseed);
		fp32 Velocity = (m_ParticleVelocity + m_ParticleVelocityNoise * GetRandS(m_Randseed));
		CVec3Dfp32 Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(m_Randseed);

		if (m_Flags & PARTICLE_FLAGS_SLOWDOWN)
		{
			Velocity = Velocity * (TimeFraction - powf(TimeFraction, m_InvParticleSlowdownPowerPlusOne) * m_InvInvParticleSlowdownPowerPlusOne) / _ParticleTime;
		}

		fp32 AdjustedTime = GetAdjustedTime(0.0f);

		CVec3Dfp32 Pos = RingDir * RingRadius + (Dir * (Offset + Velocity * _ParticleTime)) + m_MoveCtrl.GetFluctV(AdjustedTime, TimeFraction, &g_SinRandTable, m_Randseed);
		fp32 Rot = m_RotCtrl.GetFluctS(AdjustedTime, TimeFraction, &g_SinRandTable, m_Randseed);
		fp32 Size = m_SizeCtrl.GetFluctS(AdjustedTime, TimeFraction, &g_SinRandTable, m_Randseed);
		fp32 Alpha = m_AlphaCtrl.GetFluctS(AdjustedTime, TimeFraction, &g_SinRandTable, m_Randseed);

		Size *= LERP(1.0f, m_RingParticleSizeScale, (m_Time - _ParticleTime) / m_RingDuration);

		fp32 HistorySpeed;
		CMat43fp32 LocalToWorld;

		if (!(m_Flags & PARTICLE_FLAGS_NOHISTORY))
		{
			fp32 HistoryTime = m_Time - _ParticleTime;
			int HistoryFlags = 0;
			if (!m_pHistory->GetInterpolatedMatrix(HistoryTime, LocalToWorld, HistorySpeed, HistoryFlags))
				return;
		}
		else
		{
			LocalToWorld = m_LocalToWorld;
		}

		if (m_bApplyRingRotMat)
			m_RingRotMat.Multiply(LocalToWorld, LocalToWorld);

		Pos.MultiplyMatrix(LocalToWorld);

		// Add acceleration vector in worldspace instead of localspace.
		Pos += Acceleration * Sqr(_ParticleTime);

		// Apply external duration fadeout.
		Alpha *= m_DurationFade;

		// Apply FadeInTime
		if (_ParticleTime < m_ParticleFadeInTime)
			Alpha *= _ParticleTime * m_InvParticleFadeInTime;

		// Apply FadeStill
		if (m_Flags & PARTICLE_FLAGS_FADESTILL)
		{
			if (HistorySpeed < m_ParticleFadeStillThreshold)
				Alpha *= HistorySpeed / m_ParticleFadeStillThreshold;
		}

		int32 Color = m_ParticleColorRGB | (int32(Alpha * (fp32)m_ParticleColorAlpha) << 24);

		CXR_Particle2 Particle;
		Particle.m_Pos = Pos;
		Particle.m_Angle = Rot;
		Particle.m_Size = Size;
		Particle.m_Color = Color;

		CVec3Dfp32 Distance = Pos - CVec3Dfp32::GetMatrixRow(m_LocalToWorld, 3);
		m_BoundRadius = Max(m_BoundRadius, (fp32)Max3(M_Fabs(Distance[0]), M_Fabs(Distance[1]), M_Fabs(Distance[2])));

		if (m_NumParticles < m_MaxNumParticles)
		{
			RenderParticle(Particle);
			m_NumParticles++;
		}
	}

	//----------------------------------------------------------------------

	void
	GenerateParticles()
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_GenerateParticles, MAUTOSTRIP_VOID);
		int CellRandseedBase = m_RandseedBase + m_NumTotalCells;

		for (int iCell = 0; iCell < m_NumCellsToBacktrace; iCell++) {
			for (int iParticle = 0; iParticle < m_NumParticlesPerTimecell; iParticle++)
			{
				m_Randseed = CellRandseedBase - iCell + iParticle * 100;
				if (GetRand(m_Randseed) <= m_ParticleEmissionProbability) {
					m_Randseed = CellRandseedBase - iCell + iParticle * 100 + 1;
					fp32 SeparationTime = GetRand(m_Randseed) * m_ParticleTimecellDuration * m_ParticleTimecellSpread;
					fp32 ParticleTime = m_Time - fp32(m_NumTotalCells - iCell) * m_ParticleTimecellDuration - SeparationTime;
					fp32 EmissionTime = (m_Time - ParticleTime);
					if ((EmissionTime >= 0.0f) && ((m_ParticleEmissionStop == 0.0f) || (EmissionTime < m_ParticleEmissionStop)))
						GenerateParticle(ParticleTime);
				}
			}
		}
	}
	
	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_Render, MAUTOSTRIP_VOID);
		m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));

//		SetParameters();

		m_Time = _pAnimState->m_AnimTime0;
		m_Duration = _pAnimState->m_AnimTime1;

		if ((m_Flags & PARTICLE_FLAGS_APPLYDURATION) && (m_Time > m_RingDuration))
			return;

		m_pHistory = GetHistory(_pAnimState);
		if (!(m_Flags & PARTICLE_FLAGS_NOHISTORY) && (m_pHistory == NULL))
		{
#ifndef M_RTM
			m_pHistory = ForceGetHistory(_pAnimState);
			if (m_pHistory == NULL)
				m_Flags |= PARTICLE_FLAGS_NOHISTORY;
#else
			m_Flags |= PARTICLE_FLAGS_NOHISTORY;
#endif
		}

		// Set fadetime to m_ParticleFadeInTime and use GetFade() if duration > 0.
		SetFadeTime(m_ParticleFadeInTime);
		m_DurationFade = GetFade(_pAnimState);

		if (m_Flags & PARTICLE_FLAGS_APPLYDURATION)
			m_DurationFade *= 1.0f - m_Time / m_RingDuration;

		m_LocalToWorld = _WMat;

		if ((m_RingRotation != 0) || (m_RingRotationOffset != 0))
		{
			m_bApplyRingRotMat = true;
			m_RingRotMat.Unit();
			m_RingRotMat.M_x_RotX(m_RingRotationOffset[0] + m_RingRotation[0] * m_Time);
			m_RingRotMat.M_x_RotY(m_RingRotationOffset[1] + m_RingRotation[1] * m_Time);
			m_RingRotMat.M_x_RotZ(m_RingRotationOffset[2] + m_RingRotation[2] * m_Time);
//			m_RingRotMat.Multiply(LocalToWorld, LocalToWorld);
		}
		else
		{
			m_RingRotMat.Unit();
			m_bApplyRingRotMat = false;
		}

		m_RandseedBase = _pAnimState->m_Anim0;
		m_Randseed = m_RandseedBase;

		CMat43fp32 InvWorldToCamera;
		_VMat.InverseOrthogonal(InvWorldToCamera);
		m_CameraLeft = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 0);
		m_CameraUp = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 1);

		// Will be max accumulated during particle generation.
		m_BoundRadius = 0.0f;
		m_NumParticles = 0;

		// Calculate the number of time cells needed to cover the whole duration of a particle.
		// Each time cell is PARTICLE_MIN_EMISSION_DELAY long in time.
		m_NumCellsToBacktrace = TruncToInt(m_MaxParticleDuration / m_ParticleTimecellDuration) + 2;

		// Calculate the total number of cell that has been passed during full lifetime of this model.
		m_NumTotalCells = TruncToInt(m_Time / m_ParticleTimecellDuration);

		// Just give one particles margin, to try to avoid any clamping artifacts.
		if (m_Flags & PARTICLE_FLAGS_QUADS)
		{
			if (!m_DVB.Create(this, m_pVBM, (m_MaxNumParticles + 1) * 4, (m_MaxNumParticles + 1) * 2))
				return;
		}
		else
		{
			if (!m_DVB.Create(this, m_pVBM, (m_MaxNumParticles + 1) * 3, (m_MaxNumParticles + 1) * 1))
				return;
		}

		m_SK.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface);

		GenerateParticles();

		if (m_DVB.IsValid())
		{
			m_DVB.Render(_VMat);
			m_SK.Render(m_DVB.GetVB(), m_pVBM, m_pEngine);
		}
	}

	//----------------------------------------------------------------------

	void OnEvalKey(const CRegistry *_pReg)
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_OnEvalKey, MAUTOSTRIP_VOID);
		CStr Name = _pReg->GetThisName();
		CStr Value = _pReg->GetThisValue();
		int Valuei = _pReg->GetThisValue().Val_int();
		fp32 Valuef = _pReg->GetThisValue().Val_fp64();

		if ((Name == "SURFACE") || (Name == "SU"))
		{
			CFStr SurfaceName = Value.GetStrMSep(" #");
			m_iSurface = GetSurfaceID(SurfaceName);
			m_RenderPriorityBias = (fp32)Value.Val_fp64() * TransparencyPriorityBiasUnit;
		}

		else if ((Name == "MAXPARTICLES") || (Name == "MP"))
			m_MaxNumParticles = Valuei;

		else if ((Name == "DURATION") || (Name == "DU"))
			m_ParticleDuration = Valuef;
		else if ((Name == "DURATIONNOISE") || (Name == "DUN"))
			m_ParticleDurationNoise = Valuef;

		else if ((Name == "OFFSET") || (Name == "OF"))
			m_ParticleOffset = Valuef;
		else if ((Name == "OFFSETNOISE") || (Name == "OFN"))
			m_ParticleOffsetNoise = Valuef;

		else if ((Name == "VELOCITY") || (Name == "VE"))
			m_ParticleVelocity = Valuef;
		else if ((Name == "VELOCITYNOISE") || (Name == "VEN"))
			m_ParticleVelocityNoise = Valuef;
		else if ((Name == "SLOWDOWNPOWER") || (Name == "SD"))
			m_ParticleSlowdownPower = Valuef;

		else if ((Name == "ACCELERATION") || (Name == "AX"))
			m_ParticleAcceleration.ParseString(Value);
		else if ((Name == "ACCELERATIONNOISE") || (Name == "AXN"))
			m_ParticleAccelerationNoise.ParseString(Value);

		else if ((Name == "SPREADA") || (Name == "SPA"))
			m_AngleSpreadA = Valuef;
		else if ((Name == "SPREADB") || (Name == "SPB"))
			m_AngleSpreadB = Valuef;

		else if ((Name == "OFFSETA") || (Name == "OFA"))
			m_AngleOffsetA = Valuef;
		else if ((Name == "OFFSETB") || (Name == "OFB"))
			m_AngleOffsetB = Valuef;

		else if ((Name == "OFFSETCHANGEA") || (Name == "DOFA"))
			m_AngleOffsetChangeA = Valuef;
		else if ((Name == "OFFSETCHANGEB") || (Name == "DOFB"))
			m_AngleOffsetChangeB = Valuef;

		else if (Name == "MOVE")
			m_MoveCtrl.Set(Valuef);
		else if (Name == "MV0")
			m_MoveCtrl.m_StartMin = Valuef;
		else if (Name == "MV1")
			m_MoveCtrl.m_StartMax = Valuef;
		else if (Name == "MV2")
			m_MoveCtrl.m_EndMin = Valuef;
		else if (Name == "MV3")
			m_MoveCtrl.m_EndMax = Valuef;
		else if (Name == "MV4")
			m_MoveCtrl.SetSpeed(Valuef);

		else if (Name == "SIZE")
			m_SizeCtrl.Set(Valuef);
		else if (Name == "SZ0")
			m_SizeCtrl.m_StartMin = Valuef;
		else if (Name == "SZ1")
			m_SizeCtrl.m_StartMax = Valuef;
		else if (Name == "SZ2")
			m_SizeCtrl.m_EndMin = Valuef;
		else if (Name == "SZ3")
			m_SizeCtrl.m_EndMax = Valuef;
		else if (Name == "SZ4")
			m_SizeCtrl.SetSpeed(Valuef);

		else if (Name == "ALPHA")
			m_AlphaCtrl.Set(Valuef);
		else if (Name == "AL0")
			m_AlphaCtrl.SetStart(Valuef);
		else if (Name == "AL1")
			m_AlphaCtrl.SetEnd(Valuef);

		else if (Name == "ROT")
			m_RotCtrl.Set(Valuef);
		else if (Name == "RT0")
			m_RotCtrl.m_StartMin = Valuef;
		else if (Name == "RT1")
			m_RotCtrl.m_StartMax = Valuef;
		else if (Name == "RT2")
			m_RotCtrl.m_EndMin = Valuef;
		else if (Name == "RT3")
			m_RotCtrl.m_EndMax = Valuef;
		else if (Name == "RT4")
			m_RotCtrl.SetSpeed(Valuef);

		else if ((Name == "FADEINTIME") || (Name == "FI"))
			m_ParticleFadeInTime = Valuef;

		else if ((Name == "FADESTILLTHRESHOLD") || (Name == "FS"))
			m_ParticleFadeStillThreshold = Valuef;

		else if ((Name == "COLOR") || (Name == "CO"))
			m_ParticleColor = CPixel32::FromStr(Value);

		else if ((Name == "EMISSIONPROBABILITY") || (Name == "EP"))
			m_ParticleEmissionProbability = Valuef;
		else if ((Name == "TIMECELLDURATION") || (Name == "TC"))
			m_ParticleTimecellDuration = Valuef;
		else if ((Name == "PARTICLESPERTIMECELL") || (Name == "PP"))
			m_NumParticlesPerTimecell = Valuei;
		else if ((Name == "TIMECELLSPREAD") || (Name == "TCS"))
			m_ParticleTimecellSpread = Valuef;
		else if ((Name == "EMISSIONSTOP") || (Name == "ES"))
			m_ParticleEmissionStop = Valuef;

		else if ((Name == "FLAGS") || (Name == "FL"))
			m_Flags = Value.TranslateFlags(lpPARTICLE_FLAGS);

		else if (Name == "RI0")
			m_RingDuration = Valuef;
		else if (Name == "RI1")
			m_RingRadiusStart = Valuef;
		else if (Name == "RI2")
			m_RingRadiusEnd = Valuef;
		else if (Name == "RI3")
			m_RingRotation.ParseString(Value);
		else if (Name == "RI4")
			m_RingRotationOffset.ParseString(Value);
		else if (Name == "RI5")
			m_RingParticleSizeScale = Valuef;
		

		else if ((Name == "SETTING") || (Name == "SE"))
			m_Setting = Value.TranslateInt(lpSETTINGS);

		else
			CXR_Model_Custom::OnEvalKey(_pReg);
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *_keys)
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_OnCreate, MAUTOSTRIP_VOID);
		m_Keys = _keys;
		m_Setting = -1;
		SetParameters();

		ParseKeys(_keys);

		if (m_Setting != -1)
			SetParameters();

		ComputeParameters();
	}

	//----------------------------------------------------------------------

	void ComputeParameters()
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_ComputeParameters, MAUTOSTRIP_VOID);
		m_RenderPriorityBias += TransparencyPriorityBaseBias;
		
		// Typically some values lay in certain intervals, so to reduce the need of
		// redundant digits we multiply or divide by 10.
		m_MoveCtrl.m_FluctTimescale /= 10.0f;
		m_SizeCtrl.m_FluctTimescale /= 10.0f;
		m_RotCtrl.m_FluctTimescale /= 10.0f;

		m_InvParticleSlowdownPower = 1.0f / m_ParticleSlowdownPower;
		m_InvParticleSlowdownPowerPlusOne = m_InvParticleSlowdownPower + 1.0f;
		m_InvInvParticleSlowdownPowerPlusOne = 1.0f / m_InvParticleSlowdownPowerPlusOne;
		m_InvParticleFadeInTime = 1.0f / m_ParticleFadeInTime;

		m_MaxParticleDuration = m_ParticleDuration + m_ParticleDurationNoise / 2.0f;
		m_ParticleColorRGB = m_ParticleColor & 0x00FFFFFF;
		m_ParticleColorAlpha = (m_ParticleColor >> 24) & 0xFF;

		m_HistoryEntryDelay = 1.1f * m_MaxParticleDuration / (fp32)(HISTORY_LENGTH - 2);
	}
	
	//----------------------------------------------------------------------

	void SetParameters()
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_SetParameters, MAUTOSTRIP_VOID);
		SetParameters_Default();
		ComputeParameters();
	}

	//**********************************************************************
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	//**********************************************************************

	// Default
	void
	SetParameters_Default()
	{
		MAUTOSTRIP(CXR_Model_ExpandRing_SetParameters_Default, MAUTOSTRIP_VOID);
		m_iSurface = GetSurfaceID("DotParticle");
		m_RenderPriorityBias = 0;

		m_SinRandTimeSpread = 100.0f;
		m_MoveCtrl.m_FluctTimeSpread = 100.0f;

		m_MaxNumParticles = 500;

		m_RingDuration = 3.0f;
		m_RingRadiusStart = 10.0f;
		m_RingRadiusEnd = 100.0f;
		m_RingRotation = 0.0f;
		m_RingRotationOffset = 0.0f;
		m_RingParticleSizeScale = 1.0f;

		m_ParticleDuration = 3.0f;
		m_ParticleDurationNoise = 0.0f;

		m_ParticleOffset = 0.0f;
		m_ParticleOffsetNoise = 0.0f;

		m_ParticleVelocity = 0.0f;
		m_ParticleVelocityNoise = 0.0f;
		m_ParticleSlowdownPower = 0.0f;

		m_ParticleAcceleration = CVec3Dfp32(0.0f, 0.0f, 0.0f);
		m_ParticleAccelerationNoise = CVec3Dfp32(0.0f, 0.0f, 0.0f);

		m_AngleOffsetA = 0.0f;
		m_AngleOffsetB = 0.0f;
		m_AngleSpreadA = 1.0f;
		m_AngleSpreadB = 1.0f;
		m_AngleOffsetChangeA = 0.0f;
		m_AngleOffsetChangeB = 0.0f;

		m_ParticleEmissionProbability = 1.0f;
		m_ParticleTimecellDuration = 0.01f;
		m_NumParticlesPerTimecell = 1;
		m_ParticleTimecellSpread = 1.0f;
		m_ParticleEmissionStop = 0.0f;

		m_ParticleFadeInTime = 0.0f;
		m_ParticleColor = 0xFFFFFFFF;
		m_Flags = 0;
		m_ParticleFadeStillThreshold = 1.0f;

		m_SizeCtrl.Set(10.0f);

		m_AlphaCtrl.SetStart(1.0f);
		m_AlphaCtrl.SetEnd(0.0f);
	}

};

//----------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_ExpandRing, CXR_Model_Custom);

//----------------------------------------------------------------------
