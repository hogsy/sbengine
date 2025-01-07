#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PosHistory.h"

#include "CEnvelope.h"
#include "CDynamicVB.h"
#include "CSurfaceKey.h"

//----------------------------------------------------------------------

#define NUM_WIRE_SUBDIVISIONS	(6)
#define NUM_WIRE_SIDES			(3)
#define WIRE_RADIUS				(0.7f)

#define NUM_SPARKS				(20)
#define NUM_SPARK_SEGMENTS		(5)
#define SPARK_DURATION			(0.1f)
#define SPARK_RADIUS			(30.0f)
#define SPARK_LENGTH			(10.0f)
#define SPARK_WIDTH				(1.0f)

#define MAXNUMBEAMSTRIPS		(NUM_SPARKS * NUM_SPARK_SEGMENTS)

//----------------------------------------------------------------------
// CXR_Model_Fuse
//----------------------------------------------------------------------

class CXR_Model_Fuse : public CXR_Model_Custom
{

	MRTC_DECLARE;

	//----------------------------------------------------------------------
	
private:

	int m_iSurface_Wire;
	int m_iSurface_Sparks;

public:

	//----------------------------------------------------------------------
	
	fp32
	GetRand()
	{
		MAUTOSTRIP(CXR_Model_Fuse_GetRand, 0);
		return MFloat_GetRand(m_Randseed++);
	}

	//----------------------------------------------------------------------
	
