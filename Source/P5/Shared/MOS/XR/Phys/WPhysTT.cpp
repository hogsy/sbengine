
#include "PCH.h"

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

enum
{
    INTERSECTION,
    AXIS_N, AXIS_M,
    AXIS_E0xF0, AXIS_E0xF1, AXIS_E0xF2,
    AXIS_E1xF0, AXIS_E1xF1, AXIS_E1xF2,
    AXIS_E2xF0, AXIS_E2xF1, AXIS_E2xF2
};

//---------------------------------------------------------------------------
#define FABS(x) (float(M_Fabs(x)))
//---------------------------------------------------------------------------
// compare [p] to [min{q,q+d0,q+d1},max{q,q+d0,q+d1}]
#define TESTS0(p,q,d0,d1,axis) \
{ \
    if ( (q) > (p) ) \
    { \
        if ( (q)+(d0) > (p) && (q)+(d1) > (p) ) \
            return axis; \
    } \
    else if ( (q) < (p) ) \
    { \
        if ( (q)+(d0) < (p) && (q)+(d1) < (p) ) \
            return axis; \
    } \
}
//---------------------------------------------------------------------------
// compare [min{0,p},max{0,p}] to [min{q,q+d},max{q,q+d}]
#define TESTS1(p,q,d,axis) \
{ \
    if ( (p) >= 0.0f ) \
    { \
        if ( (q) < 0.0f ) \
        { \
            if ( (q)+(d) < 0.0f ) \
                return axis; \
        } \
        else if ( (q) > (p) ) \
        { \
            if ( (q)+(d) > (p) ) \
                return axis; \
        } \
    } \
    else \
    { \
        if ( (q) > 0.0f ) \
        { \
            if ( (q)+(d) > 0.0f ) \
                return axis; \
        } \
        else if ( (q) < (p) ) \
        { \
            if ( (q)+(d) < (p) ) \
                return axis; \
        } \
    } \
}

