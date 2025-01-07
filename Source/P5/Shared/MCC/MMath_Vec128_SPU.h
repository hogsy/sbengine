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


//		Load
#define M_VZero ((vec128) {0.0f, 0.0f, 0.0f, 0.0f})
#define M_VHalf ((vec128) {0.5f, 0.5f 0.5f 0.5f}; )
#define M_VOne  ((vec128) {1.0f, 1.0f 1.0f 1.0f}; )
#define M_VTwo  ((vec128) {2.0f, 2.0f 2.0f 2.0f}; )
#define M_VOne  ((vec128) {1.0f, 1.0f 1.0f 1.0f}; )
#define M_VTwo  ((vec128) {2.0f, 2.0f 2.0f 2.0f}; )
#define M_VPi   ((vec128) {3.14159265358979323f, 3.14159265358979323f, 3.14159265358979323f, 3.14159265358979323f}; )
#define M_V2Pi  ((vec128) {2.0f*3.14159265358979323f, 2.0f*3.14159265358979323f, 2.0f*3.14159265358979323f, 2.0f*3.14159265358979323f}; )

#define M_VConst(x, y, z, w) { ((vec128) {x,y,z,w}); }
//			vec128 M_VConstMsk(int _Mask);										{ (_Mask & 1) ? 0xffffffff : 0, (_Mask & 2) ? 0xffffffff : 0, (_Mask & 4) ? 0xffffffff : 0, (_Mask & 8) ? 0xffffffff : 0 }

//			vec128 M_VConst_i8(c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf);	{ c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf }, Load 16 * 8bit signed intergers
//			vec128 M_VConst_u8(c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf);	{ c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf }, Load 16 * 8bit unsigned intergers
//			vec128 M_VConst_i16(c0, c1, c2, c3, c4, c5, c6, c7);				{ c0, c1, c2, c3, c4, c5, c6, c7 }, Load 8 * 16bit signed intergers
//			vec128 M_VConst_u16(c0, c1, c2, c3, c4, c5, c6, c7);				{ c0, c1, c2, c3, c4, c5, c6, c7 }, Load 8 * 16bit unsigned intergers
//			vec128 M_VConst_i16(c0, c1, c2, c3);								{ c0, c1, c2, c3, c4, c5, c6, c7 }, Load 4 * 32bit signed intergers
//			vec128 M_VConst_u16(c0, c1, c2, c3);								{ c0, c1, c2, c3, c4, c5, c6, c7 }, Load 4 * 32bit unsigned intergers

M_FORCEINLINE vec128 M_VScalar(fp4 x)		{ return spu_splats(x); }
//			vec128 M_VScalar_i8(int8 c0)										{ c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0 }, Load 8-bit signed scalar
//			vec128 M_VScalar_u8(uint8 c0)										{ c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0 }, Load 8-bit unsigned scalar
//			vec128 M_VScalar_i16(int16 c0)										{ c0, c0, c0, c0, c0, c0, c0, c0 }, Load 16-bit signed scalar
//			vec128 M_VScalar_u16(uint16 c0)										{ c0, c0, c0, c0, c0, c0, c0, c0 }, Load 16-bit unsigned scalar
//			vec128 M_VScalar_i32(int32 c0)										{ c0, c0, c0, c0 }, Load 32-bit signed scalar
//			vec128 M_VScalar_u32(uint32 c0)										{ c0, c0, c0, c0 }, Load 32-bit unsigned scalar

//			vec128 M_VLdMem(void*);												{ .... }, Load 16 bytes of whatever, must be aligned
M_FORCEINLINE vec128 M_VLdScalar(fp4 x)					{ return spu_splats(x); }
//			vec128 M_VLdScalar_i8(int8 x)										{ x, x, x, x }, Load dynamic scalar uint32, Use M_VScalar for constants!
//			vec128 M_VLdScalar_u8(uint8 x)										{ x, x, x, x }, Load dynamic scalar uint32, Use M_VScalar for constants!
//			vec128 M_VLdScalar_i162(int16 x)									{ x, x, x, x }, Load dynamic scalar uint32, Use M_VScalar for constants!
//			vec128 M_VLdScalar_u16(uint16 x)									{ x, x, x, x }, Load dynamic scalar uint32, Use M_VScalar for constants!
//			vec128 M_VLdScalar_i32(int32 x)										{ x, x, x, x }, Load dynamic scalar uint32, Use M_VScalar for constants!
//			vec128 M_VLdScalar_u32(uint32 x)									{ x, x, x, x }, Load dynamic scalar uint32, Use M_VScalar for constants!
M_FORCEINLINE vec128 M_VLd(fp4 x, fp4 y, fp4 z, fp4 w) { return ((vec128){ x, y, z, w }); }
//			vec128 M_VLd(const CVec4Dfp4& _a);
//			void M_VLd_V3x4(CVec3Dfp4* _pSrcV, vec128& a, vec128& b, vec128& c, vec128& d);		Load 4 CVec3Dfp4, _pSrcV must be aligned of course

