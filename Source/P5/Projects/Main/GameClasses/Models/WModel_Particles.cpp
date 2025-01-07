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
const char* lpPARTICLE_FLAGS[] = { "fadestill",	"slowdown", "nohistory", "quads", "primcenter", "worldrot", "hflags", "spline", "align", "ctime", "early", "ihflags", "nolod", "localaxx", "stopmotion", "noclampcolors", "showoverflow", "channeledcolornoise", "ets", "asq", NULL };
static
const char* lpPARTICLE_FLAGS_SHORT[] = { "fs",	"sd", "nh", "qd", "pc", "wr", "hf", "sp", "al", "ct", "erl", "ihf", "nl", "la", "sm", "ncc", "sof", "ccn", "ets", "asq", NULL };
enum
{
	PARTICLE_FLAGS_FADESTILL			= 0x00001, // Fade when model reaches a still position.
	PARTICLE_FLAGS_SLOWDOWN				= 0x00002, // Slow down particle velocity to zero at full duration.
	PARTICLE_FLAGS_NOHISTORY			= 0x00004, // Use only the latest history matrix, no older ones.
	PARTICLE_FLAGS_QUADS				= 0x00008, // Render each particle using 2 triangles, as a quad, instead of one clamped triangle.
	PARTICLE_FLAGS_PRIMCENTER			= 0x00010, // Force particle velocity center to primitive/model center.
	PARTICLE_FLAGS_WORLDROT				= 0x00020, // Force particle velocity center to primitive/model center.
	PARTICLE_FLAGS_HISTORYFLAGS			= 0x00040, // Emit particles only when history flags are != 0.
	PARTICLE_FLAGS_SPLINEHISTORY		= 0x00080, // Use spline interpolation when calculating intermediate history matrices.
	PARTICLE_FLAGS_ALIGN				= 0x00100, // Align particle rotation with its current movement vector.
	PARTICLE_FLAGS_CONTINUOUSTIME		= 0x00200, // Use client gametick time istead of enginepath controlled time.
	PARTICLE_FLAGS_FORCEEARLY			= 0x00400, // Spawn particles as early as the latest tick + iptime, ignore interpolation misses.
	PARTICLE_FLAGS_IHISTORYFLAGS		= 0x00800, // Emit particles only when history flags are == 0.
	PARTICLE_FLAGS_NOLOD				= 0x01000, // Turn off loding.
	PARTICLE_FLAGS_LOCALACCELERATION	= 0x02000, // Apply acceleration in local space instead of in worldspace.
	PARTICLE_FLAGS_STOPMOTION			= 0x04000, // A better version of slowdown. Particles change velocity and stop after a certain time.
	PARTICLE_FLAGS_NOCLAMPCOLORS		= 0x08000, // Don't clamp particle color components to 0-255.
	PARTICLE_FLAGS_SHOWOVERFLOW			= 0x10000, // Use default texture when exceeding maxparticles limit.
	PARTICLE_FLAGS_CHANNELEDCOLORNOISE	= 0x20000, // Don't randomize color channels individually.
	PARTICLE_FLAGS_EMITTERTIME_SIZE		= 0x40000, // Use emitter time fraction for size interpolations.
	PARTICLE_FLAGS_ALIGN_SAFEQUAD		= 0x80000, // Quad aligned particles with movement aligned to CameraForward (to avoid thin particles).

	PARTICLE_MAXPARTICLEID				= 5,
};

//----------------------------------------------------------------------

#define HISTORY_LENGTH					(20)
#define MAXTIMEJUMP						(0.1f)

//----------------------------------------------------------------------

typedef CModelHistory CMHistory;

//----------------------------------------------------------------------
// ParticlesClassName
//----------------------------------------------------------------------

#ifdef TESTNEWPARTICLES
	#define ParticlesClassName CXR_Model_ParticlesOpt
#else
	#define ParticlesClassName CXR_Model_Particles
#endif

class ParticlesClassName : public CXR_Model_Custom
{
	MRTC_DECLARE;

public:

private:

	//----------------------------------------------------------------------
	fp32					m_lIDFade[PARTICLE_MAXPARTICLEID];
	int					m_IDDisableMask;

	fp32					m_Time; // Total lifetime of model.
	fp32					m_Duration;
	fp32					m_TimeFraction;

	uint32				m_Randseed, m_RandseedBase;

	fp32					m_SinRandTimeSpread;

	CMHistory*			m_pHistory;

	CBox3Dfp32			m_BBox;
	bool				m_BBox_bIsValid;

	CVec3Dfp32			m_CameraFwd, m_CameraLeft, m_CameraUp, m_CameraPos;

	int					m_NumCellsToBacktrace;
	int32				m_NumTotalCells;

	CDynamicVB			m_DVB;
	CSurfaceKey			m_SK;

	CWireContainer*		m_pWC;

	int					m_iSurface;
	fp32					m_RenderPriorityBias;

	CStr				m_Keys;

	int					m_Flags;

	fp32					m_TimeScale;
	fp32					m_TimeOffset;

	bool				m_bOverflow;
	int32				m_NumParticles;
	int					m_MaxNumParticles;
	fp32					m_ParticleDuration, m_ParticleDurationNoise;
	fp32					m_ParticleOffset, m_ParticleOffsetNoise;
	fp32					m_ParticleVelocity, m_ParticleVelocityNoise;

	fp32					m_Particle_StopMotionTime, m_Particle_InvStopMotionTime;

	fp32					m_ParticleSlowdownPower, m_InvParticleSlowdownPower;
	fp32					m_InvParticleSlowdownPowerPlusOne, m_InvInvParticleSlowdownPowerPlusOne;

	CVec3Dfp32			m_LocalPositionOffset;
	CVec3Dfp32			m_ParticleAcceleration, m_ParticleAccelerationNoise;
	fp32					m_AngleSpreadA, m_AngleSpreadB;
	fp32					m_AngleOffsetA, m_AngleOffsetB;
	fp32					m_AngleOffsetChangeA, m_AngleOffsetChangeB;
	CVec4Dfp32			m_ParticleColor, m_ParticleColorNoise;
	fp32					m_ParticleFadeInTime, m_InvParticleFadeInTime;
	fp32					m_ParticleFadeStillThreshold;
	fp32					m_DurationFade;
	fp32					m_FadeInTime, m_FadeOutTime;

	fp32					m_ParticleAlignLengthScale;

	CPropertyControl	m_MoveCtrl, m_RotCtrl, m_SizeCtrl, m_AlphaCtrl;

	fp32					m_MaxParticleDuration;
	fp32					m_HistoryDuration, m_InvHistoryDuration;

