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
typedef const CMat4Dfp32& CMat4Dfp32p;

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
		__m128d d;
	};
};

M_FORCEINLINE __m128 M128Convf(__m128i a)
{
	CM128Conv r;
	r.i = a;
	return r.f;
}

M_FORCEINLINE __m128 M128Convf(__m128d a)
{
	CM128Conv r;
	r.d = a;
	return r.f;
}

M_FORCEINLINE __m128i M128Convi(__m128 a)
{
	CM128Conv r;
	r.f = a;
	return r.i;
}

M_FORCEINLINE __m128d M128Convd(__m128 a)
{
	CM128Conv r;
	r.f = a;
	return r.d;
}


//----------------------------------------------------------
// M_V128IMP_LOAD
M_FORCEINLINE vec128 M_VZero() { return _mm_setzero_ps(); }
M_FORCEINLINE vec128 M_VHalf() { return _mm_set_ps1(0.5f); }
M_FORCEINLINE vec128 M_VOne() { return _mm_set_ps1(1.0f); }
M_FORCEINLINE vec128 M_VTwo() { return _mm_set_ps1(2.0f); }
M_FORCEINLINE vec128 M_VOne_u8() { return (vec128&)_mm_set1_epi8(0x01); }
M_FORCEINLINE vec128 M_VOne_u16() { return (vec128&)_mm_set1_epi16(0x0001); }
M_FORCEINLINE vec128 M_VOne_u32() { return (vec128&)_mm_set1_epi32(0x00000001); }
M_FORCEINLINE vec128 M_VNegOne_i32() {return (vec128&)_mm_set1_epi8(-1); }

//#define M_VConst(x, y, z, w) _mm_load_ps(TVec128Const<M_VFLOATTOFIXED128(x), M_VFLOATTOFIXED128(y), M_VFLOATTOFIXED128(z), M_VFLOATTOFIXED128(w)>::ms_Const)
//#define M_VConstMsk(x, y, z, w) _mm_load_ps((const fp32*)&TVec128Mask<((x)?1:0) + ((y)?2:0) + ((z)?4:0) + ((w)?8:0)>::ms_Mask)
M_FORCEINLINE vec128 M_VLd(const void* _p) { return _mm_load_ps((const fp32*)_p); }
M_FORCEINLINE vec128 M_VLdU(const void* _p) { return _mm_loadu_ps((const fp32*)_p); }
//M_FORCEINLINE vec128 M_VLdMem64(const void* _p) { vec128 a = M128Convf(_mm_load_sd((const fp64*)_p)); return _mm_shuffle_ps(a, a, M_VSHUF(0,1,0,1));  }
M_FORCEINLINE vec128 M_VLd32(const void* _p) { return _mm_load1_ps((float*) _p); }

M_FORCEINLINE vec128 M_VLd64(const void* _p) { return M128Convf(_mm_load1_pd((double*) _p));  }
M_FORCEINLINE vec128 M_VLdScalar(fp32 x) { return _mm_set1_ps(x); }
M_FORCEINLINE vec128 M_VLdScalar_i8(int8 x) { return M128Convf(_mm_set1_epi8(x)); }
M_FORCEINLINE vec128 M_VLdScalar_u8(uint8 x) { return M128Convf(_mm_set1_epi8(x)); }
M_FORCEINLINE vec128 M_VLdScalar_i16(int16 x) { return M128Convf(_mm_set1_epi16(x)); }
M_FORCEINLINE vec128 M_VLdScalar_u16(uint16 x) { return M128Convf(_mm_set1_epi16(x)); }
M_FORCEINLINE vec128 M_VLdScalar_i32(int32 x) { return M128Convf(_mm_set1_epi32(x)); }
M_FORCEINLINE vec128 M_VLdScalar_u32(uint32 x) { return M128Convf(_mm_set1_epi32(x)); }
//M_FORCEINLINE vec128 M_VLd(fp32 x) { return _mm_set_ps1(x); }
M_FORCEINLINE vec128 M_VLd(fp32 x, fp32 y, fp32 z, fp32 w) { return _mm_set_ps(w, z, y, x); }

