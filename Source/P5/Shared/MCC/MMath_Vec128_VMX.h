#ifndef __INCLUDE_XCC_MATH_VEC128_VMX
#define __INCLUDE_XCC_MATH_VEC128_VMX

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			vec128 VMX & VMX128 implementation

	Author:			Magnus Högdahl

	Copyright:		Magnus Högdahl, 2006

	History:
		2006-02-06:		Created file.



\*____________________________________________________________________________________________*/

#if defined(CPU_POWERPC) && !defined(CPU_VEC128EMU)

#ifdef PLATFORM_XENON
#include <vectorintrinsics.h>

#else
#include <altivec.h>

typedef vector signed char __vec128i8;
typedef vector unsigned char __vec128u8;
typedef vector signed short __vec128i16;
typedef vector unsigned short __vec128u16;
typedef vector signed int __vec128i32;
typedef vector unsigned int __vec128u32;

// -------------------------------------------------------------------
M_FORCEINLINE vec128 builtin_altivec_lvlx(void* _p, size_t _Offset)
{
	vec128 ret;
	asm("lvlx %0, %1, %2" : "=v"(ret) : "r"(_p), "r"(_Offset));
	return ret;
}

// -------------------------------------------------------------------
#define __vspltw(_a, _iComp) vec_vspltw(_a, _iComp)
//#define __vpermwi(_a, _Mask) vec_vperm(_a, _a, (vec128)TVec128Perm<_Mask & 3, (_Mask>>2) & 3, (_Mask>>4) & 3, (_Mask>>6) & 3>::m_Perm)
#define __vpermwi(_a, _Mask) ((vec128)__builtin_altivec_vperm_4si((__vec128i32)_a, (__vec128i32)_a, *(__vec128i8*)&TVec128Perm<(_Mask) & 3, ((_Mask)>>2) & 3, ((_Mask)>>4) & 3, ((_Mask)>>6) & 3>::m_Perm))

#define __vmrghw(a, b) (vec128)__builtin_altivec_vmrghw((__vec128i32)(a), (__vec128i32)(b))
#define __vmrglw(a, b) (vec128)__builtin_altivec_vmrglw((__vec128i32)(a), (__vec128i32)(b))
#define __lvx(a, b) (vec128)__builtin_altivec_lvx(a, (vec128*)(b))
#define __lvlx(a, b) (vec128)builtin_altivec_lvlx((void*)(a), b)

#define __vrfin(a) (vec128)__builtin_altivec_vrfin(a)
#define __vrfiz(a) (vec128)__builtin_altivec_vrfiz(a)
#define __vcfsx(a, shift) ((vec128)__builtin_altivec_vcfsx((__vec128i32)a, shift))
#define __vctsxs(a, shift) ((vec128)__builtin_altivec_vctsxs(a, shift))
#define __stvx(_v, _pDst, _Offset) __builtin_altivec_stvx((__vec128i32)(_v), _Offset, _pDst)
#define __vspltisw(a) __builtin_altivec_vspltisw(a)

// -------------------------------------------------------------------
#define __vaddfp(a, b) __builtin_altivec_vaddfp(a, b)
#define __vsubfp(a, b) __builtin_altivec_vsubfp(a, b)
#define __vmulfp(a, b) __builtin_altivec_vmaddfp(a, b, M_VZero())
#define __vmaddfp(a, b, c) __builtin_altivec_vmaddfp(a, b, c)
#define __vnmsubfp(a, b, c) __builtin_altivec_vnmsubfp(a, b, c)

#define __vminfp(a, b) __builtin_altivec_vminfp(a, b)
#define __vmaxfp(a, b) __builtin_altivec_vmaxfp(a, b)
#define __vrefp(a) __builtin_altivec_vrefp(a)
#define __vrsqrtefp(a) __builtin_altivec_vrsqrtefp(a)

// -------------------------------------------------------------------
#define __vcmpeqfp(a, b) ((vec128)__builtin_altivec_vcmpeqfp(a, b))
#define __vcmpgefp(a, b) ((vec128)__builtin_altivec_vcmpgefp(a, b))
#define __vcmpgtfp(a, b) ((vec128)__builtin_altivec_vcmpgtfp(a, b))

#define __vsel(a,b,mask) ((vec128)__builtin_altivec_vsel_4sf (a, b, (__vec128i32)(mask)))

#define __vand(a, b) ((vec128)__builtin_altivec_vand((__vec128i32)(a), (__vec128i32)(b)))
#define __vandc(a, b) ((vec128)__builtin_altivec_vandc((__vec128i32)(a), (__vec128i32)(b)))
#define __vor(a, b) ((vec128)__builtin_altivec_vor((__vec128i32)(a), (__vec128i32)(b)))
#define __vnor(a, b) ((vec128)__builtin_altivec_vnor((__vec128i32)(a), (__vec128i32)(b)))
#define __vxor(a, b) ((vec128)__builtin_altivec_vxor((__vec128i32)(a), (__vec128i32)(b)))