	fp32					m_ParticleEmissionProbability;
	fp32					m_ParticleEmissionProbability_TimeScale;
	fp32					m_ParticleTimecellDuration;
	int					m_NumParticlesPerTimecell;
	fp32					m_ParticleTimecellSpread;
	fp32					m_ParticleEmissionStop;

	fp32					m_HistoryEntryDelay;

	int					m_DistributionPrimitive;
	CVec3Dfp32			m_DistributionSize;
	CVec3Dfp32			m_DistributionRotation;
	bool				m_bHollowDistribution;
	CMat43fp32			m_DistRotMat;
	bool				m_bApplyDistRotMat;

	CMat43fp32			m_LocalToWorld;

	bool				m_bOffset;
	bool				m_bVelocity;
	bool				m_bAccelerate;

	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------

	int GetParam(int _Param)
	{
		MAUTOSTRIP(ParticlesClassName_GetParam, 0);

		if (_Param == CXR_MODEL_PARAM_TIMEMODE)
		{
			if (m_Flags & PARTICLE_FLAGS_CONTINUOUSTIME)
				return CXR_MODEL_TIMEMODE_CONTINUOUS;
			else
				return CXR_MODEL_TIMEMODE_CONTROLLED;
		}

		return CXR_Model_Custom::GetParam(_Param);
	}
	
	//----------------------------------------------------------------------

	CMHistory* AllocHistory()
	{
		MAUTOSTRIP(ParticlesClassName_AllocHistory, NULL);
		return (DNew(CMHistory) CMHistory(m_HistoryEntryDelay, 1.0f * m_TimeScale));
	}

	//----------------------------------------------------------------------

	TPtr<CXR_ModelInstance> CreateModelInstance()
	{
		MAUTOSTRIP(ParticlesClassName_CreateModelInstance, NULL);
		if (m_Flags & PARTICLE_FLAGS_NOHISTORY)
			return NULL;
		else
			return AllocHistory();
	}

	//----------------------------------------------------------------------

	// Get history from clientdata.
	CMHistory*
	GetHistory(const CXR_AnimState* pAnimState)
	{
		MAUTOSTRIP(ParticlesClassName_GetHistory, NULL);
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
		MAUTOSTRIP(ParticlesClassName_ForceGetHistory, NULL);
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
		MAUTOSTRIP(ParticlesClassName_GetBound_Box, MAUTOSTRIP_VOID);
		m_pHistory = GetHistory(_pAnimState);
		if ((m_pHistory != NULL) && m_pHistory->HasStoredBoundBox())
		{
			_Box = m_pHistory->GetStoredBoundBox();
		}
		else
		{
			fp32 MaxDistSize = Max3(m_DistributionSize[0], m_DistributionSize[1], m_DistributionSize[2]);
			fp32 MaxAcceleration = m_ParticleAcceleration.Length() + m_ParticleAcceleration.Length() * 0.5f;
			fp32 MaxVelocity = m_ParticleVelocity + m_ParticleVelocityNoise * 0.5f;
			fp32 MaxOffset = m_ParticleOffset + m_ParticleOffsetNoise * 0.5f;
			fp32 MaxSize = Max(m_SizeCtrl.m_StartMax, m_SizeCtrl.m_EndMax);
			fp32 MaxMove = Max(m_MoveCtrl.m_StartMax, m_MoveCtrl.m_EndMax);
			fp32 BoundRadius = MaxDistSize + MaxOffset + MaxVelocity * m_MaxParticleDuration + MaxAcceleration * Sqr(m_MaxParticleDuration) + MaxMove + MaxSize;
			_Box.m_Min = -BoundRadius;
			_Box.m_Max = BoundRadius;
		}
	}

	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------

	void GetCubePoint(CVec3Dfp32& _point)
	{
		MAUTOSTRIP(ParticlesClassName_GetCubePoint, MAUTOSTRIP_VOID);
		_point[0] = GetRandS(m_Randseed);
		_point[1] = GetRandS(m_Randseed);
		_point[2] = GetRandS(m_Randseed);
	}

	//----------------------------------------------------------------------

	fp32 GetRandSRadius()
	{
		MAUTOSTRIP(ParticlesClassName_GetRandSRadius, 0);
		// Apply random radius. This will also scale to signed unit.
		// InvSqrInv is used to bias the distribution towards larger radius, since those have larger circumference.
		// Two step rand is needed, since InvSqrInv is a repeating macro.
/*
		fp32 radius;
		radius = GetRand(m_Randseed);
		radius = 0.5f * Sign(radius) * InvSqrInv(InvSqrInv(InvSqrInv(radius)));
		return radius;
*/
		fp32 distribution = GetRand(m_Randseed);
		fp32 radius = 0.5f * M_Sqrt(1.0f - Sqr(1.0f - distribution));
		return radius;

	}

	//----------------------------------------------------------------------

	void GetSpherePoint(CVec3Dfp32& _point)
	{
		MAUTOSTRIP(ParticlesClassName_GetSpherePoint, MAUTOSTRIP_VOID);
		// Set random direction.
		AnglesToVector(GetRandS(m_Randseed), GetRandS(m_Randseed), _point);

		_point *= GetRandSRadius();
	}

	//----------------------------------------------------------------------

	void GetCylinderSlicePoint(CVec3Dfp32& _point, fp32 _height)
	{
		MAUTOSTRIP(ParticlesClassName_GetCylinderSlicePoint, MAUTOSTRIP_VOID);
		// Set random direction in XY plane.
		AnglesToVector(GetRandS(m_Randseed), 0, _point);

		_point *= GetRandSRadius();

		// Set slice height.
		_point[2] = _height; 
	}

	//----------------------------------------------------------------------

	void GetTubePoint(CVec3Dfp32& _point)
	{
		MAUTOSTRIP(ParticlesClassName_GetTubePoint, MAUTOSTRIP_VOID);
		GetCylinderSlicePoint(_point, GetRandS(m_Randseed));
	}

	//----------------------------------------------------------------------

	void GetCylinderPoint(CVec3Dfp32& _point)
	{
		MAUTOSTRIP(ParticlesClassName_GetCylinderPoint, MAUTOSTRIP_VOID);
		GetTubePoint(_point);
	}

	//----------------------------------------------------------------------

	void GetPyramidPoint(CVec3Dfp32& _point)
	{
		MAUTOSTRIP(ParticlesClassName_GetPyramidPoint, MAUTOSTRIP_VOID);
		// Not yet implemented.
		_point = 0;
	}

	//----------------------------------------------------------------------

	void GetTetraidPoint(CVec3Dfp32& _point)
	{
		MAUTOSTRIP(ParticlesClassName_GetTetraidPoint, MAUTOSTRIP_VOID);
		// Not yet implemented.
		_point = 0;
	}

