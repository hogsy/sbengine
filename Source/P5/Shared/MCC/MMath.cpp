// MOS_Math.cpp

#include "PCH.h"
#include "../Platform/Platform.h"
#include "MFloat.h"
#include "MMath.h"

/*
CMat4Dfp4 CMat4Dfp4::_RotXWorkMat(1.0f,0.0f,0.0f,0.0f, 0.0f,1.0f,0.0f,0.0f, 0.0f,0.0f,1.0f,0.0f, 0.0f,0.0f,0.0f,1.0f);
CMat4Dfp8 CMat4Dfp8::_RotXWorkMat(1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0);
CMat4Dfp4 CMat4Dfp4::_RotYWorkMat(1.0f,0.0f,0.0f,0.0f, 0.0f,1.0f,0.0f,0.0f, 0.0f,0.0f,1.0f,0.0f, 0.0f,0.0f,0.0f,1.0f);
CMat4Dfp8 CMat4Dfp8::_RotYWorkMat(1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0);
CMat4Dfp4 CMat4Dfp4::_RotZWorkMat(1.0f,0.0f,0.0f,0.0f, 0.0f,1.0f,0.0f,0.0f, 0.0f,0.0f,1.0f,0.0f, 0.0f,0.0f,0.0f,1.0f);
CMat4Dfp8 CMat4Dfp8::_RotZWorkMat(1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0);
*/

// -------------------------------------------------------------------
//  Equation solvers
// -------------------------------------------------------------------

