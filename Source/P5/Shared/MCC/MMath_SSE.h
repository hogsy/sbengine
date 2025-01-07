
#ifndef __INC_MMATH_SSE
#define __INC_MMATH_SSE

#if defined CPU_X86 && (((CPU_FEATURES_FORCE) & CPU_FEATURE_SSE) != 0)

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			SSE overrides for some vector operations.

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios AB 1996,2003

	Contents:

	Comments:

	History:	
		030711:		File header added

\*____________________________________________________________________________________________*/


#include "MMath.h"

MCCDLLEXPORT void SSE_Multiply_McMc_4x4(const CMat4Dfp4& _Src1, const CMat4Dfp4& _Src2, CMat4Dfp4& _Dst);
MCCDLLEXPORT void SSE_Multiply_McMc_4x3on4x4(const CMat4Dfp4& _Src1, const CMat4Dfp4& _Src2, CMat4Dfp4& _Dst);
MCCDLLEXPORT void SSE_Multiply_McMc_4x3(const TMatrix43<fp4>& _Src1, const TMatrix43<fp4>& _Src2, TMatrix43<fp4>& _Dst);

MCCDLLEXPORT void SSE_Multiply_VcMc_4x3(const TVector3Aggr<fp4>& _Src1, const CMat4Dfp4& _Mat, TVector3Aggr<fp4>& _Dst);

MCCDLLEXPORT void SSE_Box3Dfp4_And(CBox3Dfp4& _Target, const CBox3Dfp4& _Src);
MCCDLLEXPORT void SSE_Box3Dfp4_And(const CBox3Dfp4& _Src1, const CBox3Dfp4& _Src2, CBox3Dfp4& _Target);
MCCDLLEXPORT void SSE_Box3Dfp4_Expand(CBox3Dfp4& _Target, const CBox3Dfp4& _Src);
MCCDLLEXPORT void SSE_Box3Dfp4_Expand(CBox3Dfp4& _Target, const TVector3Aggr<fp4>& _Src);