	//----------------------------------------------------------------------

	void GetHollowCubePoint(CVec3Dfp32& _point)
	{
		MAUTOSTRIP(ParticlesClassName_GetHollowCubePoint, MAUTOSTRIP_VOID);
		fp32 u = GetRandS(m_Randseed);
		fp32 v = GetRandS(m_Randseed);
		fp32 w = 0.5f;
		
		int side = TruncToInt(GetRand(m_Randseed) * 5.99f); // Random integer between 0 and 5.
		switch (side)
		{
			case 0: _point = CVec3Dfp32(w, u, v); break;
			case 1: _point = CVec3Dfp32(-w, u, v); break;
			case 2: _point = CVec3Dfp32(u, w, v); break;
			case 3: _point = CVec3Dfp32(u, -w, v); break;
			case 4: _point = CVec3Dfp32(u, v, w); break;
			case 5: _point = CVec3Dfp32(u, v, -w); break;
		}
	}

	//----------------------------------------------------------------------

	void GetHollowSpherePoint(CVec3Dfp32& _point)
	{
		MAUTOSTRIP(ParticlesClassName_GetHollowSpherePoint, MAUTOSTRIP_VOID);
		// Set random direction.
		AnglesToVector(GetRandS(m_Randseed), GetRandS(m_Randseed), _point);
		_point *= 0.5f; // Scale radius to signed unit.
	}

	//----------------------------------------------------------------------

	void GetHollowTubePoint(CVec3Dfp32& _point)
	{
		MAUTOSTRIP(ParticlesClassName_GetHollowTubePoint, MAUTOSTRIP_VOID);
		// Set random direction in XY plane.
		AnglesToVector(GetRandS(m_Randseed), 0, _point);
		_point *= 0.5f; // Scale radius to signed unit.

		// Set random height.
		_point[2] = GetRandS(m_Randseed);
	}

	//----------------------------------------------------------------------

	void GetHollowCylinderPoint(CVec3Dfp32& _point)
	{
		MAUTOSTRIP(ParticlesClassName_GetHollowCylinderPoint, MAUTOSTRIP_VOID);
		// The surface area of a hollow unit cylinder is 25% for each of the two caps and 50% for the "height" surface.
		int side = TruncToInt(GetRand(m_Randseed) * 3.99f); // Random integer between 0 and 3.
		switch (side)
		{
			case 0: GetCylinderSlicePoint(_point, +0.5f); break; // 25% for top cap.
			case 1: GetCylinderSlicePoint(_point, -0.5f); break; // 25% for bottom cap.
			default: GetHollowTubePoint(_point); break; // 50% for the rest, i.e. the "height" surface.
		}
	}

	//----------------------------------------------------------------------

	void GetHollowPyramidPoint(CVec3Dfp32& _point)
	{
		MAUTOSTRIP(ParticlesClassName_GetHollowCylinderPoint, MAUTOSTRIP_VOID);
		// Not yet implemented.
		_point = 0;
	}

	//----------------------------------------------------------------------

	void GetHollowTetraidPoint(CVec3Dfp32& _point)
	{
		MAUTOSTRIP(ParticlesClassName_GetHollowTetraidPoint, MAUTOSTRIP_VOID);
		// Not yet implemented.
		_point = 0;
	}

	//----------------------------------------------------------------------

	void GetDistributedPoint(CVec3Dfp32& _point)
	{
		MAUTOSTRIP(ParticlesClassName_GetDistributedPoint, MAUTOSTRIP_VOID);
		// Generate a random point within one signed volume unit (i.e. (-0.5, -0.5, -0.5) .. (+0.5, +0.5, +0.5)).
		if (m_DistributionPrimitive == PRIMITIVE_POINT)
		{
			_point = 0;
			return;
		}

		if (m_bHollowDistribution)
		{
			switch (m_DistributionPrimitive)
			{
				case PRIMITIVE_CUBE: GetHollowCubePoint(_point); break;
				case PRIMITIVE_SPHERE: GetHollowSpherePoint(_point); break;
				case PRIMITIVE_CYLINDER: GetHollowCylinderPoint(_point); break;
				case PRIMITIVE_TUBE: GetHollowTubePoint(_point); break;
				case PRIMITIVE_PYRAMID: GetHollowPyramidPoint(_point); break;
			}
		}
		else
		{
			switch (m_DistributionPrimitive)
			{
				case PRIMITIVE_CUBE: GetCubePoint(_point); break;
				case PRIMITIVE_SPHERE: GetSpherePoint(_point); break;
				case PRIMITIVE_CYLINDER: GetCylinderPoint(_point); break;
				case PRIMITIVE_TUBE: GetHollowTubePoint(_point); break;
				case PRIMITIVE_PYRAMID: GetPyramidPoint(_point); break;
			}
		}
	}

	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------