M_FORCEINLINE void M_VLdU_V3x2_Slow(const void* _pSrc, vec128& _a, vec128& _b)
{
	vec128 selmask = M_VConst_u32(0xffffffff, 0xffffffff, 0xffffffff, 0x00000000);
	vec128 zero = M_VZero();

	vec128 a = _mm_loadu_ps((const fp32*)_pSrc);
	vec128 b = _mm_loadu_ps(((const fp32*)_pSrc) + 3);

	_a = _mm_and_ps(selmask, a);
	_b = _mm_and_ps(selmask, b);
}

M_FORCEINLINE void M_VLdU_P3x2_Slow(const void* _pSrc, vec128& _a, vec128& _b)
{
	vec128 selmask = M_VConst_u32(0xffffffff, 0xffffffff, 0xffffffff, 0x00000000);
	vec128 one = M_VOne();

	vec128 a = _mm_loadu_ps((const fp32*)_pSrc);
	vec128 b = _mm_loadu_ps(((const fp32*)_pSrc) + 3);

	_a = _mm_or_ps(_mm_and_ps(selmask, a), _mm_andnot_ps(selmask, one));
	_b = _mm_or_ps(_mm_and_ps(selmask, b), _mm_andnot_ps(selmask, one));
}

M_FORCEINLINE void M_VSt(vec128 a, void* _p) { _mm_store_ps((fp32*)_p, a); }
M_FORCEINLINE void M_VStAny32(vec128 a, void* _p) { _mm_store_ss((float*)_p, a); }
M_FORCEINLINE void M_VStAny32Ex(vec128 a, void* _p, mint _Ofs) { _mm_store_ss((float*)((mint)_p + _Ofs), a); }

M_FORCEINLINE void M_VStL64(vec128 a, void* _p) { _mm_storel_pd((double*)_p, M128Convd(a)); }
M_FORCEINLINE void M_VStH64(vec128 a, void* _p) { _mm_storeh_pd((double*)_p, M128Convd(a)); }
M_FORCEINLINE void M_VStAny64(vec128 a, void* _p) { _mm_storel_pd((double*)_p, M128Convd(a)); }

M_FORCEINLINE void M_VStU_V3x2_Slow(vec128 _a, vec128 _b, void* _pDest)
{
	vec128 selmask = M_VConst_u32(0xffffffff, 0xffffffff, 0xffffffff, 0x00000000);
	vec128 a = _mm_or_ps(_mm_and_ps(selmask, _a), _mm_andnot_ps(selmask, _mm_shuffle_ps(_b, _b, M_VSHUF(0, 0, 0, 0))));

	fp32* pD = (fp32*)_pDest;
	_mm_storeu_ps(pD, a);
	_mm_store_ss(pD + 4, _mm_shuffle_ps(_b, _b, M_VSHUF(1, 1, 1, 1)));
	_mm_store_ss(pD + 5, _mm_shuffle_ps(_b, _b, M_VSHUF(2, 2, 2, 2)));
}

//----------------------------------------------------------
// M_V128IMP_SHUFFLE
#define M_VShuf(a, _Mask) _mm_shuffle_ps(a, a, _Mask)
#define M_VSplat(a, _Comp) M_VShuf(a, M_VSHUF(_Comp, _Comp, _Comp, _Comp))
M_FORCEINLINE vec128 M_VMrgL(vec128 a, vec128 b) { return _mm_unpacklo_ps(a, b); }
M_FORCEINLINE vec128 M_VMrgH(vec128 a, vec128 b) { return _mm_unpackhi_ps(a, b); }

