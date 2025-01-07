#include "PCH.h"
#include "MSIMD.h"
#include "MFloat.h"
#include "../MOS/MSystem/Raster/MImage.h"

#define MACRO_ISALIGNED16(p) (!(mint((uint8*)p) & 0x0f))

//----------------------------------------------------------------
// PentiumIII Streaming SIMD Extension code
//----------------------------------------------------------------

// Disable X86 assembler unless we're using ICL
/*#ifndef __ICL
#undef CPU_X86
#endif*/

#ifdef CPU_X86


static void SSE_Sqrt8(fp32* _pSrc, fp32* _pDst, int _n)
{
	// Make sure padding area contains valid floats.
	int nPadd = (_n+3) & 0xfffffffc;
	for(int i = _n; i < nPadd; i++) _pSrc[i] = 1.0f;

	__asm
	{
		mov esi, _pSrc
		mov edi, _pDst
		mov ecx, _n
		add ecx, 3
		shr ecx, 2		// we do 4 sqrt every loop
	Invert:
			movaps xmm0, [esi]
			add edi, 16
			sqrtps xmm1, xmm0
			add esi, 16
			movaps [-16][edi], xmm1
			dec ecx
		jnz Invert
	}
}

static void SSE_RecpSqrt8(fp32* _pSrc, fp32* _pDst, int _n)
{
	// Make sure padding area contains valid floats.
	int nPadd = (_n+3) & 0xfffffffc;
	for(int i = _n; i < nPadd; i++)
		_pSrc[i] = 1.0f;

	__asm
	{
		mov esi, _pSrc
		mov edi, _pDst
		mov ecx, _n
		add ecx, 3
		shr ecx, 2		// divide by 4, do 4 at a time
	Invert:
			movaps xmm0, [esi]
			add edi, 16
			rsqrtps xmm1, xmm0
			add esi, 16
			movaps [-16][edi], xmm1
			dec ecx
		jnz Invert
	}
}

static void SSE_Recp8(fp32* _pSrc, fp32* _pDst, int _n)
{
	// Make sure padding area contains valid floats.
	int nPadd = (_n+15) & 0xfffffff0;
	for(int i = _n; i < nPadd; i++)
		_pSrc[i] = 1.0f;

	__asm
	{
		mov esi, _pSrc
		mov edi, _pDst
		mov ecx, _n
		add ecx, 15
		shr ecx, 4 // divide by 16, do 16 at a time
Invert:
			movaps xmm0, [esi]
			add edi, 64
			movaps xmm2, [16][esi]
			movaps xmm4, [32][esi]
			movaps xmm6, [48][esi]
			add esi, 64
			rcpps xmm1, xmm0
			rcpps xmm3, xmm2
			rcpps xmm5, xmm4
			rcpps xmm7, xmm6
			movaps [-64][edi], xmm1
			movaps [-48][edi], xmm3
			movaps [-32][edi], xmm5
			dec ecx
			movaps [-16][edi], xmm7
		jnz Invert
	}
}

static void SSE_ConvertRGB(const CVec4Dfp32* _pLight, uint32* _pColors, int _nV, int _Alpha)
{
	int Alpha = _Alpha << 24;

	__asm
	{
		mov ebx, Alpha
		mov ecx, _nV
		test ecx, ecx
		jz skip
		emms
		mov esi, _pLight
		mov edi, _pColors
Lp:
			movaps xmm0, [esi]
			movaps xmm1, xmm0
			shufps xmm0, xmm0, 4
			shufps xmm1, xmm1, 2+4+8
			add esi, 16
			cvtps2pi mm0, xmm0
			cvtps2pi mm1, xmm1
			add edi, 4
			packssdw mm0, mm1
			packuswb mm0, mm0
			movd eax, mm0
			bswap eax
			shr eax, 8
			or eax, ebx
			dec ecx
			mov dword ptr [edi-4], eax
		jnz Lp
		emms
skip:
	}
}

