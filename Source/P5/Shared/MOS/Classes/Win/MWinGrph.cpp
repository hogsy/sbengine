#include "PCH.h"
#include "MWinGrph.h"
#include "../../XR/XR.h"
#include "../../MSystem/Misc/MLocalizer.h"
//#include "../../Classes/Win/MWindows.h"
//#include "../../XR/XRShader.h"

#ifdef COMPILER_CODEWARRIOR
//	#ifdef COMPILER_RTTI
		#include <typeinfo.h>
//	#endif
#endif

uint32 CRC_Util2D::ms_IndexRamp[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
uint16 CRC_Util2D::ms_DualTringle[6] = { 0,1,2,0,2,3 };

// -------------------------------------------------------------------
int CRC_Util2D::GetTCIndex(CTextureContainer* _pTC)
{
	MAUTOSTRIP(CRC_Util2D_GetTCIndex, 0);
	int nTC = m_lspTC.Len();
	for(int i = 0; i < nTC; i++)
		if (_pTC == (CTextureContainer*)m_lspTC[i]) return i;
	return -1;
}


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
	m_CurTextureScale.k[0] = 1.0f;
	m_CurTextureScale.k[1] = 1.0f;
	m_CurFontScale = 1.0f;
	m_VBPriority = 0;
	m_spWorldLightState = NULL;
	m_SurfOptions = 0;
	m_SurfCaps = 0;
	SetPriorityValues(0);
}


void CRC_Util2D::SetPriorityValues(fp4 _PriorityBase, fp4 _PriorityAdd)
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
void CRC_Util2D::AddTxtContainer(spCTextureContainer _spTC)
{
	MAUTOSTRIP(CRC_Util2D_AddTxtContainer, MAUTOSTRIP_VOID);
	int iTC = GetTCIndex(_spTC);
	if (iTC > 0)
		m_lTCRef[iTC]++;
	else
	{
		iTC = m_lspTC.Add(_spTC);
		M_TRY
		{ 
			m_lTCRef.Add(1);  
		}
		M_CATCH(
		catch(CCException)
		{ 
			m_lspTC.Del(iTC); throw; 
		}
		)
	}
}


void CRC_Util2D::RemoveTxtContainer(spCTextureContainer _spTC)
{
	MAUTOSTRIP(CRC_Util2D_RemoveTxtContainer, MAUTOSTRIP_VOID);
	int iTC = GetTCIndex(_spTC);
	if (iTC < 0) return;

	if (--m_lTCRef[iTC] == 0)
	{
		m_lspTC.Del(iTC);
		m_lTCRef.Del(iTC);
	}
}


// -------------------------------------------------------------------
void CRC_Util2D::UpdateTransform()
{
	MAUTOSTRIP(CRC_Util2D_UpdateTransform, MAUTOSTRIP_VOID);
	CRct View = m_CurVP.GetViewRect();
	CClipRect Clip = m_CurVP.GetViewClip();
//	fp4 cw = Clip.clip.GetWidth();
//	fp4 ch = Clip.clip.GetHeight();
	fp4 w = View.GetWidth();
	fp4 h = View.GetHeight();
	fp4 dx = -w;
	fp4 dy = -h;

	m_CurTransform.Unit();
//	CVec3Dfp4::GetMatrixRow(m_CurTransform, 3) = CVec3Dfp4(dx/2.0f, dy/2.0f, _pVP->GetXScale()*0.5f);
//	const fp4 Z = 16.0f;
	const fp4 Z = m_CurVP.GetBackPlane() - 16.0f;

	fp4 xScale = m_CurVP.GetXScale() * 0.5f;
	fp4 yScale = m_CurVP.GetYScale() * 0.5f;

	m_CurTransform.k[0][0] = Z / xScale * m_CurScale.k[0];
	m_CurTransform.k[1][1] = Z / yScale * m_CurScale.k[1];
	m_CurTransform.k[3][0] = (Z*((dx)*(1.0f/2.0f) - m_TextOffset) / xScale);
	m_CurTransform.k[3][1] = (Z*((dy)*(1.0f/2.0f) - m_TextOffset) / yScale);
	m_CurTransform.k[3][2] = Z;

//	CVec3Dfp4::GetMatrixRow(m_CurTransform, 3) = CVec3Dfp4(dx/2.0f, dy/2.0f, _pVP->GetXScale()*0.5f);

//	m_CurTransform.k[2][0] += 0.375f;
//	m_CurTransform.k[2][1] += 0.375f;
	
//	m_CurTransform.k[2][0] += 10;
//	m_CurTransform.k[2][1] += 10;
}


void CRC_Util2D::Begin(CRenderContext* _pRC, CRC_Viewport* _pVP, CXR_VBManager* _pVBM)
{
	MAUTOSTRIP(CRC_Util2D_Begin, MAUTOSTRIP_VOID);
	Clear();
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


void CRC_Util2D::SetCoordinateScale(const CVec2Dfp4& _Scale)
{
	MAUTOSTRIP(CRC_Util2D_SetCoordinateScale, MAUTOSTRIP_VOID);
	m_CurScale = _Scale;
	UpdateTransform();
}

void CRC_Util2D::SetWorldLightState(spCXR_WorldLightState _spLightState)
{
	MAUTOSTRIP(CRC_Util2D_SetWorldLightState, MAUTOSTRIP_VOID);
	m_spWorldLightState = _spLightState;
}


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


void CRC_Util2D::SetTransform(const CMat4Dfp4& _Pos, const CVec2Dfp4& _Scale)
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
	m_pSurfKeyFrame = _pSurf->GetFrame(_Sequence, _AnimTime, *m_spTmpSurfKey);
//	m_pCurAttrib = NULL;
	m_pLastSubmittedAttrib	= 0;

	CVec3Dfp4 ReferenceDim = _pSurf->GetTextureMappingReferenceDimensions();
	m_CurTxtW = int(ReferenceDim[0]);
	m_CurTxtH = int(ReferenceDim[1]);
}


bool CRC_Util2D::SetTexture(const char* _pTxtName)
{
	MAUTOSTRIP(CRC_Util2D_SetTexture_2, false);
	SetTexture(m_pTC->GetTextureID(_pTxtName));
	return m_Attrib.m_TextureID[0] != 0;

/*	int nTC = m_lspTC.Len();
	for(int i = 0; i < nTC; i++)
	{
		if (typeid(*m_lspTC[i]) == typeid(CTextureContainer_Plain))
		{
			CTextureContainer_Plain* pTC = TDynamicCast<CTextureContainer_Plain> (&(*m_lspTC[i]));
			int iLocal;
			if ((iLocal = pTC->GetLocal(_pTxtName)) >= 0)
			{
				SetTexture(m_lspTC[i]->GetTextureID(iLocal));
				return true;
			}
		}
	}
	return false;*/
}


