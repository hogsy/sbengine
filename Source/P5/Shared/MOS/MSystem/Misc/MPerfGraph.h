
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
	fp4 m_Y;
	CPixel32 m_Col;

	CGraphPlot()
	{
	}

	CGraphPlot(fp4 _Y, CPixel32 _c)
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
	fp4 m_MinY;
	fp4 m_MaxY;
	fp4 m_Width;
	fp4 m_Height;

public:
	CPerfGraph();
	virtual void Create(int _Len, fp8 _MinY, fp8 _MaxY, fp4 _Width, fp4 _Height, int _nFunc = 1);
	virtual void Plotn(int _nFuncs, fp4* _pValues, CPixel32* _pColors);
	virtual void Plot(fp4 _Value, CPixel32 _Color);
	virtual void Plot2(fp4 _V0, fp4 _V1, CPixel32 _Color0, CPixel32 _Color1);
	virtual void Plot3(fp4 _V0, fp4 _V1, fp4 _V2, CPixel32 _Color0, CPixel32 _Color1, CPixel32 _Color2);
	virtual void Plot4(fp4 _V0, fp4 _V1, fp4 _V2, fp4 _V3, CPixel32 _Color0, CPixel32 _Color1, CPixel32 _Color2, CPixel32 _Color3);
	virtual class CXR_VertexBuffer* Render(class CXR_VBManager* _pVBM, const CVec2Dfp4& _Pos, const CMat43fp4& _Mat);
};

typedef TPtr<CPerfGraph> spCGraph;

#endif // _INC_MPERFGRAPH