//---------------------------------------------------------------------------
unsigned int TestIntersectionS (const CVec3Dfp32 tri0[3], const CVec3Dfp32 tri1[3])
{
	MAUTOSTRIP(TestIntersectionS, 0);
    // compute triangle edges, normals, difference of zero-th vertices
    CVec3Dfp32 E0, E1, F0, F1, N, D;
    DIFF(E0,tri0[1],tri0[0]);
    DIFF(E1,tri0[2],tri0[0]);
    DIFF(F0,tri1[1],tri1[0]);
    DIFF(F1,tri1[2],tri1[0]);
    DIFF(D,tri1[0],tri0[0]);
    CROSS(N,E0,E1);

    // axis U0+t*N
    float NdD = DOT(N,D);
    float NdF0 = DOT(N,F0);
    float NdF1 = DOT(N,F1);
    TESTS0(0.0f,NdD,NdF0,NdF1,AXIS_N);

    // axis U0+t*M
    CVec3Dfp32 M;
    CROSS(M,F0,F1);
    float MdD = DOT(M,D);
    float MdE0 = DOT(M,E0);
    float MdE1 = DOT(M,E1);
    TESTS0(MdD,0.0f,MdE0,MdE1,AXIS_M);

    // axis U0+t*E0xF0
    CVec3Dfp32 E0xF0;
    CROSS(E0xF0,E0,F0);
    float E0xF0dD = DOT(E0xF0,D);
    TESTS1(-NdF0,E0xF0dD,MdE0,AXIS_E0xF0);

    // axis U0+t*E0xF1
    CVec3Dfp32 E0xF1;
    CROSS(E0xF1,E0,F1);
    float E0xF1dD = DOT(E0xF1,D);
    TESTS1(-NdF1,E0xF1dD,-MdE0,AXIS_E0xF1);

    // axis U0+t*E0xF2
    float E0xF2dD = E0xF1dD - E0xF0dD;
    float NdF2 = NdF1 - NdF0;
    TESTS1(-NdF2,E0xF2dD,-MdE0,AXIS_E0xF2);

    // axis U0+t*E1xF0
    CVec3Dfp32 E1xF0;
    CROSS(E1xF0,E1,F0);
    float E1xF0dD = DOT(E1xF0,D);
    TESTS1(NdF0,E1xF0dD,MdE1,AXIS_E1xF0);

    // axis U0+t*E1xF1
    CVec3Dfp32 E1xF1;
    CROSS(E1xF1,E1,F1);
    float E1xF1dD = DOT(E1xF1,D);
    TESTS1(NdF1,E1xF1dD,-MdE1,AXIS_E1xF1);

    // axis U0+t*E1xF2
    float E1xF2dD = E1xF1dD - E1xF0dD;
    TESTS1(NdF2,E1xF2dD,-MdE1,AXIS_E1xF2);

    // axis U0+t*E2xF0
    float MdE2 = MdE1-MdE0;
    float E2xF0dD = E1xF0dD - E0xF0dD;
    TESTS1(NdF0,E2xF0dD,MdE2,AXIS_E2xF0);

    // axis U0+t*E2xF1
    float E2xF1dD = E1xF1dD - E0xF1dD;
    TESTS1(NdF1,E2xF1dD,-MdE2,AXIS_E2xF1);

    // axis U0+t*E2xF2
    float E2xF2dD = E2xF1dD - E2xF0dD;
    TESTS1(NdF2,E2xF2dD,-MdE2,AXIS_E2xF2);

    // triangles intersect
    return INTERSECTION;
}
//---------------------------------------------------------------------------
// compare [p] to [min{q,q+d0,q+d1},max{q,q+d0,q+d1}]+t*w for t >= 0
#define TESTV0(p,q,d0,d1,t,w,axis) \
{ \
    if ( (q) > (p) ) \
    { \
        float min; \
        if ( (d0) >= 0.0f ) \
        { \
            if ( (d1) >= 0.0f ) \
            { \
                if ( (q)+(t)*(w) > (p) ) \
                    return axis; \
            } \
            else \
            { \
                min = (q)+(d1); \
                if ( min > (p) && min+(t)*(w) > (p) ) \
                    return axis; \
            } \
        } \
        else if ( (d1) <= (d0) ) \
        { \
            min = (q)+(d1); \
            if ( min > (p) && min+(t)*(w) > (p) ) \
                return axis; \
        } \
        else \
        { \
            min = (q)+(d0); \
            if ( min > (p) && min+(t)*(w) > (p) ) \
                return axis; \
        } \
    } \
    else if ( (q) < (p) ) \
    { \
        float max; \
        if ( (d0) <= 0.0f ) \
        { \
            if ( (d1) <= 0.0f ) \
            { \
                if ( (q)+(t)*(w) < (p) ) \
                    return axis; \
            } \
            else \
            { \
                max = (q)+(d1); \
                if ( max < (p) && max+(t)*(w) < (p) ) \
                    return axis; \
            } \
        } \
        else if ( (d1) >= (d0) ) \
        { \
            max = (q)+(d1); \
            if ( max < (p) && max+(t)*(w) < (p) ) \
                return axis; \
        } \
        else \
        { \
            max = (q)+(d0); \
            if ( max < (p) && max+(t)*(w) < (p) ) \
                return axis; \
        } \
    } \
}
//---------------------------------------------------------------------------
// compare [min{0,p},max{0,p}] to [min{q,q+d},max{q,q+d}]+t*w for t >= 0
#define TESTV1(p,q,d,t,w,axis) \
{ \
    float min, max; \
 \
    if ( (p) >= 0.0f ) \
    { \
        if ( (q) < 0.0f ) \
        { \
            if ( (d) <= 0.0f ) \
            { \
                if ( (q)+(t)*(w) < 0.0f ) \
                    return axis; \
            } \
            else \
            { \
                max = (q)+(d); \
                if ( max < 0.0f && max+(t)*(w) < 0.0f ) \
                    return axis; \
            } \
        } \
        else if ( (q) > (p) ) \
        { \
            if ( (d) >= 0.0f ) \
            { \
                if ( (q)+(t)*(w) > (p) ) \
                    return axis; \
            } \
            else \
            { \
                min = (q)+(d); \
                if ( min > (p) && min+(t)*(w) > (p) ) \
                    return axis; \
            } \
        } \
    } \
    else \
    { \
        if ( (q) > 0.0f ) \
        { \
            if ( (d) >= 0.0f ) \
            { \
                if ( (q)+(t)*(w) > 0.0f ) \
                    return axis; \
            } \
            else \
            { \
                min = (q)+(d); \
                if ( min > 0.0f && min+(t)*(w) > 0.0f ) \
                    return axis; \
            } \
        } \
        else if ( (q) < (p) ) \
        { \
            if ( (d) <= 0.0f ) \
            { \
                if ( (q)+(t)*(w) < (p) ) \
                    return axis; \
            } \
            else \
            { \
                max = (q)+(d); \
                if ( max < (p) && max+(t)*(w) < (p) ) \
                    return axis; \
            } \
        } \
    } \
}
//---------------------------------------------------------------------------
unsigned int TestIntersectionV (float dt, const CVec3Dfp32 tri0[3],
    const CVec3Dfp32& V0, const CVec3Dfp32 tri1[3], const CVec3Dfp32& V1)
{
	MAUTOSTRIP(TestIntersectionV, 0);
    // Compute relative velocity of second triangle with respect to first
    // triangle so that first triangle may as well be stationary.
    CVec3Dfp32 W;
    DIFF(W,V1,V0);

    // compute triangle edges, normals, difference of zero-th vertices
    CVec3Dfp32 E0, E1, F0, F1, N, D;
    DIFF(E0,tri0[1],tri0[0]);
    DIFF(E1,tri0[2],tri0[0]);
    DIFF(F0,tri1[1],tri1[0]);
    DIFF(F1,tri1[2],tri1[0]);
    DIFF(D,tri1[0],tri0[0]);
    CROSS(N,E0,E1);

    // axis U0+t*N
    float NdD = DOT(N,D);
    float NdF0 = DOT(N,F0);
    float NdF1 = DOT(N,F1);
    float NdW = DOT(N,W);
    TESTV0(0.0f,NdD,NdF0,NdF1,dt,NdW,AXIS_N);

    // axis U0+t*M
    CVec3Dfp32 M;
    CROSS(M,F0,F1);
    float MdD = DOT(M,D);
    float MdE0 = DOT(M,E0);
    float MdE1 = DOT(M,E1);
    float MdW = DOT(M,W);
    TESTV0(MdD,0.0f,MdE0,MdE1,dt,-MdW,AXIS_M);

    // axis U0+t*E0xF0
    CVec3Dfp32 E0xF0;
    CROSS(E0xF0,E0,F0);
    float E0xF0dD = DOT(E0xF0,D);
    float E0xF0dW = DOT(E0xF0,W);
    TESTV1(-NdF0,E0xF0dD,MdE0,dt,E0xF0dW,AXIS_E0xF0);

    // axis U0+t*E0xF1
    CVec3Dfp32 E0xF1;
    CROSS(E0xF1,E0,F1);
    float E0xF1dD = DOT(E0xF1,D);
    float E0xF1dW = DOT(E0xF1,W);
    TESTV1(-NdF1,E0xF1dD,-MdE0,dt,E0xF1dW,AXIS_E0xF1);

    // axis U0+t*E0xF2
    float NdF2 = NdF1 - NdF0;
    float E0xF2dD = E0xF1dD - E0xF0dD;
    float E0xF2dW = E0xF1dW - E0xF0dW;
    TESTV1(-NdF2,E0xF2dD,-MdE0,dt,E0xF2dW,AXIS_E0xF2);

    // axis U0+t*E1xF0
    CVec3Dfp32 E1xF0;
    CROSS(E1xF0,E1,F0);
    float E1xF0dD = DOT(E1xF0,D);
    float E1xF0dW = DOT(E1xF0,W);
    TESTV1(NdF0,E1xF0dD,MdE1,dt,E1xF0dW,AXIS_E1xF0);

    // axis U0+t*E1xF1
    CVec3Dfp32 E1xF1;
    CROSS(E1xF1,E1,F1);
    float E1xF1dD = DOT(E1xF1,D);
    float E1xF1dW = DOT(E1xF1,W);
    TESTV1(NdF1,E1xF1dD,-MdE1,dt,E1xF1dW,AXIS_E1xF1);

    // axis U0+t*E1xF2
    float E1xF2dD = E1xF1dD - E1xF0dD;
    float E1xF2dW = E1xF1dW - E1xF0dW;
    TESTV1(NdF2,E1xF2dD,-MdE1,dt,E1xF2dW,AXIS_E1xF2);

    // axis U0+t*E2xF0
    float MdE2 = MdE1-MdE0;
    float E2xF0dD = E1xF0dD - E0xF0dD;
    float E2xF0dW = E1xF0dW - E0xF0dW;
    TESTV1(NdF0,E2xF0dD,MdE2,dt,E2xF0dW,AXIS_E2xF0);

    // axis U0+t*E2xF1
    float E2xF1dD = E1xF1dD - E0xF1dD;
    float E2xF1dW = E1xF1dW - E0xF1dW;
    TESTV1(NdF1,E2xF1dD,-MdE2,dt,E2xF1dW,AXIS_E2xF1);

    // axis U0+t*E2xF2
    float E2xF2dD = E2xF1dD - E2xF0dD;
    float E2xF2dW = E2xF1dW - E2xF0dW;
    TESTV1(NdF2,E2xF2dD,-MdE2,dt,E2xF2dW,AXIS_E2xF2);

    // triangles intersect
    return INTERSECTION;
}
//---------------------------------------------------------------------------
/*
static enum
{
    INTERSECTION,
    AXIS_N,
    AXIS_M,
    AXIS_E0xF0,
    AXIS_E0xF1,
    AXIS_E0xF2,
    AXIS_E1xF0,
    AXIS_E1xF1,
    AXIS_E1xF2,
    AXIS_E2xF0,
    AXIS_E2xF1,
    AXIS_E2xF2
};
*/
char* tr3tr3Axis[12] =
{
    "intersection",
    "N",
    "M",
    "E0xF0",
    "E0xF1",
    "E0xF2",
    "E1xF0",
    "E1xF1",
    "E1xF2",
    "E2xF0",
    "E2xF1",
    "E2xF2"
};

