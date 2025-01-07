#include "PCH.h"

#include "XRFog.h"
#include "XREngine.h"
#include "XREngineVar.h"

#include "MFloat.h"
#include "MSIMD.h"

#include "XRVertexBuffer.h"
#include "XRVBManager.h"

void TransformAABB(const CMat4Dfp32& _Mat, const CBox3Dfp32& _Box, CBox3Dfp32& _DestBox);

// -------------------------------------------------------------------
//  CXR_FogState
// -------------------------------------------------------------------
CXR_FogState::CXR_FogState()
{
	MAUTOSTRIP(CXR_FogState_ctor, MAUTOSTRIP_VOID);
	m_FogTableTextureID = 0;
	m_DepthFogTableTextureID = 0;
	m_LinearFogTableTextureID = 0;
	m_FogTableWidth = 0;

	m_BufferSize = 0;
	m_pSqrtBuffer = NULL;
	m_pAddBuffer = NULL;

	m_bAllowDepthFog = true;
	m_bAllowVertexFog = true;
	m_bAllowNHF = true;
}

void CXR_FogState::Create(CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CXR_FogState_Create, MAUTOSTRIP_VOID);
	m_pEngine = _pEngine;

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("Create", "No texture-context.");

	m_FogTableTextureID = pTC->GetTextureID("SPECIAL_FOGTABLE");
	if (!m_FogTableTextureID)
		ConOutL("§cf80WARNING: Could not find fog-table texture. (SPECIAL_FOGTABLE)");

	m_DepthFogTableTextureID = pTC->GetTextureID("SPECIAL_DEPTHFOGTABLE");
	if (!m_DepthFogTableTextureID)
		ConOutL("§cf80WARNING: Could not find depth fog-table texture. (SPECIAL_DEPTHFOGTABLE)");

	m_LinearFogTableTextureID = pTC->GetTextureID("SPECIAL_FOGTABLELINEAR");
	if (!m_LinearFogTableTextureID)
		ConOutL("§cf80WARNING: Could not find linear fog-table texture. (SPECIAL_FOGTABLELINEAR)");

	m_Special000000TextureID = pTC->GetTextureID("SPECIAL_000000");
	if (!m_Special000000TextureID)
		ConOutL("§cf80WARNING: Could not find linear fog-table texture. (SPECIAL_000000)");


	if(m_FogTableTextureID)
	{
#if 0
		CImage* pImg = pTC->GetTexture(m_FogTableTextureID, 0, CTC_TEXTUREVERSION_RAW);
		if (pImg)
		{
			int w = pImg->GetWidth();
			m_FogTableWidth = w;
			m_FogTableWidthInv = 1.0f / fp32(w);
			m_lFogTable.SetLen(w);
			for(int i = 0; i < w; i++)
				m_lFogTable[i] = pImg->GetPixel(pImg->GetClipRect(), CPnt(i, 0)).GetA();

			pTC->ReleaseTexture(m_FogTableTextureID, 0, CTC_TEXTUREVERSION_RAW);
		}
		else
			ConOut("§cf80WARNING: Could not acquire fog-table. Fog-table not built.");
#else
		m_lFogTable.SetLen(16);
		for(int i = 0; i < 16; i++)
			m_lFogTable[i] = i * 16 + i;
#endif

	}
	else
		ConOut("§cf80WARNING: Could not acquire fog-table. Fog-table not built.");

//	InitFogBuffer(8192);
}

