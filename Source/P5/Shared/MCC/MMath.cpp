// MOS_Math.cpp

#include "PCH.h"
#include "../Platform/Platform.h"
#include "MFloat.h"
#include "MMath.h"

/*
CMat4Dfp32 CMat4Dfp32::_RotXWorkMat(1.0f,0.0f,0.0f,0.0f, 0.0f,1.0f,0.0f,0.0f, 0.0f,0.0f,1.0f,0.0f, 0.0f,0.0f,0.0f,1.0f);
CMat4Dfp64 CMat4Dfp64::_RotXWorkMat(1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0);
CMat4Dfp32 CMat4Dfp32::_RotYWorkMat(1.0f,0.0f,0.0f,0.0f, 0.0f,1.0f,0.0f,0.0f, 0.0f,0.0f,1.0f,0.0f, 0.0f,0.0f,0.0f,1.0f);
CMat4Dfp64 CMat4Dfp64::_RotYWorkMat(1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0);
CMat4Dfp32 CMat4Dfp32::_RotZWorkMat(1.0f,0.0f,0.0f,0.0f, 0.0f,1.0f,0.0f,0.0f, 0.0f,0.0f,1.0f,0.0f, 0.0f,0.0f,0.0f,1.0f);
CMat4Dfp64 CMat4Dfp64::_RotZWorkMat(1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0);
*/

void MCCDLLEXPORT MMath_Init()
{
	// XBox 360 compiler workaround
	CBox3Dfp32 Box;
	CVec3Dfp32 RetPos;
	Box.IntersectLine(CVec3Dfp32(0, 0, 0), CVec3Dfp32(1, 1, 1), RetPos);
}

// -------------------------------------------------------------------
//  Equation solvers
// -------------------------------------------------------------------

bool SolveP2(const fp32 a, const fp32 b, const fp32 c, fp32& _s0, fp32& _s1)
{
	const fp32 arecp = 1.0f / a;
	const fp32 p = b * arecp;
	const fp32 q = c * arecp;

	const fp32 term = -p * 0.5f;
	const fp32 x = Sqr(term) - q;
	if (FloatIsNeg(x)) return false;

	const fp32 sqrtx = M_Sqrt(x);
	_s0 = term + sqrtx;
	_s1 = term - sqrtx;

	return true;
}

#define SOLVEP3_EPSILON 0.000001f

template <class Real>
int SolveP3(Real fC0, Real fC1, Real fC2, Real fC3, Real& root1, Real& root2, Real& root3)
{
	int count = 0;
	/*
	if ( fabs(fC3) <= m_fEpsilon )
	{
	// polynomial is quadratic
	return FindA(fC0,fC1,fC2);
	}*/
	// make polynomial monic, x^3+c2*x^2+c1*x+c0
	Real fInvC3 = ((Real)1.0f)/fC3;
	fC0 *= fInvC3;
	fC1 *= fInvC3;
	fC2 *= fInvC3;

	// convert to y^3+a*y+b = 0 by x = y-c2/3
	const Real fThird = (Real)1.0f/(Real)3.0f;
	const Real fTwentySeventh = (Real)1.0f/(Real)27.0f;
	Real fOffset = fThird*fC2;
	Real fA = fC1 - fC2*fOffset;
	Real fB = fC0+fC2*(((Real)2.0f)*fC2*fC2-((Real)9.0f)*fC1)*fTwentySeventh;
	Real fHalfB = ((Real)0.5f)*fB;

	Real fDiscr = fHalfB*fHalfB + fA*fA*fA*fTwentySeventh;
	if ( M_Fabs(fDiscr) <= SOLVEP3_EPSILON )
		fDiscr = (Real)0.0f;

	if ( fDiscr > (Real)0.0f )  // 1 real, 2 complex roots
	{
		fDiscr = M_Sqrt(fDiscr);
		Real fTemp = -fHalfB + fDiscr;
		if ( fTemp >= (Real)0.0f )
			root1 = M_Pow(fTemp,fThird);
		else
			root1 = -M_Pow(-fTemp,fThird);
		fTemp = -fHalfB - fDiscr;
		if ( fTemp >= (Real)0.0f )
			root1 += M_Pow(fTemp,fThird);
		else
			root1 -= M_Pow(-fTemp,fThird);
		root1 -= fOffset;
		count = 1;
	}
	else if ( fDiscr < (Real)0.0f ) 
	{
		const Real fSqrt3 = M_Sqrt((Real)3.0f);
		Real fDist = M_Sqrt(-fThird*fA);
		Real fAngle = fThird*M_ATan2(M_Sqrt(-fDiscr), -fHalfB);
		Real fCos = M_Cos(fAngle);
		Real fSin = M_Sin(fAngle);
		root1 = ((Real)2.0f)*fDist*fCos-fOffset;
		root2 = -fDist*(fCos+fSqrt3*fSin)-fOffset;
		root3 = -fDist*(fCos-fSqrt3*fSin)-fOffset;
		count = 3;
	}
	else 
	{
		Real fTemp;
		if ( fHalfB >= (Real)0.0f )
			fTemp = -M_Pow(fHalfB,fThird);
		else
			fTemp = M_Pow(-fHalfB,fThird);
		root1 = ((Real)2.0f)*fTemp-fOffset;
		root2 = -fTemp-fOffset;
		root3 = root2;
		count = 3;
	}

	return count;
}

