#ifndef __INCLUDE_XCC_MATH_VEC128_SPU
#define __INCLUDE_XCC_MATH_VEC128_SPU


#if defined(PLATFORM_SPU)


//----------------------------------------------------------
// Performance hints for compound arithmetics implementations
#define M_V128HINT_NATIVE_MADD
//#define M_V128HINT_NATIVE_DP3
//#define M_V128HINT_NATIVE_DP4

//----------------------------------------------------------
// Flag everything we implement
#define M_V128IMP_LOAD
#define M_V128IMP_LOAD_SCALAR_F32
#define M_V128IMP_SHUFFLE
#define M_V128IMP_MERGE
#define M_V128IMP_VSPLATXYZW
#define M_V128IMP_FLOAT_ARITHMETICS
#define M_V128IMP_FLOAT_RCP_SQRT_EST
//#define M_V128IMP_FLOAT_RCP_SQRT
//#define M_V128IMP_FLOAT_CMPMASK
#define M_V128IMP_LOGICOP
//#define M_V128IMP_COMPARE
#define M_V128IMP_FLOAT_DOTPROD
#define M_V128IMP_FLOAT_XPROD

//#define M_V128IMP_TRANSPOSE4X4
//#define M_V128IMP_INTEGERARITHMETICS
//#define M_V128IMP_INTEGERCOMPARE

#define M_V128IMP_FLOAT_OPERATORS

/*
class CMat43fp32
{ 
public: 
	union
	{
		vec128 v[3];
		fp32 k[4][3]; 
	};
};

class CMat4Dfp32 
{
public:
	union
	{
		vec128 r[4];
		fp32 k[4][4];  // row, column
	};

};

class CStr
{

};

CStr CStrF(const char*, ...);


template <typename T> 
inline T Max(T a, T b)
{
	return ((a > b) ? a : b);
};

template <typename T> 
inline T Min(T a, T b)
{
	return ((a < b) ? a : b);
};

class CMat4Dfp32 
{
public:
	union
	{
		vec128 r[4];
		fp32 k[4][4];  // row, column
	};

};

class CVec4Dfp32 
{ 
public: 
	CVec4Dfp32(vec128 _v) { v=_v; }
	union {
		vec128 v;
		fp32 k[4]; 
	};
};



class CMat43fp32
{ 
public: 
	union
	{
		vec128 v[3];
		fp32 k[4][3]; 
	};
};

class CMat4Dfp32 
{
public:
	union
	{
		vec128 r[4];
		fp32 k[4][4];  // row, column
	};

};

class CStr
{

};

CStr CStrF(const char*, ...);


template <typename T> 
inline T Max(T a, T b)
{
	return ((a > b) ? a : b);
};

template <typename T> 
inline T Min(T a, T b)
{
	return ((a < b) ? a : b);
};
*/

typedef vec128 vec128p;
typedef CMat4Dfp32 CMat4Dfp32p;

typedef vector signed char int8vec128;
typedef vector unsigned char uint8vec128;
typedef vector signed short int16vec128;
typedef vector unsigned short uint16vec128;
typedef vector signed int int32vec128;
typedef vector unsigned int uint32vec128;

#include <spu_intrinsics.h>

// Constants for shuffles, words are labeled [x,y,z,w] [a,b,c,d]
#define _VECTORMATH_SHUF_X 0x00010203
#define _VECTORMATH_SHUF_Y 0x04050607
#define _VECTORMATH_SHUF_Z 0x08090a0b
#define _VECTORMATH_SHUF_W 0x0c0d0e0f
#define _VECTORMATH_SHUF_A 0x10111213
#define _VECTORMATH_SHUF_B 0x14151617
#define _VECTORMATH_SHUF_C 0x18191a1b
#define _VECTORMATH_SHUF_D 0x1c1d1e1f
#define _VECTORMATH_SHUF_0 0x80808080
#define _VECTORMATH_SHUF_XYZA (vec_uchar16)(vec_uint4){ _VECTORMATH_SHUF_X, _VECTORMATH_SHUF_Y, _VECTORMATH_SHUF_Z, _VECTORMATH_SHUF_A }
#define _VECTORMATH_SHUF_ZXYW (vec_uchar16)(vec_uint4){ _VECTORMATH_SHUF_Z, _VECTORMATH_SHUF_X, _VECTORMATH_SHUF_Y, _VECTORMATH_SHUF_W }
#define _VECTORMATH_SHUF_YZXW (vec_uchar16)(vec_uint4){ _VECTORMATH_SHUF_Y, _VECTORMATH_SHUF_Z, _VECTORMATH_SHUF_X, _VECTORMATH_SHUF_W }
#define _VECTORMATH_SHUF_WABC (vec_uchar16)(vec_uint4){ _VECTORMATH_SHUF_W, _VECTORMATH_SHUF_A, _VECTORMATH_SHUF_B, _VECTORMATH_SHUF_C }
#define _VECTORMATH_SHUF_ZWAB (vec_uchar16)(vec_uint4){ _VECTORMATH_SHUF_Z, _VECTORMATH_SHUF_W, _VECTORMATH_SHUF_A, _VECTORMATH_SHUF_B }
#define _VECTORMATH_SHUF_XYZA (vec_uchar16)(vec_uint4){ _VECTORMATH_SHUF_X, _VECTORMATH_SHUF_Y, _VECTORMATH_SHUF_Z, _VECTORMATH_SHUF_A }
#define _VECTORMATH_SHUF_YZAB (vec_uchar16)(vec_uint4){ _VECTORMATH_SHUF_Y, _VECTORMATH_SHUF_Z, _VECTORMATH_SHUF_A, _VECTORMATH_SHUF_B }
#define _VECTORMATH_SHUF_ZABC (vec_uchar16)(vec_uint4){ _VECTORMATH_SHUF_Z, _VECTORMATH_SHUF_A, _VECTORMATH_SHUF_B, _VECTORMATH_SHUF_C }

