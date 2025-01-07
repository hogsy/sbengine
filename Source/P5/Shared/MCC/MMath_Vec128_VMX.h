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
/*
M_FORCEINLINE vec128 __lvlx( const void *a1,int a2 )
{
vec128 ret;
__asm__ volatile( "nop" );
__asm__( ".long 0x7c00040e+(%0<<21)+(%1<<16)+(%2<<11)" : "=v"(ret) :"b"(a1),"r"(a2) );
return ret;
}


M_FORCEINLINE vec128 __lvrx( const void *a1,int a2 )
{
vec128 ret;
__asm__ volatile( "nop" );
__asm__( ".long 0x7c00044e+(%0<<21)+(%1<<16)+(%2<<11)" : "=v"(ret) :"b"(a1),"r"(a2) );
return ret;
}

M_FORCEINLINE void __stvlx( vec128 v,void *a1,int a2 )
{
__asm__ volatile( "nop" );
__asm__( ".long 0x7c00050e+(%0<<21)+(%1<<16)+(%2<<11)" : : "v"(v),"b"(a1),"r"(a2) : "memory" );
}

M_FORCEINLINE void __stvrx( vec128 v,void *a1,int a2 )
{
__asm__ volatile( "nop" );
__asm__( ".long 0x7c00054e+(%0<<21)+(%1<<16)+(%2<<11)" : : "v"(v),"b"(a1),"r"(a2) : "memory" );
}
*/

// -------------------------------------------------------------------
#define __vspltw(_a, _iComp) vec_vspltw(_a, _iComp)
//#define __vpermwi(_a, _Mask) vec_vperm(_a, _a, (vec128)TVec128Perm<_Mask & 3, (_Mask>>2) & 3, (_Mask>>4) & 3, (_Mask>>6) & 3>::m_Perm)
#define __vpermwi(_a, _Mask) ((vec128)__builtin_altivec_vperm_4si((__vec128i32)_a, (__vec128i32)_a, *(__vec128i8*)&TVec128Perm<((_Mask) >> 6) & 3, ((_Mask) >> 4) & 3, ((_Mask)>>2) & 3, (_Mask) & 3>::m_Perm))
#define __vperm(_a, _b, _perm) ((vec128)__builtin_altivec_vperm_16qi((__vec128i8)(_a), (__vec128i8)(_b), (__vec128i8)(_perm)))

#define __vmrghw(a, b) (vec128)__builtin_altivec_vmrghw((__vec128i32)(a), (__vec128i32)(b))
#define __vmrglw(a, b) (vec128)__builtin_altivec_vmrglw((__vec128i32)(a), (__vec128i32)(b))
#define __vmrghh(a, b) (vec128)__builtin_altivec_vmrghh((__vec128i16)(a), (__vec128i16)(b))
#define __vmrglh(a, b) (vec128)__builtin_altivec_vmrglh((__vec128i16)(a), (__vec128i16)(b))
#define __vmrghb(a, b) (vec128)__builtin_altivec_vmrghb((__vec128i8)(a), (__vec128i8)(b))
#define __vmrglb(a, b) (vec128)__builtin_altivec_vmrglb((__vec128i8)(a), (__vec128i8)(b))

#define __vupklsh(a) (vec128)__builtin_altivec_vupklsh((__vec128i16)(a))
#define __vupkhsh(a) (vec128)__builtin_altivec_vupkhsh((__vec128i16)(a))
#define __vupklsb(a) (vec128)__builtin_altivec_vupklsb((__vec128i8)(a))
#define __vupkhsb(a) (vec128)__builtin_altivec_vupkhsb((__vec128i8)(a))

#define __vpkuwum(a, b) (vec128)__builtin_altivec_vpkuwum((__vec128i32)(a), (__vec128i32)(b))
#define __vpkuwus(a, b) (vec128)__builtin_altivec_vpkuwus((__vec128i32)(a), (__vec128i32)(b))
#define __vpkswss(a, b) (vec128)__builtin_altivec_vpkswss((__vec128i32)(a), (__vec128i32)(b))
#define __vpkswus(a, b) (vec128)__builtin_altivec_vpkswus((__vec128i32)(a), (__vec128i32)(b))

#define __vpkuhum(a, b) (vec128)__builtin_altivec_vpkuhum((__vec128i16)(a), (__vec128i16)(b))
#define __vpkuhus(a, b) (vec128)__builtin_altivec_vpkuhus((__vec128i16)(a), (__vec128i16)(b))
#define __vpkshss(a, b) (vec128)__builtin_altivec_vpkshss((__vec128i16)(a), (__vec128i16)(b))
#define __vpkshus(a, b) (vec128)__builtin_altivec_vpkshus((__vec128i16)(a), (__vec128i16)(b))


#define __lvx(a, b) (vec128)__builtin_altivec_lvx(a, (vec128*)(b))
//#define __lvlx(a, b) (vec128)builtin_altivec_lvlx((void*)(a), b)
#define __lvlx(a, b) (vec128)__builtin_altivec_lvlx(b, (void*)(a))
#define __lvrx(a, b) (vec128)__builtin_altivec_lvrx(b, (void*)(a))
#define __stvlx(a, b, c) __builtin_altivec_stvlx((__vec128i8)a, c, (b))
#define __stvrx(a, b, c) __builtin_altivec_stvrx((__vec128i8)a, c, (b))

