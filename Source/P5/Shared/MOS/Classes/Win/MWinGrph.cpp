#include "PCH.h"
#include "MWinGrph.h"
#include "../../XR/XR.h"
#include "../../MSystem/Misc/MLocalizer.h"
#include "../../Classes/Win/MWindows.h"
#include "../../XR/XRShader.h"

#ifdef COMPILER_CODEWARRIOR
//	#ifdef COMPILER_RTTI
		#include <typeinfo.h>
//	#endif
#endif

uint32 CRC_Util2D::ms_IndexRamp[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
uint16 CRC_Util2D::ms_DualTringle[6] = { 0,1,2,0,2,3 };

// -------------------------------------------------------------------
void CRC_Util2D::Clear()
{
	MAUTOSTRIP(CRC_Util2D_Clear, MAUTOSTRIP_VOID);
	m_pCurRC = NULL;
	m_pVBM = NULL;
	m_pSurf = NULL;
	m_pSurfKeyFrame = NULL;
	m_spTmpSurfKey = NULL;
//	m_pCurAttrib = 0;
//	m_pCurAttrib = NULL;
//	m_CurTextureID = 0;
	m_pLastSubmittedAttrib	= 0;
	m_TextOffset = 0.0f;
	m_CurTxtW = 1;
	m_CurTxtH = 1;
	m_CurTransform.Unit();
	m_CurTextureOrigo = CPnt(0, 0);
	m_CurTextureScale.SetScalar(1.0f);
	m_CurScale.SetScalar(1.0f);
	m_CurFontScale.SetScalar(1.0f);
	m_VBPriority = 0;
//	m_spWorldLightState = NULL;
	m_SurfOptions = 0;
	m_SurfCaps = 0;
	SetPriorityValues(0);
}


void CRC_Util2D::SetPriorityValues(fp32 _PriorityBase, fp32 _PriorityAdd)
{
	MAUTOSTRIP(CRC_Util2D_SetPriorityValues, MAUTOSTRIP_VOID);
	m_VBPriorityAdd = _PriorityAdd;
	m_VBPriorityBase = _PriorityBase;
}


CRC_Util2D::CRC_Util2D()
{
	MAUTOSTRIP(CRC_Util2D_ctor, MAUTOSTRIP_VOID);
	m_CurScale = 1.0f;
	m_CurFontScale = 1.0f;
//	m_bAutoFlush = false;
	Clear();
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("CRC_Util2D", "No texture-context available.");
	m_pTC = pTC;

	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
//	if (!pSC) Error("CRC_Util2D", "No surface-context available.");
	m_pSC = pSC;
}


CRC_Util2D::~CRC_Util2D()
{
	MAUTOSTRIP(CRC_Util2D_dtor, MAUTOSTRIP_VOID);
	Clear();
}

void CRC_Util2D::Flush()
{
//	m_pVBM->Flush(m_pCurRC, 0);
}

// -------------------------------------------------------------------
void CRC_Util2D::UpdateTransform()
{
	MAUTOSTRIP(CRC_Util2D_UpdateTransform, MAUTOSTRIP_VOID);
	CRct View = m_CurVP.GetViewRect();
	CClipRect Clip = m_CurVP.GetViewClip();
//	fp32 cw = Clip.clip.GetWidth();
//	fp32 ch = Clip.clip.GetHeight();
	fp32 w = View.GetWidth();
	fp32 h = View.GetHeight();
	fp32 dx = -w;
	fp32 dy = -h;

	m_CurTransform.Unit();
//	CVec3Dfp32::GetMatrixRow(m_CurTransform, 3) = CVec3Dfp32(dx/2.0f, dy/2.0f, _pVP->GetXScale()*0.5f);
//	const fp32 Z = 16.0f;
	const fp32 Z = m_CurVP.GetBackPlane() - 16.0f;

	fp32 xScale = m_CurVP.GetXScale() * 0.5f;
	fp32 yScale = m_CurVP.GetYScale() * 0.5f;

	m_CurTransform.k[0][0] = Z / xScale * m_CurScale.k[0];
	m_CurTransform.k[1][1] = Z / yScale * m_CurScale.k[1];
	m_CurTransform.k[3][0] = (Z*((dx)*(1.0f/2.0f) - m_TextOffset) / xScale);
	m_CurTransform.k[3][1] = (Z*((dy)*(1.0f/2.0f) - m_TextOffset) / yScale);
	m_CurTransform.k[3][2] = Z;

//	CVec3Dfp32::GetMatrixRow(m_CurTransform, 3) = CVec3Dfp32(dx/2.0f, dy/2.0f, _pVP->GetXScale()*0.5f);

//	m_CurTransform.k[2][0] += 0.375f;
//	m_CurTransform.k[2][1] += 0.375f;
	
//	m_CurTransform.k[2][0] += 10;
//	m_CurTransform.k[2][1] += 10;
}


void CRC_Util2D::Begin(CRenderContext* _pRC, CRC_Viewport* _pVP, CXR_VBManager* _pVBM)
{
	MAUTOSTRIP(CRC_Util2D_Begin, MAUTOSTRIP_VOID);
	Clear();

	if (!_pVBM)
	{
		M_TRACEALWAYS("(CRC_Util2D::Begin) _pVBM == NULL!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	}


	m_pCurRC = _pRC;
	m_TextOffset = 0;
	m_pVBM = _pVBM;
	m_CurVP = *_pVP;
	m_VBPriority = m_VBPriorityBase;

	m_SurfCaps = 0;
	if(_pRC)
	{
		int nMultiTex = _pRC->Attrib_GlobalGetVar(CRC_GLOBALVAR_NUMTEXTURES);
		m_SurfCaps |= (nMultiTex >= 2) ? XW_SURFREQ_MULTITEX2 : 0;
		m_SurfCaps |= (nMultiTex >= 3) ? XW_SURFREQ_MULTITEX3 : 0;
		m_SurfCaps |= (nMultiTex >= 4) ? XW_SURFREQ_MULTITEX4 : 0;
	}

	m_Attrib.SetDefault();

	UpdateTransform();
}


void CRC_Util2D::End()
{
	MAUTOSTRIP(CRC_Util2D_End, MAUTOSTRIP_VOID);
	Clear();
}


void CRC_Util2D::SetCoordinateScale(const CVec2Dfp32& _Scale)
{
	MAUTOSTRIP(CRC_Util2D_SetCoordinateScale, MAUTOSTRIP_VOID);
	m_CurScale = _Scale;
	UpdateTransform();
}
/*
void CRC_Util2D::SetWorldLightState(spCXR_WorldLightState _spLightState)
{
	MAUTOSTRIP(CRC_Util2D_SetWorldLightState, MAUTOSTRIP_VOID);
	m_spWorldLightState = _spLightState;
}
*/

void CRC_Util2D::SetTexture(int _TextureID)
{
	MAUTOSTRIP(CRC_Util2D_SetTexture, MAUTOSTRIP_VOID);
	CImage Desc;
	int nMipMaps = 0;
	if (_TextureID)
	{
		m_pTC->GetTextureDesc(_TextureID, &Desc, nMipMaps);
		m_CurTxtW = Max(1, Desc.GetWidth());
		m_CurTxtH = Max(1, Desc.GetHeight());
	}
	else
	{
		m_CurTxtW = 1;
		m_CurTxtH = 1;
	}

	m_Attrib.Attrib_TextureID(0, _TextureID);
	m_pLastSubmittedAttrib	= NULL;

//	m_CurTextureID = _TextureID;
//	m_pCurAttrib = NULL;
	m_pSurf = NULL;
	m_pSurfKeyFrame = NULL;
}


void CRC_Util2D::SetTransform(const CMat4Dfp32& _Pos, const CVec2Dfp32& _Scale)
{
	MAUTOSTRIP(CRC_Util2D_SetTransform, MAUTOSTRIP_VOID);
	// TODO
}

void CRC_Util2D::SetAttrib(CRC_Attributes* _pAttrib)
{
	m_Attrib	= *_pAttrib;
	m_pLastSubmittedAttrib	= 0;
//	m_CurTextureID = 0;
//	m_pCurAttrib = _pAttrib;
	m_pSurf = NULL;
	m_pSurfKeyFrame = NULL;
}

void CRC_Util2D::SetSurfaceOptions(int _Options)
{
	MAUTOSTRIP(CRC_Util2D_SetSurfaceOptions, MAUTOSTRIP_VOID);
	m_SurfOptions = _Options;
}


void CRC_Util2D::SetSurface(const char *_pSurfName, CMTime _AnimTime, int _Sequence)
{
	MAUTOSTRIP(CRC_Util2D_SetSurface, MAUTOSTRIP_VOID);
	if (!m_pSC)
		Error("SetSurface", "No surface-context available.");
	SetSurface(m_pSC->GetSurfaceID(_pSurfName), _AnimTime, _Sequence);
}


void CRC_Util2D::SetSurface(int _SurfaceID, CMTime _AnimTime, int _Sequence)
{
	MAUTOSTRIP(CRC_Util2D_SetSurface_2, MAUTOSTRIP_VOID);
	if (!m_pSC)
		Error("SetSurface", "No surface-context available.");
	SetSurface(m_pSC->GetSurfaceVersion(_SurfaceID, m_SurfOptions, m_SurfCaps), _AnimTime, _Sequence);
}


void CRC_Util2D::SetSurface(class CXW_Surface *_pSurf, CMTime _AnimTime, int _Sequence)
{
	MAUTOSTRIP(CRC_Util2D_SetSurface_3, MAUTOSTRIP_VOID);
	if (!m_spTmpSurfKey)
	{
		MRTC_SAFECREATEOBJECT_NOEX(spSKF, "CXW_SurfaceKeyFrame", CXW_SurfaceKeyFrame);
		if (!spSKF) Error("OnCreate", "No CXW_SurfaceKeyFrame available.");
		m_spTmpSurfKey = spSKF;
	}

	m_pSurf = _pSurf;
	m_pSurfKeyFrame = _pSurf->GetFrame(_Sequence, _AnimTime, m_spTmpSurfKey);
	m_SurfTime = _AnimTime;
//	m_pCurAttrib = NULL;
	m_pLastSubmittedAttrib	= 0;

	CVec3Dfp32 ReferenceDim = _pSurf->GetTextureMappingReferenceDimensions(m_pSurfKeyFrame);
	m_CurTxtW = int(ReferenceDim[0]);
	m_CurTxtH = int(ReferenceDim[1]);
}


bool CRC_Util2D::SetTexture(const char* _pTxtName)
{
	MAUTOSTRIP(CRC_Util2D_SetTexture_2, false);
	SetTexture(m_pTC->GetTextureID(_pTxtName));
	return m_Attrib.m_TextureID[0] != 0;
}


void CRC_Util2D::SetTextureOrigo(const CClipRect& _Clip, const CPnt& _Origo)
{
	MAUTOSTRIP(CRC_Util2D_SetTextureOrigo, MAUTOSTRIP_VOID);
	m_CurTextureOrigo = _Origo + _Clip.ofs;
}


void CRC_Util2D::SetTextureScale(fp32 _xScale, fp32 _yScale)
{
	MAUTOSTRIP(CRC_Util2D_SetTextureScale, MAUTOSTRIP_VOID);
	m_CurTextureScale.k[0] = _xScale;
	m_CurTextureScale.k[1] = _yScale;
}


void CRC_Util2D::SetFontScale(fp32 _xScale, fp32 _yScale)
{
	m_CurFontScale[0] = _xScale;
	m_CurFontScale[1] = _yScale;
}

void CRC_Util2D::Rects(const CClipRect& _Clip, const CRct *_Rect, const CPixel32 *_Color, int _nRects)
{
	for (int i = 0; i < _nRects; ++i)
	{
		Rect(_Clip, _Rect[i], _Color[i]);
	}
}

void CRC_Util2D::Rect(const CClipRect& _Clip, const CRct& _Rect, const CPixel32& _Color, CXR_Engine *_pEngine)
{
	MAUTOSTRIP(CRC_Util2D_Rect, MAUTOSTRIP_VOID);
	if (!_Clip.Visible(_Rect)) return;
	CRct Rect = _Rect;
	Rect += _Clip;

//	if (Rect.p0.x < _Clip.clip.p0.x) Rect.p0.x = _Clip.clip.p0.x;
//	if (Rect.p1.x > _Clip.clip.p1.x) Rect.p1.x = _Clip.clip.p1.x;
//	if (Rect.p0.y < _Clip.clip.p0.y) Rect.p0.y = _Clip.clip.p0.y;
//	if (Rect.p1.y > _Clip.clip.p1.y) Rect.p1.y = _Clip.clip.p1.y;

	Rect.p0.x = Max( Rect.p0.x, _Clip.clip.p0.x );
	Rect.p1.x = Min( Rect.p1.x, _Clip.clip.p1.x );
	Rect.p0.y = Max( Rect.p0.y, _Clip.clip.p0.y );
	Rect.p1.y = Min( Rect.p1.y, _Clip.clip.p1.y );

//	CVec3Dfp32 Verts[4];
	CVec3Dfp32 Verts3D[4];
	CVec2Dfp32 TVerts[4];
	CPixel32 Colors[4];
	CVec3Dfp32 *pV = Verts3D;
	CVec2Dfp32 *pTV = TVerts;
	CPixel32 *pC = Colors;
	if(m_pVBM)
	{
		pV = m_pVBM->Alloc_V3(4);
		pTV = m_pVBM->Alloc_V2(4);
		pC = m_pVBM->Alloc_CPixel32(4);
	}

	if(!pV || !pTV || !pC)
		return;

	pV[0].k[0] = Rect.p0.x;		pV[0].k[1] = Rect.p0.y;		pV[0].k[2] = 0;
	pV[1].k[0] = Rect.p1.x;		pV[1].k[1] = Rect.p0.y;		pV[1].k[2] = 0;
	pV[2].k[0] = Rect.p1.x;		pV[2].k[1] = Rect.p1.y;		pV[2].k[2] = 0;
	pV[3].k[0] = Rect.p0.x;		pV[3].k[1] = Rect.p1.y;		pV[3].k[2] = 0;
	CVec3Dfp32::MultiplyMatrix(pV, pV, m_CurTransform, 4);

	fp32 xs = 1.0f/fp32(m_CurTxtW) * m_CurTextureScale.k[0];
	fp32 ys = 1.0f/fp32(m_CurTxtH) * m_CurTextureScale.k[1];
	int txtox = m_CurTextureOrigo.x;
	int txtoy = m_CurTextureOrigo.y;
	pTV[0][0] = fp32(Rect.p0.x - txtox) * xs;
	pTV[0][1] = fp32(Rect.p0.y - txtoy) * ys;
	pTV[1][0] = fp32(Rect.p1.x - txtox) * xs;
	pTV[1][1] = fp32(Rect.p0.y - txtoy) * ys;
	pTV[2][0] = fp32(Rect.p1.x - txtox) * xs;
	pTV[2][1] = fp32(Rect.p1.y - txtoy) * ys;
	pTV[3][0] = fp32(Rect.p0.x - txtox) * xs;
	pTV[3][1] = fp32(Rect.p1.y - txtoy) * ys;
	pC[0] = _Color;
	pC[1] = _Color;
	pC[2] = _Color;
	pC[3] = _Color;

	if(m_pVBM)
	{
		CXR_VertexBuffer *pVB = m_pVBM->Alloc_VB();
		if(pVB)
		{
			pVB->m_Priority = (m_VBPriority += m_VBPriorityAdd);
			if (!pVB->AllocVBChain(m_pVBM, false))
				return;
			pVB->Geometry_VertexArray(pV, 4, true);
			pVB->Geometry_TVertexArray(pTV, 0);
			pVB->Render_IndexedTriangles(ms_DualTringle, 2);

			if(m_pSurf)
			{
				pVB->Geometry_ColorArray(pC);
				CXR_Util::Render_Surface(0, m_SurfTime, m_pSurf, m_pSurfKeyFrame, _pEngine, m_pVBM, (CMat4Dfp32*) NULL, (CMat4Dfp32*) NULL, (CMat4Dfp32*)NULL, pVB, m_VBPriority, 0.0001f);
			}
			else
			{
				pVB->Geometry_Color(_Color);
				pVB->m_pAttrib	= GetLastSubmitted();;

				if(pVB->m_pAttrib)
				{
					m_pVBM->AddVB(pVB);
//					if(m_bAutoFlush)
//						m_pVBM->Flush(m_pCurRC, 0);
				}
			}
		}
	}
	else
	{
		m_pCurRC->Attrib_TextureID(0, m_Attrib.m_TextureID[0]);

		m_pCurRC->Geometry_Clear();
		m_pCurRC->Geometry_VertexArray(pV, 4);
		m_pCurRC->Geometry_TVertexArray(pTV, 0);
		m_pCurRC->Geometry_Color(_Color);
//		m_pCurRC->Geometry_ColorArray(pC);
		m_pCurRC->Render_IndexedTriangles(ms_DualTringle, 2);
		m_pCurRC->Geometry_Clear();

//		m_pCurRC->Render_Polygon(4, pV, pTV, &ms_IndexRamp[0], &ms_IndexRamp[0], _Color);
	}
}

void CRC_Util2D::Rect(const CClipRect& _Clip, const CRct& _Rect, const CPixel32& _Color, class CXR_Shader* _pShader, const class CXR_ShaderParams* _pShaderParams, const class CXR_SurfaceShaderParams* _pSurfParams, const CXR_Light& _Light)
{
	MAUTOSTRIP(CRC_Util2D_Rect, MAUTOSTRIP_VOID);
	if (!_Clip.Visible(_Rect))
		return;
	if (!m_pVBM)
		return;
	CRct Rect = _Rect;
	Rect += _Clip;

//	if (Rect.p0.x < _Clip.clip.p0.x) Rect.p0.x = _Clip.clip.p0.x;
//	if (Rect.p1.x > _Clip.clip.p1.x) Rect.p1.x = _Clip.clip.p1.x;
//	if (Rect.p0.y < _Clip.clip.p0.y) Rect.p0.y = _Clip.clip.p0.y;
//	if (Rect.p1.y > _Clip.clip.p1.y) Rect.p1.y = _Clip.clip.p1.y;

	Rect.p0.x = Max( Rect.p0.x, _Clip.clip.p0.x );
	Rect.p1.x = Min( Rect.p1.x, _Clip.clip.p1.x );
	Rect.p0.y = Max( Rect.p0.y, _Clip.clip.p0.y );
	Rect.p1.y = Min( Rect.p1.y, _Clip.clip.p1.y );

	CVec3Dfp32 *pV = m_pVBM->Alloc_V3(4);
	CVec3Dfp32 *pN = m_pVBM->Alloc_V3(4);
	CVec2Dfp32 *pTV = m_pVBM->Alloc_V2(4);
	CVec3Dfp32 *pTangU = m_pVBM->Alloc_V3(4);
	CVec3Dfp32 *pTangV = m_pVBM->Alloc_V3(4);
	CPixel32 *pC = m_pVBM->Alloc_CPixel32(4);

	if(!pV || !pN || !pTV || !pC || !pTangU || !pTangV)
		return;

	pV[0].k[0] = Rect.p0.x;		pV[0].k[1] = Rect.p0.y;		pV[0].k[2] = 0;
	pV[1].k[0] = Rect.p1.x;		pV[1].k[1] = Rect.p0.y;		pV[1].k[2] = 0;
	pV[2].k[0] = Rect.p1.x;		pV[2].k[1] = Rect.p1.y;		pV[2].k[2] = 0;
	pV[3].k[0] = Rect.p0.x;		pV[3].k[1] = Rect.p1.y;		pV[3].k[2] = 0;
	CVec3Dfp32::MultiplyMatrix(pV, pV, m_CurTransform, 4);

	fp32 xs = 1.0f/fp32(m_CurTxtW) * m_CurTextureScale.k[0];
	fp32 ys = 1.0f/fp32(m_CurTxtH) * m_CurTextureScale.k[1];
	int txtox = m_CurTextureOrigo.x;
	int txtoy = m_CurTextureOrigo.y;
	pTV[0][0] = fp32(Rect.p0.x - txtox) * xs;
	pTV[0][1] = fp32(Rect.p0.y - txtoy) * ys;
	pTV[1][0] = fp32(Rect.p1.x - txtox) * xs;
	pTV[1][1] = fp32(Rect.p0.y - txtoy) * ys;
	pTV[2][0] = fp32(Rect.p1.x - txtox) * xs;
	pTV[2][1] = fp32(Rect.p1.y - txtoy) * ys;
	pTV[3][0] = fp32(Rect.p0.x - txtox) * xs;
	pTV[3][1] = fp32(Rect.p1.y - txtoy) * ys;
	pC[0] = _Color;
	pC[1] = _Color;
	pC[2] = _Color;
	pC[3] = _Color;

	CVec3Dfp32 Normal(0,0,-1);
	CVec3Dfp32 TangU(Sign(m_CurTextureScale.k[0]),0,0);
	CVec3Dfp32 TangV(0,Sign(m_CurTextureScale.k[1]),0);

	for(int i = 0; i < 4; i++)
	{
		pTangU[i] = TangU;
		pTangV[i] = TangV;
		pN[i] = Normal;
	}

	CMat4Dfp32* pMat = m_pVBM->Alloc_M4();

	CXR_VertexBuffer *pVB = m_pVBM->Alloc_VB();
	if(pVB && pMat)
	{
		pMat->Unit();
		if (!pVB->AllocVBChain(m_pVBM, false))
			return;
		pVB->Matrix_Set(pMat);
		pVB->Geometry_VertexArray(pV, 4, true);
		pVB->Geometry_Normal(pN);
		pVB->Geometry_TVertexArray(pTV, 0);
		pVB->Geometry_TVertexArray(pTangU, 1);
		pVB->Geometry_TVertexArray(pTangV, 2);
		pVB->Render_IndexedTriangles(ms_DualTringle, 2);

		const CXR_SurfaceShaderParams* lpSSP[1] = { _pSurfParams };
		_pShader->RenderShading(_Light, pVB, _pShaderParams, lpSSP);
	}
}

#ifndef M_RTM
void CRC_Util2D::Circle(const CClipRect& _Clip, const CVec2Dfp32& _Mid, fp32 _Radius, int32 _nSegments, const CPixel32& _Color, bool _bBorder, const CPixel32& _BorderColor)
{
	MAUTOSTRIP(CRC_Util2D_Rect, MAUTOSTRIP_VOID);
	if (!m_pVBM)
		return;

	// Num segs plus middle point
	int32 NumPoints = _nSegments + 1;

	CVec3Dfp32 *pV = m_pVBM->Alloc_V3(NumPoints);
	/*CVec3Dfp32 *pN = m_pVBM->Alloc_V3(NumPoints);
	CVec2Dfp32 *pTV = m_pVBM->Alloc_V2(NumPoints);
	CVec3Dfp32 *pTangU = m_pVBM->Alloc_V3(NumPoints);
	CVec3Dfp32 *pTangV = m_pVBM->Alloc_V3(NumPoints);*/
	uint16* pIndex = m_pVBM->Alloc_Int16(_nSegments * 3);
	CPixel32 *pC = m_pVBM->Alloc_CPixel32(NumPoints);

	if(!pV || !pIndex || !pC)
		return;

	// Calc circle points (yehyeh, this is a debug function anyway...)
	pV[0].k[0] = _Mid[0];
	pV[0].k[1] = _Mid[1];
	pV[0].k[2] = 0.0f;
	pC[0] = _Color;

	fp32 Part = 2*_PI / (fp32)_nSegments;
	for (int32 i = 0; i < _nSegments; i++)
	{
		pC[i+1] = _Color;
		pV[i + 1].k[0] = _Mid[0] + M_Sin(Part * i) * _Radius;
		pV[i + 1].k[1] = _Mid[1] + M_Cos(Part * i) * _Radius;
		pV[i + 1].k[2] = 0.0f;
	}
	for (int32 i = 0; i < _nSegments; i++)
	{
		pIndex[i * 3] = 0;
		pIndex[i * 3+1] = i+1;
		if (i == (_nSegments - 1))
			pIndex[i * 3+2] = 1;
		else
			pIndex[i * 3+2] = i+2;
	}

	CVec3Dfp32::MultiplyMatrix(pV, pV, m_CurTransform, NumPoints);

	CMat4Dfp32* pMat = m_pVBM->Alloc_M4();

	CXR_VertexBuffer *pVB = m_pVBM->Alloc_VB();
	if(pVB && pMat)
	{
		pMat->Unit();
		if (!pVB->AllocVBChain(m_pVBM, false))
			return;
		pVB->Matrix_Set(pMat);
		pVB->Geometry_VertexArray(pV, NumPoints, true);
		pVB->Geometry_Color(_Color);
		pVB->Render_IndexedTriangles(pIndex, _nSegments);
		pVB->m_pAttrib	= GetLastSubmitted();
		if(pVB->m_pAttrib)
			m_pVBM->AddVB(pVB);

		//_pShader->RenderShading(_Light, pVB, _pShaderParams);
	}

	if (_bBorder)
	{
		for (int32 i = 0; i < _nSegments; i++)
		{
			m_pCurRC->Render_Wire(pV[i], pV[i + 1], _BorderColor);
		}
	}
}

void CRC_Util2D::Line(const CClipRect& _Clip, const CVec2Dfp32& _Start, CVec2Dfp32& _End, fp32 _Width, const CPixel32& _Color)
{
	if (!m_pVBM)
		return;

	// Draw a line from startpoint to endpoint

	// Num segs plus middle point

	CVec3Dfp32 *pV = m_pVBM->Alloc_V3(4);
	CPixel32 *pC = m_pVBM->Alloc_CPixel32(4);

	if(!pV || !pC)
		return;

	CVec2Dfp32 Dir = _End - _Start;
	Dir.Normalize();
	CVec2Dfp32 Right;
	Right.k[0] = Dir.k[1];
	Right.k[1] = -Dir.k[0];

	// Set color
	pC[0] = _Color;
	pC[1] = _Color;
	pC[2] = _Color;
	pC[3] = _Color;

	CVec2Dfp32 Point =  _Start - Right * _Width * 0.5f;
	pV[0].k[0] = Point.k[0];
	pV[0].k[1] = Point.k[1];
	pV[0].k[2] = 0.0f;

	Point =  _End - Right * _Width * 0.5f;
	pV[1].k[0] = Point.k[0];
	pV[1].k[1] = Point.k[1];
	pV[1].k[2] = 0.0f;

	Point =  _End + Right * _Width * 0.5f;
	pV[2].k[0] = Point.k[0];
	pV[2].k[1] = Point.k[1];
	pV[2].k[2] = 0.0f;

	Point =  _Start + Right * _Width * 0.5f;
	pV[3].k[0] = Point.k[0];
	pV[3].k[1] = Point.k[1];
	pV[3].k[2] = 0.0f;
	CVec3Dfp32::MultiplyMatrix(pV, pV, m_CurTransform, 4);

	CMat4Dfp32* pMat = m_pVBM->Alloc_M4();

	CXR_VertexBuffer *pVB = m_pVBM->Alloc_VB();
	if(pVB && pMat)
	{
		pMat->Unit();
		if (!pVB->AllocVBChain(m_pVBM, false))
			return;
		pVB->Matrix_Set(pMat);
		pVB->Geometry_VertexArray(pV, 4, true);
		pVB->Geometry_Color(_Color);
		pVB->Render_IndexedTriangles(ms_DualTringle, 2);
		pVB->m_pAttrib	= GetLastSubmitted();
		if(pVB->m_pAttrib)
			m_pVBM->AddVB(pVB);

		//_pShader->RenderShading(_Light, pVB, _pShaderParams);
	}
}
#endif

void CRC_Util2D::AspectRect(const CClipRect& _Clip, const CRct& _Rect, const CPnt& _SourceSize, fp32 _SourcePixelAspect, const CPixel32& _Color)
{
	int Width = _Rect.p1.x - _Rect.p0.x;
	int Height = _Rect.p1.y - _Rect.p0.y;
	fp32 OffsetX = 0;
	fp32 OffsetY = 0;

	fp32 PixelAspect = (GetRC()->GetDC()->GetPixelAspect() * GetRC()->GetDC()->GetScreenAspect() / (((fp32)Width / Height)*(3.0/4.0))) / _SourcePixelAspect;
	fp32 AspectX = PixelAspect * fp32(Width) / _SourceSize.x;
	fp32 AspectY = fp32(Height) / _SourceSize.y;
	if(AspectX < AspectY)
	{
		// Horizontal borders
		OffsetY = (Height - AspectX / AspectY * Height) / 2;
		fp32 Scale = _SourceSize.x / (fp32) Width;
		SetTextureScale(Scale, Scale/PixelAspect);
	}
	else
	{
		// Vertical borders
		OffsetX = (Width - AspectY / AspectX * Width) / 2;
		fp32 Scale = _SourceSize.y / (fp32) Height;
		SetTextureScale(Scale* PixelAspect, Scale);
	}

	SetTextureOrigo(_Clip, CPnt(int(_Rect.p0.x + OffsetX), int(_Rect.p0.y + OffsetY)));
	Rect(_Clip, CRct::From_fp32(_Rect.p0.x + OffsetX, _Rect.p0.y + OffsetY, _Rect.p1.x - OffsetX, _Rect.p1.y - OffsetY),  _Color);
}

bool CRC_Util2D::DrawTexture(const CClipRect& _Clip, const CPnt& _Pos, const char *_pTexture, const CPixel32 _Color, const CVec2Dfp32& _Scale)
{
	if(!SetTexture(_pTexture))
		return false;
	
	SetTextureOrigo(_Clip, _Pos);
	SetTextureScale(_Scale[0], _Scale[1]);
	Rect(_Clip, CRct::From_fp32(_Pos.x, _Pos.y, _Pos.x + GetTextureWidth() / _Scale[0], _Pos.y + GetTextureHeight() / _Scale[1]), _Color);
	return true;
}

void CRC_Util2D::DrawSurface(const CClipRect& _Clip, const CPnt& _Pos, const char *_pSurface, const CPixel32 _Color, const CVec2Dfp32& _Scale)
{
	Error("DrawSurface", "TODO");
}

void CRC_Util2D::Rect3D(const CClipRect& _Clip, const CRct& _Rect, const CPixel32& _ColorH, const CPixel32& _ColorM, const CPixel32& _ColorD)
{
	MAUTOSTRIP(CRC_Util2D_Rect3D, MAUTOSTRIP_VOID);
	Rect(_Clip, CRct(_Rect.p0.x+1, _Rect.p0.y+1, _Rect.p1.x-1, _Rect.p1.y-1), _ColorM);
	Rect(_Clip, CRct(_Rect.p0.x, _Rect.p0.y, _Rect.p0.x+1, _Rect.p1.y-1), _ColorH);
	Rect(_Clip, CRct(_Rect.p0.x, _Rect.p0.y, _Rect.p1.x-1, _Rect.p0.y+1), _ColorH);
	Rect(_Clip, CRct(_Rect.p1.x-1, _Rect.p0.y, _Rect.p1.x, _Rect.p1.y-1), _ColorD);
	Rect(_Clip, CRct(_Rect.p0.x, _Rect.p1.y-1, _Rect.p1.x, _Rect.p1.y), _ColorD);
}


void CRC_Util2D::Pixel(const CClipRect& _Clip, const CPnt& _Pos, const CPixel32& _Color)
{
	MAUTOSTRIP(CRC_Util2D_Pixel, MAUTOSTRIP_VOID);
	Rect(_Clip, CRct(_Pos, CPnt(_Pos.x+1, _Pos.y+1)), _Color);
}


void CRC_Util2D::Sprite(const CClipRect& _Clip, const CPnt& _Pos, const CPixel32& _Color)
{
	MAUTOSTRIP(CRC_Util2D_Sprite, MAUTOSTRIP_VOID);
	if (m_Attrib.m_TextureID[0] == 0) return;
//	GetRC()->Attrib_Push();

//	GetRC()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	SetTextureOrigo(_Clip, _Pos);
	Rect(_Clip, CRct(_Pos, CPnt(m_CurTxtW, m_CurTxtH) + _Pos), _Color);

//	GetRC()->Attrib_Pop();
}

void CRC_Util2D::RotatedSprite(const CClipRect& _Clip, const CPnt& _Pos, const CPnt& _Range,  fp32 _Angle, const CPixel32& _Color)
{
	// Return if there are no texture set
	if (m_Attrib.m_TextureID[0] == 0) return;

	CRct Rect = CRct(_Pos.x - _Range.x/2, _Pos.y - _Range.y/2, _Pos.x + _Range.x/2, _Pos.y + _Range.y/2);
	if (!_Clip.Visible(Rect)) return;
	Rect += _Clip;

//	if (Rect.p0.x < _Clip.clip.p0.x) Rect.p0.x = _Clip.clip.p0.x;
//	if (Rect.p1.x > _Clip.clip.p1.x) Rect.p1.x = _Clip.clip.p1.x;
//	if (Rect.p0.y < _Clip.clip.p0.y) Rect.p0.y = _Clip.clip.p0.y;
//	if (Rect.p1.y > _Clip.clip.p1.y) Rect.p1.y = _Clip.clip.p1.y;

	Rect.p0.x = Max( Rect.p0.x, _Clip.clip.p0.x );
	Rect.p1.x = Min( Rect.p1.x, _Clip.clip.p1.x );
	Rect.p0.y = Max( Rect.p0.y, _Clip.clip.p0.y );
	Rect.p1.y = Min( Rect.p1.y, _Clip.clip.p1.y );


	CVec3Dfp32 Verts3D[4];
	CVec2Dfp32 TVerts[4];
	CPixel32 Colors[4];
	CVec3Dfp32 *pV = Verts3D;
	CVec2Dfp32 *pTV = TVerts;
	CPixel32 *pC = Colors;
	if(m_pVBM)
	{
		pV = m_pVBM->Alloc_V3(4);
		pTV = m_pVBM->Alloc_V2(4);
		pC = m_pVBM->Alloc_CPixel32(4);
	}

	if(!pV || !pTV || !pC)
		return;

	pV[0].k[0] = Rect.p0.x;		pV[0].k[1] = Rect.p0.y;		pV[0].k[2] = 0;
	pV[1].k[0] = Rect.p1.x;		pV[1].k[1] = Rect.p0.y;		pV[1].k[2] = 0;
	pV[2].k[0] = Rect.p1.x;		pV[2].k[1] = Rect.p1.y;		pV[2].k[2] = 0;
	pV[3].k[0] = Rect.p0.x;		pV[3].k[1] = Rect.p1.y;		pV[3].k[2] = 0;
	CMat4Dfp32 InMat, OutMat, Rotate;
	InMat.Unit();
	OutMat.Unit();
	Rotate.Unit();

	CVec3Dfp32(-_Pos.x, -_Pos.y,0).SetMatrixRow(InMat,3);
	CVec3Dfp32(_Pos.x, _Pos.y,0).SetMatrixRow(OutMat,3);
	Rotate.SetZRotation3x3(_Angle);
	InMat.Multiply(Rotate, Rotate);
	Rotate.Multiply(OutMat, Rotate);
	Rotate.Multiply(m_CurTransform, Rotate);
	
	// Multiply to temporary vertices otherwise rotations may be f#)&/%# up
	CVec3Dfp32 TempVerts[4];
	CVec3Dfp32::MultiplyMatrix(pV, TempVerts, Rotate, 4);
	// Copy the temporary vertices back
	memcpy(pV, TempVerts, sizeof(CVec3Dfp32)*4);

	fp32 xs = 1.0f/fp32(m_CurTxtW) * m_CurTextureScale.k[0];
	fp32 ys = 1.0f/fp32(m_CurTxtH) * m_CurTextureScale.k[1];
	int txtox = m_CurTextureOrigo.x;
	int txtoy = m_CurTextureOrigo.y;
	pTV[0][0] = fp32(Rect.p0.x - txtox) * xs;
	pTV[0][1] = fp32(Rect.p0.y - txtoy) * ys;
	pTV[1][0] = fp32(Rect.p1.x - txtox) * xs;
	pTV[1][1] = fp32(Rect.p0.y - txtoy) * ys;
	pTV[2][0] = fp32(Rect.p1.x - txtox) * xs;
	pTV[2][1] = fp32(Rect.p1.y - txtoy) * ys;
	pTV[3][0] = fp32(Rect.p0.x - txtox) * xs;
	pTV[3][1] = fp32(Rect.p1.y - txtoy) * ys;
	pC[0] = _Color;
	pC[1] = _Color;
	pC[2] = _Color;
	pC[3] = _Color;

	if(m_pVBM)
	{
		CXR_VertexBuffer *pVB = m_pVBM->Alloc_VB();
		if(pVB)
		{
			pVB->m_Priority = (m_VBPriority += m_VBPriorityAdd);
			if (!pVB->AllocVBChain(m_pVBM, false))
				return;
			pVB->Geometry_VertexArray(pV, 4, true);
			pVB->Geometry_TVertexArray(pTV, 0);
//			pVB->Geometry_ColorArray(pC);
			pVB->Geometry_Color(_Color);
			pVB->Render_IndexedTriangles(ms_DualTringle, 2);

			if(m_pSurf)
				CXR_Util::Render_Surface(0, m_SurfTime, m_pSurf, m_pSurfKeyFrame, NULL, m_pVBM, (CMat4Dfp32*)NULL, (CMat4Dfp32*)NULL, (CMat4Dfp32*)NULL, pVB, m_VBPriority, 0.0001f);
			else
			{
				pVB->m_pAttrib	= GetLastSubmitted();

				if(pVB->m_pAttrib)
				{
					m_pVBM->AddVB(pVB);
//					if(m_bAutoFlush)
//						m_pVBM->Flush(m_pCurRC, 0);
				}
			}
		}
	}
	else
	{
		m_pCurRC->Attrib_TextureID(0, m_Attrib.m_TextureID[0]);

		m_pCurRC->Geometry_VertexArray(pV, 4);
		m_pCurRC->Geometry_TVertexArray(pTV, 0);
//		m_pCurRC->Geometry_Color(_Color);
		m_pCurRC->Geometry_ColorArray(pC);
		m_pCurRC->Render_IndexedTriangles(ms_DualTringle, 2);
		m_pCurRC->Geometry_Clear();

//		m_pCurRC->Render_Polygon(4, pV, pTV, &ms_IndexRamp[0], &ms_IndexRamp[0], _Color);
	}
}

void CRC_Util2D::ScaleSprite(const CClipRect& _Clip, const CPnt& _Pos, const CPnt& _Size, const CPixel32& _Color)
{
	MAUTOSTRIP(CRC_Util2D_ScaleSprite, MAUTOSTRIP_VOID);
	if ((_Size.x == 0) || (_Size.y == 0)) return;
	if (m_Attrib.m_TextureID[0] == 0) return;

//	GetRC()->Attrib_Push();

//	GetRC()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	SetTextureOrigo(_Clip, _Pos);
	m_CurTextureScale.k[0] = m_CurTxtW/fp32(_Size.x);
	m_CurTextureScale.k[1] = m_CurTxtH/fp32(_Size.y);
	Rect(_Clip, CRct(_Pos, _Pos + _Size), _Color);

	m_CurTextureScale.k[0] = 1.0f;
	m_CurTextureScale.k[1] = 1.0f;
//	GetRC()->Attrib_Pop();
}

void CRC_Util2D::Lines(const CClipRect& _Clip, const CVec2Dfp32 *_p0, const CVec2Dfp32 *_p1, const CPixel32 *_Color0, const CPixel32 *_Color1, int _nLines)
{

	if(m_pVBM)
	{
		CXR_VertexBuffer *pVB = CXR_Util::VBM_RenderWires2D(m_pVBM, _Clip, _p0, _p1, _Color0, _Color1, _nLines);
		if (!pVB)
			return;

		pVB->m_Priority = (m_VBPriority += m_VBPriorityAdd);
		pVB->m_pAttrib = GetLastSubmitted();
		if (pVB->m_pAttrib)
		{
			m_pVBM->AddVB(pVB);
		}
	}
	else
	{
		for (int i = 0; i < _nLines; ++i)
		{
			Line(_Clip, CPnt::From_fp32(_p0[i][0], _p0[i][1]), CPnt::From_fp32(_p1[i][0], _p1[i][1]), _Color0[i]);
		}
	}
}

void CRC_Util2D::Line(const CClipRect& _Clip, const CPnt& _p0, const CPnt& _p1, const CPixel32& _Color)
{
	MAUTOSTRIP(CRC_Util2D_Line, MAUTOSTRIP_VOID);
	if(m_pVBM)
	{
		CXR_VertexBuffer *pVB = m_pVBM->Alloc_VB(CXR_VB_VERTICES, 2);
		if(!pVB)
			return;
		CXR_VBChain *pVBChain = pVB->GetVBChain();

		CVec3Dfp32 Verts[2];
		CVec3Dfp32 *Verts3D = pVBChain->m_pV;

		CPnt p0 = _p0;
		CPnt p1 = _p1;

		p0 += _Clip.ofs;
		p1 += _Clip.ofs;

		if (!CImage::ClipLine(_Clip.clip, p0.x, p0.y, p1.x, p1.y)) return;

		Verts[0] = CVec3Dfp32(p0.x, p0.y, 0);
		Verts[1] = CVec3Dfp32(p1.x, p1.y, 0);
		CVec3Dfp32::MultiplyMatrix(&Verts[0], &Verts3D[0], m_CurTransform, 2);
		GetAttrib()->Attrib_TextureID(0, 0);

		pVB->m_Priority = (m_VBPriority += m_VBPriorityAdd);
		static uint16 WireData[] = {0, 1};
		pVB->Render_IndexedWires(WireData, 2);
		pVB->Geometry_Color(_Color);
		pVB->m_pAttrib = GetLastSubmitted();

		if (pVB->m_pAttrib)
		{
			m_pVBM->AddVB(pVB);
		}
	}
	else
	{
		CVec3Dfp32 Verts[4];
		CVec3Dfp32 Verts3D[4];

	//	if (!_Clip.Visible(CRct(_p0, _p1))) return;

		CPnt p0 = _p0;
		CPnt p1 = _p1;

		p0 += _Clip.ofs;
		p1 += _Clip.ofs;

		if (!CImage::ClipLine(_Clip.clip, p0.x, p0.y, p1.x, p1.y)) return;

		Verts[0] = CVec3Dfp32(p0.x, p0.y, 0);
		Verts[1] = CVec3Dfp32(p1.x, p1.y, 0);
		CVec3Dfp32::MultiplyMatrix(&Verts[0], &Verts3D[0], m_CurTransform, 2);
		m_pCurRC->Attrib_TextureID(0, 0);
		m_pCurRC->Render_Wire(Verts3D[0], Verts3D[1], _Color);
	}

/*
	CVec2Dfp32 LineN((p1.y - p0.y), -(p1.x - p0.x));
	LineN.Normalize();

	Verts[0] = CVec3Dfp32(p0.x, p0.y, 0);
	Verts[1] = CVec3Dfp32(p1.x, p1.y, 0);
	Verts[2] = CVec3Dfp32(fp32(p1.x) + LineN.k[0], fp32(p1.y) + LineN.k[1], 0);
	Verts[3] = CVec3Dfp32(fp32(p0.x) + LineN.k[0], fp32(p0.y) + LineN.k[1], 0);

	CVec2Dfp32 TVerts[4];
	if (m_CurTextureID)
	{
		fp32 xs = 1.0f/fp32(m_CurTxtW) * m_CurTextureScale.k[0];
		fp32 ys = 1.0f/fp32(m_CurTxtH) * m_CurTextureScale.k[1];
		int txtox = m_CurTextureOrigo.x;
		int txtoy = m_CurTextureOrigo.y;
		TVerts[0].k[0] = (Verts[0].k[0] - txtox) * xs;
		TVerts[0].k[1] = (Verts[0].k[1] - txtoy) * ys;
		TVerts[1].k[0] = (Verts[1].k[0] - txtox) * xs;
		TVerts[1].k[1] = (Verts[1].k[1] - txtoy) * ys;
		TVerts[2].k[0] = (Verts[2].k[0] - txtox) * xs;
		TVerts[2].k[1] = (Verts[2].k[1] - txtoy) * ys;
		TVerts[3].k[0] = (Verts[3].k[0] - txtox) * xs;
		TVerts[3].k[1] = (Verts[3].k[1] - txtoy) * ys;
	}
	else
		FillChar(&TVerts, sizeof(TVerts), 0);

	CVec3Dfp32::MultiplyMatrix(&Verts[0], &Verts3D[0], m_CurTransform, 4);
	m_pCurRC->Attrib_TextureID(0, m_CurTextureID);
	m_pCurRC->Render_Polygon(4, &Verts3D[0], &TVerts[0], &ms_IndexRamp[0], &ms_IndexRamp[0], _Color);
*/
}


void CRC_Util2D::Frame(const CClipRect& _Clip, int _x0, int _y0, int _x1, int _y1, const CPixel32& _Color0, const CPixel32& _Color1, bool _bInverse)
{
	MAUTOSTRIP(CRC_Util2D_Frame, MAUTOSTRIP_VOID);
	if (!_bInverse)
	{
		Rect(_Clip, CRct(_x0, _y0, _x1-1, _y0+1), _Color0);	// Top horz.
		Rect(_Clip, CRct(_x0, _y0+1, _x0+1, _y1-1), _Color0);	// Left vert

		Rect(_Clip, CRct(_x0, _y1-1, _x1, _y1), _Color1);		// Bottom horz.
		Rect(_Clip, CRct(_x1-1, _y0, _x1, _y1-1), _Color1);	// Right vert
	}
	else
	{
		Rect(_Clip, CRct(_x0, _y0, _x1, _y0+1), _Color0);		// Top horz.
		Rect(_Clip, CRct(_x0, _y0+1, _x0+1, _y1), _Color0);	// Left vert

		Rect(_Clip, CRct(_x0+1, _y1-1, _x1, _y1), _Color1);	// Bottom horz.
		Rect(_Clip, CRct(_x1-1, _y0+1, _x1, _y1-1), _Color1);	// Right vert
	}
}


int CRC_Util2D::TextHeight(CRC_Font* _pFont, const char* _pStr, fp32 _FontScale)
{
	MAUTOSTRIP(CRC_Util2D_TextHeight, 0);
	if (_pStr)
		return int(_pFont->GetHeight(_pFont->GetOriginalSize() * _FontScale, _pStr));
	else
		return int(_pFont->GetOriginalSize());
}


int CRC_Util2D::TextHeight(CRC_Font* _pFont, const wchar* _pStr, fp32 _FontScale)
{
	MAUTOSTRIP(CRC_Util2D_TextHeight_2, 0);
	if (_pStr)
		return int(_pFont->GetHeight(_pFont->GetOriginalSize() * _FontScale, _pStr));
	else
		return int(_pFont->GetOriginalSize());
}


int CRC_Util2D::TextHeight(CRC_Font* _pFont, const CStr& _Str, fp32 _FontScale)
{
	MAUTOSTRIP(CRC_Util2D_TextHeight_3, 0);
	if (_Str.Len())
		return int(_pFont->GetHeight(_pFont->GetOriginalSize() * _FontScale, _Str));
	else
		return int(_pFont->GetOriginalSize());
}


int CRC_Util2D::TextWidth(CRC_Font* _pFont, const char* _pStr, fp32 _FontScale)
{
	MAUTOSTRIP(CRC_Util2D_TextWidth, 0);
	return int(_pFont->GetWidth(_pFont->GetOriginalSize() * _FontScale, _pStr));
}


int CRC_Util2D::TextWidth(CRC_Font* _pFont, const wchar* _pStr, fp32 _FontScale)
{
	MAUTOSTRIP(CRC_Util2D_TextWidth_2, 0);
	return int(_pFont->GetWidth(_pFont->GetOriginalSize() * _FontScale, _pStr));
}

int CRC_Util2D::TextWidth(CRC_Font* _pFont,const CStr& _Str, fp32 _FontScale)
{
	MAUTOSTRIP(CRC_Util2D_TextWidth_3, 0);
	return int(_pFont->GetWidth(_pFont->GetOriginalSize() * _FontScale, _Str));
}


int CRC_Util2D::TextFit(CRC_Font* _pFont, const char* _pStr, int _Width, bool _bWordWrap, fp32 _FontScale)
{
	MAUTOSTRIP(CRC_Util2D_TextFit, 0);
	return _pFont->GetFit(_pFont->GetOriginalSize() * _FontScale, _pStr, _Width, _bWordWrap);
}


int CRC_Util2D::TextFit(CRC_Font* _pFont, const wchar* _pStr, int _Width, bool _bWordWrap, fp32 _FontScale)
{
	MAUTOSTRIP(CRC_Util2D_TextFit_2, 0);
	return _pFont->GetFit(_pFont->GetOriginalSize() * _FontScale, _pStr, _Width, _bWordWrap);
}


int CRC_Util2D::TextFit(CRC_Font* _pFont, const CStr& _Str, int _Width, bool _bWordWrap, fp32 _FontScale)
{
	MAUTOSTRIP(CRC_Util2D_TextFit_3, 0);
	return _pFont->GetFit(_pFont->GetOriginalSize() * _FontScale, _Str, _Width, _bWordWrap);
}

CFStr FormatFloat(fp32 _Value)
{
	if (_Value < 0.000001f)
		return CFStrF("%.1f n", _Value * 1000000000.0f);
	if (_Value < 0.001f)
		return CFStrF("%.1f mi", _Value * 1000000.0f);
	if (_Value < 1.0f)
		return CFStrF("%.1f m", _Value * 1000.0f);
	if (_Value < 1000.0f)
		return CFStrF("%.1f", _Value);
	if (_Value < 1000000.0f)
		return CFStrF("%.1f k", _Value*.001f);
	if (_Value < 1000000000.0f)
		return CFStrF("%.1f M", _Value*.000001f);
	if (_Value < 1000000000000.0f)
		return CFStrF("%.1f G", _Value*.000000001f);

	return CFStrF("%.1f", _Value);	
}

void CRC_Util2D::DrawGraph(const CClipRect& _Clip, CRC_Font* _pF, const CRct &_Rect, const CPixel32 &_Background, const CPixel32 &_ColorLow, const CPixel32 &_ColorMid, const CPixel32 &_ColorHigh, fp32 _AvgValue, const fp32 *_pValues, int _nValues, fp32 _OriginalHeight, const ch8 *_pName)
{
	DrawGraph(_Clip, _pF, _Rect, _Background, _nValues, _OriginalHeight, _pName, 1, &_ColorLow, &_ColorMid, &_ColorHigh, &_AvgValue, &_pValues);
}

void CRC_Util2D::DrawGraph(const CClipRect& _Clip, CRC_Font* _pF, const CRct &_Rect, const CPixel32 &_Background, int _nValues, fp32 _OriginalHeight, const ch8 *_pName, int _nGraphs, const CPixel32 *_pColorLow, const CPixel32 *_pColorMid, const CPixel32 *_pColorHigh, fp32 *_pAvgValue, const fp32 **_pValues)
{

	if (_nGraphs == 0)
		return;
	CRC_Attributes *pAttr = GetAttrib();
	if (!pAttr)
		return;

	if (!m_pVBM)
		return;

	fp32 X = _Rect.p1.x; 
	fp32 Bottom = fp32(_Rect.p1.y - 2);

	int nToDraw = _nValues;
	fp32 nToDrawf = (fp32)_nValues;

	int FirstPtr = _nValues -1;
	fp32 Step = fp32(_Rect.GetWidth()-2) / nToDraw - 1;

	pAttr->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
	pAttr->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	CVec2Dfp32 *pLine00 = (CVec2Dfp32 *)m_pVBM->Alloc((nToDraw*2*_nGraphs+3) * sizeof(CVec2Dfp32));
	CVec2Dfp32 *pLine01 = (CVec2Dfp32 *)m_pVBM->Alloc((nToDraw*2*_nGraphs+3) * sizeof(CVec2Dfp32));
	CPixel32 *pColorLine0 = (CPixel32 *)m_pVBM->Alloc((nToDraw*2*_nGraphs+3) * sizeof(CPixel32));
	CPixel32 *pColorLine1 = (CPixel32 *)m_pVBM->Alloc((nToDraw*2*_nGraphs+3) * sizeof(CPixel32));

	if (!pLine00 || !pLine01 || !pColorLine0 || !pColorLine1)
		return;

	CRct Rects[2];
	CPixel32 ColorRect[2];

	int iRectStart = 0;

	fp32 ValueScale = 1.0;
	fp32 TargetScale = 1.0;

	fp32 AvgValueMax = _pAvgValue[0];
	for (int i = 1; i < _nGraphs; ++i)
	{
		AvgValueMax = Max(_pAvgValue[i], AvgValueMax);
	}

	fp32 Target = _OriginalHeight * 1.16666666666666f;

	while (AvgValueMax * TargetScale > Target)
	{
		TargetScale *= 0.5f;
	}	


	fp32 HeightFull = _Rect.GetHeight()-2;
	fp32 Height = HeightFull - 27.0f;
	fp32 Height100 = Height / 1.25f;
	fp32 Height50 = Height100 * 0.5f;

	ValueScale = Height100 / (_OriginalHeight / TargetScale);

	
	int StartX = int(X-60);
	fp32 StartXf = fp32(StartX);
	Rects[iRectStart] = CRct::From_fp32(StartX - nToDraw * Step-2, Bottom - HeightFull, X, Bottom+2);
	X -= 60;
	ColorRect[iRectStart] = _Background;
	++iRectStart;

	int iStart = 0;
	pColorLine0[iStart] = CPixel32(0,0,55,255);
	pColorLine1[iStart] = CPixel32(0,0,55,255);
	pLine00[iStart] = CVec2Dfp32(X - (nToDrawf) * Step, Bottom);
	pLine01[iStart] = CVec2Dfp32(X, Bottom);
	++iStart;

	pColorLine0[iStart] = CPixel32(200,200,255,255);
	pColorLine1[iStart] = CPixel32(200,200,255,255);
	pLine00[iStart] = CVec2Dfp32(X - (nToDraw) * Step, Bottom+1);
	pLine01[iStart] = CVec2Dfp32(X, Bottom+1);
	++iStart;

	pColorLine0[iStart] = CPixel32(128,128,128,255);
	pColorLine1[iStart] = CPixel32(128,128,128,255);
	pLine00[iStart] = CVec2Dfp32(X - (nToDraw) * Step, Bottom - Height50);
	pLine01[iStart] = CVec2Dfp32(X, Bottom - Height50);
	++iStart;

	pColorLine0[iStart] = CPixel32(200,200,200,255);
	pColorLine1[iStart] = CPixel32(200,200,200,255);
	pLine00[iStart] = CVec2Dfp32(X - (nToDraw) * Step, Bottom - Height100);
	pLine01[iStart] = CVec2Dfp32(X, Bottom - Height100);
	++iStart;

	CClipRect ClipRect = CRct::From_fp32(X - nToDraw * Step - 2, Bottom - Height - 1-2, X, Bottom+2);

	vec128 vHeight50 = M_VLdScalar(Height50);
	vec128 vHeight50Rcp = M_VRcp(vHeight50);
	vec128 vValueScale = M_VLdScalar(ValueScale);

	for (int j = 0; j < _nGraphs; ++j)
	{

		fp32 LastValue = _pValues[j][FirstPtr] * ValueScale;
		vec128 vLastValue = M_VMul(M_VLdScalar(_pValues[j][FirstPtr]), vValueScale);
		int HistPtr = _nValues -1;

		vec128 vColorLow = M_VCnv_i32_f32(M_VCnvL_u16_u32(M_VCnvL_u8_u16(M_VLd32(&_pColorLow[j]))));
		vec128 vColorMid = M_VCnv_i32_f32(M_VCnvL_u16_u32(M_VCnvL_u8_u16(M_VLd32(&_pColorMid[j]))));
		vec128 vColorHigh = M_VCnv_i32_f32(M_VCnvL_u16_u32(M_VCnvL_u8_u16(M_VLd32(&_pColorHigh[j]))));

		const CPixel32 &ColorLow = _pColorLow[j];
		const CPixel32 &ColorMid = _pColorMid[j];
		const CPixel32 &ColorHigh = _pColorHigh[j];
		X = StartXf;


		vec128 vZero = M_VZero();
		vec128 vOne = M_VOne();
		vec128 vFade = M_VScalar(0.4f);
		vec128 vFadeCol = M_VConst(0.0f, 0.0f, 0.0f, 255.0f);

		vec128 ColLast = M_VLrp(vColorLow, vColorMid, M_VMin(M_VMul(vLastValue, vHeight50Rcp), vOne));
		ColLast = M_VLrp(ColLast, vColorHigh, M_VMin(vOne, M_VMax(M_VMul(M_VSub(vLastValue, vHeight50), vHeight50Rcp), vZero)));

		vec128 tmp0 = M_VCnv_f32_i32(ColLast);
		vec128 tmp1 = M_VCnvS_i32_i16(tmp0, tmp0);
		vec128 ColLast_u8 = M_VCnvS_i16_u8(tmp1, tmp1);

		tmp0 = M_Vfp32toi32(M_VLrp(ColLast, vFadeCol, vFade));
		tmp1 = M_VCnvS_i32_i16(tmp0, tmp0);
		vec128 ColLastf_u8 = M_VCnvS_i16_u8(tmp1, tmp1);

		for (int i = 0; i < nToDraw-1; ++i)
		{
			--HistPtr;

			fp32 Value = _pValues[j][HistPtr] * ValueScale;
			vec128 vValue = M_VMul(M_VLdScalar(_pValues[j][HistPtr]), vValueScale);
//			pLine00[iStart] = CVec2Dfp32(X - Step-1.0f, Bottom - Value - 1.0f);
//			pLine01[iStart] = CVec2Dfp32(X-1.0f, Bottom - LastValue - 1.0f);
			pLine00[iStart][0] = X - Step-1.0f;
			pLine00[iStart][1] = Bottom - Value - 1.0f;
			pLine01[iStart][0] = X-1.0f;
			pLine01[iStart][1] = Bottom - LastValue - 1.0f;

/*	pLine00[iStart][0] = Random * 700.0f - 300.0f + ClipRect.clip.p0.x;
	pLine00[iStart][1] = Random * 200.0f - 50.0f + ClipRect.clip.p0.y;
	pLine01[iStart][0] = Random * 700.0f - 300.0f + ClipRect.clip.p0.x;
	pLine01[iStart][1] = Random * 200.0f - 50.0f + ClipRect.clip.p0.y;
*/
			vec128 Col0 = M_VLrp(vColorLow, vColorMid, M_VMin(M_VMul(vValue, vHeight50Rcp), vOne));
			Col0 = M_VLrp(Col0, vColorHigh, M_VMin(vOne, M_VMax(M_VMul(M_VSub(vValue, vHeight50), vHeight50Rcp), vZero)));

			vec128 tmp0 = M_VCnv_f32_i32(Col0);
			vec128 tmp1 = M_VCnvS_i32_i16(tmp0, tmp0);
			vec128 Col0_u8 = M_VCnvS_i16_u8(tmp1, tmp1);
			tmp0 = M_Vfp32toi32(M_VLrp(Col0, vFadeCol, vFade));
			tmp1 = M_VCnvS_i32_i16(tmp0, tmp0);
			vec128 Col0f_u8 = M_VCnvS_i16_u8(tmp1, tmp1);

			M_VStAny32(Col0f_u8, &pColorLine0[iStart]);
			M_VStAny32(ColLastf_u8, &pColorLine1[iStart]);

/*			pColorLine0[iStart] = CPixel32::LinearRGBA(ColorLow, ColorMid,	Min(256, int((Value / Height50) * 256.0)));
			pColorLine1[iStart] = CPixel32::LinearRGBA(ColorLow, ColorMid,	Min(256, int((LastValue / Height50) * 256.0)));
			pColorLine0[iStart] = CPixel32::LinearRGBA(pColorLine0[iStart], ColorHigh, Min(256, int((Max((Value - Height50) / Height50, 0.0f)) * 256.0)));
			pColorLine1[iStart] = CPixel32::LinearRGBA(pColorLine1[iStart], ColorHigh, Min(256, int((Max((LastValue - Height50) / Height50, 0.0f)) * 256.0)));
			pColorLine0[iStart] = CPixel32::LinearRGBA(pColorLine0[iStart], CPixel32(0,0,0,255), 100);
			pColorLine1[iStart] = CPixel32::LinearRGBA(pColorLine1[iStart], CPixel32(0,0,0,255), 100);*/
			++iStart;

//			pLine00[iStart] = CVec2Dfp32(X - Step, Bottom - Value);
//			pLine01[iStart] = CVec2Dfp32(X, Bottom - LastValue);
			pLine00[iStart][0] = X - Step;
			pLine00[iStart][1] = Bottom - Value;
			pLine01[iStart][0] = X;
			pLine01[iStart][1] = Bottom - LastValue;
/*	pLine00[iStart][0] = Random * 700.0f - 300.0f + ClipRect.clip.p0.x;
	pLine00[iStart][1] = Random * 200.0f - 50.0f + ClipRect.clip.p0.y;
	pLine01[iStart][0] = Random * 700.0f - 300.0f + ClipRect.clip.p0.x;
	pLine01[iStart][1] = Random * 200.0f - 50.0f + ClipRect.clip.p0.y;
*/
			M_VStAny32(Col0_u8, &pColorLine0[iStart]);
			M_VStAny32(ColLast_u8, &pColorLine1[iStart]);
/*			pColorLine0[iStart] = CPixel32::LinearRGBA(ColorLow, ColorMid,	Min(256, int((Value / Height50) * 256.0)));
			pColorLine1[iStart] = CPixel32::LinearRGBA(ColorLow, ColorMid,	Min(256, int((LastValue / Height50) * 256.0)));
			pColorLine0[iStart] = CPixel32::LinearRGBA(pColorLine0[iStart], ColorHigh, Min(256, int((Max((Value - Height50) / Height50, 0.0f)) * 256.0)));
			pColorLine1[iStart] = CPixel32::LinearRGBA(pColorLine1[iStart], ColorHigh, Min(256, int((Max((LastValue - Height50) / Height50, 0.0f)) * 256.0)));
*/
			++iStart;

			LastValue = Value;
			ColLast_u8 = Col0_u8;
			ColLastf_u8 = Col0f_u8;

			X -= Step;
		}
	}

//	Rects(_Clip, Rects, ColorRect, iRectStart);
	this->Rects(_Clip, Rects, ColorRect, iRectStart);
	Lines(ClipRect, pLine00, pLine01, pColorLine0, pColorLine1, iStart);

	Text(_Clip, _pF, int(X + 5), int(Bottom - HeightFull) + 5, _pName, CPixel32(255,255,255,255), _pF->GetOriginalSize() * 2.0);

	if (_nGraphs == 1)
	{
		fp32 CurrentValue = _pValues[0][FirstPtr];
		Text(_Clip, _pF, int(StartX + 10), int(Bottom - Height), FormatFloat(_OriginalHeight / TargetScale).Str(), CPixel32(255,255,255,255), _pF->GetOriginalSize() * 2.0);
		Text(_Clip, _pF, int(StartX + 10), int(Bottom - Height)+20, FormatFloat(AvgValueMax).Str(), CPixel32(255,255,255,255), _pF->GetOriginalSize() * 2.0);
		Text(_Clip, _pF, int(StartX + 10), int(Bottom - Height)+40, FormatFloat(CurrentValue).Str(), CPixel32(255,255,255,255), _pF->GetOriginalSize() * 2.0);
	}
	else
	{
		for (int j = 0; j < _nGraphs; ++j)
		{
			Text(_Clip, _pF, int(StartX + 10), int(Bottom - Height) + j * 20, FormatFloat(_pAvgValue[j]).Str(), _pColorHigh[j], _pF->GetOriginalSize() * 2.0);
		}
	}
}

void CRC_Util2D::Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const char* _pStr, const CPixel32& _Color, fp32 _Size)
{
	MAUTOSTRIP(CRC_Util2D_Text, MAUTOSTRIP_VOID);
	_x0 += _Clip.ofs.x;
	_y0 += _Clip.ofs.y;

	CVec3Dfp32 v(_x0, _y0, 0);
	v *= m_CurTransform;

	CVec3Dfp32 xv = CVec3Dfp32::GetMatrixRow(m_CurTransform, 0) * m_CurFontScale[0];
	CVec3Dfp32 yv = CVec3Dfp32::GetMatrixRow(m_CurTransform, 1) * m_CurFontScale[1];

	if (m_pVBM)
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB(CXR_VB_ATTRIB);
		if (!pVB) return;
		pVB->m_Priority = (m_VBPriority += m_VBPriorityAdd);
		if(GetLastSubmitted())
		{
			*pVB->m_pAttrib	= *GetLastSubmitted();

			if (_pFont->Write(m_pVBM, pVB, v, xv, yv, _pStr, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
				CVec2Dfp32(_Clip.clip.p0.x - _x0, _Clip.clip.p0.y - _y0),
				CVec2Dfp32(_Clip.clip.p1.x - _x0, _Clip.clip.p1.y - _y0)))
			{
				m_pVBM->AddVB(pVB);
			}
		}
	}
	else
	{
		_pFont->Write(m_pCurRC, v, xv, yv, _pStr, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
			CVec2Dfp32(_Clip.clip.p0.x - _x0, _Clip.clip.p0.y - _y0),
			CVec2Dfp32(_Clip.clip.p1.x - _x0, _Clip.clip.p1.y - _y0));
	}
}


void CRC_Util2D::Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const wchar* _pStr, const CPixel32& _Color, fp32 _Size)
{
	MAUTOSTRIP(CRC_Util2D_Text_2, MAUTOSTRIP_VOID);
	_x0 += _Clip.ofs.x;
	_y0 += _Clip.ofs.y;

	CVec3Dfp32 v(_x0, _y0, 0);
	v *= m_CurTransform;

	CVec3Dfp32 xv = CVec3Dfp32::GetMatrixRow(m_CurTransform, 0) * m_CurFontScale[0];
	CVec3Dfp32 yv = CVec3Dfp32::GetMatrixRow(m_CurTransform, 1) * m_CurFontScale[1];
//	fp32 screen_aspect = GetRC()->GetDC()->GetScreenAspect();
//	xv.k[0] = xv.k[0] / screen_aspect;

	if (m_pVBM)
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB(CXR_VB_ATTRIB);
		if (!pVB) return;
		pVB->m_Priority = (m_VBPriority += m_VBPriorityAdd);
		if(GetLastSubmitted())
		{
			*pVB->m_pAttrib	= *GetLastSubmitted();

			if (_pFont->Write(m_pVBM, pVB, v, xv, yv, _pStr, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
				CVec2Dfp32(_Clip.clip.p0.x - _x0, _Clip.clip.p0.y - _y0),
				CVec2Dfp32(_Clip.clip.p1.x - _x0, _Clip.clip.p1.y - _y0)))
			{
				m_pVBM->AddVB(pVB);
			}
		}
	}
	else
	{
		_pFont->Write(m_pCurRC, v, xv, yv, _pStr, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
			CVec2Dfp32(_Clip.clip.p0.x - _x0, _Clip.clip.p0.y - _y0),
			CVec2Dfp32(_Clip.clip.p1.x - _x0, _Clip.clip.p1.y - _y0));
	}
}