#define _VECTORMATH_SHUF_XAYB (vec_uchar16)(vec_uint4){ _VECTORMATH_SHUF_X, _VECTORMATH_SHUF_A, _VECTORMATH_SHUF_Y, _VECTORMATH_SHUF_B }
#define _VECTORMATH_SHUF_ZCWD (vec_uchar16)(vec_uint4){ _VECTORMATH_SHUF_Z, _VECTORMATH_SHUF_C, _VECTORMATH_SHUF_W, _VECTORMATH_SHUF_D }

//		Load
M_FORCEINLINE vec128 M_VZero() 							{ return spu_splats(0.0f); }
M_FORCEINLINE vec128 M_VHalf() 							{ return spu_splats(0.5f); }
M_FORCEINLINE vec128 M_VOne()  							{ return spu_splats(1.0f); }
M_FORCEINLINE vec128 M_VTwo()  							{ return spu_splats(2.0f); }

M_FORCEINLINE vec128 M_VLdScalar(fp32 _Src)					{ return vec128(spu_splats(_Src)); }
M_FORCEINLINE vec128 M_VLdScalar_u8(const uint8& _Src)		{ return vec128(spu_splats(_Src)); }
M_FORCEINLINE vec128 M_VLdScalar_i8(const uint8& _Src)		{ return vec128(spu_splats(_Src)); }
M_FORCEINLINE vec128 M_VLdScalar_u16(const uint16& _Src)	{ return vec128(spu_splats(_Src)); }
M_FORCEINLINE vec128 M_VLdScalar_i16(const int16& _Src)		{ return vec128(spu_splats(_Src)); }
M_FORCEINLINE vec128 M_VLdScalar_u32(const uint32& _Src)	{ return vec128(spu_splats(_Src)); }
M_FORCEINLINE vec128 M_VLdScalar_i32(const int32& _Src)		{ return vec128(spu_splats(_Src)); }
M_FORCEINLINE vec128 M_VLd(fp32 x, fp32 y, fp32 z, fp32 w)	{ return ((vec128){ x, y, z, w }); }
M_FORCEINLINE vec128 M_VLd(const CVec4Dfp32& _a)			{ return  _a.v; }
M_FORCEINLINE vec128 M_VLd(const void* _pSrc)				{ return vec128(si_lqd(int8vec128(si_from_ptr(_pSrc)),0)); }
M_FORCEINLINE vec128 M_VLdU(const void* _pSrc)	
{ 
	mint ShiftAmount = (mint)_pSrc & 15;
	
	vec128 Temp0 = vec128(si_lqd(int8vec128(si_from_ptr(_pSrc)),0));
	vec128 Temp1 = vec128(si_lqd(int8vec128(si_from_ptr(_pSrc)),16));
	Temp0 = (vec128)spu_slqwbyte(Temp0, ShiftAmount);
	Temp1 = (vec128)spu_rlqwbyte(Temp1, ShiftAmount);
	Temp1 = (vec128)spu_slqwbyte(Temp1, 16 - ShiftAmount);
	Temp1 = (vec128)spu_rlqwbyte(Temp1, ShiftAmount);
	return  (vec128)spu_or(Temp0, Temp1);
}


//		Load Scalar to fp32
M_FORCEINLINE vec128 M_VLdScalar_i8_f32(const uint8& _Src)		{ return spu_splats(fp32(_Src)); }
M_FORCEINLINE vec128 M_VLdScalar_u8_f32(const uint8& _Src)		{ return spu_splats(fp32(_Src)); }
M_FORCEINLINE vec128 M_VLdScalar_i16_f32(const int16& _Src)		{ return spu_splats(fp32(_Src)); }
M_FORCEINLINE vec128 M_VLdScalar_u16_f32(const uint16& _Src)	{ return spu_splats(fp32(_Src)); }
M_FORCEINLINE vec128 M_VLdScalar_i32_f32(const int32& _Src)		{ return spu_splats(fp32(_Src)); }
//M_FORCEINLINE vec128 M_VLdScalar_u32_f32(const uint32& _Src)	{ return spu_splats(fp32(_Src)); }


//		Store
M_FORCEINLINE void M_VSt(vec128 _a, void* _pDest)		{ si_stqa(int8vec128(_a),mint(_pDest)); }
M_FORCEINLINE void M_VStAny32(vec128 _a, void* _pDest) { *((fp32*)_pDest) = spu_extract(_a, 0); }
M_FORCEINLINE void M_VStAny32Ex(vec128 _a, void* _pDest, mint _Ofs) { *((fp32*)((mint)_pDest + _Ofs)) = spu_extract(_a, 0); }

//M_FORCEINLINE fp32 M_VGetX_Slow(vec128 a) { return spu_extract(a,0); }
//M_FORCEINLINE fp32 M_VGetY_Slow(vec128 a) { return spu_extract(a,1); }
//M_FORCEINLINE fp32 M_VGetZ_Slow(vec128 a) { return spu_extract(a,2); }
//M_FORCEINLINE fp32 M_VGetW_Slow(vec128 a) { return spu_extract(a,3); }

//		Shuffle:
#define M_VShuf(_a, _Mask) \
	spu_shuffle(_a,_a,(vec_uchar16)(vec_uint4){\
((_Mask &   3)>>0==0 ? _VECTORMATH_SHUF_X:0 ) | ((_Mask &   3)>>0==1 ? _VECTORMATH_SHUF_Y:0) | ((_Mask &   3)>>0==2 ? _VECTORMATH_SHUF_Z:0) | ((_Mask &   3)>>0==3 ? _VECTORMATH_SHUF_W:0),\
((_Mask &  12)>>2==0 ? _VECTORMATH_SHUF_X:0 ) | ((_Mask &  12)>>2==1 ? _VECTORMATH_SHUF_Y:0) | ((_Mask &  12)>>2==2 ? _VECTORMATH_SHUF_Z:0) | ((_Mask &  12)>>2==3 ? _VECTORMATH_SHUF_W:0),\
((_Mask &  48)>>4==0 ? _VECTORMATH_SHUF_X:0 ) | ((_Mask &  48)>>4==1 ? _VECTORMATH_SHUF_Y:0) | ((_Mask &  48)>>4==2 ? _VECTORMATH_SHUF_Z:0) | ((_Mask &  48)>>4==3 ? _VECTORMATH_SHUF_W:0),\
((_Mask & 192)>>6==0 ? _VECTORMATH_SHUF_X:0 ) | ((_Mask & 192)>>6==1 ? _VECTORMATH_SHUF_Y:0) | ((_Mask & 192)>>6==2 ? _VECTORMATH_SHUF_Z:0) | ((_Mask & 192)>>6==3 ? _VECTORMATH_SHUF_W:0)\
})


