#include "PCH.h"

#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "CDynamicVB.h"
#include "CSurfaceKey.h"
#include "CModelHistory2.h"
#include "CSinRandTable.h"
#include "CPropertyControl.h"
#include "ModelsMisc.h"
#include "MSIMD.h"
#include "MMath_Vec128.h"

#ifdef	PLATFORM_PS2
#include "../../../../Shared/MOS/RndrPS2/MRndrPS2_DmaEngine.h"
#endif

#include "WModel_ParticlesOpt.h"

static const char* g_lpTranslatePrimitive[] = { "point", "cube", "sphere", "tube", "cylinder", "pyramid", "tetraid", NULL };
static const char* g_lpTranslateParticleFlags[] = { "fadestill",	"slowdown", "nohistory", "quads", "primcenter", "worldrot", "hflags", "spline", "align", "ctime", "erl", "ihflags", "nolod", "localaxx", "stopmotion", "noclampcolors", "showoverflow", "nochanneledcolornoise", "ets", "asq", "ae", "smo", "lighting", "randomseed", NULL };
static const char* g_lpTranslateParticleFlagsShort[] = { "fs",	"sd", "nh", "qd", "pc", "wr", "hf", "sp", "al", "ct", "fe", "ihf", "nl", "la", "sm", "ncc", "sof", "nccn", "ets", "asq", "ae", "smo", "lt", "rs", NULL };

union FloatInt { fp32 f; uint32 i; }; 


#ifdef PLATFORM_XENON

# define CountLeadingZeros(x) _CountLeadingZeros(x)
# define CountTrailingZeros(x) (32 - _CountLeadingZeros(~(x) & ((x) - 1)))

/*#elif defined(PLATFORM_PS3) -- TODO: enable if correct

# define CountLeadingZeros(x) _cntlzw(x)
# define CountTrailingZeros(x) (32 - _cntlzw(~(x) & ((x) - 1)))
*/
#else

//TODO: use BSF and BSR?
M_INLINE uint CountLeadingZeros(uint32 _Value)
{
	uint n;
	for (n = 0; n < 32; n++, _Value <<= 1)
		if (_Value & 0x80000000) break;
	return n;
}

M_INLINE uint CountTrailingZeros(uint32 _Value)
{
	uint n;
	for (n = 0; n < 32; n++, _Value >>= 1)
		if (_Value & 1) break;
	return n;
}
#endif

ParticlesOptClassName::ParticlesOptClassName()
{
	SetThreadSafe(true);
}

/*
		// ----------------------------------------------------------------------
		Operations:

		Mul:				Dst = Src1 * Src2
		Combine:			Dst = Src1 + Src2 * Src3

		// ----------------------------------------------------------------------
		Source postfixes:

		Vector (CVec4Dfp32)	V
		Scalar (fp32)		S
		Matrix (CMat4Dfp32)	M

		// ----------------------------------------------------------------------
		Source suffixes:

		Array				<none>
		Constant			c
		Random [-0.5..0.5]	r		(Source data is always uint32 randomseeds regardless of source postfix type)

		// ----------------------------------------------------------------------
		NOTES:

		- Array pointers must be 16-byte aligned.

		- Constants must be dword aligned		
*/

// These didn't have any measureable effect


static void AnglesToVector2(fp32 a, fp32 b, CVec3Dfp32& v)
{
	MAUTOSTRIP(AnglesToVector2, MAUTOSTRIP_VOID);
	fp32 sina, cosa, sinb, cosb;

	QSinCos(a * 2.0f * _PI, sina, cosa);
	QSinCos(b * 2.0f * _PI, sinb, cosb);
	v.k[0] = cosa * cosb;
	v.k[1] = sina * cosb;
	v.k[2] = sinb;
}

aint ParticlesOptClassName::GetParam(int _Param)
{
	MAUTOSTRIP(ParticlesOptClassName_GetParam, 0);
	if (_Param == CXR_MODEL_PARAM_TIMEMODE)
	{
		if (m_SystemFlags & PARTICLE_FLAGS_CONTINUOUSTIME)
			return CXR_MODEL_TIMEMODE_CONTINUOUS;
		else
			return CXR_MODEL_TIMEMODE_CONTROLLED;
	}
//	if( _Param == CXR_MODEL_PARAM_ISALIVE )
//		return ( m_OptFlags & OPTFLAGS_ISALIVE ) != 0;

	return CXR_Model_Custom::GetParam(_Param);
}

//----------------------------------------------------------------------

CModelData* ParticlesOptClassName::AllocData()
{
	MAUTOSTRIP(ParticlesOptClassName_AllocData, NULL);
	return MNew(CModelData);
}

//----------------------------------------------------------------------

CMHistory* ParticlesOptClassName::AllocHistory()
{
	MAUTOSTRIP(ParticlesOptClassName_AllocHistory, NULL);
	return MNew2(CMHistory, m_HistoryEntryDelay, 1.0f * m_TimeScale);
}

//----------------------------------------------------------------------

TPtr<CXR_ModelInstance> ParticlesOptClassName::CreateModelInstance()
{
	MAUTOSTRIP(ParticlesOptClassName_CreateModelInstance, NULL);
	MSCOPE(ParticlesOptClassName::CreateModelInstance, CUSTOMMODELS);
	CModelData* pData;
	if (m_SystemFlags & PARTICLE_FLAGS_NOHISTORY)
		pData = AllocData();
	else
		pData = AllocHistory();

	// FIXME: For easier debugging only. Should be remove.
	if (pData == NULL)
		return NULL;

	return pData;
}

//----------------------------------------------------------------------

CModelData*
ParticlesOptClassName::GetData(const CXR_AnimState* _pAnimState) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetData, NULL);
	if (_pAnimState == NULL)
		return NULL;

	if (_pAnimState->m_pModelInstance == NULL)
		return NULL;

	
	const CXR_ModelInstance* pModelInstance = (const CXR_ModelInstance*)_pAnimState->m_pModelInstance;
	CModelData* pData = (CModelData*)(safe_cast<const CModelData>(pModelInstance));

	// FIXME: For easier debugging only. Should be remove.
	if (pData == NULL)
		return NULL;

	return pData;
}

//----------------------------------------------------------------------

// Get history from clientdata.
CMHistory*
ParticlesOptClassName::GetHistory(const CXR_AnimState* _pAnimState) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetHistory, NULL);
	CModelData* pData = GetData(_pAnimState);
	if (pData == NULL)
		return NULL;

	CMHistory* pHistory = TDynamicCast<CMHistory>(pData);
	return pHistory;
}

//----------------------------------------------------------------------

#ifndef M_RTM


CModelData*
ParticlesOptClassName::ForceGetData(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(ParticlesOptClassName_ForceGetData, NULL);
	if (_pAnimState == NULL)
		return NULL;

	M_ASSERT(true, "Expired");

/*	if (_pAnimState->m_pspClientData == NULL)
		return NULL;


	// Check if the clientdata is used for something else...
	CReferenceCount* RefCount = (CReferenceCount*)(*_pAnimState->m_pspClientData);
	if ((RefCount != NULL) && (TDynamicCast<CModelData>(RefCount) == NULL))
		return NULL;

	// Correct type, use it...
	CModelData* pData = (CModelData*)RefCount;

	// No history was allocated, so allocate it now...
	if (pData == NULL) {
		*_pAnimState->m_pspClientData = AllocData();
		if (*_pAnimState->m_pspClientData == NULL) return NULL;
		pData = (CModelData*)(CReferenceCount*)(*_pAnimState->m_pspClientData);
	}

	return pData;*/
	return NULL;
}

//----------------------------------------------------------------------

CMHistory*
ParticlesOptClassName::ForceGetHistory(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(ParticlesOptClassName_ForceGetHistory, NULL);
	if (_pAnimState == NULL)
		return NULL;

	M_ASSERT(true, "Expired");

/*	if (_pAnimState->m_pspClientData == NULL)
		return NULL;

	// Check if the clientdata is used for something else...
	CReferenceCount* RefCount = (CReferenceCount*)(*_pAnimState->m_pspClientData);
	if ((RefCount != NULL) && (TDynamicCast<CMHistory>(RefCount) == NULL))
		return NULL;

	// Correct type, use it...
	CMHistory* pHistory = (CMHistory*)RefCount;

	// No history was allocated, so allocate it now...
	if (pHistory == NULL) {
		*_pAnimState->m_pspClientData = AllocHistory();
		if (*_pAnimState->m_pspClientData == NULL) return NULL;
		pHistory = (CMHistory*)(CReferenceCount*)(*_pAnimState->m_pspClientData);
	}

	return pHistory;*/
	return NULL;
}

#endif

//----------------------------------------------------------------------

void ParticlesOptClassName::GrowBoxAcceleration(CBox3Dfp32& _Box, const CModelData* _pData) const
{
	MAUTOSTRIP(ParticlesOptClassName_GrowBoxAcceleration, NULL);
	if (m_OptFlags & OPTFLAGS_ACCELERATION)
	{
		// 0.5f should be correct, but in some cases it doesn't fit. 0.75f is required in those cases.
		const fp32 Scale = Sqr(m_MaxParticleDuration) * 0.5f;
		if (m_SystemFlags & PARTICLE_FLAGS_LOCALACCELERATION)
		{
			CVec3Dfp32 LocalAxx = m_ParticleAcceleration;
			CVec3Dfp32 LocalAxxNoise = m_ParticleAccelerationNoise;
			LocalAxx *= Scale;
			LocalAxxNoise *= ( Scale * 0.5f );
			_Box.Grow(LocalAxx + LocalAxxNoise);
			_Box.Grow(LocalAxx - LocalAxxNoise);
		}
		else
		{
			if ((_pData != NULL) && (_pData->m_LastWorldToLocal_bIsValid))
			{
				CVec3Dfp32 WorldAxx = m_ParticleAcceleration;
				WorldAxx.MultiplyMatrix3x3(_pData->m_LastWorldToLocal);

				CVec3Dfp32 WorldAxxNoise = m_ParticleAccelerationNoise * 0.5f;
				WorldAxxNoise.MultiplyMatrix3x3(_pData->m_LastWorldToLocal);

				WorldAxx *= Scale;
				WorldAxxNoise *= Scale;
				_Box.Grow(WorldAxx + WorldAxxNoise);
				_Box.Grow(WorldAxx - WorldAxxNoise);
			}
			else
			{
				// Fallback: Grow accelration in all directions (Should only happend if we've ran out of memory to allocate even a CModelData).
				const fp32 WorldAxx = m_ParticleAcceleration.Length() + m_ParticleAccelerationNoise.Length() * ( 0.5f * Scale ); 
				_Box.Grow(WorldAxx);
			}
		}
	}
}

//----------------------------------------------------------------------

void ParticlesOptClassName::GrowBoxAngledOffset(CBox3Dfp32& _Box, fp32 _MaxOffset) const
{
	MAUTOSTRIP(ParticlesOptClassName_GrowBoxAngledOffset, MAUTOSTRIP_VOID);
	CVec3Dfp32 x(1, 0, 0);
	CVec3Dfp32 y(0, 1, 0);
	CVec3Dfp32 z(0, 0, 1);

	if (m_OptFlags & OPTFLAGS_ANGLES)
	{
		// Angles
		fp32 AngleAMin = 0;
		fp32 AngleAMid = 0;
		fp32 AngleAMax = 0;
		fp32 AngleARange = 0;
		bool AngleAFull = false;
		bool AngleAZero = false;
		fp32 AngleBMin = 0;
		fp32 AngleBMid = 0;
		fp32 AngleBMax = 0;
		fp32 AngleBRange = 0;
		bool AngleBFull = false;
		bool AngleBZero = false;

		if (m_AngleOffsetChangeA != 0)
			AngleAFull = true;
		else
		{
			AngleAMin = m_AngleOffsetA - M_Fabs(m_AngleSpreadA * 0.5f);
			AngleAMax = m_AngleOffsetA + M_Fabs(m_AngleSpreadA * 0.5f);
			AngleAMid = (AngleAMin + AngleAMax) * 0.5f;
			AngleARange = M_Fabs(AngleAMax - AngleAMin);
			if (AngleARange >= 0.8f)
				AngleAFull = true;
			else if ((AngleARange == 0) && (AngleAMid == 0))
				AngleAZero = true;
		}

		if (m_AngleOffsetChangeB != 0)
			AngleBFull = true;
		else
		{
			AngleBMin = m_AngleOffsetB - M_Fabs(m_AngleSpreadB * 0.5f);
			AngleBMax = m_AngleOffsetB + M_Fabs(m_AngleSpreadB * 0.5f);
			AngleBMid = (AngleBMin + AngleBMax) * 0.5f;
			AngleBRange = M_Fabs(AngleBMax - AngleBMin);
			if (AngleBRange >= 0.8f)
				AngleBFull = true;
			else if ((AngleBRange == 0) && (AngleBMid == 0))
				AngleBZero = true;
		}

		if (AngleAZero)
		{
			if (AngleBZero)
			{
				// Will never reach here (since this case makes m_bAngles == false).
				_Box.Grow(x * _MaxOffset);
			}
			else if (AngleBFull)
			{
				_Box.Grow(x * _MaxOffset);
				_Box.Grow(-x * _MaxOffset);
				_Box.Grow(z * _MaxOffset);
				_Box.Grow(-z * _MaxOffset);
			}
			else 
			{
				CVec3Dfp32 BDirMin, BDirMid, BDirMax;
				AnglesToVector2(0, AngleBMin, BDirMin);
				AnglesToVector2(0, AngleBMid, BDirMid);
				AnglesToVector2(0, AngleBMax, BDirMax);
				fp32 BMinX, BMaxX, BMinZ, BMaxZ;
				BMinX = Min3(BDirMin[0], BDirMid[0], BDirMax[0]);
				BMaxX = Max3(BDirMin[0], BDirMid[0], BDirMax[0]);
				BMinZ = Min3(BDirMin[2], BDirMid[2], BDirMax[2]);
				BMaxZ = Max3(BDirMin[2], BDirMid[2], BDirMax[2]);

				if ((BMinX * BMaxX) < 0.0f) // Make sure min/max don't grow in same direction.
					_Box.Grow(x * BMinX * _MaxOffset);
				_Box.Grow(x * BMaxX * _MaxOffset);

				if ((BMinZ * BMaxZ) < 0.0f) // Make sure min/max don't grow in same direction.
					_Box.Grow(z * BMinZ * _MaxOffset);
				_Box.Grow(z * BMaxZ * _MaxOffset);
			}
		}
		else if (AngleAFull)
		{
			if (AngleBZero)
			{
				_Box.Grow(x * _MaxOffset);
				_Box.Grow(-x * _MaxOffset);
				_Box.Grow(y * _MaxOffset);
				_Box.Grow(-y * _MaxOffset);
			}
			else if (AngleBFull)
			{
				_Box.Grow(_MaxOffset);
			}
			else 
			{
				CVec3Dfp32 BDirMin, BDirMid, BDirMax;
				AnglesToVector2(0, AngleBMin, BDirMin);
				AnglesToVector2(0, AngleBMid, BDirMid);
				AnglesToVector2(0, AngleBMax, BDirMax);
				fp32 BMaxX, BMinZ, BMaxZ;
				BMaxX = Max3(M_Fabs(BDirMin[0]), M_Fabs(BDirMid[0]), M_Fabs(BDirMax[0]));
				BMinZ = Min3(BDirMin[2], BDirMid[2], BDirMax[2]);
				BMaxZ = Max3(BDirMin[2], BDirMid[2], BDirMax[2]);

				_Box.Grow(x * BMaxX * _MaxOffset);
				_Box.Grow(-x * BMaxX * _MaxOffset);
				_Box.Grow(y * BMaxX * _MaxOffset);
				_Box.Grow(-y * BMaxX * _MaxOffset);

				if ((BMinZ * BMaxZ) < 0.0f) // Make sure min/max don't grow in same direction.
					_Box.Grow(z * BMinZ * _MaxOffset);
				_Box.Grow(z * BMaxZ * _MaxOffset);
			}
		}
		else
		{
			if (AngleBZero)
			{
				CVec3Dfp32 ADirMin, ADirMid, ADirMax;
				AnglesToVector2(AngleAMin, 0, ADirMin);
				AnglesToVector2(AngleAMid, 0, ADirMid);
				AnglesToVector2(AngleAMax, 0, ADirMax);
				fp32 AMinX, AMaxX, AMinY, AMaxY;
				AMinX = Min3(ADirMin[0], ADirMid[0], ADirMax[0]);
				AMaxX = Max3(ADirMin[0], ADirMid[0], ADirMax[0]);
				AMinY = Min3(ADirMin[1], ADirMid[1], ADirMax[1]);
				AMaxY = Max3(ADirMin[1], ADirMid[1], ADirMax[1]);

				if ((AMinX * AMaxX) < 0) // Make sure min/max don't grow in same direction.
					_Box.Grow(x * AMinX * _MaxOffset);
				_Box.Grow(x * AMaxX * _MaxOffset);

				if ((AMinY * AMaxY) < 0) // Make sure min/max don't grow in same direction.
					_Box.Grow(y * AMinY * _MaxOffset);
				_Box.Grow(y * AMaxY * _MaxOffset);
			}
			else if (AngleBFull)
			{
				CVec3Dfp32 ADirMin, ADirMid, ADirMax;
				AnglesToVector2(AngleAMin, 0, ADirMin);
				AnglesToVector2(AngleAMid, 0, ADirMid);
				AnglesToVector2(AngleAMax, 0, ADirMax);
				fp32 AMaxX, AMaxY;
				AMaxX = Max3(M_Fabs(ADirMin[0]), M_Fabs(ADirMid[0]), M_Fabs(ADirMax[0]));
				AMaxY = Max3(M_Fabs(ADirMin[1]), M_Fabs(ADirMid[1]), M_Fabs(ADirMax[1]));

				_Box.Grow(x * AMaxX * _MaxOffset);
				_Box.Grow(-x * AMaxX * _MaxOffset);
				_Box.Grow(y * AMaxY * _MaxOffset);
				_Box.Grow(-y * AMaxY * _MaxOffset);
				_Box.Grow(z * _MaxOffset);
				_Box.Grow(-z * _MaxOffset);
			}
			else 
			{
				CVec3Dfp32 ADirMin, ADirMid, ADirMax;
				AnglesToVector2(AngleAMin, 0, ADirMin);
				AnglesToVector2(AngleAMid, 0, ADirMid);
				AnglesToVector2(AngleAMax, 0, ADirMax);
				fp32 AMaxX, AMaxY;
				AMaxX = Max3(M_Fabs(ADirMin[0]), M_Fabs(ADirMid[0]), M_Fabs(ADirMax[0]));
				AMaxY = Max3(M_Fabs(ADirMin[1]), M_Fabs(ADirMid[1]), M_Fabs(ADirMax[1]));
/*
				fp32 AMinX, AMaxX, AMinY, AMaxY;
				AMinX = Min3(ADirMin[0], ADirMid[0], ADirMax[0]);
				AMaxX = Max3(ADirMin[0], ADirMid[0], ADirMax[0]);
*/
				CVec3Dfp32 BDirMin, BDirMid, BDirMax;
				AnglesToVector2(0, AngleBMin, BDirMin);
				AnglesToVector2(0, AngleBMid, BDirMid);
				AnglesToVector2(0, AngleBMax, BDirMax);
				fp32 BMinX, BMaxX, BMinZ, BMaxZ;
				BMinX = Min3(BDirMin[0], BDirMid[0], BDirMax[0]);
				BMaxX = Max3(BDirMin[0], BDirMid[0], BDirMax[0]);
				BMinZ = Min3(BDirMin[2], BDirMid[2], BDirMax[2]);
				BMaxZ = Max3(BDirMin[2], BDirMid[2], BDirMax[2]);

				if ((BMinX * BMaxX) < 0)
					_Box.Grow(x * AMaxX * BMinX * _MaxOffset);
				_Box.Grow(x * AMaxX * BMaxX * _MaxOffset);

				_Box.Grow(y * AMaxY * BMaxX * _MaxOffset);
				_Box.Grow(-y * AMaxY * BMaxX * _MaxOffset);
				
				if ((BMinZ * BMaxZ) < 0.0f) // Make sure min/max don't grow in same direction.
					_Box.Grow(z * BMinZ * _MaxOffset);
				_Box.Grow(z * BMaxZ * _MaxOffset);
			}
		}
	}
	else
	{
		_Box.Grow(x * _MaxOffset);
	}
}

//----------------------------------------------------------------------

void ParticlesOptClassName::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(ParticlesOptClassName_GetBound_Box, MAUTOSTRIP_VOID);
	// History and Acceleration
	CMHistory* pHistory = GetHistory(_pAnimState);
	if (pHistory != NULL)
	{
		if (pHistory->HasValidBoundBox())
			_Box = pHistory->GetBoundBox();
		else
		{
			_Box.m_Min = 0;
			_Box.m_Max = 0;
		}
		GrowBoxAcceleration(_Box, pHistory);
	}
	else
	{
		// No boundbox available (nor needed).
		_Box.m_Min = 0;
		_Box.m_Max = 0;
		CModelData* pData = GetData(_pAnimState);
		GrowBoxAcceleration(_Box, pData);
	}

	// Offset
	fp32 MaxVelocity;
	if (m_OptFlags & OPTFLAGS_VELOCITY)
	{
		if (m_ParticleVelocity >= 0)
			MaxVelocity = m_ParticleVelocity + M_Fabs(m_ParticleVelocityNoise * 0.5f);
		else
			MaxVelocity = m_ParticleVelocity - M_Fabs(m_ParticleVelocityNoise * 0.5f);
		
		if (m_SystemFlags & PARTICLE_FLAGS_SLOWDOWN)
		{
/*
			fp32 Velocity = (m_ParticleVelocity + m_ParticleVelocityNoise * GetRandS(_pBatch->m_iRandseed[iParticle]));
			Velocity = Velocity * (_pBatch->m_TimeFraction[iParticle] - M_Pow(_pBatch->m_TimeFraction[iParticle], m_InvParticleSlowdownPowerPlusOne) * m_InvInvParticleSlowdownPowerPlusOne) / _pBatch->m_Time[iParticle];
			_pBatch->m_Offset[iParticle] += Velocity * _pBatch->m_Time[iParticle];
			_pBatch->m_Movement[iParticle][0] = Velocity;
*/
			MaxVelocity = MaxVelocity * (1.0f - m_InvInvParticleSlowdownPowerPlusOne);
		}
		else if (m_SystemFlags & PARTICLE_FLAGS_STOPMOTION)
		{
/*
			fp32 SMTimeFraction = Clamp01(pTime[iParticle] * Particle_InvStopMotionTime);
			fp32 SMTimeFraction2 = Sqr(SMTimeFraction);
			fp32 SMTime = SMTimeFraction2 * Particle_StopMotionTime;
			fp32 Time = Max(0.0f, Min(Particle_StopMotionTime * 0.5f, pTime[iParticle] - SMTime * 0.5f));
			pOffset[iParticle] += pTempScalar[iParticle] * Time;
			pMovement[iParticle][0] = pTempScalar[iParticle] * (1.0f - Time * (Particle_InvStopMotionTime * 2.0f));
*/
			MaxVelocity *= Min(m_Particle_StopMotionTime * 0.5f, m_MaxParticleDuration);
		}
		else
		{
			MaxVelocity *= m_MaxParticleDuration;
		}
	}
	else
		MaxVelocity = 0;

	fp32 MaxOffset;
	if (m_OptFlags & OPTFLAGS_OFFSET)
	{
		if (m_ParticleOffset >= 0)
			MaxOffset = m_ParticleOffset + M_Fabs(m_ParticleOffsetNoise * 0.5f);
		else
			MaxOffset = m_ParticleOffset - M_Fabs(m_ParticleOffsetNoise * 0.5f);

		MaxOffset = Max(M_Fabs(MaxOffset), M_Fabs(MaxOffset + MaxVelocity));
	}
	else
		MaxOffset = MaxVelocity;

	GrowBoxAngledOffset(_Box, MaxOffset);

	// Misc
	const fp32 MaxDistSize = Max3(m_DistributionSize[0], m_DistributionSize[1], m_DistributionSize[2]) * 0.5f;
	const fp32 MaxMove = Max(Max(M_Fabs(m_MoveCtrl.m_StartMin), M_Fabs(m_MoveCtrl.m_StartMax)), Max(M_Fabs(m_MoveCtrl.m_EndMin), M_Fabs(m_MoveCtrl.m_EndMax)));
	const fp32 MaxSize = Max(Max(m_SizeCtrl.m_StartMin, m_SizeCtrl.m_StartMax), Max(m_SizeCtrl.m_EndMin, m_SizeCtrl.m_EndMax)) * 0.5f;
	const fp32 Growth = MaxDistSize + MaxMove + MaxSize;
	_Box.Grow(Growth);

	// Offset whole boundbox in localspace.
	_Box.m_Min += m_LocalPositionOffset;
	_Box.m_Max += m_LocalPositionOffset;
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