// -------------------------------------------------------------------
#define CUTFENCE_EPSILON 0.0001f
#define CUTFENCE_EPSILONFP64 0.0000001f

int MCCDLLEXPORT CutFence(CVec3Dfp32* _pVerts, int _nv, const CPlane3Dfp32* _pPlanes, int _np, int& _bClip)
{
	_bClip = false;
	if (!_nv) return 0;
	const int MaxVClip = 32;

	CVec3Dfp32 VClip[MaxVClip];

	CVec3Dfp32* pVDest = &VClip[0];
	CVec3Dfp32* pVSrc = _pVerts;

	for(int iPlane = 0; iPlane < _np; iPlane++)
	{
		const CPlane3Dfp32* pP = &_pPlanes[iPlane];
		fp32 VertPDist[32];
		int Side = 0;

		// Calc point-2-plane distance for all vertices.
		for(int v = 0; v < _nv; v++)
		{
			VertPDist[v] = -pP->Distance(pVSrc[v]);
			if (VertPDist[v] < -CUTFENCE_EPSILON) Side |= 2;
			else if (VertPDist[v] > CUTFENCE_EPSILON) Side |= 1;
		}

		// If all points are on one side, return either all or none.
		if ((Side & 3) == 2)
		{
			_bClip = true;
			return 0;
		}
		if ((Side & 3) != 3) continue;

		_bClip = true;
		int nClip = 0;
		{
			int v = _nv-1;
			for (int v2 = 0; v2 < _nv; v2++)
			{
				if (VertPDist[v] > -CUTFENCE_EPSILON)
				{
					pVDest[nClip] = pVSrc[v];
					nClip++;

					if ((VertPDist[v2] < -CUTFENCE_EPSILON) && (VertPDist[v] > CUTFENCE_EPSILON))
					{
						CVec3Dfp32 dV;
						pVSrc[v2].Sub(pVSrc[v], dV);
						fp32 s = -dV*pP->n;
						if (s)
						{
							fp32 sp = VertPDist[v];
							fp32 t = -sp/s;
							pVSrc[v].Combine(dV, t, pVDest[nClip++]);
						}
					}
				}
				else
				{
					if (VertPDist[v2] > CUTFENCE_EPSILON)
					{
						CVec3Dfp32 dV;
						pVSrc[v2].Sub(pVSrc[v], dV);
						fp32 s = -dV*pP->n;
						if (s)
						{
							fp32 sp = VertPDist[v];
							fp32 t = -sp/s;
							pVSrc[v].Combine(dV, t, pVDest[nClip++]);
						}
					}
				}

				if (nClip > MaxVClip-1) Error_static("CutFace", "Too many vertices.");
				v = v2;
			}
		}

		if (nClip < 3) return 0;
		_nv = nClip;

		Swap(pVSrc, pVDest);
	}

	// Move if the latest vertices are in the wrong array.
	if (pVSrc != _pVerts) 
		memcpy(_pVerts, pVSrc, _nv*sizeof(CVec3Dfp32));
	return _nv;
}

