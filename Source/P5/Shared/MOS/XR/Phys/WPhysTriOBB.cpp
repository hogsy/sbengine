
#include "PCH.h"

#include "WPhys.h"
#include "../XRClass.h"
#include "MFloat.h"

#ifndef PLATFORM_PS2

//static enum
enum
{
    INTERSECTION,
    AXIS_N,
    AXIS_A0, AXIS_A1, AXIS_A2,
    AXIS_A0xE0, AXIS_A0xE1, AXIS_A0xE2,
    AXIS_A1xE0, AXIS_A1xE1, AXIS_A1xE2,
    AXIS_A2xE0, AXIS_A2xE1, AXIS_A2xE2
};

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
// This function will fail occationally if floats is used instead of doubles.
unsigned int TestIntersectionS (const CVec3Dfp64 tri[3], const CPhysOBB& _Box);
unsigned int TestIntersectionS (const CVec3Dfp64 tri[3], const CPhysOBB& _Box)
{
	MAUTOSTRIP(TestIntersectionS, 0);
    // construct triangle normal, difference of center and vertex
    CVec3Dfp64 D, E[2], N;
    DIFF(E[0],tri[1],tri[0]);
    DIFF(E[1],tri[2],tri[0]);
    CROSS(N,E[0],E[1]);
    DIFF(D,tri[0],_Box.m_C);

    // axis C+t*N
    double A0dN = DOT(_Box.m_A[0],N);
    double A1dN = DOT(_Box.m_A[1],N);
    double A2dN = DOT(_Box.m_A[2],N);
    double R = FABS(_Box.m_E[0]*A0dN)+FABS(_Box.m_E[1]*A1dN)+FABS(_Box.m_E[2]*A2dN);
    double NdD = DOT(N,D);
    if ( FABS(NdD) > R )
        return AXIS_N;

    // axis C+t*A0
    double A0dD = DOT(_Box.m_A[0],D);
    double A0dE0 = DOT(_Box.m_A[0],E[0]);
    double A0dE1 = DOT(_Box.m_A[0],E[1]);
    TESTS1(A0dD,A0dE0,A0dE1,_Box.m_E[0],AXIS_A0);

    // axis C+t*A1
    double A1dD = DOT(_Box.m_A[1],D);
    double A1dE0 = DOT(_Box.m_A[1],E[0]);
    double A1dE1 = DOT(_Box.m_A[1],E[1]);
    TESTS1(A1dD,A1dE0,A1dE1,_Box.m_E[1],AXIS_A1);

    // axis C+t*A2
    double A2dD = DOT(_Box.m_A[2],D);
    double A2dE0 = DOT(_Box.m_A[2],E[0]);
    double A2dE1 = DOT(_Box.m_A[2],E[1]);
    TESTS1(A2dD,A2dE0,A2dE1,_Box.m_E[2],AXIS_A2);

    // axis C+t*A0xE0
    CVec3Dfp64 A0xE0;
    CROSS(A0xE0,_Box.m_A[0],E[0]);
    double A0xE0dD = DOT(A0xE0,D);
	if (M_Fabs(A0xE0dD) > 0.0001f)
	{
	    R = FABS(_Box.m_E[1]*A2dE0)+FABS(_Box.m_E[2]*A1dE0);
	    TESTS2(A0xE0dD,A0dN,R,AXIS_A0xE0);
	}

    // axis C+t*A0xE1
    CVec3Dfp64 A0xE1;
    CROSS(A0xE1,_Box.m_A[0],E[1]);
    double A0xE1dD = DOT(A0xE1,D);
	if (M_Fabs(A0xE1dD) > 0.0001f)
	{
	    R = FABS(_Box.m_E[1]*A2dE1)+FABS(_Box.m_E[2]*A1dE1);
	    TESTS2(A0xE1dD,-A0dN,R,AXIS_A0xE1);
	}

    // axis C+t*A0xE2
//    double A1dE2 = A1dE1-A1dE0;
 //   double A2dE2 = A2dE1-A2dE0;

/*    CVec3Dfp64 E2;
    DIFF(E2,tri[2],tri[1]);
	double A1dE2 = DOT(_Box.m_A[1], E2);
	double A2dE2 = DOT(_Box.m_A[2], E2);
    CVec3Dfp64 A0xE2;
    CROSS(A0xE2,_Box.m_A[0],E2);
    double A0xE2dD = DOT(A0xE2,D);
    R = FABS(_Box.m_E[1]*A2dE2)+FABS(_Box.m_E[2]*A1dE2);
    TESTS2(A0xE2dD,A0dN,R,AXIS_A0xE2);*/

    double A1dE2 = A1dE1-A1dE0;
    double A2dE2 = A2dE1-A2dE0;
    double A0xE2dD = A0xE1dD-A0xE0dD;
	if (M_Fabs(A0xE2dD) > 0.0001f)
	{
		R = FABS(_Box.m_E[1]*A2dE2)+FABS(_Box.m_E[2]*A1dE2);
		TESTS2(A0xE2dD,-A0dN,R,AXIS_A0xE2);
	}

    // axis C+t*A1xE0
    CVec3Dfp64 A1xE0;
    CROSS(A1xE0,_Box.m_A[1],E[0]);
    double A1xE0dD = DOT(A1xE0,D);
	if (M_Fabs(A1xE0dD) > 0.0001f)
	{
		R = FABS(_Box.m_E[0]*A2dE0)+FABS(_Box.m_E[2]*A0dE0);
	    TESTS2(A1xE0dD,A1dN,R,AXIS_A1xE0);
	}

    // axis C+t*A1xE1
    CVec3Dfp64 A1xE1;
    CROSS(A1xE1,_Box.m_A[1],E[1]);
    double A1xE1dD = DOT(A1xE1,D);
	if (M_Fabs(A1xE1dD) > 0.0001f)
	{
		R = FABS(_Box.m_E[0]*A2dE1)+FABS(_Box.m_E[2]*A0dE1);
		TESTS2(A1xE1dD,-A1dN,R,AXIS_A1xE1);
	}

    // axis C+t*A1xE2
/*    CVec3Dfp64 A1xE2;
    CROSS(A1xE2,_Box.m_A[1],E2);
    double A0dE2 = A0dE1-A0dE0;
    double A1xE2dD = DOT(A1xE2,D);
    R = FABS(_Box.m_E[0]*A2dE2)+FABS(_Box.m_E[2]*A0dE2);
    TESTS2(A1xE2dD,-A1dN,R,AXIS_A1xE2);*/

    double A0dE2 = A0dE1-A0dE0;
    double A1xE2dD = A1xE1dD-A1xE0dD;
	if (M_Fabs(A1xE2dD) > 0.0001f)
	{
		R = FABS(_Box.m_E[0]*A2dE2)+FABS(_Box.m_E[2]*A0dE2);
		TESTS2(A1xE2dD,-A1dN,R,AXIS_A1xE2);
	}

    // axis C+t*A2xE0
    CVec3Dfp64 A2xE0;
    CROSS(A2xE0,_Box.m_A[2],E[0]);
    double A2xE0dD = DOT(A2xE0,D);
	if (M_Fabs(A2xE0dD) > 0.0001f)
	{
		R = FABS(_Box.m_E[0]*A1dE0)+FABS(_Box.m_E[1]*A0dE0);
		TESTS2(A2xE0dD,A2dN,R,AXIS_A2xE0);
	}

    // axis C+t*A2xE1
    CVec3Dfp64 A2xE1;
    CROSS(A2xE1,_Box.m_A[2],E[1]);
    double A2xE1dD = DOT(A2xE1,D);
	if (M_Fabs(A2xE1dD) > 0.0001f)
	{
		R = FABS(_Box.m_E[0]*A1dE1)+FABS(_Box.m_E[1]*A0dE1);
		TESTS2(A2xE1dD,-A2dN,R,AXIS_A2xE1);
	}

    // axis C+t*A2xE2
 /*   CVec3Dfp64 A2xE2;
    CROSS(A2xE2,_Box.m_A[2],E2);
    double A2xE2dD = DOT(A2xE2,D);
    R = FABS(_Box.m_E[0]*A1dE2)+FABS(_Box.m_E[1]*A0dE2);
    TESTS2(A2xE2dD,-A2dN,R,AXIS_A2xE2);
*/
    double A2xE2dD = A2xE1dD-A2xE0dD;
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
        if ( NdD+dt*DOT(N,W) > R ) \
            return axis; \
    } \
    else if ( NdD < -R ) \
    { \
        if ( NdD+dt*DOT(N,W) < -R ) \
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
    const CVec3Dfp32& triVel, const CPhysOBB& _Box, const CVec3Dfp32& boxVel);
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
    CROSS(N,E[0],E[1]);
    DIFF(D,tri[0],_Box.m_C);

    // axis C+t*N
    float A0dN = DOT(_Box.m_A[0],N);
    float A1dN = DOT(_Box.m_A[1],N);
    float A2dN = DOT(_Box.m_A[2],N);
    float R = FABS(_Box.m_E[0]*A0dN)+FABS(_Box.m_E[1]*A1dN)+FABS(_Box.m_E[2]*A2dN);
    float NdD = DOT(N,D);
    TESTV0(NdD,R,dt,N,W,AXIS_N);

    // axis C+t*A0
    float A0dD = DOT(_Box.m_A[0],D);
    float A0dE0 = DOT(_Box.m_A[0],E[0]);
    float A0dE1 = DOT(_Box.m_A[0],E[1]);
    float A0dW = DOT(_Box.m_A[0],W);
    TESTV1(A0dD,dt,A0dW,A0dE0,A0dE1,_Box.m_E[0],AXIS_A0);

    // axis C+t*A1
    float A1dD = DOT(_Box.m_A[1],D);
    float A1dE0 = DOT(_Box.m_A[1],E[0]);
    float A1dE1 = DOT(_Box.m_A[1],E[1]);
    float A1dW = DOT(_Box.m_A[1],W);
    TESTV1(A1dD,dt,A1dW,A1dE0,A1dE1,_Box.m_E[1],AXIS_A1);

    // axis C+t*A2
    float A2dD = DOT(_Box.m_A[2],D);
    float A2dE0 = DOT(_Box.m_A[2],E[0]);
    float A2dE1 = DOT(_Box.m_A[2],E[1]);
    float A2dW = DOT(_Box.m_A[2],W);
    TESTV1(A2dD,dt,A2dW,A2dE0,A2dE1,_Box.m_E[2],AXIS_A2);

    // axis C+t*A0xE0
    CVec3Dfp32 A0xE0;
    CROSS(A0xE0,_Box.m_A[0],E[0]);
    float A0xE0dD = DOT(A0xE0,D);
    float A0xE0dW = DOT(A0xE0,W);
    R = FABS(_Box.m_E[1]*A2dE0)+FABS(_Box.m_E[2]*A1dE0);
    TESTV2(A0xE0dD,dt,A0xE0dW,A0dN,R,AXIS_A0xE0);

    // axis C+t*A0xE1
    CVec3Dfp32 A0xE1;
    CROSS(A0xE1,_Box.m_A[0],E[1]);
    float A0xE1dD = DOT(A0xE1,D);
    float A0xE1dW = DOT(A0xE1,W);
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
    CROSS(A1xE0,_Box.m_A[1],E[0]);
    float A1xE0dD = DOT(A1xE0,D);
    float A1xE0dW = DOT(A1xE0,W);
    R = FABS(_Box.m_E[0]*A2dE0)+FABS(_Box.m_E[2]*A0dE0);
    TESTV2(A1xE0dD,dt,A1xE0dW,A1dN,R,AXIS_A1xE0);

    // axis C+t*A1xE1
    CVec3Dfp32 A1xE1;
    CROSS(A1xE1,_Box.m_A[1],E[1]);
    float A1xE1dD = DOT(A1xE1,D);
    float A1xE1dW = DOT(A1xE1,W);
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
    CROSS(A2xE0,_Box.m_A[2],E[0]);
    float A2xE0dD = DOT(A2xE0,D);
    float A2xE0dW = DOT(A2xE0,W);
    R = FABS(_Box.m_E[0]*A1dE0)+FABS(_Box.m_E[1]*A0dE0);
    TESTV2(A2xE0dD,dt,A2xE0dW,A2dN,R,AXIS_A2xE0);

    // axis C+t*A2xE1
    CVec3Dfp32 A2xE1;
    CROSS(A2xE1,_Box.m_A[2],E[1]);
    float A2xE1dD = DOT(A2xE1,D);
    float A2xE1dW = DOT(A2xE1,W);
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
void Phys_CreatePlaneOnTriangle(const CVec3Dfp64& _N, const CVec3Dfp64 tri[3], CPlane3Dfp64& _Plane);
void Phys_CreatePlaneOnTriangle(const CVec3Dfp64& _N, const CVec3Dfp64 tri[3], CPlane3Dfp64& _Plane)
{
	MAUTOSTRIP(Phys_CreatePlaneOnTriangle, MAUTOSTRIP_VOID);
	int iV = 0;
	fp64 d = _N * tri[0];
	if (_N * tri[1] > d)
	{
		d = _N * tri[1];
		iV = 1;
	}
	if (_N * tri[2] > d)
	{
		d = _N * tri[2];
		iV = 2;
	}

	_Plane.CreateNV(_N, tri[iV]);
}