void ParticlesOptClassName::GetCubePoint(CVec3Dfp32& _point, uint32& _iRandseed) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetCubePoint, MAUTOSTRIP_VOID);
	_point[0] = GetRandS(_iRandseed);
	_point[1] = GetRandS(_iRandseed);
	_point[2] = GetRandS(_iRandseed);
}

//----------------------------------------------------------------------

const fp32 ParticlesOptClassName::GetRandSRadius(uint32& _iRandseed) const
{
	// Apply random radius. This will also scale to signed unit.
	// InvSqrInv is used to bias the distribution towards larger radius, since those have larger circumference.
	// Two step rand is needed, since InvSqrInv is a repeating macro.
	MAUTOSTRIP(ParticlesOptClassName_GetRandSRadius, 0);

	const fp32 distribution = GetRand(_iRandseed);
	const fp32 radius = 0.5f * M_Sqrt(1.0f - Sqr(1.0f - distribution));
	return radius;

}

//----------------------------------------------------------------------

void ParticlesOptClassName::GetSpherePoint(CVec3Dfp32& _point, uint32& _iRandseed) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetSpherePoint, MAUTOSTRIP_VOID);
	// Set random direction.
	AnglesToVector2(GetRandS(_iRandseed), GetRandS(_iRandseed), _point);

	_point *= GetRandSRadius(_iRandseed);
}

//----------------------------------------------------------------------

void ParticlesOptClassName::GetCylinderSlicePoint(CVec3Dfp32& _point, fp32 _height, uint32& _iRandseed) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetCylinderSlicePoint, MAUTOSTRIP_VOID);
	// Set random direction in XY plane.
	AnglesToVector2(GetRandS(_iRandseed), 0, _point);

	_point *= GetRandSRadius(_iRandseed);

	// Set slice height.
	_point[2] = _height; 
}

//----------------------------------------------------------------------

void ParticlesOptClassName::GetTubePoint(CVec3Dfp32& _point, uint32& _iRandseed) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetTubePoint, MAUTOSTRIP_VOID);
	GetCylinderSlicePoint(_point, GetRandS(_iRandseed), _iRandseed);
}

//----------------------------------------------------------------------

void ParticlesOptClassName::GetCylinderPoint(CVec3Dfp32& _point, uint32& _iRandseed) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetCylinderPoint, MAUTOSTRIP_VOID);
	GetTubePoint(_point, _iRandseed);
}

//----------------------------------------------------------------------

void ParticlesOptClassName::GetPyramidPoint(CVec3Dfp32& _point, uint32& _iRandseed) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetPyramidPoint, MAUTOSTRIP_VOID);
	// Not yet implemented.
	_point = 0;
}

//----------------------------------------------------------------------

void ParticlesOptClassName::GetTetraidPoint(CVec3Dfp32& _point, uint32& _iRandseed) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetTetraidPoint, MAUTOSTRIP_VOID);
	// Not yet implemented.
	_point = 0;
}

//----------------------------------------------------------------------

void ParticlesOptClassName::GetHollowCubePoint(CVec3Dfp32& _point, uint32& _iRandseed) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetHollowCubePoint, MAUTOSTRIP_VOID);
	const fp32 u = GetRandS(_iRandseed);
	const fp32 v = GetRandS(_iRandseed);
	const fp32 w = 0.5f;
	
	int side = TruncToInt(GetRand(_iRandseed) * 5.99f); // Random integer between 0 and 5.
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

void ParticlesOptClassName::GetHollowSpherePoint(CVec3Dfp32& _point, uint32& _iRandseed) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetHollowSpherePoint, MAUTOSTRIP_VOID);
	// Set random direction.
	AnglesToVector2(GetRandS(_iRandseed), GetRandS(_iRandseed), _point);
	_point *= 0.5f; // Scale radius to signed unit.
}

//----------------------------------------------------------------------

void ParticlesOptClassName::GetHollowTubePoint(CVec3Dfp32& _point, uint32& _iRandseed) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetHollowTubePoint, MAUTOSTRIP_VOID);
	// Set random direction in XY plane.
	AnglesToVector2(GetRandS(_iRandseed), 0, _point);
	_point *= 0.5f; // Scale radius to signed unit.

	// Set random height.
	_point[2] = GetRandS(_iRandseed);
}

//----------------------------------------------------------------------

void ParticlesOptClassName::GetHollowCylinderPoint(CVec3Dfp32& _point, uint32& _iRandseed) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetHollowCylinderPoint, MAUTOSTRIP_VOID);
	// The surface area of a hollow unit cylinder is 25% for each of the two caps and 50% for the "height" surface.
	const int side = TruncToInt(GetRand(_iRandseed) * 3.99f); // Random integer between 0 and 3.
	switch (side)
	{
		case 0: GetCylinderSlicePoint(_point, +0.5f, _iRandseed); break; // 25% for top cap.
		case 1: GetCylinderSlicePoint(_point, -0.5f, _iRandseed); break; // 25% for bottom cap.
		default: GetHollowTubePoint(_point, _iRandseed); break; // 50% for the rest, i.e. the "height" surface.
	}
}

//----------------------------------------------------------------------

void ParticlesOptClassName::GetHollowPyramidPoint(CVec3Dfp32& _point, uint32& _iRandseed) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetHollowPyramidPoint, MAUTOSTRIP_VOID);
	// Not yet implemented.
	_point = 0;
}

//----------------------------------------------------------------------

void ParticlesOptClassName::GetHollowTetraidPoint(CVec3Dfp32& _point, uint32& _iRandseed) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetHollowTetraidPoint, MAUTOSTRIP_VOID);
	// Not yet implemented.
	_point = 0;
}

//----------------------------------------------------------------------

void ParticlesOptClassName::GetDistributedPoint(CVec3Dfp32& _point, uint32& _iRandseed) const
{
	MAUTOSTRIP(ParticlesOptClassName_GetDistributedPoint, MAUTOSTRIP_VOID);
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
			case PRIMITIVE_CUBE: GetHollowCubePoint(_point, _iRandseed); break;
			case PRIMITIVE_SPHERE: GetHollowSpherePoint(_point, _iRandseed); break;
			case PRIMITIVE_CYLINDER: GetHollowCylinderPoint(_point, _iRandseed); break;
			case PRIMITIVE_TUBE: GetHollowTubePoint(_point, _iRandseed); break;
			case PRIMITIVE_PYRAMID: GetHollowPyramidPoint(_point, _iRandseed); break;
		}
	}
	else
	{
		switch (m_DistributionPrimitive)
		{
			case PRIMITIVE_CUBE: GetCubePoint(_point, _iRandseed); break;
			case PRIMITIVE_SPHERE: GetSpherePoint(_point, _iRandseed); break;
			case PRIMITIVE_CYLINDER: GetCylinderPoint(_point, _iRandseed); break;
			case PRIMITIVE_TUBE: GetHollowTubePoint(_point, _iRandseed); break;
			case PRIMITIVE_PYRAMID: GetPyramidPoint(_point, _iRandseed); break;
		}
	}
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

void ParticlesOptClassName::RenderParticleWH_ZeroRot(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderParticleWH_ZeroRot, MAUTOSTRIP_VOID);

	CVec3Dfp32 w;
	CVec3Dfp32 h;
	const fp32 Size = _pBatch->m_Size[_iParticle] * 0.5f;
	_Left.Scale(Size, w);
	_Up.Scale(Size, h);

	CPixel32 color = *((CPixel32*)&(_pBatch->m_Alpha[_iParticle]));

	const int iV = _iParticle*3;
	const CVec4Dfp32& Pos = _pBatch->m_Pos[_iParticle];
	CVec3Dfp32* pV = &_pPR->m_pChain->m_pV[iV];
	pV[0][0] = Pos[0] - w[0] - h[0];
	pV[0][1] = Pos[1] - w[1] - h[1];
	pV[0][2] = Pos[2] - w[2] - h[2];

	pV[1][0] = Pos[0] + 3.0f*w[0] - h[0];
	pV[1][1] = Pos[1] + 3.0f*w[1] - h[1];
	pV[1][2] = Pos[2] + 3.0f*w[2] - h[2];

	pV[2][0] = Pos[0] - w[0] + 3.0f*h[0];
	pV[2][1] = Pos[1] - w[1] + 3.0f*h[1];
	pV[2][2] = Pos[2] - w[2] + 3.0f*h[2];

	CPixel32* pCol = &_pPR->m_pChain->m_pCol[iV];
	pCol[0] = color;
	pCol[1] = color;
	pCol[2] = color;


//	if (_pPR->m_pWC != NULL)
//	{
//		_pPR->m_pWC->RenderWire(0, (CVec3Dfp32&)_pBatch->m_Pos[_iParticle], 0xFF00FF00, 0.05f);
//		_pPR->m_pWC->RenderWire(0, _pPR->m_pChain->m_pV[_iParticle*3], 0xFFFF0000, 0.05f);
//	}

}

void ParticlesOptClassName::RenderParticleWH_Rot(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderParticleWH_Rot, MAUTOSTRIP_VOID);

	fp32 s, c;
	QSinCos(_pBatch->m_Rot[_iParticle] * TORAD, s, c);

	const fp32 Size = _pBatch->m_Size[_iParticle] * 0.5f;
	CVec3Dfp32 w, h;
	w[0] = (_Left[0] * c - _Up[0] * s) * Size;
	w[1] = (_Left[1] * c - _Up[1] * s) * Size;
	w[2] = (_Left[2] * c - _Up[2] * s) * Size;
	h[0] = (_Left[0] * s + _Up[0] * c) * Size;
	h[1] = (_Left[1] * s + _Up[1] * c) * Size;
	h[2] = (_Left[2] * s + _Up[2] * c) * Size;

	CPixel32 color = *((CPixel32*)&(_pBatch->m_Alpha[_iParticle]));

	const int iV = _iParticle*3;
	const CVec4Dfp32& Pos = _pBatch->m_Pos[_iParticle];
	CVec3Dfp32* pV = &_pPR->m_pChain->m_pV[iV];
	pV[0][0] = Pos[0] - w[0] - h[0];
	pV[0][1] = Pos[1] - w[1] - h[1];
	pV[0][2] = Pos[2] - w[2] - h[2];

	pV[1][0] = Pos[0] + 3.0f*w[0] - h[0];
	pV[1][1] = Pos[1] + 3.0f*w[1] - h[1];
	pV[1][2] = Pos[2] + 3.0f*w[2] - h[2];

	pV[2][0] = Pos[0] - w[0] + 3.0f*h[0];
	pV[2][1] = Pos[1] - w[1] + 3.0f*h[1];
	pV[2][2] = Pos[2] - w[2] + 3.0f*h[2];

	CPixel32* pCol = &_pPR->m_pChain->m_pCol[iV];
	pCol[0] = color;
	pCol[1] = color;
	pCol[2] = color;
}

void ParticlesOptClassName::RenderParticleWH(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderParticleWH, MAUTOSTRIP_VOID);

	if (m_RotCtrl.m_ValueType & CPropertyControl::VALUETYPE_ZERO)
	{
		RenderParticleWH_ZeroRot(_pPR, _pBatch, _iParticle, _Left, _Up);
	}
	else
	{
		RenderParticleWH_Rot(_pPR, _pBatch, _iParticle, _Left, _Up);
	}
}

void ParticlesOptClassName::RenderQuadParticleWH_ZeroRot(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderQuadParticleWH_ZeroRot, MAUTOSTRIP_VOID);

	CVec3Dfp32 w;
	CVec3Dfp32 h;
	const fp32 Size = _pBatch->m_Size[_iParticle] * 0.5f;
	_Left.Scale(Size, w);
	_Up.Scale(Size, h);

	CPixel32 color = *((CPixel32*)&(_pBatch->m_Alpha[_iParticle]));

	const int iV = _iParticle*4;
	const CVec4Dfp32& Pos = _pBatch->m_Pos[_iParticle];
	CVec3Dfp32* pV = &_pPR->m_pChain->m_pV[iV];
	pV[0][0] = Pos[0] - w[0] - h[0];
	pV[0][1] = Pos[1] - w[1] - h[1];
	pV[0][2] = Pos[2] - w[2] - h[2];

	pV[1][0] = Pos[0] + w[0] - h[0];
	pV[1][1] = Pos[1] + w[1] - h[1];
	pV[1][2] = Pos[2] + w[2] - h[2];

	pV[2][0] = Pos[0] + w[0] + h[0];
	pV[2][1] = Pos[1] + w[1] + h[1];
	pV[2][2] = Pos[2] + w[2] + h[2];

	pV[3][0] = Pos[0] - w[0] + h[0];
	pV[3][1] = Pos[1] - w[1] + h[1];
	pV[3][2] = Pos[2] - w[2] + h[2];

	CPixel32* pCol = &_pPR->m_pChain->m_pCol[iV];
	pCol[0] = color;
	pCol[1] = color;
	pCol[2] = color;
	pCol[3] = color;
}

void ParticlesOptClassName::RenderQuadParticleWH_Rot(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderQuadParticleWH_Rot, MAUTOSTRIP_VOID);

	fp32 s, c;
	QSinCos(_pBatch->m_Rot[_iParticle] * TORAD, s, c);

	const fp32 Size = _pBatch->m_Size[_iParticle] * 0.5f;
	CVec3Dfp32 w, h;
	w[0] = (_Left[0] * c - _Up[0] * s) * Size;
	w[1] = (_Left[1] * c - _Up[1] * s) * Size;
	w[2] = (_Left[2] * c - _Up[2] * s) * Size;
	h[0] = (_Left[0] * s + _Up[0] * c) * Size;
	h[1] = (_Left[1] * s + _Up[1] * c) * Size;
	h[2] = (_Left[2] * s + _Up[2] * c) * Size;

	CPixel32 color = *((CPixel32*)&(_pBatch->m_Alpha[_iParticle]));

	const int iV = _iParticle*4;
	const CVec4Dfp32& Pos = _pBatch->m_Pos[_iParticle];
	CVec3Dfp32* pV = &_pPR->m_pChain->m_pV[iV];
	pV[0][0] = Pos[0] - w[0] - h[0];
	pV[0][1] = Pos[1] - w[1] - h[1];
	pV[0][2] = Pos[2] - w[2] - h[2];

	pV[1][0] = Pos[0] + w[0] - h[0];
	pV[1][1] = Pos[1] + w[1] - h[1];
	pV[1][2] = Pos[2] + w[2] - h[2];

	pV[2][0] = Pos[0] + w[0] + h[0];
	pV[2][1] = Pos[1] + w[1] + h[1];
	pV[2][2] = Pos[2] + w[2] + h[2];

	pV[3][0] = Pos[0] - w[0] + h[0];
	pV[3][1] = Pos[1] - w[1] + h[1];
	pV[3][2] = Pos[2] - w[2] + h[2];

	CPixel32* pCol = &_pPR->m_pChain->m_pCol[iV];
	pCol[0] = color;
	pCol[1] = color;
	pCol[2] = color;
	pCol[3] = color;
}

void ParticlesOptClassName::RenderQuadParticleWH(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderQuadParticleWH, MAUTOSTRIP_VOID);

	if (m_RotCtrl.m_ValueType & CPropertyControl::VALUETYPE_ZERO)
	{
		RenderQuadParticleWH_ZeroRot(_pPR, _pBatch, _iParticle, _Left, _Up);
	}
	else
	{
		RenderQuadParticleWH_Rot(_pPR, _pBatch, _iParticle, _Left, _Up);
	}
}

void ParticlesOptClassName::RenderQuadParticleArrayWH(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderQuadParticleArrayWH, MAUTOSTRIP_VOID);
	uint iParticle;

	if (m_RotCtrl.m_ValueType & CPropertyControl::VALUETYPE_ZERO)
	{

		CVec4Dfp32 w;
		CVec4Dfp32 h;
		w[0] = _Left[0] * 0.5f;
		w[1] = _Left[1] * 0.5f;
		w[2] = _Left[2] * 0.5f;
		w[3] = 0;
		h[0] = _Up[0] * 0.5f;
		h[1] = _Up[1] * 0.5f;
		h[2] = _Up[2] * 0.5f;
		h[3] = 0;

		SIMD_ParticleQuad_VSVcVc_V3(_pPR->m_NumParallelParticles, _pBatch->m_Pos, _pBatch->m_Size, w, h, _pPR->m_pChain->m_pV);

		CPixel32* pCol = _pPR->m_pChain->m_pCol;
		const CPixel32* pAlpha = (CPixel32*)_pBatch->m_Alpha;
		const int nParticles = _pPR->m_NumParallelParticles;
		for (iParticle = 0; iParticle < nParticles; iParticle++)
		{
			CPixel32 color = pAlpha[iParticle];
			pCol[iParticle*4 + 0] = color;
			pCol[iParticle*4 + 1] = color;
			pCol[iParticle*4 + 2] = color;
			pCol[iParticle*4 + 3] = color;
		}
	}
	else
	{
//		CMTime T;
//		T.Start();

//		for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
//			RenderQuadParticleWH_Rot(_pPR, _pBatch, iParticle, _Left, _Up);

		uint nParticles = _pPR->m_NumParallelParticles;

		CVec4Dfp32 Left(_Left[0], _Left[1], _Left[2], 0);
		CVec4Dfp32 Up(_Up[0], _Up[1], _Up[2], 0);

		const CPixel32* pSrcCol = ((CPixel32*)&(_pBatch->m_Alpha[0]));
		const vec128* pSrcPos = &_pBatch->m_Pos[0].v;
		const fp32* pSrcRot = _pBatch->m_Rot+0;
		const fp32* pSrcSize = _pBatch->m_Size+0;

		CPixel32* M_RESTRICT pVCol = _pPR->m_pChain->m_pCol;
		CVec3Dfp32* M_RESTRICT pV = _pPR->m_pChain->m_pV;

		vec128 vleft = Left.v;
		vec128 vup = Up.v;

		uint iParticle = nParticles;
		vec128 sinmask = M_VConst_u32(0xffffffff, 0xffffffff, 0x00000000, 0x00000000);
		for (; iParticle > 1; iParticle -= 2)
		{
			vec128 sc = M_VSinCos_Est(M_VSelMsk(sinmask, M_VLdScalar(pSrcRot[0]), M_VLdScalar(pSrcRot[1])));
			vec128 p0_s = M_VSplatX(sc);
			vec128 p0_c = M_VSplatY(sc);
			vec128 p1_s = M_VSplatZ(sc);
			vec128 p1_c = M_VSplatW(sc);

			vec128 p0_vsize = M_VMul(M_VHalf(), M_VLdScalar(pSrcSize[0]));
			vec128 p0_vpos = pSrcPos[0];

			CPixel32 p0_Col = pSrcCol[0];

			vec128 p0_vsin = M_VMul(p0_vsize, p0_s);
			vec128 p0_vcos = M_VMul(p0_vsize, p0_c);

			vec128 p0_w = M_VSub(M_VMul(vleft, p0_vcos), M_VMul(vup, p0_vsin));
			vec128 p0_h = M_VAdd(M_VMul(vleft, p0_vsin), M_VMul(vup, p0_vcos));

			vec128 p0_wph = M_VAdd(p0_w, p0_h);
			vec128 p0_wmh = M_VSub(p0_w, p0_h);

			vec128 p0_v0 = M_VSub(p0_vpos, p0_wph);
			vec128 p0_v1 = M_VAdd(p0_vpos, p0_wmh);
			vec128 p0_v2 = M_VAdd(p0_vpos, p0_wph);
			vec128 p0_v3 = M_VSub(p0_vpos, p0_wmh);


			vec128 p1_vsize = M_VMul(M_VHalf(), M_VLdScalar(pSrcSize[1]));
			vec128 p1_vpos = pSrcPos[1];

			CPixel32 p1_Col = pSrcCol[1];

			vec128 p1_vsin = M_VMul(p1_vsize, p1_s);
			vec128 p1_vcos = M_VMul(p1_vsize, p1_c);

			vec128 p1_w = M_VSub(M_VMul(vleft, p1_vcos), M_VMul(vup, p1_vsin));
			vec128 p1_h = M_VAdd(M_VMul(vleft, p1_vsin), M_VMul(vup, p1_vcos));

			vec128 p1_wph = M_VAdd(p1_w, p1_h);
			vec128 p1_wmh = M_VSub(p1_w, p1_h);

			vec128 p1_v0 = M_VSub(p1_vpos, p1_wph);
			vec128 p1_v1 = M_VAdd(p1_vpos, p1_wmh);
			vec128 p1_v2 = M_VAdd(p1_vpos, p1_wph);
			vec128 p1_v3 = M_VSub(p1_vpos, p1_wmh);

			pVCol[0] = p0_Col; pVCol[1] = p0_Col; pVCol[2] = p0_Col; pVCol[3] = p0_Col;
			pVCol[4] = p1_Col; pVCol[5] = p1_Col; pVCol[6] = p1_Col; pVCol[7] = p1_Col;
			M_VSt_V3x4(pV, p0_v0, p0_v1, p0_v2, p0_v3);
			M_VSt_V3x4(pV+4, p1_v0, p1_v1, p1_v2, p1_v3);

			pSrcPos += 2;
			pSrcCol += 2;
			pSrcRot += 2;
			pSrcSize += 2;
			pVCol += 8;
			pV += 8;
		}

		if(iParticle > 0)
		{
			vec128 sc = M_VSinCos_Est(M_VLdScalar(pSrcRot[0]));
			vec128 s = M_VSplatX(sc);
			vec128 c = M_VSplatY(sc);

			vec128 vsize = M_VMul(M_VHalf(), M_VLdScalar(pSrcSize[0]));
			vec128 vpos = pSrcPos[0];

			CPixel32 Col = *pSrcCol;
			pVCol[0] = Col; pVCol[1] = Col; pVCol[2] = Col; pVCol[3] = Col;

			vec128 vsin = M_VMul(vsize, s);
			vec128 vcos = M_VMul(vsize, c);

			vec128 w = M_VSub(M_VMul(vleft, vcos), M_VMul(vup, vsin));
			vec128 h = M_VAdd(M_VMul(vleft, vsin), M_VMul(vup, vcos));

			vec128 wph = M_VAdd(w, h);
			vec128 wmh = M_VSub(w, h);

			vec128 v0 = M_VSub(vpos, wph);
			vec128 v1 = M_VAdd(vpos, wmh);
			vec128 v2 = M_VAdd(vpos, wph);
			vec128 v3 = M_VSub(vpos, wmh);

			M_VSt_V3x4(pV, v0, v1, v2, v3);
		}

//		T.Stop();

//		_pPR->m_TimeDebug += T;
//		_pPR->m_DebugCount += _pPR->m_NumParallelParticles;
	}
}

