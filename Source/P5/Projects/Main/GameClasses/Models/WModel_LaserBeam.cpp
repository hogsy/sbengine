#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

//----------------------------------------------------------------------

//----------------------------------------------------------------------
// CXR_Model_LaserBeam
//----------------------------------------------------------------------

class CXR_Model_LaserBeam : public CXR_Model_Custom
{

	MRTC_DECLARE;

protected:

	int32 m_iSurface;

public:

private:

	//----------------------------------------------------------------------
	
	CPixel32		m_LightColor;

	int32			m_iVertex;
	CVec3Dfp32*		m_pPos;
	CVec2Dfp32*		m_pTex;
	CPixel32*		m_pColor;

	int32			m_iTriIndex;
	uint16*			m_pTriIndex;

	//----------------------------------------------------------------------

	inline void AddVertex(CVec3Dfp32 _Pos, fp32 _u, fp32 _v)
	{
		m_pPos[m_iVertex] = _Pos;
		m_pTex[m_iVertex][0] = _u;
		m_pTex[m_iVertex][1] = _v;
		m_pColor[m_iVertex] = m_LightColor;
		m_iVertex++;
	}

	inline void AddTriangle(int32 _v1, int32 _v2, int32 _v3)
	{
		m_pTriIndex[m_iTriIndex + 0] = m_iVertex + _v1;
		m_pTriIndex[m_iTriIndex + 1] = m_iVertex + _v2;
		m_pTriIndex[m_iTriIndex + 2] = m_iVertex + _v3;
		m_iTriIndex += 3;
	}

	bool FillVB(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* pAnimState, 
		   const CMat4Dfp32& LocalToWorld, const CMat4Dfp32& WorldToCamera,
		   CXR_VertexBuffer* pVB);

	virtual void OnCreate(const char *surface);
	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat);
};


