// -------------------------------------------------------------------
#include "PCH.h"
#include "MRender.h"
#include "MRCCore.h"
#include "../MSystem.h"
#include "../../XR/XRVBContext.h"		// Hmm...  move it to MSystem/Raster/ ???

// -------------------------------------------------------------------
#if !defined(COMPILER_CODEWARRIOR) && !defined(COMPILER_GNU)
#pragma warning(disable : 4244)		// Slår av varning för float = int, int = float.
#endif

// -------------------------------------------------------------------
//  CRC_ClipVolume
// -------------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(CRC_ClipVolume);


CRC_ClipVolume::CRC_ClipVolume()
{
	MAUTOSTRIP(CRC_ClipVolume_ctor, MAUTOSTRIP_VOID);
	m_nPlanes = 0;
}

void CRC_ClipVolume::Init(const CVec3Dfp4& _POV)
{
	MAUTOSTRIP(CRC_ClipVolume_Init, MAUTOSTRIP_VOID);
	m_nPlanes = 0;
	m_POV = _POV;
}

void CRC_ClipVolume::GetVertices(const CVec3Dfp4* _pVertices, int _nVertices, const CVec3Dfp4& _POV, bool _bFlipDirection)
{
	MAUTOSTRIP(CRC_ClipVolume_GetVertices, MAUTOSTRIP_VOID);
	if (_nVertices < 3) 
	{
		m_nPlanes = 0;
		return;
	}
	m_nPlanes = _nVertices;
	m_POV = _POV;

	if (_bFlipDirection)
		for(int v = 0; v < _nVertices; v++)
			m_Vertices[v] = _pVertices[_nVertices - v - 1];
	else
		for(int v = 0; v < _nVertices; v++)
			m_Vertices[v] = _pVertices[v];

}

void CRC_ClipVolume::CreateFromVertices()
{
	MAUTOSTRIP(CRC_ClipVolume_CreateFromVertices, MAUTOSTRIP_VOID);
	Error_static("CreateFromVertices", "This should not be used.");

	const fp4 MinLen = 2048.0f*1.4f;
/*
	for(int v = 0; v < m_nPlanes; v++)
	{
		fp4 dvx = m_Vertices[v].k[0] - m_POV.k[0];
		fp4 dvy = m_Vertices[v].k[1] - m_POV.k[1];
		fp4 dvz = m_Vertices[v].k[2] - m_POV.k[2];
		fp4 l = sqrt(Sqr(dvx) + Sqr(dvy) + Sqr(dvz)) + 0.01f;
//		if (l < MinLen)
		{
			fp4 Scale = MinLen / l;
			m_Vertices[v].k[0] = dvx*Scale + m_POV.k[0];
			m_Vertices[v].k[1] = dvy*Scale + m_POV.k[1];
			m_Vertices[v].k[2] = dvz*Scale + m_POV.k[2];
		}
	}
*/
/*	for(int p = 0; p < m_nPlanes-1; p++)
		m_Planes[p].Create(m_POV, m_Vertices[p+1], m_Vertices[p]);

	m_Planes[m_nPlanes-1].Create(m_POV, m_Vertices[0], m_Vertices[m_nPlanes-1]);*/

	CVec3Dfp4* pV2 = &m_Vertices[0];
	fp4 l2inv, v2x, v2y, v2z;
	{
		v2x = pV2->k[0] - m_POV.k[0];
		v2y = pV2->k[1] - m_POV.k[1];
		v2z = pV2->k[2] - m_POV.k[2];

//		fp4 l2 = FastSqrt32(Sqr(v2x) + Sqr(v2y) + Sqr(v2z));
		fp4 l2 = M_Sqrt(Sqr(v2x) + Sqr(v2y) + Sqr(v2z));
//		if (!l2) l2 += 0.000001f;
		l2inv = 1.0f / l2;
	}
	fp4 v0LenInv = l2inv;

	for(int v = m_nPlanes-1; v >= 0; v--)
	{
//LogFile("1." + m_Planes[v].n.GetString() + CStrF(", %f", m_Planes[v].d));

		CVec3Dfp4* pV1 = &m_Vertices[v];
		fp4 v1x = pV1->k[0] - m_POV.k[0];
		fp4 v1y = pV1->k[1] - m_POV.k[1];
		fp4 v1z = pV1->k[2] - m_POV.k[2];

//		fp4 l1 = FastSqrt32(Sqr(v2x) + Sqr(v2y) + Sqr(v2z));
//		if (!l1) l1 += 0.000001f;
		fp4 l1inv = (!v) ? v0LenInv : ( M_InvSqrt(Sqr(v1x) + Sqr(v1y) + Sqr(v1z)) );

		fp4 Scale = MinLen * l1inv;
		pV1->k[0] = v1x*Scale + m_POV.k[0];
		pV1->k[1] = v1y*Scale + m_POV.k[1];
		pV1->k[2] = v1z*Scale + m_POV.k[2];
//ConOut(CStrF("%d, %s", v, (char*) pV1->GetString()));

		fp4 ns = -1.0f * l1inv * l2inv;
		m_Planes[v].n.k[0] = (v1y*v2z - v1z*v2y) * ns;
		m_Planes[v].n.k[1] = (-v1x*v2z + v1z*v2x) * ns;
		m_Planes[v].n.k[2] = (v1x*v2y - v1y*v2x) * ns;
		m_Planes[v].d = -m_Planes[v].n * m_POV;

//LogFile("2." + m_Planes[v].n.GetString() + CStrF(", %f", m_Planes[v].d));
//		pV2 = pV1;
		l2inv = l1inv;
		v2x = v1x;
		v2y = v1y;
		v2z = v1z;
	}
}

void CRC_ClipVolume::CreateFromVertices3(const CPlane3Dfp4& _ProjPlane)
{
	MAUTOSTRIP(CRC_ClipVolume_CreateFromVertices3, MAUTOSTRIP_VOID);
	CVec3Dfp4* pV2 = &m_Vertices[0];
	fp4 l2inv, v2x, v2y, v2z;
	{
		v2x = pV2->k[0] - m_POV.k[0];
		v2y = pV2->k[1] - m_POV.k[1];
		v2z = pV2->k[2] - m_POV.k[2];
//		fp4 l2 = sqrt(Sqr(v2x) + Sqr(v2y) + Sqr(v2z));
		l2inv = M_InvSqrt( Sqr(v2x) + Sqr(v2y) + Sqr(v2z) );
	}
	fp4 v0LenInv = l2inv;

	fp4 dot1 = -_ProjPlane.Distance(m_POV);

	for(int v = m_nPlanes-1; v >= 0; v--)
	{
//LogFile("1." + m_Planes[v].n.GetString() + CStrF(", %f", m_Planes[v].d));

		CVec3Dfp4* pV1 = &m_Vertices[v];
		fp4 v1x = pV1->k[0] - m_POV.k[0];
		fp4 v1y = pV1->k[1] - m_POV.k[1];
		fp4 v1z = pV1->k[2] - m_POV.k[2];

//		fp4 l1 = FastSqrt32(Sqr(v2x) + Sqr(v2y) + Sqr(v2z));
//		if (!l1) l1 += 0.000001f;
		fp4 l1inv = (!v) ? v0LenInv : M_InvSqrt( Sqr(v1x) + Sqr(v1y) + Sqr(v1z) );
//		fp4 l1inv = (!v) ? v0LenInv : (1.0f / sqrt(Sqr(v1x) + Sqr(v1y) + Sqr(v1z)) );

		fp4 dot2 = v1x*_ProjPlane.n.k[0] + v1y*_ProjPlane.n.k[1] + v1z*_ProjPlane.n.k[2];
//		fp4 Scale = dot1 * dot2 * Sqr(l1inv);
		fp4 Scale = dot1 / dot2;
		pV1->k[0] = v1x*Scale + m_POV.k[0];
		pV1->k[1] = v1y*Scale + m_POV.k[1];
		pV1->k[2] = v1z*Scale + m_POV.k[2];
//	ConOut(CStrF("%d, %s", v, (char*) pV1->GetString() ));

		fp4 ns = -1.0f * l1inv * l2inv;
		m_Planes[v].n.k[0] = (v1y*v2z - v1z*v2y) * ns;
		m_Planes[v].n.k[1] = (-v1x*v2z + v1z*v2x) * ns;
		m_Planes[v].n.k[2] = (v1x*v2y - v1y*v2x) * ns;
		m_Planes[v].d = -m_Planes[v].n * m_POV;

//LogFile("2." + m_Planes[v].n.GetString() + CStrF(", %f", m_Planes[v].d));
//		pV2 = pV1;
		l2inv = l1inv;
		v2x = v1x;
		v2y = v1y;
		v2z = v1z;
	}
}

void CRC_ClipVolume::CreateFromVertices2()
{
	MAUTOSTRIP(CRC_ClipVolume_CreateFromVertices2, MAUTOSTRIP_VOID);
	CVec3Dfp4* pV2 = &m_Vertices[0];
	fp4 v2x = pV2->k[0] - m_POV.k[0];
	fp4 v2y = pV2->k[1] - m_POV.k[1];
	fp4 v2z = pV2->k[2] - m_POV.k[2];
	fp4 v0LenInv = M_InvSqrt(Sqr(v2x) + Sqr(v2y) + Sqr(v2z));
	fp4 l2inv = v0LenInv;

	for(int v = m_nPlanes-1; v >= 0; v--)
	{
		CVec3Dfp4* pV1 = &m_Vertices[v];
		fp4 v1x = pV1->k[0] - m_POV.k[0];
		fp4 v1y = pV1->k[1] - m_POV.k[1];
		fp4 v1z = pV1->k[2] - m_POV.k[2];
		fp4 l1inv = (!v) ? v0LenInv : M_InvSqrt(Sqr(v1x) + Sqr(v1y) + Sqr(v1z));

		fp4 ns = -l1inv * l2inv;
		m_Planes[v].n.k[0] = ns * (v1y*v2z - v1z*v2y);
		m_Planes[v].n.k[1] = ns * (-v1x*v2z + v1z*v2x);
		m_Planes[v].n.k[2] = ns * (v1x*v2y - v1y*v2x);
		m_Planes[v].d = -m_Planes[v].n * m_POV;
		v2x = v1x;
		v2y = v1y;
		v2z = v1z;
		l2inv = l1inv;
	}
}


void CRC_ClipVolume::Invert()
{
	MAUTOSTRIP(CRC_ClipVolume_Invert, MAUTOSTRIP_VOID);
	CRC_ClipVolume Tmp;
	Tmp.Copy(*this);

	for(int i = 0; i < m_nPlanes; i++)
	{
		int iDst = m_nPlanes-1-i;
		m_Vertices[iDst] = Tmp.m_Vertices[i];
		m_Planes[iDst] = Tmp.m_Planes[i];
		m_Planes[iDst].Inverse();
	}
}

void CRC_ClipVolume::Transform(const CMat43fp4& _Mat)
{
	MAUTOSTRIP(CRC_ClipVolume_Transform, MAUTOSTRIP_VOID);
	m_POV *= _Mat;

	for(int i = 0; i < m_nPlanes; i++)
	{
		m_Vertices[i] *= _Mat;
		m_Planes[i].Transform(_Mat);
	}
}

void CRC_ClipVolume::Copy(const CRC_ClipVolume& _SrcClip)
{
	MAUTOSTRIP(CRC_ClipVolume_Copy, MAUTOSTRIP_VOID);
	m_nPlanes = _SrcClip.m_nPlanes;
	m_POV = _SrcClip.m_POV;
	for(int i = 0; i < m_nPlanes; i++)
	{
		m_Vertices[i] = _SrcClip.m_Vertices[i];
		m_Planes[i] = _SrcClip.m_Planes[i];
	}
}

#define MACRO_ISMIRRORED(Mat)	\
	((CVec3Dfp4::GetMatrixRow(Mat, 0) / CVec3Dfp4::GetMatrixRow(Mat, 1)) * CVec3Dfp4::GetMatrixRow(Mat, 2) < 0.0f)


void CRC_ClipVolume::CopyAndTransform(const CRC_ClipVolume& _SrcClip, const CMat43fp4& _Mat)
{
	MAUTOSTRIP(CRC_ClipVolume_CopyAndTransform, MAUTOSTRIP_VOID);
	m_nPlanes = _SrcClip.m_nPlanes;
	m_POV = _SrcClip.m_POV;
	m_POV *= _Mat;

//	if (!MACRO_ISMIRRORED(_pMat))
	if(!_Mat.IsMirrored())
	{

//	CVec3Dfp4 Hirr = CVec3Dfp4::GetMatrixRow(_Mat, 0) / CVec3Dfp4::GetMatrixRow(_Mat, 1);
//	fp4 dotp = Hirr * CVec3Dfp4::GetMatrixRow(_Mat, 2);

//	ConOut(CStrF("IsMirrored %d, %d",((CMat4Dfp4&)_Mat).IsMirrored(), MACRO_ISMIRRORED(_Mat)));
		CVec3Dfp4::MultiplyMatrix(&_SrcClip.m_Vertices[0], &m_Vertices[0], _Mat, m_nPlanes);
		for(int i = 0; i < m_nPlanes; i++)
		{
			m_Planes[i] = _SrcClip.m_Planes[i];
			m_Planes[i].Transform(_Mat);
/*			CVec3Dfp4 p = m_Planes[i].GetPointInPlane();
			p *= _Mat;
			m_Planes[i].n.MultiplyMatrix3x3(_Mat);
			m_Planes[i].d = -(m_Planes[i].n*p);*/
		}
	}
	else
	{
		for(int i = 0; i < m_nPlanes; i++)
		{
			int iDst = m_nPlanes-1-i;
			m_Vertices[iDst] = _SrcClip.m_Vertices[i];
			m_Vertices[iDst] *= _Mat;
			m_Planes[iDst] = _SrcClip.m_Planes[i];
			m_Planes[iDst].Transform(_Mat);
			m_Planes[iDst].Inverse();
		}
	}
}

int CRC_ClipVolume::BoxInVolume(const CBox3Dfp4& _Box) const
{
	MAUTOSTRIP(CRC_ClipVolume_BoxInVolume, 0);
	CVec3Dfp4 Center;
	_Box.GetCenter(Center);
	for (int p = 0; p < m_nPlanes; p++)
	{
		fp4 d = m_Planes[p].Distance(Center);
		if (d > 0.0f)
			if (m_Planes[p].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) > 0.001f) return 0;
	}
	return 1;
}

int CRC_ClipVolume::SphereInVolume(const CVec3Dfp4& _v, fp4 _r) const
{
	MAUTOSTRIP(CRC_ClipVolume_SphereInVolume, 0);
	for(int ip = 0; ip < m_nPlanes; ip++)
		if (m_Planes[ip].Distance(_v) > _r) return 0;
	return 1;
}

// -------------------------------------------------------------------
//  CRC_Viewport
// -------------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(CRC_Viewport);


CRC_Viewport::CRC_Viewport()
{
	MAUTOSTRIP(CRC_Viewport_ctor, MAUTOSTRIP_VOID);
	m_Mode = VP_MODE_FOV;
	m_FrontPlane = CRC_DEFAULT_FRONTPLANE;
	m_BackPlane = CRC_DEFAULT_BACKPLANE;
	m_FOV = CRC_DEFAULT_FOV;
	m_FOVAspect = 4.0f/3.0f;
	m_Scale = 1.0f;
	m_AspectRatio = 1.0f;
	m_xScale = 1.0f;
	m_yScale = 1.0f;
	m_PixelAspect = CVec2Dfp4(1,1);
	m_Clip = CRct(CPnt(0,0), CPnt(1,1));
	m_Rect = CRct(CPnt(0,0), CPnt(1,1));
	m_bVPChanged = true;
};

CRC_Viewport::~CRC_Viewport()
{
	MAUTOSTRIP(CRC_Viewport_dtor, MAUTOSTRIP_VOID);
};

void CRC_Viewport::Update()
{
	MAUTOSTRIP(CRC_Viewport_Update, MAUTOSTRIP_VOID);
	if (!m_bVPChanged) return;
	m_bVPChanged = false;

	// Clamp FOV
	if (m_FOV > 179.0f) m_FOV = 179.0f;
	if (m_FOV < 0.1f) m_FOV = 0.1f;

	if (m_Mode == VP_MODE_FOV)
	{
		// Calc xScale and yScale from FOV.
		fp4 scale = 2.0f * M_InvSqrt(Sqr(1.0f/M_Cos(((2.0f*_PI* 0.5f)*m_FOV)/360.0f)) - 1.0f);
		if ((fp4)m_Rect.GetWidth()/(fp4)m_Rect.GetHeight() < m_FOVAspect)
			m_xScale = scale * m_FOVAspect / ( (fp4)m_Rect.GetWidth()/(fp4)m_Rect.GetHeight() );
		else
			m_xScale = scale * m_FOVAspect / ( (fp4)m_Rect.GetWidth()/(fp4)m_Rect.GetHeight() );

		m_Scale = m_xScale;
		m_xScale *= fp4(m_Rect.GetWidth())*0.5f;
	}
	else if (m_Mode == VP_MODE_SCALE)
	{
		// Explicit scale
		m_xScale = m_Scale;
		m_xScale *= fp4(m_Rect.GetWidth())*0.5f;
	}
	else
		Error_static("CRC_Viewport::Update", "Invalid mode.");
	
	m_yScale = m_xScale;//*m_AspectRatio;
	m_xScale /= m_AspectRatio;

	m_xScale *= m_PixelAspect.k[0];
	m_yScale *= m_PixelAspect.k[1];

	// Calc projection matrix...
	{
		fp4 Near = m_FrontPlane;
		fp4 Far = m_BackPlane;

		// Calc 2D-translation
		CRct crect(m_Rect);
		crect.Bound(m_Clip.clip);
		fp4 ox = (m_Rect.GetWidth() - crect.GetWidth());
		fp4 oy = -(m_Rect.GetHeight() - crect.GetHeight());
//		if ((m_Rect.p0.x - crect.p0.x) < 0) ox = -ox;
//		if ((m_Rect.p0.y - crect.p0.y) < 0) oy = -oy;
		if ((m_Rect.p0.x - crect.p0.x) < 0) ox += 2*(m_Rect.p0.x - crect.p0.x);
		if ((m_Rect.p0.y - crect.p0.y) < 0) oy -= 2*(m_Rect.p0.y - crect.p0.y);

		// And the winner is...
/*		Macro_Matrix_SetRow_Dest(m_PMat, 0, m_xScale, 0, 0, 0);
		Macro_Matrix_SetRow_Dest(m_PMat, 1, 0, -m_yScale, 0, 0);
		Macro_Matrix_SetRow_Dest(m_PMat, 2, ox, oy, 2*Far/(Far-Near), 2);
		Macro_Matrix_SetRow_Dest(m_PMat, 3, 0, 0, -2*Near*Far/(Far-Near), 0);*/
		
		Macro_Matrix_SetRow_Dest(m_PMat, 0, m_xScale/2.0f, 0, 0, 0);
		Macro_Matrix_SetRow_Dest(m_PMat, 1, 0, (-m_yScale)/2.0f, 0, 0);
		Macro_Matrix_SetRow_Dest(m_PMat, 2, ox/2.0f, oy/2.0f, Far/(Far-Near), 1.0f);
		Macro_Matrix_SetRow_Dest(m_PMat, 3, 0, 0, -Near*Far/(Far-Near), 0);
	}

	{
//		fp4 w = m_Rect.GetWidth();
//		fp4 h = m_Rect.GetHeight();

		// Viewport volume by planes, in view-space
		{
			CRct r(CPnt(0, 0), m_Rect.p1 - m_Rect.p0);
//			r = m_Rect;
//		LogFile(CStrF("r1 (%d, %d, %d, %d", r.p0.x, r.p0.y, r.p1.x, r.p1.y));

			int wh = r.GetWidth();
			int hh = r.GetHeight();
			r.p0.x = r.p0.x - wh/2;
			r.p0.y = r.p0.y - hh/2;
			r.p1.x = r.p1.x - wh/2;
			r.p1.y = r.p1.y - hh/2;
			r.p0.x *= 2;
			r.p0.y *= 2;
			r.p1.x *= 2;
			r.p1.y *= 2;

/*			r.p0 += r.p0;
			r.p1 += r.p1;
			r.p0 -= CPnt(w,h);
			r.p1 -= CPnt(w,h);*/
			CVec2Dfp4 p0(fp4(r.p0.x) / m_xScale, fp4(r.p0.y) / m_yScale);
			CVec2Dfp4 p1(fp4(r.p1.x) / m_xScale, fp4(r.p1.y) / m_yScale);
			m_VViewPlanes[0].CreateNV((CVec3Dfp4(p0.k[0], 0, 1) / CVec3Dfp4(0,1,0)).Normalize(), CVec3Dfp4(0));
			m_VViewPlanes[1].CreateNV((CVec3Dfp4(p1.k[0], 0, 1) / CVec3Dfp4(0,-1,0)).Normalize(), CVec3Dfp4(0));
			m_VViewPlanes[2].CreateNV((CVec3Dfp4(0, p0.k[1], 1) / CVec3Dfp4(-1,0,0)).Normalize(), CVec3Dfp4(0));
			m_VViewPlanes[3].CreateNV((CVec3Dfp4(0, p1.k[1], 1) / CVec3Dfp4(1,0,0)).Normalize(), CVec3Dfp4(0));
		}
/*		{
			m_VViewPlanes[0].Create((CVec3Dfp4(-w, 0, m_xScale) / CVec3Dfp4(0,1,0)).Normalize(), CVec3Dfp4(0));
			m_VViewPlanes[1].Create((CVec3Dfp4(w, 0, m_xScale) / CVec3Dfp4(0,-1,0)).Normalize(), CVec3Dfp4(0));
			m_VViewPlanes[2].Create((CVec3Dfp4(0, -h, m_yScale) / CVec3Dfp4(-1,0,0)).Normalize(), CVec3Dfp4(0));
			m_VViewPlanes[3].Create((CVec3Dfp4(0, h, m_yScale) / CVec3Dfp4(1,0,0)).Normalize(), CVec3Dfp4(0));
		}*/

		// Viewport volume by vertices, in view-space
		{
			m_nViewVertices = 5;
			m_VViewVertices[0] = 0;		// Eye is assumed to be vertex #0.

/*			fp4 xcomp = m_BackPlane*w / m_xScale;
			fp4 ycomp = m_BackPlane*h / m_yScale;
			m_VViewVertices[1] = CVec3Dfp4(xcomp, ycomp, m_BackPlane);
			m_VViewVertices[2] = CVec3Dfp4(-xcomp, ycomp, m_BackPlane);
			m_VViewVertices[3] = CVec3Dfp4(-xcomp, -ycomp, m_BackPlane);
			m_VViewVertices[4] = CVec3Dfp4(xcomp, -ycomp, m_BackPlane);
*/
			CRct r(CPnt(0, 0), m_Rect.p1 - m_Rect.p0);
//			r = m_Rect;

			int wh = r.GetWidth();
			int hh = r.GetHeight();
			r.p0.x = r.p0.x - wh/2;
			r.p0.y = r.p0.y - hh/2;
			r.p1.x = r.p1.x - wh/2;
			r.p1.y = r.p1.y - hh/2;
			r.p0.x *= 2;
			r.p0.y *= 2;
			r.p1.x *= 2;
			r.p1.y *= 2;

/*			r.p0 += r.p0;
			r.p1 += r.p1;
			r.p0 -= CPnt(w,h);
			r.p1 -= CPnt(w,h);*/
			fp4 bp = m_BackPlane - 100;
			fp4 x0 = bp*fp4(r.p0.x) / m_xScale;
			fp4 x1 = bp*fp4(r.p1.x) / m_xScale;
			fp4 y0 = bp*fp4(r.p0.y) / m_yScale;
			fp4 y1 = bp*fp4(r.p1.y) / m_yScale;
			m_VViewVertices[1] = CVec3Dfp4(x1, y1, bp);
			m_VViewVertices[2] = CVec3Dfp4(x0, y1, bp);
			m_VViewVertices[3] = CVec3Dfp4(x0, y0, bp);
			m_VViewVertices[4] = CVec3Dfp4(x1, y0, bp);
		}
	}

}

void CRC_Viewport::SetFOV(fp4 _FOV)
{
	MAUTOSTRIP(CRC_Viewport_SetFOV, MAUTOSTRIP_VOID);
	m_FOV = _FOV;
	m_Mode = VP_MODE_FOV;
	m_bVPChanged = true;
}

void CRC_Viewport::SetFOVAspect(fp4 _Aspect)
{
	MAUTOSTRIP(CRC_Viewport_SetFOVAspect, MAUTOSTRIP_VOID);
	m_FOVAspect = _Aspect;
	m_bVPChanged = true;
}

void CRC_Viewport::SetScale(fp4 _Scale)
{
	MAUTOSTRIP(CRC_Viewport_SetScale, MAUTOSTRIP_VOID);
	m_Scale = _Scale;
	m_Mode = VP_MODE_SCALE;
	m_bVPChanged = true;
}

void CRC_Viewport::SetAspectRatio(fp4 _Aspect)
{
	MAUTOSTRIP(CRC_Viewport_SetAspectRatio, MAUTOSTRIP_VOID);
	m_AspectRatio = _Aspect;
	m_bVPChanged = true;
}

void CRC_Viewport::SetPixelAspect(fp4 _AspectX, fp4 _AspectY)
{
	m_PixelAspect = CVec2Dfp4(_AspectX, _AspectY);
	m_bVPChanged = true;
}


void CRC_Viewport::SetFrontPlane(fp4 _FrontPlane)
{
	MAUTOSTRIP(CRC_Viewport_SetFrontPlane, MAUTOSTRIP_VOID);
	m_FrontPlane = _FrontPlane;
	m_bVPChanged = true;
}

void CRC_Viewport::SetBackPlane(fp4 _BackPlane)
{ 
	MAUTOSTRIP(CRC_Viewport_SetBackPlane, MAUTOSTRIP_VOID);

	//AR-HACK: Sometimes (due to client/server message glitch or something),
	//         the backplane get the value 0.0f - this hack works around that
	if (_BackPlane < 16.0f)
		return;

	m_BackPlane = _BackPlane; 
	m_bVPChanged = true;
}

void CRC_Viewport::SetView(const CClipRect& _Clip, const CRct& _Rect)
{
	MAUTOSTRIP(CRC_Viewport_SetView, MAUTOSTRIP_VOID);
	m_Clip = _Clip;
	m_Rect = _Rect;

	m_bVPChanged = true;
}

void CRC_Viewport::SetView(CImage* _pImg)
{
	MAUTOSTRIP(CRC_Viewport_SetView_2, MAUTOSTRIP_VOID);
	SetView(_pImg->GetClipRect(), _pImg->GetClipRect().clip);
}

const CMat4Dfp4& CRC_Viewport::GetProjectionMatrix()
{
	MAUTOSTRIP(CRC_Viewport_GetProjectionMatrix, *((void*)NULL));
	Update();
	return m_PMat;
};

void CRC_Viewport::GetClipVolume(CRC_ClipVolume& _Clip, const CMat43fp4* _pMat)
{
	MAUTOSTRIP(CRC_Viewport_GetClipVolume, MAUTOSTRIP_VOID);
	Update();

	_Clip.GetVertices(&m_VViewVertices[1], 4, CVec3Dfp4(0), false);
	CPlane3Dfp4 BPlane(CVec3Dfp4(0,0,1), -m_BackPlane);
	_Clip.CreateFromVertices3(BPlane);
	if (_pMat)
		_Clip.Transform(*_pMat);
}

int CRC_Viewport::GetNumViewVertices()
{
	MAUTOSTRIP(CRC_Viewport_GetNumViewVertices, 0);
	Update();
	return m_nViewVertices;
}

const CVec3Dfp4* CRC_Viewport::GetViewVertices()
{
	MAUTOSTRIP(CRC_Viewport_GetViewVertices, NULL);
	Update();
	return m_VViewVertices;
}

const CPlane3Dfp4* CRC_Viewport::GetViewPlanes()
{
	MAUTOSTRIP(CRC_Viewport_GetViewPlanes, NULL);
	Update();
	return m_VViewPlanes;
}

CRct CRC_Viewport::GetViewArea() const
{ 
	MAUTOSTRIP(CRC_Viewport_GetViewArea, CRct());
	CRct r(m_Rect); 
	r.Bound(m_Clip.clip);
	return r;
};

void CRC_Viewport::DebugViewControl(CMat4Dfp4& mat, fp4 Ctrl_x, fp4 Ctrl_z, fp4 Ctrl_y, fp4 Ctrl_xaxis, fp4 Ctrl_yaxis, fp4 _apan1)
{
	MAUTOSTRIP(CRC_Viewport_DebugViewControl, MAUTOSTRIP_VOID);
	(CVec3Dfp4(mat.k[0][0], mat.k[0][1], mat.k[0][2])*Ctrl_x).AddMatrixRow(mat, 3);
	(CVec3Dfp4(mat.k[1][0], mat.k[1][1], mat.k[1][2])*Ctrl_y).AddMatrixRow(mat, 3);
	(CVec3Dfp4(mat.k[2][0], mat.k[2][1], mat.k[2][2])*Ctrl_z).AddMatrixRow(mat, 3);
};

