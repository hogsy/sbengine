/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			SSE optimizations for CMat4Dfp4

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios AB 2002
					
	History:		
		020401:		Created File
\*____________________________________________________________________________________________*/

#include "PCH.h"
#include "MMath.h"

#if defined(CPU_X86)

MCCDLLEXPORT void SSE_Multiply_McMc_4x4(const CMat4Dfp4& _Src1, const CMat4Dfp4& _Src2, CMat4Dfp4& _Dst)
{
	// Athlon: ~114c
	// Pentium 3: ~104c
	__asm
	{
		mov edx, [_Src1]
		mov eax, [_Dst]
		mov ecx, [_Src2]

		movss xmm0, [edx]
		movups xmm1, [ecx]
		shufps xmm0, xmm0, 0
		movss xmm2, [edx+4]
		mulps xmm0, xmm1
		shufps xmm2, xmm2, 0
		movups xmm3, [ecx+10h]
		movss xmm7, [edx+8]
		mulps xmm2, xmm3
		shufps xmm7, xmm7, 0
		addps xmm0, xmm2
		movups xmm4, [ecx+20h]
		movss xmm2, [edx+0Ch]
		mulps xmm7, xmm4
		shufps xmm2, xmm2, 0
		addps xmm0, xmm7
		movups xmm5, [ecx+30h]
		movss xmm6, [edx+10h]
		mulps xmm2, xmm5
		movss xmm7, [edx+14h]
		shufps xmm6, xmm6, 0
		addps xmm0, xmm2
		shufps xmm7, xmm7, 0
		movlps qword ptr [eax], xmm0
		movhps qword ptr [eax+8], xmm0
		mulps xmm7, xmm3
		movss xmm0, [edx+18h]
		mulps xmm6, xmm1
		shufps xmm0, xmm0, 0
		addps xmm6, xmm7
		mulps xmm0, xmm4
		movss xmm2, [edx+24h]
		addps xmm6, xmm0
		movss xmm0, [edx+1Ch]
		movss xmm7, [edx+20h]
		shufps xmm0, xmm0, 0
		shufps xmm7, xmm7, 0
		mulps xmm0, xmm5
		mulps xmm7, xmm1
		addps xmm6, xmm0
		shufps xmm2, xmm2, 0
		movlps qword ptr [eax+10h], xmm6
		movhps qword ptr [eax+18h], xmm6
		mulps xmm2, xmm3
		movss xmm6, [edx+28h]
		addps xmm7, xmm2
		shufps xmm6, xmm6, 0
		movss xmm2, [edx+2Ch]
		mulps xmm6, xmm4
		shufps xmm2, xmm2, 0
		addps xmm7, xmm6
		mulps xmm2, xmm5
		movss xmm0, [edx+34h]
		addps xmm7, xmm2
		shufps xmm0, xmm0, 0
		movlps qword ptr [eax+20h], xmm7
		movss xmm2, [edx+30h]
		movhps qword ptr [eax+28h], xmm7
		mulps xmm0, xmm3
		shufps xmm2, xmm2, 0
		movss xmm6, [edx+38h]
		mulps xmm2, xmm1
		shufps xmm6, xmm6, 0
		addps xmm2, xmm0
		mulps xmm6, xmm4
		movss xmm7, [edx+3Ch]
		shufps xmm7, xmm7, 0
		addps xmm2, xmm6
		mulps xmm7, xmm5
		addps xmm2, xmm7
		movups [eax+30h], xmm2
	}
}

#define MAT44_ROWSIZE 16