bool CXR_Model_LaserBeam::FillVB(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, 
	   const CMat4Dfp32& _LocalToWorld, const CMat4Dfp32& _WorldToCamera,
	   CXR_VertexBuffer* _pVB)
{
	fp32 Length = _pAnimState->m_AnimAttr0;

	fp32 Width, Height;
	Width = 2.0f;
	Height = 2.0f;

	m_BoundRadius = 0.5f * M_Sqrt(Width*Width + Length*Length + Height*Height);

	m_pPos = _pRenderParams->m_pVBM->Alloc_V3(8);
	if (!m_pPos)
		return false;

	m_pTex = _pRenderParams->m_pVBM->Alloc_V2(8);
	if (!m_pTex)
		return false;

	m_pColor = _pRenderParams->m_pVBM->Alloc_CPixel32(8);
	if (!m_pColor)
		return false;

	m_pTriIndex = _pRenderParams->m_pVBM->Alloc_Int16(12);
	if (!m_pTriIndex)
		return false;

	CMat4Dfp32 *pMatrix = _pRenderParams->m_pVBM->Alloc_M4();
	if(!pMatrix)
		return false;

	CMat4Dfp32 LocalToCamera;
	_LocalToWorld.Multiply(_WorldToCamera, LocalToCamera);

	CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(_LocalToWorld, 3);

/*
	// Normal is NULL, since the lit point is considered to be omni.
	if (m_pWLS != NULL)
	{
		m_WLS.CopyAndCull(m_pWLS, GetBound_Sphere(), Pos, 3, 2);
		m_WLS.AddLightVolume(m_RenderInfo.m_pLightVolume, Pos);
		m_WLS.InitLinks();

		CXR_WorldLightState::LightDiffuse(m_WLS.GetFirst(), 1, &Pos, NULL, true, &m_LightColor, 255, 1.7f);
	}
	else
*/
	m_LightColor = _pAnimState->m_Data[3];

	m_iVertex = 0;
	m_iTriIndex = 0;

	AddTriangle(0, 4, 2);
	AddTriangle(4, 6, 2);

	AddTriangle(3, 7, 1);
	AddTriangle(7, 5, 1);

	fp32 w, h, f;
	w = Width * 0.5f;
	h = Height * 0.5f;
	f = Length;

	AddVertex(CVec3Dfp32(0,  0,  h), 0, 0);
	AddVertex(CVec3Dfp32(0,  w,  0), 1, 0);
	AddVertex(CVec3Dfp32(0,  0, -h), 1, 0);
	AddVertex(CVec3Dfp32(0, -w,  0), 0, 0);
	AddVertex(CVec3Dfp32(f,  0,  h), 0, 1);
	AddVertex(CVec3Dfp32(f,  w,  0), 1, 1);
	AddVertex(CVec3Dfp32(f,  0, -h), 1, 1);
	AddVertex(CVec3Dfp32(f, -w,  0), 0, 1);

/*
	AddVertex(CVec3Dfp32(0,  0,  h), 0, 0, 255);
	AddVertex(CVec3Dfp32(0,  w,  0), 0, 1, 255);
	AddVertex(CVec3Dfp32(0,  0, -h), 1, 0, 255);
	AddVertex(CVec3Dfp32(0, -w,  0), 0, 0, 255);
	AddVertex(CVec3Dfp32(f,  0,  h), 0, 1, 255);
	AddVertex(CVec3Dfp32(f,  w,  0), 1, 1, 255);
	AddVertex(CVec3Dfp32(f,  0, -h), 1, 1, 255);
	AddVertex(CVec3Dfp32(f, -w,  0), 1, 0, 255);
*/

/*
	AddVertex(CVec3Dfp32(0,  0,  h), 0, 0, 255);
	AddVertex(CVec3Dfp32(0,  w,  0), 1, 0, 255);
	AddVertex(CVec3Dfp32(0,  0, -h), 1, 0, 255);
	AddVertex(CVec3Dfp32(0, -w,  0), 0, 0, 255);
	AddVertex(CVec3Dfp32(f,  0,  h), 0, 1, 255);
	AddVertex(CVec3Dfp32(f,  w,  0), 0, 1, 255);
	AddVertex(CVec3Dfp32(f,  0, -h), 1, 1, 255);
	AddVertex(CVec3Dfp32(f, -w,  0), 1, 1, 255);
*/

/*
	AddVertex(CVec3Dfp32(0,  0,  h), 0, 0, 255);
	AddVertex(CVec3Dfp32(0,  w,  0), 0, 1, 255);
	AddVertex(CVec3Dfp32(0,  0, -h), 0, 1, 255);
	AddVertex(CVec3Dfp32(0, -w,  0), 0, 0, 255);
	AddVertex(CVec3Dfp32(f,  0,  h), 1, 0, 255);
	AddVertex(CVec3Dfp32(f,  w,  0), 1, 0, 255);
	AddVertex(CVec3Dfp32(f,  0, -h), 1, 1, 255);
	AddVertex(CVec3Dfp32(f, -w,  0), 1, 1, 255);
*/

	*pMatrix = LocalToCamera;

	_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
	_pVB->Matrix_Set(pMatrix);
	if (!_pVB->AllocVBChain(_pRenderParams->m_pVBM, false))
		return false;
	_pVB->Geometry_VertexArray(m_pPos, 8, true);
	_pVB->Geometry_TVertexArray(m_pTex, 0);		// Any particular reason why this is 0
	_pVB->Geometry_ColorArray(m_pColor);
	_pVB->Render_IndexedTriangles(m_pTriIndex, 4);

	return true;
}

//----------------------------------------------------------------------

void CXR_Model_LaserBeam::OnCreate(const char *_pszSurface)
{
	if (_pszSurface != NULL) 
		m_iSurface = GetSurfaceID(_pszSurface);
	else
		m_iSurface = GetSurfaceID("HelmetLaser");
}

//----------------------------------------------------------------------

void CXR_Model_LaserBeam::Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat)
{
	MSCOPESHORT(CXR_Model_LaserBeam::Render);
	CXR_VertexBuffer* pVB = AllocVB(_pRenderParams);
	if (pVB == NULL)
		return;

	if (FillVB(_pRenderParams, _pAnimState, _WMat, _VMat, pVB))
		Render_Surface(_pRenderParams, m_iSurface, pVB, _pAnimState->m_AnimTime0);
}

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_LaserBeam, CXR_Model_Custom);

//----------------------------------------------------------------------
