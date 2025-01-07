#include "PCH.h"

// #define CPU_VEC128EMU

#include "MRTC_Callgraph.h"
#include "../../Shared/MOS/MMain.h"
#include "../../Shared/MOS/Classes/Render/MRenderCapture.h"
#include "../../Shared/MOS/Classes/Win/MWinGrph.h"
#include "../../Shared/MOS/XR/XREngineVar.h"
#include "../../Shared/MOS/XR/XRShader.h"

#include "../../Shared/MOS/MSystem/MSystem_Core.h"

#include "MMath_Vec128.h"

#ifdef CPU_VEC128EMU 
#ifdef CPU_SSE2
struct CM128Conv
{
	union
	{
		__m128 f;
		__m128i i;
	};
};

M_FORCEINLINE __m128 M128Convf(__m128i a)
{
	CM128Conv r;
	r.i = a;
	return r.f;
}

M_FORCEINLINE __m128i M128Convi(__m128 a)
{
	CM128Conv r;
	r.f = a;
	return r.i;
}

M_FORCEINLINE vec128 M_Vfp32toi32(vec128 a) { return M128Convf(_mm_cvttps_epi32(a)); }
M_FORCEINLINE vec128 M_Vi32tofp32(vec128 a) { return _mm_cvtepi32_ps(M128Convi(a)); }
M_FORCEINLINE vec128 M_VTrunc(vec128 a) { return M_Vi32tofp32(M_Vfp32toi32(a)); }
M_FORCEINLINE vec128 M_VNMSub(vec128 a, vec128 b, vec128 c) { return M_VSub(c, M_VMul(a, b)); }
#else

M_FORCEINLINE vec128 M_Vfp32toi32(vec128 a) { return M_VLd(0,0,0,0); }
M_FORCEINLINE vec128 M_Vi32tofp32(vec128 a) { return M_VLd(0,0,0,0); }
M_FORCEINLINE vec128 M_VTrunc(vec128 a) { return M_VLd(0,0,0,0); }
M_FORCEINLINE vec128 M_VNMSub(vec128 a, vec128 b, vec128 c) { return M_VLd(0,0,0,0); }

#endif
#endif

#if !defined(CPU_VEC128EMU) && defined(CPU_POWERPC)

M_FORCEINLINE vec128 M_VNot(vec128 _a)
{
	CVec128Access a(_a);
	CVec128Access ret;
	ret.ku32[0] = ~a.ku32[0];
	ret.ku32[1] = ~a.ku32[1];
	ret.ku32[2] = ~a.ku32[2];
	ret.ku32[3] = ~a.ku32[3];
	return ret.v;
}

M_FORCEINLINE vec128 M_VAddh(vec128 _a)
{
	CVec128Access a(_a);
	CVec128Access ret;
	fp32 sum = a.k[0] + a.k[1] + a.k[2] + a.k[3];
	ret.k[0] = sum;
	ret.k[1] = sum;
	ret.k[2] = sum;
	ret.k[3] = sum;
	return ret.v;
}

M_FORCEINLINE vec128 M_VAndNot(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ku32[0] = a.ku32[0] & ~b.ku32[0];
	ret.ku32[1] = a.ku32[1] & ~b.ku32[1];
	ret.ku32[2] = a.ku32[2] & ~b.ku32[2];
	ret.ku32[3] = a.ku32[3] & ~b.ku32[3];
	return ret.v;
}
#endif

#include <float.h>

CCFile gFile;
uint32 giLine = 0;
uint32 glNaNVal[3];

CFStr u32toBin(uint32 _Num)
{
	CFStr Ret;

	uint32 i;
	for(i = (1 << 31);i > 0;i /= 2)
	{
		Ret += (_Num & i) ? "1" : "0";
	}

	return Ret;
}

CFStr u32toNaN(uint32 _Num)
{
	if( _Num == glNaNVal[0] ) return "NaN ";
	if( _Num == glNaNVal[1] ) return "+Inf";
	if( _Num == glNaNVal[2] ) return "-Inf";
	return "    ";
}