#ifndef	CPU_SOFTWARE_FP64
int MCCDLLEXPORT CutFence(CVec3Dfp64* _pVerts, int _nv, const CPlane3Dfp64* _pPlanes, int _np, int& _bClip)
{
	_bClip = false;
	if (!_nv) return 0;
	const int MaxVClip = 32;

	CVec3Dfp64 VClip[MaxVClip];

	CVec3Dfp64* pVDest = &VClip[0];
	CVec3Dfp64* pVSrc = _pVerts;

	for(int iPlane = 0; iPlane < _np; iPlane++)
	{
		const CPlane3Dfp64* pP = &_pPlanes[iPlane];
		fp64 VertPDist[32];
		int Side = 0;

		// Calc point-2-plane distance for all vertices.
		for(int v = 0; v < _nv; v++)
		{
			VertPDist[v] = -pP->Distance(pVSrc[v]);
			if (VertPDist[v] < -CUTFENCE_EPSILONFP64) Side |= 2;
			else if (VertPDist[v] > CUTFENCE_EPSILONFP64) Side |= 1;
		}

		// If all points are on one side, return either all or none.
		if ((Side & 3) == 2)
		{
			_bClip = true;
			return 0;
		}
		if ((Side & 3) != 3) continue;

		_bClip = true;
		int nClip = 0;
		{
			int v = _nv-1;
			for (int v2 = 0; v2 < _nv; v2++)
			{
				if (VertPDist[v] > -CUTFENCE_EPSILONFP64)
				{
					pVDest[nClip] = pVSrc[v];
					nClip++;

					if ((VertPDist[v2] < -CUTFENCE_EPSILONFP64) && (VertPDist[v] > CUTFENCE_EPSILONFP64))
					{
						CVec3Dfp64 dV;
						pVSrc[v2].Sub(pVSrc[v], dV);
						fp64 s = -dV*pP->n;
						if (s)
						{
							fp64 t = -VertPDist[v]/s;
							pVSrc[v].Combine(dV, t, pVDest[nClip++]);
						}
					}
				}
				else
				{
					if (VertPDist[v2] > CUTFENCE_EPSILONFP64)
					{
						CVec3Dfp64 dV;
						pVSrc[v2].Sub(pVSrc[v], dV);
						fp64 s = -dV*pP->n;
						if (s)
						{
							fp64 t = -VertPDist[v]/s;
							pVSrc[v].Combine(dV, t, pVDest[nClip++]);
						}
					}
				}

				if (nClip > MaxVClip-1) Error_static("CutFace", "Too many vertices.");
				v = v2;
			}
		}

		if (nClip < 3) return 0;
		_nv = nClip;

		Swap(pVSrc, pVDest);
	}

	// Move if the latest vertices are in the wrong array.
	if (pVSrc != _pVerts) 
		memcpy(_pVerts, pVSrc, _nv*sizeof(CVec3Dfp64));
	return _nv;
}
#endif	// CPU_SOFTWARE_FP64