static void SSE_ConvertRGBA(const CVec4Dfp32* _pLight, uint32* _pColors, int _nV)
{
	__asm
	{
		mov ecx, _nV
		test ecx, ecx
		jz skip
		emms
		mov esi, _pLight
		mov edi, _pColors
Lp:
			movaps xmm0, [esi]
			movaps xmm1, xmm0
			shufps xmm0, xmm0, 4
			shufps xmm1, xmm1, 2+4+8
			add esi, 16
			cvtps2pi mm0, xmm0
			cvtps2pi mm1, xmm1
			add edi, 4
			packssdw mm0, mm1
			packuswb mm0, mm0
			movd eax, mm0
			bswap eax
			ror eax, 8
			dec ecx
			mov dword ptr [edi-4], eax
		jnz Lp
		emms
skip:
	}
}

#endif

#ifdef PLATFORM_DOLPHIN

static void PPC_RecpSqrt8(register fp32* _pSrc, register fp32* _pDst, register int _n);
static void PPC_RecpSqrt8(register fp32* _pSrc, register fp32* _pDst, register int _n)
{
/*	// Make sure padding area contains valid floats.
	int nPadd = (_n+3) & 0xfffffffc;
	for (int i=_n; i<nPadd; i++)
		_pSrc[i] = 1.0f; */

	register fp64 tmp0, tmp1;
	asm
	{
		addi _n, _n, 3
		srwi. _n, _n, 2 // 4 floats/loop
		mtctr _n
		ble done
		subi _pSrc, _pSrc, 8
		subi _pDst, _pDst, 8
	loop:
		psq_lu tmp0, 8(_pSrc), 0, 0
		psq_lu tmp1, 8(_pSrc), 0, 0
		ps_rsqrte tmp0, tmp0
		ps_rsqrte tmp1, tmp1
		psq_stu tmp0, 8(_pDst), 0, 0
		psq_stu tmp1, 8(_pDst), 0, 0
		bdnz loop
	done:
	}
}

static void PPC_Recp8(register fp32* _pSrc, register fp32* _pDst, register int _n);
static void PPC_Recp8(register fp32* _pSrc, register fp32* _pDst, register int _n)
{
	register fp64 tmp0, tmp1;
	asm
	{
		addi _n, _n, 3
		srwi. _n, _n, 2 // 4 floats/loop
		mtctr _n
		ble done
		subi _pSrc, _pSrc, 8
		subi _pDst, _pDst, 8
	loop:
		psq_lu tmp0, 8(_pSrc), 0, 0
		psq_lu tmp1, 8(_pSrc), 0, 0
		ps_res tmp0, tmp0
		ps_res tmp1, tmp1
		psq_stu tmp0, 8(_pDst), 0, 0
		psq_stu tmp1, 8(_pDst), 0, 0
		bdnz loop
	done:
	}
}

static void PPC_Sqrt8(register fp32* _pSrc, register fp32* _pDst, register int _n);
static void PPC_Sqrt8(register fp32* _pSrc, register fp32* _pDst, register int _n)
{
	register fp64 tmp0, tmp1;
	asm
	{
		addi _n, _n, 3
		srwi. _n, _n, 2 // 4 floats/loop
		mtctr _n
		ble done
		subi _pSrc, _pSrc, 8
		subi _pDst, _pDst, 8
	loop:
		psq_lu tmp0, 8(_pSrc), 0, 0
		psq_lu tmp1, 8(_pSrc), 0, 0
		ps_rsqrte tmp0, tmp0
		ps_rsqrte tmp1, tmp1
		ps_res tmp0, tmp0
		ps_res tmp1, tmp1
		psq_stu tmp0, 8(_pDst), 0, 0
		psq_stu tmp1, 8(_pDst), 0, 0
		bdnz loop
	done:
	}
}


static void PPC_ConvertRGB(register const CVec4Dfp32* _pSrc, register uint32* _pDst, register int _nV, register int _Alpha);
static void PPC_ConvertRGB(register const CVec4Dfp32* _pSrc, register uint32* _pDst, register int _nV, register int _Alpha)
{
	register fp64 tmp0, tmp1;
	asm
	{
		mtctr _nV
		cmpwi _nV, 0
		ble done
		subi _pSrc, _pSrc, 8
	loop:
		psq_lu tmp0, 8(_pSrc), 0, 0
		psq_lu tmp1, 8(_pSrc), 1, 0
		psq_st tmp0, 0(_pDst), 0, OS_FASTCAST_U8
		psq_st tmp1, 2(_pDst), 1, OS_FASTCAST_U8
		stb _Alpha, 3(_pDst)
		addi _pDst, _pDst, 4
		bdnz loop
	done:
	}
}

