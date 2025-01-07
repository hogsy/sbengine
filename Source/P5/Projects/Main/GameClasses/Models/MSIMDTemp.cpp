

#include "PCH.h"
#include "MMath.h"
#include "MFloat.h"

#ifdef CPU_X86
#pragma warning(disable : 4731)
#endif

#define MACRO_ISALIGNED16(p) (!(aint((uint8*)p) & 0x0f))

void SIMD_Mul_VS(int _nV, const CVec4Dfp32* _pSrc1, const fp32* _pSrc2, CVec4Dfp32* _pDst);
void SIMD_Mul_VS(int _nV, const CVec4Dfp32* _pSrc1, const fp32* _pSrc2, CVec4Dfp32* _pDst)
{
	MAUTOSTRIP(SIMD_Mul_VS, MAUTOSTRIP_VOID);
#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pSrc1))
		Error_static("SIMD_Mul_VS", "Source1 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pSrc2))
		Error_static("SIMD_Mul_VS", "Source2 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Mul_VS", "Destination pointer was not properly aligned.");
#endif

	if(_nV == 0)
		return;
		
#ifdef CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		__asm
		{
			emms
			push ebx
			mov esi, [_pSrc1]
			mov edi, [_pSrc2]
			mov eax, [_pDst]

			mov ecx, _nV
			mov ebx, ecx
			and ebx, 3
			jz skip1
		loop1:
				movaps xmm0, [esi]
				movss xmm1, dword ptr [edi]
				shufps xmm1, xmm1, 0
				add esi, 16
				add edi, 4
				mulps xmm0, xmm1
				add eax, 16
				dec ebx
				movaps [eax-16], xmm0
			jnz loop1
		skip1:
			shr ecx, 2
			jz quit
		loop2:
				movaps xmm0, [esi]
				movaps xmm2, [esi+16]
				movaps xmm4, [esi+32]
				movaps xmm6, [esi+48]
				movss xmm1, dword ptr [edi]
				movss xmm3, dword ptr [edi+4]
				movss xmm5, dword ptr [edi+8]
				movss xmm7, dword ptr [edi+12]
				shufps xmm1, xmm1, 0
				shufps xmm3, xmm3, 0
				shufps xmm5, xmm5, 0
				shufps xmm7, xmm7, 0
				mulps xmm0, xmm1
				add esi, 64
				mulps xmm2, xmm3
				add edi, 16
				mulps xmm4, xmm5
				add eax, 64
				mulps xmm6, xmm7
				dec ecx
				movaps [eax-64], xmm0
				movaps [eax+16-64], xmm2
				movaps [eax+32-64], xmm4
				movaps [eax+48-64], xmm6
			jnz loop2
		quit:
			pop ebx
			emms
		}
	}
	else
#elif defined( PLATFORM_PS2 )
	{
		asm volatile ("
		
			and			$1, %0, 0x3			# Calc REST
			
			addi		$2, %0, -4			# Jump to REST if
			bltz		$2, _Rest			# less than 4

		_Loop:						 		# Do 4 simultaneous here

			lqc2		vf05, 0x00(%2)
			lqc2		vf01, 0x00(%1)
			lqc2		vf02, 0x10(%1)
			lqc2		vf03, 0x20(%1)
			lqc2		vf04, 0x30(%1)
			
			vMULx		vf10, vf01, vf05
			vMULy		vf11, vf02, vf05
			vMULz		vf12, vf03, vf05
			vMULw		vf13, vf04, vf05
			
			sqc2		vf10, 0x00(%3)
			sqc2		vf11, 0x10(%3)
			sqc2		vf12, 0x20(%3)
			sqc2		vf13, 0x30(%3)
			
			addi		%1, %1, 0x40
			addi		%2, %2, 0x10
			addi		%3, %3, 0x40

			addi		%0, %0, -4
			bne			$1, %0, _Loop
			
			beqz		%0, _Done			# Jump to DONE if done
			
		_Rest:
			lqc2		vf01, 0x00(%1)
			lqc2		vf05, 0x00(%2)
			lqc2		vf02, 0x10(%1)
			lqc2		vf03, 0x20(%1)
			
			vMULx		vf10, vf01, vf05
			sqc2		vf10, 0x00(%3)

			addi		%0, %0, -1
			beqz		%0, _Done

			vMULy		vf11, vf02, vf05
			sqc2		vf11, 0x10(%3)

			addi		%0, %0, -1
			beqz		%0, _Done

			vMULz		vf12, vf03, vf05
			sqc2		vf12, 0x20(%3)

		_Done:
		"
		:
		: "r" (_nV), "r" (_pSrc1), "r" (_pSrc2), "r" (_pDst)
		: "cc", "memory", "$1", "$2", "$f1", "$f2", "$f3"
		);
		
		return;
	}	
#endif
	{
		for( int i = 0; i < _nV; i++ )
		{
			_pDst[i] = _pSrc1[i] * _pSrc2[i];
		}
	}
}