	void RenderParticleWH(const CXR_Particle2& _Particle, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up)
	{
		MAUTOSTRIP(ParticlesClassName_RenderParticleWH, MAUTOSTRIP_VOID);
//		MSCOPE(RenderParticleWH,RenderParticlesWHMem);

		fp32 c = M_Cos(_Particle.m_Angle);
		fp32 s = M_Sin(_Particle.m_Angle);

		fp32 Size = _Particle.m_Size * 0.5f;
		CVec3Dfp32 w = (_Left * c - _Up * s) * Size;
		CVec3Dfp32 h = (_Left * s + _Up * c) * Size;

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

	void RenderParticle(const CXR_Particle2& _Particle)
	{
		MAUTOSTRIP(ParticlesClassName_RenderParticle, MAUTOSTRIP_VOID);
		RenderParticleWH(_Particle, m_CameraLeft, m_CameraUp);
	}

	void RenderParticleAlighed(CXR_Particle2& _Particle, const CVec3Dfp32& _Movement)
	{
		MAUTOSTRIP(ParticlesClassName_RenderParticleAlighed, MAUTOSTRIP_VOID);
		// Up is particle forward...

		CVec3Dfp32 Dir = _Movement; Dir.Normalize();
		CVec3Dfp32 Left; Dir.CrossProd(m_CameraFwd, Left);
		if (Left != 0)
			Left.Normalize();
		else
			Left = CVec3Dfp32(0, 1, 0);

		CVec3Dfp32 Up;

		if (m_Flags & PARTICLE_FLAGS_ALIGN_SAFEQUAD)
		{
			fp32 CameraAlignment = Sqr(Sqr(M_Fabs(m_CameraFwd * Dir)));
			CVec3Dfp32 CameraUp; m_CameraFwd.CrossProd(Left, CameraUp);
			Up = LERP(_Movement, CameraUp, CameraAlignment);
		}
		else
		{
			Up = _Movement;
		}

		RenderParticleWH(_Particle, Left, Up);
	}

	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------

	fp32 GetAdjustedTime(fp32 _TimeOffset)
	{
		MAUTOSTRIP(ParticlesClassName_GetAdjustedTime, 0);
		return (m_Time + m_SinRandTimeSpread * GetRand(m_Randseed) + _TimeOffset);
	}

	fp32 GetDepthFade(fp32 _Distance, fp32 _Start, fp32 _End)
	{
		MAUTOSTRIP(ParticlesClassName_GetDepthFade, 0);
		return Clamp01((_End - _Distance) / (_End - _Start));
//		return Max(0.0f, Min(1.0f, (_End - _Distance) / (_End - _Start)));
	}

	//----------------------------------------------------------------------

	void GenerateParticle(fp32 _ParticleTime)
	{
		MAUTOSTRIP(ParticlesClassName_GenerateParticle, MAUTOSTRIP_VOID);
//		MSCOPE(GenerateParticle,GenerateParticlesMem);

		int OldRandseed = m_Randseed;

		int ParticleID = m_Randseed;

		fp32 ParticleDuration = m_ParticleDuration + m_ParticleDurationNoise * GetRandS(m_Randseed);

		if ((_ParticleTime < 0) || (_ParticleTime > ParticleDuration))
			return;

		fp32 TimeFraction = _ParticleTime / ParticleDuration;

		fp32 AdjustedTime = GetAdjustedTime(0.0f);

		CMat43fp32 LocalToWorld;

		fp32 HistorySpeed = 0;
		if (!(m_Flags & PARTICLE_FLAGS_NOHISTORY))
		{
			fp32 HistoryTime = m_Time - _ParticleTime;
			int HistoryFlags = 0;
			IPMethod ipmethod = (m_Flags & PARTICLE_FLAGS_SPLINEHISTORY) ? IPMethod_Spline : IPMethod_Linear;

			if (!m_pHistory->GetInterpolatedMatrix(HistoryTime, LocalToWorld, HistorySpeed, HistoryFlags, ipmethod))
				return;

			if ((m_Flags & PARTICLE_FLAGS_HISTORYFLAGS) && (HistoryFlags == 0))
				return;

			if ((m_Flags & PARTICLE_FLAGS_IHISTORYFLAGS) && (HistoryFlags != 0))
				return;
		}
		else
		{
			LocalToWorld = m_LocalToWorld;
		}

		fp32 Alpha = 1.0f;

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

		if (m_AlphaCtrl.m_ValueType & CPropertyControl::VALUETYPE_LINEAR)
			Alpha *= m_AlphaCtrl.GetLinear(TimeFraction);
		else
			Alpha *= m_AlphaCtrl.GetFluctS(AdjustedTime, TimeFraction, &g_SinRandTable, m_Randseed);

		if (Alpha <= 0.0f)
			return;
		
		// Distance LODing
		if (!(m_Flags & PARTICLE_FLAGS_NOLOD))
		{
/*			fp32 Distance = m_CameraFwd * (CVec3Dfp32::GetMatrixRow(LocalToWorld, 3) - m_CameraPos);
			Distance = GetCorrectedDistance(m_pEngine, Distance);

			fp32 Fade;

			if (ParticleID & 1)
				Fade = GetDepthFade(Distance, 250, 500);
			else if (ParticleID & 2)
				Fade = GetDepthFade(Distance, 500, 750);
			else if (ParticleID & 4)
				Fade = GetDepthFade(Distance, 750, 1000);
			else if (ParticleID & 8)
				Fade = GetDepthFade(Distance, 1000, 2000);
			else
				Fade = GetDepthFade(Distance, 2000, 4000);

			Alpha *= Fade;
*/
			if (ParticleID & 1)
				Alpha *=  m_lIDFade[0];
			else if (ParticleID & 2)
				Alpha *=  m_lIDFade[1];
			else if (ParticleID & 4)
				Alpha *=  m_lIDFade[2];
			else if (ParticleID & 8)
				Alpha *= m_lIDFade[3];
			else
				Alpha *= m_lIDFade[4];

			if (Alpha <= 0.0f)
			{
				if (m_NumParticles < m_MaxNumParticles)
					m_NumParticles++;
				else
					m_bOverflow = true;

				return;
			}
		}
		
		fp32 AngleA = m_AngleOffsetA + m_AngleOffsetChangeA * (m_Time - _ParticleTime) + m_AngleSpreadA * GetRandS(m_Randseed);
		fp32 AngleB = m_AngleOffsetB + m_AngleOffsetChangeB * (m_Time - _ParticleTime) + m_AngleSpreadB * GetRandS(m_Randseed);
		CVec3Dfp32 Dir;
		AnglesToVector(AngleA, AngleB, Dir);

		fp32 Offset;
		fp32 Velocity;
		fp32 VelocityOffset;
		CVec3Dfp32 Acceleration;

		if (m_bOffset)
			Offset = m_ParticleOffset + m_ParticleOffsetNoise * GetRandS(m_Randseed);
		else
			Offset = 0;

		if (m_bVelocity)
		{
			Velocity = (m_ParticleVelocity + m_ParticleVelocityNoise * GetRandS(m_Randseed));
			VelocityOffset = Velocity * _ParticleTime;
		}
		else
		{
			Velocity = 0;
			VelocityOffset = 0;
		}

		if (m_bAccelerate)
			Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(m_Randseed);
		else
			Acceleration = 0;

		if (m_Flags & PARTICLE_FLAGS_SLOWDOWN)
		{
			Velocity = Velocity * (TimeFraction - powf(TimeFraction, m_InvParticleSlowdownPowerPlusOne) * m_InvInvParticleSlowdownPowerPlusOne) / _ParticleTime;
			VelocityOffset = Velocity * _ParticleTime;
		}

		CVec3Dfp32 DistributedPoint;
		if (m_DistributionPrimitive != PRIMITIVE_POINT)
		{
			GetDistributedPoint(DistributedPoint);
/*
			if (m_bApplyDistRotMat)
				DistributedPoint.MultiplyMatrix3x3(m_DistRotMat);
*/
			if (m_Flags & PARTICLE_FLAGS_PRIMCENTER)
			{
				// Scale point from signed volume unit to desired distribution size.
				Dir = DistributedPoint;
				Dir[0] *= m_DistributionSize[0];
				Dir[1] *= m_DistributionSize[1];
				Dir[2] *= m_DistributionSize[2];
				Dir.Normalize();
			}
		}

		CVec3Dfp32 Pos;
		fp32 Size;
		fp32 Rot;

		if (m_Flags & PARTICLE_FLAGS_STOPMOTION)
		{
			fp32 SMTimeFraction = Clamp01(_ParticleTime * m_Particle_InvStopMotionTime);
			fp32 SMTimeFraction2 = Sqr(TimeFraction);
			VelocityOffset = Velocity * SMTimeFraction - Velocity * SMTimeFraction2 * 0.5f;
//			VelocityOffset = Velocity * TimeFraction - Velocity * SMTimeFraction2 * 0.5f;
		}

		// Move Control
		if (m_MoveCtrl.m_ValueType & CPropertyControl::VALUETYPE_ZERO)
			Pos = (Dir * (Offset + VelocityOffset));
		else if (m_MoveCtrl.m_ValueType & CPropertyControl::VALUETYPE_RANDOMFLUCT)
			Pos = (Dir * (Offset + VelocityOffset)) + m_MoveCtrl.GetFluctV(AdjustedTime, TimeFraction, &g_SinRandTable, m_Randseed);
		else
			Pos = (Dir * (Offset + VelocityOffset)) + m_MoveCtrl.GetRandomV(TimeFraction, m_Randseed);

		// Size Control
		if (m_Flags & PARTICLE_FLAGS_EMITTERTIME_SIZE)
		{
			fp32 SizeTime = m_Time -_ParticleTime;
			fp32 SizeTimeFraction = (m_Duration > 0) ? SizeTime / m_Duration : SizeTime;
			if (m_SizeCtrl.m_ValueType & CPropertyControl::VALUETYPE_CONSTANT)
				Size = m_SizeCtrl.GetConstant();
			else if (m_SizeCtrl.m_ValueType & CPropertyControl::VALUETYPE_LINEAR)
				Size = m_SizeCtrl.GetLinear(SizeTimeFraction);
			else
				Size = m_SizeCtrl.GetFluctS(AdjustedTime, SizeTimeFraction, &g_SinRandTable, m_Randseed);
		}
		else
		{
			if (m_SizeCtrl.m_ValueType & CPropertyControl::VALUETYPE_CONSTANT)
				Size = m_SizeCtrl.GetConstant();
			else if (m_SizeCtrl.m_ValueType & CPropertyControl::VALUETYPE_LINEAR)
				Size = m_SizeCtrl.GetLinear(TimeFraction);
			else
				Size = m_SizeCtrl.GetFluctS(AdjustedTime, TimeFraction, &g_SinRandTable, m_Randseed);
		}

		// Rotation Control
		Rot = m_RotCtrl.GetFluctS(AdjustedTime, TimeFraction, &g_SinRandTable, m_Randseed);

		if (m_DistributionPrimitive != PRIMITIVE_POINT)
		{
			// Scale point from signed volume unit to desired distribution size.
			DistributedPoint[0] *= m_DistributionSize[0];
			DistributedPoint[1] *= m_DistributionSize[1];
			DistributedPoint[2] *= m_DistributionSize[2];
			Pos += DistributedPoint;
		}

		Pos += m_LocalPositionOffset;

		if (m_bAccelerate)
		{
			if (m_Flags & PARTICLE_FLAGS_LOCALACCELERATION)
			{
				// Add acceleration vector in localspace before transforming into worldspace.
				Pos += Acceleration * Sqr(_ParticleTime);
				Pos.MultiplyMatrix(LocalToWorld);
			}
			else
			{
				// Add acceleration vector in worldspace instead of localspace.
				Pos.MultiplyMatrix(LocalToWorld);
				Pos += Acceleration * Sqr(_ParticleTime);
			}
		}
		else
		{
			Pos.MultiplyMatrix(LocalToWorld);
		}

		CVec4Dfp32 Color;

		if (m_Flags & PARTICLE_FLAGS_CHANNELEDCOLORNOISE)
		{
			Color[0] = m_ParticleColor[0] + m_ParticleColorNoise[0] * GetRandS(m_Randseed);
			Color[1] = m_ParticleColor[1] + m_ParticleColorNoise[1] * GetRandS(m_Randseed);
			Color[2] = m_ParticleColor[2] + m_ParticleColorNoise[2] * GetRandS(m_Randseed);
			Color[3] = (m_ParticleColor[3] + m_ParticleColorNoise[3] * GetRandS(m_Randseed)) * Alpha;
		}
		else
		{
			Color[0] = m_ParticleColor[0] + m_ParticleColorNoise[0] * GetRandS(m_Randseed);
			Color[1] = m_ParticleColor[1] + m_ParticleColorNoise[1] * GetRandS(m_Randseed);
			Color[2] = m_ParticleColor[2] + m_ParticleColorNoise[2] * GetRandS(m_Randseed);
			Color[3] = (m_ParticleColor[3] + m_ParticleColorNoise[3] * GetRandS(m_Randseed)) * Alpha;
		}

		if (!(m_Flags & PARTICLE_FLAGS_NOCLAMPCOLORS))
		{
			Color[0] = Max(0.0f, Min(Color[0], 255.0f));
			Color[1] = Max(0.0f, Min(Color[1], 255.0f));
			Color[2] = Max(0.0f, Min(Color[2], 255.0f));
			Color[3] = Max(0.0f, Min(Color[3], 255.0f));
		}

		CXR_Particle2 Particle;
		Particle.m_Pos = Pos;
		Particle.m_Angle = Rot;
		Particle.m_Size = Size;
		Particle.m_Color = CPixel32(Color[0], Color[1], Color[2], Color[3]);

		if (!m_BBox_bIsValid)
		{
			m_BBox.m_Min = Pos;
			m_BBox.m_Max = Pos;
			m_BBox_bIsValid = true;
		}
		else
		{
			if (Pos.k[0] < m_BBox.m_Min.k[0]) m_BBox.m_Min.k[0] = Pos.k[0];
			if (Pos.k[1] < m_BBox.m_Min.k[1]) m_BBox.m_Min.k[1] = Pos.k[1];
			if (Pos.k[2] < m_BBox.m_Min.k[2]) m_BBox.m_Min.k[2] = Pos.k[2];
			if (Pos.k[0] > m_BBox.m_Max.k[0]) m_BBox.m_Max.k[0] = Pos.k[0];
			if (Pos.k[1] > m_BBox.m_Max.k[1]) m_BBox.m_Max.k[1] = Pos.k[1];
			if (Pos.k[2] > m_BBox.m_Max.k[2]) m_BBox.m_Max.k[2] = Pos.k[2];
		}

		if (m_NumParticles < m_MaxNumParticles)
		{
			if (m_Flags & PARTICLE_FLAGS_ALIGN)
			{
				CVec3Dfp32 Movement;
				if (m_Flags & PARTICLE_FLAGS_STOPMOTION)
				{
					fp32 TimeFraction = Max(0.0f, Min(1.0f, _ParticleTime * m_Particle_InvStopMotionTime));
					Movement = Dir * (Velocity - Velocity * TimeFraction);
				}
				else
				{
					Movement = Dir * Velocity;
				}

				if (m_Flags & PARTICLE_FLAGS_LOCALACCELERATION)
				{
					Movement += Acceleration * _ParticleTime * 2.0f;
					Movement.MultiplyMatrix3x3(LocalToWorld);
				}
				else
				{
					Movement.MultiplyMatrix3x3(LocalToWorld);
					Movement += Acceleration * _ParticleTime * 2.0f;
				}

				RenderParticleAlighed(Particle, Movement * m_ParticleAlignLengthScale);
			}
			else
			{
				RenderParticle(Particle);
			}

			m_NumParticles++;
		}
		else
		{
			m_bOverflow = true;
		}
	}

	//----------------------------------------------------------------------

	void
	GenerateParticles()
	{
		MAUTOSTRIP(ParticlesClassName_GenerateParticles, MAUTOSTRIP_VOID);
		fp32 precalc0 = m_ParticleTimecellDuration * m_ParticleTimecellSpread; // TimeCellOffset
		fp32 precalc1 = m_ParticleEmissionProbability_TimeScale * 0.7f; // 
		fp32 precalc2 = m_ParticleEmissionProbability_TimeScale * 1.0f; // 
		fp32 precalc3 = m_ParticleEmissionProbability_TimeScale * 1.3f; // 
		
		int CellRandseedBase = m_RandseedBase + m_NumTotalCells;

		for (int iCell = 0; iCell < m_NumCellsToBacktrace; iCell++) {
			for (int iParticle = 0; iParticle < m_NumParticlesPerTimecell; iParticle++)
			{
				m_Randseed = CellRandseedBase - iCell + iParticle * 100;

				if (m_Randseed & m_IDDisableMask)
					continue;

				fp32 SeparationTime = GetRand(m_Randseed) * precalc0;
				fp32 ParticleTime = m_Time - fp32(m_NumTotalCells - iCell) * m_ParticleTimecellDuration - SeparationTime;
				fp32 EmissionTime = (m_Time - ParticleTime);

				fp32 Probability = 1.0f;
				if (m_ParticleEmissionProbability < 1.0f)
				{
					fp32 Scale;
					Scale = g_SinRandTable.GetRand(EmissionTime * precalc1); Probability *= Scale;
					Scale = g_SinRandTable.GetRand(EmissionTime * precalc2); Probability *= Scale;
					Scale = g_SinRandTable.GetRand(EmissionTime * precalc3); Probability *= Scale;
				}

				if (Probability <= m_ParticleEmissionProbability) {
					m_Randseed = CellRandseedBase - iCell + iParticle * 100;

					if ((EmissionTime >= 0.0f) && ((m_ParticleEmissionStop == 0.0f) || (EmissionTime < m_ParticleEmissionStop)))
						GenerateParticle(ParticleTime);
				}
			}
		}
	}
	
	//----------------------------------------------------------------------

	void ModifyEntry(CMat43fp32& _Matrix, fp32& _Time)
	{
		MAUTOSTRIP(ParticlesClassName_ModifyEntry, MAUTOSTRIP_VOID);
		if (m_Flags & PARTICLE_FLAGS_WORLDROT)
			_Matrix.Unit3x3();

		_Time += m_TimeOffset;
		_Time *= m_TimeScale;

		if (m_DistributionRotation != 0)
		{
			_Matrix.M_x_RotZ(m_DistributionRotation[2] * _Time);
			_Matrix.M_x_RotY(m_DistributionRotation[1] * _Time);
			_Matrix.M_x_RotX(m_DistributionRotation[0] * _Time);
		}
	}

	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		MAUTOSTRIP(ParticlesClassName_Render, MAUTOSTRIP_VOID);
//		M_CALLGRAPH;
		MSCOPE(Mondelore, Mondelore);

		if (!(m_Flags & PARTICLE_FLAGS_NOLOD))
		{
			CVec3Dfp32 WPos = CVec3Dfp32::GetRow(_WMat, 3);
			WPos *= _VMat;

			fp32 Dist = WPos.Length();
			if (m_pEngine)
				Dist = GetCorrectedDistance(m_pEngine, Dist);

			if (Dist > 4000.0f)
				return;

			m_lIDFade[0] = GetDepthFade(Dist, 200, 300);
			m_lIDFade[1] = GetDepthFade(Dist, 550, 650);
			m_lIDFade[2] = GetDepthFade(Dist, 850, 1000);
			m_lIDFade[3] = GetDepthFade(Dist, 1800, 2000);
			m_lIDFade[4] = GetDepthFade(Dist, 3700, 4000);
			m_IDDisableMask = 0;
			if (m_lIDFade[0] < 0.001f)
				m_IDDisableMask |= 1;
			if (m_lIDFade[1] < 0.001f)
				m_IDDisableMask |= 2;
			if (m_lIDFade[2] < 0.001f)
				m_IDDisableMask |= 4;
			if (m_lIDFade[3] < 0.001f)
				m_IDDisableMask |= 8;
		}
		else
		{
			m_lIDFade[0] = 1;
			m_lIDFade[1] = 1;
			m_lIDFade[2] = 1;
			m_lIDFade[3] = 1;
			m_lIDFade[4] = 1;
			m_IDDisableMask = 0;
		}
		
		m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));

		// Will be max accumulated during particle generation.
		m_BoundRadius = 0;
		m_NumParticles = 0;
		m_bOverflow = false;

