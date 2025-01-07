#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PosHistory.h"

#include "CEnvelope.h"
#include "CDynamicVB.h"
#include "CSurfaceKey.h"

//----------------------------------------------------------------------
// CXR_Model_Fuse
//----------------------------------------------------------------------
#ifndef M_DISABLE_TODELETE

class CXR_Model_WirePath : public CXR_Model_Custom
{

	MRTC_DECLARE;

	//----------------------------------------------------------------------
	
public:

	fp32					m_Time;
	fp32					m_Duration;
	fp32					m_TimeFraction;

	int					m_iSurface;
	fp32					m_Radius;
	fp32					m_TexScaleX, m_TexScaleY;
	int					m_Sides, m_SubDivs;

private:

	int32				m_RandseedBase, m_Randseed;

	CWO_PosHistory*	m_pPath;

	fp32					m_SegmentsAdded;
	int32				m_NumSegments;

	CMat4Dfp32			m_EndPos;

	CDynamicVB			m_DVBWire;
	CSurfaceKey			m_SKWire;

	CWireContainer*		m_pWC;

private:

	//----------------------------------------------------------------------

	int GetParam(int _Param)
	{
		if (_Param == CXR_MODEL_PARAM_NEEDPATHDATA)
			return 1;
		return 0;
	}

	//----------------------------------------------------------------------

	void AddWireSegment(CMat4Dfp32 _matrix);

	//----------------------------------------------------------------------

	bool GenerateWire()
	{
		MAUTOSTRIP(CDynamicVB_GenerateWire, false);
		m_SegmentsAdded = 0;

		CMat4Dfp32 pos;
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
	
	bool
	Init(const CXR_AnimState* _pAnimState, const CMat4Dfp32& _LocalToWorld, const CMat4Dfp32& _WorldToCamera, const CMat4Dfp32& _LocalToCamera)
	{
		MAUTOSTRIP(CDynamicVB_Init, false);
		if (!_pAnimState) return false;

/*
		m_iSurface = GetSurfaceID("ShockWave1");
		m_Radius = 10.0f;
		m_SubDivs = 6;
		m_Sides = 6;
		m_TexScaleX = 5.0f;
		m_TexScaleY = 1.0f;
*/
		Error("Init", "Expired.");

		m_Time = _pAnimState->m_AnimTime0.GetTime(); // CMTIMEFIX
		m_Duration = _pAnimState->m_AnimTime1.GetTime(); // CMTIMEFIX

		m_RandseedBase = _pAnimState->m_Anim0;
		m_Randseed = m_RandseedBase;
		m_BoundRadius = 100.0f;

/*		if (_pAnimState->m_pspClientData == NULL)
			return false;

		m_pPath = (CWO_PosHistory*)(CReferenceCount*)*_pAnimState->m_pspClientData;
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

			m_NumSegments = m_pPath->m_lSequences[0].GetNumKeyframes() * m_SubDivs;
			m_pPath->GenerateCache(m_NumSegments);
		}

		m_TimeFraction = m_Time / m_Duration;

		if (m_TimeFraction < 0.0f)
			return false;

		if (m_TimeFraction > 1.0f)
			return false;

		int numVertices, numTriangles;

		numVertices = (m_NumSegments + 1) * (m_Sides + 1);
		numTriangles = m_NumSegments * m_Sides * 2;
		
		if (!m_DVBWire.Create(this, m_pVBM, numVertices, numTriangles))
			return false;

		m_SKWire.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface);

		m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));*/

		return true;
	}

	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat)
	{
		MAUTOSTRIP(CDynamicVB_Render, MAUTOSTRIP_VOID);
		CMat4Dfp32 LocalToCamera;
		_WMat.Multiply(_VMat, LocalToCamera);

		if (!Init(_pAnimState, _WMat, _VMat, LocalToCamera))
			return;

		if (GenerateWire())
		{
			if (m_DVBWire.IsValid())
			{
				m_DVBWire.Render(_VMat);
				m_SKWire.Render(m_DVBWire.GetVB(), m_pVBM, m_pEngine);
			}
		}
	}

	//----------------------------------------------------------------------