M_FORCEINLINE vec128 M_VSplat(vec128 a,uint32 iComp) { return spu_splats(spu_extract(a,iComp)); }
M_FORCEINLINE vec128 M_VSplatX(vec128 a) { return spu_splats(spu_extract(a,0)); }
M_FORCEINLINE vec128 M_VSplatY(vec128 a) { return spu_splats(spu_extract(a,1)); }
M_FORCEINLINE vec128 M_VSplatZ(vec128 a) { return spu_splats(spu_extract(a,2)); }
M_FORCEINLINE vec128 M_VSplatW(vec128 a) { return spu_splats(spu_extract(a,3)); }
M_FORCEINLINE vec128 M_VMrgXY(vec128 a, vec128 b) { return spu_shuffle(a,b,_VECTORMATH_SHUF_XAYB); }		//{ a.x, b.x, a.y, b.y }
M_FORCEINLINE vec128 M_VMrgZW(vec128 a, vec128 b) { return spu_shuffle(a,b,_VECTORMATH_SHUF_ZCWD); }		//{ a.z, b.z, a.w, b.w }

M_FORCEINLINE vec128 M_VShl8_u128(vec128 a, int _nBytes) { return spu_slqwbyte(a,_nBytes); }		//{ a.z, b.z, a.w, b.w }


//		Conversion:
//			vec128 M_VTrunc(vec128 _a);											{ (float)Trunc(a.x), (float)Trunc(a.y), (float)Trunc(a.z), (float)Trunc(a.w) }
M_FORCEINLINE vec128 M_VCnvL_i16_i32(vec128 _a) 
{ 
	return (vec128)spu_extend(((int16vec128)spu_shuffle(_a,_a,(vec_uchar16)(vec_uint4){ 0x00000001, 0x00000203, 0x00000405, 0x00000607 }))); 
}
M_FORCEINLINE vec128 M_VCnvH_i16_i32(vec128 _a)
{ 
	return (vec128)spu_extend(((int16vec128)spu_shuffle(_a,_a,(vec_uchar16)(vec_uint4){ 0x00000809, 0x00000a0b, 0x00000c0d, 0x00000e0f }))); 
}

M_FORCEINLINE vec128 M_VCnvL_u16_u32(vec128 _a) 
{ 
	return (vec128)spu_shuffle(_a,_a,(vec_uchar16)(vec_uint4){ 0x00000001, 0x00000203, 0x00000405, 0x00000607 }); 
}
M_FORCEINLINE vec128 M_VCnvH_u16_u32(vec128 _a)
{ 
	return (vec128)spu_shuffle(_a,_a,(vec_uchar16)(vec_uint4){ 0x00000809, 0x00000a0b, 0x00000c0d, 0x00000e0f }); 
}



template <int t_0, int t_1, int t_2, int t_3>
M_FORCEINLINE vec128 M_VPerm_Helper(vec128 _a, vec128 _b) 
{ 
	return spu_shuffle(_a, _b, (vec_uchar16){ 
		t_0 * 4 + 0, t_0 * 4 + 1, t_0 * 4 + 2, t_0 * 4 + 3,
		t_1 * 4 + 0, t_1 * 4 + 1, t_1 * 4 + 2, t_1 * 4 + 3,
		t_2 * 4 + 0, t_2 * 4 + 1, t_2 * 4 + 2, t_2 * 4 + 3,
		t_3 * 4 + 0, t_3 * 4 + 1, t_3 * 4 + 2, t_3 * 4 + 3
	});
}

#define M_VPerm(_a, _b, _0, _1, _2, _3) M_VPerm_Helper<_0, _1, _2, _3>(_a, _b)

/*
M_VStAny32Ex;
M_VLdU;
M_VPerm;
*/

M_FORCEINLINE vec128 M_VTrunc(vec128 _a)
{
	return spu_convtf((int32vec128)spu_convts(_a, 0), 0);
}
//			vec128 M_Vfp32toi32(vec128 _a)										{ (int)a.x, (int)a.y, (int)a.z, (int)a.w }, float to int32, truncate
M_FORCEINLINE vec128 M_VCnv_f32_i32(vec128 _a)
{
	return (vec128)spu_convts(_a, 0);
}
//			vec128 M_Vi32tofp32(vec128 _a)										{ (float)a.x, (float)a.y, (float)a.z, (float)a.w }
M_FORCEINLINE vec128 M_VCnv_i32_f32(vec128 _a)
{
	return spu_convtf((int32vec128)_a, 0);
}

//		Logical operations:
M_FORCEINLINE vec128 M_VOr(vec128 a, vec128 b) { return spu_or(a, b); }
M_FORCEINLINE vec128 M_VAnd(vec128 a, vec128 b) { return spu_and(a, b); }
M_FORCEINLINE vec128 M_VAndNot(vec128 a, vec128 b) { return spu_andc(a, b); }
M_FORCEINLINE vec128 M_VXor(vec128 a, vec128 b) { return spu_xor(a, b); }
M_FORCEINLINE vec128 M_VNot(vec128 a) { return spu_nor(a, a); }