void CRC_Util2D::SetTextureOrigo(const CClipRect& _Clip, const CPnt& _Origo)
{
	MAUTOSTRIP(CRC_Util2D_SetTextureOrigo, MAUTOSTRIP_VOID);
	m_CurTextureOrigo = _Origo + _Clip.ofs;
}


void CRC_Util2D::SetTextureScale(fp4 _xScale, fp4 _yScale)
{
	MAUTOSTRIP(CRC_Util2D_SetTextureScale, MAUTOSTRIP_VOID);
	m_CurTextureScale.k[0] = _xScale;
	m_CurTextureScale.k[1] = _yScale;
}


void CRC_Util2D::SetFontScale(fp4 _xScale, fp4 _yScale)
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
	/*
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

//	CVec3Dfp4 Verts[4];
	CVec3Dfp4 Verts3D[4];
	CVec2Dfp4 TVerts[4];
	CPixel32 Colors[4];
	CVec3Dfp4 *pV = Verts3D;
	CVec2Dfp4 *pTV = TVerts;
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
	CVec3Dfp4::MultiplyMatrix(pV, pV, m_CurTransform, 4);

	fp4 xs = 1.0f/fp4(m_CurTxtW) * m_CurTextureScale.k[0];
	fp4 ys = 1.0f/fp4(m_CurTxtH) * m_CurTextureScale.k[1];
	int txtox = m_CurTextureOrigo.x;
	int txtoy = m_CurTextureOrigo.y;
	pTV[0][0] = fp4(Rect.p0.x - txtox) * xs;
	pTV[0][1] = fp4(Rect.p0.y - txtoy) * ys;
	pTV[1][0] = fp4(Rect.p1.x - txtox) * xs;
	pTV[1][1] = fp4(Rect.p0.y - txtoy) * ys;
	pTV[2][0] = fp4(Rect.p1.x - txtox) * xs;
	pTV[2][1] = fp4(Rect.p1.y - txtoy) * ys;
	pTV[3][0] = fp4(Rect.p0.x - txtox) * xs;
	pTV[3][1] = fp4(Rect.p1.y - txtoy) * ys;
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
			if (!pVB->SetVBChain(m_pVBM, false))
				return;
			pVB->Geometry_VertexArray(pV, 4, true);
			pVB->Geometry_TVertexArray(pTV, 0);
			pVB->Render_IndexedTriangles(ms_DualTringle, 2);

			if(m_pSurf)
			{
				pVB->Geometry_ColorArray(pC);
				CXR_Util::Render_Surface(0, m_pSurf, m_pSurfKeyFrame, _pEngine, m_pVBM, NULL, NULL, (CMat43fp4*)NULL, pVB, m_VBPriority, 0.0001f);
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
	*/
}

void CRC_Util2D::Rect(const CClipRect& _Clip, const CRct& _Rect, const CPixel32& _Color, class CXR_Shader* _pShader, const class CXR_ShaderParams* _pShaderParams, const CXR_Light& _Light)
{
	MAUTOSTRIP(CRC_Util2D_Rect, MAUTOSTRIP_VOID);
}