static void PPC_ConvertRGBA(register const CVec4Dfp32* _pSrc, register uint32* _pDst, register int _nV);
static void PPC_ConvertRGBA(register const CVec4Dfp32* _pSrc, register uint32* _pDst, register int _nV)
{
	register fp64 tmp0, tmp1;
	asm
	{
		mtctr _nV
		cmpwi _nV, 0
		ble done
		subi _pSrc, _pSrc, 8
	loop:
		psq_lu tmp0, 8(_pSrc), 0, 0
		psq_lu tmp1, 8(_pSrc), 0, 0
		psq_st tmp0, 0(_pDst), 0, OS_FASTCAST_U8
		psq_st tmp1, 2(_pDst), 0, OS_FASTCAST_U8
		addi _pDst, _pDst, 4
		bdnz loop
	done:
	}
}

#endif

//----------------------------------------------------------------
void SIMD_Sqrt8(fp32* _pSrc, fp32* _pDst, int _n)
{
	if (!_pDst) _pDst = _pSrc;

#ifndef M_RTM
	if (!MACRO_ISALIGNED16(_pSrc))
		Error_static("SIMD_Sqrt8", "Source pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Sqrt8", "Destination pointer was not properly aligned.");
#endif		

#ifdef CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		SSE_Sqrt8(_pSrc, _pDst, _n);
	}
	else
#elif defined(PLATFORM_DOLPHIN)
	if (true)
	{
		PPC_Sqrt8(_pSrc, _pDst, _n);
	}
	else
#endif
	{
		for(int i = 0; i < _n; i++)
			_pDst[i] = M_Sqrt(_pSrc[i]);
	}
}

void SIMD_RecpSqrt8(fp32* _pSrc, fp32* _pDst, int _n)
{
	if (!_pDst) _pDst = _pSrc;

#ifndef M_RTM
	if (!MACRO_ISALIGNED16(_pSrc))
		Error_static("SIMD_RecpSqrt8", "Source pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_RecpSqrt8", "Destination pointer was not properly aligned.");
#endif		

#ifdef CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		SSE_RecpSqrt8(_pSrc, _pDst, _n);
	}
	else
#elif defined(PLATFORM_DOLPHIN)
	if (true)
	{
		PPC_RecpSqrt8(_pSrc, _pDst, _n);
	}
	else
#elif defined( PLATFORM_PS2 )
		register float k = 1.f;
		asm volatile ("
			sll			%0, %0, 2
			add			%0, %0, %1
		_Loop:
			lwc1		$f1, 0x00(%1)
			addi		%1, %1, 4
			rsqrt.s		$f1, k, $f1
			swc1		$f1, 0x00(%2)
			addi		%2, %2, 4
			bne			%1, %0, _Loop
		"
		:
		: "r" (_n), "r" (_pSrc), "r" (_pDst), "r" (k)
		: "cc", "memory", "$f1"
		);
	return;
#endif
	{
		for(int i = 0; i < _n; i++)
			_pDst[i] = M_InvSqrt(_pSrc[i]);
	}
}

void SIMD_Recp8(fp32* _pSrc, fp32* _pDst, int _n)
{
	if (!_pDst) _pDst = _pSrc;

#ifndef M_RTM
	if (!MACRO_ISALIGNED16(_pSrc))
		Error_static("SIMD_Recp8", "Source pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Recp8", "Destination pointer was not properly aligned.");
#endif		

#ifdef CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		SSE_Recp8(_pSrc, _pDst, _n);
	}
	else
#elif defined(PLATFORM_DOLPHIN)
	if (true)
	{
		PPC_Recp8(_pSrc, _pDst, _n);
	}
	else	
#endif
	{
		for(int i = 0; i < _n; i++)
			_pDst[i] = 1.0f / _pSrc[i];
	}
};