void SIMD_Combine_SSS(int _nV, const fp32* _pSrc1, const fp32* _pSrc2, const fp32* _pSrc3, fp32* _pDst);
void SIMD_Combine_SSS(int _nV, const fp32* _pSrc1, const fp32* _pSrc2, const fp32* _pSrc3, fp32* _pDst)
{
	MAUTOSTRIP(SIMD_Combine_SSS, MAUTOSTRIP_VOID);

#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pSrc1))
		Error_static("SIMD_Combine_SSS", "Source1 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pSrc2))
		Error_static("SIMD_Combine_SSS", "Source2 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pSrc3))
		Error_static("SIMD_Combine_SSS", "Source3 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Combine_SSS", "Destination pointer was not properly aligned.");
#endif

	if(_nV == 0)
		return;

#ifdef CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		__asm
		{
			emms
			mov edx, [_pSrc1]
			mov esi, [_pSrc2]
			mov edi, [_pSrc3]
			mov eax, [_pDst]

			mov ecx, _nV
			add ecx, 15
			and ecx, ~15
			shr ecx, 2
			mov ebx, ecx
			and ebx, 3
			jz skip1
		loop1:
				movaps xmm0, [esi]
				movaps xmm1, [edi]
				shufps xmm1, xmm1, 0
				add esi, 16
				mulps xmm0, xmm1
				add edi, 16
				add eax, 16
				addps xmm0, [edx]
				add edx, 16
				dec ebx
				movaps [eax-16], xmm0
			jnz loop1
		skip1:
			shr ecx, 2
			jz quit
		loop2:
				movaps xmm0, [esi]
				movaps xmm2, [esi+16]
				movaps xmm4, [esi+32]
				movaps xmm6, [esi+48]
				movaps xmm1, [edi]
				movaps xmm3, [edi+16]
				movaps xmm5, [edi+32]
				movaps xmm7, [edi+48]
				mulps xmm0, xmm1
				add esi, 64
				mulps xmm2, xmm3
				add edi, 64
				mulps xmm4, xmm5
				add eax, 64
				mulps xmm6, xmm7
				addps xmm0, [edx]
				addps xmm2, [edx+16]
				addps xmm4, [edx+32]
				addps xmm6, [edx+48]
				add edx, 64
				dec ecx
				movaps [eax-64], xmm0
				movaps [eax+16-64], xmm2
				movaps [eax+32-64], xmm4
				movaps [eax+48-64], xmm6
			jnz loop2
		quit:
			emms
		}
	}
	else
#elif defined( PLATFORM_PS2 )
	{
		asm volatile ("
		
			and			$1, %0, 0x3			# Calc REST
			
			addi		$2, %0, -4			# Jump to REST if
			bltz		$2, _Rest			# less than 4

		_Loop:						 		# Do 4 simultaneous here

			lqc2		vf01, 0x0(%1)
			lqc2		vf02, 0x0(%2)
			lqc2		vf03, 0x0(%3)
			
			vMUL		vf04, vf02, vf03
			vADD		vf05, vf01, vf04
			
			sqc2		vf05, 0x0(%4)
			
			addi		%1, %1, 0x10
			addi		%2, %2, 0x10
			addi		%3, %3, 0x10
			addi		%4, %4, 0x10
			
			addi		%0, %0, -4
			bne			$1, %0, _Loop
			
			beqz		$1, _Done			# Jump to DONE if done
			
		_Rest:								# Write one at a time
			
			lwc1		$f1, 0x0(%1)
			lwc1		$f2, 0x0(%2)
			lwc1		$f3, 0x0(%3)
			
			mul.s		$f4, $f2, $f3		
			add.s		$f5, $f1, $f4
			
			swc1		$f5, 0x0(%4)
			
			addi		%1, %1, 0x04
			addi		%2, %2, 0x04
			addi		%3, %3, 0x04
			addi		%4, %4, 0x04
			
			addi		$1, $1, -1
			bgtz		$1, _Rest
			
		_Done:
		"
		:
		: "r" (_nV), "r" (_pSrc1), "r" (_pSrc2), "r" (_pSrc3), "r" (_pDst)
		: "cc", "memory", "$1", "$2", "$f1", "$f2", "$f3", "$f4", "$f5"
		);
		
		return;
	}	
#endif
	{
		for( int i = 0; i < _nV; i++ )
		{
			_pDst[i] = _pSrc1[i] + _pSrc2[i] * _pSrc3[i];
		}
	}
}

void SIMD_Combine_VVS(int _nV, const CVec4Dfp32* _pSrc1, const CVec4Dfp32* _pSrc2, const fp32* _pSrc3, CVec4Dfp32* _pDst);
void SIMD_Combine_VVS(int _nV, const CVec4Dfp32* _pSrc1, const CVec4Dfp32* _pSrc2, const fp32* _pSrc3, CVec4Dfp32* _pDst)
{
	MAUTOSTRIP(SIMD_Combine_VVS, MAUTOSTRIP_VOID);
#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pSrc1))
		Error_static("SIMD_Combine_VVS", "Source1 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pSrc2))
		Error_static("SIMD_Combine_VVS", "Source2 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pSrc3))
		Error_static("SIMD_Combine_VVS", "Source2 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Combine_VVS", "Destination pointer was not properly aligned.");
#endif

#ifdef NOTWORKING_CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		__asm
		{
			emms
			mov edx, [_pSrc1]
			mov esi, [_pSrc2]
			mov edi, [_pSrc3]
			mov eax, [_pDst]

			mov ecx, _nV
			mov ebx, ecx
			and ebx, 3
			jz skip1
		loop1:
				movaps xmm0, [esi]
				movss xmm1, dword ptr [edi]
				shufps xmm1, xmm1, 0
				add esi, 16
				add edi, 4
				mulps xmm0, xmm1
				add eax, 16
				dec ebx
				movaps [eax-16], xmm0
			jnz loop1
		skip1:
			shr ecx, 2
			jz quit
		loop2:
				movaps xmm0, [esi]
				movaps xmm2, [esi+16]
				movaps xmm4, [esi+32]
				movaps xmm6, [esi+48]
				movss xmm1, dword ptr [edi]
				movss xmm3, dword ptr [edi+4]
				movss xmm5, dword ptr [edi+8]
				movss xmm7, dword ptr [edi+12]
				shufps xmm1, xmm1, 0
				shufps xmm3, xmm3, 0
				shufps xmm5, xmm5, 0
				shufps xmm7, xmm7, 0
				mulps xmm0, xmm1
				add esi, 64
				mulps xmm2, xmm3
				add edi, 16
				mulps xmm4, xmm5
				add eax, 64
				mulps xmm6, xmm7
				addps xmm0, [edx]
				addps xmm2, [edx+16]
				addps xmm4, [edx+32]
				addps xmm6, [edx+48]
				add edx, 64
				dec ecx
				movaps [eax-64], xmm0
				movaps [eax+16-64], xmm2
				movaps [eax+32-64], xmm4
				movaps [eax+48-64], xmm6
			jnz loop2
		quit:
			emms
		}
	}
	else
#endif
	{
		for( int i = 0; i < _nV; i++ )
		{
			_pDst[i] = _pSrc1[i] + _pSrc2[i] * _pSrc3[i];
		}
	}
}
/*
static void SIMD_Mul_SS(int _nV, const fp32* _pSrc1, const fp32* _pSrc2, fp32* _pDst)
{
	MAUTOSTRIP(SIMD_Mul_SS, MAUTOSTRIP_VOID);
#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pSrc1))
		Error_static("SIMD_Mul_SS", "Source1 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pSrc2))
		Error_static("SIMD_Mul_SS", "Source2 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Mul_SS", "Destination pointer was not properly aligned.");
	if (_nV & 3)
		Error_static("SIMD_Mul_SS", "Vertex count was not a multiple of 4.");
#endif

#ifdef CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		__asm
		{
			emms
			mov esi, _pSrc1
			mov edi, _pSrc2
			mov eax, _pDst

			mov ecx, _nV
			shr ecx, 2
			mov ebx, ecx
			and ebx, 3
			jz skip1
		loop1:
				movaps xmm0, [esi]
				movaps xmm1, [edi]
				add esi, 16
				add edi, 16
				mulps xmm0, xmm1
				add eax, 16
				dec ebx
				movaps [eax-16], xmm0
			jnz loop1
		skip1:
			shr ecx, 2
			jz quit
		loop2:
				movaps xmm0, [esi]
				movaps xmm1, [edi]
				movaps xmm2, [esi+16]
				movaps xmm3, [edi+16]
				movaps xmm4, [esi+32]
				movaps xmm5, [edi+32]
				movaps xmm6, [esi+48]
				movaps xmm7, [edi+48]
				mulps xmm0, xmm1
				add esi, 64
				mulps xmm2, xmm3
				add edi, 64
				mulps xmm4, xmm5
				add eax, 64
				mulps xmm6, xmm7
				dec ecx
				movaps [eax-64], xmm0
				movaps [eax+16-64], xmm2
				movaps [eax+32-64], xmm4
				movaps [eax+48-64], xmm6
			jnz loop2
		quit:
			emms
		}
	}
	else
#endif
	{
		for(int i = 0; i < _nV; i++)
			_pDst[i] = _pSrc1[i]*_pSrc2[i];
	}
}
*/
void SIMD_Combine_VcVcVr(int _nV, const CVec4Dfp32& _Src1, const CVec4Dfp32& _Src2, uint32* _pRandSeed, CVec4Dfp32* _pDst);
void SIMD_Combine_VcVcVr(int _nV, const CVec4Dfp32& _Src1, const CVec4Dfp32& _Src2, uint32* _pRandSeed, CVec4Dfp32* _pDst)
{
	MAUTOSTRIP(SIMD_Combine_VcVcVr, MAUTOSTRIP_VOID);
#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Combine_VcVcVr", "Destination pointer was not properly aligned.");
#endif

	const fp32* pRand = MFloat_GetRandTable();

#ifdef CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		static CVec4Dfp32 NegHalf = -0.5f;
		static CVec4Dfp32 Half = 128.0f;
		static CVec4Dfp32 Noll = 0.0f;

		__asm
		{
			emms
			mov esi, [_Src1]
			mov edi, [_Src2]
			movups xmm0, [esi]
			movups xmm1, [edi]
			movups xmm7, NegHalf

			mov eax, [_pDst]
			mov esi, [_pRandSeed]
			mov edi, [pRand]

			mov ecx, _nV
			mov ebx, ecx
			and ebx, 3
			jz skip1
		loop1:
				mov edx, [esi]
				and edx, (MFLOAT_RANDTABLESIZE-1)
				movups xmm2, [edi + edx*4]
				add [esi], 4
				addps xmm2, xmm7
				add esi, 4
				mulps xmm2, xmm1
				add eax, 16
				addps xmm2, xmm0
				dec ebx
				movaps [eax-16], xmm2
			jnz loop1
		skip1:
			shr ecx, 2
			jz quit
		loop2:
				mov ebx, [esi]
				and ebx, (MFLOAT_RANDTABLESIZE-1)
				movups xmm2, [edi + ebx*4]
				mov ebx, [esi+4]
				and ebx, (MFLOAT_RANDTABLESIZE-1)
				addps xmm2, xmm7
				movups xmm3, [edi + ebx*4]
				mulps xmm2, xmm1
				addps xmm3, xmm7
				add [esi], 4
				mulps xmm3, xmm1
				add [esi+4], 4
				addps xmm2, xmm0
				addps xmm3, xmm0
				add eax, 64
				movaps [eax-64], xmm2
				movaps [eax+16-64], xmm3

				mov ebx, [esi+8]
				and ebx, (MFLOAT_RANDTABLESIZE-1)
				movups xmm2, [edi + ebx*4]
				mov ebx, [esi+12]
				and ebx, (MFLOAT_RANDTABLESIZE-1)
				addps xmm2, xmm7
				movups xmm3, [edi + ebx*4]
				mulps xmm2, xmm1
				addps xmm3, xmm7
				add [esi+8], 4
				mulps xmm3, xmm1
				add [esi+12], 4
				add esi, 16
				addps xmm2, xmm0
				addps xmm3, xmm0
				dec ecx
				movaps [eax+32-64], xmm2
				movaps [eax+48-64], xmm3

			jnz loop2
		quit:
			emms
		}
	}
	else
#endif
	{
		for( int i = 0; i < _nV; i++ )
		{
			int32 index = _pRandSeed[i] & ( MFLOAT_RANDTABLESIZE - 1 );
			_pDst[i].k[0] = _Src1.k[0] + _Src2.k[0] * ( pRand[0+index] - 0.5f );
			_pDst[i].k[1] = _Src1.k[1] + _Src2.k[1] * ( pRand[1+index] - 0.5f );
			_pDst[i].k[2] = _Src1.k[2] + _Src2.k[2] * ( pRand[2+index] - 0.5f );
			_pDst[i].k[3] = _Src1.k[3] + _Src2.k[3] * ( pRand[3+index] - 0.5f );
			_pRandSeed[i]	+= 4;
		}
	}
}

void SIMD_Combine_VcVcSr(int _nV, const CVec4Dfp32& _Src1, const CVec4Dfp32& _Src2, uint32* _pRandSeed, CVec4Dfp32* _pDst);
void SIMD_Combine_VcVcSr(int _nV, const CVec4Dfp32& _Src1, const CVec4Dfp32& _Src2, uint32* _pRandSeed, CVec4Dfp32* _pDst)
{
	MAUTOSTRIP(SIMD_Combine_VcVcSr, MAUTOSTRIP_VOID);
#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Combine_VcVcSr", "Destination pointer was not properly aligned.");
#endif

	const fp32* pRand = MFloat_GetRandTable();

#ifdef CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		static CVec4Dfp32 NegHalf = -0.5f;

		__asm
		{
			emms
			push ebx

			mov esi, [_Src1]
			mov edi, [_Src2]
			movups xmm0, [esi]
			movups xmm1, [edi]
			movups xmm7, NegHalf

			mov eax, [_pDst]
			mov esi, [_pRandSeed]
			mov edi, [pRand]

			mov ecx, _nV
			mov ebx, ecx
			and ebx, 3
			jz skip1
		loop1:
				mov edx, [esi]
				and edx, (MFLOAT_RANDTABLESIZE-1)
				movss xmm2, [edi + edx*4]
				shufps xmm2, xmm2, 0
				add [esi], 1
				addps xmm2, xmm7
				add esi, 4
				mulps xmm2, xmm1
				add eax, 16
				addps xmm2, xmm0
				dec ebx
				movaps [eax-16], xmm2
			jnz loop1
		skip1:
			shr ecx, 2
			jz quit
		loop2:
				mov ebx, [esi]
				and ebx, (MFLOAT_RANDTABLESIZE-1)
				movss xmm2, [edi + ebx*4]
				mov ebx, [esi+4]
				and ebx, (MFLOAT_RANDTABLESIZE-1)
				movss xmm3, [edi + ebx*4]
				add [esi], 1
				add [esi+4], 1
				shufps xmm2, xmm2, 0
				shufps xmm3, xmm3, 0
				addps xmm2, xmm7
				addps xmm3, xmm7
				mulps xmm2, xmm1
				mulps xmm3, xmm1
				addps xmm2, xmm0
				addps xmm3, xmm0
				add eax, 64
				movaps [eax-64], xmm2
				movaps [eax+16-64], xmm3

				mov ebx, [esi+8]
				and ebx, (MFLOAT_RANDTABLESIZE-1)
				movss xmm2, [edi + ebx*4]
				mov ebx, [esi+12]
				and ebx, (MFLOAT_RANDTABLESIZE-1)
				movss xmm3, [edi + ebx*4]
				add [esi+8], 1
				add [esi+12], 1
				shufps xmm2, xmm2, 0
				shufps xmm3, xmm3, 0
				addps xmm2, xmm7
				addps xmm3, xmm7
				mulps xmm2, xmm1
				mulps xmm3, xmm1
				add esi, 16
				addps xmm2, xmm0
				addps xmm3, xmm0
				dec ecx
				movaps [eax+32-64], xmm2
				movaps [eax+48-64], xmm3

			jnz loop2
		quit:

			pop ebx
			emms
		}
	}
	else
#endif
	{
		for( int i = 0; i < _nV; i++ )
		{
			_pDst[i] = _Src1 + _Src2 *  ( pRand[(_pRandSeed[i])&(MFLOAT_RANDTABLESIZE-1)] - 0.5f );
			_pRandSeed[i]++;
		}
	}
}