int CRC_Viewport::SphereInView(const CVec3Dfp4& _Pos, fp4 _Radius)
{
	MAUTOSTRIP(CRC_Viewport_SphereInView, 0);
/*

Return value:

0:	Sphere is outside the view.

else:

Returns mask for intersecting planes
1 : Front
2 : Back
4 : Left
8 : Right
16 : Top
32 : Bottom
256 : Visible

*/
	Update();

	int Mask = 0;
	fp4 d;
	if ((_Pos.k[2] + _Radius) < m_FrontPlane) return 0;					// Bakom?
	if ((_Pos.k[2] - _Radius) < m_FrontPlane) Mask += 1;
	if ((_Pos.k[2] - _Radius) > m_BackPlane) return 0;			// För långt bort?
	if ((_Pos.k[2] + _Radius) > m_BackPlane) Mask += 2;

	d = (m_VViewPlanes[0].n.k[0] * _Pos.k[0] + 
		m_VViewPlanes[0].n.k[1] * _Pos.k[1] + 
		m_VViewPlanes[0].n.k[2] * _Pos.k[2]);
	if (d > _Radius) return 0;
	if (d > -_Radius) Mask += 4;

	d = (m_VViewPlanes[1].n.k[0] * _Pos.k[0] + 
		m_VViewPlanes[1].n.k[1] * _Pos.k[1] + 
		m_VViewPlanes[1].n.k[2] * _Pos.k[2]);
	if (d > _Radius) return 0;
	if (d > -_Radius) Mask += 8;

	d = (m_VViewPlanes[2].n.k[0] * _Pos.k[0] + 
		m_VViewPlanes[2].n.k[1] * _Pos.k[1] + 
		m_VViewPlanes[2].n.k[2] * _Pos.k[2]);
	if (d > _Radius) return 0;
	if (d > -_Radius) Mask += 16;

	d = (m_VViewPlanes[3].n.k[0] * _Pos.k[0] + 
		m_VViewPlanes[3].n.k[1] * _Pos.k[1] + 
		m_VViewPlanes[3].n.k[2] * _Pos.k[2]);
	if (d > _Radius) return 0;
	if (d > -_Radius) Mask += 32;

	return Mask + 256;
};



void CRC_Viewport::SetBorder(CVec2Dfp4 _Border, bool _bRemove)
{
	MAUTOSTRIP(CRC_Viewport_SetBorder, MAUTOSTRIP_VOID);
//	if(_Border[0] == 0.f && _Border[1] == 0.f)
//	{
//		return;
//	}

	CClipRect _Clip = m_Clip;
	int w;
	int h;

	if(_bRemove)
	{
		w = (int)(fp4(_Clip.clip.GetWidth()) / (1.0f - _Border[0]));
		h = (int)(fp4(_Clip.clip.GetHeight()) / (1.0f - _Border[1]));

		_Clip.clip.p0.x = (int)(_Clip.clip.p0.x - (w * _Border[0] * 0.5f));
		_Clip.clip.p0.y = (int)(_Clip.clip.p0.y - (h * _Border[1] * 0.5f));
		_Clip.clip.p1.x = (int)(_Clip.clip.p0.x + w);
		_Clip.clip.p1.y = (int)(_Clip.clip.p0.y + h);
		_Clip.ofs		= _Clip.clip.p0;
	}
	else
	{
		w = (int)(fp4(_Clip.clip.GetWidth()) * (1.0f - _Border[0]));
		h = (int)(fp4(_Clip.clip.GetHeight()) * (1.0f - _Border[1]));
		 
		_Clip.clip.p0.x = (int)(_Clip.clip.GetWidth() * _Border[0] * 0.5f);
		_Clip.clip.p0.y = (int)(_Clip.clip.GetHeight() * _Border[1] * 0.5f);
		_Clip.clip.p1.x = (int)(_Clip.clip.p0.x + w);
		_Clip.clip.p1.y = (int)(_Clip.clip.p0.y + h);
		_Clip.ofs		= _Clip.clip.p0;
	}

	CRct _Rect(0,0,w,h);
	_Rect += _Clip.ofs;
	
	m_Clip = _Clip;
	m_Rect = _Rect;
	m_bVPChanged = true;
	
}

// -------------------------------------------------------------------
//  CRC_Attributes
// -------------------------------------------------------------------
CRC_Attributes::CRC_Attributes()
{
	MAUTOSTRIP(CRC_Attributes_ctor, MAUTOSTRIP_VOID);
	Clear();
}

void CRC_Attributes::Clear()
{
	MAUTOSTRIP(CRC_Attributes_Clear, MAUTOSTRIP_VOID);
	m_AttribLock = 0;
	m_AttribLockFlags = 0;

	m_Flags = 0;
	m_RasterMode = 0;
	m_SourceBlend = 0;
	m_DestBlend = 0;
	m_ZCompare = 0;
	m_AlphaCompare = 0;
	m_AlphaRef = 0;
	m_FogColor = 0;
	m_FogStart = 0;
	m_FogEnd = 0;
	m_FogDensity = 0;

	m_VBFlags = 0;

#ifndef	PLATFORM_PS2
	m_StencilRef = 0;
	m_StencilFuncAnd = 0;
//	m_StencilWriteMask = 0xFF;

	m_StencilFrontFunc = 0;
	m_StencilFrontOpFail = 0;
	m_StencilFrontOpZFail = 0;
	m_StencilFrontOpZPass = 0;
	m_StencilBackFunc = 0;
	m_StencilBackOpFail = 0;
	m_StencilBackOpZFail = 0;
	m_StencilBackOpZPass = 0;

	m_Scissor.m_Min = 0;
	m_Scissor.m_Max = 0xffff;
#endif

	m_PolygonOffsetScale = 0;
	m_PolygonOffsetUnits = 0;

	m_pLights = NULL;
	m_nLights = 0;

	m_pExtAttrib = NULL;

#ifdef PLATFORM_PS2
	m_nChannels = 0;
#endif

	{
		m_TexGenComp = 0;
		m_lTexGenMode[0] = CRC_TEXGENMODE_TEXCOORD;
		m_lTexGenMode[1] = CRC_TEXGENMODE_TEXCOORD;
		m_lTexGenMode[2] = CRC_TEXGENMODE_TEXCOORD;
		m_lTexGenMode[3] = CRC_TEXGENMODE_TEXCOORD;
		for(int i = 4; i < CRC_MAXTEXCOORDS; i++)
			m_lTexGenMode[i] = CRC_TEXGENMODE_VOID;
	}
	{
		for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
		{
			m_iTexCoordSet[i] = i;
		}
	}
	{
		for(int i = 0; i < CRC_MAXTEXTURES; i++)
		{
			m_TextureID[i] = 0;
		}
	}
	{
		for(int i = 0; i < CRC_MAXTEXTUREENV; i++)
		{
			m_TexEnvMode[i] = 0;
		}
	}
	m_pTexEnvColors = NULL;
	m_pTexGenAttr = NULL;
}

void CRC_Attributes::SetDefault()
{
	MAUTOSTRIP(CRC_Attributes_SetDefault, MAUTOSTRIP_VOID);
	m_AttribLock = 0;
	m_AttribLockFlags = 0;

	Attrib_Default();
	m_RasterMode =		CRC_RASTERMODE_NONE;
	m_SourceBlend =		CRC_BLEND_ONE;
	m_DestBlend =		CRC_BLEND_ZERO;
	m_ZCompare =		CRC_COMPARE_LESSEQUAL;
	m_AlphaCompare =	CRC_COMPARE_ALWAYS;
	m_AlphaRef = 128;
	m_FogColor = 0;
	m_FogStart = 0.0f;
	m_FogEnd = 2048.0f;
	m_FogDensity = 1.0f;
	m_VBFlags = 0;
 
 #ifndef	PLATFORM_PS2
	m_StencilRef = 0;
	m_StencilFuncAnd = 0xFF;
//	m_StencilWriteMask = 0xFF;

	m_StencilFrontFunc = CRC_COMPARE_ALWAYS;
	m_StencilFrontOpFail = 0;
	m_StencilFrontOpZFail = 0;
	m_StencilFrontOpZPass = CRC_STENCILOP_REPLACE;
	m_StencilBackFunc = CRC_COMPARE_ALWAYS;
	m_StencilBackOpFail = 0;
	m_StencilBackOpZFail = 0;
	m_StencilBackOpZPass = CRC_STENCILOP_REPLACE;

	m_Scissor.m_Min = 0;
	m_Scissor.m_Max = 0xffff;
#endif	// PLATFORM_PS2

	m_PolygonOffsetScale = 0;
	m_PolygonOffsetUnits = 0;

	m_pLights = NULL;
	m_nLights = 0;

#ifdef PLATFORM_PS2
	m_nChannels = 0;
#endif

	m_pExtAttrib = NULL;

	{
		m_TexGenComp = 0;
		m_lTexGenMode[0] = CRC_TEXGENMODE_TEXCOORD;
		m_lTexGenMode[1] = CRC_TEXGENMODE_TEXCOORD;
		m_lTexGenMode[2] = CRC_TEXGENMODE_TEXCOORD;
		m_lTexGenMode[3] = CRC_TEXGENMODE_TEXCOORD;
		for(int i = 4; i < CRC_MAXTEXCOORDS; i++)
			m_lTexGenMode[i] = CRC_TEXGENMODE_VOID;
	}
	{
		for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
		{
			m_iTexCoordSet[i] = i;
		}
	}
	{
		for(int i = 0; i < CRC_MAXTEXTURES; i++)
		{
			m_TextureID[i] = 0;
		}
	}
	{
		for(int i = 0; i < CRC_MAXTEXTUREENV; i++)
		{
			m_TexEnvMode[i] = 0;
		}
	}
	m_pTexEnvColors = NULL;
	m_pTexGenAttr = NULL;
}

void CRC_Attributes::ClearRasterModeSettings()
{
	MAUTOSTRIP(CRC_Attributes_ClearRasterModeSettings, MAUTOSTRIP_VOID);
	m_Flags &= ~(CRC_FLAGS_BLEND/* | CRC_FLAGS_ALPHA*/);
	m_SourceBlend =	CRC_BLEND_ONE;
	m_DestBlend = CRC_BLEND_ZERO;
	
#ifdef PLATFORM_PS2
	m_nChannels = 0;
#endif

}

int CRC_Attributes::Compare(const CRC_Attributes& _Attr) const
{
#if 0
	def PLATFORM_XBOX1
	int Return = 0;
	enum
	{
		SizeToCmp = (sizeof(_Attr)/sizeof(void *))
	};
	__asm
	{	
		mov ecx, SizeToCmp
		mov esi, this
		mov edi, [_Attr]
		repe cmpsd
		jb Below
		ja Above
		jmp EndIt
Below:	mov Return, -1
		jmp EndIt
Above:	mov Return, 1
EndIt:
	}

	return Return;

#else
	MAUTOSTRIP(CRC_Attributes_Compare, 0);

//	return memcmp(this, &_Attr, sizeof(_Attr));

#ifdef PLATFORM_PS2
	// Sort on texture0, texture1, etc.. then lightstate
	if (m_TextureID[3] > _Attr.m_TextureID[3]) return 1;
	if (m_TextureID[3] < _Attr.m_TextureID[3]) return -1;
	if (m_TextureID[2] > _Attr.m_TextureID[2]) return 1;
	if (m_TextureID[2] < _Attr.m_TextureID[2]) return -1;
	if (m_TextureID[1] > _Attr.m_TextureID[1]) return 1;
	if (m_TextureID[1] < _Attr.m_TextureID[1]) return -1;
	if (m_TextureID[0] > _Attr.m_TextureID[0]) return 1;
	if (m_TextureID[0] < _Attr.m_TextureID[0]) return -1;
	
	if (m_pExtAttrib > _Attr.m_pExtAttrib) return 1;
	if (m_pExtAttrib < _Attr.m_pExtAttrib) return -1;
	if (m_nLights > _Attr.m_nLights) return 1;
	if (m_nLights < _Attr.m_nLights) return -1;
	if (m_pLights > _Attr.m_pLights) return 1;
	if (m_pLights < _Attr.m_pLights) return -1;
	
#else
	// Sort on lightstate first, then texture0, texture1, etc..
	if (m_pExtAttrib > _Attr.m_pExtAttrib) return 1;
	if (m_pExtAttrib < _Attr.m_pExtAttrib) return -1;
	if (m_nLights > _Attr.m_nLights) return 1;
	if (m_nLights < _Attr.m_nLights) return -1;
	if (m_pLights > _Attr.m_pLights) return 1;
	if (m_pLights < _Attr.m_pLights) return -1;

	{
		for(int iTex = 0; iTex < CRC_MAXTEXTURES; iTex++)
		{
			if (m_TextureID[iTex] > _Attr.m_TextureID[iTex]) return 1;
			if (m_TextureID[iTex] < _Attr.m_TextureID[iTex]) return -1;
		}
	}
#endif

//return 0;

	{
		if (m_TexGenComp > _Attr.m_TexGenComp) return 1;
		if (m_TexGenComp < _Attr.m_TexGenComp) return -1;

		for(int iTex = 0; iTex < CRC_MAXTEXCOORDS; iTex++)
		{
			if (m_lTexGenMode[iTex] > _Attr.m_lTexGenMode[iTex]) return 1;
			if (m_lTexGenMode[iTex] < _Attr.m_lTexGenMode[iTex]) return -1;
		}
	}

	{
		for(int iTex = 0; iTex < CRC_MAXTEXCOORDS; iTex++)
		{
			if (m_iTexCoordSet[iTex] > _Attr.m_iTexCoordSet[iTex]) return 1;
			if (m_iTexCoordSet[iTex] < _Attr.m_iTexCoordSet[iTex]) return -1;
		}
	}
	{
		for(int iTex = 0; iTex < CRC_MAXTEXTUREENV; iTex++)
		{
			if (m_TexEnvMode[iTex] > _Attr.m_TexEnvMode[iTex]) return 1;
			if (m_TexEnvMode[iTex] < _Attr.m_TexEnvMode[iTex]) return -1;
		}
	}
	if (m_pTexEnvColors < _Attr.m_pTexEnvColors) return 1;
	if (m_pTexEnvColors > _Attr.m_pTexEnvColors) return -1;

	if (m_AttribLock > _Attr.m_AttribLock) return 1;
	if (m_AttribLock < _Attr.m_AttribLock) return -1;
	if (m_AttribLockFlags > _Attr.m_AttribLockFlags) return 1;
	if (m_AttribLockFlags < _Attr.m_AttribLockFlags) return -1;
	if (m_RasterMode > _Attr.m_RasterMode) return 1;
	if (m_RasterMode < _Attr.m_RasterMode) return -1;
	if (m_SourceBlend > _Attr.m_SourceBlend) return 1;
	if (m_SourceBlend < _Attr.m_SourceBlend) return -1;
	if (m_DestBlend > _Attr.m_DestBlend) return 1;
	if (m_DestBlend < _Attr.m_DestBlend) return -1;
	if (m_Flags > _Attr.m_Flags) return 1;
	if (m_Flags < _Attr.m_Flags) return -1;
	if (m_ZCompare > _Attr.m_ZCompare) return 1;
	if (m_ZCompare < _Attr.m_ZCompare) return -1;
	if (m_AlphaCompare > _Attr.m_AlphaCompare) return 1;
	if (m_AlphaCompare < _Attr.m_AlphaCompare) return -1;
	if (m_AlphaRef > _Attr.m_AlphaRef) return 1;
	if (m_AlphaRef < _Attr.m_AlphaRef) return -1;

#ifndef	PLATFORM_PS2
	if (m_PolygonOffsetScale > _Attr.m_PolygonOffsetScale) return 1;
	if (m_PolygonOffsetScale < _Attr.m_PolygonOffsetScale) return -1;
	if (m_PolygonOffsetUnits > _Attr.m_PolygonOffsetUnits) return 1;
	if (m_PolygonOffsetUnits < _Attr.m_PolygonOffsetUnits) return -1;

	if (m_Scissor.m_Min[0] > _Attr.m_Scissor.m_Min[0]) return 1;
	if (m_Scissor.m_Min[0] < _Attr.m_Scissor.m_Min[0]) return -1;
	if (m_Scissor.m_Min[1] > _Attr.m_Scissor.m_Min[1]) return 1;
	if (m_Scissor.m_Min[1] < _Attr.m_Scissor.m_Min[1]) return -1;
	if (m_Scissor.m_Max[0] > _Attr.m_Scissor.m_Max[0]) return 1;
	if (m_Scissor.m_Max[0] < _Attr.m_Scissor.m_Max[0]) return -1;
	if (m_Scissor.m_Max[1] > _Attr.m_Scissor.m_Max[1]) return 1;
	if (m_Scissor.m_Max[1] < _Attr.m_Scissor.m_Max[1]) return -1;

	if (m_StencilDWord1 > _Attr.m_StencilDWord1) return 1;
	if (m_StencilDWord1 < _Attr.m_StencilDWord1) return -1;
	if (m_StencilRef > _Attr.m_StencilRef) return 1;
	if (m_StencilRef < _Attr.m_StencilRef) return -1;
	if (m_StencilFuncAnd > _Attr.m_StencilFuncAnd) return 1;
	if (m_StencilFuncAnd < _Attr.m_StencilFuncAnd) return -1;
//	if (m_StencilWriteMask > _Attr.m_StencilWriteMask) return 1;
//	if (m_StencilWriteMask < _Attr.m_StencilWriteMask) return -1;
#endif
	if (m_FogColor > _Attr.m_FogColor) return 1;
	if (m_FogColor < _Attr.m_FogColor) return -1;
	if (m_FogStart > _Attr.m_FogStart) return 1;
	if (m_FogStart < _Attr.m_FogStart) return -1;
	if (m_FogEnd > _Attr.m_FogEnd) return 1;
	if (m_FogEnd < _Attr.m_FogEnd) return -1;
	if (m_FogDensity > _Attr.m_FogDensity) return 1;
	if (m_FogDensity < _Attr.m_FogDensity) return -1;

	if (m_pTexGenAttr > _Attr.m_pTexGenAttr) return 1;
	if (m_pTexGenAttr < _Attr.m_pTexGenAttr) return -1;

	if (m_VBFlags < _Attr.m_VBFlags) return -1;
	if (m_VBFlags > _Attr.m_VBFlags) return 1;

	return 0;
#endif
}

int CRC_Attributes::GetTexGenModeAttribSize(int _TexGenMode, int _TexGenComp)
{
	int nComp = (_TexGenComp & 1) + ((_TexGenComp >> 1) & 1) + ((_TexGenComp >> 2) & 1) + ((_TexGenComp >> 3) & 1);

	switch(_TexGenMode)
	{
	case CRC_TEXGENMODE_LINEAR :
		return nComp * 4;

	case CRC_TEXGENMODE_PIXELINFO :
	case CRC_TEXGENMODE_LINEARNHF :
		return 12;

	case CRC_TEXGENMODE_BOXNHF :
		return 10*4;

	case CRC_TEXGENMODE_TSREFLECTION :
	case CRC_TEXGENMODE_LIGHTING :
	case CRC_TEXGENMODE_DEPTHOFFSET :
		return 8;

	case CRC_TEXGENMODE_TSLV :
	case CRC_TEXGENMODE_CONSTANT :
	case CRC_TEXGENMODE_SHADOWVOLUME :
	case CRC_TEXGENMODE_SHADOWVOLUME2 :
		return 4;

	case CRC_TEXGENMODE_BUMPCUBEENV :
		return 32;

	default :
		return 0;
	};
}

// -------------------------------------------------------------------
//  CRC_ClipStackEntry
// -------------------------------------------------------------------
uint8 CRC_VertexFormat::ms_lVRegFormatSizes[CRC_VREGFMT_MAX] = 
{
	0, 4,8,12,16, 2,4,6,8, 2,4,6,8, 6,4,4, 4,4,4, //16,4,
};

uint8 CRC_VertexFormat::ms_lVRegFormatComp[CRC_VREGFMT_MAX] = 
{
	0, 1,2,3,4, 1,2,3,4, 1,2,3,4, 3,3,3, 4,4,4, //4,4,
};

	


int CRC_VertexFormat::GetRegisterSize(uint32 _Format)
{
	MAUTOSTRIP(CRC_VertexFormat_GetRegisterSize, 0);
	M_ASSERT(_Format < CRC_VREGFMT_MAX, "!");
	return ms_lVRegFormatSizes[_Format];
}

int CRC_VertexFormat::GetRegisterComponents(uint32 _Format)
{
	MAUTOSTRIP(CRC_VertexFormat_GetRegisterComponents, 0);
	M_ASSERT(_Format < CRC_VREGFMT_MAX, "!");
	return ms_lVRegFormatComp[_Format];
}

int CRC_VertexFormat::GetStride() const
{
	MAUTOSTRIP(CRC_VertexFormat_GetStride, 0);
	int Stride = 0;
	for(int i = 0; i < CRC_MAXVERTEXREG; i++)
		Stride += GetRegisterSize(m_lVRegFormats[i]);

	return Stride;
}

void CRC_VertexFormat::ConvertRegisterFormat(
	const void* _pSrc, int _SrcFmt, int _SrcStride, const CRC_VRegTransform* _pSrcScale,
	void* _pDst, int _DstFmt, int _DstStride, const CRC_VRegTransform* _pDstScale, int _nV)
{
	MAUTOSTRIP(CRC_VertexFormat_ConvertRegisterFormat, MAUTOSTRIP_VOID);
//	int nComp = Min(GetRegisterComponents(_SrcFmt), GetRegisterComponents(_DstFmt));
	int SrcSize = GetRegisterSize(_SrcFmt);
	int DstSize = GetRegisterSize(_DstFmt);

#ifndef M_RTM
	void* pSrcEnd = (uint8*)_pSrc + _SrcStride * _nV;
	void* pDstEnd = (uint8*)_pDst + _DstStride * _nV;
#endif
	CVec4Dfp4 DstScaleRecp;
	if (_pDstScale)
	{
		DstScaleRecp[0] = 1.0f / _pDstScale->m_Scale[0];
		DstScaleRecp[1] = 1.0f / _pDstScale->m_Scale[1];
		DstScaleRecp[2] = 1.0f / _pDstScale->m_Scale[2];
		DstScaleRecp[3] = 1.0f / _pDstScale->m_Scale[3];
	}

	for(int iV = 0; iV < _nV; iV++)
	{
		CVec4Dfp4 V(0,0,0,1);
		switch(_SrcFmt)
		{
		case CRC_VREGFMT_V1_F32 :
			V[0] = *((fp4*&)_pSrc)++;
			break;
		case CRC_VREGFMT_V2_F32 :
			V[0] = *((fp4*&)_pSrc)++;
			V[1] = *((fp4*&)_pSrc)++;
			break;
		case CRC_VREGFMT_V3_F32 :
			V[0] = *((fp4*&)_pSrc)++;
			V[1] = *((fp4*&)_pSrc)++;
			V[2] = *((fp4*&)_pSrc)++;
			break;
		case CRC_VREGFMT_V4_F32 :
			V[0] = *((fp4*&)_pSrc)++;
			V[1] = *((fp4*&)_pSrc)++;
			V[2] = *((fp4*&)_pSrc)++;
			V[3] = *((fp4*&)_pSrc)++;
			break;
		case CRC_VREGFMT_V1_I16 :
			V[0] = *((int16*&)_pSrc)++;
			break;
		case CRC_VREGFMT_V2_I16 :
			V[0] = *((int16*&)_pSrc)++;
			V[1] = *((int16*&)_pSrc)++;
			break;
		case CRC_VREGFMT_V3_I16 :
			V[0] = *((int16*&)_pSrc)++;
			V[1] = *((int16*&)_pSrc)++;
			V[2] = *((int16*&)_pSrc)++;
			break;
		case CRC_VREGFMT_V4_I16 :
			V[0] = *((int16*&)_pSrc)++;
			V[1] = *((int16*&)_pSrc)++;
			V[2] = *((int16*&)_pSrc)++;
			V[3] = *((int16*&)_pSrc)++;
			break;
		case CRC_VREGFMT_V1_U16 :
			V[0] = *((uint16*&)_pSrc)++;
			break;
		case CRC_VREGFMT_V2_U16 :
			V[0] = *((uint16*&)_pSrc)++;
			V[1] = *((uint16*&)_pSrc)++;
			break;
		case CRC_VREGFMT_V3_U16 :
			V[0] = *((uint16*&)_pSrc)++;
			V[1] = *((uint16*&)_pSrc)++;
			V[2] = *((uint16*&)_pSrc)++;
			break;
		case CRC_VREGFMT_V4_U16 :
			V[0] = *((uint16*&)_pSrc)++;
			V[1] = *((uint16*&)_pSrc)++;
			V[2] = *((uint16*&)_pSrc)++;
			V[3] = *((uint16*&)_pSrc)++;
			break;
		case CRC_VREGFMT_NS3_I16 :
			V[0] = fp4(*((int16*&)_pSrc)++) * (1.0f / 32768.0f);
			V[1] = fp4(*((int16*&)_pSrc)++) * (1.0f / 32768.0f);
			V[2] = fp4(*((int16*&)_pSrc)++) * (1.0f / 32768.0f);
			break;

		case CRC_VREGFMT_NU3_P32 :
			{
				uint32 Packed = *((uint32*&)_pSrc)++;
				V[0] = fp4((Packed >> 0) & 0x7ff) / 2047.0f;
				V[1] = fp4((Packed >> 11) & 0x7ff) / 2047.0f;
				V[2] = fp4((Packed >> 22) & 0x3ff) / 1023.0f;
			}
			break;
		case CRC_VREGFMT_NS3_P32 :
			{
				uint32 Packed = *((uint32*&)_pSrc)++;
				V[0] = fp4((Packed >> 0) & 0x7ff) / 1023.0f;
				V[1] = fp4((Packed >> 11) & 0x7ff) / 1023.0f;
				V[2] = fp4((Packed >> 22) & 0x3ff) / 511.0f;
			}
			break;
		case CRC_VREGFMT_N4_UI8_P32 :
			{
				uint32 Packed = *((uint32*&)_pSrc)++;
				V[0] = fp4((Packed >> 0)&0xff);
				V[1] = fp4((Packed >> 8)&0xff);
				V[2] = fp4((Packed >> 16)&0xff);
				V[3] = fp4((Packed >> 24)&0xff);
			}
			break;
		case CRC_VREGFMT_N4_UI8_P32_NORM :
			{
				uint32 Packed = *((uint32*&)_pSrc)++;
				V[0] = fp4((Packed >> 0)&0xff) * (1.0f / 255.0f);
				V[1] = fp4((Packed >> 8)&0xff) * (1.0f / 255.0f);
				V[2] = fp4((Packed >> 16)&0xff) * (1.0f / 255.0f);
				V[3] = fp4((Packed >> 24)&0xff) * (1.0f / 255.0f);
			}
			break;
			
		case CRC_VREGFMT_N4_COL:
#if 1 // Is this type
			{
				uint32 Packed = *((uint32*&)_pSrc)++;
				V[2] = fp4((Packed >> 0)&0xff) * (1.0f / 255.0f);
				V[1] = fp4((Packed >> 8)&0xff) * (1.0f / 255.0f);
				V[0] = fp4((Packed >> 16)&0xff) * (1.0f / 255.0f);
				V[3] = fp4((Packed >> 24)&0xff) * (1.0f / 255.0f);
			}
#else

			V[2] = fp4(*((uint8*&)_pSrc)++) * (1.0f / 255.0f);
			V[1] = fp4(*((uint8*&)_pSrc)++) * (1.0f / 255.0f);
			V[0] = fp4(*((uint8*&)_pSrc)++) * (1.0f / 255.0f);
			V[3] = fp4(*((uint8*&)_pSrc)++) * (1.0f / 255.0f);
#endif
			break;
		default:
			Error_static("CRC_VertexFormat::ConvertRegisterFormat", CStrF("Invalid source format %d", _SrcFmt));
		}

		if (_pSrcScale)
		{
			V[0] = V[0]*_pSrcScale->m_Scale[0] + _pSrcScale->m_Offset[0];
			V[1] = V[1]*_pSrcScale->m_Scale[1] + _pSrcScale->m_Offset[1];
			V[2] = V[2]*_pSrcScale->m_Scale[2] + _pSrcScale->m_Offset[2];
			V[3] = V[3]*_pSrcScale->m_Scale[3] + _pSrcScale->m_Offset[3];
		}
		if (_pDstScale)
		{
			V[0] = (V[0] - _pDstScale->m_Offset[0]) * DstScaleRecp[0];
			V[1] = (V[1] - _pDstScale->m_Offset[1]) * DstScaleRecp[1];
			V[2] = (V[2] - _pDstScale->m_Offset[2]) * DstScaleRecp[2];
			V[3] = (V[3] - _pDstScale->m_Offset[3]) * DstScaleRecp[3];
		}

		switch(_DstFmt)
		{
		case CRC_VREGFMT_V1_F32 :
			*(((fp4*&)_pDst)++) = V[0];
			break;
		case CRC_VREGFMT_V2_F32 :
			*(((fp4*&)_pDst)++) = V[0];
			*(((fp4*&)_pDst)++) = V[1];
			break;
		case CRC_VREGFMT_V3_F32 :
			*(((fp4*&)_pDst)++) = V[0];
			*(((fp4*&)_pDst)++) = V[1];
			*(((fp4*&)_pDst)++) = V[2];
			break;
		case CRC_VREGFMT_V4_F32 :
			*(((fp4*&)_pDst)++) = V[0];
			*(((fp4*&)_pDst)++) = V[1];
			*(((fp4*&)_pDst)++) = V[2];
			*(((fp4*&)_pDst)++) = V[3];
			break;
		case CRC_VREGFMT_V1_I16:
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[0]), -32768, 32767);
			break;
		case CRC_VREGFMT_V2_I16:
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[0]), -32768, 32767);
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[1]), -32768, 32767);
			break;
		case CRC_VREGFMT_V3_I16:
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[0]), -32768, 32767);
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[1]), -32768, 32767);
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[2]), -32768, 32767);
			break;
		case CRC_VREGFMT_V4_I16:
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[0]), -32768, 32767);
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[1]), -32768, 32767);
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[2]), -32768, 32767);
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[3]), -32768, 32767);
			break;
		case CRC_VREGFMT_V1_U16:
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[0]), 0, 65535);
			break;
		case CRC_VREGFMT_V2_U16:
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[0]), 0, 65535);
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[1]), 0, 65535);
			break;
		case CRC_VREGFMT_V3_U16:
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[0]), 0, 65535);
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[1]), 0, 65535);
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[2]), 0, 65535);
			break;
		case CRC_VREGFMT_V4_U16:
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[0]), 0, 65535);
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[1]), 0, 65535);
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[2]), 0, 65535);
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[3]), 0, 65535);
			break;
		case CRC_VREGFMT_NS3_I16 :
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[0]*32767.0f), -32768, 32767);
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[1]*32767.0f), -32768, 32767);
			*(((int16*&)_pDst)++) = Clamp(RoundToInt(V[2]*32767.0f), -32768, 32767);
			break;
		case CRC_VREGFMT_NS3_P32 :
			{
				uint32 Packed =
					(((Clamp(RoundToInt((V[0])*1023.0f),-1024,1023)) & 0x7ff) << 0) +
					(((Clamp(RoundToInt((V[1])*1023.0f),-1024,1023)) & 0x7ff) << 11) +
					(((Clamp(RoundToInt((V[2])*511.0f),-512,511)) & 0x3ff) << 22);
				*(((uint32*&)_pDst)++) = Packed;
			}
			break;
		case CRC_VREGFMT_NU3_P32 :
			{
				uint32 Packed =
					(((uint32)Clamp((int32)RoundToInt((V[0])*2047.0f),0,0x7ff)) << 0) +
					(((uint32)Clamp((int32)RoundToInt((V[1])*2047.0f),0,0x7ff)) << 11) +
					(((uint32)Clamp((int32)RoundToInt((V[2])*1023.0f),0,0x3ff)) << 22);
				*(((uint32*&)_pDst)++) = Packed;
			}
			break;
		case CRC_VREGFMT_N4_UI8_P32 :
			{
				uint32 Packed =
					(((uint32)Clamp((int32)RoundToInt((V[0])),0,0xff)) << 0) +
					(((uint32)Clamp((int32)RoundToInt((V[1])),0,0xff)) << 8) +
					(((uint32)Clamp((int32)RoundToInt((V[2])),0,0xff)) << 16) + 
					(((uint32)Clamp((int32)RoundToInt((V[3])),0,0xff)) << 24);
				*(((uint32*&)_pDst)++) = Packed;
			}
			break;
		case CRC_VREGFMT_N4_UI8_P32_NORM :
			{
				uint32 Packed =
					(((uint32)Clamp((int32)RoundToInt((V[0])*255.0f),0,0xff)) << 0) +
					(((uint32)Clamp((int32)RoundToInt((V[1])*255.0f),0,0xff)) << 8) +
					(((uint32)Clamp((int32)RoundToInt((V[2])*255.0f),0,0xff)) << 16) + 
					(((uint32)Clamp((int32)RoundToInt((V[3])*255.0f),0,0xff)) << 24);
				*(((uint32*&)_pDst)++) = Packed;
			}
			break;
		case CRC_VREGFMT_N4_COL :
