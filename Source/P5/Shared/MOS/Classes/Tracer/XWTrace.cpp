#include "PCH.h"
#include "XWTrace.h"
#include "../../MSystem/Misc/MRegistry.h"
#include "../GameWorld/WEvilTemplate.h"

//#define XWLIGHT_NOLIGHTBOUND
#define XWLIGHT_NOOBJECTBOUND

//#pragma optimize("", off)
//#pragma inline_depth(0)
static void TransformAABB(const CMat4Dfp32& _Mat, const CBox3Dfp32& _Box, CBox3Dfp32& _DestBox)
{
	// Transform axis-aligned bounding box.

	CVec3Dfp32 E;
	CVec3Dfp32 C;
	_Box.m_Max.Lerp(_Box.m_Min, 0.5f, C);
	_Box.m_Max.Sub(_Box.m_Min, E);
	E *= 0.5f;

	C *= _Mat;

	_DestBox.m_Max = 0;
	for(int axis = 0; axis < 3; axis++)
		for(int k = 0; k < 3; k++)
			_DestBox.m_Max.k[k] += M_Fabs(_Mat.k[axis][k]*E[axis]);

	_DestBox.m_Min = -_DestBox.m_Max;
	_DestBox.m_Min += C;
	_DestBox.m_Max += C;


	if(_DestBox.m_Min[0] > _DestBox.m_Max[0] ||
	   _DestBox.m_Min[1] > _DestBox.m_Max[1] ||
	   _DestBox.m_Min[2] > _DestBox.m_Max[2])
	{
	   LogFile("Screwed up the box.");
	   LogFile("    " + _Box.GetString());
	   LogFile("    " + _DestBox.GetString());
	   LogFile("    " + _Mat.GetString());
	}

/*   LogFile("Transformed box:");
   LogFile("    " + _Box.GetString());
   LogFile("    " + _DestBox.GetString());
   LogFile("    " + _Mat.GetString());*/
}


void CXWC_TracerObject::InitTraceBound(CXWC_Tracer* _pTracer, const CBox3Dfp32& _Box)
{
}

// -------------------------------------------------------------------
//  CXWC_Tracer_Light
// -------------------------------------------------------------------
void CXWC_Tracer_Light::SetType(int _Type)
{
	switch(m_Type)
	{
	case XWC_LIGHTTYPE_FAKESKYRADIOSITY : 
		{
			m_TraceLen = 768;
			m_MaxRange = 1000000000.0;
		}
		break;
	case XWC_LIGHTTYPE_PARALLELL :
		{
			m_TraceLen = 2048;
			m_MaxRange = 1000000000.0;
		}
		break;
	default :
		{
			m_TraceLen = 0;
			m_MaxRange = 512;
		}
	}
}

