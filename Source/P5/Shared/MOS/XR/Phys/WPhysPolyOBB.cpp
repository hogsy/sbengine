
#include "PCH.h"

#ifndef	PLATFORM_PS2

#include "WPhys.h"
#include "../XRClass.h"
#include "MFloat.h"

#define V3_DOTPROD(a, b) (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])
#define V3_CROSSPROD(p, q, cross) { cross[0] = p[1]*q[2]-p[2]*q[1]; cross[1] = p[2]*q[0]-p[0]*q[2]; cross[2] = p[0]*q[1]-p[1]*q[0]; }

unsigned int TestIntersectionS (const CVec3Dfp64 tri[3], const CPhysOBB& _Box);

enum
{
    INTERSECTION,
    AXIS_N,
    AXIS_A0, AXIS_A1, AXIS_A2,
    AXIS_A0xE0, AXIS_A0xE1, AXIS_A0xE2,
    AXIS_A1xE0, AXIS_A1xE1, AXIS_A1xE2,
    AXIS_A2xE0, AXIS_A2xE1, AXIS_A2xE2
};

void Phys_CreatePlaneOnBox(const CVec3Dfp64& _N, const CPhysOBB& _OBB, CPlane3Dfp64& _Plane);

fp64 Phys_GetMinPolyDistance( const CVec3Dfp32* _pVertices, const uint16* _pVertIndices, int _nVertexCount, const CPlane3Dfp64& _Plane, int& _VertexNr)
{
	MAUTOSTRIP(Phys_GetMinPolyDistance, 0.0f);

	fp64 BestD = V3_DOTPROD( _Plane.n, _pVertices[_pVertIndices[0]] ) + _Plane.d;
	_VertexNr = _pVertIndices[0];
	
	for( int v = 1; v < _nVertexCount; v++ )
	{
		fp64 d = V3_DOTPROD(_Plane.n, _pVertices[_pVertIndices[v]]) + _Plane.d;
		if( d < BestD )
		{
			BestD = d;
			_VertexNr = _pVertIndices[v];
		}
	}

	return BestD;
}


void Phys_CreatePlaneOnPoly(const CVec3Dfp64& _N, const CVec3Dfp32* _pVertices, const uint16* _pVertIndices, const int _nVertexCount, CPlane3Dfp64& _Plane)
{
	MAUTOSTRIP(Phys_CreatePlaneOnTriangle, MAUTOSTRIP_VOID);
	fp64 BestD = V3_DOTPROD(_N, _pVertices[_pVertIndices[0]] );
	
	for( int v = 1; v < _nVertexCount; v++ )
	{
		fp64 d = V3_DOTPROD(_N, _pVertices[_pVertIndices[v]] );
		if( d > BestD )
		{
			BestD = d;
		}
	}

	_Plane.CreateND(_N, -BestD);
}

void Phys_GetSeparatingDistance(const CPhysOBB& _Box, const CVec3Dfp32* _pVertices, const uint16* _pVertIndices, const int _nVertexCount, const CVec3Dfp64& _V, const CVec3Dfp64& _N, fp64& _BestD, CPlane3Dfp64& _BestPlane, bool _bNormalize)
{
	MAUTOSTRIP(Phys_GetSeparatingDistance, MAUTOSTRIP_VOID);
	const CVec3Dfp64* pN = &_N;
	CVec3Dfp64 N2;
	fp64 Len2 = V3_DOTPROD(_N, _N);

	if (Len2 < 0.001) return;
	if (_bNormalize)
	{
		fp64 InvLen = M_InvSqrt(Len2);
		N2.k[0] = _N.k[0] * InvLen;
		N2.k[1] = _N.k[1] * InvLen;
		N2.k[2] = _N.k[2] * InvLen;
		pN = &N2;
	}

	fp64 Dot = _V * (*pN);
	if (M_Fabs(Dot) < 0.001) return;

	if (Dot < 0.0f)
	{
		CPlane3Dfp64 P;
		Phys_CreatePlaneOnPoly( (*pN), _pVertices, _pVertIndices, _nVertexCount, P );

		fp64 d = _Box.MinDistance(P);
		if (d < Dot - 0.001) return;

		if (d > _BestD)
		{
			_BestD = d;
			_BestPlane = P;
		}
	}
	else
	{
		CPlane3Dfp64 P;
		Phys_CreatePlaneOnPoly( -(*pN), _pVertices, _pVertIndices, _nVertexCount, P );

		fp64 d = _Box.MinDistance(P);
		if (d < -Dot - 0.001) return;

		if (d > _BestD)
		{
			_BestD = d;
			_BestPlane = P;
		}
	}
}


