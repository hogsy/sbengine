#include "PCH.h"

#include "MCSolid.h"
#include "MFloat.h"
#include "../../XRModels/Model_BSP/XWCommon.h"

#include "../XRVertexBuffer.h"
#include "../XRSurfaceContext.h"

#ifndef PLATFORM_CONSOLE
#include "../../Classes/Render/OgrRender.h"
#endif

#define CSB_EDGEAND 0x3fff
#define CSB_TESS_EPSILON 0.001f

#define CSB_SNAPGRID (1.0f / 16.0f)
#define CSB_SNAPTRESH 0.01f

#ifndef CPU_SOFTWARE_FP64
template<class T>
static bool EqualPlanes(const T& _p1, const T& _p2, fp64 _Epsilon)
{
	MAUTOSTRIP(EqualPlanes, false);
	if ((M_Fabs(_p1.Distance(_p2.GetPointInPlane())) < _Epsilon) &&
		M_Fabs(_p1.n * _p2.n - 1.0f) < _Epsilon) return true;
	return false;
}
#else
template<class T>
static bool EqualPlanes(const T& _p1, const T& _p2, fp32 _Epsilon)
{
	MAUTOSTRIP(EqualPlanes, false);
	if ((M_Fabs(_p1.Distance(_p2.GetPointInPlane())) < _Epsilon) &&
		M_Fabs(_p1.n * _p2.n - 1.0f) < _Epsilon) return true;
	return false;
}
#endif


//#pragma optimize("",off)


#if 0
// -------------------------------------------------------------------
void CSB_CompactVertex::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CSB_CompactVertex_Read, MAUTOSTRIP_VOID);
	Error_static("CSB_CompactVertex::Read", "De-implemented.");
/*	_pFile->ReadLE(m_V[0]);
	_pFile->ReadLE(m_V[1]);
	_pFile->ReadLE(m_V[2]);
	_pFile->ReadLE(m_N[0]);
	_pFile->ReadLE(m_N[1]);
	_pFile->ReadLE(m_N[2]);*/
}

void CSB_CompactVertex::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CSB_CompactVertex_Write, MAUTOSTRIP_VOID);
	Error_static("CSB_CompactVertex::Read", "De-implemented.");
/*	_pFile->WriteLE(m_V[0]);
	_pFile->WriteLE(m_V[1]);
	_pFile->WriteLE(m_V[2]);
	_pFile->WriteLE(m_N[0]);
	_pFile->WriteLE(m_N[1]);
	_pFile->WriteLE(m_N[2]);*/
}

// -------------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(CSB_LightMap);


void CSB_LightMap::Decompress(int _iRow, CVec3Dfp32* _pDestV, CVec3Dfp32* _pDestN, int _nV)
{
	MAUTOSTRIP(CSB_LightMap_Decompress, MAUTOSTRIP_VOID);
	const CSB_CompactVertex* pV = &m_lVertices[_iRow*m_Width];
	fp32 sx = m_Scale.k[0];
	fp32 sy = m_Scale.k[1];
	fp32 sz = m_Scale.k[2];
	fp32 ox = m_Offset.k[0];
	fp32 oy = m_Offset.k[1];
	fp32 oz = m_Offset.k[2];

	for(int v = 0; v < _nV; v++)
	{
		_pDestV[v].k[0] = fp32(pV[v].m_V[0]) * sx + ox;
		_pDestV[v].k[1] = fp32(pV[v].m_V[1]) * sy + oy;
		_pDestV[v].k[2] = fp32(pV[v].m_V[2]) * sz + oz;
	}

	if (_pDestN)
	{
		fp32 sn = 1.0f / CSB_COMPACTVERTEXSIZE;
		for(int v = 0; v < _nV; v++)
		{
			_pDestN[v].k[0] = fp32(pV[v].m_N[0]) * sn;
			_pDestN[v].k[1] = fp32(pV[v].m_N[1]) * sn;
			_pDestN[v].k[2] = fp32(pV[v].m_N[2]) * sn;
		}
	}
}

void CSB_LightMap::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CSB_LightMap_Read, MAUTOSTRIP_VOID);
	_pFile->ReadLE(m_Width);
	_pFile->ReadLE(m_Height);
	m_Scale.Read(_pFile);
	m_Offset.Read(_pFile);

	uint32 nV = 0;
	_pFile->ReadLE(nV);
	m_lVertices.SetLen(nV);
	for(int v = 0; v < nV; v++)
		m_lVertices[v].Read(_pFile);
}

void CSB_LightMap::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CSB_LightMap_Write, MAUTOSTRIP_VOID);
	_pFile->WriteLE(m_Width);
	_pFile->WriteLE(m_Height);
	m_Scale.Write(_pFile);
	m_Offset.Write(_pFile);

	uint32 nV = m_lVertices.Len();
	_pFile->WriteLE(nV);
	for(int v = 0; v < nV; v++)
		m_lVertices[v].Write(_pFile);
}
#endif


// -------------------------------------------------------------------
CSplineBrush_Edge::CSplineBrush_Edge(fp32 _Len, const CVec3Dfp32 r0, const CVec3Dfp32& r1, const CVec3Dfp32& r2, const CVec3Dfp32& r3)
{
	MAUTOSTRIP(CSplineBrush_Edge_ctor, MAUTOSTRIP_VOID);
	SetMatrix(r0,r1,r2,r3);
	m_Flags = 0;
	m_Tess = 0;
	m_iV[0] = 0;
	m_iV[1] = 0;
	m_iFaces[0] = -1;
	m_iFaces[1] = -1;
	m_Len = _Len;
}

void CSplineBrush_Edge::SetMatrix(const CVec3Dfp32& r0, const CVec3Dfp32& r1, const CVec3Dfp32& r2, const CVec3Dfp32& r3)
{
	MAUTOSTRIP(CSplineBrush_Edge_SetMatrix, MAUTOSTRIP_VOID);
	m_Mat[0][0] = r0.k[0];
	m_Mat[0][1] = r0.k[1];
	m_Mat[0][2] = r0.k[2];
	m_Mat[1][0] = r1.k[0];
	m_Mat[1][1] = r1.k[1];
	m_Mat[1][2] = r1.k[2];
	m_Mat[2][0] = r2.k[0];
	m_Mat[2][1] = r2.k[1];
	m_Mat[2][2] = r2.k[2];
	m_Mat[3][0] = r3.k[0];
	m_Mat[3][1] = r3.k[1];
	m_Mat[3][2] = r3.k[2];

	if ((Sqr(r2.k[0]) + Sqr(r2.k[1]) + Sqr(r2.k[2]) < CSB_EPSILON) &&
		(Sqr(r3.k[0]) + Sqr(r3.k[1]) + Sqr(r3.k[2]) < CSB_EPSILON))
		m_Flags |= CSB_EDGE_LINE;
}

void CSplineBrush_Edge::SetVectors4(const CVec3Dfp32 &v0, const CVec3Dfp32 &v1, const CVec3Dfp32 &_dv0, const CVec3Dfp32 &_dv1)
{
	MAUTOSTRIP(CSplineBrush_Edge_SetVectors4, MAUTOSTRIP_VOID);
/*
	f(t) = at3 + bt2 + ct + d
	f(0) = d = v0
	f(1) = a + b + c + d = v1
	f'(t) = 3at2 + 2bt + c
	f'(0) = c
	f'(1) = 3a + 2b + c

	=>
	d = v0
	c = dv0
	3a + 2b + c = dv1    \
	a + b + c + d = v1  /

	a - c - 2d = dv1 - 2v1
	a = dv1 - 2v1 + c + 2d

	a - dv0 = dv1 - 2v1
	a = dv1 + dv0 - 2v1

	b = v1 - v0 - dv0 - a = 
	  = v1 - v0 - dv0 - dv1 - dv0 + 2v1 = 
	  = 3v1 - v0 - 2dv0 - dv1
*/
	CVec3Dfp32 a,b,c,d;

	CVec3Dfp32 dv, dvn, dv0, dv1, dv0n, dv1n;
	v1.Sub(v0, dv);
	dvn = dv; dvn.Normalize();
	dv0n = _dv0; dv0n.Normalize();
	dv1n = _dv1; dv1n.Normalize();

	if (((dv0n / dvn).LengthSqr() < Sqr(0.05f)) && ((dv1n / dvn).LengthSqr() < Sqr(0.05f)))
	{
		d = v0;
		c = dv;
		a = 0;
		b = 0;
		SetMatrix(d, c, b, a);
	}
	else
	{
		_dv0.Sub(dv, dv0);
		_dv1.Sub(dv, dv1);
		d = v0;
		c = dv0;
		a = dv1 - (v1*2.0f) + c + (d*2.0f);
		b = v1 - a - c - d;
		SetMatrix(d, c, b, a);
	}
}

void CSplineBrush_Edge::GetVectors4(CVec3Dfp32 &v0, CVec3Dfp32 &v1, CVec3Dfp32 &_dv0, CVec3Dfp32 &_dv1) const
{
	MAUTOSTRIP(CSplineBrush_Edge_GetVectors4, MAUTOSTRIP_VOID);
	CVec3Dfp32 d(m_Mat[0][0], m_Mat[0][1], m_Mat[0][2]);
	CVec3Dfp32 c(m_Mat[1][0], m_Mat[1][1], m_Mat[1][2]);
	CVec3Dfp32 b(m_Mat[2][0], m_Mat[2][1], m_Mat[2][2]);
	CVec3Dfp32 a(m_Mat[3][0], m_Mat[3][1], m_Mat[3][2]);
	CVec3Dfp32 dv;

	v0 = d;
	v1 = a + b + c + d;
	v1.Sub(v0, dv);
	_dv0 = c;
//	_dv1 = a - _dv0 + v1*2.0f;
	_dv1 = a*3 + b*2 + c;
	_dv0 += dv;
	_dv1 += dv;
}

fp32 CSplineBrush_Edge::GetLength(int _SampleResolution)
{
	MAUTOSTRIP(CSplineBrush_Edge_GetLength, 0.0f);
	fp32 Len = 0;
	CVec3Dfp32 vlast(0);
	AddEdge(vlast, 0, 1.0f);
	for(int i = 1; i < _SampleResolution; i++)
	{
		fp32 t = fp32(i) / fp32(_SampleResolution-1);
		CVec3Dfp32 vt(0);
		AddEdge(vt, t, 1.0f);
		Len += vt.Distance(vlast);
		vlast = vt;
	}

	return Len;
}

void CSplineBrush_Edge::Flip()
{
	MAUTOSTRIP(CSplineBrush_Edge_Flip, MAUTOSTRIP_VOID);
	CVec3Dfp32 v0,v1,dv0,dv1;
	GetVectors4(v0,v1,dv0,dv1);
//	Swap(v0, v1);
//	v1.Sub(v0, dv0);
//	dv1 = dv0;
//	dv0 = -dv0;
//	dv1 = -dv1;
//	SetVectors4(v0,v1,dv0,dv1);
	SetVectors4(v1,v0,-dv1,-dv0);
	Swap(m_iV[0], m_iV[1]);
}

void CSplineBrush_Edge::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CSplineBrush_Edge_Read, MAUTOSTRIP_VOID);
	for(int i = 0; i < CSB_EDGE_MATROWS; i++)
		for(int j = 0; j < 3; j++) _pFile->ReadLE(m_Mat[i][j]);

	_pFile->ReadLE(m_Flags);
	_pFile->ReadLE(m_Tess);
	_pFile->ReadLE(m_iV[0]);
	_pFile->ReadLE(m_iV[1]);
	_pFile->ReadLE(m_iFaces[0]);
	_pFile->ReadLE(m_iFaces[1]);
	_pFile->ReadLE(m_Len);
	_pFile->ReadLE(m_dVdT);
}

void CSplineBrush_Edge::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CSplineBrush_Edge_Write, MAUTOSTRIP_VOID);
	for(int i = 0; i < CSB_EDGE_MATROWS; i++)
		for(int j = 0; j < 3; j++) _pFile->WriteLE(m_Mat[i][j]);

	_pFile->WriteLE(m_Flags);
	_pFile->WriteLE(m_Tess);
	_pFile->WriteLE(m_iV[0]);
	_pFile->WriteLE(m_iV[1]);
	_pFile->WriteLE(m_iFaces[0]);
	_pFile->WriteLE(m_iFaces[1]);
	_pFile->WriteLE(m_Len);
	_pFile->WriteLE(m_dVdT);
}

// -------------------------------------------------------------------
CVec2Dfp32 CSplineBrush_Face::EvalUV_ST(const CSplineBrush* _pSB, fp32 _s, fp32 _t) const
{
	MAUTOSTRIP(CSplineBrush_Face_EvalUV_ST, CVec2Dfp32());
	CVec2Dfp32 UV;
	UV.k[0] = m_Mapping[0][0] * _s + m_Mapping[1][0] * _t + m_Mapping[2][0];
	UV.k[1] = m_Mapping[0][1] * _s + m_Mapping[1][1] * _t + m_Mapping[2][1];
	return UV;
}


CVec2Dfp32 CSplineBrush_Face::EvalUV_XYZ(const CSplineBrush* _pSB, const CVec3Dfp32& _v) const
{
	MAUTOSTRIP(CSplineBrush_Face_EvalUV_XYZ, CVec2Dfp32());
	CVec2Dfp32 UV;
	CMat4Dfp32 XForm;
	_pSB->m_TextureTransform.Inverse(XForm);
	CVec3Dfp32 v;
	_v.MultiplyMatrix(XForm, v);
	UV.k[0] = m_Mapping[0][0] * v.k[0] + m_Mapping[1][0] * v.k[1] + m_Mapping[2][0] * v.k[2] + m_Mapping[3][0];
	UV.k[1] = m_Mapping[0][1] * v.k[0] + m_Mapping[1][1] * v.k[1] + m_Mapping[2][1] * v.k[2] + m_Mapping[3][1];
	return UV;
}

CVec2Dfp32 CSplineBrush_Face::EvalUV(const CSplineBrush* _pSB, fp32 _s, fp32 _t, const CVec3Dfp32& _v) const
{
	MAUTOSTRIP(CSplineBrush_Face_EvalUV, CVec2Dfp32());
	int Mapping = (m_Flags >> XW_FACE_MAPPING_SHIFT) & XW_FACE_MAPPING_MASK;
	switch(Mapping)
	{
	case XW_FACE_MAPPING_FIXEDST :
	case XW_FACE_MAPPING_ST : return EvalUV_ST(_pSB, _s, _t);
	case XW_FACE_MAPPING_XYZ : return EvalUV_XYZ(_pSB, _v);
	default : return 0;
	}
}

CVec2Dfp32 CSplineBrush_Face::EvalUVLM(fp32 _s, fp32 _t, const CVec3Dfp32& _v) const
{
	MAUTOSTRIP(CSplineBrush_Face_EvalUVLM, CVec2Dfp32());
/*	CVec2Dfp32 UV;
	int Mapping = (m_Flags >> XW_FACE_MAPPING_SHIFT) & XW_FACE_MAPPING_MASK;
	switch(Mapping)
	{
	case XW_FACE_MAPPING_FIXEDST :
	case XW_FACE_MAPPING_ST : UV = EvalUV_ST(_s, _t); break;
	case XW_FACE_MAPPING_XYZ : UV = EvalUV_XYZ(_v); break;
	default : return 0;
	}
	return CVec2Dfp32(
		m_LightMapping[0][0] * UV.k[0] + m_LightMapping[1][0],
		m_LightMapping[0][1] * UV.k[1] + m_LightMapping[1][1]);*/
	return CVec2Dfp32(
		m_LightMapping[0][0] * _s + m_LightMapping[1][0],
		m_LightMapping[0][1] * _t + m_LightMapping[1][1]);
}

void CSplineBrush_Face::EvalUV(int _nV, const CVec3Dfp32* _pV, const CVec2Dfp32* _pST, CVec2Dfp32* _pTV) const
{
	MAUTOSTRIP(CSplineBrush_Face_EvalUV_2, MAUTOSTRIP_VOID);
	// Using the same array in _pST and _pTV will yield CORRECT result.

	int Mapping = (m_Flags >> XW_FACE_MAPPING_SHIFT) & XW_FACE_MAPPING_MASK;
	switch(Mapping)
	{
	case XW_FACE_MAPPING_FIXEDST :
	case XW_FACE_MAPPING_ST :
		{
		#ifdef PLATFORM_PS2
			fp32 uMin = FLT_MAX, vMin = FLT_MAX;
			int v;
			for(v = 0; v < _nV; v++)
			{
				fp32 s = _pST[v].k[0];
				fp32 t = _pST[v].k[1];
				_pTV[v].k[0] = m_Mapping[0][0] * s + m_Mapping[1][0] * t + m_Mapping[2][0];
				_pTV[v].k[1] = m_Mapping[0][1] * s + m_Mapping[1][1] * t + m_Mapping[2][1];
				if(_pTV[v].k[0] < uMin) uMin = _pTV[v].k[0];
				if(_pTV[v].k[1] < vMin) vMin = _pTV[v].k[1];
			}
			uMin = RoundToInt(uMin);
			vMin = RoundToInt(vMin);
			for(v = 0; v < _nV; v++)
			{
				_pTV[v].k[0] -= uMin;
				_pTV[v].k[1] -= vMin;
			}
		#else
			for(int v = 0; v < _nV; v++)
			{
				fp32 s = _pST[v].k[0];
				fp32 t = _pST[v].k[1];
				_pTV[v].k[0] = m_Mapping[0][0] * s + m_Mapping[1][0] * t + m_Mapping[2][0];
				_pTV[v].k[1] = m_Mapping[0][1] * s + m_Mapping[1][1] * t + m_Mapping[2][1];
			}
		#endif
			return;
		}
	case XW_FACE_MAPPING_XYZ :
		{
		#ifdef PLATFORM_PS2
			fp32 uMin = FLT_MAX, vMin = FLT_MAX;
			int v;
			for(v = 0; v < _nV; v++)
			{
				_pTV[v].k[0] = m_Mapping[0][0] * _pV[v].k[0] + m_Mapping[1][0] * _pV[v].k[1] + m_Mapping[2][0] * _pV[v].k[2] + m_Mapping[3][0];
				_pTV[v].k[1] = m_Mapping[0][1] * _pV[v].k[0] + m_Mapping[1][1] * _pV[v].k[1] + m_Mapping[2][1] * _pV[v].k[2] + m_Mapping[3][1];
				if(_pTV[v].k[0] < uMin) uMin = _pTV[v].k[0];//RoundToInt(_pTV[v].k[0]);
				if(_pTV[v].k[1] < vMin) vMin = _pTV[v].k[1];//RoundToInt(_pTV[v].k[1]);
			}
			uMin = RoundToInt(uMin);
			vMin = RoundToInt(vMin);
			for(v = 0; v < _nV; v++)
			{
				_pTV[v].k[0] -= uMin;
				_pTV[v].k[1] -= vMin;
			}
		#else
			for(int v = 0; v < _nV; v++)
			{
				_pTV[v].k[0] = m_Mapping[0][0] * _pV[v].k[0] + m_Mapping[1][0] * _pV[v].k[1] + m_Mapping[2][0] * _pV[v].k[2] + m_Mapping[3][0];
				_pTV[v].k[1] = m_Mapping[0][1] * _pV[v].k[0] + m_Mapping[1][1] * _pV[v].k[1] + m_Mapping[2][1] * _pV[v].k[2] + m_Mapping[3][1];
			}
		#endif
			return;
		}
	default :
		{
			for(int v = 0; v < _nV; v++)
			{
				_pTV[v].k[0] = 0;
				_pTV[v].k[1] = 0;
			}
			return;
		}
	}
}

void CSplineBrush_Face::EvalUVLM(int _nV, const CVec3Dfp32* _pV, const CVec2Dfp32* _pST, CVec2Dfp32* _pTV, CVec2Dfp32* _pLMTV) const
{
	MAUTOSTRIP(CSplineBrush_Face_EvalUVLM_2, MAUTOSTRIP_VOID);
	// Using the same array in _pST and _pTV will yield CORRECT result.

	int Mapping = (m_Flags >> XW_FACE_MAPPING_SHIFT) & XW_FACE_MAPPING_MASK;
	switch(Mapping)
	{
	case XW_FACE_MAPPING_FIXEDST :
	case XW_FACE_MAPPING_ST :
		{
		#ifdef PLATFORM_PS2
			int v;
			fp32 uMin = FLT_MAX, vMin = FLT_MAX;
			for(v = 0; v < _nV; v++)
			{
				fp32 s = _pST[v].k[0];
				fp32 t = _pST[v].k[1];
				_pTV[v].k[0] = m_Mapping[0][0] * s + m_Mapping[1][0] * t + m_Mapping[2][0];
				_pTV[v].k[1] = m_Mapping[0][1] * s + m_Mapping[1][1] * t + m_Mapping[2][1];
				_pLMTV[v].k[0] = m_LightMapping[0][0] * s + m_LightMapping[1][0];
				_pLMTV[v].k[1] = m_LightMapping[0][1] * t + m_LightMapping[1][1];
				if(_pTV[v].k[0] < uMin) uMin = _pTV[v].k[0];
				if(_pTV[v].k[1] < vMin) vMin = _pTV[v].k[1];
			}
			uMin = RoundToInt(uMin);
			vMin = RoundToInt(vMin);
			for(v = 0; v < _nV; v++)
			{
				_pTV[v].k[0] -= uMin;
				_pTV[v].k[1] -= vMin;
			}
		#else
			for(int v = 0; v < _nV; v++)
			{
				fp32 s = _pST[v].k[0];
				fp32 t = _pST[v].k[1];
				_pTV[v].k[0] = m_Mapping[0][0] * s + m_Mapping[1][0] * t + m_Mapping[2][0];
				_pTV[v].k[1] = m_Mapping[0][1] * s + m_Mapping[1][1] * t + m_Mapping[2][1];
//				_pLMTV[v].k[0] = m_LightMapping[0][0] * _pTV[v].k[0] + m_LightMapping[1][0];
//				_pLMTV[v].k[1] = m_LightMapping[0][1] * _pTV[v].k[1] + m_LightMapping[1][1];
				_pLMTV[v].k[0] = m_LightMapping[0][0] * s + m_LightMapping[1][0];
				_pLMTV[v].k[1] = m_LightMapping[0][1] * t + m_LightMapping[1][1];
			}
		#endif
			return;
		}
	case XW_FACE_MAPPING_XYZ :
		{
//		ConOut(CStrF("XYZ-LM %d", _nV));
//		ConOut(CStrF("%f, %f, %f, %f, %f, %f, %f, %f", 
//			m_Mapping[0][0], m_Mapping[1][0], m_Mapping[2][0], m_Mapping[3][0], 
//			m_Mapping[0][1], m_Mapping[1][1], m_Mapping[2][1], m_Mapping[3][1]));
		#ifdef PLATFORM_PS2
			fp32 uMin = FLT_MAX, vMin = FLT_MAX;
			int v;
			for(v = 0; v < _nV; v++)
			{
				fp32 s = _pST[v].k[0];
				fp32 t = _pST[v].k[1];
				_pLMTV[v].k[0] = m_LightMapping[0][0] * s + m_LightMapping[1][0];
				_pLMTV[v].k[1] = m_LightMapping[0][1] * t + m_LightMapping[1][1];
				_pTV[v].k[0] = m_Mapping[0][0] * _pV[v].k[0] + m_Mapping[1][0] * _pV[v].k[1] + m_Mapping[2][0] * _pV[v].k[2] + m_Mapping[3][0];
				_pTV[v].k[1] = m_Mapping[0][1] * _pV[v].k[0] + m_Mapping[1][1] * _pV[v].k[1] + m_Mapping[2][1] * _pV[v].k[2] + m_Mapping[3][1];
				if(_pTV[v].k[0] < uMin) uMin = _pTV[v].k[0];
				if(_pTV[v].k[1] < vMin) vMin = _pTV[v].k[1];
			}
			uMin = RoundToInt(uMin);
			vMin = RoundToInt(vMin);
			for(v = 0; v < _nV; v++)
			{
				_pTV[v].k[0] -= uMin;
				_pTV[v].k[1] -= vMin;
			}
		#else
			for(int v = 0; v < _nV; v++)
			{
				fp32 s = _pST[v].k[0];
				fp32 t = _pST[v].k[1];
				_pLMTV[v].k[0] = m_LightMapping[0][0] * s + m_LightMapping[1][0];
				_pLMTV[v].k[1] = m_LightMapping[0][1] * t + m_LightMapping[1][1];
				_pTV[v].k[0] = m_Mapping[0][0] * _pV[v].k[0] + m_Mapping[1][0] * _pV[v].k[1] + m_Mapping[2][0] * _pV[v].k[2] + m_Mapping[3][0];
				_pTV[v].k[1] = m_Mapping[0][1] * _pV[v].k[0] + m_Mapping[1][1] * _pV[v].k[1] + m_Mapping[2][1] * _pV[v].k[2] + m_Mapping[3][1];
//				_pLMTV[v].k[0] = m_LightMapping[0][0] * _pTV[v].k[0] + m_LightMapping[1][0];
//				_pLMTV[v].k[1] = m_LightMapping[0][1] * _pTV[v].k[1] + m_LightMapping[1][1];
			}
		#endif
			return;
		}
	default :
		{
			for(int v = 0; v < _nV; v++)
			{
				_pTV[v].k[0] = 0;
				_pTV[v].k[1] = 0;
				_pLMTV[v].k[0] = 0;
				_pLMTV[v].k[1] = 0;
			}
			return;
		}
	}
}