void SIMD_Combine_ScScSr(int _nV, fp32 _Src1, fp32 _Src2, uint32* _pRandSeed, fp32* _pDst);
void SIMD_Combine_ScScSr(int _nV, fp32 _Src1, fp32 _Src2, uint32* _pRandSeed, fp32* _pDst)
{
	MAUTOSTRIP(SIMD_Combine_ScScSr, MAUTOSTRIP_VOID);
#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Combine_ScScSr", "Destination pointer was not properly aligned.");
#endif

	const fp32* pRand = MFloat_GetRandTable();

#ifdef CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		static CVec4Dfp32 NegHalf = -0.5f;

		__asm
		{
			emms
			push ebx

			movss xmm0, [_Src1]
			shufps xmm0, xmm0, 0
			movss xmm1, [_Src2]
			shufps xmm1, xmm1, 0
			movups xmm7, NegHalf

			mov eax, [_pDst]
			mov esi, [_pRandSeed]
			mov edi, [pRand]

			mov ebx, _nV
			add ebx, 3
			shr ebx, 2
			jz skip1
		loop1:
				mov ecx, [esi]
				mov edx, [esi+4]
				and ecx, (MFLOAT_RANDTABLESIZE-1)
				and edx, (MFLOAT_RANDTABLESIZE-1)
				movss xmm2, [edi + ecx*4]
				movss xmm3, [edi + edx*4]
				add [esi], 1
				add [esi+4], 1
				mov ecx, [esi+8]
				mov edx, [esi+12]
				and ecx, (MFLOAT_RANDTABLESIZE-1)
				and edx, (MFLOAT_RANDTABLESIZE-1)
				movss xmm4, [edi + ecx*4]
				movss xmm5, [edi + edx*4]
				add [esi+8], 1
				add [esi+12], 1
				shufps xmm2, xmm3, ((0) << 0) + ((0) << 2) + ((0) << 4) + ((0) << 6)
				shufps xmm4, xmm5, ((0) << 0) + ((0) << 2) + ((0) << 4) + ((0) << 6)
				shufps xmm2, xmm4, ((0) << 0) + ((2) << 2) + ((0) << 4) + ((2) << 6)
				addps xmm2, xmm7
				add esi, 16
				mulps xmm2, xmm1
				add eax, 16
				dec ebx
				addps xmm2, xmm0
				movaps [eax-16], xmm2
			jnz loop1
		skip1:
			pop ebx
			emms
		}
	}
	else
