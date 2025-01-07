#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "CEnvelope.h"
#include "CDynamicVB.h"
#include "CSurfaceKey.h"

//----------------------------------------------------------------------

#define NUM_SPURTS				(40)
#define NUM_SPURT_SEGMENTS		(5)
#define SPURT_DURATION			(1.0f)
#define SPURT_RADIUS			(1.0f)
#define SPURT_LENGTH			(0.08f)
#define SPURT_WIDTH				(1.0f)
#define SPURT_SPREAD			(0.1f)
#define SPURT_GRAVITY			(200.0f)
#define SPURT_FORCE_RADIUS		(20.0f)
#define SPURT_TIMESPREAD		(0.3f)
#define SPURT_ALPHA				(1.0f)

#define MAXNUMBEAMSTRIPS		(NUM_SPURTS * NUM_SPURT_SEGMENTS)

//----------------------------------------------------------------------
// CXR_Model_BloodSpurt
//----------------------------------------------------------------------

class CXR_Model_BloodSpurt : public CXR_Model_Custom
{

	MRTC_DECLARE;

	//----------------------------------------------------------------------
	
private:

	int m_iSurface;

public:

	//----------------------------------------------------------------------
	
	fp32
	GetRand()
	{
		MAUTOSTRIP(CXR_Model_BloodSpurt_GetRand, 0);
		return MFloat_GetRand(m_Randseed++);
	}

	//----------------------------------------------------------------------

	void
	AnglesToVector(fp32 a, fp32 b, CVec3Dfp32& v)
	{
		MAUTOSTRIP(CXR_Model_BloodSpurt_AnglesToVector, MAUTOSTRIP_VOID);
		a *= _PI2;
		b *= _PI2;
		const fp32 cb = M_Cos(b);
		v.k[0] = M_Cos(a) * cb;
		v.k[1] = M_Sin(a) * cb;
		v.k[2] = M_Sin(b);
	}

	//----------------------------------------------------------------------

private:

	fp32					m_Time;
	fp32					m_Duration;
	fp32					m_TimeFraction;

	fp32					m_Radius;

	int32				m_Color, m_ColorRGB, m_ColorAlpha;
	CPixel32			m_LightColor;

	int32				m_RandseedBase, m_Randseed;

	CDynamicVB			m_DVB;
	CSurfaceKey			m_SK;
	CXR_VertexBuffer*	m_pVB;

//	CWireContainer*		m_pWC;

private:

	//----------------------------------------------------------------------

	void GenerateBlood(const CMat43fp32& _WMat, const CMat43fp32& _VMat)
	{
		CXR_BeamStrip pBeamStrips[MAXNUMBEAMSTRIPS];

		CVec3Dfp32 pos = CVec3Dfp32::GetMatrixRow(_WMat, 3);

		const fp32 Length = SPURT_LENGTH * m_Radius;
		const fp32 Radius = SPURT_RADIUS * m_Radius;
		const fp32 Duration = SPURT_DURATION * m_Duration;

		for (int iSpurt = 0; iSpurt < NUM_SPURTS; iSpurt++)
		{
			CVec3Dfp32 dir;
			AnglesToVector(SPURT_SPREAD * (GetRand() - 0.5f), SPURT_SPREAD * (GetRand() - 0.5f), dir);

			dir.MultiplyMatrix3x3(_WMat);

/*
			fp32 offset = (GetRand() * Duration);

			if ((m_Time < (Duration - offset)) || (m_Time > (Duration + offset)))
			{
				offset = 0.0f;
			}
			else
			{
				offset += m_Time;
				offset = M_FMod(offset, Duration) / Duration;
			}
*/

			fp32 offset = m_Time - (GetRand() * Duration * SPURT_TIMESPREAD);
			if ((offset < 0.0f) || (offset > Duration))
				offset = 0.0f;

			offset = offset / Duration;

			const fp32 fAlpha = SPURT_ALPHA * ((1.0f - offset) * fp32(m_ColorAlpha)) * (1.0f - Sqr(m_TimeFraction));
			int iAlpha = ((int32)fAlpha & 0xFF);

			for (int iSegment = 0; iSegment < NUM_SPURT_SEGMENTS; iSegment++)
			{
				const fp32 segmentfraction = (fp32)iSegment / (fp32)(NUM_SPURT_SEGMENTS - 1);
				fp32 radius = (offset * Radius - segmentfraction * Length);
				if (radius < 0)
					radius = 0;

				const fp32 radiusfraction = radius / Radius;

				int iBeamStrip = iSpurt * NUM_SPURT_SEGMENTS + iSegment;
				pBeamStrips[iBeamStrip].m_Flags = (iSegment == 0) ? CXR_BEAMFLAGS_BEGINCHAIN : 0;
				pBeamStrips[iBeamStrip].m_Pos = pos + dir * radius - CVec3Dfp32(0, 0, SPURT_GRAVITY) * Sqr(radiusfraction);
				pBeamStrips[iBeamStrip].m_Width = SPURT_WIDTH * InvSqrInv(M_Sin(segmentfraction * _PI));
				pBeamStrips[iBeamStrip].m_TextureYOfs = segmentfraction;
				pBeamStrips[iBeamStrip].m_Color = m_ColorRGB | (iAlpha << 24);
			}
		}

		CMat43fp32 unit;
		unit.Unit();

		CXR_Util::Render_BeamStrip2(m_pVBM, m_pVB, unit, _VMat, pBeamStrips, MAXNUMBEAMSTRIPS);
	}
	