void Phys_GetSeparatingDistance_NoCheck(const CPhysOBB& _Box, const CVec3Dfp32* _pVertices, const uint16* _pVertIndices, int _nVertexCount, const CVec3Dfp64& _V, const CVec3Dfp64& _N, fp64& _BestD, CPlane3Dfp64& _BestPlane);
void Phys_GetSeparatingDistance_NoCheck(const CPhysOBB& _Box, const CVec3Dfp32* _pVertices, const uint16* _pVertIndices, int _nVertexCount, const CVec3Dfp64& _V, const CVec3Dfp64& _N, fp64& _BestD, CPlane3Dfp64& _BestPlane)
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

static bool FloatIsFucked( fp32 _Value );
static bool FloatIsFucked( fp32 _Value )
{
	return FloatIsInvalid( _Value ) || FloatIsInfinite( _Value ) || FloatIsNAN( _Value ) || ( M_Fabs( _Value ) > 10000.0f );
}

bool Phys_Intersect_PolyOBB(const CVec3Dfp32* _pVertices, const uint16* _pVertIndices, const int _nVertexCount, const CPlane3Dfp32& _PolyPlane, const CPhysOBB& _BoxStart, const CPhysOBB& _BoxDest, bool _bOrder, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(Phys_Intersect_PolyOBB, false);
	// (return == False) => No intersection)
	// (_bOrder == false) => Box collides to triangle. (Plane is adjecent to triangle)
	// (_bOrder == true) => Triangle collides to box. (Plane is adjecent to box)

//	bool bLog = false;

	if (!_pCollisionInfo)
	{
		CVec3Dfp64 tri[3];
		tri[0]	= _pVertices[_pVertIndices[0]].Getfp64();
		tri[1]	= _pVertices[_pVertIndices[1]].Getfp64();
		for( int v = 2; v < _nVertexCount; v++ )
		{
			tri[2]	= _pVertices[_pVertIndices[v]].Getfp64();
			if( TestIntersectionS(tri, _BoxDest) == INTERSECTION)
				return true;
			tri[1]	= tri[2];
		}
		
		return false;
	}
	else
	{
		{
			CVec3Dfp64 tri8[3];
			int v;
			tri8[0]	= _pVertices[_pVertIndices[0]].Getfp64();
			tri8[1]	= _pVertices[_pVertIndices[1]].Getfp64();
			for( v = 2; v < _nVertexCount; v++ )
			{
				tri8[2]	= _pVertices[_pVertIndices[v]].Getfp64();
				if( TestIntersectionS(tri8, _BoxDest) == INTERSECTION)
					break;
				tri8[1]	= tri8[2];
			}
			
			if( v == _nVertexCount )
				return false;
		}

		CVec3Dfp64 SrcCenter = _BoxStart.m_C.Getfp64();
		CVec3Dfp64 DestCenter = _BoxDest.m_C.Getfp64();
		CVec3Dfp64 dV;
		DestCenter.Sub(SrcCenter, dV);

		CPlane3Dfp64 BestPlane;

		CVec3Dfp64 PolyN = _PolyPlane.n.Getfp64();
		fp64 BestD = -100001.0f;

		Phys_GetSeparatingDistance(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, PolyN, BestD, BestPlane, false);

		CVec3Dfp64 Axis[3];
		Axis[0] = _BoxDest.m_A[0].Getfp64();
		Axis[1] = _BoxDest.m_A[1].Getfp64();
		Axis[2] = _BoxDest.m_A[2].Getfp64();
		Phys_GetSeparatingDistance(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, Axis[0], BestD, BestPlane, false);
		Phys_GetSeparatingDistance(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, Axis[1], BestD, BestPlane, false);
		Phys_GetSeparatingDistance(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, Axis[2], BestD, BestPlane, false);

		int Index0 = _nVertexCount-1;
		for( int e = 0; e < _nVertexCount; e++ )
		{
			int Index1 = e;
			const CVec3Dfp64 Edge = (_pVertices[_pVertIndices[Index1]] - _pVertices[_pVertIndices[Index0]]).Getfp64();
			Phys_GetSeparatingDistance(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, (Axis[0] / Edge), BestD, BestPlane, true);
			Phys_GetSeparatingDistance(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, (Axis[1] / Edge), BestD, BestPlane, true);
			Phys_GetSeparatingDistance(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, (Axis[2] / Edge), BestD, BestPlane, true);
			
			Index0 = Index1;
		}

		if (BestD < -100000)
		{
			{
				CVec3Dfp64 E0, E1, E2;
				CVec3Dfp64 tri[3];
				tri[0] = _pVertices[_pVertIndices[0]].Getfp64();
				tri[1] = _pVertices[_pVertIndices[1]].Getfp64();
				tri[2] = _pVertices[_pVertIndices[2]].Getfp64();

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
								Phys_GetSeparatingDistance_NoCheck(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, PolyN, BestD, BestPlane);
							}
							break;
						case AXIS_A0 : 
						case AXIS_A1 : 
						case AXIS_A2 : 
							{
								int k = IntersectStart - AXIS_A0;
								Phys_GetSeparatingDistance_NoCheck(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, _BoxStart.m_A[k].Getfp64(), BestD, BestPlane);
							}
							break;
						default :
							{
								CVec3Dfp64 N;
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
								Phys_GetSeparatingDistance_NoCheck(_BoxDest, _pVertices, _pVertIndices, _nVertexCount, dV, N, BestD, BestPlane);
							}
						}
					}
				}
			}
		}

		if (_bOrder)
		{
			BestPlane.Inverse();
			Phys_CreatePlaneOnBox(BestPlane.n, _BoxDest, BestPlane);
		}

		BestPlane.n.Assignfp32( _pCollisionInfo->m_Plane.n );
		_pCollisionInfo->m_Plane.d = BestPlane.d;

		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_bIsCollision = true;
		_pCollisionInfo->m_Velocity = 0;
		if (_bOrder)
		{
			int VertexNr;
			_pCollisionInfo->m_Distance = Phys_GetMinPolyDistance(_pVertices, _pVertIndices, _nVertexCount, BestPlane, VertexNr) - 0.001f;
			_pCollisionInfo->m_LocalPos = _pVertices[VertexNr] - _pCollisionInfo->m_Plane.n*_pCollisionInfo->m_Distance;
		}
		else
		{
			_pCollisionInfo->m_Distance = _BoxDest.MinDistance(BestPlane) - 0.001f;
			_pCollisionInfo->m_Pos = _BoxDest.m_C - _pCollisionInfo->m_Plane.n*_pCollisionInfo->m_Plane.Distance(_BoxDest.m_C);
			_pCollisionInfo->m_LocalPos = _BoxDest.m_C - _pCollisionInfo->m_Plane.n*_pCollisionInfo->m_Plane.Distance(_BoxDest.m_C);
		}

		fp64 VLen = dV.Length();
		fp64 vProj = BestPlane.n * dV;
		if (vProj < -0.001f * VLen)
			_pCollisionInfo->m_Time = Clamp01(1.0f - _pCollisionInfo->m_Distance / vProj);
		else
		{
			_pCollisionInfo->m_Time = 1.0f;
		}