void ParticlesOptClassName::RenderParticleArrayWH(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderParticleArrayWH, MAUTOSTRIP_VOID);
	int iParticle;

	if (m_RotCtrl.m_ValueType & CPropertyControl::VALUETYPE_ZERO)
	{
		CVec4Dfp32 w;
		CVec4Dfp32 h;
		w[0] = _Left[0] * 0.5f;
		w[1] = _Left[1] * 0.5f;
		w[2] = _Left[2] * 0.5f;
		w[3] = 0;
		h[0] = _Up[0] * 0.5f;
		h[1] = _Up[1] * 0.5f;
		h[2] = _Up[2] * 0.5f;
		h[3] = 0;

		SIMD_ParticleTriangle_VSVcVc_V3(_pPR->m_NumParallelParticles, _pBatch->m_Pos, _pBatch->m_Size, w, h, _pPR->m_pChain->m_pV);

		CPixel32* pCol = _pPR->m_pChain->m_pCol;
		const CPixel32* pAlpha = (CPixel32*)_pBatch->m_Alpha;
		const int nParticles = _pPR->m_NumParallelParticles;
		for (iParticle = 0; iParticle < nParticles; iParticle++)
		{
			CPixel32 color = pAlpha[iParticle];
			pCol[iParticle*3 + 0] = color;
			pCol[iParticle*3 + 1] = color;
			pCol[iParticle*3 + 2] = color;
/*
			if (_pPR->m_pWC != NULL)
			{
				_pPR->m_pWC->RenderWire(0, _pPR->m_pChain->m_pV[iParticle*3+0], 0xFFFF0000, 0.05f);
				_pPR->m_pWC->RenderWire(0, _pPR->m_pChain->m_pV[iParticle*3+1], 0xFF00FF00, 0.05f);
				_pPR->m_pWC->RenderWire(0, _pPR->m_pChain->m_pV[iParticle*3+2], 0xFF0000FF, 0.05f);
			}
*/
		}
	}
	else
	{
		for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
			RenderParticleWH_Rot(_pPR, _pBatch, iParticle, _Left, _Up);
	}
}

void ParticlesOptClassName::RenderParticleArray(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderParticleArray, MAUTOSTRIP_VOID);
	RenderParticleArrayWH(_pPR, _pBatch, _pPR->m_CameraLeft, _pPR->m_CameraUp);
}

void ParticlesOptClassName::RenderParticle(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderParticle, MAUTOSTRIP_VOID);
	RenderParticleWH(_pPR, _pBatch, _iParticle, _pPR->m_CameraLeft, _pPR->m_CameraUp);
}

void ParticlesOptClassName::RenderQuadParticle(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderQuadParticle, MAUTOSTRIP_VOID);
	RenderQuadParticleWH(_pPR, _pBatch, _iParticle, _pPR->m_CameraLeft, _pPR->m_CameraUp);
}

void ParticlesOptClassName::RenderQuadParticleArray(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderQuadParticleArray, MAUTOSTRIP_VOID);
	RenderQuadParticleArrayWH(_pPR, _pBatch, _pPR->m_CameraLeft, _pPR->m_CameraUp);
}

// FIXME: Movement == 0, division by zero, N/A errors.
void ParticlesOptClassName::RenderParticleAligned(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderParticleAligned, MAUTOSTRIP_VOID);
	// Up is particle forward...

	CVec3Dfp32 Dir;
	Dir[0] = _pBatch->m_Movement[_iParticle][0];
	Dir[1] = _pBatch->m_Movement[_iParticle][1];
	Dir[2] = _pBatch->m_Movement[_iParticle][2];
	Dir.Normalize();
	CVec3Dfp32 Left; Dir.CrossProd(_pPR->m_CameraFwd, Left);
	Left.Normalize();

	RenderParticleWH(_pPR, _pBatch, _iParticle, Left, (CVec3Dfp32&)_pBatch->m_Movement[_iParticle]);
}

void ParticlesOptClassName::RenderQuadParticleAligned(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderQuadParticleAligned, MAUTOSTRIP_VOID);
	// Up is particle forward...

	CVec3Dfp32 Dir;
	Dir[0] = _pBatch->m_Movement[_iParticle][0];
	Dir[1] = _pBatch->m_Movement[_iParticle][1];
	Dir[2] = _pBatch->m_Movement[_iParticle][2];
	Dir.Normalize();
	CVec3Dfp32 Left; Dir.CrossProd(_pPR->m_CameraFwd, Left);
	Left.Normalize();

	RenderQuadParticleWH(_pPR, _pBatch, _iParticle, Left, (CVec3Dfp32&)_pBatch->m_Movement[_iParticle]);
}

void ParticlesOptClassName::RenderParticleAlignedSafeQuad(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderParticleAlignedSafeQuad, MAUTOSTRIP_VOID);
	// Up is particle forward...

	CVec3Dfp32 Dir;
	Dir[0] = _pBatch->m_Movement[_iParticle][0];
	Dir[1] = _pBatch->m_Movement[_iParticle][1];
	Dir[2] = _pBatch->m_Movement[_iParticle][2];
	Dir.Normalize();
	CVec3Dfp32 Left; Dir.CrossProd(_pPR->m_CameraFwd, Left);
	Left.Normalize();

	CVec3Dfp32 CameraToPosDir = (const CVec3Dfp32&)_pBatch->m_Pos[_iParticle] - _pPR->m_CameraPos;
	CameraToPosDir.Normalize();
	const fp32 CameraAlignment = M_Fabs(CameraToPosDir * Dir);
	CVec3Dfp32 CameraUp; 
	CameraToPosDir.CrossProd(Left, CameraUp);
	CVec3Dfp32 Up = LERP((const CVec3Dfp32&)_pBatch->m_Movement[_iParticle], CameraUp, CameraAlignment);

	RenderParticleWH(_pPR, _pBatch, _iParticle, Left, Up);
}