void CRC_Util2D::Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const CStr& _Str, const CPixel32& _Color, fp32 _Size)
{
	MAUTOSTRIP(CRC_Util2D_Text_3, MAUTOSTRIP_VOID);
	_x0 += _Clip.ofs.x;
	_y0 += _Clip.ofs.y;

	CVec3Dfp32 v(_x0, _y0, 0);
	v *= m_CurTransform;

	CVec3Dfp32 xv = CVec3Dfp32::GetMatrixRow(m_CurTransform, 0) * m_CurFontScale[0];
	CVec3Dfp32 yv = CVec3Dfp32::GetMatrixRow(m_CurTransform, 1) * m_CurFontScale[1];

	if (m_pVBM)
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB(CXR_VB_ATTRIB);
		if (!pVB) return;
		pVB->m_Priority = (m_VBPriority += m_VBPriorityAdd);
		if(GetLastSubmitted())
		{
			*pVB->m_pAttrib	= *GetLastSubmitted();

			if (_pFont->Write(m_pVBM, pVB, v, xv, yv, _Str, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
				CVec2Dfp32(_Clip.clip.p0.x - _x0, _Clip.clip.p0.y - _y0),
				CVec2Dfp32(_Clip.clip.p1.x - _x0, _Clip.clip.p1.y - _y0)))
			{
				m_pVBM->AddVB(pVB);
			}
		}
	}
	else
	{
		_pFont->Write(m_pCurRC, v, xv, yv, _Str, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
			CVec2Dfp32(_Clip.clip.p0.x - _x0, _Clip.clip.p0.y - _y0),
			CVec2Dfp32(_Clip.clip.p1.x - _x0, _Clip.clip.p1.y - _y0));
	}
}