#elif defined( PLATFORM_PS2 )
	{
		asm volatile ("
		
			lwc1		$f1, 0x0(%1)
			lwc1		$f2, 0x0(%2)
			mtc1		%7, $f3
			
		_Loop:

			lw			$1, 0x0(%4)
			and			$2, $1, %6
			sll			$2, $2, 0x02
			add			$3, $2, %3
			lwc1		$f4, 0x0($3)
			
			sub.s		$f4, $f4, $f3
			mul.s		$f4, $f4,$f2
			add.s		$f4, $f4, $f1
			
			swc1		$f4, 0x0(%5)			
			
			addi		$1, 1
			sw			$1, 0x0(%4)
			
			addi		%5, %5, 0x04
			addi		%4, %4, 0x04
			addi		%0, %0, -1
			bgtz		%0, _Loop
		"
		:
		: "r" (_nV), "r" (&_Src1), "r" (&_Src2), "r" (pRand), "r" (_pRandSeed), "r" (_pDst), "r" (MFLOAT_RANDTABLESIZE-1), "r" (0.5f)
		: "cc", "memory", "$2", "$3"
		);
		
		return;
	}	
#endif
	{
		for( int i = 0; i < _nV; i++ )
		{
			_pDst[i] = _Src1 + _Src2 *  ( pRand[(_pRandSeed[i])&(MFLOAT_RANDTABLESIZE-1)] - 0.5f );
			_pRandSeed[i]++;
		}
	}
}

void SIMD_Mul_VMc(int _nV, const CVec4Dfp32* _pSrc1, const CMat4Dfp32& _Mat, CVec4Dfp32* _pDst);
void SIMD_Mul_VMc(int _nV, const CVec4Dfp32* _pSrc1, const CMat4Dfp32& _Mat, CVec4Dfp32* _pDst)
{
	MAUTOSTRIP(SIMD_Mul_VMc, MAUTOSTRIP_VOID);
	if(_nV == 0)
		return;
#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pSrc1))
		Error_static("SIMD_Mul_VMc", "Source1 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Mul_VMc", "Destination pointer was not properly aligned.");
#endif

#ifdef CPU_X86

    const CMat4Dfp32& Mat = _Mat;

	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		__asm
		{
			emms
			mov esi, [_pSrc1]
			mov edi, [_pDst]
			mov ecx, _nV
			mov edx, [Mat]

			movups xmm4, [edx]
			movups xmm5, [edx+16]
			movups xmm6, [edx+32]
			movups xmm7, [edx+48]

			test ecx, ecx
			jz skip1
		loop1:
				movss xmm0, [esi]
				movss xmm1, [esi+4]
				movss xmm2, [esi+8]
				shufps xmm0, xmm0, 0
				shufps xmm1, xmm1, 0
				shufps xmm2, xmm2, 0
				mulps xmm0, xmm4
				mulps xmm1, xmm5
				mulps xmm2, xmm6
				add esi, 16
				add edi, 16
				addps xmm0, xmm1
				addps xmm0, xmm2
				addps xmm0, xmm7
				dec ecx
				movaps [edi-16], xmm0				
			jnz loop1
		skip1:
			emms
		}
	}
	else
