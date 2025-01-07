#include "PCH.h"

#include "MDisplayPS3.h"
#include "MRenderPS3_Context.h"
#include "MRenderPS3_Def.h"
#include "MFloat.h"

#include "../../XR/XRVBContext.h"

namespace NRenderPS3GCM
{

// #define INTERNAL_GL_VERTEX

#define CRCGL_CONVERT_PRIMITIVES_TO_TRIANGLES

//----------------------------------------------------------------
void CRCPS3GCM::Internal_RenderPolygon(int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CVec4Dfp32* _pCol, 
											  const CVec4Dfp32* _pSpec, //const fp32* _pFog,
		const CVec4Dfp32* _pTV0, const CVec4Dfp32* _pTV1, const CVec4Dfp32* _pTV2, const CVec4Dfp32* _pTV3, int _Color)
{
	MSCOPE(CRCPS3GCM::Internal_RenderPolygon, RENDER_PS3);
	M_BREAKPOINT;
}



//----------------------------------------------------------------
void CRCPS3GCM::Render_IndexedTriangles(uint16* _pIndices, int _nTriangles)
{
	MSCOPE(CRCPS3GCM::Render_IndexedTriangles, RENDER_PS3);

	if (_nTriangles == 0xffff)
	{
		// Recursively call this function
		mint *pList = ((mint *)_pIndices);
		int nLists = *pList;
		++pList;
		for (int i = 0; i < nLists; ++i)
		{
			int nTri = *pList;
			++pList;
			Render_IndexedTriangles(*((uint16 **)pList), nTri);
			++pList;
		}
		return;
	}
	else if(_nTriangles == 0)
		return;	// WTF?!


	if(m_GeomVBID)
	{
		CRenderPS3Resource_VertexBuffer_Local* pVB = m_ContextGeometry.Bind(m_GeomVBID);
		if(!pVB)
			return;
	}
	else
	{
		if(!m_ContextGeometry.Bind(m_Geom))
			return;
	}

	if (m_AttribChanged) Attrib_Update();
	if (m_MatrixChanged) Matrix_Update();

	{
		if(CRenderPS3Resource_Collect_Main::NeedCollect(_pIndices))
		{
			uint32 OffsetP = m_ContextGeometry.m_Collector.Collect(_pIndices, _nTriangles * 3);
			gcmSetDrawIndexArray(CELL_GCM_PRIMITIVE_TRIANGLES, _nTriangles * 3, CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16, CELL_GCM_LOCATION_MAIN, OffsetP);
			m_ContextGeometry.m_Collector.CollectLabel();
		}
		else
		{
			gcmSetDrawIndexArray(CELL_GCM_PRIMITIVE_TRIANGLES, _nTriangles * 3, CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16, CELL_GCM_LOCATION_MAIN, gcmAddressToOffset(_pIndices));
		}
		m_nTriangles += _nTriangles;
		m_ContextStatistics.m_nVTotal += _nTriangles * 3;
	}
}

void CRCPS3GCM::Render_IndexedWires(uint16* _pIndices, int _Len)
{
	MSCOPE(CRCPS3GCM::Render_IndexedWires, RENDER_PS3);

	if(m_GeomVBID)
	{
		CRenderPS3Resource_VertexBuffer_Local* pVB = m_ContextGeometry.Bind(m_GeomVBID);
		if(!pVB)
			return;
	}
	else
	{
		if(!m_ContextGeometry.Bind(m_Geom))
			return;
	}
	if (m_AttribChanged) Attrib_Update();
	if (m_MatrixChanged) Matrix_Update();


	{
		if(CRenderPS3Resource_Collect_Main::NeedCollect(_pIndices))
		{
			uint32 OffsetP = m_ContextGeometry.m_Collector.Collect(_pIndices, _Len);
			gcmSetDrawIndexArray(CELL_GCM_PRIMITIVE_LINES, _Len, CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16, CELL_GCM_LOCATION_MAIN, OffsetP);
			m_ContextGeometry.m_Collector.CollectLabel();
		}
		else
		{
			gcmSetDrawIndexArray(CELL_GCM_PRIMITIVE_LINES, _Len, CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16, CELL_GCM_LOCATION_MAIN, gcmAddressToOffset(_pIndices));
		}
		m_nWires += _Len >> 1;
		m_ContextStatistics.m_nVTotal += _Len;
	}
}

void CRCPS3GCM::Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen)
{
	MSCOPE(CRCPS3GCM::Render_IndexedPrimitives, RENDER_PS3);

	if(m_GeomVBID)
	{
		CRenderPS3Resource_VertexBuffer_Local* pVB = m_ContextGeometry.Bind(m_GeomVBID);
		if(!pVB)
			return;
	}
	else
	{
		if(!m_ContextGeometry.Bind(m_Geom))
			return;
	}

	if (m_AttribChanged) Attrib_Update();
	if (m_MatrixChanged) Matrix_Update();

	CRCPrimStreamIterator Iter(_pPrimStream, _StreamLen);

	uint32 nV = 0;
	const uint16* piV = NULL;
	uint32 iPrim = 0;

	if (!Iter.IsValid())
		return;

	bool bCollected = false;

	while(1)
	{
		uint32 Prim = Iter.GetCurrentType();
		const uint16* pPrim = Iter.GetCurrentPointer()-1;

		switch(Prim)
		{
		case CRC_RIP_TRIANGLES :
			{
				iPrim = CELL_GCM_PRIMITIVE_TRIANGLES;
				m_nTriangles += pPrim[1];
				nV = pPrim[1]*3;
				piV = &pPrim[2];
			}
			break;
		case CRC_RIP_QUADS :
		case CRC_RIP_QUADSTRIP :
			{
			}
			break;
		case CRC_RIP_TRISTRIP :
		case CRC_RIP_TRIFAN :
		case CRC_RIP_POLYGON :
			{
				nV = pPrim[1];
				piV = &pPrim[2];

				if (Prim == CRC_RIP_POLYGON)
				{
					iPrim = CELL_GCM_PRIMITIVE_POLYGON;
					m_nPolygons++;
				}
				else
				{
					m_nTriangles += nV-2;
					iPrim = (Prim == CRC_RIP_TRISTRIP) ? CELL_GCM_PRIMITIVE_TRIANGLE_STRIP : CELL_GCM_PRIMITIVE_TRIANGLE_FAN;
				}

			}
			break;
		default :
			{
				piV = NULL;
				break;
			}
		}

		if (piV)
		{
			m_ContextStatistics.m_nVTotal += nV;
			if(CRenderPS3Resource_Collect_Main::NeedCollect(piV))
			{
				uint32 OffsetP = m_ContextGeometry.m_Collector.Collect(piV, nV);
				gcmSetDrawIndexArray(iPrim, nV, CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16, CELL_GCM_LOCATION_MAIN, OffsetP);
				bCollected = true;
			}
			else
			{
				gcmSetDrawIndexArray(iPrim, nV, CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16, CELL_GCM_LOCATION_MAIN, gcmAddressToOffset(piV));
			}
		}

		if (!Iter.Next())
			break;
	}
	if(bCollected)
	{
		m_ContextGeometry.m_Collector.CollectLabel();
	}
}