#if 1
			{
				uint32 Packed =
					(((uint32)Clamp((int32)RoundToInt((V[2])*255.0f),0,0xff)) << 0) +
					(((uint32)Clamp((int32)RoundToInt((V[1])*255.0f),0,0xff)) << 8) +
					(((uint32)Clamp((int32)RoundToInt((V[0])*255.0f),0,0xff)) << 16) + 
					(((uint32)Clamp((int32)RoundToInt((V[3])*255.0f),0,0xff)) << 24);
				*(((uint32*&)_pDst)++) = Packed;
			}
#else
			*(((uint8*&)_pDst)++) = Clamp(RoundToInt(V[2]*255.0f), 0, 255);
			*(((uint8*&)_pDst)++) = Clamp(RoundToInt(V[1]*255.0f), 0, 255);
			*(((uint8*&)_pDst)++) = Clamp(RoundToInt(V[0]*255.0f), 0, 255);
			*(((uint8*&)_pDst)++) = Clamp(RoundToInt(V[3]*255.0f), 0, 255);
#endif
			break;
		default:
			Error_static("CRC_VertexFormat::ConvertRegisterFormat", CStrF("Invalid destination format %d", _DstFmt));
		}

		(uint8*&) _pSrc += _SrcStride - SrcSize;
		(uint8*&) _pDst += _DstStride - DstSize;
	}

#ifndef M_RTM
	if (_pSrc != pSrcEnd)
		Error_static("CRC_VertexFormat::ConvertRegisterFormat", CStrF("Invalid source pointer %.8x != %.8x", _pSrc, pSrcEnd));
	if (_pDst != pDstEnd)
		Error_static("CRC_VertexFormat::ConvertRegisterFormat", CStrF("Invalid destination pointer %.8x != %.8x", _pDst, pDstEnd));
#endif
}

/*static void CRC_VertexFormat::ConvertVertexFormat(
	const void* _pSrc, const CRC_VertexFormat& _SrcFmt,
	void* _pDst, const CRC_VertexFormat& _DstFmt, int _nV)
{
	int SrcStride = _SrcFmt::GetStride();
	int DstStride = _DstFmt::GetStride();

	for(int iVReg = 0; iVReg < CRC_MAXVERTEXREG; iVReg++)
	{
		ConvertRegisterFormat(_pSrc, _SrcFmt.GetFormat(iVReg), SrcStride, _SrcFmt.m_)
	}
}*/

mint CRC_BuildVertexBuffer::CRC_VertexBuffer_GetSize() const
{
	mint Stride = 0;
	if (m_Format.GetFormat(CRC_VREG_POS))
		Stride += sizeof(fp4) * 3;

	for (int i = 0; i < CRC_MAXTEXCOORDS; ++i)
	{
		switch (m_Format.GetFormat(CRC_VREG_TEXCOORD0 + i))
		{
		case CRC_VREGFMT_V1_F32:
		case CRC_VREGFMT_V1_I16:
		case CRC_VREGFMT_V1_U16:
			Stride += sizeof(fp4);
			break;
		case CRC_VREGFMT_V2_F32:
		case CRC_VREGFMT_V2_I16:
		case CRC_VREGFMT_V2_U16:
			Stride += sizeof(fp4) * 2;
			break;
		case CRC_VREGFMT_V3_F32:
		case CRC_VREGFMT_V3_I16:
		case CRC_VREGFMT_V3_U16:
		case CRC_VREGFMT_NS3_I16:
		case CRC_VREGFMT_NS3_P32:
		case CRC_VREGFMT_NU3_P32:
			Stride += sizeof(fp4) * 3;
			break;
		case CRC_VREGFMT_V4_F32:
		case CRC_VREGFMT_V4_I16:
		case CRC_VREGFMT_V4_U16:
		case CRC_VREGFMT_N4_UI8_P32_NORM:
		case CRC_VREGFMT_N4_UI8_P32:
		case CRC_VREGFMT_N4_COL:
			Stride += sizeof(fp4) * 4;
			break;
		}
	}

	if (m_Format.GetFormat(CRC_VREG_NORMAL))
		Stride += sizeof(fp4) * 3;
	if (m_Format.GetFormat(CRC_VREG_COLOR))
		Stride += sizeof(uint32);
	if (m_Format.GetFormat(CRC_VREG_SPECULAR))
		Stride += sizeof(uint32);
	if (m_Format.GetFormat(CRC_VREG_MI0))
		Stride += sizeof(uint32);
	if (m_Format.GetFormat(CRC_VREG_MI1))
		Stride += sizeof(uint32);

	switch (m_Format.GetFormat(CRC_VREG_MW0))
	{
		case CRC_VREGFMT_V1_F32:
		case CRC_VREGFMT_V1_I16:
		case CRC_VREGFMT_V1_U16:
			Stride += sizeof(fp4);
			break;
		case CRC_VREGFMT_V2_F32:
		case CRC_VREGFMT_V2_I16:
		case CRC_VREGFMT_V2_U16:
			Stride += sizeof(fp4) * 2;
			break;
		case CRC_VREGFMT_V3_F32:
		case CRC_VREGFMT_V3_I16:
		case CRC_VREGFMT_V3_U16:
		case CRC_VREGFMT_NS3_I16:
		case CRC_VREGFMT_NS3_P32:
		case CRC_VREGFMT_NU3_P32:
			Stride += sizeof(fp4) * 3;
			break;
		case CRC_VREGFMT_V4_F32:
		case CRC_VREGFMT_V4_I16:
		case CRC_VREGFMT_V4_U16:
		case CRC_VREGFMT_N4_UI8_P32_NORM:
		case CRC_VREGFMT_N4_UI8_P32:
		case CRC_VREGFMT_N4_COL:
			Stride += sizeof(fp4) * 4;
			break;
	}

	if (m_Format.GetFormat(CRC_VREG_MW1))
		Stride += sizeof(fp4) * 4; // Only support extra 4 weights

	return Stride * m_nV;
}

void CRC_BuildVertexBuffer::CRC_VertexBuffer_ConvertTo(void* _pDst, CRC_VertexBuffer& _DstBuffer) const
{
	_DstBuffer.Clear();
	_DstBuffer.m_nV = m_nV;
	_DstBuffer.m_piPrim = m_piPrim;
	_DstBuffer.m_nPrim = m_nPrim;
	_DstBuffer.m_PrimType = m_PrimType;

	uint8 *pDst = (uint8 *)_pDst;
	int iVReg = CRC_VREG_POS;
	if (m_Format.GetFormat(iVReg))
	{
		int SrcFmt = m_Format.GetFormat(iVReg);
		const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
		CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_V3_F32, sizeof(fp4)*3, NULL, m_nV);
		_DstBuffer.m_pV = (CVec3Dfp4*)pDst;
		pDst += sizeof(fp4) * 3 * m_nV;
	}

	for (int i = 0; i < CRC_MAXTEXCOORDS; ++i)
	{
		int iVReg = CRC_VREG_TEXCOORD0 + i;
		switch (m_Format.GetFormat(iVReg))
		{
		case CRC_VREGFMT_V1_F32:
		case CRC_VREGFMT_V1_I16:
		case CRC_VREGFMT_V1_U16:
			{
				int SrcFmt = m_Format.GetFormat(iVReg);
				const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
				CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_V1_F32, sizeof(fp4)*1, NULL, m_nV);
				_DstBuffer.m_pTV[i] = (fp4*)pDst;
				_DstBuffer.m_nTVComp[i] = 1;
				pDst += sizeof(fp4) * 1 * m_nV;
			}
			break;
		case CRC_VREGFMT_V2_F32:
		case CRC_VREGFMT_V2_I16:
		case CRC_VREGFMT_V2_U16:
			{
				int SrcFmt = m_Format.GetFormat(iVReg);
				const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
				CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_V2_F32, sizeof(fp4)*2, NULL, m_nV);
				_DstBuffer.m_pTV[i] = (fp4*)pDst;
				_DstBuffer.m_nTVComp[i] = 2;
				pDst += sizeof(fp4) * 2 * m_nV;
			}
			break;
		case CRC_VREGFMT_V3_F32:
		case CRC_VREGFMT_V3_I16:
		case CRC_VREGFMT_V3_U16:
		case CRC_VREGFMT_NS3_I16:
		case CRC_VREGFMT_NS3_P32:
		case CRC_VREGFMT_NU3_P32:
			{
				int SrcFmt = m_Format.GetFormat(iVReg);
				const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
				CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_V3_F32, sizeof(fp4)*3, NULL, m_nV);
				_DstBuffer.m_pTV[i] = (fp4*)pDst;
				_DstBuffer.m_nTVComp[i] = 3;
				pDst += sizeof(fp4) * 3 * m_nV;
			}
			break;
		case CRC_VREGFMT_V4_F32:
		case CRC_VREGFMT_V4_I16:
		case CRC_VREGFMT_V4_U16:
		case CRC_VREGFMT_N4_UI8_P32_NORM:
		case CRC_VREGFMT_N4_UI8_P32:
		case CRC_VREGFMT_N4_COL:
			{
				int SrcFmt = m_Format.GetFormat(iVReg);
				const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
				CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_V4_F32, sizeof(fp4)*4, NULL, m_nV);
				_DstBuffer.m_pTV[i] = (fp4*)pDst;
				_DstBuffer.m_nTVComp[i] = 4;
				pDst += sizeof(fp4) * 4 * m_nV;
			}
			break;
		}
	}

	iVReg = CRC_VREG_NORMAL;
	if (m_Format.GetFormat(iVReg))
	{
		int SrcFmt = m_Format.GetFormat(iVReg);
		const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
		CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_V3_F32, sizeof(fp4)*3, NULL, m_nV);
		_DstBuffer.m_pN = (CVec3Dfp4*)pDst;
		pDst += sizeof(fp4) * 3 * m_nV;
	}


	iVReg = CRC_VREG_COLOR;
	if (m_Format.GetFormat(iVReg))
	{
		int SrcFmt = m_Format.GetFormat(iVReg);
		const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
		CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_N4_COL, sizeof(uint32), NULL, m_nV);
		_DstBuffer.m_pCol = (CPixel32*)pDst;
		pDst += sizeof(uint32) * m_nV;
	}

	iVReg = CRC_VREG_SPECULAR;
	if (m_Format.GetFormat(iVReg))
	{
		int SrcFmt = m_Format.GetFormat(iVReg);
		const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
		CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_N4_COL, sizeof(uint32), NULL, m_nV);
		_DstBuffer.m_pSpec = (CPixel32*)pDst;
		pDst += sizeof(uint32) * m_nV;
	}

	iVReg = CRC_VREG_MI0;
	if (m_Format.GetFormat(iVReg))
	{
		int SrcFmt = m_Format.GetFormat(iVReg);
		const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;

		_DstBuffer.m_pMI = (uint32*)pDst;
		if( m_Format.GetFormat(CRC_VREG_MI1))//(SrcFmt == CRC_VREGFMT_N8_UI8_P32_NORM) || (SrcFmt == CRC_VREGFMT_V8_F32) )
		{
			CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_N4_UI8_P32_NORM, sizeof(uint32), NULL, m_nV);
			pDst += sizeof(uint32) * m_nV * 2;
		}
		else
		{
			CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_N4_UI8_P32_NORM, sizeof(uint32), NULL, m_nV);
			pDst += sizeof(uint32) * m_nV;
		}
	}

	iVReg = CRC_VREG_MI1;
	if (m_Format.GetFormat(iVReg))
	{
		int SrcFmt = m_Format.GetFormat(iVReg);
		const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
		CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, _DstBuffer.m_pMI + m_nV, CRC_VREGFMT_N4_UI8_P32_NORM, sizeof(uint32), NULL, m_nV);
	}

	iVReg = CRC_VREG_MW0;
	bint bEnbale8 = (m_Format.GetFormat(CRC_VREG_MW1) != 0);
	switch (m_Format.GetFormat(iVReg))
	{
	case CRC_VREGFMT_V1_F32:
	case CRC_VREGFMT_V1_I16:
	case CRC_VREGFMT_V1_U16:
		{
			int SrcFmt = m_Format.GetFormat(iVReg);
			const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
			CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_V1_F32, sizeof(fp4)*1, NULL, m_nV);
			_DstBuffer.m_pMW = (fp4*)pDst;
			_DstBuffer.m_nMWComp = 1;
			pDst += sizeof(fp4) * 1 * m_nV;
		}
		break;
	case CRC_VREGFMT_V2_F32:
	case CRC_VREGFMT_V2_I16:
	case CRC_VREGFMT_V2_U16:
		{
			int SrcFmt = m_Format.GetFormat(iVReg);
			const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
			CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_V2_F32, sizeof(fp4)*2, NULL, m_nV);
			_DstBuffer.m_pMW = (fp4*)pDst;
			_DstBuffer.m_nMWComp = 2;
			pDst += sizeof(fp4) * 2 * m_nV;
		}
		break;
	case CRC_VREGFMT_V3_F32:
	case CRC_VREGFMT_V3_I16:
	case CRC_VREGFMT_V3_U16:
	case CRC_VREGFMT_NS3_I16:
	case CRC_VREGFMT_NS3_P32:
	case CRC_VREGFMT_NU3_P32:
		{
			int SrcFmt = m_Format.GetFormat(iVReg);
			const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
			CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_V3_F32, sizeof(fp4)*3, NULL, m_nV);
			_DstBuffer.m_pMW = (fp4*)pDst;
			_DstBuffer.m_nMWComp = 3;
			pDst += sizeof(fp4) * 3 * m_nV;
		}
		break;
	case CRC_VREGFMT_V4_F32:
	case CRC_VREGFMT_V4_I16:
	case CRC_VREGFMT_V4_U16:
	case CRC_VREGFMT_N4_UI8_P32_NORM:
	case CRC_VREGFMT_N4_UI8_P32:
	case CRC_VREGFMT_N4_COL:
		{
			int SrcFmt = m_Format.GetFormat(iVReg);
			const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
			CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_V4_F32, sizeof(fp4)*(4), NULL, m_nV);
			_DstBuffer.m_pMW = (fp4*)pDst;
			_DstBuffer.m_nMWComp = 4 + bEnbale8 * 4;
			pDst += sizeof(fp4) * (4 + bEnbale8 * 4) * m_nV;
		}
		break;
/*	case CRC_VREGFMT_V8_F32:
	case CRC_VREGFMT_N8_UI8_P32_NORM:
		{
			int SrcFmt = m_Format.GetFormat(iVReg);
			const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
			CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, pDst, CRC_VREGFMT_V4_F32, sizeof(fp4)*4, NULL, m_nV * 2);
			_DstBuffer.m_pMW = (fp4*)pDst;
			_DstBuffer.m_nMWComp = 8;
			pDst += sizeof(fp4) * 8 * m_nV;
		}
		break;*/
	}

	iVReg = CRC_VREG_MW1;
	if (m_Format.GetFormat(iVReg))
	{
		int SrcFmt = m_Format.GetFormat(iVReg);
		const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
		CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, _DstBuffer.m_pMW + 4 * m_nV, CRC_VREGFMT_V4_F32, sizeof(fp4)*4, NULL, m_nV);
	}
}

void CRC_BuildVertexBuffer::ConvertToInterleaved(void* _pDst, const CRC_VertexFormat& _DstFmt, CRC_VRegTransform* _pDstScale, uint32 _DestTransformEnable, int _nV)
{
	MAUTOSTRIP(CRC_BuildVertexBuffer_ConvertToInterleaved, MAUTOSTRIP_VOID);
	int DstStride = _DstFmt.GetStride();

	for(int iVReg = 0; iVReg < CRC_MAXVERTEXREG; iVReg++)
	{
		if (m_Format.GetFormat(iVReg) != CRC_VREGFMT_VOID)
		{
			int DstFormat = _DstFmt.GetFormat(iVReg);
			if (DstFormat == CRC_VREGFMT_VOID)
				Error_static("CRC_BuildVertexBuffer::ConvertToInterleaved", "Void destination format.");

			if (!m_lpVReg[iVReg])
				Error_static("CRC_BuildVertexBuffer::ConvertToInterleaved", "NULL source vertex register.");

			int SrcFmt = m_Format.GetFormat(iVReg);
			const CRC_VRegTransform* pSrcScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (m_TransformEnable & (1 << iVReg)) ? (&(m_lTransform[iVReg])) : NULL;
			const CRC_VRegTransform* pDstScale = (iVReg < CRC_MAXVERTEXREGSCALE) && (_DestTransformEnable & (1 << iVReg)) ? (&(_pDstScale[iVReg])) : NULL;
			CRC_VertexFormat::ConvertRegisterFormat(m_lpVReg[iVReg], SrcFmt, CRC_VertexFormat::GetRegisterSize(SrcFmt), pSrcScale, _pDst, DstFormat, DstStride, pDstScale, m_nV);

			(uint8*&)_pDst += _DstFmt.GetRegisterSize(DstFormat);
		}
	}
}

// -------------------------------------------------------------------
//  CRC_ClipStackEntry
// -------------------------------------------------------------------
CRC_ClipStackEntry::CRC_ClipStackEntry()
{
	MAUTOSTRIP(CRC_ClipStackEntry_ctor, MAUTOSTRIP_VOID);
	m_nPlanes = 0;
}

void CRC_ClipStackEntry::Copy(const CRC_ClipStackEntry& _Src, const CMat4Dfp4* _pTransform)
{
	MAUTOSTRIP(CRC_ClipStackEntry_Copy, MAUTOSTRIP_VOID);
	m_nPlanes = _Src.m_nPlanes;
	for(int i = 0; i < m_nPlanes; i++)
	{
		m_Planes[i] = _Src.m_Planes[i];
		if (_pTransform) m_Planes[i].Transform(*_pTransform);
	}
}

// -------------------------------------------------------------------
//  CRC_Core
// -------------------------------------------------------------------
CRC_Core::CRC_Core()
{
	MAUTOSTRIP(CRC_Core_ctor, MAUTOSTRIP_VOID);
	m_bShowGamma = false;
	m_bAllowTextureLoad = true;
	m_Caps_TextureFormats = 0;
	m_Caps_DisplayFormats = 0;
	m_Caps_ZFormats = 0;
	m_Caps_StencilDepth = 0;
	m_Caps_AlphaDepth = 0;
	m_Caps_Flags = 0;
	m_iTC = -1;
	m_pTC = NULL;
	m_iMatrixStack = 0;
	m_iMatrixMode = 0;
	m_MatrixChanged = -1;
//	FillChar(&m_MatrixUnit, sizeof(m_MatrixUnit), -1);
	m_AttribStackDepth = 0;
	m_iAttribStack = 0;
	m_iClipStack = 0;
	m_iVPStack = 0;
	m_bAddedToConsole = NULL;

	m_Caps_nMultiTexture = 1;
	m_Caps_nMultiTextureEnv = 1;
	m_Caps_nMultiTextureCoords = 1;

	m_pRegionContainer = NULL;
	m_pRegionManager = NULL;

//	FCollect Test = m_lCollectFunctionsAll[0];
//	FCollect Test2 = m_lCollectFunctionsIndexed[0];
};

CRC_Core::~CRC_Core()
{
	MAUTOSTRIP(CRC_Core_dtor, MAUTOSTRIP_VOID);
	if (m_bAddedToConsole) 
	{
		RemoveFromConsole();
		m_bAddedToConsole = false;
	}
//	g_pOS->m_spCon->RemoveSubSystem(this);
	m_pTC = NULL;
};

void CRC_Core::BeginScene(CRC_Viewport* _pVP)
{
	MAUTOSTRIP(CRC_Core_BeginScene, MAUTOSTRIP_VOID);
	m_nParticles = 0;
	m_nWires = 0;
	m_nTriangles = 0;
	m_nPolygons = 0;
	m_nClipVertices = 0;
	m_nClipTriangles = 0;
	m_nClipTrianglesDrawn = 0;

	m_lMatrixStack[0].Clear();
	m_iMatrixStack = 0;
	m_iMatrixMode = 0;

	m_iAttribStack = 0;
	m_iClipStack = 0;
	DRenderTopClass Clip_Clear();
	m_lAttribStack[0].SetDefault();
	m_lAttribStackChanged[0] = 0;

	m_iVPStack = 0;
	DRenderTopClass Viewport_Set(_pVP);

	m_MatrixChanged = -1;
//	DRenderCoreTopClass Matrix_Update();
//	m_MatrixChanged = -1;

	DRenderTopClass Geometry_Clear();

//	BS_Init();
};

void CRC_Core::EndScene()
{
	MAUTOSTRIP(CRC_Core_EndScene, MAUTOSTRIP_VOID);
};


void CRC_Core::PreEndScene()
{
	MAUTOSTRIP(CRC_Core_PreEndScene, MAUTOSTRIP_VOID);
	if (m_bShowGamma)
	{
		CPixel32 C[4];
		CVec3Dfp4 V[4];
		static int lBarColorMask[4] = { 0xffffffff, 0xffff0000, 0xff00ff00, 0xff0000ff };
		static uint16 lTriPrim[6] = { 0,2,1,1,2,3 };

		for(int i = 0; i < 4; i++)
		{
			int Col = lBarColorMask[i];
			C[0] = 0;
			C[1] = 0;
			C[2] = Col;
			C[3] = Col;
			V[0] = CVec3Dfp4(-100, 25 + i*10, 100);
			V[1] = CVec3Dfp4(-100, 20 + i*10, 100);
			V[2] = CVec3Dfp4(100, 25 + i*10, 100);
			V[3] = CVec3Dfp4(100, 20 + i*10, 100);

			DRenderTopClass Geometry_Clear();
			DRenderTopClass Geometry_VertexArray(V, 4, true);
			DRenderTopClass Geometry_ColorArray(C);

			DRenderTopClass Render_IndexedTriangles(lTriPrim, 2);
			DRenderTopClass Geometry_Clear();
		}
	}
}

void CRC_Core::Create(CObj* _pContext, const char* _pParams)
{
	MAUTOSTRIP(CRC_Core_Create, MAUTOSTRIP_VOID);
	MSCOPESHORT(CRC_Core::Create);
	// Get TextureContext
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("CRC_Core", "No texture-context found.");
	m_pTC = pTC;

	// Get VB Context
	MACRO_GetRegisterObject(CXR_VBContext, pVBCtx, "SYSTEM.VBCONTEXT");
	if (!pVBCtx) Error("CRC_Core", "No VB-context found.");
	m_pVBCtx = pVBCtx;

	AddToConsole();
	m_bAddedToConsole = true;

	m_lMatrixStack.SetLen(CRC_MATRIXSTACKDEPTH);

	m_AttribStackDepth = CRC_ATTRIBSTACKDEPTH;

#ifdef RC_DYNAMIC_ATTRIB_STACK
	m_lAttribStack.SetLen(CRC_ATTRIBSTACKDEPTH);
	m_lAttribStackChanged.SetLen(CRC_ATTRIBSTACKDEPTH);
	FillChar(m_lAttribStackChanged.GetBasePtr(), m_lAttribStackChanged.ListSize(), 0);
#else
	FillChar(m_lAttribStackChanged, sizeof(m_lAttribStackChanged), 0);
#endif
	m_iAttribStack = 0;
	m_lAttribStack[0].SetDefault();

	m_lClipStack.SetLen(CRC_CLIPSTACK);
	m_iClipStack = 0;

	m_lVPStack.SetLen(16 /*CRC_VIEWPORTSTACKDEPTH*/);
	m_iVPStack = 0;

	m_spTCIDInfo = MNew1(CRC_TCIDInfo, m_pTC->GetIDCapacity());
	m_lVBIDInfo.SetLen(m_pVBCtx->GetIDCapacity());
}

void CRC_Core::Viewport_Update()
{
	MAUTOSTRIP(CRC_Core_Viewport_Update, MAUTOSTRIP_VOID);
	CRC_Viewport* pVP = Viewport_Get();
	m_BackPlane = pVP->GetBackPlane();
	m_FrontPlane = pVP->GetFrontPlane();
	m_BackPlaneInv = 1.0f / m_BackPlane;
	m_FrontPlaneInv = 1.0f / m_FrontPlane;
}

CRC_Viewport* CRC_Core::Viewport_Get()
{
	MAUTOSTRIP(CRC_Core_Viewport_Get, NULL);
	return &m_lVPStack[m_iVPStack];
}

void CRC_Core::Viewport_Set(CRC_Viewport* _pVP)
{
	MAUTOSTRIP(CRC_Core_Viewport_Set, MAUTOSTRIP_VOID);
	m_lVPStack[m_iVPStack] = *_pVP;
	DRenderTopClass Viewport_Update();
}

class CRasterModePair
{
public:
	uint8 m_Src;
	uint8 m_Dst;
};

const static CRasterModePair gs_Pairs[] = 
{
	{CRC_BLEND_ONE, CRC_BLEND_ZERO},
	{CRC_BLEND_SRCALPHA, CRC_BLEND_INVSRCALPHA},
	{CRC_BLEND_ZERO, CRC_BLEND_INVSRCCOLOR},
	{CRC_BLEND_DESTCOLOR, CRC_BLEND_ZERO},
	{CRC_BLEND_ONE, CRC_BLEND_ONE},
	{CRC_BLEND_SRCALPHA, CRC_BLEND_ONE},
	{CRC_BLEND_DESTCOLOR, CRC_BLEND_INVSRCALPHA},
	{CRC_BLEND_DESTCOLOR, CRC_BLEND_SRCCOLOR},
	{CRC_BLEND_DESTCOLOR, CRC_BLEND_ONE},
	{CRC_BLEND_ONE, CRC_BLEND_SRCALPHA},
	{CRC_BLEND_ONE, CRC_BLEND_INVSRCALPHA},
	{CRC_BLEND_DESTALPHA, CRC_BLEND_INVDESTALPHA}
};