	//----------------------------------------------------------------------
	
	bool
	Init(const CXR_AnimState* _pAnimState, const CMat43fp32& _LocalToWorld, const CMat43fp32& _WorldToCamera, const CMat43fp32& _LocalToCamera)
	{
		if (!_pAnimState) return false;

		m_Time = _pAnimState->m_AnimTime0;
		m_Duration = _pAnimState->m_AnimTime1;

//		m_RandseedBase = _pAnimState->m_Anim0;
		m_RandseedBase = 0;
		m_Randseed = m_RandseedBase;

		// Normal is NULL, since the lit point is considered to be omni.
		if (m_pWLS != NULL)
		{
			CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(_LocalToWorld, 3);
			m_WLS.CopyAndCull(m_pWLS, GetBound_Sphere(), Pos, 3, 2);
			m_WLS.AddLightVolume(m_RenderInfo.m_pLightVolume, Pos);
			m_WLS.InitLinks();

			CXR_WorldLightState::LightDiffuse(m_WLS.GetFirst(), 1, &Pos, NULL, true, &m_LightColor, 255, 1.7f);
		}
		else
			m_LightColor = 0xFFFFFFFF;

		m_Color = m_LightColor * CPixel32(_pAnimState->m_Colors[0]);
		m_ColorRGB = m_Color & 0x00FFFFFF;
		m_ColorAlpha = (m_Color >> 24) & 0xFF;

		fp32 force = _pAnimState->m_Anim0;
		fp32 damage = _pAnimState->m_Anim1;

		m_Radius = force * SPURT_FORCE_RADIUS;
		m_BoundRadius = m_Radius;

		if (m_Duration > 0.0f)
		{
			m_TimeFraction = m_Time / m_Duration;

			if (m_TimeFraction < 0.0f)
				return false;

			if (m_TimeFraction > 1.0f)
				return false;
		}

		int numVertices, numTriangles;

		numVertices = 0;
		numTriangles = 0;

/*
		if (!m_DVB.Create(this, m_pVBM, numVertices, numTriangles))
			return false;
*/

		m_pVB = AllocVB();
		if (m_pVB == NULL)
			return false;

		m_SK.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface);

//		m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));

		return true;
	}

	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		CMat43fp32 LocalToCamera;
		_WMat.Multiply(_VMat, LocalToCamera);

		if (!Init(_pAnimState, _WMat, _VMat, LocalToCamera))
			return;

		GenerateBlood(_WMat, _VMat);

/*
		if (m_DVB.IsValid())
		{
			m_DVB.Render(_VMat);
			m_SK.Render(m_DVB.GetVB(), m_pVBM, m_pEngine);
		}
*/

		m_SK.Render(m_pVB, m_pVBM, m_pEngine);
	}

	//----------------------------------------------------------------------
	
	virtual
	void
	OnCreate(const char *)
	{
		m_iSurface = GetSurfaceID("BloodSpurt01");
	}

};

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_BloodSpurt, CXR_Model_Custom);

//----------------------------------------------------------------------