// PC PORT: Print using float position
void CRC_Util2D::TextFloat(const CClipRect& _Clip, CRC_Font* _pFont, fp32 _x0, fp32 _y0, const CStr& _Str, const CPixel32& _Color, fp32 _Size)
{
	MAUTOSTRIP(CRC_Util2D_TextFloat, MAUTOSTRIP_VOID);

	const fp32 x0 = (fp32)_x0 + _Clip.ofs.x;
	const fp32 y0 = (fp32)_y0 + _Clip.ofs.y;

	CVec3Dfp32 v(x0, y0, 0.0f);
	v *= m_CurTransform;

	CVec3Dfp32 xv = CVec3Dfp32::GetMatrixRow(m_CurTransform, 0) * m_CurFontScale[0];
	CVec3Dfp32 yv = CVec3Dfp32::GetMatrixRow(m_CurTransform, 1) * m_CurFontScale[1];

	if (m_pVBM)
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB(CXR_VB_ATTRIB);
		if (!pVB) return;
		pVB->m_Priority = (m_VBPriority += m_VBPriorityAdd);
		if(GetLastSubmitted())
		{
			*pVB->m_pAttrib	= *GetLastSubmitted();

			if (_pFont->Write(m_pVBM, pVB, v, xv, yv, _Str, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
				CVec2Dfp32((fp32)_Clip.clip.p0.x - x0, (fp32)_Clip.clip.p0.y - y0),
				CVec2Dfp32((fp32)_Clip.clip.p1.x - x0, (fp32)_Clip.clip.p1.y - y0)))
			{
				m_pVBM->AddVB(pVB);
			}
		}
	}
	else
	{
		_pFont->Write(m_pCurRC, v, xv, yv, _Str, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
			CVec2Dfp32((fp32)_Clip.clip.p0.x - x0, (fp32)_Clip.clip.p0.y - y0),
			CVec2Dfp32((fp32)_Clip.clip.p1.x - x0, (fp32)_Clip.clip.p1.y - y0));
	}
}
		
