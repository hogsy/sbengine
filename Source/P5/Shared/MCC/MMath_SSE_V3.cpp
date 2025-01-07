/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			SSE optimizations for CVec3Dfp4

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios AB 2002
					
	History:		
		020401:		Created File
\*____________________________________________________________________________________________*/

#include "PCH.h"
#include "MMath.h"

#if defined(CPU_X86)

void SSE_Multiply_VcMc_4x3(const TVector3Aggr<fp4>& _Src1, const CMat4Dfp4& _Mat, TVector3Aggr<fp4>& _Dst)
{
	__asm
	{
		mov edx, [_Mat]
		mov esi, [_Src1]

		movups xmm4, [edx]

		movss xmm0, [esi]

		movups xmm5, [edx+16]
		shufps xmm0, xmm0, 0

		movss xmm1, [esi+4]

		movups xmm6, [edx+32]
		mulps xmm0, xmm4
		shufps xmm1, xmm1, 0

		movss xmm2, [esi+8]

		movups xmm7, [edx+48]
		mulps xmm1, xmm5
		shufps xmm2, xmm2, 0

		mulps xmm2, xmm6

		addps xmm0, xmm7
		addps xmm2, xmm1
		addps xmm0, xmm2
		mov edi, [_Dst]
		movhlps xmm1, xmm0
		movlps [edi], xmm0
		movss [edi+8], xmm1
	}
}

void SSE_Box3Dfp4_And(CBox3Dfp4& _Target, const CBox3Dfp4& _Src)
{
	__asm
	{
		mov edi, [_Target]
		mov esi, [_Src]

		// Load target box: xmm0 = m_Min, xmm1 = m_Max
		movups xmm0, [edi]
		movlps xmm1, qword ptr [edi+12]
		movss xmm2, [edi+12+8]
		shufps xmm1, xmm2, 0+4

		// Load source box: xmm2 = m_Min, xmm3 = m_Max
		movups xmm2, [esi]
		movlps xmm3, qword ptr [esi+12]
		movss xmm4, [esi+12+8]
		shufps xmm3, xmm4, 0+4

		maxps xmm0, xmm2
		minps xmm1, xmm3

		// Write xmm0,xmm1 to _Target.m_Min/m_Max
		movhlps xmm4, xmm0
		movhlps xmm5, xmm1
		movlps qword ptr [edi], xmm0
		movss [edi+8], xmm4
		movlps qword ptr [edi+12], xmm1
		movss [edi+12+8], xmm5
	}
}

void SSE_Box3Dfp4_And(const CBox3Dfp4& _Src1, const CBox3Dfp4& _Src2, CBox3Dfp4& _Target)
{
	__asm
	{
		mov edi, [_Src1]
		mov esi, [_Src2]

		// Load target box: xmm0 = m_Min, xmm1 = m_Max
		movups xmm0, [edi]
		movlps xmm1, qword ptr [edi+12]
		movss xmm2, [edi+12+8]
		shufps xmm1, xmm2, 0+4

		// Load source box: xmm2 = m_Min, xmm3 = m_Max
		movups xmm2, [esi]
		movlps xmm3, qword ptr [esi+12]
		movss xmm4, [esi+12+8]
		shufps xmm3, xmm4, 0+4

		maxps xmm0, xmm2
		minps xmm1, xmm3

		// Write xmm0,xmm1 to _Target.m_Min/m_Max
		mov edi, [_Target]
		movhlps xmm4, xmm0
		movhlps xmm5, xmm1
		movlps qword ptr [edi], xmm0
		movss [edi+8], xmm4
		movlps qword ptr [edi+12], xmm1
		movss [edi+12+8], xmm5
	}
}

void SSE_Box3Dfp4_Expand(CBox3Dfp4& _Target, const CBox3Dfp4& _Src)
{
	__asm
	{
		mov edi, [_Target]
		mov esi, [_Src]

		// Load target box: xmm0 = m_Min, xmm1 = m_Max
		movups xmm0, [edi]
		movlps xmm1, qword ptr [edi+12]
		movss xmm2, [edi+12+8]
		shufps xmm1, xmm2, 0+4

		// Load source box: xmm2 = m_Min, xmm3 = m_Max
		movups xmm2, [esi]
		movlps xmm3, qword ptr [esi+12]
		movss xmm4, [esi+12+8]
		shufps xmm3, xmm4, 0+4

		minps xmm0, xmm2
		maxps xmm1, xmm3

		// Write xmm0,xmm1 to _Target.m_Min/m_Max
		movhlps xmm4, xmm0
		movhlps xmm5, xmm1
		movlps qword ptr [edi], xmm0
		movss [edi+8], xmm4
		movlps qword ptr [edi+12], xmm1
		movss [edi+12+8], xmm5
	}
}

void SSE_Box3Dfp4_Expand(CBox3Dfp4& _Target, const TVector3Aggr<fp4>& _Src)
{
	__asm
	{
		mov edi, [_Target]
		mov esi, [_Src]

		// Load target box: xmm0 = m_Min, xmm1 = m_Max
		movups xmm0, [edi]
		movlps xmm1, qword ptr [edi+12]
		movss xmm2, [edi+12+8]
		shufps xmm1, xmm2, 0+4

		// Load vector: xmm2 = _Src
		movlps xmm2, qword ptr [esi]
		movss xmm3, [esi+8]
		shufps xmm2, xmm3, 0+4

		minps xmm0, xmm2
		maxps xmm1, xmm2

		// Write xmm0,xmm1 to _Target.m_Min/m_Max
		movhlps xmm4, xmm0
		movhlps xmm5, xmm1
		movlps qword ptr [edi], xmm0
		movss [edi+8], xmm4
		movlps qword ptr [edi+12], xmm1
		movss [edi+12+8], xmm5
	}
}

bool SSE_Box3Dfp4_IsInside(const CBox3Dfp4& _Src0, const CBox3Dfp4& _Src1)
{
	bool bRet;
	__asm
	{
		mov edi, [_Src0]
		mov esi, [_Src1]

		// Load target box: xmm0 = m_Min, xmm1 = m_Max
		movlps xmm0, qword ptr [edi]
		movss xmm2, [edi+8]
		shufps xmm0, xmm2, 0+4

		movlps xmm1, qword ptr [edi+12]
		movss xmm2, [edi+12+8]
		shufps xmm1, xmm2, 0+4

		// Load source box: xmm2 = m_Min, xmm3 = m_Max
		movlps xmm2, qword ptr [esi]
		movss xmm4, [esi+8]
		shufps xmm2, xmm4, 0+4

		movlps xmm3, qword ptr [esi+12]
		movss xmm4, [esi+12+8]
		shufps xmm3, xmm4, 0+4

		cmpltps xmm1, xmm2
		cmpltps xmm3, xmm0
		movmskps eax, xmm1
		movmskps ebx, xmm3
		or eax, ebx
		mov ecx, 1
		xor ebx, ebx
		cmp eax, 0
		cmove ebx, ecx
		mov bRet, bl
	}

	return bRet;
}

#endif