//----------------------------------------------------------------
void CRCPS3GCM::Render_VertexBuffer(int _VBID)
{
	MSCOPE(CRCPS3GCM::Render_VertexBuffer, RENDER_PS3);

	CRenderPS3Resource_VertexBuffer_Local* pVB = m_ContextGeometry.Bind(_VBID);
	if(!pVB)
		return;

	if (m_AttribChanged) Attrib_Update();
	if (m_MatrixChanged) Matrix_Update();

	if(pVB->m_PrimLen)
	{
//		MTRACE_SCOPECOMMANDS("gcmSetDrawIndexArray");
		gcmSetDrawIndexArray(pVB->m_PrimType, pVB->m_PrimLen, CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16, CELL_GCM_LOCATION_LOCAL, pVB->m_PrimOffset);
		m_nTriangles += pVB->m_nTri;
		m_ContextStatistics.m_nVTotal += pVB->m_PrimLen;
	}
	else
	{
		gcmSetDrawArrays(pVB->m_PrimType, 0, pVB->m_nV);
		m_ContextStatistics.m_nVTotal += pVB->m_nV;
	}
}

void CRCPS3GCM::Render_VertexBuffer_IndexBufferTriangles(uint _VBID, uint _IBID, uint _nTriangles, uint _PrimOffset)
{
	MSCOPE(CRCPS3GCM::Render_VertexBuffer_IndexBufferTriangles, RENDER_PS3);


	CRenderPS3Resource_VertexBuffer_Local* pVB = m_ContextGeometry.Bind(_VBID);
	CRenderPS3Resource_VertexBuffer_Local* pIB = pVB;
	
	if(_VBID != _IBID)
		pIB = m_ContextGeometry.GetVB(_IBID);
	
	if(!pVB || !pIB)
		return;

	if (m_AttribChanged) Attrib_Update();
	if (m_MatrixChanged) Matrix_Update();

	{
		gcmSetDrawIndexArray(CELL_GCM_PRIMITIVE_TRIANGLES, _nTriangles * 3, CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16, CELL_GCM_LOCATION_LOCAL, pIB->m_PrimOffset + (_PrimOffset << 1));
		m_nTriangles += _nTriangles;
		m_ContextStatistics.m_nVTotal += _nTriangles * 3;
	}
}

//----------------------------------------------------------------
void CRCPS3GCM::Render_Wire(const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, CPixel32 _Color)
{
	MSCOPE(CRCPS3GCM::Render_Wire, RENDER_PS3);
	M_BREAKPOINT;
}

void CRCPS3GCM::Render_WireStrip(const CVec3Dfp32* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color)
{
	MSCOPE(CRCPS3GCM::Render_WireStrip, RENDER_PS3);
	M_BREAKPOINT;
}

void CRCPS3GCM::Render_WireLoop(const CVec3Dfp32* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color)
{
	MSCOPE(CRCPS3GCM::Render_WireLoop, RENDER_PS3);
	M_BREAKPOINT;
}


bool CRCPS3GCM::ReadDepthPixels(int _x, int _y, int _w, int _h, fp32* _pBuffer)
{
	MSCOPE(CRCPS3GCM::ReadDepthPixels, RENDER_PS3);
	M_BREAKPOINT;
	return false;
}

};	// namespace NRenderPS3GCM
