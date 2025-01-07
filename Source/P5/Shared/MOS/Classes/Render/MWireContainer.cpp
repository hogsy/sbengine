/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			MWireContainer.cpp

	Copyright:		Starbreeze AB, 2002

	Contents:		CWireContainer, CDebugRenderContainer
\*____________________________________________________________________*/
#include "PCH.h"

#include "../../MOS.h"
#include "MWireContainer.h"
#include "MFloat.h"


/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯| 
| CWireContainer
|____________________________________________________________________________________|
\************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CWireContainer, CReferenceCount);
IMPLEMENT_OPERATOR_NEW(CWireContainer);

void CWireContainer::Create(int _MaxWires)
{
	MAUTOSTRIP(CWireContainer_Create, MAUTOSTRIP_VOID);
	m_lV.SetLen(_MaxWires*2);
	m_lCol.SetLen(_MaxWires*2);
	m_lWires.SetLen(_MaxWires);
	m_IDHeap.Create(_MaxWires);
	m_iWNext = 0;
}

int CWireContainer::AllocWire()
{
	MAUTOSTRIP(CWireContainer_AllocWire, 0);
	int iW = m_IDHeap.AllocID();
	if (iW >= 0) 
	{
		m_lWires[iW].Clear();
		m_lWires[iW].m_bInUse = 1;
		m_iWNext = iW;
		return iW;
	}

	m_iWNext = (m_iWNext + 1) % m_lWires.Len();
	iW = m_iWNext;
	m_lWires[iW].Clear();
	m_lWires[iW].m_bInUse = 1;
	return m_iWNext;
}

void CWireContainer::FreeWire(int _iW)
{
	MAUTOSTRIP(CWireContainer_FreeWire, MAUTOSTRIP_VOID);
	if (m_lWires[_iW].m_bInUse)
		m_IDHeap.FreeID(_iW);
	m_lWires[_iW].Clear();
}

void CWireContainer::RenderWire(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWireContainer_RenderWire, MAUTOSTRIP_VOID);
	int iW = AllocWire();
	CWire& Wire = m_lWires[iW];
	Wire.m_bFade = _bFade;
	Wire.m_Duration = _Duration;
	Wire.m_Age = 0;

	int iV = iW*2;
	m_lV[iV] = _p0;
	m_lV[iV+1] = _p1;
	m_lCol[iV] = _Color;
	m_lCol[iV+1] = _Color;

//	ConOut(CStrF("Wire added %d, %s, %s", iW, (char*)_p0.GetString(), (char*)_p1.GetString()));
}

void CWireContainer::RenderVertex(const CVec3Dfp32& _p, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWireContainer_RenderVertex, MAUTOSTRIP_VOID);
	RenderWire(_p - CVec3Dfp32(2,0,0), _p + CVec3Dfp32(2,0,0), _Color, _Duration, _bFade);
	RenderWire(_p - CVec3Dfp32(0,2,0), _p + CVec3Dfp32(0,2,0), _Color, _Duration, _bFade);
	RenderWire(_p - CVec3Dfp32(0,0,2), _p + CVec3Dfp32(0,0,2), _Color, _Duration, _bFade);
}

void CWireContainer::RenderVector(const CVec3Dfp32& _p, const CVec3Dfp32& _v, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWireContainer_RenderVector, MAUTOSTRIP_VOID);
	CMat4Dfp32 Mat;
	_v.SetRow(Mat, 0);
	if (M_Fabs(_v[2]) < 0.9f)
		CVec3Dfp32(0,0,1).SetRow(Mat, 1);
	else
		CVec3Dfp32(0,1,0).SetRow(Mat, 1);

	Mat.RecreateMatrix(0,1);

	CVec3Dfp32 p1 = _p + _v;

	fp32 Len = _v.Length()*0.2f;
	fp32 Len2 = Len*0.6f;

	RenderWire(_p, p1, _Color, _Duration, _bFade);
	RenderWire(p1, p1 - (CVec3Dfp32::GetRow(Mat,0)*Len) + (CVec3Dfp32::GetRow(Mat,1)*Len2), _Color, _Duration, _bFade);
	RenderWire(p1, p1 - (CVec3Dfp32::GetRow(Mat,0)*Len) - (CVec3Dfp32::GetRow(Mat,1)*Len2), _Color, _Duration, _bFade);
	RenderWire(p1, p1 - (CVec3Dfp32::GetRow(Mat,0)*Len) + (CVec3Dfp32::GetRow(Mat,2)*Len2), _Color, _Duration, _bFade);
	RenderWire(p1, p1 - (CVec3Dfp32::GetRow(Mat,0)*Len) - (CVec3Dfp32::GetRow(Mat,2)*Len2), _Color, _Duration, _bFade);
}