void CXWC_Tracer_Light::Parse(CRegistry *_pReg)
{
	m_Pos = Entity_GetPosition(_pReg);

	int iKey;
	if ((iKey = _pReg->FindIndex("LIGHT_ID")) >= 0)
		m_LightID = _pReg->GetValuef(iKey);
	
	if ((iKey = _pReg->FindIndex("LIGHT_RANGE")) >= 0)
		m_MaxRange = _pReg->GetValuef(iKey);

	if ((iKey = _pReg->FindIndex("LIGHT")) >= 0)
	{
//LogFile(_pReg->GetKeyValue(iKey));
		CVec3Dfp32 Col; Col.ParseColor(_pReg->GetValue(iKey));
		m_Intensity.CompMul(Col, m_Intensity);
		m_Intensity *= 1.0f / 256.0f;
/*					CVec3Dfp64 i;
		i.ParseString(_pReg->GetKeyValue(iKey));
		if (i.k[1] == 0) i.k[1] = i.k[0];
		if (i.k[2] == 0) i.k[2] = i.k[0];
		m_Intensity = CVec3Dfp32(i.k[0], i.k[1], i.k[2]);*/
	}
	if ((iKey = _pReg->FindIndex("LIGHT_COLOR")) >= 0)
	{
		CVec3Dfp32 Col; Col.ParseColor(_pReg->GetValue(iKey), true);
		m_Intensity.CompMul(Col, m_Intensity);
/*					CVec3Dfp64 Col;
		Col.ParseString(_pReg->GetKeyValue(iKey));
		for(int i = 0; i < 3; i++) m_Intensity.k[i] *= Col.k[i];*/
	}
	if ((iKey = _pReg->FindIndex("LIGHT_DIRECTION")) >= 0)
	{
		CVec3Dfp32 p; p.ParseString(_pReg->GetValue(iKey));
		p.Normalize();
		CMat4Dfp32 m;
		p.SetMatrixRow(m, 0);
		if (p[0] != 0.0)
			CVec3Dfp32(0,1,0).SetMatrixRow(m, 1);
		else
			CVec3Dfp32(1,0,0).SetMatrixRow(m, 1);
		m.RecreateMatrix(0, 1);
		GetPos().SetMatrixRow(m, 3);
		m_Pos = m;
/*					CVec3Dfp64 Col;
		Col.ParseString(_pReg->GetKeyValue(iKey));
		for(int i = 0; i < 3; i++) m_Intensity.k[i] *= Col.k[i];*/
	}
	if ((iKey = _pReg->FindIndex("LIGHT_TYPE")) >= 0)
	{
		m_Type = _pReg->GetValuei(iKey);
	}
	if ((iKey = _pReg->FindIndex("LIGHT_TRACELEN")) >= 0)
	{
		m_TraceLen = _pReg->GetValuei(iKey);
	}
	if ((iKey = _pReg->FindIndex("LIGHT_PROJMAP")) >= 0)
	{
		m_ProjMapSurface = _pReg->GetValue(iKey);
	}
	if ((iKey = _pReg->FindIndex("LIGHT_PROJMAPORIGIN")) >= 0)
	{
		m_ProjMapOrigin.ParseString( _pReg->GetValue(iKey));
	}
	if ((iKey = _pReg->FindIndex("LIGHT_FLAGS")) >= 0)
	{
		m_Flags = CXR_Light::ParseFlags(_pReg->GetValue(iKey));
	}

	m_SpotWidth = Min(89.0f, Max(1.0f, 0.5f * _pReg->GetValuef("LIGHT_SPOTWIDTH", 90)));
	m_SpotHeight = Min(89.0f, Max(1.0f, 0.5f * _pReg->GetValuef("LIGHT_SPOTHEIGHT", 90)));
	m_SpotWidth = M_Tan(m_SpotWidth / 180.0f * _PI);
	m_SpotHeight = M_Tan(m_SpotHeight / 180.0f * _PI);

/*	if ((iKey = _pReg->FindIndex("ORIGIN")) >= 0)
	{
		CVec3Dfp32 p; p.ParseString(_pReg->GetValue(iKey));
		GetPos() = p;
	}*/
}

fp32 GetMinBoxesDistance(const CBox3Dfp32& _A, const CBox3Dfp32& _B)
{
	CVec3Dfp32 VMin;
	for(int i = 0; i < 3; i++)
	{
		fp32 d = (_A.m_Min[i] > _B.m_Max[i]) ? (_A.m_Min[i] - _B.m_Max[i]) :  _B.m_Min[i] - _A.m_Max[i];
		if (d < 0) d = 0;
		VMin[i] = d;
	}

	return VMin.Length();
}

static bool EqualPlanes(const CPlane3Dfp32& _p1, const CPlane3Dfp32& _p2, fp32 _Epsilon)
{
	if ((Abs(_p1.Distance(_p2.GetPointInPlane())) < _Epsilon) &&
		Abs(_p1.n * _p2.n - 1.0f) < _Epsilon) return true;
	return false;
}