#elif defined( PLATFORM_PS2 )
	 const CMat4Dfp32& Mat = _Mat;
		asm volatile ("
		    mtsab			%2,	0
		    lq				$1,	0x00(%2)
		    lq				$2,	0x10(%2)
		    lq				$3,	0x20(%2)
		    lq				$4,	0x30(%2)
		    lq				$5,	0x40(%2)
		    qfsrv			$1,	$2,	$1
		    qfsrv			$2,	$3,	$2
		    qfsrv			$3,	$4,	$3
		    qfsrv			$4,	$5,	$4
		    qmtc2			$1,	vf04
		    qmtc2			$2,	vf05
		    qmtc2			$3,	vf06
		    qmtc2			$4,	vf07

		
		_Loop:
		
			lqc2		vf01, 0x00(%1)
			
			vMULAx.xyz	acc, vf04, vf01			# _Mat.k[0][j] * _pSrc1[i].k[0] + 
			vMADDAy.xyz	acc, vf05, vf01			# _Mat.k[1][j] * _pSrc1[i].k[1] + 
			vMADDAz.xyz	acc, vf06, vf01			# _Mat.k[2][j] * _pSrc1[i].k[2] + 
			vMADDw.xyz	vf03, vf07, vf00		# _Mat.k[3][j];
			
			sqc2		vf03, 0x00(%3)

			addi		%1, %1, 0x10
			addi		%3, %3, 0x10
			addi		%0, %0, -1
			bgtz		%0, _Loop

			
		"
		:
		: "r" (_nV), "r" (_pSrc1), "r" (&Mat), "r" (_pDst)
		: "cc", "memory", "$2", "$3"
		);

	return;
#endif
	{
		for( int i = 0; i < _nV; i++ )
		{
			fp32 a = _pSrc1[i].k[0];
			fp32 b = _pSrc1[i].k[1];
			fp32 c = _pSrc1[i].k[2];
			_pDst[i].k[0] = _Mat.k[0][0] * a + _Mat.k[1][0] * b + _Mat.k[2][0] * c + _Mat.k[3][0];
			_pDst[i].k[1] = _Mat.k[0][1] * a + _Mat.k[1][1] * b + _Mat.k[2][1] * c + _Mat.k[3][1];
			_pDst[i].k[2] = _Mat.k[0][2] * a + _Mat.k[1][2] * b + _Mat.k[2][2] * c + _Mat.k[3][2];
		}
	}
}

void SIMD_Combine_VVMc(int _nV, const CVec4Dfp32* _pSrc1, const CVec4Dfp32* _pSrc2, const CMat4Dfp32& _Mat, CVec4Dfp32* _pDst);
void SIMD_Combine_VVMc(int _nV, const CVec4Dfp32* _pSrc1, const CVec4Dfp32* _pSrc2, const CMat4Dfp32& _Mat, CVec4Dfp32* _pDst)
{
	MAUTOSTRIP(SIMD_Combine_VVMc, MAUTOSTRIP_VOID);
#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pSrc1))
		Error_static("SIMD_Combine_VVMc", "Source1 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pSrc2))
		Error_static("SIMD_Combine_VVMc", "Source2 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Combine_VVMc", "Destination pointer was not properly aligned.");
#endif

//		for (int i = 0; i < 3; i++)
//			dest.k[i] = m.k[0][i]*k[0] + m.k[1][i]*k[1] + m.k[2][i]*k[2] + m.k[3][i];

#ifdef CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		__asm
		{
			emms
			push ebx

			mov esi, [_pSrc2]
			mov edi, [_pDst]
			mov ecx, _nV
			mov edx, [_Mat]
			mov ebx, [_pSrc1]

			movups xmm4, [edx]
			movups xmm5, [edx+16]
			movups xmm6, [edx+32]
			movups xmm7, [edx+48]

			test ecx, ecx
			jz skip1
		loop1:
				movss xmm0, [esi]
				movss xmm1, [esi+4]
				movss xmm2, [esi+8]
				shufps xmm0, xmm0, 0
				shufps xmm1, xmm1, 0
				shufps xmm2, xmm2, 0
				mulps xmm0, xmm4
				mulps xmm1, xmm5
				mulps xmm2, xmm6
				add esi, 16
				add edi, 16
				add ebx, 16
				addps xmm0, xmm1
				addps xmm0, xmm2
				addps xmm0, xmm7
				addps xmm0, [ebx-16]
				dec ecx
				movaps [edi-16], xmm0				
			jnz loop1
		skip1:
			pop ebx
			emms
		}
	}
	else
#endif
	{
		for( int i = 0; i < _nV; i++ )
		{
			for( int j = 0; j < 3; j++ )
			{
				_pDst[i].k[j] = _pSrc1[i].k[j] + _Mat.k[0][j] * _pSrc2[i].k[0] + _Mat.k[1][j] * _pSrc2[i].k[1] + _Mat.k[2][j] * _pSrc2[i].k[2] + _Mat.k[3][j];
			}
		}
	}
}