void CXR_FogState::OnPrecache()
{
	MAUTOSTRIP(CXR_FogState_OnPrecache, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("Create", "No texture-context.");

	pTC->SetTextureParam(m_LinearFogTableTextureID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_DepthFogTableTextureID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_FogTableTextureID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_Special000000TextureID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
}


// -------------------------------------------------------------------
bool CXR_FogState::NeedFog_Sphere(const CVec3Dfp32& _Pos, fp32 _Raidius)
{
	MAUTOSTRIP(CXR_FogState_NeedFog_Sphere, false);
	return true;
}

bool CXR_FogState::NeedFog_Box(const CBox3Dfp32& _Box, int _iNode)
{
	MAUTOSTRIP(CXR_FogState_NeedFog_Box, false);
	return false;

	if (m_nVolumes) return true;

	fp32 dsqr = _Box.GetMaxSqrDistance(CVec3Dfp32::GetMatrixRow(m_Eye, 3));
	return (dsqr > Sqr(m_DepthFogStart));
}

void CXR_FogState::PrepareFrame(CXR_ViewContext* _pView, const CPlane3Dfp32& _WFrontPlane)
{
	MAUTOSTRIP(CXR_FogState_PrepareFrame, MAUTOSTRIP_VOID);
	ClearVBMDependencies();

	m_bAllowDepthFog = m_pEngine->GetVar(XR_ENGINE_ALLOWDEPTHFOG) != 0;
	m_bAllowVertexFog = m_pEngine->GetVar(XR_ENGINE_ALLOWVERTEXFOG) != 0;
	m_bAllowNHF = m_pEngine->GetVar(XR_ENGINE_ALLOWNHF) != 0;

	m_DepthFogEnable = false;
	m_VtxFog_Enable = false;

	m_WFrontPlane = _WFrontPlane;
	m_bClipFront = false;
	m_bTransform = false;
	m_bTranslate = false;
	m_EyeFrontPlaneDist = 0;
	m_nVolumes = 0;
	m_nFogModels = 0;
	m_lVolumes.SetLen(6);
	m_lpFogModels.SetLen(1);

	m_DepthFogStart = 0;
	m_DepthFogStart = 500*1;
	m_DepthFogEnd = 2000;
//	m_DepthFogColor = /*0xffa0a0c0;*/ 0xff202030;
	m_DepthFogColor = /*0xffa0a0c0;*/ 0xff000000;
	m_DepthFogDensity = 1.0f;
	
	m_DepthFogIntervalK = 256.0f / (m_DepthFogEnd - m_DepthFogStart);

	SetEye(_pView->m_CameraWMat);
}

void CXR_FogState::InitFogBuffer(int _nV)
{
	MAUTOSTRIP(CXR_FogState_InitFogBuffer, MAUTOSTRIP_VOID);
	_nV = (_nV + 15) & ~16;
	if (m_BufferSize >= _nV) return;

	int BufferSize = _nV * 2 + 16;
	if (BufferSize < _nV) Error("InitFogBuffer", "Internal error.");
	int Size = BufferSize*8 + 16;

	m_FogBuffer.SetLen(Size);

	m_pSqrtBuffer = (fp32*)((aint(m_FogBuffer.GetBasePtr())+15) & 0xfffffff0);
	m_pAddBuffer = (fp32*)(aint(m_pSqrtBuffer)+_nV*4);
	m_BufferSize = BufferSize;
//LogFile(CStrF("(CXR_FogState::InitFogBuffer) %d nV, BufferSize %d, Size %d, Len %d", _nV, BufferSize, Size, m_FogBuffer.Len()));
}

void CXR_FogState::ClearVBMDependencies()
{
	MAUTOSTRIP(CXR_FogState_ClearVBMDependencies, MAUTOSTRIP_VOID);
	m_VtxFog_pAttribOpaque = NULL;
	m_VtxFog_pAttribTransparent = NULL;
}

void CXR_FogState::AddSphere(const CVec3Dfp32& _Pos, fp32 _Radius, CPixel32 _Color, fp32 _Thickness)
{
	MAUTOSTRIP(CXR_FogState_AddSphere, MAUTOSTRIP_VOID);
	ConOut("§cf80WARNING (CXR_FogState::AddSphere): Ignoring fog-sphere.");
return;

	if (m_nVolumes >= m_lVolumes.Len()) return;
	CXR_FogSphere* pS = &m_lVolumes[m_nVolumes];
	m_nVolumes++;

	pS->m_Pos = _Pos;
	pS->m_Radius = _Radius;
	pS->m_RSqr = Sqr(_Radius);
	pS->m_Color = _Color;
	pS->m_Thickness = _Thickness;
}


void CXR_FogState::AddModel(CXR_FogInterface* _pFogModel, const CVec3Dfp32& _Pos)
{
	MAUTOSTRIP(CXR_FogState_AddModel, MAUTOSTRIP_VOID);
	if (m_nFogModels >= m_lpFogModels.Len()) return;
//	m_lpFogModels[m_nFogModels] = _pFogModel;

	m_pFogModel = _pFogModel;
	m_hFogModelAccelerator = 1;
	m_nFogModels++;
}

void CXR_FogState::SetTransform(const CMat4Dfp32* _pMat)
{
	MAUTOSTRIP(CXR_FogState_SetTransform, MAUTOSTRIP_VOID);
	if (!_pMat)
	{
		m_Transform.Unit();
		m_bTransform = false;
		m_bTranslate = false;
		return;
	}

	if ((_pMat->k[0][0] == 1.0f) &&
		(_pMat->k[0][1] == 0.0f) &&
		(_pMat->k[0][2] == 0.0f) &&
		(_pMat->k[1][0] == 0.0f) &&
		(_pMat->k[1][1] == 1.0f) &&
		(_pMat->k[1][2] == 0.0f) &&	
		(_pMat->k[2][0] == 0.0f) &&
		(_pMat->k[2][1] == 0.0f) &&
		(_pMat->k[2][2] == 1.0f))
	{
		m_bTransform = false;
		if ((_pMat->k[3][0] == 0.0f) &&
			(_pMat->k[3][1] == 0.0f) &&
			(_pMat->k[3][2] == 0.0f)) 
			m_bTranslate = false;
		else
			m_bTranslate = true;
	}
	else
	{
		m_bTranslate = false;
		m_bTransform = true;
	}

	m_Transform = *_pMat;
}

void CXR_FogState::SetEye(const CMat4Dfp32& _Pos)
{
	MAUTOSTRIP(CXR_FogState_SetEye, MAUTOSTRIP_VOID);
	m_Eye = _Pos;
}

void CXR_FogState::TraceBound(const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_FogState_TraceBound, MAUTOSTRIP_VOID);
	m_hFogModelAccelerator = m_pFogModel->Fog_InitTraceBound(_Box);
}

void CXR_FogState::TraceBoundRelease()
{
	MAUTOSTRIP(CXR_FogState_TraceBoundRelease, MAUTOSTRIP_VOID);
	m_pFogModel->Fog_ReleaseTraceBound(m_hFogModelAccelerator);
	m_hFogModelAccelerator = 1;
}

CPixel32 CXR_FogState::Trace(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CXR_FogState_Trace, CPixel32());
	CPixel32 Col(0);
	CVec3Dfp32 v(_v);
	const CVec3Dfp32& veye = CVec3Dfp32::GetMatrixRow(m_Eye, 3);
	if (m_bTransform)
		v *= m_Transform;
	else if (m_bTranslate)
		v += CVec3Dfp32::GetMatrixRow(m_Transform, 3);

//	CVec3Dfp32 dv;
//	v.Sub(veye, dv);


/*	for(int iModel = 0; iMod el < m_nFogModels; iModel++)
	{
		m_lpFogModels[iModel]->Fog_Trace(0, veye, &v, 1, &Col);
	}*/

	m_pFogModel->Fog_Trace(m_hFogModelAccelerator, veye, &v, 1, &Col);

	// Clip ray to front-plane.
/*	if (m_bClipFront)
	{
		fp32 d = m_WFrontPlane.Distance(v);
		if (d < 0.0f) return Col;
		veye.Combine(v, m_EyeFrontPlaneDist / (dv*m_WFrontPlane.n), veye);
		v.Sub(veye, dv);
	}

	fp32 d = M_Sqrt(dv.LengthSqr());
//ConOut(CStrF("%f, %s, ", d, (char*) _v.GetString()) + m_Transform.GetString());
	if (d > m_DepthFogStart)
	{
		if (d > m_DepthFogEnd)
			Col = m_DepthFogColor;
		else
		{
			Col = m_DepthFogColor;
			Col.MultiplyRGBA_Fixed_NoClip((d - m_DepthFogStart) * m_DepthFogIntervalK);
		}
	}*/

//ConOut(CStrF("Trace %s -> %s", (char*)veye.GetString(), (char*)v.GetString()));
return Col;

	// Process fog-volumes
/*	for(int iV = 0; iV < m_nVolumes; iV++)
	{
		CXR_FogSphere* pS = &m_lVolumes[iV];
		CVec3Dfp32 a;
		pS->m_Pos.Sub(veye, a);
		fp32 dvlen = M_Sqrt(dv.LengthSqr());
		if (!dvlen) continue;
		fp32 aproj = (dv * a) / dvlen;
		CVec3Dfp32 cp;
		dv.CrossProd(a, cp);
		fp32 cplen = M_Sqrt(cp.LengthSqr());
		fp32 h =  cplen / dvlen;
		if (h < pS->m_Radius)
		{
			fp32 d = M_Sqrt(pS->m_RSqr - Sqr(h));
			fp32 l1 = aproj - d;
			fp32 l2 = aproj + d;
			if (l1 > l2) Swap(l1, l2);
			if (l2 < 0.0f) continue;
			if (l1 > dvlen) continue;
			if (l1 < 0.0f) l1 = 0.0f;
			if (l2 > dvlen) l2 = dvlen;
			fp32 dist = (l2 - l1);
			if (dist > 2.0f)
				Col = pS->m_Color.AlphaBlendRGBA(Col, Min(255, int(dist * pS->m_Thickness)) );
		}
	}*/

	// Process fog-volumes
/*	for(int iV = 0; iV < m_nVolumes; iV++)
	{
		CXR_FogSphere* pS = &m_lVolumes[iV];
		CVec3Dfp32 v = CVec3Dfp32::GetMatrixRow(m_Eye, 3) - _v;
		CVec3Dfp32 a = pS->m_Pos - _v;
		fp32 vlensqr = Sqr(v.k[0]) + Sqr(v.k[1]) + Sqr(v.k[2]);
		fp32 vlen = M_Sqrt(vlensqr);
		if (!vlen) continue;
		fp32 aproj = (v * a) / vlen;
		CVec3Dfp32 cp = (v / a);
		fp32 cplen = M_Sqrt(Sqr(cp.k[0]) + Sqr(cp.k[1]) + Sqr(cp.k[2]));
		fp32 h =  cplen / vlen;
		if (h < pS->m_Radius)
		{
			fp32 d = M_Sqrt(pS->m_RSqr - Sqr(h));
			fp32 l1 = aproj - d;
			fp32 l2 = aproj + d;
			if (l1 > l2) Swap(l1, l2);
			if (l2 < 0.0f) continue;
			if (l1 > vlen) continue;
			if (l1 < 0.0f) l1 = 0.0f;
			if (l2 > vlen) l2 = vlen;
			fp32 dist = (l2 - l1);
			if (dist > 2.0f)
				Col = pS->m_Color.AlphaBlendRGBA(Col, Min(255, int(dist * pS->m_Thickness)) );
		}
	}
*/
	return Col;
}