static int ExtrudeBoxContour(const CBox3Dfp32& _Box, const CVec3Dfp32& _Vector, CPlane3Dfp32* _pP)
{
/*	CBox3Dfp64 Box8;
	_Box.m_Min.Assignfp64(Box8.m_Min);
	_Box.m_Max.Assignfp64(Box8.m_Max);*/

	CVec3Dfp32 V[9];
	_Box.GetVertices(V);

	const int EdgeTab[12][2] = 
	{
		{ 0, 1 },
		{ 2, 3 },
		{ 0, 2 },
		{ 1, 3 },
		{ 4, 5 },
		{ 6, 7 },
		{ 4, 6 },
		{ 5, 7 },
		{ 0, 4 },
		{ 1, 5 },
		{ 2, 6 },
		{ 3, 7 }
	};

	int nP = 0;

	{
		for(int i = 0; i < 12; i++)
		{
			int iv0 = EdgeTab[i][0];
			int iv1 = EdgeTab[i][1];

			// Check if the points are colinear.
			CVec3Dfp32 E;
			V[iv1].Sub(V[iv0], E);
			if ((E / _Vector).Length() < 0.001f)
			{
//				LogFile("Parallell edge " + E.GetString() + _Vector.GetString());
				continue;
			}

			CPlane3Dfp32 P;
			P.Create(V[iv0], V[iv1], V[iv0] + _Vector);

			int Mask = P.GetArrayPlaneSideMask_Epsilon(V, 8, 0.01f);
			Mask &= 3;
			if (Mask == 3)
			{
//				LogFile("PlaneCut " + P.GetString());
				continue;
			}
			
			if (Mask ==  1)
			{
				P.Inverse();
			}

/*			int p = 0;
			for(p = 0; p < nP; p++)
				if (EqualPlanes(_pP[p], P, 0.001f)) break;

			if (p == nP)*/
			_pP[nP++] = P;
//			LogFile(CStrF("PlaneAdd %d, ", nP) + P.GetString());
		}
	}

	return nP;
//LogFile(CStrF("nPlanes %d (Cube %d, Edges %d)", nPlanes, ncp, nPlanes-ncp));
}

int CreatePolyhedronFromBox(const CBox3Dfp32& _Box, const CVec3Dfp32& _Point, CPlane3Dfp32* _pP)
{
/*	CBox3Dfp64 Box8;
	_Box.m_Min.Assignfp64(Box8.m_Min);
	_Box.m_Max.Assignfp64(Box8.m_Max);*/

	CVec3Dfp32 V[9];
	_Box.GetVertices(V);
	V[8] = _Point;


//	{ for(int v = 0; v < 9; v++) LogFile(V[v].GetString()); }

	int nPlanes = 0;
	{
		for(int i = 0; i < 3; i++)
		{
			if (_Point[i] > _Box.m_Min[i])
			{
				CVec3Dfp32 P(0);
				CVec3Dfp32 N(0);
				P[i] = _Box.m_Min[i];
				N[i] = -1;
				_pP[nPlanes++].CreateNV(N, P);
			}

			if (_Point[i] < _Box.m_Max[i])
			{
				CVec3Dfp32 P(0);
				CVec3Dfp32 N(0);
				P[i] = _Box.m_Max[i];
				N[i] = 1;
				_pP[nPlanes++].CreateNV(N, P);
			}
		}
	}

	if (_Box.GetMinSqrDistance(_Point) < 0.001f) return nPlanes;

	const int EdgeTab[12][2] = 
	{
		{ 0, 1 },
		{ 2, 3 },
		{ 0, 2 },
		{ 1, 3 },
		{ 4, 5 },
		{ 6, 7 },
		{ 4, 6 },
		{ 5, 7 },
		{ 0, 4 },
		{ 1, 5 },
		{ 2, 6 },
		{ 3, 7 }
	};


	{
		for(int i = 0; i < 12; i++)
		{
			int iv0 = EdgeTab[i][0];
			int iv1 = EdgeTab[i][1];

			// Check if the points are colinear.
			if (((V[iv0] - _Point ) / (V[iv1] - _Point)).Length() < 0.0001f) continue;

			CPlane3Dfp32 P;
			P.Create(V[iv0], V[iv1], _Point);

			int Mask = P.GetArrayPlaneSideMask_Epsilon(V, 9, 0.001f);
			Mask &= 3;
			if (Mask == 3) continue;
			
			if (Mask ==  1)
			{
				P.Inverse();
			}

			for(int i = 0; i < nPlanes; i++)
				if (EqualPlanes(_pP[i], P, 0.001f)) continue;

			_pP[nPlanes++] = P;
		}
	}

	return nPlanes;
}


bool CXWC_Tracer_Light::CanIlluminate(const CBox3Dfp32& _Box) const
{
	switch(m_Type)
	{
	case XWC_LIGHTTYPE_FAKESKYRADIOSITY : 
		{
			return true;
		}
		break;
	case XWC_LIGHTTYPE_PARALLELL :
		{
			return true;
		}
		break;
	default :
		{
			fp32 d = _Box.GetMinDistance(GetLocalPos().Getfp32());
			if (d < m_MaxRange) return true;
		}
	}
	return false;
}