void SIMD_ParticleQuad_VSVcVc_V3(int _nV, const CVec4Dfp32* _pSrc1, const fp32* _pSrc2, const CVec4Dfp32& _Src3, const CVec4Dfp32& _Src4, CVec3Dfp32* _pDst);
void SIMD_ParticleQuad_VSVcVc_V3(int _nV, const CVec4Dfp32* _pSrc1, const fp32* _pSrc2, const CVec4Dfp32& _Src3, const CVec4Dfp32& _Src4, CVec3Dfp32* _pDst)
{
	MAUTOSTRIP(SIMD_ParticleQuad_VSVcVc_V3, MAUTOSTRIP_VOID);
#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pSrc1))
		Error_static("SIMD_ParticleQuad_VSVcVc_V3", "Source1 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pSrc2))
		Error_static("SIMD_ParticleQuad_VSVcVc_V3", "Source2 pointer was not properly aligned.");
#endif

#ifdef CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		__asm
		{
			emms
			mov esi, [_Src3]
			mov edi, [_Src4]
			movups xmm6, [esi]
			movups xmm7, [edi]

			mov esi, [_pSrc1]
			mov edx, [_pSrc2]
			mov edi, [_pDst]
			mov ecx, _nV

			test ecx, ecx
			jz skip1
		loop1:
				movaps xmm0, [esi]
				movss xmm1, [edx]
				shufps xmm1, xmm1, 0
				movaps xmm2, xmm1
				mulps xmm1, xmm6
				mulps xmm2, xmm7
				add esi, 16
				movaps xmm4, xmm0
				movaps xmm5, xmm0
				subps xmm4, xmm1
				addps xmm5, xmm1
				subps xmm4, xmm2
				subps xmm5, xmm2
				movups [edi], xmm4
				movups [edi+12], xmm5

				movaps xmm4, xmm0
				movaps xmm5, xmm0
				addps xmm4, xmm1
				subps xmm5, xmm1
				addps xmm4, xmm2
				addps xmm5, xmm2
				add edx, 4
				movups [edi+24], xmm4
				movups [edi+36], xmm5

				add edi, 12*4

				dec ecx
			jnz loop1
		skip1:
			emms
		}
	}
	else
#elif defined( PLATFORM_PS2 )
	{
		asm volatile ("
			lq			$2,	0x00(%3)			# Load/align Src3
			lq			$3, 0x10(%3)
			mtsab		%3,	0
			qfsrv		$2,	$3,	$2
			qmtc2		$2,	vf05

			nop
			
			lq			$2,	0x00(%4)			# Load/align Src4
			lq			$3, 0x10(%4)
			mtsab		%4,	0
			qfsrv		$2,	$3,	$2
			qmtc2		$2,	vf06
			
			andi		$2, %5, 15
			bne			$2, zero, _unaligned

		_aligned:
			lqc2		vf01, 0x00(%0)			# Src1
			lw			$2, 0x00(%1)			# Src2
			qmtc2		$2,	vf02

			vMULx.xyz	vf13, vf05, vf02		# p
			vMULx.xyz	vf14, vf06, vf02		# q

			vSUB		vf03, vf13, vf14		# p - q
			vADD		vf04, vf13, vf14		# p + q
			
			vSUB		vf10, vf01, vf04		# Src1 - (p + q)
			vADD		vf11, vf01, vf03		# Src1 + (p - q)
			vADD		vf12, vf01, vf04		# Src1 + (p + q)
			vSUB		vf13, vf01, vf03		# Src1 - (p - q)

			vMR32.w		vf10, vf11				# vf10.w = vf11.x
			vMR32.xy	vf11, vf11				# vf11.xy = vf11.yz
			vMR32.wx	vf12, vf12				#					(vf12.wx = vf12.xy)
			vMR32.zw	vf11, vf12				# vf11.zw = vf12.xy	(vf11.zw = vf12.wx)
	
			vADDAz.x	acc, vf00, vf12			#				 	(acc.x = vf12.z)
			vMADD.x		vf12, vf00, vf00		#					(vf12.x = acc.x)
			vMR32 		vf13, vf13				#					(Rotate vf13)
			vMR32 		vf13, vf13				#					(Rotate vf13)
			vMR32.yzw 	vf12, vf13				# vf12.yzw = vf13.xyz
		
												# Results are: 	(vf10x, vf10y, vf10z)
												#				(vf10w, vf11x, vf11y)
												#				(vf11z, vf11w, vf12x)
												#				(vf12y, vf12z, vf12w)

			sqc2		vf10, 0x00(%5)
			sqc2		vf11, 0x10(%5)
			sqc2		vf12, 0x20(%5)
			
			addi		%0, %0, 0x10
			addi		%1, %1, 0x04
			addi		%5, %5, 0x30
			addi		%2, %2, -1
			bgtz		%2, _aligned
			
			b			_done

		_unaligned:
		
			lqc2		vf01, 0x00(%0)			# Src1
			lw			$2, 0x00(%1)			# Src2
			qmtc2		$2,	vf02

			vMULx.xyz	vf13, vf05, vf02		# p
			vMULx.xyz	vf14, vf06, vf02		# q

			vSUB		vf03, vf13, vf14		# p - q
			vADD		vf04, vf13, vf14		# p + q
			
			vSUB		vf10, vf01, vf04		# Src1 - (p + q)
			vADD		vf11, vf01, vf03		# Src1 + (p - q)
			vADD		vf12, vf01, vf04		# Src1 + (p + q)
			vSUB		vf13, vf01, vf03		# Src1 - (p - q)

			vMR32.w		vf10, vf11				# vf10.w = vf11.x
			vMR32.xy	vf11, vf11				# vf11.xy = vf11.yz
			vMR32.wx	vf12, vf12				#					(vf12.wx = vf12.xy)
			vMR32.zw	vf11, vf12				# vf11.zw = vf12.xy	(vf11.zw = vf12.wx)
	
			vADDAz.x	acc, vf00, vf12			#				 	(acc.x = vf12.z)
			vMADD.x		vf12, vf00, vf00		#					(vf12.x = acc.x)
			vMR32 		vf13, vf13				#					(Rotate vf13)
			vMR32 		vf13, vf13				#					(Rotate vf13)
			vMR32.yzw 	vf12, vf13				# vf12.yzw = vf13.xyz
		
												# Results are: 	(vf10x, vf10y, vf10z)
												#				(vf10w, vf11x, vf11y)
												#				(vf11z, vf11w, vf12x)
												#				(vf12y, vf12z, vf12w)
			mtsab		%5, 0
			lq			$2, 0x00(%5)
			lq			$6, 0x30(%5)
			qfsrv		$2, $2, $2
			qfsrv		$6, $6, $6

			nop
			nop

			addi		$3, %5, -1
			mtsab		$3,	-1

			qmfc2		$3, vf10
			qmfc2		$4, vf11
			qmfc2		$5, vf12

			qfsrv		$2, $3, $2
			qfsrv		$3, $4, $3
			qfsrv		$4, $5, $4
			qfsrv		$5, $6, $5

			sq			$2, 0x00(%5)
			sq			$3, 0x10(%5)
			sq			$4, 0x20(%5)
			sq			$5, 0x30(%5)
			
			addi		%0, %0, 0x10
			addi		%1, %1, 0x04
			addi		%5, %5, 0x30
			addi		%2, %2, -1
			bgtz		%2, _unaligned

		_done:
			
		"
		:
		: "r" (_pSrc1), "r" (_pSrc2), "r" (_nV), "r" (&_Src3), "r" (&_Src4), "r"(_pDst)
		: "cc", "memory", "$2", "$3", "$4", "$5", "$6"
		);
		return;
	}
#endif
	{
		for( int i = 0; i < _nV; i++ )
		{
			CVec3Dfp32 p = *((const CVec3Dfp32*)&_Src3) * _pSrc2[i], q = *((const CVec3Dfp32*)&_Src4) * _pSrc2[i];
			_pDst[i*4+0] = *(const CVec3Dfp32*)&_pSrc1[i] - p - q;
			_pDst[i*4+1] = *(const CVec3Dfp32*)&_pSrc1[i] + p - q;
			_pDst[i*4+2] = *(const CVec3Dfp32*)&_pSrc1[i] + p + q;
			_pDst[i*4+3] = *(const CVec3Dfp32*)&_pSrc1[i] - p + q;
		}
	}
}