void CSplineBrush_Face::TransformMapping(const CVec2Dfp32 &_O, const CVec2Dfp32 &_S, float _Rot)
{
	MAUTOSTRIP(CSplineBrush_Face_TransformMapping, MAUTOSTRIP_VOID);
	int Mapping = (m_Flags >> XW_FACE_MAPPING_SHIFT) & XW_FACE_MAPPING_MASK;
	switch(Mapping)
	{
	case XW_FACE_MAPPING_ST :
	case XW_FACE_MAPPING_FIXEDST:
		{
			CVec2Dfp32 S(_S);
			if (Mapping == XW_FACE_MAPPING_FIXEDST)
			{
				S.k[0] = 1.0f / S.k[0];
				S.k[1] = 1.0f / S.k[1];
			}
			CVec2Dfp32 SU(m_Mapping[0][0], m_Mapping[0][1]);
			CVec2Dfp32 SV(m_Mapping[1][0], m_Mapping[1][1]);
			fp32 v0 =  CVec3Dfp32::AngleFromVector(m_Mapping[0][0] / SU.Length(), m_Mapping[1][0] / SV.Length());
			RotateElements(SU.k[0], SU.k[1], -v0);
			RotateElements(SV.k[0], SV.k[1], -v0);
			fp32 MagU = SU.Length() * S.k[0];
			fp32 MagV = SV.Length() * S.k[1];

			{
				//Fix for neagive scale
				float vU = CVec3Dfp32::AngleFromVector(SU[0], SU[1]);
				float vV = CVec3Dfp32::AngleFromVector(SV[0], SV[1]);
				if(vV - vU > 0.5f || vV - vU < 0)
				{
					MagV = -MagV;
 					v0 = -v0;
				}
//				LogFile(CStrF("T: (%f %f) (%f %f) (%f %f) %f", SU[0], SU[1], SV[0], SV[1], vU * 360, vV * 360, vV - vU));
			}

			//			RotateElements(S.k[0], S.k[1], _Rot);
//			fp32 v0 = CVec3Dfp32::AngleFromVector(m_Mapping[0][0], m_Mapping[1][0]);
//			fp32 MagU = Length2(m_Mapping[0][0], m_Mapping[1][0]) * S.k[0];
//			fp32 MagV = Length2(m_Mapping[0][1], m_Mapping[1][1]) * S.k[1];
			v0 += _Rot;
			m_Mapping[0][0] = MagU;
			m_Mapping[1][0] = 0;
			m_Mapping[0][1] = 0;
			m_Mapping[1][1] = MagV;
			RotateElements(m_Mapping[0][0], m_Mapping[0][1], v0);
			RotateElements(m_Mapping[1][0], m_Mapping[1][1], v0);

/*			m_Mapping[0][0] = cos(v0*2.0*_PI)*MagU;
			m_Mapping[1][0] = sin(v0*2.0*_PI)*MagU;
			m_Mapping[0][1] = cos(v1*2.0*_PI)*MagV;
			m_Mapping[1][1] = sin(v1*2.0*_PI)*MagV;*/
			m_Mapping[2][0] += _O.k[0];
			m_Mapping[2][1] += _O.k[1];
//			LogFile(CStrF("M: (%f %f) (%f %f) (%f %f) (%f %f)", m_Mapping[0][0], m_Mapping[0][1], m_Mapping[1][0], m_Mapping[1][1], m_Mapping[2][0], m_Mapping[2][1], m_Mapping[3][0], m_Mapping[3][1]));
		}
		break;

	case XW_FACE_MAPPING_XYZ :
		{
			// Rotates U & V-vectors around normal.
			CVec3Dfp32 U(m_Mapping[0][0], m_Mapping[1][0], m_Mapping[2][0]);
			CVec3Dfp32 V(m_Mapping[0][1], m_Mapping[1][1], m_Mapping[2][1]);
			CVec3Dfp32 N;
			U.CrossProd(V, N);
			CQuatfp32 QRot(N, _Rot);
			CMat4Dfp32 MatRot;
			QRot.CreateMatrix(MatRot);
			U *= MatRot;
			V *= MatRot;
			U *= 1.0f / _S.k[0];
			V *= 1.0f / _S.k[1];
			m_Mapping[0][0] = U.k[0];
			m_Mapping[1][0] = U.k[1];
			m_Mapping[2][0] = U.k[2];
			m_Mapping[3][0] += _O.k[0];
			m_Mapping[0][1] = V.k[0];
			m_Mapping[1][1] = V.k[1];
			m_Mapping[2][1] = V.k[2];
			m_Mapping[3][1] += _O.k[1];
		}
		break;
	}
}

void CSplineBrush_Face::ScaleMapping(const CVec2Dfp32& _Scale)
{
	MAUTOSTRIP(CSplineBrush_Face_ScaleMapping, MAUTOSTRIP_VOID);
	for(int r = 0; r < XW_FACE_MAPPINGMATRIXROWS; r++)
	{
		m_Mapping[r][0] *= _Scale.k[0];
		m_Mapping[r][1] *= _Scale.k[1];
	}

//	m_LightMapping[0][0] /= _Scale.k[0];
//	m_LightMapping[0][1] /= _Scale.k[1];
/*	for(r = 0; r < 2; r++)
	{
		m_LightMapping[0][r] /= _Scale.k[0];
		m_LightMapping[1][r] /= _Scale.k[1];
	}*/
}

void CSplineBrush_Face::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CSplineBrush_Face_Read, MAUTOSTRIP_VOID);
	_pFile->ReadLE(m_Flags);
	uint16 nEdges;
	_pFile->ReadLE(nEdges);
	m_nEdges = nEdges;
#if defined( PLATFORM_DOLPHIN)
	uint32 Temp32;
	uint16 Temp16;
	_pFile->ReadLE(Temp32); m_iiEdges = Temp32;
	_pFile->ReadLE(Temp32); m_iSurface = Temp32;
	_pFile->ReadLE(Temp16);// m_nLightInfo
	_pFile->ReadLE(Temp16); m_iLightInfo = Temp16;

#else
	_pFile->ReadLE(m_iiEdges);
	_pFile->ReadLE(m_iSurface);

//	_pFile->ReadLE(m_iLMVertexMap);
	_pFile->ReadLE(m_nLightInfo);
	_pFile->ReadLE(m_iLightInfo);
#endif
	// Mapping
	int r;
	for(r = 0; r < XW_FACE_MAPPINGMATRIXROWS; r++)
		for(int c = 0; c < XW_FACE_MAPPINGMATRIXCOLUMNS; c++)
			_pFile->ReadLE(m_Mapping[r][c]);

	// Lightmap Mapping
	for(r = 0; r < 2; r++)
		for(int c = 0; c < 2; c++)
			_pFile->ReadLE(m_LightMapping[r][c]);

	m_BoundBox.Read(_pFile);
}

void CSplineBrush_Face::Write(CCFile* _pFile) const
{
	MAUTOSTRIP(CSplineBrush_Face_Write, MAUTOSTRIP_VOID);
	_pFile->WriteLE(m_Flags);
	uint16 nEdges = m_nEdges; // + (m_PhysTessShiftS << 16) + (m_PhysTessShiftT << (16+4));
	_pFile->WriteLE(nEdges);
	_pFile->WriteLE(m_iiEdges);
	_pFile->WriteLE(m_iSurface);

//	_pFile->WriteLE(m_iLMVertexMap);
#if !defined(PLATFORM_DOLPHIN)
	_pFile->WriteLE(m_nLightInfo);
#endif
	_pFile->WriteLE(m_iLightInfo);

	// Mapping
	int r;
	for(r = 0; r < XW_FACE_MAPPINGMATRIXROWS; r++)
		for(int c = 0; c < XW_FACE_MAPPINGMATRIXCOLUMNS; c++)
			_pFile->WriteLE(m_Mapping[r][c]);

	// Lightmap Mapping
	for(r = 0; r < 2; r++)
		for(int c = 0; c < 2; c++)
			_pFile->WriteLE(m_LightMapping[r][c]);

	m_BoundBox.Write(_pFile);
}

// -------------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(CSplineBrush);


CXW_SurfaceKeyFrame CSplineBrush::m_TmpSurfKeyFrame;

int CSplineBrush::AddEdge(int _iv0, int _iv1, const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CSplineBrush_AddEdge, 0);
	fp32 Len = (m_lVertices[_iv1] - m_lVertices[_iv0]).Length();
//	int iE = m_lEdges.Add(CSplineBrush_Edge(Len, m_lVertices[_iv0], m_lVertices[_iv1]-m_lVertices[_iv0]+_v, -_v));
	int iE = m_lEdges.Add(CSplineBrush_Edge());
	m_lEdges[iE].m_Len = Len;
	m_lEdges[iE].SetVectors(m_lVertices[_iv0], m_lVertices[_iv1], _v);
	m_lEdges[iE].m_iV[0] = _iv0;
	m_lEdges[iE].m_iV[1] = _iv1;
	return iE;
}

int CSplineBrush::AddEdge4(int _iv0, int _iv1, const CVec3Dfp32& _dv0, const CVec3Dfp32& _dv1)
{
	MAUTOSTRIP(CSplineBrush_AddEdge4, 0);
	fp32 Len = (m_lVertices[_iv1] - m_lVertices[_iv0]).Length();
//	int iE = m_lEdges.Add(CSplineBrush_Edge(Len, m_lVertices[_iv0], m_lVertices[_iv1]-m_lVertices[_iv0]+_v, -_v));
	int iE = m_lEdges.Add(CSplineBrush_Edge());
	m_lEdges[iE].m_Len = Len;
	m_lEdges[iE].SetVectors4(m_lVertices[_iv0], m_lVertices[_iv1], _dv0, _dv1);
	m_lEdges[iE].m_iV[0] = _iv0;
	m_lEdges[iE].m_iV[1] = _iv1;

	CVec3Dfp32 v0,v1,dv0,dv1;
	m_lEdges[iE].GetVectors4(v0,v1,dv0,dv1);
//	LogFile("Before: " + _dv0.GetString() + _dv1.GetString());
//	LogFile("After:  " + dv0.GetString() + dv1.GetString());

	return iE;
}

int CSplineBrush::AddQuad(int _ie0, int _ie1, int _ie2, int _ie3)
{
	MAUTOSTRIP(CSplineBrush_AddQuad, 0);
	int iie = m_liEdges.Add(_ie0);
	m_liEdges.Add(_ie1);
	m_liEdges.Add(_ie2);
	m_liEdges.Add(_ie3);
	int iFace = m_lFaces.Add(CSplineBrush_Face(iie, 4));
	m_lEdges[_ie0 & 0x7fff].AddFace(iFace);
	m_lEdges[_ie1 & 0x7fff].AddFace(iFace);
	m_lEdges[_ie2 & 0x7fff].AddFace(iFace);
	m_lEdges[_ie3 & 0x7fff].AddFace(iFace);
	return iFace;
}

void CSplineBrush::Face_Delete(int _iFace)
{
	MAUTOSTRIP(CSplineBrush_Face_Delete, MAUTOSTRIP_VOID);
	int iie = m_lFaces[_iFace].m_iiEdges;
	int ne = m_lFaces[_iFace].m_nEdges;
	m_lFaces.Del(_iFace);
	if (m_lFaceSurf.Len())
		m_lFaceSurf.Del(_iFace);
	for(int f = 0; f < m_lFaces.Len(); f++)
		if ((m_lFaces[f].m_iiEdges & 0x7fff) > iie)
			m_lFaces[f].m_iiEdges -= ne;

	m_liEdges.Delx(iie, ne);

	for(int iE = 0; iE < m_lEdges.Len(); iE++)
		m_lEdges[iE].DeleteFaceIndex(_iFace);

	// Todo: Remove edges that aren't used anymore.
}

CSplineBrush::CSplineBrush()
{
	MAUTOSTRIP(CSplineBrush_ctor, MAUTOSTRIP_VOID);
	m_Flags = 0;
	m_LastViewRendered = -1;
	m_DynLightMask = 0;
	m_TextureTransform.Unit();

//	m_Displacement.Read(CRct(-1,-1,-1,-1), "E:\\TEST\\DISPLACEMENT.TGA", IMAGE_MEM_IMAGE);
//	m_Displacement.Write(m_Displacement.GetRect(), "E:\\TEST\\HIRR.TGA");
}

CVec3Dfp32 CSplineBrush::EvalXYZ(int _iFace, fp32 _s, fp32 _t)
{
	MAUTOSTRIP(CSplineBrush_EvalXYZ, CVec3Dfp32());
	CSplineBrush_Face* pF = &m_lFaces[_iFace];
	int ie0 = m_liEdges[pF->m_iiEdges];
	int ie1 = m_liEdges[pF->m_iiEdges+1];
	int ie2 = m_liEdges[pF->m_iiEdges+2];
	int ie3 = m_liEdges[pF->m_iiEdges+3];
	CVec3Dfp32 v(0);
	m_lEdges[ie0 & 0x7fff].AddEdge(v, (ie0 & 0x8000) ? (1.0f - _s) : _s, 1.0f - _t);
	m_lEdges[ie2 & 0x7fff].AddEdge(v, !(ie2 & 0x8000) ? (1.0f - _s) : _s, _t);
	m_lEdges[ie1 & 0x7fff].AddEdge(v, (ie1 & 0x8000) ? (1.0f - _t) : _t, _s); 
	m_lEdges[ie3 & 0x7fff].AddEdge(v, !(ie3 & 0x8000) ? (1.0f - _t) : _t, 1.0f - _s);
	v *= 0.5f;
//	AddDisplacement(_s, _t, v);
	return v;
}

void CSplineBrush::EvalXYZ(int _iFace, fp32 _s, fp32 _t, CVec3Dfp32& _Dst)
{
	MAUTOSTRIP(CSplineBrush_EvalXYZ_2, MAUTOSTRIP_VOID);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];
	int ie0 = m_liEdges[pF->m_iiEdges];
	int ie1 = m_liEdges[pF->m_iiEdges+1];
	int ie2 = m_liEdges[pF->m_iiEdges+2];
	int ie3 = m_liEdges[pF->m_iiEdges+3];
	_Dst = 0;
	m_lEdges[ie0 & 0x7fff].AddEdge(_Dst, (ie0 & 0x8000) ? (1.0f - _s) : _s, 1.0f - _t);
	m_lEdges[ie2 & 0x7fff].AddEdge(_Dst, !(ie2 & 0x8000) ? (1.0f - _s) : _s, _t);
	m_lEdges[ie1 & 0x7fff].AddEdge(_Dst, (ie1 & 0x8000) ? (1.0f - _t) : _t, _s); 
	m_lEdges[ie3 & 0x7fff].AddEdge(_Dst, !(ie3 & 0x8000) ? (1.0f - _t) : _t, 1.0f - _s);
	_Dst *= 0.5f;
}

void CSplineBrush::EvalXYZ_dXYZ(int _iFace, fp32 _s, fp32 _t, CVec3Dfp32& _dXYZdS, CVec3Dfp32& _dXYZdT)
{
	MAUTOSTRIP(CSplineBrush_EvalXYZ_dXYZ, MAUTOSTRIP_VOID);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];
	int ie0 = m_liEdges[pF->m_iiEdges];
	int ie1 = m_liEdges[pF->m_iiEdges+1];
	int ie2 = m_liEdges[pF->m_iiEdges+2];
	int ie3 = m_liEdges[pF->m_iiEdges+3];
	_dXYZdS = 0;
	m_lEdges[ie0 & 0x7fff].AddEdge_dXYZdT(_dXYZdS, (ie0 & 0x8000) ? (1.0f - _s) : _s, (ie0 & 0x8000) ? -(1.0f - _t) : (1.0f - _t));
	m_lEdges[ie2 & 0x7fff].AddEdge_dXYZdT(_dXYZdS, !(ie2 & 0x8000) ? (1.0f - _s) : _s, !(ie2 & 0x8000) ? - _t : _t);
	m_lEdges[ie1 & 0x7fff].AddEdge(_dXYZdS, (ie1 & 0x8000) ? (1.0f - _t) : _t, 1.0f); 
	m_lEdges[ie3 & 0x7fff].AddEdge(_dXYZdS, !(ie3 & 0x8000) ? (1.0f - _t) : _t, -1.0f);
	_dXYZdT = 0;
	m_lEdges[ie1 & 0x7fff].AddEdge_dXYZdT(_dXYZdT, (ie1 & 0x8000) ? (1.0f - _t) : _t, (ie1 & 0x8000) ? -_s : _s); 
	m_lEdges[ie3 & 0x7fff].AddEdge_dXYZdT(_dXYZdT, !(ie3 & 0x8000) ? (1.0f - _t) : _t, !(ie3 & 0x8000) ? -(1.0f - _s) : (1.0f - _s));
	m_lEdges[ie0 & 0x7fff].AddEdge(_dXYZdT, (ie0 & 0x8000) ? (1.0f - _s) : _s, -1.0f);
	m_lEdges[ie2 & 0x7fff].AddEdge(_dXYZdT, !(ie2 & 0x8000) ? (1.0f - _s) : _s, 1.0f);
}


/*

  f(g(x))' = f'(g(x)) * g'(x)
f(g(s,t))' = f'(g(s,t)) * g'(s,t)

            [ fdx(g(s,t)) ]   
dS =        [ fdy(g(s,t)) ] * gds
            [ fdz(g(s,t)) ]   

            [ fdx(g(s,t)) ]   
dT =        [ fdy(g(s,t)) ] * gdt
            [ fdz(g(s,t)) ]   





*/

CVec3Dfp32 CSplineBrush::EvalNormal(int _iFace, fp32 _s, fp32 _t)
{
	MAUTOSTRIP(CSplineBrush_EvalNormal, CVec3Dfp32());
//	CSplineBrush_Face* pF = &m_lFaces[_iFace];

	CVec3Dfp32 dXYZdS, dXYZdT, N;
	EvalXYZ_dXYZ(_iFace, _s, _t, dXYZdS, dXYZdT);
	dXYZdS.CrossProd(dXYZdT, N);
	N.Normalize();
	return N;
}

void CSplineBrush::EvalTangentSpace(int _iFace, fp32 _s, fp32 _t, CVec3Dfp32& _N, CVec3Dfp32& _TangU, CVec3Dfp32& _TangV)
{
	MAUTOSTRIP(CSplineBrush_EvalTangentSpace, MAUTOSTRIP_VOID);
	CSplineBrush_Face& Face = m_lFaces[_iFace];
	CVec3Dfp32 V[4];
	CVec2Dfp32 UV[4];
	CVec3Dfp32 N = EvalNormal(_iFace, _s, _t);
	EvalXYZ(_iFace, _s, _t, V[0]);
	EvalXYZ(_iFace, _s+0.01f, _t, V[1]);
	EvalXYZ(_iFace, _s, _t+0.01f, V[2]);
	V[3] = V[0] + N;


	UV[0] = Face.EvalUV(this, _s, _t, V[0]);
	UV[1] = Face.EvalUV(this, _s+0.01f, _t, V[1]);
	UV[2] = Face.EvalUV(this, _s, _t+0.01f, V[2]);

	UV[1] -= UV[0];
	UV[2] -= UV[0];
	V[1] -= V[0];
	V[2] -= V[0];

	CMat4Dfp32 Unit2UV;
	Unit2UV.Unit();
//	CVec3Dfp32(UV[1].k[0], UV[1].k[1], 0).SetRow(Unit2UV, 0);
//	CVec3Dfp32(UV[2].k[0], UV[2].k[1], 0).SetRow(Unit2UV, 1);
	CVec3Dfp32 _tempa(UV[1].k[0], UV[1].k[1], 0);
	CVec3Dfp32 _tempb(UV[2].k[0], UV[2].k[1], 0);

	_tempa.SetRow( Unit2UV, 0 );
	_tempb.SetRow( Unit2UV, 1 );

	CVec3Dfp32(0, 0, 1).SetRow(Unit2UV, 2);

	CMat4Dfp32 Unit2XYZ;
	Unit2XYZ.Unit();
	V[1].SetRow(Unit2XYZ, 0);
	V[2].SetRow(Unit2XYZ, 1);
	N.SetRow(Unit2XYZ, 2);

	CMat4Dfp32 UV2Unit;
	Unit2UV.Inverse(UV2Unit);
	CMat4Dfp32 XYZ2Unit;
	Unit2XYZ.Inverse(XYZ2Unit);

	CMat4Dfp32 UV2XYZ;
//	Unit2UV.Multiply(XYZ2Unit, UV2XYZ);
	UV2Unit.Multiply(Unit2XYZ, UV2XYZ);

	CVec3Dfp32 TangU = CVec3Dfp32::GetRow(UV2XYZ, 0);
	CVec3Dfp32 TangV = CVec3Dfp32::GetRow(UV2XYZ, 1);
	TangU.Normalize();
	TangV.Normalize();

	_N = N;
	_TangU = TangU;
	_TangV = TangV;
}

void CSplineBrush::EvalNormal(int _iFace, int _nV, const CVec2Dfp32* _pST, CVec3Dfp32* _pN, bool _bNormalize)
{
	MAUTOSTRIP(CSplineBrush_EvalNormal_2, MAUTOSTRIP_VOID);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];
	int ie0 = m_liEdges[pF->m_iiEdges];
	int ie1 = m_liEdges[pF->m_iiEdges+1];
	int ie2 = m_liEdges[pF->m_iiEdges+2];
	int ie3 = m_liEdges[pF->m_iiEdges+3];

	CVec3Dfp32 dXYZdS, dXYZdT, N;
	for(int v = 0; v < _nV; v++)
	{
		fp32 s = _pST[v].k[0];
		fp32 t = _pST[v].k[1];
		dXYZdS = 0;
		m_lEdges[ie0 & 0x7fff].AddEdge_dXYZdT(dXYZdS, (ie0 & 0x8000) ? (1.0f - s) : s, (ie0 & 0x8000) ? -(1.0f - t) : (1.0f - t));
		m_lEdges[ie2 & 0x7fff].AddEdge_dXYZdT(dXYZdS, !(ie2 & 0x8000) ? (1.0f - s) : s, !(ie2 & 0x8000) ? - t : t);
		m_lEdges[ie1 & 0x7fff].AddEdge(dXYZdS, (ie1 & 0x8000) ? (1.0f - t) : t, 1.0f); 
		m_lEdges[ie3 & 0x7fff].AddEdge(dXYZdS, !(ie3 & 0x8000) ? (1.0f - t) : t, -1.0f);
		dXYZdT = 0;
		m_lEdges[ie1 & 0x7fff].AddEdge_dXYZdT(dXYZdT, (ie1 & 0x8000) ? (1.0f - t) : t, (ie1 & 0x8000) ? -s : s); 
		m_lEdges[ie3 & 0x7fff].AddEdge_dXYZdT(dXYZdT, !(ie3 & 0x8000) ? (1.0f - t) : t, !(ie3 & 0x8000) ? -(1.0f - s) : (1.0f - s));
		m_lEdges[ie0 & 0x7fff].AddEdge(dXYZdT, (ie0 & 0x8000) ? (1.0f - s) : s, -1.0f);
		m_lEdges[ie2 & 0x7fff].AddEdge(dXYZdT, !(ie2 & 0x8000) ? (1.0f - s) : s, 1.0f);
		dXYZdS.CrossProd(dXYZdT, _pN[v]);
		if (_bNormalize)
		{
//			_pN[v] *= 1.0f / GetMathAccel()->fsqrt(Sqr(_pN[v][0]) + Sqr(_pN[v][1]) + Sqr(_pN[v][2]));
			_pN[v].Normalize();
		}
	}
}

void CSplineBrush::EvalTangentSpaceArrays(int _iFace, int _nV, const CVec2Dfp32* _pST, CVec3Dfp32* _pN, CVec3Dfp32* _pTangU, CVec3Dfp32* _pTangV)
{
	MAUTOSTRIP(CSplineBrush_EvalTangentSpaceArrays, MAUTOSTRIP_VOID);
	for(int v = 0; v < _nV; v++)
	{
		EvalTangentSpace(_iFace, _pST[v].k[0], _pST[v].k[1], _pN[v], _pTangU[v], _pTangV[v]);
	}
}

void CSplineBrush::GetBoundBox(int _iFace, int _nS, int _nT, fp32 _s, fp32 _t, fp32 _sStep, fp32 _tStep, CVec3Dfp32& _vMin, CVec3Dfp32& _vMax)
{
	MAUTOSTRIP(CSplineBrush_GetBoundBox, MAUTOSTRIP_VOID);
	_vMin = _FP32_MAX;
	_vMax = -_FP32_MAX;

	fp32 fS = _s;
	for(int s = 0; s < _nS; s++)
	{
		fp32 fT = _t;
		for(int t = 0; t < _nT; t++)
		{
			CVec3Dfp32 v(EvalXYZ(_iFace, fS, fT));
			for(int i = 0; i < 3; i++)
			{
				if (v.k[i] < _vMin.k[i]) _vMin.k[i] = v.k[i];
				if (v.k[i] > _vMax.k[i]) _vMax.k[i] = v.k[i];
			}
			fT += _tStep;
		}
		fS += _sStep;
	}
}

int CSplineBrush::EnumBoundNodes_r(int _iFace, int _iNode, const CBox3Dfp32& _Box, uint32* _pST)
{
	MAUTOSTRIP(CSplineBrush_EnumBoundNodes_r, 0);
	CSB_BoundNode* pN = &m_lBoundNodes[_iNode];
	int nNodes = 0;
	if (pN->m_Box.IsInside(_Box))
	{
		if (pN->m_iNodes[0] == -1)
		{
			_pST[nNodes++] = pN->m_STMin[0] + (pN->m_STMin[1] << 16);
			return 1;
		}
		else
		{
			int nST = EnumBoundNodes_r(_iFace, pN->m_iNodes[0], _Box, _pST);
			nST += EnumBoundNodes_r(_iFace, pN->m_iNodes[1], _Box, &_pST[nST]);
			return nST;
		}
	}
	return 0;
}

int CSplineBrush::EnumBoundNodesLine_r(int _iFace, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, uint32* _pST)
{
	MAUTOSTRIP(CSplineBrush_EnumBoundNodesLine_r, 0);
	CSB_BoundNode* pN = &m_lBoundNodes[_iNode];
	int nNodes = 0;

	CVec3Dfp32 HitPos;
	if (pN->m_Box.IntersectLine(_p0, _p1, HitPos))
	{
		if (pN->m_iNodes[0] == -1)
		{
			_pST[nNodes++] = pN->m_STMin[0] + (pN->m_STMin[1] << 16);
			return 1;
		}
		else
		{
			int nST = EnumBoundNodesLine_r(_iFace, pN->m_iNodes[0], _p0, _p1, _pST);
			nST += EnumBoundNodesLine_r(_iFace, pN->m_iNodes[1], _p0, _p1, &_pST[nST]);
			return nST;
		}
	}
	return 0;
}

int CSplineBrush::EnumBoundNodes(int _iFace, const CBox3Dfp32& _Box, uint32* _pST)
{
	MAUTOSTRIP(CSplineBrush_EnumBoundNodes, 0);
	int iNode = m_lFaces[_iFace].m_iBoundNode;
	if (iNode < 0) Error("EnumBoundNodes", "No bounding calculated for face.");

	return EnumBoundNodes_r(_iFace, iNode, _Box, _pST);
}

int CSplineBrush::EnumBoundNodesLine(int _iFace, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, uint32* _pST)
{
	MAUTOSTRIP(CSplineBrush_EnumBoundNodesLine, 0);
	int iNode = m_lFaces[_iFace].m_iBoundNode;
	if (iNode < 0) Error("EnumBoundNodes", "No bounding calculated for face.");

	return EnumBoundNodesLine_r(_iFace, iNode, _p0, _p1, _pST);
}

int CSplineBrush::BuildBoundNodes_r(int _iFace, int _S, int _T, int _nS, int _nT, fp32 _sStep, fp32 _tStep)
{
	MAUTOSTRIP(CSplineBrush_BuildBoundNodes_r, 0);
	CSB_BoundNode Node;
	Node.m_STMin[0] = _S;
	Node.m_STMin[1] = _T;
	Node.m_STMax[0] = _S+_nS;
	Node.m_STMax[1] = _T+_nT;
	GetBoundBox(_iFace, _nS+1, _nT+1, fp32(_S)*_sStep, fp32(_T)*_tStep, _sStep, _tStep, Node.m_Box.m_Min, Node.m_Box.m_Max);
	int iNode = m_lBoundNodes.Add(Node);

	if (_nS == 1 && _nT == 1)
	{
		// Leaf
		m_lBoundNodes[iNode].m_iNodes[0] = -1;
		m_lBoundNodes[iNode].m_iNodes[1] = -1;
	}
	else
	{
		if (_nT < _nS)
		{
			// Split S
			int iNode0 = BuildBoundNodes_r(_iFace, _S, _T, (_nS >> 1), _nT, _sStep, _tStep);
			int iNode1 = BuildBoundNodes_r(_iFace, _S + (_nS >> 1), _T, (_nS >> 1), _nT, _sStep, _tStep);
#ifdef CPU_INTELP5			
			__asm xor eax,eax;
#else
			// ?
#endif
			m_lBoundNodes[iNode].m_iNodes[0] = iNode0;
			m_lBoundNodes[iNode].m_iNodes[1] = iNode1;
		}
		else
		{
			// Split T
			int iNode0 = BuildBoundNodes_r(_iFace, _S, _T, _nS, (_nT >> 1), _sStep, _tStep);
			int iNode1 = BuildBoundNodes_r(_iFace, _S, _T + (_nT >> 1), _nS, (_nT >> 1), _sStep, _tStep);
#ifdef CPU_INTELP5
			__asm xor eax,eax;
#else
			// ?
#endif
			m_lBoundNodes[iNode].m_iNodes[0] = iNode0;
			m_lBoundNodes[iNode].m_iNodes[1] = iNode1;
		}
	}
	return iNode;
}

