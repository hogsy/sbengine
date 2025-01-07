
#include "PCH.h"

#include "WPhys.h"
#include "../XRClass.h"
#include "MFloat.h"

#define V3_ADD(a, b, c) { c[0] = a[0] + b[0]; c[1] = a[1] + b[1]; c[2] = a[2] + b[2]; }
#define V3_SUB(a, b, c) { c[0] = a[0] - b[0]; c[1] = a[1] - b[1]; c[2] = a[2] - b[2]; }
#define V3_DOTPROD(a, b) (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])
#define V3_CROSSPROD(p, q, cross) { cross[0] = p[1]*q[2]-p[2]*q[1]; cross[1] = p[2]*q[0]-p[0]*q[2]; cross[2] = p[0]*q[1]-p[1]*q[0]; }

#ifdef PLATFORM_PS2

#undef V3_DOTPROD
#undef V3_CROSSPROD
#define V3_CROSSPROD( a, b, dest ) _CrossProd( a.k[0], a.k[1], a.k[2], b.k[0], b.k[1], b.k[2], &dest )
#define V3_DOTPROD( a, b ) _DotProd( a.k[0], a.k[1], a.k[2], b.k[0], b.k[1], b.k[2] )
#define DIFF( dest, a, b ) _Diff( a.k[0], a.k[1], a.k[2], b.k[0], b.k[1], b.k[2], &dest )

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