//		Floating point compare
/*
M_FORCEINLINE vec128 M_VCmpEqMsk(vec128 a, vec128 b) { return vec128(spu_cmpeq(a,b)); }
M_FORCEINLINE vec128 M_VCmpGEMsk(vec128 a, vec128 b) { return vec128(spu_or(spu_cmpeq(a,b),spu_cmpgt(a,b))); }
M_FORCEINLINE vec128 M_VCmpGTMsk(vec128 a, vec128 b) { return vec128(spu_cmpgt(a,b)); }
M_FORCEINLINE vec128 M_VCmpLEMsk(vec128 a, vec128 b) { return vec128(spu_or(spu_cmpeq(a,b),spu_cmpgt(b,a))); }
M_FORCEINLINE vec128 M_VCmpLTMsk(vec128 a, vec128 b) { return vec128(spu_cmpgt(b,a)); }

M_FORCEINLINE uint32 M_VCmpAllEq(vec128 a, vec128 b)
{
	vec128 x=M_VCmpEqMsk(a,b);
	x=spu_nor(x,x);
	return 0x0==spu_extract(spu_orx(uint32vec128(x)),0);
}
*/
//			uint M_VCmpAllGE(vec128 a, vec128 b);								(a.x >= b.x) && (a.y >= b.y) && (a.z >= b.z) && (a.w >= b.w)
//			uint M_VCmpAllGT(vec128 a, vec128 b);								(a.x >  b.x) && (a.y >  b.y) && (a.z >  b.z) && (a.w >  b.w)
//			uint M_VCmpAllLE(vec128 a, vec128 b);								(a.x <= b.x) && (a.y <= b.y) && (a.z <= b.z) && (a.w <= b.w)
//			uint M_VCmpAllLT(vec128 a, vec128 b);								(a.x <  b.x) && (a.y <  b.y) && (a.z <  b.z) && (a.w <  b.w)

//			uint M_VCmpAnyEq(vec128 a, vec128 b);								(a.x == b.x) || (a.y == b.y) || (a.z == b.z) || (a.w == b.w)
//			uint M_VCmpAnyGE(vec128 a, vec128 b);								(a.x >= b.x) || (a.y >= b.y) || (a.z >= b.z) || (a.w >= b.w)
//			uint M_VCmpAnyGT(vec128 a, vec128 b);								(a.x >  b.x) || (a.y >  b.y) || (a.z >  b.z) || (a.w >  b.w)
//			uint M_VCmpAnyLE(vec128 a, vec128 b);								(a.x <= b.x) || (a.y <= b.y) || (a.z <= b.z) || (a.w <= b.w)
//			uint M_VCmpAnyLT(vec128 a, vec128 b);								(a.x <  b.x) || (a.y <  b.y) || (a.z <  b.z) || (a.w <  b.w)

//			vec128 M_VSelMsk(vec128 msk, vec128 a, vec128 b);					M_VOr(M_VAnd(a, msk), M_VAnd(a, M_VNot(msk)));
//			vec128 M_VSelMskRev(mskk, a, b)										M_VSelMsk(msk, b, a);
//			vec128 M_VSel(vec128 comp, vec128 a, vec128 b);						{ (comp.x >= 0) ? a.x : b.x, (comp.y >= 0) ? a.y : b.y, (comp.z >= 0) ? a.z : b.z, (comp.w >= 0) ? a.w : b.w }

//			uint operator== (vec128 _a, vec128 _b)								M_VCmpAllEq(_a, _b)
//			uint operator>= (vec128 _a, vec128 _b)								M_VCmpAllGE(_a, _b)
//			uint operator> (vec128 _a, vec128 _b)								M_VCmpAllGT(_a, _b)
//			uint operator<= (vec128 _a, vec128 _b)								M_VCmpAllLE(_a, _b)
//			uint operator< (vec128 _a, vec128 _b)								M_VCmpAllLT(_a, _b)

//		Floating point core arithmetics:
M_FORCEINLINE vec128 M_VNeg(vec128 _a)						{ return spu_sub(M_VZero(), _a); }
M_FORCEINLINE vec128 M_VAdd(vec128 a, vec128 b)				{ return spu_add(a, b); }
M_FORCEINLINE vec128 M_VAddh(vec128 a)						{ return spu_splats(spu_extract(a,0)+spu_extract(a,1)+spu_extract(a,2)+spu_extract(a,3)); }
M_FORCEINLINE vec128 M_VSub(vec128 a, vec128 b)				{ return spu_sub(a, b); }
M_FORCEINLINE vec128 M_VMul(vec128 a, vec128 b)				{ return spu_mul(a, b); }
M_FORCEINLINE vec128 M_VMAdd(vec128 a, vec128 b, vec128 c)	{ return spu_madd(a, b, c); }
M_FORCEINLINE vec128 M_VNMSub(vec128 a, vec128 b, vec128 c)	{ return spu_nmsub(a, b, c); }
M_FORCEINLINE vec128 M_VMin(vec128 a, vec128 b)				{ return spu_sel(a, b, spu_cmpgt(a,b)); }
M_FORCEINLINE vec128 M_VMax(vec128 a, vec128 b)				{ return spu_sel(a, b, spu_cmpgt(b,a)); }

M_FORCEINLINE vec128 M_VXpd(vec128 a, vec128 b)
{
	vec128 tmp0, tmp1, tmp2, tmp3, result;
	tmp0 = spu_shuffle( a, a, _VECTORMATH_SHUF_YZXW );
	tmp1 = spu_shuffle( b, b, _VECTORMATH_SHUF_ZXYW );
	tmp2 = spu_shuffle( a, a, _VECTORMATH_SHUF_ZXYW );
	tmp3 = spu_shuffle( b, b, _VECTORMATH_SHUF_YZXW );
	result = spu_mul( tmp0, tmp1 );
	result = spu_nmsub( tmp2, tmp3, result );
	return result;
}

M_FORCEINLINE vec128 M_VRcp_Est(vec128 a)				{ return spu_re(a); }
M_FORCEINLINE vec128 M_VRsq_Est(vec128 a)				{ return spu_rsqrte(a); }
M_FORCEINLINE vec128 M_VSqrt_Est(vec128 a)				{ return spu_re(spu_rsqrte(a)); }
//			vec128 M_VRcp(vec128 a);											{ 1/a.x, 1/a.y, 1/a.z, 1/a.w }, 7 correct digits
//			vec128 M_VRsq(vec128 a);											{ 1/sqrt(a.x), 1/sqrt(a.y), 1/sqrt(a.z), 1/sqrt(a.w) }, 7 correct digits
//			vec128 M_VSqrt(vec128 a);											{ sqrt(a.x), sqrt(a.y), sqrt(a.z), sqrt(a.w) }, 7 correct digits