void CWireContainer::RenderMatrix(const CMat4Dfp32& _Mat, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWireContainer_RenderMatrix, MAUTOSTRIP_VOID);
	const CVec3Dfp32& p = CVec3Dfp32::GetRow(_Mat, 3);
	RenderVector(p, CVec3Dfp32::GetRow(_Mat, 0)*8.0f, _Color, _Duration, _bFade);
	RenderVector(p, CVec3Dfp32::GetRow(_Mat, 1)*8.0f, _Color, _Duration, _bFade);
	RenderVector(p, CVec3Dfp32::GetRow(_Mat, 2)*8.0f, _Color, _Duration, _bFade);
}

void CWireContainer::RenderMatrix(const CMat4Dfp32& _Mat, fp32 _Duration, bool _bFade, CPixel32 _ColorX, CPixel32 _ColorY, CPixel32 _ColorZ)
{
	MAUTOSTRIP(CWireContainer_RenderMatrix_2, MAUTOSTRIP_VOID);
	const CVec3Dfp32& p = CVec3Dfp32::GetRow(_Mat, 3);
	RenderVector(p, CVec3Dfp32::GetRow(_Mat, 0)*8.0f, _ColorX, _Duration, _bFade);
	RenderVector(p, CVec3Dfp32::GetRow(_Mat, 1)*8.0f, _ColorY, _Duration, _bFade);
	RenderVector(p, CVec3Dfp32::GetRow(_Mat, 2)*8.0f, _ColorZ, _Duration, _bFade);
}

void CWireContainer::RenderQuaternion(const CVec3Dfp32& _p, const CQuatfp32& _Q, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWireContainer_RenderQuaternion, MAUTOSTRIP_VOID);
	CMat4Dfp32 Mat;
	_Q.CreateMatrix(Mat);
	_p.SetRow(Mat, 3);
	if (_Color == 0xffffffff)
		RenderMatrix(Mat, _Duration, _bFade);
	else
		RenderMatrix(Mat, _Color, _Duration, _bFade);
}

void CWireContainer::RenderAxisRot(const CVec3Dfp32& _p, const CAxisRotfp32& _AxisRot, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWireContainer_RenderAxisRot, MAUTOSTRIP_VOID);
	CMat4Dfp32 Mat;
	_AxisRot.CreateMatrix(Mat);
	_p.SetRow(Mat, 3);
	if (_Color == 0xffffffff)
		RenderMatrix(Mat, _Duration, _bFade);
	else
		RenderMatrix(Mat, _Color, _Duration, _bFade);
}

void CWireContainer::RenderAABB(const CVec3Dfp32& _Min, const CVec3Dfp32& _Max, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWireContainer_RenderAABB, MAUTOSTRIP_VOID);
	CVec3Dfp32 Verts[8];

	for(int i = 0; i < 8; i++)
	{
		Verts[i].k[0] = (i & 1) ? _Min.k[0] : _Max.k[0];
		Verts[i].k[1] = (i & 2) ? _Min.k[1] : _Max.k[1];
		Verts[i].k[2] = (i & 4) ? _Min.k[2] : _Max.k[2];
	}

	for(int k = 0; k < 4; k++)
	{
		int iY = (k&2)*2+(k&1);
		RenderWire(Verts[k*2], Verts[(k*2+1)&7], _Color, _Duration, _bFade);	// X lines
		RenderWire(Verts[iY], Verts[iY+2], _Color, _Duration, _bFade);			// Y lines
		RenderWire(Verts[k], Verts[k+4], _Color, _Duration, _bFade);			// Z lines
	}
}