M_INLINE bint Attrib_SetRasterMode_Inl(CRC_Attributes* _pA, int _RasterMode)
{
	if (_RasterMode >= 12) // Invalid raster mode
		return false;

	M_ASSERT(_RasterMode >= 0 && _RasterMode < (sizeof(gs_Pairs) / sizeof(CRasterModePair)), "RasterMode new ?");
	_pA->m_Flags |= CRC_FLAGS_BLEND;
	if (_pA->m_SourceBlend != gs_Pairs[_RasterMode].m_Src)
	{
		_pA->m_SourceBlend = gs_Pairs[_RasterMode].m_Src;
		_pA->m_DestBlend = gs_Pairs[_RasterMode].m_Dst;
		return true;	
	}
	if (_pA->m_DestBlend != gs_Pairs[_RasterMode].m_Dst)
	{
		_pA->m_DestBlend = gs_Pairs[_RasterMode].m_Dst;
		return true;	
	}
	return false;	
}
    
bint CRC_Core::Attrib_SetRasterMode(CRC_Attributes* _pA, int _RasterMode)
{
	if (_RasterMode != CRC_RASTERMODE_NONE)
		return Attrib_SetRasterMode_Inl(_pA, _RasterMode);		
	return false;
}

	void CRC_Core::Internal_RenderPolygon(int _nV, const CVec3Dfp4* _pV, const CVec3Dfp4* _pN, const CVec4Dfp4* _pCol, const CVec4Dfp4* _pSpec , 
//const fp4* _pFog = NULL,
const CVec4Dfp4* _pTV0, const CVec4Dfp4* _pTV1 , const CVec4Dfp4* _pTV2 , const CVec4Dfp4* _pTV3 , int _Color){};
	void CRC_Core::Attrib_Set(CRC_Attributes* _pAttrib){};
	void CRC_Core::Matrix_SetRender(int _iMode, const CMat4Dfp4* _pMatrix){};


void CRC_Core::Viewport_Push()
{
	MAUTOSTRIP(CRC_Core_Viewport_Push, MAUTOSTRIP_VOID);
	if (m_iVPStack+1 >= m_lVPStack.Len()) Error("Viewport_Push", "Viewport stack overflow.");
	m_lVPStack[m_iVPStack+1] = m_lVPStack[m_iVPStack];
	m_iVPStack++;
}

void CRC_Core::Viewport_Pop()
{
	MAUTOSTRIP(CRC_Core_Viewport_Pop, MAUTOSTRIP_VOID);
	if (!m_iVPStack) Error("Viewport_Pop", "Viewport stack underflow.");
	m_iVPStack--;
	DRenderTopClass Viewport_Update();
}

void CRC_Core::RenderTarget_Copy(CRct _SrcRect, CPnt _Dest, int _CopyType)
{
	// nop
}

void CRC_Core::RenderTarget_CopyToTexture(int _TextureID, CRct _SrcRect, CPnt _Dest, bint _bContinueTiling, uint16 _Slice)
{
	// nop
}

void CRC_Core::Render_EnableHardwareMemoryRegion(CXR_VBManager *_pManager, CXR_VBMContainer *_pContainer)
{
	m_pRegionManager = _pManager;
	m_pRegionContainer = _pContainer;
}

void CRC_Core::Render_DisableHardwareMemoryRegion()
{
	if (m_pRegionManager)
	{
		m_pRegionContainer->AddAvailVBM(m_pRegionManager);
		m_pRegionContainer = NULL;
		m_pRegionManager = NULL;
	}
}


// -------------------------------------------------------------------
CTextureContext* CRC_Core::Texture_GetTC()
{
	MAUTOSTRIP(CRC_Core_Texture_GetTC, NULL);
	return m_pTC;
}

CRC_TCIDInfo* CRC_Core::Texture_GetTCIDInfo()
{
	MAUTOSTRIP(CRC_Core_Texture_GetTCIDInfo, NULL);
	return m_spTCIDInfo;
}

void CRC_Core::Texture_Precache(int _TextureID)
{
	MAUTOSTRIP(CRC_Core_Texture_Precache, MAUTOSTRIP_VOID);
}

void CRC_Core::Texture_Copy(int _SourceTexID, int _DestTexID, CRct _SrcRgn, CPnt _DstPos)
{
	MAUTOSTRIP(CRC_Core_Texture_Copy, MAUTOSTRIP_VOID);
}

CRC_TextureMemoryUsage CRC_Core::Texture_GetMem(int _TextureID)
{
	MAUTOSTRIP(CRC_Core_Texture_GetMem, 0);
	return CRC_TextureMemoryUsage();
}

int CRC_Core::Texture_GetPicmipFromGroup(int _iPicmip)
{
	MAUTOSTRIP(CRC_Core_Texture_GetPicmipFromGroup, 0);
	return 0;
}

void CRC_Core::Texture_MakeAllDirty(int _iPicMip)
{
	MAUTOSTRIP(CRC_Core_Texture_MakeAllDirty, MAUTOSTRIP_VOID);
	CRC_IDInfo* pIDInfo = m_spTCIDInfo->m_pTCIDInfo;
	int nTxt = m_pTC->GetIDCapacity();

	if (_iPicMip < 0)
	{
		for(int i = 0; i < nTxt; i++)
			pIDInfo[i].m_Fresh &= ~1;
	}
	else
	{
		for(int i = 0; i < nTxt; i++)
			if (m_pTC->IsValidID(i))
			{
				CTC_TextureProperties Prop;
				m_pTC->GetTextureProperties(i, Prop);
				if (Prop.m_iPicMipGroup == _iPicMip &&
					!(Prop.m_Flags & (CTC_TEXTUREFLAGS_NOPICMIP | CTC_TEXTUREFLAGS_RENDER)))
					pIDInfo[i].m_Fresh &= ~1;
			}
	}
}

// -------------------------------------------------------------------
// VertexBuffer stuff
CXR_VBContext* CRC_Core::VB_GetVBContext()
{
	MAUTOSTRIP(CRC_Core_VB_GetVBContext, NULL);
	return m_pVBCtx;
}

CRC_VBIDInfo* CRC_Core::VB_GetVBIDInfo()
{
	MAUTOSTRIP(CRC_Core_VB_GetVBIDInfo, NULL);
	return m_lVBIDInfo.GetBasePtr();
}

// -------------------------------------------------------------------
//  Attribute
// -------------------------------------------------------------------
void CRC_Core::Attrib_Update()
{
	MAUTOSTRIP(CRC_Core_Attrib_Update, MAUTOSTRIP_VOID);
	if (m_AttribChanged)
	{
		m_lAttribStackChanged[m_iAttribStack] |= m_AttribChanged;
		DRenderCoreTopClass Attrib_Set(&m_lAttribStack[m_iAttribStack]);
	}
}

void CRC_Core::Attrib_GlobalUpdate()
{
	MAUTOSTRIP(CRC_Core_Attrib_GlobalUpdate, MAUTOSTRIP_VOID);
}

void CRC_Core::Attrib_SetAbsolute(CRC_Attributes* _pAttrib)
{
	m_AttribChanged = -1;
	DRenderCoreTopClass Attrib_Set(_pAttrib);
}

void CRC_Core::Attrib_Push()
{
	MAUTOSTRIP(CRC_Core_Attrib_Push, MAUTOSTRIP_VOID);
//	OutputDebugString(CStrF("Attrib_Push %d\n", m_iAttribStack).Str()); 
	if (m_iAttribStack >= m_AttribStackDepth - 1) 
		Error("Attrib_Push", "Attribute stack overflow.");
	m_iAttribStack++;
	m_lAttribStack[m_iAttribStack] = m_lAttribStack[m_iAttribStack-1];
	m_lAttribStackChanged[m_iAttribStack] = 0;
}

void CRC_Core::Attrib_Pop()
{
	MAUTOSTRIP(CRC_Core_Attrib_Pop, MAUTOSTRIP_VOID);
//	OutputDebugString(CStrF("Attrib_Pop %d\n", m_iAttribStack).Str()); 
	if (m_iAttribStack <= 0) 
		Error("Attrib_Pop", "Attribute stack underflow.");
//	m_AttribChanged |= m_lAttribStackChanged[m_iAttribStack];
	m_AttribChanged = -1;
	m_iAttribStack--;
}

void CRC_Core::Attrib_Set(const CRC_Attributes& _Attrib)
{
	MAUTOSTRIP(CRC_Core_Attrib_Set, MAUTOSTRIP_VOID);
	CRC_Attributes& A = m_lAttribStack[m_iAttribStack];

	if (A.m_AttribLock)
	{
		// Not extremely efficient, but on the other hand we don't have to flags everything as changed.
		DRenderTopClass Attrib_Disable(-1);
		DRenderTopClass Attrib_Enable(_Attrib.m_Flags);
		DRenderTopClass Attrib_RasterMode(_Attrib.m_RasterMode);
		DRenderTopClass Attrib_SourceBlend(_Attrib.m_SourceBlend);
		DRenderTopClass Attrib_DestBlend(_Attrib.m_DestBlend);
		DRenderTopClass Attrib_ZCompare(_Attrib.m_ZCompare);
		DRenderTopClass Attrib_AlphaCompare(_Attrib.m_AlphaCompare, _Attrib.m_AlphaRef);
#ifndef	PLATFORM_PS2
		DRenderTopClass Attrib_StencilRef(_Attrib.m_StencilRef, _Attrib.m_StencilFuncAnd);
		DRenderTopClass Attrib_StencilFrontOp(_Attrib.m_StencilFrontFunc, _Attrib.m_StencilFrontOpFail, _Attrib.m_StencilFrontOpZFail, _Attrib.m_StencilFrontOpZPass);
		DRenderTopClass Attrib_StencilBackOp(_Attrib.m_StencilBackFunc, _Attrib.m_StencilBackOpFail, _Attrib.m_StencilBackOpZFail, _Attrib.m_StencilBackOpZPass);
		DRenderTopClass Attrib_Scissor(_Attrib.m_Scissor);
#endif	// PLATFORM_PS2
		DRenderTopClass Attrib_FogStart(_Attrib.m_FogStart);
		DRenderTopClass Attrib_FogEnd(_Attrib.m_FogEnd);
		DRenderTopClass Attrib_FogDensity(_Attrib.m_FogDensity);
		DRenderTopClass Attrib_FogColor(_Attrib.m_FogColor);
		DRenderTopClass Attrib_PolygonOffset(_Attrib.m_PolygonOffsetScale, _Attrib.m_PolygonOffsetUnits);
		DRenderTopClass Attrib_Lights(_Attrib.m_pLights, _Attrib.m_nLights);

		{
			for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
			{
				DRenderTopClass Attrib_TexGen(i, _Attrib.m_lTexGenMode[i], _Attrib.GetTexGenComp(i));
			}
			DRenderTopClass Attrib_TexGenAttr(_Attrib.m_pTexGenAttr);
		}
		DRenderTopClass Attrib_TexGenAttr(_Attrib.m_pTexGenAttr);

		{
			for(int i = 0; i < CRC_MAXTEXTURES; i++)
			{
				DRenderTopClass Attrib_TextureID(i, _Attrib.m_TextureID[i]);
	//			Attrib_TexEnvColor(i, _Attrib.m_TexEnvColor[i]);
	/*			Attrib_TexGen(i, _Attrib.m_TexGenMode[i], 
					_Attrib.m_pTexGenParams[i][0], 
					_Attrib.m_pTexGenParams[i][1], 
					_Attrib.m_pTexGenParams[i][2], 
					_Attrib.m_pTexGenParams[i][3]);*/

				// iTexCoordSet has no ATTRCHG flag since it is a geometry state rather than a attribute.
				A.m_iTexCoordSet[i] = _Attrib.m_iTexCoordSet[i];	
			}
		}
		{
			for(int i = 0; i < CRC_MAXTEXTUREENV; i++)
			{
				DRenderTopClass Attrib_TexEnvMode(i, _Attrib.m_TexEnvMode[i]);
			}
		}

		// NOTE: Set locks last. They are assumed to lock attribute changes after this attribute change.
		DRenderTopClass Attrib_Lock(_Attrib.m_AttribLock);
		DRenderTopClass Attrib_LockFlags(_Attrib.m_AttribLockFlags);
	}
	else if (A.m_AttribLockFlags)
	{
		int Flags = A.m_Flags;
		A = _Attrib;
		A.m_Flags = Flags;
		DRenderTopClass Attrib_Disable(-1);
		DRenderTopClass Attrib_Enable(_Attrib.m_Flags);
		m_AttribChanged = -1;
	}
	else
	{
		A = _Attrib;
		m_AttribChanged = -1;
	}
}

void CRC_Core::Attrib_Get(CRC_Attributes& _Attrib) const
{
	MAUTOSTRIP(CRC_Core_Attrib_Get, MAUTOSTRIP_VOID);
	_Attrib = m_lAttribStack[m_iAttribStack];
}

const char * CRC_Core::GetRenderingStatus()
{
	return "";
}

CRC_Attributes* CRC_Core::Attrib_Begin()
{
	MAUTOSTRIP(CRC_Core_Attrib_Begin, MAUTOSTRIP_VOID);
	return &m_lAttribStack[m_iAttribStack];
}

void CRC_Core::Attrib_End(uint _ChgFlags)
{
	m_AttribChanged |= _ChgFlags;
}

void CRC_Core::Attrib_Lock(int _Flags)
{
	MAUTOSTRIP(CRC_Core_Attrib_Lock, MAUTOSTRIP_VOID);
	m_lAttribStack[m_iAttribStack].m_AttribLock |= _Flags;
}

void CRC_Core::Attrib_LockFlags(int _Flags)
{
	MAUTOSTRIP(CRC_Core_Attrib_LockFlags, MAUTOSTRIP_VOID);
	m_lAttribStack[m_iAttribStack].m_AttribLockFlags |= _Flags;
}

void CRC_Core::Attrib_Enable(int _Flags)
{
	MAUTOSTRIP(CRC_Core_Attrib_Enable, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_FLAGS)
		return;

	_Flags &= ~m_lAttribStack[m_iAttribStack].m_AttribLockFlags;

	int OldFlags = m_lAttribStack[m_iAttribStack].m_Flags;
	m_lAttribStack[m_iAttribStack].m_Flags |= _Flags;
	if (m_lAttribStack[m_iAttribStack].m_Flags != OldFlags)
		m_AttribChanged |= CRC_ATTRCHG_FLAGS;
}

void CRC_Core::Attrib_Disable(int _Flags)
{
	MAUTOSTRIP(CRC_Core_Attrib_Disable, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_FLAGS)
		return;

	_Flags &= ~m_lAttribStack[m_iAttribStack].m_AttribLockFlags;

	int OldFlags = m_lAttribStack[m_iAttribStack].m_Flags;
	m_lAttribStack[m_iAttribStack].m_Flags &= (-1 - _Flags);
	if (m_lAttribStack[m_iAttribStack].m_Flags != OldFlags)
		m_AttribChanged |= CRC_ATTRCHG_FLAGS;
}

void CRC_Core::Attrib_Switch(int _Flags)
{
	MAUTOSTRIP(CRC_Core_Attrib_Switch, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_FLAGS)
		return;

	_Flags &= ~m_lAttribStack[m_iAttribStack].m_AttribLockFlags;
	if (!_Flags)
		return;

	m_lAttribStack[m_iAttribStack].m_Flags ^= _Flags;
	m_AttribChanged |= CRC_ATTRCHG_FLAGS;
}

void CRC_Core::Attrib_ZCompare(int _Compare)
{
	MAUTOSTRIP(CRC_Core_Attrib_ZCompare, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_ZCOMPARE)
		return;
	if (m_lAttribStack[m_iAttribStack].m_ZCompare == _Compare) return;
	m_lAttribStack[m_iAttribStack].m_ZCompare = _Compare;
	m_AttribChanged |= CRC_ATTRCHG_ZCOMPARE;
}

void CRC_Core::Attrib_AlphaCompare(int _Compare, int _AlphaRef)
{
	MAUTOSTRIP(CRC_Core_Attrib_AlphaCompare, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_ALPHACOMPARE)
		return;
	if ((m_lAttribStack[m_iAttribStack].m_AlphaCompare == _Compare) &&
		(m_lAttribStack[m_iAttribStack].m_AlphaRef == _AlphaRef)) return;
	m_lAttribStack[m_iAttribStack].m_AlphaCompare = _Compare;
	m_lAttribStack[m_iAttribStack].m_AlphaRef = _AlphaRef;
	m_AttribChanged |= CRC_ATTRCHG_ALPHACOMPARE;
}

void CRC_Core::Attrib_StencilRef(int _Ref, int _FuncAnd)
{
	MAUTOSTRIP(CRC_Core_Attrib_StencilFunction, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_PS2
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_STENCIL)
		return;
/*	if ((m_lAttribStack[m_iAttribStack].m_StencilFunc == _Func) &&
		(m_lAttribStack[m_iAttribStack].m_StencilRef == _Ref) &&
		(m_lAttribStack[m_iAttribStack].m_StencilFuncAnd == _FuncAnd)) return;
*/
	m_lAttribStack[m_iAttribStack].m_StencilRef = _Ref;
	m_lAttribStack[m_iAttribStack].m_StencilFuncAnd = _FuncAnd;

	m_AttribChanged |= CRC_ATTRCHG_STENCIL;
#endif
}

void CRC_Core::Attrib_StencilWriteMask(int _Mask)
{
	MAUTOSTRIP(CRC_Core_Attrib_StencilWriteMask, MAUTOSTRIP_VOID);
/*#ifndef	PLATFORM_PS2
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_STENCIL)
		return;
	if (m_lAttribStack[m_iAttribStack].m_StencilWriteMask == _Mask) return;

	m_lAttribStack[m_iAttribStack].m_StencilWriteMask = _Mask;
	m_AttribChanged |= CRC_ATTRCHG_STENCIL;
#endif	// PLATFORM_PS2*/
}

void CRC_Core::Attrib_StencilFrontOp(int _Func, int _OpFail, int _OpZFail, int _OpZPass)
{
	MAUTOSTRIP(CRC_Core_Attrib_StencilOp, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_PS2
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_STENCIL)
		return;
/*	if ((m_lAttribStack[m_iAttribStack].m_StencilOpFail == _OpFail) &&
		(m_lAttribStack[m_iAttribStack].m_StencilOpZFail == _OpZFail) &&
		(m_lAttribStack[m_iAttribStack].m_StencilOpZPass == _OpZPass)) return;
*/
	m_lAttribStack[m_iAttribStack].m_StencilFrontFunc = _Func;
	m_lAttribStack[m_iAttribStack].m_StencilFrontOpFail = _OpFail;
	m_lAttribStack[m_iAttribStack].m_StencilFrontOpZFail = _OpZFail;
	m_lAttribStack[m_iAttribStack].m_StencilFrontOpZPass = _OpZPass;

	m_AttribChanged |= CRC_ATTRCHG_STENCIL;
#endif	// PLATFORM_PS2
}

void CRC_Core::Attrib_StencilBackOp(int _Func, int _OpFail, int _OpZFail, int _OpZPass)
{
	MAUTOSTRIP(CRC_Core_Attrib_StencilOp, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_PS2
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_STENCIL)
		return;
/*	if ((m_lAttribStack[m_iAttribStack].m_StencilOpFail == _OpFail) &&
		(m_lAttribStack[m_iAttribStack].m_StencilOpZFail == _OpZFail) &&
		(m_lAttribStack[m_iAttribStack].m_StencilOpZPass == _OpZPass)) return;
*/
	m_lAttribStack[m_iAttribStack].m_StencilBackFunc = _Func;
	m_lAttribStack[m_iAttribStack].m_StencilBackOpFail = _OpFail;
	m_lAttribStack[m_iAttribStack].m_StencilBackOpZFail = _OpZFail;
	m_lAttribStack[m_iAttribStack].m_StencilBackOpZPass = _OpZPass;

	m_AttribChanged |= CRC_ATTRCHG_STENCIL;
#endif	// PLATFORM_PS2
}

void CRC_Core::Attrib_RasterMode(int _RasterMode)
{
	MAUTOSTRIP(CRC_Core_Attrib_RasterMode, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_RASTERMODE)
		return;
	if (m_lAttribStack[m_iAttribStack].m_RasterMode == _RasterMode) return;
	CRC_Attributes* pA = &m_lAttribStack[m_iAttribStack];
	pA->m_RasterMode = _RasterMode;
//	pA->ClearRasterModeSettings();
//	Attrib_SetRasterMode(pA, _RasterMode);
	m_AttribChanged |= CRC_ATTRCHG_RASTERMODE | CRC_ATTRCHG_BLEND | CRC_ATTRCHG_FLAGS;
}

void CRC_Core::Attrib_SourceBlend(int _Blend)
{
	MAUTOSTRIP(CRC_Core_Attrib_SourceBlend, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_BLEND)
		return;
	if (m_lAttribStack[m_iAttribStack].m_RasterMode) return;
	if (m_lAttribStack[m_iAttribStack].m_SourceBlend == _Blend) return;
	m_lAttribStack[m_iAttribStack].m_SourceBlend = _Blend;
	m_AttribChanged |= CRC_ATTRCHG_BLEND;
}

void CRC_Core::Attrib_DestBlend(int _Blend)
{
	MAUTOSTRIP(CRC_Core_Attrib_DestBlend, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_BLEND)
		return;
	if (m_lAttribStack[m_iAttribStack].m_RasterMode) return;
	if (m_lAttribStack[m_iAttribStack].m_DestBlend == _Blend) return;
	m_lAttribStack[m_iAttribStack].m_DestBlend = _Blend;
	m_AttribChanged |= CRC_ATTRCHG_BLEND;
}

void CRC_Core::Attrib_FogColor(CPixel32 _FogColor)
{
	MAUTOSTRIP(CRC_Core_Attrib_FogColor, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_FOG)
		return;
	if (m_lAttribStack[m_iAttribStack].m_FogColor == _FogColor) return;
	m_lAttribStack[m_iAttribStack].m_FogColor = _FogColor;
	m_AttribChanged |= CRC_ATTRCHG_FOG;
}

void CRC_Core::Attrib_FogStart(fp4 _FogStart)
{
	MAUTOSTRIP(CRC_Core_Attrib_FogStart, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_FOG)
		return;
	if (m_lAttribStack[m_iAttribStack].m_FogStart == _FogStart) return;
	m_lAttribStack[m_iAttribStack].m_FogStart = _FogStart;
	m_AttribChanged |= CRC_ATTRCHG_FOG;
}

void CRC_Core::Attrib_FogEnd(fp4 _FogEnd)
{
	MAUTOSTRIP(CRC_Core_Attrib_FogEnd, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_FOG)
		return;
	if (m_lAttribStack[m_iAttribStack].m_FogEnd == _FogEnd) return;
	m_lAttribStack[m_iAttribStack].m_FogEnd = _FogEnd;
	m_AttribChanged |= CRC_ATTRCHG_FOG;
}

void CRC_Core::Attrib_FogDensity(fp4 _FogDensity)
{
	MAUTOSTRIP(CRC_Core_Attrib_FogDensity, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_FOG)
		return;
	if (m_lAttribStack[m_iAttribStack].m_FogDensity == _FogDensity) return;
	m_lAttribStack[m_iAttribStack].m_FogDensity = _FogDensity;
	m_AttribChanged |= CRC_ATTRCHG_FOG;
}

void CRC_Core::Attrib_PolygonOffset(fp4 _Scale, fp4 _Offset)
{
	MAUTOSTRIP(CRC_Core_Attrib_PolygonOffset, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_POLYGONOFFSET)
		return;
	if ((m_lAttribStack[m_iAttribStack].m_PolygonOffsetScale == _Scale) &&
		(m_lAttribStack[m_iAttribStack].m_PolygonOffsetUnits == _Offset)) return;
	m_lAttribStack[m_iAttribStack].m_PolygonOffsetScale = _Scale;
	m_lAttribStack[m_iAttribStack].m_PolygonOffsetUnits = _Offset;
	m_AttribChanged |= CRC_ATTRCHG_POLYGONOFFSET;
	DRenderTopClass Attrib_Enable(CRC_FLAGS_POLYGONOFFSET);
}

void CRC_Core::Attrib_Scissor(const CRect2Duint16& _Scissor)
{
	MAUTOSTRIP(CRC_Core_Attrib_Scissor, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_PS2
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_SCISSOR)
		return;
	if ((m_lAttribStack[m_iAttribStack].m_Scissor.m_Min == _Scissor.m_Min) &&
		(m_lAttribStack[m_iAttribStack].m_Scissor.m_Max == _Scissor.m_Max)) return;
	m_lAttribStack[m_iAttribStack].m_Scissor = _Scissor;
	m_AttribChanged |= CRC_ATTRCHG_SCISSOR;
#endif	// PLATFORM_PS2
}

void CRC_Core::Attrib_Lights(const CRC_Light* _pLights, int _nLights)
{
	MAUTOSTRIP(CRC_Core_Attrib_Lights, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_OTHER)
		return;
	if ((m_lAttribStack[m_iAttribStack].m_pLights == _pLights) &&
		(m_lAttribStack[m_iAttribStack].m_nLights == _nLights)) return;
	m_lAttribStack[m_iAttribStack].m_pLights = _pLights;
	m_lAttribStack[m_iAttribStack].m_nLights = _nLights;
	m_AttribChanged |= CRC_ATTRCHG_OTHER;
}

void CRC_Core::Attrib_TextureID(int _iTxt, int _TextureID)
{
	MAUTOSTRIP(CRC_Core_Attrib_TextureID, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_TEXTUREID)
		return;
	if (m_lAttribStack[m_iAttribStack].m_TextureID[_iTxt] == _TextureID) return;
	m_lAttribStack[m_iAttribStack].m_TextureID[_iTxt] = _TextureID;
	m_AttribChanged |= CRC_ATTRCHG_TEXTUREID;
}

void CRC_Core::Attrib_TexEnvMode(int _iTxt, int _TexEnvMode)
{
	MAUTOSTRIP(CRC_Core_Attrib_TexEnvMode, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_TEXENVMODE)
		return;
	if (m_lAttribStack[m_iAttribStack].m_TexEnvMode[_iTxt] == _TexEnvMode) return;
	m_lAttribStack[m_iAttribStack].m_TexEnvMode[_iTxt] = _TexEnvMode;
	m_AttribChanged |= CRC_ATTRCHG_TEXENVMODE;
}


/*void CRC_Core::Attrib_TexEnvColor(int _iTxt, CPixel32 _TexEnvColor)
{
	MAUTOSTRIP(CRC_Core_Attrib_TexEnvColor, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_TEXENVCOLOR)
		return;
	if (m_lAttribStack[m_iAttribStack].m_TexEnvColor[_iTxt] == _TexEnvColor) return;
	m_lAttribStack[m_iAttribStack].m_TexEnvColor[_iTxt] = _TexEnvColor;
	m_AttribChanged |= CRC_ATTRCHG_TEXENVCOLOR;
}*/

void CRC_Core::Attrib_TexGen(int _iTxt, int _TexGenMode, int _Comp)
{
	MAUTOSTRIP(CRC_Core_Attrib_TexGen, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_TEXGEN)
		return;
	CRC_Attributes& Attr = m_lAttribStack[m_iAttribStack];
	if (Attr.m_lTexGenMode[_iTxt] == _TexGenMode)
		return;
	Attr.m_lTexGenMode[_iTxt] = _TexGenMode;
	m_AttribChanged |= CRC_ATTRCHG_TEXGEN;
}

void CRC_Core::Attrib_TexGenAttr(fp4* _pTexGenAttr)
{
	MAUTOSTRIP(CRC_Core_Attrib_TexGen, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_TEXGEN)
		return;
	CRC_Attributes& Attr = m_lAttribStack[m_iAttribStack];
	if (Attr.m_pTexGenAttr == _pTexGenAttr)
		return;
	Attr.m_pTexGenAttr = _pTexGenAttr;
	m_AttribChanged |= CRC_ATTRCHG_TEXGEN;
}


void CRC_Core::Attrib_VBFlags(uint32 _Flags)
{
	MAUTOSTRIP(CRC_Core_Attrib_VBFlags, MAYTOSTRIP_VOID);
	uint32 & AFlags = m_lAttribStack[m_iAttribStack].m_VBFlags;
	if (AFlags == _Flags) return;
	
	AFlags = _Flags;
	m_AttribChanged |= CRC_ATTRCHG_VB_FLAGS;
}