void SIMD_ConvertRGB(const CVec4Dfp32* _pLight, uint32* _pColors, int _nV, int _Alpha)
{
#ifndef M_RTM
	if (!MACRO_ISALIGNED16(_pLight))
		Error_static("SIMD_ConvertRGB", "Source pointer was not properly aligned.");
#endif		

#ifdef CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		SSE_ConvertRGB(_pLight, _pColors, _nV, _Alpha);
	}
	else
#elif defined(PLATFORM_DOLPHIN)
	if (true)
	{
		PPC_ConvertRGB(_pLight, _pColors, _nV, _Alpha);
	}
	else
#endif
	{
		CPixel32* pDest = (CPixel32*)_pColors;
		for (int v=0; v<_nV; v++)
		{
			pDest[v].R() = (int)ClampRange(_pLight[v][0], 255.0f);
			pDest[v].G() = (int)ClampRange(_pLight[v][1], 255.0f);
			pDest[v].B() = (int)ClampRange(_pLight[v][2], 255.0f);
			pDest[v].A() = _Alpha;
		}
	}
}

void SIMD_ConvertRGBA(const CVec4Dfp32* _pLight, uint32* _pColors, int _nV)					// _pLight must be 16-byte aligned
{
#ifndef M_RTM
	if (!MACRO_ISALIGNED16(_pLight))
		Error_static("SIMD_ConvertRGBA", "Source pointer was not properly aligned.");
#ifdef PLATFORM_PS2
	if (!MACRO_ISALIGNED16(_pColors))
		Error_static("SIMD_ConvertRGBA", "Color pointer was not properly aligned.");
#endif
#endif		

#ifdef CPU_X86
	if (GetCPUFeatures() & CPU_FEATURE_SSE)
	{
		SSE_ConvertRGBA(_pLight, _pColors, _nV);
	}
	else
#elif defined(PLATFORM_DOLPHIN)
	if (true)
	{
		PPC_ConvertRGBA(_pLight, _pColors, _nV);
	}
	else
#elif defined( PLATFORM_PS2 )
		asm volatile ("
		
			and			$10, %0, 0x3		# Calc REST
			
			addi		$2, %0, -4			# Jump to REST if
			bltz		$2, _Rest			# less than 4

			pcpyld		%3, %3, %3			# extend 255 to all hwords
			
			
		_Loop:						 		# Do 4 simultaneous here

			lqc2		vf01, 0x00(%1)
			lqc2		vf02, 0x10(%1)
			lqc2		vf03, 0x20(%1)
			lqc2		vf04, 0x30(%1)
			
			vFTOI0		vf01, vf01
			vFTOI0		vf02, vf02
			vFTOI0		vf03, vf03
			vFTOI0		vf04, vf04

			qmfc2		$1, vf01
			qmfc2		$2, vf02
			qmfc2		$3, vf03
			qmfc2		$4, vf04

			ppach		$1, $2, $1
			ppach		$2, $4, $3
			pexeh		$1, $1				# Swap red and blue (det här borde inte behövas!)
			pexeh		$2, $2				# Swap red and blue (det här borde inte behövas!)
			
			pminh		$1, $1, %3
			pminh		$2, $2, %3
			
			ppacb		$1, $2, $1
			
			sq			$1, 0x00(%2)
			
			addi		%1, %1, 0x40
			addi		%2, %2, 0x10

			addi		%0, %0, -4
			bne			$10, %0, _Loop
			
			beqz		%0, _Done			# Jump to DONE if done
			
		_Rest:
			lqc2		vf01, 0x00(%1)
			lqc2		vf02, 0x10(%1)
			lqc2		vf03, 0x20(%1)
			
			vFTOI0		vf01, vf01
			qmfc2		$1, vf01
			ppach		$1, $0, $1
			pexeh		$1, $1				# Swap red and blue (det här borde inte behövas!)
			pminh		$1, $1, %3
			ppacb		$1, $0, $1
			sw			$1, 0x00(%2)

			addi		%0, %0, -1
			beqz		%0, _Done

			vFTOI0		vf02, vf02
			qmfc2		$1, vf02
			ppach		$1, $0, $1
			pexeh		$1, $1				# Swap red and blue (det här borde inte behövas!)
			pminh		$1, $1, %3
			ppacb		$1, $0, $1
			sw			$1, 0x04(%2)

			addi		%0, %0, -1
			beqz		%0, _Done

			vFTOI0		vf03, vf03
			qmfc2		$1, vf03
			ppach		$1, $0, $1
			pexeh		$1, $1				# Swap red and blue (det här borde inte behövas!)
			pminh		$1, $1, %3
			ppacb		$1, $0, $1
			sw			$1, 0x08(%2)

		_Done:
		"
		:
		: "r" (_nV), "r" (_pLight), "r" (_pColors), "r" (0x00ff00ff00ff00ffL)
		: "cc", "memory", "$10", "$1", "$2", "$3", "$4"
		);
	return;
#endif
	{
		CPixel32* pDest = (CPixel32*)_pColors;
		for (int v=0; v<_nV; v++)
		{
			pDest[v].R() = (int)Ceilx(_pLight[v][0], 255.0f);
			pDest[v].G() = (int)Ceilx(_pLight[v][1], 255.0f);
			pDest[v].B() = (int)Ceilx(_pLight[v][2], 255.0f);
			pDest[v].A() = (int)Ceilx(_pLight[v][3], 255.0f);
		}
	}
}