//---------------------------------------------------------------------------
#define FABS(x) (float(M_Fabs(x)))
//---------------------------------------------------------------------------
// compare [p] to [min{q,q+d0,q+d1},max{q,q+d0,q+d1}]+t*w for t >= 0
#define FINDV0(p,q,d0,d1,t,w,tmax,type,extr,side,axis) \
{ \
    float tmp; \
    if ( (q) > (p) ) \
    { \
        float min; \
        if ( (d0) >= 0.0f ) \
        { \
            if ( (d1) >= 0.0f ) \
            { \
                if ( (q)+(t)*(w) > (p) ) \
                    return axis; \
                tmp = (t)*((p)-(q))/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 0; \
                    side = +1; \
                } \
            } \
            else \
            { \
                min = (q)+(d1); \
                if ( min > (p) ) \
                { \
                    if ( min+(t)*(w) > (p) ) \
                        return axis; \
                    tmp = (t)*((p)-min)/(w); \
                    if ( tmp > tmax ) \
                    { \
                        tmax = tmp; \
                        type = axis; \
                        extr = 2; \
                        side = +1; \
                    } \
                } \
            } \
        } \
        else if ( (d1) <= (d0) ) \
        { \
            min = (q)+(d1); \
            if ( min > (p) ) \
            { \
                if ( min+(t)*(w) > (p) ) \
                    return axis; \
                tmp = (t)*((p)-min)/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 2; \
                    side = +1; \
                } \
            } \
        } \
        else \
        { \
            min = (q)+(d0); \
            if ( min > (p) ) \
            { \
                if ( min+(t)*(w) > (p) ) \
                    return axis; \
                tmp = (t)*((p)-min)/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 1; \
                    side = +1; \
                } \
            } \
        } \
    } \
    else if ( (q) < (p) ) \
    { \
        float max; \
        if ( (d0) <= 0.0f ) \
        { \
            if ( (d1) <= 0.0f ) \
            { \
                if ( (q)+(t)*(w) < (p) ) \
                    return axis; \
                tmp = (t)*((p)-(q))/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 0; \
                    side = -1; \
                } \
            } \
            else \
            { \
                max = (q)+(d1); \
                if ( max < (p) ) \
                { \
                    if ( max+(t)*(w) < (p) ) \
                        return axis; \
                    tmp = (t)*((p)-max)/(w); \
                    if ( tmp > tmax ) \
                    { \
                        tmax = tmp; \
                        type = axis; \
                        extr = 2; \
                        side = -1; \
                    } \
                } \
            } \
        } \
        else if ( (d1) >= (d0) ) \
        { \
            max = (q)+(d1); \
            if ( max < (p) ) \
            { \
                if ( max+(t)*(w) < (p) ) \
                    return axis; \
                tmp = (t)*((p)-max)/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 2; \
                    side = -1; \
                } \
            } \
        } \
        else \
        { \
            max = (q)+(d0); \
            if ( max < (p) ) \
            { \
                if ( max+(t)*(w) < (p) ) \
                    return axis; \
                tmp = (t)*((p)-max)/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 1; \
                    side = -1; \
                } \
            } \
        } \
    } \
}
//---------------------------------------------------------------------------
// compare [min{0,p},max{0,p}] to [min{q,q+d},max{q,q+d}]+t*w for t >= 0
#define FINDV1(p,q,d,t,w,tmax,type,extr,side,axis) \
{ \
    float tmp, min, max; \
 \
    if ( (p) >= 0.0f ) \
    { \
        if ( (q) < 0.0f ) \
        { \
            if ( (d) <= 0.0f ) \
            { \
                if ( (q)+(t)*(w) < 0.0f ) \
                    return axis; \
                tmp = -(t)*(q)/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 0; \
                    side = -1; \
                } \
            } \
            else \
            { \
                max = (q)+(d); \
                if ( max < 0.0f ) \
                { \
                    if ( max+(t)*(w) < 0.0f ) \
                        return axis; \
                    tmp = -(t)*max/(w); \
                    if ( tmp > tmax ) \
                    { \
                        tmax = tmp; \
                        type = axis; \
                        extr = 0; \
                        side = -1; \
                    } \
                } \
            } \
        } \
        else if ( (q) > (p) ) \
        { \
            if ( (d) >= 0.0f ) \
            { \
                if ( (q)+(t)*(w) > (p) ) \
                    return axis; \
                tmp = (t)*((p)-(q))/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 1; \
                    side = +1; \
                } \
            } \
            else \
            { \
                min = (q)+(d); \
                if ( min > (p) ) \
                { \
                    if ( min+(t)*(w) > (p) ) \
                        return axis; \
                    tmp = (t)*((p)-min)/(w); \
                    if ( tmp > tmax ) \
                    { \
                        tmax = tmp; \
                        type = axis; \
                        extr = 1; \
                        side = +1; \
                    } \
                } \
            } \
        } \
    } \
    else \
    { \
        if ( (q) > 0.0f ) \
        { \
            if ( (d) >= 0.0f ) \
            { \
                if ( (q)+(t)*(w) > 0.0f ) \
                    return axis; \
                tmp = -(t)*(q)/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 0; \
                    side = +1; \
                } \
            } \
            else \
            { \
                min = (q)+(d); \
                if ( min > 0.0f ) \
                { \
                    if ( min+(t)*(w) > 0.0f ) \
                        return axis; \
                    tmp = -(t)*min/(w); \
                    if ( tmp > tmax ) \
                    { \
                        tmax = tmp; \
                        type = axis; \
                        extr = 0; \
                        side = +1; \
                    } \
                } \
            } \
        } \
        else if ( (q) < (p) ) \
        { \
            if ( (d) <= 0.0f ) \
            { \
                if ( (q)+(t)*(w) < (p) ) \
                    return axis; \
                tmp = (t)*((p)-(q))/(w); \
                if ( tmp > tmax ) \
                { \
                    tmax = tmp; \
                    type = axis; \
                    extr = 1; \
                    side = -1; \
                } \
            } \
            else \
            { \
                max = (q)+(d); \
                if ( max < (p) ) \
                { \
                    if ( max+(t)*(w) < (p) ) \
                        return axis; \
                    tmp = (t)*((p)-max)/(w); \
                    if ( tmp > tmax ) \
                    { \
                        tmax = tmp; \
                        type = axis; \
                        extr = 1; \
                        side = -1; \
                    } \
                } \
            } \
        } \
    } \
}
//---------------------------------------------------------------------------
unsigned int FindIntersectionV (float dt, const CVec3Dfp32 tri0[3],
    const CVec3Dfp32& V0, const CVec3Dfp32 tri1[3], const CVec3Dfp32& V1,
    float& T, CVec3Dfp32& P)
{
    // Compute relative velocity of second triangle with respect to first
    // triangle so that first triangle may as well be stationary.
    CVec3Dfp32 W;
    DIFF(W,V1,V0);

    // compute triangle edges, normals, difference of zero-th vertices
    CVec3Dfp32 E0, E1, F0, F1, N, D;
    DIFF(E0,tri0[1],tri0[0]);
    DIFF(E1,tri0[2],tri0[0]);
    DIFF(F0,tri1[1],tri1[0]);
    DIFF(F1,tri1[2],tri1[0]);
    DIFF(D,tri1[0],tri0[0]);
    CROSS(N,E0,E1);

    // track maximum time of projection-intersection
    unsigned int type = INTERSECTION, extr = 0;
    int side = 0;
    T = 0.0f;

    // axis U0+t*N
    float NdD = DOT(N,D);
    float NdF0 = DOT(N,F0);
    float NdF1 = DOT(N,F1);
    float NdW = DOT(N,W);
    FINDV0(0.0f,NdD,NdF0,NdF1,dt,NdW,T,type,extr,side,AXIS_N);

    // axis U0+t*M
    CVec3Dfp32 M;
    CROSS(M,F0,F1);
    float MdD = DOT(M,D);
    float MdE0 = DOT(M,E0);
    float MdE1 = DOT(M,E1);
    float MdW = DOT(M,W);
    FINDV0(MdD,0.0f,MdE0,MdE1,dt,-MdW,T,type,extr,side,AXIS_M);

    // axis U0+t*E0xF0
    CVec3Dfp32 E0xF0;
    CROSS(E0xF0,E0,F0);
    float E0xF0dD = DOT(E0xF0,D);
    float E0xF0dW = DOT(E0xF0,W);
    FINDV1(-NdF0,E0xF0dD,MdE0,dt,E0xF0dW,T,type,extr,side,AXIS_E0xF0);

    // axis U0+t*E0xF1
    CVec3Dfp32 E0xF1;
    CROSS(E0xF1,E0,F1);
    float E0xF1dD = DOT(E0xF1,D);
    float E0xF1dW = DOT(E0xF1,W);
    FINDV1(-NdF1,E0xF1dD,-MdE0,dt,E0xF1dW,T,type,extr,side,AXIS_E0xF1);

    // axis U0+t*E0xF2
    float NdF2 = NdF1 - NdF0;
    float E0xF2dD = E0xF1dD - E0xF0dD;
    float E0xF2dW = E0xF1dW - E0xF0dW;
    FINDV1(-NdF2,E0xF2dD,-MdE0,dt,E0xF2dW,T,type,extr,side,AXIS_E0xF2);

    // axis U0+t*E1xF0
    CVec3Dfp32 E1xF0;
    CROSS(E1xF0,E1,F0);
    float E1xF0dD = DOT(E1xF0,D);
    float E1xF0dW = DOT(E1xF0,W);
    FINDV1(NdF0,E1xF0dD,MdE1,dt,E1xF0dW,T,type,extr,side,AXIS_E1xF0);

    // axis U0+t*E1xF1
    CVec3Dfp32 E1xF1;
    CROSS(E1xF1,E1,F1);
    float E1xF1dD = DOT(E1xF1,D);
    float E1xF1dW = DOT(E1xF1,W);
    FINDV1(NdF1,E1xF1dD,-MdE1,dt,E1xF1dW,T,type,extr,side,AXIS_E1xF1);

    // axis U0+t*E1xF2
    float E1xF2dD = E1xF1dD - E1xF0dD;
    float E1xF2dW = E1xF1dW - E1xF0dW;
    FINDV1(NdF2,E1xF2dD,-MdE1,dt,E1xF2dW,T,type,extr,side,AXIS_E1xF2);

    // axis U0+t*E2xF0
    float MdE2 = MdE1-MdE0;
    float E2xF0dD = E1xF0dD - E0xF0dD;
    float E2xF0dW = E1xF0dW - E0xF0dW;
    FINDV1(NdF0,E2xF0dD,MdE2,dt,E2xF0dW,T,type,extr,side,AXIS_E2xF0);

    // axis U0+t*E2xF1
    float E2xF1dD = E1xF1dD - E0xF1dD;
    float E2xF1dW = E1xF1dW - E0xF1dW;
    FINDV1(NdF1,E2xF1dD,-MdE2,dt,E2xF1dW,T,type,extr,side,AXIS_E2xF1);

    // axis U0+t*E2xF2
    float E2xF2dD = E2xF1dD - E2xF0dD;
    float E2xF2dW = E2xF1dW - E2xF0dW;
    FINDV1(NdF2,E2xF2dD,-MdE2,dt,E2xF2dW,T,type,extr,side,AXIS_E2xF2);

    // determine the point of intersection
    int i;
    float x[2];

    switch ( type )
    {
        case AXIS_N:
        {
            if ( extr == 0 )
            {
                // y0 = 0, y1 = 0
                for (i = 0; i < 3; i++)
                    P[i] = tri1[0][i]+T*V1[i];
            }
            else if ( extr == 1 )
            {
                // y0 = 1, y1 = 0
                for (i = 0; i < 3; i++)
                    P[i] = tri1[0][i]+T*V1[i]+F0[i];
            }
            else  // extr == 2
            {
                // y0 = 0, y1 = 1
                for (i = 0; i < 3; i++)
                    P[i] = tri1[0][i]+T*V1[i]+F1[i];
            }
            break;
        }
        case AXIS_M:
        {
            if ( extr == 0 )
            {
                // x0 = 0, x1 = 0
                for (i = 0; i < 3; i++)
                    P[i] = tri0[0][i]+T*V0[i];
            }
            else if ( extr == 1 )
            {
                // x0 = 1, x1 = 0
                for (i = 0; i < 3; i++)
                    P[i] = tri0[0][i]+T*V0[i]+E0[i];
            }
            else  // extr == 2
            {
                // x0 = 0, x1 = 1
                for (i = 0; i < 3; i++)
                    P[i] = tri0[0][i]+T*V0[i]+E1[i];
            }
            break;
        }
        case AXIS_E0xF0:
        case AXIS_E0xF1:
        case AXIS_E0xF2:
        {
            COMBO(D,D,T,W);
            if ( extr == 0 )
            {
                // x1 = 0
                x[0] = DOT(M,D)/DOT(M,E0);
                for (i = 0; i < 3; i++)
                    P[i] = tri0[0][i]+T*V0[i]+x[0]*E0[i];
            }
            else  // extr == 1
            {
                // x1 = 1, x0 = 0
                for (i = 0; i < 3; i++)
                    P[i] = tri0[0][i]+T*V0[i]+E1[i];
            }
            break;
        }
        case AXIS_E1xF0:
        case AXIS_E1xF1:
        case AXIS_E1xF2:
        {
            COMBO(D,D,T,W);
            if ( extr == 0 )
            {
                // x0 = 0
                x[1] = DOT(M,D)/DOT(M,E1);
                for (i = 0; i < 3; i++)
                    P[i] = tri0[0][i]+T*V0[i]+x[1]*E1[i];
            }
            else  // extr == 1
            {
                // x0 = 1, x1 = 0
                for (i = 0; i < 3; i++)
                    P[i] = tri0[0][i]+T*V0[i]+E0[i];
            }
            break;
        }
        case AXIS_E2xF0:
        case AXIS_E2xF1:
        case AXIS_E2xF2:
        {
            CVec3Dfp32 E2;
            DIFF(E2,E1,E0);

            COMBO(D,D,T,W);
            if ( extr == 0 )
            {
                // x0+x1 = 0 (so x0 = x1 = 0)
                for (i = 0; i < 3; i++)
                    P[i] = tri0[0][i]+T*V0[i];
            }
            else  // extr == 1
            {
                // x0+x1 = 1
                x[0] = (DOT(M,E1)-DOT(M,D))/DOT(M,E2);
                x[1] = 1.0f-x[0];
                for (i = 0; i < 3; i++)
                    P[i] = tri0[0][i]+T*V0[i]+x[0]*E0[i]+x[1]*E1[i];
            }
            break;
        }
    }

    return type;
}