//		Floating point compound arithmetics:
//			vec128 M_VAbs(vec128 _a);
//			vec128 M_VNegMsk(vec128 a, vec128 msk);								{ mx? -a.x : a.x, my? -a.y : a.y, mz? -a.z : a.z, mw? -a.w : a.w }, if msk = M_VConstMsk(mx,my,mz,mw)
//			vec128 M_VLrp(vec128 a, vec128 b, vec128 t);						{ a*(1-t) + b*t }
//			vec128 M_VClamp01(vec128 a);										{ clamp01(a.x), clamp01(a.y), clamp01(a.z), clamp01(a.w) }
//			vec128 M_VClamp(vec128 a, vec128 _min, vec128 _max);				Component Min(Max(a, _min), _max)


M_FORCEINLINE vec128 M_VDp3(vec128 a, vec128 b) 
{
	vec128 result;
	result = spu_mul( a, b);
	result = spu_madd( spu_rlqwbyte( a, 4 ), spu_rlqwbyte( b, 4 ), result );
	result = spu_madd( spu_rlqwbyte( a, 8 ), spu_rlqwbyte( b, 8 ), result );
	return spu_splats(spu_extract(result,0));
}

M_FORCEINLINE vec128 M_VDp3x2(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1)
{
	vec128 dp0 = M_VDp3(_a0, _b0);
	vec128 dp1 = M_VDp3(_a1, _b1);
	return M_VMrgXY(dp0, dp1);
}

M_FORCEINLINE vec128 M_VDp3x3(vec128 _a0, vec128 _b0, vec128p _a1, vec128p _b1, vec128p _a2, vec128p _b2)
{
	vec128 p0 = M_VMul(_a0, _b0);
	vec128 p1 = M_VMul(_a1, _b1);
	vec128 p2 = M_VMul(_a2, _b2);
	vec128 p3 = M_VZero();
	M_VTranspose4x4(p0, p1, p2, p3);
	return M_VAdd(M_VAdd(p0, p1), p2);
}

M_FORCEINLINE vec128 M_VDp3x4(vec128 _a0, vec128 _b0, vec128p _a1, vec128p _b1, vec128p _a2, vec128p _b2, vec128p _a3, vec128p _b3)
{
	vec128 p0 = M_VMul(_a0, _b0);
	vec128 p1 = M_VMul(_a1, _b1);
	vec128 p2 = M_VMul(_a2, _b2);
	vec128 p3 = M_VMul(_a3, _b3);
	M_VTranspose4x4(p0, p1, p2, p3);
	return M_VAdd(M_VAdd(p0, p1), p2);
}

M_FORCEINLINE vec128 M_VDp4(vec128 a, vec128 b) 
{
	vec128 result;
	result = spu_mul( a, b );
	result = spu_madd( spu_rlqwbyte( a, 4 ), spu_rlqwbyte( b, 4 ), result );
	result = spu_add( spu_rlqwbyte( result, 8 ), result );
	return spu_splats(spu_extract(result,0));
}

M_FORCEINLINE vec128 M_VDp4x2(vec128 _a0, vec128 _b0, vec128 _a1, vec128 _b1)
{
	vec128 dp0 = M_VDp4(_a0, _b0);
	vec128 dp1 = M_VDp4(_a1, _b1);
	return M_VMrgXY(dp0, dp1);
}

M_FORCEINLINE vec128 M_VDp4x3(vec128 _a0, vec128 _b0, vec128p _a1, vec128p _b1, vec128p _a2, vec128p _b2)
{
	vec128 p0 = M_VMul(_a0, _b0);
	vec128 p1 = M_VMul(_a1, _b1);
	vec128 p2 = M_VMul(_a2, _b2);
	vec128 p3 = M_VZero();
	M_VTranspose4x4(p0, p1, p2, p3);
	return M_VAdd(M_VAdd(p0, p1), M_VAdd(p2, p3));
}

M_FORCEINLINE vec128 M_VDp4x4(vec128 _a0, vec128 _b0, vec128p _a1, vec128p _b1, vec128p _a2, vec128p _b2, vec128p _a3, vec128p _b3)
{
	vec128 p0 = M_VMul(_a0, _b0);
	vec128 p1 = M_VMul(_a1, _b1);
	vec128 p2 = M_VMul(_a2, _b2);
	vec128 p3 = M_VMul(_a3, _b3);
	M_VTranspose4x4(p0, p1, p2, p3);
	return M_VAdd(M_VAdd(p0, p1), M_VAdd(p2, p3));
}

//			vec128 M_VDp4x3(vec128 a0, vec128 b0, vec128 a1, vec128 b1, vec128 a2, vec128 b2);		{ dp4(a0, b0), dp4(a1, b1), dp4(a2, b2), 0 }
//			vec128 M_VDp4x4(vec128 a0, vec128 b0, vec128 a1, vec128 b1, 
//							vec128 a2, vec128 b2, vec128 a3, vec128 b3);							{ dp4(a0, b0), dp4(a1, b1), dp4(a2, b2), dp4(a3, b3) }

//			vec128 M_VLen3(vec128 a);											{ |a|, |a|, |a|, |a| }
//			vec128 M_VLen3x2(vec128 a, vec128 b);								{ |a|, |b|, |a|, |b| }
//			vec128 M_VLen3x4(vec128 a, vec128 b, vec128 c, vec128 d);			{ |a|, |b|, |c|, |d| }
//			vec128 M_VLen4(vec128 a);											{ |a|, |a|, |a|, |a| }
//			vec128 M_VLen4x2(vec128 a, vec128 b);								{ |a|, |b|, |a|, |b| }
//			vec128 M_VLen4x4(vec128 a, vec128 b, vec128 c, vec128 d);			{ |a|, |b|, |c|, |d| }