void ParticlesOptClassName::RenderQuadParticleAlignedSafeQuad(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle)
{
	MAUTOSTRIP(ParticlesOptClassName_RenderQuadParticleAlignedSafeQuad, MAUTOSTRIP_VOID);
	// Up is particle forward...

	CVec3Dfp32 Dir = (const CVec3Dfp32&)_pBatch->m_Movement[_iParticle]; Dir.Normalize();
	CVec3Dfp32 Left; Dir.CrossProd(_pPR->m_CameraFwd, Left);
	Left.Normalize();

	CVec3Dfp32 CameraToPosDir = (const CVec3Dfp32&)_pBatch->m_Pos[_iParticle] - _pPR->m_CameraPos;
	CameraToPosDir.Normalize();
	const fp32 CameraAlignment = M_Fabs(CameraToPosDir * Dir);
	CVec3Dfp32 CameraUp; 
	CameraToPosDir.CrossProd(Left, CameraUp);
	CVec3Dfp32 Up = LERP((const CVec3Dfp32&)_pBatch->m_Movement[_iParticle], CameraUp, CameraAlignment);

	RenderQuadParticleWH(_pPR, _pBatch, _iParticle, Left, Up);
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

M_INLINE fp32 ParticlesOptClassName::GetDepthFade(fp32 _Distance, fp32 _Start, fp32 _End)
{
	MAUTOSTRIP(ParticlesOptClassName_GetDepthFade, 0);
	return Clamp01((_End - _Distance) / (_End - _Start));
}


//----------------------------------------------------------------------

M_INLINE uint ParticlesOptClassName::RemoveParticle(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, uint _iParticle, uint _nParallelParticles)
{
	MAUTOSTRIP(ParticlesOptClassName_RemoveParticle, MAUTOSTRIP_VOID);
	uint idx = _nParallelParticles - 1;
	{
		_pBatch->m_ID[_iParticle] =				_pBatch->m_ID[idx];
		_pBatch->m_iRandseed[_iParticle] =		_pBatch->m_iRandseed[idx];
		_pBatch->m_EmissionTime[_iParticle] =	_pBatch->m_EmissionTime[idx];
		_pBatch->m_Time[_iParticle] =			_pBatch->m_Time[idx];
		_pBatch->m_TimeFraction[_iParticle] =	_pBatch->m_TimeFraction[idx];
		_pBatch->m_Offset[_iParticle] =			_pBatch->m_Offset[idx];

		_pBatch->m_Pos[_iParticle] =			_pBatch->m_Pos[idx];
		_pBatch->m_Dir[_iParticle] =			_pBatch->m_Dir[idx];
		_pBatch->m_Movement[_iParticle] =		_pBatch->m_Movement[idx];

		_pBatch->m_Rot[_iParticle] =			_pBatch->m_Rot[idx];
		_pBatch->m_Size[_iParticle] =			_pBatch->m_Size[idx];
		_pBatch->m_Alpha[_iParticle] =			_pBatch->m_Alpha[idx];
	}
	_pPR->m_NumParallelParticles = idx;
	return idx;
}

M_INLINE uint ParticlesOptClassName::RemoveParticle2(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, uint _iParticle, uint _nParallelParticles)
{
	MAUTOSTRIP(ParticlesOptClassName_RemoveParticle2, MAUTOSTRIP_VOID);
	uint idx = _nParallelParticles - 1;
	{
		_pBatch->m_ID[_iParticle] =				_pBatch->m_ID[idx];
		_pBatch->m_iRandseed[_iParticle] =		_pBatch->m_iRandseed[idx];
		_pBatch->m_EmissionTime[_iParticle] =	_pBatch->m_EmissionTime[idx];
		_pBatch->m_Time[_iParticle] =			_pBatch->m_Time[idx];
	}
	_pPR->m_NumParallelParticles = idx;
	return idx;
}

M_INLINE uint ParticlesOptClassName::RemoveParticle21(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, uint _iParticle, uint _nParallelParticles)
{
	MAUTOSTRIP(ParticlesOptClassName_RemoveParticle21, MAUTOSTRIP_VOID);
	uint idx = _nParallelParticles - 1;
	{
		_pBatch->m_ID[_iParticle] =				_pBatch->m_ID[idx];
		_pBatch->m_iRandseed[_iParticle] =		_pBatch->m_iRandseed[idx];
		_pBatch->m_EmissionTime[_iParticle] =	_pBatch->m_EmissionTime[idx];
		_pBatch->m_Time[_iParticle] =			_pBatch->m_Time[idx];
		_pBatch->m_TimeFraction[_iParticle] =	_pBatch->m_TimeFraction[idx];
	}
	_pPR->m_NumParallelParticles = idx;
	return idx;
}

M_INLINE void ParticlesOptClassName::MoveParticle3(CBatch* M_RESTRICT _pBatch, uint _iFrom, uint _iTo)
{
	_pBatch->m_ID[_iTo] =				_pBatch->m_ID[_iFrom];
	_pBatch->m_iRandseed[_iTo] =		_pBatch->m_iRandseed[_iFrom];
	_pBatch->m_EmissionTime[_iTo] =		_pBatch->m_EmissionTime[_iFrom];
	_pBatch->m_Time[_iTo] =				_pBatch->m_Time[_iFrom];
	_pBatch->m_TimeFraction[_iTo] =		_pBatch->m_TimeFraction[_iFrom];
//	_pBatch->m_Alpha[_iParticle] =		_pBatch->m_Alpha[_iFrom];
}

M_INLINE uint ParticlesOptClassName::RemoveParticle3(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, uint _iParticle, uint _nParallelParticles)
{
	MAUTOSTRIP(ParticlesOptClassName_RemoveParticle3, MAUTOSTRIP_VOID);
	int iLast = _nParallelParticles - 1;
	MoveParticle3(_pBatch, iLast, _iParticle); // replace curr with last
	_pPR->m_NumParallelParticles = iLast;
	return iLast;
}



M_INLINE uint ParticlesOptClassName::RemoveParticle4(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, uint _iParticle, uint _nParallelParticles)
{
	MAUTOSTRIP(ParticlesOptClassName_RemoveParticle4, MAUTOSTRIP_VOID);
	int idx = _nParallelParticles - 1;
	{
		_pBatch->m_ID[_iParticle] =				_pBatch->m_ID[idx];
		_pBatch->m_iRandseed[_iParticle] =		_pBatch->m_iRandseed[idx];
		_pBatch->m_EmissionTime[_iParticle] =	_pBatch->m_EmissionTime[idx];
		_pBatch->m_Time[_iParticle] =			_pBatch->m_Time[idx];
		_pBatch->m_TimeFraction[_iParticle] =	_pBatch->m_TimeFraction[idx];
		_pBatch->m_Alpha[_iParticle] =			_pBatch->m_Alpha[idx];
	}
	_pPR->m_NumParallelParticles = idx;
	return idx;
}



// -------------------------------------------------------------------
bool ParticlesOptClassName::Generate_StandardSetup(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int _GenerateFlags)
{
	MAUTOSTRIP(ParticlesOptClassName_Generate_StandardSetup, MAUTOSTRIP_VOID);

	PerRenderVariables* M_RESTRICT pPR = _pPR;
	CBatch* M_RESTRICT pBatch = _pBatch;
	uint iParticle;
	uint nParallelParticles = _pPR->m_NumParallelParticles;

//	This function shouldn't be called with a 0 count
//	if (!_pPR->m_NumParallelParticles) return true;

	if (m_ParticleEmissionStop > 0.0f)
	{
		//
		// 1. We do this test first, because it seems to be the most likely to remove particles (based on measurements)
		// 2. Also based on the high amount of removed particles, we add active slots instead of removing dead slots
		// 3. we do integer compares on float values (faster + free culling of negative values)
		// 
		FloatInt EmissionStop;
		EmissionStop.f = m_ParticleEmissionStop;
		const uint32* M_RESTRICT pEmissionTime = (uint32*)pBatch->m_EmissionTime;

		uint iDest = 0;
		for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
		{
			if (pEmissionTime[iParticle] <= EmissionStop.i)	// time >= 0.0f && time <= EmissionStop
			{
				if(iParticle != iDest)
					MoveParticle3(pBatch, iParticle, iDest);

				iDest++;
			}
		}
		nParallelParticles = pPR->m_NumParallelParticles = iDest;

		if (!nParallelParticles) 
			return true;
	}

	{
		// we compare time as integer instead of floats
		const int32* M_RESTRICT pTime = (int32*)pBatch->m_Time;
		for (iParticle = 0; iParticle < nParallelParticles; )
		{
			if (pTime[iParticle] < 0)	// pTime[iParticle] < 0.0f
			{
				nParallelParticles = RemoveParticle2(_pPR, pBatch, iParticle, nParallelParticles);				// argh#1! vc++ generates incorrect code if I use 'pPR' instead of '_pPR' here
				continue;
			}
			iParticle++;
		}
	}

	if (!nParallelParticles) 
		return true;

	if (m_ParticleEmissionProbability < 1.0f)
	{
		/*
		fp32 precalc1 = m_ParticleEmissionProbability_TimeScale * 0.7f;
		fp32 precalc2 = m_ParticleEmissionProbability_TimeScale * 1.0f;
		fp32 precalc3 = m_ParticleEmissionProbability_TimeScale * 1.3f;
		*/
		fp32 TimeScale = m_ParticleEmissionProbability_TimeScale;
		const fp32 precalc1 = TimeScale * ( 2.0f * 0.3f );
		const fp32 precalc2 = TimeScale * ( 5.0f * 0.3f );
		const fp32 precalc3 = TimeScale * ( 7.0f * 0.3f );
		fp32 ParticleEmissionProbability = m_ParticleEmissionProbability;

		for (iParticle = 0; iParticle < nParallelParticles; )
		{
			fp32 Probability = 0.0f;

//			fp32 Scale;
//			Scale = g_SinRandTable.GetRand(_pBatch->m_EmissionTime[iParticle] * precalc1);
//			Probability += (Scale - 0.5f) * 0.5f;
//			Scale = g_SinRandTable.GetRand(_pBatch->m_EmissionTime[iParticle] * precalc2);
//			Probability += (Scale - 0.5f) * 0.25f;
//			Scale = g_SinRandTable.GetRand(_pBatch->m_EmissionTime[iParticle] * precalc3);
//			Probability += (Scale - 0.5f) * 0.125f;

			fp32 EmissionTime = pBatch->m_EmissionTime[iParticle];
			Probability += ( g_SinRandTable.GetRand(EmissionTime * precalc1) - 0.5f ) * 0.5f;
			Probability += ( g_SinRandTable.GetRand(EmissionTime * precalc2) - 0.5f ) * 0.25f;
			Probability += ( g_SinRandTable.GetRand(EmissionTime * precalc3) - 0.5f ) * 0.125f;

			// Scale, since accumulated amplitudes don't sum up to 1.0f.
			// Boost with 1.2 to reach all range. Don't know why this is needed. It shouldn't.
//			Probability *= 1.2f / (0.5f + 0.25f + 0.125f);

			// Make unsigned, i.e shift from -0.5/+0.5 to 0.0/1.0.
//			Probability += 0.5f;

//			Probability = Clamp01(Probability);

			Probability = Clamp01( ( Probability * (1.2f / (0.5f + 0.25f + 0.125f)) ) + 0.5f );

			if (Probability > ParticleEmissionProbability)
			{
				nParallelParticles = RemoveParticle2(_pPR, pBatch, iParticle, nParallelParticles);
				continue;
			}
			iParticle++;
		}

		if (!nParallelParticles) 
			return true;
	}

	{
		// { _pBatch->m_TimeFraction[iParticle] = m_ParticleDuration + m_ParticleDurationNoise * GetRandS(_pBatch->m_iRandseed[iParticle]); }
		SIMD_Combine_ScScSr(nParallelParticles, m_ParticleDuration, m_ParticleDurationNoise, pBatch->m_iRandseed, pBatch->m_TimeFraction);

		// Compare ints instead of floats
		const uint32* M_RESTRICT pTime = (uint32*)pBatch->m_Time;
		const uint32* M_RESTRICT pDuration = (uint32*)pBatch->m_TimeFraction;

		for (iParticle = 0; iParticle < nParallelParticles; )
		{
			if (pTime[iParticle] > pDuration[iParticle])
			{
				nParallelParticles = RemoveParticle21(_pPR, pBatch, iParticle, nParallelParticles);
				continue;
			}
			iParticle++;
		}

		if (!nParallelParticles) 
			return true;
	}

	SIMD_Recp8(&pBatch->m_TimeFraction[0], &pBatch->m_TimeFraction[0], nParallelParticles);

//		SIMD_Mul_SS(_pPR->m_NumParallelParticles, _pBatch->m_TimeFraction, _pBatch->_pPR->m_Time, _pBatch->m_TimeFraction);
//		SIMD_Mul_VSc(_pPR->m_NumParallelParticles, _pBatch->m_Alpha, m_DurationFade, _pBatch->m_Alpha);

	{
		fp32 ParticleFadeInTime = m_ParticleFadeInTime;
		fp32 InvParticleFadeInTime = m_InvParticleFadeInTime;
		fp32 DurationFade = pPR->m_DurationFade;

		for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
		{
			fp32 Time = pBatch->m_Time[iParticle];
			fp32 TimeFraction = Time * pBatch->m_TimeFraction[iParticle];

			// Apply external duration fadeout & FadeInTime.
			//fp32 Alpha = (Time >= ParticleFadeInTime ? DurationFade : DurationFade * InvParticleFadeInTime);
			fp32 Alpha = M_FSel(Time - ParticleFadeInTime, DurationFade, Time * DurationFade * InvParticleFadeInTime);

			pBatch->m_TimeFraction[iParticle] = TimeFraction;
			pBatch->m_Alpha[iParticle] = Alpha;
		}
	}

	// LODing
	if (!(_pPR->m_Flags & PARTICLE_FLAGS_NOLOD))
	{
		const uint32* M_RESTRICT pID = pBatch->m_ID;
		fp32* M_RESTRICT pAlpha = pBatch->m_Alpha;

		fp32 lFade[33];
		lFade[0] = pPR->m_lIDFade[0];
		lFade[1] = pPR->m_lIDFade[1];
		lFade[2] = pPR->m_lIDFade[2];
		lFade[3] = pPR->m_lIDFade[3];
		lFade[32] = pPR->m_lIDFade[4];

		for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
		{
			uint ID = pID[iParticle];
			fp32 Alpha = pAlpha[iParticle];

			uint iFade = CountTrailingZeros(ID & 15);
			Alpha *= lFade[iFade];

			pAlpha[iParticle] = Alpha;
		}
	}

	if (m_AlphaCtrl.m_ValueType & CPropertyControl::VALUETYPE_LINEAR)
	{
		for (iParticle = 0; iParticle < nParallelParticles;)
		{
			fp32 Alpha = pBatch->m_Alpha[iParticle];
			Alpha *= m_AlphaCtrl.GetLinear( pBatch->m_TimeFraction[iParticle] );
			if (Alpha <= 0.0f)
			{
				nParallelParticles = RemoveParticle4(_pPR, pBatch, iParticle, nParallelParticles);
				continue;
			}
			pBatch->m_Alpha[iParticle] = Alpha;
			iParticle++;
		}
	}
	else if (m_AlphaCtrl.m_ValueType & CPropertyControl::VALUETYPE_CONSTANT)
	{
		const fp32 AlphaConstant = m_AlphaCtrl.GetConstant();
		for (iParticle = 0; iParticle < nParallelParticles;)
		{
			fp32 Alpha = pBatch->m_Alpha[iParticle] * AlphaConstant;
			if (Alpha <= 0.0f)
			{
				nParallelParticles = RemoveParticle4(_pPR, pBatch, iParticle, nParallelParticles);
				continue;
			}
			pBatch->m_Alpha[iParticle] = Alpha;
			iParticle++;
		}
	}

	if (!nParallelParticles)
		return true;

	const CMHistory* M_RESTRICT pHistory = pPR->m_pHistory;
	if ((_pPR->m_Flags & PARTICLE_FLAGS_FADESTILL) && pHistory)
	{
		const fp32 InvParticleFadeStillThresholdSqr = 1.0f / Sqr(m_ParticleFadeStillThreshold);
		fp32 *pAlpha = pBatch->m_Alpha;
		for (iParticle = 0; iParticle < nParallelParticles;)
		{
			CVec3Dfp32 Speed;
			pHistory->GetInterpolatedSpeed(pBatch->m_EmissionTime[iParticle], Speed);

			fp32 FadeStill = Clamp01(Speed.LengthSqr() * InvParticleFadeStillThresholdSqr);
			fp32 Alpha = pAlpha[iParticle];
			Alpha *= FadeStill;
			if (Alpha <= 0.0f)
			{
				nParallelParticles = RemoveParticle4(_pPR, pBatch, iParticle, nParallelParticles);
				continue;
			}
			pAlpha[iParticle] = Alpha;
			iParticle++;
		}
	}

	return true;
}


// -------------------------------------------------------------------

void ParticlesOptClassName::Generate_HistoryCulling(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int _GenerateFlags)
{
	MAUTOSTRIP(ParticlesOptClassName_Generate_HistoryCulling, MAUTOSTRIP_VOID);

	CBatch*  M_RESTRICT pBatch = _pBatch;
	PerRenderVariables* M_RESTRICT pPR = _pPR;
	const fp32* pEmissionTime = pBatch->m_EmissionTime;
	CMHistory* pHistory = pPR->m_pHistory;
	uint nParallelParticles = pPR->m_NumParallelParticles;

	for (uint iParticle = 0; iParticle < nParallelParticles;)
	{
		int HistoryFlags = 0;

		fp32 EmissionTime = pEmissionTime[iParticle];
		if (!pHistory->GetFlags(EmissionTime, HistoryFlags))
		{ // Should this really ever happen?!!
			nParallelParticles = RemoveParticle4(pPR, pBatch, iParticle, nParallelParticles);
			continue;
		}

		if ((HistoryFlags == 0) && (_pPR->m_Flags & PARTICLE_FLAGS_HISTORYFLAGS))
		{
			//M_TRACE("RemoveParticle4(%d)\n", iParticle);
			nParallelParticles = RemoveParticle4(pPR, pBatch, iParticle, nParallelParticles);
			continue;
		}
		else if ((HistoryFlags != 0) && (_pPR->m_Flags & PARTICLE_FLAGS_IHISTORYFLAGS))
		{
			nParallelParticles = RemoveParticle4(pPR, pBatch, iParticle, nParallelParticles);
			continue;
		}
		iParticle++;
	}
}

// -------------------------------------------------------------------

void ParticlesOptClassName::Generate_Velocity(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int _GenerateFlags)
{
	MAUTOSTRIP(ParticlesOptClassName_Generate_Velocity, MAUTOSTRIP_VOID);
	int iParticle;

	CBatch* M_RESTRICT pBatch = _pBatch;
	const int nParticles = _pPR->m_NumParallelParticles;
	if (_pPR->m_Flags & PARTICLE_FLAGS_STOPMOTION)
	{
//		const uint32* pRandSeed = pBatch->m_iRandseed;
		const fp32* M_RESTRICT pTime = pBatch->m_Time;
		const fp32* M_RESTRICT pTimeFraction = pBatch->m_TimeFraction;
		fp32* M_RESTRICT pOffset = pBatch->m_Offset;
		CVec4Dfp32* M_RESTRICT pMovement = pBatch->m_Movement;
		fp32* M_RESTRICT pTempScalar = pBatch->m_TempScalar;

		const fp32 Particle_StopMotionTime = m_Particle_StopMotionTime;
		const fp32 Particle_InvStopMotionTime = m_Particle_InvStopMotionTime;
//		const fp32 ParticleVelocity = m_ParticleVelocity;
//		const fp32 ParticleVelocityNoise = m_ParticleVelocityNoise;

		SIMD_Combine_ScScSr(nParticles, m_ParticleVelocity, m_ParticleVelocityNoise, pBatch->m_iRandseed, pTempScalar);

		if (!(_pPR->m_Flags & PARTICLE_FLAGS_STOPMOTIONOLD)) // New version
		{
			if (_GenerateFlags & GENERATE_FLAGS_TOTALOFFSET)
			{
				for (iParticle = 0; iParticle < nParticles; iParticle++)
				{
					const fp32 SMTimeFraction = Clamp01(pTime[iParticle] * Particle_InvStopMotionTime);
					const fp32 SMTimeFraction2 = Sqr(SMTimeFraction);
					const fp32 SMTime = SMTimeFraction2 * Particle_StopMotionTime;
					const fp32 Time = Max(0.0f, Min(Particle_StopMotionTime * 0.5f, pTime[iParticle] - SMTime * 0.5f));
					pOffset[iParticle] += pTempScalar[iParticle] * Time;
					pMovement[iParticle][0] = pTempScalar[iParticle] * (1.0f - Time * (Particle_InvStopMotionTime * 2.0f));
				}
			}
			else
			{
				for (iParticle = 0; iParticle < nParticles; iParticle++)
				{
					const fp32 SMTimeFraction = Clamp01(pTime[iParticle] * Particle_InvStopMotionTime);
					const fp32 SMTimeFraction2 = Sqr(SMTimeFraction);
					const fp32 SMTime = SMTimeFraction2 * Particle_StopMotionTime;
					const fp32 Time = Max(0.0f, Min(Particle_StopMotionTime * 0.5f, pTime[iParticle] - SMTime * 0.5f));
					pOffset[iParticle] = pTempScalar[iParticle] * Time;
					pMovement[iParticle][0] = pTempScalar[iParticle] * (1.0f - Time * (Particle_InvStopMotionTime * 2.0f));
				}
			}
		}
		else // Old ugly version
		{
			if (_GenerateFlags & GENERATE_FLAGS_TOTALOFFSET)
			{
				for (iParticle = 0; iParticle < nParticles; iParticle++)
				{
					//fp32 Velocity = (ParticleVelocity + ParticleVelocityNoise * GetRandS(pRandSeed[iParticle]));

					const fp32 SMTimeFraction = Clamp01(pTime[iParticle] * Particle_InvStopMotionTime);
					const fp32 SMTimeFraction2 = Sqr(pTimeFraction[iParticle]);
					//pOffset[iParticle] += Velocity * SMTimeFraction - Velocity * SMTimeFraction2 * 0.5f;
					//pMovement[iParticle][0] = Velocity - Velocity * SMTimeFraction;
					pOffset[iParticle] += pTempScalar[iParticle] * (SMTimeFraction - SMTimeFraction2 * 0.5f);
					pMovement[iParticle][0] = pTempScalar[iParticle] * (1.0f - SMTimeFraction);
				}
			}
			else
			{
				for (iParticle = 0; iParticle < nParticles; iParticle++)
				{
					//fp32 Velocity = (ParticleVelocity + ParticleVelocityNoise * GetRandS(pRandSeed[iParticle]));

					const fp32 SMTimeFraction = Clamp01(pTime[iParticle] * Particle_InvStopMotionTime);
					const fp32 SMTimeFraction2 = Sqr(pTimeFraction[iParticle]);
					//pOffset[iParticle] = Velocity * SMTimeFraction - Velocity * SMTimeFraction2 * 0.5f;
					//pMovement[iParticle][0] = Velocity - Velocity * SMTimeFraction;
					pOffset[iParticle] = pTempScalar[iParticle] * (SMTimeFraction - SMTimeFraction2 * 0.5f);
					pMovement[iParticle][0] = pTempScalar[iParticle] * (1.0f - SMTimeFraction);
				}
			}
		}
	}
	else if (_pPR->m_Flags & PARTICLE_FLAGS_SLOWDOWN)
	{
		if (_GenerateFlags & GENERATE_FLAGS_TOTALOFFSET)
		{
			for (iParticle = 0; iParticle < nParticles; iParticle++)
			{
				fp32 Velocity = (m_ParticleVelocity + m_ParticleVelocityNoise * GetRandS(pBatch->m_iRandseed[iParticle]));
				Velocity = Velocity * (pBatch->m_TimeFraction[iParticle] - M_Pow(pBatch->m_TimeFraction[iParticle], m_InvParticleSlowdownPowerPlusOne) * m_InvInvParticleSlowdownPowerPlusOne) / pBatch->m_Time[iParticle];
				pBatch->m_Offset[iParticle] += Velocity * pBatch->m_Time[iParticle];
				pBatch->m_Movement[iParticle][0] = Velocity;
			}
		}
		else
		{
			for (iParticle = 0; iParticle < nParticles; iParticle++)
			{
				fp32 Velocity = (m_ParticleVelocity + m_ParticleVelocityNoise * GetRandS(pBatch->m_iRandseed[iParticle]));
				Velocity = Velocity * (pBatch->m_TimeFraction[iParticle] - M_Pow(pBatch->m_TimeFraction[iParticle], m_InvParticleSlowdownPowerPlusOne) * m_InvInvParticleSlowdownPowerPlusOne) / pBatch->m_Time[iParticle];
				pBatch->m_Offset[iParticle] = Velocity * pBatch->m_Time[iParticle];
				pBatch->m_Movement[iParticle][0] = Velocity;
			}
		}
	}
	else
	{
		if (_GenerateFlags & GENERATE_FLAGS_TOTALOFFSET)
		{
			SIMD_Combine_ScScSr(nParticles, m_ParticleVelocity, m_ParticleVelocityNoise, pBatch->m_iRandseed, pBatch->m_TempScalar);
			SIMD_Combine_SSS(nParticles, pBatch->m_Offset, pBatch->m_TempScalar, pBatch->m_Time, pBatch->m_Offset);
			for (iParticle = 0; iParticle < nParticles; iParticle++)
				pBatch->m_Movement[iParticle][0] = pBatch->m_TempScalar[iParticle];

/*
			for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
			{
				fp32 Velocity = (m_ParticleVelocity + m_ParticleVelocityNoise * GetRandS(pBatch->m_iRandseed[iParticle]));
				pBatch->m_Offset[iParticle] += Velocity * _pBatch->m_Time[iParticle];
				_pBatch->m_Movement[iParticle][0] = Velocity;
			}
*/
		}
		else
		{
			SIMD_Combine_ScScSr(nParticles, m_ParticleVelocity, m_ParticleVelocityNoise, pBatch->m_iRandseed, pBatch->m_TempScalar);
//				SIMD_Mul_SS(_pPR->m_NumParallelParticles, pBatch->m_TempScalar, _pBatch->_pPR->m_Time, _pBatch->m_Offset);
			for (iParticle = 0; iParticle < nParticles; iParticle++)
			{
				pBatch->m_Offset[iParticle] = pBatch->m_TempScalar[iParticle] * pBatch->m_Time[iParticle];
				pBatch->m_Movement[iParticle][0] = pBatch->m_TempScalar[iParticle];
			}
/*
			for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
			{
				fp32 Velocity = (m_ParticleVelocity + m_ParticleVelocityNoise * GetRandS(_pBatch->m_iRandseed[iParticle]));
				_pBatch->m_Offset[iParticle] = Velocity * _pBatch->m_Time[iParticle];
				_pBatch->m_Movement[iParticle][0] = Velocity;
			}
*/
		}
	}
}

// -------------------------------------------------------------------
void ParticlesOptClassName::Generate_SizeControl(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int _GenerateFlags)
{
	MAUTOSTRIP(ParticlesOptClassName_Generate_SizeControl, MAUTOSTRIP_VOID);
	int iParticle;

	// Size Control
	if ((_pPR->m_Flags & PARTICLE_FLAGS_EMITTERTIME_SIZE) && (_pPR->m_Duration > 0))
	{
		if (m_SizeCtrl.m_ValueType & CPropertyControl::VALUETYPE_CONSTANT)
		{
			fp32* pSize = _pBatch->m_Size;
			const int nParticles = _pPR->m_NumParallelParticles;
			const fp32 Size = m_SizeCtrl.GetConstant();

			for (iParticle = 0; iParticle < nParticles; iParticle++)
				pSize[iParticle] = Size;
		}
		else if (m_SizeCtrl.m_ValueType & CPropertyControl::VALUETYPE_LINEAR)
		{
			for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
			{
				const fp32 EmissionTime = _pPR->m_Time - _pBatch->m_Time[iParticle];
				const fp32 SizeTimeFraction = EmissionTime * _pPR->m_InvDuration;
				_pBatch->m_Size[iParticle] = m_SizeCtrl.GetLinear(SizeTimeFraction);
			}
		}
		else if (m_SizeCtrl.m_ValueType & CPropertyControl::VALUETYPE_RANDOMCONSTANT)
		{
			for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
			{
				const fp32 EmissionTime = _pPR->m_Time - _pBatch->m_Time[iParticle];
				const fp32 SizeTimeFraction = EmissionTime * _pPR->m_InvDuration;
				_pBatch->m_Size[iParticle] = m_SizeCtrl.GetRandomS(SizeTimeFraction, _pBatch->m_iRandseed[iParticle]);
			}
		}
		else
		{
			for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
			{
				const fp32 EmissionTime = _pPR->m_Time - _pBatch->m_Time[iParticle];
				const fp32 SizeTimeFraction = EmissionTime * _pPR->m_InvDuration;
				const fp32 IndividualTime = _pPR->m_Time + m_SinRandTimeSpread * GetRand(_pBatch->m_iRandseed[iParticle]);
				_pBatch->m_Size[iParticle] = m_SizeCtrl.GetFluctS(IndividualTime, SizeTimeFraction, &g_SinRandTable, _pBatch->m_iRandseed[iParticle]);
			}
		}
	}
	else
	{
		if (m_SizeCtrl.m_ValueType & CPropertyControl::VALUETYPE_CONSTANT)
		{
			fp32* pSize = _pBatch->m_Size;
			const int nParticles = _pPR->m_NumParallelParticles;
			const fp32 Size = m_SizeCtrl.GetConstant();

			for (iParticle = 0; iParticle < nParticles; iParticle++)
				pSize[iParticle] = Size;
		}
		else if (m_SizeCtrl.m_ValueType & CPropertyControl::VALUETYPE_LINEAR)
		{
			for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
				_pBatch->m_Size[iParticle] = m_SizeCtrl.GetLinear(_pBatch->m_TimeFraction[iParticle]);
		}
		else if (m_SizeCtrl.m_ValueType & CPropertyControl::VALUETYPE_RANDOMCONSTANT)
		{
			for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
			{
				_pBatch->m_Size[iParticle] = m_SizeCtrl.GetRandomS(_pBatch->m_TimeFraction[iParticle], _pBatch->m_iRandseed[iParticle]);
			}
		}
		else
		{
			for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
			{
				const fp32 IndividualTime = _pPR->m_Time + m_SinRandTimeSpread * GetRand(_pBatch->m_iRandseed[iParticle]);
				_pBatch->m_Size[iParticle] = m_SizeCtrl.GetFluctS(IndividualTime, _pBatch->m_TimeFraction[iParticle], &g_SinRandTable, _pBatch->m_iRandseed[iParticle]);
			}
		}
	}
}

// -------------------------------------------------------------------
void ParticlesOptClassName::Generate_Colors(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int _GenerateFlags)
{
	MAUTOSTRIP(ParticlesOptClassName_Generate_Colors, MAUTOSTRIP_VOID);
	int iParticle;

	// Color, Alpha is transfered from CBatch::m_Alpha to CBatch::m_Dir[3] here.
	if (m_OptFlags & OPTFLAGS_COLORNOISE)
	{
		if (_pPR->m_Flags & PARTICLE_FLAGS_NOCHANNELEDCOLORNOISE)
		{
			SIMD_Combine_VcVcSr(_pPR->m_NumParallelParticles, m_ParticleColor, m_ParticleColorNoise, _pBatch->m_iRandseed, _pBatch->m_Dir);

/*
			for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
			{
				fp32 Rand = GetRandS(_pBatch->m_iRandseed[iParticle]);
				_pBatch->m_Dir[iParticle][0] = m_ParticleColor[0] + m_ParticleColorNoise[0] * Rand;
				_pBatch->m_Dir[iParticle][1] = m_ParticleColor[1] + m_ParticleColorNoise[1] * Rand;
				_pBatch->m_Dir[iParticle][2] = m_ParticleColor[2] + m_ParticleColorNoise[2] * Rand;
				_pBatch->m_Dir[iParticle][3] = m_ParticleColor[3] + m_ParticleColorNoise[3] * Rand;
			}
*/
		}
		else
		{
			SIMD_Combine_VcVcVr(_pPR->m_NumParallelParticles, m_ParticleColor, m_ParticleColorNoise, _pBatch->m_iRandseed, _pBatch->m_Dir);

/*
			for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
			{
				_pBatch->m_Dir[iParticle][0] = m_ParticleColor[0] + m_ParticleColorNoise[0] * GetRandS(_pBatch->m_iRandseed[iParticle]);
				_pBatch->m_Dir[iParticle][1] = m_ParticleColor[1] + m_ParticleColorNoise[1] * GetRandS(_pBatch->m_iRandseed[iParticle]);
				_pBatch->m_Dir[iParticle][2] = m_ParticleColor[2] + m_ParticleColorNoise[2] * GetRandS(_pBatch->m_iRandseed[iParticle]);
				_pBatch->m_Dir[iParticle][3] = m_ParticleColor[3] + m_ParticleColorNoise[3] * GetRandS(_pBatch->m_iRandseed[iParticle]);
			}
*/
		}
	}
	else
	{
/*
		for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
		{
			_pBatch->m_Dir[iParticle][0] = m_ParticleColor[0];
			_pBatch->m_Dir[iParticle][1] = m_ParticleColor[1];
			_pBatch->m_Dir[iParticle][2] = m_ParticleColor[2];
			_pBatch->m_Dir[iParticle][3] = m_ParticleColor[3];
		}
*/
		SIMD_Set_Vc( _pPR->m_NumParallelParticles, m_ParticleColor, _pBatch->m_Dir );
	}

	if (_pPR->m_Flags & PARTICLE_FLAGS_NOCLAMPCOLORS)
	{
/*
		for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
		{
			int color = CPixel32(_pBatch->m_Dir[iParticle][0], _pBatch->m_Dir[iParticle][1], _pBatch->m_Dir[iParticle][2], _pBatch->m_Alpha[iParticle]);
			_pBatch->m_Alpha[iParticle] = *((fp32*)&color);
		}
*/

		for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
			(CPixel32&)(_pBatch->m_Alpha[iParticle]) = 0xffffffff;
	}
	else
	{
		for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
			_pBatch->m_Dir[iParticle][3] *= _pBatch->m_Alpha[iParticle];

		SIMD_ConvertRGBA(_pBatch->m_Dir, (uint32*)_pBatch->m_Alpha, _pPR->m_NumParallelParticles);

/*
		for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
		{
			_pBatch->m_Dir[iParticle][0] = ClampRange(_pBatch->m_Dir[iParticle][0], 255.0f);
			_pBatch->m_Dir[iParticle][1] = ClampRange(_pBatch->m_Dir[iParticle][1], 255.0f);
			_pBatch->m_Dir[iParticle][2] = ClampRange(_pBatch->m_Dir[iParticle][2], 255.0f);
			_pBatch->m_Alpha[iParticle] = ClampRange(_pBatch->m_Alpha[iParticle], 255.0f);
			int color = CPixel32(_pBatch->m_Dir[iParticle][0], _pBatch->m_Dir[iParticle][1], _pBatch->m_Dir[iParticle][2], _pBatch->m_Alpha[iParticle]);
			_pBatch->m_Alpha[iParticle] = *((fp32*)&color);
		}
*/
	}
}

// -------------------------------------------------------------------
void ParticlesOptClassName::Generate_Distribution_Point(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int& _GenerateFlags)
{
	MAUTOSTRIP(ParticlesOptClassName_Generate_Distribution_Point, MAUTOSTRIP_VOID);
	int iParticle;

	if (m_OptFlags & OPTFLAGS_ANGLES)
	{
		const fp32* pTime = _pBatch->m_Time;
		uint32* pRandSeed = _pBatch->m_iRandseed;
		CVec4Dfp32* pDir = _pBatch->m_Dir;
		const int nParticles = _pPR->m_NumParallelParticles;

		for (iParticle = 0; iParticle < nParticles; iParticle++)
		{
			const fp32 EmissionTime = (_pPR->m_Time - pTime[iParticle]);
			const fp32 AngleA = m_AngleOffsetA + m_AngleOffsetChangeA * EmissionTime + m_AngleSpreadA * GetRandS(pRandSeed[iParticle]);
			const fp32 AngleB = m_AngleOffsetB + m_AngleOffsetChangeB * EmissionTime + m_AngleSpreadB * GetRandS(pRandSeed[iParticle]);
			AnglesToVector2(AngleA, AngleB, (CVec3Dfp32&)pDir[iParticle]);
		}
	}
	else
	{
		CVec4Dfp32 Constant( 1, 0, 0, 0 );
		SIMD_Set_Vc( _pPR->m_NumParallelParticles, Constant, _pBatch->m_Dir );
	}
}

// -------------------------------------------------------------------
void ParticlesOptClassName::Generate_Distribution(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int& _GenerateFlags)
{
	MAUTOSTRIP(ParticlesOptClassName_Generate_Distribution, MAUTOSTRIP_VOID);
	int iParticle;

	uint32* pRandSeed = _pBatch->m_iRandseed;
	CVec4Dfp32* pPos = _pBatch->m_Pos;
	const int nParticles = _pPR->m_NumParallelParticles;
	if (_pPR->m_Flags & PARTICLE_FLAGS_PRIMCENTER)
	{
		CVec4Dfp32* pDir = _pBatch->m_Dir;
		if( m_OptFlags & OPTFLAGS_NONUNITDISTSIZE )
		{
			for (iParticle = 0; iParticle < nParticles; iParticle++)
			{
				CVec3Dfp32 &DistributedPoint = (CVec3Dfp32&)pPos[iParticle];
				GetDistributedPoint(DistributedPoint, pRandSeed[iParticle]);

				// Scale point from signed volume unit to desired distribution size.
				DistributedPoint[0] *= m_DistributionSize[0];
				DistributedPoint[1] *= m_DistributionSize[1];
				DistributedPoint[2] *= m_DistributionSize[2];

				// This is really the only thing that PRIMCENTER does, but we extract the if-test to gain it's cycles (vs. code size?).
				pDir[iParticle] = DistributedPoint;
				pDir[iParticle].Normalize();
			}
		}
		else
		{
			for (iParticle = 0; iParticle < nParticles; iParticle++)
			{
				CVec3Dfp32 &DistributedPoint = (CVec3Dfp32&)pPos[iParticle];
				GetDistributedPoint(DistributedPoint, pRandSeed[iParticle]);

				// This is really the only thing that PRIMCENTER does, but we extract the if-test to gain it's cycles (vs. code size?).
				pDir[iParticle] = DistributedPoint;
				pDir[iParticle].Normalize();
			}
		}
	}
	else
	{
		if (m_OptFlags & OPTFLAGS_ANGLES)
		{
			const fp32* pTime = _pBatch->m_Time;
			CVec4Dfp32* pDir = _pBatch->m_Dir;

			if( m_OptFlags & OPTFLAGS_NONUNITDISTSIZE )
			{
				for (iParticle = 0; iParticle < nParticles; iParticle++)
				{
					const fp32 EmissionTime = (_pPR->m_Time - pTime[iParticle]);
					const fp32 AngleA = m_AngleOffsetA + m_AngleOffsetChangeA * EmissionTime + m_AngleSpreadA * GetRandS(pRandSeed[iParticle]);
					const fp32 AngleB = m_AngleOffsetB + m_AngleOffsetChangeB * EmissionTime + m_AngleSpreadB * GetRandS(pRandSeed[iParticle]);
					AnglesToVector2(AngleA, AngleB, (CVec3Dfp32&)pDir[iParticle]);

					CVec3Dfp32 &DistributedPoint = (CVec3Dfp32&)pPos[iParticle];
					GetDistributedPoint(DistributedPoint, pRandSeed[iParticle]);

					DistributedPoint[0] *= m_DistributionSize[0];
					DistributedPoint[1] *= m_DistributionSize[1];
					DistributedPoint[2] *= m_DistributionSize[2];

				}
			}
			else
			{
				for (iParticle = 0; iParticle < nParticles; iParticle++)
				{
					const fp32 EmissionTime = (_pPR->m_Time - pTime[iParticle]);
					const fp32 AngleA = m_AngleOffsetA + m_AngleOffsetChangeA * EmissionTime + m_AngleSpreadA * GetRandS(pRandSeed[iParticle]);
					const fp32 AngleB = m_AngleOffsetB + m_AngleOffsetChangeB * EmissionTime + m_AngleSpreadB * GetRandS(pRandSeed[iParticle]);
					AnglesToVector2(AngleA, AngleB, (CVec3Dfp32&)pDir[iParticle]);

					CVec3Dfp32 &DistributedPoint = (CVec3Dfp32&)pPos[iParticle];
					GetDistributedPoint(DistributedPoint, pRandSeed[iParticle]);
				}
			}
		}
		else
		{
			// Set m_Dir to unit
			CVec4Dfp32 Constant( 1, 0, 0, 0 );
			SIMD_Set_Vc( _pPR->m_NumParallelParticles, Constant, _pBatch->m_Dir );

			if( m_OptFlags & OPTFLAGS_NONUNITDISTSIZE )
			{
				for (iParticle = 0; iParticle < nParticles; iParticle++)
				{
					CVec3Dfp32 &DistributedPoint = (CVec3Dfp32&)pPos[iParticle];
					GetDistributedPoint(DistributedPoint, pRandSeed[iParticle]);

					DistributedPoint[0] *= m_DistributionSize[0];
					DistributedPoint[1] *= m_DistributionSize[1];
					DistributedPoint[2] *= m_DistributionSize[2];
				}
			}
			else
			{
				for (iParticle = 0; iParticle < nParticles; iParticle++)
				{
					CVec3Dfp32 &DistributedPoint = (CVec3Dfp32&)pPos[iParticle];
					GetDistributedPoint(DistributedPoint, pRandSeed[iParticle]);
				}
			}
		}
	}
}

// -------------------------------------------------------------------

void ParticlesOptClassName::Generate_UVQuads(CXR_VBChain* _pVBChain, PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch)
{
//	tri per particle = 2;
//	verts per particle = 4
//	m_lQuadParticleTVertices[iV + 0][0] = 0;
//	m_lQuadParticleTVertices[iV + 0][1] = 0;
//	m_lQuadParticleTVertices[iV + 1][0] = 1;
//	m_lQuadParticleTVertices[iV + 1][1] = 0;
//	m_lQuadParticleTVertices[iV + 2][0] = 1;
//	m_lQuadParticleTVertices[iV + 2][1] = 1;
//	m_lQuadParticleTVertices[iV + 3][0] = 0;
//	m_lQuadParticleTVertices[iV + 3][1] = 1;
 
	int nParallelParticles = (int)_pPR->m_NumParallelParticles;
	fp32* pTimeFraction = _pBatch->m_TimeFraction;
	fp32* M_RESTRICT pUV = _pVBChain->m_pTV[0];

	fp32 UVMapFramesU = m_UVMapFramesU;
	fp32 UVMapFramesV = m_UVMapFramesV;
	fp32 UVMapFramesMul = m_UVMapFramesMul;

	vec128 nFrames_V = M_VLdScalar(fp32(m_UVMapFrames) - 0.001f);

	vec128 UVMapFramesMulX_V = M_VLdScalar(1.0f / UVMapFramesMul);
	vec128 UVMapFramesMulY_V = M_VLdScalar(UVMapFramesMul);
	
	vec128 UVMapFramesU_V = M_VLdScalar(m_UVMapFramesU);
	vec128 UVMapFramesV_V = M_VLdScalar(m_UVMapFramesV);

	vec128 UVMapFrameAddMaskU = M_VSelMsk(M_VConstMsk(1,1,0,1), M_VZero(), UVMapFramesU_V);
	vec128 UVMapFrameAddMaskV = M_VSelMsk(M_VConstMsk(0,1,1,1), M_VZero(), UVMapFramesU_V);
	UVMapFrameAddMaskV = M_VSelMsk(M_VConstMsk(1,0,1,0), UVMapFrameAddMaskV, UVMapFramesV_V);
 /*
	for(int i = 0; i < nParallelParticles; i += 4)
	{
		
		// Calculate frame position for 4 diffrent particles
		vec128 iFrame4_V = M_VMul(M_VLd(pTimeFraction), nFrames_V);
		pTimeFraction += 4;

		// Calculate x and y frame position for the 4 particles
		vec128 iFrameY4_V = M_VTrunc(M_VMul(iFrame4_V, UVMapFramesMulY_V));
		vec128 iFrameX4_V = M_VTrunc(M_VSub(iFrame4_V, M_VMul(iFrameY4_V, UVMapFramesMulX_V)));

		// Calculate u and v coordinates for the 4 particles
		vec128 u4_V = M_VMul(UVMapFramesU_V, iFrameX4_V);
		vec128 v4_V = M_VMul(UVMapFramesV_V, iFrameY4_V);

		vec128 uvuv0_V = M_VSelMsk(M_VConstMsk(1,0,1,0), M_VSplatX(u4_V), M_VSplatX(v4_V));
		vec128 uvuv1_V = M_VSelMsk(M_VConstMsk(1,0,1,0), M_VSplatY(u4_V), M_VSplatY(v4_V));
		vec128 uvuv2_V = M_VSelMsk(M_VConstMsk(1,0,1,0), M_VSplatZ(u4_V), M_VSplatZ(v4_V));
		vec128 uvuv3_V = M_VSelMsk(M_VConstMsk(1,0,1,0), M_VSplatW(u4_V), M_VSplatW(v4_V));

		// Finally set uv coords
		*((vec128 * M_RESTRICT)pUV) = M_VAdd(uvuv0_V, UVMapFrameAddMaskU); pUV += 4;
		*((vec128 * M_RESTRICT)pUV) = M_VAdd(uvuv0_V, UVMapFrameAddMaskV); pUV += 4;

		*((vec128 * M_RESTRICT)pUV) = M_VAdd(uvuv1_V, UVMapFrameAddMaskU); pUV += 4;
		*((vec128 * M_RESTRICT)pUV) = M_VAdd(uvuv1_V, UVMapFrameAddMaskV); pUV += 4;

		*((vec128 * M_RESTRICT)pUV) = M_VAdd(uvuv2_V, UVMapFrameAddMaskU); pUV += 4;
		*((vec128 * M_RESTRICT)pUV) = M_VAdd(uvuv2_V, UVMapFrameAddMaskV); pUV += 4;

		*((vec128 * M_RESTRICT)pUV) = M_VAdd(uvuv3_V, UVMapFrameAddMaskU); pUV += 4;
		*((vec128 * M_RESTRICT)pUV) = M_VAdd(uvuv3_V, UVMapFrameAddMaskV); pUV += 4;
	}
*/

	for(int i = 0; i < nParallelParticles; i++)
	{
		// Calculate frame position for particle
		vec128 iFrame_V = M_VMul(M_VLdScalar(pTimeFraction[i]), nFrames_V);

		// Calculate x and y frame position for particle
		vec128 iFrameY_V = M_VTrunc(M_VMul(iFrame_V, UVMapFramesMulY_V));
		vec128 iFrameX_V = M_VTrunc(M_VSub(iFrame_V, M_VMul(iFrameY_V, UVMapFramesMulX_V)));

		// Calculate u and v coordinates for particle
		vec128 u_V = M_VMul(UVMapFramesU_V, iFrameX_V);
		vec128 v_V = M_VMul(UVMapFramesV_V, iFrameY_V);

		vec128 uvuv_V = M_VSelMsk(M_VConstMsk(1,0,1,0), u_V, v_V);

		vec128 X1 = M_VAdd(uvuv_V, UVMapFrameAddMaskU);
		vec128 X2 = M_VAdd(uvuv_V, UVMapFrameAddMaskV);

		// Set
		*((vec128 * M_RESTRICT)pUV) = X1;
		pUV += 4;
		*((vec128 * M_RESTRICT)pUV) = X2;
		pUV += 4;
      }
}

// -------------------------------------------------------------------
/*
void ParticlesOptClassName::Generate_UVTriangles(CXR_VBChain* _pVBChain, PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch)
{
	int32 nParallelParticles = (uint)_pPR->m_NumParallelParticles;
	fp32* pTimeFraction = _pBatch->m_TimeFraction;
	fp32* M_RESTRICT pUV = _pVBChain->m_pTV[0];
	//fp32 nFrames = fp32(m_UVMapFrames) - 0.001f;

	//int iFrame = 0;
	//int iFrameY = 0;
	//int iFrameX = 0;
	//fp32 u = 0.0f;
	//fp32 v = 0.0f;

	fp32 UVMapFramesU = m_UVMapFramesU;
	fp32 UVMapFramesV = m_UVMapFramesV;
	fp32 UVMapFramesMul = m_UVMapFramesMul;

	vec128 nFrames_V = M_VLdScalar(fp32(m_UVMapFrames) - 0.001f);
	vec128 UVMapFramesMulX_V = M_VLdScalar(1.0f / UVMapFramesMul);
	vec128 UVMapFramesMulY_V = M_VLdScalar(UVMapFramesMul);
	vec128 UVMapFramesU_V = M_VLdScalar(UVMapFramesU);
	vec128 UVMapFramesV_V = M_VLdScalar(UVMapFramesV);
	vec128 UVMapFrameAddMask = M_VSelMsk(M_VConstMsk(0,1,1,1), M_VZero(), M_VMul(UVMapFramesU_V, M_VTwo()));
	UVMapFrameAddMask = M_VSelMsk(M_VConstMsk(1,1,1,0), UVMapFrameAddMask, M_VMul(UVMapFramesV_V, M_VTwo()));

	//fp32 UVMapFramesU2 = m_UVMapFramesU * 2.0f;
	//fp32 UVMapFramesV2 = m_UVMapFramesV * 2.0f;
	for(int32 i = 0; i < nParallelParticles; i++)
	{
		// Calculate frame position for particle
		vec128 iFrame_V = M_VMul(M_VLdScalar(pTimeFraction[i]), nFrames_V);

		// Calculate x and y frame position for particle
		vec128 iFrameY_V = M_VTrunc(M_VMul(iFrame_V, UVMapFramesMulY_V));
		vec128 iFrameX_V = M_VTrunc(M_VSub(iFrame_V, M_VMul(iFrameY_V, UVMapFramesMulX_V)));

		// Calculate u and v coordinates for particle
		vec128 u_V = M_VMul(UVMapFramesU_V, iFrameX_V);
		vec128 v_V = M_VMul(UVMapFramesV_V, iFrameY_V);
		vec128 uvuv_V = M_VSelMsk(M_VConstMsk(1,0,1,0), u_V, v_V);
		vec128 X1 = M_VAdd(uvuv_V, UVMapFramesAddMask);
		
		// Nope, this doesn't work
		*((vec128 * M_RESTRICT)pUV) = uvuv_V;
		pUV += 2;
		*((vec128 * M_RESTRICT)pUV) = X1;
		pUV += 4;

//		pUV[0] = u;
//		pUV[1] = v;
//		pUV[2] = u + UVMapFramesU2;
//		pUV[3] = v;
//		pUV[4] = u;
//		pUV[5] = v + UVMapFramesV2;
//		pUV += 6;
	}
}
*/
// -------------------------------------------------------------------

bool ParticlesOptClassName::Generate_Render(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const CXR_Model_Custom_RenderParams* M_RESTRICT _pRenderParams, int _GenerateFlags)
{
	MAUTOSTRIP(ParticlesOptClassName_Generate_Render, false);
	if (_pPR->m_NumParallelParticles == 0)
		return true;

	int iParticle;

	const int nVPerParticle = (_pPR->m_Flags & PARTICLE_FLAGS_QUADS) ? 4 : 3;
	const int nTriPerParticle = (_pPR->m_Flags & PARTICLE_FLAGS_QUADS) ? 2 : 1;
//	const int nPrimPerParticle = nTriPerParticle * 3;

	//JK-NOTE: We allocate rooms for an extra 3 vertices here so the SIMD might work outside the allocated memory otherwise
	int VBContents = CXR_VB_VERTICES | CXR_VB_COLORS | ((_pPR->m_Flags & PARTICLE_FLAGS_USECUSTOMUV) ? CXR_VB_TVERTICES0 : 0);
	CXR_VBChain* pVB = _pRenderParams->m_pVBM->Alloc_VBChain(VBContents, (_pPR->m_NumParallelParticles + 3) * nVPerParticle);
	if (pVB == NULL)
		return false;

	if (_pPR->m_Flags & PARTICLE_FLAGS_QUADS)
	{
		if(_pPR->m_Flags & PARTICLE_FLAGS_USECUSTOMUV)
			Generate_UVQuads(pVB, _pPR, _pBatch);
		else
		{
			pVB->m_pTV[0] = (fp32 *)CXR_Util::m_lQuadParticleTVertices;
			pVB->m_nTVComp[0] = 2;
		}
		pVB->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, _pPR->m_NumParallelParticles * nTriPerParticle);
	}
	else
	{
		if(_pPR->m_Flags & PARTICLE_FLAGS_USECUSTOMUV)
		{
			// Shouldn't reach here... All texture animated particles should automatically be converted to quads...
			//Generate_UVTriangles(pVB, _pPR, _pBatch);
		}
		else
		{
			pVB->m_pTV[0] = (fp32 *)CXR_Util::m_lTriParticleTVertices;
			pVB->m_nTVComp[0] = 2;
		}
		pVB->Render_IndexedTriangles(CXR_Util::m_lTriParticleTriangles, _pPR->m_NumParallelParticles * nTriPerParticle);
	}

	pVB->m_pNextVB = _pPR->m_pChain;
	_pPR->m_pChain = pVB;

	if ((_pPR->m_Flags & PARTICLE_FLAGS_ALIGN) && (_GenerateFlags & GENERATE_FLAGS_MOVEMENTSET))
	{
//		for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
//			_pBatch->m_Movement[iParticle] *= m_ParticleAlignLengthScale;
		SIMD_Mul_VSc( _pPR->m_NumParallelParticles, _pBatch->m_Movement, m_ParticleAlignLengthScale, _pBatch->m_Movement );

		if (_pPR->m_Flags & PARTICLE_FLAGS_ALIGN_SAFEQUAD)
		{
			if (_pPR->m_Flags & PARTICLE_FLAGS_QUADS)
			{
				for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
					RenderQuadParticleAlignedSafeQuad(_pPR, _pBatch, iParticle);
			}
			else
			{
				for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
					RenderParticleAlignedSafeQuad(_pPR, _pBatch, iParticle);
			}
		}
		else
		{
			if (_pPR->m_Flags & PARTICLE_FLAGS_QUADS)
			{
				for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
					RenderQuadParticleAligned(_pPR, _pBatch, iParticle);
			}
			else
			{
				for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
					RenderParticleAligned(_pPR, _pBatch, iParticle);
			}
		}
	}
	else
	{
		if (_pPR->m_Flags & PARTICLE_FLAGS_QUADS)
		{
			RenderQuadParticleArray(_pPR, _pBatch);

//				for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
//					RenderQuadParticle(iParticle);
		}
		else
		{
			RenderParticleArray(_pPR, _pBatch);

//			for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
//				RenderParticle(iParticle);

		}
	}

	return true;
}