int MCCDLLEXPORT CutFence(CVec3Dfp32* _pVerts, int _nv, const CPlane3Dfp32* _pPlanes, int _np, 
	bool _bInvertPlanes, CVec2Dfp32* _pTVerts1, CVec2Dfp32* _pTVerts2, CVec2Dfp32* _pTVerts3)
{
	if (!_nv) return 0;
	const int MaxVClip = 32;

	int bClipTVerts1 = (_pTVerts1 != NULL);
	int bClipTVerts2 = (_pTVerts2 != NULL);
	int bClipTVerts3 = (_pTVerts3 != NULL);

	CVec3Dfp32 VClip[MaxVClip];
	CVec2Dfp32 TVClip1[MaxVClip];
	CVec2Dfp32 TVClip2[MaxVClip];
	CVec2Dfp32 TVClip3[MaxVClip];

	CVec3Dfp32* pVDest = &VClip[0];
	CVec2Dfp32* pTVDest1 = &TVClip1[0];
	CVec2Dfp32* pTVDest2 = &TVClip2[0];
	CVec2Dfp32* pTVDest3 = &TVClip3[0];
	CVec3Dfp32* pVSrc = _pVerts;
	CVec2Dfp32* pTVSrc1 = _pTVerts1;
	CVec2Dfp32* pTVSrc2 = _pTVerts2;
	CVec2Dfp32* pTVSrc3 = _pTVerts3;

	for(int iPlane = 0; iPlane < _np; iPlane++)
	{
		const CPlane3Dfp32* pP = &_pPlanes[iPlane];
		fp32 VertPDist[32];
//		fp32 d = -((pP->p.k[0] * pP->n.k[0]) + (pP->p.k[1] * pP->n.k[1]) + (pP->p.k[2] * pP->n.k[2]));
//		fp32 d = pP->d;
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
				if (VertPDist[v] > -CUTFENCE_EPSILON)
				{
					pVDest[nClip] = pVSrc[v];
					if (bClipTVerts1) pTVDest1[nClip] = pTVSrc1[v];
					if (bClipTVerts2) pTVDest2[nClip] = pTVSrc2[v];
					if (bClipTVerts3) pTVDest3[nClip] = pTVSrc3[v];
					nClip++;

					if ((VertPDist[v2] < -CUTFENCE_EPSILON) && (VertPDist[v] > CUTFENCE_EPSILON))
					{
	//					BSPModel_GetIntersectionPoint(_pi.n, d, pVSrc[v], pVSrc[v2], pVDest[nClip++]);

						fp32 dvx = (pVSrc[v2].k[0] - pVSrc[v].k[0]);
						fp32 dvy = (pVSrc[v2].k[1] - pVSrc[v].k[1]);
						fp32 dvz = (pVSrc[v2].k[2] - pVSrc[v].k[2]);
						fp32 s = dvx*pP->n.k[0] + dvy*pP->n.k[1] + dvz*pP->n.k[2];
						if (_bInvertPlanes) s = -s;
						if (s)
						{
							fp32 sp = VertPDist[v];
							fp32 t = -sp/s;
							pVDest[nClip].k[0] = pVSrc[v].k[0] + dvx * t;
							pVDest[nClip].k[1] = pVSrc[v].k[1] + dvy * t;
							pVDest[nClip].k[2] = pVSrc[v].k[2] + dvz * t;

							if (bClipTVerts1)
							{
								fp32 dtvx = (pTVSrc1[v2].k[0] - pTVSrc1[v].k[0]);
								fp32 dtvy = (pTVSrc1[v2].k[1] - pTVSrc1[v].k[1]);
								pTVDest1[nClip].k[0] = pTVSrc1[v].k[0] + dtvx * t;
								pTVDest1[nClip].k[1] = pTVSrc1[v].k[1] + dtvy * t;
							}
							if (bClipTVerts2)
							{
								fp32 dtvx = (pTVSrc2[v2].k[0] - pTVSrc2[v].k[0]);
								fp32 dtvy = (pTVSrc2[v2].k[1] - pTVSrc2[v].k[1]);
								pTVDest2[nClip].k[0] = pTVSrc2[v].k[0] + dtvx * t;
								pTVDest2[nClip].k[1] = pTVSrc2[v].k[1] + dtvy * t;
							}
							if (bClipTVerts3)
							{
								fp32 dtvx = (pTVSrc3[v2].k[0] - pTVSrc3[v].k[0]);
								fp32 dtvy = (pTVSrc3[v2].k[1] - pTVSrc3[v].k[1]);
								pTVDest3[nClip].k[0] = pTVSrc3[v].k[0] + dtvx * t;
								pTVDest3[nClip].k[1] = pTVSrc3[v].k[1] + dtvy * t;
							}
							nClip++;
						}
					}
				}
				else
				{
					if (VertPDist[v2] > CUTFENCE_EPSILON)
					{
	//					BSPModel_GetIntersectionPoint(_pi.n, d, pVSrc[v], pVSrc[v2], pVDest[nClip++]);

						fp32 dvx = (pVSrc[v2].k[0] - pVSrc[v].k[0]);
						fp32 dvy = (pVSrc[v2].k[1] - pVSrc[v].k[1]);
						fp32 dvz = (pVSrc[v2].k[2] - pVSrc[v].k[2]);
						fp32 s = dvx*pP->n.k[0] + dvy*pP->n.k[1] + dvz*pP->n.k[2];
						if (_bInvertPlanes) s = -s;
						if (s)
						{
							fp32 sp = VertPDist[v];
							fp32 t = -sp/s;
							pVDest[nClip].k[0] = pVSrc[v].k[0] + dvx * t;
							pVDest[nClip].k[1] = pVSrc[v].k[1] + dvy * t;
							pVDest[nClip].k[2] = pVSrc[v].k[2] + dvz * t;

							if (bClipTVerts1)
							{
								fp32 dtvx = (pTVSrc1[v2].k[0] - pTVSrc1[v].k[0]);
								fp32 dtvy = (pTVSrc1[v2].k[1] - pTVSrc1[v].k[1]);
								pTVDest1[nClip].k[0] = pTVSrc1[v].k[0] + dtvx * t;
								pTVDest1[nClip].k[1] = pTVSrc1[v].k[1] + dtvy * t;
							}
							if (bClipTVerts2)
							{
								fp32 dtvx = (pTVSrc2[v2].k[0] - pTVSrc2[v].k[0]);
								fp32 dtvy = (pTVSrc2[v2].k[1] - pTVSrc2[v].k[1]);
								pTVDest2[nClip].k[0] = pTVSrc2[v].k[0] + dtvx * t;
								pTVDest2[nClip].k[1] = pTVSrc2[v].k[1] + dtvy * t;
							}
							if (bClipTVerts3)
							{
								fp32 dtvx = (pTVSrc3[v2].k[0] - pTVSrc3[v].k[0]);
								fp32 dtvy = (pTVSrc3[v2].k[1] - pTVSrc3[v].k[1]);
								pTVDest3[nClip].k[0] = pTVSrc3[v].k[0] + dtvx * t;
								pTVDest3[nClip].k[1] = pTVSrc3[v].k[1] + dtvy * t;
							}
							nClip++;
						}
					}
				}

				if (nClip > MaxVClip-1) Error_static("CutFace", "Too many vertices.");
				v = v2;
			}
		}

		if (nClip < 3) return 0;
		_nv = nClip;

		Swap(pVSrc, pVDest);
		Swap(pTVSrc1, pTVDest1);
		Swap(pTVSrc2, pTVDest2);
		Swap(pTVSrc3, pTVDest3);
	}

	// Move if the latest vertices are in the wrong array.
	if (pVSrc != _pVerts) 
	{
		memcpy(_pVerts, pVSrc, _nv*sizeof(CVec3Dfp32));
		if (bClipTVerts1) memcpy(_pTVerts1, pTVSrc1, _nv*sizeof(CVec2Dfp32));
		if (bClipTVerts2) memcpy(_pTVerts2, pTVSrc2, _nv*sizeof(CVec2Dfp32));
		if (bClipTVerts3) memcpy(_pTVerts3, pTVSrc3, _nv*sizeof(CVec2Dfp32));
	}
	return _nv;
}