M_FORCEINLINE vec128 M_VMrgL_u16(vec128 a, vec128 b) { return M128Convf(_mm_unpacklo_epi16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VMrgH_u16(vec128 a, vec128 b) { return M128Convf(_mm_unpackhi_epi16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VMrgL_u8(vec128 a, vec128 b) { return M128Convf(_mm_unpacklo_epi8(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VMrgH_u8(vec128 a, vec128 b) { return M128Convf(_mm_unpackhi_epi8(M128Convi(a), M128Convi(b))); }

M_FORCEINLINE vec128 M_VMrgL_u16_u32(vec128 a, vec128 b) { return M128Convf(_mm_unpacklo_epi16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VMrgH_u16_u32(vec128 a, vec128 b) { return M128Convf(_mm_unpackhi_epi16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VMrgL_u8_u16(vec128 a, vec128 b) { return M128Convf(_mm_unpacklo_epi8(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VMrgH_u8_u16(vec128 a, vec128 b) { return M128Convf(_mm_unpackhi_epi8(M128Convi(a), M128Convi(b))); }



vec128 M_VSplatX(vec128 a);
vec128 M_VSplatY(vec128 a);
vec128 M_VSplatZ(vec128 a);
vec128 M_VSplatW(vec128 a);

//----------------------------------------------------------
M_FORCEINLINE vec128 M_VCnv_f32_i32(vec128 a) { return M128Convf(_mm_cvttps_epi32(a)); }
M_FORCEINLINE vec128 M_VCnv_i32_f32(vec128 a) { return _mm_cvtepi32_ps(M128Convi(a)); }
M_FORCEINLINE vec128 M_VTrunc(vec128 a) { return M_VCnv_i32_f32(M_VCnv_f32_i32(a)); }

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
#define M_VSarImm_i16(a, ImmShift) M128Convf(_mm_srai_epi16(M128Convi(a), ImmShift))
#define M_VShrImm_u16(a, ImmShift) M128Convf(_mm_srli_epi16(M128Convi(a), ImmShift))
#define M_VShlImm_u16(a, ImmShift) M128Convf(_mm_slli_epi16(M128Convi(a), ImmShift))
#define M_VSarImm_i32(a, ImmShift) M128Convf(_mm_srai_epi32(M128Convi(a), ImmShift))
#define M_VShrImm_u32(a, ImmShift) M128Convf(_mm_srli_epi32(M128Convi(a), ImmShift))
#define M_VShlImm_u32(a, ImmShift) M128Convf(_mm_slli_epi32(M128Convi(a), ImmShift))

//M_FORCEINLINE vec128 M_VSar_i16(vec128 a, vec128 b) { return M128Convf(_mm_sra_epi16(M128Convi(a), M128Convi(b))); }
//M_FORCEINLINE vec128 M_VShr_u16(vec128 a, vec128 b) { return M128Convf(_mm_srl_epi16(M128Convi(a), M128Convi(b))); }
//M_FORCEINLINE vec128 M_VShl_u16(vec128 a, vec128 b) { return M128Convf(_mm_sll_epi16(M128Convi(a), M128Convi(b))); }
//M_FORCEINLINE vec128 M_VSar_i32(vec128 a, vec128 b) { return M128Convf(_mm_sra_epi32(M128Convi(a), M128Convi(b))); }
//M_FORCEINLINE vec128 M_VShr_u32(vec128 a, vec128 b) { return M128Convf(_mm_srl_epi32(M128Convi(a), M128Convi(b))); }
//M_FORCEINLINE vec128 M_VShl_u32(vec128 a, vec128 b) { return M128Convf(_mm_sll_epi32(M128Convi(a), M128Convi(b))); }

M_FORCEINLINE vec128 M_VShl_u16(vec128 a, vec128 b)
{
	vec128 mask = M_VConst_u16(~0, 0, 0, 0, 0, 0, 0, 0);
	vec128 t0 = M_VAnd((vec128&)_mm_sll_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 0), mask)), mask);
	vec128 t1 = M_VAnd((vec128&)_mm_sll_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 2), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 2));
	vec128 t2 = M_VAnd((vec128&)_mm_sll_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 4), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 4));
	vec128 t3 = M_VAnd((vec128&)_mm_sll_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 6), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 6));
	vec128 t4 = M_VAnd((vec128&)_mm_sll_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 8), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 8));
	vec128 t5 = M_VAnd((vec128&)_mm_sll_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 10), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 10));
	vec128 t6 = M_VAnd((vec128&)_mm_sll_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 12), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 12));
	vec128 t7 = M_VAnd((vec128&)_mm_sll_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 14), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 14));

	return M_VOr(M_VOr(M_VOr(t0, t1), M_VOr(t2, t3)), M_VOr(M_VOr(t4, t5), M_VOr(t6, t7)));
}

M_FORCEINLINE vec128 M_VShr_u16(vec128 a, vec128 b)
{
	vec128 mask = M_VConst_u16(~0, 0, 0, 0, 0, 0, 0, 0);
	vec128 t0 = M_VAnd((vec128&)_mm_srl_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 0), mask)), mask);
	vec128 t1 = M_VAnd((vec128&)_mm_srl_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 2), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 2));
	vec128 t2 = M_VAnd((vec128&)_mm_srl_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 4), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 4));
	vec128 t3 = M_VAnd((vec128&)_mm_srl_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 6), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 6));
	vec128 t4 = M_VAnd((vec128&)_mm_srl_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 8), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 8));
	vec128 t5 = M_VAnd((vec128&)_mm_srl_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 10), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 10));
	vec128 t6 = M_VAnd((vec128&)_mm_srl_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 12), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 12));
	vec128 t7 = M_VAnd((vec128&)_mm_srl_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 14), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 14));

	return M_VOr(M_VOr(M_VOr(t0, t1), M_VOr(t2, t3)), M_VOr(M_VOr(t4, t5), M_VOr(t6, t7)));
}