/*
bool Phys_IntersectPolygon_Polygon(
	const CVec3Dfp32* _pV1, const uint32* _piV1, int _nV1, 
	const CVec3Dfp32* _pV2, const uint32* _piV2, int _nV2, CCollisionInfo* _pCollisionInfo)
{
	structured
}
*/

//---------------------------------------------------------------------------
void Phys_CreatePlaneOnPolygon(const CVec3Dfp32& _N, const CVec3Dfp32* _pV, int _nV, CPlane3Dfp32& _Plane);
void Phys_CreatePlaneOnPolygon(const CVec3Dfp32& _N, const CVec3Dfp32* _pV, int _nV, CPlane3Dfp32& _Plane)
{
	MAUTOSTRIP(Phys_CreatePlaneOnPolygon, MAUTOSTRIP_VOID);
	fp32 d = _N * _pV[0];
//	fp32 d = DOT( _N, _pV[0] );
	for(int v = 1; v < _nV; v++)
	{
		fp32 dot = _N * _pV[v];
//		fp32 dot = DOT( _N, _pV[v] );
		if (dot > d) d = dot;
	}

	_Plane.CreateND(_N, -d);
}


bool Phys_GetSeparatingDistance(const CVec3Dfp32* _pV0, int _nV0, const CVec3Dfp32* _pV1, int _nV1, const CVec3Dfp32& _V, const CVec3Dfp32& _N, fp32& _BestD, CPlane3Dfp32& _BestPlane);
bool Phys_GetSeparatingDistance(const CVec3Dfp32* _pV0, int _nV0, const CVec3Dfp32* _pV1, int _nV1, const CVec3Dfp32& _V, const CVec3Dfp32& _N, fp32& _BestD, CPlane3Dfp32& _BestPlane)
{
	MAUTOSTRIP(Phys_GetSeparatingDistance, false);
	if (_N.LengthSqr() < 0.001f) return false;

//	if (M_Fabs(1.0f - _N.Length()) > 0.01f)
//		ConOut("No unit normal: " + _N.GetString());

	bool bModifed = false;
	fp32 Dot = DOT( _V, _N );
	if (M_Fabs(Dot) < 0.001f) return false;

	if (Dot < 0.0f)
	{
		CPlane3Dfp32 P;
		Phys_CreatePlaneOnPolygon(_N, _pV1, _nV1, P);

		fp32 mind = _FP32_MAX;
		for(int v = 0; v < _nV0; v++)
		{
			fp32 d = P.Distance(_pV0[v]);
			if (d < mind) mind = d;
		}

		if (mind < Dot - 0.001f) return false;

		if (mind > _BestD)
		{
			_BestD = mind;
			_BestPlane = P;
			bModifed = true;
		}
	}
	else
	{
		CPlane3Dfp32 P;
		Phys_CreatePlaneOnPolygon(-_N, _pV1, _nV1, P);

		fp32 mind = _FP32_MAX;
		for(int v = 0; v < _nV0; v++)
		{
			fp32 d = P.Distance(_pV0[v]);
			if (d < mind) mind = d;
		}

		if (mind < -Dot - 0.001f) return false;

		if (mind > _BestD)
		{
			_BestD = mind;
			_BestPlane = P;
			bModifed = true;
		}
	}
	return bModifed;
}