#define __vrfin(a) (vec128)__builtin_altivec_vrfin(a)
#define __vrfiz(a) (vec128)__builtin_altivec_vrfiz(a)
#define __vcfsx(a, shift) ((vec128)__builtin_altivec_vcfsx((__vec128i32)a, shift))
#define __vctsxs(a, shift) ((vec128)__builtin_altivec_vctsxs(a, shift))
#define __stvx(_v, _pDst, _Offset) __builtin_altivec_stvx((__vec128i32)(_v), _Offset, _pDst)
#define __stvewx(_v, _pDst, _Offset) __builtin_altivec_stvewx((__vec128i32)(_v), _Offset, _pDst)

#define __vspltisb(a) __builtin_altivec_vspltisb(a)
#define __vspltish(a) __builtin_altivec_vspltish(a)
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

// -------------------------------------------------------------------
#define __vsraw(a, b) ((vec128)__builtin_altivec_vsraw((__vec128i32)(a), (__vec128i32)(b)))
#define __vsrw(a, b) ((vec128)__builtin_altivec_vsrw((__vec128i32)(a), (__vec128i32)(b)))
#define __vslw(a, b) ((vec128)__builtin_altivec_vslw((__vec128i32)(a), (__vec128i32)(b)))

#define __vsrah(a, b) ((vec128)__builtin_altivec_vsrah((__vec128i16)(a), (__vec128i16)(b)))
#define __vsrh(a, b) ((vec128)__builtin_altivec_vsrh((__vec128i16)(a), (__vec128i16)(b)))
#define __vslh(a, b) ((vec128)__builtin_altivec_vslh((__vec128i16)(a), (__vec128i16)(b)))

#define __vsrab(a, b) ((vec128)__builtin_altivec_vsrab((__vec128i8)(a), (__vec128i8)(b)))
#define __vsrb(a, b) ((vec128)__builtin_altivec_vsrb((__vec128i8)(a), (__vec128i8)(b)))
#define __vslb(a, b) ((vec128)__builtin_altivec_vslb((__vec128i8)(a), (__vec128i8)(b)))


#endif

//typedef const vec128& vec128p;
//typedef const CMat4Dfp32& CMat4Dfp32p;

typedef vec128 vec128p;
typedef CMat4Dfp32 CMat4Dfp32p;

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
#define M_V128IMP_INTEGERCOMPARE
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
uint8 M_ALIGN(16) TVec128Perm<x, y, z, w>::m_Perm[16] = 
{ 
	(x<<2),(x<<2)+1,(x<<2)+2,(x<<2)+3, 
	(y<<2),(y<<2)+1,(y<<2)+2,(y<<2)+3, 
	(z<<2),(z<<2)+1,(z<<2)+2,(z<<2)+3, 
	(w<<2),(w<<2)+1,(w<<2)+2,(w<<2)+3
};


// -------------------------------------------------------------------
// Load, Store & Shuffle

M_FORCEINLINE vec128 M_VZero() { return (vec128)__vspltisw(0); }										// { 0,0,0,0 }
M_FORCEINLINE vec128 M_VOne_u8() {return (vec128)__vspltisb(1); }
M_FORCEINLINE vec128 M_VOne_u16() {return (vec128)__vspltish(1); }
M_FORCEINLINE vec128 M_VOne_u32() {return (vec128)__vspltisw(1); }
//M_FORCEINLINE vec128 M_VNegOne_i8() {return (vec128)__vspltisb(-1); }
//M_FORCEINLINE vec128 M_VNegOne_i16() {return (vec128)__vspltish(-1); }
M_FORCEINLINE vec128 M_VNegOne_i32() {return (vec128)__vspltisw(-1); }
M_FORCEINLINE vec128 M_VHalf();																			// { 0.5,0.5,0.5,0.5 }
M_FORCEINLINE vec128 M_VOne();																			// { 1,1,1,1 }
#define M_VShuf(_a, _Mask) __vpermwi(_a,((_Mask & 3)<<6) | ((_Mask & 12)<<2) | ((_Mask & 48)>>2) | ((_Mask & 192)>>6))														// { a[iComp0], a[iComp1], a[iComp2], a[iComp3] }
#define M_VSplat(_a, _iComp) __vspltw(_a, _iComp)														// { a[iComp], a[iComp], a[iComp], a[iComp] }
M_FORCEINLINE vec128 M_VSplatX(vec128 _a) { return __vspltw(_a, 0); }									// { x, x, x, x }
M_FORCEINLINE vec128 M_VSplatY(vec128 _a) { return __vspltw(_a, 1); }									// { y, y, y, y }
M_FORCEINLINE vec128 M_VSplatZ(vec128 _a) { return __vspltw(_a, 2); }									// { z, z, z, z }
M_FORCEINLINE vec128 M_VSplatW(vec128 _a) { return __vspltw(_a, 3); }									// { w, w, w, w }
M_FORCEINLINE vec128 M_VMrgL(vec128 _a, vec128 _b) { return __vmrghw(_a, _b); }						// { a.x, b.x, a.y, b.y }
M_FORCEINLINE vec128 M_VMrgH(vec128 _a, vec128 _b) { return __vmrglw(_a, _b); }						// { a.z, b.z, a.w, b.w }