/*
void CRC_Core::Attrib_TexGen(int _iTxt, int _TexGenMode, fp4* _pParamsU, fp4* _pParamsV, fp4* _pParamsW, fp4* _pParamsQ)
{
	MAUTOSTRIP(CRC_Core_Attrib_TexGen, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_TEXGEN)
		return;
	CRC_Attributes& Attr = m_lAttribStack[m_iAttribStack];
	if (Attr.m_TexGenMode[_iTxt] == _TexGenMode &&
		Attr.m_pTexGenParams[_iTxt][0] == _pParamsU &&
		Attr.m_pTexGenParams[_iTxt][1] == _pParamsV &&
		Attr.m_pTexGenParams[_iTxt][2] == _pParamsW &&
		Attr.m_pTexGenParams[_iTxt][3] == _pParamsQ) return;
	Attr.m_TexGenMode[_iTxt] = _TexGenMode;
	Attr.m_pTexGenParams[_iTxt][0] = _pParamsU;
	Attr.m_pTexGenParams[_iTxt][1] = _pParamsV;
	Attr.m_pTexGenParams[_iTxt][2] = _pParamsW;
	Attr.m_pTexGenParams[_iTxt][3] = _pParamsQ;
	m_AttribChanged |= CRC_ATTRCHG_TEXGEN;
}

void CRC_Core::Attrib_TexGenU(int _iTxt, int _TexGenMode, fp4* _pParams)
{
	MAUTOSTRIP(CRC_Core_Attrib_TexGenU, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_TEXGEN)
		return;
	CRC_Attributes& Attr = m_lAttribStack[m_iAttribStack];
	if ((Attr.m_TexGenMode[_iTxt] & CRC_TEXGENMODE_MASK_U) == (_TexGenMode << CRC_TEXGENMODE_SHIFT_U) &&
		Attr.m_pTexGenParams[_iTxt][0] == _pParams) return;
	Attr.m_TexGenMode[_iTxt] = _TexGenMode;
	Attr.m_pTexGenParams[_iTxt][0] = _pParams;
	m_AttribChanged |= CRC_ATTRCHG_TEXGEN;
}

void CRC_Core::Attrib_TexGenV(int _iTxt, int _TexGenMode, fp4* _pParams)
{
	MAUTOSTRIP(CRC_Core_Attrib_TexGenV, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_TEXGEN)
		return;
	CRC_Attributes& Attr = m_lAttribStack[m_iAttribStack];
	if ((Attr.m_TexGenMode[_iTxt] & CRC_TEXGENMODE_MASK_V) == (_TexGenMode << CRC_TEXGENMODE_SHIFT_V) &&
		Attr.m_pTexGenParams[_iTxt][1] == _pParams) return;
	Attr.m_TexGenMode[_iTxt] = _TexGenMode;
	Attr.m_pTexGenParams[_iTxt][1] = _pParams;
	m_AttribChanged |= CRC_ATTRCHG_TEXGEN;
}

void CRC_Core::Attrib_TexGenW(int _iTxt, int _TexGenMode, fp4* _pParams)
{
	MAUTOSTRIP(CRC_Core_Attrib_TexGenW, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_TEXGEN)
		return;
	CRC_Attributes& Attr = m_lAttribStack[m_iAttribStack];
	if ((Attr.m_TexGenMode[_iTxt] & CRC_TEXGENMODE_MASK_W) == (_TexGenMode << CRC_TEXGENMODE_SHIFT_W) &&
		Attr.m_pTexGenParams[_iTxt][2] == _pParams) return;
	Attr.m_TexGenMode[_iTxt] = _TexGenMode;
	Attr.m_pTexGenParams[_iTxt][2] = _pParams;
	m_AttribChanged |= CRC_ATTRCHG_TEXGEN;
}

void CRC_Core::Attrib_TexGenQ(int _iTxt, int _TexGenMode, fp4* _pParams)
{
	MAUTOSTRIP(CRC_Core_Attrib_TexGenQ, MAUTOSTRIP_VOID);
	if (m_lAttribStack[m_iAttribStack].m_AttribLock & CRC_ATTRCHG_TEXGEN)
		return;
	CRC_Attributes& Attr = m_lAttribStack[m_iAttribStack];
	if ((Attr.m_TexGenMode[_iTxt] & CRC_TEXGENMODE_MASK_Q) == (_TexGenMode << CRC_TEXGENMODE_SHIFT_Q) &&
		Attr.m_pTexGenParams[_iTxt][3] == _pParams) return;
	Attr.m_TexGenMode[_iTxt] = _TexGenMode;
	Attr.m_pTexGenParams[_iTxt][3] = _pParams;
	m_AttribChanged |= CRC_ATTRCHG_TEXGEN;
}
*/

/*
int CRC_Core::Attrib_PushSortAttribute()
{
	MAUTOSTRIP(CRC_Core_Attrib_PushSortAttribute, 0);
	if (m_nBSAttributes >= m_lBSAttributes.Len()) return -1;
	m_lBSAttributes[m_nBSAttributes] = m_lAttribStack[m_iAttribStack];
	return m_nBSAttributes++;
}

bool CRC_Core::Attrib_SortEnable(int _iAttrib)
{
	MAUTOSTRIP(CRC_Core_Attrib_SortEnable, false);
	if (_iAttrib >= 0)
		m_iCurrentSortAttrib = _iAttrib;
	else
	{
		if (m_nBSAttributes >= m_lBSAttributes.Len()) return false;
		m_iCurrentSortAttrib = m_nBSAttributes;
		m_lBSAttributes[m_nBSAttributes++] = m_lAttribStack[m_iAttribStack];
	}
	Attrib_Enable(CRC_FLAGS_SORT);
	return true;
}

void CRC_Core::Attrib_SortDisable()
{
	MAUTOSTRIP(CRC_Core_Attrib_SortDisable, MAUTOSTRIP_VOID);
	Attrib_Disable(CRC_FLAGS_SORT);
}
*/

// -------------------------------------------------------------------
// Global attributes
// -------------------------------------------------------------------
void CRC_Core::Attrib_GlobalEnable(int _Flags)
{
	MAUTOSTRIP(CRC_Core_Attrib_GlobalEnable, MAUTOSTRIP_VOID);
	m_Mode.m_Flags |= _Flags;
	DRenderCoreTopClass Attrib_GlobalUpdate();
}

void CRC_Core::Attrib_GlobalDisable(int _Flags)
{
	MAUTOSTRIP(CRC_Core_Attrib_GlobalDisable, MAUTOSTRIP_VOID);
	m_Mode.m_Flags &= ~_Flags;
	DRenderCoreTopClass Attrib_GlobalUpdate();
}

void CRC_Core::Attrib_GlobalSwitch(int _Flags)
{
	MAUTOSTRIP(CRC_Core_Attrib_GlobalSwitch, MAUTOSTRIP_VOID);
	m_Mode.m_Flags ^= _Flags;
	DRenderCoreTopClass Attrib_GlobalUpdate();
}

void CRC_Core::Attrib_GlobalSetVar(int _Var, int _Value)
{
	MAUTOSTRIP(CRC_Core_Attrib_GlobalSetVar, MAUTOSTRIP_VOID);
	bool bUpdate = false;
	int FlagsBefore = m_Mode.m_Flags;
	switch(_Var)
	{
	case CRC_GLOBALVAR_FILTER : 
		{
			if (m_Mode.m_Filter != _Value)
			{
				bUpdate = true;
				m_Mode.m_Filter = _Value;
			}
			break;
		}
	case CRC_GLOBALVAR_WIRE : 
		{
			m_Mode.m_Flags = (m_Mode.m_Flags & ~CRC_GLOBALFLAGS_WIRE) | ((_Value) ? CRC_GLOBALFLAGS_WIRE : 0); 
			break;
		}

	case CRC_GLOBALVAR_ALLOWTEXTURELOAD : 
		{
			m_bAllowTextureLoad = _Value != 0;
			break;
		}

	default : break;
	}

	if (m_Mode.m_Flags != FlagsBefore)
		bUpdate = true;

	if (bUpdate)
		DRenderCoreTopClass Attrib_GlobalUpdate();
}

void CRC_Core::Attrib_GlobalSetVarfv(int _Var, const fp4* _pValues)
{
	MAUTOSTRIP(CRC_Core_Attrib_GlobalSetVarfv, MAUTOSTRIP_VOID);
	bool bUpdate = false;
	switch(_Var)
	{
	case CRC_GLOBALVAR_GAMMARAMPSCALE : 
		{
			for(int i = 0; i < 4; i++)
				if (m_Mode.m_GammaScale.k[i] != _pValues[i])
				{
					bUpdate = true;
					m_Mode.m_GammaScale.k[i] = _pValues[i];
				}
			break;
		}
	case CRC_GLOBALVAR_GAMMARAMPADD :
		{
			for(int i = 0; i < 4; i++)
				if (m_Mode.m_GammaAdd.k[i] != _pValues[i])
				{
					bUpdate = true;
					m_Mode.m_GammaAdd.k[i] = _pValues[i];
				}
			break;
		}
	case CRC_GLOBALVAR_GAMMA : 
		{
			if (m_Mode.m_GammaCorrection != _pValues[0])
			{
				bUpdate = true;
				m_Mode.m_GammaCorrection = _pValues[0];
			}
			break;
		}

	default : break;
	}

	if (bUpdate)
		DRenderCoreTopClass Attrib_GlobalUpdate();
}

int CRC_Core::Attrib_GlobalGetVar(int _Var)
{
	MAUTOSTRIP(CRC_Core_Attrib_GlobalGetVar, 0);
	switch(_Var)
	{
	case CRC_GLOBALVAR_NUMTEXTURES :
		{
			return m_Caps_nMultiTexture;
		}
	case CRC_GLOBALVAR_NUMTEXTUREENV:
		{
			return m_Caps_nMultiTextureEnv;
		}
	case CRC_GLOBALVAR_NUMTEXTURECOORDS :
		{
			return m_Caps_nMultiTextureCoords;
		}
	case CRC_GLOBALVAR_FILTER : return m_Mode.m_Filter;

	case CRC_GLOBALVAR_WIRE : return (m_Mode.m_Flags & CRC_GLOBALFLAGS_WIRE) ? 1 : 0; break;
	case CRC_GLOBALVAR_ALLOWTEXTURELOAD : return m_bAllowTextureLoad; break;
	}
	return 0;
}

int CRC_Core::Attrib_GlobalGetVarfv(int _Var, fp4* _pValues)
{
	MAUTOSTRIP(CRC_Core_Attrib_GlobalGetVarfv, 0);
	Error("Attrib_GlobalGetVarf", "Not implemented.");
/*	switch(_Var)
	{
	default : return 0.0f;
	}*/
	return 0;
}

// -------------------------------------------------------------------
//  Transform
// -------------------------------------------------------------------
#define MACRO_ISMIRRORED(Mat)	\
	((CVec3Dfp4::GetMatrixRow(Mat, 0) / CVec3Dfp4::GetMatrixRow(Mat, 1)) * CVec3Dfp4::GetMatrixRow(Mat, 2) < 0.0f)

CRC_Core::CRC_MatrixState& CRC_Core::Matrix_GetState()
{
	MAUTOSTRIP(CRC_Core_Matrix_GetState, *((void*)NULL));
	return m_lMatrixStack[m_iMatrixStack];
}

const CMat4Dfp4& CRC_Core::Matrix_Get()
{
	MAUTOSTRIP(CRC_Core_Matrix_Get, *((void*)NULL));
	return m_lMatrixStack[m_iMatrixStack].m_lMatrices[m_iMatrixMode];
}

void CRC_Core::Matrix_Update()
{
	MAUTOSTRIP(CRC_Core_Matrix_Update, MAUTOSTRIP_VOID);
	CRC_MatrixState& MS = Matrix_GetState();

#if	DEF_CRC_MAXTEXTURES == 8
	const int iStart = CRC_MATRIX_TEXTURE0;
	const int iEnd = CRC_MATRIX_TEXTURE7;
	const int iMask = (
		1 << CRC_MATRIX_TEXTURE0 |
		1 << CRC_MATRIX_TEXTURE1 | 
		1 << CRC_MATRIX_TEXTURE2 | 
		1 << CRC_MATRIX_TEXTURE3 | 
		1 << CRC_MATRIX_TEXTURE4 | 
		1 << CRC_MATRIX_TEXTURE5 | 
		1 << CRC_MATRIX_TEXTURE6 | 
		1 << CRC_MATRIX_TEXTURE7 );
#elif DEF_CRC_MAXTEXTURES == 4
	const int iStart = CRC_MATRIX_TEXTURE0;
	const int iEnd = CRC_MATRIX_TEXTURE3;
	const int iMask = (
		1 << CRC_MATRIX_TEXTURE0 |
		1 << CRC_MATRIX_TEXTURE1 | 
		1 << CRC_MATRIX_TEXTURE2 | 
		1 << CRC_MATRIX_TEXTURE3 );
#else
#error "DEF_CRC_MAXTEXTURES is not a valid value"
#endif
	if (m_MatrixChanged & iMask )
	{
		for(int i = iStart; i <= iEnd; i++)
			if (m_MatrixChanged & (1 << i))
			{
				if (MS.m_MatrixUnit & (1 << i))
					DRenderCoreTopClass Matrix_SetRender(i, NULL);
				else
					DRenderCoreTopClass Matrix_SetRender(i, &MS.m_lMatrices[i]);

				m_MatrixChanged &= ~(1 << i);
			}
	}

	// We assume the matrix is orthonormalized.
	if (m_MatrixChanged & 1)
	{
		const CMat4Dfp4& Mat = MS.m_lMatrices[CRC_MATRIX_MODEL];
		if (Clip_IsEnabled())
		{
			Mat.InverseOrthogonal(m_ClipCurrentMatrixInv);
			CVec3Dfp4 VP(0);
			VP.MultiplyMatrix(m_ClipCurrentMatrixInv);
			m_ClipCurrentLocalVP = VP;
			m_bClipCurrentMatrixIsMirrored = MACRO_ISMIRRORED(Mat); //Mat.IsMirrored();
		}
		DRenderCoreTopClass Matrix_SetRender(CRC_MATRIX_MODEL, &Mat);
		m_MatrixChanged &= ~1;
	}
}

void CRC_Core::Matrix_SetMode(int _iMode)
{
	MAUTOSTRIP(CRC_Core_Matrix_SetMode, MAUTOSTRIP_VOID);
	if (_iMode < 0 || _iMode >= CRC_MATRIXSTACKS)
		Error("Matrix_SetMode", CStrF("Invalid matrix mode (%d)", _iMode));
	m_iMatrixMode = _iMode;
}

void CRC_Core::Matrix_Push()
{
	MAUTOSTRIP(CRC_Core_Matrix_Push, MAUTOSTRIP_VOID);
	if (m_iMatrixStack >= m_lMatrixStack.Len() - 1) 
	{
		Error("Matrix_Push", "Matrix stack overflow.");
	}

	m_iMatrixStack++;
	m_lMatrixStack[m_iMatrixStack] = m_lMatrixStack[m_iMatrixStack-1];
	m_lMatrixStack[m_iMatrixStack].m_MatrixChanged = 0;
//	m_MatrixChanged = -1;

/*	m_iMatrixStack[m_iMatrixMode]++;
	m_lMatrixStack[m_iMatrixMode][m_iMatrixStack[m_iMatrixMode]] = m_lMatrixStack[m_iMatrixMode][m_iMatrixStack[m_iMatrixMode]-1];
	m_MatrixUnit[m_iMatrixStack[m_iMatrixMode]] &= (1 << m_iMatrixMode);
	m_MatrixUnit[m_iMatrixStack[m_iMatrixMode]] |= m_MatrixUnit[m_iMatrixStack[m_iMatrixMode] - 1] & (1 << m_iMatrixMode);*/
	
}

void CRC_Core::Matrix_Pop()
{
	MAUTOSTRIP(CRC_Core_Matrix_Pop, MAUTOSTRIP_VOID);
	if (m_iMatrixStack <= 0) Error("Matrix_Pop", "Matrix stack underflow.");
	m_MatrixChanged = -1;
	m_MatrixChanged |= m_lMatrixStack[m_iMatrixStack].m_MatrixChanged;
	if (m_MatrixChanged)
		m_AttribChanged |= CRC_ATTRCHG_MATRIX;
	if (m_MatrixChanged & (1 << CRC_MATRIX_PALETTE))
		m_AttribChanged |= CRC_ATTRCHG_MATRIXPALETTE;

	m_iMatrixStack--;
	m_bClipChanged = true;
}

void CRC_Core::Matrix_SetUnit()
{
	MAUTOSTRIP(CRC_Core_Matrix_SetUnit, MAUTOSTRIP_VOID);
	CRC_MatrixState& MS = Matrix_GetState();
	if (MS.m_MatrixUnit & (1 << m_iMatrixMode)) return;
	MS.m_lMatrices[m_iMatrixMode].Unit();
	MS.m_MatrixUnit |= 1 << m_iMatrixMode;
	MS.m_MatrixChanged |= 1 << m_iMatrixMode;
	m_MatrixChanged |= 1 << m_iMatrixMode;
	m_AttribChanged |= CRC_ATTRCHG_MATRIX;
	m_bClipChanged = true;
}

void CRC_Core::Matrix_Set(const CMat4Dfp4& _Matrix)
{
	MAUTOSTRIP(CRC_Core_Matrix_Set, MAUTOSTRIP_VOID);
	CRC_MatrixState& MS = Matrix_GetState();
	MS.m_lMatrices[m_iMatrixMode] = _Matrix;
	MS.m_MatrixUnit &= ~(1 << m_iMatrixMode);
	MS.m_MatrixChanged |= 1 << m_iMatrixMode;
	m_MatrixChanged |= 1 << m_iMatrixMode;
	m_AttribChanged |= CRC_ATTRCHG_MATRIX;
	m_bClipChanged = true;
}

void CRC_Core::Matrix_Get(CMat4Dfp4& _Matrix)
{
	MAUTOSTRIP(CRC_Core_Matrix_Get_2, MAUTOSTRIP_VOID);
	CRC_MatrixState& MS = Matrix_GetState();
	_Matrix = MS.m_lMatrices[m_iMatrixMode];
}

void CRC_Core::Matrix_Multiply(const CMat4Dfp4& _Matrix)
{
	MAUTOSTRIP(CRC_Core_Matrix_Multiply, MAUTOSTRIP_VOID);
	CMat4Dfp4 TmpMat;
	_Matrix.Multiply(Matrix_Get(), TmpMat);
	DRenderTopClass Matrix_Set(TmpMat);
}

void CRC_Core::Matrix_MultiplyInverse(const CMat4Dfp4& _Matrix)
{
	MAUTOSTRIP(CRC_Core_Matrix_MultiplyInverse, MAUTOSTRIP_VOID);
	CMat4Dfp4 TmpMat1, TmpMat2;
	_Matrix.InverseOrthogonal(TmpMat1);
	TmpMat1.Multiply(Matrix_Get(), TmpMat2);
	DRenderTopClass Matrix_Set(TmpMat2);
}

void CRC_Core::Matrix_PushMultiply(const CMat4Dfp4& _Matrix)
{
	MAUTOSTRIP(CRC_Core_Matrix_PushMultiply, MAUTOSTRIP_VOID);
	DRenderTopClass Matrix_Push();
	DRenderTopClass Matrix_Multiply(_Matrix);
}

void CRC_Core::Matrix_PushMultiplyInverse(const CMat4Dfp4& _Matrix)
{
	MAUTOSTRIP(CRC_Core_Matrix_PushMultiplyInverse, MAUTOSTRIP_VOID);
	DRenderTopClass Matrix_Push();
	DRenderTopClass Matrix_MultiplyInverse(_Matrix);
}


void CRC_Core::Matrix_SetPalette(const CRC_MatrixPalette* _pMatrixPaletteArgs)
{
	MAUTOSTRIP(CRC_Core_Matrix_SetPalette, MAUTOSTRIP_VOID);
	CRC_MatrixState& MS = Matrix_GetState();
	if (MS.m_pMatrixPaletteArgs == _pMatrixPaletteArgs)
		return;

	MS.m_pMatrixPaletteArgs = _pMatrixPaletteArgs;

	MS.m_MatrixChanged |= 1 << CRC_MATRIX_PALETTE;
	m_MatrixChanged |= 1 << CRC_MATRIX_PALETTE;
	m_AttribChanged |= CRC_ATTRCHG_MATRIXPALETTE;
	m_bClipChanged = true;
}

#ifndef DEFINE_MAT43_IS_MAT4D
void CRC_Core::Matrix_Set(const CMat43fp4& _Matrix)
{
	DRenderTopClass Matrix_Set(_Matrix.Get4x4());
}

void CRC_Core::Matrix_Multiply(const CMat43fp4& _Matrix)
{
	DRenderTopClass Matrix_Multiply(_Matrix.Get4x4());
}

void CRC_Core::Matrix_MultiplyInverse(const CMat43fp4& _Matrix)
{
	DRenderTopClass Matrix_MultiplyInverse(_Matrix.Get4x4());
}

void CRC_Core::Matrix_PushMultiply(const CMat43fp4& _Matrix)
{
	DRenderTopClass Matrix_PushMultiply(_Matrix.Get4x4());
}

void CRC_Core::Matrix_PushMultiplyInverse(const CMat43fp4& _Matrix)
{
	DRenderTopClass Matrix_PushMultiplyInverse(_Matrix.Get4x4());
}
#endif

// -------------------------------------------------------------------
//  Clipping
// -------------------------------------------------------------------
#define FACECUT2_EPSILON 0.01f

int CRC_Core::Clip_CutFace(int _nv, const CPlane3Dfp4* _pPlanes, int _np, int _Components, int _bInvertPlanes)
{
	MAUTOSTRIP(CRC_Core_Clip_CutFace, 0);
	/*
		Supported components:
			Vertex
			Color
			TVertex0
			TVertex1
	*/
	if (!_nv) return 0;
	const int MaxVClip = 32;

	CVec3Dfp4 VClip[MaxVClip];
	CVec3Dfp4 NClip[MaxVClip];
	CVec4Dfp4 ColClip[MaxVClip];
	CVec4Dfp4 SpecClip[MaxVClip];
//	fp4 FogClip[MaxVClip];
	CVec4Dfp4 TVClip0[MaxVClip];
	CVec4Dfp4 TVClip1[MaxVClip];
	CVec4Dfp4 TVClip2[MaxVClip];
	CVec4Dfp4 TVClip3[MaxVClip];

	CVec3Dfp4* pVDest = &VClip[0];
	CVec3Dfp4* pVSrc = m_ClipV;

	CVec3Dfp4* pNDest = (_Components & CRC_CLIPARRAY_NORMAL) ? &NClip[0] : NULL;
	CVec3Dfp4* pNSrc = (_Components & CRC_CLIPARRAY_NORMAL) ? m_ClipN : NULL;
	CVec4Dfp4* pColDest0 = (_Components & CRC_CLIPARRAY_COLOR) ? ColClip : NULL;
	CVec4Dfp4* pColSrc0 = (_Components & CRC_CLIPARRAY_COLOR) ? m_ClipCol: NULL;
	CVec4Dfp4* pSpecDest0 = (_Components & CRC_CLIPARRAY_SPECULAR) ? SpecClip : NULL;
	CVec4Dfp4* pSpecSrc0 = (_Components & CRC_CLIPARRAY_SPECULAR) ? m_ClipSpec: NULL;
//	fp4* pFogDest0 = (_Components & CRC_CLIPARRAY_FOG) ? FogClip : NULL;
//	fp4* pFogSrc0 = (_Components & CRC_CLIPARRAY_FOG) ? m_ClipFog : NULL;
	CVec4Dfp4* pTVDest0 = (_Components & CRC_CLIPARRAY_TVERTEX0) ? TVClip0 : NULL;
	CVec4Dfp4* pTVDest1 = (_Components & CRC_CLIPARRAY_TVERTEX1) ? TVClip1 : NULL;
	CVec4Dfp4* pTVDest2 = (_Components & CRC_CLIPARRAY_TVERTEX2) ? TVClip2 : NULL;
	CVec4Dfp4* pTVDest3 = (_Components & CRC_CLIPARRAY_TVERTEX3) ? TVClip3 : NULL;
	CVec4Dfp4* pTVSrc0 = (_Components & CRC_CLIPARRAY_TVERTEX0) ? m_ClipTV0 : NULL;
	CVec4Dfp4* pTVSrc1 = (_Components & CRC_CLIPARRAY_TVERTEX1) ? m_ClipTV1 : NULL;
	CVec4Dfp4* pTVSrc2 = (_Components & CRC_CLIPARRAY_TVERTEX2) ? m_ClipTV2 : NULL;
	CVec4Dfp4* pTVSrc3 = (_Components & CRC_CLIPARRAY_TVERTEX3) ? m_ClipTV3 : NULL;


	int PlaneShift = 1;
	for(int iPlane = 0; iPlane < _np; iPlane++)
	{
		int Shift = PlaneShift;
		PlaneShift <<= 1;
		if (!(m_ClipMask & Shift)) continue;

		const CPlane3Dfp4* pP = &_pPlanes[iPlane];
		fp4 VertPDist[32];
		bool bBehind = false;
		bool bFront = false;

		// Calc point-2-plane distance for all vertices.
		for(int v = 0; v < _nv; v++)
		{
			VertPDist[v] = pP->Distance(pVSrc[v]);
			if (_bInvertPlanes) VertPDist[v] = -VertPDist[v];
			if (VertPDist[v] < 0.0f) bBehind = true; else bFront = true;
		}

		// If all points are on one side, return either all or none.
		if (!(bFront && bBehind))
		{
			if (bFront) continue;
			return 0;
		}

		int nClip = 0;
		{
			int v = _nv-1;
			for (int v2 = 0; v2 < _nv; v2++)
			{
				if (VertPDist[v] > -FACECUT2_EPSILON)
				{
					pVDest[nClip] = pVSrc[v];
					if (pColDest0) pColDest0[nClip] = pColSrc0[v];
					if (pTVDest0) pTVDest0[nClip] = pTVSrc0[v];
					if (pTVDest1) pTVDest1[nClip] = pTVSrc1[v];
					if (pTVDest2) pTVDest2[nClip] = pTVSrc2[v];
					if (pTVDest3) pTVDest3[nClip] = pTVSrc3[v];
					nClip++;

					if ((VertPDist[v2] < -FACECUT2_EPSILON) && (VertPDist[v] > FACECUT2_EPSILON))
					{
						fp4 dvx = (pVSrc[v2].k[0] - pVSrc[v].k[0]);
						fp4 dvy = (pVSrc[v2].k[1] - pVSrc[v].k[1]);
						fp4 dvz = (pVSrc[v2].k[2] - pVSrc[v].k[2]);
						fp4 s = dvx*pP->n.k[0] + dvy*pP->n.k[1] + dvz*pP->n.k[2];
						if (_bInvertPlanes) s = -s;
						if (s)
						{
							fp4 sp = VertPDist[v];
							fp4 t = -sp/s;
							pVDest[nClip].k[0] = pVSrc[v].k[0] + dvx * t;
							pVDest[nClip].k[1] = pVSrc[v].k[1] + dvy * t;
							pVDest[nClip].k[2] = pVSrc[v].k[2] + dvz * t;

							if (pNDest)
							{
								pNDest[nClip].k[0] = pNSrc[v].k[0] + (pNSrc[v2].k[0] - pNSrc[v].k[0]) * t;
								pNDest[nClip].k[1] = pNSrc[v].k[1] + (pNSrc[v2].k[1] - pNSrc[v].k[1]) * t;
								pNDest[nClip].k[2] = pNSrc[v].k[2] + (pNSrc[v2].k[2] - pNSrc[v].k[2]) * t;
							}

							if (pColDest0)
							{
								pColDest0[nClip].k[0] = pColSrc0[v].k[0] + (pColSrc0[v2].k[0] - pColSrc0[v].k[0]) * t;
								pColDest0[nClip].k[1] = pColSrc0[v].k[1] + (pColSrc0[v2].k[1] - pColSrc0[v].k[1]) * t;
								pColDest0[nClip].k[2] = pColSrc0[v].k[2] + (pColSrc0[v2].k[2] - pColSrc0[v].k[2]) * t;
								pColDest0[nClip].k[3] = pColSrc0[v].k[3] + (pColSrc0[v2].k[3] - pColSrc0[v].k[3]) * t;
							}
							if (pSpecDest0)
							{
								pSpecDest0[nClip].k[0] = pSpecSrc0[v].k[0] + (pSpecSrc0[v2].k[0] - pSpecSrc0[v].k[0]) * t;
								pSpecDest0[nClip].k[1] = pSpecSrc0[v].k[1] + (pSpecSrc0[v2].k[1] - pSpecSrc0[v].k[1]) * t;
								pSpecDest0[nClip].k[2] = pSpecSrc0[v].k[2] + (pSpecSrc0[v2].k[2] - pSpecSrc0[v].k[2]) * t;
								pSpecDest0[nClip].k[3] = pSpecSrc0[v].k[3] + (pSpecSrc0[v2].k[3] - pSpecSrc0[v].k[3]) * t;
							}
/*
							if (pFogDest0)
							{
								pFogDest0[nClip] = pFogSrc0[v] + (pFogSrc0[v2] - pFogSrc0[v]) * t;
							}*/

							if (pTVDest0) pTVSrc0[v].Lerp(pTVSrc0[v2], t, pTVDest0[nClip]);
							if (pTVDest1) pTVSrc1[v].Lerp(pTVSrc1[v2], t, pTVDest1[nClip]);
							if (pTVDest2) pTVSrc2[v].Lerp(pTVSrc2[v2], t, pTVDest2[nClip]);
							if (pTVDest3) pTVSrc3[v].Lerp(pTVSrc3[v2], t, pTVDest3[nClip]);

							nClip++;
						}
					}
				}
				else
				{
					if (VertPDist[v2] > FACECUT2_EPSILON)
					{
						fp4 dvx = (pVSrc[v2].k[0] - pVSrc[v].k[0]);
						fp4 dvy = (pVSrc[v2].k[1] - pVSrc[v].k[1]);
						fp4 dvz = (pVSrc[v2].k[2] - pVSrc[v].k[2]);
						fp4 s = dvx*pP->n.k[0] + dvy*pP->n.k[1] + dvz*pP->n.k[2];
						if (_bInvertPlanes) s = -s;
						if (s)
						{
							fp4 sp = VertPDist[v];
							fp4 t = -sp/s;
							pVDest[nClip].k[0] = pVSrc[v].k[0] + dvx * t;
							pVDest[nClip].k[1] = pVSrc[v].k[1] + dvy * t;
							pVDest[nClip].k[2] = pVSrc[v].k[2] + dvz * t;

							if (pNDest)
							{
								pNDest[nClip].k[0] = pNSrc[v].k[0] + (pNSrc[v2].k[0] - pNSrc[v].k[0]) * t;
								pNDest[nClip].k[1] = pNSrc[v].k[1] + (pNSrc[v2].k[1] - pNSrc[v].k[1]) * t;
								pNDest[nClip].k[2] = pNSrc[v].k[2] + (pNSrc[v2].k[2] - pNSrc[v].k[2]) * t;
							}

							if (pColDest0)
							{
								pColDest0[nClip].k[0] = pColSrc0[v].k[0] + (pColSrc0[v2].k[0] - pColSrc0[v].k[0]) * t;
								pColDest0[nClip].k[1] = pColSrc0[v].k[1] + (pColSrc0[v2].k[1] - pColSrc0[v].k[1]) * t;
								pColDest0[nClip].k[2] = pColSrc0[v].k[2] + (pColSrc0[v2].k[2] - pColSrc0[v].k[2]) * t;
								pColDest0[nClip].k[3] = pColSrc0[v].k[3] + (pColSrc0[v2].k[3] - pColSrc0[v].k[3]) * t;
							}
							if (pSpecDest0)
							{
								pSpecDest0[nClip].k[0] = pSpecSrc0[v].k[0] + (pSpecSrc0[v2].k[0] - pSpecSrc0[v].k[0]) * t;
								pSpecDest0[nClip].k[1] = pSpecSrc0[v].k[1] + (pSpecSrc0[v2].k[1] - pSpecSrc0[v].k[1]) * t;
								pSpecDest0[nClip].k[2] = pSpecSrc0[v].k[2] + (pSpecSrc0[v2].k[2] - pSpecSrc0[v].k[2]) * t;
								pSpecDest0[nClip].k[3] = pSpecSrc0[v].k[3] + (pSpecSrc0[v2].k[3] - pSpecSrc0[v].k[3]) * t;
							}
/*							if (pFogDest0)
							{
								pFogDest0[nClip] = pFogSrc0[v] + (pFogSrc0[v2] - pFogSrc0[v]) * t;
							}*/

							if (pTVDest0) pTVSrc0[v].Lerp(pTVSrc0[v2], t, pTVDest0[nClip]);
							if (pTVDest1) pTVSrc1[v].Lerp(pTVSrc1[v2], t, pTVDest1[nClip]);
							if (pTVDest2) pTVSrc2[v].Lerp(pTVSrc2[v2], t, pTVDest2[nClip]);
							if (pTVDest3) pTVSrc3[v].Lerp(pTVSrc3[v2], t, pTVDest3[nClip]);

							nClip++;
						}
					}
				}

				if (nClip > MaxVClip-1) Error_static("CutFace", "Too many vertices.");
				v = v2;
			}
		}

		if (!nClip) return 0;
		_nv = nClip;

		Swap(pVSrc, pVDest);
		if (pNDest) Swap(pNSrc, pNDest);
		if (pColDest0) Swap(pColSrc0, pColDest0);
		if (pSpecDest0) Swap(pSpecSrc0, pSpecDest0);
//		if (pFogDest0) Swap(pFogSrc0, pFogDest0);
		if (pTVDest0) Swap(pTVSrc0, pTVDest0);
		if (pTVDest1) Swap(pTVSrc1, pTVDest1);
		if (pTVDest2) Swap(pTVSrc2, pTVDest2);
		if (pTVDest3) Swap(pTVSrc3, pTVDest3);
	}

	// Move if the latest vertices are in the wrong array.
	if (pVSrc != m_ClipV) 
	{
		memcpy(m_ClipV, pVSrc, _nv*sizeof(CVec3Dfp4));
		if (pNSrc) memcpy(m_ClipN, pNSrc, _nv*sizeof(CVec3Dfp4));
		if (pColSrc0) memcpy(m_ClipCol, pColSrc0, _nv*sizeof(CVec4Dfp4));
		if (pSpecSrc0) memcpy(m_ClipSpec, pSpecSrc0, _nv*sizeof(CVec4Dfp4));
//		if (pFogSrc0) memcpy(m_ClipFog, pFogSrc0, _nv*sizeof(fp4));
		if (pTVSrc0) memcpy(m_ClipTV0, pTVSrc0, _nv*sizeof(CVec4Dfp4));
		if (pTVSrc1) memcpy(m_ClipTV1, pTVSrc1, _nv*sizeof(CVec4Dfp4));
		if (pTVSrc2) memcpy(m_ClipTV2, pTVSrc2, _nv*sizeof(CVec4Dfp4));
		if (pTVSrc3) memcpy(m_ClipTV3, pTVSrc3, _nv*sizeof(CVec4Dfp4));
	}
	return _nv;
}