void CSplineBrush::BuildBoundNodes()
{
	MAUTOSTRIP(CSplineBrush_BuildBoundNodes, MAUTOSTRIP_VOID);
	MSCOPESHORT(CSplineBrush::BuildBoundNodes);
	m_lBoundNodes.Clear();

	//AR-ADD: precalculate the size needed.
	//IMPORTANT: If BuildBoundNodes_r() is changed, the formula below should be adjusted accordingly!
//JK-NOTE: MemOpt, enable this later
//	const CSplineBrush_Face* pFaces = m_lFaces.GetBasePtr();
//	int nFaces = m_lFaces.Len();
//	int iFace;
//	int nLen = 0;
//	for (iFace=0; iFace < nFaces; iFace++)
//		nLen += (1 << (1 + pFaces[iFace].m_PhysTessShiftS + pFaces[iFace].m_PhysTessShiftT)) - 1;
//	m_lBoundNodes.SetGrow(nLen);

	m_lBoundNodes.SetGrow(128);
	for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
	{
//		int nS = (Face_IsFlatS(iFace)) ? 1 : CSB_TESS_PHYSICS;
//		int nT = (Face_IsFlatT(iFace)) ? 1 : CSB_TESS_PHYSICS;
		const CSplineBrush_Face& Face = m_lFaces[iFace];
		int nS = 1 << Face.m_PhysTessShiftS;
		int nT = 1 << Face.m_PhysTessShiftT;
		m_lFaces[iFace].m_iBoundNode = BuildBoundNodes_r(iFace, 0, 0, nS, nT, 1.0f / fp32(nS), 1.0f / fp32(nT));
	}
	m_lBoundNodes.OptimizeMemory();
}

void CSplineBrush::UpdateBoundBox()
{
	MAUTOSTRIP(CSplineBrush_UpdateBoundBox, MAUTOSTRIP_VOID);
	MSCOPESHORT(CSplineBrush::UpdateBoundBox);
	// Create culling planes
	const int MaxCullPlanes = 5;
	m_lCullPlanes.SetLen(m_lFaces.Len() * (Sqr(MaxCullPlanes)+1));
	int iP = 0;
	for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
	{
		CSplineBrush_Face* pF = &m_lFaces[iFace];

		CVec3Dfp32 AvgNormal(0);
		CVec3Dfp32 AvgPos(0);

		int NumCullPlanes = Min(MaxCullPlanes, 3 + TruncToInt(m_BoundVertexBox.m_Max.Distance(m_BoundVertexBox.m_Min) / 256.0f));

		pF->m_iCullPlanes = iP;
		int nS = Face_IsFlatS(iFace) ? 2 : NumCullPlanes;
		int nT = Face_IsFlatT(iFace) ? 2 : NumCullPlanes;
		pF->m_nCullPlanes = 0;
		for(int s = 0; s < nS; s++)
			for(int t = 0; t < nT; t++)
			{
				fp32 fS = fp32(s) / fp32(Max(nS-1, 1));
				fp32 fT = fp32(t) / fp32(Max(nT-1, 1));
				CVec3Dfp32 N = EvalNormal(iFace, fS, fT);
				CVec3Dfp32 V = EvalXYZ(iFace, fS, fT);
				m_lCullPlanes[iP].CreateNV(N, V);

				AvgNormal += N;
				AvgPos += V;

				// Check for already existing identical plane.
				int bEqual = false;
				for(int p = pF->m_iCullPlanes; p < iP; p++)
					if (EqualPlanes(m_lCullPlanes[iP], m_lCullPlanes[p], 0.001f))
					{
						bEqual = true;
						break;
					}

				if (!bEqual)
				{
					iP++;
					pF->m_nCullPlanes++;
				}
			}

		// Create lighting plane and projection vectors.
		AvgNormal.Normalize();
		AvgPos *= 1.0f / fp32(nS*nT);
		m_lCullPlanes[iP].CreateNV(AvgNormal, AvgPos);
		pF->m_iLightingPlane = iP;
		iP++;

		CVec3Dfp32 UVec, VVec;

		if (M_Fabs(AvgNormal[2]) < 0.01f)
		{
			UVec = CVec3Dfp32(0,0,1);
			UVec.CrossProd(AvgNormal, VVec);
			VVec.Normalize();
			VVec.CrossProd(AvgNormal, UVec);
			UVec.Normalize();
		}
		else
		{
			UVec = CVec3Dfp32(1,0,0);
			UVec.CrossProd(AvgNormal, VVec);
			VVec.Normalize();
			VVec.CrossProd(AvgNormal, UVec);
			UVec.Normalize();
		}

		pF->m_LightUVec = UVec;
		pF->m_LightVVec = VVec;
	}
	m_lCullPlanes.SetLen(iP);

	// Calculate bounding box
	{
		CVec3Dfp32 vmin(_FP32_MAX);
		CVec3Dfp32 vmax(-_FP32_MAX);
		for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
		{
			CBox3Dfp32 FaceBox;
			FaceBox.m_Min = _FP32_MAX;
			FaceBox.m_Max = -_FP32_MAX;
			for(int s = 0; s <= CSB_TESS_PHYSICS; s++)
				for(int t = 0; t <= CSB_TESS_PHYSICS; t++)
				{
					CVec3Dfp32 v(EvalXYZ(iFace,fp32(s)/fp32(CSB_TESS_PHYSICS), fp32(t)/fp32(CSB_TESS_PHYSICS)));
					for(int i = 0; i < 3; i++)
					{
						if (v.k[i] < FaceBox.m_Min.k[i]) FaceBox.m_Min.k[i] = v.k[i];
						if (v.k[i] > FaceBox.m_Max.k[i]) FaceBox.m_Max.k[i] = v.k[i];
					}
				}

			m_lFaces[iFace].m_BoundBox = FaceBox;
			for(int i = 0; i < 3; i++)
			{
				if (FaceBox.m_Min.k[i] < vmin.k[i]) vmin.k[i] = FaceBox.m_Min.k[i];
				if (FaceBox.m_Max.k[i] > vmax.k[i]) vmax.k[i] = FaceBox.m_Max.k[i];
			}
		}

		CVec3Dfp32 ProjMax(0);
		CVec3Dfp32 ProjMin(0);
		for(int i = 0; i < m_lCullPlanes.Len(); i++)
		{
			const CPlane3Dfp32& P = m_lCullPlanes[i];
			for(int k = 0; k < 3; k++)
			{
				ProjMax[k] = Max(P.n[k], ProjMax[k]);
				ProjMin[k] = Min(P.n[k], ProjMin[k]);
			}
		}
		
		m_BoundBox.m_Min = vmin + ProjMin;
		m_BoundBox.m_Max = vmax + ProjMax;
	}

	// Lägg på 10%, hirr!#?!@@@... *chuckle*
/*	CVec3Dfp32 boxsize;
	vmax.Sub(vmin, boxsize);
	m_BoundBox.m_Min = vmin - CVec3Dfp32(1);	// Add epsilon to box. (1.0f)
	m_BoundBox.m_Max = vmax + CVec3Dfp32(1);*/

//	m_BoundBox.m_Min = vmin - (boxsize*0.1);
//	m_BoundBox.m_Max = vmax + (boxsize*0.1);

	m_BoundPos = (m_BoundBox.m_Min + m_BoundBox.m_Max) * 0.5f;
	m_BoundRadius = m_BoundBox.m_Max.Distance(m_BoundBox.m_Min) * 0.5f;

	// Calc BoundVertexBox
	{
		CVec3Dfp32 vmin(_FP32_MAX);
		CVec3Dfp32 vmax(-_FP32_MAX);
		int v;
		for(v = 0; v < m_lVertices.Len(); v++)
		{
//			CVec3Dfp32 v(m_lVertices[v]);
			CVec3Dfp32 vv(m_lVertices[v]);
			for(int i = 0; i < 3; i++)
			{
				if (vv.k[i] < vmin.k[i]) vmin.k[i] = vv.k[i];
				if (vv.k[i] > vmax.k[i]) vmax.k[i] = vv.k[i];
			}
		}
		m_BoundVertexBox.m_Min = vmin;
		m_BoundVertexBox.m_Max = vmax;
	}


//	LogFile("BoundBox " + m_BoundBox.m_Min.GetString() + m_BoundBox.m_Max.GetString());
}

void CSplineBrush::UpdatePhysTess()
{
	MAUTOSTRIP(CSplineBrush_UpdatePhysTess, MAUTOSTRIP_VOID);
	MSCOPESHORT(CSplineBrush::UpdatePhysTess);
	fp32 lEdgeLen[256];
	if (m_lEdges.Len() > 256)
		Error("UpdatePhysTess", "Too many edges.");

	// Calculate length of all edges.
	for(int e = 0; e < m_lEdges.Len(); e++)
		lEdgeLen[e] = m_lEdges[e].GetLength(20);

	for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
	{
		CSplineBrush_Face& Face = m_lFaces[iFace];

		int ie0 = m_liEdges[Face.m_iiEdges] & 0x7fff;
		int ie1 = m_liEdges[Face.m_iiEdges+1] & 0x7fff;
		int ie2 = m_liEdges[Face.m_iiEdges+2] & 0x7fff;
		int ie3 = m_liEdges[Face.m_iiEdges+3] & 0x7fff;
		fp32 SLen = Max(lEdgeLen[ie0], lEdgeLen[ie2]);
		fp32 TLen = Max(lEdgeLen[ie1], lEdgeLen[ie3]);

		//Since distance is relative, we need to take the distance between points into account
		CVec3Dfp32 lCorners[4];
		EvalXYZ(iFace,0.0f,0.0f,lCorners[0]);
		EvalXYZ(iFace,1.0f,0.0f,lCorners[1]);
		EvalXYZ(iFace,1.0f,1.0f,lCorners[2]);
		EvalXYZ(iFace,0.0f,1.0f,lCorners[3]);
		fp32 lDist[4];
		lDist[0] = (lCorners[0] - lCorners[1]).LengthSqr();
		lDist[1] = (lCorners[1] - lCorners[2]).LengthSqr();
		lDist[2] = (lCorners[2] - lCorners[3]).LengthSqr();
		lDist[3] = (lCorners[3] - lCorners[0]).LengthSqr();
		fp32 SDist,TDist;
		SDist = Max( lDist[0],lDist[2] );
		TDist = Max( lDist[1],lDist[3] );

		SLen *= SLen;
		TLen *= TLen;

		int SShift = 0;
		if (!Face_IsFlatS(iFace))
		{
			if (SLen > SDist)
				SShift = 3;		// 8*8
			else if (SLen > SDist * 0.75f)
				SShift = 2;		// 4*4
			else if (SLen > SDist * 0.5f)
				SShift = 1;		// 2*2
		}

		int TShift = 0;
		if (!Face_IsFlatT(iFace))
		{
			if (TLen > TDist)
				TShift = 3;		// 8*8
			else if (TLen > TDist*0.75f)
				TShift = 2;		// 4*4
			else if (TLen > TDist * 0.5f)
				TShift = 1;		// 2*2
		}

		Face.m_PhysTessShiftS = SShift;
		Face.m_PhysTessShiftT = TShift;
	}	
}

#ifndef M_RTM

void CSplineBrush::UpdateSurfaces(CXR_SurfaceContext *_pContext)
{
	MAUTOSTRIP(CSplineBrush_UpdateSurfaces, MAUTOSTRIP_VOID);
	int nr = m_lFaces.Len();
	for(int i = 0; i < nr; i++)
	{
		int iIndex = _pContext->GetSurfaceID(m_lFaceSurf[i]);
		if(iIndex != 0)
			m_lFaces[i].m_pSurface = _pContext->GetSurface(iIndex);
		else
			m_lFaces[i].m_pSurface = NULL;

		fp32 InvTxtW = 1.0f;
		fp32 InvTxtH = 1.0f;
		if(m_lFaces[i].m_pSurface)
		{
			CVec3Dfp32 ReferenceDim = m_lFaces[i].m_pSurface->GetTextureMappingReferenceDimensions();
			InvTxtW = 1.0f / ReferenceDim[0];
			InvTxtH = 1.0f / ReferenceDim[1];
		}
		m_lFaces[i].m_LightMapping[0][0] = InvTxtW;
		m_lFaces[i].m_LightMapping[0][1] = InvTxtH;
	}
}

void CSplineBrush::SetSurface(CXW_Surface *_pSurface, int _iFace)
{
	MAUTOSTRIP(CSplineBrush_SetSurface, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CXR_SurfaceContext , pSC, "SYSTEM.SURFACECONTEXT");
	if(!pSC)
		return;

//	int iSurface = pSC->GetSurfaceID(_pSurface->m_Name);

	CVec3Dfp32 ReferenceDim = _pSurface->GetTextureMappingReferenceDimensions();
	fp32 InvTxtW = 1.0f / ReferenceDim[0];
	fp32 InvTxtH = 1.0f / ReferenceDim[1];

	if(_iFace == -1)
	{
		int nr = m_lFaces.Len();
		for(int i = 0; i < nr; i++)
		{
			m_lFaceSurf[i] = _pSurface->m_Name;
			m_lFaces[i].m_pSurface = _pSurface;
			m_lFaces[i].m_LightMapping[0][0] = InvTxtW;
			m_lFaces[i].m_LightMapping[0][1] = InvTxtH;
		}
	}
	else
	{
		m_lFaceSurf[_iFace] = _pSurface->m_Name;
		m_lFaces[_iFace].m_pSurface = _pSurface;
		m_lFaces[_iFace].m_LightMapping[0][0] = InvTxtW;
		m_lFaces[_iFace].m_LightMapping[0][1] = InvTxtH;
	}
}

CXW_Surface *CSplineBrush::GetSurface(int _iFace)
{
	MAUTOSTRIP(CSplineBrush_GetSurface, NULL);
	return m_lFaces[_iFace].m_pSurface;
}

CStr CSplineBrush::GetSurfaceName(int _iFace)
{
	MAUTOSTRIP(CSplineBrush_GetSurfaceName, CStr());
	return m_lFaceSurf[_iFace];
}

#endif

void CSplineBrush::CreateFaceMeshes()
{
	MAUTOSTRIP(CSplineBrush_CreateFaceMeshes, MAUTOSTRIP_VOID);
	MSCOPESHORT(CSplineBrush::CreateFaceMeshes);
	// Init tesselation.
	m_lEdgeTess.SetLen(m_lEdges.Len());
	for(int iE = 0; iE < m_lEdges.Len(); iE++)
		m_lEdgeTess[iE] = GetMinTesselation(&m_lEdges[iE]);

	// Create meshes.
/*	m_lspCompiledFaces.Clear();
	for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
		m_lspCompiledFaces.Add(CreateFaceMesh(iFace));*/
}

void CSplineBrush::ModelChanged(bool _bCompiledBrush)
{
	MAUTOSTRIP(CSplineBrush_ModelChanged, MAUTOSTRIP_VOID);
	UpdateBoundBox();
	UpdatePhysTess();
	BuildBoundNodes();
	InitMinTessDistance();

/*	LogFile("SplineBrush edges.");
	for(int iE = 0; iE < m_lEdges.Len(); iE++)
		LogFile(CStrF("%d, %f, %f", iE, m_lEdges[iE].m_Len, m_lEdges[iE].m_dVdT));*/


	if (!_bCompiledBrush)
	{
		// Length and Curvature is calculated in XWC and should not be destroyed here.

		for(int iE = 0; iE < m_lEdges.Len(); iE++)
		{
			CSplineBrush_Edge* pE = &m_lEdges[iE];
			pE->m_Len = (m_lVertices[pE->m_iV[1]] - m_lVertices[pE->m_iV[0]]).Length();

			// Get average (was max) curvature in degrees/t.
			pE->m_dVdT = 00.0;
			fp32 SumdVdT = 0;
			const int nSamp = 20;
			for(int t = 0; t < 20; t++)
			{
				fp32 fT = fp32(t) / fp32(nSamp - 1);
				
				CVec3Dfp32 v0,v1,v2;
				v0 = 0;
				v1 = 0;
				v2 = 0;
				pE->AddEdge(v0, fT-0.01f, 1.0f);
				pE->AddEdge(v1, fT, 1.0f);
				pE->AddEdge(v2, fT+0.01f, 1.0f);
				CVec3Dfp32 dv0,dv1;
				v1.Sub(v0, dv0);
				v2.Sub(v1, dv1);
				dv0.Normalize();
				dv1.Normalize();
				fp32 dcosv = dv0*dv1;
				fp32 dv = M_ACos(Max(Min(dcosv, 1.0f), -1.0f))*180.0f/_PI;
				dv /= 0.01f;
				SumdVdT += dv;
				if (dv > pE->m_dVdT) 
					pE->m_dVdT = dv;
			}
//LogFile(CStrF("m_dvdT %f -> %f", pE->m_dVdT, SumdVdT / fp32(nSamp)));
			pE->m_dVdT = SumdVdT / fp32(nSamp);

		}
	}

	CreateFaceMeshes();
}

void CSplineBrush::Edge_Flip(int _iE)
{
	MAUTOSTRIP(CSplineBrush_Edge_Flip_2, MAUTOSTRIP_VOID);
	m_lEdges[_iE].Flip();
/*	for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
	{
		CSplineBrush_Face* pF = &m_lFaces[iFace];
		int iie = pF->m_iiEdges;
		int ne = pF->m_nEdges;
		for(int e = 0; e < ne; e++)
			if (m_liEdges[iie + e] & 0x7fff == _iE)
				m_liEdges[iie + e] ^= 0x8000;
	}*/

	for(int e = 0; e < m_liEdges.Len(); e++)
		if ((m_liEdges[e] & 0x7fff) == _iE)
			m_liEdges[e] ^= 0x10000;
}

bool CSplineBrush::TraceLine(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int& _iFace, CVec3Dfp32* _pPos)
{
	MAUTOSTRIP(CSplineBrush_TraceLine, false);
	// True == Trace was ok - no hit.
	// False == Hit, _iFace is set, intersectionpoint is returned in _Pos.

	CBox3Dfp32 linebox;
	for(int i = 0; i < 3; i++)
	{
		linebox.m_Min.k[i] = Min(_p0.k[i], _p1.k[i]);
		linebox.m_Max.k[i] = Max(_p0.k[i], _p1.k[i]);
		if (linebox.m_Max.k[i] < m_BoundBox.m_Min.k[i]) return true;
		if (linebox.m_Min.k[i] > m_BoundBox.m_Max.k[i]) return true;
	}

	fp32 Nearest = _FP32_MAX;
	_iFace = -1;

	for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
	{
		for(int i = 0; i < 3; i++)
		{
			if (linebox.m_Max.k[i] < m_lFaces[iFace].m_BoundBox.m_Min.k[i]) continue;
			if (linebox.m_Min.k[i] > m_lFaces[iFace].m_BoundBox.m_Max.k[i]) continue;
		}

		CVec3Dfp32 Pos;
		if (!Face_TraceLine(iFace, _p0, _p1, Pos))
		{
			if (_pPos)
			{
				fp32 Dist = (_p0-Pos).Length();
				if (Dist < Nearest)
				{
					Nearest = Dist;
					_iFace = iFace;
					*_pPos = Pos;
				}
			}
			else
				return false;
		}
	}

	if (_pPos)
		return (_iFace < 0);
	else
		return true;
}

#ifndef PLATFORM_CONSOLE
bool CSplineBrush::Ogr_TraceLine(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int& _iFace, CVec3Dfp32* _pPos)
{
	MAUTOSTRIP(CSplineBrush_Ogr_TraceLine, false);
	// True == Trace was ok - no hit.
	// False == Hit, _iFace is set, intersectionpoint is returned in _Pos.

	CBox3Dfp32 linebox;
	for(int i = 0; i < 3; i++)
	{
		linebox.m_Min.k[i] = Min(_p0.k[i], _p1.k[i]);
		linebox.m_Max.k[i] = Max(_p0.k[i], _p1.k[i]);
		if (linebox.m_Max.k[i] < m_BoundBox.m_Min.k[i]) return true;
		if (linebox.m_Min.k[i] > m_BoundBox.m_Max.k[i]) return true;
	}

	fp32 Nearest = _FP32_MAX;
	_iFace = -1;

	for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
	{
		if(m_lFaces[iFace].m_pSurface && m_lFaces[iFace].m_pSurface->m_Flags & XW_SURFFLAGS_OGRINVISIBLE)
			continue;

		for(int i = 0; i < 3; i++)
		{
			if (linebox.m_Max.k[i] < m_lFaces[iFace].m_BoundBox.m_Min.k[i]) continue;
			if (linebox.m_Min.k[i] > m_lFaces[iFace].m_BoundBox.m_Max.k[i]) continue;
		}

		CVec3Dfp32 Pos;
		if (!Face_TraceLine(iFace, _p0, _p1, Pos, false))
		{
			if (_pPos)
			{
				fp32 Dist = (_p0-Pos).Length();
				if (Dist < Nearest)
				{
					Nearest = Dist;
					_iFace = iFace;
					*_pPos = Pos;
				}
			}
			else
				return false;
		}
	}

	if (_pPos)
		return (_iFace < 0);
	else
		return true;
}
#endif

void CSplineBrush::CleanUp()
{
	MAUTOSTRIP(CSplineBrush_CleanUp, MAUTOSTRIP_VOID);
	TArray<int> lEdgeUsed;
	lEdgeUsed.SetLen(m_lEdges.Len());
	FillChar(lEdgeUsed.GetBasePtr(), lEdgeUsed.ListSize(), 0);

	TArray<uint32> liEdges;
	liEdges.SetGrow(m_liEdges.Len());

	for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
	{
		CSplineBrush_Face* pF = &m_lFaces[iFace];

//		int iiEdgesNew = liEdges.Len();
		for(int iiE = 0; iiE < pF->m_nEdges; iiE++)
		{
			int iE = m_liEdges[pF->m_iiEdges + iiE];
			liEdges.Add(iE);
			lEdgeUsed[iE & 0x7fff] = 1;
		}

//		pF->m_iiEdges = iiEdgesNew;
	}


//	m_liEdges = liEdges;

	TArray<int> lVertUsed;
	lVertUsed.SetLen(m_lVertices.Len());
	FillChar(lVertUsed.GetBasePtr(), lVertUsed.ListSize(), 0);
	for(int iE = m_lEdges.Len()-1; iE >= 0; iE--)
		if (!lEdgeUsed[iE])
		{
			for(int i = 0; i < m_liEdges.Len(); i++)
			{
				if (m_liEdges[i] == iE)
					Error("CleanUp", "Internal error.");
				if ((m_liEdges[i] & 0x7fff) > iE) m_liEdges[i]--;
			}
			m_lEdges.Del(iE);
		}
		else
		{
			lVertUsed[m_lEdges[iE].m_iV[0]] = 1;
			lVertUsed[m_lEdges[iE].m_iV[1]] = 1;
		}

	for(int v = lVertUsed.Len()-1; v >= 0; v--)
		if (!lVertUsed[v])
		{
			for(int e = 0; e < m_lEdges.Len(); e++)
			{
				if (m_lEdges[e].m_iV[0] >= v) m_lEdges[e].m_iV[0]--;
				if (m_lEdges[e].m_iV[1] >= v) m_lEdges[e].m_iV[1]--;
			}
			m_lVertices.Del(v);
		}
}

bool CSplineBrush::Phys_IntersectLine(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CSplineBrush_Phys_IntersectLine, false);
	MSCOPESHORT(CSplineBrush::Phys_IntersectLine);
	CBox3Dfp32 linebox;
	for(int i = 0; i < 3; i++)
	{
		linebox.m_Min.k[i] = Min(_p0.k[i], _p1.k[i]);
		linebox.m_Max.k[i] = Max(_p0.k[i], _p1.k[i]);
		if (linebox.m_Max.k[i] < m_BoundBox.m_Min.k[i]) return false;
		if (linebox.m_Min.k[i] > m_BoundBox.m_Max.k[i]) return false;
	}

	bool bImpact = false;
	for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
	{
		const CSplineBrush_Face& Face = m_lFaces[iFace];
		if (!Face.m_pSurface)
			continue;
		if (!(Face.m_pSurface->GetBaseFrame()->m_Medium.m_MediumFlags & _MediumFlags))
			continue;
		
		if (linebox.m_Max.k[0] < Face.m_BoundBox.m_Min.k[0]) continue;
		if (linebox.m_Min.k[0] > Face.m_BoundBox.m_Max.k[0]) continue;
		if (linebox.m_Max.k[1] < Face.m_BoundBox.m_Min.k[1]) continue;
		if (linebox.m_Min.k[1] > Face.m_BoundBox.m_Max.k[1]) continue;
		if (linebox.m_Max.k[2] < Face.m_BoundBox.m_Min.k[2]) continue;
		if (linebox.m_Min.k[2] > Face.m_BoundBox.m_Max.k[2]) continue;

		if (Phys_FaceIntersectLine(iFace, _p0, _p1, _pCollisionInfo))
		{
			if (!_pCollisionInfo) return true;
			bImpact = true;
		}
	}

	return bImpact;
}

/*
static bool IntersectTriangleOld(
				const CVec3Dfp32& _p0,
				const CVec3Dfp32& _p1,
				const CVec3Dfp32& _v0,
				const CVec3Dfp32& _v1,
				const CVec3Dfp32& _v2,
				CVec3Dfp32& _Pos,
				bool _bIntersectBackSide = true)
{
	MAUTOSTRIP(IntersectTriangle, false);
	CPlane3Dfp32 Pi4(_v0, _v1, _v2);
	if (Pi4.Distance(_p0) > 0.0f)
	{
		if (Pi4.Distance(_p1) > 0.0f) return false;

		CPlane3Dfp32 Pi1(_p0, _v1, _v0);
		if (Pi1.Distance(_p1) < 0.0f) return false;
		CPlane3Dfp32 Pi2(_p0, _v2, _v1);
		if (Pi2.Distance(_p1) < 0.0f) return false;
		CPlane3Dfp32 Pi3(_p0, _v0, _v2);
		if (Pi3.Distance(_p1) < 0.0f) return false;

		Pi4.GetIntersectionPoint(_p0, _p1, _Pos);
	}
	else
	{
		if (!_bIntersectBackSide) return false;

		if (Pi4.Distance(_p1) < 0.0f) return false;
	
		CPlane3Dfp32 Pi1(_p0, _v0, _v1);
		if (Pi1.Distance(_p1) < 0.0f) return false;
		CPlane3Dfp32 Pi2(_p0, _v1, _v2);
		if (Pi2.Distance(_p1) < 0.0f) return false;
		CPlane3Dfp32 Pi3(_p0, _v2, _v0);
		if (Pi3.Distance(_p1) < 0.0f) return false;

		Pi4.GetIntersectionPoint(_p0, _p1, _Pos);
	}

//if (_Pos.k[1] < 64 && _Pos.k[1] > -64 && _Pos.k[2] < 80)
//	LogFile("Hit " + _Pos.GetString() + ", Tri " + _v0.GetString()+ _v1.GetString()+ _v2.GetString());
	return true;
}
//*/