void CRC_Util2D::Text_Draw(const CClipRect& _Clip, CRC_Font* _pF, wchar* _pStr, int _x, int _y, int _Style, int ColM, int ColH, int ColD, fp32 _FontScale)
{
	MAUTOSTRIP(CRC_Util2D_Text_Draw, MAUTOSTRIP_VOID);
	int bShw = _Style & WSTYLE_TEXT_SHADOW;
	int bHL = _Style & WSTYLE_TEXT_HIGHLIGHT;
	int bCut = _Style & WSTYLE_TEXT_CUTOUT;
	
	if(_FontScale == 1.0f) // Text() takes -1 as default....
		_FontScale = -1.0f;
	else
		_FontScale = _pF->GetOriginalSize() * _FontScale;

	int xhl = _x;
	int yhl = _y;
	int xsh = 2+_x;
	int ysh = 2+_y;
	if (bCut) { Swap(xhl, xsh); Swap(yhl, ysh); }
	
	int x = _x;
	int y = _y;
	if (bShw || bHL) { x += 1; y += 1; }
	
	if (bHL) Text(_Clip, _pF, xhl, yhl, _pStr, ColH, _FontScale);
	if (bShw) Text(_Clip, _pF, xsh, ysh, _pStr, ColD, _FontScale);
	Text(_Clip, _pF, x, y, _pStr, ColM, _FontScale);
}


