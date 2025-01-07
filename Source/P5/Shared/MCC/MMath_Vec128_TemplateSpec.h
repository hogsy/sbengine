/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			vec128 template specialisation

Author:			Magnus Högdahl

Copyright:		Starbreeze AB, 2006

History:
				2006-07-05:		Created file.

Comments:


\*____________________________________________________________________________*/

#ifndef __INC_MMATH_VEC128_TEMPLATESPEC
#define __INC_MMATH_VEC128_TEMPLATESPEC

#include "MMath_Vec128.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMat4Dfp32 Overloads
|__________________________________________________________________________________________________
\*************************************************************************************************/
template <>
M_FORCEINLINE void CMat4Dfp32::Multiply(const CMat4Dfp32& _Src, CMat4Dfp32& _Dest) const
{
	M_VMatMul(*this, _Src, _Dest);
};

template <>
M_FORCEINLINE void CMat4Dfp32::Multiply3x3(const CMat4Dfp32& _Src, CMat4Dfp32& _Dest) const
{
	M_VMatMul3x3(*this, _Src, _Dest);
};


template <>
M_FORCEINLINE void CMat4Dfp32::Transpose(CMat4Dfp32& _DestMat) const
{
	vec128 x = r[0];
	vec128 y = r[1];
	vec128 z = r[2];
	vec128 w = r[3];
	M_VTranspose4x4(x, y, z, w);
	_DestMat.r[0] = x;
	_DestMat.r[1] = y;
	_DestMat.r[2] = z;
	_DestMat.r[3] = w;
}


M_FORCEINLINE vec128 GetNonColinearWith(vec128 _Locked, vec128 _Alt0, vec128 _Alt1)
{
	vec128 xp=M_VXpd(_Locked, _Alt0);
	vec128 dp=M_VDp3(xp,xp);
	vec128 Invalid=M_VCmpGTMsk(dp,M_VScalar(0));
	return M_VSelMsk(Invalid,_Alt0,_Alt1);
}


template <class fp32> template<int _PrioRow0,int _PrioRow1>
void M_FORCEINLINE TMatrix4<fp32>::RecreateMatrix()
{
/*	
	CMat4Dfp32 m=*this;
	m.RecreateMatrix(_PrioRow0,_PrioRow1);
*/	
	// The matrix normalizes the _Priority0 row, then uses the _Priority1 row to create an orthogonal matrix.
	// The information in the third row (not _Priority0 and not _Priority1) is completely ignored.
	enum { Missing = 3 - (_PrioRow0 + _PrioRow1) };
	enum { iType = _PrioRow0  - _PrioRow1 };
	M_STATIC_ASSERT(_PrioRow0 >= 0 && _PrioRow1 <= 2);

	vec128 r0 = r[_PrioRow0];
	vec128 r1 = r[_PrioRow1];
	vec128 rm = M_VXpd(r1, r0);
	vec128 tmp = M_VMrgXY(M_VDp3(r0, r0), M_VDp3(rm, rm));
	vec128 zero = M_VZero();

	vec128 InvalidRM = M_VCmpGTMsk(M_VDp3(rm, rm), zero);
	tmp = M_VRsq(tmp);
	
	r0 = M_VMul(r0, M_VSplatX(tmp));
	rm = M_VMul(rm, M_VSplatY(tmp));
	rm = M_VSelMsk(InvalidRM, rm, zero);

	if (iType == -1 || iType == 2)
	{
		rm = M_VNeg(M_VSetW0(rm));
		r1 = M_VXpd(rm, r0);
	}
	else
	{
		r1 = M_VXpd(r0, rm);
	}
	r[_PrioRow0] = M_VSetW0(r0);
	r[_PrioRow1] = M_VSetW0(r1);
	r[Missing] = rm;
/*
	CVec4Dfp32 x=M_VSub(m.r[0],r[0]);
	x=M_VAdd(x,M_VSub(m.r[1],r[1]));
	x=M_VAdd(x,M_VSub(m.r[2],r[2]));
	x=M_VAdd(x,M_VSub(m.r[3],r[3]));
	x=M_VDp4(x,x);
	if (x.k[0]>0.01f)
		M_BREAKPOINT;
	*this=m;
*/
}


template <>
M_FORCEINLINE void CMat4Dfp32::Unit()
{
	r[0] = M_VConst(1.0f,0.0f,0.0f,0.0f);
	r[1] = M_VConst(0.0f,1.0f,0.0f,0.0f);
	r[2] = M_VConst(0.0f,0.0f,1.0f,0.0f);
	r[3] = M_VConst(0.0f,0.0f,0.0f,1.0f);
};

template <>
M_FORCEINLINE void CMat4Dfp32::CreateTranslation(const TVector3Aggr<fp32>& _Translation)
{
	r[0] = M_VConst(1.0f,0.0f,0.0f,0.0f);
	r[1] = M_VConst(0.0f,1.0f,0.0f,0.0f);
	r[2] = M_VConst(0.0f,0.0f,1.0f,0.0f);
	r[3] = M_VLd_P3_Slow(&_Translation);
}

template <>
M_FORCEINLINE void CMat4Dfp32::CreateTranslation(const TVector4<fp32>& _Translation)
{
	r[0] = M_VConst(1.0f,0.0f,0.0f,0.0f);
	r[1] = M_VConst(0.0f,1.0f,0.0f,0.0f);
	r[2] = M_VConst(0.0f,0.0f,1.0f,0.0f);
	const vec128 one = M_VOne();
	const vec128 m = M_VConstMsk(1, 1, 1, 0);
	r[3] = M_VSelMsk(m, _Translation, one);
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TVector3Aggr<fp32> Overloads
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*
template <>
static void TVector3Aggr<fp32>::MultiplyMatrix(const TVector3Aggr<fp32>* _pSrc, TVector3Aggr<fp32>* _pDest, const CMat4Dfp32& _Mat, int n)
{
	CMat4Dfp32 M = _Mat;
	for(int i = 0; i < n; i++)
	{
		vec128 v = M_VLd_P3_Slow(_pSrc+i);
		vec128 vt = M_VMulMat4x3(v, M);
		M_VSt_V3_Slow(vt, _pDest+i);
	}
}

template <>
void TVector3Aggr<fp32>::operator*= (const CMat4Dfp32& _Mat)
{
	vec128 v = M_VLd_P3_Slow(this);
	vec128 vt = M_VMulMat4x3(v, _Mat);
	M_VSt_V3_Slow(vt, this);
}

template <>
void TVector3Aggr<fp32>::MultiplyMatrix(const CMat4Dfp32& _Mat)
{
	vec128 v = M_VLd_P3_Slow(this);
	vec128 vt = M_VMulMat4x3(v, _Mat);
	M_VSt_V3_Slow(vt, this);
}

template <>
void TVector3Aggr<fp32>::MultiplyMatrix(const CMat4Dfp32& _Mat, TVector3Aggr<fp32>& _Dst) const
{
	vec128 v = M_VLd_P3_Slow(this);
	vec128 vt = M_VMulMat4x3(v, _Mat);
	M_VSt_V3_Slow(vt, &_Dst);
}
*/


#endif // __INC_MMATH_VEC128_TEMPLATESPEC