bool SolveP2(const fp4 a, const fp4 b, const fp4 c, fp4& _s0, fp4& _s1)
{
	const fp4 arecp = 1.0f / a;
	const fp4 p = b * arecp;
	const fp4 q = c * arecp;

	const fp4 term = -p * 0.5f;
	const fp4 x = Sqr(term) - q;
	if (FloatIsNeg(x)) return false;

	const fp4 sqrtx = M_Sqrt(x);
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
#define CUTFENCE_EPSILONFP8 0.0000001f

int MCCDLLEXPORT CutFence(CVec3Dfp4* _pVerts, int _nv, const CPlane3Dfp4* _pPlanes, int _np, int& _bClip)
{
	_bClip = false;
	if (!_nv) return 0;
	const int MaxVClip = 32;

	CVec3Dfp4 VClip[MaxVClip];

	CVec3Dfp4* pVDest = &VClip[0];
	CVec3Dfp4* pVSrc = _pVerts;

	for(int iPlane = 0; iPlane < _np; iPlane++)
	{
		const CPlane3Dfp4* pP = &_pPlanes[iPlane];
		fp4 VertPDist[32];
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
						CVec3Dfp4 dV;
						pVSrc[v2].Sub(pVSrc[v], dV);
						fp4 s = -dV*pP->n;
						if (s)
						{
							fp4 sp = VertPDist[v];
							fp4 t = -sp/s;
							pVSrc[v].Combine(dV, t, pVDest[nClip++]);
						}
					}
				}
				else
				{
					if (VertPDist[v2] > CUTFENCE_EPSILON)
					{
						CVec3Dfp4 dV;
						pVSrc[v2].Sub(pVSrc[v], dV);
						fp4 s = -dV*pP->n;
						if (s)
						{
							fp4 sp = VertPDist[v];
							fp4 t = -sp/s;
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
		memcpy(_pVerts, pVSrc, _nv*sizeof(CVec3Dfp4));
	return _nv;
}

#ifndef	CPU_SOFTWARE_FP8
int MCCDLLEXPORT CutFence(CVec3Dfp8* _pVerts, int _nv, const CPlane3Dfp8* _pPlanes, int _np, int& _bClip)
{
	_bClip = false;
	if (!_nv) return 0;
	const int MaxVClip = 32;

	CVec3Dfp8 VClip[MaxVClip];

	CVec3Dfp8* pVDest = &VClip[0];
	CVec3Dfp8* pVSrc = _pVerts;

	for(int iPlane = 0; iPlane < _np; iPlane++)
	{
		const CPlane3Dfp8* pP = &_pPlanes[iPlane];
		fp8 VertPDist[32];
		int Side = 0;

		// Calc point-2-plane distance for all vertices.
		for(int v = 0; v < _nv; v++)
		{
			VertPDist[v] = -pP->Distance(pVSrc[v]);
			if (VertPDist[v] < -CUTFENCE_EPSILONFP8) Side |= 2;
			else if (VertPDist[v] > CUTFENCE_EPSILONFP8) Side |= 1;
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
				if (VertPDist[v] > -CUTFENCE_EPSILONFP8)
				{
					pVDest[nClip] = pVSrc[v];
					nClip++;

					if ((VertPDist[v2] < -CUTFENCE_EPSILONFP8) && (VertPDist[v] > CUTFENCE_EPSILONFP8))
					{
						CVec3Dfp8 dV;
						pVSrc[v2].Sub(pVSrc[v], dV);
						fp8 s = -dV*pP->n;
						if (s)
						{
							fp8 t = -VertPDist[v]/s;
							pVSrc[v].Combine(dV, t, pVDest[nClip++]);
						}
					}
				}
				else
				{
					if (VertPDist[v2] > CUTFENCE_EPSILONFP8)
					{
						CVec3Dfp8 dV;
						pVSrc[v2].Sub(pVSrc[v], dV);
						fp8 s = -dV*pP->n;
						if (s)
						{
							fp8 t = -VertPDist[v]/s;
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
		memcpy(_pVerts, pVSrc, _nv*sizeof(CVec3Dfp8));
	return _nv;
}
#endif	// CPU_SOFTWARE_FP8

int MCCDLLEXPORT CutFence(CVec3Dfp4* _pVerts, int _nv, const CPlane3Dfp4* _pPlanes, int _np, 
	bool _bInvertPlanes, CVec2Dfp4* _pTVerts1, CVec2Dfp4* _pTVerts2, CVec2Dfp4* _pTVerts3)
{
	if (!_nv) return 0;
	const int MaxVClip = 32;

	int bClipTVerts1 = (_pTVerts1 != NULL);
	int bClipTVerts2 = (_pTVerts2 != NULL);
	int bClipTVerts3 = (_pTVerts3 != NULL);

	CVec3Dfp4 VClip[MaxVClip];
	CVec2Dfp4 TVClip1[MaxVClip];
	CVec2Dfp4 TVClip2[MaxVClip];
	CVec2Dfp4 TVClip3[MaxVClip];

	CVec3Dfp4* pVDest = &VClip[0];
	CVec2Dfp4* pTVDest1 = &TVClip1[0];
	CVec2Dfp4* pTVDest2 = &TVClip2[0];
	CVec2Dfp4* pTVDest3 = &TVClip3[0];
	CVec3Dfp4* pVSrc = _pVerts;
	CVec2Dfp4* pTVSrc1 = _pTVerts1;
	CVec2Dfp4* pTVSrc2 = _pTVerts2;
	CVec2Dfp4* pTVSrc3 = _pTVerts3;

	for(int iPlane = 0; iPlane < _np; iPlane++)
	{
		const CPlane3Dfp4* pP = &_pPlanes[iPlane];
		fp4 VertPDist[32];
//		fp4 d = -((pP->p.k[0] * pP->n.k[0]) + (pP->p.k[1] * pP->n.k[1]) + (pP->p.k[2] * pP->n.k[2]));
//		fp4 d = pP->d;
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

							if (bClipTVerts1)
							{
								fp4 dtvx = (pTVSrc1[v2].k[0] - pTVSrc1[v].k[0]);
								fp4 dtvy = (pTVSrc1[v2].k[1] - pTVSrc1[v].k[1]);
								pTVDest1[nClip].k[0] = pTVSrc1[v].k[0] + dtvx * t;
								pTVDest1[nClip].k[1] = pTVSrc1[v].k[1] + dtvy * t;
							}
							if (bClipTVerts2)
							{
								fp4 dtvx = (pTVSrc2[v2].k[0] - pTVSrc2[v].k[0]);
								fp4 dtvy = (pTVSrc2[v2].k[1] - pTVSrc2[v].k[1]);
								pTVDest2[nClip].k[0] = pTVSrc2[v].k[0] + dtvx * t;
								pTVDest2[nClip].k[1] = pTVSrc2[v].k[1] + dtvy * t;
							}
							if (bClipTVerts3)
							{
								fp4 dtvx = (pTVSrc3[v2].k[0] - pTVSrc3[v].k[0]);
								fp4 dtvy = (pTVSrc3[v2].k[1] - pTVSrc3[v].k[1]);
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

							if (bClipTVerts1)
							{
								fp4 dtvx = (pTVSrc1[v2].k[0] - pTVSrc1[v].k[0]);
								fp4 dtvy = (pTVSrc1[v2].k[1] - pTVSrc1[v].k[1]);
								pTVDest1[nClip].k[0] = pTVSrc1[v].k[0] + dtvx * t;
								pTVDest1[nClip].k[1] = pTVSrc1[v].k[1] + dtvy * t;
							}
							if (bClipTVerts2)
							{
								fp4 dtvx = (pTVSrc2[v2].k[0] - pTVSrc2[v].k[0]);
								fp4 dtvy = (pTVSrc2[v2].k[1] - pTVSrc2[v].k[1]);
								pTVDest2[nClip].k[0] = pTVSrc2[v].k[0] + dtvx * t;
								pTVDest2[nClip].k[1] = pTVSrc2[v].k[1] + dtvy * t;
							}
							if (bClipTVerts3)
							{
								fp4 dtvx = (pTVSrc3[v2].k[0] - pTVSrc3[v].k[0]);
								fp4 dtvy = (pTVSrc3[v2].k[1] - pTVSrc3[v].k[1]);
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
		memcpy(_pVerts, pVSrc, _nv*sizeof(CVec3Dfp4));
		if (bClipTVerts1) memcpy(_pTVerts1, pTVSrc1, _nv*sizeof(CVec2Dfp4));
		if (bClipTVerts2) memcpy(_pTVerts2, pTVSrc2, _nv*sizeof(CVec2Dfp4));
		if (bClipTVerts3) memcpy(_pTVerts3, pTVSrc3, _nv*sizeof(CVec2Dfp4));
	}
	return _nv;
}

// -------------------------------------------------------------------

// Added by Mondelore.
void MCCDLLEXPORT MatrixLerp(const CMat4Dfp4 &_M0, const CMat4Dfp4 &_M1, fp4 _T, CMat4Dfp4 &_Res)
{
	CVec3Dfp4 VRes;
	CVec3Dfp4::GetMatrixRow(_M0, 3).Lerp(CVec3Dfp4::GetMatrixRow(_M1, 3), _T, VRes);

	CQuatfp4 q1, q2;
	q1.Create(_M0);
	q2.Create(_M1);

	CQuatfp4 QRes;
	q1.Interpolate(q2, QRes, _T);

	QRes.CreateMatrix(_Res);
	VRes.SetMatrixRow(_Res, 3);
}

// -------------------------------------------------------------------

// -------------------------------------------------------------------

// Added by Mondelore.
void MCCDLLEXPORT MatrixSpline(const CMat4Dfp4 &_M0, const CMat4Dfp4 &_M1, const CMat4Dfp4 &_M2, const CMat4Dfp4 &_M3, 
							   fp4 _TimeFraction, CMat4Dfp4 &_Res)
{
	CVec3Dfp4 pres;
	CQuatfp4 qres;

	CVec3Dfp4 p0, p1, p2, p3;
	p0 = CVec3Dfp4::GetMatrixRow(_M0, 3);
	p1 = CVec3Dfp4::GetMatrixRow(_M1, 3);
	p2 = CVec3Dfp4::GetMatrixRow(_M2, 3);
	p3 = CVec3Dfp4::GetMatrixRow(_M3, 3);

	CQuatfp4 q0, q1, q2, q3;
	q0.Create(_M0);
	q1.Create(_M1);
	q2.Create(_M2);
	q3.Create(_M3);

	fp4 dt01 = 1;
	fp4 dt12 = 1;
	fp4 dt23 = 1;
	
	CVec3Dfp4::Spline(&p0, &p1, &p2, &p1, &p2, &p3, &pres, _TimeFraction, dt01, dt12, dt12, dt23, 1);
	CQuatfp4::Spline(&q0, &q1, &q2, &q1, &q2, &q3, &qres, _TimeFraction, dt01, dt12, dt12, dt23, 1);

	qres.CreateMatrix(_Res);
	pres.SetMatrixRow(_Res, 3);
}