int CRC_Util2D::Text_WordWrap(CRC_Font* _pF, int _Width, wchar* _pStr, int _Len, wchar** _ppLines, int _MaxLines, fp32 _FontScale)
{
	MAUTOSTRIP(CRC_Util2D_Text_WordWrap, 0);
	int last = 0;
	int nLines = 0;
	wchar CtrlCodesBuf[512];
	int CtrlCodesBufLen = 0;
	
	while(last < _Len)
	{
		int n = _Len-last;
		if (n > 511)
		{
			ConOut("CMWnd_Text_WordWrap, Tried to draw a string larger than 511 bytes");
			return nLines;
		}
		
		if(CtrlCodesBufLen > 0)
			//Copy all controlcodes to new line
			memcpy(&_ppLines[nLines][0], CtrlCodesBuf, CtrlCodesBufLen*sizeof(wchar));
		
		memcpy(&_ppLines[nLines][CtrlCodesBufLen], &_pStr[last], n*sizeof(wchar));
		_ppLines[nLines][n + CtrlCodesBufLen] = 0;
		
		//LogFile(CStrF("Ick       %d, %d, %d", last, _Len, n));
		int nFit = TextFit(_pF, &_ppLines[nLines][0], _Width, true, _FontScale);
		//LogFile(CStrF("Bla   %d, %d, %d, %d", nFit, last, _Len, n));
		if (!nFit) nFit = TextFit(_pF, &_ppLines[nLines][0], _Width, false, _FontScale);
		if (!nFit) return nLines;
		_ppLines[nLines][nFit] = 0;
		
		for(int j = 0; j < nFit; j++)
			if(_ppLines[nLines][j] == '|')
			{
				_ppLines[nLines][j] = 0;
				nFit = j + 1;
				break;
			}
			
			last += nFit - CtrlCodesBufLen;
			
			if(last < _Len)
				CtrlCodesBufLen = CRC_Font::GetControlCodes(_ppLines[nLines], CtrlCodesBuf, sizeof(CtrlCodesBuf));
			
			nLines++;
			if (nLines >= _MaxLines) return _MaxLines;
	}
	
	return nLines;
}


