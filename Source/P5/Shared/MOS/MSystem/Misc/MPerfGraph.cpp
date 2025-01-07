
#include "PCH.h"
#include "MPerfGraph.h"
#include "../../XR/XRVBManager.h"

#ifdef GRAPH_MAXFUNCS
#undef GRAPH_MAXFUNCS
#endif

#define GRAPH_MAXFUNCS 16

// -------------------------------------------------------------------
//  CPerfGraph
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CPerfGraph, CReferenceCount);

CPerfGraph::CPerfGraph()
{
	MAUTOSTRIP(CPerfGraph_ctor, MAUTOSTRIP_VOID);
}

void CPerfGraph::Create(int _Len, fp64 _MinY, fp64 _MaxY, fp32 _Width, fp32 _Height, int _nFunc)
{
	MAUTOSTRIP(CPerfGraph_Create, MAUTOSTRIP_VOID);
	m_lspGraphs.SetLen(_nFunc);
	for(int f = 0; f < _nFunc; f++)
	{
		typedef TQueue<CGraphPlot> TQueue_CGraphPlot;
		m_lspGraphs[f] = MNew(TQueue_CGraphPlot);
		if (!m_lspGraphs[f]) MemError("Create");
		m_lspGraphs[f]->Create(_Len);
	}

	m_GraphLen = _Len;
	m_MinY = _MinY;
	m_MaxY = _MaxY;
	m_Width = _Width;
	m_Height = _Height;
}

void CPerfGraph::Plotn(int _nFuncs, fp32* _pValues, CPixel32* _pColors)
{
	MAUTOSTRIP(CPerfGraph_Plotn, MAUTOSTRIP_VOID);
	if (!m_lspGraphs.Len()) return;

	if (_nFuncs > m_lspGraphs.Len())
	{
		int f = m_lspGraphs.Len();
		m_lspGraphs.SetLen(_nFuncs);
		for(; f < _nFuncs; f++)
		{
			typedef TQueue<CGraphPlot> TQueue_CGraphPlot;
			m_lspGraphs[f] = MNew(TQueue_CGraphPlot);
			if (!m_lspGraphs[f]) MemError("Create");
			m_lspGraphs[f]->Create(m_GraphLen);
		}
	}

	int f = 0;
	for(f = 0; f < Min(m_lspGraphs.Len(), _nFuncs); f++)
	{
		if (!m_lspGraphs[f])
			Error("Plotn", "Graph not allocated.");
		if (!m_lspGraphs[f]->MaxPut()) m_lspGraphs[f]->Get();
		m_lspGraphs[f]->Put(CGraphPlot(Min(Max(_pValues[f], m_MinY), m_MaxY), _pColors[f]));
	}

	for(; f < m_lspGraphs.Len(); f++)
	{
		if (!m_lspGraphs[f]->MaxPut()) m_lspGraphs[f]->Get();
		m_lspGraphs[f]->Put(CGraphPlot(Min(Max(0.0f, m_MinY), m_MaxY), 0x00000000));
	}
}

void CPerfGraph::Plot(fp32 _Value, CPixel32 _Color)
{
	MAUTOSTRIP(CPerfGraph_Plot, MAUTOSTRIP_VOID);
	Plotn(1, &_Value, &_Color);
}

void CPerfGraph::Plot2(fp32 _V0, fp32 _V1, CPixel32 _Color0, CPixel32 _Color1)
{
	MAUTOSTRIP(CPerfGraph_Plot2, MAUTOSTRIP_VOID);
	fp32 V[2];
	V[0] = _V0;
	V[1] = _V1;
	CPixel32 C[2];
	C[0] = _Color0;
	C[1] = _Color1;
	Plotn(2, V, C);
}

void CPerfGraph::Plot3(fp32 _V0, fp32 _V1, fp32 _V2, CPixel32 _Color0, CPixel32 _Color1, CPixel32 _Color2)
{
	MAUTOSTRIP(CPerfGraph_Plot3, MAUTOSTRIP_VOID);
	fp32 V[3];
	V[0] = _V0;
	V[1] = _V1;
	V[2] = _V2;
	CPixel32 C[3];
	C[0] = _Color0;
	C[1] = _Color1;
	C[2] = _Color2;
	Plotn(3, V, C);
}