bool CXWC_Tracer_Light::CanCastShadow(const CBox3Dfp32& _Box, const CBox3Dfp32 _ShadowObj) const
{
	switch(m_Type)
	{
	case XWC_LIGHTTYPE_FAKESKYRADIOSITY : 
		{
			if (_ShadowObj.m_Max[2] < _Box.m_Min[2]) return false;

			fp32 d = GetMinBoxesDistance(_Box, _ShadowObj);
			if (d < m_TraceLen) return true;
		}
		break;
	case XWC_LIGHTTYPE_PARALLELL :
		{
			CVec3Dfp32 DirLamp = CVec3Dfp64::GetMatrixRow(m_LocalPos, 0).Getfp32();
			CPlane3Dfp32 Planes[16];
			int nP = ExtrudeBoxContour(_Box, DirLamp, Planes);

			int32 p;
			for(p = 0; p < nP; p++)
			{
				fp32 mind = Planes[p].GetBoxMinDistance(_Box.m_Min, _Box.m_Max);
				fp32 maxd = Planes[p].GetBoxMaxDistance(_Box.m_Min, _Box.m_Max);
//LogFile(CStrF("P %d, %f, %f, %s, %s", p, 
//		mind, maxd, (char*) Planes[p].GetString(), (char*)_Box.GetString()));
			}

			for(p = 0; p < nP; p++)
			{
//LogFile(CStrF("P %d, %s", p, (char*)     Planes[p].GetString()));
				if (Planes[p].GetBoxMinDistance(_ShadowObj.m_Min, _ShadowObj.m_Max) > 0.0f) break;
			}

			if (p != nP)
			{
//LogFile(CStrF("Box was outside. %s", (char*)_ShadowObj.GetString()));

				return false;
			}

//			if (_ShadowObj.InVolume(Planes, nP))
			{
//return true;
				CPlane3Dfp32 P;
				CVec3Dfp32 Center;
				_Box.GetCenter(Center);
				P.CreateNV(-DirLamp, Center);
				fp32 min = P.GetBoxMinDistance(_Box.m_Min, _Box.m_Max);
				fp32 max = P.GetBoxMaxDistance(_Box.m_Min, _Box.m_Max);
				fp32 mins = P.GetBoxMinDistance(_ShadowObj.m_Min, _ShadowObj.m_Max);
				fp32 maxs = P.GetBoxMaxDistance(_ShadowObj.m_Min, _ShadowObj.m_Max);
				if (maxs < min) return false;
				if (mins-m_TraceLen > max) return false;
				return true;
			}
		}
		break;
	default :
		{
			fp32 d = _Box.GetMinDistance(GetLocalPos().Getfp32());
			if (d > m_MaxRange) return false;

//			CPlane3Dfp32 Planes[32];
//			int nP = CreatePolyhedronFromBox(_Box, GetLocalPos().Getfp32(), Planes);
//			if (_ShadowObj.InVolume(Planes, nP)) return true;
			return true;
		}
	}
	return false;
}

// -------------------------------------------------------------------
//  CXWC_Tracer
// -------------------------------------------------------------------
TPtr<CXWC_Tracer> CXWC_Tracer::Duplicate()
{
	TPtr<CXWC_Tracer> spT = MNew(CXWC_Tracer);
	if (!spT)
		MemError("Duplicate");

	spT->m_lspObjects = m_lspObjects;
	spT->m_lObjPos.Add(m_lObjPos);
	spT->m_lLocalPos.Add(m_lLocalPos);
	spT->m_lLights.Add(m_lLights);

	spT->InitStack();
	spT->m_bSkipTrace = m_bSkipTrace;
;
	return spT;
}

CXWC_Tracer::CXWC_Tracer()
{
//	m_nActiveObjects = 0;
	m_iStack = -1;
	m_bSkipTrace = false;
}

void CXWC_Tracer::AddLight(const CXWC_Tracer_Light& _Light)
{
	m_lLights.Add(_Light);
}

void CXWC_Tracer::AddObject(spCXWC_TracerObject _s_pRegect, const CMat4Dfp64& _Pos)
{
	m_lspObjects.Add(_s_pRegect);
	m_lObjPos.Add(_Pos);
	m_lLocalPos.Add(_Pos);
}