// -------------------------------------------------------------------
#define __vaddsbs(a, b) ((vec128)__builtin_altivec_vaddsbs((__vec128i8)(a), (__vec128i8)(b)))
#define __vaddshs(a, b) ((vec128)__builtin_altivec_vaddshs((__vec128i16)(a), (__vec128i16)(b)))
#define __vaddsws(a, b) ((vec128)__builtin_altivec_vaddsws((__vec128i32)(a), (__vec128i32)(b)))

#define __vsubsbs(a, b) ((vec128)__builtin_altivec_vsubsbs((__vec128i8)(a), (__vec128i8)(b)))
#define __vsubshs(a, b) ((vec128)__builtin_altivec_vsubshs((__vec128i16)(a), (__vec128i16)(b)))
#define __vsubsws(a, b) ((vec128)__builtin_altivec_vsubsws((__vec128i32)(a), (__vec128i32)(b)))

#define __vminsb(a, b) ((vec128)__builtin_altivec_vminsb((__vec128i8)(a), (__vec128i8)(b)))
#define __vminsh(a, b) ((vec128)__builtin_altivec_vminsh((__vec128i16)(a), (__vec128i16)(b)))
#define __vminsw(a, b) ((vec128)__builtin_altivec_vminsw((__vec128i32)(a), (__vec128i32)(b)))

#define __vmaxsb(a, b) ((vec128)__builtin_altivec_vmaxsb((__vec128i8)(a), (__vec128i8)(b)))
#define __vmaxsh(a, b) ((vec128)__builtin_altivec_vmaxsh((__vec128i16)(a), (__vec128i16)(b)))
#define __vmaxsw(a, b) ((vec128)__builtin_altivec_vmaxsw((__vec128i32)(a), (__vec128i32)(b)))

// -------------------------------------------------------------------
// Unsigned Integer arithmetics
#define __vaddubm(a, b) ((vec128)__builtin_altivec_vaddubm((__vec128i8)(a), (__vec128i8)(b)))
#define __vaddubs(a, b) ((vec128)__builtin_altivec_vaddubs((__vec128i8)(a), (__vec128i8)(b)))
#define __vadduhm(a, b) ((vec128)__builtin_altivec_vadduhm((__vec128i16)(a), (__vec128i16)(b)))
#define __vadduhs(a, b) ((vec128)__builtin_altivec_vadduhs((__vec128i16)(a), (__vec128i16)(b)))
#define __vadduwm(a, b) ((vec128)__builtin_altivec_vadduwm((__vec128i32)(a), (__vec128i32)(b)))
#define __vadduws(a, b) ((vec128)__builtin_altivec_vadduws((__vec128i32)(a), (__vec128i32)(b)))

#define __vaddcuw(a, b) ((vec128)__builtin_altivec_vaddcuw((__vec128i32)(a), (__vec128i32)(b)))

// -------------------------------------------------------------------
#define __vsububm(a, b) ((vec128)__builtin_altivec_vsububm((__vec128i8)(a), (__vec128i8)(b)))
#define __vsububs(a, b) ((vec128)__builtin_altivec_vsububs((__vec128i8)(a), (__vec128i8)(b)))
#define __vsubuhm(a, b) ((vec128)__builtin_altivec_vsubuhm((__vec128i16)(a), (__vec128i16)(b)))
#define __vsubuhs(a, b) ((vec128)__builtin_altivec_vsubuhs((__vec128i16)(a), (__vec128i16)(b)))
#define __vsubuwm(a, b) ((vec128)__builtin_altivec_vsubuwm((__vec128i32)(a), (__vec128i32)(b)))
#define __vsubuws(a, b) ((vec128)__builtin_altivec_vsubuws((__vec128i32)(a), (__vec128i32)(b)))

#define __vsubcuw(a, b) ((vec128)__builtin_altivec_vsubcuw((__vec128i32)(a), (__vec128i32)(b)))

// -------------------------------------------------------------------
#define __vminub(a, b) ((vec128)__builtin_altivec_vminub((__vec128i8)(a), (__vec128i8)(b)))
#define __vminuh(a, b) ((vec128)__builtin_altivec_vminuh((__vec128i16)(a), (__vec128i16)(b)))
#define __vminuw(a, b) ((vec128)__builtin_altivec_vminuw((__vec128i32)(a), (__vec128i32)(b)))

#define __vmaxub(a, b) ((vec128)__builtin_altivec_vmaxub((__vec128i8)(a), (__vec128i8)(b)))
#define __vmaxuh(a, b) ((vec128)__builtin_altivec_vmaxuh((__vec128i16)(a), (__vec128i16)(b)))
#define __vmaxuw(a, b) ((vec128)__builtin_altivec_vmaxuw((__vec128i32)(a), (__vec128i32)(b)))



#endif

//typedef const vec128& vec128p;
//typedef const CMat4Dfp4& CMat4Dfp4p;

typedef vec128 vec128p;
typedef CMat4Dfp4 CMat4Dfp4p;