//----------------------------------------------------------------
//- Vector operations --------------------------------------------
//----------------------------------------------------------------

void SIMD_Mul(const fp32* _pSrc1, const fp32* _pSrc2, fp32* _pDst, int _n)
{
#ifndef M_RTM
	if (!MACRO_ISALIGNED16(_pSrc1))
		Error_static("SIMD_Mul", "Source1 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pSrc2))
		Error_static("SIMD_Mul", "Source2 pointer was not properly aligned.");
	if (!MACRO_ISALIGNED16(_pDst))
		Error_static("SIMD_Mul", "Destination pointer was not properly aligned.");
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

			mov ecx, _n
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
				mulps xmm0, xmm1
				movaps [eax], xmm0
				movaps xmm4, [esi+32]
				movaps xmm5, [edi+32]
				mulps xmm2, xmm3
				movaps [eax+16], xmm2
				movaps xmm6, [esi+48]
				movaps xmm7, [edi+48]
				add esi, 64
				add edi, 64
				mulps xmm4, xmm5
				add eax, 64
				mulps xmm6, xmm7
				movaps [eax+32-64], xmm4
				dec ecx
				movaps [eax+48-64], xmm6
			jnz loop2
		quit:
			emms
		}
	}
	else
#endif
	{
		for(int i = 0; i < _n*4; i++)
			_pDst[i] = _pSrc1[i]*_pSrc2[i];
	}
}

//----------------------------------------------------------------

/*
static void SIMD_Mul_SS_D(const fp32* _pSrc1, const fp32* _pSrc2, fp32* _pDst, int _n)
static void SIMD_Mul_SC_D(const fp32* _pSrc1, fp32 _Src2, fp32* _pDst, int _n)

static void SIMD_Mul_SSS_D(const fp32* _pSrc1, const fp32* _pSrc2, const fp32* _pSrc3, fp32* _pDst, int _n)
static void SIMD_Mul_SSC_D(const fp32* _pSrc1, const fp32* _pSrc2, fp32 _Src3, fp32* _pDst, int _n)

static void SIMD_Add_SS_D(const fp32* _pSrc1, const fp32* _pSrc2, fp32* _pDst, int _n)
static void SIMD_Add_SC_D(const fp32* _pSrc1, fp32 _Src2, fp32* _pDst, int _n)

static void SIMD_Div_SS_D(const fp32* _pSrc1, const fp32* _pSrc2, fp32* _pDst, int _n)
static void SIMD_Div_SC_D(const fp32* _pSrc1, fp32 _Src2, fp32* _pDst, int _n)

*/