M_FORCEINLINE vec128 M_VMrgL_u16(vec128 _a, vec128 _b) { return __vmrghh(_a, _b); }
M_FORCEINLINE vec128 M_VMrgH_u16(vec128 _a, vec128 _b) { return __vmrglh(_a, _b); }
M_FORCEINLINE vec128 M_VMrgL_u8(vec128 _a, vec128 _b)  { return __vmrghb(_a, _b); }
M_FORCEINLINE vec128 M_VMrgH_u8(vec128 _a, vec128 _b) { return __vmrglb(_a, _b); }

M_FORCEINLINE vec128 M_VMrgL_u16_u32(vec128 _a, vec128 _b) { return __vmrghh(_b, _a); }
M_FORCEINLINE vec128 M_VMrgH_u16_u32(vec128 _a, vec128 _b) { return __vmrglh(_b, _a); }
M_FORCEINLINE vec128 M_VMrgL_u8_u16(vec128 _a, vec128 _b)  { return __vmrghb(_b, _a); }
M_FORCEINLINE vec128 M_VMrgH_u8_u16(vec128 _a, vec128 _b) { return __vmrglb(_b, _a); }


M_FORCEINLINE vec128 M_VLd(fp32 _x, fp32 _y, fp32 _z, fp32 _w) { CVec128Access v; v.x = _x, v.y = _y; v.z = _z, v.w = _w; return v.v; }

M_FORCEINLINE vec128 M_VLdScalar(const fp32& x)
{
	return M_VSplatX(__lvlx((const vec128*)&x, 0));
}

M_FORCEINLINE vec128 M_VLdScalar_u8(const uint8& _Src) { vec128 a = __lvlx(&_Src, NULL); return __vperm(a, a, M_VZero()); }
M_FORCEINLINE vec128 M_VLdScalar_i8(const uint8& _Src) { vec128 a = __lvlx(&_Src, NULL); return __vperm(a, a, M_VZero()); }
M_FORCEINLINE vec128 M_VLdScalar_u16(const uint16& _Src) { vec128 a = __lvlx(&_Src, NULL); return __vperm(a, a, M_VScalar_u16(0x0001)); }
M_FORCEINLINE vec128 M_VLdScalar_i16(const int16& _Src) { vec128 a = __lvlx(&_Src, NULL); return __vperm(a, a, M_VScalar_u16(0x0001)); }
M_FORCEINLINE vec128 M_VLdScalar_u32(const uint32& x) { return M_VSplatX(__lvlx((const vec128*)&x, 0)); }
M_FORCEINLINE vec128 M_VLdScalar_i32(const int32& x) { return M_VSplatX(__lvlx((const vec128*)&x, 0)); }


M_FORCEINLINE vec128 M_VRound(vec128 _a) { return __vrfin(_a); }
M_FORCEINLINE vec128 M_VTrunc(vec128 _a) { return __vrfiz(_a); }

M_FORCEINLINE vec128 M_VCnv_f32_i32(vec128 _a) { return __vctsxs(_a, 0); }
//M_FORCEINLINE vec128 M_Vfp32tou32(vec128 _a) { return __vctuxs(_a, 0); }
M_FORCEINLINE vec128 M_VCnv_i32_f32(vec128 _a) { return __vcfsx(_a, 0); }
//M_FORCEINLINE vec128 M_Vu32tofp32(vec128 _a) { return __vcfux(_a, 0); }

M_FORCEINLINE vec128 M_VCnvM_u32_u16(vec128 a, vec128 b) { return __vpkuwum(a, b); };
M_FORCEINLINE vec128 M_VCnvS_u32_u16(vec128 a, vec128 b) { return __vpkuwus(a, b); };
M_FORCEINLINE vec128 M_VCnvS_i32_i16(vec128 a, vec128 b) { return __vpkswss(a, b); };
M_FORCEINLINE vec128 M_VCnvS_i32_u16(vec128 a, vec128 b) { return __vpkswus(a, b); };

M_FORCEINLINE vec128 M_VCnvM_u16_u8(vec128 a, vec128 b) { return __vpkuhum(a, b); };
M_FORCEINLINE vec128 M_VCnvS_u16_u8(vec128 a, vec128 b) { return __vpkuhus(a, b); };
M_FORCEINLINE vec128 M_VCnvS_i16_i8(vec128 a, vec128 b) { return __vpkshss(a, b); };
M_FORCEINLINE vec128 M_VCnvS_i16_u8(vec128 a, vec128 b) { return __vpkshus(a, b); };

M_FORCEINLINE vec128 M_VCnvL_u8_u16(vec128 _a) { return __vmrghb(M_VZero(), _a); }
M_FORCEINLINE vec128 M_VCnvH_u8_u16(vec128 _a) { return __vmrglb(M_VZero(), _a); }
M_FORCEINLINE vec128 M_VCnvL_i8_i16(vec128 _a) { return __vupkhsb(_a); }
M_FORCEINLINE vec128 M_VCnvH_i8_i16(vec128 _a) { return __vupklsb(_a); }
M_FORCEINLINE vec128 M_VCnvL_u16_u32(vec128 _a) { return __vmrghh(M_VZero(), _a); }
M_FORCEINLINE vec128 M_VCnvH_u16_u32(vec128 _a) { return __vmrglh(M_VZero(), _a); }
M_FORCEINLINE vec128 M_VCnvL_i16_i32(vec128 _a) { return __vupkhsh(_a); }
M_FORCEINLINE vec128 M_VCnvH_i16_i32(vec128 _a) { return __vupklsh(_a); }