M_FORCEINLINE vec128 M_VSar_i16(vec128 a, vec128 b)
{
	vec128 mask = M_VConst_u16(~0, 0, 0, 0, 0, 0, 0, 0);
	vec128 t0 = M_VAnd((vec128&)_mm_sra_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 0), mask)), mask);
	vec128 t1 = M_VAnd((vec128&)_mm_sra_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 2), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 2));
	vec128 t2 = M_VAnd((vec128&)_mm_sra_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 4), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 4));
	vec128 t3 = M_VAnd((vec128&)_mm_sra_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 6), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 6));
	vec128 t4 = M_VAnd((vec128&)_mm_sra_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 8), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 8));
	vec128 t5 = M_VAnd((vec128&)_mm_sra_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 10), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 10));
	vec128 t6 = M_VAnd((vec128&)_mm_sra_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 12), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 12));
	vec128 t7 = M_VAnd((vec128&)_mm_sra_epi16((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 14), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 14));

	return M_VOr(M_VOr(M_VOr(t0, t1), M_VOr(t2, t3)), M_VOr(M_VOr(t4, t5), M_VOr(t6, t7)));
}

M_FORCEINLINE vec128 M_VShl_u32(vec128 a, vec128 b)
{
	vec128 mask = M_VConst_u32(~0, 0, 0, 0);
	vec128 t0 = M_VAnd((vec128&)_mm_sll_epi32((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 0), mask)), mask);
	vec128 t1 = M_VAnd((vec128&)_mm_sll_epi32((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 4), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 4));
	vec128 t2 = M_VAnd((vec128&)_mm_sll_epi32((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 8), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 8));
	vec128 t3 = M_VAnd((vec128&)_mm_sll_epi32((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 12), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 12));

	return M_VOr(M_VOr(t0, t1), M_VOr(t2, t3));
}

M_FORCEINLINE vec128 M_VShr_u32(vec128 a, vec128 b)
{
	vec128 mask = M_VConst_u32(~0, 0, 0, 0);
	vec128 t0 = M_VAnd((vec128&)_mm_srl_epi32((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 0), mask)), mask);
	vec128 t1 = M_VAnd((vec128&)_mm_srl_epi32((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 4), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 4));
	vec128 t2 = M_VAnd((vec128&)_mm_srl_epi32((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 8), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 8));
	vec128 t3 = M_VAnd((vec128&)_mm_srl_epi32((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 12), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 12));

	return M_VOr(M_VOr(t0, t1), M_VOr(t2, t3));
}

M_FORCEINLINE vec128 M_VSar_i32(vec128 a, vec128 b)
{
	vec128 mask = M_VConst_u32(~0, 0, 0, 0);
	vec128 t0 = M_VAnd((vec128&)_mm_sra_epi32((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 0), mask)), mask);
	vec128 t1 = M_VAnd((vec128&)_mm_sra_epi32((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 4), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 4));
	vec128 t2 = M_VAnd((vec128&)_mm_sra_epi32((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 8), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 8));
	vec128 t3 = M_VAnd((vec128&)_mm_sra_epi32((__m128i&)a, (__m128i&)M_VAnd((vec128&)_mm_srli_si128((__m128i&)b, 12), mask)), (vec128&)_mm_slli_si128((__m128i&)mask, 12));

	return M_VOr(M_VOr(t0, t1), M_VOr(t2, t3));
}


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
M_FORCEINLINE vec128 M_VMax_u32(vec128 a, vec128 b)
{
	return M_VSelMsk(M_VCmpGTMsk_u32(a, b), a, b);
}

M_FORCEINLINE vec128 M_VMin_u32(vec128 a, vec128 b)
{
	return M_VSelMsk(M_VCmpLTMsk_u32(a, b), a, b);
}