//*
// Backman: Improved performance compared to commented out above
static inline bool IntersectTriangle(
			const CVec3Dfp32& _p0,
			const CVec3Dfp32& _p1,
			const CVec3Dfp32& _v0,
			const CVec3Dfp32& _v1,
			const CVec3Dfp32& _v2,
			CVec3Dfp32& _Pos,
			bool _bIntersectBackSide = true)
{
	MAUTOSTRIP(IntersectTriangle, false);

	CVec3Dfp32 dir = _p1 - _p0;
	fp32 u,v,t;
	CVec3Dfp32 edge01,edge02,tvec,pvec,qvec;
	fp32 det,inv_det;

	// Calculate edge vectors from vert0
	edge02 = _v2 - _v0;
	edge01 = _v1 - _v0;

	// Begin calc determinant
	pvec = dir / edge02;
	det = edge01 * pvec;

	if (_bIntersectBackSide == true)
	{	
		if ((det > -0.01f) && (det < 0.01f))
		{
			return(false);
		}

		// we go through this sign hoopla under the assumption that the function mostly
		// will return false and that we wish to avoid 'inv_det = 1.0f / det;'
		fp32 signDet = Sign(det);
		fp32 absDet = M_Fabs(det);
		tvec = _p0 - _v0;

		// Calc u and check its bounds
		// We don't normalize u,v as the divide cost us precious cycles
		// We need to get the sign of det to make sure the comparisons work
		u = (tvec * pvec) * signDet;
		if ((u < 0.0f)||(u > absDet))
		{
			return(false);
		}

		// Calc v param and check bounds
		qvec = tvec / edge01;
		v = (dir * qvec) * signDet;

		// Q: Why test against det instead of 1.0?
		// A: This way we may bail before doing the costly inv_det division
		if ((v < 0.0f)||((u) + (v) > absDet))
		{
			return(false);
		}
		
		// We now know that the ray hits the triangle, we scale the u,v,t params
		t = (edge02 * qvec) * signDet;
		if ((t < 0.0f)||(t > absDet))
		{
			return(false);
		}

		// We divide here hoping most cases return false
		// (If most cases return true we should divide as early as possible to help pipelining)
		inv_det = 1.0f / det;
		_Pos = _p0 + dir * t * inv_det;

		return(true);
	}
	else	// Ignore backfaced polys
	{
		// If determinant is near zero ray is parallell to triangle plane
		// Use fabs(det) if we don't want backface culling
		if (det < 0.01f)
		{
			return(false);
		}

		// Calculate parameterized distance from vert0 to ray origin
		tvec = _p0 - _v0;

		// Calc u and check its bounds
		u = tvec * pvec;
		if ((u < 0.0f)||(u > det))
		{
			return(false);
		}

		// Calc v param and check bounds
		qvec = tvec / edge01;
		v = dir * qvec;

		// Q: Why test against det instead of 1.0?
		// A: This way we may bail before doing the costly inv_det division
		if ((v < 0.0f)||((u) + (v) > det))
		{
			return(false);
		}
		
		// We now know that the ray hits the triangle, we scale the u,v,t params
		t = edge02 * qvec;
		inv_det = 1.0f / det;
		t *= inv_det;

		_Pos = _p0 + dir * t;

		return(true);
	}
}
//*/


// Time testing
/*
static bool IntersectTriangle(
	const CVec3Dfp32& _p0,
	const CVec3Dfp32& _p1,
	const CVec3Dfp32& _v0,
	const CVec3Dfp32& _v1,
	const CVec3Dfp32& _v2,
	CVec3Dfp32& _Pos,
	bool _bIntersectBackSide = true)
{
	bool oldResult,newResult;

	// *** Debug stuff
	int i;
	static int32 count = 0;
	static fp64 oldSeconds = 0;
	static fp64 newSeconds = 0;
	int64	oldTime;
	int64	newTime;


	T_Start(oldTime);
	for (i = 0; i < 5; i++)
	{
		oldResult = IntersectTriangleOld(_p0,_p1,_v0,_v1,_v2,_Pos,_bIntersectBackSide);
	}
	T_Stop(oldTime);
	oldSeconds += oldTime / GetCPUFrequency();
	
	T_Start(newTime);
	for (i = 0; i < 5; i++)
	{
		newResult = IntersectTriangleNew(_p0,_p1,_v0,_v1,_v2,_Pos,_bIntersectBackSide);
	}
	T_Stop(newTime);
	newSeconds += newTime / GetCPUFrequency();

	// ***

	count++;
	if (count >= 1000)
	{
		count = 0;
		bool debug = false;
	}
	return(newResult);
}
*/

#ifndef PLATFORM_CONSOLE
fp32 CSplineBrush::Ogr_IntersectRay(const CVec3Dfp32 &_Origin, const CVec3Dfp32 &_Ray)
{
	MAUTOSTRIP(CSplineBrush_Ogr_IntersectRay, 0.0f);
	int iFace;
	CVec3Dfp32 Pos;
	if(!Ogr_TraceLine(_Origin, _Origin + _Ray * 10000000, iFace, &Pos))
		return (_Origin - Pos).Length();
	else
		return -1;
}

int CSplineBrush::Ogr_IntersectFaces(const CVec3Dfp32 &_Origin, const CVec3Dfp32 &_Ray, float &_Dist)
{
	MAUTOSTRIP(CSplineBrush_Ogr_IntersectFaces, 0);
	int iFace;
	CVec3Dfp32 Pos;
	if(!Ogr_TraceLine(_Origin, _Origin + _Ray * 10000000, iFace, &Pos))
	{
		_Dist = (_Origin - Pos).Length();
		return iFace;
	}
	else
		return -1;
}
#endif

#ifdef NEVER
void CSplineBrush::Face_CalcVertexMap(int _iFace, int _nBumps, int _Width, int _Height)
{
	MAUTOSTRIP(CSplineBrush_Face_CalcVertexMap, MAUTOSTRIP_VOID);
	const CVec3Dfp64 BumpNormals[5] =
	{
		CVec3Dfp64(0,0,1),
		CVec3Dfp64(1,0,0),
		CVec3Dfp64(0,1,0),
		CVec3Dfp64(-1,0,0),
		CVec3Dfp64(0,-1,0)
	};

	// Build lightmap vertices & normals

	for(int iBump = 0; iBump < _nBumps; iBump++)
	{
		CSplineBrush_Face* pF = &m_lFaces[_iFace];

		spCSB_LightMap spL = DNew(CSB_LightMap) CSB_LightMap;
		if (!spL) MemError("Light_BuildSplineBrushFace");

		CVec3Dfp32 VMin = m_BoundBox.m_Min;
		CVec3Dfp32 VScale;
		for(int i = 0; i < 3; i++)
		{
			VScale.k[i] = CSB_COMPACTVERTEXSIZE / (m_BoundBox.m_Max.k[i] - VMin.k[i]);
			spL->m_Scale.k[i] = 1.0f/VScale.k[i];
		}

		int w = _Width;
		int h = _Height;
		spL->m_lVertices.SetLen(w*h);
		spL->m_Width = w;
		spL->m_Height = h;
		spL->m_Offset = VMin;

		for(int y = 0; y < h; y++)
			for(int x = 0; x < w; x++)
			{
				CSB_CompactVertex* pV = &spL->m_lVertices[y*w + x];

				fp32 s = fp32(x)/fp32(w-1);
				fp32 t = fp32(y)/fp32(h-1);
				CVec3Dfp32 V, dXYZdS, dXYZdT, n;
				EvalXYZ(_iFace, s, t, V);
				EvalXYZ_dXYZ(_iFace, s, t, dXYZdS, dXYZdT);
				dXYZdS.Normalize();
				dXYZdT.Normalize();
				dXYZdS.CrossProd(dXYZdT, n);
				n.Normalize();
				CVec3Dfp32 N = 
					(dXYZdS * -BumpNormals[iBump].k[0]) + 
					(dXYZdT * -BumpNormals[iBump].k[1]) +
					(n * BumpNormals[iBump].k[2]);
				N.Normalize();

//				CVec3Dfp32 vx = EvalXYZ(_iFace, fx+0.01f, fy);
//				CVec3Dfp32 vy = EvalXYZ(_iFace, fx, fy+0.01f);
//				CVec3Dfp32 dx,dy,n;
//				vx.Sub(V, dx);
//				vy.Sub(V, dy);
//				dx.Normalize();
//				dy.Normalize();
//				dx.CrossProd(dy, n);

				N *= CSB_COMPACTVERTEXSIZE;
				pV->m_N[0] = N.k[0];
				pV->m_N[1] = N.k[1];
				pV->m_N[2] = N.k[2];

				for(int i = 0; i < 3; i++)
					V.k[i] = (V.k[i] - VMin.k[i]) * VScale.k[i];
				pV->m_V[0] = V.k[0];
				pV->m_V[1] = V.k[1];
				pV->m_V[2] = V.k[2];

/*				CVec3Dfp32 V = EvalXYZ(_iFace, fp32(x)/fp32(w-1), fp32(y)/fp32(h-1));
				CVec3Dfp32 N = EvalNormal(_iFace, fp32(x)/fp32(w-1), fp32(y)/fp32(h-1));
				N *= CSB_COMPACTVERTEXSIZE;
				pV->m_N[0] = N.k[0];
				pV->m_N[1] = N.k[1];
				pV->m_N[2] = N.k[2];

				for(int i = 0; i < 3; i++)
					V.k[i] = (V.k[i] - VMin.k[i]) * VScale.k[i];
				pV->m_V[0] = V.k[0];
				pV->m_V[1] = V.k[1];
				pV->m_V[2] = V.k[2];*/
			}
		pF->m_iLMVertexMap[iBump] = m_lspFaceLightMaps.Add(spL);
	}
}
#endif

bool CSplineBrush::Face_IsVisible(int _iFace, const CVec3Dfp32& _VP)
{
	MAUTOSTRIP(CSplineBrush_Face_IsVisible, false);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];
	const CPlane3Dfp32* pP = &m_lCullPlanes[pF->m_iCullPlanes];
	for(int i = 0; i < pF->m_nCullPlanes; i++)
		if (pP[i].Distance(_VP) > 0.0f) return true;
	return false;
}

bool CSplineBrush::Face_IsFlatS(int _iFace)
{
	MAUTOSTRIP(CSplineBrush_Face_IsFlatS, false);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];
	int ie0 = m_liEdges[pF->m_iiEdges];
	int ie2 = m_liEdges[pF->m_iiEdges+2];
	CSplineBrush_Edge* pE0 = &m_lEdges[ie0 & 0x7fff];
	CSplineBrush_Edge* pE2 = &m_lEdges[ie2 & 0x7fff];
	if (M_Fabs(pE0->m_Mat[2][0]) < CSB_EPSILON &&
		M_Fabs(pE0->m_Mat[2][1]) < CSB_EPSILON &&
		M_Fabs(pE0->m_Mat[2][2]) < CSB_EPSILON &&
		M_Fabs(pE2->m_Mat[2][0]) < CSB_EPSILON &&
		M_Fabs(pE2->m_Mat[2][1]) < CSB_EPSILON &&
		M_Fabs(pE2->m_Mat[2][2]) < CSB_EPSILON)
		return true;
	return false;
}

bool CSplineBrush::Face_IsFlatT(int _iFace)
{
	MAUTOSTRIP(CSplineBrush_Face_IsFlatT, false);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];
	int ie1 = m_liEdges[pF->m_iiEdges+1];
	int ie3 = m_liEdges[pF->m_iiEdges+3];
	CSplineBrush_Edge* pE1 = &m_lEdges[ie1 & 0x7fff];
	CSplineBrush_Edge* pE3 = &m_lEdges[ie3 & 0x7fff];
	if (M_Fabs(pE1->m_Mat[2][0]) < CSB_EPSILON &&
		M_Fabs(pE1->m_Mat[2][1]) < CSB_EPSILON &&
		M_Fabs(pE1->m_Mat[2][2]) < CSB_EPSILON &&
		M_Fabs(pE3->m_Mat[2][0]) < CSB_EPSILON &&
		M_Fabs(pE3->m_Mat[2][1]) < CSB_EPSILON &&
		M_Fabs(pE3->m_Mat[2][2]) < CSB_EPSILON)
		return true;
	return false;
}

bool CSplineBrush::Face_IsPlanar(int _iFace, CPlane3Dfp32& _Plane)
{
	MAUTOSTRIP(CSplineBrush_Face_IsPlanar, false);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];
	int ie0 = m_liEdges[pF->m_iiEdges];
	int ie2 = m_liEdges[pF->m_iiEdges+2];
	int ie1 = m_liEdges[pF->m_iiEdges+1];
	int ie3 = m_liEdges[pF->m_iiEdges+3];
	int iv0 = (ie0 & 0x8000) ? m_lEdges[ie0 & 0x7fff].m_iV[1] : m_lEdges[ie0 & 0x7fff].m_iV[0];
	int iv1 = (ie1 & 0x8000) ? m_lEdges[ie1 & 0x7fff].m_iV[1] : m_lEdges[ie1 & 0x7fff].m_iV[0];
	int iv2 = (ie2 & 0x8000) ? m_lEdges[ie2 & 0x7fff].m_iV[1] : m_lEdges[ie2 & 0x7fff].m_iV[0];
	int iv3 = (ie3 & 0x8000) ? m_lEdges[ie3 & 0x7fff].m_iV[1] : m_lEdges[ie3 & 0x7fff].m_iV[0];

	CVec3Dfp32 v0,v1;
	m_lVertices[iv1].Sub(m_lVertices[iv0], v0);
	m_lVertices[iv2].Sub(m_lVertices[iv0], v1);
	_Plane.Create(m_lVertices[iv0], m_lVertices[iv1], m_lVertices[iv2]);
	if (M_Fabs(_Plane.Distance(m_lVertices[iv3])) > CSB_EPSILON) return false;

	CSplineBrush_Edge* pE0 = &m_lEdges[ie0 & 0x7fff];
	CSplineBrush_Edge* pE2 = &m_lEdges[ie2 & 0x7fff];
	CSplineBrush_Edge* pE1 = &m_lEdges[ie1 & 0x7fff];
	CSplineBrush_Edge* pE3 = &m_lEdges[ie3 & 0x7fff];

	if (M_Fabs(pE0->GetBendVector() * _Plane.n) > CSB_EPSILON) return false;
	if (M_Fabs(pE1->GetBendVector() * _Plane.n) > CSB_EPSILON) return false;
	if (M_Fabs(pE2->GetBendVector() * _Plane.n) > CSB_EPSILON) return false;
	if (M_Fabs(pE3->GetBendVector() * _Plane.n) > CSB_EPSILON) return false;
	return true;
}

int CSplineBrush::Face_GetFence(int _iFace, int _MaxVerts, CVec3Dfp32* _pDst)
{
	MAUTOSTRIP(CSplineBrush_Face_GetFence, 0);
	int nv = _MaxVerts / 4;
	fp32 fnv = nv;
	for(int v = 0; v < nv; v++)
	{
		_pDst[v + nv*0] = EvalXYZ(_iFace, fp32(v) / fnv, 0);
		_pDst[v + nv*1] = EvalXYZ(_iFace, 1.0f, fp32(v) / fnv);
		_pDst[v + nv*2] = EvalXYZ(_iFace, 1.0f - fp32(v) / fnv, 1.0f);
		_pDst[v + nv*3] = EvalXYZ(_iFace, 0, 1.0f - fp32(v) / fnv);
	}
	return nv*4;
}

bool CSplineBrush::Face_TraceLine(int _iFace, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CVec3Dfp32& _Pos, bool _bIntersectBackSide)
{
	MAUTOSTRIP(CSplineBrush_Face_TraceLine, false);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];

//	int nS = (Face_IsFlatS(_iFace)) ? 1 : CSB_TESS_PHYSICS;
//	int nT = (Face_IsFlatT(_iFace)) ? 1 : CSB_TESS_PHYSICS;
	int nS = 1 << pF->m_PhysTessShiftS;
	int nT = 1 << pF->m_PhysTessShiftT;
	fp32 sStep = 1.0f / fp32(nS);
	fp32 tStep = 1.0f / fp32(nT);

	CBox3Dfp32 linebox;
	for(int i = 0; i < 3; i++)
	{
		linebox.m_Min.k[i] = Min(_p0.k[i], _p1.k[i]);
		linebox.m_Max.k[i] = Max(_p0.k[i], _p1.k[i]);
	}

	uint32 ST[CSB_TESS_PHYSICS*CSB_TESS_PHYSICS];
//	int nST = EnumBoundNodes(_iFace, linebox, ST);
	int nST = EnumBoundNodesLine(_iFace, _p0, _p1, ST);

	for(int iST = 0; iST < nST; iST++)
	{
		fp32 fS = fp32(ST[iST] & 0x7fff) * sStep;
		fp32 fT = fp32(ST[iST] >> 16) * tStep;
		CVec3Dfp32 Verts[4];
		EvalXYZ(_iFace, fS, fT, Verts[0]);
		EvalXYZ(_iFace, fS + sStep, fT, Verts[1]);
		EvalXYZ(_iFace, fS, fT + tStep, Verts[2]);
		EvalXYZ(_iFace, fS + sStep, fT + tStep, Verts[3]);

		if (IntersectTriangle(_p0, _p1, Verts[0], Verts[1], Verts[2], _Pos, _bIntersectBackSide))
		{
			return false;
			if ((_p0 - _Pos).LengthSqr() > Sqr(8.0f)) return false;
		}
		if (IntersectTriangle(_p0, _p1, Verts[1], Verts[3], Verts[2], _Pos, _bIntersectBackSide))
		{
			return false;
			if ((_p0 - _Pos).LengthSqr() > Sqr(8.0f)) return false;
		}
	}
	return true;
}

bool CSplineBrush::Phys_FaceIntersectLine(int _iFace, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CSplineBrush_Phys_FaceIntersectLine, false);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];

//	int nS = (Face_IsFlatS(_iFace)) ? 1 : CSB_TESS_PHYSICS;
//	int nT = (Face_IsFlatT(_iFace)) ? 1 : CSB_TESS_PHYSICS;
	int nS = 1 << pF->m_PhysTessShiftS;
	int nT = 1 << pF->m_PhysTessShiftT;
	fp32 sStep = 1.0f / fp32(nS);
	fp32 tStep = 1.0f / fp32(nT);

	CBox3Dfp32 linebox;
	for(int i = 0; i < 3; i++)
	{
		linebox.m_Min.k[i] = Min(_p0.k[i], _p1.k[i]);
		linebox.m_Max.k[i] = Max(_p0.k[i], _p1.k[i]);
	}

	uint32 ST[CSB_TESS_PHYSICS*CSB_TESS_PHYSICS];
//	int nST = EnumBoundNodes(_iFace, linebox, ST);
	int nST = EnumBoundNodesLine(_iFace, _p0, _p1, ST);

	bool bImpact = false;
	for(int iST = 0; iST < nST; iST++)
	{
		fp32 fS = fp32(ST[iST] & 0x7fff) * sStep;
		fp32 fT = fp32(ST[iST] >> 16) * tStep;
		CVec3Dfp32 Verts[4];
		EvalXYZ(_iFace, fS, fT, Verts[0]);
		EvalXYZ(_iFace, fS + sStep, fT, Verts[1]);
		EvalXYZ(_iFace, fS, fT + tStep, Verts[2]);
		EvalXYZ(_iFace, fS + sStep, fT + tStep, Verts[3]);

		CVec3Dfp32 Pos;
		if (IntersectTriangle(_p0, _p1, Verts[0], Verts[1], Verts[2], Pos))
		{
			if (!_pCollisionInfo) return true;
			fp32 Time = _p0.Distance(Pos) / _p1.Distance(_p0);
			
			if (Time < -0.001f || Time > 1.001f)
				ConOut(CStrF("T = %f, _p0 %s, _p1 %s", Time, _p0.GetString().Str(), _p1.GetString().Str()));

			if (_pCollisionInfo->IsImprovement(Time))
			{
				_pCollisionInfo->m_LocalPos = Pos;
				_pCollisionInfo->m_Plane.Create(Verts[0], Verts[1], Verts[2]);
				_pCollisionInfo->m_Distance = _pCollisionInfo->m_Plane.Distance(_p1);
				_pCollisionInfo->m_Time = Time;
				_pCollisionInfo->m_bIsValid = true;
				_pCollisionInfo->m_bIsCollision = true;
				bImpact = true;
				if (_pCollisionInfo->IsComplete())
					return true;
			}
		}
		if (IntersectTriangle(_p0, _p1, Verts[1], Verts[3], Verts[2], Pos))
		{
			if (!_pCollisionInfo) return true;
			fp32 Time = _p0.Distance(Pos) / _p1.Distance(_p0);
			if (Time < -0.001f || Time > 1.001f)
				ConOut(CStrF("T = %f, _p0 %s, _p1 %s", Time, _p0.GetString().Str(), _p1.GetString().Str()));

			if (_pCollisionInfo->IsImprovement(Time))
			{
				_pCollisionInfo->m_LocalPos = Pos;
				_pCollisionInfo->m_Plane.Create(Verts[1], Verts[3], Verts[2]);
				_pCollisionInfo->m_Distance = _pCollisionInfo->m_Plane.Distance(_p1);
				_pCollisionInfo->m_Time = Time;
				_pCollisionInfo->m_bIsValid = true;
				_pCollisionInfo->m_bIsCollision = true;
				bImpact = true;
				if (_pCollisionInfo->IsComplete())
					return true;
			}
		}
	}
	return bImpact;
}

#ifdef NEVER
bool CSplineBrush::Face_TraceLine(int _iFace, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CVec3Dfp32& _Pos)
{
	MAUTOSTRIP(CSplineBrush_Face_TraceLine_2, false);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];

	int nS = (Face_IsFlatS(_iFace)) ? 1 : 8;
	int nT = (Face_IsFlatT(_iFace)) ? 1 : 8;
	nS++;
	nT++;

	CVec3Dfp32 Verts[100];
	Tesselate(_iFace, nS, nT, 1.0f / fp32(nS-1), 1.0f / fp32(nT-1), 100, Verts, NULL);

	for(int s = 0; s < nS-1; s++)
		for(int t = 0; t < nT-1; t++)
		{
			int iv = s*nT + t;
			if (IntersectTriangle(_p0, _p1, Verts[iv], Verts[iv+1], Verts[iv+nT], _Pos))
			{
				if ((_p0 - _Pos).LengthSqr() > Sqr(8.0f)) return false;
			}
			if (IntersectTriangle(_p0, _p1, Verts[iv+1], Verts[iv+1+nT], Verts[iv+nT], _Pos))
			{
				if ((_p0 - _Pos).LengthSqr() > Sqr(8.0f)) return false;
			}
		}
	return true;
}
#endif

void CSplineBrush::GetTesselationSpaceExt(int _iFace, int& _nV, int& _nPrim, int& _nS, int& _nT)
{
	MAUTOSTRIP(CSplineBrush_GetTesselationSpaceExt, MAUTOSTRIP_VOID);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];

	const uint32* piEdges = m_liEdges.GetBasePtr();
	const fp32* pEdgeTess = m_lEdgeTess.GetBasePtr();
//	const CSplineBrush_Edge* pEdges = m_lEdges.GetBasePtr();
	int ie0 = piEdges[pF->m_iiEdges];
	int ie1 = piEdges[pF->m_iiEdges+1];
	int ie2 = piEdges[pF->m_iiEdges+2];
	int ie3 = piEdges[pF->m_iiEdges+3];

	fp32 fnS1 = pEdgeTess[ie0 & 0x7fff];
	fp32 fnS2 = pEdgeTess[ie2 & 0x7fff];
	fp32 fnT1 = pEdgeTess[ie1 & 0x7fff];
	fp32 fnT2 = pEdgeTess[ie3 & 0x7fff];
	fp32 MaxS = Max(fnS1, fnS2);
	fp32 MaxT = Max(fnT1, fnT2);

	_nS = RoundToInt(MaxS - CSB_EPSILON - 0.5f) + 2;
	_nT = RoundToInt(MaxT - CSB_EPSILON - 0.5f) + 2;

	_nV = _nS*_nT;
	_nPrim = (_nS-1)*(_nT*2+2);
}

void CSplineBrush::GetTesselationSpace(int _iFace, int& _nV, int& _nPrim)
{
	MAUTOSTRIP(CSplineBrush_GetTesselationSpace, MAUTOSTRIP_VOID);
	int nS, nT;
	GetTesselationSpaceExt(_iFace, _nV, _nPrim, nS, nT);
}


#define CSB_EDGE_REVERSETESS 0x10000