void CWireContainer::RenderAABB(const CBox3Dfp32& _Box, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWireContainer_RenderAABB_2, MAUTOSTRIP_VOID);
	RenderAABB(_Box.m_Min, _Box.m_Max, _Color, _Duration, _bFade);
}

void CWireContainer::RenderAABB(const CMat4Dfp32& _Transform, const CBox3Dfp32& _Box, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWireContainer_RenderAABB_2, MAUTOSTRIP_VOID);
	CVec3Dfp32 BoxMin, BoxMax;
	BoxMin	= _Box.m_Min * _Transform;
	BoxMax	= _Box.m_Max * _Transform;
	RenderAABB(BoxMin, BoxMax, _Color, _Duration, _bFade);
}

void CWireContainer::RenderOBB(const CMat4Dfp32& _Pos, const CVec3Dfp32& _Extents, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWireContainer_RenderOBB, MAUTOSTRIP_VOID);
	CVec3Dfp32 Verts[8];

	for(int i = 0; i < 8; i++)
	{
		Verts[i] = CVec3Dfp32::GetRow(_Pos,3);
		for(int k = 0; k < 3; k++)
			Verts[i] += CVec3Dfp32::GetRow(_Pos, k) * ((i & (1 << k)) ? _Extents[k] : -_Extents[k]);
	}

	for(int k = 0; k < 4; k++)
	{
		int iY = (k&2)*2+(k&1);
		RenderWire(Verts[k*2], Verts[(k*2+1)&7], _Color, _Duration, _bFade);	// X lines
		RenderWire(Verts[iY], Verts[iY+2], _Color, _Duration, _bFade);			// Y lines
		RenderWire(Verts[k], Verts[k+4], _Color, _Duration, _bFade);			// Z lines
	}
}

void CWireContainer::RenderSphere(const CMat4Dfp32& _Pos, fp32 _Radius, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWireContainer_RenderSphere, MAUTOSTRIP_VOID);

	const int nSubDiv = 16;
	CVec3Dfp32 Verts[nSubDiv * 3];

	int i, iV;
	for (i = 0, iV = 0; i < nSubDiv; i++)
	{
		fp32 QSin, QCos;
		QSinCos(_PI2 * fp32(i) / nSubDiv, QSin, QCos);
		Verts[iV++] = CVec3Dfp32(0, _Radius * QSin, _Radius * QCos) * _Pos;
		Verts[iV++] = CVec3Dfp32(_Radius * QSin, 0, _Radius * QCos) * _Pos;
		Verts[iV++] = CVec3Dfp32(_Radius * QSin, _Radius * QCos, 0) * _Pos;
	}

	for (i = 0, iV = 0; i < nSubDiv; i++, iV += 3)
	{
		if (i != nSubDiv - 1)
		{
			RenderWire(Verts[iV+0], Verts[iV+3], _Color, _Duration, _bFade);
			RenderWire(Verts[iV+1], Verts[iV+4], _Color, _Duration, _bFade);
			RenderWire(Verts[iV+2], Verts[iV+5], _Color, _Duration, _bFade);
		}
		else
		{
			RenderWire(Verts[iV+0], Verts[0], _Color, _Duration, _bFade);
			RenderWire(Verts[iV+1], Verts[1], _Color, _Duration, _bFade);
			RenderWire(Verts[iV+2], Verts[2], _Color, _Duration, _bFade);
		}
	}
}


void CWireContainer::Refresh(fp64 _dTime)
{
	MAUTOSTRIP(CWireContainer_Refresh, MAUTOSTRIP_VOID);
	CWire* pW = m_lWires.GetBasePtr();
	int nW = m_lWires.Len();

	for(int iW = 0; iW < nW; iW++)
		if (pW[iW].m_bInUse)
		{
			pW[iW].m_Age += _dTime;
			if (pW[iW].m_Age > pW[iW].m_Duration)
				FreeWire(iW);
		}
}