M_FORCEINLINE vec128 M_VMax_i32(vec128 a, vec128 b)
{
	return M_VSelMsk(M_VCmpGTMsk_i32(a, b), a, b);
}

M_FORCEINLINE vec128 M_VMin_i32(vec128 a, vec128 b)
{
	return M_VSelMsk(M_VCmpLTMsk_i32(a, b), a, b);
}

// -------------------------------------------------------------------
M_FORCEINLINE vec128 M_VCnvS_i32_i16(vec128 a, vec128 b) { return M128Convf(_mm_packs_epi32(M128Convi(a), M128Convi(b))); }

M_FORCEINLINE vec128 M_VCnvM_u32_u16(vec128 a, vec128 b)
{
	vec128 ac = M_VSub_u32(M_VAnd(a, M_VScalar_u32(0x0000ffff)), M_VScalar_u32(0x8000));
	vec128 bc = M_VSub_u32(M_VAnd(b, M_VScalar_u32(0x0000ffff)), M_VScalar_u32(0x8000));
	return M_VAdd_u16(M_VScalar_u16(0x8000), M_VCnvS_i32_i16(ac, bc));
}
M_FORCEINLINE vec128 M_VCnvS_u32_u16(vec128 a, vec128 b)
{ 
	vec128 ac = M_VSub_u32(M_VMin_u32(a, M_VScalar_u32(0x0000ffff)), M_VScalar_u32(0x8000));
	vec128 bc = M_VSub_u32(M_VMin_u32(b, M_VScalar_u32(0x0000ffff)), M_VScalar_u32(0x8000));
	return M_VAdd_u16(M_VScalar_u16(0x8000), M_VCnvS_i32_i16(ac, bc));
}
M_FORCEINLINE vec128 M_VCnvS_i32_u16(vec128 a, vec128 b)
{ 
	vec128 ac = M_VSub_u32(a, M_VScalar_u32(0x8000));
	vec128 bc = M_VSub_u32(b, M_VScalar_u32(0x8000));
	return M_VAdd_u16(M_VScalar_u16(0x8000), M_VCnvS_i32_i16(ac, bc));
}

M_FORCEINLINE vec128 M_VCnvM_u16_u8(vec128 a, vec128 b) { return M128Convf(_mm_packus_epi16(M128Convi(M_VAnd(M_VScalar_u16(0x00ff), a)), M128Convi(M_VAnd(M_VScalar_u16(0x00ff), b)))); }
M_FORCEINLINE vec128 M_VCnvS_u16_u8(vec128 a, vec128 b) { vec128 ac = M_VMin_u16(a, M_VScalar_u16(0x00ff)); vec128 bc = M_VMin_u16(b, M_VScalar_u16(0x00ff)); return M128Convf(_mm_packus_epi16(M128Convi(ac), M128Convi(bc))); }
M_FORCEINLINE vec128 M_VCnvS_i16_i8(vec128 a, vec128 b) { return M128Convf(_mm_packs_epi16(M128Convi(a), M128Convi(b))); }
M_FORCEINLINE vec128 M_VCnvS_i16_u8(vec128 a, vec128 b) { return M128Convf(_mm_packus_epi16(M128Convi(a), M128Convi(b))); }

M_FORCEINLINE vec128 M_VCnvL_u8_u16(vec128 _a) { return M128Convf(_mm_unpacklo_epi8(M128Convi(_a), M128Convi(M_VZero()))); }
M_FORCEINLINE vec128 M_VCnvH_u8_u16(vec128 _a) { return M128Convf(_mm_unpackhi_epi8(M128Convi(_a), M128Convi(M_VZero()))); }

M_FORCEINLINE vec128 M_VCnvL_i8_i16(vec128 _a)
{ 
	return M128Convf(_mm_subs_epi16(_mm_unpacklo_epi8(_mm_add_epi8(M128Convi(_a), M128Convi(M_VScalar_u8(0x80))), M128Convi(M_VZero())), M128Convi(M_VScalar_u16(0x80)) ));
}

M_FORCEINLINE vec128 M_VCnvH_i8_i16(vec128 _a)
{ 
	return M128Convf(_mm_subs_epi16(_mm_unpackhi_epi8(_mm_add_epi8(M128Convi(_a), M128Convi(M_VScalar_u8(0x80))), M128Convi(M_VZero())), M128Convi(M_VScalar_u16(0x80)) )); 
}