int CRC_Util2D::Text_DrawFormatted(const CClipRect& _Clip, CRC_Font* _pF, const CStr& _Text, int& _x, int& _y, int _Style, int _ColM, int _ColH, int _ColD, int _Width, int _Height, fp32 _PercentVis, int _ExtraHeight, fp32 _FontScale)
{
	MAUTOSTRIP(CRC_Util2D_Text_DrawFormatted, 0);
	wchar Text[1024];
	//wchar PartialText[1024];
	Localize_Str(_Text, Text, 1023);
	_PercentVis = Min(1.0f, Max(0.f, _PercentVis));
	
	int visIndex = 0;
/*	if(Text[0] == 167) // Remove the size stuff
	{
		visIndex = (CStrBase::StrLen(Text) - 4) * _PercentVis;
		visIndex += 4; 
	}
	else*/
	{
		visIndex = int(CStrBase::StrLen(Text) * _PercentVis);
	}

	int Len = CStrBase::StrLen(Text);
	
	wchar Lines[16][512];
	const int MaxLines = 15;
	wchar* lpLines[16];
	for(int i = 0; i < 16; i++) lpLines[i] = &Lines[i][0];
	
	int bWW = _Style & WSTYLE_TEXT_WORDWRAP;
	int nLines = 0;
	if(bWW)
		nLines += Text_WordWrap(_pF, _Width, (wchar*) Text, Len, &lpLines[nLines], 15-nLines, _FontScale);
	else if (_Text.Find("|") >= 0)
	{
		const char* pStr = _Text;
		int len = _Text.Len();
		int last = 0;
		while(last < len)
		{
			int i;
			for(i = last; i < len; i++)
				if (pStr[i] == wchar('|')) break;
				
				// Copy whole part.
				int n = i-last;
				if (n > 511) return nLines;
				memcpy(&Lines[nLines], &pStr[last], n*sizeof(wchar));
				Lines[nLines][n] = 0;
				nLines++;
				if (nLines >= MaxLines) break;
				
				last = i+1;
				//			LogFile(CStrF("Hirr       %d, %d", last, i));
		}
		
	}
	else
	{
		// Copy whole string
		CStrBase::mfscpy(&Lines[0][0], CSTR_FMT_UNICODE, Text, CSTR_FMT_UNICODE);
		nLines = 1;
	}
	

	int th = -1;
	int y = 0;
	if (_Style & WSTYLE_TEXT_CENTERY)
	{
		th = TextHeight(_pF, (wchar*)Text, _FontScale);
		int Add = (_Height - th*nLines) >> 1;
		y += Add;
	}
	
	int nVisibleCharsOnLine = 0;
	int nVisibleChars = 0;
	int ln;
	for(ln = 0; ln < nLines; ln++)
	{
		int x = 0;
		if (_Style & WSTYLE_TEXT_CENTER)
			x += (_Width - TextWidth(_pF, &Lines[ln][0],  _FontScale)) >> 1;

		nVisibleCharsOnLine = CStrBase::StrLen(lpLines[ln]);
		
		if(nVisibleChars + nVisibleCharsOnLine > visIndex)
			Lines[ln][visIndex - nVisibleChars] = NULL;

		Text_Draw(_Clip, _pF, &Lines[ln][0], _x + x, _y + y, _Style, _ColM, _ColH, _ColD, _FontScale);
		/*if (nLines > 1 && th < 0)*/ th = TextHeight(_pF, &Lines[ln][0], _FontScale);
		y += th + _ExtraHeight;

		nVisibleChars += nVisibleCharsOnLine;
		if(nVisibleChars > visIndex)
		{
			ln++;
			break;
		}

		visIndex += 4; // Add the characters for the text sizing
		
	}
	
	_y += y;
	_x += TextWidth(_pF, lpLines[ln-1], _FontScale);
	
	return nLines;
}