//----------------------------------------------------------
// Performance hints for compound arithmetics implementations
#define M_V128HINT_NATIVE_MADD


#ifdef PLATFORM_XENON
	#define M_V128HINT_NATIVE_DP3
	#define M_V128HINT_NATIVE_DP4
#endif

//----------------------------------------------------------
// Flag everything we implement
#define M_V128IMP_LOAD					// Required unless fully emulated
#define M_V128IMP_SHUFFLE				// Required unless fully emulated
#define M_V128IMP_VSPLATXYZW
#define M_V128IMP_MERGE					// Required unless fully emulated
//#define M_V128IMP_SPLATXYZW
#define M_V128IMP_FLOAT_ARITHMETICS		// Required unless fully emulated
#define M_V128IMP_FLOAT_RCP_SQRT_EST
//#define M_V128IMP_FLOAT_RCP_SQRT
#define M_V128IMP_FLOAT_CMPMASK
#define M_V128IMP_LOGICOP
#define M_V128IMP_COMPARE
#define M_V128IMP_INTEGERARITHMETICS
//#define M_V128IMP_FLOAT_COMPAREOPERATORS
#define M_V128IMP_FLOAT_OPERATORS

#ifdef PLATFORM_XENON
	#define M_V128IMP_FLOAT_DOTPROD
#endif

//#define M_V128IMP_FLOAT_XPROD
#define M_V128IMP_TRANSPOSE4X4
#define M_V128IMP_STR

//----------------------------------------------------------
template<int x, int y, int z, int w>
class TVec128Perm
{
public:
	static uint8 m_Perm[16];
};

template<int x, int y, int z, int w>
uint8 TVec128Perm<x, y, z, w>::m_Perm[16] = 
{ 
	(x<<2),(x<<2)+1,(x<<2)+2,(x<<2)+3, 
		(y<<2),(y<<2)+1,(y<<2)+2,(y<<2)+3, 
		(z<<2),(z<<2)+1,(z<<2)+2,(z<<2)+3, 
		(w<<2),(w<<2)+1,(w<<2)+2,(w<<2)+3
};


// -------------------------------------------------------------------
// Load, Store & Shuffle

#define M_VShuf(_a, _Mask) __vpermwi(_a,((_Mask & 3)<<6) | ((_Mask & 12)<<2) | ((_Mask & 48)>>2) | ((_Mask & 192)>>6))														// { a[iComp0], a[iComp1], a[iComp2], a[iComp3] }
#define M_VSplat(_a, _iComp) __vspltw(_a, _iComp)														// { a[iComp], a[iComp], a[iComp], a[iComp] }
M_FORCEINLINE vec128 M_VSplatX(vec128 _a) { return __vspltw(_a, 0); }									// { x, x, x, x }
M_FORCEINLINE vec128 M_VSplatY(vec128 _a) { return __vspltw(_a, 1); }									// { y, y, y, y }
M_FORCEINLINE vec128 M_VSplatZ(vec128 _a) { return __vspltw(_a, 2); }									// { z, z, z, z }
M_FORCEINLINE vec128 M_VSplatW(vec128 _a) { return __vspltw(_a, 3); }									// { w, w, w, w }
M_FORCEINLINE vec128 M_VMrgXY(vec128 _a, vec128 _b) { return __vmrghw(_a, _b); }						// { a.x, b.x, a.y, b.y }
M_FORCEINLINE vec128 M_VMrgZW(vec128 _a, vec128 _b) { return __vmrglw(_a, _b); }						// { a.z, b.z, a.w, b.w }

M_FORCEINLINE vec128 M_VLd(fp4 _a) { return M_VSplat(__lvx(NULL, (int)&_a), 0); }
M_FORCEINLINE vec128 M_VLd(fp4 _x, fp4 _y, fp4 _z, fp4 _w) { CVec128Access v; v.x = _x, v.y = _y; v.z = _z, v.w = _w; return v.v; }

M_FORCEINLINE vec128 M_VLdScalar(fp4 x)
{
	return M_VSplatX(__lvlx((vec128*)&x, 0));
}

M_FORCEINLINE vec128 M_VRound(vec128 _a) { return __vrfin(_a); }
M_FORCEINLINE vec128 M_VTrunc(vec128 _a) { return __vrfiz(_a); }

M_FORCEINLINE vec128 M_Vfp4toi32(vec128 _a) { return __vctsxs(_a, 0); }
//M_FORCEINLINE vec128 M_Vfp4tou32(vec128 _a) { return __vctuxs(_a, 0); }
M_FORCEINLINE vec128 M_Vi32tofp4(vec128 _a) { return __vcfsx(_a, 0); }
//M_FORCEINLINE vec128 M_Vu32tofp4(vec128 _a) { return __vcfux(_a, 0); }