static inline void _Diff( fp32 _a0, fp32 _a1, fp32 _a2, fp32 _b0, fp32 _b1, fp32 _b2, CVec3Dfp32* _pDest )
{
	fp32 val0, val1, val2;
	asm("
		sub.s	val0, _a0, _b0
		sub.s	val1, _a1, _b1
		sub.s	val2, _a2, _b2
		");
	_pDest->k[0]	= val0;
	_pDest->k[1]	= val1;
	_pDest->k[2]	= val2;
}

#endif

enum
{
    INTERSECTION,
    AXIS_N,
    AXIS_A0, AXIS_A1, AXIS_A2,
    AXIS_A0xE0, AXIS_A0xE1, AXIS_A0xE2,
    AXIS_A1xE0, AXIS_A1xE1, AXIS_A1xE2,
    AXIS_A2xE0, AXIS_A2xE1, AXIS_A2xE2
};

unsigned int TestIntersectionS (const CVec3Dfp32 tri[3], const CPhysOBB& _Box);
unsigned int TestIntersectionV (float dt, const CVec3Dfp32 tri[3],
                                const CVec3Dfp32& triVel, const CPhysOBB& _Box, const CVec3Dfp32& boxVel);
void Phys_CreatePlaneOnTriangle(const CVec3Dfp32& _N, const CVec3Dfp32 tri[3], CPlane3Dfp32& _Plane);
fp32 Phys_GetMinTriangleDistance(const CVec3Dfp32 tri[3], const CPlane3Dfp32& _Plane, int& _VertexNr);
CStr Phys_GetPlane(int _Intersection);
void Phys_CalcIntersectionPoint(const CPhysOBB& _OBB, const CPlane3Dfp32& _Plane, CVec3Dfp32& _RetPos);
void Phys_GetSeparatingDistance(const CPhysOBB& _Box, const CVec3Dfp32 tri[3], const CVec3Dfp32& _V, const CVec3Dfp32& _N, fp32& _BestD, CPlane3Dfp32& _BestPlane, bool _bNormalize = false);
void Phys_GetSeparatingDistance_NoCheck(const CPhysOBB& _Box, const CVec3Dfp32 tri[3], const CVec3Dfp32& _V, const CVec3Dfp32& _N, fp32& _BestD, CPlane3Dfp32& _BestPlane);



//---------------------------------------------------------------------------
#define FABS(x) (float(M_Fabs(x)))
//---------------------------------------------------------------------------
// compare [-r,r] to [min{p,p+d0,p+d1},max{p,p+d0,p+d1}]
#define TESTS1(p,d0,d1,r,axis) \
{ \
    if ( (p) > (r) ) \
    { \
        if ( (d0) >= 0.0f ) \
        { \
            if ( (p)+(d1) > (r) ) \
                return axis; \
        } \
        else if ( (d1) <= (d0) ) \
        { \
            if ( (p)+(d1) > (r) ) \
                return axis; \
        } \
        else \
        { \
            if ( (p)+(d0) > (r) ) \
                return axis; \
        } \
    } \
    else if ( (p) < -(r) ) \
    { \
        if ( (d0) <= 0.0f ) \
        { \
            if ( (p)+(d1) < -(r) ) \
                return axis; \
        } \
        else if ( (d1) >= (d0) ) \
        { \
            if ( (p)+(d1) < -(r) ) \
                return axis; \
        } \
        else \
        { \
            if ( (p)+(d0) < -(r) ) \
                return axis; \
        } \
    } \
}
//---------------------------------------------------------------------------
// compare [-r,r] to [min{p,p+d},max{p,p+d}]
#define TESTS2(p,d,r,axis) \
{ \
    if ( (p) > (r) ) \
    { \
        if ( (p)+(d) > (r) ) \
            return axis; \
    } \
    else if ( (p) < -(r) ) \
    { \
        if ( (p)+(d) < -(r) ) \
            return axis; \
    } \
}
//---------------------------------------------------------------------------
unsigned int TestIntersectionS (const CVec3Dfp32 tri[3], const CPhysOBB& _Box)
{
	MAUTOSTRIP(TestIntersectionS, 0);
    // construct triangle normal, difference of center and vertex
    CVec3Dfp32 D, E[3], N;
    DIFF(E[0],tri[1],tri[0]);
    DIFF(E[1],tri[2],tri[0]);
	DIFF(E[2],tri[2],tri[1]);
    V3_CROSSPROD(E[0],E[1],N);
    DIFF(D,tri[0],_Box.m_C);

    // axis C+t*N
    float A0dN = V3_DOTPROD(_Box.m_A[0],N);
    float A1dN = V3_DOTPROD(_Box.m_A[1],N);
    float A2dN = V3_DOTPROD(_Box.m_A[2],N);
    float R = FABS(_Box.m_E[0]*A0dN)+FABS(_Box.m_E[1]*A1dN)+FABS(_Box.m_E[2]*A2dN);
    float NdD = V3_DOTPROD(N,D);
    if ( FABS(NdD) > R )
        return AXIS_N;

    // axis C+t*A0
    float A0dD = V3_DOTPROD(_Box.m_A[0],D);
    float A0dE0 = V3_DOTPROD(_Box.m_A[0],E[0]);
    float A0dE1 = V3_DOTPROD(_Box.m_A[0],E[1]);
	float A0dE2 = V3_DOTPROD(_Box.m_A[0],E[2]);
    TESTS1(A0dD,A0dE0,A0dE1,_Box.m_E[0],AXIS_A0);

    // axis C+t*A1
    float A1dD = V3_DOTPROD(_Box.m_A[1],D);
    float A1dE0 = V3_DOTPROD(_Box.m_A[1],E[0]);
    float A1dE1 = V3_DOTPROD(_Box.m_A[1],E[1]);
	float A1dE2 = V3_DOTPROD(_Box.m_A[1],E[2]);
    TESTS1(A1dD,A1dE0,A1dE1,_Box.m_E[1],AXIS_A1);

    // axis C+t*A2
    float A2dD = V3_DOTPROD(_Box.m_A[2],D);
    float A2dE0 = V3_DOTPROD(_Box.m_A[2],E[0]);
    float A2dE1 = V3_DOTPROD(_Box.m_A[2],E[1]);
	float A2dE2 = V3_DOTPROD(_Box.m_A[2],E[2]);
    TESTS1(A2dD,A2dE0,A2dE1,_Box.m_E[2],AXIS_A2);

    // axis C+t*A0xE0
    CVec3Dfp32 A0xE0;
    V3_CROSSPROD(_Box.m_A[0],E[0],A0xE0);
    float A0xE0dD = V3_DOTPROD(A0xE0,D);
	if (M_Fabs(A0xE0dD) > 0.0001f)
	{
	    R = FABS(_Box.m_E[1]*A2dE0)+FABS(_Box.m_E[2]*A1dE0);
	    TESTS2(A0xE0dD,A0dN,R,AXIS_A0xE0);
	}

    // axis C+t*A0xE1
    CVec3Dfp32 A0xE1;
    V3_CROSSPROD(_Box.m_A[0],E[1],A0xE1);
    float A0xE1dD = V3_DOTPROD(A0xE1,D);
	if (M_Fabs(A0xE1dD) > 0.0001f)
	{
		R = FABS(_Box.m_E[1]*A2dE1)+FABS(_Box.m_E[2]*A1dE1);
		TESTS2(A0xE1dD,-A0dN,R,AXIS_A0xE1);
	}
	// axis C+t*A0xE2
	//    float A1dE2 = A1dE1-A1dE0;
	//   float A2dE2 = A2dE1-A2dE0;


	//	CVec3Dfp32 E2;
	//	DIFF(E2,tri[2],tri[1]);
	//	float A1dE2 = V3_DOTPROD(_Box.m_A[1], E2);
	//	float A2dE2 = V3_DOTPROD(_Box.m_A[2], E2);
	//	CVec3Dfp32 A0xE2;
	//	CROSS(A0xE2,_Box.m_A[0],E2);
	//	float A0xE2dD = V3_DOTPROD(A0xE2,D);
	//	R = FABS(_Box.m_E[1]*A2dE2)+FABS(_Box.m_E[2]*A1dE2);
	//	TESTS2(A0xE2dD,A0dN,R,AXIS_A0xE2);

	//float A1dE2 = A1dE1-A1dE0;
	//float A2dE2 = A2dE1-A2dE0;
	//float A0xE2dD = A0xE1dD-A0xE0dD;
	CVec3Dfp32 A0xE2;
	V3_CROSSPROD(_Box.m_A[0],E[2],A0xE2);
	float A0xE2dD = V3_DOTPROD(A0xE2,D);
	if (M_Fabs(A0xE2dD) > 0.0001f)
	{
		R = FABS(_Box.m_E[1]*A2dE2)+FABS(_Box.m_E[2]*A1dE2);
		TESTS2(A0xE2dD,-A0dN,R,AXIS_A0xE2);
	}

	// axis C+t*A1xE0
	CVec3Dfp32 A1xE0;
	V3_CROSSPROD(_Box.m_A[1],E[0],A1xE0);
	float A1xE0dD = V3_DOTPROD(A1xE0,D);
	if (M_Fabs(A1xE0dD) > 0.0001f)
	{
		R = FABS(_Box.m_E[0]*A2dE0)+FABS(_Box.m_E[2]*A0dE0);
		TESTS2(A1xE0dD,A1dN,R,AXIS_A1xE0);
	}

	// axis C+t*A1xE1
	CVec3Dfp32 A1xE1;
	V3_CROSSPROD(_Box.m_A[1],E[1],A1xE1);
	float A1xE1dD = V3_DOTPROD(A1xE1,D);
	if (M_Fabs(A1xE1dD) > 0.0001f)
	{
		R = FABS(_Box.m_E[0]*A2dE1)+FABS(_Box.m_E[2]*A0dE1);
		TESTS2(A1xE1dD,-A1dN,R,AXIS_A1xE1);
	}

	// axis C+t*A1xE2

	//	CVec3Dfp32 A1xE2;
	//	CROSS(A1xE2,_Box.m_A[1],E2);
	//	float A0dE2 = A0dE1-A0dE0;
	//	float A1xE2dD = V3_DOTPROD(A1xE2,D);
	//	R = FABS(_Box.m_E[0]*A2dE2)+FABS(_Box.m_E[2]*A0dE2);
	//	TESTS2(A1xE2dD,-A1dN,R,AXIS_A1xE2);

	//float A0dE2 = A0dE1-A0dE0;
	//float A1xE2dD = A1xE1dD-A1xE0dD;
	CVec3Dfp32 A1xE2;
	V3_CROSSPROD(_Box.m_A[1],E[2],A1xE2);
	float A1xE2dD = V3_DOTPROD(A1xE2,D);
	if (M_Fabs(A1xE2dD) > 0.0001f)
	{
		R = FABS(_Box.m_E[0]*A2dE2)+FABS(_Box.m_E[2]*A0dE2);
		TESTS2(A1xE2dD,-A1dN,R,AXIS_A1xE2);
	}

	// axis C+t*A2xE0
	CVec3Dfp32 A2xE0;
	V3_CROSSPROD(_Box.m_A[2],E[0],A2xE0);
	float A2xE0dD = V3_DOTPROD(A2xE0,D);
	if (M_Fabs(A2xE0dD) > 0.0001f)
	{
		R = FABS(_Box.m_E[0]*A1dE0)+FABS(_Box.m_E[1]*A0dE0);
		TESTS2(A2xE0dD,A2dN,R,AXIS_A2xE0);
	}
	// axis C+t*A2xE1
	CVec3Dfp32 A2xE1;
	V3_CROSSPROD(_Box.m_A[2],E[1],A2xE1);
	float A2xE1dD = V3_DOTPROD(A2xE1,D);
	if (M_Fabs(A2xE1dD) > 0.0001f)
	{
		R = FABS(_Box.m_E[0]*A1dE1)+FABS(_Box.m_E[1]*A0dE1);
		TESTS2(A2xE1dD,-A2dN,R,AXIS_A2xE1);
	}

	// axis C+t*A2xE2

	//	CVec3Dfp32 A2xE2;
	//	CROSS(A2xE2,_Box.m_A[2],E2);
	//	float A2xE2dD = V3_DOTPROD(A2xE2,D);
	//	R = FABS(_Box.m_E[0]*A1dE2)+FABS(_Box.m_E[1]*A0dE2);
	//	TESTS2(A2xE2dD,-A2dN,R,AXIS_A2xE2);

	//float A2xE2dD = A2xE1dD-A2xE0dD;
	CVec3Dfp32 A2xE2;
	V3_CROSSPROD(_Box.m_A[2],E[2],A2xE2);
	float A2xE2dD = V3_DOTPROD(A2xE2,D);
	if (M_Fabs(A2xE2dD) > 0.0001f)
	{
		R = FABS(_Box.m_E[0]*A1dE2)+FABS(_Box.m_E[1]*A0dE2);
		TESTS2(A2xE2dD,-A2dN,R,AXIS_A2xE2);
	}

    // intersection occurs
    return INTERSECTION;
}
//---------------------------------------------------------------------------
// compare [-r,r] to [NdD+dt*NdW]
#define TESTV0(NdD,R,dt,N,W,axis) \
{ \
    if ( NdD > R ) \
    { \
        if ( NdD+dt*V3_DOTPROD(N,W) > R ) \
            return axis; \
    } \
    else if ( NdD < -R ) \
    { \
        if ( NdD+dt*V3_DOTPROD(N,W) < -R ) \
            return axis; \
    } \
}
//---------------------------------------------------------------------------
// compare [-r,r] to [min{p,p+d0,p+d1},max{p,p+d0,p+d1}]+t*w where t >= 0
#define TESTV1(p,t,w,d0,d1,r,axis) \
{ \
    if ( (p) > (r) ) \
    { \
        float min; \
        if ( (d0) >= 0.0f ) \
        { \
            if ( (d1) >= 0.0f ) \
            { \
                if ( (p)+(t)*(w) > (r) ) \
                    return axis; \
            } \
            else \
            { \
                min = (p)+(d1); \
                if ( min > (r) && min+(t)*(w) > (r) ) \
                    return axis; \
            } \
        } \
        else if ( (d1) <= (d0) ) \
        { \
            min = (p)+(d1); \
            if ( min > (r) && min+(t)*(w) > (r) ) \
                return axis; \
        } \
        else \
        { \
            min = (p)+(d0); \
            if ( min > (r) && min+(t)*(w) > (r) ) \
                return axis; \
        } \
    } \
    else if ( (p) < -(r) ) \
    { \
        float max; \
        if ( (d0) <= 0.0f ) \
        { \
            if ( (d1) <= 0.0f ) \
            { \
                if ( (p)+(t)*(w) < -(r) ) \
                    return axis; \
            } \
            else \
            { \
                max = (p)+(d1); \
                if ( max < -(r) && max+(t)*(w) < -(r) ) \
                    return axis; \
            } \
        } \
        else if ( (d1) >= (d0) ) \
        { \
            max = (p)+(d1); \
            if ( max < -(r) && max+(t)*(w) < -(r) ) \
                return axis; \
        } \
        else \
        { \
            max = (p)+(d0); \
            if ( max < -(r) && max+(t)*(w) < -(r) ) \
                return axis; \
        } \
    } \
}
//---------------------------------------------------------------------------
// compare [-r,r] to [min{p,p+d},max{p,p+d}]+t*w where t >= 0
#define TESTV2(p,t,w,d,r,axis) \
{ \
    if ( (p) > (r) ) \
    { \
        if ( (d) >= 0.0f ) \
        { \
            if ( (p)+(t)*(w) > (r) ) \
                return axis; \
        } \
        else \
        { \
            float min = (p)+(d); \
            if ( min > (r) && min+(t)*(w) > (r) ) \
                return axis; \
        } \
    } \
    else if ( (p) < -(r) ) \
    { \
        if ( (d) <= 0.0f ) \
        { \
            if ( (p)+(t)*(w) < -(r) ) \
                return axis; \
        } \
        else \
        { \
            float max = (p)+(d); \
            if ( max < -(r) && max+(t)*(w) < -(r) ) \
                return axis; \
        } \
    } \
}
//---------------------------------------------------------------------------
unsigned int TestIntersectionV (float dt, const CVec3Dfp32 tri[3],
    const CVec3Dfp32& triVel, const CPhysOBB& _Box, const CVec3Dfp32& boxVel)
{
	MAUTOSTRIP(TestIntersectionV, 0);
    // Compute relative velocity of triangle with respect to box so that box
    // may as well be stationary.
    CVec3Dfp32 W;
    DIFF(W,triVel,boxVel);

    // construct triangle normal, difference of center and vertex (18 ops)
    CVec3Dfp32 D, E[2], N;
    DIFF(E[0],tri[1],tri[0]);
    DIFF(E[1],tri[2],tri[0]);
    V3_CROSSPROD(E[0],E[1],N);
    DIFF(D,tri[0],_Box.m_C);

    // axis C+t*N
    float A0dN = V3_DOTPROD(_Box.m_A[0],N);
    float A1dN = V3_DOTPROD(_Box.m_A[1],N);
    float A2dN = V3_DOTPROD(_Box.m_A[2],N);
    float R = FABS(_Box.m_E[0]*A0dN)+FABS(_Box.m_E[1]*A1dN)+FABS(_Box.m_E[2]*A2dN);
    float NdD = V3_DOTPROD(N,D);
    TESTV0(NdD,R,dt,N,W,AXIS_N);

    // axis C+t*A0
    float A0dD = V3_DOTPROD(_Box.m_A[0],D);
    float A0dE0 = V3_DOTPROD(_Box.m_A[0],E[0]);
    float A0dE1 = V3_DOTPROD(_Box.m_A[0],E[1]);
    float A0dW = V3_DOTPROD(_Box.m_A[0],W);
    TESTV1(A0dD,dt,A0dW,A0dE0,A0dE1,_Box.m_E[0],AXIS_A0);

    // axis C+t*A1
    float A1dD = V3_DOTPROD(_Box.m_A[1],D);
    float A1dE0 = V3_DOTPROD(_Box.m_A[1],E[0]);
    float A1dE1 = V3_DOTPROD(_Box.m_A[1],E[1]);
    float A1dW = V3_DOTPROD(_Box.m_A[1],W);
    TESTV1(A1dD,dt,A1dW,A1dE0,A1dE1,_Box.m_E[1],AXIS_A1);

    // axis C+t*A2
    float A2dD = V3_DOTPROD(_Box.m_A[2],D);
    float A2dE0 = V3_DOTPROD(_Box.m_A[2],E[0]);
    float A2dE1 = V3_DOTPROD(_Box.m_A[2],E[1]);
    float A2dW = V3_DOTPROD(_Box.m_A[2],W);
    TESTV1(A2dD,dt,A2dW,A2dE0,A2dE1,_Box.m_E[2],AXIS_A2);

    // axis C+t*A0xE0
    CVec3Dfp32 A0xE0;
    V3_CROSSPROD(_Box.m_A[0],E[0],A0xE0);
    float A0xE0dD = V3_DOTPROD(A0xE0,D);
    float A0xE0dW = V3_DOTPROD(A0xE0,W);
    R = FABS(_Box.m_E[1]*A2dE0)+FABS(_Box.m_E[2]*A1dE0);
    TESTV2(A0xE0dD,dt,A0xE0dW,A0dN,R,AXIS_A0xE0);

    // axis C+t*A0xE1
    CVec3Dfp32 A0xE1;
    V3_CROSSPROD(_Box.m_A[0],E[1],A0xE1);
    float A0xE1dD = V3_DOTPROD(A0xE1,D);
    float A0xE1dW = V3_DOTPROD(A0xE1,W);
    R = FABS(_Box.m_E[1]*A2dE1)+FABS(_Box.m_E[2]*A1dE1);
    TESTV2(A0xE1dD,dt,A0xE1dW,-A0dN,R,AXIS_A0xE1);

    // axis C+t*A0xE2
    float A1dE2 = A1dE1-A1dE0;
    float A2dE2 = A2dE1-A2dE0;
    float A0xE2dD = A0xE1dD-A0xE0dD;
    float A0xE2dW = A0xE1dW-A0xE0dW;
    R = FABS(_Box.m_E[1]*A2dE2)+FABS(_Box.m_E[2]*A1dE2);
    TESTV2(A0xE2dD,dt,A0xE2dW,-A0dN,R,AXIS_A0xE2);

    // axis C+t*A1xE0
    CVec3Dfp32 A1xE0;
    V3_CROSSPROD(_Box.m_A[1],E[0],A1xE0);
    float A1xE0dD = V3_DOTPROD(A1xE0,D);
    float A1xE0dW = V3_DOTPROD(A1xE0,W);
    R = FABS(_Box.m_E[0]*A2dE0)+FABS(_Box.m_E[2]*A0dE0);
    TESTV2(A1xE0dD,dt,A1xE0dW,A1dN,R,AXIS_A1xE0);

    // axis C+t*A1xE1
    CVec3Dfp32 A1xE1;
    V3_CROSSPROD(_Box.m_A[1],E[1],A1xE1);
    float A1xE1dD = V3_DOTPROD(A1xE1,D);
    float A1xE1dW = V3_DOTPROD(A1xE1,W);
    R = FABS(_Box.m_E[0]*A2dE1)+FABS(_Box.m_E[2]*A0dE1);
    TESTV2(A1xE1dD,dt,A1xE1dW,-A1dN,R,AXIS_A1xE1);

    // axis C+t*A1xE2
    float A0dE2 = A0dE1-A0dE0;
    float A1xE2dD = A1xE1dD-A1xE0dD;
    float A1xE2dW = A1xE1dW-A1xE0dW;
    R = FABS(_Box.m_E[0]*A2dE2)+FABS(_Box.m_E[2]*A0dE2);
    TESTV2(A1xE2dD,dt,A1xE2dW,-A1dN,R,AXIS_A1xE2);

    // axis C+t*A2xE0
    CVec3Dfp32 A2xE0;
    V3_CROSSPROD(_Box.m_A[2],E[0],A2xE0);
    float A2xE0dD = V3_DOTPROD(A2xE0,D);
    float A2xE0dW = V3_DOTPROD(A2xE0,W);
    R = FABS(_Box.m_E[0]*A1dE0)+FABS(_Box.m_E[1]*A0dE0);
    TESTV2(A2xE0dD,dt,A2xE0dW,A2dN,R,AXIS_A2xE0);

    // axis C+t*A2xE1
    CVec3Dfp32 A2xE1;
    V3_CROSSPROD(_Box.m_A[2],E[1],A2xE1);
    float A2xE1dD = V3_DOTPROD(A2xE1,D);
    float A2xE1dW = V3_DOTPROD(A2xE1,W);
    R = FABS(_Box.m_E[0]*A1dE1)+FABS(_Box.m_E[1]*A0dE1);
    TESTV2(A2xE1dD,dt,A2xE1dW,-A2dN,R,AXIS_A2xE1);

    // axis C+t*A2xE2
    float A2xE2dD = A2xE1dD-A2xE0dD;
    float A2xE2dW = A2xE1dW-A2xE0dW;
    R = FABS(_Box.m_E[0]*A1dE2)+FABS(_Box.m_E[1]*A0dE2);
    TESTV2(A2xE2dD,dt,A2xE2dW,-A2dN,R,AXIS_A2xE2);

    // intersection occurs
    return INTERSECTION;
}

//---------------------------------------------------------------------------
void Phys_CreatePlaneOnTriangle(const CVec3Dfp32& _N, const CVec3Dfp32 tri[3], CPlane3Dfp32& _Plane)
{
	MAUTOSTRIP(Phys_CreatePlaneOnTriangle, MAUTOSTRIP_VOID);
	fp32 d = V3_DOTPROD(_N, tri[0]);

	fp32 d2 = V3_DOTPROD(_N, tri[1]);
	if (d2 > d)
		d = d2;

	d2 = V3_DOTPROD(_N, tri[2]);
	if (d2 > d)
		d = d2;

	_Plane.CreateND(_N, -d);
}

fp32 Phys_GetMinTriangleDistance(const CVec3Dfp32 tri[3], const CPlane3Dfp32& _Plane, int& _VertexNr)
{
	MAUTOSTRIP(Phys_GetMinTriangleDistance, 0.0f);
	fp32 d1 = _Plane.Distance(tri[0]);
	fp32 d2 = _Plane.Distance(tri[1]);
	fp32 d3 = _Plane.Distance(tri[2]);

	_VertexNr = 0;
	if (d2 < d1)
	{
		_VertexNr = 1;
		d1 = d2;
	}
	if (d3 < d1)
	{
		_VertexNr = 2;
		d1 = d3;
	}

	return d1;
}

CStr Phys_GetPlane(int _Intersection)
{
	MAUTOSTRIP(Phys_GetPlane, CStr());
	switch(_Intersection)
	{
	case INTERSECTION :
		return "Intersection";
	case AXIS_N :
		return "Triangle";
	case AXIS_A0 : 
	case AXIS_A1 : 
	case AXIS_A2 : 
		return CStrF("Axis %d", _Intersection - AXIS_A0);
	default:
		return CStrF("Edge %d", _Intersection - AXIS_A0xE0);
	} 
}

void Phys_CalcIntersectionPoint(const CPhysOBB& _OBB, const CPlane3Dfp32& _Plane, CVec3Dfp32& _RetPos)
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

//#define PhysLog(s) ConOutL(s)
#define PhysLog(s) 

#define NEW_SEPARATING_PLANE


void Phys_GetSeparatingDistance(const CPhysOBB& _Box, const CVec3Dfp32 tri[3], const CVec3Dfp32& _V, const CVec3Dfp32& _N, fp32& _BestD, CPlane3Dfp32& _BestPlane, bool _bNormalize)
{
	MAUTOSTRIP(Phys_GetSeparatingDistance, false);
	const CVec3Dfp32* pN = &_N;
	CVec3Dfp32 N2;
	fp32 Len2 = V3_DOTPROD(_N, _N);

	if (Len2 < 0.001f) return;
	if (_bNormalize)
	{
		fp32 InvLen = M_InvSqrt(Len2);
		N2.k[0] = _N.k[0] * InvLen;
		N2.k[1] = _N.k[1] * InvLen;
		N2.k[2] = _N.k[2] * InvLen;
		pN = &N2;
	}

//	if (M_Fabs(1.0f - (*pN).Length()) > 0.01f)
//		ConOut("No unit normal: " + _N.GetString());

	fp32 Dot = _V * (*pN);
	if (M_Fabs(Dot) < 0.001f) return;

	if (Dot < 0.0f)
	{
		CPlane3Dfp32 P;
		Phys_CreatePlaneOnTriangle((*pN), tri, P);

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
		Phys_CreatePlaneOnTriangle(-(*pN), tri, P);

		fp32 d = _Box.MinDistance(P);
		if (d < -Dot - 0.001f) return;

		if (d > _BestD)
		{
			_BestD = d;
			_BestPlane = P;
		}
	}
}


void Phys_GetSeparatingDistance_NoCheck(const CPhysOBB& _Box, const CVec3Dfp32 tri[3], const CVec3Dfp32& _V, const CVec3Dfp32& _N, fp32& _BestD, CPlane3Dfp32& _BestPlane)
{
	MAUTOSTRIP(Phys_GetSeparatingDistance_NoCheck, MAUTOSTRIP_VOID);

	if (V3_DOTPROD(_V, _N) < 0.0f)
	{
		Phys_CreatePlaneOnTriangle(_N, tri, _BestPlane);

		_BestD = _Box.MinDistance(_BestPlane);
	}
	else
	{
		Phys_CreatePlaneOnTriangle(-_N, tri, _BestPlane);

		_BestD = _Box.MinDistance(_BestPlane);
	}
}


//---------------------------------------------------------------------------
void Phys_CreatePlaneOnBox(const CVec3Dfp32& _N, const CPhysOBB& _OBB, CPlane3Dfp32& _Plane)
{
	MAUTOSTRIP(Phys_CreatePlaneOnBox, MAUTOSTRIP_VOID);
	_Plane.n = _N;
	_Plane.d = 0;

	fp32 MaxD = _OBB.MaxDistance(_Plane);
	_Plane.d = -MaxD;

	MaxD = _OBB.MaxDistance(_Plane);
}


bool Phys_Intersect_TriOBB(const CVec3Dfp32 tri[3], const CPhysOBB& _BoxStart, const CPhysOBB& _BoxDest, bool _bOrder, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(Phys_Intersect_TriOBB, false);
	// (return == False) => No intersection)
	// (_bOrder == false) => Box collides to triangle. (Plane is adjecent to triangle)
	// (_bOrder == true) => Triangle collides to box. (Plane is adjecent to box)

//	bool bLog = false;

	if (!_pCollisionInfo)
	{
		return (TestIntersectionS(tri, _BoxDest) == INTERSECTION);
	}
	else
	{
		CVec3Dfp32 E0, E1, TriN, E2;
		tri[1].Sub(tri[0], E0);
		tri[2].Sub(tri[0], E1);
		E0.CrossProd(E1, TriN);
		if (TriN.LengthSqr() < 0.0001f) return false;
		tri[2].Sub(tri[1], E2);

//PhysLog(CStrF("Phys_Intersect_TriOBB: NORMAL %s TRIANGLE ", (const char*) TriN.GetString()) + tri[0].GetString() + tri[1].GetString() + tri[2].GetString());

		int IntersectDest = TestIntersectionS(tri, _BoxDest);
		if (IntersectDest != INTERSECTION)
		{
//			PhysLog("Phys_Intersect_TriOBB: No intersection. " + Phys_GetPlane(IntersectDest) + ",  " + tri[1].GetString() + _BoxDest.m_C.GetString());
			return false;
		}

/*		int IntersectStart = TestIntersectionS(tri, _BoxStart);
		if (IntersectStart == INTERSECTION)
		{
			// Was inside from the beginning, so we can't figure out how it collided.
			PhysLog("Phys_Intersect_TriOBB: Origin Inside.");
			return true;
		}*/

		CVec3Dfp32 dV;
		_BoxDest.m_C.Sub(_BoxStart.m_C, dV);
//#ifdef NEW_SEPARATING_PLANE

		TriN.Normalize();
		fp32 BestD = -100001.0f;
		Phys_GetSeparatingDistance(_BoxDest, tri, dV, TriN, BestD, _pCollisionInfo->m_Plane);

		Phys_GetSeparatingDistance(_BoxDest, tri, dV, _BoxDest.m_A[0], BestD, _pCollisionInfo->m_Plane);
		Phys_GetSeparatingDistance(_BoxDest, tri, dV, _BoxDest.m_A[1], BestD, _pCollisionInfo->m_Plane);
		Phys_GetSeparatingDistance(_BoxDest, tri, dV, _BoxDest.m_A[2], BestD, _pCollisionInfo->m_Plane);

		Phys_GetSeparatingDistance(_BoxDest, tri, dV, (_BoxDest.m_A[0] / E0), BestD, _pCollisionInfo->m_Plane, true);
		Phys_GetSeparatingDistance(_BoxDest, tri, dV, (_BoxDest.m_A[0] / E1), BestD, _pCollisionInfo->m_Plane, true);
		Phys_GetSeparatingDistance(_BoxDest, tri, dV, (_BoxDest.m_A[0] / E2), BestD, _pCollisionInfo->m_Plane, true);
		Phys_GetSeparatingDistance(_BoxDest, tri, dV, (_BoxDest.m_A[1] / E0), BestD, _pCollisionInfo->m_Plane, true);
		Phys_GetSeparatingDistance(_BoxDest, tri, dV, (_BoxDest.m_A[1] / E1), BestD, _pCollisionInfo->m_Plane, true);
		Phys_GetSeparatingDistance(_BoxDest, tri, dV, (_BoxDest.m_A[1] / E2), BestD, _pCollisionInfo->m_Plane, true);
		Phys_GetSeparatingDistance(_BoxDest, tri, dV, (_BoxDest.m_A[2] / E0), BestD, _pCollisionInfo->m_Plane, true);
		Phys_GetSeparatingDistance(_BoxDest, tri, dV, (_BoxDest.m_A[2] / E1), BestD, _pCollisionInfo->m_Plane, true);
		Phys_GetSeparatingDistance(_BoxDest, tri, dV, (_BoxDest.m_A[2] / E2), BestD, _pCollisionInfo->m_Plane, true);

		if (BestD < -100000)
		{
//			ConOut("No separating plane.");
//			return true;

			int IntersectStart = TestIntersectionS(tri, _BoxStart);
/*			if (IntersectStart == INTERSECTION)
			{
				Phys_GetSeparatingDistance_NoCheck(_BoxDest, tri, dV, TriN, BestD, _pCollisionInfo->m_Plane);
//				ConOut("(Phys_Intersect_TriOBB) That akward case.");
				// Was inside from the beginning, so we can't figure out how it collided.
				PhysLog("Phys_Intersect_TriOBB: Origin Inside.");
//				return true;
			}*/
//#ifdef NEVER
			switch(IntersectStart)
			{
			case INTERSECTION :
			case AXIS_N :
				{
					Phys_GetSeparatingDistance_NoCheck(_BoxDest, tri, dV, TriN, BestD, _pCollisionInfo->m_Plane);
				}
				break;
			case AXIS_A0 : 
			case AXIS_A1 : 
			case AXIS_A2 : 
				{
					int k = IntersectStart - AXIS_A0;
					Phys_GetSeparatingDistance_NoCheck(_BoxDest, tri, dV, _BoxStart.m_A[k], BestD, _pCollisionInfo->m_Plane);
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
					Phys_GetSeparatingDistance_NoCheck(_BoxDest, tri, dV, N, BestD, _pCollisionInfo->m_Plane);

	//				_pCollisionInfo->m_Plane.Create(N, tri[0]);
	//if (bLog) ConOutL(CStrF("Phys_Intersect_TriOBB: Edge %d, N ", IntersectStart - AXIS_A0xE0) + N.GetString() + tri[0].GetString());
	PhysLog(CStrF("Phys_Intersect_TriOBB: Edge %d, P ", IntersectStart - AXIS_A0xE0) + _pCollisionInfo->m_Plane.GetString());
				}
			}
//#endif
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
			_pCollisionInfo->m_Distance = Phys_GetMinTriangleDistance(tri, _pCollisionInfo->m_Plane, VertexNr) - 0.001f;
			_pCollisionInfo->m_LocalPos = tri[VertexNr] - _pCollisionInfo->m_Plane.n*_pCollisionInfo->m_Distance;
		}
		else
		{
			_pCollisionInfo->m_Distance = _BoxDest.MinDistance(_pCollisionInfo->m_Plane) - 0.001f;
	//		Phys_CalcIntersectionPoint(_BoxDest, _pCollisionInfo->m_Plane, _pCollisionInfo->m_Pos);
	//		_pCollisionInfo->m_LocalPos = _pCollisionInfo->m_Pos;
			_pCollisionInfo->m_Pos = _BoxDest.m_C - _pCollisionInfo->m_Plane.n*_pCollisionInfo->m_Plane.Distance(_BoxDest.m_C);
			_pCollisionInfo->m_LocalPos = _BoxDest.m_C - _pCollisionInfo->m_Plane.n*_pCollisionInfo->m_Plane.Distance(_BoxDest.m_C);
	//if (bLog) ConOutL(CStrF("Phys_Intersect_TriOBB: Distance %f, OBB %s, Pos ", _pCollisionInfo->m_Distance, (const char*) _BoxDest.m_C.GetString()) + _pCollisionInfo->m_Pos.GetString());
		}

		fp32 VLen = dV.Length();
		fp32 vProj = _pCollisionInfo->m_Plane.n * dV;
		if (vProj < -0.001f * VLen)
			_pCollisionInfo->m_Time = Clamp01(1.0f - _pCollisionInfo->m_Distance / vProj);
		else
		{
//			ConOut(CStrF("Bad velocity projection dV %s, N %s", (char*) dV.GetString(), (char*)_pCollisionInfo->m_Plane.n.GetString()));
			_pCollisionInfo->m_Time = 1.0f;
		}

// ConOut(CStrF("BoxTri P %d, d %f, t %f, N ", PlaneNr, _pCollisionInfo->m_Distance, _pCollisionInfo->m_Time) + _pCollisionInfo->m_Plane.n.GetString());

//		fp32 BoxDim = Max3(_BoxDest.m_E[0], _BoxDest.m_E[1], _BoxDest.m_E[2]);
//		if (_pCollisionInfo->m_Distance < -BoxDim * 0.5f) _pCollisionInfo->m_bIsValid = false;

		return true;
	}
}
