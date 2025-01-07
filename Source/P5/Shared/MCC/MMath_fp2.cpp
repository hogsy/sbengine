
#include "Pch.h"
#include "MMath_fp2.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| SSE version
|__________________________________________________________________________________________________
\*************************************************************************************************/

#if 0
#ifdef CPU_X86

class CXMMReg
{
public:
	union
	{
		fp32 kf[4];
		uint32 ki[4];
	};
};

#define COPY_VEC4FP2_TO_XMM(Frag, XMM) \
{	uint32 a = *((uint32*)&Frag.k[0]);	\
	uint32 b = *((uint32*)&Frag.k[2]);	\
	XMM.ki[0] = a << 16;					\
	XMM.ki[1] = a & 0xffff0000;			\
	XMM.ki[2] = b << 16;					\
	XMM.ki[3] = b & 0xffff0000; }

#define COPY_XMM_TO_VEC4FP2(XMM, Frag) \
{	uint32 a = (XMM.ki[0] >> 16) + (XMM.ki[1] & 0xffff0000);	\
	uint32 b = (XMM.ki[2] >> 16) + (XMM.ki[3] & 0xffff0000);	\
	*((uint32*)&Frag.k[0]) = a;	\
	*((uint32*)&Frag.k[2]) = b;	}

/*
#define COPY2_FRAGFP2_TO_XMM(_pOper1, _pOper2, _TempXMM0, _TempXMM1)	\
			mov edi, [_pOper1]			\
			mov esi, [_pOper2]			\
			movq mm0, qword ptr [edi]	\
			pxor mm1, mm1				\
			pxor mm3, mm3				\
			movq mm4, qword ptr [esi]	\
			pxor mm5, mm5				\
			movq mm2, mm0				\
			pxor mm7, mm7				\
			punpcklwd mm1, mm0			\
			punpckhwd mm3, mm2			\
			movq mm6, mm4				\
			movq [_TempXMM0], mm1		\
			punpcklwd mm5, mm4			\
			movq [_TempXMM0+8], mm3		\
			punpckhwd mm7, mm6			\
			movq [_TempXMM1], mm5		\
			movq [_TempXMM1+8], mm7
*/

void operator<< (CVec4Dfp32& _Dst, const CVec4Dfp2& _Src)
{
	CXMMReg& XMM = *(CXMMReg*)&_Dst;
	COPY_VEC4FP2_TO_XMM(_Src, XMM);
}

void operator<< (CVec4Dfp2& _Dst, const CVec4Dfp32& _Src)
{
	const CXMMReg& XMM = *(CXMMReg*)&_Src;
	COPY_XMM_TO_VEC4FP2(XMM, _Dst);
}

__forceinline static void COPY2_FRAGFP2_TO_XMM(const class CVec4Dfp2* _pOper1, const class CVec4Dfp2* _pOper2, CXMMReg* _pXMM0, CXMMReg* _pXMM1)
{
	__asm
	{
			emms
			mov edi, [_pOper1]
			mov esi, [_pOper2]
			movq mm0, qword ptr [edi]
			pxor mm1, mm1
			mov edi, [_pXMM0]
			pxor mm3, mm3
			movq mm4, qword ptr [esi]
			pxor mm5, mm5
			mov esi, [_pXMM1]
			movq mm2, mm0
			pxor mm7, mm7
			punpcklwd mm1, mm0
			punpckhwd mm3, mm2
			movq mm6, mm4
			movq [edi], mm1
			punpcklwd mm5, mm4
			movq [edi+8], mm3
			punpckhwd mm7, mm6
			movq [esi], mm5
			movq [esi+8], mm7
			emms
	}
}