//		m_Time = (_pAnimState->m_AnimTime0 + MoveForth) * m_TimeScale;
//		m_Duration = _pAnimState->m_AnimTime1;
		
		if (false)
			ConOutL(CStrF("Render: Time = %f (Unscaled = %f)", m_Time, _pAnimState->m_AnimTime0));

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

		m_LocalToWorld = _WMat;

		m_Time = _pAnimState->m_AnimTime0;
		m_Duration = _pAnimState->m_AnimTime1;
		
		if (m_pHistory != NULL)
		{
			CMat43fp32 Matrix = _WMat;
			int Flags = _pAnimState->m_Colors[3];

			ModifyEntry(Matrix, m_Time);
			m_pHistory->AddEntry(Matrix, m_Time, Flags);
		}
		else
		{
			CMat43fp32 Matrix;
			ModifyEntry(Matrix, m_Time);
			if (m_Flags & PARTICLE_FLAGS_WORLDROT)
				m_LocalToWorld.Unit3x3();
		}

/*
		if (m_Duration > 0)
			m_TimeFraction = Clamp01(m_Time / m_Duration);
		else
			m_TimeFraction = m_Time;
*/

		if (m_Flags & PARTICLE_FLAGS_NOHISTORY)
		{
			if (m_DistributionRotation != 0)
			{
				m_LocalToWorld.M_x_RotZ(m_DistributionRotation[2] * m_Time);
				m_LocalToWorld.M_x_RotY(m_DistributionRotation[1] * m_Time);
				m_LocalToWorld.M_x_RotX(m_DistributionRotation[0] * m_Time);
			}
		}