// -------------------------------------------------------------------

// Added by Mondelore.
void MCCDLLEXPORT MatrixLerp(const CMat4Dfp32 &_M0, const CMat4Dfp32 &_M1, fp32 _T, CMat4Dfp32 &_Res)
{
	CVec3Dfp32 VRes;
	CVec3Dfp32::GetMatrixRow(_M0, 3).Lerp(CVec3Dfp32::GetMatrixRow(_M1, 3), _T, VRes);

	CQuatfp32 q1, q2;
	q1.Create(_M0);
	q2.Create(_M1);

	CQuatfp32 QRes;
	q1.Lerp(q2, _T, QRes);

	QRes.CreateMatrix(_Res);
	VRes.SetMatrixRow(_Res, 3);
}

// -------------------------------------------------------------------

// -------------------------------------------------------------------

// Added by Mondelore.
void MCCDLLEXPORT MatrixSpline(const CMat4Dfp32 &_M0, const CMat4Dfp32 &_M1, const CMat4Dfp32 &_M2, const CMat4Dfp32 &_M3, 
							   fp32 _TimeFraction, CMat4Dfp32 &_Res)
{
	CVec3Dfp32 pres;
	CQuatfp32 qres;

	CVec3Dfp32 p0, p1, p2, p3;
	p0 = CVec3Dfp32::GetMatrixRow(_M0, 3);
	p1 = CVec3Dfp32::GetMatrixRow(_M1, 3);
	p2 = CVec3Dfp32::GetMatrixRow(_M2, 3);
	p3 = CVec3Dfp32::GetMatrixRow(_M3, 3);

	CQuatfp32 q0, q1, q2, q3;
	q0.Create(_M0);
	q1.Create(_M1);
	q2.Create(_M2);
	q3.Create(_M3);

	fp32 dt01 = 1;
	fp32 dt12 = 1;
	fp32 dt23 = 1;
	
	CVec3Dfp32::Spline(&p0, &p1, &p2, &p1, &p2, &p3, &pres, _TimeFraction, dt01, dt12, dt12, dt23, 1);
	CQuatfp32::Spline(&q0, &q1, &q2, &q1, &q2, &q3, &qres, _TimeFraction, dt01, dt12, dt12, dt23, 1);

	qres.CreateMatrix(_Res);
	pres.SetMatrixRow(_Res, 3);
}