CVec3Dfp32* CSplineBrush::Tesselate(int _iFace, int _MaxV, CVec3Dfp32* _pV, CVec2Dfp32* _pST, int& _nS, int& _nT, CXR_VBManager* _pVBM)
{
	MAUTOSTRIP(CSplineBrush_Tesselate, NULL);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];
	if (!_pST) Error("Tesselate", "pSTArray == NULL");

	const uint32* piEdges = m_liEdges.GetBasePtr();
	const fp32* pEdgeTess = m_lEdgeTess.GetBasePtr();
	const CSplineBrush_Edge* pEdges = m_lEdges.GetBasePtr();
	int ie0 = piEdges[pF->m_iiEdges];
	int ie1 = piEdges[pF->m_iiEdges+1];
	int ie2 = piEdges[pF->m_iiEdges+2];
	int ie3 = piEdges[pF->m_iiEdges+3];
	const CSplineBrush_Edge* pE0 = &pEdges[ie0 & 0x7fff];
	const CSplineBrush_Edge* pE1 = &pEdges[ie1 & 0x7fff];
	const CSplineBrush_Edge* pE2 = &pEdges[ie2 & 0x7fff];
	const CSplineBrush_Edge* pE3 = &pEdges[ie3 & 0x7fff];

	fp32 fnS1 = pEdgeTess[ie0 & 0x7fff];
	fp32 fnS2 = pEdgeTess[ie2 & 0x7fff];
	fp32 fnT1 = pEdgeTess[ie1 & 0x7fff];
	fp32 fnT2 = pEdgeTess[ie3 & 0x7fff];
	fp32 MaxS = Max(fnS1, fnS2);
	fp32 MaxT = Max(fnT1, fnT2);
	fp32 sStep1 = 1.0f/fnS1;
	fp32 sStep2 = 1.0f/fnS2;
	fp32 tStep1 = 1.0f/fnT1;
	fp32 tStep2 = 1.0f/fnT2;
	_nS = RoundToInt(MaxS - CSB_EPSILON - 0.5f) + 2;
	_nT = RoundToInt(MaxT - CSB_EPSILON - 0.5f) + 2;

	if(_nS <= 0 || _nT <= 0)
	{
		_nS = 0;
		_nT = 0;
		return NULL;
	}

	if (!_pV)
	{
		if (!_pVBM) Error("Tesselate", "No vertex array or VB-manager.");
		_pV = _pVBM->Alloc_V3(_nS*_nT);
		if (!_pV)
		{
			_nS = 0;
			_nT = 0;
			return NULL;
		}
	}
	else if (_nS*_nT > _MaxV)
	{
		ConOut(CStrF("(CSplineBrush::Tesselate) Vertex buffer too small %dx%d = %d > %d", _nS, _nT, _nS*_nT, _MaxV));
		_nS = 0;
		_nT = 0;
		return NULL;
	}


	int CntS = _nS;
	int SStep, TStep, s;
	fp32 fS1;
	fp32 fS2;
	bool bSReverse, bTReverse;
	int tStart;
	fp32 fT1Start, fT2Start;

	if (ie0 & CSB_EDGE_REVERSETESS)
	{
		SStep = -1;
		s = _nS-1;
		fS1 = 1.0f;
		fS2 = 1.0f;
		sStep1 = -sStep1;
		sStep2 = -sStep2;
		bSReverse = true;
	}
	else
	{
		SStep = 1;
		s = 0;
		fS1 = 0.0f;
		fS2 = 0.0f;
		bSReverse = false;
	}

	if (ie1 & CSB_EDGE_REVERSETESS)
	{
		tStep1 = -tStep1;
		tStep2 = -tStep2;
		TStep = -1;
		tStart = _nT-1;
		fT1Start = 1.0f;
		fT2Start = 1.0f;
		bTReverse = true;
	}
	else
	{
		TStep = 1;
		tStart = 0;
		fT1Start = 0.0f;
		fT2Start = 0.0f;
		bTReverse = false;
	}

	const CVec3Dfp32* pV = m_lVertices.GetBasePtr();
	CntS -= 2;

	// -------------------------------------------------------------------
	//  First S-Segment
	// -------------------------------------------------------------------
	{
		fp32 fT1 = fT1Start;
		fp32 fT2 = fT2Start;
		int t = tStart;
		int CntT = _nT-2;
		if (!bSReverse)
		{
			const CVec3Dfp32& v0 = pV[pE3->m_iV[1 - ((ie3 & 0x8000) >> 15)]];
			const CVec3Dfp32& v1 = pV[pE3->m_iV[(ie3 & 0x8000) >> 15]];
			{
				int iV = s*_nT + t;
				_pST[iV].k[0] = 0.0f;	_pST[iV].k[1] = fT2;
				v0.Lerp(v1, fT2, _pV[iV]);
				fT2 = Clamp01(fT2 + tStep2);
				t += TStep;
			}
			while(CntT--)
			{
				int iV = s*_nT + t;
				fp32 v = fT2;
				v0.Lerp(v1, v, _pV[iV]);
				pE3->AddEdge(_pV[iV], !(ie3 & 0x8000) ? (1.0f - v) : v);
				_pV[iV] *= 0.5f;
				_pST[iV].k[0] = 0.0f;	_pST[iV].k[1] = v;
				fT2 = Clamp01(fT2 + tStep2);
				t += TStep;
			}
			{
				int iV = s*_nT + t;
				_pST[iV].k[0] = 0.0f;	_pST[iV].k[1] = fT2;
				v0.Lerp(v1, fT2, _pV[iV]);
			}
		}
		else
		{
			const CVec3Dfp32& v0 = pV[pE1->m_iV[(ie1 & 0x8000) >> 15]];
			const CVec3Dfp32& v1 = pV[pE1->m_iV[1 - ((ie1 & 0x8000) >> 15)]];
			{
				int iV = s*_nT + t;
				_pST[iV].k[0] = 1.0f;	_pST[iV].k[1] = fT1;
				v0.Lerp(v1, fT1, _pV[iV]);
				fT1 = Clamp01(fT1 + tStep1);
				t += TStep;
			}
			while(CntT--)
			{
				int iV = s*_nT + t;
				fp32 v = fT1;
				v0.Lerp(v1, v, _pV[iV]);
				pE1->AddEdge(_pV[iV], (ie1 & 0x8000) ? (1.0f - v) : v);
				_pV[iV] *= 0.5f;
				_pST[iV].k[0] = 1.0f;	_pST[iV].k[1] = v;
				fT1 = Clamp01(fT1 + tStep1);
				t += TStep;
			}
			{
				int iV = s*_nT + t;
				_pST[iV].k[0] = 1.0f;	_pST[iV].k[1] = fT1;
				v0.Lerp(v1, fT1, _pV[iV]);
			}
		}

		fS1 = Clamp01(fS1 + sStep1);
		fS2 = Clamp01(fS2 + sStep2);
		s += SStep;
	}

	// -------------------------------------------------------------------
	//  Middle S-Segments
	// -------------------------------------------------------------------
	while(CntS--)
	{
		fp32 fT1 = fT1Start;
		fp32 fT2 = fT2Start;
		int t = tStart;

		{
			int iV = s*_nT + t;
			fp32 u, v;
			if (!bTReverse)
			{
				u = fS1;
				v = 0.0f;
				pV[pE0->m_iV[(ie0 & 0x8000) >> 15]].Lerp(pV[pE0->m_iV[1 - ((ie0 & 0x8000) >> 15)]], u, _pV[iV]);
				pE0->AddEdge(_pV[iV], (ie0 & 0x8000) ? (1.0f - u) : u);
			}
			else
			{
				u = fS2;
				v = 1.0f;
				pV[pE2->m_iV[1 - ((ie2 & 0x8000) >> 15)]].Lerp(pV[pE2->m_iV[(ie2 & 0x8000) >> 15]], u, _pV[iV]);
				pE2->AddEdge(_pV[iV], !(ie2 & 0x8000) ? (1.0f - u) : u);
			}
			_pV[iV] *= 0.5f;
			_pST[iV].k[0] = u;
			_pST[iV].k[1] = v;
			fT1 = Clamp01(fT1 + tStep1);
			fT2 = Clamp01(fT2 + tStep2);
			t += TStep;
		}

		int CntT = _nT-2;
		while(CntT--)
		{
			int iV = s*_nT + t;
			fp32 ds = fS2-fS1;
			fp32 dt = fT1-fT2;
			fp32 u = (fS1 + fT2*ds) / (1.0f - ds*dt);
			fp32 v = fT2 + u*dt;
			_pV[iV] = 0;
			pE0->AddEdge(_pV[iV], (ie0 & 0x8000) ? (1.0f - u) : u, 1.0f-v);
			pE2->AddEdge(_pV[iV], !(ie2 & 0x8000) ? (1.0f - u) : u, v);
			pE1->AddEdge(_pV[iV], (ie1 & 0x8000) ? (1.0f - v) : v, u);
			pE3->AddEdge(_pV[iV], !(ie3 & 0x8000) ? (1.0f - v) : v, 1.0f-u);
			_pV[iV] *= 0.5f;
			_pST[iV].k[0] = u;
			_pST[iV].k[1] = v;
			fT1 = Clamp01(fT1 + tStep1);
			fT2 = Clamp01(fT2 + tStep2);
			t += TStep;
		}

		{
			int iV = s*_nT + t;
			fp32 u, v;
			if (!bTReverse)
			{
				u = fS2;
				v = 1.0f;
				pV[pE2->m_iV[1 - ((ie2 & 0x8000) >> 15)]].Lerp(pV[pE2->m_iV[(ie2 & 0x8000) >> 15]], u, _pV[iV]);
				pE2->AddEdge(_pV[iV], !(ie2 & 0x8000) ? (1.0f - u) : u);
			}
			else
			{
				u = fS1;
				v = 0.0f;
				pV[pE0->m_iV[(ie0 & 0x8000) >> 15]].Lerp(pV[pE0->m_iV[1 - ((ie0 & 0x8000) >> 15)]], u, _pV[iV]);
				pE0->AddEdge(_pV[iV], (ie0 & 0x8000) ? (1.0f - u) : u);
			}
			_pV[iV] *= 0.5f;
			_pST[iV].k[0] = u;
			_pST[iV].k[1] = v;
		}

		fS1 = Clamp01(fS1 + sStep1);
		fS2 = Clamp01(fS2 + sStep2);
		s += SStep;
	}

	// -------------------------------------------------------------------
	//  Last S-Segment
	// -------------------------------------------------------------------
	{
		fp32 fT1 = fT1Start;
		fp32 fT2 = fT2Start;
		int t = tStart;
		int CntT = _nT-2;
		if (bSReverse)
		{
			const CVec3Dfp32& v0 = pV[pE3->m_iV[1 - ((ie3 & 0x8000) >> 15)]];
			const CVec3Dfp32& v1 = pV[pE3->m_iV[(ie3 & 0x8000) >> 15]];
			{
				int iV = s*_nT + t;
				_pST[iV].k[0] = 0.0f;	_pST[iV].k[1] = fT2;
				v0.Lerp(v1, fT2, _pV[iV]);
				fT2 = Clamp01(fT2 + tStep2);
				t += TStep;
			}
			while(CntT--)
			{
				int iV = s*_nT + t;
				fp32 v = fT2;
				v0.Lerp(v1, v, _pV[iV]);
				pE3->AddEdge(_pV[iV], !(ie3 & 0x8000) ? (1.0f - v) : v);
				_pV[iV] *= 0.5f;
				_pST[iV].k[0] = 0.0f;	_pST[iV].k[1] = v;
				fT2 = Clamp01(fT2 + tStep2);
				t += TStep;
			}
			{
				int iV = s*_nT + t;
				_pST[iV].k[0] = 0.0f;	_pST[iV].k[1] = fT2;
				v0.Lerp(v1, fT2, _pV[iV]);
			}
		}
		else
		{
			const CVec3Dfp32& v0 = pV[pE1->m_iV[(ie1 & 0x8000) >> 15]];
			const CVec3Dfp32& v1 = pV[pE1->m_iV[1 - ((ie1 & 0x8000) >> 15)]];
			{
				int iV = s*_nT + t;
				_pST[iV].k[0] = 1.0f;	_pST[iV].k[1] = fT1;
				v0.Lerp(v1, fT1, _pV[iV]);
				fT1 = Clamp01(fT1 + tStep1);
				t += TStep;
			}
			while(CntT--)
			{
				int iV = s*_nT + t;
				fp32 v = fT1;
				v0.Lerp(v1, v, _pV[iV]);
				pE1->AddEdge(_pV[iV], (ie1 & 0x8000) ? (1.0f - v) : v);
				_pV[iV] *= 0.5f;
				_pST[iV].k[0] = 1.0f;	_pST[iV].k[1] = v;
				fT1 = Clamp01(fT1 + tStep1);
				t += TStep;
			}
			{
				int iV = s*_nT + t;
				_pST[iV].k[0] = 1.0f;	_pST[iV].k[1] = fT1;
				v0.Lerp(v1, fT1, _pV[iV]);
			}
		}
	}

	return _pV;
}

void CSplineBrush::Tesselate(int _iFace, fp32 _nS, fp32 _nT, fp32 _sStep, fp32 _tStep, int _MaxV, CVec3Dfp32* _pV, CVec2Dfp32* _pST)
{
	MAUTOSTRIP(CSplineBrush_Tesselate_2, MAUTOSTRIP_VOID);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];

	int ie0 = m_liEdges[pF->m_iiEdges];
	int ie1 = m_liEdges[pF->m_iiEdges+1];
	int ie2 = m_liEdges[pF->m_iiEdges+2];
	int ie3 = m_liEdges[pF->m_iiEdges+3];
	CSplineBrush_Edge* pE0 = &m_lEdges[ie0 & 0x7fff];
	CSplineBrush_Edge* pE1 = &m_lEdges[ie1 & 0x7fff];
	CSplineBrush_Edge* pE2 = &m_lEdges[ie2 & 0x7fff];
	CSplineBrush_Edge* pE3 = &m_lEdges[ie3 & 0x7fff];
	fp32 fS = 0.0f;
	for(int s = 0; s < _nS; s++)
	{
		fp32 fT = 0.0f;
		for(int t = 0; t < _nT; t++)
		{
			int iV = int(s*_nT + t);
			_pV[iV] = 0;
			pE0->AddEdge(_pV[iV], (ie0 & 0x8000) ? (1.0f - fS) : fS, 1.0f - fT);
			pE2->AddEdge(_pV[iV], !(ie2 & 0x8000) ? (1.0f - fS) : fS, fT);
			pE1->AddEdge(_pV[iV], (ie1 & 0x8000) ? (1.0f - fT) : fT, fS); 
			pE3->AddEdge(_pV[iV], !(ie3 & 0x8000) ? (1.0f - fT) : fT, 1.0f - fS);
			_pV[iV] *= 0.5f;
//			AddDisplacement(fS, fT, _pV[iV]);

			if (_pST)
			{
				_pST[iV].k[0] = fS;
				_pST[iV].k[1] = fT;
			}

			fT += _tStep;
			if (fT > 1.0f) fT = 1.0f;
		}
		fS += _sStep;
		if (fS > 1.0f) fS = 1.0f;
	}
}

void CSplineBrush::Tesselate(fp32 _sSize, fp32 _tSize, int _iFace, CVec3Dfp32* _pV, CVec2Dfp32* _pST, int _MaxV, int& _nS, int& _nT, const CVec2Dfp32& _UVScale)
{
	MAUTOSTRIP(CSplineBrush_Tesselate_3, MAUTOSTRIP_VOID);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];
	if (pF->m_nEdges != 4) return;

	int ie0 = m_liEdges[pF->m_iiEdges];
	int ie1 = m_liEdges[pF->m_iiEdges+1];
	int ie2 = m_liEdges[pF->m_iiEdges+2];
	int ie3 = m_liEdges[pF->m_iiEdges+3];
	CSplineBrush_Edge* pE0 = &m_lEdges[ie0 & 0x7fff];
	CSplineBrush_Edge* pE1 = &m_lEdges[ie1 & 0x7fff];
	CSplineBrush_Edge* pE2 = &m_lEdges[ie2 & 0x7fff];
	CSplineBrush_Edge* pE3 = &m_lEdges[ie3 & 0x7fff];

	fp32 SLen = Max(m_lEdges[ie0 & 0x7fff].m_Len, m_lEdges[ie2 & 0x7fff].m_Len);
	fp32 TLen = Max(m_lEdges[ie1 & 0x7fff].m_Len, m_lEdges[ie3 & 0x7fff].m_Len);
	fp32 fnS = SLen / _sSize;
	fp32 fnT = TLen / _tSize;
	_nS = int(M_Floor(fnS - CSB_EPSILON)) + 1;
	_nT = int(M_Floor(fnT - CSB_EPSILON)) + 1;

	if (M_Fabs(pE0->m_Mat[2][0]) < CSB_EPSILON &&
		M_Fabs(pE0->m_Mat[2][1]) < CSB_EPSILON &&
		M_Fabs(pE0->m_Mat[2][2]) < CSB_EPSILON &&
		M_Fabs(pE2->m_Mat[2][0]) < CSB_EPSILON &&
		M_Fabs(pE2->m_Mat[2][1]) < CSB_EPSILON &&
		M_Fabs(pE2->m_Mat[2][2]) < CSB_EPSILON)
	{
		_nS = 1;
		_sSize = 1.0f;
		SLen = 1.0f;
	}

	if (M_Fabs(pE1->m_Mat[2][0]) < CSB_EPSILON &&
		M_Fabs(pE1->m_Mat[2][1]) < CSB_EPSILON &&
		M_Fabs(pE1->m_Mat[2][2]) < CSB_EPSILON &&
		M_Fabs(pE3->m_Mat[2][0]) < CSB_EPSILON &&
		M_Fabs(pE3->m_Mat[2][1]) < CSB_EPSILON &&
		M_Fabs(pE3->m_Mat[2][2]) < CSB_EPSILON)
	{
		_nT = 1;
		_tSize = 1.0f;
		TLen = 1.0f;
	}

	_sSize *= 1.0f/SLen;
	_tSize *= 1.0f/TLen;

	_nS++;
	_nT++;
	if (_nS*_nT*2 > _MaxV)
	{
		_nS = int(M_Sqrt(_MaxV / 2));
		_nT = _MaxV / 2 / _nS;
		_sSize = 1.0f/(_nS-1);
		_tSize = 1.0f/(_nT-1);
	}


	if (_nS*_nT*2 > _MaxV)
	{
		_nS = 0;
		_nT = 0;
		return;
	}

//LogFile(CStrF("%f, %f, %f, %f", _tSize, _uSize, TLen, ULen));
//LogFile(CStrF("%f, %f, %f, %f", _tSize, _uSize, TLen, ULen));
	
	fp32 fS = 0.0f;

/*	fp32 UScale = pF->m_UVScale.k[0] * _UVScale.k[0];
	fp32 VScale = pF->m_UVScale.k[1] * _UVScale.k[1];
	fp32 UOffset = pF->m_UVOffset.k[0] * _UVScale.k[0];
	fp32 VOffset = pF->m_UVOffset.k[1] * _UVScale.k[1];*/

	for(int s = 0; s < _nS; s++)
	{
		fp32 fT = 0.0f;
		for(int t = 0; t < _nT; t++)
		{
			int iV = s*_nT + t;
			_pV[iV] = 0;
			m_lEdges[ie0 & 0x7fff].AddEdge(_pV[iV], (ie0 & 0x8000) ? (1.0f - fS) : fS, 1.0f - fT);
			m_lEdges[ie2 & 0x7fff].AddEdge(_pV[iV], !(ie2 & 0x8000) ? (1.0f - fS) : fS, fT);
			m_lEdges[ie1 & 0x7fff].AddEdge(_pV[iV], (ie1 & 0x8000) ? (1.0f - fT) : fT, fS); 
			m_lEdges[ie3 & 0x7fff].AddEdge(_pV[iV], !(ie3 & 0x8000) ? (1.0f - fT) : fT, 1.0f - fS);
			_pV[iV] *= 0.5f;
//			AddDisplacement(fS, fT, _pV[iV]);

			_pST[iV].k[0] = fS;
			_pST[iV].k[1] = fT;
			fT += _tSize;
			if (fT > 1.0f) fT = 1.0f;
		}
		fS += _sSize;
		if (fS > 1.0f) fS = 1.0f;
	}
}

#define MINTESS_K	0.3f

fp32 CSplineBrush::InitMinTessDistance()
{
	MAUTOSTRIP(CSplineBrush_InitMinTessDistance, 0.0f);
	MSCOPESHORT(CSplineBrush::InitMinTessDistance);
	m_MinTessDistance = 0.0f;
	for(int iE = 0; iE < m_lEdges.Len(); iE++)
	{
		const CSplineBrush_Edge* pE = &m_lEdges[iE];
		fp32 s = pE->m_Len * 1.5f;
		fp32 d = s / MINTESS_K;
		if (d > m_MinTessDistance) m_MinTessDistance = d;
	}
	m_MinTessDistance += 32.0f;
	return m_MinTessDistance;
}

fp32 CSplineBrush::GetMinTesselation(const CSplineBrush_Edge* _pE)
{
	MAUTOSTRIP(CSplineBrush_GetMinTesselation, 0.0f);
	if (_pE->m_Tess > 0) return _pE->m_Tess;

//	fp32 Size = m_lVertices[_pE->m_iV[0]].Distance(m_lVertices[_pE->m_iV[1]]);
	fp32 Size = _pE->m_Len;
	Size *= 1.5f;
	const fp32 MaxTess = Max(5.0f, 12.0f - Size / 128.0f);
	const fp32 MinTess = 90.0f;
//	fp32 Dist = Size * 10.0f;
//	fp32 ProjSize = Size  / Max(Size, Dist);
	fp32 ProjSize = MINTESS_K;

	fp32 nSegMin = _pE->m_dVdT / MinTess;
	fp32 nSegMax = _pE->m_dVdT / MaxTess;
//nSegMin = 16;
//nSegMax = 16;
	fp32 ds = 1.0f - ProjSize;
	ds = M_Sqrt(ds);
	fp32 nSeg = nSegMax + (nSegMin-nSegMax)*ds;
	return Min(Max(1.0f, nSeg), 8.0f);
//	return 4;
}

fp32 CSplineBrush::GetMaxTesselation(const CSplineBrush_Edge* _pE)
{
	MAUTOSTRIP(CSplineBrush_GetMaxTesselation, 0.0f);
	if (_pE->m_Tess > 0) return _pE->m_Tess;

	fp32 Size = _pE->m_Len;
	Size *= 1.5f;
	const fp32 MaxTess = Max(5.0f, 12.0f - Size / 128.0f);

	fp32 nSeg = _pE->m_dVdT / MaxTess;
	return ClampRange(nSeg - 1.0f, 15.0f) + 1.0f;
}

fp32 CSplineBrush::GetTesselation(const CSplineBrush_Edge* _pE, const CVec3Dfp32& _ViewPos)
{
	MAUTOSTRIP(CSplineBrush_GetTesselation, 0.0f);
	if (_pE->m_Tess > 0) return _pE->m_Tess;

	fp32 Size = _pE->m_Len;
	const fp32 MaxTess = Max(5.0f, 12.0f - Size / 128.0f);
	const fp32 MinTess = 90.0f;

/*	CVec3Dfp32 vControl(_pE->m_Mat[2][0], _pE->m_Mat[2][1], _pE->m_Mat[2][2]);
	CVec3Dfp32 vEdge;
	m_lVertices[_pE->m_iV[0]].Sub(m_lVertices[_pE->m_iV[1]], vEdge);
	CVec3Dfp32 vCross;
	vEdge.CrossProd(vControl, vCross);
	vCross.Normalize();
*/
	CVec3Dfp32 v0,v1;
	fp32 Dist = v0.Point2LineDistance(_ViewPos, m_lVertices[_pE->m_iV[0]], m_lVertices[_pE->m_iV[1]]);
	Dist -= 32.0f;
	Size *= 1.5f;
	fp32 ProjSize = Size / Max(Size, Dist);
/*	m_lVertices[_pE->m_iV[0]].Sub(_ViewPos, v0);
	m_lVertices[_pE->m_iV[1]].Sub(_ViewPos, v1);
	v0.Normalize();
	v1.Normalize();
	fp32 cosv = v0*v1;
	fp32 cosv2 = (M_Fabs(vCross*v0) + M_Fabs(vCross*v1))*0.5f;
	if (cosv < 0.0) cosv = -cosv;
	if (cosv2 < 0.0) cosv2 = -cosv2;*/
	fp32 ds = 1.0f - ProjSize;
	fp32 dsc = ds;
//	cosv2 = cosv2*ds + 1.0f-ds;
//	fp32 dsc = ds*cosv2 + 1.0f-cosv2;

//	fp32 nSeg = _pE->m_dVdT / (MaxTess + (MinTess - MaxTess)*dsc);

	fp32 nSegMin = _pE->m_dVdT / MinTess;
	fp32 nSegMax = _pE->m_dVdT / MaxTess;
	fp32 nSeg = nSegMax + (nSegMin-nSegMax)*dsc;

	return ClampRange(nSeg - 1.0f, 15.0f) + 1.0f;

//	fp32 MinSegs = 1.0f;
//	return Min(Max(1.0f, nSeg), 16.0f);
}

fp32 CSplineBrush::GetTesselation(const CSplineBrush_Edge* _pE, fp32 _Level)
{
	MAUTOSTRIP(CSplineBrush_GetTesselation_2, 0.0f);
	if (_pE->m_Tess > 0) return _pE->m_Tess;

	fp32 MinTess = GetMinTesselation(_pE);
	fp32 MaxTess = GetMaxTesselation(_pE);
	fp32 Tess = MinTess + (MaxTess - MinTess) * _Level;
	Tess = M_Ceil(Tess);
	return Tess;
}

void CSplineBrush::SetAllFacesVisible()
{
	MAUTOSTRIP(CSplineBrush_SetAllFacesVisible, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lFaces.Len(); i++)
		m_lFaces[i].m_Flags |= XW_FACE_VISIBLE;
}

void CSplineBrush::InitMinTesselation()
{
	MAUTOSTRIP(CSplineBrush_InitMinTesselation, MAUTOSTRIP_VOID);
	if (m_lEdgeMinTess.Len() != m_lEdges.Len())
	{
		m_lEdgeMinTess.SetLen(m_lEdges.Len());
		for(int iE = 0; iE < m_lEdges.Len(); iE++)
			m_lEdgeMinTess[iE] = GetMinTesselation(&m_lEdges[iE]);
	}

	m_lEdgeTess.SetLen(m_lEdges.Len());
	for(int iE = 0; iE < m_lEdges.Len(); iE++)
		m_lEdgeTess[iE] = GetMinTesselation(&m_lEdges[iE]);
}

bool CSplineBrush::InitTesselation(const CVec3Dfp32& _ViewPos)
{
	MAUTOSTRIP(CSplineBrush_InitTesselation, false);
	// returns true if compiled faces could be used.
	if (_ViewPos.Distance(m_BoundPos) - m_BoundRadius > m_MinTessDistance) return true;

	// Initialize min-tesselation if needed.
	if (m_lEdgeMinTess.Len() != m_lEdges.Len())
	{
		m_lEdgeMinTess.SetLen(m_lEdges.Len());
		for(int iE = 0; iE < m_lEdges.Len(); iE++)
			m_lEdgeMinTess[iE] = GetMinTesselation(&m_lEdges[iE]);
	}

	bool bUseCompiled = true;
	m_lEdgeTess.SetLen(m_lEdges.Len());
	fp32* pTess = m_lEdgeTess.GetBasePtr();
	fp32* pMinTess = m_lEdgeMinTess.GetBasePtr();
	CSplineBrush_Face* pFaces = m_lFaces.GetBasePtr();

	for(int iE = 0; iE < m_lEdges.Len(); iE++)
	{
		CSplineBrush_Edge* pE = &m_lEdges[iE];
		int FaceFlags = 0;
		if (pE->m_iFaces[0] >= 0) FaceFlags |= pFaces[pE->m_iFaces[0]].m_Flags;
		if (pE->m_iFaces[1] >= 0) FaceFlags |= pFaces[pE->m_iFaces[1]].m_Flags;
		if (FaceFlags & XW_FACE_VISIBLE)
		{
			pTess[iE] = GetTesselation(&m_lEdges[iE], _ViewPos);
			if (pTess[iE] - pMinTess[iE] < CSB_EPSILON)
				pTess[iE] = pMinTess[iE];
			else
				bUseCompiled = false;
		}
	}
	return bUseCompiled;
}

void CSplineBrush::InitTesselation(fp32 _Level)
{
	MAUTOSTRIP(CSplineBrush_InitTesselation_2, MAUTOSTRIP_VOID);
	// Initialize min-tesselation if needed.
	if (m_lEdgeMinTess.Len() != m_lEdges.Len())
	{
		m_lEdgeMinTess.SetLen(m_lEdges.Len());
		for(int iE = 0; iE < m_lEdges.Len(); iE++)
			m_lEdgeMinTess[iE] = GetMinTesselation(&m_lEdges[iE]);
	}

	m_lEdgeTess.SetLen(m_lEdges.Len());
	fp32* pTess = m_lEdgeTess.GetBasePtr();
	fp32* pMinTess = m_lEdgeMinTess.GetBasePtr();
//	CSplineBrush_Face* pFaces = m_lFaces.GetBasePtr();

	for(int iE = 0; iE < m_lEdges.Len(); iE++)
	{
//		CSplineBrush_Edge* pE = &m_lEdges[iE];
		pTess[iE] = GetTesselation(&m_lEdges[iE], _Level);
		if (pTess[iE] - pMinTess[iE] < CSB_EPSILON)
			pTess[iE] = pMinTess[iE];
	}
}