//		Store
//			void M_VSt(vec128 _a, void* _pDest);
//			void M_VSt_V3x4(CVec3Dfp4* _pDstV, vec128 a, vec128 b, vec128 c, vec128 d);			Store 4 CVec3Dfp4, _pSrcV must be aligned of course

//			CVec3Dfp4 M_VGetV3_Slow(vec128 a);
//			CVec4Dfp4 M_VGetV4_Slow(vec128 a);
M_FORCEINLINE fp4 M_VGetX_Slow(vec128 a) { return spu_extract(a,0); }
M_FORCEINLINE fp4 M_VGetY_Slow(vec128 a) { return spu_extract(a,1); }
M_FORCEINLINE fp4 M_VGetZ_Slow(vec128 a) { return spu_extract(a,2); }
M_FORCEINLINE fp4 M_VGetW_Slow(vec128 a) { return spu_extract(a,3); }

//		Shuffle:
//			vec128 M_VShuf(vec128 a, imm _Mask);								{ a.k[_Mask.x], a.k[_Mask.y], a.k[_Mask.z], a.k[_Mask.w] }
M_FORCEINLINE vec128 M_VSplat(vec128 a,uint32 iComp) { return spu_splats(spu_extract(a,iComp)); }
M_FORCEINLINE vec128 M_VSplatX(vec128 a) { return spu_splats(spu_extract(a,0)); }
M_FORCEINLINE vec128 M_VSplatY(vec128 a) { return spu_splats(spu_extract(a,1)); }
M_FORCEINLINE vec128 M_VSplatZ(vec128 a) { return spu_splats(spu_extract(a,2)); }
M_FORCEINLINE vec128 M_VSplatW(vec128 a) { return spu_splats(spu_extract(a,3)); }
//			vec128 M_VMrgXY(vec128 a, vec128 b);								{ a.x, b.x, a.y, b.y }
//			vec128 M_VMrgZW(vec128 a, vec128 b);								{ a.z, b.z, a.w, b.w }

//		Conversion:
//			vec128 M_VTrunc(vec128 _a);											{ (float)Trunc(a.x), (float)Trunc(a.y), (float)Trunc(a.z), (float)Trunc(a.w) }

//			vec128 M_Vfp4toi32(vec128 _a)										{ (int)a.x, (int)a.y, (int)a.z, (int)a.w }, float to int32, truncate
//			vec128 M_Vi32tofp4(vec128 _a)										{ (float)a.x, (float)a.y, (float)a.z, (float)a.w }

//		Logical operations:
M_FORCEINLINE vec128 M_VOr(vec128 a, vec128 b) { return spu_or(a, b); }
M_FORCEINLINE vec128 M_VAnd(vec128 a, vec128 b) { return spu_and(a, b); }
M_FORCEINLINE vec128 M_VAndNot(vec128 a, vec128 b) { return spu_andc(a, b); }
M_FORCEINLINE vec128 M_VXor(vec128 a, vec128 b) { return spu_xor(a, b); }
M_FORCEINLINE vec128 M_VNot(vec128 a) { return spu_nor(a, M_VZero); }

//		Floating point compare
M_FORCEINLINE vec128 M_VCmpEqMsk(vec128 a, vec128 b) { return vec128(spu_cmpeq(a,b)); }
M_FORCEINLINE vec128 M_VCmpGEMsk(vec128 a, vec128 b) { return vec128(spu_or(spu_cmpeq(a,b),spu_cmpgt(a,b))); }
M_FORCEINLINE vec128 M_VCmpGTMsk(vec128 a, vec128 b) { return vec128(spu_cmpgt(a,b)); }
M_FORCEINLINE vec128 M_VCmpLEMsk(vec128 a, vec128 b) { return vec128(spu_or(spu_cmpeq(a,b),spu_cmpgt(b,a))); }
M_FORCEINLINE vec128 M_VCmpLTMsk(vec128 a, vec128 b) { return vec128(spu_cmpgt(b,a)); }