template <>
bool MCCDLLEXPORT TBox<fp32>::IntersectLine(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CVec3Dfp32& _RetHitPos) const
{
	vec128 BoxMin, BoxMax;
	M_VLdU_V3x2_Slow(&m_Min, BoxMin, BoxMax);
	vec128 p0 = M_VLd_V3_Slow(&_p0);
	vec128 p1 = M_VLd_V3_Slow(&_p1);

	{
		vec128 pmin = M_VMin(p0, p1);
		vec128 pmax = M_VMax(p0, p1);

		vec128 outlow = M_VCmpLTMsk(pmax, BoxMin);
		vec128 outhi = M_VCmpGTMsk(pmin, BoxMax);
		

		if(!M_VCmpAllEq(M_VOr(outlow, outhi), M_VZero()))
			return false;
	}

	// Possible intersect

	// Ignore early outs, clip line to box and check if there is a collision in the end

	{
		// Clip X
		vec128 cmpmask = M_VSplatX(M_VCmpLTMsk(p0, p1));

		vec128 pmin = M_VSelMsk(cmpmask, p0, p1);
		vec128 pmax = M_VSelMsk(cmpmask, p1, p0);

		{
			vec128 lowmin = M_VSplatX(M_VCmpLTMsk(pmin, BoxMin));
			vec128 d = M_VSub(pmax, pmin);
			vec128 t = M_VSplatX(M_VMul(M_VSub(BoxMin, pmin), M_VRcp(d)));
			pmin = M_VSelMsk(lowmin, M_VMAdd(t, d, pmin), pmin);
		}
		{
			vec128 himax = M_VSplatX(M_VCmpGTMsk(pmax, BoxMax));
			vec128 d = M_VSub(pmax, pmin);
			vec128 t = M_VSplatX(M_VNeg(M_VMul(M_VSub(pmax, BoxMax), M_VRcp(d))));
			pmax = M_VSelMsk(himax, M_VMAdd(t, d, pmax), pmax);
		}

		p0 = M_VSelMsk(cmpmask, pmin, pmax);
		p1 = M_VSelMsk(cmpmask, pmax, pmin);
	}

	{
		// Clip Y
		vec128 cmpmask = M_VSplatY(M_VCmpLTMsk(p0, p1));

		vec128 pmin = M_VSelMsk(cmpmask, p0, p1);
		vec128 pmax = M_VSelMsk(cmpmask, p1, p0);

		{
			vec128 lowmin = M_VSplatY(M_VCmpLTMsk(pmin, BoxMin));
			vec128 d = M_VSub(pmax, pmin);
			vec128 t = M_VSplatY(M_VMul(M_VSub(BoxMin, pmin), M_VRcp(d)));
			pmin = M_VSelMsk(lowmin, M_VMAdd(t, d, pmin), pmin);
		}
		{
			vec128 himax = M_VSplatY(M_VCmpGTMsk(pmax, BoxMax));
			vec128 d = M_VSub(pmax, pmin);
			vec128 t = M_VSplatY(M_VNeg(M_VMul(M_VSub(pmax, BoxMax), M_VRcp(d))));
			pmax = M_VSelMsk(himax, M_VMAdd(t, d, pmax), pmax);
		}

		p0 = M_VSelMsk(cmpmask, pmin, pmax);
		p1 = M_VSelMsk(cmpmask, pmax, pmin);
	}

	{
		// Clip Z
		vec128 cmpmask = M_VSplatZ(M_VCmpLTMsk(p0, p1));

		vec128 pmin = M_VSelMsk(cmpmask, p0, p1);
		vec128 pmax = M_VSelMsk(cmpmask, p1, p0);

		{
			vec128 lowmin = M_VSplatZ(M_VCmpLTMsk(pmin, BoxMin));
			vec128 d = M_VSub(pmax, pmin);
			vec128 t = M_VSplatZ(M_VMul(M_VSub(BoxMin, pmin), M_VRcp(d)));
			pmin = M_VSelMsk(lowmin, M_VMAdd(t, d, pmin), pmin);
		}
		{
			vec128 himax = M_VSplatZ(M_VCmpGTMsk(pmax, BoxMax));
			vec128 d = M_VSub(pmax, pmin);
			vec128 t = M_VSplatZ(M_VNeg(M_VMul(M_VSub(pmax, BoxMax), M_VRcp(d))));
			pmax = M_VSelMsk(himax, M_VMAdd(t, d, pmax), pmax);
		}

		p0 = M_VSelMsk(cmpmask, pmin, pmax);
		p1 = M_VSelMsk(cmpmask, pmax, pmin);
	}

	{
		vec128 pmin = M_VMin(p0, p1);
		vec128 pmax = M_VMax(p0, p1);

		vec128 outlow = M_VCmpLTMsk(pmax, BoxMin);
		vec128 outhi = M_VCmpGTMsk(pmin, BoxMax);
		

		if(!M_VCmpAllEq(M_VOr(outlow, outhi), M_VZero()))
			return false;
	}

	M_VSt_V3_Slow(p0, &_RetHitPos);
	return true;
}