MCCDLLEXPORT void SSE_Multiply_McMc_4x3on4x4(const CMat4Dfp4& _Src1, const CMat4Dfp4& _Src2, CMat4Dfp4& _Dst)
{
	__asm
	{
		mov edx, [_Src1]
		mov eax, [_Dst]
		mov ecx, [_Src2]

		movss xmm0, [edx]
		movups xmm1, [ecx]
		shufps xmm0, xmm0, 0
		movss xmm2, [edx+4]
		mulps xmm0, xmm1
		shufps xmm2, xmm2, 0
		movups xmm3, [ecx+MAT44_ROWSIZE]
		movss xmm7, [edx+8]
		mulps xmm2, xmm3
		shufps xmm7, xmm7, 0
		addps xmm0, xmm2
		movups xmm4, [ecx+MAT44_ROWSIZE*2]
//		movss xmm2, [edx+12]
		mulps xmm7, xmm4
//		shufps xmm2, xmm2, 0
		addps xmm0, xmm7
		movups xmm5, [ecx+MAT44_ROWSIZE*3]
		movss xmm6, [edx+MAT44_ROWSIZE]
//		mulps xmm2, xmm5
		movss xmm7, [edx+MAT44_ROWSIZE+4]
		shufps xmm6, xmm6, 0
//		addps xmm0, xmm2
		shufps xmm7, xmm7, 0
		movlps qword ptr [eax], xmm0
		movhps qword ptr [eax+8], xmm0
		mulps xmm7, xmm3
		movss xmm0, [edx+MAT44_ROWSIZE+8]
		mulps xmm6, xmm1
		shufps xmm0, xmm0, 0
		addps xmm6, xmm7
		mulps xmm0, xmm4
		movss xmm2, [edx+MAT44_ROWSIZE*2+4]
		addps xmm6, xmm0
//		movss xmm0, [edx+MAT44_ROWSIZE+12]
		movss xmm7, [edx+MAT44_ROWSIZE*2]
//		shufps xmm0, xmm0, 0
		shufps xmm7, xmm7, 0
//		mulps xmm0, xmm5
		mulps xmm7, xmm1
//		addps xmm6, xmm0
		shufps xmm2, xmm2, 0
		movlps qword ptr [eax+MAT44_ROWSIZE], xmm6
		movhps qword ptr [eax+MAT44_ROWSIZE+8], xmm6
		mulps xmm2, xmm3
		movss xmm6, [edx+MAT44_ROWSIZE*2+8]
		addps xmm7, xmm2
		shufps xmm6, xmm6, 0
//		movss xmm2, [edx+MAT44_ROWSIZE*2+12]
		mulps xmm6, xmm4
//		shufps xmm2, xmm2, 0
		addps xmm7, xmm6
//		mulps xmm2, xmm5
		movss xmm0, [edx+MAT44_ROWSIZE*3+4]
//		addps xmm7, xmm2
		shufps xmm0, xmm0, 0
		movlps qword ptr [eax+MAT44_ROWSIZE*2], xmm7
		movss xmm2, [edx+MAT44_ROWSIZE*3]
		movhps qword ptr [eax+MAT44_ROWSIZE*2+8], xmm7
		mulps xmm0, xmm3
		shufps xmm2, xmm2, 0
		movss xmm6, [edx+MAT44_ROWSIZE*3+8]
		mulps xmm2, xmm1
		shufps xmm6, xmm6, 0
		addps xmm2, xmm0
		mulps xmm6, xmm4
//		movss xmm7, [edx+MAT44_ROWSIZE*3+12]
//		shufps xmm7, xmm7, 0
		addps xmm2, xmm6
//		mulps xmm7, xmm5
//		addps xmm2, xmm7
		addps xmm2, xmm5
		movups [eax+MAT44_ROWSIZE*3], xmm2
	}
}

#define MAT43_ROWSIZE 12

MCCDLLEXPORT void SSE_Multiply_McMc_4x3(const TMatrix43<fp4>& _Src1, const TMatrix43<fp4>& _Src2, TMatrix43<fp4>& _Dst)
{
	__asm
	{
		mov edx, [_Src1]
		mov eax, [_Dst]
		mov ecx, [_Src2]

		movss xmm0, [edx]
		movups xmm1, [ecx]
		shufps xmm0, xmm0, 0
		movss xmm2, [edx+4]
		mulps xmm0, xmm1
		shufps xmm2, xmm2, 0
		movups xmm3, [ecx+MAT43_ROWSIZE]
		movss xmm7, [edx+8]
		mulps xmm2, xmm3
		shufps xmm7, xmm7, 0
		addps xmm0, xmm2
		movups xmm4, [ecx+MAT43_ROWSIZE*2]
//		movss xmm2, [edx+12]
		mulps xmm7, xmm4
//		shufps xmm2, xmm2, 0
		addps xmm0, xmm7
		movups xmm5, [ecx+MAT43_ROWSIZE*3]
		movss xmm6, [edx+MAT43_ROWSIZE]
//		mulps xmm2, xmm5
		movss xmm7, [edx+MAT43_ROWSIZE+4]
		shufps xmm6, xmm6, 0
//		addps xmm0, xmm2
		shufps xmm7, xmm7, 0

		movups [eax], xmm0
//		movlps qword ptr [eax], xmm0
//		movhps qword ptr [eax+8], xmm0

		mulps xmm7, xmm3
		movss xmm0, [edx+MAT43_ROWSIZE+8]
		mulps xmm6, xmm1
		shufps xmm0, xmm0, 0
		addps xmm6, xmm7
		mulps xmm0, xmm4
		movss xmm2, [edx+MAT43_ROWSIZE*2+4]
		addps xmm6, xmm0
//		movss xmm0, [edx+MAT43_ROWSIZE+12]
		movss xmm7, [edx+MAT43_ROWSIZE*2]
//		shufps xmm0, xmm0, 0
		shufps xmm7, xmm7, 0
//		mulps xmm0, xmm5
		mulps xmm7, xmm1
//		addps xmm6, xmm0
		shufps xmm2, xmm2, 0

		movups [eax+MAT43_ROWSIZE], xmm6
//		movlps qword ptr [eax+MAT43_ROWSIZE], xmm6
//		movhps qword ptr [eax+MAT43_ROWSIZE+8], xmm6

		mulps xmm2, xmm3
		movss xmm6, [edx+MAT43_ROWSIZE*2+8]
		addps xmm7, xmm2
		shufps xmm6, xmm6, 0
//		movss xmm2, [edx+MAT43_ROWSIZE*2+12]
		mulps xmm6, xmm4
//		shufps xmm2, xmm2, 0
		addps xmm7, xmm6
//		mulps xmm2, xmm5
		movss xmm0, [edx+MAT43_ROWSIZE*3+4]
//		addps xmm7, xmm2
		shufps xmm0, xmm0, 0
		movups [eax+MAT43_ROWSIZE*2], xmm7
//		movlps qword ptr [eax+MAT43_ROWSIZE*2], xmm7
		movss xmm2, [edx+MAT43_ROWSIZE*3]
//		movhps qword ptr [eax+MAT43_ROWSIZE*2+8], xmm7
		mulps xmm0, xmm3
		shufps xmm2, xmm2, 0
		movss xmm6, [edx+MAT43_ROWSIZE*3+8]
		mulps xmm2, xmm1
		shufps xmm6, xmm6, 0
		addps xmm2, xmm0
		mulps xmm6, xmm4
//		movss xmm7, [edx+MAT43_ROWSIZE*3+12]
//		shufps xmm7, xmm7, 0
		addps xmm2, xmm6
//		mulps xmm7, xmm5
//		addps xmm2, xmm7
		addps xmm2, xmm5
		movlps [eax+MAT43_ROWSIZE*3], xmm2
		movhlps xmm3, xmm2
		movss [eax+MAT43_ROWSIZE*3+8], xmm3
//		movups [eax+MAT43_ROWSIZE*3], xmm2
	}
}