#ifndef PLATFORM_CONSOLE
bool CSplineBrush::Ogr_InitTesselation(const CVec3Dfp32& _ViewPos)
{
	MAUTOSTRIP(CSplineBrush_Ogr_InitTesselation, false);
	// returns true if compiled faces could be used.
	if (_ViewPos.Distance(m_BoundPos) - m_BoundRadius > m_MinTessDistance) return true;

	// Initialize min-tesselation if needed.
	if (m_lEdgeMinTess.Len() != m_lEdges.Len())
	{
		m_lEdgeMinTess.SetLen(m_lEdges.Len());
		for(int iE = 0; iE < m_lEdges.Len(); iE++)
			m_lEdgeMinTess[iE] = GetMinTesselation(&m_lEdges[iE]);
	}

	bool bUseCompiled = true;
	m_lEdgeTess.SetLen(m_lEdges.Len());
	fp32* pTess = m_lEdgeTess.GetBasePtr();
	fp32* pMinTess = m_lEdgeMinTess.GetBasePtr();
	CSplineBrush_Face* pFaces = m_lFaces.GetBasePtr();

	for(int iE = 0; iE < m_lEdges.Len(); iE++)
	{
		CSplineBrush_Edge* pE = &m_lEdges[iE];
		int FaceFlags = 0;
		if (pE->m_iFaces[0] >= 0) FaceFlags |= pFaces[pE->m_iFaces[0]].m_Flags;
		if (pE->m_iFaces[1] >= 0) FaceFlags |= pFaces[pE->m_iFaces[1]].m_Flags;
		if (FaceFlags & XW_FACE_VISIBLE)
		{
			pTess[iE] = GetTesselation(&m_lEdges[iE], _ViewPos);
			if (pTess[iE] - pMinTess[iE] < CSB_EPSILON)
				pTess[iE] = pMinTess[iE];
			else
				bUseCompiled = false;
		}
	}

	fp32 TessTresh = 0.7f;
	int NumLoops = 0;
	bool bChanged = true;
	while(bChanged)
	{
		bChanged = false;
		for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
		{
			CSplineBrush_Face* pF = &m_lFaces[iFace];

			int ie0 = m_liEdges[pF->m_iiEdges] & 0x7fff;
			int ie1 = m_liEdges[pF->m_iiEdges+1] & 0x7fff;
			int ie2 = m_liEdges[pF->m_iiEdges+2] & 0x7fff;
			int ie3 = m_liEdges[pF->m_iiEdges+3] & 0x7fff;
			CSplineBrush_Edge* pE0 = &m_lEdges[ie0];
			CSplineBrush_Edge* pE1 = &m_lEdges[ie1];
			CSplineBrush_Edge* pE2 = &m_lEdges[ie2];
			CSplineBrush_Edge* pE3 = &m_lEdges[ie3];
			fp32 fnS1 = m_lEdgeTess[ie0];
			fp32 fnS2 = m_lEdgeTess[ie2];
			fp32 fnT1 = m_lEdgeTess[ie1];
			fp32 fnT2 = m_lEdgeTess[ie3];

			if (fnS1 * TessTresh > fnS2 && !pE2->m_Tess)
			{
				m_lEdgeTess[ie2] = fnS1*TessTresh;
				bChanged = true;
			}
			else if (fnS2 * TessTresh > fnS1 && !pE0->m_Tess)
			{
				m_lEdgeTess[ie0] = fnS2*TessTresh;
				bChanged = true;
			}

			if (fnT1*TessTresh > fnT2 && !pE3->m_Tess)
			{
				m_lEdgeTess[ie3] = fnT1*TessTresh;
				bChanged = true;
			}
			else if (fnT2*TessTresh > fnT1 && !pE1->m_Tess)
			{
				m_lEdgeTess[ie1] = fnT2*TessTresh;
				bChanged = true;
			}
		}

		if (NumLoops++ > 20)
		{
			ConOut("That a lot of loops.");
			break;
		}
	}


	return bUseCompiled;
}
#endif

void CSplineBrush::Parse(CKeyContainer* _pKeys)
{
	MAUTOSTRIP(CSplineBrush_Parse, MAUTOSTRIP_VOID);
	// -----------------------------
	// Parse parameters
	int ik = _pKeys->GetKeyIndex("SPLINE_FLAGS");
	if (ik >= 0) m_Flags = _pKeys->GetKeyValue(ik).Val_int();

	// -----------------------------
	// Parse origin
	CVec3Dfp32 Origin(0);
	int ikOrigin = _pKeys->GetKeyIndex("SPLINE_ORIGIN");
	if (ikOrigin < 0) ikOrigin = _pKeys->GetKeyIndex("ORIGIN");
	if (ikOrigin >= 0)
	{
		CStr s = _pKeys->GetKeyValue(ikOrigin);
		CVec3Dfp32 v;
		v.ParseString(s);
		Origin = v; //.GetSnapped(CSB_SNAPGRID, CSB_SNAPTRESH);
	}

	// -----------------------------
	// Vertices	
	int ikVerts = _pKeys->GetKeyIndex("SPLINE_VERTICES");
	if (ikVerts < 0) return;

	{
		CStr s = _pKeys->GetKeyValue(ikVerts);
		while(s != "")
		{
			CVec3Dfp32 v;
			v.k[0] = s.GetStrSep(",").Val_fp64();
			v.k[1] = s.GetStrSep(",").Val_fp64();
			v.k[2] = s.GetStrSep(",").Val_fp64();
//			v.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);
			v += Origin;
			m_lVertices.Add(v);
//		LogFile("V: " + v.GetString());
			s.Trim();
		}
	}

	// -----------------------------
	// Edges
	int iE = 0;
	int ikEdge2 = 0;
	int ikEdge3 = 0;
	while( ((ikEdge2 = _pKeys->GetKeyIndex(CStrF("SPLINE_EDGE%d", iE))) >= 0) ||
		((ikEdge3 = _pKeys->GetKeyIndex(CStrF("SPLINE_EDGE3_%d", iE))) >= 0) )
	{
		CStr s =_pKeys->GetKeyValue((ikEdge2 >= 0) ? ikEdge2 : ikEdge3);
		int iv0 = s.GetStrSep(",").Val_int();
		int iv1 = s.GetStrSep(",").Val_int();
		CVec3Dfp32 bend, bend2;
		bend.k[0] = s.GetStrSep(",").Val_fp64();
		bend.k[1] = s.GetStrSep(",").Val_fp64();
		bend.k[2] = s.GetStrSep(",").Val_fp64();
//		bend.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);

//	LogFile("E: " + bend.GetString());
		if ((iv0 & CSB_EDGEAND) < 0 || (iv0 & CSB_EDGEAND) >= m_lVertices.Len()) Error("Parse", "Edge-vertex index out of range.");
		if ((iv1 & CSB_EDGEAND) < 0 || (iv1 & CSB_EDGEAND) >= m_lVertices.Len()) Error("Parse", "Edge-vertex index out of range.");

		if (ikEdge2 >= 0)
			AddEdge(iv0, iv1, bend);
		else
		{
			bend2.k[0] = s.GetStrSep(",").Val_fp64();
			bend2.k[1] = s.GetStrSep(",").Val_fp64();
			bend2.k[2] = s.GetStrSep(",").Val_fp64();
//			bend2.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);

			int ie = AddEdge4(iv0, iv1, bend, bend2);

			m_lEdges[ie].m_Flags = s.GetStrSep(",").Val_int();
			m_lEdges[ie].m_Tess = s.GetStrSep(",").Val_int();
		}
		iE++;
	}

	// -----------------------------
	// Faces
	int ikFace = 0;
	int iFace = 0;
	while((ikFace = _pKeys->GetKeyIndex(CStrF("SPLINE_QUAD%d", iFace)) ) >= 0)
	{
		//	"spline_quad0" "0, 1, 2, 3, rock_wall3, 0, 0, 0, 0.500000, 0.500000"

		CStr s = _pKeys->GetKeyValue(ikFace);

		CSplineBrush_Face Face;

		Face.m_iiEdges = m_liEdges.Add((s.GetStrSep(",")).Val_int());
		m_liEdges.Add((s.GetStrSep(",")).Val_int());
		m_liEdges.Add((s.GetStrSep(",")).Val_int());
		m_liEdges.Add((s.GetStrSep(",")).Val_int());
		Face.m_nEdges = 4;

		CStr SurfName = s.GetStrSep(",");
		SurfName.Trim();
		SurfName.MakeUpperCase();
		m_lFaceSurf.Add(SurfName);
		Face.m_Mapping[3][0] = s.GetStrSep(",").Val_fp64();
		Face.m_Mapping[3][1] = s.GetStrSep(",").Val_fp64();
		fp32 UVRotate = s.GetStrSep(",").Val_fp64() / 360.0f;
		Face.m_Mapping[0][0] = s.GetStrSep(",").Val_fp64();
		Face.m_Mapping[1][1] = s.GetStrSep(",").Val_fp64();
		RotateElements(Face.m_Mapping[0][0], Face.m_Mapping[0][1], UVRotate);
		RotateElements(Face.m_Mapping[1][0], Face.m_Mapping[1][1], UVRotate);
		Face.m_Flags |= (XW_FACE_MAPPING_ST << XW_FACE_MAPPING_SHIFT);

/*		Face.m_UVOffset.k[0] = s.GetStrSep(",").Val_fp64();
		Face.m_UVOffset.k[1] = s.GetStrSep(",").Val_fp64();
		Face.m_UVRotate = s.GetStrSep(",").Val_fp64();
		Face.m_UVScale.k[0] = s.GetStrSep(",").Val_fp64();
		Face.m_UVScale.k[1] = s.GetStrSep(",").Val_fp64();*/
		int iF = m_lFaces.Add(Face);

		for(int i = 0; i < 4; i++)
			m_lEdges[m_liEdges[Face.m_iiEdges+i] & 0x7fff].AddFace(iF);

		iFace++;
	}

	// -----------------------------
	// Faces
	ikFace = 0;
	iFace = 0;
	while((ikFace = _pKeys->GetKeyIndex(CStrF("SPLINE_QUAD2_%d", iFace)) ) >= 0)
	{
		//	"spline_quad2_0" "0, 1, 2, 3, rock_wall3, m00, m01, m02, m03, m10, m11, m12, m13"

		CStr s = _pKeys->GetKeyValue(ikFace);

		CSplineBrush_Face Face;

		Face.m_Flags = (s.GetStrSep(",")).Val_int();
		Face.m_Flags &= XW_FACE_SPLINELOADMASK;
		Face.m_iiEdges = m_liEdges.Add((s.GetStrSep(",")).Val_int());
		m_liEdges.Add((s.GetStrSep(",")).Val_int());
		m_liEdges.Add((s.GetStrSep(",")).Val_int());
		m_liEdges.Add((s.GetStrSep(",")).Val_int());
		Face.m_nEdges = 4;

		CStr SurfName = s.GetStrSep(",");
		SurfName.Trim();
		m_lFaceSurf.Add(SurfName);
		Face.m_Mapping[0][0] = s.GetStrSep(",").Val_fp64();
		Face.m_Mapping[1][0] = s.GetStrSep(",").Val_fp64();
		Face.m_Mapping[2][0] = s.GetStrSep(",").Val_fp64();
		Face.m_Mapping[3][0] = s.GetStrSep(",").Val_fp64();
		Face.m_Mapping[0][1] = s.GetStrSep(",").Val_fp64();
		Face.m_Mapping[1][1] = s.GetStrSep(",").Val_fp64();
		Face.m_Mapping[2][1] = s.GetStrSep(",").Val_fp64();
		Face.m_Mapping[3][1] = s.GetStrSep(",").Val_fp64();

		// Check for invalid scales
/*		int Mapping = (Face.m_Flags >> XW_FACE_MAPPING_SHIFT) & XW_FACE_MAPPING_MASK;
		switch(Mapping)
		{
		case XW_FACE_MAPPING_FIXEDST:
		case XW_FACE_MAPPING_ST:
			{
				CVec2Dfp32 SU(Face.m_Mapping[0][0], Face.m_Mapping[0][1]);
				CVec2Dfp32 SV(Face.m_Mapping[1][0], Face.m_Mapping[1][1]);
				if(SU.LengthSqr() == 0 || SV.LengthSqr() == 0)
				{
					Face.m_Mapping[0][0] = 0;
					Face.m_Mapping[0][1] = 1;
					Face.m_Mapping[1][0] = 1;
					Face.m_Mapping[1][1] = 0;
				}
			}
			break;
		case XW_FACE_MAPPING_XYZ :
			{
				CVec3Dfp32 SU(Face.m_Mapping[0][0], Face.m_Mapping[1][0], Face.m_Mapping[2][0]);
				CVec3Dfp32 SV(Face.m_Mapping[0][1], Face.m_Mapping[1][1], Face.m_Mapping[2][1]);
				if(SU.LengthSqr() == 0 || SV.LengthSqr() == 0)
				{
					Face.m_Mapping[0][0] = 0;
					Face.m_Mapping[0][1] = 1;
					Face.m_Mapping[1][0] = 1;
					Face.m_Mapping[1][1] = 0;
					Face.m_Mapping[2][0] = 0;
					Face.m_Mapping[2][1] = 0;
				}
			}
			break;
		}*/

		int iF = m_lFaces.Add(Face);
		for(int i = 0; i < 4; i++)
			m_lEdges[m_liEdges[Face.m_iiEdges+i] & 0x7fff].AddFace(iF);

		iFace++;
	}

	FixXYZPlanes();

	ModelChanged();


	if (M_Fabs(m_BoundBox.m_Min[0]) > 100000.0f || M_Fabs(m_BoundBox.m_Min[1]) > 100000.0f || M_Fabs(m_BoundBox.m_Min[2]) > 100000.0f ||
		M_Fabs(m_BoundBox.m_Max[0]) > 100000.0f || M_Fabs(m_BoundBox.m_Max[1]) > 100000.0f || M_Fabs(m_BoundBox.m_Max[2]) > 100000.0f)
		LogFile(CStrF("    WARNING: Wierd m_Bounding box for spline brush %s", m_BoundBox.GetString().Str()));
}

bool CSplineBrush::ParseKey(CStr _Key, CStr _Value)
{
	MAUTOSTRIP(CSplineBrush_ParseKey, false);
	if (_Key == "SPLINE_FLAGS")
		m_Flags = _Value.Val_int();
	else 
		return false;
	return true;
}

void CSplineBrush::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CSplineBrush_Read, MAUTOSTRIP_VOID);
	MSCOPESHORT(CSplineBrush::Read);
	CCFile* pF = _pDFile->GetFile();

	// PARAMS
	{
		_pDFile->PushPosition();
		if (_pDFile->GetNext("PARAMETERS"))
		{
			if(_pDFile->GetEntrySize() != 4)
				Error("Read", CStrF("Unsupported parameters-version. (%d != 4)", _pDFile->GetEntrySize()));
			pF->ReadLE(m_Flags);
		}
		_pDFile->PopPosition();
	}

	// VERTICES
	{
		_pDFile->PushPosition();
		if (!_pDFile->GetNext("VERTICES")) Error("Read", "No VERTICES entry.");
		m_lVertices.SetLen(_pDFile->GetUserData());
		for(int i = 0; i < m_lVertices.Len(); i++)
			m_lVertices[i].Read(pF);
		_pDFile->PopPosition();
	}

	// EDGES
	{
		_pDFile->PushPosition();
		if (!_pDFile->GetNext("EDGES")) Error("Read", "No EDGES entry.");
		m_lEdges.SetLen(_pDFile->GetUserData());
		for(int i = 0; i < m_lEdges.Len(); i++)
			m_lEdges[i].Read(pF);
		_pDFile->PopPosition();
	}

	// EDGEINDICES
	{
		_pDFile->PushPosition();
		if (!_pDFile->GetNext("EDGEINDICES")) Error("Read", "No EDGEINDICES entry.");
		m_liEdges.SetLen(_pDFile->GetUserData());
		for(int i = 0; i < m_liEdges.Len(); i++)
			pF->ReadLE(m_liEdges[i]);
		_pDFile->PopPosition();
	}

	// FACES
	{
		_pDFile->PushPosition();
		if (!_pDFile->GetNext("FACES")) Error("Read", "No FACES entry.");
		m_lFaces.SetLen(_pDFile->GetUserData());
		for(int i = 0; i < m_lFaces.Len(); i++)
			m_lFaces[i].Read(pF);
		_pDFile->PopPosition();
	}

	// FACESURFACES
	{
		_pDFile->PushPosition();
		if (!_pDFile->GetNext("FACESURFACES")) Error("Read", "No FACESURFACES entry.");
		m_lFaceSurf.SetLen(_pDFile->GetUserData());
		for(uint i = 0; i < m_lFaceSurf.Len(); i++)
			m_lFaceSurf[i].Read(pF);
		_pDFile->PopPosition();
	}

	ModelChanged(true);
}

void CSplineBrush::Write(CDataFile* _pDFile)
{
	MAUTOSTRIP(CSplineBrush_Write, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();

	// PARAMETERS
	{
		_pDFile->BeginEntry("PARAMETERS");
		pF->WriteLE(m_Flags);
		_pDFile->EndEntry(4);
	}

	// VERTICES
	{
		_pDFile->BeginEntry("VERTICES");
		for(int i = 0; i < m_lVertices.Len(); i++)
			m_lVertices[i].Write(pF);
		_pDFile->EndEntry(m_lVertices.Len());
	}

	// EDGES
	{
		_pDFile->BeginEntry("EDGES");
		for(int i = 0; i < m_lEdges.Len(); i++)
			m_lEdges[i].Write(pF);
		_pDFile->EndEntry(m_lEdges.Len());
	}

	// EDGEINDICES
	{
		_pDFile->BeginEntry("EDGEINDICES");
		for(int i = 0; i < m_liEdges.Len(); i++)
			pF->WriteLE(m_liEdges[i]);
		_pDFile->EndEntry(m_liEdges.Len());
	}

	// FACES
	{
		_pDFile->BeginEntry("FACES");
		for(int i = 0; i < m_lFaces.Len(); i++)
			m_lFaces[i].Write(pF);
		_pDFile->EndEntry(m_lFaces.Len());
	}

	// FACESURFACES
	if (m_lFaceSurf.Len())
	{
		_pDFile->BeginEntry("FACESURFACES");
		for(int i = 0; i < m_lFaceSurf.Len(); i++)
			m_lFaceSurf[i].Write(pF);
		_pDFile->EndEntry(m_lFaceSurf.Len());
	}
}

CSplineBrush &CSplineBrush::operator =(const CSplineBrush &_SplineBrush)
{
	MAUTOSTRIP(CSplineBrush_operator_assign, *((CSplineBrush*)NULL));
	m_lVertices.Clear();
	m_lVertices.Add(&_SplineBrush.m_lVertices);
	m_liEdges.Clear();
	m_liEdges.Add(&_SplineBrush.m_liEdges);
	m_lEdges.Clear();
	m_lEdges.Add(&_SplineBrush.m_lEdges);
	m_lFaces.Clear();
	m_lFaces.Add(&_SplineBrush.m_lFaces);
	m_lFaceSurf.Clear();
	m_lFaceSurf.Add(&_SplineBrush.m_lFaceSurf);

	ModelChanged();

	return *this;
}

void CSplineBrush::CalcScale(CVec2Dfp32* _pPreScale, bool _bRecord)
{
	MAUTOSTRIP(CSplineBrush_CalcScale, MAUTOSTRIP_VOID);
	for(int f = 0; f < m_lFaces.Len(); f++)
	{
		CSplineBrush_Face* pF = &m_lFaces[f];
		if(((pF->m_Flags >> XW_FACE_MAPPING_SHIFT) & XW_FACE_MAPPING_MASK) == XW_FACE_MAPPING_FIXEDST)
		{
			CVec2Dfp32 S = GetMappingScale(f);
			if (_bRecord)
			{
				CVec2Dfp32 NewScale = S;
//				float Rot =  CVec3Dfp32::AngleFromVector(pF->m_Mapping[0][0], pF->m_Mapping[1][0]);
//				RotateElements(NewScale.k[0], NewScale.k[1], Rot);
				pF->TransformMapping(CVec2Dfp32(0), CVec2Dfp32(_pPreScale[f].k[0] / NewScale.k[0], _pPreScale[f].k[1] / NewScale.k[1]), 0);
			}
			else
			{
				_pPreScale[f] = S;
//				float Rot =  CVec3Dfp32::AngleFromVector(pF->m_Mapping[0][0], pF->m_Mapping[1][0]);
//				RotateElements(_pPreScale[f].k[0], _pPreScale[f].k[1], Rot);
			}
		}
	}
}

void CSplineBrush::FixXYZPlane(int _iFace)
{
	MAUTOSTRIP(CSplineBrush_FixXYZPlane, MAUTOSTRIP_VOID);
	CSplineBrush_Face *pF = &m_lFaces[_iFace];
	if(((pF->m_Flags >> XW_FACE_MAPPING_SHIFT) & XW_FACE_MAPPING_MASK) == XW_FACE_MAPPING_XYZ)
	{
		CVec2Dfp32 O, S;
		CVec3Dfp32 U, V;
		float Rot;
		GetMapping(_iFace, O, S, Rot, U, V);
//		LogFile(CStrF("GetMapping0:  O: %s  S: %s  Rot: %s  U: %s  V: %s", (char *)O.GetFilteredString(), (char *)S.GetFilteredString(), (char *)CStr::GetFilteredString(Rot), (char *)U.GetFilteredString(), (char *)V.GetFilteredString()));
		GetBoxMappingPlane(_iFace, Rot, U, V);
//		LogFile(CStrF("GetBoxPlane0: O: %s  S: %s  Rot: %s  U: %s  V: %s", (char *)O.GetFilteredString(), (char *)S.GetFilteredString(), (char *)CStr::GetFilteredString(Rot), (char *)U.GetFilteredString(), (char *)V.GetFilteredString()));
		U *= 1.0f / S.k[0];
		V *= 1.0f / S.k[1];
		pF->m_Mapping[0][0] = U.k[0];
		pF->m_Mapping[1][0] = U.k[1];
		pF->m_Mapping[2][0] = U.k[2];
		pF->m_Mapping[3][0] = O.k[0];
		pF->m_Mapping[0][1] = V.k[0];
		pF->m_Mapping[1][1] = V.k[1];
		pF->m_Mapping[2][1] = V.k[2];
		pF->m_Mapping[3][1] = O.k[1];
		GetMapping(_iFace, O, S, Rot, U, V);
//		LogFile(CStrF("GetMapping1:  O: %s  S: %s  Rot: %s  U: %s  V: %s", (char *)O.GetFilteredString(), (char *)S.GetFilteredString(), (char *)CStr::GetFilteredString(Rot), (char *)U.GetFilteredString(), (char *)V.GetFilteredString()));
		GetBoxMappingPlane(_iFace, Rot, U, V);
//		LogFile(CStrF("GetBoxPlane1: O: %s  S: %s  Rot: %s  U: %s  V: %s", (char *)O.GetFilteredString(), (char *)S.GetFilteredString(), (char *)CStr::GetFilteredString(Rot), (char *)U.GetFilteredString(), (char *)V.GetFilteredString()));
	}
}

void CSplineBrush::FixXYZPlanes()
{
	MAUTOSTRIP(CSplineBrush_FixXYZPlanes, MAUTOSTRIP_VOID);
	int nFaces = m_lFaces.Len();
	for(int i = 0; i < nFaces; i++)
		FixXYZPlane(i);
}

#define MAT_EPSILON 0.001f

static bool IsNiceMatrix(const CMat4Dfp32 &_Mat)
{
	MAUTOSTRIP(IsNiceMatrix, false);
	CVec3Dfp32 V0 = CVec3Dfp32::GetMatrixRow(_Mat, 0);
	CVec3Dfp32 V1 = CVec3Dfp32::GetMatrixRow(_Mat, 1);
	CVec3Dfp32 V2 = CVec3Dfp32::GetMatrixRow(_Mat, 2);
	V0.Normalize();
	V1.Normalize();
	V2.Normalize();

	if (M_Fabs(V0 * V1) > MAT_EPSILON) return false;
	if (M_Fabs(V0 * V2) > MAT_EPSILON) return false;
	if (M_Fabs(V1 * V2) > MAT_EPSILON) return false;

	static CVec3Dfp32 X(1,0,0);
	static CVec3Dfp32 Y(0,1,0);
	static CVec3Dfp32 Z(0,0,1);

	fp32 dotp = M_Fabs(V0 * X);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;
	dotp = M_Fabs(V0 * Y);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;
	dotp = M_Fabs(V0 * Z);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;

	dotp = M_Fabs(V1 * X);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;
	dotp = M_Fabs(V1 * Y);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;
	dotp = M_Fabs(V1 * Z);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;

	dotp = M_Fabs(V2 * X);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;
	dotp = M_Fabs(V2 * Y);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;
	dotp = M_Fabs(V2 * Z);
	if ((dotp > MAT_EPSILON) && (dotp < (1.0f-MAT_EPSILON))) return false;

	return true;
}

void CSplineBrush::Apply(const CMat4Dfp32 &_Mat, bool _bSnap, bool _bTextureLock)
{
	MAUTOSTRIP(CSplineBrush_Apply, MAUTOSTRIP_VOID);
	if (_bSnap) _bSnap = IsNiceMatrix(_Mat);

	CVec2Dfp32 PreScales[256];
	CalcScale(PreScales, false);

	int nVert = m_lVertices.Len();
	int i;
	for(i = 0; i < nVert; i++)
	{
		m_lVertices[i] *= _Mat;
		if(_bSnap)
			m_lVertices[i].Snap(CSB_SNAPGRID, CSB_SNAPTRESH);
	}

	int nEdges = m_lEdges.Len();
	for(i = 0; i < nEdges; i++)
	{
		CVec3Dfp32 v0, v1, b0, b1;
		m_lEdges[i].GetVectors4(v0, v1, b0, b1);
		v0.MultiplyMatrix(_Mat);
		v1.MultiplyMatrix(_Mat);
		b0.MultiplyMatrix3x3(_Mat);
		b1.MultiplyMatrix3x3(_Mat);
		if(_bSnap)
		{
			v0.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);
			v1.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);
			b0.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);
			b1.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);
		}
		m_lEdges[i].SetVectors4(v0, v1, b0, b1);
	}

	CalcScale(PreScales, true);

	FixXYZPlanes();


