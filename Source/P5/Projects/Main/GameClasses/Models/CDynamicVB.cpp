#include "PCH.h"

#include "CDynamicVB.h"

void CDynamicVB::Clear()
{
	m_MaxNumVertices = 0;
	m_MaxNumTriangles = 0;
	m_NumVertices = 0;
	m_NumTriangles = 0;
	m_pVertexPos = NULL;
	m_pVertexTex = NULL;
	m_pVertexNormal = NULL;
	m_pVertexColor = NULL;
	m_pIndex = NULL;
	m_pVB = NULL;
	m_Flags = DVBFLAGS_POSONLY;

	m_pFlippedIndex = NULL;
	m_pFlippedVB = NULL;

}

bool CDynamicVB::Create(CXR_Model_Custom_RenderParams* _pRenderParams, CXR_Model_Custom* _pCustomModel, CXR_VBManager* _pVBM, int32 _NumVertices, int32 _NumTriangles, int _Flags)
{
	Clear();

	m_Flags = _Flags;

	if (m_Flags & DVBFLAGS_FLATSHADED)
		_NumVertices = _NumTriangles * 3;

	m_MaxNumVertices = _NumVertices;
	m_MaxNumTriangles = _NumTriangles;

	m_NumVertices = 0;
	m_NumTriangles = 0;

	m_pVB = _pCustomModel->AllocVB(_pRenderParams);
	if (m_pVB == NULL)
		return false;

	if (!m_pVB->AllocVBChain(_pVBM, false))
		return false;

	m_pVertexPos = _pVBM->Alloc_V3(m_MaxNumVertices);
	if (m_pVertexPos == NULL)
		return false;

	if (m_Flags & DVBFLAGS_TEXTURE)
	{
		m_pVertexTex = _pVBM->Alloc_V2(m_MaxNumVertices);
		if (m_pVertexTex == NULL)
			return false;
	}

	if (m_Flags & DVBFLAGS_COLORS)
	{
		m_pVertexColor = _pVBM->Alloc_CPixel32(m_MaxNumVertices);
		if (m_pVertexColor == NULL)
			return false;
	}

	if (m_Flags & DVBFLAGS_NORMALS)
	{
		m_pVertexNormal = _pVBM->Alloc_V3(m_MaxNumVertices);
		if (m_pVertexNormal == NULL)
			return false;
	}
	
	m_pIndex = _pVBM->Alloc_Int16(m_MaxNumTriangles * 3);
	if (m_pIndex == NULL)
		return false;

	m_pMatrix = _pVBM->Alloc_M4();
	if(m_pMatrix == NULL)
		return false;

	if (m_Flags & DVBFLAGS_CULLSORT)
	{
		m_pFlippedVB = _pCustomModel->AllocVB(_pRenderParams);
		if (m_pVB == NULL)
			return false;

		m_pFlippedIndex = _pVBM->Alloc_Int16(m_MaxNumTriangles * 3);
		if (m_pFlippedIndex == NULL)
			return false;
	}

	return true;
}

void CDynamicVB::AddTriangle(int _v1, int _v2, int _v3, int _vbase)
{
/*
#ifndef M_RTM
	if (m_NumTriangles >= m_MaxNumTriangles)
	{
		ConOut(CStrF("CDynamicVB: Too many triangles (%d/%d)", m_NumTriangles, m_MaxNumTriangles));
		m_NumTriangles++;
		return;
	}
#endif
*/
	if (_vbase == -1)
		_vbase = m_NumVertices;

	const int offset = m_NumTriangles * 3;

	m_pIndex[offset + 0] = _vbase + _v1;
	m_pIndex[offset + 1] = _vbase + _v2;
	m_pIndex[offset + 2] = _vbase + _v3;
	m_NumTriangles++;
}