void CRC_Core::Clip_RenderPolygon(int _nV, int _Components, int _PlaneMask)
{
	MAUTOSTRIP(CRC_Core_Clip_RenderPolygon, MAUTOSTRIP_VOID);
	m_nClipTrianglesDrawn++;

	Clip_Update();
	if (m_ClipMask) _nV = Clip_CutFace(_nV, m_ClipCurrent.m_Planes, m_ClipCurrent.m_nPlanes, _Components, false);
	if (_nV > 2)
	{
		DRenderCoreTopClass Internal_RenderPolygon(_nV, m_ClipV,
			(_Components & CRC_CLIPARRAY_NORMAL) ? m_ClipN : NULL,
			(_Components & CRC_CLIPARRAY_COLOR) ? m_ClipCol : NULL,
			(_Components & CRC_CLIPARRAY_SPECULAR) ? m_ClipSpec : NULL,
//			(_Components & CRC_CLIPARRAY_FOG) ? m_ClipFog : NULL,
			(_Components & CRC_CLIPARRAY_TVERTEX0) ? m_ClipTV0 : NULL,
			(_Components & CRC_CLIPARRAY_TVERTEX1) ? m_ClipTV1 : NULL,
			(_Components & CRC_CLIPARRAY_TVERTEX2) ? m_ClipTV2 : NULL,
			(_Components & CRC_CLIPARRAY_TVERTEX3) ? m_ClipTV3 : NULL,
			m_ClipColor);
	}
}

int CRC_Core::Clip_InitVertexMasks(int _nV, const CVec3Dfp4* _pV, const uint16* _piV)
{
	MAUTOSTRIP(CRC_Core_Clip_InitVertexMasks, 0);
	if (_nV > m_lClipVertexMask.Len())
		m_lClipVertexMask.SetLen(_nV + 2048);

	Clip_Update();
	uint16* pM = m_lClipVertexMask.GetBasePtr();
	const CPlane3Dfp4* pP = m_ClipCurrent.m_Planes;
	int nP = m_ClipCurrent.m_nPlanes;

	int TotalMask = -1;	// If this one turns out != 0, all vertices are clipped away.

	int ClipMask = 0;	// This one represent all planes that any vertex was outside.
						// if it turns out to be zero, no clipping is needed.

	m_nClipVertices += _nV;

	for(int v = 0; v < _nV; v++)
	{
		int iv = (_piV) ? _piV[v] : v;
		const CVec3Dfp4* pVert = &_pV[iv];
		int Mask = 0;
		int Shift = 1;
		for(int p = 0; p < nP; p++)
		{
			if (pP[p].Distance(*pVert) < 0.0f) Mask |= Shift;
			Shift <<= 1;
		}
		pM[iv] = Mask;
		ClipMask |= Mask;
		TotalMask &= Mask;
	}
	m_ClipMask = ClipMask;
	return TotalMask;
}

void CRC_Core::Clip_Update()
{
	MAUTOSTRIP(CRC_Core_Clip_Update, MAUTOSTRIP_VOID);
	if (m_bClipChanged)
	{
		CRC_MatrixState& MS = Matrix_GetState();

		const CMat4Dfp4& Mat = MS.m_lMatrices[CRC_MATRIX_MODEL];
		Mat.InverseOrthogonal(m_ClipCurrentMatrixInv);
		CVec3Dfp4 VP(0);
		VP.MultiplyMatrix(m_ClipCurrentMatrixInv);
		m_ClipCurrentLocalVP = VP;
		m_bClipCurrentMatrixIsMirrored = MACRO_ISMIRRORED(Mat);

		m_ClipCurrent.Copy(m_lClipStack[m_iClipStack], &m_ClipCurrentMatrixInv);

		m_bClipChanged = false;
	}
}

// -------------------------------------------------------------------
void CRC_Core::Clip_Push()
{
	MAUTOSTRIP(CRC_Core_Clip_Push, MAUTOSTRIP_VOID);
	if (m_iClipStack >= m_lClipStack.Len() - 1) Error("Clip_Push", "Clip stack overflow.");
	m_iClipStack++;
	m_lClipStack[m_iClipStack].Copy(m_lClipStack[m_iClipStack-1]);
}

void CRC_Core::Clip_Pop()
{
	MAUTOSTRIP(CRC_Core_Clip_Pop, MAUTOSTRIP_VOID);
	if (m_iClipStack <= 0) Error("Clip_Pop", "Clip stack underflow.");
	m_iClipStack--;
	m_bClipChanged = true;
}

void CRC_Core::Clip_Clear()
{
	MAUTOSTRIP(CRC_Core_Clip_Clear, MAUTOSTRIP_VOID);
	m_lClipStack[m_iClipStack].m_nPlanes = 0;
	m_bClipChanged = true;
}

void CRC_Core::Clip_Set(const CPlane3Dfp4* _pPlanes, int _nPlanes)
{
	MAUTOSTRIP(CRC_Core_Clip_Set, MAUTOSTRIP_VOID);
	CRC_ClipStackEntry* pClip = &m_lClipStack[m_iClipStack];
	if (_nPlanes > CRC_CLIPMAXPLANES)
		Error("Clip_Set", CStrF("Too many planes. (%d)", _nPlanes));

	pClip->m_nPlanes = _nPlanes;
	memcpy(&pClip->m_Planes, _pPlanes, sizeof(CPlane3Dfp4) * _nPlanes);
	m_bClipChanged = true;
}

void CRC_Core::Clip_AddPlane(const CPlane3Dfp4& _Plane, const CMat4Dfp4* _pTransform, bool _bClipBack)
{
	MAUTOSTRIP(CRC_Core_Clip_AddPlane, MAUTOSTRIP_VOID);
	CRC_ClipStackEntry* pClip = &m_lClipStack[m_iClipStack];
	if (pClip->m_nPlanes >= CRC_CLIPMAXPLANES) Error("Clip_AddPlane", "Plane limit exceeded.");

	int p = pClip->m_nPlanes;
	pClip->m_Planes[p] = _Plane;
	if (_pTransform) pClip->m_Planes[p].Transform(*_pTransform);
	if (!_bClipBack) pClip->m_Planes[p].Inverse();
	pClip->m_nPlanes++;
	m_bClipChanged = true;
}

// -------------------------------------------------------------------
void CRC_Core::Geometry_VertexArray(const CVec3Dfp4* _pV, int _nVertices, int _bAllUsed)
{
	MAUTOSTRIP(CRC_Core_Geometry_VertexArray, MAUTOSTRIP_VOID);
	m_Geom.m_pV = const_cast<CVec3Dfp4*>(_pV);
	m_Geom.m_nV = _nVertices;
	m_bGeomArrayAllUsed = _bAllUsed;
}

void CRC_Core::Geometry_NormalArray(const CVec3Dfp4* _pN)
{
	MAUTOSTRIP(CRC_Core_Geometry_NormalArray, MAUTOSTRIP_VOID);
	m_Geom.m_pN = const_cast<CVec3Dfp4*>(_pN);
}

void CRC_Core::Geometry_TVertexArray(const fp4* _pTV, int _TxtChannel, int _nComp)
{
	MAUTOSTRIP(CRC_Core_Geometry_TVertexArray, MAUTOSTRIP_VOID);
	m_Geom.m_pTV[_TxtChannel] = const_cast<fp4*>(_pTV);
	m_Geom.m_nTVComp[_TxtChannel] = _nComp;
}

void CRC_Core::Geometry_TVertexArray(const CVec2Dfp4* _pTV, int _TxtChannel)
{
	MAUTOSTRIP(CRC_Core_Geometry_TVertexArray_2, MAUTOSTRIP_VOID);
	m_Geom.m_pTV[_TxtChannel] = (fp4*)_pTV;
	m_Geom.m_nTVComp[_TxtChannel] = 2;
}

void CRC_Core::Geometry_TVertexArray(const CVec3Dfp4* _pTV, int _TxtChannel)
{
	MAUTOSTRIP(CRC_Core_Geometry_TVertexArray_3, MAUTOSTRIP_VOID);
	m_Geom.m_pTV[_TxtChannel] = (fp4*)_pTV;
	m_Geom.m_nTVComp[_TxtChannel] = 3;
}

void CRC_Core::Geometry_TVertexArray(const CVec4Dfp4* _pTV, int _TxtChannel)
{
	MAUTOSTRIP(CRC_Core_Geometry_TVertexArray_4, MAUTOSTRIP_VOID);
	m_Geom.m_pTV[_TxtChannel] = (fp4*)_pTV;
	m_Geom.m_nTVComp[_TxtChannel] = 4;
}

void CRC_Core::Geometry_ColorArray(const CPixel32* _pCol)
{
	MAUTOSTRIP(CRC_Core_Geometry_ColorArray, MAUTOSTRIP_VOID);
	m_Geom.m_pCol = const_cast<CPixel32*>(_pCol);
}


void CRC_Core::Geometry_SpecularArray(const CPixel32* _pSpec)
{
	MAUTOSTRIP(CRC_Core_Geometry_SpecularArray, MAUTOSTRIP_VOID);
	m_Geom.m_pSpec = const_cast<CPixel32*>(_pSpec);
}
/*
void CRC_Core::Geometry_FogArray(const fp4* _pFog)
{
	MAUTOSTRIP(CRC_Core_Geometry_FogArray, MAUTOSTRIP_VOID);
	m_Geom.m_pFog = const_cast<fp4*>(_pFog);
}
*/
void CRC_Core::Geometry_MatrixIndexArray(const uint32* _pMI)
{
	MAUTOSTRIP(CRC_Core_Geometry_MatrixIndexArray, MAUTOSTRIP_VOID);
	m_Geom.m_pMI = const_cast<uint32*>(_pMI);
}

void CRC_Core::Geometry_MatrixWeightArray(const fp4* _pMW, int _nComp)
{
	MAUTOSTRIP(CRC_Core_Geometry_MatrixWeightArray, MAUTOSTRIP_VOID);
	m_Geom.m_pMW = const_cast<fp4*>(_pMW);
	m_Geom.m_nMWComp = _nComp;
}

void CRC_Core::Geometry_Color(CPixel32 _Col)
{
	MAUTOSTRIP(CRC_Core_Geometry_Color, MAUTOSTRIP_VOID);
	m_GeomColor = _Col;
}

void CRC_Core::Geometry_VertexBuffer(const CRC_VertexBuffer& _VB, int _bAllUsed)
{
	MAUTOSTRIP(CRC_Core_Geometry_VertexBuffer, MAUTOSTRIP_VOID);
	memcpy(&m_Geom, &_VB, sizeof(m_Geom));
	m_GeomVBID = 0;
	m_bGeomArrayAllUsed = _bAllUsed;
}

void CRC_Core::Geometry_VertexBuffer(int _VBID, int _bAllUsed)
{
	MAUTOSTRIP(CRC_Core_Geometry_VertexBuffer_2, MAUTOSTRIP_VOID);
	m_GeomVBID = _VBID;
	m_bGeomArrayAllUsed = _bAllUsed;
}

void CRC_Core::Geometry_Clear()
{
	MAUTOSTRIP(CRC_Core_Geometry_Clear, MAUTOSTRIP_VOID);
	m_Geom.Clear();
	m_GeomColor = 0xffffffff;
	m_GeomVBID = 0;
}

void CRC_Core::Geometry_PrecacheBegin( int _Count )
{
	MAUTOSTRIP(CRC_Core_Geometry_PrecacheBegin, MAUTOSTRIP_VOID);
}

void CRC_Core::Geometry_PrecacheEnd()
{
	MAUTOSTRIP(CRC_Core_Geometry_PrecacheEnd, MAUTOSTRIP_VOID);
}

void CRC_Core::Geometry_Precache(int _VBID)
{
	MAUTOSTRIP(CRC_Core_Geometry_Precache, MAUTOSTRIP_VOID);
}

void CRC_Core::Geometry_PrecacheFlush()
{
}

// -------------------------------------------------------------------
void CRC_Core::Geometry_GetVertexFormat(const CRC_VertexBuffer& _VB, int &_RetVF, int &_RetVSize, int &_RetCollectFunction)
{
	MAUTOSTRIP(CRC_Core_Geometry_GetVertexFormat, MAUTOSTRIP_VOID);
	MSCOPESHORT(CRC_Core::Geometry_GetVertexFormat);
	
	int ProcNr = 0;
	int Size = 12;
	if (_VB.m_pN) { ProcNr += CRC_VF_NORMAL; Size += 12; }
//	if (_VB.m_pCol) { ProcNr += CRC_VF_COLOR; Size += 4; }

	if (_VB.m_pCol) { ProcNr += CRC_VF_COLOR;  }
	Size += 4;

	if (_VB.m_pSpec) { ProcNr += CRC_VF_SPECULAR; Size += 4; }

// Fogcoord arrays are now unsupported because 
//   1) GeForce doesn't support AGP pulling of fogcoordinates
//	 2) We should not be calculating any fogcoords on the CPU in the first place.

//	if (_VB.m_pFog) { ProcNr += CRC_VF_FOG; Size += 4; }

	int bNoFunction = 0;
	int bNonStandard = 0;
	int nTex = 0;
	int Format = ProcNr;
	if (_VB.m_pTV[0])
	{
		nTex++;
		Size += _VB.m_nTVComp[0] * 4;
		Format += CRC_FV_TEXCOORD(0, _VB.m_nTVComp[0]);
		bNonStandard |= _VB.m_nTVComp[0] - 2;

		if (_VB.m_pTV[1])
		{
			nTex++;
			Size += _VB.m_nTVComp[1] * 4;
			Format += CRC_FV_TEXCOORD(1, _VB.m_nTVComp[1]);
			bNonStandard |= _VB.m_nTVComp[1] - 2;

			if (_VB.m_pTV[2])
			{
				nTex++;
				Size += _VB.m_nTVComp[2] * 4;
				Format += CRC_FV_TEXCOORD(2, _VB.m_nTVComp[2]);
				bNonStandard |= _VB.m_nTVComp[2] - 2;

				if (_VB.m_pTV[3])
				{
					nTex++;
					Size += _VB.m_nTVComp[3] * 4;
					Format += CRC_FV_TEXCOORD(3, _VB.m_nTVComp[3]);
					bNonStandard |= _VB.m_nTVComp[3] - 2;
#if DEF_CRC_MAXTEXTURES > 4
					if (_VB.m_pTV[4])
					{
						nTex++;
						Size += _VB.m_nTVComp[4] * 4;
						Format += CRC_FV_TEXCOORD(4, _VB.m_nTVComp[4]);
						bNonStandard |= _VB.m_nTVComp[4] - 2;

						if (_VB.m_pTV[5])
						{
							nTex++;
							Size += _VB.m_nTVComp[5] * 4;
							Format += CRC_FV_TEXCOORD(5, _VB.m_nTVComp[5]);
							bNonStandard |= _VB.m_nTVComp[5] - 2;

							if (_VB.m_pTV[6])
							{
								nTex++;
								Size += _VB.m_nTVComp[6] * 4;
								Format += CRC_FV_TEXCOORD(6, _VB.m_nTVComp[6]);
								bNonStandard |= _VB.m_nTVComp[6] - 2;

								if (_VB.m_pTV[7])
								{
									nTex++;
									Size += _VB.m_nTVComp[7] * 4;
									Format += CRC_FV_TEXCOORD(7, _VB.m_nTVComp[7]);
									bNonStandard |= _VB.m_nTVComp[7] - 2;
								}
							}
						}
					}
#endif // DEF_CRC_MAXTEXTURES
				}
			}
		}
	}

	ProcNr += nTex * 16;
	if (bNonStandard) ProcNr += 80;

	if (_VB.m_pMI) { Format += CRC_VF_MATRIXI; Size += (_VB.m_nMWComp > 4) ? 8 : 4; bNoFunction = 1; }
	if (_VB.m_pMW) { Format += CRC_FV_MATRIXW(_VB.m_nMWComp); Size += 4*_VB.m_nMWComp; bNoFunction = 1; }

	if (bNoFunction)
		ProcNr = 160;

	_RetVSize = Size;
	_RetVF = Format;
	_RetCollectFunction = ProcNr;

//	Collect_GetFunctionAll(ProcNr);	// Dummy to get rid of unresolved external.
}

int CRC_Core::Geometry_GetCollectFunction(int _VF)
{
	MAUTOSTRIP(CRC_Core_Geometry_GetCollectFunction, 0);
	Error("Geometry_GetCollectFunction", "Not implemented.");
	return 0;
}

int CRC_Core::Geometry_BuildTriangleListFromPrimitivesCount(CRCPrimStreamIterator &_StreamIterate)
{
	// Primitive stream
	
	int DestPos = 0;
	
	if (_StreamIterate.IsValid())
	{		
		while(1)
		{
			const uint16* pPrim = _StreamIterate.GetCurrentPointer();
			
			switch(_StreamIterate.GetCurrentType())
			{
			case CRC_RIP_TRIFAN :
				{
					int nV = *pPrim;
					const uint16* piV = pPrim + 1;
					int nTri = nV-2;
					if (nTri > 0)
					{
						piV++;
						piV++;
						for(int t = 0; t < nTri; t++)
						{
							DestPos++;
							DestPos++;
							piV++;
							DestPos++;
						}
					}
				}
				break;
			case CRC_RIP_TRISTRIP :
				{
					int nV = *pPrim;
					int nTri = nV-2;
					if (nTri > 0)
					{
						for(int v = 2; v < nV; v++)
						{
							if (v & 1)
							{
								DestPos++;
								DestPos++;
								DestPos++;
							}
							else
							{
								DestPos++;
								DestPos++;
								DestPos++;
							}
						}
					}
				}
				break;
			case CRC_RIP_TRIANGLES :
				{
					int nV = (*pPrim) * 3;
					DestPos += nV;
				}
				break;
			default :
				{
					Error("Geometry_BuildTriangleListFromPrimitives", CStrF("Unsupported primitive: %d", _StreamIterate.GetCurrentType()));
					break;
				}
			}

			if (!_StreamIterate.Next())
				break;
		}
		
	}

	return DestPos;
}

// -------------------------------------------------------------------
bool CRC_Core::Geometry_BuildTriangleListFromPrimitives(CRCPrimStreamIterator &_StreamIterate, uint16* _pDest, int& _nMaxDestRetSize)
{
	MAUTOSTRIP(CRC_Core_Geometry_BuildTriangleListFromPrimitives, false);
	// Returns true if the whole stream was processed and fit into destination buffer.
	// _pPrim pointer to source primitive stream, points to the next primitive to process when function returns.
	// _nPrimRetPos source primitive count, contains the primitive number left when function returns.
	// _pDest destination buffer to put triangle list in.
	// _nMaxDestRetSize size of destination buffer, updated with the number of _INDICES_ written.
	
	// Primitive stream
	
	int DestPos = 0;
	int DestMax = _nMaxDestRetSize;
	
	bool bIsComplete = false;
	uint16* pDest = _pDest;
	
	if (_StreamIterate.IsValid())
	{		
		while(1)
		{
			const uint16* pPrim = _StreamIterate.GetCurrentPointer();
			
			switch(_StreamIterate.GetCurrentType())
			{
			case CRC_RIP_TRIFAN :
				{
					int nV = *pPrim;
					const uint16* piV = pPrim + 1;
					int nTri = nV-2;
					if (nTri > 0)
					{
						if (DestPos + nTri*3 > DestMax) goto End;
						int iv0 = *(piV++);
						int iv1 = *(piV++);
						for(int t = 0; t < nTri; t++)
						{
							pDest[DestPos++] = iv0;
							pDest[DestPos++] = iv1;
							int iv2 = *(piV++);
							pDest[DestPos++] = iv2;
							iv1 = iv2;
						}
					}
				}
				break;
			case CRC_RIP_TRISTRIP :
				{
					int nV = *pPrim;
					const uint16* piV = pPrim + 1;
					int nTri = nV-2;
					if (nTri > 0)
					{
						if (DestPos + nTri*3 > DestMax) goto End;
						
						for(int v = 2; v < nV; v++)
						{
							if (v & 1)
							{
								pDest[DestPos++] = piV[v];
								pDest[DestPos++] = piV[v-1]; 
								pDest[DestPos++] = piV[v-2]; 
							}
							else
							{
								pDest[DestPos++] = piV[v-2]; 
								pDest[DestPos++] = piV[v-1]; 
								pDest[DestPos++] = piV[v];
							}
						}
					}
				}
				break;
			case CRC_RIP_TRIANGLES :
				{
					int nV = (*pPrim) * 3;
					const uint16* piV = pPrim + 1;
					if (DestPos + nV > DestMax) goto End;
					memcpy(&pDest[DestPos], piV, nV*2);
					DestPos += nV;
				}
				break;
			default :
				{
					Error("Geometry_BuildTriangleListFromPrimitives", CStrF("Unsupported primitive: %d", _StreamIterate.GetCurrentType()));
					break;
				}
			}

			if (!_StreamIterate.Next())
				break;
		}
		
	}

	bIsComplete = true;

End:
	_nMaxDestRetSize = DestPos;
	return bIsComplete;
}


// -------------------------------------------------------------------
#define AddIndexParamsNI \
uint16* Usage;\
uint8* pVMapBool;\
uint16* pVMapIndex;\
uint16 CurrentVertex;\
uint16 *CurrentUse;\
uint8 *CurrentUseBool;


//uint8 a;
//uint8 b;

#define AddIndexNI(Index) \
	CurrentUse = pVMapIndex + Index;\
	CurrentUseBool = pVMapBool + Index;\
	if ((*(CurrentUseBool)) != CurrentUsagePlace)\
	{\
		*CurrentUseBool = CurrentUsagePlace;\
		*CurrentUse = CurrentVertex;\
		*Out = CurrentVertex;\
		*Usage = Index;\
		++Usage;\
		++CurrentVertex;\
		++Out;\
	}\
	else\
	{\
		*Out = *CurrentUse;\
		++Out;\
	}\

#define AddIndexOnlyMapNI(Index) \
	CurrentUse = pVMapIndex + Index;\
	CurrentUseBool = pVMapBool + Index;\
	if ((*(CurrentUseBool)) != CurrentUsagePlace)\
	{\
		*CurrentUseBool = CurrentUsagePlace;\
		*CurrentUse = CurrentVertex;\
		*Usage = Index;\
		++Usage;\
		++CurrentVertex;\
	}\


uint8 CurrentUsagePlace = 1;
int MaxUse = 0;


