
#include "PCH.h"

#include "WPhys.h"
#include "../XRClass.h"
#include "MFloat.h"

#define V3_DOTPROD(a, b) (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])
#define V3_CROSSPROD(p, q, cross) { cross[0] = p[1]*q[2]-p[2]*q[1]; cross[1] = p[2]*q[0]-p[0]*q[2]; cross[2] = p[0]*q[1]-p[1]*q[0]; }

unsigned int TestIntersectionS (const CVec3Dfp32 tri[3], const CPhysOBB& _Box);

enum
{
    INTERSECTION,
    AXIS_N,
    AXIS_A0, AXIS_A1, AXIS_A2,
    AXIS_A0xE0, AXIS_A0xE1, AXIS_A0xE2,
    AXIS_A1xE0, AXIS_A1xE1, AXIS_A1xE2,
    AXIS_A2xE0, AXIS_A2xE1, AXIS_A2xE2
};

void Phys_CreatePlaneOnBox(const CVec3Dfp32& _N, const CPhysOBB& _OBB, CPlane3Dfp32& _Plane);

fp32 Phys_GetMinPolyDistance( const CVec3Dfp32* _pVertices, const uint32* _pVertIndices, int _nVertexCount, const CPlane3Dfp32& _Plane, int& _VertexNr)
{
	MAUTOSTRIP(Phys_GetMinPolyDistance, 0.0f);

	fp32 BestD = V3_DOTPROD( _Plane.n, _pVertices[_pVertIndices[0]] ) + _Plane.d;
	_VertexNr = _pVertIndices[0];
	
	for( int v = 1; v < _nVertexCount; v++ )
	{
		fp32 d = V3_DOTPROD(_Plane.n, _pVertices[_pVertIndices[v]]) + _Plane.d;
		if( d < BestD )
		{
			BestD = d;
			_VertexNr = _pVertIndices[v];
		}
	}

	return BestD;
}


void Phys_CreatePlaneOnPoly(const CVec3Dfp32& _N, const CVec3Dfp32* _pVertices, const uint32* _pVertIndices, const int _nVertexCount, CPlane3Dfp32& _Plane)
{
	MAUTOSTRIP(Phys_CreatePlaneOnPoly, MAUTOSTRIP_VOID);
	fp32 BestD = V3_DOTPROD(_N, _pVertices[_pVertIndices[0]] );
	
	for( int v = 1; v < _nVertexCount; v++ )
	{
		fp32 d = V3_DOTPROD(_N, _pVertices[_pVertIndices[v]] );
		if( d > BestD )
		{
			BestD = d;
		}
	}

	_Plane.CreateND(_N, -BestD);
}

void Phys_GetSeparatingDistance(const CPhysOBB& _Box, const CVec3Dfp32* _pVertices, const uint32* _pVertIndices, const int _nVertexCount, const CVec3Dfp32& _V, const CVec3Dfp32& _N, fp32& _BestD, CPlane3Dfp32& _BestPlane, bool _bNormalize)
{
	MAUTOSTRIP(Phys_GetSeparatingDistance, MAUTOSTRIP_VOID);
	const CVec3Dfp32* pN = &_N;
	CVec3Dfp32 N2;
	fp32 Len2 = V3_DOTPROD(_N, _N);

	if (Len2 < 0.001f) return;
	if (_bNormalize)
	{
		fp64 InvLen = M_InvSqrt(Len2);
		N2.k[0] = _N.k[0] * InvLen;
		N2.k[1] = _N.k[1] * InvLen;
		N2.k[2] = _N.k[2] * InvLen;
		pN = &N2;
	}

	fp32 Dot = _V * (*pN);
	if (M_Fabs(Dot) < 0.001f) return;

	if (Dot < 0.0f)
	{
		CPlane3Dfp32 P;
		Phys_CreatePlaneOnPoly( (*pN), _pVertices, _pVertIndices, _nVertexCount, P );

		fp32 d = _Box.MinDistance(P);
		if (d < Dot - 0.001f) return;

		if (d > _BestD)
		{
			_BestD = d;
			_BestPlane = P;
		}
	}
	else
	{
		CPlane3Dfp32 P;
		Phys_CreatePlaneOnPoly( -(*pN), _pVertices, _pVertIndices, _nVertexCount, P );

		fp32 d = _Box.MinDistance(P);
		if (d < -Dot - 0.001f) return;

		if (d > _BestD)
		{
			_BestD = d;
			_BestPlane = P;
		}
	}
}