M_FORCEINLINE vec128 M_VPerm_Helper(vec128 _a, vec128 _b, vec128 _Perm) 
{ 
	return __vperm(_a, _b, _Perm);
}

#define M_VPerm(_a, _b, _0, _1, _2, _3) \
	M_VPerm_Helper(_a, _b, M_VConst_u8(_0*4+0,_0*4+1,_0*4+2,_0*4+3, _1*4+0,_1*4+1,_1*4+2,_1*4+3, _2*4+0,_2*4+1,_2*4+2,_2*4+3, _3*4+0,_3*4+1,_3*4+2,_3*4+3));


M_FORCEINLINE vec128 M_VLd(const void* _pSrc)
{
	return *((vec128*)_pSrc);
//	return __lvlx (_pSrc, 0);
/*	vec128 a = __lvlx (_pSrc, 0);
	a = __lvrx((void*)(16+(int)_pSrc), 0);
	return a;*/
}

M_FORCEINLINE vec128 M_VLdU(const void* _pMem)
{
	vec128 l = __lvlx((void*) _pMem, 0);
	vec128 r = __lvrx((void*) _pMem, 16);
	vec128 res = __vor(r,l);
	return res;
}


M_FORCEINLINE vec128 M_VLd64(const void* _pSrc) { return M_VShuf(__lvlx(_pSrc, NULL), M_VSHUF(0,1,0,1)); }
M_FORCEINLINE vec128 M_VLd32(const void* _pSrc) { return M_VSplatX(__lvlx(_pSrc, NULL)); }

template<class T>
M_FORCEINLINE void M_VStU_V3x2_Slow(vec128 _a, vec128 _b, T*M_RESTRICT _pDest)
{
	vec128 selmask = M_VConst_u32(0xffffffff, 0xffffffff, 0xffffffff, 0x00000000);
	vec128 a = __vsel(M_VSplatX(_b), _a, selmask);

	__stvlx(a, _pDest, 0);
	__stvrx(a, _pDest, 16);
	__stvewx(M_VSplatY(_b), _pDest, 16);
	__stvewx(M_VSplatZ(_b), _pDest, 20);
}

M_FORCEINLINE void M_VLdU_V3x2_Slow(const void* _pSrc, vec128& _a, vec128& _b)
{
	vec128 selmask = M_VConst_u32(0xffffffff, 0xffffffff, 0xffffffff, 0x00000000);
	vec128 zero = M_VZero();
	uint8* pS = (uint8*)_pSrc;

	vec128 a0 = __lvlx(pS, 0);
	vec128 a1 = __lvrx(pS, 16);
	vec128 b0 = __lvlx(pS, 12);
	vec128 b1 = __lvrx(pS, 12 + 16);
	_a = __vsel(zero, __vor(a0, a1), selmask);
	_b = __vsel(zero, __vor(b0, b1), selmask);
}

template<class T>
M_FORCEINLINE void M_VSt(vec128 _a, T*M_RESTRICT _pDest) { __stvx(_a, _pDest, 0); }

template<class T>
M_FORCEINLINE void M_VStAny64(vec128 _a, T*M_RESTRICT _pDest)
{ 
	__stvewx(_a, _pDest, 0); 
	__stvewx(_a, _pDest, 4);
}

template<class T>
M_FORCEINLINE void M_VStL64(vec128 _a, T*M_RESTRICT _pDest)
{ 
	vec128 a = M_VShuf(_a, M_VSHUF(0, 1, 0, 1));
	__stvewx(a, _pDest, 0);
	__stvewx(a, _pDest, 4);
}

template<class T>
M_FORCEINLINE void M_VStH64(vec128 _a, T*M_RESTRICT _pDest)
{ 
	vec128 a = M_VShuf(_a, M_VSHUF(2, 3, 2, 3));
	__stvewx(a, _pDest, 0);
	__stvewx(a, _pDest, 4);
}

template<class T>
M_FORCEINLINE void M_VStAny32(vec128 _a, T*M_RESTRICT _pDest) { __stvewx(_a, _pDest, 0); }

template<class T>
M_FORCEINLINE void M_VStAny32Ex(vec128 _a, T*M_RESTRICT _pDest, mint _Ofs) { __stvewx(_a, _pDest, _Ofs); }

M_FORCEINLINE void M_VLdU_P3x2_Slow(const void* _pSrc, vec128& _a, vec128& _b)
{
	vec128 selmask = M_VConst_u32(0xffffffff, 0xffffffff, 0xffffffff, 0x00000000);
	vec128 one = M_VOne();
	uint8* pS = (uint8*)_pSrc;

	vec128 a0 = __lvlx(pS, 0);
	vec128 a1 = __lvrx(pS, 16);
	vec128 b0 = __lvlx(pS, 12);
	vec128 b1 = __lvrx(pS, 12 + 16);
	_a = __vsel(one, __vor(a0, a1), selmask);
	_b = __vsel(one, __vor(b0, b1), selmask);
}

