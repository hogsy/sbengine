#include "PCH.h"

#include "MFloat.h"
#include "MMath.h"

const fp32 MIPS_Sin( fp32 _Val )
{
	const float onepi = _PI;
	const float twopi = _PI2;
	const float halfpi = _PIHALF;

	float sign;

	sign = 1.f;
	if( _Val < 0.f)
	{ // Sin is an Odd funtion
		sign = -sign;
		_Val = -_Val;
	}

	// modulo 2 * pi
	float x1 = _Val - ( (int)( _Val *(1.0f/twopi)) ) * twopi;

	if( x1 > onepi )
	{ // within pi
		sign = -sign;
		x1 -= onepi;
	}

	if( x1 > halfpi )
	{ // within pi / 2 
		x1 = onepi - x1;
	}

	const float s2 = -0.16666666666666666666666666666667f;
	const float s3 = 0.0083333333333333333333333333333333f;
	const float s4 = -0.0001984126984126984126984126984127f;
	const float s5 = 2.7557319223985890652557319223986e-6f;

	float x2 = x1 * x1;
	float x3 = x2 * x1;
	float x5 = x2 * x3;
	float x7 = x2 * x5;
	float x9 = x2 * x7;

	return sign * (x1 + s2 * x3 + s3 * x5 + s4 * x7 + s5 * x9);
}