void ConvMat8Mat4(const CMat4Dfp64& _Src, CMat4Dfp32& _Dst)
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			_Dst.k[i][j] = _Src.k[i][j];
}

void CXWC_Tracer::SetLocalSystem(CMat4Dfp64 _Mat)
{
//	CMat4Dfp64 Inv;
//	_Mat.InverseOrthogonal(Inv);

	for(int32 i = 0; i < m_lspObjects.Len(); i++)
	{
		CMat4Dfp64 Inv;
		m_lObjPos[i].InverseOrthogonal(Inv);
		Inv.Multiply(_Mat, m_lLocalPos[i]);		// Matrix from _Mat to Obj[i]
		CMat4Dfp32 LocalPos4, LInv;
		ConvMat8Mat4(m_lLocalPos[i], LocalPos4);
		LocalPos4.InverseOrthogonal(LInv);
		
		TransformAABB(LInv, m_lspObjects[i]->m_Bound, m_lspObjects[i]->m_LocalBound);
//		LogFile(m_lLocalPos[i].GetString());
	}

	CMat4Dfp64 Inv;
	_Mat.InverseOrthogonal(Inv);
	for(int32 i = 0; i < m_lLights.Len(); i++)
	{
		m_lLights[i].CopyToLocal(m_lLights[i].m_Pos);

		CMat4Dfp64 m;
		m_lLights[i].m_LocalPos.Multiply(Inv, m);
		m_lLights[i].m_LocalPos = m;
//	LogFile(m_lLights[i].m_Pos.GetString() + "  =>  " + m_lLights[i].m_LocalPos.GetString());
//		m_lLights[i].m_LocalPos *= _Mat;
	}
}

void CXWC_Tracer::InitStack()
{
	m_lBoundStack.Clear();
	m_lBoundStack.Add(MNew2(CXWC_Tracer_Bound, m_lspObjects.Len(), m_lLights.Len()));

	m_iStack = 0;
	CXWC_Tracer_Bound* pSrc = m_lBoundStack[m_iStack];

	int32 i;
	for(i = 0; i < m_lspObjects.Len(); i++)
		pSrc->m_liActiveObjects[i] = i;
	pSrc->m_nActiveObjects = m_lspObjects.Len();

	for(i = 0; i < m_lLights.Len(); i++)
		pSrc->m_lActiveLights[i] = m_lLights[i];
	pSrc->m_nActiveLights = m_lLights.Len();
}

#ifdef NEVER
bool CXWC_Tracer::PushTraceBound_Sphere(const CVec3Dfp64 _Pos, fp64 _Radius)
{
	CXWC_Tracer_Bound* pSrc = m_lBoundStack[m_iStack];
	m_iStack++;
	if (m_iStack >= m_lBoundStack.Len()) m_lBoundStack.Add(DNew(CXWC_Tracer_Bound) CXWC_Tracer_Bound(m_lspObjects.Len(), m_lLights.Len()));
	CXWC_Tracer_Bound* pDest = m_lBoundStack[m_iStack];

	pDest->m_nActiveObjects = 0;
	pDest->m_nActiveLights = 0;

	fp32 MaxLDist = 0;
	for(int i = 0; i < pSrc->m_nActiveLights; i++)
	{
		fp64 Dist = (_Pos - pSrc->m_lActiveLights[i].GetLocalPos()).Length();

#ifndef XWLIGHT_NOLIGHTBOUND
		if (Dist < (pSrc->m_lActiveLights[i].m_MaxRange + _Radius))
#endif
		{
			pDest->m_lActiveLights[pDest->m_nActiveLights++] = pSrc->m_lActiveLights[i];
			if (Dist > MaxLDist) MaxLDist = Dist;
		}
	}

	for(i = 0; i < pSrc->m_nActiveObjects; i++)
	{
		int iObj = pSrc->m_liActiveObjects[i];

		CVec3Dfp64 p(_Pos);
		p *= m_lLocalPos[iObj];

#ifndef XWLIGHT_NOOBJECTBOUND
		if (p.Length() < (MaxLDist + m_lspObjects[iObj]->m_BoundRadius))
#endif
			pDest->m_liActiveObjects[pDest->m_nActiveObjects++] = iObj;
	}

	return true;
}