/*
// -------------------------------------------------------------------
#define ADDVERTEX(iv) { int iV = (iv); uint8 a = pVMap[iV >> 3]; uint8 b = (1 << (iV & 7)); \
	if (!(a & b)) { if (!a) piVClear[nVClear++] = iV >> 3; pVMap[iV >> 3] |= b; piVertUse[nVU++] = iV; } }
*/
int CRC_Core::Geometry_BuildVertexUsage(int _bStream, const uint16* _piPrim, int _nPrim, uint16*& _piVRet)
{
	// NOTE: This method assumes m_Geom and m_bGeomArrayAllUsed should be considered.

	MAUTOSTRIP(CRC_Core_Geometry_BuildVertexUsage, 0);

	// Check AllUsed hint.
	if (m_bGeomArrayAllUsed)
	{
//		int nVUse = (_bStream) ?
//			Geometry_GetVertexCount_Primitives(_piPrim, _nPrim) :
//			Geometry_GetVertexCount(_piPrim, _nPrim);

		_piVRet = NULL;
		return m_Geom.m_nV;
	}

	CRCPrimStreamIterator StreamIterate(_piPrim, _nPrim);

	if (_bStream)
	{
	if (!StreamIterate.IsValid())
		return 0;
	}

	AddIndexParamsNI;

	// Grow working arrays if necessary.

	int nVUseMapBool = m_Geom.m_nV;//((m_Geom.m_nV + 7) >> 3);

	if (nVUseMapBool > m_lBVUMap.Len())
	{
		int nSize = nVUseMapBool + (nVUseMapBool >> 1);
		m_lBVUMap.SetLen(nSize);
		m_lBVUMapIndex.SetLen(nSize);
		FillChar(m_lBVUMap.GetBasePtr(), m_lBVUMap.ListSize(), 0);
		MaxUse = 0;
		CurrentUsagePlace = 1;
	}

	if (m_Geom.m_nV > MaxUse)
		MaxUse = m_Geom.m_nV;

	int UseLen = m_Geom.m_nV * 3;

	if (UseLen > 65536)
		UseLen = 65536;

	// Grow vertex-clip mask array if necessary
	if (UseLen > m_lClipVertexMask.Len())
	{
		m_lClipVertexMask.SetLen(UseLen + 2048);
	}

	if (UseLen > m_liVUse.Len())
	{
		int nLen = UseLen + (UseLen >> 1);
		m_liVUse.SetLen(nLen);
		FillChar(m_liVUse.GetBasePtr(), m_liVUse.ListSize(), 0);
	}

	pVMapBool = m_lBVUMap.GetBasePtr();
	pVMapIndex = m_lBVUMapIndex.GetBasePtr();

	CurrentVertex = 0;

	Usage = m_liVUse.GetBasePtr();


	if (_bStream)
	{
		// Primitive stream
		while(1)
		{
			int nV = 0;
			const uint16* piV = 0;

			const uint16* pPrim = StreamIterate.GetCurrentPointer();

			switch(StreamIterate.GetCurrentType())
			{
			case CRC_RIP_TRIFAN:
			case CRC_RIP_TRISTRIP:
			case CRC_RIP_POLYGON:
			case CRC_RIP_QUADSTRIP:
				{
					nV = *pPrim;
					piV = pPrim+1;
				}
				break;
			case CRC_RIP_TRIANGLES :
				{
					nV = (*pPrim)*3;
					piV = pPrim+1;
				}
				break;
			case CRC_RIP_QUADS :
				{
					piV = pPrim+1;
					nV = (*pPrim)*4;
				}
				break;
			default :
				{
					Error("Geometry_BuildVertexUsage", CStrF("Unsupported primitive: %d", StreamIterate.GetCurrentType()));
					break;
				}
			}

			if (piV) 
			{
				const uint16 *piVEnd = piV + nV;
				while(piV < piVEnd)
				{
#if defined(PLATFORM_WIN_PC) && !defined(M_RTM)
					int iv = *piV;
					if (iv > UseLen)
						M_BREAKPOINT;
#endif
					AddIndexOnlyMapNI(*piV);
					++piV;
				}
			}

			if (!StreamIterate.Next())
				break;

		}
	}
	else
	{
		// Index soup
		const uint16* piV = _piPrim;
		const uint16* piVEnd = piV + _nPrim;
		while(piV < piVEnd)
		{
#ifdef _DEBUG
//			int iv = *piV;
//			if (iv > nVUse) Error("Geometry_BuildVertexUsage", CStrF("Vertex index out of range.(2) (%d / %d)", iv, nVUse));
#endif
			AddIndexOnlyMapNI(*piV);
			piV++;
		}
	}

	// Clear working map
	if (CurrentUsagePlace == 255)
	{
		memset(pVMapBool,0, MaxUse);
		MaxUse = 0;
		CurrentUsagePlace = 1;
	}
	else
	{
		++CurrentUsagePlace;
	}

	_piVRet = m_liVUse.GetBasePtr();
	return Usage - _piVRet;
}

int CRC_Core::Geometry_BuildVertexUsageMap(int _bStream, const uint16* _pPrim, int _nPrim, uint16*& _piVRet, uint16 *&_pIndexRemap, int _MaxV)
{
	// NOTE: This method assumes m_Geom and m_bGeomArrayAllUsed should be considered.

	MAUTOSTRIP(CRC_Core_Geometry_BuildVertexUsage, 0);

	// Check AllUsed hint.
	if (m_bGeomArrayAllUsed)
	{
//		int nVUse = (_bStream) ?
//			Geometry_GetVertexCount_Primitives(_piPrim, _nPrim) :
//			Geometry_GetVertexCount(_piPrim, _nPrim);

		_piVRet = NULL;
		return m_Geom.m_nV;
	}

	AddIndexParamsNI;

	// Grow working arrays if necessary.

	int nVUseMapBool = _MaxV;//((_MaxV + 7) >> 3);

	if (nVUseMapBool > m_lBVUMap.Len())
	{
		nVUseMapBool = nVUseMapBool + (nVUseMapBool >> 1);
		if (nVUseMapBool > 0x10000)
			nVUseMapBool = 0x10000;
		m_lBVUMap.SetLen(nVUseMapBool);
		m_lBVUMapIndex.SetLen(nVUseMapBool);
		FillChar(m_lBVUMap.GetBasePtr(), m_lBVUMap.ListSize(), 0);
		MaxUse = 0;
		CurrentUsagePlace = 1;
	}

	if (_MaxV > MaxUse)
		MaxUse = _MaxV;

	int UseLen = _MaxV;

	if (UseLen > 65536)
		UseLen = 65536;

	if (UseLen > m_liVUse.Len())
	{
		m_liVUse.SetLen(UseLen);
	}

	pVMapBool = m_lBVUMap.GetBasePtr();
	pVMapIndex = m_lBVUMapIndex.GetBasePtr();
	_pIndexRemap = pVMapIndex;

	CurrentVertex = 0;

	Usage = m_liVUse.GetBasePtr();


	if (_bStream)
	{
		CRCPrimStreamIterator StreamIterate(_pPrim, _nPrim);

		if (!StreamIterate.IsValid())
			return 0;

		// Primitive stream
		while(1)
		{
			int nV = 0;
			const uint16* piV = 0;

			const uint16* pPrim = StreamIterate.GetCurrentPointer();

			switch(StreamIterate.GetCurrentType())
			{
			case CRC_RIP_TRIFAN:
			case CRC_RIP_TRISTRIP:
			case CRC_RIP_POLYGON:
			case CRC_RIP_QUADSTRIP:
				{
					nV = *pPrim;
					piV = pPrim+1;
				}
				break;
			case CRC_RIP_TRIANGLES :
				{
					nV = (*pPrim)*3;
					piV = pPrim+1;
				}
				break;
			case CRC_RIP_QUADS :
				{
					piV = pPrim+1;
					nV = (*pPrim)*4;
				}
				break;
			default :
				{
					Error("Geometry_BuildVertexUsage", CStrF("Unsupported primitive: %d", StreamIterate.GetCurrentType()));
					break;
				}
			}

			if (piV) 
			{
				const uint16 *piVEnd = piV + nV;
				while(piV < piVEnd)
				{
#if defined(PLATFORM_WIN_PC) && !defined(M_RTM)
					int iv = *piV;
					if (iv > UseLen)
						M_BREAKPOINT;
#endif
					AddIndexOnlyMapNI(*piV);
					++piV;
				}
			}

			if (!StreamIterate.Next())
				break;

		}
	}
	else
	{
		// Index soup
		const uint16* piV = _pPrim;
		const uint16* piVEnd = piV + _nPrim;
		while(piV < piVEnd)
		{
#ifdef _DEBUG
//			int iv = *piV;
//			if (iv > nVUse) Error("Geometry_BuildVertexUsage", CStrF("Vertex index out of range.(2) (%d / %d)", iv, nVUse));
#endif
			AddIndexOnlyMapNI(*piV);
			piV++;
		}
	}

	// Clear working map
	if (CurrentUsagePlace == 255)
	{
		memset(pVMapBool,0, MaxUse);
		MaxUse = 0;
		CurrentUsagePlace = 1;
	}
	else
	{
		++CurrentUsagePlace;
	}

	_piVRet = m_liVUse.GetBasePtr();
	return Usage - _piVRet;
}

int CRC_Core::Geometry_BuildVertexUsageCount(int _bStream, const uint16* _piPrim, int _nPrim)
{
	// NOTE: This method assumes m_Geom and m_bGeomArrayAllUsed should be considered.

	MAUTOSTRIP(CRC_Core_Geometry_BuildVertexUsage, 0);

	CRCPrimStreamIterator StreamIterate(_piPrim, _nPrim);

	if (_bStream)
	{
		if (!StreamIterate.IsValid())
			return 0;
	}

	AddIndexParamsNI;

	// Grow working arrays if necessary.

	int nVUseMapBool = m_Geom.m_nV;//((m_Geom.m_nV + 7) >> 3);

	if (nVUseMapBool > m_lBVUMap.Len())
	{
		int nSize = nVUseMapBool + (nVUseMapBool >> 1);
		m_lBVUMap.SetLen(nSize);
		m_lBVUMapIndex.SetLen(nSize);
		FillChar(m_lBVUMap.GetBasePtr(), m_lBVUMap.ListSize(), 0);
		MaxUse = 0;
		CurrentUsagePlace = 1;
	}

	if (m_Geom.m_nV > MaxUse)
		MaxUse = m_Geom.m_nV;

	int UseLen = m_Geom.m_nV * 3;

	if (UseLen > 65536)
		UseLen = 65536;

	if (UseLen > m_liVUse.Len())
	{
		int nLen = UseLen + (UseLen >> 1);
		m_liVUse.SetLen(nLen);
		FillChar(m_liVUse.GetBasePtr(), m_liVUse.ListSize(), 0);
	}

	pVMapBool = m_lBVUMap.GetBasePtr();
	pVMapIndex = m_lBVUMapIndex.GetBasePtr();


	CurrentVertex = 0;

	Usage = m_liVUse.GetBasePtr();


	if (_bStream)
	{
		// Primitive stream
		while(1)
		{
			int nV = 0;
			const uint16* piV = 0;

			const uint16* pPrim = StreamIterate.GetCurrentPointer();

			switch(StreamIterate.GetCurrentType())
			{
			case CRC_RIP_TRIFAN:
			case CRC_RIP_TRISTRIP:
			case CRC_RIP_POLYGON:
			case CRC_RIP_QUADSTRIP:
				{
					nV = *pPrim;
					piV = pPrim+1;
				}
				break;
			case CRC_RIP_TRIANGLES :
				{
					nV = (*pPrim)*3;
					piV = pPrim+1;
				}
				break;
			case CRC_RIP_QUADS :
				{
					piV = pPrim+1;
					nV = (*pPrim)*4;
				}
				break;
			default :
				{
					Error("Geometry_BuildVertexUsage", CStrF("Unsupported primitive: %d", StreamIterate.GetCurrentType()));
					break;
				}
			}

			if (piV) 
			{
				const uint16 *piVEnd = piV + nV;
				while(piV < piVEnd)
				{
#if defined(PLATFORM_WIN_PC) && !defined(M_RTM)
					int iv = *piV;
					if (iv > UseLen)
						M_BREAKPOINT;
#endif
					AddIndexOnlyMapNI(*piV);
					++piV;
				}
			}

			if (!StreamIterate.Next())
				break;

		}
	}
	else
	{
		// Index soup
		const uint16* piV = _piPrim;
		const uint16* piVEnd = piV + _nPrim;
		while(piV < piVEnd)
		{
#ifdef _DEBUG
//			int iv = *piV;
//			if (iv > nVUse) Error("Geometry_BuildVertexUsage", CStrF("Vertex index out of range.(2) (%d / %d)", iv, nVUse));
#endif
			AddIndexOnlyMapNI(*piV);
			piV++;
		}
	}

	// Clear working map
	if (CurrentUsagePlace == 255)
	{
		memset(pVMapBool,0, MaxUse);
		MaxUse = 0;
		CurrentUsagePlace = 1;
	}
	else
	{
		++CurrentUsagePlace;
	}

	return Usage - m_liVUse.GetBasePtr();
}

int CRC_Core::Geometry_BuildVertexUsage2(int _bStream, const uint16* _piPrim, int _nPrim, uint16*& _piVRet, int _MaxV)
{
	// NOTE: This method does NOT use m_Geom and m_bGeomArrayAllUsed.

	MAUTOSTRIP(CRC_Core_Geometry_BuildVertexUsage2, 0);


	CRCPrimStreamIterator StreamIterate(_piPrim, _nPrim);

	if (_bStream)
	{
	if (!StreamIterate.IsValid())
		return 0;
	}

	AddIndexParamsNI;

	// Grow working arrays if necessary.

	int nVUseMapBool = _MaxV;

	if (nVUseMapBool > m_lBVUMap.Len())
	{
		int nSize = nVUseMapBool + (nVUseMapBool >> 1);
		m_lBVUMap.SetLen(nSize);
		m_lBVUMapIndex.SetLen(nSize);
		FillChar(m_lBVUMap.GetBasePtr(), m_lBVUMap.ListSize(), 0);
		MaxUse = 0;
		CurrentUsagePlace = 1;
	}

	if (_MaxV > MaxUse)
		MaxUse = _MaxV;

	int UseLen = _MaxV * 3;

	if (UseLen > 65536)
		UseLen = 65536;

	// Grow vertex-clip mask array if necessary
	if (UseLen > m_lClipVertexMask.Len())
	{
		m_lClipVertexMask.SetLen(UseLen + 2048);
	}

	if (UseLen > m_liVUse.Len())
	{
		int nLen = UseLen + (UseLen >> 1);
		m_liVUse.SetLen(nLen);
		FillChar(m_liVUse.GetBasePtr(), m_liVUse.ListSize(), 0);
	}

	pVMapBool = m_lBVUMap.GetBasePtr();
	pVMapIndex = m_lBVUMapIndex.GetBasePtr();


	CurrentVertex = 0;

	Usage = m_liVUse.GetBasePtr();


	if (_bStream)
	{
		// Primitive stream
		while(1)
		{
			int nV = 0;
			const uint16* piV = 0;

			const uint16* pPrim = StreamIterate.GetCurrentPointer();

			switch(StreamIterate.GetCurrentType())
			{
			case CRC_RIP_TRIFAN:
			case CRC_RIP_TRISTRIP:
			case CRC_RIP_POLYGON:
			case CRC_RIP_QUADSTRIP:
				{
					nV = *pPrim;
					piV = pPrim+1;
				}
				break;
			case CRC_RIP_TRIANGLES :
				{
					nV = (*pPrim)*3;
					piV = pPrim+1;
				}
				break;
			case CRC_RIP_QUADS :
				{
					piV = pPrim+1;
					nV = (*pPrim)*4;
				}
				break;
			default :
				{
					Error("Geometry_BuildVertexUsage2", CStrF("Unsupported primitive: %d", StreamIterate.GetCurrentType()));
					break;
				}
			}

			if (piV) 
			{
				const uint16 *piVEnd = piV + nV;
				while(piV < piVEnd)
				{
#if defined(PLATFORM_WIN_PC) && !defined(M_RTM)
					int iv = *piV;
					if (iv > UseLen)
						M_BREAKPOINT;
#endif
					AddIndexOnlyMapNI(*piV);
					++piV;
				}
			}

			if (!StreamIterate.Next())
				break;

		}
	}
	else
	{
		// Index soup
		const uint16* piV = _piPrim;
		const uint16* piVEnd = piV + _nPrim;
		while(piV < piVEnd)
		{
#ifdef _DEBUG
//			int iv = *piV;
//			if (iv > nVUse) Error("Geometry_BuildVertexUsage", CStrF("Vertex index out of range.(2) (%d / %d)", iv, nVUse));
#endif
			AddIndexOnlyMapNI(*piV);
			piV++;
		}
	}

	// Clear working map
	if (CurrentUsagePlace == 255)
	{
		memset(pVMapBool,0, MaxUse);
		MaxUse = 0;
		CurrentUsagePlace = 1;
	}
	else
	{
		++CurrentUsagePlace;
	}

	_piVRet = m_liVUse.GetBasePtr();
	return Usage - _piVRet;
}


#define AddIndexParams \
uint16* Usage;\
uint16 *Out;\
uint8* pVMapBool;\
uint16* pVMapIndex;\
uint16 CurrentVertex;\
uint16 *CurrentUse;\
uint8 *CurrentUseBool;

//uint8 a;
//uint8 b;

#define AddIndex(Index) \
	CurrentUse = pVMapIndex + Index;\
	CurrentUseBool = pVMapBool + Index;\
	if ((*(CurrentUseBool)) != CurrentUsagePlace)\
	{\
		*CurrentUseBool = CurrentUsagePlace;\
		*CurrentUse = CurrentVertex;\
		*Out = CurrentVertex;\
		*Usage = Index;\
		++Usage;\
		++CurrentVertex;\
		++Out;\
	}\
	else\
	{\
		*Out = *CurrentUse;\
		++Out;\
	}\

#define AddIndexOnlyMap(Index) \
	CurrentUse = pVMapIndex + Index;\
	CurrentUseBool = pVMapBool + Index;\
	if ((*(CurrentUseBool)) != CurrentUsagePlace)\
	{\
		*CurrentUseBool = CurrentUsagePlace;\
		*CurrentUse = CurrentVertex;\
		*Usage = Index;\
		++Usage;\
		++CurrentVertex;\
	}\

#define AddIndexIsMapped(Index) \
	*Out = *(pVMapIndex + Index);\
	++Out;\

// Returns Num Processed PrimitiveIndices
bool CRC_Core::Geometry_BuildVertexUsageAndTriangleList_Stream(CRCPrimStreamIterator &_StreamIterate, uint16* _pUsage, int &_UsageSize, uint16* _pRetPrimList, int &_RetListSize, int _StartVertexIndex)
{
	MAUTOSTRIP(CRC_Core_Geometry_BuildVertexUsageAndTriangleList_Stream, false);

	AddIndexParams;

	// Grow working arrays if necessary.

	int nVUseMapBool = m_Geom.m_nV;//((m_Geom.m_nV + 7) >> 3);

	if (nVUseMapBool > m_lBVUMap.Len())
	{
		int nSize = nVUseMapBool + (nVUseMapBool >> 1);
		m_lBVUMap.SetLen(nSize);
		m_lBVUMapIndex.SetLen(nSize);
		FillChar(m_lBVUMap.GetBasePtr(), m_lBVUMap.ListSize(), 0);
		MaxUse = 0;
		CurrentUsagePlace = 1;
	}

	if (m_Geom.m_nV > MaxUse)
		MaxUse = m_Geom.m_nV;

	pVMapBool = m_lBVUMap.GetBasePtr();
	pVMapIndex = m_lBVUMapIndex.GetBasePtr();

	// Primitive stream
	bool Finished = false;

	CurrentVertex = _StartVertexIndex;

	int NumTri;
	int LeftInBuffer = _RetListSize;
	int FirstIndex;
	
	int Comp;
	int i;
	const uint16* piV;
	
	Usage = _pUsage;
	uint16* MaxUsage = _pUsage + _UsageSize;
	
	Out = _pRetPrimList;
	
	if (_StreamIterate.IsValid())
	{		
		while(1)
		{
			const uint16* CurrentPrim = _StreamIterate.GetCurrentPointer();
			
			switch(_StreamIterate.GetCurrentType())
			{
			case CRC_RIP_TRIFAN:
				{
					if (*(CurrentPrim) < 3)
						break;
					
					NumTri = *(CurrentPrim) - 2;
					piV = (CurrentPrim + 1);
					
					Comp = LeftInBuffer < (MaxUsage - Usage) ? LeftInBuffer : (MaxUsage - Usage);
					
					if (Comp < (NumTri * 3))
					{
						goto End;
					}				
					
					FirstIndex = (*piV);
					++piV;
					
					AddIndexOnlyMap(FirstIndex);
					AddIndexOnlyMap((*piV));
					
					//					LockCollectIndexBuffers(NumToRender < LeftInBuffer ? NumToRender : LeftInBuffer);
					
					for (i = 0; i < NumTri; ++i)
					{
						AddIndexIsMapped(FirstIndex);
						//					AddIndex(FirstIndex);
						AddIndexIsMapped((*piV));
						//					AddIndex((*piV));
						++piV;
						AddIndex((*piV));
					}				
					
					LeftInBuffer -= NumTri * 3;
					
				}
				break;
			case CRC_RIP_TRISTRIP:
				{
					if (*(CurrentPrim) < 3)
						break;
					
					NumTri = *(CurrentPrim) - 2;
					piV = (CurrentPrim + 1);
					
					Comp = LeftInBuffer < (MaxUsage - Usage) ? LeftInBuffer : (MaxUsage - Usage);
					
					if (Comp < (NumTri * 3))
					{
						goto End;
					}				
					
					// Map First
					AddIndexOnlyMap((*piV));
					
					for (i = 0; i < NumTri >> 1; ++i)
					{
						AddIndexIsMapped((*(piV)));	// 1 Mapped Last Round
						++piV;
						AddIndex((*(piV)));			// 2
						++piV;
						AddIndex((*(piV)));			// 3
						--piV;
						
						AddIndexIsMapped((*(piV))); // 2 Already mapped
						piV += 2;
						AddIndex((*(piV)));			// 4 
						--piV;
						AddIndexIsMapped((*(piV))); // 3 Already mapped
					}
					
					// One Left to render
					if (NumTri - ((NumTri >> 1) << 1))
					{
						AddIndexIsMapped((*(piV)));
						++piV;
						AddIndex((*(piV)));
						++piV;
						AddIndex((*(piV)));
						--piV;
					}
					
					LeftInBuffer -= NumTri * 3;
				}
				break;
			case CRC_RIP_TRIANGLES:
				{
					NumTri = *(CurrentPrim );
					piV = (CurrentPrim + 1);
					
					Comp = LeftInBuffer < (MaxUsage - Usage) ? LeftInBuffer : (MaxUsage - Usage);
					
					if (Comp < (NumTri * 3))
					{
						goto End;
					}				
					
					for (i = 0; i < NumTri; ++i)
					{
						AddIndex((*piV));
						++piV;
						AddIndex((*piV));
						++piV;
						AddIndex((*piV));
						++piV;
					}				
					
					LeftInBuffer -= NumTri * 3;
				}
				break;
				// Unsupported
				//		case CRC_RIP_POLYGON :
				//		case CRC_RIP_QUADSTRIP:
				/*		case CRC_RIP_QUADS :
				{
				int nQuads = pPrim[Pos+1];
				piV = &pPrim[Pos+2];
				nV = nQuads*4;
				}
				break;*/
			default :
				{
					Error("Geometry_BuildVertexUsage", CStrF("Unsupported primitive: %d", _StreamIterate.GetCurrentType()));
					break;
				}
		}
		
		if (!_StreamIterate.Next())
		{
			Finished = true;
			break;
		}
	}
	
	}
End:
	if (CurrentUsagePlace == 255)
	{
		memset(pVMapBool,0, MaxUse);
		MaxUse = 0;
		CurrentUsagePlace = 1;
	}
	else
	{
		++CurrentUsagePlace;
	}

	_UsageSize = (Usage - _pUsage);
	_RetListSize = _RetListSize - LeftInBuffer;
	return Finished;
	
}

// Returns Num Processed PrimitiveIndices
bool CRC_Core::Geometry_BuildVertexUsageAndTriangleList_List(
	const uint16* &_pPrimList, 
	int &_nIndices, 
	uint16* _pUsage, 
	int &_UsageSize, 
	uint16* _pRetPrimList, 
	int &_RetListSize, 
	int StartVertexIndex, 
	int PrimitiveSize)
{
	MAUTOSTRIP(CRC_Core_Geometry_BuildVertexUsageAndTriangleList_List, false);
	AddIndexParams;


	// Grow working arrays if necessary.

	int nVUseMapBool = m_Geom.m_nV;//((m_Geom.m_nV + 7) >> 3);

	if (nVUseMapBool > m_lBVUMap.Len())
	{
		int nSize = nVUseMapBool + (nVUseMapBool >> 1);
		m_lBVUMap.SetLen(nSize);
		m_lBVUMapIndex.SetLen(nSize);
		FillChar(m_lBVUMap.GetBasePtr(), m_lBVUMap.ListSize(), 0);
		MaxUse = 0;
		CurrentUsagePlace = 1;
	}

	if (m_Geom.m_nV > MaxUse)
		MaxUse = m_Geom.m_nV;

	pVMapBool = m_lBVUMap.GetBasePtr();
	pVMapIndex = m_lBVUMapIndex.GetBasePtr();

	CurrentVertex = StartVertexIndex;

	bool Finished = true;
	
	int ToProcess = _nIndices;

	if (ToProcess > _RetListSize)
	{
		Finished = false;
		ToProcess = _RetListSize;
	}

	if (ToProcess > _UsageSize)
	{
		Finished = false;
		ToProcess = _UsageSize;
	}

	ToProcess = (ToProcess / PrimitiveSize) * PrimitiveSize;

	// Primitive stream
	const uint16* StreamEnd = _pPrimList + ToProcess;

	Usage = _pUsage;

	Out = _pRetPrimList;

	while(_pPrimList < StreamEnd)
	{
		CurrentUse = pVMapIndex + (*(_pPrimList));
		CurrentUseBool = pVMapBool + (*(_pPrimList));
		if ((*(CurrentUseBool)) != CurrentUsagePlace)
		{
			*CurrentUseBool = CurrentUsagePlace;
			*CurrentUse = CurrentVertex;
			*Out = CurrentVertex;
			*Usage = (*(_pPrimList));
			++Usage;
			++CurrentVertex;
			++Out;
		}
		else
		{
			*Out = *CurrentUse;
			++Out;
		}
//		AddIndex((*(_pPrimList)));

		++_pPrimList;
		--_nIndices;
	}

	if (CurrentUsagePlace == 255)
	{
		memset(pVMapBool,0, MaxUse);
		MaxUse = 0;
		CurrentUsagePlace = 1;
	}
	else
	{
		++CurrentUsagePlace;
	}
	_UsageSize = (Usage - _pUsage);
	_RetListSize = ToProcess;

	return Finished;
}

// -------------------------------------------------------------------
int CRC_Core::Geometry_GetVertexCount(const uint16* _pIndices, int _nIndices)
{
	MAUTOSTRIP(CRC_Core_Geometry_GetVertexCount, 0);
	if (m_Geom.m_nV)
		return m_Geom.m_nV;
	else
	{
		int nV = 0;
		for(int i = 0; i < _nIndices; i++)
			if (_pIndices[i] > nV) nV = _pIndices[i];
		return nV+1;
	}
}

int CRC_Core::Geometry_GetVertexCount_Primitives(const uint16* _pPrimStream, int _StreamLen)
{
	MAUTOSTRIP(CRC_Core_Geometry_GetVertexCount_Primitives, 0);
	if (m_Geom.m_nV)
		return m_Geom.m_nV;
	else
	{
		int MaxV = 0;

		CRCPrimStreamIterator StreamIterate(_pPrimStream, _StreamLen);
		
		if (!StreamIterate.IsValid())
			return 0;		

		int nV = 0;
		const uint16* piV = NULL;

		while(1)
		{
			const uint16* pPrim = StreamIterate.GetCurrentPointer();

			switch(StreamIterate.GetCurrentType())
			{
			case CRC_RIP_TRIANGLES :
				{
					nV = (*pPrim)*3;
					piV = pPrim+1;
				}
				break;
			case CRC_RIP_QUADS :
				{
					nV = (*pPrim) * 4;
					piV = pPrim+1;
				}
				break;
			case CRC_RIP_TRISTRIP :
			case CRC_RIP_TRIFAN :
			case CRC_RIP_QUADSTRIP :
			case CRC_RIP_POLYGON :
				{
					nV = *pPrim;
					piV = pPrim + 1;
				}
				break;
			default :
				{
					piV = NULL;
					break;
				}
			}

			if (piV)
				for(int i = 0; i < nV; i++)
					if (piV[i] > MaxV) MaxV = piV[i];

			if (!StreamIterate.Next())
				break;
		}

		return MaxV+1;
	}
}

// -------------------------------------------------------------------
int CRC_Core::Geometry_GetComponentMask(const CRC_VertexBuffer& _VB)
{
	MAUTOSTRIP(CRC_Core_Geometry_GetComponentMask, 0);
	int Comp = CRC_CLIPARRAY_VERTEX | 
		((_VB.m_pCol) ? CRC_CLIPARRAY_COLOR : 0) |
		((_VB.m_pSpec) ? CRC_CLIPARRAY_SPECULAR : 0);
//		((_VB.m_pFog) ? CRC_CLIPARRAY_FOG : 0);

	if (_VB.m_pTV[0])
	{
		Comp |= CRC_CLIPARRAY_TVERTEX0;
		if (_VB.m_pTV[1])
		{
			Comp |= CRC_CLIPARRAY_TVERTEX1;
			if (_VB.m_pTV[2])
			{
				Comp |= CRC_CLIPARRAY_TVERTEX2;
				if (_VB.m_pTV[3])
					Comp |= CRC_CLIPARRAY_TVERTEX3;
			}
		}
	}

	return Comp;
}