static void InterpolateFogBox(const CBox3Dfp32& _BoundBox, const CPixel32* _pBoxFog, int _nV, const CVec3Dfp32* _pV, CPixel32* _pFog, const CMat4Dfp32* _pTransform)
{
	MAUTOSTRIP(InterpolateFogBox, MAUTOSTRIP_VOID);
	fp32 oox = 1.0f / (_BoundBox.m_Max[0] - _BoundBox.m_Min[0]);
	fp32 ooy = 1.0f / (_BoundBox.m_Max[1] - _BoundBox.m_Min[1]);
	fp32 ooz = 1.0f / (_BoundBox.m_Max[2] - _BoundBox.m_Min[2]);

	if (_pTransform)
	{
		for(int v = 0; v < _nV; v++)
		{
			CVec3Dfp32 V;
			V = _pV[v];
			V *= *_pTransform;
			int fx = RoundToInt(256.0f * Clamp01((V[0] - _BoundBox.m_Min[0]) * oox));
			int fy = RoundToInt(256.0f * Clamp01((V[1] - _BoundBox.m_Min[1]) * ooy));
			int fz = RoundToInt(256.0f * Clamp01((V[2] - _BoundBox.m_Min[2]) * ooz));
			_pFog[v] = CPixel32::TrilinearRGBA(_pBoxFog, fx, fy, fz);
//		ConOut(CStrF("Vert %d, %d, %d, %d", v, fx, fy, fz));
		}
	}
	else
	{
		for(int v = 0; v < _nV; v++)
		{
			int fx = RoundToInt(256.0f * Clamp01((_pV[v][0] - _BoundBox.m_Min[0]) * oox));
			int fy = RoundToInt(256.0f * Clamp01((_pV[v][1] - _BoundBox.m_Min[1]) * ooy));
			int fz = RoundToInt(256.0f * Clamp01((_pV[v][2] - _BoundBox.m_Min[2]) * ooz));
//		ConOut(CStrF("Vert %d, %d, %d, %d", v, fx, fy, fz));
			_pFog[v] = CPixel32::TrilinearRGBA(_pBoxFog, fx, fy, fz);
		}
	}
}