int SIMD_Max(const int16* _pSrc, int _nValues, int _CurMax)
{
#if 0 //def PLATFORM_XBOX
//	if (MRTC_GOM()->m_SystemInfo.m_CPUFamily >= 6)
	{
		__asm
		{
			mov edx, [_CurMax]
			mov esi, [_pSrc]
			mov ecx, [_nValues]
			shr ecx, 2
			jz skip1
			lea edi, [esi + ecx*8]
	lp1:
				mov eax, [esi]
				movsx ebx, ax
				sar eax, 16
				cmp eax, edx
				cmovge edx, eax
				mov eax, [esi+4]
				cmp ebx, edx
				cmovge edx, ebx
				lea esi, [esi+8]
				movsx ebx, ax
				sar eax, 16
				cmp ebx, edx
				cmovge edx, ebx
				cmp eax, edx
				cmovge edx, eax
				cmp esi, edi
			jne lp1
	skip1:
			mov ecx, [_nValues]
			and ecx, 3
			jz skip2
			lea edi, [esi + ecx*2]
	lp2:
				xor eax, eax
				mov ax, [esi]
				cmp eax, edx
				lea esi, [esi+2]
				cmovge edx, eax
				cmp esi, edi
			jne lp2
	skip2:
			mov [_CurMax], edx
		}
	}
//	else
#else
	{
		const int16* pSrc = _pSrc;
		for(int i = 0; i < _nValues; i++)
		{
			int a = *(pSrc++);
			if (a > _CurMax)
				_CurMax = a;
		}
	}
#endif
	return _CurMax;
}


int SIMD_Max(const uint16* _pSrc, int _nValues, int _CurMax)
{
#if 0 //def PLATFORM_XBOX
//	if (MRTC_GOM()->m_SystemInfo.m_CPUFamily >= 6)
	{
		__asm
		{
			mov edx, [_CurMax]
			mov esi, [_pSrc]
			mov ecx, [_nValues]
			shr ecx, 2
			jz skip1
			lea edi, [esi + ecx*8]
	lp1:
				mov eax, [esi]
				movsx ebx, ax
				sar eax, 16
				cmp eax, edx
				cmovge edx, eax
				mov eax, [esi+4]
				cmp ebx, edx
				cmovge edx, ebx
				lea esi, [esi+8]
				movsx ebx, ax
				sar eax, 16
				cmp ebx, edx
				cmovge edx, ebx
				cmp eax, edx
				cmovge edx, eax
				cmp esi, edi
			jne lp1
	skip1:
			mov ecx, [_nValues]
			and ecx, 3
			jz skip2
			lea edi, [esi + ecx*2]
	lp2:
				xor eax, eax
				mov ax, [esi]
				cmp eax, edx
				lea esi, [esi+2]
				cmovge edx, eax
				cmp esi, edi
			jne lp2
	skip2:
			mov [_CurMax], edx
		}
	}
//	else
#else
	{
		const uint16* pSrc = _pSrc;
		for(int i = 0; i < _nValues; i++)
		{
			int a = *(pSrc++);
			if (a > _CurMax)
				_CurMax = a;
		}
	}
#endif
	return _CurMax;
}
int SIMD_Max(const uint8* _pSrc, int _nValues, int _CurMax)
{
#if 0 //def PLATFORM_XBOX
//	if (MRTC_SystemInfo::m_CPUFamily >= 6)
	{
		__asm
		{
			mov edx, [_CurMax]
			mov esi, [_pSrc]
			mov ecx, [_nValues]
			shr ecx, 2
			jz skip1
			lea edi, [esi + ecx*4]
			xor ebx, ebx
			xor ecx, ecx
	lp1:
				mov eax, [esi]
				mov bl, al
				mov cl, ah
				cmp bl, dl
				cmova edx, ebx
				cmp cl, dl
				cmova edx, ecx
				shr eax, 16
				lea esi, [esi+4]
				mov bl, al
				mov cl, ah
				cmp bl, dl
				cmova edx, ebx
				cmp cl, dl
				cmova edx, ecx
				cmp esi, edi
			jne lp1
	skip1:
			mov ecx, [_nValues]
			and ecx, 3
			jz skip2
			lea edi, [esi + ecx]
			xor eax, eax
	lp2:
				mov al, [esi]
				cmp al, dl
				lea esi, [esi+1]
				cmova edx, eax
				cmp esi, edi
			jne lp2
	skip2:
			mov [_CurMax], edx
		}
	}
#else
	{
		const uint8* pSrc = _pSrc;
		for(int i = 0; i < _nValues; i++)
		{
			int a = *(pSrc++);
			if (a > _CurMax)
				_CurMax = a;
		}
	}
#endif
	return _CurMax;
}