M_FORCEINLINE vec128 M_VLd(const CVec4Dfp4& _a) { return _a.v; }
M_FORCEINLINE vec128 M_VLdMem(const void* _pSrc)
{
	return __lvlx (_pSrc, 0);
/*	vec128 a = __lvlx (_pSrc, 0);
	a = __lvrx((void*)(16+(int)_pSrc), 0);
	return a;*/
}
M_FORCEINLINE void M_VSt(vec128 _a, void* _pDest) { __stvx(_a, _pDest, 0); }
M_FORCEINLINE CVec3Dfp4 M_VGetV3(vec128 _a) { CVec128Access v; v.v = _a; return *((CVec3Dfp4*)v.k); };
M_FORCEINLINE CVec4Dfp4 M_VGetV4(vec128 _a) { CVec4Dfp4 v; v.v = _a; return v; };

M_FORCEINLINE vec128 M_VZero() { return (vec128)__vspltisw(0); }												// { 0,0,0,0 }
M_FORCEINLINE vec128 M_VHalf();																			// { 0.5,0.5,0.5,0.5 }
M_FORCEINLINE vec128 M_VOne();																			// { 1,1,1,1 }

// -------------------------------------------------------------------
// Floating point arithmetics
M_FORCEINLINE vec128 M_VNeg(vec128 _a) { return __vsubfp(M_VZero(), _a); }
M_FORCEINLINE vec128 M_VAdd(vec128 _a, vec128 _b) { return __vaddfp(_a, _b); }
M_FORCEINLINE vec128 M_VAddh(vec128 a)
{
	return M_VAdd(M_VAdd(M_VSplatX(a), M_VSplatY(a)), M_VAdd(M_VSplatZ(a), M_VSplatW(a)));
}
M_FORCEINLINE vec128 M_VSub(vec128 _a, vec128 _b) { return __vsubfp(_a, _b); }
M_FORCEINLINE vec128 M_VMul(vec128 _a, vec128 _b) { return __vmulfp(_a, _b); }
M_FORCEINLINE vec128 M_VMAdd(vec128 _a, vec128 _b, vec128 _c) { return __vmaddfp(_a, _b, _c); }			// (a*b) + c
M_FORCEINLINE vec128 M_VNMSub(vec128 _a, vec128 _b, vec128 _c) { return __vnmsubfp(_a, _b, _c); }		// c -(a*b)

M_FORCEINLINE vec128 M_VDp3(vec128 _a, vec128 _b);														// { dp3, dp3, dp3, dp3 }
M_FORCEINLINE vec128 M_VDp3x2(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1);							// { dp3_0, dp3_1, dp3_0, dp3_1 }
M_FORCEINLINE vec128 M_VDp3x3(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1, vec128 _a2, vec128 _b2);	// { dp3_0, dp3_1, dp3_2, 0 }
M_FORCEINLINE vec128 M_VDp3x4(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1, vec128 _a2, vec128 _b2, vec128 _a3, vec128 _b3);	// { dp3_0, dp3_1, dp3_2, dp3_3 }
M_FORCEINLINE vec128 M_VDp4(vec128 _a, vec128 _b);														// { dp, dp, dp, dp }
M_FORCEINLINE vec128 M_VDp4x2(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1);							// { dp0, dp1, dp0, dp1 }
M_FORCEINLINE vec128 M_VDp4x3(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1, vec128 _a2, vec128 _b2);	// { dp0, dp1, dp2, 0 }
M_FORCEINLINE vec128 M_VDp4x4(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1, vec128 _a2, vec128 _b2, vec128 _a3, vec128 _b3);	// // { dp0, dp1, dp2, dp3 }

M_FORCEINLINE vec128 M_VLen3(vec128 _a);																// { |a|, |a|, |a|, |a| }
M_FORCEINLINE vec128 M_VLen3x2(vec128 _a, vec128 _b);													// { |a|, |b|, |a|, |b| }
M_FORCEINLINE vec128 M_VLen3x4(vec128 _a, vec128 _b, vec128 _c, vec128 _d);								// { |a|, |b|, |c|, |d| }
M_FORCEINLINE vec128 M_VLen4(vec128 _a);
M_FORCEINLINE vec128 M_VLen4x2(vec128 _a, vec128 _b);
M_FORCEINLINE vec128 M_VLen4x4(vec128 _a, vec128 _b, vec128 _c, vec128 _d);

M_FORCEINLINE vec128 M_VLenrcp3est(vec128 _a);
M_FORCEINLINE vec128 M_VLenrcp3x2est(vec128 _a, vec128 _b);
M_FORCEINLINE vec128 M_VLenrcp3x4est(vec128 _a, vec128 _b, vec128 _c, vec128 _d);
M_FORCEINLINE vec128 M_VLenrcp4est(vec128 _a);
M_FORCEINLINE vec128 M_VLenrcp4x2est(vec128 _a, vec128 _b);
M_FORCEINLINE vec128 M_VLenrcp4x4est(vec128 _a, vec128 _b, vec128 _c, vec128 _d);

