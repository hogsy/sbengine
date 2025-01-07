#ifndef __INCLUDE_XCC_MATH_VEC128_SSE3
#define __INCLUDE_XCC_MATH_VEC128_SSE3

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Vec128 SSE2 implementation

	Author:			Magnus Högdahl

	Copyright:		Magnus Högdahl, 2006

	History:
		2006-02-05:		Created file.





\*____________________________________________________________________________*/



#if defined(CPU_SSE2) && !defined(CPU_VEC128EMU)

typedef const vec128& vec128p;
typedef const CMat4Dfp4& CMat4Dfp4p;

//----------------------------------------------------------
// Performance hints for compound arithmetics implementations
#define M_V128HINT_NATIVE_MADD
//#define M_V128HINT_NATIVE_DP3
//#define M_V128HINT_NATIVE_DP4

//----------------------------------------------------------
// Flag everything we implement
#define M_V128IMP_LOAD					// Required unless fully emulated
#define M_V128IMP_SHUFFLE				// Required unless fully emulated
#define M_V128IMP_MERGE					// Required unless fully emulated
//#define M_V128IMP_SPLATXYZW
#define M_V128IMP_FLOAT_ARITHMETICS		// Required unless fully emulated
#define M_V128IMP_FLOAT_RCP_SQRT_EST
//#define M_V128IMP_FLOAT_RCP_SQRT
#define M_V128IMP_FLOAT_CMPMASK
#define M_V128IMP_LOGICOP
#define M_V128IMP_COMPARE
//#define M_V128IMP_FLOAT_DOTPROD
//#define M_V128IMP_FLOAT_XPROD
#define M_V128IMP_TRANSPOSE4X4
#define M_V128IMP_INTEGERARITHMETICS
#define M_V128IMP_INTEGERCOMPARE

//----------------------------------------------------------
// Some workaround for blatant stupidity on the part of whomever designed <emmintrin.h>
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


//----------------------------------------------------------
// M_V128IMP_LOAD
//#define M_VConst(x, y, z, w) _mm_load_ps(TVec128Const<M_VFLOATTOFIXED128(x), M_VFLOATTOFIXED128(y), M_VFLOATTOFIXED128(z), M_VFLOATTOFIXED128(w)>::ms_Const)
//#define M_VConstMsk(x, y, z, w) _mm_load_ps((const fp4*)&TVec128Mask<((x)?1:0) + ((y)?2:0) + ((z)?4:0) + ((w)?8:0)>::ms_Mask)
M_FORCEINLINE vec128 M_VLdMem(const void* _p) { return _mm_load_ps((const fp4*)_p); }
M_FORCEINLINE vec128 M_VLdScalar(fp4 x) { return _mm_set1_ps(x); }
M_FORCEINLINE vec128 M_VLdScalar_i8(int8 x) { return M128Convf(_mm_set1_epi8(x)); }
M_FORCEINLINE vec128 M_VLdScalar_u8(uint8 x) { return M128Convf(_mm_set1_epi8(x)); }
M_FORCEINLINE vec128 M_VLdScalar_i16(int16 x) { return M128Convf(_mm_set1_epi16(x)); }
M_FORCEINLINE vec128 M_VLdScalar_u16(uint16 x) { return M128Convf(_mm_set1_epi16(x)); }
M_FORCEINLINE vec128 M_VLdScalar_i32(int32 x) { return M128Convf(_mm_set1_epi32(x)); }
M_FORCEINLINE vec128 M_VLdScalar_u32(uint32 x) { return M128Convf(_mm_set1_epi32(x)); }
M_FORCEINLINE vec128 M_VLd(fp4 x, fp4 y, fp4 z, fp4 w) { return _mm_set_ps(w, z, y, x); }