void out(const char * _pDesc,const vec128 & _Vec)
{
	//CVec4Dfp32 v4 = M_VGetV4_Slow(_Vec);
	CVec4Dfp32 v4;
	uint32 *pK = (uint32*)v4.k;
	M_VSt(_Vec,v4.k);
	gFile.Writeln(CStrF("%d (%s): %f %f %f %f",giLine,_pDesc,v4.k[0],v4.k[1],v4.k[2],v4.k[3]));
	gFile.Writeln(CStrF("         %s %s %s %s",u32toBin(pK[0]).Str(),u32toBin(pK[1]).Str(),u32toBin(pK[2]).Str(),u32toBin(pK[3]).Str()));
	gFile.Writeln(CStrF("         %s %s %s %s",u32toNaN(pK[0]).Str(),u32toNaN(pK[1]).Str(),u32toNaN(pK[2]).Str(),u32toNaN(pK[3]).Str()));
	giLine++;
}

void out(const char * _pDesc,uint _Out)
{
	gFile.Writeln(CStrF("%d (%s): %d",giLine,_pDesc,_Out));
//	gFile.Writeln(CStrF("         %s",u32toBin(_Out).Str()));
	giLine++;
}

void out(const char * _pDesc,const CStr &_String)
{
	gFile.Writeln(CStrF("%d (%s): %s",giLine,_pDesc,_String.Str()));
	giLine++;
}

//Disable "potential divide by 0" warning... Since that's what we *want* to do!
#pragma warning(disable:4723)