/*
		if (false && (m_pHistory != NULL))
		{
			if (false)
				ConOutL(CStrF("Render: Time = %f", m_Time));

			CMat4Dfp32 Pos;
			fp32 Speed;
			int Flags;
			if (m_pHistory->GetInterpolatedMatrix(m_Time, Pos, Speed, Flags))
			{
				if ((m_pWC != NULL) && false)
				{
					m_pWC->RenderWire(CVec3Dfp32::GetMatrixRow(Pos, 3), 0, 0xFFFF0000, 0.5f);
				}
				if (false)
					ConOutL(CStr("Render: History->GetIPMatrix Successfull!"));
			}
			else
			{
				if (false)
					ConOutL(CStr("Render: History->GetIPMatrix Failed!"));
			}
			if (false) return;
		}
*/

		if ((m_ParticleEmissionStop > 0) && (m_Time > (m_ParticleEmissionStop + m_MaxParticleDuration)))
			return;

		if (false)
			ConOutL(CStrF("Render: Time = %f, OldestTime = %f", m_Time, m_Time - m_MaxParticleDuration));

		m_DurationFade = ::GetFade(m_Time / m_TimeScale, m_Duration, m_FadeInTime, m_FadeOutTime);

		m_RandseedBase = _pAnimState->m_Anim0;
		m_Randseed = m_RandseedBase;

		CMat43fp32 InvWorldToCamera;
		_VMat.InverseOrthogonal(InvWorldToCamera);
		m_CameraLeft = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 0);
		m_CameraUp = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 1);
		m_CameraFwd = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 2);
		m_CameraPos = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 3);

		if ((m_Flags & PARTICLE_FLAGS_FADESTILL) && ((m_pHistory != NULL) && m_pHistory->IsSingularity()))
			return;

		// Calculate the number of time cells needed to cover the whole duration of a particle.
		// Each time cell is PARTICLE_MIN_EMISSION_DELAY long in time.
		m_NumCellsToBacktrace = TruncToInt(m_MaxParticleDuration / m_ParticleTimecellDuration) + 2;

		// Calculate the total number of cell that has been passed during full lifetime of this model.
		m_NumTotalCells = TruncToInt(m_Time / m_ParticleTimecellDuration);