#ifndef M_RTM
void CRC_Util2D::Circle(const CClipRect& _Clip, const CVec2Dfp4& _Mid, fp4 _Radius, int32 _nSegments, const CPixel32& _Color, bool _bBorder, const CPixel32& _BorderColor)
{
	MAUTOSTRIP(CRC_Util2D_Rect, MAUTOSTRIP_VOID);
	if (!m_pVBM)
		return;

	// Num segs plus middle point
	int32 NumPoints = _nSegments + 1;

	CVec3Dfp4 *pV = m_pVBM->Alloc_V3(NumPoints);
	/*CVec3Dfp4 *pN = m_pVBM->Alloc_V3(NumPoints);
	CVec2Dfp4 *pTV = m_pVBM->Alloc_V2(NumPoints);
	CVec3Dfp4 *pTangU = m_pVBM->Alloc_V3(NumPoints);
	CVec3Dfp4 *pTangV = m_pVBM->Alloc_V3(NumPoints);*/
	uint16* pIndex = m_pVBM->Alloc_Int16(_nSegments * 3);
	CPixel32 *pC = m_pVBM->Alloc_CPixel32(NumPoints);

	if(!pV || !pIndex || !pC)
		return;

	// Calc circle points (yehyeh, this is a debug function anyway...)
	pV[0].k[0] = _Mid[0];
	pV[0].k[1] = _Mid[1];
	pV[0].k[2] = 0.0f;
	pC[0] = _Color;

	fp4 Part = 2*_PI / (fp4)_nSegments;
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

	CVec3Dfp4::MultiplyMatrix(pV, pV, m_CurTransform, NumPoints);

	CMat4Dfp4* pMat = m_pVBM->Alloc_M4();

	CXR_VertexBuffer *pVB = m_pVBM->Alloc_VB();
	if(pVB && pMat)
	{
		pMat->Unit();
		if (!pVB->SetVBChain(m_pVBM, false))
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

void CRC_Util2D::Line(const CClipRect& _Clip, const CVec2Dfp4& _Start, CVec2Dfp4& _End, fp4 _Width, const CPixel32& _Color)
{
	if (!m_pVBM)
		return;

	// Draw a line from startpoint to endpoint

	// Num segs plus middle point

	CVec3Dfp4 *pV = m_pVBM->Alloc_V3(4);
	CPixel32 *pC = m_pVBM->Alloc_CPixel32(4);

	if(!pV || !pC)
		return;

	CVec2Dfp4 Dir = _End - _Start;
	Dir.Normalize();
	CVec2Dfp4 Right;
	Right.k[0] = Dir.k[1];
	Right.k[1] = -Dir.k[0];

	// Set color
	pC[0] = _Color;
	pC[1] = _Color;
	pC[2] = _Color;
	pC[3] = _Color;

	CVec2Dfp4 Point =  _Start - Right * _Width * 0.5f;
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
	CVec3Dfp4::MultiplyMatrix(pV, pV, m_CurTransform, 4);

	CMat4Dfp4* pMat = m_pVBM->Alloc_M4();

	CXR_VertexBuffer *pVB = m_pVBM->Alloc_VB();
	if(pVB && pMat)
	{
		pMat->Unit();
		if (!pVB->SetVBChain(m_pVBM, false))
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

void CRC_Util2D::AspectRect(const CClipRect& _Clip, const CRct& _Rect, const CPnt& _SourceSize, fp4 _SourcePixelAspect, const CPixel32& _Color)
{
	int Width = _Rect.p1.x - _Rect.p0.x;
	int Height = _Rect.p1.y - _Rect.p0.y;
	fp4 OffsetX = 0;
	fp4 OffsetY = 0;

	fp4 PixelAspect = (GetRC()->GetDC()->GetPixelAspect() * GetRC()->GetDC()->GetScreenAspect() / (((fp4)Width / Height)*(3.0/4.0))) / _SourcePixelAspect;
	fp4 AspectX = PixelAspect * fp4(Width) / _SourceSize.x;
	fp4 AspectY = fp4(Height) / _SourceSize.y;
	if(AspectX < AspectY)
	{
		// Horizontal borders
		OffsetY = (Height - AspectX / AspectY * Height) / 2;
		fp4 Scale = _SourceSize.x / (fp4) Width;
		SetTextureScale(Scale, Scale/PixelAspect);
	}
	else
	{
		// Vertical borders
		OffsetX = (Width - AspectY / AspectX * Width) / 2;
		fp4 Scale = _SourceSize.y / (fp4) Height;
		SetTextureScale(Scale* PixelAspect, Scale);
	}

	SetTextureOrigo(_Clip, CPnt(int(_Rect.p0.x + OffsetX), int(_Rect.p0.y + OffsetY)));
	Rect(_Clip, CRct::From_fp4(_Rect.p0.x + OffsetX, _Rect.p0.y + OffsetY, _Rect.p1.x - OffsetX, _Rect.p1.y - OffsetY),  0xffffffff);
}

bool CRC_Util2D::DrawTexture(const CClipRect& _Clip, const CPnt& _Pos, const char *_pTexture, const CPixel32 _Color, const CVec2Dfp4& _Scale)
{
	if(!SetTexture(_pTexture))
		return false;
	
	SetTextureOrigo(_Clip, _Pos);
	SetTextureScale(_Scale[0], _Scale[1]);
	Rect(_Clip, CRct::From_fp4(_Pos.x, _Pos.y, _Pos.x + GetTextureWidth() / _Scale[0], _Pos.y + GetTextureHeight() / _Scale[1]), _Color);
	return true;
}

void CRC_Util2D::DrawSurface(const CClipRect& _Clip, const CPnt& _Pos, const char *_pSurface, const CPixel32 _Color, const CVec2Dfp4& _Scale)
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

void CRC_Util2D::RotatedSprite(const CClipRect& _Clip, const CPnt& _Pos, const CPnt& _Range,  fp4 _Angle, const CPixel32& _Color)
{
	/*
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


	CVec3Dfp4 Verts3D[4];
	CVec2Dfp4 TVerts[4];
	CPixel32 Colors[4];
	CVec3Dfp4 *pV = Verts3D;
	CVec2Dfp4 *pTV = TVerts;
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
	CMat4Dfp4 InMat, OutMat, Rotate;
	InMat.Unit();
	OutMat.Unit();
	Rotate.Unit();

	CVec3Dfp4(-_Pos.x, -_Pos.y,0).SetMatrixRow(InMat,3);
	CVec3Dfp4(_Pos.x, _Pos.y,0).SetMatrixRow(OutMat,3);
	Rotate.SetZRotation3x3(_Angle);
	InMat.Multiply(Rotate, Rotate);
	Rotate.Multiply(OutMat, Rotate);
	Rotate.Multiply(m_CurTransform.Get4x4(), Rotate);
	
	// Multiply to temporary vertices otherwise rotations may be f#)&/%# up
	CVec3Dfp4 TempVerts[4];
	CVec3Dfp4::MultiplyMatrix(pV, TempVerts, Rotate, 4);
	// Copy the temporary vertices back
	memcpy(pV, TempVerts, sizeof(CVec3Dfp4)*4);

	fp4 xs = 1.0f/fp4(m_CurTxtW) * m_CurTextureScale.k[0];
	fp4 ys = 1.0f/fp4(m_CurTxtH) * m_CurTextureScale.k[1];
	int txtox = m_CurTextureOrigo.x;
	int txtoy = m_CurTextureOrigo.y;
	pTV[0][0] = fp4(Rect.p0.x - txtox) * xs;
	pTV[0][1] = fp4(Rect.p0.y - txtoy) * ys;
	pTV[1][0] = fp4(Rect.p1.x - txtox) * xs;
	pTV[1][1] = fp4(Rect.p0.y - txtoy) * ys;
	pTV[2][0] = fp4(Rect.p1.x - txtox) * xs;
	pTV[2][1] = fp4(Rect.p1.y - txtoy) * ys;
	pTV[3][0] = fp4(Rect.p0.x - txtox) * xs;
	pTV[3][1] = fp4(Rect.p1.y - txtoy) * ys;
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
			if (!pVB->SetVBChain(m_pVBM, false))
				return;
			pVB->Geometry_VertexArray(pV, 4, true);
			pVB->Geometry_TVertexArray(pTV, 0);
//			pVB->Geometry_ColorArray(pC);
			pVB->Geometry_Color(_Color);
			pVB->Render_IndexedTriangles(ms_DualTringle, 2);

			if(m_pSurf)
				CXR_Util::Render_Surface(0, m_pSurf, m_pSurfKeyFrame, NULL, m_pVBM, (CMat43fp4*)NULL, (CMat43fp4*)NULL, (CMat43fp4*)NULL, pVB, m_VBPriority, 0.0001f);
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
	*/
}

void CRC_Util2D::ScaleSprite(const CClipRect& _Clip, const CPnt& _Pos, const CPnt& _Size, const CPixel32& _Color)
{
	MAUTOSTRIP(CRC_Util2D_ScaleSprite, MAUTOSTRIP_VOID);
	if ((_Size.x == 0) || (_Size.y == 0)) return;
	if (m_Attrib.m_TextureID[0] == 0) return;

//	GetRC()->Attrib_Push();

//	GetRC()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	SetTextureOrigo(_Clip, _Pos);
	m_CurTextureScale.k[0] = m_CurTxtW/fp4(_Size.x);
	m_CurTextureScale.k[1] = m_CurTxtH/fp4(_Size.y);
	Rect(_Clip, CRct(_Pos, _Pos + _Size), _Color);

	m_CurTextureScale.k[0] = 1.0f;
	m_CurTextureScale.k[1] = 1.0f;
//	GetRC()->Attrib_Pop();
}

void CRC_Util2D::Lines(const CClipRect& _Clip, const CPnt *_p0, const CPnt *_p1, const CPixel32 *_Color0, const CPixel32 *_Color1, int _nLines)
{

	if(m_pVBM)
	{
		CXR_VertexBuffer *pVB = m_pVBM->Alloc_VB(CXR_VB_VERTICES|CXR_VB_COLORS, _nLines * 2);
		if(!pVB)
			return;
		uint16 *pInd = (uint16 *)m_pVBM->Alloc_Int16(_nLines * 2);
		if (!pInd)
			return;

		CXR_VBChain *pVBChain = pVB->GetVBChain();

		CVec3Dfp4 Verts[2];
		int iDest = 0;
		for (int i = 0; i < _nLines; ++i)
		{
			CVec3Dfp4 *Verts3D = pVBChain->m_pV + i * 2;

			CPnt p0 = _p0[i];
			CPnt p1 = _p1[i];

			p0 += _Clip.ofs;
			p1 += _Clip.ofs;

			if (!CImage::ClipLine(_Clip.clip, p0.x, p0.y, p1.x, p1.y)) 
				continue;

			pVBChain->m_pCol[i * 2] = _Color0[i];
			pVBChain->m_pCol[i * 2 + 1] = _Color1[i];

			Verts[0] = CVec3Dfp4(p0.x, p0.y, 0);
			Verts[1] = CVec3Dfp4(p1.x, p1.y, 0);
			CVec3Dfp4::MultiplyMatrix(Verts, Verts3D, m_CurTransform, 2);
			pInd[iDest++] = i * 2;
			pInd[iDest++] = i * 2 + 1;
		}

		GetAttrib()->Attrib_TextureID(0, 0);

		if (!iDest)
			return;

		pVB->m_Priority = (m_VBPriority += m_VBPriorityAdd);
		pVB->Render_IndexedWires(pInd, iDest);
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
			Line(_Clip, _p0[i], _p1[i], _Color0[i]);
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

		CVec3Dfp4 Verts[2];
		CVec3Dfp4 *Verts3D = pVBChain->m_pV;

		CPnt p0 = _p0;
		CPnt p1 = _p1;

		p0 += _Clip.ofs;
		p1 += _Clip.ofs;

		if (!CImage::ClipLine(_Clip.clip, p0.x, p0.y, p1.x, p1.y)) return;

		Verts[0] = CVec3Dfp4(p0.x, p0.y, 0);
		Verts[1] = CVec3Dfp4(p1.x, p1.y, 0);
		CVec3Dfp4::MultiplyMatrix(&Verts[0], &Verts3D[0], m_CurTransform, 2);
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
		CVec3Dfp4 Verts[4];
		CVec3Dfp4 Verts3D[4];

	//	if (!_Clip.Visible(CRct(_p0, _p1))) return;

		CPnt p0 = _p0;
		CPnt p1 = _p1;

		p0 += _Clip.ofs;
		p1 += _Clip.ofs;

		if (!CImage::ClipLine(_Clip.clip, p0.x, p0.y, p1.x, p1.y)) return;

		Verts[0] = CVec3Dfp4(p0.x, p0.y, 0);
		Verts[1] = CVec3Dfp4(p1.x, p1.y, 0);
		CVec3Dfp4::MultiplyMatrix(&Verts[0], &Verts3D[0], m_CurTransform, 2);
		m_pCurRC->Attrib_TextureID(0, 0);
		m_pCurRC->Render_Wire(Verts3D[0], Verts3D[1], _Color);
	}

/*
	CVec2Dfp4 LineN((p1.y - p0.y), -(p1.x - p0.x));
	LineN.Normalize();

	Verts[0] = CVec3Dfp4(p0.x, p0.y, 0);
	Verts[1] = CVec3Dfp4(p1.x, p1.y, 0);
	Verts[2] = CVec3Dfp4(fp4(p1.x) + LineN.k[0], fp4(p1.y) + LineN.k[1], 0);
	Verts[3] = CVec3Dfp4(fp4(p0.x) + LineN.k[0], fp4(p0.y) + LineN.k[1], 0);

	CVec2Dfp4 TVerts[4];
	if (m_CurTextureID)
	{
		fp4 xs = 1.0f/fp4(m_CurTxtW) * m_CurTextureScale.k[0];
		fp4 ys = 1.0f/fp4(m_CurTxtH) * m_CurTextureScale.k[1];
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

	CVec3Dfp4::MultiplyMatrix(&Verts[0], &Verts3D[0], m_CurTransform, 4);
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


int CRC_Util2D::TextHeight(CRC_Font* _pFont, const char* _pStr)
{
	MAUTOSTRIP(CRC_Util2D_TextHeight, 0);
	if (_pStr)
		return int(_pFont->GetHeight(_pFont->GetOriginalSize(), _pStr));
	else
		return int(_pFont->GetOriginalSize());
}


int CRC_Util2D::TextHeight(CRC_Font* _pFont, const wchar* _pStr)
{
	MAUTOSTRIP(CRC_Util2D_TextHeight_2, 0);
	if (_pStr)
		return int(_pFont->GetHeight(_pFont->GetOriginalSize(), _pStr));
	else
		return int(_pFont->GetOriginalSize());
}


int CRC_Util2D::TextHeight(CRC_Font* _pFont, const CStr& _Str)
{
	MAUTOSTRIP(CRC_Util2D_TextHeight_3, 0);
	if (_Str.Len())
		return int(_pFont->GetHeight(_pFont->GetOriginalSize(), _Str));
	else
		return int(_pFont->GetOriginalSize());
}


int CRC_Util2D::TextWidth(CRC_Font* _pFont, const char* _pStr)
{
	MAUTOSTRIP(CRC_Util2D_TextWidth, 0);
	return int(_pFont->GetWidth(_pFont->GetOriginalSize(), _pStr));
}


int CRC_Util2D::TextWidth(CRC_Font* _pFont, const wchar* _pStr)
{
	MAUTOSTRIP(CRC_Util2D_TextWidth_2, 0);
	return int(_pFont->GetWidth(_pFont->GetOriginalSize(), _pStr));
}

int CRC_Util2D::TextWidth(CRC_Font* _pFont,const CStr& _Str)
{
	MAUTOSTRIP(CRC_Util2D_TextWidth_3, 0);
	return int(_pFont->GetWidth(_pFont->GetOriginalSize(), _Str));
}


int CRC_Util2D::TextFit(CRC_Font* _pFont, const char* _pStr, int _Width, bool _bWordWrap)
{
	MAUTOSTRIP(CRC_Util2D_TextFit, 0);
	return _pFont->GetFit(_pFont->GetOriginalSize(), _pStr, _Width, _bWordWrap);
}


int CRC_Util2D::TextFit(CRC_Font* _pFont, const wchar* _pStr, int _Width, bool _bWordWrap)
{
	MAUTOSTRIP(CRC_Util2D_TextFit_2, 0);
	return _pFont->GetFit(_pFont->GetOriginalSize(), _pStr, _Width, _bWordWrap);
}


int CRC_Util2D::TextFit(CRC_Font* _pFont, const CStr& _Str, int _Width, bool _bWordWrap)
{
	MAUTOSTRIP(CRC_Util2D_TextFit_3, 0);
	return _pFont->GetFit(_pFont->GetOriginalSize(), _Str, _Width, _bWordWrap);
}

CFStr FormatFloat(fp4 _Value)
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

void CRC_Util2D::DrawGraph(const CClipRect& _Clip, CRC_Font* _pF, const CRct &_Rect, const CPixel32 &_Background, const CPixel32 &_ColorLow, const CPixel32 &_ColorMid, const CPixel32 &_ColorHigh, fp4 _AvgValue, const fp4 *_pValues, int _nValues, fp4 _OriginalHeight, const ch8 *_pName)
{

	CRC_Attributes *pAttr = GetAttrib();
	if (!pAttr)
		return;

	if (!m_pVBM)
		return;

	fp4 X = _Rect.p1.x; 
	int Bottom = _Rect.p1.y-2;

	int nToDraw = _nValues;

	int HistPtr = _nValues -1;
	fp4 Step = fp4(_Rect.GetWidth()-2) / nToDraw - 1;

	pAttr->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
	pAttr->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	CPnt *Line00 = (CPnt *)m_pVBM->Alloc((nToDraw*2+3) * sizeof(CPnt));
	CPnt *Line01 = (CPnt *)m_pVBM->Alloc((nToDraw*2+3) * sizeof(CPnt));
	CPixel32 *ColorLine0 = (CPixel32 *)m_pVBM->Alloc((nToDraw*2+3) * sizeof(CPixel32));
	CPixel32 *ColorLine1 = (CPixel32 *)m_pVBM->Alloc((nToDraw*2+3) * sizeof(CPixel32));

	if (!Line00 || !Line01 || !ColorLine0 || !ColorLine1)
		return;

	CRct Rects[2];
	CPixel32 ColorRect[2];

	int iRectStart = 0;

	fp4 ValueScale = 1.0;
	fp4 TargetScale = 1.0;

	fp4 AvgValue = _AvgValue;

	fp4 Target = _OriginalHeight * 1.16666666666666f;

	while (AvgValue * TargetScale > Target)
	{
		TargetScale *= 0.5f;
	}	


	fp4 HeightFull = _Rect.GetHeight()-2;
	fp4 Height = HeightFull - 27.0f;
	fp4 Height100 = Height / 1.25f;
	fp4 Height50 = Height100 * 0.5f;

	ValueScale = Height100 / (_OriginalHeight / TargetScale);

	fp4 LastValue = _pValues[HistPtr] * ValueScale;
	fp4 CurrentValue = _pValues[HistPtr];
	
	int StartX = int(X-60);
	Rects[iRectStart] = CRct::From_fp4(StartX - nToDraw * Step-2, Bottom - HeightFull, X, Bottom+2);
	X -= 60;
	ColorRect[iRectStart] = _Background;
	++iRectStart;

	int iStart = 0;
	ColorLine0[iStart] = CPixel32(0,0,55,255);
	ColorLine1[iStart] = CPixel32(0,0,55,255);
	Line00[iStart] = CPnt::From_fp4(X - (nToDraw) * Step, Bottom);
	Line01[iStart] = CPnt::From_fp4(X, Bottom);
	++iStart;

	ColorLine0[iStart] = CPixel32(200,200,255,255);
	ColorLine1[iStart] = CPixel32(200,200,255,255);
	Line00[iStart] = CPnt::From_fp4(X - (nToDraw) * Step, Bottom+1);
	Line01[iStart] = CPnt::From_fp4(X, Bottom+1);
	++iStart;

	ColorLine0[iStart] = CPixel32(128,128,128,255);
	ColorLine1[iStart] = CPixel32(128,128,128,255);
	Line00[iStart] = CPnt::From_fp4(X - (nToDraw) * Step, Bottom - Height50);
	Line01[iStart] = CPnt::From_fp4(X, Bottom - Height50);
	++iStart;

	ColorLine0[iStart] = CPixel32(200,200,200,255);
	ColorLine1[iStart] = CPixel32(200,200,200,255);
	Line00[iStart] = CPnt::From_fp4(X - (nToDraw) * Step, Bottom - Height100);
	Line01[iStart] = CPnt::From_fp4(X, Bottom - Height100);
	++iStart;

	CClipRect ClipRect = CRct::From_fp4(X - nToDraw * Step - 2, Bottom - Height - 1-2, X, Bottom+2);
	for (int i = 0; i < nToDraw-1; ++i)
	{
		--HistPtr;

		fp4 Value = _pValues[HistPtr] * ValueScale;
		Line00[iStart] = CPnt::From_fp4(X - Step-1, Bottom - Value - 1);
		Line01[iStart] = CPnt::From_fp4(X-1, Bottom - LastValue - 1);
		ColorLine0[iStart] = CPixel32::LinearRGBA(_ColorLow, _ColorMid,	Min(256, int((Value / Height50) * 256.0)));
		ColorLine1[iStart] = CPixel32::LinearRGBA(_ColorLow, _ColorMid,	Min(256, int((LastValue / Height50) * 256.0)));
		ColorLine0[iStart] = CPixel32::LinearRGBA(ColorLine0[iStart], _ColorHigh, Min(256, int((Max((Value - Height50) / Height50, 0.0f)) * 256.0)));
		ColorLine1[iStart] = CPixel32::LinearRGBA(ColorLine1[iStart], _ColorHigh, Min(256, int((Max((LastValue - Height50) / Height50, 0.0f)) * 256.0)));
		ColorLine0[iStart] = CPixel32::LinearRGBA(ColorLine0[iStart], CPixel32(0,0,0,255), 100);
		ColorLine1[iStart] = CPixel32::LinearRGBA(ColorLine1[iStart], CPixel32(0,0,0,255), 100);
		++iStart;

		Line00[iStart] = CPnt::From_fp4(X - Step, Bottom - Value);
		Line01[iStart] = CPnt::From_fp4(X, Bottom - LastValue);
		ColorLine0[iStart] = CPixel32::LinearRGBA(_ColorLow, _ColorMid,	Min(256, int((Value / Height50) * 256.0)));
		ColorLine1[iStart] = CPixel32::LinearRGBA(_ColorLow, _ColorMid,	Min(256, int((LastValue / Height50) * 256.0)));
		ColorLine0[iStart] = CPixel32::LinearRGBA(ColorLine0[iStart], _ColorHigh, Min(256, int((Max((Value - Height50) / Height50, 0.0f)) * 256.0)));
		ColorLine1[iStart] = CPixel32::LinearRGBA(ColorLine1[iStart], _ColorHigh, Min(256, int((Max((LastValue - Height50) / Height50, 0.0f)) * 256.0)));

		++iStart;

		LastValue = Value;

		X -= Step;
	}

//	Rects(_Clip, Rects, ColorRect, iRectStart);
	this->Rects(_Clip, Rects, ColorRect, iRectStart);
	Lines(ClipRect, Line00, Line01, ColorLine0, ColorLine1, iStart);

	Text(_Clip, _pF, int(X + 5), int(Bottom - HeightFull) + 5, _pName, CPixel32(255,255,255,255), _pF->GetOriginalSize() * 2.0);
	Text(_Clip, _pF, int(StartX + 10), int(Bottom - Height), FormatFloat(_OriginalHeight / TargetScale).Str(), CPixel32(255,255,255,255), _pF->GetOriginalSize() * 2.0);
	Text(_Clip, _pF, int(StartX + 10), int(Bottom - Height)+20, FormatFloat(AvgValue).Str(), CPixel32(255,255,255,255), _pF->GetOriginalSize() * 2.0);
	Text(_Clip, _pF, int(StartX + 10), int(Bottom - Height)+40, FormatFloat(CurrentValue).Str(), CPixel32(255,255,255,255), _pF->GetOriginalSize() * 2.0);
}

void CRC_Util2D::Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const char* _pStr, const CPixel32& _Color, fp4 _Size)
{
	MAUTOSTRIP(CRC_Util2D_Text, MAUTOSTRIP_VOID);
	_x0 += _Clip.ofs.x;
	_y0 += _Clip.ofs.y;

	CVec3Dfp4 v(_x0, _y0, 0);
	v *= m_CurTransform;

	CVec3Dfp4 xv = CVec3Dfp4::GetMatrixRow(m_CurTransform, 0) * m_CurFontScale[0];
	CVec3Dfp4 yv = CVec3Dfp4::GetMatrixRow(m_CurTransform, 1) * m_CurFontScale[1];

	if (m_pVBM)
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB(CXR_VB_ATTRIB);
		if (!pVB) return;
		pVB->m_Priority = (m_VBPriority += m_VBPriorityAdd);
		if(GetLastSubmitted())
		{
			*pVB->m_pAttrib	= *GetLastSubmitted();

			if (_pFont->Write(m_pVBM, pVB, v, xv, yv, _pStr, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
				CVec2Dfp4(_Clip.clip.p0.x - _x0, _Clip.clip.p0.y - _y0),
				CVec2Dfp4(_Clip.clip.p1.x - _x0, _Clip.clip.p1.y - _y0)))
			{
				m_pVBM->AddVB(pVB);
			}
		}
	}
	else
	{
		_pFont->Write(m_pCurRC, v, xv, yv, _pStr, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
			CVec2Dfp4(_Clip.clip.p0.x - _x0, _Clip.clip.p0.y - _y0),
			CVec2Dfp4(_Clip.clip.p1.x - _x0, _Clip.clip.p1.y - _y0));
	}
}


void CRC_Util2D::Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const wchar* _pStr, const CPixel32& _Color, fp4 _Size)
{
	MAUTOSTRIP(CRC_Util2D_Text_2, MAUTOSTRIP_VOID);
	_x0 += _Clip.ofs.x;
	_y0 += _Clip.ofs.y;

	CVec3Dfp4 v(_x0, _y0, 0);
	v *= m_CurTransform;

	CVec3Dfp4 xv = CVec3Dfp4::GetMatrixRow(m_CurTransform, 0) * m_CurFontScale[0];
	CVec3Dfp4 yv = CVec3Dfp4::GetMatrixRow(m_CurTransform, 1) * m_CurFontScale[1];
//	fp4 screen_aspect = GetRC()->GetDC()->GetScreenAspect();
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
				CVec2Dfp4(_Clip.clip.p0.x - _x0, _Clip.clip.p0.y - _y0),
				CVec2Dfp4(_Clip.clip.p1.x - _x0, _Clip.clip.p1.y - _y0)))
			{
				m_pVBM->AddVB(pVB);
			}
		}
	}
	else
	{
		_pFont->Write(m_pCurRC, v, xv, yv, _pStr, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
			CVec2Dfp4(_Clip.clip.p0.x - _x0, _Clip.clip.p0.y - _y0),
			CVec2Dfp4(_Clip.clip.p1.x - _x0, _Clip.clip.p1.y - _y0));
	}
}


void CRC_Util2D::Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const CStr& _Str, const CPixel32& _Color, fp4 _Size)
{
	MAUTOSTRIP(CRC_Util2D_Text_3, MAUTOSTRIP_VOID);
	_x0 += _Clip.ofs.x;
	_y0 += _Clip.ofs.y;

	CVec3Dfp4 v(_x0, _y0, 0);
	v *= m_CurTransform;

	CVec3Dfp4 xv = CVec3Dfp4::GetMatrixRow(m_CurTransform, 0) * m_CurFontScale[0];
	CVec3Dfp4 yv = CVec3Dfp4::GetMatrixRow(m_CurTransform, 1) * m_CurFontScale[1];

	if (m_pVBM)
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB(CXR_VB_ATTRIB);
		if (!pVB) return;
		pVB->m_Priority = (m_VBPriority += m_VBPriorityAdd);
		if(GetLastSubmitted())
		{
			*pVB->m_pAttrib	= *GetLastSubmitted();

			if (_pFont->Write(m_pVBM, pVB, v, xv, yv, _Str, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
				CVec2Dfp4(_Clip.clip.p0.x - _x0, _Clip.clip.p0.y - _y0),
				CVec2Dfp4(_Clip.clip.p1.x - _x0, _Clip.clip.p1.y - _y0)))
			{
				m_pVBM->AddVB(pVB);
			}
		}
	}
	else
	{
		_pFont->Write(m_pCurRC, v, xv, yv, _Str, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
			CVec2Dfp4(_Clip.clip.p0.x - _x0, _Clip.clip.p0.y - _y0),
			CVec2Dfp4(_Clip.clip.p1.x - _x0, _Clip.clip.p1.y - _y0));
	}
}

// PC PORT: Print using float position
void CRC_Util2D::TextFloat(const CClipRect& _Clip, CRC_Font* _pFont, fp4 _x0, fp4 _y0, const CStr& _Str, const CPixel32& _Color, fp4 _Size)
{
	MAUTOSTRIP(CRC_Util2D_TextFloat, MAUTOSTRIP_VOID);

	const fp4 x0 = (fp4)_x0 + _Clip.ofs.x;
	const fp4 y0 = (fp4)_y0 + _Clip.ofs.y;

	CVec3Dfp4 v(x0, y0, 0.0f);
	v *= m_CurTransform;

	CVec3Dfp4 xv = CVec3Dfp4::GetMatrixRow(m_CurTransform, 0) * m_CurFontScale[0];
	CVec3Dfp4 yv = CVec3Dfp4::GetMatrixRow(m_CurTransform, 1) * m_CurFontScale[1];

	if (m_pVBM)
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB(CXR_VB_ATTRIB);
		if (!pVB) return;
		pVB->m_Priority = (m_VBPriority += m_VBPriorityAdd);
		if(GetLastSubmitted())
		{
			*pVB->m_pAttrib	= *GetLastSubmitted();

			if (_pFont->Write(m_pVBM, pVB, v, xv, yv, _Str, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
				CVec2Dfp4((fp4)_Clip.clip.p0.x - x0, (fp4)_Clip.clip.p0.y - y0),
				CVec2Dfp4((fp4)_Clip.clip.p1.x - x0, (fp4)_Clip.clip.p1.y - y0)))
			{
				m_pVBM->AddVB(pVB);
			}
		}
	}
	else
	{
		_pFont->Write(m_pCurRC, v, xv, yv, _Str, (_Size < 0) ? _pFont->GetOriginalSize() : _Size, _Color, 
			CVec2Dfp4((fp4)_Clip.clip.p0.x - x0, (fp4)_Clip.clip.p0.y - y0),
			CVec2Dfp4((fp4)_Clip.clip.p1.x - x0, (fp4)_Clip.clip.p1.y - y0));
	}
}
		
void CRC_Util2D::Text_Draw(const CClipRect& _Clip, CRC_Font* _pF, wchar* _pStr, int _x, int _y, int _Style, int ColM, int ColH, int ColD)
{
	MAUTOSTRIP(CRC_Util2D_Text_Draw, MAUTOSTRIP_VOID);

//	int bShw = _Style & WSTYLE_TEXT_SHADOW;
//	int bHL = _Style & WSTYLE_TEXT_HIGHLIGHT;
//	int bCut = _Style & WSTYLE_TEXT_CUTOUT;
	
	int xhl = _x;
	int yhl = _y;
	int xsh = 2+_x;
	int ysh = 2+_y;
//	if (bCut) { Swap(xhl, xsh); Swap(yhl, ysh); }
	
	int x = _x;
	int y = _y;
//	if (bShw || bHL) { x += 1; y += 1; }
	
//	if (bHL) Text(_Clip, _pF, xhl, yhl, _pStr, ColH);
//	if (bShw) Text(_Clip, _pF, xsh, ysh, _pStr, ColD);
	Text(_Clip, _pF, x, y, _pStr, ColM);
}


int CRC_Util2D::Text_WordWrap(CRC_Font* _pF, int _Width, wchar* _pStr, int _Len, wchar** _ppLines, int _MaxLines)
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
		int nFit = TextFit(_pF, &_ppLines[nLines][0], _Width, true);
		//LogFile(CStrF("Bla   %d, %d, %d, %d", nFit, last, _Len, n));
		if (!nFit) nFit = TextFit(_pF, &_ppLines[nLines][0], _Width, false);
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


int CRC_Util2D::Text_DrawFormatted(const CClipRect& _Clip, CRC_Font* _pF, const CStr& _Text, int& _x, int& _y, int _Style, int _ColM, int _ColH, int _ColD, int _Width, int _Height, fp4 _PercentVis)
{
	MAUTOSTRIP(CRC_Util2D_Text_DrawFormatted, 0);
/*
	wchar Text[1024];
	//wchar PartialText[1024];
	Localize_Str(_Text, Text, 1023);
	_PercentVis = Min(1.0f, Max(0.f, _PercentVis));
	
	int visIndex = 0;
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
		nLines += Text_WordWrap(_pF, _Width, (wchar*) Text, Len, &lpLines[nLines], 15-nLines);
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
		th = TextHeight(_pF, (wchar*)Text);
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
			x += (_Width - TextWidth(_pF, &Lines[ln][0])) >> 1;

		nVisibleCharsOnLine = CStrBase::StrLen(lpLines[ln]);
		
		if(nVisibleChars + nVisibleCharsOnLine > visIndex)
			Lines[ln][visIndex - nVisibleChars] = NULL;

		Text_Draw(_Clip, _pF, &Lines[ln][0], _x + x, _y + y, _Style, _ColM, _ColH, _ColD);
		th = TextHeight(_pF, &Lines[ln][0]);
		y += th;

		nVisibleChars += nVisibleCharsOnLine;
		if(nVisibleChars > visIndex)
		{
			ln++;
			break;
		}

		visIndex += 4; // Add the characters for the text sizing
		
	}
	
	_y += y;
	_x += TextWidth(_pF, lpLines[ln-1]);
	
	return nLines;
*/
	return 0;
}

int CRC_Util2D::Text_DrawFormatted(const CClipRect& _Clip, CRC_Font* _pF, const CStr& _Text, int _x, int _y, int _Style, int _ColM, int _ColH, int _ColD, int _Width, int _Height, bool _bShadow)
{
	MAUTOSTRIP(CRC_Util2D_Text_DrawFormatted_2, 0);
	return 0;
/*
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
		nLines += Text_WordWrap(_pF, _Width, (wchar*) Text, Len, &lpLines[nLines], 31-nLines);
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
		th = TextHeight(_pF, (wchar*)Text);
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

			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)-1, (_y + y)-1, _Style, _ColM, _ColH, _ColD);
			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)-1, (_y + y)+0, _Style, _ColM, _ColH, _ColD);
			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)-1, (_y + y)+1, _Style, _ColM, _ColH, _ColD);
			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)+0, (_y + y)-1, _Style, _ColM, _ColH, _ColD);
			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)+0, (_y + y)+1, _Style, _ColM, _ColH, _ColD);
			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)+1, (_y + y)-1, _Style, _ColM, _ColH, _ColD);
			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)+1, (_y + y)+0, _Style, _ColM, _ColH, _ColD);
			Text_Draw(_Clip, _pF, &Lines[ln][0], (_x + x)+1, (_y + y)+1, _Style, _ColM, _ColH, _ColD);
			th = TextHeight(_pF, &Lines[ln][0]);
			y += th;
		}
	}
	else
	{
		for(int ln = 0; ln < nLines; ln++)
		{
			int x = 0;
			if (_Style & WSTYLE_TEXT_CENTER)
				x += (_Width - TextWidth(_pF, &Lines[ln][0])) >> 1;

			Text_Draw(_Clip, _pF, &Lines[ln][0], _x + x, _y + y, _Style, _ColM, _ColH, _ColD);
			th = TextHeight(_pF, &Lines[ln][0]);
			y += th;
		}
	}
	
	return nLines;
*/
}


void CRC_Util2D::Localize_Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const char* _pStr, const CPixel32& _Color, fp4 _Size)
{
	MAUTOSTRIP(CRC_Util2D_Localize_Text, MAUTOSTRIP_VOID);
	wchar Buffer[1024];
	Localize_Str(_pStr, Buffer, 1024);
	Text(_Clip, _pFont, _x0, _y0, Buffer, _Color, _Size);
}

void CRC_Util2D::Localize_Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const wchar* _pStr, const CPixel32& _Color, fp4 _Size)
{
	MAUTOSTRIP(CRC_Util2D_Localize_Text_2, MAUTOSTRIP_VOID);
	wchar Buffer[1024];
	Localize_Str(_pStr, Buffer, 1024);
	Text(_Clip, _pFont, _x0, _y0, Buffer, _Color, _Size);
}

void CRC_Util2D::Localize_Text(const CClipRect& _Clip, CRC_Font* _pFont, int _x0, int _y0, const CStr& _Str, const CPixel32& _Color, fp4 _Size)
{
	MAUTOSTRIP(CRC_Util2D_Localize_Text_3, MAUTOSTRIP_VOID);
	wchar Buffer[1024];
	Localize_Str(_Str, Buffer, 1024);
	Text(_Clip, _pFont, _x0, _y0, Buffer, _Color, _Size);
}

void CRC_Util2D::View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat43fp4& _WMat, const CMat43fp4& _VMat, int _Flags)
{
}

void CRC_Util2D::View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CVec3Dfp4* _pVPortal, int _nVPortal,
		const CXR_AnimState* _pAnimState, const CMat43fp4& _WMat, const CMat43fp4& _VMat, int _Flags)
{
}