int CRC_Util2D::Text_DrawFormatted(const CClipRect& _Clip, CRC_Font* _pF, const CStr& _Text, int _x, int _y, int _Style, int _ColM, int _ColH, int _ColD, int _Width, int _Height, bool _bShadow, int _ExtraHeight, fp32 _FontScale)
{
	MAUTOSTRIP(CRC_Util2D_Text_DrawFormatted_2, 0);
	wchar Text[1024];
	Localize_Str(_Text, Text, 1023);
	int Len = CStrBase::StrLen(Text);
	
	wchar Lines[32][512];
	const int MaxLines = 31;
	wchar* lpLines[32];
	for(int i = 0; i < 32; i++) lpLines[i] = &Lines[i][0];
	
	int bWW = _Style & WSTYLE_TEXT_WORDWRAP;
	int nLines = 0;
	if(bWW)
		nLines += Text_WordWrap(_pF, _Width, (wchar*) Text, Len, &lpLines[nLines], 31-nLines, _FontScale);
	else if (_Text.Find("|") >= 0)
	{
		const char* pStr = _Text;
		int len = _Text.Len();
		int last = 0;
		while(last < len)
		{
			int i;
			for(i = last; i < len; i++)
			{
				if (pStr[i] == wchar('|'))
					break;
			}
				
			// Copy whole part.
			int n = i-last;
			if (n > 511)
				return nLines;

			memcpy(&Lines[nLines], &pStr[last], n*sizeof(wchar));
			Lines[nLines][n] = 0;
			nLines++;
			if(nLines >= MaxLines)
				break;
			
			last = i+1;
			//			LogFile(CStrF("Hirr       %d, %d", last, i));
		}
		
	}
	else
	{
		// Copy whole string
		CStrBase::mfscpy(&Lines[0][0], CSTR_FMT_UNICODE, Text, CSTR_FMT_UNICODE);
		nLines = 1;
	}
	
	int th = -1;
	int y = 0;
	if (_Style & WSTYLE_TEXT_CENTERY)
	{
		th = TextHeight(_pF, (wchar*)Text, _FontScale);
		int Add = (_Height - th*nLines) >> 1;
		y += Add;
	}

	if( _bShadow )
	{
		for(int ln = 0; ln < nLines; ln++)
		{
			int x = 0;
			if (_Style & WSTYLE_TEXT_CENTER)
				x += (_Width - TextWidth(_pF, &Lines[ln][0])) >> 1;

			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)-1, (_y + y)-1, _Style, _ColM, _ColH, _ColD, _FontScale);
			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)-1, (_y + y)+0, _Style, _ColM, _ColH, _ColD, _FontScale);
			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)-1, (_y + y)+1, _Style, _ColM, _ColH, _ColD, _FontScale);
			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)+0, (_y + y)-1, _Style, _ColM, _ColH, _ColD, _FontScale);
			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)+0, (_y + y)+1, _Style, _ColM, _ColH, _ColD, _FontScale);
			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)+1, (_y + y)-1, _Style, _ColM, _ColH, _ColD, _FontScale);
			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)+1, (_y + y)+0, _Style, _ColM, _ColH, _ColD, _FontScale);
			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)+1, (_y + y)+1, _Style, _ColM, _ColH, _ColD, _FontScale);
			/*if (nLines > 1 && th < 0)*/ th = TextHeight(_pF, &Lines[ln][0], _FontScale);
			y += th + _ExtraHeight;
		}
	}
	else
	{
		for(int ln = 0; ln < nLines; ln++)
		{
			int x = 0;
			if (_Style & WSTYLE_TEXT_CENTER)
				x += (_Width - TextWidth(_pF, &Lines[ln][0], _FontScale)) >> 1;

			Text_Draw(_Clip, _pF, &Lines[ln][0], _x + x, _y + y, _Style, _ColM, _ColH, _ColD, _FontScale);
			/*if (nLines > 1 && th < 0)*/ th = TextHeight(_pF, &Lines[ln][0], _FontScale);
			y += th + _ExtraHeight;
		}
	}
	
	return nLines;
}


void CRC_Util2D::Localize_Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const char* _pStr, const CPixel32& _Color, fp32 _Size)
{
	MAUTOSTRIP(CRC_Util2D_Localize_Text, MAUTOSTRIP_VOID);
	wchar Buffer[1024];
	Localize_Str(_pStr, Buffer, 1024);
	Text(_Clip, _pFont, _x0, _y0, Buffer, _Color, _Size);
}

void CRC_Util2D::Localize_Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const wchar* _pStr, const CPixel32& _Color, fp32 _Size)
{
	MAUTOSTRIP(CRC_Util2D_Localize_Text_2, MAUTOSTRIP_VOID);
	wchar Buffer[1024];
	Localize_Str(_pStr, Buffer, 1024);
	Text(_Clip, _pFont, _x0, _y0, Buffer, _Color, _Size);
}