void SIMD_ParticleTriangle_VSVcVc_V3(int _nV, const CVec4Dfp32* _pSrc1, const fp32* _pSrc2, const CVec4Dfp32& _Src3, const CVec4Dfp32& _Src4, CVec3Dfp32* _pDst);
void SIMD_ParticleTriangle_VSVcVc_V3(int _nV, const CVec4Dfp32* _pSrc1, const fp32* _pSrc2, const CVec4Dfp32& _Src3, const CVec4Dfp32& _Src4, CVec3Dfp32* _pDst)
{
	MAUTOSTRIP(SIMD_ParticleTriangle_VSVcVc_V3, MAUTOSTRIP_VOID);
#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pSrc1))
		Error_static("SIMD_ParticleTriangle_VSVcVc_V3", "Source1 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pSrc2))
		Error_static("SIMD_ParticleTriangle_VSVcVc_V3", "Source2 pointer was not properly aligned.");
#endif

#ifdef CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		__asm
		{
			emms
			mov esi, [_Src3]
			mov edi, [_Src4]
			movups xmm6, [esi]
			movups xmm7, [edi]

			mov esi, [_pSrc1]
			mov edx, [_pSrc2]
			mov edi, [_pDst]
			mov ecx, _nV

			test ecx, ecx
			jz skip1
		loop1:
				movaps xmm0, [esi]
				movss xmm1, [edx]
				shufps xmm1, xmm1, 0
				movaps xmm2, xmm1
				mulps xmm1, xmm6
				mulps xmm2, xmm7

				movaps xmm4, xmm0
				movaps xmm5, xmm0
				addps xmm4, xmm1
				subps xmm0, xmm1
				addps xmm4, xmm1
				subps xmm0, xmm2
				addps xmm4, xmm1
				movlps [edi], xmm0
				subps xmm5, xmm1
				shufps xmm0, xmm0, 2			// Why are we storing Z twice?
				subps xmm4, xmm2
				movss [edi+8], xmm0				// Why are we storing Z twice?
				add edx, 4
				movlps [edi+12], xmm4
				addps xmm5, xmm2
				add esi, 16
				addps xmm5, xmm2
				shufps xmm4, xmm4, 2			// Why are we storing Z twice?
				addps xmm5, xmm2
				movss [edi+12+8], xmm4			// Why are we storing Z twice?
				movlps [edi+24], xmm5
				shufps xmm5, xmm5, 2			// Why are we storing Z twice?
				movss [edi+24+8], xmm5			// Why are we storing Z twice?



/*				movups [edi], xmm0
				movups [edi+12], xmm4
				movups [edi+24], xmm5*/

				add edi, 12*3

				dec ecx
			jnz loop1
		skip1:
			emms
		}
	}
	else
/*#elif defined( PLATFORM_PS2 )
	{
		asm volatile ("
			lq			$2,	0x00(%3)			# Load/align Src3
			lq			$3, 0x10(%3)
			mtsab		%3,	0
			qfsrv		$2,	$3,	$2
			qmtc2		$2,	vf05

			qmtc2		%6, vf20
		
			lq			$2,	0x00(%4)			# Load/align Src4
			lq			$3, 0x10(%4)
			mtsab		%4,	0
			qfsrv		$2,	$3,	$2
			qmtc2		$2,	vf06
			
			addi		%0, %0, 0x10
			addi		%1, %1, 0x04
			addi		%5, %5, 0x30
			addi		%2, %2, -1
			bgtz		%2, _aligned
			

		_nV:
		
			lqc2		vf01, 0x00(%0)			# Src1
			lw			$2, 0x00(%1)			# Src2
			qmtc2		$2,	vf02

			vMULx.xyz	vf13, vf05, vf02		# p
			vMULx.xyz	vf14, vf06, vf02		# q

			vSUB		vf10, vf01, vf13		# Src1 - p
			vSUB		vf10, vf10, vf14		# Src1 - p - q

			vSUBA		acc, vf01, vf14			# Src1 - q
			vMADDx		vf11, vf13, vf20		# Src1 + p * 3 - q

			vSUBA		acc, vf01, vf13			# Src1 - p
			vMADDx		vf12, vf14, vf20		# Src1 + q * 3 - p

			vMR32.w		vf10, vf11				# vf10.w = vf11.x
			vMR32.xy	vf11, vf11				# vf11.xy = vf11.yz
			vMR32.wx	vf12, vf12				#					(vf12.wx = vf12.xy)
			vMR32.zw	vf11, vf12				# vf11.zw = vf12.xy	(vf11.zw = vf12.wx)
	
			vADDAz.x	acc, vf00, vf12			#				 	(acc.x = vf12.z)
			vMADD.x		vf12, vf00, vf00		# vf12.x = vf12.z	(vf12.x = acc.x)
		
												# Results are: 	(vf10x, vf10y, vf10z)
												#				(vf10w, vf11x, vf11y)
												#				(vf11z, vf11w, vf12x)
			andi		$2, %5, 15
			beq			$2, zero, _aligned

			mtsab		%5, 0
			lq			$2, 0x00(%5)
			lq			$5, 0x20(%5)
			qfsrv		$2, $2, $2
			qfsrv		$5, $5, $5

			nop
			nop

			addi		$3, %5, -1
			mtsab		$3,	-1

			qmfc2		$3, vf10
			qmfc2		$4, vf11

			qfsrv		$2, $3, $2
			qfsrv		$3, $4, $3
			qfsrv		$4, $5, $4

			sq			$2, 0x00(%5)
			sq			$3, 0x10(%5)
			sq			$4, 0x20(%5)

			b			_continue
			
		_aligned:
			sqc2		vf10, 0x00(%5)
			sqc2		vf11, 0x10(%5)
			
		_continue:
			qmfc2		$5, vf12
			sw			$5, 0x20(%5)
			
			addi		%0, %0, 0x10
			addi		%1, %1, 0x04
			addi		%5, %5, 0x24
			addi		%2, %2, -1
			bgtz		%2, _nV
		"
		:
		: "r" (_pSrc1), "r" (_pSrc2), "r" (_nV), "r" (&_Src3), "r" (&_Src4), "r"(_pDst), "r"(3.f)
		: "cc", "memory", "$2", "$3"
		);
		return;
	}*/
#endif
	{
		for( int i = 0; i < _nV; i++ )
		{
			CVec3Dfp32 p = *(const CVec3Dfp32*)&_Src3 * _pSrc2[i], q = *(const CVec3Dfp32*)&_Src4 * _pSrc2[i];
			_pDst[i*3+0] = *(const CVec3Dfp32*)&_pSrc1[i] - p - q;
			_pDst[i*3+1] = *(const CVec3Dfp32*)&_pSrc1[i] + p * 3.0f - q;
			_pDst[i*3+2] = *(const CVec3Dfp32*)&_pSrc1[i] - p + q * 3.0f;
		}
	}
}