//			vec128 M_VLenrcp3_Est(vec128 a);									Estimate of { 1/|a|, 1/|a|, 1/|a|, 1/|a| }
//			vec128 M_VLenrcp3x2_Est(vec128 a, vec128 b);						Estimate of { 1/|a|, 1/|b|, 1/|a|, 1/|b| }
//			vec128 M_VLenrcp3x4_Est(vec128 a, vec128 b, vec128 c, vec128 d);	Estimate of { 1/|a|, 1/|b|, 1/|c|, 1/|d| }
//			vec128 M_VLenrcp4_Est(vec128 a);									Estimate of { 1/|a|, 1/|a|, 1/|a|, 1/|a| }
//			vec128 M_VLenrcp4x2_Est(vec128 a, vec128 b);						Estimate of { 1/|a|, 1/|b|, 1/|a|, 1/|b| }
//			vec128 M_VLenrcp4x4_Est(vec128 a, vec128 b, vec128 c, vec128 d);	Estimate of { 1/|a|, 1/|b|, 1/|c|, 1/|d| }

//			vec128 M_VLenrcp3(vec128 a);										{ 1/|a|, 1/|a|, 1/|a|, 1/|a| }
//			vec128 M_VLenrcp3x2(vec128 a, vec128 b);							{ 1/|a|, 1/|b|, 1/|a|, 1/|b| }
//			vec128 M_VLenrcp3x4(vec128 a, vec128 b, vec128 c, vec128 d);		{ 1/|a|, 1/|b|, 1/|c|, 1/|d| }
//			vec128 M_VLenrcp4(vec128 a);										{ 1/|a|, 1/|a|, 1/|a|, 1/|a| }
//			vec128 M_VLenrcp4x2(vec128 a, vec128 b);							{ 1/|a|, 1/|b|, 1/|a|, 1/|b| }
//			vec128 M_VLenrcp4x4(vec128 a, vec128 b, vec128 c, vec128 d);		{ 1/|a|, 1/|b|, 1/|c|, 1/|d| }

//			vec128 M_VNrm3_Est(vec128 a);										Estimate of 3-component normalize, { x, y, z, 0 }
//			void M_VNrm3x2_Est(vec128& a, vec128& b);							Estimate of 3-component normalize * 2, Writes result to input vectors
//			void M_VNrm3x4_Est(vec128& a, vec128& b, vec128& c, vec128& d);		Estimate of 3-component normalize * 4, Writes result to input vectors
//			vec128 M_VNrm4_Est(vec128 a);										Estimate of 4-component normalize, { x, y, z, w }
//			void M_VNrm4x2_Est(vec128& a, vec128& b);							Estimate of 4-component normalize * 2, Writes result to input vectors
//			void M_VNrm4x4_Est(vec128& a, vec128& b, vec128& c, vec128& d);		Estimate of 4-component normalize * 4, Writes result to input vectors

//			vec128 M_VNrm3(vec128 a);											3-component normalize, { x, y, z, 0 }
//			void M_VNrm3x2(vec128& a, vec128& b);								3-component normalize * 2, Writes result to input vectors
//			void M_VNrm3x4(vec128& a, vec128& b, vec128& c, vec128& d);			3-component normalize * 4, Writes result to input vectors
//			vec128 M_VNrm4(vec128 a);											4-component normalize, { x, y, z, w }
//			void M_VNrm4x2(vec128& a, vec128& b);								4-component normalize * 2, Writes result to input vectors
//			void M_VNrm4x4(vec128& a, vec128& b, vec128& c, vec128& d);			4-component normalize * 4, Writes result to input vectors

//			void M_VTranspose4x4(vec128& a, vec128& b, vec128& c, vec128& d);	Vectors a,b,c,d viewed as a 4x4 matrix is transposed
//			void M_VMatMul(const CMat4Dfp32& a, const CMat4Dfp32& b, CMat4Dfp32& c);	Matrix-Matrix multiply, aliasing permitted
//			vec128 M_VMul(vec128 a, CMat4Dfp32 m);								Vector-Matrix multiply
//			vec128 M_VMul4x3(vec128 a, CMat4Dfp32 m);							Vector-Matrix multiply

//			vec128 M_VQuatMul(vec128 a, vec128 b);								Quat(a) * Quat(b)

//		Signed Integer arithmetics:
//			vec128 M_VAdds_i8(vec128 a, vec128 b);								16 * 8bit signed add with saturate, Min(0x7f, a+b)
//			vec128 M_VAdds_i16(vec128 a, vec128 b);								8 * 16bit signed add with saturate, Min(0x7fff, a+b)
//			vec128 M_VSubs_i8(vec128 a, vec128 b);								16 * 8bit signed subtract with saturate, Max(-0x80, a+b)
//			vec128 M_VSubs_i16(vec128 a, vec128 b);								8 * 16bit signed subtract with saturate, Max(-0x8000, a+b)

//			vec128 M_VMin_i8(vec128 a, vec128 b);								16 * 8bit signed minimum, Min(a, b)
//			vec128 M_VMin_i16(vec128 a, vec128 b);								8 * 16bit signed minimum, Min(a, b)
//			vec128 M_VMax_i8(vec128 a, vec128 b);								16 * 8bit signed maximum, Max(a, b)
//			vec128 M_VMax_i16(vec128 a, vec128 b);								8 * 16bit signed maximum, Max(a, b)

//		Unsigned Integer arithmetics:
//			vec128 M_VAdd_u8(vec128 a, vec128 b);								16 * 8bit unsigned add, (a+b) & 0xff
//			vec128 M_VAdds_u8(vec128 a, vec128 b);								16 * 8bit unsigned add with saturate, Min(0xff, a+b)
//			vec128 M_VAdd_u16(vec128 a, vec128 b);								8 * 16bit unsigned add, (a+b) & 0xffff
//			vec128 M_VAdds_u16(vec128 a, vec128 b);								8 * 16bit unsigned add with saturate, Min(0xffff, a+b)
//			vec128 M_VAdd_u32(vec128 a, vec128 b);								4 * 32bit unsigned add, (a+b) & 0xffffffff

//			vec128 M_VSub_u8(vec128 a, vec128 b);								16 * 8bit unsigned subtract, (a-b) & 0xff
//			vec128 M_VSubs_u8(vec128 a, vec128 b);								16 * 8bit unsigned subtract with saturate, Max(0, a-b)
//			vec128 M_VSub_u16(vec128 a, vec128 b);								8 * 16bit unsigned subtract, (a-b) & 0xffff
//			vec128 M_VSubs_u16(vec128 a, vec128 b);								8 * 16bit unsigned subtract with saturate, Max(0, a-b)
//			vec128 M_VSub_u32(vec128 a, vec128 b);								4 * 32bit unsigned subtract, (a-b) & 0xffffffff