M_FORCEINLINE vec128 M_VZero() { return _mm_setzero_ps(); }
M_FORCEINLINE vec128 M_VHalf() { return _mm_set_ps1(0.5f); }
M_FORCEINLINE vec128 M_VOne() { return _mm_set_ps1(1.0f); }
M_FORCEINLINE vec128 M_VTwo() { return _mm_set_ps1(2.0f); }
M_FORCEINLINE vec128 M_VPi() { return _mm_set_ps1(3.14159265358979323f); }
M_FORCEINLINE vec128 M_V2Pi() { return _mm_set_ps1(2.0f*3.14159265358979323f); }

M_FORCEINLINE void M_VSt(vec128 a, void* _p) { return _mm_store_ps((fp4*)_p, a); }

//----------------------------------------------------------
// M_V128IMP_SHUFFLE
#define M_VShuf(a, _Mask) _mm_shuffle_ps(a, a, _Mask)
#define M_VSplat(a, _Comp) M_VShuf(a, M_VSHUF(_Comp, _Comp, _Comp, _Comp))
M_FORCEINLINE vec128 M_VMrgXY(vec128 a, vec128 b) { return _mm_unpacklo_ps(a, b); }
M_FORCEINLINE vec128 M_VMrgZW(vec128 a, vec128 b) { return _mm_unpackhi_ps(a, b); }
vec128 M_VSplatX(vec128 a);
vec128 M_VSplatY(vec128 a);
vec128 M_VSplatZ(vec128 a);
vec128 M_VSplatW(vec128 a);

//----------------------------------------------------------
M_FORCEINLINE vec128 M_Vfp4toi32(vec128 a) { return M128Convf(_mm_cvttps_epi32(a)); }
M_FORCEINLINE vec128 M_Vi32tofp4(vec128 a) { return _mm_cvtepi32_ps(M128Convi(a)); }
M_FORCEINLINE vec128 M_VTrunc(vec128 a) { return M_Vi32tofp4(M_Vfp4toi32(a)); }

//----------------------------------------------------------
// M_V128IMP_LOGICOP
M_FORCEINLINE vec128 M_VOr(vec128 a, vec128 b) { return _mm_or_ps(a, b); }
M_FORCEINLINE vec128 M_VAnd(vec128 a, vec128 b) { return _mm_and_ps(a, b); }
M_FORCEINLINE vec128 M_VAndNot(vec128 a, vec128 b) { return _mm_andnot_ps(b, a); }
M_FORCEINLINE vec128 M_VXor(vec128 a, vec128 b) { return _mm_xor_ps(a, b); }
M_FORCEINLINE vec128 M_VNot(vec128 a) { return _mm_xor_ps(a, M128Convf(_mm_set1_epi32(char(0xffffffff)))); }

//----------------------------------------------------------
M_FORCEINLINE vec128 M_VCmpEqMsk(vec128 a, vec128 b) { return _mm_cmpeq_ps(a, b); }
M_FORCEINLINE vec128 M_VCmpGEMsk(vec128 a, vec128 b) { return _mm_cmpge_ps(a, b); }
M_FORCEINLINE vec128 M_VCmpGTMsk(vec128 a, vec128 b) { return _mm_cmpgt_ps(a, b); }
M_FORCEINLINE vec128 M_VCmpLEMsk(vec128 a, vec128 b) { return _mm_cmple_ps(a, b); }
M_FORCEINLINE vec128 M_VCmpLTMsk(vec128 a, vec128 b) { return _mm_cmplt_ps(a, b); }

//----------------------------------------------------------
M_FORCEINLINE uint M_VCmpAllEq(vec128 a, vec128 b)
{
	vec128 msk = _mm_cmpeq_ps(a, b);
	return _mm_movemask_ps(msk) == 0x0f;
}
M_FORCEINLINE uint M_VCmpAllGE(vec128 a, vec128 b)
{
	vec128 msk = _mm_cmpge_ps(a, b);
	return _mm_movemask_ps(msk) == 0x0f;
}
M_FORCEINLINE uint M_VCmpAllGT(vec128 a, vec128 b)
{
	vec128 msk = _mm_cmpgt_ps(a, b);
	return _mm_movemask_ps(msk) == 0x0f;
}
M_FORCEINLINE uint M_VCmpAllLE(vec128 a, vec128 b)
{
	vec128 msk = _mm_cmple_ps(a, b);
	return _mm_movemask_ps(msk) == 0x0f;
}
M_FORCEINLINE uint M_VCmpAllLT(vec128 a, vec128 b)
{
	vec128 msk = _mm_cmplt_ps(a, b);
	return _mm_movemask_ps(msk) == 0x0f;
}