void SIMD_Add_VVc(int _nV, const CVec4Dfp32* _pSrc1, const CVec4Dfp32& _Src2, CVec4Dfp32* _pDst );
void SIMD_Add_VVc(int _nV, const CVec4Dfp32* _pSrc1, const CVec4Dfp32& _Src2, CVec4Dfp32* _pDst )
{
#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pSrc1))
		Error_static("SIMD_Add_VVc", "Source1 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Add_VVc", "Destination pointer was not properly aligned.");
#endif

#ifdef	PLATFORM_PS2
	{
		asm volatile("
			lq			$2,	0x00(%1)			# Load/align Src1
			lq			$3, 0x10(%1)
			mtsab		%1,	0
			qfsrv		$2,	$3,	$2
			qmtc2		$2, vf05

		_repeat:
			lqc2	vf01,0x00(%0)
			lqc2	vf02,0x10(%0)
			lqc2	vf03,0x20(%0)
			lqc2	vf04,0x30(%0)

			vADD.xyz	vf01, vf01, vf05
			vADD.xyz	vf02, vf02, vf05
			vADD.xyz	vf03, vf03, vf05
			vADD.xyz	vf04, vf04, vf05

			sqc2	vf01,0x00(%2)
			sqc2	vf02,0x10(%2)
			sqc2	vf03,0x20(%2)
			sqc2	vf04,0x30(%2)
			
			addi	%3,%3,-4
			addi	%0,%0,0x40
			addi	%2,%2,0x40
			bgtz	%3, _repeat
		"
		:
		: "r" (_pSrc1), "r" (&_Src2), "r" (_pDst), "r" (_nV)
		: "cc", "memory", "$2", "$3"
		);
		return;
	}
#else
	{
		for( int i = 0; i < _nV; i++ )
		{
			_pDst[i]	= _pSrc1[i] + _Src2;
		}
	}
#endif
}

void SIMD_Set_V(int _nV, const CVec4Dfp32* _pSrc1, CVec4Dfp32* _pDst );
void SIMD_Set_V(int _nV, const CVec4Dfp32* _pSrc1, CVec4Dfp32* _pDst )
{
#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pSrc1))
		Error_static("SIMD_Set_V", "Source1 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Set_V", "Destination pointer was not properly aligned.");
#endif

#ifdef	PLATFORM_PS2
	{
		asm volatile("
		_repeat:
			lq	$2,0x00(%0)
			lq	$3,0x10(%0)
			lq	$4,0x20(%0)
			lq	$5,0x30(%0)
			sq	$2,0x00(%1)
			sq	$3,0x10(%1)
			sq	$4,0x20(%1)
			sq	$5,0x30(%1)
			addi	%2,%2,-4
			addi	%0,%0,0x40
			addi	%1,%1,0x40
			bgtz	%2, _repeat
		"
		:
		: "r" (_pSrc1), "r" (_pDst), "r" (_nV)
		: "cc", "memory", "$2", "$3", "$4", "$5"
		);
		return;
	}
#else
	{
		for( int i = 0; i < _nV; i++ )
		{
			_pDst[i]	= _pSrc1[i];
		}
	}
#endif
}

void SIMD_Set_Vc(int _nV, const CVec4Dfp32& _Src1, CVec4Dfp32* _pDst );
void SIMD_Set_Vc(int _nV, const CVec4Dfp32& _Src1, CVec4Dfp32* _pDst )
{
#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Set_Vc", "Destination pointer was not properly aligned.");
#endif

#ifdef	PLATFORM_PS2
	{
		asm volatile("
			lq			$2,	0x00(%1)			# Load/align Src1
			lq			$3, 0x10(%1)
			mtsab		%1,	0
			qfsrv		$2,	$3,	$2
		_repeat:
			sq	$2,0x00(%2)
			sq	$2,0x10(%2)
			sq	$2,0x20(%2)
			sq	$2,0x30(%2)
			addi	%0,%0,-4
			addi	%2,%2,0x40
			bgtz	%0, _repeat
		"
		:
		: "r" (_nV), "r" (&_Src1), "r" (_pDst)
		: "cc", "memory", "$2", "$3"
		);
		return;
	}
#else
	{
		for( int i = 0; i < _nV; i++ )
		{
			_pDst[i]	= _Src1;
		}
	}
#endif
}

void SIMD_Mul_VSc( int _nV, const CVec4Dfp32* _pSrc1, const fp32& _Src2, CVec4Dfp32* _pDst );
void SIMD_Mul_VSc( int _nV, const CVec4Dfp32* _pSrc1, const fp32& _Src2, CVec4Dfp32* _pDst )
{
#ifdef _DEBUG
	if (!MACRO_ISALIGNED16(_pSrc1))
		Error_static("SIMD_Mul_VSc", "Source pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Mul_VSc", "Destination pointer was not properly aligned.");
#endif

#ifdef	PLATFORM_PS2
	{
		asm volatile("
			lw		$2, 0x00(%2)
			qmtc2	$2, vf01

		_repeat:
			lqc2	vf02, 0x00(%1)
			lqc2	vf03, 0x10(%1)
			lqc2	vf04, 0x20(%1)
			lqc2	vf05, 0x30(%1)
			
			vMULx.xyzw	vf02xyzw,vf02xyzw,vf01x
			vMULx.xyzw	vf03xyzw,vf03xyzw,vf01x
			vMULx.xyzw	vf04xyzw,vf04xyzw,vf01x
			vMULx.xyzw	vf05xyzw,vf05xyzw,vf01x
			
			sqc2	vf02, 0x00(%3)
			sqc2	vf03, 0x10(%3)
			sqc2	vf04, 0x20(%3)
			sqc2	vf05, 0x30(%3)
			
			addi	%0,%0,-4
			addi	%1,%1,0x40
			addi	%3,%3,0x40
			bgtz	%0, _repeat
		"
		:
		: "r" (_nV), "r" (_pSrc1), "r" (&_Src2), "r" (_pDst)
		: "cc", "memory", "$2"
		);
		return;
	}
#else
	{
		for( int i = 0; i < _nV; i++ )
		{
			_pDst[i]	= _pSrc1[i] * _Src2;
		}
	}
#endif
}