// TODO!
// Fix the mess below to handle movement first, rotations later.
// It also needs to handle mirroring for all kinds of rotations
// May require some re-write of the other mapping code -ae
	CMat4Dfp32 Temp;
	if (_bTextureLock && (_Mat.Inverse(Temp)))
	{
		CMat4Dfp32 TT;
		//m_TextureTransform.Multiply(_Mat, TT);
		_Mat.Multiply(m_TextureTransform, TT);
		//m_TextureTransform = TT;
		bool bMirror;
		bool lbIsNeg[3] = { !(AlmostEqual(_Mat.k[0][0],1.0f,0.001f)),
			!(AlmostEqual(_Mat.k[1][1],1.0f,0.001f)),
			!(AlmostEqual(_Mat.k[2][2],1.0f,0.001f)) };

		//If all axes were messed with, *or* none (identity matrix), we don't need to mirror
		if( (lbIsNeg[0] && lbIsNeg[1] && lbIsNeg[2]) || ((!lbIsNeg[0]) && (!lbIsNeg[1]) && (!lbIsNeg[2])) )
		{
			bMirror = false;
		}

		//If we had a 180 turn, we don't need to mirror either
		else if( (!lbIsNeg[0] || (_Mat.k[0][0] < 0)) && (!lbIsNeg[1] || (_Mat.k[1][1] < 0)) &&
			(!lbIsNeg[2] || (_Mat.k[2][2] < 0)) )
		{
			bMirror = false;
		}
		else
		{
			fp32 Sum = Abs(_Mat.k[0][0]) + Abs(_Mat.k[1][0]) + Abs(_Mat.k[2][0]) + 
				Abs(_Mat.k[0][1]) + Abs(_Mat.k[1][1]) + Abs(_Mat.k[2][1]) + 
				Abs(_Mat.k[0][2]) + Abs(_Mat.k[1][2]) + Abs(_Mat.k[2][2]);
			bMirror = AlmostEqual(Sum,3.0f,0.001f);
		}


		//*
		for(i = 0; i < m_lFaces.Len(); i++)
		{
			CSplineBrush_Face& Face = m_lFaces[i];
			//			CVec3Dfp32 &Move = CVec3Dfp32::GetRow(_Mat, 3);
			//			Move.MultiplyMatrix(Temp);
			if (GetMappingType(i) == XW_FACE_MAPPING_XYZ)
			{
				fp32 pBuf[4][2];
				memcpy(pBuf,Face.m_Mapping,8*sizeof(fp32));

				if( bMirror )
				{
					//Determine if this face needs to be mirrored by looking at the normal
					CVec3Dfp32 Normal = EvalNormal(i,0.5f,0.5f);
					uint8 iAxis = (AlmostEqual(Abs(Normal.k[0]),1.0f,0.001f)) ? 0 :
					((AlmostEqual(Abs(Normal.k[1]),1.0f,0.001f)) ? 1 : 2);
					CMat4Dfp32 Mat = Temp;

					//Mirror matrix
					if( lbIsNeg[iAxis] )
					{
						//	do iAxis = (iAxis+1) % 3; while( lbIsNeg[iAxis] );
						Mat.k[iAxis][0] = -Mat.k[iAxis][0];
						Mat.k[iAxis][1] = -Mat.k[iAxis][1];
						Mat.k[iAxis][2] = -Mat.k[iAxis][2];
						//Face.m_Mapping[3][1] = -Abs(Face.m_Mapping[3][1]);
					}

					//Multiply...
					for(int j=0;j < 3;j++)
					{
						Face.m_Mapping[j][0] = pBuf[0][0] * Mat.k[j][0] + pBuf[1][0] * Mat.k[j][1] + pBuf[2][0] * Mat.k[j][2];
						Face.m_Mapping[j][1] = pBuf[0][1] * Mat.k[j][0] + pBuf[1][1] * Mat.k[j][1] + pBuf[2][1] * Mat.k[j][2];
					}
					Face.m_Mapping[3][0] += pBuf[0][0] * Mat.k[3][0] + pBuf[1][0] * Mat.k[3][1] + pBuf[2][0] * Mat.k[3][2];
					Face.m_Mapping[3][1] += pBuf[0][1] * Mat.k[3][0] + pBuf[1][1] * Mat.k[3][1] + pBuf[2][1] * Mat.k[3][2];
				}

				//No mirroring, just multiply it
				else
				{
					for(int j=0;j < 3;j++)
					{
						Face.m_Mapping[j][0] = pBuf[0][0] * Temp.k[j][0] + pBuf[1][0] * Temp.k[j][1] + pBuf[2][0] * Temp.k[j][2];
						Face.m_Mapping[j][1] = pBuf[0][1] * Temp.k[j][0] + pBuf[1][1] * Temp.k[j][1] + pBuf[2][1] * Temp.k[j][2];
					}
					Face.m_Mapping[3][0] += pBuf[0][0] * Temp.k[3][0] + pBuf[1][0] * Temp.k[3][1] + pBuf[2][0] * Temp.k[3][2];
					Face.m_Mapping[3][1] += pBuf[0][1] * Temp.k[3][0] + pBuf[1][1] * Temp.k[3][1] + pBuf[2][1] * Temp.k[3][2];
				}
			}
		}
//*/
	}

	FixXYZPlanes();

//	UpdateBoundBox();
	ModelChanged();
}

void CSplineBrush::Snap(float _fGrid)
{
	MAUTOSTRIP(CSplineBrush_Snap, MAUTOSTRIP_VOID);
	CVec2Dfp32 PreScales[256];
	CalcScale(PreScales, false);

	{
		int nr = m_lVertices.Len();
		for(int i = 0; i < nr; i++)
			m_lVertices[i].Snap(_fGrid, _fGrid);
	}

	{
		int nr = m_lEdges.Len();
		for(int i = 0; i < nr; i++)
		{
			CVec3Dfp32 v0, v1, b0, b1;
			m_lEdges[i].GetVectors4(v0, v1, b0, b1);
			v0.Snap(_fGrid, _fGrid);
			v1.Snap(_fGrid, _fGrid);
			b0.Snap(_fGrid, _fGrid);
			b1.Snap(_fGrid, _fGrid);
			m_lEdges[i].SetVectors4(v0, v1, b0, b1);
		}
	}

	CalcScale(PreScales, true);
	FixXYZPlanes();
	ModelChanged();
}

void CSplineBrush::MoveEdge(int _iIndex, const CVec3Dfp32 &_Bend0, const CVec3Dfp32 &_Bend1)
{
	MAUTOSTRIP(CSplineBrush_MoveEdge, MAUTOSTRIP_VOID);
	CVec2Dfp32 PreScales[256];
	CalcScale(PreScales, false);

//	int e0 = m_lEdges[_iIndex].m_iV[0];
//	int e1 = m_lEdges[_iIndex].m_iV[1];

	CVec3Dfp32 v0, v1, b0, b1;
	m_lEdges[_iIndex].GetVectors4(v0, v1, b0, b1);
	b0 += _Bend0;
	b1 += _Bend1;
//	b0.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);
//	b1.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);
	m_lEdges[_iIndex].SetVectors4(v0, v1, b0, b1);

	CalcScale(PreScales, true);
	ModelChanged();
}

void CSplineBrush::MoveVertex(int _iIndex, const CVec3Dfp32 &_Pos, bool _bHard)
{
	MAUTOSTRIP(CSplineBrush_MoveVertex, MAUTOSTRIP_VOID);
	CVec2Dfp32 PreScales[256];
	CalcScale(PreScales, false);

	m_lVertices[_iIndex] += _Pos;

	CVec3Dfp32 v0, v1, b0, b1;
	int nr = m_lEdges.Len();
	for(int i = 0; i < nr; i++)
	{
		if(m_lEdges[i].m_iV[0] == _iIndex || m_lEdges[i].m_iV[1] == _iIndex)
		{
			m_lEdges[i].GetVectors4(v0, v1, b0, b1);
			if(_bHard)
			{
				bool bBwd = m_lEdges[i].m_iV[1] == _iIndex;
				CVec3Dfp64 n0;
				CVec3Dfp64 n1;
				fp64 l0, l1;
				
				if(!bBwd)
				{
					n0 = (v0 - v1).Getfp64();
					n1 = ((v0 + _Pos) - v1).Getfp64();
				}
				else
				{
					n0 = (v0 - v1).Getfp64();
					n1 = (v0 - (v1 + _Pos)).Getfp64();
				}

				l0 = n0.Length();
				l1 = n1.Length();

				n0 *= 1.0f/l0;
				n1 *= 1.0f/l1;

				CVec3Dfp64 Ref = _Pos.Getfp64();
				Ref.Normalize();

				float l = (v0 - v1).Length();
				if((l != 0) && (M_Fabs(Ref*n0) < 1.0f-0.0000001f) && (l0 > 0.0001f) && (l1 > 0.0001f) && _Pos.LengthSqr() > 0.0001f)
				{
					CVec3Dfp32 Axis;
					(n0.Getfp32()).CrossProd(Ref.Getfp32(), Axis);
					Axis.Normalize();
					fp64 Angle = -0.5f*M_ACos(Max(Min(fp32(n0*n1), 1.0f), -1.0f)) / _PI;
					if (bBwd) Angle = -Angle;
					CMat4Dfp32 Rot;
					Axis.CreateAxisRotateMatrix(Angle, Rot);
					b0 *= Rot;
					b1 *= Rot;

					fp32 Scale = l1 / l0;
					b0 *= Scale;
					b1 *= Scale;
				}
			}
			if(m_lEdges[i].m_iV[0] == _iIndex)
				v0 += _Pos;
			if(m_lEdges[i].m_iV[1] == _iIndex)
				v1 += _Pos;

/*			v0.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);
			v1.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);
			b0.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);
			b1.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);*/
			m_lEdges[i].SetVectors4(v0, v1, b0, b1);
		}
	}

	CalcScale(PreScales, true);
	FixXYZPlanes();

	ModelChanged();
}

void CSplineBrush::MoveTexture(int _iIndex, const CVec2Dfp32 &_O, const CVec2Dfp32 &_S, float _Rot)
{
	MAUTOSTRIP(CSplineBrush_MoveTexture, MAUTOSTRIP_VOID);
	m_lFaces[_iIndex].TransformMapping(_O, _S, _Rot);


/*	m_lFaces[_iIndex].m_UVOffset += _O;
	m_lFaces[_iIndex].m_UVRotate += _Rot;
	m_lFaces[_iIndex].m_UVScale.k[0] *= _S.k[0];
	m_lFaces[_iIndex].m_UVScale.k[1] *= _S.k[1];*/
}

CVec2Dfp32 CSplineBrush::GetMappingScale(int _iFace)
{
	MAUTOSTRIP(CSplineBrush_GetMappingScale, CVec2Dfp32());
	CSplineBrush_Face* pF = &m_lFaces[_iFace];
	int ie0 = m_liEdges[pF->m_iiEdges];
	int ie1 = m_liEdges[pF->m_iiEdges+1];
	int ie2 = m_liEdges[pF->m_iiEdges+2];
	int ie3 = m_liEdges[pF->m_iiEdges+3];
	CSplineBrush_Edge* pE0 = &m_lEdges[ie0 & 0x7fff];
	CSplineBrush_Edge* pE1 = &m_lEdges[ie1 & 0x7fff];
	CSplineBrush_Edge* pE2 = &m_lEdges[ie2 & 0x7fff];
	CSplineBrush_Edge* pE3 = &m_lEdges[ie3 & 0x7fff];
	return CVec2Dfp32(
		(pE0->GetLength(20) + pE2->GetLength(20)) * 0.5f, 
		(pE1->GetLength(20) + pE3->GetLength(20)) * 0.5f);
}

void CSplineBrush::GetBoxMappingPlane(int _iFace, fp32 _Rot, CVec3Dfp32& _U, CVec3Dfp32& _V)
{
	MAUTOSTRIP(CSplineBrush_GetBoxMappingPlane, MAUTOSTRIP_VOID);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];

	CVec3Dfp32 v[4];
	int i;
	for(i = 0; i < 4; i++)
	{
		int ie = m_liEdges[pF->m_iiEdges + i];
		v[i] = (ie & 0x8000) ? m_lVertices[m_lEdges[ie & 0x7fff].m_iV[1]] : m_lVertices[m_lEdges[ie & 0x7fff].m_iV[0]];
	}
	CPlane3Dfp32 Plane[4];
	Plane[0].Create(v[0], v[1], v[2]);
	Plane[1].Create(v[1], v[2], v[3]);
	Plane[2].Create(v[0], v[2], v[3]);
	Plane[3].Create(v[0], v[1], v[3]);
	CVec3Dfp32 n(0);
	for(i = 0; i < 4; i++)
		n += Plane[i].n;
	n.Normalize();

	if (M_Fabs(n.k[2] + CSB_EPSILON) > M_Sqrt(Sqr(n.k[0]) + Sqr(n.k[1])))
	{
		_U = CVec3Dfp32(1.0f, 0.0f, 0.0f);
		_V = CVec3Dfp32(0.0f, -1.0f, 0.0f);
		RotateElements(_U.k[0], _U.k[1], fp32(-_Rot));
		RotateElements(_V.k[0], _V.k[1], fp32(-_Rot));
	}
	else
	{
		if (M_Fabs(n.k[0]) + CSB_EPSILON > M_Fabs(n.k[1]))
		{
			_U = CVec3Dfp32(0.0f, 1.0f, 0.0f);
			_V = CVec3Dfp32(0.0f, 0.0f, -1.0f);
			RotateElements(_U.k[1], _U.k[2], fp32(-_Rot));
			RotateElements(_V.k[1], _V.k[2], fp32(-_Rot));
		}
		else
		{
			_U = CVec3Dfp32(1.0f, 0.0f, 0.0f);
			_V = CVec3Dfp32(0.0f, 0.0f, -1.0f);
			RotateElements(_U.k[0], _U.k[2], fp32(-_Rot));
			RotateElements(_V.k[0], _V.k[2], fp32(-_Rot));
		}
	}
}

void CSplineBrush::SetMapping(int _iFace, const CVec2Dfp32 &_O, const CVec2Dfp32 &_S, float _Rot)
{
	MAUTOSTRIP(CSplineBrush_SetMapping, MAUTOSTRIP_VOID);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];
	int Mapping = (pF->m_Flags >> XW_FACE_MAPPING_SHIFT) & XW_FACE_MAPPING_MASK;
	switch(Mapping)
	{
	case XW_FACE_MAPPING_FIXEDST :
	case XW_FACE_MAPPING_ST :
		{
			CVec2Dfp32 S(_S);
			if(Mapping == XW_FACE_MAPPING_FIXEDST)
			{
				CVec2Dfp32 Scale = GetMappingScale(_iFace);
				S.k[0] = Scale.k[0] / S.k[0];
				S.k[1] = Scale.k[1] / S.k[1];
			}
			pF->m_Mapping[0][0] = S.k[0];
			pF->m_Mapping[1][0] = 0;
			pF->m_Mapping[0][1] = 0;
			pF->m_Mapping[1][1] = S.k[1];
			RotateElements(pF->m_Mapping[0][0], pF->m_Mapping[0][1], _Rot);
			RotateElements(pF->m_Mapping[1][0], pF->m_Mapping[1][1], _Rot);
			pF->m_Mapping[2][0] = _O.k[0];
			pF->m_Mapping[2][1] = _O.k[1];
		}
		break;

	case XW_FACE_MAPPING_XYZ :
		{
			CVec3Dfp32 U, V;
			GetBoxMappingPlane(_iFace, _Rot, U, V);
			U *= 1.0f / _S.k[0];
			V *= 1.0f / _S.k[1];
			pF->m_Mapping[0][0] = U.k[0];
			pF->m_Mapping[1][0] = U.k[1];
			pF->m_Mapping[2][0] = U.k[2];
			pF->m_Mapping[3][0] = _O.k[0];
			pF->m_Mapping[0][1] = V.k[0];
			pF->m_Mapping[1][1] = V.k[1];
			pF->m_Mapping[2][1] = V.k[2];
			pF->m_Mapping[3][1] = _O.k[1];
		}
		break;
	}
}

void CSplineBrush::GetMapping(int _iFace, CVec2Dfp32 &_O, CVec2Dfp32 &_S, float& _Rot, CVec3Dfp32 &_U, CVec3Dfp32 &_V)
{
	MAUTOSTRIP(CSplineBrush_GetMapping, MAUTOSTRIP_VOID);
	CSplineBrush_Face* pF = &m_lFaces[_iFace];
	int Mapping = (pF->m_Flags >> XW_FACE_MAPPING_SHIFT) & XW_FACE_MAPPING_MASK;
	switch(Mapping)
	{
	case XW_FACE_MAPPING_FIXEDST:
	case XW_FACE_MAPPING_ST:
		{
			CVec2Dfp32 SU(pF->m_Mapping[0][0], pF->m_Mapping[0][1]);
			CVec2Dfp32 SV(pF->m_Mapping[1][0], pF->m_Mapping[1][1]);
			_Rot =  CVec3Dfp32::AngleFromVector(pF->m_Mapping[0][0] / SU.Length(), pF->m_Mapping[1][0] / SV.Length());
			RotateElements(SU.k[0], SU.k[1], -_Rot);
			RotateElements(SV.k[0], SV.k[1], -_Rot);
//			_S.k[0] = Length2(pF->m_Mapping[0][0], pF->m_Mapping[1][0]);
//			_S.k[1] = Length2(pF->m_Mapping[0][1], pF->m_Mapping[1][1]);
			_S.k[0] = SU.Length();
			_S.k[1] = SV.Length();
			_O.k[0] = pF->m_Mapping[2][0];
			_O.k[1] = pF->m_Mapping[2][1];

			{
				//Fix for neagive scale
				float vU = CVec3Dfp32::AngleFromVector(SU[0], SU[1]);
				float vV = CVec3Dfp32::AngleFromVector(SV[0], SV[1]);
				if(vV - vU > 0.5f || vV - vU < 0)
				{
					_S[1] = -_S[1];
					_Rot = -_Rot;
				}
			}

			CVec3Dfp32 V = EvalXYZ(_iFace, 0, 0);
			CVec3Dfp32 VS = EvalXYZ(_iFace, 1, 0);
			CVec3Dfp32 VT = EvalXYZ(_iFace, 0, 1);
			CVec2Dfp32 UV = pF->EvalUV(this, 0, 0, V);
			CVec2Dfp32 UVS = pF->EvalUV(this, 1, 0, VS);
			CVec2Dfp32 UVT = pF->EvalUV(this, 0, 1, VT);
			UVS -= UV;
			UVT -= UV;
			VS -= V;
			VT -= V;
//			VS.Scale(1.0f / UVS.Length(), _U);
//			VT.Scale(1.0f / UVT.Length(), _V);
			_U = VS;
			_V = VT;
			_U.Normalize();
			_V.Normalize();
			CVec3Dfp32 N;
			_U.CrossProd(_V, N);
			N.Normalize();
			CQuatfp32 Q(N, -_Rot);
			CMat4Dfp32 M;
			Q.CreateMatrix(M);
			_U *= M;
			_V *= M;

			if(Mapping == XW_FACE_MAPPING_FIXEDST)
			{
				CVec2Dfp32 Scale = GetMappingScale(_iFace);
				_S.k[0] = Scale.k[0] / _S.k[0];
				_S.k[1] = Scale.k[1] / _S.k[1];
			}
//			RotateElements(_S.k[0], _S.k[1], -_Rot);
		}
		break;

	case XW_FACE_MAPPING_XYZ :
		{
			_U = CVec3Dfp32(pF->m_Mapping[0][0], pF->m_Mapping[1][0], pF->m_Mapping[2][0]);
			_V = CVec3Dfp32(pF->m_Mapping[0][1], pF->m_Mapping[1][1], pF->m_Mapping[2][1]);
			CVec3Dfp32 n, u0, v0;
			_U.CrossProd(_V, n);
			if (M_Fabs(n.k[2] + CSB_EPSILON) > M_Sqrt(Sqr(n.k[0]) + Sqr(n.k[1])))
			{
				u0 = CVec3Dfp32(1.0f, 0.0f, 0.0f);
				v0 = CVec3Dfp32(0.0f, -1.0f, 0.0f);
			}
			else
			{
				if (M_Fabs(n.k[0]) + CSB_EPSILON > M_Fabs(n.k[1]))
				{
					u0 = CVec3Dfp32(0.0f, 1.0f, 0.0f);
					v0 = CVec3Dfp32(0.0f, 0.0f, -1.0f);
				}
				else
				{
					u0 = CVec3Dfp32(1.0f, 0.0f, 0.0f);
					v0 = CVec3Dfp32(0.0f, 0.0f, -1.0f);
				}
			}
			float ud0 = u0 * _U;
			float ud1 = _U.Length() * u0.Length();

			CVec3Dfp32 RefN;
			u0.CrossProd(v0, RefN);

//			float vd0 = v0 * _V;
//			float vd1 = _V.Length() * v0.Length();

/*			float un1 = u0 * _V;
			float ud2 = v0 * _V;
			float res = ud0 * ud2;
*/
			float RotU = M_ACos(Max(Min(ud0 / ud1, 1.0f), -1.0f)) / (2.0f * _PI);

			if ((_U / u0) * RefN < 0.0f) RotU = -RotU;

//			float RotV = M_ACos(Max(Min(vd0 / vd1, 1.0f), -1.0f)) / (2.0f * _PI);

//			if(un1 < -CSB_EPSILON) _Rot = -_Rot;

/*			if((res > 0 && un1 < 0) || (res < 0 && un1 > 0))
				_Rot = -_Rot;*/
/*			LogFile(CStrF("U: %s  V: %s  ud0: %s  ud1: %s  ud2: %s  un1: %s res: %s Rot: %s",
					(char *)_U.GetFilteredString(),
					(char *)_V.GetFilteredString(),
					(char *)CStr::GetFilteredString(ud0),
					(char *)CStr::GetFilteredString(ud1),
					(char *)CStr::GetFilteredString(ud2),
					(char *)CStr::GetFilteredString(un1),
					(char *)CStr::GetFilteredString(res),
					(char *)CStr::GetFilteredString(_Rot)));*/


/*			LogFile(CStrF("res > 0: %s   un1 < 0: %s   res < 0: %s   un1 > 0: %s", 
				(res > 0) ? "true" : "false",
				(un1 < 0) ? "true" : "false",
				(res < 0) ? "true" : "false",
				(un1 > 0) ? "true" : "false"));*/

			_S[0] = 1.0f / _U.Length();
			_S[1] = 1.0f / _V.Length();
			_Rot = RotU;
			if (n * RefN < 0.0f)
				_S[1] = -_S[1];
/*			if (M_Fabs(RotU - RotV) > 0.25f)
				_S[0] = -_S[0];*/

//			if(res < 0) _S[1] = -_S[1];
			_O[0] = pF->m_Mapping[3][0];
			_O[1] = pF->m_Mapping[3][1];
		}
		break;
	}
}

int CSplineBrush::GetMappingType(int _iFace)
{
	MAUTOSTRIP(CSplineBrush_GetMappingType, 0);
	return (m_lFaces[_iFace].m_Flags >> XW_FACE_MAPPING_SHIFT) & XW_FACE_MAPPING_MASK;
}

bool CSplineBrush::SetMappingType(int _iFace, int _iMapping)
{
	MAUTOSTRIP(CSplineBrush_SetMappingType, false);
	CSplineBrush_Face *pF = &m_lFaces[_iFace];
	CVec2Dfp32 O, S; 
	fp32 Rot;
	CVec3Dfp32 U, V;
	GetMapping(_iFace, O, S, Rot, U, V);
	pF->m_Flags &= ~(XW_FACE_MAPPING_MASK << XW_FACE_MAPPING_SHIFT);
	pF->m_Flags |= (_iMapping << XW_FACE_MAPPING_SHIFT);
	SetMapping(_iFace, CVec2Dfp32(0), CVec2Dfp32(1), 0);
	pF->TransformMapping(O, S, Rot);

	return true;
}

#ifndef PLATFORM_CONSOLE
int CSplineBrush::GetMappingTypeOGR(int _iFace)
{
	MAUTOSTRIP(CSplineBrush_GetMappingTypeOGR, 0);
	switch(GetMappingType(_iFace))
	{
	case XW_FACE_MAPPING_FIXEDST: return OGIER_MAPPINGTYPE_FIXEDSTMAPPED;
	case XW_FACE_MAPPING_XYZ: return OGIER_MAPPINGTYPE_BOXMAPPED;
	case XW_FACE_MAPPING_ST: return OGIER_MAPPINGTYPE_STMAPPED;
	}

	return -1;
}

bool CSplineBrush::SetMappingTypeOGR(int _iFace, int _iMappingOGR)
{
	MAUTOSTRIP(CSplineBrush_SetMappingTypeOGR, false);
	switch(_iMappingOGR)
	{
	case OGIER_MAPPINGTYPE_FIXEDSTMAPPED : return SetMappingType(_iFace, XW_FACE_MAPPING_FIXEDST);
	case OGIER_MAPPINGTYPE_BOXMAPPED : return SetMappingType(_iFace, XW_FACE_MAPPING_XYZ);
	case OGIER_MAPPINGTYPE_STMAPPED : return SetMappingType(_iFace, XW_FACE_MAPPING_ST);
	}

	return false;
}
#endif

void CSplineBrush::GetKeys(CKeyContainer *_pKeys) const
{
	MAUTOSTRIP(CSplineBrush_GetKeys, MAUTOSTRIP_VOID);
	CVec3Dfp32 Origin = (m_BoundVertexBox.m_Min + m_BoundVertexBox.m_Max) * 0.5f;
	_pKeys->AddKey("SPLINE_ORIGIN", CStrF("%f, %f, %f", Origin.k[0], Origin.k[1], Origin.k[2]));

	if (m_Flags) _pKeys->AddKey("SPLINE_FLAGS", CStrF("%.8x", m_Flags));
//	if (m_UTess != -1) _pKeys->AddKey("SPLINE_UTESS", CStrF("%d", m_UTess));
//	if (m_VTess != -1) _pKeys->AddKey("SPLINE_VTESS", CStrF("%d", m_VTess));

	{
		CStr Vert;
		int nVert = m_lVertices.Len();
		for(int i = 0; i < nVert; i++)
		{
			CVec3Dfp32 v = (m_lVertices[i] - Origin);//.GetSnapped(CSB_SNAPGRID, CSB_SNAPTRESH);
			Vert += CStr(v.GetFilteredString());
			if(i != nVert - 1)
				Vert += ",   ";
		}
		_pKeys->AddKey("SPLINE_VERTICES", Vert);
	}

	{
		int nEdge = m_lEdges.Len();
		for(int i = 0; i < nEdge; i++)
		{
			CVec3Dfp32 v0, v1, b0, b1;
			m_lEdges[i].GetVectors4(v0, v1, b0, b1);
//			b0.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);
//			b1.Snap(CSB_SNAPGRID, CSB_SNAPTRESH);
			_pKeys->AddKey(CStrF("SPLINE_EDGE3_%i", i), CStrF("%i, %i, %s, %s, %d, %d", 
				m_lEdges[i].m_iV[0], m_lEdges[i].m_iV[1],
				(char*)b0.GetFilteredString(), 
				(char*)b1.GetFilteredString(), 
				m_lEdges[i].m_Flags, m_lEdges[i].m_Tess));
		}
	}

/*	{
		int nQuads = m_lFaceSurf.Len();
		for(int i = 0; i < nQuads; i++)
		{
			_pKeys->AddKey(CStrF("SPLINE_QUAD%i", i), CStrF("%i, %i, %i, %i, %s, %f, %f, %f, %f, %f",
														  m_liEdges[i * 4], m_liEdges[i * 4 + 1], 
														  m_liEdges[i * 4 + 2], m_liEdges[i * 4 + 3], 
														  (char *)m_lFaceSurf[i],
														  m_lFaces[i].m_UVOffset.k[0], m_lFaces[i].m_UVOffset.k[1],
														  m_lFaces[i].m_UVRotate,
														  m_lFaces[i].m_UVScale.k[0], m_lFaces[i].m_UVScale.k[1]));
		}
	}*/
	{
		int nQuads = m_lFaceSurf.Len();
		for(int i = 0; i < nQuads; i++)
		{
			const CSplineBrush_Face* pF = &m_lFaces[i];
			int iie = pF->m_iiEdges;
			_pKeys->AddKey(CStrF("SPLINE_QUAD2_%i", i), CStrF("0x%.8x, %i, %i, %i, %i, %s, %s, %s, %s, %s, %s, %s, %s, %s",
				pF->m_Flags, 
				m_liEdges[iie], m_liEdges[iie + 1], 
				m_liEdges[iie + 2], m_liEdges[iie + 3], 
				(const char*)m_lFaceSurf[i],
				(char*)CStr::GetFilteredString(pF->m_Mapping[0][0]),
				(char*)CStr::GetFilteredString(pF->m_Mapping[1][0]),
				(char*)CStr::GetFilteredString(pF->m_Mapping[2][0]),
				(char*)CStr::GetFilteredString(pF->m_Mapping[3][0]),
				(char*)CStr::GetFilteredString(pF->m_Mapping[0][1]),
				(char*)CStr::GetFilteredString(pF->m_Mapping[1][1]),
				(char*)CStr::GetFilteredString(pF->m_Mapping[2][1]),
				(char*)CStr::GetFilteredString(pF->m_Mapping[3][1])));
		}
	}
}