void CVec4Dfp2::Add(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, CVec4Dfp2& _Dest)
{
	CXMMReg TempXMM0;
	CXMMReg TempXMM1;

/*		__asm
	{
		mov edi, [_Oper1]
		mov esi, [_Oper2]
		movq mm0, qword ptr [edi]
		pxor mm1, mm1
		pxor mm3, mm3
		movq mm4, qword ptr [esi]
		pxor mm5, mm5
		movq mm2, mm0
		pxor mm7, mm7
		punpcklwd mm1, mm0
		punpckhwd mm3, mm2
		movq mm6, mm4
		movq [TempXMM0], mm1
		punpcklwd mm5, mm4
		movq [TempXMM0+8], mm3
		punpckhwd mm7, mm6
		movq [TempXMM1], mm5
		movq [TempXMM1+8], mm7
		emms
	}*/

//		COPY2_FRAGFP2_TO_XMM(&_Oper1, &_Oper2, &TempXMM0, &TempXMM1);

	COPY_VEC4FP2_TO_XMM(_Oper1, TempXMM0);
	COPY_VEC4FP2_TO_XMM(_Oper2, TempXMM1);
	__asm
	{
		movups xmm0, [TempXMM0]
		movups xmm1, [TempXMM1]
		addps xmm0, xmm1
		movups [TempXMM0], xmm0
	}
	COPY_XMM_TO_VEC4FP2(TempXMM0, _Dest);
/*		__asm
	{
		mov edi, [_Dest]
		mov eax, [TempXMM0]
		mov ebx, [TempXMM0+4]
		mov ecx, [TempXMM0+8]
		mov edx, [TempXMM0+12]
		shr eax, 16
		shr ecx, 16
		mov bx, ax
		mov dx, cx
		mov [edi], ebx
		mov [edi+4], edx
	}*/
}

void CVec4Dfp2::Scale(const CVec4Dfp2& _Oper1, fp32 _Factor, CVec4Dfp2& _Dest)
{
	CXMMReg TempXMM0;

	__asm
	{
		// MMX , single load
		mov edi, [_Oper1]
		pxor mm1, mm1
		movq mm0, [edi]
		pxor mm3, mm3
		movq mm2, mm0
		punpcklwd mm1, mm0
		punpckhwd mm3, mm2
		movq [TempXMM0], mm1
		movq [TempXMM0+8], mm3

		movss xmm1, [_Factor]
		movups xmm0, [TempXMM0]
		shufps xmm1, xmm1, 0

		mulps xmm0, xmm1
		movups [TempXMM0], xmm0

		emms
	}

	COPY_XMM_TO_VEC4FP2(TempXMM0, _Dest);
}

void CVec4Dfp2::Multiply(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, CVec4Dfp2& _Dest)
{
	CXMMReg TempXMM0;
	CXMMReg TempXMM1;
/*
	COPY_VEC4FP2_TO_XMM(_Oper1, TempXMM0);
	COPY_VEC4FP2_TO_XMM(_Oper2, TempXMM1);
	__asm
	{
		movups xmm0, [TempXMM0]
		movups xmm1, [TempXMM1]
		mulps xmm0, xmm1
		movups [TempXMM0], xmm0
	}
	COPY_XMM_TO_VEC4FP2(TempXMM0, _Dest);
*/
#if 1
	__asm
	{
		// MMX, double load
		mov edi, [_Oper1]
		mov esi, [_Oper2]
		movq mm0, [edi]
		pxor mm1, mm1
		pxor mm3, mm3
		movq mm4, [esi]
		pxor mm5, mm5
		movq mm2, mm0
		pxor mm7, mm7
		punpcklwd mm1, mm0
		punpckhwd mm3, mm2
		movq mm6, mm4
		movq [TempXMM0], mm1
		punpcklwd mm5, mm4
		movq [TempXMM0+8], mm3
		punpckhwd mm7, mm6
		movq [TempXMM1], mm5
		movq [TempXMM1+8], mm7


		// MMX , single load
/*			mov edi, [_Oper1]
		pxor mm1, mm1
		movq mm0, [edi]
		pxor mm3, mm3
		movq mm2, mm0
		punpcklwd mm1, mm0
		punpckhwd mm3, mm2
		movq [TempXMM0], mm1
		movq [TempXMM0+8], mm3*/


		movups xmm0, [TempXMM0]
		movups xmm1, [TempXMM1]
		mulps xmm0, xmm1
		movups [TempXMM0], xmm0

/*			mov edi, [_Dest]
		mov eax, [TempXMM0]
		mov ebx, [TempXMM0+4]
		mov ecx, [TempXMM0+8]
		mov edx, [TempXMM0+12]
		shr eax, 16
		shr ecx, 16
		mov bx, ax
		mov dx, cx
		mov [edi], ebx
		mov [edi+4], edx
*/
		emms
	}

	COPY_XMM_TO_VEC4FP2(TempXMM0, _Dest);
#endif
}