/*
	__asm
	{
		mov eax, [_Src1]
		mov esi, [_Src2]
		mov edi, [_Dst]

		movups xmm4, [eax]
		movups xmm5, [eax+16]
		movups xmm6, [eax+32]
		movups xmm7, [eax+48]

		movss xmm0, [esi]
		movss xmm1, [esi+4]
		movss xmm2, [esi+8]
		shufps xmm0, xmm0, 0
		shufps xmm1, xmm1, 0
		shufps xmm2, xmm2, 0
		mulps xmm0, xmm4
		mulps xmm1, xmm5
		mulps xmm2, xmm6
		addps xmm0, xmm1
		addps xmm0, xmm2

		movss xmm3, [esi+16]
		movss xmm1, [esi+16+4]
		movss xmm2, [esi+16+8]
		shufps xmm3, xmm3, 0
		shufps xmm1, xmm1, 0
		shufps xmm2, xmm2, 0
		mulps xmm3, xmm4
		mulps xmm1, xmm5
		mulps xmm2, xmm6
		movups [edi], xmm0
		addps xmm3, xmm1
		addps xmm3, xmm2

		movss xmm0, [esi+32]
		movss xmm1, [esi+32+4]
		movss xmm2, [esi+32+8]
		shufps xmm0, xmm0, 0
		shufps xmm1, xmm1, 0
		shufps xmm2, xmm2, 0
		mulps xmm0, xmm4
		mulps xmm1, xmm5
		mulps xmm2, xmm6
		movups [edi+16], xmm3
		addps xmm0, xmm1
		addps xmm0, xmm2

		movss xmm3, [esi+48]
		movss xmm1, [esi+48+4]
		movss xmm2, [esi+48+8]
		shufps xmm3, xmm3, 0
		shufps xmm1, xmm1, 0
		shufps xmm2, xmm2, 0
		mulps xmm3, xmm4
		mulps xmm1, xmm5
		mulps xmm2, xmm6
		movups [edi+32], xmm0
		addps xmm3, xmm1
		addps xmm3, xmm2
		addps xmm3, xmm7
		movups [edi+48], xmm3
	}
*/

#elif defined(PLATFORM_XENON) 

#include "XTL.h"
#include "Xboxmath.h"
void SSE_Multiply_McMc_4x4(const CMat4Dfp4& _Src1, const CMat4Dfp4& _Src2, CMat4Dfp4& _Dst)
{
	for (int row=0; row<4; row++)
		for (int kol=0; kol<4; kol++)
			_Dst.k[row][kol] = 
				(_Src1.k[row][0]*_Src2.k[0][kol]) + 
				(_Src1.k[row][1]*_Src2.k[1][kol]) + 
				(_Src1.k[row][2]*_Src2.k[2][kol]) + 
				(_Src1.k[row][3]*_Src2.k[3][kol]);

	(XMMATRIX&)_Dst = XMMatrixMultiply((const XMMATRIX&)_Src1, (const XMMATRIX&)_Src2);
}


#endif