void DumpV128tests(const char *_FileName)
{
	const fp32 NumZero = 0.0f;
	//*
	const fp32 lValues[10] = 
		{ 0.0f / NumZero, 0.0f,
		  1.0f / NumZero, -1.0f / NumZero,
		  FLT_MAX, -FLT_MAX,
		  FLT_MIN, -FLT_MIN,
		  100.0f, -100.0f };
	char lpValDesc[10][5] =
	{
		"NaN ","Zero","+Inf","-Inf","+Max","-Max","+Min","-Min","+100","-100"
	};
		//  */
	/*
	const fp32 lValues[10] =
	{
		1.0f,22.0f,333.0f,4444.0f,55555.0f,
		6.0f,77.0f,88.0f,99.0f,1000.0f
	};
	char lpValDesc[10][6] =
	{
		"1    ","22   ","333  ","4444 ","55555","6    ","77   ","88   ","99   ","1000 "
	};
	//*/

	{
		vec128 tst = M_VLd(lValues[0],lValues[2],lValues[3],lValues[1]);
		CVec4Dfp32 v; M_VSt(tst,v.k);
		glNaNVal[0] = ((uint32*)v.k)[0];
		glNaNVal[1] = ((uint32*)v.k)[1];
		glNaNVal[2] = ((uint32*)v.k)[2];
	}

	uint32 i;

	gFile.Open(_FileName,CFILE_WRITE);

#ifdef CPU_SSE2
	gFile.Writeln("SSE2 Defined");
#endif
#ifdef CPU_POWERPC
	gFile.Writeln("PPC Defined");
#endif
#ifdef CPU_VEC128EMU
	gFile.Writeln("V128EMU Defined");
#endif

	{
		vec128 dst = M_VZero();
		out("Zero",dst);
		dst = M_VHalf(); out("Half",dst);
		dst = M_VOne(); out("One",dst);
		dst = M_VTwo(); out("Two",dst);

		dst = M_VConst(1.0f,2.0f,3.0f,4.0f); out("Const",dst);
		dst = M_VConstMsk(1,2,3,4); out("ConstMsk",dst);
		dst = M_VConst_i8(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16); out("Const_i8",dst);
		dst = M_VConst_u8(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16); out("Const_u8",dst);
		dst = M_VConst_i16(1,2,3,4,5,6,7,8); out("Const_i16",dst);
		dst = M_VConst_u16(1,2,3,4,5,6,7,8); out("Const_u16",dst);

		dst = M_VScalar(3.1415926f); out("Scalar",dst);
		dst = M_VScalar_i8(1); out("Scalar_i8",dst);
		dst = M_VScalar_u8(2); out("Scalar_u8",dst);
		dst = M_VScalar_i16(3); out("Scalar_i16",dst);
		dst = M_VScalar_u16(4); out("Scalar_u16",dst);
		dst = M_VScalar_i32(5); out("Scalar_i32",dst);
		dst = M_VScalar_u32(6); out("Scalar_u32",dst);

		/*
		dst = M_VLdMem(lValues); out("LdMem",dst);
		dst = M_VLdScalar(lValues[1]); out("LdScalar",dst);

		// SSE only
		dst = M_VLdScalar_i8(123); out("LdScalar_i8",dst);
		dst = M_VLdScalar_u8(243); out("LdScalar_u8",dst);
		dst = M_VLdScalar_i16(12432); out("LdScalar_i16",dst);
		dst = M_VLdScalar_u16(43202); out("LdScalar_u16",dst);
		dst = M_VLdScalar_i32(123934993); out("LdScalar_i32",dst);
		dst = M_VLdScalar_u32(4012331334); out("LdScalar_u32",dst);
		*/

		CVec4Dfp32 vec4(1.0f,2.0f,3.0f,4.0f);
		dst = M_VLd(vec4.k[0],vec4.k[1],vec4.k[2],vec4.k[3]); out("Ld",dst);
		// dst = M_VLd(vec4.k); out("Ld (vec)",dst);
	}

	for(i = 0;i < 10;i++)
	{
		uint8 liVal[4] = {i,(i+9)%10,(i+8)%10,(i+7)%10};
		vec128 src = M_VLd(lValues[liVal[0]],lValues[liVal[1]],lValues[liVal[2]],lValues[liVal[3]]);
		vec128 dst;

		gFile.Writeln("-------------------------------------------");
		gFile.Writeln(CStrF("- %s %s %s %s",lpValDesc[liVal[0]],lpValDesc[liVal[1]],lpValDesc[liVal[2]],lpValDesc[liVal[3]]));
		out("Single",src);

		const uint8 iVec = 3 + (2 << 2) + (0 << 4) + (1 << 6);
		dst = M_VShuf(src,iVec); out("Shuf",dst);
		dst = M_VSplat(src,3); out("Splat",dst);
		dst = M_VSplatX(src); out("SplatX",dst);
		dst = M_VSplatY(src); out("SplatY",dst);
		dst = M_VSplatZ(src); out("SplatZ",dst);
		dst = M_VSplatW(src); out("SplatW",dst);

		dst = M_VTrunc(src); out("Trunc",dst);
		dst = M_Vfp32toi32(src); out("fp32toi32",dst);
		dst = M_Vi32tofp32(src); out("i32tofp32",dst);

		dst = M_VNot(src); out("Not",dst);

		dst = M_VAddh(src); out("Addh",dst);

		dst = -src; out("Neg",dst);

		dst = M_VRcp_Est(src); out("Rcp_Est",dst);
		dst = M_VRsq_Est(src); out("Rsq_Est",dst);
		dst = M_VSqrt_Est(src); out("Sqrt_Est",dst);

		dst = M_VRcp(src); out("Rcp",dst);
		dst = M_VRsq(src); out("Rsq",dst);
		dst = M_VSqrt(src); out("Sqrt",dst);

		dst = M_VAbs(src); out("Abs",dst);
		dst = M_VClamp01(src); out("Clamp01",dst);

		dst = M_VDp3(src,src); out("Dp3(Self)",dst);
		dst = M_VDp4(src,src); out("Dp4(Self)",dst);

		dst = M_VLen3(src); out("Len3",dst);
		dst = M_VLen4(src); out("Len4",dst);
		dst = M_VLenrcp3(src); out("Lenrcp3",dst);
		dst = M_VLenrcp4(src); out("Lenrcp4",dst);
		dst = M_VLenrcp3_Est(src); out("Lenrcp3_Est",dst);
		dst = M_VLenrcp4_Est(src); out("Lenrcp4_Est",dst);

		dst = M_VNrm3(src); out("Nrm3",dst);
		dst = M_VNrm3_Est(src); out("Nrm3_Est",dst);
		dst = M_VNrm4(src); out("Nrm4",dst);
		dst = M_VNrm4_Est(src); out("Nrm4_Est",dst);

		CStr strdst;
		strdst = M_VStr(src); out("Str",strdst);
		strdst = M_VStr_u32(src); out("Str_u32",strdst);
		strdst = M_VStr_i32(src); out("Str_i32",strdst);
		strdst = M_VStr_u16(src); out("Str_u16",strdst);
		strdst = M_VStr_i16(src); out("Str_i16",strdst);
		strdst = M_VStr_u8(src); out("Str_u8",strdst);
		strdst = M_VStr_i8(src); out("Str_i8",strdst);
	}

	for(i = 0;i < 100;i++)
	{
		vec128 src[2];
		uint8 liVal[8] = { (i/10),(i/10 + 7)%10,(i/10 + 4)%10,(i/10 + 1)%10,
			(i%10),(i%10 + 3)%10,(i%10 + 6)%10,(i%10 + 9)%10 };
		src[0] = M_VLd(lValues[liVal[0]],lValues[liVal[1]],lValues[liVal[2]],lValues[liVal[3]]);
		src[1] = M_VLd(lValues[liVal[4]],lValues[liVal[5]],lValues[liVal[6]],lValues[liVal[7]]);
		vec128 dst;//,dst2;

		gFile.Writeln("-------------------------------------------");
		gFile.Writeln(CStrF("- %s %s %s %s",lpValDesc[liVal[0]],lpValDesc[liVal[1]],lpValDesc[liVal[2]],lpValDesc[liVal[3]]));
		gFile.Writeln(CStrF("- %s %s %s %s",lpValDesc[liVal[4]],lpValDesc[liVal[5]],lpValDesc[liVal[6]],lpValDesc[liVal[7]]));
		out("Double1",src[0]);
		out("Double2",src[1]);

		// TODO - test other initializers

		dst = M_VMrgXY(src[0],src[1]); out("MrgXY",dst);
		dst = M_VMrgZW(src[0],src[1]); out("MrgZW",dst);

		dst = M_VOr(src[0],src[1]); out("Or",dst);
		dst = M_VAnd(src[0],src[1]); out("And",dst);
		dst = M_VAndNot(src[0],src[1]); out("AndNot",dst);
		dst = M_VXor(src[0],src[1]); out("Xor",dst);

		dst = M_VCmpEqMsk(src[0],src[1]); out("CmpEqMsk",dst);
		dst = M_VCmpGEMsk(src[0],src[1]); out("CmpGEMsk",dst);
		dst = M_VCmpGTMsk(src[0],src[1]); out("CmpGTMsk",dst);
		dst = M_VCmpLEMsk(src[0],src[1]); out("CmpLEMsk",dst);
		dst = M_VCmpLTMsk(src[0],src[1]); out("CmpLTMsk",dst);

		uint intdst;
		intdst = M_VCmpAllEq(src[0],src[1]); out("CmpAllEq",intdst);
		intdst = M_VCmpAllGE(src[0],src[1]); out("CmpAllGE",intdst);
		intdst = M_VCmpAllGT(src[0],src[1]); out("CmpAllGT",intdst);
		intdst = M_VCmpAllLE(src[0],src[1]); out("CmpAllLE",intdst);
		intdst = M_VCmpAllLT(src[0],src[1]); out("CmpAllLT",intdst);

		intdst = M_VCmpAnyEq(src[0],src[1]); out("CmpAnyEq",intdst);
		intdst = M_VCmpAnyGE(src[0],src[1]); out("CmpAnyGE",intdst);
		intdst = M_VCmpAnyGT(src[0],src[1]); out("CmpAnyGT",intdst);
		intdst = M_VCmpAnyLE(src[0],src[1]); out("CmpAnyLE",intdst);
		intdst = M_VCmpAnyLT(src[0],src[1]); out("CmpAnyLT",intdst);

		intdst = (src[0] == src[1]); out("Eq",intdst);
		intdst = (src[0] >= src[1]); out("GE",intdst);
		intdst = (src[0] > src[1]); out("GT",intdst);
		intdst = (src[0] <= src[1]); out("LE",intdst);
		intdst = (src[0] < src[1]); out("LT",intdst);

		dst = M_VAdd(src[0],src[1]); out("Add",dst);
		dst = M_VSub(src[0],src[1]); out("Sub",dst);
		dst = M_VMul(src[0],src[1]); out("Mul",dst);
		dst = M_VMin(src[0],src[1]); out("Min",dst);
		dst = M_VMax(src[0],src[1]); out("Max",dst);
		dst = M_VDp3(src[0],src[1]); out("Dp3",dst);
		dst = M_VDp4(src[0],src[1]); out("Dp4",dst);
		dst = M_VXpd(src[0],src[1]); out("Xpd",dst);

		dst = src[0] + src[1]; out("OpAdd",dst);
		dst = src[0] - src[1]; out("OpSub",dst);

		dst = M_VNegMsk(src[0],src[1]); out("NegMsk",dst);

		dst = M_VLen3x2(src[0],src[1]); out("Len3x2",dst);
		dst = M_VLen4x2(src[0],src[1]); out("Len4x2",dst);
		dst = M_VLenrcp3x2(src[0],src[1]); out("Lenrcp3x2",dst);
		dst = M_VLenrcp4x2(src[0],src[1]); out("Lenrcp4x2",dst);
		dst = M_VLenrcp3x2_Est(src[0],src[1]); out("Lenrcp3x2_Est",dst);
		dst = M_VLenrcp4x2_Est(src[0],src[1]); out("Lenrcp4x2_Est",dst);
		
		/* Only definitions, no need to test
		dst = src[0];dst2 = src[1]; M_VNrm3x2(dst,dst2); out(dst); out(dst2);
		dst = src[0];dst2 = src[1]; M_VNrm3x2_Est(dst,dst2); out(dst); out(dst2);
		dst = src[0];dst2 = src[1]; M_VNrm4x2(dst,dst2); out(dst); out(dst2);
		dst = src[0];dst2 = src[1]; M_VNrm4x2_Est(dst,dst2); out(dst); out(dst2);
		//*/

		dst = M_VQuatMul(src[0],src[1]); out("QuatMul",dst);

		dst = M_VAdds_i8(src[0],src[1]); out("Adds_i8",dst);
		dst = M_VAdds_i16(src[0],src[1]); out("Adds_i16",dst);
		dst = M_VSubs_i8(src[0],src[1]); out("Subs_i8",dst);
		dst = M_VSubs_i16(src[0],src[1]); out("Subs_i16",dst);

		dst = M_VMin_i8(src[0],src[1]); out("Min_i8",dst);
		dst = M_VMin_i16(src[0],src[1]); out("Min_i16",dst);
		dst = M_VMax_i8(src[0],src[1]); out("Max_i8",dst);
		dst = M_VMax_i16(src[0],src[1]); out("Max_i16",dst);

		dst = M_VCmpEqMsk_u8(src[0],src[1]); out("CmpEqMsk_u8",dst);
		dst = M_VCmpGTMsk_u8(src[0],src[1]); out("CmpGTMsk_u8",dst);
		dst = M_VCmpLTMsk_u8(src[0],src[1]); out("CmpLTMsk_u8",dst);
		dst = M_VCmpEqMsk_i8(src[0],src[1]); out("CmpEqMsk_i8",dst);
		dst = M_VCmpGTMsk_i8(src[0],src[1]); out("CmpGTMsk_i8",dst);
		dst = M_VCmpLTMsk_i8(src[0],src[1]); out("CmpLTMsk_i8",dst);
		dst = M_VCmpEqMsk_u16(src[0],src[1]); out("CmpEqMsk_u16",dst);
		dst = M_VCmpGTMsk_u16(src[0],src[1]); out("CmpGTMsk_u16",dst);
		dst = M_VCmpLTMsk_u16(src[0],src[1]); out("CmpLTMsk_u16",dst);
		dst = M_VCmpEqMsk_i16(src[0],src[1]); out("CmpEqMsk_i16",dst);
		dst = M_VCmpGTMsk_i16(src[0],src[1]); out("CmpGTMsk_i16",dst);
		dst = M_VCmpLTMsk_i16(src[0],src[1]); out("CmpLTMsk_i16",dst);
		dst = M_VCmpEqMsk_u32(src[0],src[1]); out("CmpEqMsk_u32",dst);
		dst = M_VCmpGTMsk_u32(src[0],src[1]); out("CmpGTMsk_u32",dst);
		dst = M_VCmpLTMsk_u32(src[0],src[1]); out("CmpLTMsk_u32",dst);
		dst = M_VCmpEqMsk_i32(src[0],src[1]); out("CmpEqMsk_i32",dst);
		dst = M_VCmpGTMsk_i32(src[0],src[1]); out("CmpGTMsk_i32",dst);
		dst = M_VCmpLTMsk_i32(src[0],src[1]); out("CmpLTMsk_i32",dst);

		intdst = M_VCmpAllEq_u8(src[0],src[1]); out("CmpAllEq_u8",intdst);
		intdst = M_VCmpAllGT_u8(src[0],src[1]); out("CmpAllGT_u8",intdst);
		intdst = M_VCmpAllLT_u8(src[0],src[1]); out("CmpAllLT_u8",intdst);
		intdst = M_VCmpAllEq_i8(src[0],src[1]); out("CmpAllEq_i8",intdst);
		intdst = M_VCmpAllGT_i8(src[0],src[1]); out("CmpAllGT_i8",intdst);
		intdst = M_VCmpAllLT_i8(src[0],src[1]); out("CmpAllLT_i8",intdst);
		intdst = M_VCmpAllEq_u16(src[0],src[1]); out("CmpAllEq_u16",intdst);
		intdst = M_VCmpAllGT_u16(src[0],src[1]); out("CmpAllGT_u16",intdst);
		intdst = M_VCmpAllLT_u16(src[0],src[1]); out("CmpAllLT_u16",intdst);
		intdst = M_VCmpAllEq_i16(src[0],src[1]); out("CmpAllEq_i16",intdst);
		intdst = M_VCmpAllGT_i16(src[0],src[1]); out("CmpAllGT_i16",intdst);
		intdst = M_VCmpAllLT_i16(src[0],src[1]); out("CmpAllLT_i16",intdst);
		intdst = M_VCmpAllEq_u32(src[0],src[1]); out("CmpAllEq_u32",intdst);
		intdst = M_VCmpAllGT_u32(src[0],src[1]); out("CmpAllGT_u32",intdst);
		intdst = M_VCmpAllLT_u32(src[0],src[1]); out("CmpAllLT_u32",intdst);
		intdst = M_VCmpAllEq_i32(src[0],src[1]); out("CmpAllEq_i32",intdst);
		intdst = M_VCmpAllGT_i32(src[0],src[1]); out("CmpAllGT_i32",intdst);
		intdst = M_VCmpAllLT_i32(src[0],src[1]); out("CmpAllLT_i32",intdst);

		intdst = M_VCmpAnyEq_u8(src[0],src[1]); out("CmpAnyEq_u8",intdst);
		intdst = M_VCmpAnyGT_u8(src[0],src[1]); out("CmpAnyGT_u8",intdst);
		intdst = M_VCmpAnyLT_u8(src[0],src[1]); out("CmpAnyLT_u8",intdst);
		intdst = M_VCmpAnyEq_i8(src[0],src[1]); out("CmpAnyEq_i8",intdst);
		intdst = M_VCmpAnyGT_i8(src[0],src[1]); out("CmpAnyGT_i8",intdst);
		intdst = M_VCmpAnyLT_i8(src[0],src[1]); out("CmpAnyLT_i8",intdst);
		intdst = M_VCmpAnyEq_u16(src[0],src[1]); out("CmpAnyEq_u16",intdst);
		intdst = M_VCmpAnyGT_u16(src[0],src[1]); out("CmpAnyGT_u16",intdst);
		intdst = M_VCmpAnyLT_u16(src[0],src[1]); out("CmpAnyLT_u16",intdst);
		intdst = M_VCmpAnyEq_i16(src[0],src[1]); out("CmpAnyEq_i16",intdst);
		intdst = M_VCmpAnyGT_i16(src[0],src[1]); out("CmpAnyGT_i16",intdst);
		intdst = M_VCmpAnyLT_i16(src[0],src[1]); out("CmpAnyLT_i16",intdst);
		intdst = M_VCmpAnyEq_u32(src[0],src[1]); out("CmpAnyEq_u32",intdst);
		intdst = M_VCmpAnyGT_u32(src[0],src[1]); out("CmpAnyGT_u32",intdst);
		intdst = M_VCmpAnyLT_u32(src[0],src[1]); out("CmpAnyLT_u32",intdst);
		intdst = M_VCmpAnyEq_i32(src[0],src[1]); out("CmpAnyEq_i32",intdst);
		intdst = M_VCmpAnyGT_i32(src[0],src[1]); out("CmpAnyGT_i32",intdst);
		intdst = M_VCmpAnyLT_i32(src[0],src[1]); out("CmpAnyLT_i32",intdst);
	}

	for(i = 0;i < 1000;i++)
	{
		vec128 src[4];
		uint8 liVal[16] = { ((i/10)%10), ((i/10)%10 + 7)%10, ((i/10)%10 + 4)%10, ((i/10)%10 + 1)%10,
			(i%10), (i%10 + 3)%10, (i%10 + 6)%10, (i%10 + 9)%10,
			(i/100), (i/100 + 1)%10, (i/100 + 2)%10, (i/100 + 3)%10,
			(i%10), (i/100 + 7)%10, ((i/10)%10 + 3)%10, (i + 1)%10 };
		src[0] = M_VLd(lValues[liVal[0]],lValues[liVal[1]],lValues[liVal[2]],lValues[liVal[3]]);
		src[1] = M_VLd(lValues[liVal[4]],lValues[liVal[5]],lValues[liVal[6]],lValues[liVal[7]]);
		src[2] = M_VLd(lValues[liVal[8]],lValues[liVal[9]],lValues[liVal[10]],lValues[liVal[11]]);
		src[3] = M_VLd(lValues[liVal[12]],lValues[liVal[13]],lValues[liVal[14]],lValues[liVal[15]]);
		vec128 dst;//,dst2,dst3,dst4;

		gFile.Writeln("-------------------------------------------");
		gFile.Writeln(CStrF("- %s %s %s %s",lpValDesc[liVal[0]],lpValDesc[liVal[1]],lpValDesc[liVal[2]],lpValDesc[liVal[3]]));
		gFile.Writeln(CStrF("- %s %s %s %s",lpValDesc[liVal[4]],lpValDesc[liVal[5]],lpValDesc[liVal[6]],lpValDesc[liVal[7]]));
		gFile.Writeln(CStrF("- %s %s %s %s",lpValDesc[liVal[8]],lpValDesc[liVal[9]],lpValDesc[liVal[10]],lpValDesc[liVal[11]]));
		gFile.Writeln(CStrF("- %s %s %s %s",lpValDesc[liVal[4]],lpValDesc[liVal[12]],lpValDesc[liVal[13]],lpValDesc[liVal[14]]));

		dst = M_VSelMsk(src[0],src[1],src[2]); out("SelMsk",dst);
		dst = M_VSel(src[0],src[1],src[2]); out("Sel",dst);

		dst = M_VMAdd(src[0],src[1],src[2]); out("MAdd",dst);
		dst = M_VNMSub(src[0],src[1],src[2]); out("MSub",dst);

		dst = M_VLrp(src[0],src[1],src[2]); out("Lrp",dst);

		//Might need to test these matrix multipliers more thoroughly
		dst = M_VDp3x2(src[0],src[1],src[2],src[3]); out("VDp3x2",dst);
		// undefined? dst = M_VDp3x3(src[2],src[1],src[0],src[3],src[1],src[2]); out(dst);
		dst = M_VDp3x4(src[3],src[2],src[0],src[1],src[0],src[1],src[2],src[3]); out("VDp3x4",dst);
		dst = M_VDp4x2(src[0],src[1],src[2],src[3]); out("VDp4x2",dst);
		dst = M_VDp4x3(src[2],src[1],src[0],src[3],src[1],src[2]); out("VDp4x3",dst);
		dst = M_VDp4x4(src[3],src[2],src[0],src[1],src[0],src[1],src[2],src[3]); out("VDp4x4",dst);

		dst = M_VLen3x4(src[0],src[1],src[2],src[3]); out("Len3x4",dst);
		dst = M_VLen4x4(src[0],src[1],src[2],src[3]); out("Len4x4",dst);
		dst = M_VLenrcp3x4(src[0],src[1],src[2],src[3]); out("Lenrcp3x4",dst);
		dst = M_VLenrcp4x4(src[0],src[1],src[2],src[3]); out("Lenrcp4x4",dst);
		dst = M_VLenrcp3x4_Est(src[0],src[1],src[2],src[3]); out("Lenrcp3x4_Est",dst);
		dst = M_VLenrcp4x4_Est(src[0],src[1],src[2],src[3]); out("Lenrcp4x4_Est",dst);
		
		/* Only definitions, no need to test
		dst = src[0];dst2 = src[1]; dst3 = src[2];dst4 = src[3]; M_VNrm3x4(dst,dst2); out(dst); out(dst2); out(dst3); out(dst4);
		dst = src[0];dst2 = src[1]; dst3 = src[2];dst4 = src[3]; M_VNrm3x4_Est(dst,dst2); out(dst); out(dst2); out(dst3); out(dst4);
		dst = src[0];dst2 = src[1]; dst3 = src[2];dst4 = src[3]; M_VNrm4x4(dst,dst2); out(dst); out(dst2); out(dst3); out(dst4);
		dst = src[0];dst2 = src[1]; dst3 = src[2];dst4 = src[3]; M_VNrm4x4_Est(dst,dst2); out(dst); out(dst2); out(dst3); out(dst4);
		//*/

		// dst = src[0];dst2 = src[1]; dst3 = src[2];dst4 = src[3]; M_VTranspose4x4(dst,dst2); out(dst); out(dst2); out(dst3); out(dst4);

		//TODO - Matrix Mul
	}

	gFile.Close();
}