	void
	AnglesToVector(fp32 a, fp32 b, CVec3Dfp32& v)
	{
		MAUTOSTRIP(CXR_Model_Fuse_AnglesToVector, MAUTOSTRIP_VOID);
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

	CWO_PosHistory *m_pPath;

	fp32					m_SegmentsAdded;
	CMat43fp32			m_SparksPos;

	int32				m_NumSegments;

	CDynamicVB			m_DVBWire;
	CXR_VertexBuffer*	m_pVBSparks;
	CSurfaceKey			m_SKWire, m_SKSparks;

	CWireContainer*		m_pWC;

private:

	//----------------------------------------------------------------------

	int GetParam(int _Param)
	{
		MAUTOSTRIP(CXR_Model_Fuse_GetParam, 0);
		if (_Param == CXR_MODEL_PARAM_NEEDPATHDATA)
			return 1;

		if (_Param == CXR_MODEL_PARAM_TIMEMODE)
			return CXR_MODEL_TIMEMODE_CONTROLLED;

		return CXR_Model_Custom::GetParam(_Param);
	}
	
	//----------------------------------------------------------------------

	void AddWireSegment(CMat43fp32 _matrix)
	{
		MAUTOSTRIP(CXR_Model_Fuse_AddWireSegment, MAUTOSTRIP_VOID);
		int iSide;

		if (m_SegmentsAdded > 0)
		{
			int vo;
			if (m_SegmentsAdded == 1)
				vo = 0;
			else
				vo = -(NUM_WIRE_SIDES + 1);

			for (iSide = 0; iSide < NUM_WIRE_SIDES; iSide++)
			{
				m_DVBWire.AddTriangle(vo + iSide, vo + iSide + 1, vo + iSide + NUM_WIRE_SIDES + 2);
				m_DVBWire.AddTriangle(vo + iSide, vo + iSide + NUM_WIRE_SIDES + 1, vo + iSide + NUM_WIRE_SIDES + 2);
			}

			CVec3Dfp32 dir;
			CVec3Dfp32 left(0, 1, 0);
			CVec3Dfp32 up(0, 0, 1);
			dir = CVec3Dfp32::GetMatrixRow(_matrix, 3) - CVec3Dfp32::GetMatrixRow(m_SparksPos, 3);
			up.CrossProd(dir, left);
			left.CrossProd(dir, up);
			up.Normalize();
			left.Normalize();

			if (m_SegmentsAdded == 1)
			{
				CVec3Dfp32 pos = CVec3Dfp32::GetMatrixRow(m_SparksPos, 3);

				for (iSide = 0; iSide <= NUM_WIRE_SIDES; iSide++)
				{
					fp32 t = (fp32)iSide / (fp32)NUM_WIRE_SIDES;
					fp32 u = WIRE_RADIUS * M_Cos(t * 2.0f * _PI);
					fp32 v = WIRE_RADIUS * M_Sin(t * 2.0f * _PI);
					m_DVBWire.AddVertex(pos + left * u + up * v, t, 0.0f, 0xFFFFFFFF);
				}

			}

			for (iSide = 0; iSide <= NUM_WIRE_SIDES; iSide++)
			{
				CVec3Dfp32 pos = CVec3Dfp32::GetMatrixRow(_matrix, 3);

				fp32 t = (fp32)iSide / (fp32)NUM_WIRE_SIDES;
				fp32 u = WIRE_RADIUS * M_Cos(t * 2.0f * _PI);
				fp32 v = WIRE_RADIUS * M_Sin(t * 2.0f * _PI);
				m_DVBWire.AddVertex(pos + left * u + up * v, t, (fp32)m_SegmentsAdded, 0xFFFFFFFF);
			}
		}

		m_SegmentsAdded++;
		m_SparksPos = _matrix;
	}

	//----------------------------------------------------------------------

	bool GenerateWire()
	{
		MAUTOSTRIP(CXR_Model_Fuse_GenerateWire, false);
		m_SegmentsAdded = 0;

		CMat43fp32 pos;
		fp32 SegmentDuration = (m_Duration / (fp32)m_NumSegments);
		for (fp32 t = m_Duration; t > (m_Time + SegmentDuration * 0.5f); t -= SegmentDuration)
		{
			if (m_pPath->GetCacheMatrix(0, t, pos))
				AddWireSegment(pos);
		}

		if (m_SegmentsAdded > 0)
		{
			if (m_pPath->GetCacheMatrix(0, m_Time, pos))
				AddWireSegment(pos);
		}

		return (m_SegmentsAdded > 1);
	}

	//----------------------------------------------------------------------

	bool GenerateSparks(const CMat43fp32& _VMat)
	{
		MAUTOSTRIP(CXR_Model_Fuse_GenerateSparks, false);
		if (m_Time == 0.0f)
			return false;

		CXR_BeamStrip pBeamStrips[MAXNUMBEAMSTRIPS];

		CVec3Dfp32 pos = CVec3Dfp32::GetMatrixRow(m_SparksPos, 3);
//		pos = CVec3Dfp32(0, 0, 10);

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

	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
	{
		MAUTOSTRIP(CXR_Model_Fuse_GetBound_Box, MAUTOSTRIP_VOID);
		m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));

		_Box.m_Min = 0;
		_Box.m_Max = 0;
		
		m_pPath = GetData(_pAnimState);
		if (m_pPath == NULL)
			return;

		if (!m_pPath->IsValid())
			return;

		m_Duration = m_pPath->m_lSequences[0].GetDuration();

		if (_pAnimState != NULL)
			m_Time = _pAnimState->m_AnimTime0;
		else
			m_Time = 0;

		CMat43fp32 WorldToLocal;

		CMat43fp32 Matrix;
		if (!m_pPath->GetCacheMatrix(0, m_Time, Matrix))
			return;

		Matrix.InverseOrthogonal(WorldToLocal);

		if (m_pPath->GetCacheMatrix(0, 0, Matrix))
		{
			CVec3Dfp32 Pos;
			
			Pos = CVec3Dfp32::GetMatrixRow(Matrix, 3);

			if (m_pWC != NULL)
				m_pWC->RenderVertex(Pos, 0xFFFFFF00, 0.1f);

			Pos.MultiplyMatrix(WorldToLocal);
			_Box.m_Min = Pos;
			_Box.m_Max = Pos;
			
			if (m_pPath->GetCacheMatrix(0, m_Duration, Matrix))
			{
				CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(Matrix, 3);

				if (m_pWC != NULL)
					m_pWC->RenderVertex(Pos, 0xFFFFFF00, 0.1f);

				Pos.MultiplyMatrix(WorldToLocal);

				_Box.Expand(Pos);
			}
				
			m_NumSegments = m_pPath->m_lSequences[0].GetNumKeyframes() * NUM_WIRE_SUBDIVISIONS / 3;
			for (fp32 t = 0; t <= m_Duration; t += (m_Duration / (fp32)m_NumSegments))
			{
				if (m_pPath->GetCacheMatrix(0, t, Matrix))
				{
					CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(Matrix, 3);

					if (m_pWC != NULL)
						m_pWC->RenderVertex(Pos, 0xFFFFFF00, 0.1f);

					Pos.MultiplyMatrix(WorldToLocal);

					_Box.Expand(Pos);
				}
			}
		}
	}

	//----------------------------------------------------------------------