bool CXWC_Tracer::PushTraceBound_Plane(const CPlane3Dfp64& _Plane)
{
	CXWC_Tracer_Bound* pSrc = m_lBoundStack[m_iStack];
	m_iStack++;
	if (m_iStack >= m_lBoundStack.Len()) m_lBoundStack.Add(DNew(CXWC_Tracer_Bound) CXWC_Tracer_Bound(m_lspObjects.Len(), m_lLights.Len()));
	CXWC_Tracer_Bound* pDest = m_lBoundStack[m_iStack];

	pDest->m_nActiveObjects = 0;
	pDest->m_nActiveLights = 0;

	fp32 MaxLDist = 0;
	for(int i = 0; i < pSrc->m_nActiveLights; i++)
	{
		fp64 Dist = -_Plane.Distance(pSrc->m_lActiveLights[i].GetLocalPos());
#ifndef XWLIGHT_NOLIGHTBOUND
		if (Dist < pSrc->m_lActiveLights[i].m_MaxRange)
#endif
		{
			pDest->m_lActiveLights[pDest->m_nActiveLights++] = pSrc->m_lActiveLights[i];
			if (Dist > MaxLDist) MaxLDist = Dist;
		}
	}

	for(i = 0; i < pSrc->m_nActiveObjects; i++)
	{
		int iObj = pSrc->m_liActiveObjects[i];

		CPlane3Dfp64 Plane = _Plane;
		Plane.Transform(m_lLocalPos[iObj]);

#ifndef XWLIGHT_NOOBJECTBOUND
		if (-Plane.Distance(0) < m_lspObjects[iObj]->m_BoundRadius + MaxLDist)
#endif
			pDest->m_liActiveObjects[pDest->m_nActiveObjects++] = iObj;
	}

	return true;
}

bool CXWC_Tracer::PushTraceBound_Planes(const CPlane3Dfp64 *_pP, int _nPlanes)
{
	CXWC_Tracer_Bound* pSrc = m_lBoundStack[m_iStack];
	m_iStack++;
	if (m_iStack >= m_lBoundStack.Len()) m_lBoundStack.Add(DNew(CXWC_Tracer_Bound) CXWC_Tracer_Bound(m_lspObjects.Len(), m_lLights.Len()));
	CXWC_Tracer_Bound* pDest = m_lBoundStack[m_iStack];

	pDest->m_nActiveObjects = 0;
	pDest->m_nActiveLights = 0;

	fp32 MaxLDist = 0;
	for(int i = 0; i < pSrc->m_nActiveLights; i++)
	{
		fp64 Dist;
		for(int p = 0; p < _nPlanes; p++)
		{
			Dist = -_pP[p].Distance(pSrc->m_lActiveLights[i].GetLocalPos());
			if(Dist >= pSrc->m_lActiveLights[i].m_MaxRange)
				break;
		}
#ifndef XWLIGHT_NOLIGHTBOUND
		if(p == _nPlanes)
#endif
		{
			pDest->m_lActiveLights[pDest->m_nActiveLights++] = pSrc->m_lActiveLights[i];
			if(Dist > MaxLDist)
				MaxLDist = Dist;
		}
	}

	for(i = 0; i < pSrc->m_nActiveObjects; i++)
	{
		int iObj = pSrc->m_liActiveObjects[i];

#ifndef XWLIGHT_NOOBJECTBOUND
		for(int p = 0; p < _nPlanes; p++)
		{
			CPlane3Dfp64 Plane = _pP[p];
			Plane.Transform(m_lLocalPos[iObj]);
		if (-Plane.Distance(0) >= m_lspObjects[iObj]->m_BoundRadius + MaxLDist)
			break;
		}
		if(p == _nPlanes)
#endif
			pDest->m_liActiveObjects[pDest->m_nActiveObjects++] = iObj;
	}

	return true;
}