void Phys_GetSeparatingDistance_NoCheck(const CPhysOBB& _Box, const CVec3Dfp32* _pVertices, const uint32* _pVertIndices, int _nVertexCount, const CVec3Dfp32& _V, const CVec3Dfp32& _N, fp32& _BestD, CPlane3Dfp32& _BestPlane)
{
	MAUTOSTRIP(Phys_GetSeparatingDistance_NoCheck, MAUTOSTRIP_VOID);

	if (V3_DOTPROD(_V, _N) < 0.0f)
	{
		Phys_CreatePlaneOnPoly(_N, _pVertices, _pVertIndices, _nVertexCount, _BestPlane);

		_BestD = _Box.MinDistance(_BestPlane);
	}
	else
	{
		Phys_CreatePlaneOnPoly(-_N, _pVertices, _pVertIndices, _nVertexCount, _BestPlane);

		_BestD = _Box.MinDistance(_BestPlane);
	}
}

// Poly is assumed to be a convex fan

bool Phys_Intersect_PolyOBB(const CVec3Dfp32* _pVertices, const uint32* _pVertIndices, const int _nVertexCount, const CPlane3Dfp32& _PolyPlane, const CPhysOBB& _BoxStart, const CPhysOBB& _BoxDest, bool _bOrder, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(Phys_Intersect_PolyOBB, false);
	// (return == False) => No intersection)
	// (_bOrder == false) => Box collides to triangle. (Plane is adjecent to triangle)
	// (_bOrder == true) => Triangle collides to box. (Plane is adjecent to box)

//	bool bLog = false;


	if (!_pCollisionInfo)
	{
		CVec3Dfp32 tri[3];
		tri[0] = _pVertices[_pVertIndices[0]];
		tri[1] = _pVertices[_pVertIndices[1]];
		for( int v = 2; v < _nVertexCount; v++ )
		{
			tri[2] = _pVertices[_pVertIndices[v]];
			if( TestIntersectionS(tri, _BoxDest) == INTERSECTION)
				return true;
			tri[1]	= tri[2];
		}
		
		return false;
	}
	else
	{
		{
			CVec3Dfp32 tri[3];
			int v;
			tri[0] = _pVertices[_pVertIndices[0]];
			tri[1] = _pVertices[_pVertIndices[1]];
			for( v = 2; v < _nVertexCount; v++ )
			{
				tri[2] = _pVertices[_pVertIndices[v]];
				if( TestIntersectionS(tri, _BoxDest) == INTERSECTION)
					break;
				tri[1]	= tri[2];
			}
			
			if( v == _nVertexCount )
				return false;
		}

		CVec3Dfp32 dV;
		_BoxDest.m_C.Sub(_BoxStart.m_C, dV);

		const CVec3Dfp32& PolyN = _PolyPlane.n;
		fp32 BestD = -100001.0f;
		Phys_GetSeparatingDistance(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, PolyN, BestD, _pCollisionInfo->m_Plane, false);

		Phys_GetSeparatingDistance(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, _BoxDest.m_A[0], BestD, _pCollisionInfo->m_Plane, false);
		Phys_GetSeparatingDistance(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, _BoxDest.m_A[1], BestD, _pCollisionInfo->m_Plane, false);
		Phys_GetSeparatingDistance(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, _BoxDest.m_A[2], BestD, _pCollisionInfo->m_Plane, false);

		const CVec3Dfp32 *p0, *p1;
		p0	= &_pVertices[_pVertIndices[_nVertexCount-1]];
		for( int e = 0; e < _nVertexCount; e++ )
		{
			p1 = &_pVertices[_pVertIndices[e]];
			const CVec3Dfp32 Edge = (*p1) - (*p0);
			Phys_GetSeparatingDistance(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, (_BoxDest.m_A[0] / Edge), BestD, _pCollisionInfo->m_Plane, true);
			Phys_GetSeparatingDistance(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, (_BoxDest.m_A[1] / Edge), BestD, _pCollisionInfo->m_Plane, true);
			Phys_GetSeparatingDistance(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, (_BoxDest.m_A[2] / Edge), BestD, _pCollisionInfo->m_Plane, true);
			
			p0 = p1;
		}

		if (BestD < -100000)
		{
			{
				CVec3Dfp32 E0, E1, E2;
				CVec3Dfp32 tri[3];
				tri[0] = _pVertices[_pVertIndices[0]];
				tri[1] = _pVertices[_pVertIndices[1]];
				tri[2] = _pVertices[_pVertIndices[2]];
				{
					
					tri[1].Sub( tri[0], E0 );
					tri[2].Sub( tri[0], E1 );
					tri[2].Sub( tri[1], E2 );
					int IntersectStart = TestIntersectionS(tri, _BoxStart);
					{
						switch(IntersectStart)
						{
						case INTERSECTION :
						case AXIS_N :
							{
								Phys_GetSeparatingDistance_NoCheck(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, PolyN, BestD, _pCollisionInfo->m_Plane);
							}
							break;
						case AXIS_A0 : 
						case AXIS_A1 : 
						case AXIS_A2 : 
							{
								int k = IntersectStart - AXIS_A0;
								Phys_GetSeparatingDistance_NoCheck(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, _BoxStart.m_A[k], BestD, _pCollisionInfo->m_Plane);
							}
							break;
						default :
							{
								CVec3Dfp32 N;
								switch(IntersectStart)
								{
								case AXIS_A0xE0 :
									V3_CROSSPROD(_BoxStart.m_A[0], E0, N);
									break;
								case AXIS_A0xE1 :
									V3_CROSSPROD(_BoxStart.m_A[0], E1, N);
									break;
								case AXIS_A0xE2 :
									V3_CROSSPROD(_BoxStart.m_A[0], E2, N);
									break;
								case AXIS_A1xE0 :
									V3_CROSSPROD(_BoxStart.m_A[1], E0, N);
									break;
								case AXIS_A1xE1 :
									V3_CROSSPROD(_BoxStart.m_A[1], E1, N);
									break;
								case AXIS_A1xE2 :
									V3_CROSSPROD(_BoxStart.m_A[1], E2, N);
									break;
								case AXIS_A2xE0 :
									V3_CROSSPROD(_BoxStart.m_A[2], E0, N);
									break;
								case AXIS_A2xE1 :
									V3_CROSSPROD(_BoxStart.m_A[2], E1, N);
									break;
								case AXIS_A2xE2 :
									V3_CROSSPROD(_BoxStart.m_A[2], E2, N);
									break;
								}
								N.Normalize();
								Phys_GetSeparatingDistance_NoCheck(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, N, BestD, _pCollisionInfo->m_Plane);
							}
						}
					}
				}
			}
		}

		if (_bOrder)
		{
			_pCollisionInfo->m_Plane.Inverse();
			Phys_CreatePlaneOnBox(_pCollisionInfo->m_Plane.n, _BoxDest, _pCollisionInfo->m_Plane);
		}


		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_bIsCollision = true;
		_pCollisionInfo->m_Velocity = 0;
		if (_bOrder)
		{
			int VertexNr;
			_pCollisionInfo->m_Distance = Phys_GetMinPolyDistance(_pVertices, _pVertIndices, _nVertexCount, _pCollisionInfo->m_Plane, VertexNr) - 0.001f;
			_pCollisionInfo->m_LocalPos = _pVertices[VertexNr] - _pCollisionInfo->m_Plane.n*_pCollisionInfo->m_Distance;
		}
		else
		{
			_pCollisionInfo->m_Distance = _BoxDest.MinDistance(_pCollisionInfo->m_Plane) - 0.001f;
			_pCollisionInfo->m_Pos = _BoxDest.m_C - _pCollisionInfo->m_Plane.n*_pCollisionInfo->m_Plane.Distance(_BoxDest.m_C);
			_pCollisionInfo->m_LocalPos = _BoxDest.m_C - _pCollisionInfo->m_Plane.n*_pCollisionInfo->m_Plane.Distance(_BoxDest.m_C);
		}

		fp32 VLen = dV.Length();
		fp32 vProj = _pCollisionInfo->m_Plane.n * dV;
		if (vProj < -0.001f * VLen)
			_pCollisionInfo->m_Time = Clamp01(1.0f - _pCollisionInfo->m_Distance / vProj);
		else
		{
			_pCollisionInfo->m_Time = 1.0f;
		}

		return true;
	}
}