void CRC_Util2D::View_SetCurrent(int _iView, CXR_SceneGraphInstance* _pSceneGraphInstance)
{
	MAUTOSTRIP(CRC_Util2D_View_SetCurrent, MAUTOSTRIP_VOID);
}

bool CRC_Util2D::View_GetClip_Sphere(CVec3Dfp4 _v0, fp4 _Radius, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo)
{
	MAUTOSTRIP(CRC_Util2D_View_GetClip_Sphere, false);
	if(_pRenderInfo)
	{
		_pRenderInfo->Clear(_pRenderInfo->m_pCurrentEngine);
		_pRenderInfo->m_BasePriority_Opaque = m_VBPriority += m_VBPriorityAdd;
		_pRenderInfo->m_BasePriority_Transparent = _pRenderInfo->m_BasePriority_Opaque;
	}
	return true;
}

bool CRC_Util2D::View_GetClip_Box(CVec3Dfp4 _min, CVec3Dfp4 _max, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo)
{
	MAUTOSTRIP(CRC_Util2D_View_GetClip_Box, false);
	if(_pRenderInfo)
	{
		_pRenderInfo->Clear(_pRenderInfo->m_pCurrentEngine);
		_pRenderInfo->m_BasePriority_Opaque = m_VBPriority += m_VBPriorityAdd;
		_pRenderInfo->m_BasePriority_Transparent = _pRenderInfo->m_BasePriority_Opaque;
	}
	return true;
}