	virtual void OnCreate(const char *_params)
	{
		MAUTOSTRIP(CDynamicVB_OnCreate, MAUTOSTRIP_VOID);
		if (_params != NULL)
		{
			CStr params(_params);
			m_iSurface = GetSurfaceID(params.GetStrSep(","));
			m_Radius = params.Getfp64Sep(",");
			m_SubDivs = params.GetIntSep(",");
			m_Sides = params.GetIntSep(",");
			m_TexScaleX = params.GetIntSep(",");
			m_TexScaleY = params.GetIntSep(",");
		}
		else
		{
			m_iSurface = GetSurfaceID("FuseWire");
			m_Radius = 0;
			m_SubDivs = 0;
			m_Sides = 0;
			m_TexScaleX = 0;
			m_TexScaleY = 0;
		}

		if (m_Radius == 0.0f) m_Radius = 1.0f;
		if (m_Sides == 0) m_Sides = 3;
		if (m_SubDivs == 0) m_SubDivs = 4;
		if (m_TexScaleX == 0.0f) m_TexScaleX = 1.0f;
		if (m_TexScaleY == 0.0f) m_TexScaleY = 1.0f;

		// Invert from number of repeats to scaling factor.
		m_TexScaleX = 1.0f / m_TexScaleX;
		m_TexScaleY = 1.0f / m_TexScaleY;
	}

	//----------------------------------------------------------------------

};

//----------------------------------------------------------------------
void CXR_Model_WirePath::AddWireSegment(CMat4Dfp32 _matrix)
{
	MAUTOSTRIP(CDynamicVB_AddWireSegment, MAUTOSTRIP_VOID);
	int iSide;

	if (m_SegmentsAdded > 0)
	{
		int vo;
		if (m_SegmentsAdded == 1)
			vo = 0;
		else
			vo = -(m_Sides + 1);

		for (iSide = 0; iSide < m_Sides; iSide++)
		{
			m_DVBWire.AddTriangle(vo + iSide, vo + iSide + 1, vo + iSide + m_Sides + 2);
//			m_DVBWire.AddTriangle(vo + iSide, vo + iSide + m_Sides + 1, vo + iSide + m_Sides + 2);
			m_DVBWire.AddTriangle(vo + iSide, vo + iSide + m_Sides + 2, vo + iSide + m_Sides + 1);
		}

		CVec3Dfp32 dir;
		CVec3Dfp32 left(0, 1, 0);
		CVec3Dfp32 up(0, 0, 1);
		dir = CVec3Dfp32::GetMatrixRow(_matrix, 3) - CVec3Dfp32::GetMatrixRow(m_EndPos, 3);
		up.CrossProd(dir, left);
		left.CrossProd(dir, up);
		up.Normalize();
		left.Normalize();

		if (m_SegmentsAdded == 1)
		{
			CVec3Dfp32 pos = CVec3Dfp32::GetMatrixRow(m_EndPos, 3);

			for (iSide = 0; iSide <= m_Sides; iSide++)
			{
				fp32 t = (fp32)iSide / (fp32)m_Sides;
				fp32 u = m_Radius * M_Cos(t * 2.0f * _PI);
				fp32 v = m_Radius * M_Sin(t * 2.0f * _PI);
				m_DVBWire.AddVertex(pos + left * u + up * v, t * m_TexScaleX, 0.0f, 0xFFFFFFFF);
			}

		}

		for (iSide = 0; iSide <= m_Sides; iSide++)
		{
			CVec3Dfp32 pos = CVec3Dfp32::GetMatrixRow(_matrix, 3);

			fp32 t = (fp32)iSide / (fp32)m_Sides;
			fp32 u = m_Radius * M_Cos(t * 2.0f * _PI);
			fp32 v = m_Radius * M_Sin(t * 2.0f * _PI);
			fp32 SegmentFraction = ((fp32)m_SegmentsAdded / (fp32)(m_NumSegments - 1));
			m_DVBWire.AddVertex(pos + left * u + up * v, t * m_TexScaleX, m_TexScaleY * SegmentFraction, 0xFFFFFFFF);
		}
	}

	m_SegmentsAdded++;
	m_EndPos = _matrix;
}
//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_WirePath, CXR_Model_Custom);

//----------------------------------------------------------------------
#endif