void CWireContainer::Render(CXR_VBManager* _pVBM, const CMat4Dfp32& _Transform)
{
	MAUTOSTRIP(CWireContainer_Render, MAUTOSTRIP_VOID);
	MSCOPE(CWireContainer::Render, WIRECONTAINER);

	CXR_VertexBuffer* pVB = _pVBM->Alloc_VBAttrib();
	CMat4Dfp32* pMat = _pVBM->Alloc_M4(_Transform);
	if (!pVB || !pMat)
		return;
	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ADD);
	pVB->m_pAttrib->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);
 	// pVB->m_pAttrib->Attrib_ZCompare(CRC_COMPARE_ALWAYS); No Z-buffer
	pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_ZCOMPARE);
	pVB->m_pTransform = pMat;

	int nWDraw = 0;

	int nW = m_lWires.Len();
	CWire* pW = m_lWires.GetBasePtr();
	int nWires = 0;

	// First cound how many wires that are to be rendered
	for(int iW = 0; iW < nW; iW++)
	{
		if(pW[iW].m_bInUse)
			nWires++;
	}
	if(nWires == 0)
		return;

	if (!pVB->AllocVBChain(_pVBM, false))
		return;
	CXR_VBChain* pChain = pVB->GetVBChain();
	pChain->m_pV	= _pVBM->Alloc_V3(nWires * 2);
	pChain->m_pCol	= (CPixel32*)_pVBM->Alloc(sizeof(CPixel32) * nWires * 2);
	pChain->m_piPrim	= (uint16*)_pVBM->Alloc(sizeof(uint16) * nWires * 2);
	pChain->m_nPrim	= nWires * 2;
	pChain->m_PrimType	= CRC_RIP_WIRES;
	pChain->m_nV	= nWires * 2;
	if(!pChain->m_pV || !pChain->m_pCol || !pChain->m_piPrim)
		return;
	for(int iW = 0; iW < nW; iW++)
	{
		const CWire& W = pW[iW];
		if (W.m_bInUse)
		{
			int iVSrc = iW*2;
			int iVDst = nWDraw*2;

			pChain->m_pV[iVDst] = m_lV[iVSrc];
			pChain->m_pV[iVDst+1] = m_lV[iVSrc+1];
			if(W.m_bFade)
				pChain->m_pCol[iVDst] = CPixel32::LinearRGBA(m_lCol[iVSrc], CPixel32(0), RoundToInt(Clamp01(W.m_Age / W.m_Duration)*255.0f));
			else
				pChain->m_pCol[iVDst] = m_lCol[iVSrc];

			pChain->m_pCol[iVDst+1] = pChain->m_pCol[iVDst];
			pChain->m_piPrim[iVDst] = iVDst;
			pChain->m_piPrim[iVDst+1] = iVDst+1;
			nWDraw++;
		}
	}

	pVB->m_Priority	= 100000000;
	_pVBM->AddVB(pVB);
}


/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯| 
| CDebugRenderContainer							  
|____________________________________________________________________________________|
\************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CDebugRenderContainer, CWireContainer);
IMPLEMENT_OPERATOR_NEW(CDebugRenderContainer);

M_INLINE CVec4Dfp32 MakePoint4(const CVec3Dfp32& _v3) { return CVec4Dfp32(_v3.k[0], _v3.k[1], _v3.k[2], 1.0f); }
M_INLINE CVec3Dfp32 MakeVec3(const CVec4Dfp32& _v4)   { return CVec3Dfp32(_v4.k[0], _v4.k[1], _v4.k[2]); }

void CDebugRenderContainer::Create(int _MaxWires, int _MaxText, CRC_Font* _pFont)
{
	CWireContainer::Create(_MaxWires);

	m_lText.SetLen(_MaxText);
	m_IDTHeap.Create(_MaxText);
	m_iTNext = 0;
	m_pFont = _pFont;
}