#ifndef M_COMPILING_ON_VPU
M_FORCEINLINE CVec3Dfp32 M_VGetV3(vec128 _a) { CVec128Access v; v.v = _a; return *((CVec3Dfp32*)v.k); };
M_FORCEINLINE CVec4Dfp32 M_VGetV4(vec128 _a) { CVec4Dfp32 v; v.v = _a; return v; };
#endif

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
//M_FORCEINLINE vec128 M_VMulMat(vec128 _a, CMat4Dfp32 _Mat);

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
M_FORCEINLINE vec128 M_VNot(vec128 _a) { return __vnor(_a,_a); }

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


M_FORCEINLINE vec128 M_VShl8_u128_Helper(vec128 _a, vec128 _Perm) 
{ 
	return __vperm(_a, M_VConst(0.0f,0.0f,0.0f,0.0f), _Perm);
}

#define M_VShl8_u128(_a, _n) \
	M_VShl8_u128_Helper(_a, M_VConst_u8(0+_n,1+_n,2+_n,3+_n,4+_n,5+_n,6+_n,7+_n,8+_n,9+_n,10+_n,11+_n,12+_n,13+_n,14+_n,15+_n));


M_FORCEINLINE vec128 M_VSar_i32(vec128 _a, vec128 _b) { return __vsraw(_a, _b); };
#define M_VSarImm_i32(a, ImmShift) __vsraw(a, M_VScalar_u32(ImmShift))
M_FORCEINLINE vec128 M_VShr_u32(vec128 _a, vec128 _b) { return __vsrw(_a, _b); };
#define M_VShrImm_u32(a, ImmShift) __vsrw(a, M_VScalar_u32(ImmShift))
M_FORCEINLINE vec128 M_VShl_u32(vec128 _a, vec128 _b) { return __vslw(_a, _b); };
#define M_VShlImm_u32(a, ImmShift) __vslw(a, M_VScalar_u32(ImmShift))

M_FORCEINLINE vec128 M_VSar_i16(vec128 _a, vec128 _b) { return __vsrah(_a, _b); };
#define M_VSarImm_i16(a, ImmShift) __vsrah(a, M_VScalar_u16(ImmShift))
M_FORCEINLINE vec128 M_VShr_u16(vec128 _a, vec128 _b) { return __vsrh(_a, _b); };
#define M_VShrImm_u16(a, ImmShift) __vsrh(a, M_VScalar_u16(ImmShift))
M_FORCEINLINE vec128 M_VShl_u16(vec128 _a, vec128 _b) { return __vslh(_a, _b); };
#define M_VShlImm_u16(a, ImmShift) __vslh(a, M_VScalar_u16(ImmShift))

M_FORCEINLINE vec128 M_VSar_i8(vec128 _a, vec128 _b) { return __vsrab(_a, _b); };
#define M_VSarImm_i8(a, ImmShift) __vsrab(a, M_VScalar_u8(ImmShift))
M_FORCEINLINE vec128 M_VShr_u8(vec128 _a, vec128 _b) { return __vsrb(_a, _b); };
#define M_VShrImm_u8(a, ImmShift) __vsrb(a, M_VScalar_u8(ImmShift))
M_FORCEINLINE vec128 M_VShl_u8(vec128 _a, vec128 _b) { return __vslb(_a, _b); };
#define M_VShlImm_u8(a, ImmShift) __vslb(a, M_VScalar_u8(ImmShift))

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

M_FORCEINLINE vec128 M_VDp3x3(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1, vec128 _a2, vec128 _b2)
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

#define M_VCmpAnyEq_u32(a, b) vec_any_eq((__vec128u32)a, (__vec128u32)b)
#define M_VCmpAnyGE_u32(a, b) vec_any_ge((__vec128u32)a, (__vec128u32)b)
#define M_VCmpAnyGT_u32(a, b) vec_any_gt((__vec128u32)a, (__vec128u32)b)
#define M_VCmpAnyLE_u32(a, b) vec_any_le((__vec128u32)a, (__vec128u32)b)
#define M_VCmpAnyLT_u32(a, b) vec_any_lt((__vec128u32)a, (__vec128u32)b)

#define M_VCmpAnyEq_u16(a, b) vec_any_eq((__vec128u16)a, (__vec128u16)b)
#define M_VCmpAnyGE_u16(a, b) vec_any_ge((__vec128u16)a, (__vec128u16)b)
#define M_VCmpAnyGT_u16(a, b) vec_any_gt((__vec128u16)a, (__vec128u16)b)
#define M_VCmpAnyLE_u16(a, b) vec_any_le((__vec128u16)a, (__vec128u16)b)
#define M_VCmpAnyLT_u16(a, b) vec_any_lt((__vec128u16)a, (__vec128u16)b)

#define M_VCmpAnyEq_u8(a, b) vec_any_eq((__vec128u8)a, (__vec128u8)b)
#define M_VCmpAnyGE_u8(a, b) vec_any_ge((__vec128u8)a, (__vec128u8)b)
#define M_VCmpAnyGT_u8(a, b) vec_any_gt((__vec128u8)a, (__vec128u8)b)
#define M_VCmpAnyLE_u8(a, b) vec_any_le((__vec128u8)a, (__vec128u8)b)
#define M_VCmpAnyLT_u8(a, b) vec_any_lt((__vec128u8)a, (__vec128u8)b)