//		int CalculatedMaxNumParticles = m_NumCellsToBacktrace * m_NumParticlesPerTimecell;

		{
			fp32 MaxSize = Max(Max(m_SizeCtrl.m_StartMin, m_SizeCtrl.m_StartMax), Max(m_SizeCtrl.m_EndMin, m_SizeCtrl.m_EndMax));
			if (MaxSize > 15)
				m_Flags |= PARTICLE_FLAGS_QUADS;

			// Check camera distance here too...
		}

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

		m_DVB.GetVB()->m_Priority += m_RenderPriorityBias;
		
//		if (m_pWC != NULL)
//			m_pWC->RenderMatrix(m_LocalToWorld, 10);

		m_BBox_bIsValid = false;

		GenerateParticles();


		if (m_bOverflow && (m_Flags & PARTICLE_FLAGS_SHOWOVERFLOW))
			m_SK.Create(GetSurfaceContext(), m_pEngine, _pAnimState, 0);
		else
			m_SK.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface);

		if (false)
			ConOut(CStrF("NumParticles = %d", m_NumParticles));

		if (m_BBox_bIsValid && (m_pHistory != NULL))
		{
//			if (m_pWC != NULL)
//				m_pWC->RenderMatrix(_WMat, 20);

			fp32 MaxSize = Max(m_SizeCtrl.m_StartMax, m_SizeCtrl.m_EndMax); 
			m_BBox.m_Min -= MaxSize;
			m_BBox.m_Max += MaxSize;

			CMat43fp32 WorldToLocal;
			_WMat.InverseOrthogonal(WorldToLocal);
			CBox3Dfp32 BBLocal;
			m_BBox.Transform(WorldToLocal, BBLocal);
			m_pHistory->StoreBoundBox(BBLocal);
		}

		if (m_DVB.IsValid())
		{
			m_DVB.Render(_VMat);
			m_SK.Render(m_DVB.GetVB(), m_pVBM, m_pEngine);
		}
	}

	//----------------------------------------------------------------------

	void OnEvalKey(const CRegistry *_pReg)
	{
		MAUTOSTRIP(ParticlesClassName_OnEvalKey, MAUTOSTRIP_VOID);
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

		else if ((Name == "TIMESCALE") || (Name == "TS"))
			m_TimeScale = Valuef;

		else if ((Name == "TIMEOFFSET") || (Name == "TO"))
			m_TimeOffset = Valuef;

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

		else if ((Name == "STOPMOTIONTIME") || (Name == "SMT"))
			m_Particle_StopMotionTime = Valuef;

		else if ((Name == "LOCALOFFSET") || (Name == "LO"))
			m_LocalPositionOffset.ParseString(Value);

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

		else if ((Name == "LENGTHSCALE") || (Name == "LS"))
			m_ParticleAlignLengthScale = Valuef;

		else if ((Name == "COLOR") || (Name == "CO"))
			m_ParticleColor.ParseColor(Value);
		else if ((Name == "COLORNOISE") || (Name == "CON"))
			m_ParticleColorNoise.ParseColor(Value);

		else if ((Name == "EMISSIONPROBABILITY") || (Name == "EP"))
			m_ParticleEmissionProbability = Valuef;
		else if ((Name == "EMISSIONPROBABILITYCHANGE") || (Name == "EPC"))
			m_ParticleEmissionProbability_TimeScale = Valuef;
		else if ((Name == "TIMECELLDURATION") || (Name == "TC"))
			m_ParticleTimecellDuration = Valuef;
		else if ((Name == "PARTICLESPERTIMECELL") || (Name == "PP"))
			m_NumParticlesPerTimecell = Valuei;
		else if ((Name == "TIMECELLSPREAD") || (Name == "TCS"))
			m_ParticleTimecellSpread = Valuef;
		else if ((Name == "EMISSIONSTOP") || (Name == "ES"))
			m_ParticleEmissionStop = Valuef;

		else if ((Name == "FLAGS") || (Name == "FL"))
		{
			m_Flags = Value.TranslateFlags(lpPARTICLE_FLAGS);
			m_Flags |= Value.TranslateFlags(lpPARTICLE_FLAGS_SHORT);
		}

		else if ((Name == "DISTRIBUTION") || (Name == "DI"))
			m_DistributionPrimitive = Value.TranslateInt(lpPRIMITIVE);
		else if ((Name == "DISTRIBUTIONSIZE") || (Name == "DIS"))
			m_DistributionSize.ParseString(Value);
		else if ((Name == "HOLLOWDISTRIBUTION") || (Name == "HO"))
			m_bHollowDistribution = Valuei != 0;
		else if ((Name == "DISTRIBUTIONROTATION") || (Name == "DR"))
			m_DistributionRotation.ParseString(Value);

		else
			CXR_Model_Custom::OnEvalKey(_pReg);
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *_keys)
	{
		MAUTOSTRIP(ParticlesClassName_OnCreate, MAUTOSTRIP_VOID);
		m_Keys = _keys;
		SetParameters_Default();

		ParseKeys(_keys);

		ComputeParameters();
	}

	//----------------------------------------------------------------------

	void ComputeParameters()
	{
		MAUTOSTRIP(ParticlesClassName_ComputeParameters, MAUTOSTRIP_VOID);
		m_RenderPriorityBias += TransparencyPriorityBaseBias;

		// Typically some values lay in certain intervals, so to reduce the need of
		// redundant digits we multiply or divide by 10.
		m_DistributionSize *= 10.0f;
		m_DistributionRotation *= 1.0f / 10.0f;
		m_MoveCtrl.m_FluctTimescale /= 10.0f;
		m_SizeCtrl.m_FluctTimescale /= 10.0f;
		m_RotCtrl.m_FluctTimescale /= 10.0f;

		m_MoveCtrl.Optimize();
		m_SizeCtrl.Optimize();
		m_RotCtrl.Optimize();
		m_AlphaCtrl.Optimize();

		m_Particle_InvStopMotionTime = 1.0f / m_Particle_StopMotionTime;

		m_InvParticleSlowdownPower = 1.0f / m_ParticleSlowdownPower;
		m_InvParticleSlowdownPowerPlusOne = m_InvParticleSlowdownPower + 1.0f;
		m_InvInvParticleSlowdownPowerPlusOne = 1.0f / m_InvParticleSlowdownPowerPlusOne;

		m_InvParticleFadeInTime = 1.0f / m_ParticleFadeInTime;

		m_FadeInTime = m_ParticleFadeInTime / m_TimeScale * 2.0f;
		m_FadeOutTime = m_ParticleFadeInTime / m_TimeScale * 2.0f;

		m_MaxParticleDuration = m_ParticleDuration + m_ParticleDurationNoise / 2.0f;

		// Why -2?
		m_HistoryEntryDelay = 1.2f * m_MaxParticleDuration / (fp32)(HISTORY_LENGTH - 2);

		m_ParticleAlignLengthScale /= 100.0f;

		if ((m_ParticleOffset != 0) || (m_ParticleOffsetNoise != 0))
			m_bOffset = true;

		if ((m_ParticleVelocity != 0) || (m_ParticleVelocityNoise != 0))
			m_bVelocity = true;

		if ((m_ParticleAcceleration != 0) || (m_ParticleAccelerationNoise != 0))
			m_bAccelerate = true;

		if (m_Flags & PARTICLE_FLAGS_ALIGN_SAFEQUAD)
			m_Flags |= PARTICLE_FLAGS_ALIGN;
	}
	
	//----------------------------------------------------------------------

	// Default
	void
	SetParameters_Default()
	{
		MAUTOSTRIP(ParticlesClassName_SetParameters_Default, MAUTOSTRIP_VOID);
		m_iSurface = GetSurfaceID("DotParticle");
		m_RenderPriorityBias = 0;
		
		m_Flags = 0;

		m_TimeScale = 1;
		m_TimeOffset = 0;

		m_SinRandTimeSpread = 100.0f;
		m_MoveCtrl.m_FluctTimeSpread = 100.0f;

		m_MaxNumParticles = 500;

		m_DistributionPrimitive = PRIMITIVE_POINT;
		m_DistributionSize = 0;
		m_DistributionRotation = 0;
		m_bHollowDistribution = false;

		m_ParticleDuration = 3.0f;
		m_ParticleDurationNoise = 0.0f;

		m_ParticleOffset = 0.0f;
		m_ParticleOffsetNoise = 0.0f;

		m_ParticleVelocity = 0.0f;
		m_ParticleVelocityNoise = 0.0f;
		m_ParticleSlowdownPower = 0.0f;

		m_Particle_StopMotionTime = 0.0f;
		m_Particle_InvStopMotionTime = 0.0f;

		m_LocalPositionOffset = 0;

		m_ParticleAcceleration = 0;
		m_ParticleAccelerationNoise = 0;

		m_AngleOffsetA = 0.0f;
		m_AngleOffsetB = 0.0f;
		m_AngleSpreadA = 1.0f;
		m_AngleSpreadB = 1.0f;
		m_AngleOffsetChangeA = 0.0f;
		m_AngleOffsetChangeB = 0.0f;

		m_ParticleEmissionProbability = 1.0f;
		m_ParticleEmissionProbability_TimeScale = 1.0f;

		m_ParticleTimecellDuration = 0.01f;
		m_NumParticlesPerTimecell = 1;
		m_ParticleTimecellSpread = 1.0f;
		m_ParticleEmissionStop = 0.0f;

		m_ParticleFadeInTime = 0.0f;
		m_ParticleColor = 255; // 0xFFFFFFFF
		m_ParticleColorNoise = 0; // 0x00000000

		m_ParticleFadeStillThreshold = 1.0f;
		m_ParticleAlignLengthScale = 1.0f;

		m_SizeCtrl.Set(10.0f);

		m_AlphaCtrl.SetStart(1.0f);
		m_AlphaCtrl.SetEnd(0.0f);

		m_bOffset = false;
		m_bVelocity = false;
		m_bAccelerate = false;
	}

};

//----------------------------------------------------------------------

#ifdef TESTNEWPARTICLES
	MRTC_IMPLEMENT_DYNAMIC(CXR_Model_ParticlesOpt, CXR_Model_Custom);
#else
	MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Particles, CXR_Model_Custom);
#endif

//----------------------------------------------------------------------
