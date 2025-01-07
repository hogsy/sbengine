#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

//----------------------------------------------------------------------

//----------------------------------------------------------------------
// CXR_Model_Butterfly
//----------------------------------------------------------------------

class CXR_Model_Butterfly : public CXR_Model_Custom
{

	MRTC_DECLARE;

protected:

	int32 m_iSurface;

private:

	//----------------------------------------------------------------------
	
	CPixel32		m_LightColor;

	int32			iVertex, iIndex;
	CVec3Dfp32*		pVertexPos;
	CVec2Dfp32*		pVertexTex;
	CPixel32*		pVertexColor;
	uint16*			pIndex;

	//----------------------------------------------------------------------

	M_INLINE void addVertex(CVec3Dfp32 pos, fp32 tu, fp32 tv, int32 alpha)
	{
		MAUTOSTRIP(CXR_Model_Butterfly_addVertex, MAUTOSTRIP_VOID);
		pVertexPos[iVertex] = pos;
		pVertexTex[iVertex][0] = tu;
		pVertexTex[iVertex][1] = tv;
		pVertexColor[iVertex] = (m_LightColor & 0x00FFFFFF) + (alpha << 24);
		iVertex++;
	}

	//----------------------------------------------------------------------

	M_INLINE void addTriangle(int32 v1, int32 v2, int32 v3)
	{
		MAUTOSTRIP(CXR_Model_Butterfly_addTriangle, MAUTOSTRIP_VOID);
		pIndex[iIndex + 0] = iVertex + v1;
		pIndex[iIndex + 1] = iVertex + v2;
		pIndex[iIndex + 2] = iVertex + v3;
		iIndex += 3;
	}

	//----------------------------------------------------------------------

	bool fillVB(const CXR_AnimState* pAnimState, const CMat43fp32& LocalToWorld, const CMat43fp32& WorldToCamera, CXR_VertexBuffer* pVB);
	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat);
	virtual void OnCreate(const char *surface);
};

bool CXR_Model_Butterfly::fillVB(const CXR_AnimState* pAnimState, const CMat43fp32& LocalToWorld, const CMat43fp32& WorldToCamera, CXR_VertexBuffer* pVB)
{
	MAUTOSTRIP(CXR_Model_Butterfly_fillVB, false);
	const fp32 time = pAnimState->m_AnimTime0;
	const fp32 size = pAnimState->m_Anim0;

//	ConOut(CStrF("animtime = %f", time));

	fp32 width, length, height;
	width = size;
	length = 0.5f * size;
	height = 0.5f * size;
	m_BoundRadius = 0.5f * M_Sqrt(Sqr(width) + Sqr(length) + Sqr(height));

	pVertexPos = m_pVBM->Alloc_V3(6);
	if (!pVertexPos)
		return false;

	pVertexTex = m_pVBM->Alloc_V2(6);
	if (!pVertexTex)
		return false;

	pVertexColor = m_pVBM->Alloc_CPixel32(6);
	if (!pVertexColor)
		return false;

	pIndex = m_pVBM->Alloc_Int16(12);
	if (!pIndex)
		return false;

	CMat43fp32 LocalToCamera;
	LocalToWorld.Multiply(WorldToCamera, LocalToCamera);

	CMat4Dfp32 *pMatrix = m_pVBM->Alloc_M4(LocalToCamera);
	if(!pMatrix)
		return false;

	iVertex = 0;
	iIndex = 0;

	CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(LocalToWorld, 3);

	// Normal is NULL, since the lit point is considered to be omni.
	if (m_pWLS != NULL)
	{
		m_WLS.CopyAndCull(m_pWLS, GetBound_Sphere(), Pos, 3, 2);
		m_WLS.AddLightVolume(m_RenderInfo.m_pLightVolume, Pos);
		m_WLS.InitLinks();

		CXR_WorldLightState::LightDiffuse(m_WLS.GetFirst(), 1, &Pos, NULL, true, &m_LightColor, 255, 1.7f);
	}
	else
		m_LightColor = 0xFFFFFFFF;

	fp32 w, l, h0, h1;

	w = M_Cos(time * 0.48f * _PI) * 0.5f * width;
	l = 0.5f * length;
	h0 = 0.0f * height;
	h1 = M_Sin(time * 0.48f * _PI) * height;

	addTriangle(0, 1, 2);
	addTriangle(2, 3, 0);
	addTriangle(0, 3, 4);
	addTriangle(4, 5, 0);
	addVertex(CVec3Dfp32(l, 0, h0), 0.0f, 0.0f, 255); // 0
	addVertex(CVec3Dfp32(l, w, h1), 1.0f, 0.0f, 255); // 1
	addVertex(CVec3Dfp32(-l, w, h1), 1.0f, 1.0f, 255); // 2
	addVertex(CVec3Dfp32(-l, 0, h0), 0.0f, 1.0f, 255); // 3
	addVertex(CVec3Dfp32(-l, -w, h1), 1.0f, 1.0f, 255); // 4
	addVertex(CVec3Dfp32(l, -w, h1), 1.0f, 0.0f, 255); // 5

	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
	pVB->Matrix_Set(pMatrix);
	if (!pVB->SetVBChain(m_pVBM, false))
		return false;
	pVB->Geometry_VertexArray(pVertexPos, 6, true);
	pVB->Geometry_TVertexArray(pVertexTex, 0);
	pVB->Geometry_ColorArray(pVertexColor);
	pVB->Render_IndexedTriangles(pIndex, 4);

	return true;
}

//----------------------------------------------------------------------
	
void CXR_Model_Butterfly::OnCreate(const char *surface)
{
	MAUTOSTRIP(CXR_Model_Butterfly_OnCreate, MAUTOSTRIP_VOID);
	if (surface != NULL) 
		m_iSurface = GetSurfaceID(surface);
	else
		m_iSurface = GetSurfaceID("Butterfly05");
}

//----------------------------------------------------------------------

void CXR_Model_Butterfly::Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
{
	MAUTOSTRIP(CXR_Model_Butterfly_Render, MAUTOSTRIP_VOID);
	CXR_VertexBuffer* pVB = AllocVB();
	if (pVB == NULL)
		return;

	if (fillVB(_pAnimState, _WMat, _VMat, pVB))
		Render_Surface(m_iSurface, pVB, _pAnimState->m_AnimTime0);
}

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Butterfly, CXR_Model_Custom);

//----------------------------------------------------------------------