#define M_VCmpAnyEq_i32(a, b) vec_any_eq((__vec128i32)a, (__vec128i32)b)
#define M_VCmpAnyGE_i32(a, b) vec_any_ge((__vec128i32)a, (__vec128i32)b)
#define M_VCmpAnyGT_i32(a, b) vec_any_gt((__vec128i32)a, (__vec128i32)b)
#define M_VCmpAnyLE_i32(a, b) vec_any_le((__vec128i32)a, (__vec128i32)b)
#define M_VCmpAnyLT_i32(a, b) vec_any_lt((__vec128i32)a, (__vec128i32)b)

#define M_VCmpAnyEq_i16(a, b) vec_any_eq((__vec128i16)a, (__vec128i16)b)
#define M_VCmpAnyGE_i16(a, b) vec_any_ge((__vec128i16)a, (__vec128i16)b)
#define M_VCmpAnyGT_i16(a, b) vec_any_gt((__vec128i16)a, (__vec128i16)b)
#define M_VCmpAnyLE_i16(a, b) vec_any_le((__vec128i16)a, (__vec128i16)b)
#define M_VCmpAnyLT_i16(a, b) vec_any_lt((__vec128i16)a, (__vec128i16)b)

#define M_VCmpAnyEq_i8(a, b) vec_any_eq((__vec128i8)a, (__vec128i8)b)
#define M_VCmpAnyGE_i8(a, b) vec_any_ge((__vec128i8)a, (__vec128i8)b)
#define M_VCmpAnyGT_i8(a, b) vec_any_gt((__vec128i8)a, (__vec128i8)b)
#define M_VCmpAnyLE_i8(a, b) vec_any_le((__vec128i8)a, (__vec128i8)b)
#define M_VCmpAnyLT_i8(a, b) vec_any_lt((__vec128i8)a, (__vec128i8)b)


#define M_VCmpEqMsk_u32(a, b) (vec128)vec_vcmpequw((__vec128u32)a, (__vec128u32)b)
#define M_VCmpGTMsk_u32(a, b) (vec128)vec_vcmpgtuw((__vec128u32)a, (__vec128u32)b)
#define M_VCmpLTMsk_u32(a, b) (vec128)vec_vcmpgtuw((__vec128u32)b, (__vec128u32)a)
#define M_VCmpGEMsk_u32(a, b) (vec128)vec_vcmpgeuw((__vec128u32)a, (__vec128u32)b)
#define M_VCmpLEMsk_u32(a, b) (vec128)vec_vcmpgeuw((__vec128u32)b, (__vec128u32)a)

#define M_VCmpEqMsk_u16(a, b) (vec128)vec_vcmpequh((__vec128u16)a, (__vec128u16)b)
#define M_VCmpGTMsk_u16(a, b) (vec128)vec_vcmpgtuh((__vec128u16)a, (__vec128u16)b)
#define M_VCmpLTMsk_u16(a, b) (vec128)vec_vcmpgtuh((__vec128u16)b, (__vec128u16)a)
#define M_VCmpGEMsk_u16(a, b) (vec128)vec_vcmpgeuh((__vec128u16)a, (__vec128u16)b)
#define M_VCmpLEMsk_u16(a, b) (vec128)vec_vcmpgeuh((__vec128u16)b, (__vec128u16)a)

#define M_VCmpEqMsk_u8(a, b) (vec128)vec_vcmpequb((__vec128u8)a, (__vec128u8)b)
#define M_VCmpGTMsk_u8(a, b) (vec128)vec_vcmpgtub((__vec128u8)a, (__vec128u8)b)
#define M_VCmpLTMsk_u8(a, b) (vec128)vec_vcmpgtub((__vec128u8)b, (__vec128u8)a)
#define M_VCmpGEMsk_u8(a, b) (vec128)vec_vcmpgeub((__vec128u8)a, (__vec128u8)b)
#define M_VCmpLEMsk_u8(a, b) (vec128)vec_vcmpgeub((__vec128u8)b, (__vec128u8)a)

#define M_VCmpEqMsk_i32(a, b) (vec128)vec_vcmpeqsw((__vec128i32)a, (__vec128i32)b)
#define M_VCmpGTMsk_i32(a, b) (vec128)vec_vcmpgtsw((__vec128i32)a, (__vec128i32)b)
#define M_VCmpLTMsk_i32(a, b) (vec128)vec_vcmpgtsw((__vec128i32)b, (__vec128i32)a)
#define M_VCmpGEMsk_i32(a, b) (vec128)vec_vcmpgesw((__vec128i32)a, (__vec128i32)b)
#define M_VCmpLEMsk_i32(a, b) (vec128)vec_vcmpgesw((__vec128i32)b, (__vec128i32)a)

#define M_VCmpEqMsk_i16(a, b) (vec128)vec_vcmpeqsh((__vec128i16)a, (__vec128i16)b)
#define M_VCmpGTMsk_i16(a, b) (vec128)vec_vcmpgtsh((__vec128i16)a, (__vec128i16)b)
#define M_VCmpLTMsk_i16(a, b) (vec128)vec_vcmpgtsh((__vec128i16)b, (__vec128i16)a)
#define M_VCmpGEMsk_i16(a, b) (vec128)vec_vcmpgesh((__vec128i16)a, (__vec128i16)b)
#define M_VCmpLEMsk_i16(a, b) (vec128)vec_vcmpgesh((__vec128i16)b, (__vec128i16)a)