void CPerfGraph::Plot4(fp32 _V0, fp32 _V1, fp32 _V2, fp32 _V3, CPixel32 _Color0, CPixel32 _Color1, CPixel32 _Color2, CPixel32 _Color3)
{
	MAUTOSTRIP(CPerfGraph_Plot4, MAUTOSTRIP_VOID);
	fp32 V[4];
	V[0] = _V0;
	V[1] = _V1;
	V[2] = _V2;
	V[3] = _V3;
	CPixel32 C[3];
	C[0] = _Color0;
	C[1] = _Color1;
	C[2] = _Color2;
	C[3] = _Color3;
	Plotn(4, V, C);
}

CXR_VertexBuffer* CPerfGraph::Render(CXR_VBManager* _pVBM, const CVec2Dfp32& _Pos, const CMat4Dfp32& _Mat)
{
	MAUTOSTRIP(CPerfGraph_Render, NULL);
	if (!m_lspGraphs.Len()) return NULL;
	CGraphPlot* lpPlot[GRAPH_MAXFUNCS];

	CXR_VertexBuffer* pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES | CXR_VB_COLORS, m_GraphLen * 4 * m_lspGraphs.Len());
	if (!pVB) return NULL;
	CMat4Dfp32* pMat = _pVBM->Alloc_M4(_Mat);
	if (!pMat) return NULL;

	{
		for(int f = 0; f < m_lspGraphs.Len(); f++)
			lpPlot[f] = m_lspGraphs[f]->GetTail();
	}

	pVB->Matrix_Set(pMat);

	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	uint16* pPrim = _pVBM->Alloc_Int16(m_GraphLen * 6 * m_lspGraphs.Len());
	if (!pPrim)
		return NULL;

	CVec3Dfp32* pV = pVB->GetVBChain()->m_pV;
	CPixel32* pC = pVB->GetVBChain()->m_pCol;

	fp32 x = _Pos.k[0];
	fp32 dx = m_Width / m_GraphLen;
	fp32 yscale = m_Height / (m_MaxY - m_MinY);
	int iP = 0;
	int iV = 0;
	int nT = 0;
	while(lpPlot[0])
	{
		fp32 YLast = 0.0f;
		for(int f = 0; f < m_lspGraphs.Len(); f++)
		{
			CGraphPlot* pPlot = lpPlot[f];
			if (!pPlot) continue;
			if (pPlot->m_Col.GetA())
			{
				fp32 Y0 = m_Height - (YLast - m_MinY) * yscale;
				fp32 Y1 = m_Height - (YLast + pPlot->m_Y - m_MinY) * yscale;
				YLast = YLast + pPlot->m_Y;
				if (Y0 > Y1) Swap(Y0, Y1);
				Y0 += _Pos.k[1];
				Y1 += _Pos.k[1];

				*pV++ = CVec3Dfp32(x, Y0, 0);
				*pV++ = CVec3Dfp32(x, Y1, 0);
				*pV++ = CVec3Dfp32(x+dx, Y1, 0);
				*pV++ = CVec3Dfp32(x+dx, Y0, 0);
				*pC++ = pPlot->m_Col;
				*pC++ = pPlot->m_Col;
				*pC++ = pPlot->m_Col;
				*pC++ = pPlot->m_Col;

				pPrim[iP + 0] = iV + 0;
				pPrim[iP + 1] = iV + 1;
				pPrim[iP + 2] = iV + 3;
				pPrim[iP + 3] = iV + 3;
				pPrim[iP + 4] = iV + 1;
				pPrim[iP + 5] = iV + 2;
				iP += 6;
				iV += 4;
				nT += 2;
			}

			lpPlot[f] = m_lspGraphs[f]->GetNext(lpPlot[f]);
		}

		x += dx;
	}

	pVB->Render_IndexedTriangles(pPrim, nT);
	return (nT) ? pVB : NULL;
}