bool CXWC_Tracer::PushTraceBound_Polyhedron(const CVec3Dfp64* _pV, int _nVertices, const CPlane3Dfp64* _pP, int _nPlanes)
{
	CXWC_Tracer_Bound* pSrc = m_lBoundStack[m_iStack];
	m_iStack++;
	if (m_iStack >= m_lBoundStack.Len()) m_lBoundStack.Add(DNew(CXWC_Tracer_Bound) CXWC_Tracer_Bound(m_lspObjects.Len(), m_lLights.Len()));
	CXWC_Tracer_Bound* pDest = m_lBoundStack[m_iStack];

	pDest->m_nActiveObjects = 0;
	for(int i = 0; i < pSrc->m_nActiveObjects; i++)
	{
		int iObj = pSrc->m_liActiveObjects[i];
		CVec3Dfp64 Verts[32];
		CPlane3Dfp64 Planes[32];
		for(int v = 0; v < _nVertices; v++)
			CVec3Dfp64::MultiplyMatrix(_pV, &Verts[0], m_lLocalPos[iObj], _nVertices);
		for(int p = 0; p < _nPlanes; p++)
		{
			Planes[p] = _pP[p];
			Planes[p].Transform(m_lLocalPos[iObj]);
		}

		if (m_lspObjects[iObj]->InitTraceBound_Polyhedron(&Verts[0], _nVertices, &Planes[0], _nPlanes))
			pDest->m_liActiveObjects[pDest->m_nActiveObjects++] = iObj;
	}

//LogFile(CStrF("nActive %d", m_nActiveObjects));

	return (pDest->m_nActiveObjects != 0);
}

#endif

bool CXWC_Tracer::PushTraceBound_Box(const CBox3Dfp32& _Box)
{
	CXWC_Tracer_Bound* pSrc = m_lBoundStack[m_iStack];
	m_iStack++;
	if (m_iStack >= m_lBoundStack.Len()) m_lBoundStack.Add(MNew2(CXWC_Tracer_Bound, m_lspObjects.Len(), m_lLights.Len()));
	CXWC_Tracer_Bound* pDest = m_lBoundStack[m_iStack];

//LogFile(CStrF("BoundBox, iStack %d,  %d obj, %d lights", m_iStack, pSrc->m_nActiveObjects, pSrc->m_nActiveLights));

	pDest->m_nActiveLights = 0;
	for(int i = 0; i < pSrc->m_nActiveLights; i++)
	{
#ifndef XWLIGHT_NOLIGHTBOUND
		if (pSrc->m_lActiveLights[i].CanIlluminate(_Box))
#endif
		{
			pDest->m_lActiveLights[pDest->m_nActiveLights++] = pSrc->m_lActiveLights[i];
		}
	}


/*	{
		for(int i = 0; i < 3; i++)
		{
			pDest->m_Bound.m_Min[i] = Max(pSrc->m_Bound.m_Min[i], _Box.m_Min[i]);
			pDest->m_Bound.m_Max[i] = Min(pSrc->m_Bound.m_Max[i], _Box.m_Max[i]);
		}

	}*/

//	pDest->m_Bound = pSrc->m_Bound;
//	pDest->m_Bound.And(_Box);

	pDest->m_Bound = _Box;

	{
		pDest->m_nActiveObjects = 0;
		for(int i = 0; i < pSrc->m_nActiveObjects; i++)
		{
			int iObj = pSrc->m_liActiveObjects[i];
			if (_Box.IsInside(m_lspObjects[iObj]->m_LocalBound))
			{
				pDest->m_liActiveObjects[pDest->m_nActiveObjects++] = iObj;
			}
			else
			{
				for(int l = 0; l < pDest->m_nActiveLights; l++)
					if (pDest->m_lActiveLights[l].CanCastShadow(_Box, m_lspObjects[iObj]->m_LocalBound))
					{
						pDest->m_liActiveObjects[pDest->m_nActiveObjects++] = iObj;
						break;
					}
			}
		}
	}

//LogFile(CStrF("BoundBoxDone,  iStack %d, %d obj, %d lights", m_iStack, pDest->m_nActiveObjects, pDest->m_nActiveLights));
//LogFile(CStrF("nActive %d", m_nActiveObjects));

	return (pDest->m_nActiveObjects != 0);
}

void CXWC_Tracer::PopTraceBound()
{
	m_iStack--;
}

bool CXWC_Tracer::BoxAffectsRendering(const CBox3Dfp32& _Box)
{
	CXWC_Tracer_Bound* pSrc = m_lBoundStack[m_iStack];
	for(int l = 0; l < pSrc->m_nActiveLights; l++)
		if (pSrc->m_lActiveLights[l].CanCastShadow(pSrc->m_Bound, _Box))
			return true;

	return true;
}