//			vec128 M_VMin_u8(vec128 a, vec128 b);								16 * 8bit unsigned minimum, Min(a, b)
//			vec128 M_VMin_u16(vec128 a, vec128 b);								8 * 16bit unsigned minimum, Min(a, b)
//			vec128 M_VMax_u8(vec128 a, vec128 b);								16 * 8bit unsigned maximum, Min(a, b)
//			vec128 M_VMax_u16(vec128 a, vec128 b);								8 * 16bit unsigned maximum, Min(a, b)

//			There is no integer multiply, stop looking.

//		Integer shift: (Not implemented)
//			vec128 M_VSar_i16(vec128 a, uint b);								8 * 16bit algebraic shift right, a >> b
//			vec128 M_VShr_u16(vec128 a, uint b);								8 * 16bit shift right, a >> b
//			vec128 M_VShl_u16(vec128 a, uint b);								8 * 16bit shift left, a << b
//			vec128 M_VSar_i32(vec128 a, uint b);								4 * 32bit algebraic shift right, a >> b
//			vec128 M_VShr_u32(vec128 a, uint b);								4 * 32bit shift right, a >> b
//			vec128 M_VShl_u32(vec128 a, uint b);								4 * 32bit shift left, a << b

//		Integer compare:
//			vec128 M_VCmpEqMsk_u8(vec128 a, vec128 b);							16 * 8bit compare equal
//			vec128 M_VCmpGTMsk_u8(vec128 a, vec128 b);							16 * 8bit unsigned compare greater than
//			vec128 M_VCmpLTMsk_u8(vec128 a, vec128 b);							16 * 8bit unsigned compare less than
//			vec128 M_VCmpEqMsk_i8(vec128 a, vec128 b);							16 * 8bit compare equal (same as M_VCmpEqMsk_u8)
//			vec128 M_VCmpGTMsk_i8(vec128 a, vec128 b);							16 * 8bit signed compare greater than
//			vec128 M_VCmpLTMsk_i8(vec128 a, vec128 b);							16 * 8bit signed compare less than
//			vec128 M_VCmpEqMsk_u16(vec128 a, vec128 b);							8 * 16bit compare equal
//			vec128 M_VCmpGTMsk_u16(vec128 a, vec128 b);							8 * 16bit unsigned compare greater than
//			vec128 M_VCmpLTMsk_u16(vec128 a, vec128 b);							8 * 16bit unsigned compare less than
//			vec128 M_VCmpEqMsk_i16(vec128 a, vec128 b);							8 * 16bit compare equal (same as M_VCmpEqMsk_u16)
//			vec128 M_VCmpGTMsk_i16(vec128 a, vec128 b);							8 * 16bit signed compare greater than
//			vec128 M_VCmpLTMsk_i16(vec128 a, vec128 b);							8 * 16bit signed compare less than
//			vec128 M_VCmpEqMsk_u32(vec128 a, vec128 b);							4 * 32bit compare equal
//			vec128 M_VCmpGTMsk_u32(vec128 a, vec128 b);							4 * 32bit unsigned compare greater than
//			vec128 M_VCmpLTMsk_u32(vec128 a, vec128 b);							4 * 32bit unsigned compare less than
//			vec128 M_VCmpEqMsk_i32(vec128 a, vec128 b);							4 * 32bit compare equal (same as M_VCmpEqMsk_u32)
//			vec128 M_VCmpGTMsk_i32(vec128 a, vec128 b);							4 * 32bit signed compare greater than
//			vec128 M_VCmpLTMsk_i32(vec128 a, vec128 b);							4 * 32bit signed compare less than

//			uint M_VCmpAllEq_u8(vec128 a, vec128 b);							16 * 8bit compare equal
//			uint M_VCmpAllGT_u8(vec128 a, vec128 b);							16 * 8bit unsigned compare greater than
//			uint M_VCmpAllLT_u8(vec128 a, vec128 b);							16 * 8bit unsigned compare less than
//			uint M_VCmpAllEq_i8(vec128 a, vec128 b);							16 * 8bit compare equal (same as M_VCmpEqMsk_u8)
//			uint M_VCmpAllGT_i8(vec128 a, vec128 b);							16 * 8bit signed compare greater than
//			uint M_VCmpAllLT_i8(vec128 a, vec128 b);							16 * 8bit signed compare less than
//			uint M_VCmpAllEq_u16(vec128 a, vec128 b);							8 * 16bit compare equal
//			uint M_VCmpAllGT_u16(vec128 a, vec128 b);							8 * 16bit unsigned compare greater than
//			uint M_VCmpAllLT_u16(vec128 a, vec128 b);							8 * 16bit unsigned compare less than
//			uint M_VCmpAllEq_i16(vec128 a, vec128 b);							8 * 16bit compare equal (same as M_VCmpEqMsk_u8)
//			uint M_VCmpAllGT_i16(vec128 a, vec128 b);							8 * 16bit signed compare greater than
//			uint M_VCmpAllLT_i16(vec128 a, vec128 b);							8 * 16bit signed compare less than
//			uint M_VCmpAllEq_u32(vec128 a, vec128 b);							4 * 32bit compare equal
//			uint M_VCmpAllGT_u32(vec128 a, vec128 b);							4 * 32bit unsigned compare greater than
//			uint M_VCmpAllLT_u32(vec128 a, vec128 b);							4 * 32bit unsigned compare less than
//			uint M_VCmpAllEq_i32(vec128 a, vec128 b);							4 * 32bit compare equal (same as M_VCmpEqMsk_u8)
//			uint M_VCmpAllGT_i32(vec128 a, vec128 b);							4 * 32bit signed compare greater than
//			uint M_VCmpAllLT_i32(vec128 a, vec128 b);							4 * 32bit signed compare less than