fp64 Phys_GetMinTriangleDistance(const CVec3Dfp64 tri[3], const CPlane3Dfp64& _Plane, int& _VertexNr);
fp64 Phys_GetMinTriangleDistance(const CVec3Dfp64 tri[3], const CPlane3Dfp64& _Plane, int& _VertexNr)
{
	MAUTOSTRIP(Phys_GetMinTriangleDistance, 0.0);
	fp64 d1 = _Plane.Distance(tri[0]);
	fp64 d2 = _Plane.Distance(tri[1]);
	fp64 d3 = _Plane.Distance(tri[2]);

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

CStr Phys_GetPlane(int _Intersection);
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

void Phys_CalcIntersectionPoint(const CPhysOBB& _OBB, const CPlane3Dfp32& _Plane, CVec3Dfp32& _RetPos);
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


bool Phys_GetSeparatingDistance(const CPhysOBB& _Box, const CVec3Dfp64 tri[3], const CVec3Dfp64& _V, const CVec3Dfp64& _N, fp64& _BestD, CPlane3Dfp64& _BestPlane, bool _bNormalize = false);
bool Phys_GetSeparatingDistance(const CPhysOBB& _Box, const CVec3Dfp64 tri[3], const CVec3Dfp64& _V, const CVec3Dfp64& _N, fp64& _BestD, CPlane3Dfp64& _BestPlane, bool _bNormalize)
{
	MAUTOSTRIP(Phys_GetSeparatingDistance, false);
	const CVec3Dfp64* pN = &_N;
	CVec3Dfp64 N2;

	if (_N.LengthSqr() < 0.001) return false;
	if (_bNormalize)
	{
		N2 = _N;
		N2.Normalize();
		pN = &N2;
	}

//	if (M_Fabs(1.0f - (*pN).Length()) > 0.01f)
//		ConOut("No unit normal: " + _N.GetString());

	bool bModifed = false;
	fp64 Dot = _V * (*pN);
	if (M_Fabs(Dot) < 0.001f) return false;

	if (Dot < 0.0f)
	{
		CPlane3Dfp64 P;
		Phys_CreatePlaneOnTriangle((*pN), tri, P);

		fp64 d = _Box.MinDistance(P);
		if (d < Dot - 0.001f) return false;

		if (d > _BestD)
		{
			_BestD = d;
			_BestPlane = P;
			bModifed = true;
		}
	}
	else
	{
		CPlane3Dfp64 P;
		Phys_CreatePlaneOnTriangle(-(*pN), tri, P);

		fp64 d = _Box.MinDistance(P);
		if (d < -Dot - 0.001f) return false;

		if (d > _BestD)
		{
			_BestD = d;
			_BestPlane = P;
			bModifed = true;
		}
	}
	return bModifed;
}


void Phys_GetSeparatingDistance_NoCheck(const CPhysOBB& _Box, const CVec3Dfp64 tri[3], const CVec3Dfp64& _V, const CVec3Dfp64& _N, fp64& _BestD, CPlane3Dfp64& _BestPlane);
void Phys_GetSeparatingDistance_NoCheck(const CPhysOBB& _Box, const CVec3Dfp64 tri[3], const CVec3Dfp64& _V, const CVec3Dfp64& _N, fp64& _BestD, CPlane3Dfp64& _BestPlane)
{
	MAUTOSTRIP(Phys_GetSeparatingDistance_NoCheck, MAUTOSTRIP_VOID);
	bool bModifed = false;
	fp64 Dot = _V * _N;

	if (Dot < 0.0f)
	{
		CPlane3Dfp64 P;
		Phys_CreatePlaneOnTriangle(_N, tri, P);

		fp32 d = _Box.MinDistance(P);
		_BestD = d;
		_BestPlane = P;
	}
	else
	{
		CPlane3Dfp64 P;
		Phys_CreatePlaneOnTriangle(-_N, tri, P);

		fp64 d = _Box.MinDistance(P);
		_BestD = d;
		_BestPlane = P;
	}
}

//---------------------------------------------------------------------------
void Phys_CreatePlaneOnBox(const CVec3Dfp64& _N, const CPhysOBB& _OBB, CPlane3Dfp64& _Plane)
{
	MAUTOSTRIP(Phys_CreatePlaneOnBox, MAUTOSTRIP_VOID);
	_Plane.n = _N;
	_Plane.d = 0;

	fp32 MaxD = _OBB.MaxDistance(_Plane);
	_Plane.d = -MaxD;

	MaxD = _OBB.MaxDistance(_Plane);
}

#define V3_ADD(a, b, c) { c[0] = a[0] + b[0]; c[1] = a[1] + b[1]; c[2] = a[2] + b[2]; }
#define V3_SUB(a, b, c) { c[0] = a[0] - b[0]; c[1] = a[1] - b[1]; c[2] = a[2] - b[2]; }
#define V3_DOTPROD(a, b) (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])
#define V3_CROSSPROD(p, q, cross) { cross[0] = p[1]*q[2]-p[2]*q[1]; cross[1] = p[2]*q[0]-p[0]*q[2]; cross[2] = p[0]*q[1]-p[1]*q[0]; }

bool Phys_Intersect_TriOBB(const CVec3Dfp32 tri[3], const CPhysOBB& _BoxStart, const CPhysOBB& _BoxDest, bool _bOrder, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(Phys_Intersect_TriOBB, false);
	// (return == False) => No intersection)
	// (_bOrder == false) => Box collides to triangle. (Plane is adjecent to triangle)
	// (_bOrder == true) => Triangle collides to box. (Plane is adjecent to box)

	bool bLog = false;

	CVec3Dfp64 tri8[3];
	tri[0].Assignfp64(tri8[0]);
	tri[1].Assignfp64(tri8[1]);
	tri[2].Assignfp64(tri8[2]);

	if (!_pCollisionInfo)
	{
		return (TestIntersectionS(tri8, _BoxDest) == INTERSECTION);
	}
	else
	{
		CVec3Dfp64 E0, E1, TriN, E2;
		tri8[1].Sub(tri8[0], E0);
		tri8[2].Sub(tri8[0], E1);
		E0.CrossProd(E1, TriN);
		if (TriN.LengthSqr() < 0.0001) return false;
		tri8[2].Sub(tri8[1], E2);
		
//ConOut(CStrF("Phys_Intersect_TriOBB: NORMAL %s TRIANGLE ", (const char*) TriN.GetString()) + tri[0].GetString() + tri[1].GetString() + tri[2].GetString());

		int IntersectDest = TestIntersectionS(tri8, _BoxDest);
		if (IntersectDest != INTERSECTION)
		{
//			ConOut("Phys_Intersect_TriOBB: No intersection. " + Phys_GetPlane(IntersectDest) + ",  " + tri[1].GetString() + _BoxDest.m_C.GetString());
			return false;
		}


/*		int IntersectStart = TestIntersectionS(tri, _BoxStart);
		if (IntersectStart == INTERSECTION)
		{
			// Was inside from the beginning, so we can't figure out how it collided.
			PhysLog("Phys_Intersect_TriOBB: Origin Inside.");
			return true;
		}*/

		CVec3Dfp64 dV;
		V3_SUB(_BoxDest.m_C, _BoxStart.m_C, dV);

//#ifdef NEW_SEPARATING_PLANE

		CPlane3Dfp64 Plane;

		TriN.Normalize();
		fp64 BestD = -100001.0f;
		int PlaneNr = -1;
		if (Phys_GetSeparatingDistance(_BoxDest, tri8, dV, TriN, BestD, Plane)) PlaneNr = 1;

		if (Phys_GetSeparatingDistance(_BoxDest, tri8, dV, _BoxDest.m_A[0].Getfp64(), BestD, Plane)) PlaneNr = 2;
		if (Phys_GetSeparatingDistance(_BoxDest, tri8, dV, _BoxDest.m_A[1].Getfp64(), BestD, Plane)) PlaneNr = 3;
		if (Phys_GetSeparatingDistance(_BoxDest, tri8, dV, _BoxDest.m_A[2].Getfp64(), BestD, Plane)) PlaneNr = 4;

		if (Phys_GetSeparatingDistance(_BoxDest, tri8, dV, (_BoxDest.m_A[0].Getfp64() / E0), BestD, Plane, true)) PlaneNr = 10;
		if (Phys_GetSeparatingDistance(_BoxDest, tri8, dV, (_BoxDest.m_A[0].Getfp64() / E1), BestD, Plane, true)) PlaneNr = 11;
		if (Phys_GetSeparatingDistance(_BoxDest, tri8, dV, (_BoxDest.m_A[0].Getfp64() / E2), BestD, Plane, true)) PlaneNr = 12;
		if (Phys_GetSeparatingDistance(_BoxDest, tri8, dV, (_BoxDest.m_A[1].Getfp64() / E0), BestD, Plane, true)) PlaneNr = 20;
		if (Phys_GetSeparatingDistance(_BoxDest, tri8, dV, (_BoxDest.m_A[1].Getfp64() / E1), BestD, Plane, true)) PlaneNr = 21;
		if (Phys_GetSeparatingDistance(_BoxDest, tri8, dV, (_BoxDest.m_A[1].Getfp64() / E2), BestD, Plane, true)) PlaneNr = 22;
		if (Phys_GetSeparatingDistance(_BoxDest, tri8, dV, (_BoxDest.m_A[2].Getfp64() / E0), BestD, Plane, true)) PlaneNr = 30;
		if (Phys_GetSeparatingDistance(_BoxDest, tri8, dV, (_BoxDest.m_A[2].Getfp64() / E1), BestD, Plane, true)) PlaneNr = 31;
		if (Phys_GetSeparatingDistance(_BoxDest, tri8, dV, (_BoxDest.m_A[2].Getfp64() / E2), BestD, Plane, true)) PlaneNr = 32;

		if (BestD < -100000)
		{
//			ConOut("No separating plane.");
//			return true;

//#ifdef NEVER
			int IntersectStart = TestIntersectionS(tri8, _BoxStart);
/*			if (IntersectStart == INTERSECTION)
			{
				Phys_GetSeparatingDistance_NoCheck(_BoxDest, tri, dV, TriN, BestD, _pCollisionInfo->m_Plane);
//				ConOut("(Phys_Intersect_TriOBB) That akward case.");
				// Was inside from the beginning, so we can't figure out how it collided.
				PhysLog("Phys_Intersect_TriOBB: Origin Inside.");
//				return true;
			}*/
			switch(IntersectStart)
			{
			case INTERSECTION :
			case AXIS_N :
				{
					Phys_GetSeparatingDistance_NoCheck(_BoxDest, tri8, dV, TriN, BestD, Plane);
				}
				break;
			case AXIS_A0 : 
			case AXIS_A1 : 
			case AXIS_A2 : 
				{
					int k = IntersectStart - AXIS_A0;
					Phys_GetSeparatingDistance_NoCheck(_BoxDest, tri8, dV, _BoxStart.m_A[k].Getfp64(), BestD, Plane);
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
					Phys_GetSeparatingDistance_NoCheck(_BoxDest, tri8, dV, N, BestD, Plane);

	//				_pCollisionInfo->m_Plane.Create(N, tri[0]);
	//if (bLog) ConOutL(CStrF("Phys_Intersect_TriOBB: Edge %d, N ", IntersectStart - AXIS_A0xE0) + N.GetString() + tri[0].GetString());
	//PhysLog(CStrF("Phys_Intersect_TriOBB: Edge %d, P ", IntersectStart - AXIS_A0xE0) + _pCollisionInfo->m_Plane.GetString());
				}
			}
//#endif
		}

		if (_bOrder)
		{
			Plane.Inverse();
			Phys_CreatePlaneOnBox(Plane.n, _BoxDest, Plane);
		}

		Plane.n.Assignfp32(_pCollisionInfo->m_Plane.n);
		_pCollisionInfo->m_Plane.d = Plane.d;

		_pCollisionInfo->m_bIsCollision = true;
		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_Velocity = 0;
		if (_bOrder)
		{
			int VertexNr = 0;
			_pCollisionInfo->m_Distance = Phys_GetMinTriangleDistance(tri8, Plane, VertexNr) - 0.001f;
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

		fp64 VLen = dV.Length();
		fp64 vProj = Plane.n * dV;
		if (vProj < -0.001f * VLen)
			_pCollisionInfo->m_Time = Clamp01(1.0f - _pCollisionInfo->m_Distance / vProj);
		else
		{
//			ConOut(CStrF("Bad velocity projection dV %s, N %s", (char*) dV.GetString(), (char*)_pCollisionInfo->m_Plane.n.GetString()));
			_pCollisionInfo->m_Time = 1.0;
		}

//ConOut(CStrF("BoxTri P %d, d %f, t %f, N ", PlaneNr, _pCollisionInfo->m_Distance, _pCollisionInfo->m_Time) + _pCollisionInfo->m_Plane.n.GetString());

//		fp32 BoxDim = Max3(_BoxDest.m_E[0], _BoxDest.m_E[1], _BoxDest.m_E[2]);
//		if (_pCollisionInfo->m_Distance < -BoxDim * 0.5f) _pCollisionInfo->m_bIsValid = false;

		return true;
	}
}

#endif