bool CXR_FogState::TraceBox(const CBox3Dfp32& _BoundBox, CPixel32* _pFog)
{
	MAUTOSTRIP(CXR_FogState_TraceBox, false);
	CVec3Dfp32 BoxV[8];
	_BoundBox.GetVertices(BoxV);

//	ConOut("Box: " + _BoundBox.GetString());

	int bFog = 0;
	for(int v = 0; v < 8; v++)
	{
		_pFog[v] = Trace(BoxV[v]);
		bFog |= (int)_pFog[v];

//		ConOut(CStrF("BoxVert %d, %.8x", v, _pFog[v]));
	}

	return (bFog & 0xff000000) != 0;
}

bool CXR_FogState::InterpolateBox(const CBox3Dfp32& _BoundBox, const CPixel32* _pBoxFog, int _nV, const CVec3Dfp32* _pV, CPixel32* _pFog, CVec2Dfp32* _pFogUV, const CMat4Dfp32* _pTransform)
{
	MAUTOSTRIP(CXR_FogState_InterpolateBox, false);
	InterpolateFogBox(_BoundBox, _pBoxFog, _nV, _pV, _pFog, _pTransform);
	ConvertFogColors(_nV, _pFog, _pFogUV);
	return true;
}


bool CXR_FogState::Trace(const CBox3Dfp32& _BoundBox, int _nV, const CVec3Dfp32* _pV, CPixel32* _pFog, CVec2Dfp32* _pFogUV, bool _bFast)
{
	MAUTOSTRIP(CXR_FogState_Trace_2, false);
//	TraceBound(_BoundBox);

	if (_bFast)
	{
		CBox3Dfp32 BoundBox;
		CVec3Dfp32::GetMinBoundBox(_pV, BoundBox.m_Min, BoundBox.m_Max, _nV);

		CVec3Dfp32 BoxV[8];
		CPixel32 BoxFog[8];
		BoundBox.GetVertices(BoxV);

		int bFog = 0;
		for(int v = 0; v < 8; v++)
		{
			BoxFog[v] = Trace(BoxV[v]);
			bFog |= (int)BoxFog[v];
		}

		if (!(bFog & 0xff000000))
		{
//			TraceBoundRelease();
			return false;
		}

		InterpolateFogBox(BoundBox, BoxFog, _nV, _pV, _pFog, NULL);
		ConvertFogColors(_nV, _pFog, _pFogUV);
	}
	else
	{
		int bFog = 0;
		for(int v = 0; v < _nV; v++)
		{
			_pFog[v] = Trace(_pV[v]);
			bFog |= (int)_pFog[v];
		}

		if (!(bFog & 0xff000000))
		{
//			TraceBoundRelease();
			return false;
		}

		ConvertFogColors(_nV, _pFog, _pFogUV);
	}

//	TraceBoundRelease();
	return true;
}

CPixel32 CXR_FogState::TranslateFogTable(CPixel32 _Fog)
{
	MAUTOSTRIP(CXR_FogState_TranslateFogTable, CPixel32());
	if (m_lFogTable.Len() < 2)
		return _Fog;
	else
	{
		int f = _Fog.GetA();
		int fi = f * (m_lFogTable.Len() - 1);
		fi = fi + (fi >> 8);
		int ffrac = fi & 255;
		int f0 = m_lFogTable[fi >> 8];
		int f1 = m_lFogTable[(fi >> 8) + 1];
		int a = f0 + (((f1 - f0)*ffrac) >> 8);

//ConOut(CStrF("Fog translate %d -> %d", _Fog.GetA(), a));
		return (_Fog & 0xffffff) + (a << 24);
	}
}

// #define NODEPTHFOG

void CXR_FogState::DepthFog_Init(fp32 _Start, fp32 _End, CPixel32 _Color, fp32 _Density)
{
	MAUTOSTRIP(CXR_FogState_DepthFog_Init, MAUTOSTRIP_VOID);
	m_DepthFogStart = _Start;
	m_DepthFogEnd = _End;
	m_DepthFogColor = _Color;
	m_DepthFogDensity = Clamp01(_Density);
	m_DepthFogIntervalK = 256.0f / (m_DepthFogEnd - m_DepthFogStart);
	m_DepthFogEnable = (m_DepthFogStart < 100000);
}


