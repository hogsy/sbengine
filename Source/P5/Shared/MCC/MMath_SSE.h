
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

MCCDLLEXPORT void SSE_Multiply_McMc_4x4(const CMat4Dfp32& _Src1, const CMat4Dfp32& _Src2, CMat4Dfp32& _Dst);
MCCDLLEXPORT void SSE_Multiply_McMc_4x3on4x4(const CMat4Dfp32& _Src1, const CMat4Dfp32& _Src2, CMat4Dfp32& _Dst);
MCCDLLEXPORT void SSE_Multiply_McMc_4x3(const TMatrix43<fp32>& _Src1, const TMatrix43<fp32>& _Src2, TMatrix43<fp32>& _Dst);

MCCDLLEXPORT void SSE_Multiply_VcMc_4x3(const TVector3Aggr<fp32>& _Src1, const CMat4Dfp32& _Mat, TVector3Aggr<fp32>& _Dst);

MCCDLLEXPORT void SSE_Box3Dfp32_And(CBox3Dfp32& _Target, const CBox3Dfp32& _Src);
MCCDLLEXPORT void SSE_Box3Dfp32_And(const CBox3Dfp32& _Src1, const CBox3Dfp32& _Src2, CBox3Dfp32& _Target);
MCCDLLEXPORT void SSE_Box3Dfp32_Expand(CBox3Dfp32& _Target, const CBox3Dfp32& _Src);
MCCDLLEXPORT void SSE_Box3Dfp32_Expand(CBox3Dfp32& _Target, const TVector3Aggr<fp32>& _Src);

MCCDLLEXPORT bool SSE_Box3Dfp32_IsInside(const CBox3Dfp32& _Src0, const CBox3Dfp32& _Src1);


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMat4Dfp32 Overloads
|__________________________________________________________________________________________________
\*************************************************************************************************/
void TMatrix43<fp32>::Multiply(const TMatrix43<fp32>& _Src, TMatrix43<fp32>& _Dest) const
{
	SSE_Multiply_McMc_4x3(*this, _Src, _Dest);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CVec3Dfp32 Overloads
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CBox3Dfp32::And(const CBox3Dfp32& _Box)
{
	SSE_Box3Dfp32_And(*this, _Box);
}

void CBox3Dfp32::And(const CBox3Dfp32& _Box, CBox3Dfp32& _Dst) const
{
	SSE_Box3Dfp32_And(*this, _Box, _Dst);
}

void CBox3Dfp32::Expand(const CBox3Dfp32& _Box)
{
	SSE_Box3Dfp32_Expand(*this, _Box);
}

void CBox3Dfp32::Expand(const CVec3Dfp32& _v)
{
	SSE_Box3Dfp32_Expand(*this, _v);
}

bool CBox3Dfp32::IsInside(const CBox3Dfp32& _Box) const
{
	return SSE_Box3Dfp32_IsInside(*this, _Box);
}

#endif
#endif // __INC_MMATH_SSE