#define M_VCmpEqMsk_i8(a, b) (vec128)vec_vcmpeqsb((__vec128i8)a, (__vec128i8)b)
#define M_VCmpGTMsk_i8(a, b) (vec128)vec_vcmpgtsb((__vec128i8)a, (__vec128i8)b)
#define M_VCmpLTMsk_i8(a, b) (vec128)vec_vcmpgtsb((__vec128i8)b, (__vec128i8)a)
#define M_VCmpGEMsk_i8(a, b) (vec128)vec_vcmpgesb((__vec128i8)a, (__vec128i8)b)
#define M_VCmpLEMsk_i8(a, b) (vec128)vec_vcmpgesb((__vec128i8)b, (__vec128i8)a)

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

// -------------------------------------------------------------------
M_FORCEINLINE vec128 M_VCmpEqMsk_u32(vec128 _a, vec128 _b) { return __vcmpequw(_a, _b); }								// Equal
//M_FORCEINLINE vec128 M_VCmpGEMsk_u32(vec128 _a, vec128 _b) { return __vor(__vcmpequw(_a, _b), __vcmpgtuw(_a, _b)); }	// Greater-Equal
M_FORCEINLINE vec128 M_VCmpGTMsk_u32(vec128 _a, vec128 _b) { return __vcmpgtuw(_a, _b); }								// Greater Than
//M_FORCEINLINE vec128 M_VCmpLEMsk_u32(vec128 _a, vec128 _b) { return __vor(__vcmpequw(_a, _b), __vcmpgtuw(_b, _a)); }	// Lesser-Equal
M_FORCEINLINE vec128 M_VCmpLTMsk_u32(vec128 _a, vec128 _b) { return __vcmpgtuw(_b, _a); }								// Lesser-Than

M_FORCEINLINE vec128 M_VCmpEqMsk_u16(vec128 _a, vec128 _b) { return __vcmpequh(_a, _b); }								// Equal
//M_FORCEINLINE vec128 M_VCmpGEMsk_u16(vec128 _a, vec128 _b) { return __vor(__vcmpequh(_a, _b), __vcmpgtuh(_a, _b)); }	// Greater-Equal
M_FORCEINLINE vec128 M_VCmpGTMsk_u16(vec128 _a, vec128 _b) { return __vcmpgtuh(_a, _b); }								// Greater Than
//M_FORCEINLINE vec128 M_VCmpLEMsk_u16(vec128 _a, vec128 _b) { return __vor(__vcmpequh(_a, _b), __vcmpgtuh(_b, _a)); }	// Lesser-Equal
M_FORCEINLINE vec128 M_VCmpLTMsk_u16(vec128 _a, vec128 _b) { return __vcmpgtuh(_b, _a); }								// Lesser-Than

M_FORCEINLINE vec128 M_VCmpEqMsk_u8(vec128 _a, vec128 _b) { return __vcmpequb(_a, _b); }								// Equal
//M_FORCEINLINE vec128 M_VCmpGEMsk_u8(vec128 _a, vec128 _b) { return __vor(__vcmpequb(_a, _b), __vcmpgtub(_a, _b)); }		// Greater-Equal
M_FORCEINLINE vec128 M_VCmpGTMsk_u8(vec128 _a, vec128 _b) { return __vcmpgtub(_a, _b); }								// Greater Than
//M_FORCEINLINE vec128 M_VCmpLEMsk_u8(vec128 _a, vec128 _b) { return __vor(__vcmpequb(_a, _b), __vcmpgtub(_b, _a)); }		// Lesser-Equal
M_FORCEINLINE vec128 M_VCmpLTMsk_u8(vec128 _a, vec128 _b) { return __vcmpgtub(_b, _a); }								// Lesser-Than

M_FORCEINLINE vec128 M_VCmpEqMsk_i32(vec128 _a, vec128 _b) { return __vcmpequw(_a, _b); }								// Equal
//M_FORCEINLINE vec128 M_VCmpGEMsk_i32(vec128 _a, vec128 _b) { return __vor(__vcmpeqsw(_a, _b), __vcmpgtsw(_a, _b)); }	// Greater-Equal
M_FORCEINLINE vec128 M_VCmpGTMsk_i32(vec128 _a, vec128 _b) { return __vcmpgtsw(_a, _b); }								// Greater Than
//M_FORCEINLINE vec128 M_VCmpLEMsk_i32(vec128 _a, vec128 _b) { return __vor(__vcmpeqsw(_a, _b), __vcmpgtsw(_b, _a)); }	// Lesser-Equal
M_FORCEINLINE vec128 M_VCmpLTMsk_i32(vec128 _a, vec128 _b) { return __vcmpgtsw(_b, _a); }								// Lesser-Than

M_FORCEINLINE vec128 M_VCmpEqMsk_i16(vec128 _a, vec128 _b) { return __vcmpequh(_a, _b); }								// Equal
//M_FORCEINLINE vec128 M_VCmpGEMsk_i16(vec128 _a, vec128 _b) { return __vor(__vcmpeqsh(_a, _b), __vcmpgtsh(_a, _b)); }	// Greater-Equal
M_FORCEINLINE vec128 M_VCmpGTMsk_i16(vec128 _a, vec128 _b) { return __vcmpgtsh(_a, _b); }								// Greater Than
//M_FORCEINLINE vec128 M_VCmpLEMsk_i16(vec128 _a, vec128 _b) { return __vor(__vcmpeqsh(_a, _b), __vcmpgtsh(_b, _a)); }	// Lesser-Equal
M_FORCEINLINE vec128 M_VCmpLTMsk_i16(vec128 _a, vec128 _b) { return __vcmpgtsh(_b, _a); }								// Lesser-Than