//			uint M_VCmpAllEq(vec128 a, vec128 b);								(a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w)

//			uint M_VCmpAllGE(vec128 a, vec128 b);								(a.x >= b.x) && (a.y >= b.y) && (a.z >= b.z) && (a.w >= b.w)
//			uint M_VCmpAllGT(vec128 a, vec128 b);								(a.x >  b.x) && (a.y >  b.y) && (a.z >  b.z) && (a.w >  b.w)
//			uint M_VCmpAllLE(vec128 a, vec128 b);								(a.x <= b.x) && (a.y <= b.y) && (a.z <= b.z) && (a.w <= b.w)
//			uint M_VCmpAllLT(vec128 a, vec128 b);								(a.x <  b.x) && (a.y <  b.y) && (a.z <  b.z) && (a.w <  b.w)

//			uint M_VCmpAnyEq(vec128 a, vec128 b);								(a.x == b.x) || (a.y == b.y) || (a.z == b.z) || (a.w == b.w)
//			uint M_VCmpAnyGE(vec128 a, vec128 b);								(a.x >= b.x) || (a.y >= b.y) || (a.z >= b.z) || (a.w >= b.w)
//			uint M_VCmpAnyGT(vec128 a, vec128 b);								(a.x >  b.x) || (a.y >  b.y) || (a.z >  b.z) || (a.w >  b.w)
//			uint M_VCmpAnyLE(vec128 a, vec128 b);								(a.x <= b.x) || (a.y <= b.y) || (a.z <= b.z) || (a.w <= b.w)
//			uint M_VCmpAnyLT(vec128 a, vec128 b);								(a.x <  b.x) || (a.y <  b.y) || (a.z <  b.z) || (a.w <  b.w)

//			vec128 M_VSelMsk(vec128 msk, vec128 a, vec128 b);					M_VOr(M_VAnd(a, msk), M_VAnd(a, M_VNot(msk)))
//			vec128 M_VSelMskRev(mskk, a, b)										M_VSelMsk(msk, b, a)
//			vec128 M_VSel(vec128 comp, vec128 a, vec128 b);						{ (comp.x >= 0) ? a.x : b.x, (comp.y >= 0) ? a.y : b.y, (comp.z >= 0) ? a.z : b.z, (comp.w >= 0) ? a.w : b.w }

//			uint operator== (vec128 _a, vec128 _b)								M_VCmpAllEq(_a, _b)
//			uint operator>= (vec128 _a, vec128 _b)								M_VCmpAllGE(_a, _b)
//			uint operator> (vec128 _a, vec128 _b)								M_VCmpAllGT(_a, _b)
//			uint operator<= (vec128 _a, vec128 _b)								M_VCmpAllLE(_a, _b)
//			uint operator< (vec128 _a, vec128 _b)								M_VCmpAllLT(_a, _b)

//		Floating point core arithmetics:
M_FORCEINLINE vec128 M_VAdd(vec128 a, vec128 b)				{ return spu_add(a, b); }
M_FORCEINLINE vec128 M_VAddh(vec128 a)						{ return spu_splats(spu_extract(a,0)+spu_extract(a,1)+spu_extract(a,2)+spu_extract(a,3)); }
M_FORCEINLINE vec128 M_VSub(vec128 a, vec128 b)				{ return spu_sub(a, b); }
M_FORCEINLINE vec128 M_VMul(vec128 a, vec128 b)				{ return spu_sub(a, b); }
M_FORCEINLINE vec128 M_VMAdd(vec128 a, vec128 b, vec128 c)	{ return spu_madd(a, b, c); }
M_FORCEINLINE vec128 M_VNMSub(vec128 a, vec128 b, vec128 c)	{ return spu_nmsub(a, b, c); }
M_FORCEINLINE vec128 M_VMin(vec128 a, vec128 b)				{ return spu_sel(a, b, spu_cmpgt(a,b)); }
M_FORCEINLINE vec128 M_VMax(vec128 a, vec128 b)				{ return spu_sel(a, b, spu_cmpgt(b,a)); }