void CRC_Util2D::Model(CXR_Engine* _pEngine, const CClipRect& _Clip, const CRct& _Rect, class CXR_Model *_pModel, class CXR_AnimState *_pAnimState, CMat43fp4* _Mat)
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

	CBox3Dfp4 Bound;
	_pModel->GetBound_Box(Bound, _pAnimState);

	CVec3Dfp4 Size;
	Bound.m_Max.Sub(Bound.m_Min, Size);
	fp4 SizeX = Length2(Size[0], Size[1]);
	fp4 SizeY = Size[2];

	//pRC->Viewport_Get()->GetFOV()
	const fp4 FOV = 20;

	CVec2Dfp4 Scale = GetCoordinateScale();
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

	fp4 Dist = SizeX * View.GetScale() * 0.33f;
	fp4 DistY = SizeY * View.GetScale() * 0.33f;
	
	fp4 w = Rect.GetWidth();
	fp4 h = Rect.GetHeight();

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

//	fp4 Dist = Max(SizeX / m_Pos.GetWidth(), SizeY / m_Pos.GetHeight());

	CMat43fp4 Camera;
	Camera.Unit();
	Camera.RotX_x_M(0.25f);
	Camera.RotY_x_M(-0.25f);
	Camera.RotZ_x_M(0.5f);
	Camera.k[3][0] = Max(Dist, DistY) * 1.0f;
	Camera.k[3][2] = 0;

	
	CMat43fp4 Mat;
	if(_Mat != NULL)
		Mat = *_Mat;
	else
		Mat.Unit();
	Mat.RotZ_x_M(_pAnimState->m_AnimTime1.GetTimeModulusScaled(-0.1f, _PI2));

	CVec3Dfp4 Center;
	Bound.GetCenter(Center);
	Center.MultiplyMatrix3x3(Mat);
	CVec3Dfp4::GetMatrixRow(Mat, 3) -= Center;

	if (m_pVBM->Viewport_Push(&View))
	{
		CMat43fp4 VMat;
		Camera.InverseOrthogonal(VMat);

		_pModel->OnRender(_pEngine, pRC, m_pVBM, this, m_spWorldLightState, _pAnimState, Mat, VMat, CXR_MODEL_ONRENDERFLAGS_MAXLOD);

		m_pVBM->Viewport_Pop();
	}
}


void CRC_Util2D::Model(const CClipRect& _Clip, const CRct& _Rect, class CXR_Model *_pModel, class CXR_AnimState *_pAnimState, CMat43fp4* _Mat)
{
	MAUTOSTRIP(CRC_Util2D_Model_2, MAUTOSTRIP_VOID);
}


// Renders a model and animates it
void CRC_Util2D::Model(CXR_Engine* _pEngine, class CWorld_Client* _pWClient, const CClipRect& _Clip, const CRct& _Rect, CXR_Anim_Base* _pAnim, class CXR_Model* _pModel, class CXR_AnimState *_pAnimState)
{	
	MAUTOSTRIP(CRC_Util2D_Model_3, MAUTOSTRIP_VOID);
}


void CRC_Util2D::Model(class CWorld_Client* _pWClient, const CClipRect& _Clip, const CRct& _Rect, CXR_Anim_Base* _pAnim, class CXR_Model* _pModel, class CXR_AnimState *_pAnimState)
{
	MAUTOSTRIP(CRC_Util2D_Model_4, MAUTOSTRIP_VOID);
}
	
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