void CXR_FogState::SetDepthFog(CRenderContext* _pRC, int _iPass, int _RasterMode, fp32 _DepthScale)
{
	MAUTOSTRIP(CXR_FogState_SetDepthFog, MAUTOSTRIP_VOID);
	Error("CXR_FogState::SetDepthFog", "FixMe: Uses legacy rendering API");
	if (!DepthFogEnable()) return;
	int Color = 0;
	switch(_RasterMode)
	{
	case CRC_RASTERMODE_ADD :
	case CRC_RASTERMODE_ALPHAADD :
	case CRC_RASTERMODE_MULADD :
		Color = 0x00000000;
		break;
	default :
		Color = m_DepthFogColor;
	}

	_pRC->Attrib_FogStart(m_DepthFogStart * _DepthScale);
	_pRC->Attrib_FogEnd(m_DepthFogEnd * _DepthScale);
	_pRC->Attrib_FogDensity(1.0f);
	_pRC->Attrib_FogColor(Color);
	_pRC->Attrib_Enable(CRC_FLAGS_FOG);
}

void CXR_FogState::SetDepthFogBlack(CRenderContext* _pRC, fp32 _DepthScale)
{
	MAUTOSTRIP(CXR_FogState_SetDepthFogBlack, MAUTOSTRIP_VOID);
	Error("CXR_FogState::SetDepthFogBlack", "FixMe: Uses legacy rendering API");
	if (!DepthFogEnable()) return;
	_pRC->Attrib_FogStart(m_DepthFogStart * _DepthScale);
	_pRC->Attrib_FogEnd(m_DepthFogEnd * _DepthScale);
	_pRC->Attrib_FogDensity(m_DepthFogDensity);
	_pRC->Attrib_FogColor(0);
	_pRC->Attrib_Enable(CRC_FLAGS_FOG);
}

void CXR_FogState::SetDepthFog(CRC_Attributes* _pAttr, int _iPass, fp32 _DepthScale)
{
	MAUTOSTRIP(CXR_FogState_SetDepthFog_2, MAUTOSTRIP_VOID);
	if (!DepthFogEnable()) return;

//	if (_DepthScale != 1.0f) ConOut(CStrF("DepthScale %f", _DepthScale));

#ifndef NODEPTHFOG
	int Color = m_DepthFogColor;
	if (_iPass != -1)
	{
		if(_pAttr->m_DestBlend == CRC_BLEND_ONE)
		{
			switch(_pAttr->m_SourceBlend)
			{
			case CRC_BLEND_ONE :		//CRC_RASTERMODE_ADD
			case CRC_BLEND_SRCALPHA :	//CRC_RASTERMODE_ALPHAADD
			case CRC_BLEND_DESTCOLOR :	//CRC_RASTERMODE_MULADD
				Color = 0x00000000;
				break;
			}
		}
	}

	_pAttr->Attrib_FogStart(m_DepthFogStart * _DepthScale);
	_pAttr->Attrib_FogEnd(m_DepthFogEnd * _DepthScale);
	_pAttr->Attrib_FogDensity(m_DepthFogDensity);
	_pAttr->Attrib_FogColor(Color);
	_pAttr->Attrib_Enable(CRC_FLAGS_FOG);
#else
	SetDepthFogNone(_pAttr);
#endif
}

void CXR_FogState::SetDepthFogBlack(CRC_Attributes* _pAttr, fp32 _DepthScale)
{
	MAUTOSTRIP(CXR_FogState_SetDepthFogBlack_2, MAUTOSTRIP_VOID);
	if (!DepthFogEnable()) return;

//	if (_DepthScale != 1.0f) ConOut(CStrF("DepthScale %f", _DepthScale));

#ifndef NODEPTHFOG
	_pAttr->Attrib_FogStart(m_DepthFogStart * _DepthScale);
	_pAttr->Attrib_FogEnd(m_DepthFogEnd * _DepthScale);
	_pAttr->Attrib_FogDensity(m_DepthFogDensity);
	_pAttr->Attrib_FogColor(0);
	_pAttr->Attrib_Enable(CRC_FLAGS_FOG);
#else
	SetDepthFogNone(_pAttr);
#endif
}

void CXR_FogState::SetDepthFogNone(CRC_Attributes* _pAttr, fp32 _DepthScale)
{
	MAUTOSTRIP(CXR_FogState_SetDepthFogNone, MAUTOSTRIP_VOID);
	if (!DepthFogEnable()) return;

//	if (_DepthScale != 1.0f) ConOut(CStrF("DepthScale %f", _DepthScale));

	_pAttr->Attrib_FogStart(m_DepthFogEnd+100000);
	_pAttr->Attrib_FogEnd(m_DepthFogEnd+100001);
	_pAttr->Attrib_FogDensity(0.0001f);
	_pAttr->Attrib_FogColor(0);
	_pAttr->Attrib_Enable(CRC_FLAGS_FOG);
}