M_FORCEINLINE vec128 M_VCmpEqMsk_i8(vec128 _a, vec128 _b) { return __vcmpequb(_a, _b); }								// Equal
//M_FORCEINLINE vec128 M_VCmpGEMsk_i8(vec128 _a, vec128 _b) { return __vor(__vcmpeqsb(_a, _b), __vcmpgtsb(_a, _b)); }		// Greater-Equal
M_FORCEINLINE vec128 M_VCmpGTMsk_i8(vec128 _a, vec128 _b) { return __vcmpgtsb(_a, _b); }								// Greater Than
//M_FORCEINLINE vec128 M_VCmpLEMsk_i8(vec128 _a, vec128 _b) { return __vor(__vcmpeqsb(_a, _b), __vcmpgtsb(_b, _a)); }		// Lesser-Equal
M_FORCEINLINE vec128 M_VCmpLTMsk_i8(vec128 _a, vec128 _b) { return __vcmpgtsb(_b, _a); }								// Lesser-Than

// -------------------------------------------------------------------
M_FORCEINLINE unsigned int M_VCmpAnyEq_u32(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpequwR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyGT_u32(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtuwR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyLT_u32(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtuwR(_b, _a, &CR);
	return (CR & 32) ? 0 : 1;
}

// -------------------------------------------------------------------
M_FORCEINLINE unsigned int M_VCmpAnyEq_i32(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpequwR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyGT_i32(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtswR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyLT_i32(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtswR(_b, _a, &CR);
	return (CR & 32) ? 0 : 1;
}

// -------------------------------------------------------------------
M_FORCEINLINE unsigned int M_VCmpAnyEq_u16(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpequhR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyGT_u16(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtuhR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyLT_u16(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtuhR(_b, _a, &CR);
	return (CR & 32) ? 0 : 1;
}

// -------------------------------------------------------------------
M_FORCEINLINE unsigned int M_VCmpAnyEq_i16(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpequhR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyGT_i16(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtshR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyLT_i16(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtshR(_b, _a, &CR);
	return (CR & 32) ? 0 : 1;
}

// -------------------------------------------------------------------
M_FORCEINLINE unsigned int M_VCmpAnyEq_u8(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpequbR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyGT_u8(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtubR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyLT_u8(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtubR(_b, _a, &CR);
	return (CR & 32) ? 0 : 1;
}

// -------------------------------------------------------------------
M_FORCEINLINE unsigned int M_VCmpAnyEq_i8(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpequbR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyGT_i8(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtsbR(_a, _b, &CR);
	return (CR & 32) ? 0 : 1;
}

M_FORCEINLINE unsigned int M_VCmpAnyLT_i8(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtsbR(_b, _a, &CR);
	return (CR & 32) ? 0 : 1;
}

// -------------------------------------------------------------------
// -------------------------------------------------------------------
// -------------------------------------------------------------------
M_FORCEINLINE unsigned int M_VCmpAllEq_u32(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpequwR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllGT_u32(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtuwR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllLT_u32(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtuwR(_b, _a, &CR);
	return (CR & 128) ? 1 : 0;
}

// -------------------------------------------------------------------
M_FORCEINLINE unsigned int M_VCmpAllEq_i32(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpequwR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllGT_i32(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtswR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllLT_i32(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtswR(_b, _a, &CR);
	return (CR & 128) ? 1 : 0;
}

// -------------------------------------------------------------------
M_FORCEINLINE unsigned int M_VCmpAllEq_u16(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpequhR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllGT_u16(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtuhR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllLT_u16(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtuhR(_b, _a, &CR);
	return (CR & 128) ? 1 : 0;
}

// -------------------------------------------------------------------
M_FORCEINLINE unsigned int M_VCmpAllEq_i16(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpequhR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllGT_i16(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtshR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllLT_i16(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtshR(_b, _a, &CR);
	return (CR & 128) ? 1 : 0;
}

// -------------------------------------------------------------------
M_FORCEINLINE unsigned int M_VCmpAllEq_u8(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpequbR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllGT_u8(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtubR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllLT_u8(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtubR(_b, _a, &CR);
	return (CR & 128) ? 1 : 0;
}

// -------------------------------------------------------------------
M_FORCEINLINE unsigned int M_VCmpAllEq_i8(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpequbR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllGT_i8(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtsbR(_a, _b, &CR);
	return (CR & 128) ? 1 : 0;
}

M_FORCEINLINE unsigned int M_VCmpAllLT_i8(vec128 _a, vec128 _b)
{
	unsigned int CR;
	__vcmpgtsbR(_b, _a, &CR);
	return (CR & 128) ? 1 : 0;
}

#endif // Xenon
// -------------------------------------------------------------------
#endif // defined(CPU_POWERPC) && !defined(CPU_VEC128EMU)
#endif // __INCLUDE_XCC_MATH_VEC128_VMX