void CVec4Dfp2::MultiplyAdd(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, const CVec4Dfp2& _Oper3, CVec4Dfp2& _Dest)
{
	// ~ 54c

	CXMMReg TempXMM0;
	CXMMReg TempXMM1;

	__asm
	{
		// MMX, double load
		mov edi, [_Oper1]
		mov esi, [_Oper2]
		movq mm0, [edi]
		pxor mm1, mm1
		pxor mm3, mm3
		movq mm4, [esi]
		pxor mm5, mm5
		movq mm2, mm0
		pxor mm7, mm7
		punpcklwd mm1, mm0
		punpckhwd mm3, mm2
		movq mm6, mm4
		movq [TempXMM0], mm1
		punpcklwd mm5, mm4
		movq [TempXMM0+8], mm3
		punpckhwd mm7, mm6
		movq [TempXMM1], mm5
		movq [TempXMM1+8], mm7

		movups xmm0, [TempXMM0]
		movups xmm1, [TempXMM1]
		mulps xmm0, xmm1

		// MMX , single load
		mov edi, [_Oper3]
		pxor mm1, mm1
		movq mm0, [edi]
		pxor mm3, mm3
		movq mm2, mm0
		punpcklwd mm1, mm0
		punpckhwd mm3, mm2
		movq [TempXMM0], mm1
		movq [TempXMM0+8], mm3

		movups xmm2, [TempXMM0]
		addps xmm0, xmm2
		movups [TempXMM0], xmm0

		emms
	}

	COPY_XMM_TO_VEC4FP2(TempXMM0, _Dest);
}

void CVec4Dfp2::Lerp(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, CVec4Dfp2& _Dest, fp32 _t)
{
	CXMMReg TempXMM0;
	CXMMReg TempXMM1;

	COPY_VEC4FP2_TO_XMM(_Oper1, TempXMM0);
	COPY_VEC4FP2_TO_XMM(_Oper2, TempXMM1);

	__asm
	{
		movss xmm2, [_t]
		movups xmm0, [TempXMM0]
		movups xmm1, [TempXMM1]
		shufps xmm2, xmm2, 0

		subps xmm1, xmm0
		mulps xmm1, xmm2
		addps xmm1, xmm0

		movups [TempXMM0], xmm1
	}

	COPY_XMM_TO_VEC4FP2(TempXMM0, _Dest);
}

#else

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Default version
|__________________________________________________________________________________________________
\*************************************************************************************************/

void operator<< (CVec4Dfp32& _Dst, const CVec4Dfp2& _Src)
{
	_Dst.k[0] = _Src.k[0];
	_Dst.k[1] = _Src.k[1];
	_Dst.k[2] = _Src.k[2];
	_Dst.k[3] = _Src.k[3];
}

void operator<< (CVec4Dfp2& _Dst, const CVec4Dfp32& _Src)
{
	_Dst.k[0] = _Src.k[0];
	_Dst.k[1] = _Src.k[1];
	_Dst.k[2] = _Src.k[2];
	_Dst.k[3] = _Src.k[3];
}

void CVec4Dfp2::Add(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, CVec4Dfp2& _Dest)
{
	_Dest.k[0] = _Oper1.k[0] + _Oper2.k[0];
	_Dest.k[1] = _Oper1.k[1] + _Oper2.k[1];
	_Dest.k[2] = _Oper1.k[2] + _Oper2.k[2];
	_Dest.k[3] = _Oper1.k[3] + _Oper2.k[3];
}