M_FORCEINLINE vec128 M_VLenrcp3(vec128 _a);
M_FORCEINLINE vec128 M_VLenrcp3x2(vec128 _a, vec128 _b);
M_FORCEINLINE vec128 M_VLenrcp3x4(vec128 _a, vec128 _b, vec128 _c, vec128 _d);
M_FORCEINLINE vec128 M_VLenrcp4(vec128 _a);
M_FORCEINLINE vec128 M_VLenrcp4x2(vec128 _a, vec128 _b);
M_FORCEINLINE vec128 M_VLenrcp4x4(vec128 _a, vec128 _b, vec128 _c, vec128 _d);

M_FORCEINLINE vec128 M_VNrm3est(vec128 _a);
//M_FORCEINLINE void M_VNrm3x2est(vec128 _a, vec128 _b);												// Macro, writes result to input vectors
//M_FORCEINLINE void M_VNrm3x4est(vec128 _a, vec128 _b, vec128 _c, vec128 _d);							// Macro, writes result to input vectors
M_FORCEINLINE vec128 M_VNrm4est(vec128 _a);
//M_FORCEINLINE void M_VNrm4x2est(vec128 _a, vec128 _b);												// Macro, writes result to input vectors
//M_FORCEINLINE void M_VNrm4x4est(vec128 _a, vec128 _b, vec128 _c, vec128 _d);							// Macro, writes result to input vectors

M_FORCEINLINE vec128 M_VNrm3(vec128 _a);
//M_FORCEINLINE void M_VNrm3x2(vec128 _a, vec128 _b);													// Macro, writes result to input vectors
//M_FORCEINLINE void M_VNrm3x4(vec128 _a, vec128 _b, vec128 _c, vec128 _d);								// Macro, writes result to input vectors
M_FORCEINLINE vec128 M_VNrm4(vec128 _a);
//M_FORCEINLINE void M_VNrm4x2(vec128 _a, vec128 _b);													// Macro, writes result to input vectors
//M_FORCEINLINE void M_VNrm4x4(vec128 _a, vec128 _b, vec128 _c, vec128 _d);								// Macro, writes result to input vectors

M_FORCEINLINE vec128 M_VXpd(vec128 _a, vec128 _b);
M_FORCEINLINE vec128 M_VMin(vec128 _a, vec128 _b) { return __vminfp(_a, _b); }
M_FORCEINLINE vec128 M_VMax(vec128 _a, vec128 _b) { return __vmaxfp(_a, _b); }
M_FORCEINLINE vec128 M_VAbs(vec128 _a);

M_FORCEINLINE vec128 M_VRcp_Est(vec128 _a) { return __vrefp(_a); }										// Estimate: { 1/x, 1/y, 1/z, 1/w }
M_FORCEINLINE vec128 M_VRsq_Est(vec128 _a) { return __vrsqrtefp(_a); }									// Estimate: { 1/sqrt(x), 1/sqrt(y), 1/sqrt(z), 1/sqrt(w) }
M_FORCEINLINE vec128 M_VSqrt_Est(vec128 _a) { return M_VRcp_Est(M_VRsq_Est(_a)); }						// Estimate: { sqrt(x), sqrt(y), sqrt(z), sqrt(w) }
/*M_FORCEINLINE vec128 M_VExp2_Est(vec128 _a) { return __vexptefp(_a); }									// Estimate: { 2^x, 2^y, 2^z, 2^w }
M_FORCEINLINE vec128 M_VLog2_Est(vec128 _a) { return __vlogefp(_a); }									// Estimate: { log2(x), log2(y), log2(z), log2(w) }
M_FORCEINLINE vec128 M_VPow_Est(vec128 _a, vec128 _b) { return M_VExp2_Est(M_VMul(M_VLog2_Est(_a), _b)); }	// Estimate: exp2(log2(a) * b)
*/
M_FORCEINLINE vec128 M_VRcp(vec128 _a);																	// { 1/x, 1/y, 1/z, 1/w }
M_FORCEINLINE vec128 M_VSqrt(vec128 _a);																// { sqrt(x), sqrt(y), sqrt(z), sqrt(w) }
M_FORCEINLINE vec128 M_VRsqrt(vec128 _a);																// { 1/sqrt(x), 1/sqrt(y), 1/sqrt(z), 1/sqrt(w) }
/*M_FORCEINLINE vec128 M_VExp2(vec128 _a);																// { 2^x, 2^y, 2^z, 2^w }
M_FORCEINLINE vec128 M_VLog2(vec128 _a);																// { log2(x), log2(y), log2(z), log2(w) }
M_FORCEINLINE vec128 M_VPow(vec128 _a, vec128 _b);														// exp2(log2(a) * b)
*/
//M_FORCEINLINE vec128 M_VMulMat(vec128 _a, CMat4Dfp4 _Mat);

