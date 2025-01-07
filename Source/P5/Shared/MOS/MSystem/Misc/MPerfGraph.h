
#ifndef _INC_MPERFGRAPH
#define _INC_MPERFGRAPH

#include "../Raster/MImage.h"

class CRenderContext;

// -------------------------------------------------------------------
//  CPerfGraph
// -------------------------------------------------------------------
#define GRAPH_MAXFUNCS	4

class CGraphPlot
{
public:
	fp32 m_Y;
	CPixel32 m_Col;

	CGraphPlot()
	{
	}

	CGraphPlot(fp32 _Y, CPixel32 _c)
	{
		m_Y = _Y;
		m_Col = _c;
	}
};

class SYSTEMDLLEXPORT CPerfGraph : public CReferenceCount
{
	MRTC_DECLARE;

protected:
	TArray<TPtr<TQueue<CGraphPlot> > > m_lspGraphs;

	int m_GraphLen;
	fp32 m_MinY;
	fp32 m_MaxY;
	fp32 m_Width;
	fp32 m_Height;

public:
	CPerfGraph();
	virtual void Create(int _Len, fp64 _MinY, fp64 _MaxY, fp32 _Width, fp32 _Height, int _nFunc = 1);
	virtual void Plotn(int _nFuncs, fp32* _pValues, CPixel32* _pColors);
	virtual void Plot(fp32 _Value, CPixel32 _Color);
	virtual void Plot2(fp32 _V0, fp32 _V1, CPixel32 _Color0, CPixel32 _Color1);
	virtual void Plot3(fp32 _V0, fp32 _V1, fp32 _V2, CPixel32 _Color0, CPixel32 _Color1, CPixel32 _Color2);
	virtual void Plot4(fp32 _V0, fp32 _V1, fp32 _V2, fp32 _V3, CPixel32 _Color0, CPixel32 _Color1, CPixel32 _Color2, CPixel32 _Color3);
	virtual class CXR_VertexBuffer* Render(class CXR_VBManager* _pVBM, const CVec2Dfp32& _Pos, const CMat4Dfp32& _Mat);
};

typedef TPtr<CPerfGraph> spCGraph;

#endif // _INC_MPERFGRAPH