void CDynamicVB::AddVertex(CVec3Dfp32 _pos, fp32 _tu, fp32 _tv, int32 _color)
{
/*
#ifndef M_RTM
	if (m_NumVertices >= m_MaxNumVertices)
	{
		ConOut(CStrF("CDynamicVB: Too many vertices (%d/%d)", m_NumVertices, m_MaxNumVertices));
		m_NumVertices++;
		return;
	}
#endif
*/
	m_pVertexPos[m_NumVertices] = _pos;

	if (m_Flags & DVBFLAGS_TEXTURE)
	{
		m_pVertexTex[m_NumVertices][0] = _tu;
		m_pVertexTex[m_NumVertices][1] = _tv;
	}

	if (m_Flags & DVBFLAGS_COLORS)
		m_pVertexColor[m_NumVertices] = _color;

	m_NumVertices++;
}

void CDynamicVB::AddVertex(CVec3Dfp32 _pos, CVec3Dfp32 _normal, CVec2Dfp32 _tex, int32 _color)
{
/*
#ifndef M_RTM
	if (m_NumVertices >= m_MaxNumVertices)
	{
		ConOut(CStrF("CDynamicVB: Too many vertices (%d/%d)", m_NumVertices, m_MaxNumVertices));
		m_NumVertices++;
		return;
	}
#endif
*/
	m_pVertexPos[m_NumVertices] = _pos;

	if (m_Flags & DVBFLAGS_NORMALS)
		m_pVertexNormal[m_NumVertices] = _normal;

	if (m_Flags & DVBFLAGS_TEXTURE)
		m_pVertexTex[m_NumVertices] = _tex;

	if (m_Flags & DVBFLAGS_COLORS)
		m_pVertexColor[m_NumVertices] = _color;

	m_NumVertices++;
}

void CDynamicVB::Render(const CMat4Dfp32& _matrix)
{
	*m_pMatrix = _matrix;

//	m_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);

	m_pVB->Matrix_Set(m_pMatrix);
	m_pVB->Geometry_VertexArray(m_pVertexPos, m_NumVertices, true);

	if (m_Flags & DVBFLAGS_NORMALS)
		m_pVB->Geometry_Normal(m_pVertexNormal);

	if (m_Flags & DVBFLAGS_TEXTURE)
		m_pVB->Geometry_TVertexArray(m_pVertexTex, 0);

	if (m_Flags & DVBFLAGS_COLORS)
		m_pVB->Geometry_ColorArray(m_pVertexColor);

	m_pVB->m_Priority -= TransparencyPriorityBiasUnit;
	m_pVB->Render_IndexedTriangles(m_pIndex, m_NumTriangles);

	if (m_Flags & DVBFLAGS_CULLSORT)
	{
		m_pFlippedVB->Matrix_Set(m_pMatrix);
		m_pFlippedVB->Geometry_VertexArray(m_pVertexPos, m_NumVertices, true);

		if (m_Flags & DVBFLAGS_NORMALS)
			m_pFlippedVB->Geometry_Normal(m_pVertexNormal);

		if (m_Flags & DVBFLAGS_TEXTURE)
			m_pFlippedVB->Geometry_TVertexArray(m_pVertexTex, 0);

		if (m_Flags & DVBFLAGS_COLORS)
			m_pFlippedVB->Geometry_ColorArray(m_pVertexColor);

		for (int i = 0; i < m_NumTriangles; i++)
		{
			const int offset = i * 3;
			m_pFlippedIndex[offset + 0] = m_pIndex[offset + 0];
			m_pFlippedIndex[offset + 1] = m_pIndex[offset + 2];
			m_pFlippedIndex[offset + 2] = m_pIndex[offset + 1];
		}

		m_pFlippedVB->m_Priority += TransparencyPriorityBiasUnit;
		m_pFlippedVB->Render_IndexedTriangles(m_pFlippedIndex, m_NumTriangles);
	}
}