// -------------------------------------------------------------------
// Floating point compare
M_FORCEINLINE vec128 M_VCmpEqMsk(vec128 _a, vec128 _b) { return __vcmpeqfp(_a, _b); }					// Equal
M_FORCEINLINE vec128 M_VCmpGEMsk(vec128 _a, vec128 _b) { return __vcmpgefp(_a, _b); }					// Greater-Equal
M_FORCEINLINE vec128 M_VCmpGTMsk(vec128 _a, vec128 _b) { return __vcmpgtfp(_a, _b); }					// Greater Than
M_FORCEINLINE vec128 M_VCmpLEMsk(vec128 _a, vec128 _b) { return __vcmpgefp(_b, _a); }					// Lesser-Equal
M_FORCEINLINE vec128 M_VCmpLTMsk(vec128 _a, vec128 _b) { return __vcmpgtfp(_b, _a); }					// Lesser-Than
M_FORCEINLINE uint M_VCmpAllEq(vec128 _a, vec128 _b);
M_FORCEINLINE uint M_VCmpAllGE(vec128 _a, vec128 _b);
M_FORCEINLINE uint M_VCmpAllGT(vec128 _a, vec128 _b);
M_FORCEINLINE uint M_VCmpAllLE(vec128 _a, vec128 _b);
M_FORCEINLINE uint M_VCmpAllLT(vec128 _a, vec128 _b);
M_FORCEINLINE uint M_VCmpAnyEq(vec128 _a, vec128 _b);
M_FORCEINLINE uint M_VCmpAnyGE(vec128 _a, vec128 _b);
M_FORCEINLINE uint M_VCmpAnyGT(vec128 _a, vec128 _b);
M_FORCEINLINE uint M_VCmpAnyLE(vec128 _a, vec128 _b);
M_FORCEINLINE uint M_VCmpAnyLT(vec128 _a, vec128 _b);

// -------------------------------------------------------------------
M_FORCEINLINE vec128 M_VSelMsk(vec128 _mask, vec128 _a, vec128 _b) { return __vsel(_b, _a, _mask); }		// (mask & a) | (!mask & b)

// -------------------------------------------------------------------
// Logical operations
M_FORCEINLINE vec128 M_VAnd(vec128 _a, vec128 _b) { return __vand(_a, _b); }
M_FORCEINLINE vec128 M_VAndc(vec128 _a, vec128 _b) { return __vandc(_a, _b); }
M_FORCEINLINE vec128 M_VOr(vec128 _a, vec128 _b) { return __vor(_a, _b); }
M_FORCEINLINE vec128 M_VNor(vec128 _a, vec128 _b) { return __vnor(_a, _b); }
M_FORCEINLINE vec128 M_VXor(vec128 _a, vec128 _b) { return __vxor(_a, _b); }

// -------------------------------------------------------------------
// Signed Integer arithmetics
M_FORCEINLINE vec128 M_VAdds_i8(vec128 _a, vec128 _b) { return __vaddsbs(_a, _b); }
M_FORCEINLINE vec128 M_VAdds_i16(vec128 _a, vec128 _b) { return __vaddshs(_a, _b); }
M_FORCEINLINE vec128 M_VAdds_i32(vec128 _a, vec128 _b) { return __vaddsws(_a, _b); }

M_FORCEINLINE vec128 M_VSubs_i8(vec128 _a, vec128 _b) { return __vsubsbs(_a, _b); }
M_FORCEINLINE vec128 M_VSubs_i16(vec128 _a, vec128 _b) { return __vsubshs(_a, _b); }
M_FORCEINLINE vec128 M_VSubs_i32(vec128 _a, vec128 _b) { return __vsubsws(_a, _b); }

M_FORCEINLINE vec128 M_VMin_i8(vec128 _a, vec128 _b) { return __vminsb(_a, _b); }
M_FORCEINLINE vec128 M_VMin_i16(vec128 _a, vec128 _b) { return __vminsh(_a, _b); }
M_FORCEINLINE vec128 M_VMin_i32(vec128 _a, vec128 _b) { return __vminsw(_a, _b); }

M_FORCEINLINE vec128 M_VMax_i8(vec128 _a, vec128 _b) { return __vmaxsb(_a, _b); }
M_FORCEINLINE vec128 M_VMax_i16(vec128 _a, vec128 _b) { return __vmaxsh(_a, _b); }
M_FORCEINLINE vec128 M_VMax_i32(vec128 _a, vec128 _b) { return __vmaxsw(_a, _b); }

// -------------------------------------------------------------------
// Unsigned Integer arithmetics
M_FORCEINLINE vec128 M_VAdd_u8(vec128 _a, vec128 _b) { return __vaddubm(_a, _b); }
M_FORCEINLINE vec128 M_VAdds_u8(vec128 _a, vec128 _b) { return __vaddubs(_a, _b); }
M_FORCEINLINE vec128 M_VAdd_u16(vec128 _a, vec128 _b) { return __vadduhm(_a, _b); }
M_FORCEINLINE vec128 M_VAdds_u16(vec128 _a, vec128 _b) { return __vadduhs(_a, _b); }
M_FORCEINLINE vec128 M_VAdd_u32(vec128 _a, vec128 _b) { return __vadduwm(_a, _b); }
M_FORCEINLINE vec128 M_VAdds_u32(vec128 _a, vec128 _b) { return __vadduws(_a, _b); }

