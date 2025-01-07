
#include "PCH.h"

#include "WPhysOBB.h"
#include "WPhys.h"
#include "../XRClass.h"
#include "MFloat.h"

#if defined(PLATFORM_PS2)
#undef DOT
#undef CROSS

static inline fp32 _DotProd( fp32 _a0, fp32 _a1, fp32 _a2, fp32 _b0, fp32 _b1, fp32 _b2 )
{
	fp32 ret;
	asm ("
			mula.s _a0,_b0
			madda.s	_a1,_b1
			madd.s ret, _a2,_b2
		 ");
	return ret;
}

static inline void _CrossProd( fp32 _a0, fp32 _a1, fp32 _a2, fp32 _b0, fp32 _b1, fp32 _b2, CVec3Dfp32* _pDest )
{
	fp32 vala, valb, valc;

	asm ("
			mula.s _a1, _b2
			msub.s vala, _a2, _b1

			mula.s _a2, _b0
			msub.s valb, _a0, _b2

			mula.s _a0, _b1
			msub.s valc, _a1, _b0
		 "
		);


	_pDest->k[0]	= vala;
	_pDest->k[1]	= valb;
	_pDest->k[2]	= valc;
}

#define CROSS( dest, a, b ) _CrossProd( a.k[0], a.k[1], a.k[2], b.k[0], b.k[1], b.k[2], &dest )
#define DOT( a, b ) _DotProd( a.k[0], a.k[1], a.k[2], b.k[0], b.k[1], b.k[2] )
#endif

#ifdef PLATFORM_XBOX1

fp32 SSE_PhysOBB_MinDistance(const CPhysOBB& _OBB, const CPlane3Dfp32& _Plane)
{
	static uint32 ms_XMMMaskVec3[4] = { 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000 };
	static uint32 ms_XMMZeroVec[4] = { 0x00000000, 0x00000000, 0x00000000, 0x3f800000 };
	static fp32 ms_XMMNegVec[4] = { -1.0f, -1.0f, -1.0f, 1.0f };

	fp32 Result;

	__asm
	{
		movups xmm6, [ms_XMMMaskVec3]
		mov esi, [_OBB]
		movups xmm7, [ms_XMMZeroVec]
		mov edi, [_Plane]

		movlps xmm5, [esi+12*4]	// xmm5 = m_E
		movss xmm4, [esi+12*4+8]
		shufps xmm5, xmm4, 0+4

		movups xmm0, [edi]	// xmm0 = _Plane
		movups xmm1, [esi+12] // xmm2 = _OBB.m_A[0]
		movups xmm2, [esi+12*2] // xmm2 = _OBB.m_A[1]
		movups xmm3, [esi+12*3] // xmm2 = _OBB.m_A[2]
		movups xmm4, [esi]	// xmm1 = _OBB.m_C

		// Zero/One the 4:th component
		andps xmm1, xmm6
		andps xmm2, xmm6
		andps xmm3, xmm6
		andps xmm4, xmm6
		andps xmm5, xmm6
		orps xmm4, xmm7
		orps xmm5, xmm7

		// 4 dot products, xmm1 = (xmm1*xmm0, xmm2*xmm0, xmm3*xmm0, xmm4*xmm0)
		mulps xmm1, xmm0		
		mulps xmm2, xmm0
		mulps xmm3, xmm0
		mulps xmm4, xmm0

		movaps xmm0, xmm1
		movlhps xmm1, xmm2
		movhlps xmm2, xmm0
		addps xmm1, xmm2

		movaps xmm0, xmm3
		movlhps xmm3, xmm4
		movhlps xmm4, xmm0
		addps xmm3, xmm4

		movaps xmm0, xmm1
		shufps xmm1, xmm3, 0 + (2 << 2) + (0 << 4) + (2 << 6)
		shufps xmm0, xmm3, 1 + (3 << 2) + (1 << 4) + (3 << 6)
		addps xmm1, xmm0

		// xmm1 *= _OBB.m_E
		mulps xmm1, xmm5

		// Abs() on the first 3 components
		movups xmm2, [ms_XMMNegVec]
		movaps xmm3, xmm1
		mulps xmm3, xmm2
		maxps xmm1, xmm3

		// Negate the first 3 components
		mulps xmm1, xmm2

		// Horizontal add of xmm1 components
		movhlps xmm4, xmm1
		addps xmm1, xmm4
		movaps xmm4, xmm1
		shufps xmm4, xmm4, 1
		addss xmm1, xmm4

		movss [Result], xmm1
	}

	return Result;
}

#endif

//---------------------------------------------------------------------------
float CPhysOBB::GetRadiusSquared() const
{
	MAUTOSTRIP(CPhysOBB_GetVertices, 0);

	register int i;

	float ret = 0.f;
	
	CVec3Dfp32 v[4];

    for( i = 0; i < 3; i++ )
    {
    	register const float e0a0 = m_E[0]*m_A[0][i];
    	register const float e1a1 = m_E[1]*m_A[1][i];
    	register const float e2a2 = m_E[2]*m_A[2][i];
    
        v[0][i] = e0a0 - e1a1 - e2a2;
        v[1][i] = e0a0 + e1a1 - e2a2;
        v[2][i] = e0a0 - e1a1 + e2a2;
        v[3][i] = e0a0 + e1a1 + e2a2;
    }


	register float *v_ptr = (float *)v;

	for( i = 0; i < 3; i++ )
	{
		register const float x = *v_ptr++;
		register const float y = *v_ptr++;
		register const float z = *v_ptr++;

		register const float r = x * x + y * y + z * z;
		
		if( r > ret )
			ret = r;
	}
	
	return ret;
}


//---------------------------------------------------------------------------
void CPhysOBB::GetVertices (CVec3Dfp32 vertex[8]) const
{
	MAUTOSTRIP(CPhysOBB_GetVertices, MAUTOSTRIP_VOID);
    for(int d = 0; d < 3; d++)
    {
    	register const fp32 e0a0 = m_E[0]*m_A[0][d];
    	register const fp32 e1a1 = m_E[1]*m_A[1][d];
    	register const fp32 e2a2 = m_E[2]*m_A[2][d];
    	register const fp32 c = m_C[d];
    
        vertex[0][d] = c - e0a0 - e1a1 - e2a2;
        vertex[1][d] = c + e0a0 - e1a1 - e2a2;
        vertex[2][d] = c + e0a0 + e1a1 - e2a2;
        vertex[3][d] = c - e0a0 + e1a1 - e2a2;
        vertex[4][d] = c - e0a0 - e1a1 + e2a2;
        vertex[5][d] = c + e0a0 - e1a1 + e2a2;
        vertex[6][d] = c + e0a0 + e1a1 + e2a2;
        vertex[7][d] = c - e0a0 + e1a1 + e2a2;
    }
}

//---------------------------------------------------------------------------
void CPhysOBB::Transform(const CMat4Dfp32& _Mat, CPhysOBB& _Dest) const
{
	MAUTOSTRIP(CPhysOBB_Transform, MAUTOSTRIP_VOID);
	CMat4Dfp32 Mat = _Mat;
	_Dest.m_C = m_C * Mat;
	m_A[0].MultiplyMatrix3x3(Mat, _Dest.m_A[0]);
	m_A[1].MultiplyMatrix3x3(Mat, _Dest.m_A[1]);
	m_A[2].MultiplyMatrix3x3(Mat, _Dest.m_A[2]);
	_Dest.m_E = m_E;
}

void CPhysOBB::GetBoundBox(CBox3Dfp32& _Box) const
{
	MAUTOSTRIP(CPhysOBB_GetBoundBox, MAUTOSTRIP_VOID);

	CVec3Dfp32 TMax;
	fp32 e0 = m_E[0];
	fp32 e1 = m_E[1];
	fp32 e2 = m_E[2];
	for(int k = 0; k < 3; k++)
	{
		TMax[k] = M_Fabs(m_A[0][k]*e0) + M_Fabs(m_A[1][k]*e1) + M_Fabs(m_A[2][k]*e2);
	}
	
	_Box.m_Min[0] = m_C[0] - TMax[0];
	_Box.m_Max[0] = m_C[0] + TMax[0];
	_Box.m_Min[1] = m_C[1] - TMax[1];
	_Box.m_Max[1] = m_C[1] + TMax[1];
	_Box.m_Min[2] = m_C[2] - TMax[2];
	_Box.m_Max[2] = m_C[2] + TMax[2];
}

void CPhysOBB::TransformToBoxSpace(const CVec3Dfp32& _v, CVec3Dfp32& _Dst) const
{
	MAUTOSTRIP(CPhysOBB_TransformToBoxSpace, MAUTOSTRIP_VOID);

	fp32 vx = _v[0] - m_C[0];
	fp32 vy = _v[1] - m_C[1];
	fp32 vz = _v[2] - m_C[2];

	_Dst.k[0] = vx*m_A[0][0] + vy*m_A[0][1] + vz*m_A[0][2];
	_Dst.k[1] = vx*m_A[1][0] + vy*m_A[1][1] + vz*m_A[1][2];
	_Dst.k[2] = vx*m_A[2][0] + vy*m_A[2][1] + vz*m_A[2][2];
}

void CPhysOBB::TransformFromBoxSpace(const CVec3Dfp32& _v, CVec3Dfp32& _Dst) const
{
	MAUTOSTRIP(CPhysOBB_TransformFromBoxSpace, MAUTOSTRIP_VOID);

	_Dst.k[0] = _v.k[0]*m_A[0][0] + _v.k[1]*m_A[1][0] + _v.k[2]*m_A[2][0] + m_C[0];
	_Dst.k[1] = _v.k[0]*m_A[0][1] + _v.k[1]*m_A[1][1] + _v.k[2]*m_A[2][1] + m_C[1];
	_Dst.k[2] = _v.k[0]*m_A[0][2] + _v.k[1]*m_A[1][2] + _v.k[2]*m_A[2][2] + m_C[2];
}

fp32 CPhysOBB::GetMinSqrDistance(const CVec3Dfp32& _v) const
{
	MAUTOSTRIP(CPhysOBB_GetMinSqrDistance, 0.0f);
	// _v is in box-space
	CVec3Dfp32 dV;

	for(int i = 0; i < 3; i++)
	{
		if (_v.k[i] > -m_E.k[i])
		{
			if (_v.k[i] > m_E.k[i])
				dV.k[i] = _v.k[i] - m_E.k[i];
			else
				dV.k[i] = 0;
		}

		else
			dV.k[i] = _v.k[i]-(m_E.k[i]);
	}

	return dV.LengthSqr();
}

fp32 CPhysOBB::GetMaxSqrDistance(const CVec3Dfp32& _v) const
{
	MAUTOSTRIP(CPhysOBB_GetMaxSqrDistance, 0.0f);
	// _v is in box-space
	CVec3Dfp32 dV;
	for(int i = 0; i < 3; i++)
	{
		if (_v.k[i] > 0)
			dV.k[i] = -m_E.k[i] - _v.k[i];
		else
			dV.k[i] = m_E.k[i] - _v.k[i];
	}

	return dV.LengthSqr();
}

//---------------------------------------------------------------------------

fp32 CPhysOBB::MinDistance(const CPlane3Dfp32& _Plane) const
{
	MAUTOSTRIP(CPhysOBB_MinDistance, 0.0f);
#ifdef PLATFORM_XBOX1

	return SSE_PhysOBB_MinDistance(*this, _Plane);

//	fp32 SSERes = SSE_PhysOBB_MinDistance(*this, _Plane);

/*	fp32 d = _Plane.Distance(m_C);
	for(int k = 0; k < 3; k++)
	{
		d -= M_Fabs(_Plane.n * m_A[k]) * m_E[k];
	}

	if (M_Fabs(SSERes - d) > 0.000001f)
	{
		int a = 1;
	}

	return d;*/
#else

	fp32 d = _Plane.Distance(m_C);
	for(int k = 0; k < 3; k++)
	{
		d -= M_Fabs(_Plane.n * m_A[k]) * m_E[k];
	}
	return d;
#endif
}

fp32 CPhysOBB::MaxDistance(const CPlane3Dfp32& _Plane) const
{
	MAUTOSTRIP(CPhysOBB_MaxDistance, 0.0f);
	fp32 d = _Plane.Distance(m_C);
	for(int k = 0; k < 3; k++)
	{
		d += M_Fabs(_Plane.n * m_A[k]) * m_E[k];
	}
	return d;
}

#define DOTPROD(a, b) (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])


#ifndef	CPU_SOFTWARE_FP64

fp64 CPhysOBB::MinDistance(const CPlane3Dfp64& _Plane) const
{
	MAUTOSTRIP(CPhysOBB_MinDistance_2, 0.0);

#ifdef PLATFORM_XBOX1
	CPlane3Dfp32 Plane4;
	_Plane.n.Assignfp32(Plane4.n);
	Plane4.d = _Plane.d;
	fp32 SSERes = SSE_PhysOBB_MinDistance(*this, Plane4);

	fp64 d = DOTPROD(_Plane.n, m_C) + _Plane.d;
	for(int k = 0; k < 3; k++)
	{
		d -= M_Fabs(DOTPROD(_Plane.n, m_A[k])) * m_E[k];
	}

	if (M_Fabs(SSERes - d) > 0.000001f)
	{
		int a = 1;
	}

	return d;

#else
	fp64 d = DOTPROD(_Plane.n, m_C) + _Plane.d;
	for(int k = 0; k < 3; k++)
	{
		d -= M_Fabs(DOTPROD(_Plane.n, m_A[k])) * m_E[k];
	}
	return d;
#endif
}

fp64 CPhysOBB::MaxDistance(const CPlane3Dfp64& _Plane) const
{
	MAUTOSTRIP(CPhysOBB_MaxDistance_2, 0.0);
	fp64 d = DOTPROD(_Plane.n, m_C) + _Plane.d;
	for(int k = 0; k < 3; k++)
	{
		d += M_Fabs(DOTPROD(_Plane.n, m_A[k])) * m_E[k];
	}
	return d;
}
#endif	// CPU_SOFTWARE_FP64

bool CPhysOBB::LocalPointInBox(const CVec3Dfp32 _p0) const
{
	MAUTOSTRIP(CPhysOBB_LocalPointInBox, false);
	for(int i = 0; i < 3; i++)
	{
		if (_p0.k[i] > m_E[i]) return false;
		if (_p0.k[i] < -m_E[i]) return false;
	}
	return true;
}

bool CPhysOBB::IntersectLine(const CVec3Dfp32 _p0, const CVec3Dfp32 _p1, CVec3Dfp32& _HitPos) const
{
	MAUTOSTRIP(CPhysOBB_IntersectLine, false);
	// _p0, _p1 == Line in box-coordinates.
	// Internal processing is in the box' coordinate-space.

	CVec3Dfp32 p0(_p0);
	CVec3Dfp32 p1(_p1);

	if (p0.k[0] < p1.k[0])
	{
		if (p0.k[0] > m_E.k[0]) return false;
		if (p1.k[0] < -m_E.k[0]) return false;

		if (p0.k[0] < -m_E.k[0])
		{
			fp32 t = (-m_E.k[0] - p0.k[0]) / (p1.k[0] - p0.k[0]);
			p0.k[0] = -m_E.k[0];
			p0.k[1] += t*(p1.k[1] - p0.k[1]);
			p0.k[2] += t*(p1.k[2] - p0.k[2]);
		}
		if (p1.k[0] > m_E.k[0])
		{
			fp32 t = -(p1.k[0] - m_E.k[0]) / (p1.k[0] - p0.k[0]);
			p1.k[0] = m_E.k[0];
			p1.k[1] += t*(p1.k[1] - p0.k[1]);
			p1.k[2] += t*(p1.k[2] - p0.k[2]);
		}
	}
	else
	{
		if (p1.k[0] > m_E.k[0]) return false;
		if (p0.k[0] < -m_E.k[0]) return false;

		if (p1.k[0] < -m_E.k[0])
		{
			fp32 t = (-m_E.k[0] - p1.k[0]) / (p0.k[0] - p1.k[0]);
			p1.k[0] = -m_E.k[0];
			p1.k[1] += t*(p0.k[1] - p1.k[1]);
			p1.k[2] += t*(p0.k[2] - p1.k[2]);
		}
		if (p0.k[0] > m_E.k[0])
		{
			fp32 t = -(p0.k[0] - m_E.k[0]) / (p0.k[0] - p1.k[0]);
			p0.k[0] = m_E.k[0];
			p0.k[1] += t*(p0.k[1] - p1.k[1]);
			p0.k[2] += t*(p0.k[2] - p1.k[2]);
		}
	}

	if (p0.k[1] < p1.k[1])
	{
		if (p0.k[1] > m_E.k[1]) return false;
		if (p1.k[1] < -m_E.k[1]) return false;

		if (p0.k[1] < -m_E.k[1])
		{
			fp32 t = (-m_E.k[1] - p0.k[1]) / (p1.k[1] - p0.k[1]);
			p0.k[0] += t*(p1.k[0] - p0.k[0]);
			p0.k[1] = -m_E.k[1];
			p0.k[2] += t*(p1.k[2] - p0.k[2]);
		}
		if (p1.k[1] > m_E.k[1])
		{
			fp32 t = -(p1.k[1] - m_E.k[1]) / (p1.k[1] - p0.k[1]);
			p1.k[0] += t*(p1.k[0] - p0.k[0]);
			p1.k[1] = m_E.k[1];
			p1.k[2] += t*(p1.k[2] - p0.k[2]);
		}
	}
	else
	{
		if (p1.k[1] > m_E.k[1]) return false;
		if (p0.k[1] < -m_E.k[1]) return false;

		if (p1.k[1] < -m_E.k[1])
		{
			fp32 t = (-m_E.k[1] - p1.k[1]) / (p0.k[1] - p1.k[1]);
			p1.k[0] += t*(p0.k[0] - p1.k[0]);
			p1.k[1] = -m_E.k[1];
			p1.k[2] += t*(p0.k[2] - p1.k[2]);
		}
		if (p0.k[1] > m_E.k[1])
		{
			fp32 t = -(p0.k[1] - m_E.k[1]) / (p0.k[1] - p1.k[1]);
			p0.k[0] += t*(p0.k[0] - p1.k[0]);
			p0.k[1] = m_E.k[1];
			p0.k[2] += t*(p0.k[2] - p1.k[2]);
		}
	}

	if (p0.k[2] < p1.k[2])
	{
		if (p0.k[2] > m_E.k[2]) return false;
		if (p1.k[2] < -m_E.k[2]) return false;

		if (p0.k[2] < -m_E.k[2])
		{
			fp32 t = (-m_E.k[2] - p0.k[2]) / (p1.k[2] - p0.k[2]);
			p0.k[0] += t*(p1.k[0] - p0.k[0]);
			p0.k[1] += t*(p1.k[1] - p0.k[1]);
			p0.k[2] = -m_E.k[2];
		}
		if (p1.k[2] > m_E.k[2])
		{
			fp32 t = -(p1.k[2] - m_E.k[2]) / (p1.k[2] - p0.k[2]);
			p1.k[0] += t*(p1.k[0] - p0.k[0]);
			p1.k[1] += t*(p1.k[1] - p0.k[1]);
			p1.k[2] = m_E.k[2];
		}
	}
	else
	{
		if (p1.k[2] > m_E.k[2]) return false;
		if (p0.k[2] < -m_E.k[2]) return false;

		if (p1.k[2] < -m_E.k[2])
		{
			fp32 t = (-m_E.k[2] - p1.k[2]) / (p0.k[2] - p1.k[2]);
			p1.k[0] += t*(p0.k[0] - p1.k[0]);
			p1.k[1] += t*(p0.k[1] - p1.k[1]);
			p1.k[2] = -m_E.k[2];
		}
		if (p0.k[2] > m_E.k[2])
		{
			fp32 t = -(p0.k[2] - m_E.k[2]) / (p0.k[2] - p1.k[2]);
			p0.k[0] += t*(p0.k[0] - p1.k[0]);
			p0.k[1] += t*(p0.k[1] - p1.k[1]);
			p0.k[2] = m_E.k[2];
		}
	}

	_HitPos = p0;
	return true;
}

void CPhysOBB::GetPlaneFromLocalPoint(const CVec3Dfp32& _p0, CPlane3Dfp32& _Plane) const
{
	MAUTOSTRIP(CPhysOBB_GetPlaneFromLocalPoint, MAUTOSTRIP_VOID);
	// _p0 == Point in box-space, assumed to be somewhat close to the box's surface.
	// _Plane == Result plane in world-coordinates.

	CVec3Dfp32 N(0);
	_Plane.d = 0;

	int i;
	for(i = 0; i < 3; i++)
	{
		if (M_Fabs(_p0[i]+m_E[i]) < 0.001f)
		{
			N[i] = -1.0f;
			_Plane.d = m_E[i];
			break;
		}
		if (M_Fabs(_p0[i]-m_E[i]) < 0.001f)
		{
			N[i] = 1.0f;
			_Plane.d = -m_E[i];
			break;
		}
	}
	if (i == 3)
	{
		ConOut("(CPhysOBB::GetPlaneFromLocalPoint) Invalid point.");
		N[0] = 1;
		_Plane.d = 1;
	}

	_Plane.n[0] = N[0]*m_A[0][0] + N[1]*m_A[1][0] + N[2]*m_A[2][0];
	_Plane.n[1] = N[0]*m_A[0][1] + N[1]*m_A[1][1] + N[2]*m_A[2][1];
	_Plane.n[2] = N[0]*m_A[0][2] + N[1]*m_A[1][2] + N[2]*m_A[2][2];
	_Plane.Translate(m_C);
}

//---------------------------------------------------------------------------
enum
{
    INTERSECTION,
    AXIS_A0, AXIS_A1, AXIS_A2,
    AXIS_B0, AXIS_B1, AXIS_B2,
    AXIS_A0xB0, AXIS_A0xB1, AXIS_A0xB2,
    AXIS_A1xB0, AXIS_A1xB1, AXIS_A1xB2,
    AXIS_A2xB0, AXIS_A2xB1, AXIS_A2xB2
};

//---------------------------------------------------------------------------
#define FABS(x) (float(M_Fabs(x)))
//---------------------------------------------------------------------------
unsigned int Phys_IntersectBoxes(const CPhysOBB& _Box0, const CPhysOBB& _Box1);
unsigned int Phys_IntersectBoxes(const CPhysOBB& _Box0, const CPhysOBB& _Box1)
{
	MAUTOSTRIP(Phys_IntersectBoxes, 0);
    CVec3Dfp32 D;         // center1 - center0
    float AB[3][3];   // matrix C = A^T B, c_{ij} = Dot(A_i,B_j)
    float fAB[3][3];  // fabs(c_{ij})
    float AD[3];      // Dot(A_i,D)
    float BD[3];      // Dot(B_i,D)
    float R0, R1, R;  // interval radii and distance between centers
    float R01;       // R0+R1
    
    // compute difference of box centers
    DIFF(D, _Box1.m_C, _Box0.m_C);

    // axis C0+t*A0
    AB[0][0] = DOT(_Box0.m_A[0], _Box1.m_A[0]);
    AB[0][1] = DOT(_Box0.m_A[0], _Box1.m_A[1]);
    AB[0][2] = DOT(_Box0.m_A[0], _Box1.m_A[2]);
    AD[0] = DOT(_Box0.m_A[0],D);
    fAB[0][0] = FABS(AB[0][0]);
    fAB[0][1] = FABS(AB[0][1]);
    fAB[0][2] = FABS(AB[0][2]);
    R1 = _Box1.m_E[0]*fAB[0][0] + _Box1.m_E[1]*fAB[0][1] + _Box1.m_E[2]*fAB[0][2];
    R01 = _Box0.m_E[0] + R1;
    if ( FABS(AD[0]) > R01 )
        return AXIS_A0;

    // axis C0+t*A1
    AB[1][0] = DOT(_Box0.m_A[1], _Box1.m_A[0]);
    AB[1][1] = DOT(_Box0.m_A[1], _Box1.m_A[1]);
    AB[1][2] = DOT(_Box0.m_A[1], _Box1.m_A[2]);
    AD[1] = DOT(_Box0.m_A[1],D);
    fAB[1][0] = FABS(AB[1][0]);
    fAB[1][1] = FABS(AB[1][1]);
    fAB[1][2] = FABS(AB[1][2]);
    R1 = _Box1.m_E[0]*fAB[1][0] + _Box1.m_E[1]*fAB[1][1] + _Box1.m_E[2]*fAB[1][2];
    R01 = _Box0.m_E[1] + R1;
    if ( FABS(AD[1]) > R01 )
        return AXIS_A1;

    // axis C0+t*A2
    AB[2][0] = DOT(_Box0.m_A[2], _Box1.m_A[0]);
    AB[2][1] = DOT(_Box0.m_A[2], _Box1.m_A[1]);
    AB[2][2] = DOT(_Box0.m_A[2], _Box1.m_A[2]);
    AD[2] = DOT(_Box0.m_A[2],D);
    fAB[2][0] = FABS(AB[2][0]);
    fAB[2][1] = FABS(AB[2][1]);
    fAB[2][2] = FABS(AB[2][2]);
    R1 = _Box1.m_E[0]*fAB[2][0] + _Box1.m_E[1]*fAB[2][1] + _Box1.m_E[2]*fAB[2][2];
    R01 = _Box0.m_E[2] + R1;
    if ( FABS(AD[2]) > R01 )
        return AXIS_A2;

    // axis C0+t*B0
    BD[0] = DOT(_Box1.m_A[0],D);
    R0 = _Box0.m_E[0]*fAB[0][0] + _Box0.m_E[1]*fAB[1][0] + _Box0.m_E[2]*fAB[2][0];
    R01 = R0 + _Box1.m_E[0];
    if ( FABS(BD[0]) > R01 )
         return AXIS_B0;

    // axis C0+t*B1
    BD[1] = DOT(_Box1.m_A[1],D);
    R0 = _Box0.m_E[0]*fAB[0][1] + _Box0.m_E[1]*fAB[1][1] + _Box0.m_E[2]*fAB[2][1];
    R01 = R0 + _Box1.m_E[1];
    if ( FABS(BD[1]) > R01 )
         return AXIS_B1;

    // axis C0+t*B2
    BD[2] = DOT(_Box1.m_A[2],D);
    R0 = _Box0.m_E[0]*fAB[0][2] + _Box0.m_E[1]*fAB[1][2] + _Box0.m_E[2]*fAB[2][2];
    R01 = R0 + _Box1.m_E[2];
    if ( FABS(BD[2]) > R01 )
         return AXIS_B2;

    // axis C0+t*A0xB0
    R0 = _Box0.m_E[1]*fAB[2][0] + _Box0.m_E[2]*fAB[1][0];
    R1 = _Box1.m_E[1]*fAB[0][2] + _Box1.m_E[2]*fAB[0][1];
    R = FABS(AD[2]*AB[1][0]-AD[1]*AB[2][0]);
    R01 = R0 + R1;
    if ( R > R01 )
         return AXIS_A0xB0;

    // axis C0+t*A0xB1
    R0 = _Box0.m_E[1]*fAB[2][1] + _Box0.m_E[2]*fAB[1][1];
    R1 = _Box1.m_E[0]*fAB[0][2] + _Box1.m_E[2]*fAB[0][0];
    R = FABS(AD[2]*AB[1][1]-AD[1]*AB[2][1]);
    R01 = R0 + R1;
    if ( R > R01 )
         return AXIS_A0xB1;

    // axis C0+t*A0xB2
    R0 = _Box0.m_E[1]*fAB[2][2] + _Box0.m_E[2]*fAB[1][2];
    R1 = _Box1.m_E[0]*fAB[0][1] + _Box1.m_E[1]*fAB[0][0];
    R = FABS(AD[2]*AB[1][2]-AD[1]*AB[2][2]);
    R01 = R0 + R1;
    if ( R > R01 )
         return AXIS_A0xB2;

    // axis C0+t*A1xB0
    R0 = _Box0.m_E[0]*fAB[2][0] + _Box0.m_E[2]*fAB[0][0];
    R1 = _Box1.m_E[1]*fAB[1][2] + _Box1.m_E[2]*fAB[1][1];
    R = FABS(AD[0]*AB[2][0]-AD[2]*AB[0][0]);
    R01 = R0 + R1;
    if ( R > R01 )
        return AXIS_A1xB0;

    // axis C0+t*A1xB1
    R0 = _Box0.m_E[0]*fAB[2][1] + _Box0.m_E[2]*fAB[0][1];
    R1 = _Box1.m_E[0]*fAB[1][2] + _Box1.m_E[2]*fAB[1][0];
    R = FABS(AD[0]*AB[2][1]-AD[2]*AB[0][1]);
    R01 = R0 + R1;
    if ( R > R01 )
         return AXIS_A1xB1;

    // axis C0+t*A1xB2
    R0 = _Box0.m_E[0]*fAB[2][2] + _Box0.m_E[2]*fAB[0][2];
    R1 = _Box1.m_E[0]*fAB[1][1] + _Box1.m_E[1]*fAB[1][0];
    R = FABS(AD[0]*AB[2][2]-AD[2]*AB[0][2]);
    R01 = R0 + R1;
    if ( R > R01 )
         return AXIS_A1xB2;

    // axis C0+t*A2xB0
    R0 = _Box0.m_E[0]*fAB[1][0] + _Box0.m_E[1]*fAB[0][0];
    R1 = _Box1.m_E[1]*fAB[2][2] + _Box1.m_E[2]*fAB[2][1];
    R = FABS(AD[1]*AB[0][0]-AD[0]*AB[1][0]);
    R01 = R0 + R1;
    if ( R > R01 )
         return AXIS_A2xB0;

    // axis C0+t*A2xB1
    R0 = _Box0.m_E[0]*fAB[1][1] + _Box0.m_E[1]*fAB[0][1];
    R1 = _Box1.m_E[0]*fAB[2][2] + _Box1.m_E[2]*fAB[2][0];
    R = FABS(AD[1]*AB[0][1]-AD[0]*AB[1][1]);
    R01 = R0 + R1;
    if ( R > R01 )
        return AXIS_A2xB1;

    // axis C0+t*A2xB2
    R0 = _Box0.m_E[0]*fAB[1][2] + _Box0.m_E[1]*fAB[0][2];
    R1 = _Box1.m_E[0]*fAB[2][1] + _Box1.m_E[1]*fAB[2][0];
    R = FABS(AD[1]*AB[0][2]-AD[0]*AB[1][2]);
    R01 = R0 + R1;
    if ( R > R01 )
         return AXIS_A2xB2;

    // intersection occurs
    return INTERSECTION;
}

//---------------------------------------------------------------------------
static void Phys_CreatePlaneOnBox(const CVec3Dfp32& _N, const CPhysOBB& _OBB, CPlane3Dfp32& _Plane)
{
	MAUTOSTRIP(Phys_CreatePlaneOnBox, MAUTOSTRIP_VOID);
	_Plane.n = _N;
	_Plane.d = 0;

	fp32 MaxD = _OBB.MaxDistance(_Plane);
	_Plane.d = -MaxD;
}

#if 0
static CStr Phys_GetPlane(int _Intersection)
{
	MAUTOSTRIP(Phys_GetPlane, CStr());
	switch(_Intersection)
	{
	case INTERSECTION :
		return "Intersection";
	case AXIS_A0 : 
	case AXIS_A1 : 
	case AXIS_A2 : 
		return CStrF("AxisA %d", _Intersection - AXIS_A0);
	case AXIS_B0 : 
	case AXIS_B1 : 
	case AXIS_B2 : 
		return CStrF("AxisB %d", _Intersection - AXIS_B0);
	default:
		return CStrF("Edge %d", _Intersection - AXIS_A0xB0);
	} 
}
#endif

/*    AXIS_A0, AXIS_A1, AXIS_A2,
    AXIS_B0, AXIS_B1, AXIS_B2,
    AXIS_A0xB0, AXIS_A0xB1, AXIS_A0xB2,
    AXIS_A1xB0, AXIS_A1xB1, AXIS_A1xB2,
    AXIS_A2xB0, AXIS_A2xB1, AXIS_A2xB2*/
#if 0
static void Phys_CalcIntersectionPoint(const CPhysOBB& _OBB, const CPlane3Dfp32& _Plane, CVec3Dfp32& _RetPos)
{
	MAUTOSTRIP(Phys_CalcIntersectionPoint, MAUTOSTRIP_VOID);
	CVec3Dfp32 v[8];
	_OBB.GetVertices(v);

	_RetPos = 0;
	int ni = 0;
	for(int i = 0; i < 8; i++)
	{
		fp32 d = _Plane.Distance(v[i]);
		if (d < 0.0f)
		{
			_RetPos += v[i];
			_RetPos.Combine(_Plane.n, -d, _RetPos);
			ni++;
		}
	}
	if (!ni)
	{
		ConOut("(Phys_CalcIntersectionPoint) No point on backside of plane.");
		return;
	}
	_RetPos *= 1.0f / fp32(ni);
}
#endif

//---------------------------------------------------------------------------
bool Phys_GetSeparatingDistance(const CPhysOBB& _Box, const CPhysOBB& _Box2, const CVec3Dfp32& _V, const CVec3Dfp32& _N, fp32& _BestD, CPlane3Dfp32& _BestPlane, bool _bNormalize = false);
bool Phys_GetSeparatingDistance(const CPhysOBB& _Box, const CPhysOBB& _Box2, const CVec3Dfp32& _V, const CVec3Dfp32& _N, fp32& _BestD, CPlane3Dfp32& _BestPlane, bool _bNormalize)
{
	MAUTOSTRIP(Phys_GetSeparatingDistance, false);
	const CVec3Dfp32* pN = &_N;
	CVec3Dfp32 N2;

	if (_N.LengthSqr() < 0.001f) return false;
	if (_bNormalize)
	{
		N2 = _N;
		N2.Normalize();
		pN = &N2;
	}

	bool bModifed = false;
	fp32 Dot = _V * (*pN);

	if (Dot < 0.0f)
	{
		CPlane3Dfp32 P;
		Phys_CreatePlaneOnBox(*pN, _Box2, P);

		fp32 d = _Box.MinDistance(P);

		if (d > _BestD)
		{
			_BestD = d;
			_BestPlane = P;
			bModifed = true;
		}
	}
	else
	{
		CPlane3Dfp32 P;
		Phys_CreatePlaneOnBox(-(*pN), _Box2, P);

		fp32 d = _Box.MinDistance(P);

		if (d > _BestD)
		{
			_BestD = d;
			_BestPlane = P;
			bModifed = true;
		}
	}
	return bModifed;
}

//---------------------------------------------------------------------------

bool Phys_Intersect_OBB(const CPhysOBB& _Box, const CPhysOBB& _BoxStart, const CPhysOBB& _BoxDest, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(Phys_Intersect_OBB, false);

//	bool bLog = false;

	if (!_pCollisionInfo)
	{
		return (Phys_IntersectBoxes(_Box, _BoxDest) == INTERSECTION);
	}
	else
	{
		int IntersectDest = Phys_IntersectBoxes(_Box, _BoxDest);
		if (IntersectDest != INTERSECTION)
		{
			return false;
		}

		CVec3Dfp32 dV;
		_BoxDest.m_C.Sub(_BoxStart.m_C, dV);

		fp32 BestD = -100001.0f;
		int PlaneNr = -1;

		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, _BoxDest.m_A[0], BestD, _pCollisionInfo->m_Plane)) PlaneNr = 2;
		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, _BoxDest.m_A[1], BestD, _pCollisionInfo->m_Plane)) PlaneNr = 3;
		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, _BoxDest.m_A[2], BestD, _pCollisionInfo->m_Plane)) PlaneNr = 4;
		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, _Box.m_A[0], BestD, _pCollisionInfo->m_Plane)) PlaneNr = 5;
		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, _Box.m_A[1], BestD, _pCollisionInfo->m_Plane)) PlaneNr = 6;
		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, _Box.m_A[2], BestD, _pCollisionInfo->m_Plane)) PlaneNr = 7;

		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, (_BoxDest.m_A[0] / _Box.m_A[0]), BestD, _pCollisionInfo->m_Plane, true)) PlaneNr = 10;
		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, (_BoxDest.m_A[0] / _Box.m_A[1]), BestD, _pCollisionInfo->m_Plane, true)) PlaneNr = 11;
		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, (_BoxDest.m_A[0] / _Box.m_A[2]), BestD, _pCollisionInfo->m_Plane, true)) PlaneNr = 12;
		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, (_BoxDest.m_A[1] / _Box.m_A[0]), BestD, _pCollisionInfo->m_Plane, true)) PlaneNr = 20;
		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, (_BoxDest.m_A[1] / _Box.m_A[1]), BestD, _pCollisionInfo->m_Plane, true)) PlaneNr = 21;
		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, (_BoxDest.m_A[1] / _Box.m_A[2]), BestD, _pCollisionInfo->m_Plane, true)) PlaneNr = 22;
		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, (_BoxDest.m_A[2] / _Box.m_A[0]), BestD, _pCollisionInfo->m_Plane, true)) PlaneNr = 30;
		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, (_BoxDest.m_A[2] / _Box.m_A[1]), BestD, _pCollisionInfo->m_Plane, true)) PlaneNr = 31;
		if (Phys_GetSeparatingDistance(_BoxDest, _Box, dV, (_BoxDest.m_A[2] / _Box.m_A[2]), BestD, _pCollisionInfo->m_Plane, true)) PlaneNr = 32;

		_pCollisionInfo->m_Distance = _BoxDest.MinDistance(_pCollisionInfo->m_Plane) - 0.001f;
		_pCollisionInfo->m_bIsValid = PlaneNr >= 0;
		_pCollisionInfo->m_bIsCollision = true;
		_pCollisionInfo->m_Pos = _BoxDest.m_C - _pCollisionInfo->m_Plane.n*_pCollisionInfo->m_Plane.Distance(_BoxDest.m_C);
		_pCollisionInfo->m_LocalPos = _BoxDest.m_C - _pCollisionInfo->m_Plane.n*_pCollisionInfo->m_Plane.Distance(_BoxDest.m_C);
		_pCollisionInfo->m_Velocity = 0;

		fp32 vProj = _pCollisionInfo->m_Plane.n * dV;
		if (vProj < -0.001f)
			_pCollisionInfo->m_Time = Clamp01(1.0f - _pCollisionInfo->m_Distance / vProj);
		else
		{
			_pCollisionInfo->m_Time = 1;
		}

//		fp32 BoxDim = Max3(_BoxDest.m_E[0], _BoxDest.m_E[1], _BoxDest.m_E[2]);

		return true;
	}
}

