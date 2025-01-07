#ifndef ModelsMisc_h
#define ModelsMisc_h

#include "PCH.h"
#include "MFloat.h"

//----------------------------------------------------------------------

static M_INLINE void AnglesToVector(fp32 a, fp32 b, CVec3Dfp32& v)
{
	a *= 2.0f * _PI;
	b *= 2.0f * _PI;
	const fp32 sa = M_Sin( a );
	const fp32 ca = M_Cos( a );
	const fp32 sb = M_Sin( b );
	const fp32 cb = M_Cos( b );
	v.k[0] = ca * cb;
	v.k[1] = sa * cb;
	v.k[2] = sb;
//	v.k[0] = cosf(a) * cosf(b);
//	v.k[1] = sinf(a) * cosf(b);
//	v.k[2] = sinf(b);
}

//----------------------------------------------------------------------

#define TESTNEWPARTICLES

//----------------------------------------------------------------------

// 0.0 .. 1.0
static M_INLINE fp32 GetRand(uint32& _Randseed)
{
	return MFloat_GetRand(_Randseed++);
}

static M_INLINE const fp32 GetRandConst( const uint32 _Randseed )
{
	return MFloat_GetRand(_Randseed);
}

// -0.5 .. +0.5
static M_INLINE fp32 GetRandS(uint32& _Randseed)
{
	return (GetRand(_Randseed) - 0.5f);
}

// -1.0 .. +1.0
static M_INLINE fp32 GetRandS2(uint32& _Randseed)
{
	return (2.0f * GetRandS(_Randseed));
}

//----------------------------------------------------------------------

#endif /* ModelMisc_h */