//			uint M_VCmpAnyEq_u8(vec128 a, vec128 b);							16 * 8bit compare equal
//			uint M_VCmpAnyGT_u8(vec128 a, vec128 b);							16 * 8bit unsigned compare greater than
//			uint M_VCmpAnyLT_u8(vec128 a, vec128 b);							16 * 8bit unsigned compare less than
//			uint M_VCmpAnyEq_i8(vec128 a, vec128 b);							16 * 8bit compare equal (same as M_VCmpEqMsk_u8)
//			uint M_VCmpAnyGT_i8(vec128 a, vec128 b);							16 * 8bit signed compare greater than
//			uint M_VCmpAnyLT_i8(vec128 a, vec128 b);							16 * 8bit signed compare less than
//			uint M_VCmpAnyEq_u16(vec128 a, vec128 b);							8 * 16bit compare equal
//			uint M_VCmpAnyGT_u16(vec128 a, vec128 b);							8 * 16bit unsigned compare greater than
//			uint M_VCmpAnyLT_u16(vec128 a, vec128 b);							8 * 16bit unsigned compare less than
//			uint M_VCmpAnyEq_i16(vec128 a, vec128 b);							8 * 16bit compare equal (same as M_VCmpEqMsk_u8)
//			uint M_VCmpAnyGT_i16(vec128 a, vec128 b);							8 * 16bit signed compare greater than
//			uint M_VCmpAnyLT_i16(vec128 a, vec128 b);							8 * 16bit signed compare less than
//			uint M_VCmpAnyEq_u32(vec128 a, vec128 b);							4 * 32bit compare equal
//			uint M_VCmpAnyGT_u32(vec128 a, vec128 b);							4 * 32bit unsigned compare greater than
//			uint M_VCmpAnyLT_u32(vec128 a, vec128 b);							4 * 32bit unsigned compare less than
//			uint M_VCmpAnyEq_i32(vec128 a, vec128 b);							4 * 32bit compare equal (same as M_VCmpEqMsk_u8)
//			uint M_VCmpAnyGT_i32(vec128 a, vec128 b);							4 * 32bit signed compare greater than
//			uint M_VCmpAnyLT_i32(vec128 a, vec128 b);							4 * 32bit signed compare less than
			

//		String formating:
//			CStr M_VStr(vec128 a)
//			CStr M_VStr_u32(vec128 a)
//			CStr M_VStr_i32(vec128 a)
//			CStr M_VStr_u16(vec128 a)
//			CStr M_VStr_i16(vec128 a)
//			CStr M_VStr_u8(vec128 a)
//			CStr M_VStr_i8(vec128 a)


//		Miscellaneous stuff that got cut:	(on a case by case basis because their implementation on either VMX or SSE2 would suck nuts)
//			vec128 M_VMul_i8(vec128 a, vec128 b);								16 * 8bit signed multiply, a*b
//			vec128 M_VMul_i16(vec128 a, vec128 b);								8 * 16bit signed multiply, a*b
//			vec128 M_VMul_i32(vec128 a, vec128 b);								4 * 32bit signed multiply, a*b
//			vec128 M_VMul_u8(vec128 a, vec128 b);								16 * 8bit unsigned multiply, a*b
//			vec128 M_VMul_u16(vec128 a, vec128 b);								8 * 16bit unsigned multiply, a*b
//			vec128 M_VMul_u32(vec128 a, vec128 b);								4 * 32bit unsigned multiply, a*b
//			vec128 M_VAdds_i32(vec128 a, vec128 b);								4 * 32bit signed add with saturate, Min(0x7fffffff, a+b)
//			vec128 M_VSubs_i32(vec128 a, vec128 b);								4 * 32bit signed subtract with saturate, Max(-0x80000000, a+b)
//			vec128 M_VMin_i32(vec128 a, vec128 b);								4 * 32bit signed minimum, Min(a, b)
//			vec128 M_VMax_i32(vec128 a, vec128 b);								4 * 32bit signed maximum, Max(a, b)
//			vec128 M_VAdds_u32(vec128 a, vec128 b);								4 * 32bit unsigned add with saturate, Min(0xffffffff, a+b)
//			vec128 M_VSubs_u32(vec128 a, vec128 b);								4 * 32bit unsigned subtract with saturate, Max(0, a-b)
//			vec128 M_VAddc_u32(vec128 a, vec128 b);								4 * 32bit unsigned add w/ carry-out
//			vec128 M_VSubc_u32(vec128 a, vec128 b);								4 * 32bit unsigned subtract w/ carry-out
//			vec128 M_VMin_u32(vec128 a, vec128 b);								4 * 32bit unsigned minimum, Min(a, b)
//			vec128 M_VMax_u32(vec128 a, vec128 b);								4 * 32bit unsigned maximum, Min(a, b)
//			vec128 M_VSar_i8(vec128 a, vec128 b);								16 * 8bit algebraic shift right, a >> b
//			vec128 M_VShr_u8(vec128 a, vec128 b);								16 * 8bit shift right, a >> b
//			vec128 M_VShl_u8(vec128 a, vec128 b);								16 * 8bit shift left, a << b
//			vec128 M_VSar_i16(vec128 a, vec128 b);								8 * 16bit algebraic shift right, a >> b
//			vec128 M_VShr_u16(vec128 a, vec128 b);								8 * 16bit shift right, a >> b
//			vec128 M_VShl_u16(vec128 a, vec128 b);								8 * 16bit shift left, a << b
//			vec128 M_VSar_i32(vec128 a, vec128 b);								4 * 32bit algebraic shift right, a >> b
//			vec128 M_VShr_u32(vec128 a, vec128 b);								4 * 32bit shift right, a >> b
//			vec128 M_VShl_u32(vec128 a, vec128 b);								4 * 32bit shift left, a << b
//			vec128 M_VRound(vec128 _a);											{ (float)Round(a.x), (float)Round(a.y), (float)Round(a.z), (float)Round(a.w) }
//			vec128 M_Vfp32tou32(vec128 _a) { return __vctuxs(_a, 0); }			{ (int)a.x, (int)a.y, (int)a.z, (int)a.w }, float to uint32, truncate
//			vec128 M_Vu32tofp32(vec128 _a) { return __vcfux(_a, 0); }



#endif //PLATFORM_SPU
#endif