bool CXWC_Tracer::PotentialIntersection(const CBox3Dfp32& _Box)
{
	// This function does nothing and should be removed..

	return false;
}

void CXWC_Tracer::InitTraceBound()
{
	CXWC_Tracer_Bound* pSrc = m_lBoundStack[m_iStack];
	for(int i = 0; i < pSrc->m_nActiveObjects; i++)
		m_lspObjects[pSrc->m_liActiveObjects[i]]->InitTraceBound(this, pSrc->m_Bound);
}


void CXWC_Tracer::ReleaseTraceBound()
{
	CXWC_Tracer_Bound* pSrc = m_lBoundStack[m_iStack];
	for(int i = 0; i < pSrc->m_nActiveObjects; i++)
		m_lspObjects[pSrc->m_liActiveObjects[i]]->ReleaseTraceBound();
}

int CXWC_Tracer::TraceLine_Bounded(const CVec3Dfp64& _p0, const CVec3Dfp64& _p1, CVec3Dfp32& _Color)
{
	if(m_bSkipTrace)
		return 1;

	m_TraceLineMedium = 0;
	CXWC_Tracer_Bound* pSrc = m_lBoundStack[m_iStack];

//	if(!pSrc->m_nActiveObjects)
//		LogFile("No active objects.");

	for(int i = 0; i < pSrc->m_nActiveObjects; i++)
	{
		int iObj = pSrc->m_liActiveObjects[i];
		CVec3Dfp64 p0(_p0);
		CVec3Dfp64 p1(_p1);
		p0 *= m_lLocalPos[iObj];
		p1 *= m_lLocalPos[iObj];
		m_lspObjects[iObj]->m_TraceLineMedium = 0;
		if (!m_lspObjects[iObj]->TraceLine_Bounded(p0, p1, _Color))
		{
			return 0;
		}

		m_TraceLineMedium |= m_lspObjects[iObj]->m_TraceLineMedium;

//		int Medium = m_lspObjects[iObj]->TraceLine_Bounded(p0, p1, _Color);
//		if (Medium) return Medium;
	}
	return 1;
}

int CXWC_Tracer::TraceLine_Bounded(const CVec3Dfp64& _p0, const CVec3Dfp64& _p1, int _MediumFlags, CCollisionInfo* _pCInfo)
{
	if(m_bSkipTrace)
		return 1;

	m_TraceLineMedium = 0;
	CXWC_Tracer_Bound* pSrc = m_lBoundStack[m_iStack];

//	if(!pSrc->m_nActiveObjects)
//		LogFile("No active objects.");

	for(int i = 0; i < pSrc->m_nActiveObjects; i++)
	{
		int iObj = pSrc->m_liActiveObjects[i];
		CVec3Dfp64 p0(_p0);
		CVec3Dfp64 p1(_p1);
		p0 *= m_lLocalPos[iObj];
		p1 *= m_lLocalPos[iObj];
		m_lspObjects[iObj]->m_TraceLineMedium = 0;
		if (!m_lspObjects[iObj]->TraceLine_Bounded(p0, p1, _MediumFlags, _pCInfo)) return 0;

		m_TraceLineMedium |= m_lspObjects[iObj]->m_TraceLineMedium;

//		int Medium = m_lspObjects[iObj]->TraceLine_Bounded(p0, p1, _Color);
//		if (Medium) return Medium;
	}
	return 1;
}


int CXWC_Tracer::GetMedium(const CBox3Dfp64& _Box, CXR_MediumDesc& _Medium) const
{
	int Medium = 0;
	const CXWC_Tracer_Bound* pSrc = m_lBoundStack[m_iStack];
	for (int i=0; i<pSrc->m_nActiveObjects; i++)
	{
		int iObj = pSrc->m_liActiveObjects[i];
		const CXWC_TracerObject* pObj = m_lspObjects[iObj];
		const CVec3Dfp64& ObjPos = CVec3Dfp64::GetMatrixRow(m_lObjPos[iObj], 3);
		const CBox3Dfp64 LocalBox(_Box.m_Min-ObjPos, _Box.m_Max-ObjPos);
		Medium |= pObj->GetMedium(LocalBox, _Medium);
	}
	return Medium;
}

