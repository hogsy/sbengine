#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Hook.h"

#include "CEnvelope.h"
#include "CDynamicVB.h"
#include "CSurfaceKey.h"

//----------------------------------------------------------------------

#define NUM_SPARKS				(20)
#define NUM_SPARK_SEGMENTS		(5)
#define SPARK_DURATION			(0.2f)
#define SPARK_RADIUS			(10.0f)
#define SPARK_LENGTH			(5.0f)
#define SPARK_WIDTH				(1.0f)

#define MAXNUMBEAMSTRIPS		(NUM_SPARKS * NUM_SPARK_SEGMENTS)

//----------------------------------------------------------------------
// CXR_Model_Fuse
//----------------------------------------------------------------------

class CXR_Model_FuseSparks : public CXR_Model_Custom
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
		MAUTOSTRIP(CXR_Model_FuseSparks_GetRand, 0);
		return MFloat_GetRand(m_Randseed++);
	}

	//----------------------------------------------------------------------
	
	fp32
	GetAbs(fp32 x)
	{
		MAUTOSTRIP(CXR_Model_FuseSparks_GetAbs, 0);
		return ((x < 0.0f) ? -x : x);
	}

	//----------------------------------------------------------------------

	void
	AnglesToVector(fp32 a, fp32 b, CVec3Dfp32& v)
	{
		MAUTOSTRIP(CXR_Model_FuseSparks_AnglesToVector, MAUTOSTRIP_VOID);
		a *= 2.0f * _PI;
		b *= 2.0f * _PI;
		v.k[0] = M_Cos(a) * M_Cos(b);
		v.k[1] = M_Sin(a) * M_Cos(b);
		v.k[2] = M_Sin(b);
	}

	//----------------------------------------------------------------------

public:

	fp32					m_Time;
	fp32					m_Duration;
	fp32					m_TimeFraction;

	fp32					m_Radius;

private:

	int32				m_RandseedBase, m_Randseed;

	CMat43fp32			m_SparksPos;

	int32				m_NumSegments;

	CXR_VertexBuffer*	m_pVBSparks;
	CSurfaceKey			m_SKSparks;

	CWireContainer*		m_pWC;

private:

	//----------------------------------------------------------------------

	bool GenerateSparks(const CMat43fp32& _VMat)
	{
		MAUTOSTRIP(CXR_Model_FuseSparks_GenerateSparks, false);

		if (m_Time == 0.0f)
			return false;

		CXR_BeamStrip pBeamStrips[MAXNUMBEAMSTRIPS];

		CVec3Dfp32 pos = CVec3Dfp32::GetMatrixRow(m_SparksPos, 3);

		for (int iSpark = 0; iSpark < NUM_SPARKS; iSpark++)
		{
			CVec3Dfp32 dir;
			AnglesToVector(GetRand(), GetRand(), dir);

			fp32 offset = m_Time + (GetRand() * SPARK_DURATION);
			offset = M_FMod(offset, SPARK_DURATION) / SPARK_DURATION;

			for (int iSegment = 0; iSegment < NUM_SPARK_SEGMENTS; iSegment++)
			{
				fp32 segmentfraction = (fp32)iSegment / (fp32)(NUM_SPARK_SEGMENTS - 1);
				fp32 radius = (offset * SPARK_RADIUS - segmentfraction * SPARK_LENGTH);
				if (radius < 0)
					radius = 0;

				pBeamStrips[iSpark * NUM_SPARK_SEGMENTS + iSegment].m_Flags = (iSegment == 0) ? CXR_BEAMFLAGS_BEGINCHAIN : 0;
				pBeamStrips[iSpark * NUM_SPARK_SEGMENTS + iSegment].m_Pos = pos + dir * radius;
				pBeamStrips[iSpark * NUM_SPARK_SEGMENTS + iSegment].m_Width = SPARK_WIDTH * M_Sin(segmentfraction * _PI);
				pBeamStrips[iSpark * NUM_SPARK_SEGMENTS + iSegment].m_TextureYOfs = segmentfraction;
				pBeamStrips[iSpark * NUM_SPARK_SEGMENTS + iSegment].m_Color = 0x00FFFFFF | ((int32)((1.0f - offset) * 255.0f) << 24);
			}
		}

		CMat43fp32 unit;
		unit.Unit();

		CXR_Util::Render_BeamStrip2(m_pVBM, m_pVBSparks, unit, _VMat, pBeamStrips, MAXNUMBEAMSTRIPS);

		return true;
	}
	
	//----------------------------------------------------------------------
	
	bool
	Init(const CXR_AnimState* _pAnimState, const CMat43fp32& _LocalToWorld, const CMat43fp32& _WorldToCamera, const CMat43fp32& _LocalToCamera)
	{
		MAUTOSTRIP(CXR_Model_FuseSparks_Init, false);
		if (!_pAnimState) return false;

		m_Time = _pAnimState->m_AnimTime0;
		m_Duration = _pAnimState->m_AnimTime1;

		m_RandseedBase = _pAnimState->m_Anim0;
		m_Randseed = m_RandseedBase;
		m_BoundRadius = 100.0f;

		m_TimeFraction = m_Time / m_Duration;

		if (m_TimeFraction < 0.0f)
			return false;

		if (m_TimeFraction > 1.0f)
			return false;

		if (m_Time > 0.0f)
		{
			m_pVBSparks = AllocVB();
			if (m_pVBSparks == NULL)
				return false;

			m_SKSparks.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface);
		}

		m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));

		m_SparksPos = _LocalToWorld;

		return true;
	}

	//----------------------------------------------------------------------

	void OnPreRender(const CXR_AnimState *_pAnimState, const CMat43fp32 &_WMat)
	{
		MAUTOSTRIP(CXR_Model_FuseSparks_OnPreRender, MAUTOSTRIP_VOID);
		if (_pAnimState->m_AnimTime0 > 0.0f)
		{
			AddDistortLight(_pAnimState, CVec3Dfp32::GetMatrixRow(_WMat, 3),
							CVec3Dfp32(0.6f, 0.6f, 0.3f), 
							CVec3Dfp32(0.1f, 0.1f, 0.1f), SPARK_RADIUS * 3.0f, SPARK_RADIUS * 1.5f);
		}
	}

	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_FuseSparks_Render, MAUTOSTRIP_VOID);
		CMat43fp32 LocalToCamera;
		_WMat.Multiply(_VMat, LocalToCamera);

		if (!Init(_pAnimState, _WMat, _VMat, LocalToCamera))
			return;

		if (GenerateSparks(_VMat))
		{
			m_SKSparks.Render(m_pVBSparks, m_pVBM, m_pEngine);
		}

	}

	//----------------------------------------------------------------------
	
	virtual
	void
	OnCreate(const char *)
	{
		MAUTOSTRIP(CXR_Model_FuseSparks_OnCreate, MAUTOSTRIP_VOID);
		m_iSurface = GetSurfaceID("FuseSparks");
	}

};

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_FuseSparks, CXR_Model_Custom);

//----------------------------------------------------------------------