/*
		if( FloatIsFucked( _pCollisionInfo->m_Pos.k[0] ) ||
			FloatIsFucked( _pCollisionInfo->m_Pos.k[1] ) ||
			FloatIsFucked( _pCollisionInfo->m_Pos.k[2] ) ||
			FloatIsFucked( _pCollisionInfo->m_Time ) ||
			FloatIsFucked( _pCollisionInfo->m_Plane.d ) ||
			FloatIsFucked( _pCollisionInfo->m_Plane.n.k[0] ) ||
			FloatIsFucked( _pCollisionInfo->m_Plane.n.k[1] ) ||
			FloatIsFucked( _pCollisionInfo->m_Plane.n.k[2] ) ||
			FloatIsFucked( _pCollisionInfo->m_LocalPos.k[0] ) ||
			FloatIsFucked( _pCollisionInfo->m_LocalPos.k[1] ) ||
			FloatIsFucked( _pCollisionInfo->m_LocalPos.k[2] ) ||
			FloatIsFucked( _pCollisionInfo->m_Distance ) )
		{
			int j = 0;
			return Phys_Intersect_PolyOBB(_pVertices, _pVertIndices, _nVertexCount, _PolyPlane, _BoxStart, _BoxDest, _bOrder, _pCollisionInfo );
		}
*/
		return true;
	}
}


#endif	// PLATFORM_PS2