//----------------------------------------------------------
M_FORCEINLINE uint M_VCmpAnyEq(vec128 a, vec128 b)
{
	vec128 msk = _mm_cmpeq_ps(a, b);
	return _mm_movemask_ps(msk) != 0;
}
M_FORCEINLINE uint M_VCmpAnyGE(vec128 a, vec128 b)
{
	vec128 msk = _mm_cmpge_ps(a, b);
	return _mm_movemask_ps(msk) != 0;
}
M_FORCEINLINE uint M_VCmpAnyGT(vec128 a, vec128 b)
{
	vec128 msk = _mm_cmpgt_ps(a, b);
	return _mm_movemask_ps(msk) != 0;
}
M_FORCEINLINE uint M_VCmpAnyLE(vec128 a, vec128 b)
{
	vec128 msk = _mm_cmple_ps(a, b);
	return _mm_movemask_ps(msk) != 0;
}
M_FORCEINLINE uint M_VCmpAnyLT(vec128 a, vec128 b)
{
	vec128 msk = _mm_cmplt_ps(a, b);
	return _mm_movemask_ps(msk) != 0;
}

//----------------------------------------------------------
// M_V128IMP_FLOAT_ARITHMETICS
M_FORCEINLINE vec128 M_VAdd(vec128 a, vec128 b) { return _mm_add_ps(a, b); }
M_FORCEINLINE vec128 M_VAddh(vec128 a)
{
	return M_VAdd(M_VAdd(M_VSplatX(a), M_VSplatY(a)), M_VAdd(M_VSplatZ(a), M_VSplatW(a)));
}
//M_FORCEINLINE vec128 M_VAddh(vec128 a) { vec128 sum = _mm_hadd_ps(a, a); return _mm_hadd_ps(sum, sum); }		// SSE3 version

M_FORCEINLINE vec128 M_VSub(vec128 a, vec128 b) { return _mm_sub_ps(a, b); }
M_FORCEINLINE vec128 M_VMul(vec128 a, vec128 b) { return _mm_mul_ps(a, b); }
M_FORCEINLINE vec128 M_VMAdd(vec128 a, vec128 b, vec128 c) { return M_VAdd(M_VMul(a, b), c); }
M_FORCEINLINE vec128 M_VNMSub(vec128 a, vec128 b, vec128 c) { return M_VSub(c, M_VMul(a, b)); }
M_FORCEINLINE vec128 M_VMin(vec128 a, vec128 b) { return _mm_min_ps(a, b); }
M_FORCEINLINE vec128 M_VMax(vec128 a, vec128 b) { return _mm_max_ps(a, b); }
M_FORCEINLINE vec128 M_VNeg(vec128 a) { return _mm_sub_ps(M_VZero(), a); }
M_FORCEINLINE vec128 M_VSelMsk(vec128 _Mask, vec128 a, vec128 b) { return _mm_or_ps(_mm_and_ps(_Mask, a), _mm_andnot_ps(_Mask, b)); }

M_FORCEINLINE vec128 M_VRcp_Est(vec128 a) { return _mm_rcp_ps(a); }
M_FORCEINLINE vec128 M_VRsq_Est(vec128 a) { return _mm_rsqrt_ps(a); }
M_FORCEINLINE vec128 M_VSqrt_Est(vec128 a) { return _mm_sqrt_ps(a); }