void CDebugRenderContainer::RenderText(const CVec3Dfp32& _Pos, const char* _pText, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	if (!m_pFont)
		return;

	// Alloc entry
	int iT = m_IDTHeap.AllocID();
	if (iT < 0) 
		iT = (m_iTNext + 1) % m_lText.Len();
	m_iTNext = iT;

	// Set params
	CText& Entry = m_lText[iT];
	Entry.Clear();
	Entry.m_bInUse = 1;
	Entry.m_bFade = _bFade;
	Entry.m_Duration = _Duration;
	Entry.m_Age = 0.0f;
	Entry.m_Pos = _Pos;
	Entry.m_Col = _Color;
	Entry.m_Str = _pText;
}

void CDebugRenderContainer::Refresh(fp64 _dTime)
{
	MSCOPESHORT(CDebugRenderContainer::Refresh);

	CWireContainer::Refresh(_dTime);

	TAP<CText> pText = m_lText;
	for(int iT = 0; iT < pText.Len(); iT++)
		if (pText[iT].m_bInUse)
		{
			pText[iT].m_Age += _dTime;
			if (pText[iT].m_Age > pText[iT].m_Duration)
			{ // Free entry
				m_IDTHeap.FreeID(iT);
				pText[iT].Clear();
			}
		}
}

void CDebugRenderContainer::Render(CXR_VBManager* _pVBM, const CMat4Dfp32& _Transform)
{
	MSCOPE(CDebugRenderContainer::Render, WIRECONTAINER);

	// Render wires
	CWireContainer::Render(_pVBM, _Transform);

	// Render text (2D text width 3D position)
	if (!m_pFont)
		return;

	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	CMat4Dfp32* pMat = _pVBM->Alloc_M4(_Transform);

	if (!pA || !pMat) return;
	pA->SetDefault();
	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
	pA->Attrib_TextureID(0, m_pFont->m_spTC->GetTextureID(0));

	fp32 InvBackPlane = 1.0f / _pVBM->Viewport_Get()->GetBackPlane();
	fp32 Width = 0.5f * _pVBM->Viewport_Get()->GetViewRect().GetWidth();
	fp32 Height = 0.5f * _pVBM->Viewport_Get()->GetViewRect().GetHeight();
	CVec2Dfp32 Size(m_pFont->GetOriginalSize() / Width, m_pFont->GetOriginalSize() / Height);

	CMat4Dfp32 ProjMat, InvProjMat;
	_Transform.Multiply(_pVBM->Viewport_Get()->GetProjectionMatrix(), ProjMat);
	ProjMat.Inverse(InvProjMat);
	CVec3Dfp32 Dir = MakeVec3(CVec4Dfp32(Width, 0.0f, 0.0f, 0.0f) * InvProjMat);
	CVec3Dfp32 Down = MakeVec3(CVec4Dfp32(0.0f, -Height, 0.0f, 0.0f) * InvProjMat);

	TAP<const CText> pText = m_lText;
	for(int iT = 0; iT < pText.Len(); iT++)
	{
		const CText& T = pText[iT];
		if (T.m_bInUse)
		{
			CVec3Dfp32 Pos = T.m_Pos;
			CVec4Dfp32 PosH = MakePoint4(Pos) * ProjMat;
			fp32 w = PosH.k[3];
			PosH.k[0] = int(PosH.k[0] / w) * w;		// Snap to integer coordinates
			PosH.k[1] = int(PosH.k[1] / w) * w;		//  (to prevent ugly texture interpolation)
			Pos = MakeVec3(PosH * InvProjMat);
			CVec2Dfp32 TextSize = Size * w;

			CPixel32 Color = T.m_Col;
			if (T.m_bFade)
				Color.A() -= RoundToInt(Color.A() * Clamp01(T.m_Age / T.m_Duration));

			CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
			if (!pVB) return;
			m_pFont->Write(_pVBM, pVB, Pos, Dir, Down, T.m_Str.Str(), TextSize, Color);
			pVB->Matrix_Set(pMat);
			pVB->m_pAttrib = pA;
			pVB->m_Priority = CXR_VBPRIORITY_WINDOWS + 1.0f - w * InvBackPlane;
			_pVBM->AddVB(pVB);
		}
	}
}