void CXR_FogState::VertexFog_Init(const CMat4Dfp32& _POV, fp32 _EndDistance, fp32 _HeightAttenuation, CPixel32 _Color, fp32 _ReferenceHeight)
{
	MAUTOSTRIP(CXR_FogState_VertexFog_Init, MAUTOSTRIP_VOID);
	m_VtxFog_Enable = true;
	m_VtxFog_End = _EndDistance;
	m_VtxFog_HeightAttn = _HeightAttenuation;
	m_VtxFog_Color = _Color;
	m_VtxFog_ReferenceHeight = _ReferenceHeight;

	// FIXME: If projected VRight == 0 || projected VFwd == 0 this won't work.

	CVec3Dfp32 VFwd = CVec3Dfp32::GetMatrixRow(_POV, 2);
	CVec3Dfp32 VRight = CVec3Dfp32::GetMatrixRow(_POV, 0);
	VFwd[2] = 0;
	VRight[2] = 0;
	VFwd.Normalize();
	VRight.Normalize();
	CVec3Dfp32 VUp(0,0,1);

	CMat4Dfp32 Mat;
	Mat.Unit();
	VRight.SetMatrixRow(Mat, 0);
	VUp.SetMatrixRow(Mat, 1);
	VFwd.SetMatrixRow(Mat, 2);

	if (Mat.IsMirrored()) VRight = -VRight;

	// Create vector in plane
	VFwd *= m_VtxFog_End;
	CVec3Dfp32 VPlane;
	VPlane = VFwd;
	VPlane[2] += m_VtxFog_HeightAttn;

	// Cross projected right, normal is facing into the fog.
	CVec3Dfp32 N;
	VRight.CrossProd(VPlane, N);
	N.Normalize();

	CVec3Dfp32 Pos(CVec3Dfp32::GetMatrixRow(_POV, 3));
	fp32 OldPos = Pos.k[2] - m_VtxFog_ReferenceHeight;
	Pos.k[2] *= 0.5f;
	m_VtxFog_RelHeight = Pos.k[2] - OldPos;

	m_VtxFog_EndPlane.CreateNV(N, Pos + VFwd + CVec3Dfp32(0,0,_ReferenceHeight*2.0f));

	m_VtxFog_VertexCount = 0;
}

bool CXR_FogState::VertexFog_Eval(int _nV, const CVec3Dfp32* _pV, const CPixel32* _pSrcCol, CPixel32* _pDstCol, int _Oper, bool _Transparent)
{
	MAUTOSTRIP(CXR_FogState_VertexFog_Eval, false);
	fp32 k = 1.0f / m_VtxFog_End;
	fp32 k2 = 1.0f / m_VtxFog_HeightAttn;

//	CMathAccel* pMA = GetMathAccel();

	CPixel32 FogColor(m_VtxFog_Color & 0xffffff);

	InitFogBuffer(_nV);

	const CVec3Dfp32& Eye = CVec3Dfp32::GetMatrixRow(m_Eye, 3);

	fp32* pSqrt = m_pSqrtBuffer;
	fp32* pAdd = m_pAddBuffer;

	{
		for(int v = 0; v < _nV; v++)
		{
			CVec3Dfp32 dV(_pV[v]);
			dV *= m_Transform;
			dV -= Eye;
			pSqrt[v] = Sqr(dV[0]) + Sqr(dV[1]) + Sqr(dV[2]) * 0.5f;
			pAdd[v] = - ((dV[2]-m_VtxFog_RelHeight) * k2);
		}
	}

	SIMD_Sqrt8(pSqrt, NULL, _nV);

	{
		for(int v = 0; v < _nV; v++)
		{
			fp32 fFog = pSqrt[v] * k + pAdd[v];
			fFog = Clamp01((fFog-0.25f)*1.5f);
			_pDstCol[v] = FogColor;
			(int&)_pDstCol[v] |= RoundToInt(fFog*255.0f) << 24;
		}
	}

	return false;	
}

CRC_Attributes* CXR_FogState::VertexFog_GetAttrib(CXR_VBManager* _pVBM, bool _bTransparent)
{
	MAUTOSTRIP(CXR_FogState_VertexFog_GetAttrib, NULL);
	CRC_Attributes* pA = (_bTransparent) ? m_VtxFog_pAttribTransparent: m_VtxFog_pAttribOpaque;
	if (pA) return pA;

	pA = _pVBM->Alloc_Attrib();
	if (!pA) return NULL;


	pA->SetDefault();
	m_pEngine->SetDefaultAttrib(pA);

	if (DepthFogEnable()) SetDepthFogNone(pA);

	pA->Attrib_TextureID(0, m_LinearFogTableTextureID);
	pA->Attrib_Enable(CRC_FLAGS_CULL);
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pA->Attrib_ZCompare((_bTransparent) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL);

	if (_bTransparent)
		m_VtxFog_pAttribTransparent = pA;
	else
		m_VtxFog_pAttribOpaque = pA;

	return pA;
}