void CRC_Core::Geometry_MoveToClipBuffer(int _iV, int _iClip)
{
	MAUTOSTRIP(CRC_Core_Geometry_MoveToClipBuffer, MAUTOSTRIP_VOID);
	m_ClipV[_iClip] = m_Geom.m_pV[_iV];

	if (m_Geom.m_pTV[0])
	{
		int iComp = 0;
		for(; iComp < m_Geom.m_nTVComp[0]; iComp++)
			m_ClipTV0[_iClip][iComp] = m_Geom.m_pTV[0][_iV*m_Geom.m_nTVComp[0] + iComp];
		for(; iComp < 3; iComp++)
			m_ClipTV0[_iClip][iComp] = 0;
		for(; iComp < 4; iComp++)
			m_ClipTV0[_iClip][iComp] = 1.0f;

		if (m_Geom.m_pTV[1])
		{
			int iComp = 0;
			for(; iComp < m_Geom.m_nTVComp[1]; iComp++)
				m_ClipTV1[_iClip][iComp] = m_Geom.m_pTV[1][_iV*m_Geom.m_nTVComp[1] + iComp];
			for(; iComp < 3; iComp++)
				m_ClipTV1[_iClip][iComp] = 0;
			for(; iComp < 4; iComp++)
				m_ClipTV1[_iClip][iComp] = 1.0f;

			if (m_Geom.m_pTV[2])
			{
				int iComp = 0;
				for(; iComp < m_Geom.m_nTVComp[2]; iComp++)
					m_ClipTV2[_iClip][iComp] = m_Geom.m_pTV[2][_iV*m_Geom.m_nTVComp[2] + iComp];
				for(; iComp < 3; iComp++)
					m_ClipTV2[_iClip][iComp] = 0;
				for(; iComp < 4; iComp++)
					m_ClipTV2[_iClip][iComp] = 1.0f;

				if (m_Geom.m_pTV[3])
				{
					int iComp = 0;
					for(; iComp < m_Geom.m_nTVComp[3]; iComp++)
						m_ClipTV3[_iClip][iComp] = m_Geom.m_pTV[3][_iV*m_Geom.m_nTVComp[3] + iComp];
					for(; iComp < 3; iComp++)
						m_ClipTV3[_iClip][iComp] = 0;
					for(; iComp < 4; iComp++)
						m_ClipTV3[_iClip][iComp] = 1.0f;
				}
			}
		}
	}

	if (m_Geom.m_pN)
	{
		m_ClipN[_iClip] = m_Geom.m_pN[_iV];
	}

	if (m_Geom.m_pCol)
	{
		m_ClipCol[_iClip].k[0] = fp4((m_Geom.m_pCol[_iV] >> 16) & 0xff) / 255.0f;
		m_ClipCol[_iClip].k[1] = fp4((m_Geom.m_pCol[_iV] >> 8) & 0xff) / 255.0f;
		m_ClipCol[_iClip].k[2] = fp4((m_Geom.m_pCol[_iV] >> 0) & 0xff) / 255.0f;
		m_ClipCol[_iClip].k[3] = fp4((m_Geom.m_pCol[_iV] >> 24) & 0xff) / 255.0f;
	}
	if (m_Geom.m_pSpec)
	{
		m_ClipSpec[_iClip].k[0] = fp4((m_Geom.m_pSpec[_iV] >> 16) & 0xff) / 255.0f;
		m_ClipSpec[_iClip].k[1] = fp4((m_Geom.m_pSpec[_iV] >> 8) & 0xff) / 255.0f;
		m_ClipSpec[_iClip].k[2] = fp4((m_Geom.m_pSpec[_iV] >> 0) & 0xff) / 255.0f;
		m_ClipSpec[_iClip].k[3] = fp4((m_Geom.m_pSpec[_iV] >> 24) & 0xff) / 255.0f;
	}
/*	if (m_Geom.m_pFog)
	{
		m_ClipFog[_iClip] = m_Geom.m_pFog[_iV];
	}*/
}

void CRC_Core::Internal_IndexedTriangles2Wires(uint16* _pPrim, int _nTri)
{
	MAUTOSTRIP(CRC_Core_Internal_IndexedTriangles2Wires, MAUTOSTRIP_VOID);
	uint16 iWires[1024+6];

	DRenderTopClass Attrib_Push();

		DRenderTopClass Attrib_TextureID(0, 0);
		DRenderTopClass Attrib_TextureID(1, 0);

		CPixel32* pCol = m_Geom.m_pCol;
		CPixel32 Col = m_GeomColor;

		m_Geom.m_pCol = NULL;
		DRenderTopClass Geometry_Color(0xffffffff);

		int iW = 0;
		int t = 0;
		for(int i = 0; i < _nTri; i++)
		{
			iWires[iW++] = _pPrim[t];
			iWires[iW++] = _pPrim[t+1];
			iWires[iW++] = _pPrim[t+1];
			iWires[iW++] = _pPrim[t+2];
			iWires[iW++] = _pPrim[t+2];
			iWires[iW++] = _pPrim[t+0];
			if (iW >= 1024)
			{
				DRenderTopClass Render_IndexedWires(iWires, iW);
				iW = 0;
			}
			t += 3;
		}

		if (iW)
			DRenderTopClass Render_IndexedWires(iWires, iW);

		m_Geom.m_pCol = pCol;
		DRenderTopClass Geometry_Color(Col);

	DRenderTopClass Attrib_Pop();
}

void CRC_Core::Internal_IndexedTriStrip2Wires(uint16* _pPrim, int _Len)
{
	MAUTOSTRIP(CRC_Core_Internal_IndexedTriStrip2Wires, MAUTOSTRIP_VOID);
	uint16 iWires[4096];

	int iW = 0;

	iWires[iW++] = _pPrim[0];
	iWires[iW++] = _pPrim[1];
	for(int i = 2; i < _Len; i++)
	{
		iWires[iW++] = _pPrim[i-1];
		iWires[iW++] = _pPrim[i];
		iWires[iW++] = _pPrim[i-2];
		iWires[iW++] = _pPrim[i];
	}

	DRenderTopClass Attrib_Push();

		DRenderTopClass Attrib_TextureID(0, 0);
		DRenderTopClass Attrib_TextureID(1, 0);

		CPixel32* pCol = m_Geom.m_pCol;
		CPixel32 Col = m_GeomColor;

		m_Geom.m_pCol = NULL;
		DRenderTopClass Geometry_Color(0xffffffff);

		DRenderTopClass Render_IndexedWires(iWires, iW);

		m_Geom.m_pCol = pCol;
		DRenderTopClass Geometry_Color(Col);

	DRenderTopClass Attrib_Pop();
}

void CRC_Core::Internal_IndexedTriFan2Wires(uint16* _pPrim, int _Len)
{
	MAUTOSTRIP(CRC_Core_Internal_IndexedTriFan2Wires, MAUTOSTRIP_VOID);
	uint16 iWires[4096];

	int iW = 0;

	for(int i = 1; i < _Len; i++)
	{
		iWires[iW++] = _pPrim[0];
		iWires[iW++] = _pPrim[i];
		if (i > 1)
		{
			iWires[iW++] = _pPrim[i-1];
			iWires[iW++] = _pPrim[i];
		}
	}

	DRenderTopClass Render_IndexedWires(iWires, iW);
}

void CRC_Core::Internal_IndexedPrimitives2Wires(uint16* _pPrimStream, int _StreamLen)
{
	MAUTOSTRIP(CRC_Core_Internal_IndexedPrimitives2Wires, MAUTOSTRIP_VOID);
	CRCPrimStreamIterator StreamIterate(_pPrimStream, _StreamLen);
	
	if (!StreamIterate.IsValid())
		return;		
	
	DRenderTopClass Attrib_Push();
	DRenderTopClass Attrib_TextureID(0, 0);
	DRenderTopClass Attrib_TextureID(1, 0);

	CPixel32* pCol = m_Geom.m_pCol;
	CPixel32 Col = m_GeomColor;
	
	m_Geom.m_pCol = NULL;
	DRenderTopClass Geometry_Color(0xffffffff);

	while(1)
	{
		const uint16* pPrim = StreamIterate.GetCurrentPointer();
		switch(StreamIterate.GetCurrentType())
		{
		case CRC_RIP_TRIANGLES :
			{
				int nV = (*pPrim)*3;
				const uint16* piV = pPrim+1;
				DRenderCoreTopClass Internal_IndexedTriangles2Wires((uint16 *)piV, nV);
			}
			break;
		case CRC_RIP_QUADS :
			{
//				int nV = (*pPrim)*4;
//				const uint16* piV = pPrim+1;
			}
			break;
		case CRC_RIP_TRISTRIP :
			{
				int nV = *pPrim;
				const uint16* piV = pPrim+1;
				DRenderCoreTopClass Internal_IndexedTriStrip2Wires((uint16*)piV, nV);
			}
			break;
		case CRC_RIP_TRIFAN :
			{
				int nV = *pPrim;
				const uint16* piV = pPrim+1;
				DRenderCoreTopClass Internal_IndexedTriFan2Wires((uint16 *)piV, nV);
			}
			break;
		case CRC_RIP_QUADSTRIP :
		case CRC_RIP_POLYGON :
			{
//				int nV = *pPrim*4;
//				const uint16* piV = pPrim+1;
			}
			break;
		default :
			{
				break;
			}
		}

		if (!StreamIterate.Next())
			break;
	}

	m_Geom.m_pCol = pCol;
	DRenderTopClass Geometry_Color(Col);

	DRenderTopClass Attrib_Pop();
}

// -------------------------------------------------------------------
void CRC_Core::Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles)
{
	MAUTOSTRIP(CRC_Core_Render_IndexedTriangles, MAUTOSTRIP_VOID);
	if (Clip_IsEnabled())
	{
		// We assume this has been done already.
//		int nV = Geometry_GetVertexCount(_pTriVertIndices, _nTriangles*3);
//		if (Clip_InitVertexMasks(nV, m_Geom.m_pV, NULL)) return;
//		if (!m_ClipMask)


		if (!m_Geom.m_pV) return;
		m_ClipColor = m_GeomColor;
		int Comp = Geometry_GetComponentMask(m_Geom);
		int bCull = (m_CurrentAttrib.m_Flags & CRC_FLAGS_CULL);

		m_nClipTriangles += _nTriangles;
		const uint16* pClipMask = m_lClipVertexMask.GetBasePtr();

		for(int t = 0; t < _nTriangles; t++)
		{
			int iv0 = _pTriVertIndices[t*3 + 0];
			int iv1 = _pTriVertIndices[t*3 + 1];
			int iv2 = _pTriVertIndices[t*3 + 2];
			// Completely clipped away?
			if (pClipMask[iv0] &
				pClipMask[iv1] &
				pClipMask[iv2]) continue;

			if (bCull) 
				if (!Clip_IsVisible(&m_Geom.m_pV[iv0], &m_Geom.m_pV[iv1], &m_Geom.m_pV[iv2])) continue;

			// No!, prepare clipping vertex-buffers.
			int ClipMask = 0;
			for(int v = 0; v < 3; v++)
			{
				int iv = _pTriVertIndices[t*3 + v];
				ClipMask |= pClipMask[iv];
				Geometry_MoveToClipBuffer(iv, v);
			}
			m_ClipMask = ClipMask;
			Clip_RenderPolygon(3, Comp);
		}
	}
}

void CRC_Core::Render_IndexedTriangleStrip(uint16* _pIndices, int _Len)
{
	MAUTOSTRIP(CRC_Core_Render_IndexedTriangleStrip, MAUTOSTRIP_VOID);
	ConOut("(CRC_Core::Render_IndexedTriangleStrip) This function is currently a no-op.");
}

void CRC_Core::Render_IndexedWires(uint16* _pIndices, int _Len)
{
	MAUTOSTRIP(CRC_Core_Render_IndexedWires, MAUTOSTRIP_VOID);
	ConOut("(CRC_Core::Render_IndexedWires) This function is currently a no-op.");
}

void CRC_Core::Render_IndexedPolygon(uint16* _pIndices, int _Len)
{
	MAUTOSTRIP(CRC_Core_Render_IndexedPolygon, MAUTOSTRIP_VOID);
	ConOut("(CRC_Core::Render_IndexedPolygon) This function is currently a no-op.");
}

void CRC_Core::Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen)
{
	MAUTOSTRIP(CRC_Core_Render_IndexedPrimitives, MAUTOSTRIP_VOID);
	if (Clip_IsEnabled())
	{
		// We assume this has been done already.
//		int nV = Geometry_GetVertexCount(_pTriVertIndices, _nTriangles*3);
//		if (Clip_InitVertexMasks(nV, m_Geom.m_pV, NULL)) return;
//		if (!m_ClipMask)

		if (!m_Geom.m_pV) return;

		CRCPrimStreamIterator StreamIterate(_pPrimStream, _StreamLen);
	
		if (!StreamIterate.IsValid())
			return;		

		m_ClipColor = m_GeomColor;
		int Comp = Geometry_GetComponentMask(m_Geom);
		int bCull = (m_CurrentAttrib.m_Flags & CRC_FLAGS_CULL);

		int nV = 0;
		const uint16* piV = NULL;
		const uint16* pClipMask = m_lClipVertexMask.GetBasePtr();

		while(1)
		{
			const uint16* pPrim = StreamIterate.GetCurrentPointer();
			switch(StreamIterate.GetCurrentType())
			{
			case CRC_RIP_TRIANGLES :
				{
					nV = (*pPrim)*3;
					piV = pPrim+1;

					for(int t = 0; t < nV; t++)
					{
						m_nClipTriangles++;
						int iv0 = piV[t*3 + 0];
						int iv1 = piV[t*3 + 1];
						int iv2 = piV[t*3 + 2];
						// Completely clipped away?
						if (pClipMask[iv0] &
							pClipMask[iv1] &
							pClipMask[iv2]) continue;

						if (bCull) 
							if (!Clip_IsVisible(&m_Geom.m_pV[iv0], &m_Geom.m_pV[iv1], &m_Geom.m_pV[iv2])) continue;

						// No!, prepare clipping vertex-buffers.
						m_ClipMask = 
							pClipMask[iv0] |
							pClipMask[iv1] |
							pClipMask[iv2];

						Geometry_MoveToClipBuffer(iv0, 0);
						Geometry_MoveToClipBuffer(iv1, 1);
						Geometry_MoveToClipBuffer(iv2, 2);
						Clip_RenderPolygon(3, Comp);
						t += 3;
					}
				}
				break;
			case CRC_RIP_QUADS :
				{
					nV = (*pPrim)*4;
					piV = pPrim+1;

					for(int t = 0; t < nV; t++)
					{
						int iv0 = piV[t*3 + 0];
						int iv1 = piV[t*3 + 1];
						int iv2 = piV[t*3 + 2];
						int iv3 = piV[t*3 + 3];
						// Completely clipped away?
						if (pClipMask[iv0] &
							pClipMask[iv1] &
							pClipMask[iv2] &
							pClipMask[iv3]) continue;

						if (bCull) 
							if (!Clip_IsVisible(&m_Geom.m_pV[iv0], &m_Geom.m_pV[iv1], &m_Geom.m_pV[iv2])) continue;

						// No!, prepare clipping vertex-buffers.
						m_ClipMask = 
							pClipMask[iv0] |
							pClipMask[iv1] |
							pClipMask[iv2] |
							pClipMask[iv3];

						Geometry_MoveToClipBuffer(iv0, 0);
						Geometry_MoveToClipBuffer(iv1, 1);
						Geometry_MoveToClipBuffer(iv2, 2);
						Geometry_MoveToClipBuffer(iv3, 3);
						Clip_RenderPolygon(4, Comp);
						t += 4;
					}
				}
				break;
			case CRC_RIP_TRISTRIP :
				{
					nV = *pPrim;
					piV = pPrim+1;
					m_nClipTriangles += nV-2;

					int iv0, iv1, iv2;
					for(int v = 2; v < nV; v++)
					{
						if (v & 1)
						{
							iv2 = piV[v-2]; 
							iv1 = piV[v-1]; 
							iv0 = piV[v];
						}
						else
						{
							iv0 = piV[v-2]; 
							iv1 = piV[v-1]; 
							iv2 = piV[v];
//							iv0 = iv1;
//							iv1 = iv2;
						}

//						iv2 = piV[v];
						// Completely clipped away?
						if (!(pClipMask[iv0] &
							pClipMask[iv1] &
							pClipMask[iv2]))
						{
							if (bCull) 
								if (!Clip_IsVisible(&m_Geom.m_pV[iv0], &m_Geom.m_pV[iv1], &m_Geom.m_pV[iv2])) continue;

							// No!, prepare clipping vertex-buffers.
							m_ClipMask = 
								pClipMask[iv0] |
								pClipMask[iv1] |
								pClipMask[iv2];

							Geometry_MoveToClipBuffer(iv0, 0);
							Geometry_MoveToClipBuffer(iv1, 1);
							Geometry_MoveToClipBuffer(iv2, 2);
							Clip_RenderPolygon(3, Comp);
						}
					}
				}
				break;
			case CRC_RIP_TRIFAN :
				{
					nV = *pPrim;
					piV = pPrim+1;
					m_nClipTriangles += nV-2;

					int iv0, iv1, iv2;
					for(int v = 2; v < nV; v++)
					{
						iv0 = piV[0]; 
						iv1 = piV[v-1]; 
						iv2 = piV[v];

//						iv2 = piV[v];
						// Completely clipped away?
						if (!(pClipMask[iv0] &
							pClipMask[iv1] &
							pClipMask[iv2]))
						{
							if (bCull) 
								if (!Clip_IsVisible(&m_Geom.m_pV[iv0], &m_Geom.m_pV[iv1], &m_Geom.m_pV[iv2])) continue;

							// No!, prepare clipping vertex-buffers.
							m_ClipMask = 
								pClipMask[iv0] |
								pClipMask[iv1] |
								pClipMask[iv2];

							Geometry_MoveToClipBuffer(iv0, 0);
							Geometry_MoveToClipBuffer(iv1, 1);
							Geometry_MoveToClipBuffer(iv2, 2);
							Clip_RenderPolygon(3, Comp);
						}
					}
				}
				break;
			case CRC_RIP_QUADSTRIP :
			case CRC_RIP_POLYGON :
				{
					Error("Render_IndexedPrimitives", CStrF("Clipping not supported for primtive %d", StreamIterate.GetCurrentType()));
				}
				break;
			default :
				{
					break;
				}
			}

			if (!StreamIterate.Next())
				break;
		}
	}
}

void CRC_Core::Render_VertexBuffer(int _VBID)
{
	MAUTOSTRIP(CRC_Core_Render_VertexBuffer, MAUTOSTRIP_VOID);
	// TODO: Emulate VB rendering..
}

// -------------------------------------------------------------------
void CRC_Core::Render_Wire(const CVec3Dfp4& _v0, const CVec3Dfp4& _v1, CPixel32 _Color)
{
	MAUTOSTRIP(CRC_Core_Render_Wire, MAUTOSTRIP_VOID);
}

void CRC_Core::Render_WireStrip(const CVec3Dfp4* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color)
{
	MAUTOSTRIP(CRC_Core_Render_WireStrip, MAUTOSTRIP_VOID);
	DRenderTopClass Attrib_TextureID(0, 0);
	DRenderTopClass Attrib_TextureID(1, 0);
	if (m_AttribChanged) DRenderCoreTopClass Attrib_Update();
	if (m_MatrixChanged) DRenderCoreTopClass Matrix_Update();

	if (_piV)
	{
		int iv1 = 1;
		for(int iv0 = 0; iv1 < _nVertices; iv0++, iv1++)
			DRenderTopClass Render_Wire(_pV[_piV[iv0]], _pV[_piV[iv1]], _Color);
	}
	else
	{
		int iv1 = 1;
		for(int iv0 = 0; iv1 < _nVertices; iv0++, iv1++)
			DRenderTopClass Render_Wire(_pV[iv0], _pV[iv1], _Color);
	}
}

void CRC_Core::Render_WireLoop(const CVec3Dfp4* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color)
{
	MAUTOSTRIP(CRC_Core_Render_WireLoop, MAUTOSTRIP_VOID);
	DRenderTopClass Attrib_TextureID(0, 0);
	DRenderTopClass Attrib_TextureID(1, 0);
	if (m_AttribChanged) DRenderCoreTopClass Attrib_Update();
	if (m_MatrixChanged) DRenderCoreTopClass Matrix_Update();

	if (_piV)
	{
		int iv0 = _nVertices-1;
		for(int iv1 = 0; iv1 < _nVertices; iv1++)
		{
			DRenderTopClass Render_Wire(_pV[_piV[iv0]], _pV[_piV[iv1]], _Color);
			iv0 = iv1;
		}
	}
	else
	{
		int iv0 = _nVertices-1;
		for(int iv1 = 0; iv1 < _nVertices; iv1++)
		{
			DRenderTopClass Render_Wire(_pV[iv0], _pV[iv1], _Color);
			iv0 = iv1;
		}
	}
}

void CRC_Core::OcclusionQuery_Begin(int _QueryID)
{
	MAUTOSTRIP(CRC_Core_OcclusionQuery_Begin, MAUTOSTRIP_VOID);
}

void CRC_Core::OcclusionQuery_End()
{
	MAUTOSTRIP(CRC_Core_OcclusionQuery_End, MAUTOSTRIP_VOID);
}

int CRC_Core::OcclusionQuery_GetVisiblePixelCount(int _QueryID)
{
	MAUTOSTRIP(CRC_Core_OcclusionQuery_GetVisiblePixelCount, 0);
	return 0;
}

int CRC_Core::OcclusionQuery_Rect(int _QueryID, CRct _Rct, fp4 _z)
{
	MAUTOSTRIP(CRC_Core_OcclusionQuery_Rect, 0);
	if (!(Caps_Flags() & CRC_CAPS_FLAGS_OCCLUSIONQUERY))
		Error("OcclusionQuery_Rect", "Invalid operation.");

	//ConOut(CStrF("Query %d, Rct %d,%d,%d,%d, Z %f", _QueryID, _Rct, _z));

	// Render..
	DRenderTopClass Matrix_SetMode(CRC_MATRIX_MODEL);
	DRenderTopClass Matrix_SetUnit();
	DRenderTopClass Attrib_Push();

	CRC_Attributes Attr;
	Attr.SetDefault();
	DRenderCoreTopClass Attrib_Set(&Attr);

	DRenderTopClass Attrib_Enable(CRC_FLAGS_ZCOMPARE);
	DRenderTopClass Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	DRenderTopClass Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE);

	CVec3Dfp4 V[4];
	static uint16 lPrimQuad[6] = { 0, 1, 2, 0, 2, 3 };

	CRC_Viewport* pVP = Viewport_Get();

	fp4 xScale = pVP->GetXScale() * 0.5f;
	fp4 yScale = pVP->GetYScale() * 0.5f;
	fp4 xs = _z / xScale;
	fp4 ys = _z / yScale;

	V[0][0] = _Rct.p0.x * xs;
	V[0][1] = _Rct.p0.y * ys;
	V[0][2] = _z;
	V[1][0] = _Rct.p1.x * xs;
	V[1][1] = _Rct.p0.y * ys;
	V[1][2] = _z;
	V[2][0] = _Rct.p1.x * xs;
	V[2][1] = _Rct.p1.y * ys;
	V[2][2] = _z;
	V[3][0] = _Rct.p0.x * xs;
	V[3][1] = _Rct.p1.y * ys;
	V[3][2] = _z;

	DRenderTopClass Geometry_Clear();
	DRenderTopClass Geometry_Color(0);
	DRenderTopClass Geometry_VertexArray(V, 4, true);

	DRenderTopClass OcclusionQuery_Begin(_QueryID);

	DRenderTopClass Render_IndexedTriangles(lPrimQuad, 2);

	DRenderTopClass Geometry_VertexArray(NULL, 0, 0);

	DRenderTopClass OcclusionQuery_End();

	DRenderTopClass Attrib_Pop();
	return DRenderTopClass OcclusionQuery_GetVisiblePixelCount(_QueryID);
}

bool CRC_Core::ReadDepthPixels(int _x, int _y, int _w, int _h, fp4* _pBuffer)
{
	MAUTOSTRIP(CRC_Core_ReadDepthPixels, false);
	return false;
}

// -------------------------------------------------------------------
void CRC_Core::Con_EnableFlags(int _Flags)
{
	MAUTOSTRIP(CRC_Core_Con_EnableFlags, MAUTOSTRIP_VOID);
	m_Mode.m_Flags |= _Flags;
	DRenderCoreTopClass Attrib_GlobalUpdate();
}

void CRC_Core::Con_DisableFlags(int _Flags)
{
	MAUTOSTRIP(CRC_Core_Con_DisableFlags, MAUTOSTRIP_VOID);
	m_Mode.m_Flags &= (~_Flags);
	DRenderCoreTopClass Attrib_GlobalUpdate();
}

void CRC_Core::Con_Filter(int _Filter)
{
	MAUTOSTRIP(CRC_Core_Con_Filter, MAUTOSTRIP_VOID);
	if (m_Mode.m_Filter == _Filter) return;
	m_Mode.m_Filter = _Filter;
	DRenderCoreTopClass Attrib_GlobalUpdate();
	DRenderCoreTopClass Texture_MakeAllDirty();

	// Notify all subsystems
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys) 
		pSys->System_BroadcastMessage(CSS_Msg(CSS_MSG_PRECACHEHINT_TEXTURES));
};

void CRC_Core::Con_EchoZFormats()
{
	MAUTOSTRIP(CRC_Core_Con_EchoZFormats, MAUTOSTRIP_VOID);
	ConOut("Z-Buffer formats:");
	for(int i=1; i < 0x40000000; i += i)
		if (m_Caps_ZFormats & i)
			ConOut(CImage::GetFormatName(i));
};

void CRC_Core::Con_EchoDisplayFormats()
{
	MAUTOSTRIP(CRC_Core_Con_EchoDisplayFormats, MAUTOSTRIP_VOID);
	ConOut("Display formats:");
	for(int i=1; i < 0x40000000; i += i)
		if (m_Caps_DisplayFormats & i)
			ConOut(CImage::GetFormatName(i));
};

void CRC_Core::Con_EchoTextureFormats()
{
	MAUTOSTRIP(CRC_Core_Con_EchoTextureFormats, MAUTOSTRIP_VOID);
	ConOut("Texture formats:");
	for(int i=1; i < 0x40000000; i += i)
		if (m_Caps_TextureFormats & i)
			ConOut(CImage::GetFormatName(i));
};

void CRC_Core::Con_ShowGamma(int _v)
{
	MAUTOSTRIP(CRC_Core_Con_ShowGamma, MAUTOSTRIP_VOID);
	m_bShowGamma = _v != 0;
}

void CRC_Core::Con_DumpTextures(int _v)
{
	MAUTOSTRIP(CRC_Core_Con_DumpTextures, MAUTOSTRIP_VOID);

	int nTxt = m_pTC->GetIDCapacity();
	for(int i = 0; i < nTxt; i++)
	{
		int Flags = m_pTC->GetTextureFlags(i);

		if ((_v & 1) && !(Flags & CTC_TXTIDFLAGS_PRECACHE))
			continue;

		if (Flags & CTC_TXTIDFLAGS_ALLOCATED)
		{

			CStr Name = m_pTC->GetName(i);
			CImage Desc;
			int nMip = 0;
			m_pTC->GetTextureDesc(i, &Desc, nMip);
			CTC_TextureProperties Prop;
			m_pTC->GetTextureProperties(i, Prop);

			fp4 PixelSize = Desc.GetPixelSize();
			if (Desc.GetMemModel() & IMAGE_MEM_COMPRESSED)
			{
				if (Desc.GetMemModel() & IMAGE_MEM_COMPRESSTYPE_S3TC)
				{
					if (Desc.GetFormat() & IMAGE_FORMAT_ALPHA)
						PixelSize = 1;
					else
						PixelSize = 0.5f;
				}
			}

//			int PicMip = (Prop.m_Flags & CTC_TEXTUREFLAGS_NOPICMIP) ? 0 : m_lPicMips[Prop.m_iPicMipGroup];
			fp4 PixelsOrg = Desc.GetWidth() * Desc.GetHeight();
//			fp4 Pixels = Max(1, Desc.GetWidth() >> PicMip) * Max(1, Desc.GetHeight()  >> PicMip);

			if (!(Prop.m_Flags & CTC_TEXTUREFLAGS_NOMIPMAP))
			{
				PixelsOrg = PixelsOrg*1.3333f;
//				Pixels = Pixels*1.3333f;
			}

			fp4 SizeOrg = PixelsOrg * PixelSize;
//			fp4 Size = Pixels * PixelSize;
			fp4 Size = Texture_GetMem(i).m_BestCase;

			LogFile(CStrF("ID %.4x, Flags %.4x, PropFlags %.4x, PropPMG %.2d, %.4dx%.4d, Fmt %.8x, ApproxBytes %.10d/%.10d, %s", 
				i, Flags, Prop.m_Flags, Prop.m_iPicMipGroup, Desc.GetWidth(), Desc.GetHeight(), Desc.GetFormat(), 
				RoundToInt(Size), RoundToInt(SizeOrg), Name.Str() ));
		}
		else
		{
//			LogFile(CStrF("ID %.4x, Flags %.4x", i, Flags));
		}
	}
}

void CRC_Core::Register(CScriptRegisterContext & _RegContext)
{
	CRenderContext::Register(_RegContext);
	MAUTOSTRIP(CRC_Core_Register, MAUTOSTRIP_VOID);
	_RegContext.RegConstant("R_FLG_WIRE", (int)CRC_GLOBALFLAGS_WIRE);
	_RegContext.RegConstant("R_FLG_TEXTURE", (int)CRC_GLOBALFLAGS_TEXTURE);
	_RegContext.RegConstant("FILTER_POINT", (int)CRC_GLOBALFILTER_POINT);
	_RegContext.RegConstant("FILTER_BILINEAR", (int)CRC_GLOBALFILTER_BILINEAR);
	_RegContext.RegConstant("FILTER_TRILINEAR", (int)CRC_GLOBALFILTER_TRILINEAR);

	_RegContext.RegFunction("r_enable", this, &CRC_Core::Con_EnableFlags);
	_RegContext.RegFunction("r_disable", this, &CRC_Core::Con_DisableFlags);
	_RegContext.RegFunction("r_filter", this, &CRC_Core::Con_Filter);
	_RegContext.RegFunction("r_zformats", this, &CRC_Core::Con_EchoZFormats);
	_RegContext.RegFunction("r_displayformats", this, &CRC_Core::Con_EchoDisplayFormats);
	_RegContext.RegFunction("r_textureformats", this, &CRC_Core::Con_EchoTextureFormats);
	_RegContext.RegFunction("r_showgamma", this, &CRC_Core::Con_ShowGamma);
	_RegContext.RegFunction("r_dumptextures", this, &CRC_Core::Con_DumpTextures);
};