M_FORCEINLINE vec128 M_VDp3(vec128 a, vec128 b) 
{
	vec128 result;
	result = spu_mul( a, b);
	result = spu_madd( spu_rlqwbyte( a, 4 ), spu_rlqwbyte( b, 4 ), result );
	result = spu_madd( spu_rlqwbyte( a, 8 ), spu_rlqwbyte( b, 8 ), result );
	return spu_splats(spu_extract(result,0));
}

M_FORCEINLINE vec128 M_VDp4(vec128 a, vec128 b) 
{
	vec128 result;
	result = spu_mul( a, b );
 	result = spu_madd( spu_rlqwbyte( a, 4 ), spu_rlqwbyte( b, 4 ), result );
	result = spu_add( spu_rlqwbyte( result, 8 ), result );
	return spu_splats(spu_extract(result,0));
}

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




//			vec128 operator+ (vec128 _a, vec128 _b)								M_VAdd(_a, _b)
//			vec128 operator- (vec128 _a, vec128 _b)								M_VSub(_a, _b)
//			vec128 operator- (vec128 _a)										M_VNeg(_a)

//			vec128 M_VRcp_Est(vec128 a);										Estimate of { 1/a.x, 1/a.y, 1/a.z, 1/a.w }
//			vec128 M_VRsq_Est(vec128 a);										Estimate of { 1/sqrt(a.x), 1/sqrt(a.y), 1/sqrt(a.z), 1/sqrt(a.w) }
//			vec128 M_VSqrt_Est(vec128 a);										Estimate of { sqrt(a.x), sqrt(a.y), sqrt(a.z), sqrt(a.w) }

//			vec128 M_VRcp(vec128 a);											{ 1/a.x, 1/a.y, 1/a.z, 1/a.w }, 7 correct digits
//			vec128 M_VRsq(vec128 a);											{ 1/sqrt(a.x), 1/sqrt(a.y), 1/sqrt(a.z), 1/sqrt(a.w) }, 7 correct digits
//			vec128 M_VSqrt(vec128 a);											{ sqrt(a.x), sqrt(a.y), sqrt(a.z), sqrt(a.w) }, 7 correct digits

//		Floating point compound arithmetics:
//			vec128 M_VAbs(vec128 _a);
//			vec128 M_VNegMsk(vec128 a, vec128 msk);								{ mx? -a.x : a.x, my? -a.y : a.y, mz? -a.z : a.z, mw? -a.w : a.w }, if msk = M_VConstMsk(mx,my,mz,mw)
//			vec128 M_VLrp(vec128 a, vec128 b, vec128 t);						{ a*(1-t) + b*t }
//			vec128 M_VClamp01(vec128 a);										{ clamp01(a.x), clamp01(a.y), clamp01(a.z), clamp01(a.w) }
//			vec128 M_VClamp(vec128 a, vec128 _min, vec128 _max);				Component Min(Max(a, _min), _max)

//			vec128 M_VDp3x2(vec128 a0, vec128 b0, vec128 a1, vec128 b1);							{ dp3(a0, b0), dp3(a1, b1), dp3(a0, b0), dp3(a1, b1) }
//			vec128 M_VDp3x3(vec128 a0, vec128 b0, vec128 a1, vec128 b1, vec128 a2, vec128 b2);		{ dp3(a0, b0), dp3(a1, b1), dp3(a2, b2), 0 }
//			vec128 M_VDp3x4(vec128 a0, vec128 b0, vec128 a1, vec128 b1, 
//							vec128 a2, vec128 b2, vec128 a3, vec128 b3);							{ dp3(a0, b0), dp3(a1, b1), dp3(a2, b2), dp3(a3, b3) }

//			vec128 M_VDp4x2(vec128 a0, vec128 b0, vec128 a1, vec128 b1);							{ dp4(a0, b0), dp4(a1, b1), dp4(a0, b0), dp4(a1, b1) }
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
//			void M_VMatMul(const CMat4Dfp4& a, const CMat4Dfp4& b, CMat4Dfp4& c);	Matrix-Matrix multiply, aliasing permitted
//			vec128 M_VMul(vec128 a, CMat4Dfp4 m);								Vector-Matrix multiply
//			vec128 M_VMul4x3(vec128 a, CMat4Dfp4 m);							Vector-Matrix multiply

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
//			vec128 M_Vfp4tou32(vec128 _a) { return __vctuxs(_a, 0); }			{ (int)a.x, (int)a.y, (int)a.z, (int)a.w }, float to uint32, truncate
//			vec128 M_Vu32tofp4(vec128 _a) { return __vcfux(_a, 0); }