fp32* CXR_FogState::VertexFog_EvalCoord(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB)
{

	MAUTOSTRIP(CXR_FogState_VertexFog_EvalCoord, NULL);
	CXR_VBChain *pChain = _pVB->GetVBChain();
	if (!pChain->BuildVertexUsage(_pVBM)) 
		return false;

	const uint16* piV = pChain->m_piVertUse;
	int nVA = pChain->m_nV;
	int nV = (piV) ? pChain->m_nVertUse : pChain->m_nV;
	const CVec3Dfp32* pV = pChain->m_pV;

	fp32* pFog = _pVBM->Alloc_fp32(nVA);
	if (!pFog) return NULL;


/*	const int MaxVerts = 2048;
	if (nV > MaxVerts)
	{
		ConOut("(CXR_FogState::VertexFog_Eval) Insufficient buffer.");
		return false;
	}

	uint8 Data[4*MaxVerts*2 + 16];
	fp32* pSqrt = (fp32*)((int(&Data)+15) & 0xfffffff0);
	fp32* pAdd = (fp32*)(int(pSqrt)+MaxVerts*4);*/
	InitFogBuffer(nV);
	fp32* pSqrt = m_pSqrtBuffer;
	fp32* pAdd = m_pAddBuffer;


	fp32 k = 1.0f / m_VtxFog_End;
	fp32 k2 = 1.0f / m_VtxFog_HeightAttn;
	{
		CVec3Dfp32 Eye = CVec3Dfp32::GetMatrixRow(m_Eye, 3);

		if (piV)
		{
			for(int v = 0; v < nV; v++)
			{
				CVec3Dfp32 dV(pV[piV[v]]);
				dV *= m_Transform;
				dV -= Eye;
				pSqrt[v] = Sqr(dV[0]) + Sqr(dV[1]) + Sqr(dV[2]) * 0.5f;
				pAdd[v] = - ((dV[2]-m_VtxFog_RelHeight) * k2);
			}
		}
		else
		{
			for(int v = 0; v < nV; v++)
			{
				CVec3Dfp32 dV(pV[v]);
				dV *= m_Transform;
				dV -= Eye;
				pSqrt[v] = Sqr(dV[0]) + Sqr(dV[1]) + Sqr(dV[2]) * 0.5f;
				pAdd[v] = - ((dV[2]-m_VtxFog_RelHeight) * k2);
			}
		}
	}

	SIMD_Sqrt8(pSqrt, NULL, nV);

	bool bFog = false;
	{
		if (piV)
		{
			for(int v = 0; v < nV; v++)
			{
				fp32 fFog = pSqrt[v] * k + pAdd[v];
				if (fFog < 1.0f) bFog = true;
				fFog = Clamp01((fFog-0.25f)*1.5f);
				pFog[piV[v]] = fFog * 1000.0f;
			}
		}
		else
		{
			for(int v = 0; v < nV; v++)
			{
				fp32 fFog = pSqrt[v] * k + pAdd[v];
				if (fFog < 1.0f) bFog = true;
				fFog = Clamp01((fFog-0.25f)*1.5f);
				pFog[v] = fFog * 1000.0f;
			}
		}
	}

	m_VtxFog_VertexCount += nV;

	if (!bFog) pFog = NULL;

	return pFog;
}

void CXR_FogState::VertexFog_SetFogCoord(CRC_Attributes* _pAttr)
{
	MAUTOSTRIP(CXR_FogState_VertexFog_SetFogCoord, MAUTOSTRIP_VOID);
	Error("VertexFog_SetFogCoord", "Unsupported.");
	
/*	_pAttr->Attrib_Enable(CRC_FLAGS_FOGCOORD | CRC_FLAGS_FOG);
	_pAttr->Attrib_FogStart(0.0f);
	_pAttr->Attrib_FogEnd(1000.0f);
	_pAttr->Attrib_FogDensity(1.0f);
	CPixel32 Col = 0;
	switch(_pAttr->m_RasterMode)
	{
	case CRC_RASTERMODE_ALPHAADD : 
	case CRC_RASTERMODE_ADD : 
	case CRC_RASTERMODE_MULADD : 
		Col = 0;
		break;
	default : 
		Col = m_VtxFog_Color;
	}
	_pAttr->Attrib_FogColor(Col);
*/
}


bool CXR_FogState::VertexFog_Eval(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, bool _Transparent)
{
	MAUTOSTRIP(CXR_FogState_VertexFog_Eval_2, false);
	if (!_pVB->m_pAttrib)
	{
		CRC_Attributes* pA = VertexFog_GetAttrib(_pVBM, _Transparent);
		if (!pA) return false;

		_pVB->m_pAttrib = pA;
	}
	else
		_pVB->m_pAttrib->Attrib_TextureID(0, m_LinearFogTableTextureID);

	CXR_VBChain *pChain = _pVB->GetVBChain();
	int nV = pChain->m_nV;
	CPixel32* pFog = _pVBM->Alloc_CPixel32(nV);
	CVec2Dfp32* pFogUV = _pVBM->Alloc_V2(nV);
	if (!pFog || !pFogUV) return false;
	const CVec3Dfp32* pV = pChain->m_pV;
	CVec3Dfp32 Eye = CVec3Dfp32::GetMatrixRow(m_Eye, 3);

	_pVB->Geometry_ColorArray(pFog);
	_pVB->Geometry_TVertexArray(pFogUV, 0);

	fp32 oow = m_FogTableWidthInv;
	fp32 omw = m_FogTableWidth - 1;

	fp32 k = 1.0f / m_VtxFog_End;
	fp32 k2 = 1.0f / m_VtxFog_HeightAttn;

//	CMathAccel* pMA = GetMathAccel();

//	CPixel32 FogColor(m_DepthFogColor.GetR()*2, m_DepthFogColor.GetG()*2, m_DepthFogColor.GetB()*2, 255);
	CPixel32 FogColor(m_VtxFog_Color);

//	fp32 MaxFog = (1.0f * omw + 0.5f) * oow;

/*	const int MaxVerts = 2048;
	if (nV > MaxVerts)
	{
		ConOut("(CXR_FogState::VertexFog_Eval) Insufficient buffer.");
		return false;
	}

	uint8 Data[4*MaxVerts*2 + 16];
	fp32* pSqrt = (fp32*)((int(&Data)+15) & 0xfffffff0);
	fp32* pAdd = (fp32*)(int(pSqrt)+MaxVerts*4);*/

	InitFogBuffer(nV);
	fp32* pSqrt = m_pSqrtBuffer;
	fp32* pAdd = m_pAddBuffer;

	{
		for(int v = 0; v < nV; v++)
		{
			CVec3Dfp32 dV(pV[v]);
			dV *= m_Transform;
			dV -= Eye;
			pSqrt[v] = Sqr(dV[0]) + Sqr(dV[1]) + Sqr(dV[2]) * 0.5f;
			pAdd[v] = - ((dV[2]-m_VtxFog_RelHeight) * k2);
		}
	}

	SIMD_Sqrt8(pSqrt, NULL, nV);

	bool bFog = false;
	{
		for(int v = 0; v < nV; v++)
		{
			fp32 fFog = pSqrt[v] * k + pAdd[v];
			fp32 fFog2 = (fFog * omw + 0.5f) * oow;
			if (fFog < 1.0f) bFog = true;
			pFogUV[v].k[0] = fFog2;
			pFogUV[v].k[1] = 0;
			pFog[v] = FogColor;
		}
	}


/*	bool bFog = false;
	for(int v = 0; v < nV; v++)
	{
		CVec3Dfp32 dV(pV[v]);
		dV *= m_Transform;
//		if (m_bTranslate) dV += CVec3Dfp32::GetMatrixRow(m_Transform, 3);
//		else if (m_bTransform) dV *= m_Transform;

		if (m_VtxFog_EndPlane.Distance(dV) > 0.0f)
		{
			pFogUV[v].k[0] = MaxFog;
			pFogUV[v].k[1] = 0;
			pFog[v] = FogColor;
		}
		else
		{
			dV -= Eye;
			fp32 fFog = pMA->fsqrt(Sqr(dV[0]) + Sqr(dV[1]) + Sqr(dV[2]) * 0.5f) * k - ((dV[2]-m_VtxFog_RelHeight) * k2);
			fp32 fFog2 = (fFog * omw + 0.5f) * oow;
			if (fFog < 1.0f) bFog = true;
			pFogUV[v].k[0] = fFog2;
			pFogUV[v].k[1] = 0;
			pFog[v] = FogColor;
		}
//ConOut(CStrF("%d, Fog %f, %f", v, fFog, fFog2) );
	}*/
//	return true;
	return bFog;
}