//----------------------------------------------------------
M_FORCEINLINE vec128 M_VAdds_i8(vec128 a, vec128 b) { return M128Convf(_mm_adds_epi8(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VAdds_i16(vec128 a, vec128 b) { return M128Convf(_mm_adds_epi16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VSubs_i8(vec128 a, vec128 b)  { return M128Convf(_mm_subs_epi8(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VSubs_i16(vec128 a, vec128 b)  { return M128Convf(_mm_subs_epi16(M128Convi(a), M128Convi(b))); }

M_FORCEINLINE vec128 M_VMin_i8(vec128 a, vec128 b)
{ 
	vec128 ofs = M_VScalar_u8(0x80);
	__m128i ofsi = M128Convi(ofs);
	return M128Convf(_mm_sub_epi8(_mm_min_epu8(_mm_add_epi8(ofsi, M128Convi(a)), _mm_add_epi8(ofsi, M128Convi(b))), ofsi));
}
M_FORCEINLINE vec128 M_VMin_i16(vec128 a, vec128 b)  { return M128Convf(_mm_min_epi16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VMax_i8(vec128 a, vec128 b)
{ 
	vec128 ofs = M_VScalar_u8(0x80);
	__m128i ofsi = M128Convi(ofs);
	return M128Convf(_mm_sub_epi8(_mm_max_epu8(_mm_add_epi8(ofsi, M128Convi(a)), _mm_add_epi8(ofsi, M128Convi(b))), ofsi));
}
M_FORCEINLINE vec128 M_VMax_i16(vec128 a, vec128 b)  { return M128Convf(_mm_max_epi16(M128Convi(a), M128Convi(b))); }

M_FORCEINLINE vec128 M_VAdd_u8(vec128 a, vec128 b) { return M128Convf(_mm_add_epi8(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VAdds_u8(vec128 a, vec128 b) { return M128Convf(_mm_adds_epu8(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VAdd_u16(vec128 a, vec128 b) { return M128Convf(_mm_add_epi16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VAdds_u16(vec128 a, vec128 b) { return M128Convf(_mm_adds_epu16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VAdd_u32(vec128 a, vec128 b) { return M128Convf(_mm_add_epi32(M128Convi(a), M128Convi(b))); }

M_FORCEINLINE vec128 M_VSub_u8(vec128 a, vec128 b) { return M128Convf(_mm_sub_epi8(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VSubs_u8(vec128 a, vec128 b) { return M128Convf(_mm_subs_epu8(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VSub_u16(vec128 a, vec128 b) { return M128Convf(_mm_sub_epi16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VSubs_u16(vec128 a, vec128 b) { return M128Convf(_mm_subs_epu16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VSub_u32(vec128 a, vec128 b) { return M128Convf(_mm_sub_epi32(M128Convi(a), M128Convi(b))); }

M_FORCEINLINE vec128 M_VMin_u8(vec128 a, vec128 b) { return M128Convf(_mm_min_epu8(M128Convi(a), M128Convi(b))); }

M_FORCEINLINE vec128 M_VMin_u16(vec128 a, vec128 b)
{ 
	vec128 ofs = M_VScalar_u16(0x8000);
	__m128i ofsi = M128Convi(ofs);
	return M128Convf(_mm_sub_epi16(_mm_min_epi16(_mm_add_epi16(ofsi, M128Convi(a)), _mm_add_epi16(ofsi, M128Convi(b))), ofsi));
}

M_FORCEINLINE vec128 M_VMax_u8(vec128 a, vec128 b) { return M128Convf(_mm_max_epu8(M128Convi(a), M128Convi(b))); }

M_FORCEINLINE vec128 M_VMax_u16(vec128 a, vec128 b)
{ 
	vec128 ofs = M_VScalar_u16(0x8000);
	__m128i ofsi = M128Convi(ofs);
	return M128Convf(_mm_sub_epi16(_mm_max_epi16(_mm_add_epi16(ofsi, M128Convi(a)), _mm_add_epi16(ofsi, M128Convi(b))), ofsi));
}

// -------------------------------------------------------------------
M_FORCEINLINE vec128 M_VSar_i16(vec128 a, uint b) { return M128Convf(_mm_srai_epi16(M128Convi(a), b)); }
M_FORCEINLINE vec128 M_VShr_u16(vec128 a, uint b) { return M128Convf(_mm_srli_epi16(M128Convi(a), b)); }
M_FORCEINLINE vec128 M_VShl_u16(vec128 a, uint b) { return M128Convf(_mm_slli_epi16(M128Convi(a), b)); }
M_FORCEINLINE vec128 M_VSar_i32(vec128 a, uint b) { return M128Convf(_mm_srai_epi32(M128Convi(a), b)); }
M_FORCEINLINE vec128 M_VShr_u32(vec128 a, uint b) { return M128Convf(_mm_srli_epi32(M128Convi(a), b)); }
M_FORCEINLINE vec128 M_VShl_u32(vec128 a, uint b) { return M128Convf(_mm_slli_epi32(M128Convi(a), b)); }

//----------------------------------------------------------
M_FORCEINLINE vec128 M_VCmpEqMsk_u8(vec128 a, vec128 b) { return M128Convf(_mm_cmpeq_epi8(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VCmpEqMsk_i8(vec128 a, vec128 b) { return M128Convf(_mm_cmpeq_epi8(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VCmpGTMsk_i8(vec128 a, vec128 b) { return M128Convf(_mm_cmpgt_epi8(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VCmpLTMsk_i8(vec128 a, vec128 b) { return M128Convf(_mm_cmplt_epi8(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VCmpEqMsk_u16(vec128 a, vec128 b) { return M128Convf(_mm_cmpeq_epi16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VCmpEqMsk_i16(vec128 a, vec128 b) { return M128Convf(_mm_cmpeq_epi16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VCmpGTMsk_i16(vec128 a, vec128 b) { return M128Convf(_mm_cmpgt_epi16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VCmpLTMsk_i16(vec128 a, vec128 b) { return M128Convf(_mm_cmplt_epi16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VCmpEqMsk_u32(vec128 a, vec128 b) { return M128Convf(_mm_cmpeq_epi32(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VCmpEqMsk_i32(vec128 a, vec128 b) { return M128Convf(_mm_cmpeq_epi32(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VCmpGTMsk_i32(vec128 a, vec128 b) { return M128Convf(_mm_cmpgt_epi32(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VCmpLTMsk_i32(vec128 a, vec128 b) { return M128Convf(_mm_cmplt_epi32(M128Convi(a), M128Convi(b))); }

//----------------------------------------------------------
M_FORCEINLINE vec128 M_VCmpGTMsk_u8(vec128 a, vec128 b)
{
	vec128 ofs = M_VScalar_u8(0x80);
	__m128i ofsi = M128Convi(ofs);
	return M128Convf(_mm_cmpgt_epi8(_mm_add_epi8(ofsi, M128Convi(a)), _mm_add_epi8(ofsi, M128Convi(b))));
}
M_FORCEINLINE vec128 M_VCmpLTMsk_u8(vec128 a, vec128 b)
{
	vec128 ofs = M_VScalar_u8(0x80);
	__m128i ofsi = M128Convi(ofs);
	return M128Convf(_mm_cmplt_epi8(_mm_add_epi8(ofsi, M128Convi(a)), _mm_add_epi8(ofsi, M128Convi(b))));
}
M_FORCEINLINE vec128 M_VCmpGTMsk_u16(vec128 a, vec128 b)
{
	vec128 ofs = M_VScalar_u16(0x8000);
	__m128i ofsi = M128Convi(ofs);
	return M128Convf(_mm_cmpgt_epi16(_mm_add_epi16(ofsi, M128Convi(a)), _mm_add_epi16(ofsi, M128Convi(b))));
}
M_FORCEINLINE vec128 M_VCmpLTMsk_u16(vec128 a, vec128 b)
{
	vec128 ofs = M_VScalar_u16(0x8000);
	__m128i ofsi = M128Convi(ofs);
	return M128Convf(_mm_cmplt_epi16(_mm_add_epi16(ofsi, M128Convi(a)), _mm_add_epi16(ofsi, M128Convi(b))));
}
M_FORCEINLINE vec128 M_VCmpGTMsk_u32(vec128 a, vec128 b)
{
	vec128 ofs = M_VScalar_u32(0x80000000);
	__m128i ofsi = M128Convi(ofs);
	return M128Convf(_mm_cmpgt_epi16(_mm_add_epi32(ofsi, M128Convi(a)), _mm_add_epi32(ofsi, M128Convi(b))));
}
M_FORCEINLINE vec128 M_VCmpLTMsk_u32(vec128 a, vec128 b)
{
	vec128 ofs = M_VScalar_u32(0x80000000);
	__m128i ofsi = M128Convi(ofs);
	return M128Convf(_mm_cmplt_epi16(_mm_add_epi32(ofsi, M128Convi(a)), _mm_add_epi32(ofsi, M128Convi(b))));
}

//----------------------------------------------------------
M_FORCEINLINE uint M_VCmpAllEq_u8(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpEqMsk_u8(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllGT_u8(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpGTMsk_u8(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllLT_u8(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpLTMsk_u8(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllEq_i8(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpEqMsk_i8(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllGT_i8(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpGTMsk_i8(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllLT_i8(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpLTMsk_i8(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}

//----------------------------------------------------------
M_FORCEINLINE uint M_VCmpAllEq_u16(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpEqMsk_u16(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllGT_u16(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpGTMsk_u16(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllLT_u16(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpLTMsk_u16(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllEq_i16(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpEqMsk_i16(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllGT_i16(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpGTMsk_i16(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllLT_i16(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpLTMsk_i16(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}

//----------------------------------------------------------
M_FORCEINLINE uint M_VCmpAllEq_u32(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpEqMsk_u32(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllGT_u32(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpGTMsk_u32(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllLT_u32(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpLTMsk_u32(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllEq_i32(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpEqMsk_i32(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllGT_i32(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpGTMsk_i32(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}
M_FORCEINLINE uint M_VCmpAllLT_i32(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpLTMsk_i32(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) == 0xffff;
}

//----------------------------------------------------------
M_FORCEINLINE uint M_VCmpAnyEq_u8(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpEqMsk_u8(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyGT_u8(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpGTMsk_u8(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyLT_u8(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpLTMsk_u8(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyEq_i8(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpEqMsk_i8(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyGT_i8(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpGTMsk_i8(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyLT_i8(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpLTMsk_i8(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
//----------------------------------------------------------
M_FORCEINLINE uint M_VCmpAnyEq_u16(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpEqMsk_u16(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyGT_u16(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpGTMsk_u16(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyLT_u16(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpLTMsk_u16(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyEq_i16(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpEqMsk_i16(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyGT_i16(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpGTMsk_i16(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyLT_i16(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpLTMsk_i16(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}

//----------------------------------------------------------
M_FORCEINLINE uint M_VCmpAnyEq_u32(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpEqMsk_u32(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyGT_u32(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpGTMsk_u32(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyLT_u32(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpLTMsk_u32(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyEq_i32(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpEqMsk_i32(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyGT_i32(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpGTMsk_i32(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
M_FORCEINLINE uint M_VCmpAnyLT_i32(vec128 a, vec128 b)
{
	vec128 msk = M_VCmpLTMsk_i32(a, b);
	return _mm_movemask_epi8(M128Convi(msk)) != 0;
}
//----------------------------------------------------------

#endif // defined(CPU_SSE2) && !defined(CPU_VEC128EMU)

#endif // __INCLUDE_XCC_MATH_VEC128_SSE3