void CRC_Util2D::Localize_Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const CStr& _Str, const CPixel32& _Color, fp32 _Size)
{
	MAUTOSTRIP(CRC_Util2D_Localize_Text_3, MAUTOSTRIP_VOID);
	wchar Buffer[1024];
	Localize_Str(_Str, Buffer, 1024);
	Text(_Clip, _pFont, _x0, _y0, Buffer, _Color, _Size);
}


#if 0

void CRC_Util2D::Model(CXR_Engine* _pEngine, const CClipRect& _Clip, const CRct& _Rect, class CXR_Model *_pModel, class CXR_AnimState *_pAnimState, CMat4Dfp32* _Mat)
{
	MAUTOSTRIP(CRC_Util2D_Model, MAUTOSTRIP_VOID);
//	int x0 = 0;
//	int y0 = 0;
//	int x1 = _Rect.GetWidth();
//	int y1 = _Rect.GetHeight();
	

	CRenderContext* pRC = GetRC();

//	pRC->Attrib_Push();
		GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		GetAttrib()->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
		GetAttrib()->Attrib_Enable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);

//		pRC->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
//		pRC->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
//		pRC->Attrib_Enable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
//		pRC->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
//		Rect(_Clip, CRct(x0,y0,x1,y1), 0x00000000);
//		Rect(_Clip, _Rect, 0x00000000);
		SetTexture(0);
		Rect(_Clip, _Rect, 0x00000000);

//	pRC->Attrib_Pop();

	CBox3Dfp32 Bound;
	_pModel->GetBound_Box(Bound, _pAnimState);

	CVec3Dfp32 Size;
	Bound.m_Max.Sub(Bound.m_Min, Size);
	fp32 SizeX = Length2(Size[0], Size[1]);
	fp32 SizeY = Size[2];

	//pRC->Viewport_Get()->GetFOV()
	const fp32 FOV = 20;

	CVec2Dfp32 Scale = GetCoordinateScale();
	CRct Rect = _Rect;
	CClipRect Clip = _Clip;
	Rect += Clip.ofs;

	Rect.p0.x = int(Rect.p0.x * Scale[0]);
	Rect.p0.y = int(Rect.p0.y * Scale[1]);
	Rect.p1.x = int(Rect.p1.x * Scale[0]);
	Rect.p1.y = int(Rect.p1.y * Scale[1]);
	Clip.clip.p0.x = int(Clip.clip.p0.x * Scale[0]);
	Clip.clip.p0.y = int(Clip.clip.p0.y * Scale[1]);
	Clip.clip.p1.x = int(Clip.clip.p1.x * Scale[0]);
	Clip.clip.p1.y = int(Clip.clip.p1.y * Scale[1]);
	Clip.ofs.x = int(Clip.ofs.x * Scale[0]);
	Clip.ofs.y = int(Clip.ofs.y * Scale[1]);

	Clip += m_CurVP.GetViewClip();
	Rect += m_CurVP.GetViewClip().ofs;

	CRC_Viewport View;
	View.SetView(Clip, Rect);
	View.SetFOV(FOV);
	
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys && pSys->m_spDisplay)
	{
		View.SetAspectRatio(pSys->m_spDisplay->GetPixelAspect());
	}

	fp32 Dist = SizeX * View.GetScale() * 0.33f;
	fp32 DistY = SizeY * View.GetScale() * 0.33f;
	
	fp32 w = Rect.GetWidth();
	fp32 h = Rect.GetHeight();

	if (w > h)
	{
		if (Dist / w > DistY / h)
			DistY = Dist / w * h;
	}
	else
	{
		if (Dist / w < DistY / h)
			Dist = DistY / h * w;
	}

//	fp32 Dist = Max(SizeX / m_Pos.GetWidth(), SizeY / m_Pos.GetHeight());

	CMat4Dfp32 Camera;
	Camera.Unit();
	Camera.RotX_x_M(0.25f);
	Camera.RotY_x_M(-0.25f);
	Camera.RotZ_x_M(0.5f);
	Camera.k[3][0] = Max(Dist, DistY) * 1.0f;
	Camera.k[3][2] = 0;

	
	CMat4Dfp32 Mat;
	if(_Mat != NULL)
		Mat = *_Mat;
	else
		Mat.Unit();
	Mat.RotZ_x_M(_pAnimState->m_AnimTime1.GetTimeModulusScaled(-0.1f, _PI2));

	CVec3Dfp32 Center;
	Bound.GetCenter(Center);
	Center.MultiplyMatrix3x3(Mat);
	CVec3Dfp32::GetMatrixRow(Mat, 3) -= Center;

	if (m_pVBM->Viewport_Push(&View))
	{
		CMat4Dfp32 VMat;
		Camera.InverseOrthogonal(VMat);

		_pModel->OnRender(_pEngine, pRC, m_pVBM, this, m_spWorldLightState, _pAnimState, Mat, VMat, CXR_MODEL_ONRENDERFLAGS_MAXLOD);

		m_pVBM->Viewport_Pop();
	}
}


void CRC_Util2D::Model(const CClipRect& _Clip, const CRct& _Rect, class CXR_Model *_pModel, class CXR_AnimState *_pAnimState, CMat4Dfp32* _Mat)
{
	MAUTOSTRIP(CRC_Util2D_Model_2, MAUTOSTRIP_VOID);
	Model(NULL, _Clip, _Rect, _pModel, _pAnimState, _Mat);
}


// Renders a model and animates it
void CRC_Util2D::Model(CXR_Engine* _pEngine, class CWorld_Client* _pWClient, const CClipRect& _Clip, const CRct& _Rect, CXR_Anim_Base* _pAnim, class CXR_Model* _pModel, class CXR_AnimState *_pAnimState)
{	
	MAUTOSTRIP(CRC_Util2D_Model_3, MAUTOSTRIP_VOID);
	if (_pModel == NULL)
		return;
	
	CXR_Skeleton* pSkeleton = (CXR_Skeleton*)_pModel->GetParam(MODEL_PARAM_SKELETON);
	
	// Return success, for we have a model, but it has no skeleton, and is thus not skeleton animated.
	// Though the AnimState is properly default initialised for a regular non-skeleton animated model.
	if (pSkeleton == NULL)
		return ; 

	CMat4Dfp32 Mat;
	Mat.Unit();

	CBox3Dfp32 Box; 
	_pModel->GetBound_Box(Box);
	
	CVec3Dfp32 middle;
	Box.GetCenter(middle); 
	middle[0] = -middle[0];
	middle[1] = -middle[1];
	middle[2] = -middle[2];
	middle.AddMatrixRow(Mat, 3); 

	if(_pAnim)
	{
		// Eval animation
		CXR_AnimLayer Layer;
		Layer.Create2(_pAnim, _pAnimState->m_Anim0, _pAnimState->m_AnimTime1, 1.0f, 1.0f, 0);
		pSkeleton->EvalAnim(&Layer, 1, _pAnimState->m_pSkeletonInst, Mat);
	}

	// =====================================================================================

//	int x0 = 0;
//	int y0 = 0;
//	int x1 = _Rect.GetWidth();
//	int y1 = _Rect.GetHeight();
	
	CRenderContext* pRC = GetRC();
	
//	pRC->Attrib_Push();
	
	GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	GetAttrib()->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
	GetAttrib()->Attrib_Enable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
	//		pRC->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
	//		Rect(_Clip, CRct(x0,y0,x1,y1), 0x00000000);
	//		Rect(_Clip, _Rect, 0x00000000);
	SetTexture(0);
	Rect(_Clip, _Rect, 0x00000000);
	
//	pRC->Attrib_Pop();
	
	CBox3Dfp32 Bound;
	_pModel->GetBound_Box(Bound, _pAnimState);
	
	CVec3Dfp32 Size;
	Bound.m_Max.Sub(Bound.m_Min, Size);
	fp32 SizeX = Length2(Size[0], Size[1]);
	fp32 SizeY = Size[2];

	//pRC->Viewport_Get()->GetFOV()
	const fp32 FOV = 20;

	CVec2Dfp32 Scale = GetCoordinateScale();
	CClipRect Clip = _Clip;
	CRct Rect = _Rect;
	Rect += Clip.ofs;
	Rect.p0.x = int(Rect.p0.x * Scale[0]);
	Rect.p0.y = int(Rect.p0.y * Scale[1]);
	Rect.p1.x = int(Rect.p1.x * Scale[0]);
	Rect.p1.y = int(Rect.p1.y * Scale[1]);
	Clip.clip.p0.x = int(Clip.clip.p0.x * Scale[0]);
	Clip.clip.p0.y = int(Clip.clip.p0.y * Scale[1]);
	Clip.clip.p1.x = int(Clip.clip.p1.x * Scale[0]);
	Clip.clip.p1.y = int(Clip.clip.p1.y * Scale[1]);
	Clip.ofs.x = int(Clip.ofs.x * Scale[0]);
	Clip.ofs.y = int(Clip.ofs.y * Scale[1]);

	Clip += m_CurVP.GetViewClip();
	Rect += m_CurVP.GetViewClip().ofs;

	CRC_Viewport View;
	View.SetView(Clip, Rect);
	View.SetFOV(FOV);
	
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys && pSys->m_spDisplay)
	{
		View.SetAspectRatio(pSys->m_spDisplay->GetPixelAspect());
	}

	fp32 Dist = SizeX * View.GetScale() * 0.33f;
	fp32 DistY = SizeY * View.GetScale() * 0.33f;
	
	fp32 w = Rect.GetWidth();
	fp32 h = Rect.GetHeight();

	if (w > h)
	{
		if (Dist / w > DistY / h)
			DistY = Dist / w * h;
	}
	else
	{
		if (Dist / w < DistY / h)
			Dist = DistY / h * w;
	}

//	fp32 Dist = Max(SizeX / m_Pos.GetWidth(), SizeY / m_Pos.GetHeight());

	CMat4Dfp32 Camera;
	Camera.Unit();
	Camera.RotX_x_M(0.25f);
	Camera.RotY_x_M(-0.25f);
	Camera.RotZ_x_M(0.5f);
	Camera.k[3][0] = Max(Dist, DistY) * 1.0f;
	Camera.k[3][2] = 0;


	//Mat.RotZ_x_M(-_pAnimState->m_AnimTime1 * 0.1);

	CVec3Dfp32 Center;
	Bound.GetCenter(Center);
	Center.MultiplyMatrix3x3(Mat);
	CVec3Dfp32::GetMatrixRow(Mat, 3) -= Center;

	if (m_pVBM->Viewport_Push(&View))
	{
		CMat4Dfp32 VMat;
		Camera.InverseOrthogonal(VMat);
		_pModel->OnRender(_pEngine, pRC, m_pVBM, this, m_spWorldLightState, _pAnimState, Mat, VMat, CXR_MODEL_ONRENDERFLAGS_MAXLOD);

		m_pVBM->Viewport_Pop();
	}
}


void CRC_Util2D::Model(class CWorld_Client* _pWClient, const CClipRect& _Clip, const CRct& _Rect, CXR_Anim_Base* _pAnim, class CXR_Model* _pModel, class CXR_AnimState *_pAnimState)
{
	MAUTOSTRIP(CRC_Util2D_Model_4, MAUTOSTRIP_VOID);
	Model(NULL, _pWClient, _Clip, _Rect, _pAnim, _pModel, _pAnimState);

}
#endif

CRC_Attributes* CRC_Util2D::GetAttrib()
{
	if(m_pVBM == 0)
		Error("CRC_Util2D::GetAttrib", "Attrib manipulation only works if you have a CXR_VBManager assigned to the CRC_Util2D class");
	m_pLastSubmittedAttrib	= 0;
	return &m_Attrib;
}

CRC_Attributes* CRC_Util2D::GetLastSubmitted()
{
	M_ASSERT(m_pVBM, "Must have a CXR_VBManager attached to use CRC_Util2D::GetLastSubmitted");
	if(!m_pLastSubmittedAttrib)
	{
		m_pLastSubmittedAttrib	= m_pVBM->Alloc_Attrib();
		if(m_pLastSubmittedAttrib)
			*m_pLastSubmittedAttrib	= m_Attrib;
	}

	return m_pLastSubmittedAttrib;
}