int CXR_FogState::ConvertFogColors(int _nV, CPixel32* _pFog, CVec2Dfp32* _pTV)
{
	MAUTOSTRIP(CXR_FogState_ConvertFogColors, 0);
	fp32 oow = m_FogTableWidthInv;
	fp32 omw = m_FogTableWidth - 1;

	int bFog = 0;
	for(int v = 0; v < _nV; v++)
	{
		CPixel32 F = _pFog[v];
		int Alpha = F.GetA();
		bFog |= Alpha;
		_pFog[v] = int(F) | 0xff000000;
		fp32 fFog = fp32(Alpha) / 255.0f;
		fFog = (fFog * omw + 0.5f) * oow;
		_pTV[v].k[0] = fFog;
		_pTV[v].k[1] = 0;
//ConOut(CStrF("%d, Color %.8x, U %f", v, _pFog[v], _pTV[v].k[0] ));
	}

	return bFog;
}

void CXR_FogState::RenderPolygon(const CVec3Dfp32* _pV, const uint32* _piV)
{
	MAUTOSTRIP(CXR_FogState_RenderPolygon, MAUTOSTRIP_VOID);
}

// -------------------------------------------------------------------
//  CXR_Model_FogVolume
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_FogVolume, CXR_Model);

CXR_Model_FogVolume::CXR_Model_FogVolume()
{
	MAUTOSTRIP(CXR_Model_FogVolume_ctor, MAUTOSTRIP_VOID);
}

fp32 CXR_Model_FogVolume::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_FogVolume_GetBound_Sphere, 0.0f);
	return 384;
}

void CXR_Model_FogVolume::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_FogVolume_GetBound_Box, MAUTOSTRIP_VOID);
	_Box.m_Min = 0;
	_Box.m_Max = 0;
}

void CXR_Model_FogVolume::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_FogVolume_OnRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_FogVolume::OnRender);
	if (!_pRender) return;

//	fp32 Radius = _pAnimState->m_Anim0;
	fp32 Radius = fp32(_pAnimState->m_Anim0 & 0xff) * 16.0f;
	fp32 Thickness = fp32(_pAnimState->m_Anim0 >> 8) / 16.0f;
	CPixel32 Color = CImage::ConvFromRGBA4444(_pAnimState->m_Anim1);

	CVec3Dfp32 ObjWPos = CVec3Dfp32::GetMatrixRow(_WMat, 3);

//ConOut(CStrF("* %f, %f", Radius, Thickness));
	if(_pViewClip)
	{
		CRC_ClipVolume Clip;

		// Get bound-sphere, get the CRC_ClipView
//		fp32 BoundR = GetBound_Sphere();		// Local
		if (!_pViewClip->View_GetClip_Sphere(ObjWPos, Radius, 0, 0, &Clip, NULL)) return;

		CMat4Dfp32 L2VMat;
		_WMat.Multiply(_VMat, L2VMat);

		// Transform ViewClip from W to V.
		CVec3Dfp32 v(L2VMat.k[3][0], L2VMat.k[3][1], L2VMat.k[3][2]);
		for(int ip = 0; ip < Clip.m_nPlanes; ip++)
		{
			Clip.m_Planes[ip].Transform(_VMat);
			CPlane3Dfp32* pP = &Clip.m_Planes[ip];
			fp32 d = pP->Distance(v);
			if (d > Radius) return;
		}
	}

	CXR_FogState* pFog = _pEngine->m_pCurrentFogState;
	pFog->AddSphere(ObjWPos, Radius, Color, Thickness);
}