M_FORCEINLINE vec128 M_VAddc_u32(vec128 _a, vec128 _b) { return __vaddcuw(_a, _b); }				// Add w/ carry-out

// -------------------------------------------------------------------
M_FORCEINLINE vec128 M_VSub_u8(vec128 _a, vec128 _b) { return __vsububm(_a, _b); }
M_FORCEINLINE vec128 M_VSubs_u8(vec128 _a, vec128 _b) { return __vsububs(_a, _b); }
M_FORCEINLINE vec128 M_VSub_u16(vec128 _a, vec128 _b) { return __vsubuhm(_a, _b); }
M_FORCEINLINE vec128 M_VSubs_u16(vec128 _a, vec128 _b) { return __vsubuhs(_a, _b); }
M_FORCEINLINE vec128 M_VSub_u32(vec128 _a, vec128 _b) { return __vsubuwm(_a, _b); }
M_FORCEINLINE vec128 M_VSubs_u32(vec128 _a, vec128 _b) { return __vsubuws(_a, _b); }

M_FORCEINLINE vec128 M_VSubc_u32(vec128 _a, vec128 _b) { return __vsubcuw(_a, _b); }				// Subtract w/ carry-out

// -------------------------------------------------------------------
M_FORCEINLINE vec128 M_VMin_u8(vec128 _a, vec128 _b) { return __vminub(_a, _b); }
M_FORCEINLINE vec128 M_VMin_u16(vec128 _a, vec128 _b) { return __vminuh(_a, _b); }
M_FORCEINLINE vec128 M_VMin_u32(vec128 _a, vec128 _b) { return __vminuw(_a, _b); }

M_FORCEINLINE vec128 M_VMax_u8(vec128 _a, vec128 _b) { return __vmaxub(_a, _b); }
M_FORCEINLINE vec128 M_VMax_u16(vec128 _a, vec128 _b) { return __vmaxuh(_a, _b); }
M_FORCEINLINE vec128 M_VMax_u32(vec128 _a, vec128 _b) { return __vmaxuw(_a, _b); }

// -------------------------------------------------------------------
// Integer shift
/*vec128 M_VSar_i16(vec128 a, uint b)
vec128 M_VShr_u16(vec128 a, uint b)
vec128 M_VShl_u16(vec128 a, uint b);								8 * 16bit shift left, a << b
vec128 M_VSar_i32(vec128 a, uint b);								4 * 32bit algebraic shift right, a >> b
vec128 M_VShr_u32(vec128 a, uint b);								4 * 32bit shift right, a >> b
vec128 M_VShl_u32(vec128 a, uint b);								4 * 32bit shift left, a << b
*/
// -------------------------------------------------------------------
// Implementation
#ifdef PLATFORM_XENON
M_FORCEINLINE vec128 M_VHalf()
{
    return __vcfsx(__vspltisw(1), 1);
}

M_FORCEINLINE vec128 M_VOne()
{
	vec128 a;
	a = __vspltisw(0);
    a = __vupkd3d(a, VPACK_NORMSHORT2);
    return __vspltw(a, 3);
}

#else
M_FORCEINLINE vec128 M_VHalf()
{
	static const vec128 a = { 0.5f, 0.5f, 0.5f, 0.5f }; 
	return a;
}

M_FORCEINLINE vec128 M_VOne()
{
	static const vec128 a = { 1.0f, 1.0f, 1.0f, 1.0f }; 
	return a;
}
#endif

M_FORCEINLINE vec128 M_VTwo()
{
	static const vec128 a = { 2.0f, 2.0f, 2.0f, 2.0f }; 
	return a;
}

#ifdef PLATFORM_XENON
M_FORCEINLINE vec128 M_VDp3(vec128 _a, vec128 _b)
{
	return __vmsum3fp(_a, _b);
}

M_FORCEINLINE vec128 M_VDp3x2(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1)
{
	vec128 dp0 = __vmsum3fp(_a0, _b0);
	vec128 dp1 = __vmsum3fp(_a1, _b1);
	return M_VMrgXY(dp0, dp1);
}

M_FORCEINLINE vec128 M_VDp3x4(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1, vec128 _a2, vec128 _b2)
{
	vec128 dp0 = __vmsum3fp(_a0, _b0);
	vec128 dp1 = __vmsum3fp(_a1, _b1);
	vec128 dp2 = __vmsum3fp(_a2, _b2);
	vec128 dp3 = M_VZero();
	return M_VMrgXY(M_VMrgXY(dp0, dp2), M_VMrgXY(dp1, dp3));
}

M_FORCEINLINE vec128 M_VDp3x4(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1, vec128 _a2, vec128 _b2, vec128 _a3, vec128 _b3)
{
	vec128 dp0 = __vmsum3fp(_a0, _b0);
	vec128 dp1 = __vmsum3fp(_a1, _b1);
	vec128 dp2 = __vmsum3fp(_a2, _b2);
	vec128 dp3 = __vmsum3fp(_a3, _b3);
	return M_VMrgXY(M_VMrgXY(dp0, dp2), M_VMrgXY(dp1, dp3));
}