bool Phys_IntersectPolygons(const CVec3Dfp32* _pV0, int _nV0, const CVec3Dfp32* _pV1, int _nV1, const CVec3Dfp32& _Velocity1, CCollisionInfo* _pCollisionInfo);
bool Phys_IntersectPolygons(const CVec3Dfp32* _pV0, int _nV0, const CVec3Dfp32* _pV1, int _nV1, const CVec3Dfp32& _Velocity1, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(Phys_IntersectPolygons, false);
	/*
	NOTE:		The first three vertices of both polygons cannot be colinear.

	Return:		False == No hit.
				True == Hit, _pCollisionInfo is initialized if not null.
	*/

	CVec3Dfp32 N0, N1;
	CPlane3Dfp32 P0, P1;
	{
		CVec3Dfp32 e0, e1;
		_pV0[1].Sub(_pV0[0], e0);
		_pV0[2].Sub(_pV0[0], e1);
		e0.CrossProd(e1, N0);
		N0.Normalize();
		P0.CreateNV(N0, _pV0[0]);
	}

	{
			int Mask1 = P0.GetArrayPlaneSideMask(_pV1, _nV1);
			if (Mask1 != 3)
				Mask1 = Mask1;
	}

	fp32 BestD = -100001.0f;
	int PlaneNr = -1;
	if (Phys_GetSeparatingDistance(_pV0, _nV0, _pV1, _nV1, _Velocity1, N0, BestD, _pCollisionInfo->m_Plane)) PlaneNr = 1;
	if (BestD > -0.001f)
		return false;

	{
		CVec3Dfp32 e0, e1;
		_pV1[1].Sub(_pV1[0], e0);
		_pV1[2].Sub(_pV1[0], e1);
		e0.CrossProd(e1, N1);
		N1.Normalize();
		P1.CreateNV(N1, _pV1[0]);
	}

	{
			int Mask0 = P1.GetArrayPlaneSideMask(_pV0, _nV0);
			if (Mask0 != 3)
				Mask0 = Mask0;
	}

	if(_pCollisionInfo)
	{
		//Bluff och båg!
		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_bIsCollision = true;
		_pCollisionInfo->m_Plane = P1;
		int iMin = -1;
		fp32 Min = _FP32_MAX;
		for(int i = 0; i < _nV0; i++)
		{
			fp32 D = P1.Distance(_pV0[i]);
			if(D < Min)
			{
				Min = D;
				iMin = i;
			}
		}
		M_ASSERT(iMin >= 0, "Illegal to collide with something built from 0 vertices");
		_pCollisionInfo->m_Pos = _pV0[iMin] - _pCollisionInfo->m_Plane.n * _pCollisionInfo->m_Plane.Distance(_pV0[iMin]);
	}

	if (Phys_GetSeparatingDistance(_pV0, _nV0, _pV1, _nV1, _Velocity1, N1, BestD, _pCollisionInfo->m_Plane)) PlaneNr = 2;
	if (BestD > -0.001f)
		return false;

	int v00 = _nV0-1;
	for(int v01 = 1; v01 < _nV0; v01++)
	{
		int v10 = _nV1-1;
		for(int v11 = 1; v11 < _nV1; v11++)
		{
			CVec3Dfp32 E0, E1, N;
			_pV0[v00].Sub(_pV0[v01], E0);
			_pV1[v10].Sub(_pV1[v11], E1);
			E0.CrossProd(E1, N);
			N.Normalize();
			if (Phys_GetSeparatingDistance(_pV0, _nV0, _pV1, _nV1, _Velocity1, N, BestD, _pCollisionInfo->m_Plane)) PlaneNr = 3;
			if (BestD > -0.001f) return false;
		}
	}

	if (!_pCollisionInfo) return true;

	CVec3Dfp32 V1Center(0);
	for(int v1 = 0; v1 < _nV1; v1++)
		V1Center += _pV1[v1];

	V1Center *= 1.0f / _nV1;

	_pCollisionInfo->m_bIsValid = true;
	_pCollisionInfo->m_bIsCollision = true;
	_pCollisionInfo->m_Velocity = 0;
	_pCollisionInfo->m_Distance = BestD;
	_pCollisionInfo->m_Pos = V1Center - _pCollisionInfo->m_Plane.n * _pCollisionInfo->m_Plane.Distance(V1Center);
	_pCollisionInfo->m_LocalPos = _pCollisionInfo->m_Pos;

	fp32 vProj = _pCollisionInfo->m_Plane.n * _Velocity1;
	if (vProj < -0.001f)
		_pCollisionInfo->m_Time = Clamp01(1.0f - _pCollisionInfo->m_Distance / vProj);
	else
	{
		ConOut(CStrF("Bad velocity projection dV %s, N %s", (char*) _Velocity1.GetString(), (char*)_pCollisionInfo->m_Plane.n.GetString()));
		_pCollisionInfo->m_Time = 1;
	}


	// !!!!!!!!!!! VERY INCOMPLETE !!!!!!!!!!!!
/*	CVec3Dfp32 Tri0[3];
	CVec3Dfp32 Tri1[3];
	Tri[0] = _pV0[0];
	Tri[1] = _pV1[1];

	for(int v0 = 1; v0 < _nV0-1; v0++)
		for(int v1 = 1; v1 < _nV1-1; v1++)
		{
			Tri0[1] = _pV0[v0];
			Tri0[2] = _pV0[v0+1];
			Tri1[1] = _pV1[v1];
			Tri1[2] = _pV1[v1+1];

			TestIntersectionS(Tri0, Tri1);
		}
*/
	return true;
}