M_FORCEINLINE vec128 M_VCnvL_u16_u32(vec128 _a) { return M128Convf(_mm_unpacklo_epi16(M128Convi(_a), M128Convi(M_VZero()))); }
M_FORCEINLINE vec128 M_VCnvH_u16_u32(vec128 _a) { return M128Convf(_mm_unpackhi_epi16(M128Convi(_a), M128Convi(M_VZero()))); }

M_FORCEINLINE vec128 M_VCnvL_i16_i32(vec128 _a)
{ 
	return M128Convf(_mm_sub_epi32(_mm_unpacklo_epi16(_mm_add_epi16(M128Convi(_a), M128Convi(M_VScalar_u16(0x8000))), M128Convi(M_VZero())), M128Convi(M_VScalar_u32(0x8000)) ));
}
M_FORCEINLINE vec128 M_VCnvH_i16_i32(vec128 _a) 
{ 
	return M128Convf(_mm_sub_epi32(_mm_unpackhi_epi16(_mm_add_epi16(M128Convi(_a), M128Convi(M_VScalar_u16(0x8000))), M128Convi(M_VZero())), M128Convi(M_VScalar_u32(0x8000)) ));
}

//----------------------------------------------------------
#define M_VSarImm_i8(a, ImmShift) M_VCnvS_i16_i8(M_VSarImm_i16(M_VCnvL_i8_i16(a), ImmShift), M_VSarImm_i16(M_VCnvH_i8_i16(a), ImmShift))
#define M_VShrImm_u8(a, ImmShift) M_VCnvS_i16_i8(M_VShrImm_u16(M_VCnvL_u8_u16(a), ImmShift), M_VShrImm_u16(M_VCnvL_u8_u16(a), ImmShift))
#define M_VShlImm_u8(a, ImmShift) M_VCnvM_u16_u8(M_VShlImm_u16(M_VCnvL_u8_u16(a), ImmShift), M_VShlImm_u16(M_VCnvL_u8_u16(a), ImmShift))

//----------------------------------------------------------
M_FORCEINLINE vec128 M_VSar_i8(vec128 a, vec128 b)
{
	return M_VCnvS_i16_i8(M_VSar_i16(M_VCnvL_i8_i16(a), M_VCnvL_u8_u16(b)), M_VSar_i16(M_VCnvH_i8_i16(a), M_VCnvH_u8_u16(b)));
}
M_FORCEINLINE vec128 M_VShr_u8(vec128 a, vec128 b)
{
	return M_VCnvS_u16_u8(M_VShr_u16(M_VCnvL_u8_u16(a), M_VCnvL_u8_u16(b)), M_VShr_u16(M_VCnvH_u8_u16(a), M_VCnvH_u8_u16(b)));
}
M_FORCEINLINE vec128 M_VShl_u8(vec128 a, vec128 b)
{
	return M_VCnvS_u16_u8(M_VShl_u16(M_VCnvL_u8_u16(a), M_VCnvL_u8_u16(b)), M_VShl_u16(M_VCnvH_u8_u16(a), M_VCnvH_u8_u16(b)));
}


template <uint32 _nBytes>
M_FORCEINLINE vec128 M_VShl8_u128_Helper(vec128 _a) 
{ 
	return M128Convf(_mm_slli_si128(M128Convi(_a), _nBytes));
}

#define M_VShl8_u128(_a, _nBytes) M_VShl8_u128_Helper<_nBytes>(_a)

template <int t_0, int t_1, int t_2, int t_3>
M_FORCEINLINE vec128 M_VPerm_Helper(vec128 _a, vec128 _b) 
{ 
	vec128 a = M_VShuf(_a, M_VSHUF(t_0 & 3, t_1 & 3, t_2 & 3, t_3 & 3));
	vec128 b = M_VShuf(_b, M_VSHUF(t_0 & 3, t_1 & 3, t_2 & 3, t_3 & 3));
	return M_VSelMsk(M_VConstMsk(t_0 < 4,t_1 < 4,t_2 < 4,t_3 < 4),a,b);
}

#define M_VPerm(_a, _b, _0, _1, _2, _3) M_VPerm_Helper<_0, _1, _2, _3>(_a, _b)

//----------------------------------------------------------

#endif // defined(CPU_SSE2) && !defined(CPU_VEC128EMU)

#endif // __INCLUDE_XCC_MATH_VEC128_SSE3