void CDynamicVB::RecalculateNormals()
{
	if (!(m_Flags & DVBFLAGS_NORMALS))
		return;

	int v, t;

	for (v = 0; v < m_NumVertices; v++)
		m_pVertexNormal[v] = 0;

	for (t = 0; t < m_NumTriangles; t++)
	{
		int v0, v1, v2;
		CVec3Dfp32 p0, p1, p2, a, b, n;

		const int offset = t * 3;

		v0 = m_pIndex[offset + 0];
		v1 = m_pIndex[offset + 1];
		v2 = m_pIndex[offset + 2];

		p0 = m_pVertexPos[v0];
		p1 = m_pVertexPos[v1];
		p2 = m_pVertexPos[v2];

		a = p1 - p0;
		b = p2 - p0;

		a.CrossProd(b, n);
//				n.Normalize(); // Not needed according to Vogue.

		m_pVertexNormal[v0] += n;
		m_pVertexNormal[v1] += n;
		m_pVertexNormal[v2] += n;
	}

	for (v = 0; v < m_NumVertices; v++)
		m_pVertexNormal[v].Normalize();
}

void CDynamicVB::ConvertToFlatShaded()
{
	if (!(m_Flags & DVBFLAGS_FLATSHADED))
		return;

	if (!(m_Flags & DVBFLAGS_NORMALS))
		return;

	int t;

	// Store positions in normalarray.
	for (t = 0; t < m_NumTriangles; t++)
	{
		int v0, v1, v2;
		CVec3Dfp32 p0, p1, p2;

		const int offset = t * 3;

		v0 = m_pIndex[offset + 0];
		v1 = m_pIndex[offset + 1];
		v2 = m_pIndex[offset + 2];

		p0 = m_pVertexPos[v0];
		p1 = m_pVertexPos[v1];
		p2 = m_pVertexPos[v2];

		m_pVertexNormal[offset + 0] = p0;
		m_pVertexNormal[offset + 1] = p1;
		m_pVertexNormal[offset + 2] = p2;
	}

	// Restore rearranged and replicated positions from normalarray.
	for (t = 0; t < m_NumTriangles; t++)
	{
		int v0, v1, v2;
		CVec3Dfp32 p0, p1, p2;

		v0 = t * 3 + 0;
		v1 = t * 3 + 1;
		v2 = t * 3 + 2;

		p0 = m_pVertexNormal[v0];
		p1 = m_pVertexNormal[v1];
		p2 = m_pVertexNormal[v2];

		m_pVertexPos[v0] = p0;
		m_pVertexPos[v1] = p1;
		m_pVertexPos[v2] = p2;
	}

	if (m_Flags & DVBFLAGS_TEXTURE)
	{
		// Store texcoords in normalarray.
		for (t = 0; t < m_NumTriangles; t++)
		{
			int v0, v1, v2;
			CVec2Dfp32 t0, t1, t2;

			const int offset = t * 3;

			v0 = m_pIndex[offset + 0];
			v1 = m_pIndex[offset + 1];
			v2 = m_pIndex[offset + 2];

			t0 = m_pVertexTex[v0];
			t1 = m_pVertexTex[v1];
			t2 = m_pVertexTex[v2];

			m_pVertexNormal[offset + 0] = CVec3Dfp32(t0[0], t0[1], 0);
			m_pVertexNormal[offset + 1] = CVec3Dfp32(t1[0], t1[1], 0);
			m_pVertexNormal[offset + 2] = CVec3Dfp32(t2[0], t2[1], 0);
		}

		// Restore rearranged and replicated texcoords from normalarray.
		for (t = 0; t < m_NumTriangles; t++)
		{
			int v0, v1, v2;
			CVec3Dfp32 t0, t1, t2;

			v0 = t * 3 + 0;
			v1 = t * 3 + 1;
			v2 = t * 3 + 2;

			t0 = m_pVertexNormal[v0];
			t1 = m_pVertexNormal[v1];
			t2 = m_pVertexNormal[v2];

			m_pVertexTex[v0] = CVec2Dfp32(t0[0], t0[1]);
			m_pVertexTex[v1] = CVec2Dfp32(t1[0], t1[1]);
			m_pVertexTex[v2] = CVec2Dfp32(t2[0], t2[1]);
		}
	}

	if (m_Flags & DVBFLAGS_COLORS)
	{
		// Store colors in normalarray.
		for (t = 0; t < m_NumTriangles; t++)
		{
			int v0, v1, v2;
			CPixel32 c0, c1, c2;

			const int offset = t * 3;

			v0 = m_pIndex[offset + 0];
			v1 = m_pIndex[offset + 1];
			v2 = m_pIndex[offset + 2];

			c0 = m_pVertexColor[v0];
			c1 = m_pVertexColor[v1];
			c2 = m_pVertexColor[v2];

			m_pVertexNormal[offset + 0][0] = *(fp32*)(&c0);
			m_pVertexNormal[offset + 1][0] = *(fp32*)(&c1);
			m_pVertexNormal[offset + 2][0] = *(fp32*)(&c2);
		}

		// Restore rearranged and replicated colors from normalarray.
		for (t = 0; t < m_NumTriangles; t++)
		{
			int v0, v1, v2;
			CVec3Dfp32 c0, c1, c2;

			v0 = t * 3 + 0;
			v1 = t * 3 + 1;
			v2 = t * 3 + 2;

			c0 = m_pVertexNormal[v0];
			c1 = m_pVertexNormal[v1];
			c2 = m_pVertexNormal[v2];

			m_pVertexColor[v0] = *(CPixel32*)(&c0[0]);
			m_pVertexColor[v1] = *(CPixel32*)(&c1[0]);
			m_pVertexColor[v2] = *(CPixel32*)(&c2[0]);
		}
	}

	// Calculate triangle normals and set the 3 corresponding vertex normals.
	for (t = 0; t < m_NumTriangles; t++)
	{
		int v0, v1, v2;
		CVec3Dfp32 p0, p1, p2, a, b, n;

		v0 = t * 3 + 0;
		v1 = t * 3 + 1;
		v2 = t * 3 + 2;

		m_pIndex[v0] = v0;
		m_pIndex[v1] = v1;
		m_pIndex[v2] = v2;

		p0 = m_pVertexPos[v0];
		p1 = m_pVertexPos[v1];
		p2 = m_pVertexPos[v2];

		a = p1 - p0;
		b = p2 - p0;

		b.CrossProd(a, n);
		n.Normalize();

		m_pVertexNormal[v0] = n;
		m_pVertexNormal[v1] = n;
		m_pVertexNormal[v2] = n;
	}

	m_NumVertices = m_NumTriangles * 3;
}
/*
bool CDynamicVB::ApplyDiffuseLight(CVec3Dfp32 &_Center, CXR_Model_Custom* _pCustomModel, CXR_VBManager* _pVBM)
{
	if (!(m_Flags & DVBFLAGS_COLORS))
		return false;

	CPixel32* pVertexLight = _pVBM->Alloc_CPixel32(m_NumVertices);
	if (pVertexLight == NULL)
		return false;

	// Normal is NULL, since the lit point is considered to be omni.
	if (_pCustomModel->ApplyDiffuseLight(_Center, m_NumVertices, m_pVertexPos, m_pVertexNormal, pVertexLight, 1.0f, 2.0f))
	{
		for (int v = 0; v < m_NumVertices; v++)
			m_pVertexColor[v] = m_pVertexColor[v] * pVertexLight[v];

		return true;
	}

	return false;
}
*/
void CDynamicVB::DebugRenderNormals(CWireContainer* _pWC, fp32 _Duration, int32 _Color, fp32 _Length)
{
	if (_pWC == NULL)
		return;

	for (int v = 0; v < m_NumVertices; v++)
		_pWC->RenderVector(m_pVertexPos[v], m_pVertexNormal[v] * _Length, _Color, _Duration, true);
}