MCCDLLEXPORT bool SSE_Box3Dfp4_IsInside(const CBox3Dfp4& _Src0, const CBox3Dfp4& _Src1);


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMat4Dfp4 Overloads
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CMat4Dfp4::Multiply(const CMat4Dfp4& _Src, CMat4Dfp4& _Dest) const
{
	SSE_Multiply_McMc_4x4(*this, _Src, _Dest);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMat43fp4 Overloads
|__________________________________________________________________________________________________
\*************************************************************************************************/
void TMatrix43<fp4>::Multiply(const TMatrix43<fp4>& _Src, TMatrix43<fp4>& _Dest) const
{
	SSE_Multiply_McMc_4x3(*this, _Src, _Dest);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CVec3Dfp4 Overloads
|__________________________________________________________________________________________________
\*************************************************************************************************/
void TVector3Aggr<fp4>::operator*= (const CMat4Dfp4& _Mat)
{
	SSE_Multiply_VcMc_4x3(*this, _Mat, *this);
}

void TVector3Aggr<fp4>::MultiplyMatrix(const CMat4Dfp4& _Mat, TVector3Aggr<fp4>& _Dst) const
{
	SSE_Multiply_VcMc_4x3(*this, _Mat, _Dst);
}

void TVector3Aggr<fp4>::MultiplyMatrix(const CMat4Dfp4& _Mat)
{
	SSE_Multiply_VcMc_4x3(*this, _Mat, *this);
}

void CBox3Dfp4::And(const CBox3Dfp4& _Box)
{
	SSE_Box3Dfp4_And(*this, _Box);
}

void CBox3Dfp4::And(const CBox3Dfp4& _Box, CBox3Dfp4& _Dst) const
{
	SSE_Box3Dfp4_And(*this, _Box, _Dst);
}

void CBox3Dfp4::Expand(const CBox3Dfp4& _Box)
{
	SSE_Box3Dfp4_Expand(*this, _Box);
}

void CBox3Dfp4::Expand(const CVec3Dfp4& _v)
{
	SSE_Box3Dfp4_Expand(*this, _v);
}

bool CBox3Dfp4::IsInside(const CBox3Dfp4& _Box) const
{
	return SSE_Box3Dfp4_IsInside(*this, _Box);
}

#elif defined(PLATFORM_XENON)

#include "xtl.h"
#include "xboxmath.h"
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMat4Dfp4 Overloads
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CMat4Dfp4::Multiply(const CMat4Dfp4& _Src, CMat4Dfp4& _Dest) const
{
	if (((mint(&_Src) | mint(&_Dest) | mint(this)) & 0xf))
	{
		XMMATRIX Source;
		memcpy(&Source, &_Src, sizeof(XMMATRIX));
		XMMATRIX This;
		memcpy(&This, this, sizeof(XMMATRIX));
		XMMATRIX Dest = XMMatrixMultiply(This, Source);
		memcpy(&_Dest, &Dest, sizeof(XMMATRIX));
	}
	else
		(XMMATRIX&)_Dest = XMMatrixMultiply((const XMMATRIX&)(*this), (const XMMATRIX&)_Src);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMat43fp4 Overloads
|__________________________________________________________________________________________________
\*************************************************************************************************/

#if 0
void TMatrix43<fp4>::Multiply(const TMatrix43<fp4>& m, TMatrix43<fp4>& DestMat) const
{

	XMVECTOR M00, M01, M02, M03,
             M10, M11, M12, M13,
             M20, M21, M22, M23,
             M30, M31, M32, M33;
    XMVECTOR M00M02, M01M03, M10M12, M11M13,
             M20M22, M21M23, M30M32, M31M33;
    XMMATRIX M2T, P;
    XMMATRIX Result;

    P.r[0] = __vmrghw(M2.r[0], M2.r[2]);
    P.r[1] = __vmrghw(M2.r[1], M2.r[3]);
    P.r[2] = __vmrglw(M2.r[0], M2.r[2]);
    P.r[3] = __vmrglw(M2.r[1], M2.r[3]);

    M2T.r[0] = __vmrghw(P.r[0], P.r[1]);
    M2T.r[2] = __vmrghw(P.r[2], P.r[3]);
    M2T.r[1] = __vmrglw(P.r[0], P.r[1]);
    M2T.r[3] = __vmrglw(P.r[2], P.r[3]);

    M00 = __vmsum4fp(M1.r[0], M2T.r[0]);
    M02 = __vmsum4fp(M1.r[0], M2T.r[2]);
    M01 = __vmsum4fp(M1.r[0], M2T.r[1]);
    M03 = __vmsum4fp(M1.r[0], M2T.r[3]);

    M10 = __vmsum4fp(M1.r[1], M2T.r[0]);
    M12 = __vmsum4fp(M1.r[1], M2T.r[2]);
    M11 = __vmsum4fp(M1.r[1], M2T.r[1]);
    M13 = __vmsum4fp(M1.r[1], M2T.r[3]);

    M20 = __vmsum4fp(M1.r[2], M2T.r[0]);
    M22 = __vmsum4fp(M1.r[2], M2T.r[2]);
    M21 = __vmsum4fp(M1.r[2], M2T.r[1]);
    M23 = __vmsum4fp(M1.r[2], M2T.r[3]);

    M30 = __vmsum4fp(M1.r[3], M2T.r[0]);
    M32 = __vmsum4fp(M1.r[3], M2T.r[2]);
    M31 = __vmsum4fp(M1.r[3], M2T.r[1]);
    M33 = __vmsum4fp(M1.r[3], M2T.r[3]);

    M00M02 = __vmrghw(M00, M02);
    M01M03 = __vmrghw(M01, M03);
    M10M12 = __vmrghw(M10, M12);
    M11M13 = __vmrghw(M11, M13);
    M20M22 = __vmrghw(M20, M22);
    M21M23 = __vmrghw(M21, M23);
    M30M32 = __vmrghw(M30, M32);
    M31M33 = __vmrghw(M31, M33);

    Result.r[0] = __vmrghw(M00M02, M01M03);
    Result.r[1] = __vmrghw(M10M12, M11M13);
    Result.r[2] = __vmrghw(M20M22, M21M23);
    Result.r[3] = __vmrghw(M30M32, M31M33);

    return Result;

}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CVec3Dfp4 Overloads
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CVec3Dfp4::operator*= (const CMat4Dfp4& _Mat)
{
	XMVECTOR Src;
	Src.x = k[0];
	Src.y = k[1];
	Src.z = k[2];
	XMVECTOR Dest = XMVector3Transform(Src, (const XMMATRIX&)_Mat);
	k[0] = Dest.x;
	k[1] = Dest.y;
	k[2] = Dest.z;
}

void CVec3Dfp4::MultiplyMatrix(const CMat4Dfp4& _Mat, CVec3Dfp4& _Dst) const
{
	XMVECTOR Src;
	Src.x = k[0];
	Src.y = k[1];
	Src.z = k[2];
	XMVECTOR Dest = XMVector3Transform(Src, (const XMMATRIX&)_Mat);
	_Dst.k[0] = Dest.x;
	_Dst.k[1] = Dest.y;
	_Dst.k[2] = Dest.z;
}

void CVec3Dfp4::MultiplyMatrix(const CMat4Dfp4& _Mat)
{
	XMVECTOR Src;
	Src.x = k[0];
	Src.y = k[1];
	Src.z = k[2];
	XMVECTOR Dest = XMVector3Transform(Src, (const XMMATRIX&)_Mat);
	k[0] = Dest.x;
	k[1] = Dest.y;
	k[2] = Dest.z;
}

#endif

#endif
#endif