void CVec4Dfp2::Scale(const CVec4Dfp2& _Oper1, fp32 _Factor, CVec4Dfp2& _Dest)
{
	_Dest.k[0] = _Oper1.k[0] * _Factor;
	_Dest.k[1] = _Oper1.k[1] * _Factor;
	_Dest.k[2] = _Oper1.k[2] * _Factor;
	_Dest.k[3] = _Oper1.k[3] * _Factor;
}

void CVec4Dfp2::Multiply(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, CVec4Dfp2& _Dest)
{
	_Dest.k[0] = _Oper1.k[0] * _Oper2.k[0];
	_Dest.k[1] = _Oper1.k[1] * _Oper2.k[1];
	_Dest.k[2] = _Oper1.k[2] * _Oper2.k[2];
	_Dest.k[3] = _Oper1.k[3] * _Oper2.k[3];
}

void CVec4Dfp2::MultiplyAdd(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, const CVec4Dfp2& _Oper3, CVec4Dfp2& _Dest)
{
	_Dest.k[0] = _Oper3.k[0] + _Oper1.k[0] * _Oper2.k[0];
	_Dest.k[1] = _Oper3.k[1] + _Oper1.k[1] * _Oper2.k[1];
	_Dest.k[2] = _Oper3.k[2] + _Oper1.k[2] * _Oper2.k[2];
	_Dest.k[3] = _Oper3.k[3] + _Oper1.k[3] * _Oper2.k[3];
}

void CVec4Dfp2::Lerp(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, CVec4Dfp2& _Dest, fp32 _t)
{
	_Dest.k[0] = _Oper1.k[0] + _t * (_Oper2.k[0] - _Oper1.k[0]);
	_Dest.k[1] = _Oper1.k[1] + _t * (_Oper2.k[1] - _Oper1.k[1]);
	_Dest.k[2] = _Oper1.k[2] + _t * (_Oper2.k[2] - _Oper1.k[2]);
	_Dest.k[3] = _Oper1.k[3] + _t * (_Oper2.k[3] - _Oper1.k[3]);
}


#endif
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Multiplatform code
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CVec4Dfp2Aggr::ParseColor(const CStr& _s, bool _bHexUnit)
{
	if (_s.CompareSubStr("0x") == 0)
	{
		// Hex color
		CVec4Dfp32 v;
		int c = _s.Val_int();
		v.k[0] = (c >> 16) & 0xff;
		v.k[1] = (c >> 8) & 0xff;
		v.k[2] = (c >> 0) & 0xff;
		v.k[3] = (c >> 24) & 0xff;
		if (_bHexUnit)
			v *= 1.0f / 255.0f;
		*this << v;
	}
	else
	{
		const char* pStr = (const char*) _s;
		if (!pStr) { SetZero(); return; }
		int len = CStr::StrLen(pStr);
		int pos = 0;
		pos = CStr::GoToDigit(pStr, pos, len);
		int i = 0;
		for(; i < 4; i++)
		{
			k[i].Set(M_AToF(&pStr[pos]));
			pos = CStr::SkipADigit(pStr, pos, len);
			pos = CStr::GoToDigit(pStr, pos, len);
			if (pos == len) break;
		}

		if (i == 0)
		{
			// Replicate scalar value
			k[1] = k[0];
			k[2] = k[0];
			k[3] = k[0];
		}
		else if (i == 2)
		{
			// 3-components was specified, set alpha to 1
			k[3].Set(_bHexUnit ? 1.0f : 255.0f);
		}
	}
}

void CVec4Dfp2Aggr::Read(CCFile* _pFile)
{
	_pFile->ReadLE((uint16*)k, 4);
}

void CVec4Dfp2Aggr::Write(CCFile* _pFile) const
{
	_pFile->WriteLE((const uint16*)k, 4);
}

void CVec4Dfp2Aggr::SwapLE()
{
	k[0].SwapLE();
	k[1].SwapLE();
	k[2].SwapLE();
	k[3].SwapLE();
}