// -------------------------------------------------------------------
//  Rendering
// -------------------------------------------------------------------
#ifndef PLATFORM_CONSOLE
void CSplineBrush::InternalRenderFace(CRenderContext* _pRC, CXR_VBManager *_pVBM, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, int _iFace, CPixel32 _Col, int _iType, CMTime _AnimTime, COgrRenderBuffer &_RenderBuffer)
{
	MAUTOSTRIP(CSplineBrush_InternalRenderFace, MAUTOSTRIP_VOID);
	// ACHTUNG!, InitTesselation must've been run.

	CSplineBrush_Face* pF = &m_lFaces[_iFace];

	if(pF->m_pSurface && pF->m_pSurface->m_Flags & XW_SURFFLAGS_OGRINVISIBLE)
		return;

	CMat4Dfp32 L2V;
	_WMat.Multiply(_VMat, L2V);

	// Temp mesh.
	int nS, nT;
	nS = nT = 0;

	int ie0 = m_liEdges[pF->m_iiEdges];
	int ie2 = m_liEdges[pF->m_iiEdges+2];
	CSplineBrush_Edge* pE0 = &m_lEdges[ie0 & 0x7fff];
	CSplineBrush_Edge* pE2 = &m_lEdges[ie2 & 0x7fff];
	int ie1 = m_liEdges[pF->m_iiEdges+1];
	int ie3 = m_liEdges[pF->m_iiEdges+3];
	CSplineBrush_Edge* pE1 = &m_lEdges[ie1 & 0x7fff];
	CSplineBrush_Edge* pE3 = &m_lEdges[ie3 & 0x7fff];
	int nV = 0;

	CMat4Dfp32 L2VInv;
	L2V.InverseOrthogonal(L2VInv);

	const int MaxVerts = 1024;
	CVec2Dfp32 ST[MaxVerts];

	if(!(_iType & OGIER_RENDER_WIRE))
	{
		CVec3Dfp32 *pV = Tesselate(_iFace, MaxVerts, NULL, ST, nS, nT, _pVBM);
		if(!pV)
			return;
		nV = nS*nT;

		if (nT*2+6 > 256) return;	// Overflow!, can't build trianglestrip under this condition.

		CXR_VertexBuffer *pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB);
		if(!pVB)
			return;

		CXW_Surface *pSurf = NULL;
		if(_iType & OGIER_RENDER_TEXTURES && pF->m_pSurface)
			pSurf = pF->m_pSurface;

		if (!pVB->AllocVBChain(_pVBM, false))
			return;
		CXR_VBChain *pChain = pVB->GetVBChain();
		pVB->Geometry_VertexArray(pV, nV, true);
		pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_SMOOTH);

		if(_iType & OGIER_RENDER_CULL)
			pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_CULL);
		else
			pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);


		CVec3Dfp32 vLight(-0.4f, -0.6f, 1.0f);
		vLight.Normalize();

		CVec3Dfp32 Normals[MaxVerts];
		CVec3Dfp32 *pNormals = Normals;
//		if(pSurf && pF->m_pSurface->m_Flags & XW_SURFFLAGS_NEEDNORMALS)
		{
			pNormals = _pVBM->Alloc_V3(nV);
			if(!pNormals)
				return;
			pChain->m_pN = pNormals;
			EvalNormal(_iFace, nV, ST, pNormals, true);
		}
		
		if(_iType & OGIER_RENDER_PERPIXEL)
		{
			CVec3Dfp32 *pTangU = _pVBM->Alloc_V3(nV);
			CVec3Dfp32 *pTangV = _pVBM->Alloc_V3(nV);
			if(!pTangU || !pTangV)
				return;
			EvalTangentSpaceArrays(_iFace, nV, ST, pNormals, pTangU, pTangV);

			pVB->Geometry_TVertexArray(pTangU, 1);
			pVB->Geometry_TVertexArray(pTangU, 2);
			pVB->Geometry_TVertexArray(pTangV, 3);
			for(int i = 0; i < nV; i++)
			{
				pTangU[i].MultiplyMatrix3x3(L2V);
				pTangV[i].MultiplyMatrix3x3(L2V);
			}
		}
		
//		else if(_iType & OGIER_RENDER_SHADE)
//			EvalNormal(_iFace, nV, ST, pNormals, true);

		// Need cheap lighting?
		CPixel32 *pC = _pVBM->Alloc_CPixel32(nV);
		if(!pC)
			return;

		int v;
		CPixel32 Col = _Col * pF->m_Color;
		if((int(Col) & 0xfcfcfcfc) == 0xfcfcfcfc)
			Col = 0xffffffff;
		bool bTrans = (Col.GetA() & 0xfc) != 0xfc;
		for(v = 0; v < nV; v++)
		{
			pC[v] = Col;
			if(Col != -1 && _iType & OGIER_RENDER_PERPIXEL)
				pVB->m_Flags |= CXR_VBFLAGS_OGR_SELECTED;

			if(COgrRenderBuffer::ShouldShade(pSurf, _iType))
			{
				CVec3Dfp32 WNormal = pNormals[v];
				WNormal.MultiplyMatrix3x3(_WMat);
				fp32 dotp = vLight * WNormal;
				fp32 i = 0.60f + dotp*0.40f;
				pC[v].R() *= i;
				pC[v].G() *= i;
				pC[v].B() *= i;
			}
		}

		pVB->Geometry_ColorArray(pC);

		int Size = (nS - 1) * (nT * 2 + 2) + 1;
		uint16 *pPrim = _pVBM->Alloc_Int16(Size);
		if(!pPrim)
			return;

		int iP = 0;
		for(int s = 0; s < nS-1; s++)
		{
			pPrim[iP++] = CRC_RIP_TRISTRIP + ((nT*2+2) << 8);
			pPrim[iP++] = nT*2;

			for(int t = 0; t < nT; t++)
			{
				pPrim[iP++] = (s+1)*nT + t;
				pPrim[iP++] = s*nT + t;
			}
		}

		pPrim[iP++] = CRC_RIP_END + (1 << 8);
		pVB->Render_IndexedPrimitives(pPrim, iP);

		if(pSurf)
		{
			CVec2Dfp32 *pTV = _pVBM->Alloc_V2(nV);
			if(!pTV)
				return;

			pF->EvalUV(nV, pV, ST, pTV);

			for(int v = 0; v < nV; v++)
			{
				pTV[v].k[0] *= pF->m_LightMapping[0][0];
				pTV[v].k[1] *= pF->m_LightMapping[0][1];
			}

			pVB->Geometry_TVertexArray(pTV, 0);

		}

		if(pChain->m_pN)
		{
			for(int i = 0; i < nV; i++)
				pChain->m_pN[i].MultiplyMatrix3x3(L2V);
		}
		
		for(v = 0; v < nV; v++) 
			pV[v] *= L2V;

		_RenderBuffer.OgrAddVB(_pRC, _pVBM, pVB, bTrans, m_BoundPos, CVec3Dfp32::GetMatrixRow(L2VInv, 3), _AnimTime, pSurf);
	}
	else
	{
		const int MaxVerts = 1024;
		CVec3Dfp32 Verts[MaxVerts];
		uint16 Prim[256];
	
		nS = (Face_IsFlatS(_iFace)) ? 2 : 9;
		nT = (Face_IsFlatT(_iFace)) ? 2 : 9;
		nS = Min((pE0->m_Tess > 0) ? pE0->m_Tess+1 : nS, (pE2->m_Tess > 0) ? pE2->m_Tess+1 : nS);
		nT = Min((pE1->m_Tess > 0) ? pE1->m_Tess+1 : nT, (pE3->m_Tess > 0) ? pE3->m_Tess+1 : nT);
		nV = nS*nT;
		if (nV > MaxVerts) return;

		if (nT*2+6 > 256) return;	// Overflow!, can't build trianglestrip under this condition.

		Tesselate(_iFace, nS, nT, 1.0f/fp32(nS-1), 1.0f/fp32(nT-1), nV, Verts, ST);
		for(int v = 0; v < nV; v++)
			Verts[v] *= L2V;

		_pRC->Geometry_VertexArray(Verts, nV, true);
		_pRC->Geometry_Color(_Col);

		int s;
		int iP = 0;
		for(s = 0; s < nS; s++)
			for(int t = 0; t < nT-1; t++)
			{
				int iV = s*nT + t;
				Prim[iP++] = iV;
				Prim[iP++] = iV + 1;
			}
		_pRC->Render_IndexedWires(Prim, iP);

		iP = 0;
		for(s = 0; s < nS-1; s++)
			for(int t = 0; t < nT; t++)
			{
				int iV = s*nT + t;
				Prim[iP++] = iV;
				Prim[iP++] = iV + nT;
			}
		_pRC->Render_IndexedWires(Prim, iP);

		_pRC->Geometry_Clear();
	}
}
#endif

void CSplineBrush::InternalRenderEdge(CRenderContext* _pRC, int _iEdge, CPixel32 _Col)
{
	MAUTOSTRIP(CSplineBrush_InternalRenderEdge, MAUTOSTRIP_VOID);
	CSplineBrush_Edge* pE = &m_lEdges[_iEdge];

	fp32 fnS = m_lEdgeTess[_iEdge];
	int nS = int(M_Floor(fnS - CSB_EPSILON)) + 1;
	fp32 sStep1 = 1.0f/fnS;
	nS++;

	CVec3Dfp32 Verts[128];
	if (nS > 128) return;

	fp32 fS = 0;
	for(int s = 0; s < nS; s++)
	{
		Verts[s] = 0;
		pE->AddEdge(Verts[s], fS, 1.0f);
		fS += sStep1;
		if (fS > 1.0f) fS = 1.0f;
	}

	_pRC->Render_WireStrip(Verts, g_IndexRamp16, nS, _Col);
}


void CSplineBrush::RenderEdges(CRenderContext* _pRC, CMat4Dfp32& _WMat, CMat4Dfp32& _VMat, CPixel32 _Col)
{
	MAUTOSTRIP(CSplineBrush_RenderEdges, MAUTOSTRIP_VOID);
	CMat4Dfp32 m_CurL2VMat;
	_WMat.Multiply(_VMat, m_CurL2VMat);
	CVec3Dfp32 Pos = m_BoundPos;
	Pos *= m_CurL2VMat;
	if (!_pRC->Viewport_Get()->SphereInView(Pos, m_BoundRadius)) return;

	CMat4Dfp32 VMatInv;
	m_CurL2VMat.InverseOrthogonal(VMatInv);
	CVec3Dfp32 LocalVP;
	LocalVP = CVec3Dfp32::GetRow(VMatInv, 3);

	SetAllFacesVisible();
	InitTesselation(LocalVP);

	_pRC->Attrib_Push();
	_pRC->Matrix_PushMultiply(m_CurL2VMat);
	for(int i = 0; i < m_lEdges.Len(); i++)
		InternalRenderEdge(_pRC, i, _Col);
	_pRC->Matrix_Pop();
	_pRC->Attrib_Pop();
}

#ifndef PLATFORM_CONSOLE
void CSplineBrush::Render(CRenderContext* _pRC, CXR_VBManager *_pVBM, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, CPixel32 _Col, int _iType, CMTime _AnimTime, COgrRenderBuffer &_RenderBuffer)
{
	MAUTOSTRIP(CSplineBrush_Render, MAUTOSTRIP_VOID);
	MSCOPE(CSplineBrush::Render, OGIER);
	if(!_pVBM)
		return;

	CMat4Dfp32 L2V;
	_WMat.Multiply(_VMat, L2V);
	CVec3Dfp32 Pos = m_BoundPos;
	Pos *= L2V;
	if(!_pRC->Viewport_Get()->SphereInView(Pos, m_BoundRadius))
		return;

	CMat4Dfp32 VMatInv;
	L2V.InverseOrthogonal(VMatInv);
	CVec3Dfp32 LocalVP;
	LocalVP = CVec3Dfp32::GetRow(VMatInv, 3);

	if(!(_iType & OGIER_RENDER_WIRE))
	{
		SetAllFacesVisible();
		Ogr_InitTesselation(LocalVP);
	}

	for(int f = 0; f < m_lFaces.Len(); f++)
		InternalRenderFace(_pRC, _pVBM, _WMat, _VMat, f, _Col, _iType, _AnimTime, _RenderBuffer);
}
#endif

// -------------------------------------------------------------------
void CSplineBrush::BuildTestSurfaces()
{
	MAUTOSTRIP(CSplineBrush_BuildTestSurfaces, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("-", "No texture-context.");
/*
{
	"classname" "brush"
	"XMPstoneflags" "0"
		( -48 112 128 ) ( -48 112 0 ) ( -16 112 0 ) rock_wall3 0 0 0 0.500000 0.500000
		( -16 80 128 ) ( -16 80 0 ) ( -48 80 0 ) rock_wall3 0 0 0 0.500000 0.500000
		( -48 80 128 ) ( -48 80 0 ) ( -48 112 0 ) rock_wall3 0 0 0 0.500000 0.500000
		( -16 112 128 ) ( -16 112 0 ) ( -16 80 0 ) rock_wall3 0 0 0 0.500000 0.500000
		( -16 112 0 ) ( -48 112 0 ) ( -48 80 0 ) rock_wall3 0 0 0 0.500000 0.500000
		( -16 80 128 ) ( -48 80 128 ) ( -48 112 128 ) rock_wall3 0 0 0 0.500000 0.500000

	"classname "curvebrush"
	"spline_vertices" "-48, 112, 128, -48, 112, 128, -48, 112, 128, -48, 112, 128, -48 112 128 ) ( -48 112 128 ) ( -48 112 128 ) ( -48 112 128 )"
	"spline_edge0" "0, 1, -48, 112, 128"
	"spline_edge1" "1, 2, -48, 112, 128"
	"spline_edge2" "2, 3, -48, 112, 128"
	"spline_edge3" "3, 0, -48, 112, 128"
	"spline_quad0" "0, 1, 2, 3, rock_wall3, 0, 0, 0, 0.500000, 0.500000"
}

*/	
/*	m_lVertices.Add(CVec3Dfp32(-16, -16, -16));
	m_lVertices.Add(CVec3Dfp32(16, -16, -8));
	m_lVertices.Add(CVec3Dfp32(32, 16, -16));
	m_lVertices.Add(CVec3Dfp32(-16, 16, -16));
	m_lVertices.Add(CVec3Dfp32(-16, -24, 16));
	m_lVertices.Add(CVec3Dfp32(24, -16, 16));
	m_lVertices.Add(CVec3Dfp32(16, 16, 32));
	m_lVertices.Add(CVec3Dfp32(-16, 24, 8));

	AddEdge(0, 1, CVec3Dfp32(0,0,64));
	AddEdge(1, 2, CVec3Dfp32(0,0,-64));
	AddEdge(2, 3, CVec3Dfp32(-16,64,0));
	AddEdge(3, 0, CVec3Dfp32(-16,0,32));
	AddEdge(4, 5, CVec3Dfp32(0,32,0));
	AddEdge(5, 6, CVec3Dfp32(-16,0,0));
	AddEdge(6, 7, CVec3Dfp32(0,0,32));
	AddEdge(7, 4, CVec3Dfp32(0,32,0));
	AddEdge(0, 4, CVec3Dfp32(32,0,32));
	AddEdge(1, 5, CVec3Dfp32(0,64,0));
	AddEdge(2, 6, CVec3Dfp32(-32,0,0));
	AddEdge(3, 7, CVec3Dfp32(0,0,64));
*/
/*	m_lVertices.Add(CVec3Dfp32(-16, -16, -32));
	m_lVertices.Add(CVec3Dfp32(16, -16, -32));
	m_lVertices.Add(CVec3Dfp32(16, 16, -32));
	m_lVertices.Add(CVec3Dfp32(-16, 16, -32));
	m_lVertices.Add(CVec3Dfp32(-16, -16, 32));
	m_lVertices.Add(CVec3Dfp32(16, -16, 32));
	m_lVertices.Add(CVec3Dfp32(16, 16, 16));
	m_lVertices.Add(CVec3Dfp32(-16, 16, 32));

	AddEdge(0, 1, CVec3Dfp32(0,-56,0));
	AddEdge(1, 2, CVec3Dfp32(56,0,0));
	AddEdge(2, 3, CVec3Dfp32(0,56,0));
	AddEdge(3, 0, CVec3Dfp32(-56,0,0));
	AddEdge(4, 5, CVec3Dfp32(0,-56,0));
	AddEdge(5, 6, CVec3Dfp32(56,0,-28));
	AddEdge(6, 7, CVec3Dfp32(0,56,-28));
	AddEdge(7, 4, CVec3Dfp32(-56,0,0));
	AddEdge(0, 4, CVec3Dfp32(0,0,0));
	AddEdge(1, 5, CVec3Dfp32(0,0,0));
	AddEdge(2, 6, CVec3Dfp32(0,0,0));
	AddEdge(3, 7, CVec3Dfp32(0,0,0));
*/
/*
	m_lVertices.Add(CVec3Dfp32(16, 0, 0));
	m_lVertices.Add(CVec3Dfp32(32, 16, 0));
	m_lVertices.Add(CVec3Dfp32(48, 0, 0));
	m_lVertices.Add(CVec3Dfp32(32, -16, 0));
	m_lVertices.Add(CVec3Dfp32(0, 0, 16));
	m_lVertices.Add(CVec3Dfp32(0, 16, 32));
	m_lVertices.Add(CVec3Dfp32(0, 0, 48));
	m_lVertices.Add(CVec3Dfp32(0, -16, 32));

	AddEdge(0, 1, CVec3Dfp32(-30,30,0));
	AddEdge(1, 2, CVec3Dfp32(30,30,0));
	AddEdge(2, 3, CVec3Dfp32(30,-30,0));
	AddEdge(3, 0, CVec3Dfp32(-30,-30,0));
	AddEdge(4, 5, CVec3Dfp32(0,30,-30));
	AddEdge(5, 6, CVec3Dfp32(0,30,30));
	AddEdge(6, 7, CVec3Dfp32(0,-30,30));
	AddEdge(7, 4, CVec3Dfp32(0,-30,-30));
	AddEdge(0, 4, CVec3Dfp32(30,0,30));
	AddEdge(1, 5, CVec3Dfp32(60,0,60));
	AddEdge(2, 6, CVec3Dfp32(90,0,90));
	AddEdge(3, 7, CVec3Dfp32(60,0,60));

	AddQuad(0,1,2,3);
	AddQuad(4,5,6,7);
	AddQuad(0x0000, 0x0009, 0x8004, 0x8008);
	AddQuad(0x0001, 0x000a, 0x8005, 0x8009);
	AddQuad(0x0002, 0x000b, 0x8006, 0x800a);
	AddQuad(0x0003, 0x0008, 0x8007, 0x800b);

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("-", "No texture-context.");
	m_lFaces[0].m_TextureID = pTC->GetTextureID("ROCK_WALL21");
	m_lFaces[1].m_TextureID = pTC->GetTextureID("MARB_FLR4S");
	m_lFaces[2].m_TextureID = pTC->GetTextureID("MARB_FLR2");
	m_lFaces[3].m_TextureID = pTC->GetTextureID("MARB_FLR4");
	m_lFaces[4].m_TextureID = pTC->GetTextureID("MARBLE1");
	m_lFaces[5].m_TextureID = pTC->GetTextureID("WHITE_MARBLE");
*/
	fp32 r1 = 32;
	fp32 r2 = 12;
	int nSeg = 3;
	for(int i = 0; i < nSeg; i++)
	{
		CVec3Dfp32 rv(0);
		rv.k[2] = fp32(i)/fp32(nSeg);
//		rv.k[0] = 0.25f;
		CMat4Dfp32 Mat; Mat.Unit();
		rv.CreateMatrixFromAngles(0, Mat);

		int iv = m_lVertices.Add(CVec3Dfp32(r1-r2, 0, 0)*Mat);
		m_lVertices.Add(CVec3Dfp32(r1, 0, r2)*Mat);
		m_lVertices.Add(CVec3Dfp32(r1+r2, 0, 0)*Mat);
		m_lVertices.Add(CVec3Dfp32(r1, 0, -r2)*Mat);
		m_lVertices.Add(CVec3Dfp32(0, r1-r2, 0)*Mat);
		m_lVertices.Add(CVec3Dfp32(0, r1, r2)*Mat);
		m_lVertices.Add(CVec3Dfp32(0, r1+r2, 0)*Mat);
		m_lVertices.Add(CVec3Dfp32(0, r1, -r2)*Mat);

		for(int v = iv; v < m_lVertices.Len(); v++)
			m_lVertices[v] += CVec3Dfp32(Random*r2, Random*r2, Random*r2)*1.5f;

		fp32 k2 = r2/24.0f*45.0f;
		int ie = AddEdge(iv+0, iv+1, CVec3Dfp32(-k2,0,k2)*Mat);
		AddEdge(iv+1, iv+2, CVec3Dfp32(k2,0,k2)*Mat);
		AddEdge(iv+2, iv+3, CVec3Dfp32(k2,0,-k2)*Mat);
		AddEdge(iv+3, iv+0, CVec3Dfp32(-k2,0,-k2)*Mat);

		AddEdge(iv+4, iv+5, CVec3Dfp32(0,-k2,k2)*Mat);
		AddEdge(iv+5, iv+6, CVec3Dfp32(0,k2,k2)*Mat);
		AddEdge(iv+6, iv+7, CVec3Dfp32(0,k2,-k2)*Mat);
		AddEdge(iv+7, iv+4, CVec3Dfp32(0,-k2,-k2)*Mat);

		fp32 k = 1.65f;
		AddEdge(iv+0, iv+4, CVec3Dfp32(k*30,k*30,0)*Mat);
		AddEdge(iv+1, iv+5, CVec3Dfp32(k*60,k*60,0)*Mat);
		AddEdge(iv+2, iv+6, CVec3Dfp32(k*90,k*90,0)*Mat);
		AddEdge(iv+3, iv+7, CVec3Dfp32(k*60,k*60,0)*Mat);

		int ifc = AddQuad(ie+0,ie+1,ie+2,ie+3);
		AddQuad(ie+4,ie+5,ie+6,ie+7);
		AddQuad(ie+0x0000, ie+0x0009, ie+0x8004, ie+0x8008);
		AddQuad(ie+0x0001, ie+0x000a, ie+0x8005, ie+0x8009);
		AddQuad(ie+0x0002, ie+0x000b, ie+0x8006, ie+0x800a);
		AddQuad(ie+0x0003, ie+0x0008, ie+0x8007, ie+0x800b);

		m_lFaces[ifc+0].m_TextureID = pTC->GetTextureID("ROCK_WALL21");
		m_lFaces[ifc+1].m_TextureID = pTC->GetTextureID("MARB_FLR4S");
		m_lFaces[ifc+2].m_TextureID = pTC->GetTextureID("MARB_FLR2");
		m_lFaces[ifc+3].m_TextureID = pTC->GetTextureID("MARB_FLR4");
		m_lFaces[ifc+4].m_TextureID = pTC->GetTextureID("MARBLE1");
		m_lFaces[ifc+5].m_TextureID = pTC->GetTextureID("WHITE_MARBLE");
	}

/*	AddQuad(0,1,2,3);
	AddQuad(0,1,2,3);
	AddQuad(0,1,2,3);*/

//	m_lEdges.Add(CSplineBrush_Edge(64.0f, CVec3Dfp32(-32, -32, 0), CVec3Dfp32(64, 0, 128), CVec3Dfp32(0, 0, -128)) );


/*	m_lEdges.Add(CSplineBrush_Edge(64.0f, CVec3Dfp32(-32, -32, 0), CVec3Dfp32(64, 0, 128), CVec3Dfp32(0, 0, -128)) );
	m_lEdges.Add(CSplineBrush_Edge(64.0f, CVec3Dfp32(-32, 32, 0), CVec3Dfp32(64, 0, 0), CVec3Dfp32(0, 0, 0)) );
	m_lEdges.Add(CSplineBrush_Edge(64.0f, CVec3Dfp32(32, 32, 0), CVec3Dfp32(64, -64, -128), CVec3Dfp32(64, 0, 128)) );
	m_lEdges.Add(CSplineBrush_Edge(64.0f, CVec3Dfp32(-32, 32, 0), CVec3Dfp32(-64, -64, -64), CVec3Dfp32(-64, 0, 64)) );
*/
/*	m_lEdges.Add(CSplineBrush_Edge(64.0f, CVec3Dfp32(-32, -32, 0), CVec3Dfp32(64, 0, 128), CVec3Dfp32(0, 0, -128)) );
	m_lEdges.Add(CSplineBrush_Edge(64.0f, CVec3Dfp32(-32, 32, 0), CVec3Dfp32(64, 0, 264), CVec3Dfp32(0, 0, -264)) );
	m_lEdges.Add(CSplineBrush_Edge(64.0f, CVec3Dfp32(32, 32, 0), CVec3Dfp32(64, -64, -128), CVec3Dfp32(0, 0, 128)) );
	m_lEdges.Add(CSplineBrush_Edge(64.0f, CVec3Dfp32(-32, 32, 0), CVec3Dfp32(-64, -64, -264), CVec3Dfp32(0, 0, 264)) );
*/
//	m_lEdges.Add(CSplineBrush_Edge(64.0f, CVec3Dfp32(32, 32, 0), CVec3Dfp32(-64, 0, 0), CVec3Dfp32(0, 0, 0)) );
//	m_lEdges.Add(CSplineBrush_Edge(64.0f, CVec3Dfp32(-32, 32, 0), CVec3Dfp32(0, -64, -64), CVec3Dfp32(0, 0, 64)) );

/*	m_liEdges.Add((uint32)0);
	m_liEdges.Add(1);
	m_liEdges.Add(2);
	m_liEdges.Add(3);
*/
//	m_lFaces.Add(CSplineBrush_Face(0, 4));
}


TPtr<CSplineBrush> CSplineBrush::Duplicate() const
{
	TPtr<CSplineBrush> spNew = MNew(CSplineBrush);
	if(!spNew) MemError("CSplineBrush::Duplicate");
	m_lVertices.Duplicate(&spNew->m_lVertices);
	m_lEdges.Duplicate(&spNew->m_lEdges);
	m_liEdges.Duplicate(&spNew->m_liEdges);
	spNew->m_lCullPlanes.Add(m_lCullPlanes);
	m_lFaces.Duplicate(&spNew->m_lFaces);
	m_lFaceSurf.Duplicate(&spNew->m_lFaceSurf);
	spNew->m_lEdgeTess.Add(m_lEdgeTess);
	spNew->m_lEdgeMinTess.Add(m_lEdgeMinTess);
	m_lBoundNodes.Duplicate(&spNew->m_lBoundNodes);
	spNew->m_Flags	= m_Flags;
	spNew->m_LastViewRendered	= m_LastViewRendered;
	spNew->m_DynLightMask	= m_DynLightMask;
	spNew->m_BoundVertexBox	= m_BoundVertexBox;
	spNew->m_BoundBox	= m_BoundBox;
	spNew->m_BoundPos	= m_BoundPos;
	spNew->m_BoundRadius	= m_BoundRadius;
	spNew->m_MinTessDistance	= m_MinTessDistance;

	return spNew;
}