M_FORCEINLINE vec128 M_VDp4(vec128 _a, vec128 _b)
{
	return __vmsum4fp(_a, _b);
}

M_FORCEINLINE vec128 M_VDp4x2(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1)
{
	vec128 dp0 = __vmsum4fp(_a0, _b0);
	vec128 dp1 = __vmsum4fp(_a1, _b1);
	return M_VMrgXY(dp0, dp1);
}

M_FORCEINLINE vec128 M_VDp4x3(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1, vec128 _a2, vec128 _b2)
{
	vec128 dp0 = __vmsum4fp(_a0, _b0);
	vec128 dp1 = __vmsum4fp(_a1, _b1);
	vec128 dp2 = __vmsum4fp(_a2, _b2);
	vec128 dp3 = M_VZero();
	return M_VMrgXY(M_VMrgXY(dp0, dp2), M_VMrgXY(dp1, dp3));
}

M_FORCEINLINE vec128 M_VDp4x4(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1, vec128 _a2, vec128 _b2, vec128 _a3, vec128 _b3)
{
	vec128 dp0 = __vmsum4fp(_a0, _b0);
	vec128 dp1 = __vmsum4fp(_a1, _b1);
	vec128 dp2 = __vmsum4fp(_a2, _b2);
	vec128 dp3 = __vmsum4fp(_a3, _b3);
	return M_VMrgXY(M_VMrgXY(dp0, dp2), M_VMrgXY(dp1, dp3));
}

#else
#endif

// -------------------------------------------------------------------

/*M_FORCEINLINE vec128 M_VExp2(vec128 _a)
{
	return M_VZero();
}

M_FORCEINLINE vec128 M_VLog2(vec128 _a)
{
	return M_VZero();
}

M_FORCEINLINE vec128 M_VPow(vec128 _a, vec128 _b)
{
	return M_VZero();
}*/

// -------------------------------------------------------------------
// Do not use these:
/*M_FORCEINLINE vec128 M_VCmpEqMskCR(vec128 _a, vec128 _b, unsigned int* _pCR) { unsigned int CR; vec128 c = __vcmpeqfpR(_a, _b, &CR); *_pCR = CR >> 4; return c; }
M_FORCEINLINE vec128 M_VCmpGEMskCR(vec128 _a, vec128 _b, unsigned int* _pCR) { unsigned int CR; vec128 c = __vcmpgefpR(_a, _b, &CR); *_pCR = CR >> 4; return c; }
M_FORCEINLINE vec128 M_VCmpGTMskCR(vec128 _a, vec128 _b, unsigned int* _pCR) { unsigned int CR; vec128 c = __vcmpgtfpR(_a, _b, &CR); *_pCR = CR >> 4; return c; }
M_FORCEINLINE vec128 M_VCmpLEMskCR(vec128 _a, vec128 _b, unsigned int* _pCR) { unsigned int CR; vec128 c = __vcmpgefpR(_b, _a, &CR); *_pCR = CR >> 4; return c; }
M_FORCEINLINE vec128 M_VCmpLTMskCR(vec128 _a, vec128 _b, unsigned int* _pCR) { unsigned int CR; vec128 c = __vcmpgtfpR(_b, _a, &CR); *_pCR = CR >> 4; return c; }
*/
// -------------------------------------------------------------------
#ifndef PLATFORM_XENON

#define M_VCmpAllEq(a, b) vec_all_eq(a, b)
#define M_VCmpAllGE(a, b) vec_all_ge(a, b)
#define M_VCmpAllGT(a, b) vec_all_gt(a, b)
#define M_VCmpAllLE(a, b) vec_all_le(a, b)
#define M_VCmpAllLT(a, b) vec_all_lt(a, b)

#define M_VCmpAnyEq(a, b) vec_any_eq(a, b)
#define M_VCmpAnyGE(a, b) vec_any_ge(a, b)
#define M_VCmpAnyGT(a, b) vec_any_gt(a, b)
#define M_VCmpAnyLE(a, b) vec_any_le(a, b)
#define M_VCmpAnyLT(a, b) vec_any_lt(a, b)

#else

M_FORCEINLINE unsigned int M_VCmpAllEq(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpeqfpR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllGE(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgefpR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllGT(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtfpR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllLE(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgefpR(_b, _a, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllLT(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtfpR(_b, _a, &CR);
	return (CR & 128) ? 1 : 0;
}

// -------------------------------------------------------------------
M_FORCEINLINE unsigned int M_VCmpAnyEq(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpeqfpR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyGE(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgefpR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyGT(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtfpR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyLE(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgefpR(_b, _a, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyLT(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtfpR(_b, _a, &CR);
	return (CR & 32) ? 0 : 1;
}

#endif // Xenon
// -------------------------------------------------------------------
#endif // defined(CPU_POWERPC) && !defined(CPU_VEC128EMU)
#endif // __INCLUDE_XCC_MATH_VEC128_VMX
