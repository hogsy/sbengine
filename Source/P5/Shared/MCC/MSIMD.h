
#ifndef _INC_MSIMD
#define _INC_MSIMD

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Vector operations

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios AB 1996,2003

	Contents:

	Comments:

		// ----------------------------------------------------------------------
		Operations:

		Mul:				Dst = Src1 * Src2
		Combine:			Dst = Src1 + Src2 * Src3

		// ----------------------------------------------------------------------
		Source type:

		Vector (CVec4Dfp4)	V
		Scalar (fp4)		S
		Matrix (CMat4Dfp4)	M

		// ----------------------------------------------------------------------
		Source suffixes:

		Array				<none>
		Constant			c
		Random [-0.5..0.5]	r		(Source data is always uint32 randomseeds regardless of source postfix type)

		// ----------------------------------------------------------------------
		NOTES:

		- Array pointers must be 16-byte aligned.
		- Constants must be dword aligned		


	History:	
		030711:		File header added

\*____________________________________________________________________________________________*/


#include "MMath.h"


void MCCDLLEXPORT SIMD_Add_VVc(int _nV, const CVec4Dfp4* _pSrc1, const CVec4Dfp4& _Src2, CVec4Dfp4* _pDst );
void MCCDLLEXPORT SIMD_Set_V(int _nV, const CVec4Dfp4* _pSrc1, CVec4Dfp4* _pDst );
void MCCDLLEXPORT SIMD_Set_Vc(int _nV, const CVec4Dfp4& _Src1, CVec4Dfp4* _pDst );
void MCCDLLEXPORT SIMD_Mul_VS(int _nV, const CVec4Dfp4* _pSrc1, const fp4* _pSrc2, CVec4Dfp4* _pDst);
void MCCDLLEXPORT SIMD_Mul_VSc( int _nV, const CVec4Dfp4* _pSrc1, const fp4& _Src2, CVec4Dfp4* _pDst );
void MCCDLLEXPORT SIMD_Combine_SSS(int _nV, const fp4* _pSrc1, const fp4* _pSrc2, const fp4* _pSrc3, fp4* _pDst);
void MCCDLLEXPORT SIMD_Combine_VVS(int _nV, const CVec4Dfp4* _pSrc1, const CVec4Dfp4* _pSrc2, const fp4* _pSrc3, CVec4Dfp4* _pDst);
void MCCDLLEXPORT SIMD_Combine_VcVcVr(int _nV, const CVec4Dfp4& _Src1, const CVec4Dfp4& _Src2, uint32* _pRandSeed, CVec4Dfp4* _pDst);
void MCCDLLEXPORT SIMD_Combine_VcVcSr(int _nV, const CVec4Dfp4& _Src1, const CVec4Dfp4& _Src2, uint32* _pRandSeed, CVec4Dfp4* _pDst);
void MCCDLLEXPORT SIMD_Combine_ScScSr(int _nV, fp4 _Src1, fp4 _Src2, uint32* _pRandSeed, fp4* _pDst);
void MCCDLLEXPORT SIMD_Mul_VMc(int _nV, const CVec4Dfp4* _pSrc1, const CMat43fp4& _Mat, CVec4Dfp4* _pDst);
void MCCDLLEXPORT SIMD_Combine_VVMc(int _nV, const CVec4Dfp4* _pSrc1, const CVec4Dfp4* _pSrc2, const CMat4Dfp4& _Mat, CVec4Dfp4* _pDst);

void MCCDLLEXPORT SIMD_ParticleQuad_VSVcVc_V3(int _nV, const CVec4Dfp4* _pSrc1, const fp4* _pSrc2, const CVec4Dfp4& _Src3, const CVec4Dfp4& _Src4, CVec3Dfp4* _pDst);
void MCCDLLEXPORT SIMD_ParticleTriangle_VSVcVc_V3(int _nV, const CVec4Dfp4* _pSrc1, const fp4* _pSrc2, const CVec4Dfp4& _Src3, const CVec4Dfp4& _Src4, CVec3Dfp4* _pDst);