	CWO_PosHistory* GetData(const CXR_AnimState* _pAnimState)
	{
		MAUTOSTRIP(CXR_Model_Fuse_GetData, NULL);
		if (_pAnimState == NULL)
			return NULL;

		CReferenceCount* pData = (CReferenceCount*)*_pAnimState->m_pspClientData;
		CWO_PosHistory* pPH = safe_cast<CWO_PosHistory>(pData);
		return pPH;
	}

	//----------------------------------------------------------------------
	
	bool
	Init(const CXR_AnimState* _pAnimState, const CMat43fp32& _LocalToWorld, const CMat43fp32& _WorldToCamera, const CMat43fp32& _LocalToCamera)
	{
		MAUTOSTRIP(CXR_Model_Fuse_Init, false);
		if (!_pAnimState) return false;

		m_Time = _pAnimState->m_AnimTime0;
		m_Duration = _pAnimState->m_AnimTime1;

		m_RandseedBase = _pAnimState->m_Anim0;
		m_Randseed = m_RandseedBase;
	
		if (_pAnimState->m_pspClientData == NULL)
			return false;

		m_pPath = GetData(_pAnimState);
		if (m_pPath == NULL)
		{
			return false;
		}
		else
		{
			if (m_pPath->IsValid())
				m_Duration = m_pPath->m_lSequences[0].GetDuration();
			else
				return false;

			m_NumSegments = m_pPath->m_lSequences[0].GetNumKeyframes() * NUM_WIRE_SUBDIVISIONS;
			m_pPath->GenerateCache(m_NumSegments);
		}

		m_TimeFraction = m_Time / m_Duration;

		if (m_TimeFraction < 0.0f)
			return false;

		if (m_TimeFraction > 1.0f)
			return false;

		int numVertices, numTriangles;

		numVertices = (m_NumSegments + 1) * (NUM_WIRE_SIDES + 1);
		numTriangles = m_NumSegments * NUM_WIRE_SIDES * 2;
		
		if (!m_DVBWire.Create(this, m_pVBM, numVertices, numTriangles))
			return false;

		m_SKWire.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface_Wire);

		if (m_Time > 0.0f)
		{
			m_pVBSparks = AllocVB();
			if (m_pVBSparks == NULL)
				return false;

			m_SKSparks.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface_Sparks);
		}

		m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));

		return true;
	}

	//----------------------------------------------------------------------

	void OnPreRender(const CXR_AnimState *_pAnimState, const CMat43fp32 &_WMat)
	{
		MAUTOSTRIP(CXR_Model_Fuse_OnPreRender, MAUTOSTRIP_VOID);
		if (_pAnimState->m_AnimTime0 > 0.0f)
		{
			m_pPath = (CWO_PosHistory*)(CReferenceCount*)*_pAnimState->m_pspClientData;
			if (m_pPath == NULL)
				return;

			CMat43fp32 pos;

			if (m_pPath->GetCacheMatrix(0, _pAnimState->m_AnimTime0, pos))
				AddDistortLight(_pAnimState, CVec3Dfp32::GetMatrixRow(pos, 3),
								CVec3Dfp32(0.6f, 0.6f, 0.6f), CVec3Dfp32(0.2f, 0.2f, 0.2f), SPARK_RADIUS * 1.5f, SPARK_RADIUS * 0.3f);
		}
	}

	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_Fuse_Render, MAUTOSTRIP_VOID);
		CMat43fp32 LocalToCamera;
		_WMat.Multiply(_VMat, LocalToCamera);

		if (!Init(_pAnimState, _WMat, _VMat, LocalToCamera))
			return;

		if (GenerateWire())
		{
//			ConOutL(CStrF("Time = %f, #V,#T = (%d,%d), Min/Max = %d/%d", m_Time, m_DVBWire.GetNumVertices(), m_DVBWire.GetNumTriangles(), m_DVBWire.m_MinIndex, m_DVBWire.m_MaxIndex));

			if (m_DVBWire.IsValid())
			{
				m_DVBWire.Render(_VMat);
				m_SKWire.Render(m_DVBWire.GetVB(), m_pVBM, m_pEngine);
			}
		}

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
		MAUTOSTRIP(CXR_Model_Fuse_OnCreate, MAUTOSTRIP_VOID);
		m_iSurface_Wire = GetSurfaceID("FuseWire");
		m_iSurface_Sparks = GetSurfaceID("FuseSparks");
	}

};

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Fuse, CXR_Model_Custom);

//----------------------------------------------------------------------