// -------------------------------------------------------------------
// Return false if any particle couldn't be emitted (particle overflow).

bool ParticlesOptClassName::GenerateParallelParticles(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const CXR_Model_Custom_RenderParams* M_RESTRICT _pRenderParams)
{
	MAUTOSTRIP(ParticlesOptClassName_GenerateParallelParticles, false);

	PerRenderVariables* M_RESTRICT pPR = _pPR;
	CBatch* M_RESTRICT pBatch = _pBatch;
	CMHistory* M_RESTRICT pHistory = pPR->m_pHistory;

	int GenerateFlags = 0;
	uint iParticle;

	if (!Generate_StandardSetup(_pPR, _pBatch, GenerateFlags))
		return false;

	if (pPR->m_NumParallelParticles == 0)
		return true;

	// Get history matrix
	if (!(_pPR->m_Flags & PARTICLE_FLAGS_NOHISTORY))
	{
		Generate_HistoryCulling(_pPR, _pBatch, GenerateFlags);
	}

	if (pPR->m_NumParallelParticles == 0)
		return true;

	if ((pPR->m_NumParticles + pPR->m_NumParallelParticles) >= m_MaxNumParticles)
	{
		pPR->m_NumParallelParticles = m_MaxNumParticles - pPR->m_NumParticles;
		pPR->m_bOverflow |= true;
	}

	if (m_OptFlags & OPTFLAGS_OFFSET)
	{
		SIMD_Combine_ScScSr(pPR->m_NumParallelParticles, m_ParticleOffset, m_ParticleOffsetNoise, pBatch->m_iRandseed, pBatch->m_Offset);
/*
		for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
			_pBatch->m_Offset[iParticle] = m_ParticleOffset + m_ParticleOffsetNoise * GetRandS(_pBatch->m_iRandseed[iParticle]);
*/
		GenerateFlags |= GENERATE_FLAGS_TOTALOFFSET;
	}

	// -------------------------------------------------------------------
	// Velocity
	if (m_OptFlags & OPTFLAGS_VELOCITY)
	{
		Generate_Velocity(_pPR, _pBatch, GenerateFlags);
		GenerateFlags |= GENERATE_FLAGS_TOTALOFFSET;
	}

	if (m_DistributionPrimitive != PRIMITIVE_POINT)
	{
		Generate_Distribution(_pPR, _pBatch, GenerateFlags);
		GenerateFlags |= GENERATE_FLAGS_POSSET;
	}
	else //if ((m_DistributionPrimitive == PRIMITIVE_POINT) || (!(_pPR->m_Flags & PARTICLE_FLAGS_PRIMCENTER)))
	{
		Generate_Distribution_Point(_pPR, _pBatch, GenerateFlags);
	}

	if (m_OptFlags & OPTFLAGS_LOCALOFFSET)
	{
		uint nParallelParticles = pPR->m_NumParallelParticles;

		CVec4Dfp32 LocalPos;
		LocalPos.k[0]	= m_LocalPositionOffset[0];
		LocalPos.k[1]	= m_LocalPositionOffset[1];
		LocalPos.k[2]	= m_LocalPositionOffset[2];
		if (GenerateFlags & GENERATE_FLAGS_POSSET)
		{
			LocalPos.k[3]	= 0.0f;
//			for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
//				(CVec3Dfp32&)_pBatch->m_Pos[iParticle] += m_LocalPositionOffset;
			SIMD_Add_VVc(nParallelParticles, pBatch->m_Pos, LocalPos, pBatch->m_Pos);
		}
		else
		{
			LocalPos.k[3]	= 1.0f;
//			for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
//				(CVec3Dfp32&)_pBatch->m_Pos[iParticle] = m_LocalPositionOffset;

			SIMD_Set_Vc(nParallelParticles, LocalPos, pBatch->m_Pos);
		}
		GenerateFlags |= GENERATE_FLAGS_POSSET;
	}

	// Move Control & Offset
	if (m_MoveCtrl.m_ValueType & CPropertyControl::VALUETYPE_ZERO)
	{
		uint nParallelParticles = pPR->m_NumParallelParticles;

		if (GenerateFlags & GENERATE_FLAGS_TOTALOFFSET)
		{
			if (!(GenerateFlags & GENERATE_FLAGS_POSSET))
			{
				SIMD_Mul_VS(nParallelParticles, pBatch->m_Dir, pBatch->m_Offset, pBatch->m_Pos);

/*
				for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
				{
					_pBatch->m_Pos[iParticle] = _pBatch->m_Dir[iParticle] * _pBatch->m_Offset[iParticle];
					(CVec3Dfp32&)_pBatch->m_Pos[iParticle] = (CVec3Dfp32&)_pBatch->m_Dir[iParticle] * _pBatch->m_Offset[iParticle];
				}
*/
			}
			else
				SIMD_Combine_VVS(nParallelParticles, pBatch->m_Pos, pBatch->m_Dir, pBatch->m_Offset, pBatch->m_Pos);

				//for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
				//	_pBatch->m_Pos[iParticle] += _pBatch->m_Dir[iParticle] * _pBatch->m_Offset[iParticle];

			GenerateFlags |= GENERATE_FLAGS_POSSET;
		}
	}
	else
	{
		uint nParallelParticles = pPR->m_NumParallelParticles;

		fp32 TimeBase = pPR->m_Time;
		fp32 SinRandTimeSpread = m_SinRandTimeSpread;

		if (GenerateFlags & GENERATE_FLAGS_TOTALOFFSET)
		{
			if (!(GenerateFlags & GENERATE_FLAGS_POSSET))
			{
				SIMD_Mul_VS( nParallelParticles, pBatch->m_Dir, pBatch->m_Offset, pBatch->m_Pos );

				if (m_MoveCtrl.m_ValueType & CPropertyControl::VALUETYPE_RANDOMFLUCT)
				{
					for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
					{
						fp32 IndividualTime = TimeBase + SinRandTimeSpread * GetRand( pBatch->m_iRandseed[iParticle] );
						(CVec3Dfp32&)pBatch->m_Pos[iParticle] += m_MoveCtrl.GetFluctV( IndividualTime, pBatch->m_TimeFraction[iParticle], &g_SinRandTable, pBatch->m_iRandseed[iParticle] );
					}
				}
				else
				{
					for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
					{
						fp32 IndividualTime = TimeBase + SinRandTimeSpread * GetRand( pBatch->m_iRandseed[iParticle] );
						(CVec3Dfp32&)pBatch->m_Pos[iParticle] += m_MoveCtrl.GetRandomV( pBatch->m_TimeFraction[iParticle], pBatch->m_iRandseed[iParticle] );
					}

				}
			}
			else
			{
				SIMD_Combine_VVS(nParallelParticles, pBatch->m_Pos, pBatch->m_Dir, pBatch->m_Offset, pBatch->m_Pos);

				if (m_MoveCtrl.m_ValueType & CPropertyControl::VALUETYPE_RANDOMFLUCT)
				{
					for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
					{
						//_pBatch->m_Pos[iParticle] += _pBatch->m_Dir[iParticle] * _pBatch->m_Offset[iParticle];
						fp32 IndividualTime = TimeBase + SinRandTimeSpread * GetRand( pBatch->m_iRandseed[iParticle] );
						(CVec3Dfp32&)pBatch->m_Pos[iParticle] += m_MoveCtrl.GetFluctV(IndividualTime, pBatch->m_TimeFraction[iParticle], &g_SinRandTable, pBatch->m_iRandseed[iParticle]);
					}
				}
				else
				{
					for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
					{
						//_pBatch->m_Pos[iParticle] += _pBatch->m_Dir[iParticle] * _pBatch->m_Offset[iParticle];
						fp32 IndividualTime = TimeBase + SinRandTimeSpread * GetRand( pBatch->m_iRandseed[iParticle] );
						(CVec3Dfp32&)pBatch->m_Pos[iParticle] += m_MoveCtrl.GetRandomV(pBatch->m_TimeFraction[iParticle], pBatch->m_iRandseed[iParticle]);
					}
				}
			}
		}
		else
		{
			if (m_MoveCtrl.m_ValueType & CPropertyControl::VALUETYPE_RANDOMFLUCT)
			{
				if (!(GenerateFlags & GENERATE_FLAGS_POSSET))
				{
					for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
					{
						fp32 IndividualTime = TimeBase + SinRandTimeSpread * GetRand(pBatch->m_iRandseed[iParticle]);
						pBatch->m_Pos[iParticle] = m_MoveCtrl.GetFluctV(IndividualTime, pBatch->m_TimeFraction[iParticle], &g_SinRandTable, pBatch->m_iRandseed[iParticle]);
					}
				}
				else
				{
					for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
					{
						fp32 IndividualTime = TimeBase + SinRandTimeSpread * GetRand(pBatch->m_iRandseed[iParticle]);
						(CVec3Dfp32&)pBatch->m_Pos[iParticle] += m_MoveCtrl.GetFluctV(IndividualTime, pBatch->m_TimeFraction[iParticle], &g_SinRandTable, pBatch->m_iRandseed[iParticle]);
					}
				}
			}
			else
			{
				if (!(GenerateFlags & GENERATE_FLAGS_POSSET))
				{
					for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
					{
						fp32 IndividualTime = TimeBase + SinRandTimeSpread * GetRand(pBatch->m_iRandseed[iParticle]);
						pBatch->m_Pos[iParticle] = m_MoveCtrl.GetRandomV(pBatch->m_TimeFraction[iParticle], pBatch->m_iRandseed[iParticle]);
					}
				}
				else
				{
					for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
					{
						fp32 IndividualTime = TimeBase + SinRandTimeSpread * GetRand(pBatch->m_iRandseed[iParticle]);
						(CVec3Dfp32&)pBatch->m_Pos[iParticle] += m_MoveCtrl.GetRandomV(pBatch->m_TimeFraction[iParticle], pBatch->m_iRandseed[iParticle]);
					}
				}
			}
		}

		GenerateFlags |= GENERATE_FLAGS_POSSET;
	}

	if ((_pPR->m_Flags & PARTICLE_FLAGS_ALIGN) && (m_OptFlags & OPTFLAGS_VELOCITY)) // See if we have velocity, which is then placed in movement[0].
	{
		uint nParallelParticles = pPR->m_NumParallelParticles;

/*
		// This MUST not happend, since Movement is used for velocity movement!!!
		// This should not happens, since this is the first place movement is set.
		if (GenerateFlags & GENERATE_FLAGS_MOVEMENTSET)
		{
			for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
				_pBatch->m_Movement[iParticle] += _pBatch->m_Dir[iParticle] * _pBatch->m_Movement[iParticle][0];
		}
		else
*/
		{
			for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
				pBatch->m_Movement[iParticle] = pBatch->m_Dir[iParticle] * pBatch->m_Movement[iParticle][0];
		}
		GenerateFlags |= GENERATE_FLAGS_MOVEMENTSET;
	}

	const fp32 AxxScale = 1.0f;
	const fp32 AxxmentScale = 2.0f;
//		IPMethod ipmethod = (_pPR->m_Flags & PARTICLE_FLAGS_SPLINEHISTORY) ? IPMethod_Spline : IPMethod_Linear;
	CMat4Dfp32 LocalToWorld;
	CVec3Dfp32 EmitterSpeed;
	CVec3Dfp32 Acceleration;

	if (m_OptFlags & OPTFLAGS_ACCELERATION)
	{
		uint nParallelParticles = pPR->m_NumParallelParticles;

		if (_pPR->m_Flags & PARTICLE_FLAGS_LOCALACCELERATION)	// Local acceleration.
		{
			if (_pPR->m_Flags & PARTICLE_FLAGS_ALIGN)			// Align.
			{
				if (_pPR->m_Flags & PARTICLE_FLAGS_NOHISTORY)	// No history.
				{
					// No emitter movement available if we have no history.
					if (GenerateFlags & GENERATE_FLAGS_POSSET) // Add Position
					{
						if (GenerateFlags & GENERATE_FLAGS_MOVEMENTSET)	//	Add Movement.
						{
							for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
							{
								Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
								(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );
								((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(pBatch->m_LocalToWorld);
								Acceleration.MultiplyMatrix3x3(pBatch->m_LocalToWorld);

								((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(pBatch->m_LocalToWorld);
								(CVec3Dfp32&)pBatch->m_Movement[iParticle] += Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
							}
						}
						else
						{
							for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
							{
								Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
								(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );
								((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(pBatch->m_LocalToWorld);
								Acceleration.MultiplyMatrix3x3(pBatch->m_LocalToWorld);

								pBatch->m_Movement[iParticle] = Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
							}
						}
					}
					else // Set Position
					{
						if (GenerateFlags & GENERATE_FLAGS_MOVEMENTSET)	//	Add Movement.
						{
							for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
							{
								Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
								Acceleration.MultiplyMatrix3x3(pBatch->m_LocalToWorld);
								pBatch->m_Pos[iParticle] = CVec3Dfp32::GetMatrixRow(pBatch->m_LocalToWorld, 3);
								(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );

								((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(pBatch->m_LocalToWorld);
								(CVec3Dfp32&)pBatch->m_Movement[iParticle] += Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
							}
						}
						else // Set movement
						{
							for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
							{
								Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
								Acceleration.MultiplyMatrix3x3(pBatch->m_LocalToWorld);
								pBatch->m_Pos[iParticle] = CVec3Dfp32::GetMatrixRow(pBatch->m_LocalToWorld, 3);
								(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );

								pBatch->m_Movement[iParticle] = Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
							}
						}
					}
				}
				else // Has history
				{
					if (_pPR->m_Flags & PARTICLE_FLAGS_ALIGN_EMITTERMOVEMENT) // EmitterMovement
					{
						if (GenerateFlags & GENERATE_FLAGS_POSSET) // Add Position
						{
							if (GenerateFlags & GENERATE_FLAGS_MOVEMENTSET)	//	Add Movement.
							{
								for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
								{
									pHistory->GetInterpolatedMatrix(pBatch->m_EmissionTime[iParticle], LocalToWorld, EmitterSpeed);
									Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
									(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );
									((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(LocalToWorld);
									Acceleration.MultiplyMatrix3x3(LocalToWorld);
									((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(LocalToWorld);

									(CVec3Dfp32&)pBatch->m_Movement[iParticle] += Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] += EmitterSpeed * m_EmitterSpeedAlignScale;
								}
							}
							else // Set movement
							{
								for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
								{
									pHistory->GetInterpolatedMatrix(pBatch->m_EmissionTime[iParticle], LocalToWorld, EmitterSpeed);
									Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
									(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );
									((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(LocalToWorld);
									Acceleration.MultiplyMatrix3x3(LocalToWorld);
									((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(LocalToWorld);

									(CVec3Dfp32&)pBatch->m_Movement[iParticle] = Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] += EmitterSpeed * m_EmitterSpeedAlignScale;
								}
							}
						}
						else // Set Position
						{
							for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
							{
								pHistory->GetInterpolatedMatrix(pBatch->m_EmissionTime[iParticle], LocalToWorld, EmitterSpeed);
								Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
								Acceleration.MultiplyMatrix3x3(LocalToWorld);
								pBatch->m_Pos[iParticle] = CVec3Dfp32::GetMatrixRow(pBatch->m_LocalToWorld, 3);
								(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );

								if( GenerateFlags & GENERATE_FLAGS_MOVEMENTSET )	//	Add Movement.
								{
									((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(LocalToWorld);
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] += Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
								}
								else												//	Set Movement.
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] = Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );

								(CVec3Dfp32&)pBatch->m_Movement[iParticle] += EmitterSpeed * m_EmitterSpeedAlignScale;
							}
						}
					}
					else // No EmitterMovement
					{
						if (GenerateFlags & GENERATE_FLAGS_POSSET)	//	Add Position.
						{
							if (GenerateFlags & GENERATE_FLAGS_MOVEMENTSET)	//	Add Movement.
							{
								for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
								{
									pHistory->GetInterpolatedMatrix(pBatch->m_EmissionTime[iParticle], LocalToWorld, EmitterSpeed);
									Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
									(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );
									((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(LocalToWorld);
									Acceleration.MultiplyMatrix3x3(LocalToWorld);

									((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(LocalToWorld);
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] += Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
								}
							}
							else //	Set Movement.
							{
								for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
								{
									pHistory->GetInterpolatedMatrix(pBatch->m_EmissionTime[iParticle], LocalToWorld, EmitterSpeed);
									Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
									(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );
									((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(LocalToWorld);
									Acceleration.MultiplyMatrix3x3(LocalToWorld);

									(CVec3Dfp32&)pBatch->m_Movement[iParticle] = Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
								}
							}
						}
						else // Set Position
						{
							for( iParticle=0; iParticle < nParallelParticles; iParticle++ )
							{
								pHistory->GetInterpolatedMatrix(pBatch->m_EmissionTime[iParticle], LocalToWorld, EmitterSpeed);
								Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
								Acceleration.MultiplyMatrix3x3(LocalToWorld);
								pBatch->m_Pos[iParticle] = CVec3Dfp32::GetMatrixRow(pBatch->m_LocalToWorld, 3);
								(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );

								if( GenerateFlags & GENERATE_FLAGS_MOVEMENTSET )	//	Add Movement.
								{
									((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(pBatch->m_LocalToWorld);
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] += Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
								}
								else												//	Set Movement.
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] = Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
							}
						}
					}
				}
				GenerateFlags |= GENERATE_FLAGS_MOVEMENTSET;
			}
			else // No align
			{
				//	Since we don't align, we don't need movement (and thus not emittermovement either).

				if( _pPR->m_Flags & PARTICLE_FLAGS_NOHISTORY )		//	No history.
				{
					for( iParticle=0; iParticle < nParallelParticles; iParticle++ )
					{
						Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);

						if( GenerateFlags & GENERATE_FLAGS_POSSET )	//	Add Position.
						{
							(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );
							((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(pBatch->m_LocalToWorld);
						}
						else
						{
							Acceleration.MultiplyMatrix3x3(pBatch->m_LocalToWorld);
							pBatch->m_Pos[iParticle] = CVec3Dfp32::GetMatrixRow(pBatch->m_LocalToWorld, 3);
							(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );
						}
					}
				}
				else // Has history
				{
					for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
					{
						pHistory->GetInterpolatedMatrix(pBatch->m_EmissionTime[iParticle], LocalToWorld, EmitterSpeed);
						Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);

						if( GenerateFlags & GENERATE_FLAGS_POSSET )	//	Add Position.
						{
							(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );
							((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(LocalToWorld);
						}
						else										//	Set Position.
						{
							Acceleration.MultiplyMatrix3x3(pBatch->m_LocalToWorld);
							(CVec3Dfp32&)pBatch->m_Pos[iParticle] = CVec3Dfp32::GetMatrixRow(pBatch->m_LocalToWorld, 3);
							(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );
						}								
					}
				}
			}
		}
		else // World acceleration
		{
			if (_pPR->m_Flags & PARTICLE_FLAGS_ALIGN) // Align
			{
				if (_pPR->m_Flags & PARTICLE_FLAGS_NOHISTORY) // No history
				{
					// No emitter movement available if we have no history.
					if (GenerateFlags & GENERATE_FLAGS_POSSET) // Add Position
					{
						for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
						{
							Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
							((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(pBatch->m_LocalToWorld);
							(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );

							if( GenerateFlags & GENERATE_FLAGS_MOVEMENTSET )	//	Add Movement.
							{
								((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(pBatch->m_LocalToWorld);
								(CVec3Dfp32&)pBatch->m_Movement[iParticle] += Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
							}
							else												//	Set Movement.
								(CVec3Dfp32&)pBatch->m_Movement[iParticle] = Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
						}
					}
					else // Set Position
					{
						for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
						{
							Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
							(CVec3Dfp32&)pBatch->m_Pos[iParticle] = CVec3Dfp32::GetMatrixRow(pBatch->m_LocalToWorld, 3);
							(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );

							if( GenerateFlags & GENERATE_FLAGS_MOVEMENTSET )	// Add Movement.
							{
								((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(pBatch->m_LocalToWorld);
								(CVec3Dfp32&)pBatch->m_Movement[iParticle] += Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
							}
							else												//	Set Movement.
								(CVec3Dfp32&)pBatch->m_Movement[iParticle] = Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
						}
					}
				}
				else // Has history
				{
					if (_pPR->m_Flags & PARTICLE_FLAGS_ALIGN_EMITTERMOVEMENT) // EmitterMovement
					{
						if (GenerateFlags & GENERATE_FLAGS_POSSET) // Add Position
						{
							for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
							{
								pHistory->GetInterpolatedMatrix(pBatch->m_EmissionTime[iParticle], LocalToWorld, EmitterSpeed);
								Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
								((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(LocalToWorld);
								(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );

								if( GenerateFlags & GENERATE_FLAGS_MOVEMENTSET )	//	Add Movement.
								{
									((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(LocalToWorld);
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] += Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
								}
								else												//	Set Movement.
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] = Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );

								(CVec3Dfp32&)pBatch->m_Movement[iParticle] += EmitterSpeed * m_EmitterSpeedAlignScale;
							}
						}
						else // Set Position
						{
							for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
							{
								pHistory->GetInterpolatedPos(pBatch->m_EmissionTime[iParticle], (CVec3Dfp32&)pBatch->m_Pos[iParticle], EmitterSpeed);
								Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
								(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );

								if( GenerateFlags & GENERATE_FLAGS_MOVEMENTSET )	//	Add Movement.
								{
									((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(LocalToWorld);
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] += Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
								}
								else												//	Set Movement.
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] = Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );

								(CVec3Dfp32&)pBatch->m_Movement[iParticle] += EmitterSpeed * m_EmitterSpeedAlignScale;
							}
						}
					}
					else // No EmitterMovement
					{
						if (GenerateFlags & GENERATE_FLAGS_POSSET) // Add Position
						{
							for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
							{
								pHistory->GetInterpolatedMatrix(pBatch->m_EmissionTime[iParticle], LocalToWorld, EmitterSpeed);
								Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
								((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(LocalToWorld);
								(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );

								if( GenerateFlags & GENERATE_FLAGS_MOVEMENTSET )	//	Add Movement.
								{
									((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(LocalToWorld);
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] += Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
								}
								else												//	Set Movement.
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] = Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
							}
						}
						else // Set Position
						{
							for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
							{
								pHistory->GetInterpolatedPos(pBatch->m_EmissionTime[iParticle], (CVec3Dfp32&)pBatch->m_Pos[iParticle], EmitterSpeed);
								Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);
								(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );

								if( GenerateFlags & GENERATE_FLAGS_MOVEMENTSET )	//	Add Movement.
								{
									((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(LocalToWorld);
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] += Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
								}
								else												//	Set Movement.
									(CVec3Dfp32&)pBatch->m_Movement[iParticle] = Acceleration * ( pBatch->m_Time[iParticle] * AxxmentScale );
							}
						}
					}
				}
				GenerateFlags |= GENERATE_FLAGS_MOVEMENTSET;
			}
			else // No align
			{
				// Since we don't align, we don't need movement (and thus not emittermovement either).
				if (_pPR->m_Flags & PARTICLE_FLAGS_NOHISTORY) // No history
				{
					for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
					{
						Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);

						if( GenerateFlags & GENERATE_FLAGS_POSSET )	//	Add Position.
							((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(pBatch->m_LocalToWorld);
						else										//	Set Position.
							(CVec3Dfp32&)pBatch->m_Pos[iParticle] = CVec3Dfp32::GetMatrixRow(pBatch->m_LocalToWorld, 3);

						(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );
					}
				}
				else // Has history
				{
					for( iParticle=0; iParticle < nParallelParticles; iParticle++ )
					{
						if( GenerateFlags & GENERATE_FLAGS_POSSET )	//	Add Position.
							pHistory->GetInterpolatedMatrix(pBatch->m_EmissionTime[iParticle], LocalToWorld, EmitterSpeed);
						else
							pHistory->GetInterpolatedPos(pBatch->m_EmissionTime[iParticle], (CVec3Dfp32&)pBatch->m_Pos[iParticle], EmitterSpeed);

						Acceleration = m_ParticleAcceleration + m_ParticleAccelerationNoise * GetRandS(pBatch->m_iRandseed[iParticle]);

						if( GenerateFlags & GENERATE_FLAGS_POSSET )	//	Add Position.
							((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(LocalToWorld);

						(CVec3Dfp32&)pBatch->m_Pos[iParticle] += Acceleration * ( Sqr(pBatch->m_Time[iParticle]) * AxxScale );
					}
				}
			}
		}
	}
	else // No acceleration
	{
		uint nParallelParticles = pPR->m_NumParallelParticles;

		// No need to branch local/world acceleration.

		if (_pPR->m_Flags & PARTICLE_FLAGS_NOHISTORY)
		{
			// No movement available if we have no history.
			if( GenerateFlags & GENERATE_FLAGS_POSSET )	// Add Position.
			{
				SIMD_Mul_VMc(nParallelParticles, pBatch->m_Pos, pBatch->m_LocalToWorld, pBatch->m_Pos);

//					for (iParticle = 0; iParticle < _pPR->m_NumParallelParticles; iParticle++)
//						((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(pBatch->m_LocalToWorld);
			}
			else										// Set Position.
			{
				for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
					pBatch->m_Pos[iParticle] = CVec3Dfp32::GetMatrixRow(pBatch->m_LocalToWorld, 3);
			}
		}
		else
		{
			if ((_pPR->m_Flags & PARTICLE_FLAGS_ALIGN_EMITTERMOVEMENT) && (_pPR->m_Flags & PARTICLE_FLAGS_ALIGN))
			{
				if( GenerateFlags & GENERATE_FLAGS_POSSET )	//	Add Position.
				{
					for( iParticle=0; iParticle < nParallelParticles; iParticle++ )
					{
						pHistory->GetInterpolatedMatrix(pBatch->m_EmissionTime[iParticle], LocalToWorld, EmitterSpeed);
						((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(LocalToWorld);

						if( GenerateFlags & GENERATE_FLAGS_MOVEMENTSET )	//	Add Movement.
						{
							((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(LocalToWorld);
							(CVec3Dfp32&)pBatch->m_Movement[iParticle] += EmitterSpeed * m_EmitterSpeedAlignScale;
						}
						else												//	Set Movement.
							pBatch->m_Movement[iParticle] = EmitterSpeed * m_EmitterSpeedAlignScale;
					}
				}
				else // Set Position
				{
					if( GenerateFlags & GENERATE_FLAGS_MOVEMENTSET )	//	Add Movement.
					{
						for( iParticle=0; iParticle < nParallelParticles; iParticle++ )
						{
							pHistory->GetInterpolatedMatrix(pBatch->m_EmissionTime[iParticle], LocalToWorld, EmitterSpeed);
							(CVec3Dfp32&)pBatch->m_Pos[iParticle] = CVec3Dfp32::GetMatrixRow(LocalToWorld, 3);
							((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(LocalToWorld);
							(CVec3Dfp32&)pBatch->m_Movement[iParticle] += EmitterSpeed * m_EmitterSpeedAlignScale;
						}
					}
					else												//	Set Movement
					{
						for( iParticle=0; iParticle < nParallelParticles; iParticle++ )
						{
							pHistory->GetInterpolatedPos(pBatch->m_EmissionTime[iParticle], (CVec3Dfp32&)pBatch->m_Pos[iParticle], EmitterSpeed);
							pBatch->m_Movement[iParticle] = EmitterSpeed * m_EmitterSpeedAlignScale;
						}
					}
				}
				GenerateFlags |= GENERATE_FLAGS_MOVEMENTSET;
			}
			else
			{
				if ((_pPR->m_Flags & PARTICLE_FLAGS_ALIGN) && (GenerateFlags & GENERATE_FLAGS_MOVEMENTSET))
				{
					for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
					{
						pHistory->GetInterpolatedMatrix(pBatch->m_EmissionTime[iParticle], LocalToWorld, EmitterSpeed);

						if( GenerateFlags & GENERATE_FLAGS_POSSET )	//	Add Position.
							((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix(LocalToWorld);
						else										//	Set Position.
							((CVec3Dfp32&)pBatch->m_Pos[iParticle]) = CVec3Dfp32::GetMatrixRow(LocalToWorld, 3);

						((CVec3Dfp32&)pBatch->m_Movement[iParticle]).MultiplyMatrix3x3(LocalToWorld);
					}
				}
				else
				{
					for( iParticle=0; iParticle < nParallelParticles; iParticle++ )
					{
						if( GenerateFlags & GENERATE_FLAGS_POSSET )	//	Add Position.
						{
							pHistory->GetInterpolatedMatrix(pBatch->m_EmissionTime[iParticle], LocalToWorld, EmitterSpeed );
							((CVec3Dfp32&)pBatch->m_Pos[iParticle]).MultiplyMatrix( LocalToWorld );
						}
						else
							pHistory->GetInterpolatedPos(pBatch->m_EmissionTime[iParticle], (CVec3Dfp32&)pBatch->m_Pos[iParticle], EmitterSpeed );
					}
				}
			}
		}
	}

	GenerateFlags |= GENERATE_FLAGS_POSSET;
	//GenerateFlags |= (m_Flags & PARTICLE_FLAGS_ALIGN) ? GENERATE_FLAGS_MOVEMENTSET : 0;

	// -------------------------------------------------------------------
	// Size control
	Generate_SizeControl(_pPR, _pBatch, GenerateFlags);

	// -------------------------------------------------------------------
	// CloseUp LODing
	// -------------------------------------------------------------------
#ifndef	PLATFORM_PS2
	if (!(_pPR->m_Flags & PARTICLE_FLAGS_NOLOD))
	{
		CVec4Dfp32* pPos = (CVec4Dfp32*)&(pBatch->m_Pos);
		fp32* pSize = (fp32*)&(pBatch->m_Size);
		fp32* pAlpha = (fp32*)&(pBatch->m_Alpha);

		fp32 MaxArea[PARTICLE_MAXPARTICLEID];
		fp32 ThresholdArea[PARTICLE_MAXPARTICLEID];
		fp32 AreaScale[PARTICLE_MAXPARTICLEID];
			
		for (int i = 0; i < PARTICLE_MAXPARTICLEID; i++)
		{
			MaxArea[i] = Sqr(45.0f * (fp32)(i+1) / (fp32)PARTICLE_MAXPARTICLEID);
			ThresholdArea[i] = Sqr(35.0f * (fp32)(i+1) / (fp32)PARTICLE_MAXPARTICLEID);

			fp32 FadeRange = MaxArea[i] - ThresholdArea[i];
			fp32 InvFadeRange = 1.0f / FadeRange;

			AreaScale[i] = _pPR->m_ViewportXScale * InvFadeRange;

			// These seem to be equal for all i.
			MaxArea[i] *= InvFadeRange;
			ThresholdArea[i] *= InvFadeRange;
		}

		uint nParallelParticles = pPR->m_NumParallelParticles;
		for (iParticle = 0; iParticle < nParallelParticles; )
		{
			bool bCull = false;

			CVec3Dfp32& Pos = (CVec3Dfp32&)pPos[iParticle];
			const fp32 Depth = pPR->m_CameraFwd[0] * (Pos[0] - pPR->m_CameraPos[0]) + 
							  pPR->m_CameraFwd[1] * (Pos[1] - pPR->m_CameraPos[1]) + 
							  pPR->m_CameraFwd[2] * (Pos[2] - pPR->m_CameraPos[2]);

			if (Depth < 1.0f)
			{
				bCull = true;
			}
			else
			{
				int i;
				uint32 ID = pBatch->m_ID[iParticle];

				if (ID & 1)
					i = 0;
				else if (ID & 2)
					i = 1;
				else if (ID & 4)
					i = 2;
				else if (ID & 8)
					i = 3;
				else
				{
					iParticle++;
					continue;
				}
//						i = 4;

				fp32 Area = Sqr(pSize[iParticle]);

				if (_pPR->m_Flags & PARTICLE_FLAGS_ALIGN)
				{
					const fp32 MovementLength = ((CVec3Dfp32&)pBatch->m_Movement[iParticle]).Length();
					Area *= MovementLength * m_ParticleAlignLengthScale;
				}

				const fp32 InvDepth = 1.0f / Depth;
				const fp32 ProjectedArea = Area * Sqr(InvDepth) * AreaScale[i];

				if (ProjectedArea > MaxArea[i])
				{
					bCull = true;
				}
				else if (ProjectedArea > ThresholdArea[i])
				{
					const fp32 Fade = 1.0f - Clamp01(ProjectedArea - ThresholdArea[i]);
					pAlpha[iParticle] *= Fade;
				}
			}

			if (bCull)
			{
				nParallelParticles = RemoveParticle(pPR, pBatch, iParticle, nParallelParticles);
				continue;
			}

			iParticle++;
		}
	}
#endif

	// -------------------------------------------------------------------
	// Rotation Control
	if (!(m_RotCtrl.m_ValueType & CPropertyControl::VALUETYPE_ZERO))
	{
		uint nParallelParticles = _pPR->m_NumParallelParticles;
		for (iParticle = 0; iParticle < nParallelParticles; iParticle++)
		{
			const fp32 IndividualTime = _pPR->m_Time + m_SinRandTimeSpread * GetRand(pBatch->m_iRandseed[iParticle]);
			pBatch->m_Rot[iParticle] = m_RotCtrl.GetFluctS(IndividualTime, pBatch->m_TimeFraction[iParticle], &g_SinRandTable, pBatch->m_iRandseed[iParticle]);
		}
	}

	// -------------------------------------------------------------------
	// Colors
	Generate_Colors(_pPR, _pBatch, GenerateFlags);

	// -------------------------------------------------------------------
	if (!Generate_Render(_pPR, _pBatch, _pRenderParams, GenerateFlags))
		return false;

	_pPR->m_NumParticles += _pPR->m_NumParallelParticles;


	return true;
}

//----------------------------------------------------------------------

bool ParticlesOptClassName::GenerateParticle(const CXR_Model_Custom_RenderParams* M_RESTRICT _pRenderParams, 
                                             PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch,
                                             int _iParticleID, fp32 _EmissionTime, fp32 _ParticleTime, int _iRandseed)
{
	MAUTOSTRIP(ParticlesOptClassName_GenerateParticle, false);
	if(_ParticleTime < 0.0f)
		return true;

	int idx = _pPR->m_NumParallelParticles;
	_pBatch->m_ID[idx] = _iParticleID;
	_pBatch->m_EmissionTime[idx] = _EmissionTime;
	_pBatch->m_Time[idx] = _ParticleTime;
	_pBatch->m_iRandseed[idx] = _iRandseed;
	_pPR->m_NumParallelParticles = ++idx;

	if (idx == PARALELLPARTICLES_WINDOWSIZE)
	{
		if (!GenerateParallelParticles(_pPR, _pBatch, _pRenderParams))
			return false;

		_pPR->m_NumParallelParticles = 0;
	}
	return true;
}

//----------------------------------------------------------------------

// Return false if reaching particle overflow.
bool ParticlesOptClassName::GenerateParticles(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const CXR_Model_Custom_RenderParams* M_RESTRICT _pRenderParams)
{
	MAUTOSTRIP(ParticlesOptClassName_GenerateParticles, false);

	PerRenderVariables* M_RESTRICT pPR = _pPR;
	const CMHistory* M_RESTRICT pHistory = pPR->m_pHistory;
	CBatch* M_RESTRICT pBatch = _pBatch;

	fp32 Time = pPR->m_Time;

	// Test "this should never happend" cases. Fail totally if they fail.
	if (pHistory != NULL)
	{
		if (pHistory->IsEmpty())
			return false;

		fp32 OldestPossibleParticleTime = Max(0.0f, Time - m_MaxParticleDuration);
		if (pHistory->GetOldestTime() > OldestPossibleParticleTime)
			return false;

		if (pHistory->GetLatestTime() < Time)
			return false;
	}

	fp32 precalc0 = m_ParticleTimecellDuration * m_ParticleTimecellSpread; // TimeCellOffset

	uint nCellsToBacktrace = pPR->m_NumCellsToBacktrace;
	int nTotalCells = pPR->m_NumTotalCells;
	int nParticlesPerTimecell = m_NumParticlesPerTimecell;
	uint IDDisableMask = pPR->m_IDDisableMask;
	fp32  ParticleTimecellDuration = m_ParticleTimecellDuration;
	uint32 RandseedBase = _pPR->m_RandseedBase + nTotalCells + m_SeedCtrl * 100 - 100;

	int CellID = nTotalCells * nParticlesPerTimecell;
	fp32 EmissionTimeBase = fp32(fp64(nTotalCells) * ParticleTimecellDuration);

	for (uint iCell = 0; iCell < nCellsToBacktrace; iCell++)
	{
		uint32 Randseed = RandseedBase;
		for (uint iParticle = 0; iParticle < nParticlesPerTimecell; iParticle++)
		{
			Randseed += 100;

			uint ParticleID = CellID + iParticle;
			if (ParticleID & IDDisableMask)
				continue;

			fp32 SeparationTime = GetRandConst(Randseed) * precalc0;
			fp32 EmissionTime = EmissionTimeBase + SeparationTime;
			fp32 ParticleTime = Time - EmissionTime;

			if (!GenerateParticle(_pRenderParams, pPR, pBatch, ParticleID, EmissionTime, ParticleTime, Randseed))
				return false;
		}

		CellID -= nParticlesPerTimecell;
		RandseedBase--;
		EmissionTimeBase -= ParticleTimecellDuration;
	}

	// Generate residual parallel particles.
	if (pPR->m_NumParallelParticles > 0)
		return GenerateParallelParticles(_pPR, _pBatch, _pRenderParams);

	return true;
}

//----------------------------------------------------------------------

void ParticlesOptClassName::ModifyEntry(CMat4Dfp32& _Matrix, fp32& _Time)
{
	MAUTOSTRIP(ParticlesOptClassName_ModifyEntry, MAUTOSTRIP_VOID);
	if (m_SystemFlags & PARTICLE_FLAGS_WORLDROT)
		_Matrix.Unit3x3();

	_Time = (_Time + m_TimeOffset) * m_TimeScale;

	if(m_OptFlags & OPTFLAGS_DISTRIBUTIONROTATION)
	{
		_Matrix.M_x_RotZ(m_DistributionRotation[2] * _Time);
		_Matrix.M_x_RotY(m_DistributionRotation[1] * _Time);
		_Matrix.M_x_RotX(m_DistributionRotation[0] * _Time);
	}
}

//----------------------------------------------------------------------

void ParticlesOptClassName::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	fp32 Time = ( _pAnimState->m_AnimTime0.GetTime() + m_TimeOffset ) * m_TimeScale;

	if (m_SystemFlags & PARTICLE_FLAGS_FORCEEARLY)
		Time += 0.025f;

	fp32 ParticleEmissionStop = m_ParticleEmissionStop;
	fp32 InActive = M_FSel(-Time, 1, M_FSel(-ParticleEmissionStop, 0, M_FSel(Time - (ParticleEmissionStop + m_MaxParticleDuration), 1, 0)));
	if(InActive != 0)
		return;

	CXR_Model_Custom::OnRender(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS, _pAnimState, _WMat, _VMat, _Flags);
}


void ParticlesOptClassName::RenderPerRenderVariables(const CXR_AnimState* _pAnimState, PerRenderVariables* M_RESTRICT _pPR)
{
	_pPR->m_nSpawnBoxes = 0;
	_pPR->m_SkeletonType = 0;
	_pPR->m_pSkeleton = NULL;
	_pPR->m_pSpawnBoxes = NULL;
	_pPR->m_pSkeletonInst = NULL;
}


void ParticlesOptClassName::Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat)
{
	MSCOPESHORT(ParticlesOptClassName::Render);
//		M_CALLGRAPH;
//		MSCOPE(Mondelore, Mondelore);
	//MAUTOSTRIP(ParticlesOptClassName_Render, MAUTOSTRIP_VOID);

//	M_ASSERT(_pBatch, "!");

/*
	CBox3Dfp32 BBox;
	GetBound_Box(BBox, _pAnimState);
	CVec3Dfp32 BBMinDistanceVector;
	BBox.GetMinDistance(BBMinDistanceVector);
	fp32 BBMinDistance = BBMinDistanceVector.Length();
*/

	PerRenderVariables M_ALIGN(128) PR;
	CBatch M_ALIGN(128) Batch;
	PR.m_TimeDebug.Reset();
	PR.m_DebugCount = 0;

	RenderPerRenderVariables(_pAnimState, &PR);

	PR.m_Time = ( _pAnimState->m_AnimTime0.GetTime() + m_TimeOffset ) * m_TimeScale;

	if (m_SystemFlags & PARTICLE_FLAGS_FORCEEARLY)
	{
		PR.m_Time += 0.025f;		/* was:  += SERVER_TIMEPERFRAME * 0.5f; */
	}

	// If time is <= 0 then the system has not been activated yet
	if(PR.m_Time <= 0.0f)
		return;

	if ((m_ParticleEmissionStop > 0) && (PR.m_Time > (m_ParticleEmissionStop + m_MaxParticleDuration)))
	{
//		m_OptFlags	&= ~OPTFLAGS_ISALIVE;
		return;
	}
	
	PR.m_Flags = m_SystemFlags;

//	m_OptFlags	|= OPTFLAGS_ISALIVE;

	PR.m_ViewportXScale = _pRenderParams->m_pVBM->Viewport_Get()->GetXScale();

	if (!(PR.m_Flags & PARTICLE_FLAGS_NOLOD))
	{
		CVec3Dfp32 WorldPos = CVec3Dfp32::GetRow(_WMat, 3);
		CVec3Dfp32 CameraPos = WorldPos * _VMat;

		fp32 CameraDistance = CameraPos.LengthSqr();
		if (_pRenderParams->m_pEngine)
			CameraDistance = GetCorrectedDistance(_pRenderParams->m_pEngine, CameraDistance);

		// Distant LODing
		{
			if (CameraDistance > (4000.0f*4000.0f))
				return;

			PR.m_lIDFade[0] = GetDepthFade(CameraDistance, 200*200, 300*300);
			PR.m_lIDFade[1] = GetDepthFade(CameraDistance, 550*550, 650*650);
			PR.m_lIDFade[2] = GetDepthFade(CameraDistance, 850*850, 1000*1000);
			PR.m_lIDFade[3] = GetDepthFade(CameraDistance, 1800*1800, 2000*2000);
			PR.m_lIDFade[4] = GetDepthFade(CameraDistance, 3700*3700, 4000*4000);
		}
/*
		// CloseUp LODing.
		{
			fp32 MaxParticleSize = Max(Max(m_SizeCtrl.m_StartMin, m_SizeCtrl.m_StartMax), Max(m_SizeCtrl.m_EndMin, m_SizeCtrl.m_EndMax));
			CBox3Dfp32 BBox;
			GetBound_Box(BBox, _pAnimState);
			CVec3Dfp32 BBSize = BBox.m_Max - BBox.m_Min;
			fp32 BBVolume = BBSize[0] * BBSize[1] * BBSize[2];
			fp32 ParticleVolume = MaxParticleSize * MaxParticleSize * MaxParticleSize; // Should mayby be square or cubic.

//				BBVolume = sqrt(BBVolume / 10000) * 10000;
//				ParticleVolume = Sqr(ParticleVolume / 1000) * 1000;

			fp32 Density = (fp32)m_MaxNumParticles * ParticleVolume / BBVolume;

//				Density = Sqr(Density);

			fp32 CellSize = 2 * Density;
			
			PR.m_lIDFade[0] *= GetDepthFade(CameraDistance, 31.0f * CellSize, 15.0f * CellSize);
			PR.m_lIDFade[1] *= GetDepthFade(CameraDistance, 15.0f * CellSize, 7.0f * CellSize);
			PR.m_lIDFade[2] *= GetDepthFade(CameraDistance, 7.0f * CellSize, 3.0f * CellSize);
			PR.m_lIDFade[3] *= GetDepthFade(CameraDistance, 3.0f * CellSize, CellSize);
			PR.m_lIDFade[4] *= GetDepthFade(CameraDistance, CellSize, 0);
		}
*/

		PR.m_IDDisableMask = 0;
		if (PR.m_lIDFade[0] < 0.001f)
			PR.m_IDDisableMask |= 1;
		if (PR.m_lIDFade[1] < 0.001f)
			PR.m_IDDisableMask |= 2;
		if (PR.m_lIDFade[2] < 0.001f)
			PR.m_IDDisableMask |= 4;
		if (PR.m_lIDFade[3] < 0.001f)
			PR.m_IDDisableMask |= 8;
	}
	else
	{
		PR.m_lIDFade[0] = 1;
		PR.m_lIDFade[1] = 1;
		PR.m_lIDFade[2] = 1;
		PR.m_lIDFade[3] = 1;
		PR.m_lIDFade[4] = 1;
		PR.m_IDDisableMask = 0;
	}
	
	if (m_AlphaCtrl.m_ValueType & CPropertyControl::VALUETYPE_ZERO)
		return;

#if defined( M_RTM ) && defined( PLATFORM_CONSOLE )
//	PR.m_pWC = 0;
#else
//	PR.m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));
#endif

	// Will be max accumulated during particle generation.
	PR.m_NumParticles = 0;
	PR.m_NumParallelParticles = 0;
	PR.m_bOverflow = false;

//		PR.m_Time = (_pAnimState->m_AnimTime0 + MoveForth) * m_TimeScale;
//		m_Duration = _pAnimState->m_AnimTime1;
	
	if (false)
		ConOutL(CStrF("Render: Time = %f (Unscaled = %f)", PR.m_Time, _pAnimState->m_AnimTime0.GetTime()));

	PR.m_pHistory = GetHistory(_pAnimState);
	if (!(PR.m_Flags & PARTICLE_FLAGS_NOHISTORY) && (PR.m_pHistory == NULL))
	{
#ifndef M_RTM
		PR.m_pHistory = ForceGetHistory(_pAnimState);
		if (PR.m_pHistory == NULL)
			PR.m_Flags |= PARTICLE_FLAGS_NOHISTORY;
#else
		PR.m_Flags |= PARTICLE_FLAGS_NOHISTORY;
#endif
	}

	if (PR.m_pHistory != NULL)
		PR.m_pHistory->SetEntryDuration(m_HistoryEntryDelay);

	SetLocalToWorld(&PR, &Batch, _WMat);
	//_pBatch->m_LocalToWorld = _WMat;

	PR.m_Duration = _pAnimState->m_AnimTime1.GetTime(); // CMTIMEFIX
	
	if (PR.m_Duration > 0)
		PR.m_InvDuration = 1.0f / PR.m_Duration;
	else
		PR.m_InvDuration = 0;

	if (PR.m_pHistory != NULL)
	{
		CMat4Dfp32 Matrix = _WMat;
		const int Flags = _pAnimState->m_Data[3];

		if (PR.m_Flags & PARTICLE_FLAGS_WORLDROT)
			Matrix.Unit3x3();

		if(m_OptFlags & OPTFLAGS_DISTRIBUTIONROTATION)
		{
			Matrix.M_x_RotZ(m_DistributionRotation[2] * PR.m_Time);
			Matrix.M_x_RotY(m_DistributionRotation[1] * PR.m_Time);
			Matrix.M_x_RotX(m_DistributionRotation[0] * PR.m_Time);
		}
		PR.m_pHistory->AddEntry(Matrix, PR.m_Time, Flags);
	}
	else
	{
		if (PR.m_Flags & PARTICLE_FLAGS_WORLDROT)
			Batch.m_LocalToWorld.Unit3x3();

		if(m_OptFlags & OPTFLAGS_DISTRIBUTIONROTATION)
		{
			Batch.m_LocalToWorld.M_x_RotZ(m_DistributionRotation[2] * PR.m_Time);
			Batch.m_LocalToWorld.M_x_RotY(m_DistributionRotation[1] * PR.m_Time);
			Batch.m_LocalToWorld.M_x_RotX(m_DistributionRotation[0] * PR.m_Time);
		}
	}

	if ((PR.m_Flags & PARTICLE_FLAGS_FADESTILL) && ((PR.m_pHistory != NULL) && PR.m_pHistory->IsSingularity()))
		return;

	if (false)
		ConOutL(CStrF("Render: Time = %f, OldestTime = %f", PR.m_Time, PR.m_Time - m_MaxParticleDuration));

	PR.m_DurationFade = ::GetFade(PR.m_Time / m_TimeScale, PR.m_Duration, m_FadeInTime, m_FadeOutTime);
	
	//ConOut(CStrF("Time = %f, Duration %f, DurationFade = %f", PR.m_Time / m_TimeScale, PR.m_Duration, PR.m_DurationFade));

	PR.m_RandseedBase = ((PR.m_Flags & PARTICLE_FLAGS_RANDOMSEED) && PR.m_pHistory) ? PR.m_pHistory->m_CreationTick : _pAnimState->m_Anim0;
	
	CMat4Dfp32 InvWorldToCamera;
	_VMat.InverseOrthogonal(InvWorldToCamera);
	PR.m_CameraLeft = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 0);
	PR.m_CameraUp = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 1);
	PR.m_CameraFwd = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 2);
	PR.m_CameraPos = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 3);

	// Calculate the total number of cell that has been passed during full lifetime of this model.
	PR.m_NumTotalCells = Max(TruncToInt(PR.m_Time / m_ParticleTimecellDuration), 0);

	// Calculate the number of time cells needed to cover the whole duration of a particle.
	// Each time cell is PARTICLE_MIN_EMISSION_DELAY long in time.
	PR.m_NumCellsToBacktrace = TruncToInt(m_MaxParticleDuration / m_ParticleTimecellDuration);
	PR.m_NumCellsToBacktrace = Max(Min(PR.m_NumTotalCells, PR.m_NumCellsToBacktrace) + 1, 0);

//		int CalculatedMaxNumParticles = m_NumCellsToBacktrace * m_NumParticlesPerTimecell;

	{
		const fp32 MaxSize = Max(Max(m_SizeCtrl.m_StartMin, m_SizeCtrl.m_StartMax), Max(m_SizeCtrl.m_EndMin, m_SizeCtrl.m_EndMax));
		const fp32 MaxScreenSize = MaxSize; // FIXME: Use camera distance also.
		if (MaxScreenSize > 15)
			PR.m_Flags |= PARTICLE_FLAGS_QUADS;
	}

	// Just give one particles margin, to try to avoid any clamping artifacts.
/*		if (PR.m_Flags & PARTICLE_FLAGS_QUADS)
	{
		if (!m_DVB.Create(this, PR.m_pVBM, (m_MaxNumParticles + 1) * 4, (m_MaxNumParticles + 1) * 2, ))
			return;
	}
	else
	{
		if (!m_DVB.Create(this, PR.m_pVBM, (m_MaxNumParticles + 1) * 3, (m_MaxNumParticles + 1) * 1, ))
			return;
	}

	m_DVB.GetVB()->m_Priority += m_RenderPriorityBias;
*/		
/*	if (PR.m_pWC != NULL)
		PR.m_pWC->RenderMatrix(Batch.m_LocalToWorld, 10);*/

	PR.m_pChain = NULL;

	GenerateParticles(&PR, &Batch, _pRenderParams);
//		PR.m_bOverflow |= !bResult;

//		if (m_DVB.IsValid())
	if (PR.m_pChain)
	{
		if (PR.m_bOverflow && (PR.m_Flags & PARTICLE_FLAGS_SHOWOVERFLOW))
			PR.m_SK.Create(GetSurfaceContext(), _pRenderParams->m_pEngine, _pAnimState, 0);
		else
			PR.m_SK.Create(GetSurfaceContext(), _pRenderParams->m_pEngine, _pAnimState, m_iSurface);

		if (false)
			ConOut(CStrF("NumParticles = %d", PR.m_NumParticles));

		CXR_VertexBuffer VB;
		VB.SetVBChain(PR.m_pChain);
		CXW_Surface* pSurf = PR.m_SK.m_pSurface;
		if (pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT)
			VB.m_Priority = _pRenderParams->m_RenderInfo.m_BasePriority_Transparent;
		else
			VB.m_Priority = _pRenderParams->m_RenderInfo.m_BasePriority_Opaque;
		VB.m_Priority += m_RenderPriorityBias * TransparencyPriorityBiasUnit;
		VB.m_Priority += pSurf->m_PriorityOffset * 0.001f;

//ConOut(CStrF("Prior %.3f", pVB->m_Priority));

//			m_DVB.Render(_VMat);
//			m_SK.Render(m_DVB.GetVB(), PR.m_pVBM, _pRenderParams->m_pEngine);

		CMat4Dfp32* pVMat = _pRenderParams->m_pVBM->Alloc_M4(_VMat);
		if (!pVMat) return;
		VB.Matrix_Set(pVMat);


		// Enumerate lights and transform them to model space
		if ((PR.m_Flags & PARTICLE_FLAGS_LIGHTING) && 
			(PR.m_SK.m_pSurface->m_Flags & XW_SURFFLAGS_LIGHTING) && 
			_pRenderParams->m_pEngine && 
			_pRenderParams->m_pEngine->m_pSceneGraphInstance)
		{
			CXR_RenderSurfExtParam Params;
			const int MaxLights = 3;
			CXR_Light const* lpLights[MaxLights];

			CBox3Dfp32 Box, BoxW;
			GetBound_Box(Box, _pAnimState);
			Box.Transform(_WMat, BoxW);

			CMat4Dfp32 WMatInv;
			_WMat.InverseOrthogonal(WMatInv);
			int nLights = _pRenderParams->m_pEngine->m_pSceneGraphInstance->SceneGraph_Light_Enum(BoxW, lpLights, MaxLights);

			CXR_Light lLights[MaxLights];
			for(int i = 0; i < nLights; i++)
			{
				lLights[i] = *(lpLights[i]);
//				lLights[i].Transform(WMatInv);
			}

			Params.m_pLights = lLights;
			Params.m_nLights = nLights;

			CVec4Dfp32 lLFAxes[6];
			if (_pRenderParams->m_RenderInfo.m_pLightVolume)
			{
				CVec3Dfp32 BoundV[8];
				BoxW.GetVertices(BoundV);
				_pRenderParams->m_RenderInfo.m_pLightVolume->Light_EvalPointArrayMax(BoundV, 8, lLFAxes);

//				_pRenderParams->m_RenderInfo.m_pLightVolume->Light_EvalPoint(CVec3Dfp32::GetRow(_WMat, 3),lLFAxes);
				Params.m_pLightFieldAxes = lLFAxes;
			}
			PR.m_SK.Render(&VB, _pRenderParams->m_pVBM, _pRenderParams->m_pEngine, &Params, RENDERSURFACE_LIGHTNONORMAL);
#ifdef M_Profile
			if(_pRenderParams->m_pEngine->m_DebugFlags & XR_DEBUGFLAGS_SHOWLIGHTCOUNT)
			{
				CVec3Dfp32 lV[MaxLights*2];
				CPixel32 lC[MaxLights*2];

				uint nV = 0;
				for(uint i = 0;i < nLights; i++)
				{
					const CXR_Light * pL = &lLights[i];

					if( pL->GetIntensitySqrf() < 0.0001f )
						continue;

					lV[nV] = _WMat.GetRow(3);
					lV[nV+1] = pL->GetPosition();
					lC[nV] = 0xff202000;
					lC[nV+1] = 0xffffff00;
					nV += 2;

		//			_pRenderParams->m_pVBM->RenderWire(_VMat,_WMat.GetRow(3), pL->GetPosition(),CPixel32(255,255,255,255));
				}
				if (nV)
					_pRenderParams->m_pVBM->RenderWires(_VMat, lV, g_IndexRamp32, nV, lC, false);
			}
#endif // M_Profile
		}
		else
			PR.m_SK.Render(&VB, _pRenderParams->m_pVBM, _pRenderParams->m_pEngine);
	}

//////////////////////////	M_TRACE("Quad time %.4f ms, Particles %d\n", PR.m_TimeDebug.GetTime() * 1000.0f, PR.m_DebugCount);
}

//----------------------------------------------------------------------

void ParticlesOptClassName::OnEvalKey(const CRegistry *_pReg)
{
	MAUTOSTRIP(ParticlesOptClassName_OnEvalKey, MAUTOSTRIP_VOID);
	CStr KeyName = _pReg->GetThisName();
	uint32 KeyHash = StringToHash(KeyName);
	CStr Value = _pReg->GetThisValue();
	const int Valuei = _pReg->GetThisValue().Val_int();
	const fp32 Valuef = (fp32)_pReg->GetThisValue().Val_fp64();

	switch (KeyHash)
	{
	case MHASH1('SU'): // "SU"
		{
			CFStr SurfaceName = Value.GetStrMSep(" #");
			m_iSurface = GetSurfaceID(SurfaceName);
			m_RenderPriorityBias = (fp32)Value.Val_fp64();
			break;
		}

	case MHASH1('MP'): // "MP"
		{
			m_MaxNumParticles = Valuei;
			break;
		}

	case MHASH1('TS'): // "TS"
		{
			m_TimeScale = Valuef;
			break;
		}

	case MHASH1('TO'): // "TO"
		{
			m_TimeOffset = Valuef;
			break;
		}

	case MHASH1('DU'): // "DU"
		{
			m_ParticleDuration = Valuef;
			break;
		}
	case MHASH1('DUN'): // "DUN"
		{
			m_ParticleDurationNoise = Valuef;
			break;
		}

	case MHASH1('OF'): // "OF"
		{
			m_ParticleOffset = Valuef;
			break;
		}
	case MHASH1('OFN'): // "OFN"
		{
			m_ParticleOffsetNoise = Valuef;
			break;
		}

	case MHASH1('VE'): // "VE"
		{
			m_ParticleVelocity = Valuef;
			break;
		}
	case MHASH1('VEN'): // "VEN"
		{
			m_ParticleVelocityNoise = Valuef;
			break;
		}

	case MHASH1('SD'): // "SD"
		{
			m_ParticleSlowdownPower = Valuef;
			break;
		}

	case MHASH1('SMT'): // "SMT"
		{
			m_Particle_StopMotionTime = Valuef;
			break;
		}

	case MHASH1('LO'): // "LO"
		{
			m_LocalPositionOffset.ParseString(Value);
			break;
		}

	case MHASH1('AX'): // "AX"
		{
			m_ParticleAcceleration.ParseString(Value);
			break;
		}
	case MHASH1('AXN'): // "AXN"
		{
			m_ParticleAccelerationNoise.ParseString(Value);
			break;
		}

	case MHASH1('SPA'): // "SPA"
		{
			m_AngleSpreadA = Valuef;
			break;
		}
	case MHASH1('SPB'): // "SPB"
		{
			m_AngleSpreadB = Valuef;
			break;
		}

	case MHASH1('OFA'): // "OFA"
		{
			m_AngleOffsetA = Valuef;
			break;
		}
	case MHASH1('OFB'): // "OFB"
		{
			m_AngleOffsetB = Valuef;
			break;
		}

	case MHASH1('DOFA'): // "DOFA"
		{
			m_AngleOffsetChangeA = Valuef;
			break;
		}
	case MHASH1('DOFB'): // "DOFB"
		{
			m_AngleOffsetChangeB = Valuef;
			break;
		}

	case MHASH1('MV0'): // "MV0"
		{
			m_MoveCtrl.m_StartMin = Valuef;
			break;
		}
	case MHASH1('MV1'): // "MV1"
		{
			m_MoveCtrl.m_StartMax = Valuef;
			break;
		}
	case MHASH1('MV2'): // "MV2"
		{
			m_MoveCtrl.m_EndMin = Valuef;
			break;
		}
	case MHASH1('MV3'): // "MV3"
		{
			m_MoveCtrl.m_EndMax = Valuef;
			break;
		}
	case MHASH1('MV4'): // "MV4"
		{
			m_MoveCtrl.SetSpeed(Valuef);
			break;
		}

	case MHASH1('SZ0'): // "SZ0"
		{
			m_SizeCtrl.m_StartMin = Valuef;
			break;
		}
	case MHASH1('SZ1'): // "SZ1"
		{
			m_SizeCtrl.m_StartMax = Valuef;
			break;
		}
	case MHASH1('SZ2'): // "SZ2"
		{
			m_SizeCtrl.m_EndMin = Valuef;
			break;
		}
	case MHASH1('SZ3'): // "SZ3"
		{
			m_SizeCtrl.m_EndMax = Valuef;
			break;
		}
	case MHASH1('SZ4'): // "SZ4"
		{
			m_SizeCtrl.SetSpeed(Valuef);
			break;
		}

	case MHASH1('AL0'): // "AL0"
		{
			m_AlphaCtrl.SetStart(Valuef);
			break;
		}
	case MHASH1('AL1'): // "AL1"
		{
			m_AlphaCtrl.SetEnd(Valuef);
			break;
		}

	case MHASH1('RT0'): // "RT0"
		{
			m_RotCtrl.m_StartMin = Valuef;
			break;
		}
	case MHASH1('RT1'): // "RT1"
		{
			m_RotCtrl.m_StartMax = Valuef;
			break;
		}
	case MHASH1('RT2'): // "RT2"
		{
			m_RotCtrl.m_EndMin = Valuef;
			break;
		}
	case MHASH1('RT3'): // "RT3"
		{
			m_RotCtrl.m_EndMax = Valuef;
			break;
		}
	case MHASH1('RT4'): // "RT4"
		{
			m_RotCtrl.SetSpeed(Valuef);
			break;
		}

	case MHASH1('FI'): // "FI"
		{
			m_ParticleFadeInTime = Valuef;
			break;
		}

	case MHASH1('FS'): // "FS"
		{
			m_ParticleFadeStillThreshold = Valuef;
			break;
		}

	case MHASH1('LS'): // "LS"
		{
			m_ParticleAlignLengthScale = Valuef;
			break;
		}

	case MHASH1('CO'): // "CO"
		{
			m_ParticleColor.ParseColor(Value);
			break;
		}
	case MHASH1('CON'): // "CON"
		{
			m_ParticleColorNoise.ParseColor(Value);
			break;
		}

	case MHASH1('EP'): // "EP"
		{
			m_ParticleEmissionProbability = Valuef;
			break;
		}
	case MHASH1('EPC'): // "EPC"
		{
			m_ParticleEmissionProbability_TimeScale = Valuef;
			break;
		}
	case MHASH1('TC'): // "TC"
		{
			m_ParticleTimecellDuration = Valuef;
			break;
		}
	case MHASH1('PP'): // "PP"
		{
			m_NumParticlesPerTimecell = Valuei;
			break;
		}
	case MHASH1('TCS'): // "TCS"
		{
			m_ParticleTimecellSpread = Valuef;
			break;
		}
	case MHASH1('ES'): // "ES"
		{
			m_ParticleEmissionStop = Valuef;
			break;
		}

	case MHASH1('FL'): // "FL"
		{
			m_SystemFlags = Value.TranslateFlags(g_lpTranslateParticleFlags);
			m_SystemFlags |= Value.TranslateFlags(g_lpTranslateParticleFlagsShort);
			break;
		}

	case MHASH1('DI'): // "DI"
		{
			m_DistributionPrimitive = Value.TranslateInt(g_lpTranslatePrimitive);
			break;
		}
	case MHASH1('DIS'): // "DIS"
		{
			m_DistributionSize.ParseString(Value);
			break;
		}
	case MHASH1('HO'): // "HO"
		{
			m_bHollowDistribution = Valuei != 0;
			break;
		}
	case MHASH1('DR'): // "DR"
		{
			m_DistributionRotation.ParseString(Value);
			break;
		}

	case MHASH1('RS'): // "RS"
		{
			m_SeedCtrl = Valuei;
			break;
		}

	case MHASH1('TA'): // "TA"
		{
			m_UVMapFrames = ((uint32)Value.GetStrSep(" ").Val_int()) & 0x00ffffff;
			
			int Width = Value.GetStrSep(" ").Val_int();
			int Height = Value.GetStrSep(" ").Val_int();
			int SubWidth = Value.GetStrSep(" ").Val_int();
			int SubHeight = Value.GetStrSep(" ").Val_int();

			if(m_UVMapFrames && Width && Height && SubWidth && SubHeight)
			{
				m_UVMapFramesU = 1.0f / (fp32)(Width / SubWidth);
				m_UVMapFramesV = 1.0f / (fp32)(Height / SubHeight);
				m_UVMapFramesMul = fp32(SubWidth) / fp32(Width);
			}
			else
			{
				// Reset on invalid parameters
				m_UVMapFrames = 0;
				m_UVMapFramesU = 0.0f;
				m_UVMapFramesV = 0.0f;
				m_UVMapFramesMul = 0.0f;
			}

			break;
		}

	default:
		{
			CXR_Model_Custom::OnEvalKey(_pReg);
			break;
		}
	}
}

//----------------------------------------------------------------------

void ParticlesOptClassName::OnCreate(const char *_keys)
{
	MAUTOSTRIP(ParticlesOptClassName_OnCreate, MAUTOSTRIP_VOID);
/*
#ifndef	PLATFORM_PS2
	if ((_pBatch == NULL) || (m_lBatchData.Len() == 0))
	{
		m_lBatchData.SetLen(sizeof(CBatch) + 16);
		FillChar(m_lBatchData.GetBasePtr(), m_lBatchData.ListSize(), 0);
		_pBatch = (CBatch*) ( ((aint)m_lBatchData.GetBasePtr() + 15) & ~15);
		_pBatch->InitBlitArea();
		_pPR		= &m_PR;
	}
#else
	// Place these on the scratchpad for faster access
	_pBatch	= (CXR_Model_Particles::CBatch*)0x70000000;
	_pPR		= (CXR_Model_Particles::PerRenderVariables*)(((0x70000000+sizeof(CXR_Model_Particles::CBatch))+63)&~63);
#endif
*/
	//JK-NOTE: Move this one here, no need to do it runtime.. i think :p
	CXR_Util::Init();

	//m_Keys = _keys;
	SetParameters_Default();
	ParseKeys(_keys);
	ComputeParameters();
	
//	m_OptFlags	|= OPTFLAGS_ISALIVE;
//	m_SystemFlags |= PARTICLE_FLAGS_NOHISTORY;
}

//----------------------------------------------------------------------

void ParticlesOptClassName::ComputeParameters()
{
	MAUTOSTRIP(ParticlesOptClassName_ComputeParameters, MAUTOSTRIP_VOID);
	// Typically some values lay in certain intervals, so to reduce the need of
	// redundant digits we multiply or divide by 10.
	m_DistributionSize *= 10.0f;
	m_DistributionRotation *= (1.0f / 10.0f);
	m_MoveCtrl.m_FluctTimescale *= (1.0f / 10.0f);
	m_SizeCtrl.m_FluctTimescale *= (1.0f / 10.0f);
	m_RotCtrl.m_FluctTimescale *= (1.0f / 10.0f);

	m_MoveCtrl.Optimize();
	m_SizeCtrl.Optimize();
	m_RotCtrl.Optimize();
	m_AlphaCtrl.Optimize();

	if (m_Particle_StopMotionTime)
		m_Particle_InvStopMotionTime = 1.0f / m_Particle_StopMotionTime;
	else
		m_Particle_StopMotionTime = 0;

	if (m_ParticleSlowdownPower)
		m_InvParticleSlowdownPower = 1.0f / m_ParticleSlowdownPower;
	else
		m_InvParticleSlowdownPower = 0.0;

	m_InvParticleSlowdownPowerPlusOne = m_InvParticleSlowdownPower + 1.0f;
	m_InvInvParticleSlowdownPowerPlusOne = 1.0f / m_InvParticleSlowdownPowerPlusOne;

	if (m_ParticleFadeInTime)
		m_InvParticleFadeInTime = 1.0f / m_ParticleFadeInTime;
	else
		m_InvParticleFadeInTime = 0;

	// Divided by m_TimeScale before, but lightning shockwave faded to quick.
	m_FadeInTime = m_ParticleFadeInTime * 2.0f;
	m_FadeOutTime = m_ParticleFadeInTime * 2.0f;

	m_MaxParticleDuration = m_ParticleDuration + m_ParticleDurationNoise / 2.0f;

	// Why -2?
	m_HistoryEntryDelay = m_MaxParticleDuration * ( 1.2f / (fp32)(HISTORY_LENGTH - 2) );

	m_ParticleAlignLengthScale /= 100.0f;
	m_EmitterSpeedAlignScale = LERP(m_ParticleAlignLengthScale * -200.0f, 1.0f, 0.5f);

	if ((m_AngleSpreadA != 0) || (m_AngleOffsetA != 0) || (m_AngleOffsetChangeA != 0) || 
		(m_AngleSpreadB != 0) || (m_AngleOffsetB != 0) || (m_AngleOffsetChangeB != 0))
		m_OptFlags |= OPTFLAGS_ANGLES;

	if ((m_ParticleOffset != 0) || (m_ParticleOffsetNoise != 0))
		m_OptFlags |= OPTFLAGS_OFFSET;

	if ((m_ParticleVelocity != 0) || (m_ParticleVelocityNoise != 0))
		m_OptFlags |= OPTFLAGS_VELOCITY;

	if ((m_ParticleAcceleration != CVec3Dfp32(0)) || (m_ParticleAccelerationNoise != CVec3Dfp32(0)))
		m_OptFlags |= OPTFLAGS_ACCELERATION;

	if (m_DistributionSize != CVec3Dfp32(1))
		m_OptFlags |= OPTFLAGS_NONUNITDISTSIZE;

	if (m_LocalPositionOffset != CVec3Dfp32(0))
		m_OptFlags |= OPTFLAGS_LOCALOFFSET;

	if (m_ParticleColorNoise != 0)
		m_OptFlags |= OPTFLAGS_COLORNOISE;

	if (m_DistributionRotation != CVec3Dfp32(0))
		m_OptFlags |= OPTFLAGS_DISTRIBUTIONROTATION;

	if (m_SystemFlags & PARTICLE_FLAGS_ALIGN_EMITTERMOVEMENT)
		m_SystemFlags |= PARTICLE_FLAGS_ALIGN;

	if (m_SystemFlags & PARTICLE_FLAGS_ALIGN_SAFEQUAD)
		m_SystemFlags |= PARTICLE_FLAGS_ALIGN;

	if (m_UVMapFrames > 0)
		m_SystemFlags |= PARTICLE_FLAGS_USECUSTOMUV | PARTICLE_FLAGS_QUADS;
}

//----------------------------------------------------------------------

// Default
void
ParticlesOptClassName::SetParameters_Default()
{
	MAUTOSTRIP(ParticlesOptClassName_SetParameters_Default, MAUTOSTRIP_VOID);
	m_iSurface = GetSurfaceID("DotParticle");
	m_RenderPriorityBias = 0;
	
	m_SystemFlags = 0;

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
	m_EmitterSpeedAlignScale = 1.0f;

	m_SizeCtrl.Set(10.0f);

	m_AlphaCtrl.SetStart(1.0f);
	m_AlphaCtrl.SetEnd(0.0f);

	m_OptFlags = 0;

	m_SeedCtrl = 0;

	m_UVMapFrames = 0;
	m_UVMapFramesMul = 0.0f;
	m_UVMapFramesU = 0.0f;
	m_UVMapFramesV = 0.0f;
/*
	m_bAngles = false;
	m_bOffset = false;
	m_bVelocity = false;
	m_bAccelerate = false;
	m_bNonUnitDistributionSize = false;
	m_bLocalPositionOffset = false;
	m_bColorNoise = false;
*/
}


//----------------------------------------------------------------------

#ifdef TESTNEWPARTICLES
	MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Particles, CXR_Model_Custom);
#else
	MRTC_IMPLEMENT_DYNAMIC(CXR_Model_ParticlesOpt, CXR_Model_Custom);
#endif

//----------------------------------------------------------------------


/*

class CStatistics
{
public:
	enum 
	{
		e_Test_NegTime,
		e_Test_Probability,
		e_Test_Duration,
		e_Test_Emission,
		e_Test_Alpha_Linear,
		e_Test_Alpha_Constant,
		e_Test_Fade,

		e_NumCategories
	};
#if 0
	uint64 m_Times[e_NumCategories];
	uint m_NumInput[e_NumCategories];
	uint m_NumRemoved[e_NumCategories];

	uint m_iCategory;
	uint m_NumCalls;

	CStatistics()
	{
		for (uint i = 0; i < e_NumCategories; i++)
		{
			m_Times[i] = 0;
			m_NumInput[i] = 0;
			m_NumRemoved[i] = 0;
		}
		m_iCategory = 0;
		m_NumCalls = 0;
	}

	void AddCall()
	{
		m_NumCalls++;
	}

	void StartCategory(uint _iCategory, uint _nInput)
	{
		m_Times[_iCategory] -= MGetCPUClock();
		m_NumInput[_iCategory] += _nInput;
		m_iCategory = _iCategory;
	}

	void EndCategory()
	{
		m_Times[m_iCategory] += MGetCPUClock();
	}

	void RegisterRemoved()
	{
		m_NumRemoved[m_iCategory]++;
	}

	void Dump()
	{
		static const char* pNames[] = { "NegTime", "Probability", "Duration", "Emission", "Alpha_Linear", "Alpha_Constant", "Fade" };
		fp64 Inv = 1.0f / fp32(m_NumCalls);
		for (uint i = 0; i < e_NumCategories; i++)
		{
			M_TRACEALWAYS("\n%s:\n", pNames[i]);
			M_TRACEALWAYS("- %d\n", (int)(m_Times[i] / m_NumCalls));
			M_TRACEALWAYS("- %.2f, %.2f\n", m_NumInput[i] * Inv, m_NumRemoved[i] * Inv);
		}
	}
#else
	void AddCall() {}
	void StartCategory(uint , uint ) {}
	void EndCategory() {}
	void RegisterRemoved() {}
	void Dump() {}
#endif
};
CStatistics g_Statistics;



g_Statistics.StartCategory(CStatistics::e_Test_Fade, nParallelParticles);
g_Statistics.AddCall();
g_Statistics.StartCategory(CStatistics::e_Test_Emission, nParallelParticles);
g_Statistics.RegisterRemoved();
g_Statistics.EndCategory();
g_Statistics.StartCategory(CStatistics::e_Test_NegTime, nParallelParticles);
g_Statistics.RegisterRemoved();
g_Statistics.EndCategory();
g_Statistics.StartCategory(CStatistics::e_Test_Probability, nParallelParticles);
g_Statistics.RegisterRemoved();
g_Statistics.EndCategory();
g_Statistics.StartCategory(CStatistics::e_Test_Duration, nParallelParticles);
g_Statistics.RegisterRemoved();
g_Statistics.EndCategory();
g_Statistics.StartCategory(CStatistics::e_Test_Alpha_Linear, nParallelParticles);
g_Statistics.RegisterRemoved();
g_Statistics.EndCategory();
g_Statistics.StartCategory(CStatistics::e_Test_Alpha_Constant, nParallelParticles);
g_Statistics.RegisterRemoved();
g_Statistics.EndCategory();
g_Statistics.RegisterRemoved();
g_Statistics.EndCategory();


	static bool s_bDump = false;
	if (s_bDump)
	{
		s_bDump = false;
		g_Statistics.Dump();
	}

*/