//----------------------------------------------------------------
// Approximations with 8-bit precision
//----------------------------------------------------------------
void MCCDLLEXPORT SIMD_Recp8(fp4* _pSrc, fp4* _pDst, int _n);												// All pointers must be 16-byte aligned, target must be 16-floats padded.
void MCCDLLEXPORT SIMD_Sqrt8(fp4* _pSrc, fp4* _pDst, int _n);												// All pointers must be 16-byte aligned, target must be 16-floats padded.
void MCCDLLEXPORT SIMD_RecpSqrt8(fp4* _pSrc, fp4* _pDst, int _n);											// All pointers must be 16-byte aligned, target must be 16-floats padded.

//----------------------------------------------------------------
// Approximations with 12-bit precision
//----------------------------------------------------------------
void MCCDLLEXPORT SIMD_Recp12(fp4* _pSrc, fp4* _pDst, int _n);												// All pointers must be 16-byte aligned, target must be 16-floats padded.
void MCCDLLEXPORT SIMD_Sqrt12(fp4* _pSrc, fp4* _pDst, int _n);												// All pointers must be 16-byte aligned, target must be 16-floats padded.
void MCCDLLEXPORT SIMD_RecpSqrt12(fp4* _pSrc, fp4* _pDst, int _n);											// All pointers must be 16-byte aligned, target must be 16-floats padded.

//----------------------------------------------------------------
// Color convertion
//----------------------------------------------------------------
void MCCDLLEXPORT SIMD_ConvertRGB(const CVec4Dfp4* _pLight, uint32* _pColors, int _nV, int _Alpha);			// _pLight must be 16-byte aligned
void MCCDLLEXPORT SIMD_ConvertRGBA(const CVec4Dfp4* _pLight, uint32* _pColors, int _nV);					// _pLight must be 16-byte aligned

//----------------------------------------------------------------
// Vector operations
//----------------------------------------------------------------
void MCCDLLEXPORT SIMD_Add(const fp4* _pSrc1, const fp4* _pSrc2, fp4* _pDst, int _n);						// All pointers must be 16-byte aligned, target must be 4-floats padded.
void MCCDLLEXPORT SIMD_Sub(const fp4* _pSrc1, const fp4* _pSrc2, fp4* _pDst, int _n);						// All pointers must be 16-byte aligned, target must be 4-floats padded.
void MCCDLLEXPORT SIMD_Mul(const fp4* _pSrc1, const fp4* _pSrc2, fp4* _pDst, int _n);						// All pointers must be 16-byte aligned, target must be 4-floats padded.

void MCCDLLEXPORT SIMD_Multiply(const CMat4Dfp4* _pSrc1, const CMat4Dfp4* _pSrc2, CMat4Dfp4* _pDst);		// All pointers must be 16-byte aligned
void MCCDLLEXPORT SIMD_Multiply(const CMat4Dfp4* _pSrc1, const CVec4Dfp4* _pSrc2, CVec4Dfp4* _pDst, int _nV);// All pointers must be 16-byte aligned

void MCCDLLEXPORT SIMD_Multiply(const CMat43fp4* _pSrc1, const CMat43fp4* _pSrc2, CMat43fp4* _pDst);		// All pointers must be 16-byte aligned
void MCCDLLEXPORT SIMD_Multiply(const CMat43fp4* _pSrc1, const CVec4Dfp4* _pSrc2, CVec4Dfp4* _pDst, int _nV);// All pointers must be 16-byte aligned

int MCCDLLEXPORT SIMD_Max(const uint8* _pSrc, int _nValues, int _CurMax);
int MCCDLLEXPORT SIMD_Max(const int16* _pSrc, int _nValues, int _CurMax);
int MCCDLLEXPORT SIMD_Max(const uint16* _pSrc, int _nValues, int _CurMax);


#endif // _INC_MSIMD
