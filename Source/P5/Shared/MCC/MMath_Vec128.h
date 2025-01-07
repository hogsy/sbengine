#ifndef __INCLUDE_XCC_MATH_VEC128
#define __INCLUDE_XCC_MATH_VEC128

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			vec128 math library

	Author:			Magnus Högdahl

	Copyright:		Starbreeze AB, 2006

	History:
		2006-02-04:		Created file.

	Comments:

		MMath classes vec128 intrinsic union members:
			CVec4Dfp4::v
			CPlane3Dfp4::v
			CQuatfp4::v
			CMat4Dfp4::r[4]
			CAxisRotfp4::v

\*____________________________________________________________________________*/


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Instruction set reference:

		Load
			vec128 M_VZero();													{ 0, 0, 0, 0 }
			vec128 M_VHalf();													{ 0.5, 0.5, 0.5, 0.5 }
			vec128 M_VOne();													{ 1, 1, 1, 1 }
			vec128 M_VTwo();													{ 2, 2, 2, 2 }
			vec128 M_VPi();														{ pi, pi, pi, pi }
			vec128 M_V2Pi();													{ 2pi, 2pi, 2pi, 2pi }

			vec128 M_VNaN();													{ NaN, NaN, NaN, NaN }
			vec128 M_VInf();													{ Inf, Inf, Inf, Inf }

			vec128 M_VConst(x, y, z, w);										{ x, y, z, w }
			vec128 M_VConstMsk(x, y, z, w);										{ x ? 0xffffffff : 0, y ? 0xffffffff : 0, z ? 0xffffffff : 0, w ? 0xffffffff : 0 }
			vec128 M_VConstMsk2(int _Mask);										{ (_Mask & 1) ? 0xffffffff : 0, (_Mask & 2) ? 0xffffffff : 0, (_Mask & 4) ? 0xffffffff : 0, (_Mask & 8) ? 0xffffffff : 0 }

			vec128 M_VConst_i8(c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf);	{ c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf }, Load 16 * 8bit signed intergers
			vec128 M_VConst_u8(c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf);	{ c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf }, Load 16 * 8bit unsigned intergers
			vec128 M_VConst_i16(c0, c1, c2, c3, c4, c5, c6, c7);				{ c0, c1, c2, c3, c4, c5, c6, c7 }, Load 8 * 16bit signed intergers
			vec128 M_VConst_u16(c0, c1, c2, c3, c4, c5, c6, c7);				{ c0, c1, c2, c3, c4, c5, c6, c7 }, Load 8 * 16bit unsigned intergers
			vec128 M_VConst_i16(c0, c1, c2, c3);								{ c0, c1, c2, c3, c4, c5, c6, c7 }, Load 4 * 32bit signed intergers
			vec128 M_VConst_u16(c0, c1, c2, c3);								{ c0, c1, c2, c3, c4, c5, c6, c7 }, Load 4 * 32bit unsigned intergers

			vec128 M_VScalar(fp4 x)												{ x, x, x, x }, Load float scalar
			vec128 M_VScalar_i8(int8 c0)										{ c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0 }, Load 8-bit signed scalar
			vec128 M_VScalar_u8(uint8 c0)										{ c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0 }, Load 8-bit unsigned scalar
			vec128 M_VScalar_i16(int16 c0)										{ c0, c0, c0, c0, c0, c0, c0, c0 }, Load 16-bit signed scalar
			vec128 M_VScalar_u16(uint16 c0)										{ c0, c0, c0, c0, c0, c0, c0, c0 }, Load 16-bit unsigned scalar
			vec128 M_VScalar_i32(int32 c0)										{ c0, c0, c0, c0 }, Load 32-bit signed scalar
			vec128 M_VScalar_u32(uint32 c0)										{ c0, c0, c0, c0 }, Load 32-bit unsigned scalar

			vec128 M_VLdMem(void*);												{ .... }, Load 16 bytes of whatever, must be aligned
			vec128 M_VLdScalar(fp4 x);											{ x, x, x, x }, Load dynamic scalar float, Use M_VScalar for constants!
			vec128 M_VLdScalar_i8(int8 x)										{ x, x, x, x }, Load dynamic scalar uint32, Use M_VScalar for constants!
			vec128 M_VLdScalar_u8(uint8 x)										{ x, x, x, x }, Load dynamic scalar uint32, Use M_VScalar for constants!
			vec128 M_VLdScalar_i162(int16 x)									{ x, x, x, x }, Load dynamic scalar uint32, Use M_VScalar for constants!
			vec128 M_VLdScalar_u16(uint16 x)									{ x, x, x, x }, Load dynamic scalar uint32, Use M_VScalar for constants!
			vec128 M_VLdScalar_i32(int32 x)										{ x, x, x, x }, Load dynamic scalar uint32, Use M_VScalar for constants!
			vec128 M_VLdScalar_u32(uint32 x)									{ x, x, x, x }, Load dynamic scalar uint32, Use M_VScalar for constants!
			vec128 M_VLd(fp4 x, fp4 y, fp4 z, fp4 w)							{ x, y, z, w }, Load 4 dynamic floats, Use M_VConst for constants!
			vec128 M_VLd(const CVec4Dfp4& _a);
			void M_VLd_V3x4(CVec3Dfp4* _pSrcV, vec128& a, vec128& b, vec128& c, vec128& d);		Load 4 CVec3Dfp4, _pSrcV must be aligned of course

		Store
			void M_VSt(vec128 _a, void* _pDest);
			void M_VSt_V3x4(CVec3Dfp4* _pDstV, vec128 a, vec128 b, vec128 c, vec128 d);			Store 4 CVec3Dfp4, _pSrcV must be aligned of course

			CVec3Dfp4 M_VGetV3_Slow(vec128 a);
			CVec4Dfp4 M_VGetV4_Slow(vec128 a);
			fp4 M_VGetX_Slow(vec128 a);
			fp4 M_VGetY_Slow(vec128 a);
			fp4 M_VGetZ_Slow(vec128 a);
			fp4 M_VGetW_Slow(vec128 a);

		Shuffle:
			vec128 M_VShuf(vec128 a, imm _Mask);								{ a.k[_Mask.x], a.k[_Mask.y], a.k[_Mask.z], a.k[_Mask.w] }
			vec128 M_VSplat(vec128 a, _iComp);									{ a.k[iComp], a.k[iComp], a.k[iComp], a.k[iComp] }
			vec128 M_VSplatX(vec128 a);											{ a.x, a.x, a.x, a.x }
			vec128 M_VSplatY(vec128 a);											{ a.y, a.y, a.y, a.y }
			vec128 M_VSplatZ(vec128 a);											{ a.z, a.z, a.z, a.z }
			vec128 M_VSplatW(vec128 a);											{ a.w, a.w, a.w, a.w }
			vec128 M_VMrgXY(vec128 a, vec128 b);								{ a.x, b.x, a.y, b.y }
			vec128 M_VMrgZW(vec128 a, vec128 b);								{ a.z, b.z, a.w, b.w }

		Conversion:
			vec128 M_VTrunc(vec128 _a);											{ (float)Trunc(a.x), (float)Trunc(a.y), (float)Trunc(a.z), (float)Trunc(a.w) }

			vec128 M_Vfp4toi32(vec128 _a)										{ (int)a.x, (int)a.y, (int)a.z, (int)a.w }, float to int32, truncate
			vec128 M_Vi32tofp4(vec128 _a)										{ (float)a.x, (float)a.y, (float)a.z, (float)a.w }

		Logical operations:
			vec128 M_VOr(vec128 a, vec128 b);									{ a | b, bitwise }
			vec128 M_VAnd(vec128 a, vec128 b);									{ a & b, bitwise }
			vec128 M_VAndNot(vec128 a, vec128 b);								{ a & ~b, bitwise }
			vec128 M_VXor(vec128 a, vec128 b);									{ a ^ b, bitwise }
			vec128 M_Not(vec128 a);												{ ~a, bitwise }

		Floating point compare
			vec128 M_VCmpEqMsk(vec128 a, vec128 b);								{ (a.x == b.x)?0xffffffff:0x00000000, (a.y == b.y)?0xffffffff:0x00000000, (a.z == b.z)?0xffffffff:0x00000000, (a.w == b.w)?0xffffffff:0x00000000 }
			vec128 M_VCmpGEMsk(vec128 a, vec128 b);								{ (a.x >= b.x)?0xffffffff:0x00000000, (a.y >= b.y)?0xffffffff:0x00000000, (a.z >= b.z)?0xffffffff:0x00000000, (a.w >= b.w)?0xffffffff:0x00000000 }
			vec128 M_VCmpGTMsk(vec128 a, vec128 b);								{ (a.x >  b.x)?0xffffffff:0x00000000, (a.y >  b.y)?0xffffffff:0x00000000, (a.z >  b.z)?0xffffffff:0x00000000, (a.w >  b.w)?0xffffffff:0x00000000 }
			vec128 M_VCmpLEMsk(vec128 a, vec128 b);								{ (a.x <= b.x)?0xffffffff:0x00000000, (a.y <= b.y)?0xffffffff:0x00000000, (a.z <= b.z)?0xffffffff:0x00000000, (a.w <= b.w)?0xffffffff:0x00000000 }
			vec128 M_VCmpLTMsk(vec128 a, vec128 b);								{ (a.x <  b.x)?0xffffffff:0x00000000, (a.y <  b.y)?0xffffffff:0x00000000, (a.z <  b.z)?0xffffffff:0x00000000, (a.w <  b.w)?0xffffffff:0x00000000 }

			uint M_VCmpAllEq(vec128 a, vec128 b);								(a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w)
			uint M_VCmpAllGE(vec128 a, vec128 b);								(a.x >= b.x) && (a.y >= b.y) && (a.z >= b.z) && (a.w >= b.w)
			uint M_VCmpAllGT(vec128 a, vec128 b);								(a.x >  b.x) && (a.y >  b.y) && (a.z >  b.z) && (a.w >  b.w)
			uint M_VCmpAllLE(vec128 a, vec128 b);								(a.x <= b.x) && (a.y <= b.y) && (a.z <= b.z) && (a.w <= b.w)
			uint M_VCmpAllLT(vec128 a, vec128 b);								(a.x <  b.x) && (a.y <  b.y) && (a.z <  b.z) && (a.w <  b.w)

			uint M_VCmpAnyEq(vec128 a, vec128 b);								(a.x == b.x) || (a.y == b.y) || (a.z == b.z) || (a.w == b.w)
			uint M_VCmpAnyGE(vec128 a, vec128 b);								(a.x >= b.x) || (a.y >= b.y) || (a.z >= b.z) || (a.w >= b.w)
			uint M_VCmpAnyGT(vec128 a, vec128 b);								(a.x >  b.x) || (a.y >  b.y) || (a.z >  b.z) || (a.w >  b.w)
			uint M_VCmpAnyLE(vec128 a, vec128 b);								(a.x <= b.x) || (a.y <= b.y) || (a.z <= b.z) || (a.w <= b.w)
			uint M_VCmpAnyLT(vec128 a, vec128 b);								(a.x <  b.x) || (a.y <  b.y) || (a.z <  b.z) || (a.w <  b.w)

			vec128 M_VSelComp(int _iComp, vec128 a, vec128 b);					M_VSel(M_VConstMsk(1 << (_iComp)), _a, _b), iComp must be an immediate
			vec128 M_VSelMsk(vec128 msk, vec128 a, vec128 b);					M_VOr(M_VAnd(a, msk), M_VAnd(a, M_VNot(msk)))
			vec128 M_VSelMskRev(mskk, a, b)										M_VSelMsk(msk, b, a)
			vec128 M_VSel(vec128 comp, vec128 a, vec128 b);						{ (comp.x >= 0) ? a.x : b.x, (comp.y >= 0) ? a.y : b.y, (comp.z >= 0) ? a.z : b.z, (comp.w >= 0) ? a.w : b.w }

			uint operator== (vec128 _a, vec128 _b)								M_VCmpAllEq(_a, _b)
			uint operator>= (vec128 _a, vec128 _b)								M_VCmpAllGE(_a, _b)
			uint operator> (vec128 _a, vec128 _b)								M_VCmpAllGT(_a, _b)
			uint operator<= (vec128 _a, vec128 _b)								M_VCmpAllLE(_a, _b)
			uint operator< (vec128 _a, vec128 _b)								M_VCmpAllLT(_a, _b)

		Floating point core arithmetics:
			vec128 M_VAdd(vec128 a, vec128 b);									{ a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w }
			vec128 M_VAddh(vec128 a);											{ sum, sum, sum, sum }, sum = a.x + a.y + a.z + a.w
			vec128 M_VSub(vec128 a, vec128 b);									{ a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w }
			vec128 M_VMul(vec128 a, vec128 b);									{ a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w }
			vec128 M_VMAdd(vec128 a, vec128 b, vec128 c);						{ a * b + c }
			vec128 M_VNMSub(vec128 _a, vec128 _b, vec128 _c);					{ c - (a * b) }
			vec128 M_VMin(vec128 a, vec128 b);									{ Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z), Min(a.w, b.w) }
			vec128 M_VMax(vec128 a, vec128 b);									{ Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z), Max(a.w, b.w) }
			vec128 M_VDp3(vec128 a, vec128 b);									{ dp3, dp3, dp3, dp3 }, dp3 = a.x*b.x + a.y*b.y + a.z*b.z }
			vec128 M_VDp4(vec128 a, vec128 b);									{ dp4, dp4, dp4, dp4 }, dp4 = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w }
			vec128 M_VXpd(vec128 a, vec128 b);									{ a.y*b.z - a.z*b.y, -a.x*b.z + a.z*b.x, a.x*b.y - a.z*b.y, 0 }

			vec128 operator+ (vec128 _a, vec128 _b)								M_VAdd(_a, _b)
			vec128 operator- (vec128 _a, vec128 _b)								M_VSub(_a, _b)
			vec128 operator- (vec128 _a)										M_VNeg(_a)

			vec128 M_VRcp_Est(vec128 a);										Estimate of { 1/a.x, 1/a.y, 1/a.z, 1/a.w }
			vec128 M_VRsq_Est(vec128 a);										Estimate of { 1/sqrt(a.x), 1/sqrt(a.y), 1/sqrt(a.z), 1/sqrt(a.w) }
			vec128 M_VSqrt_Est(vec128 a);										Estimate of { sqrt(a.x), sqrt(a.y), sqrt(a.z), sqrt(a.w) }

			vec128 M_VRcp(vec128 a);											{ 1/a.x, 1/a.y, 1/a.z, 1/a.w }, 7 correct digits
			vec128 M_VRsq(vec128 a);											{ 1/sqrt(a.x), 1/sqrt(a.y), 1/sqrt(a.z), 1/sqrt(a.w) }, 7 correct digits
			vec128 M_VSqrt(vec128 a);											{ sqrt(a.x), sqrt(a.y), sqrt(a.z), sqrt(a.w) }, 7 correct digits

		Floating point compound arithmetics:
			vec128 M_VAbs(vec128 _a);
			vec128 M_VNegMsk(vec128 a, vec128 msk);								{ mx? -a.x : a.x, my? -a.y : a.y, mz? -a.z : a.z, mw? -a.w : a.w }, if msk = M_VConstMsk(mx+my*2+mz*4+mw*8) and mx,my,mz,mw = [0,1]
			vec128 M_VLrp(vec128 a, vec128 b, vec128 t);						{ a*(1-t) + b*t }
			vec128 M_VClamp01(vec128 a);										{ clamp01(a.x), clamp01(a.y), clamp01(a.z), clamp01(a.w) }
			vec128 M_VClamp(vec128 a, vec128 _min, vec128 _max);				Component Min(Max(a, _min), _max)

			vec128 M_VDp3x2(vec128 a0, vec128 b0, vec128 a1, vec128 b1);							{ dp3(a0, b0), dp3(a1, b1), dp3(a0, b0), dp3(a1, b1) }
			vec128 M_VDp3x3(vec128 a0, vec128 b0, vec128 a1, vec128 b1, vec128 a2, vec128 b2);		{ dp3(a0, b0), dp3(a1, b1), dp3(a2, b2), 0 }
			vec128 M_VDp3x4(vec128 a0, vec128 b0, vec128 a1, vec128 b1, 
							vec128 a2, vec128 b2, vec128 a3, vec128 b3);							{ dp3(a0, b0), dp3(a1, b1), dp3(a2, b2), dp3(a3, b3) }

			vec128 M_VDp4x2(vec128 a0, vec128 b0, vec128 a1, vec128 b1);							{ dp4(a0, b0), dp4(a1, b1), dp4(a0, b0), dp4(a1, b1) }
			vec128 M_VDp4x3(vec128 a0, vec128 b0, vec128 a1, vec128 b1, vec128 a2, vec128 b2);		{ dp4(a0, b0), dp4(a1, b1), dp4(a2, b2), 0 }
			vec128 M_VDp4x4(vec128 a0, vec128 b0, vec128 a1, vec128 b1, 
							vec128 a2, vec128 b2, vec128 a3, vec128 b3);							{ dp4(a0, b0), dp4(a1, b1), dp4(a2, b2), dp4(a3, b3) }

			vec128 M_VLen3(vec128 a);											{ |a|, |a|, |a|, |a| }
			vec128 M_VLen3x2(vec128 a, vec128 b);								{ |a|, |b|, |a|, |b| }
			vec128 M_VLen3x4(vec128 a, vec128 b, vec128 c, vec128 d);			{ |a|, |b|, |c|, |d| }
			vec128 M_VLen4(vec128 a);											{ |a|, |a|, |a|, |a| }
			vec128 M_VLen4x2(vec128 a, vec128 b);								{ |a|, |b|, |a|, |b| }
			vec128 M_VLen4x4(vec128 a, vec128 b, vec128 c, vec128 d);			{ |a|, |b|, |c|, |d| }

			vec128 M_VLenrcp3_Est(vec128 a);									Estimate of { 1/|a|, 1/|a|, 1/|a|, 1/|a| }
			vec128 M_VLenrcp3x2_Est(vec128 a, vec128 b);						Estimate of { 1/|a|, 1/|b|, 1/|a|, 1/|b| }
			vec128 M_VLenrcp3x4_Est(vec128 a, vec128 b, vec128 c, vec128 d);	Estimate of { 1/|a|, 1/|b|, 1/|c|, 1/|d| }
			vec128 M_VLenrcp4_Est(vec128 a);									Estimate of { 1/|a|, 1/|a|, 1/|a|, 1/|a| }
			vec128 M_VLenrcp4x2_Est(vec128 a, vec128 b);						Estimate of { 1/|a|, 1/|b|, 1/|a|, 1/|b| }
			vec128 M_VLenrcp4x4_Est(vec128 a, vec128 b, vec128 c, vec128 d);	Estimate of { 1/|a|, 1/|b|, 1/|c|, 1/|d| }

			vec128 M_VLenrcp3(vec128 a);										{ 1/|a|, 1/|a|, 1/|a|, 1/|a| }
			vec128 M_VLenrcp3x2(vec128 a, vec128 b);							{ 1/|a|, 1/|b|, 1/|a|, 1/|b| }
			vec128 M_VLenrcp3x4(vec128 a, vec128 b, vec128 c, vec128 d);		{ 1/|a|, 1/|b|, 1/|c|, 1/|d| }
			vec128 M_VLenrcp4(vec128 a);										{ 1/|a|, 1/|a|, 1/|a|, 1/|a| }
			vec128 M_VLenrcp4x2(vec128 a, vec128 b);							{ 1/|a|, 1/|b|, 1/|a|, 1/|b| }
			vec128 M_VLenrcp4x4(vec128 a, vec128 b, vec128 c, vec128 d);		{ 1/|a|, 1/|b|, 1/|c|, 1/|d| }

			vec128 M_VNrm3_Est(vec128 a);										Estimate of 3-component normalize, { x, y, z, 0 }
			void M_VNrm3x2_Est(vec128& a, vec128& b);							Estimate of 3-component normalize * 2, Writes result to input vectors
			void M_VNrm3x4_Est(vec128& a, vec128& b, vec128& c, vec128& d);		Estimate of 3-component normalize * 4, Writes result to input vectors
			vec128 M_VNrm4_Est(vec128 a);										Estimate of 4-component normalize, { x, y, z, w }
			void M_VNrm4x2_Est(vec128& a, vec128& b);							Estimate of 4-component normalize * 2, Writes result to input vectors
			void M_VNrm4x4_Est(vec128& a, vec128& b, vec128& c, vec128& d);		Estimate of 4-component normalize * 4, Writes result to input vectors

			vec128 M_VNrm3(vec128 a);											3-component normalize, { x, y, z, 0 }
			void M_VNrm3x2(vec128& a, vec128& b);								3-component normalize * 2, Writes result to input vectors
			void M_VNrm3x4(vec128& a, vec128& b, vec128& c, vec128& d);			3-component normalize * 4, Writes result to input vectors
			vec128 M_VNrm4(vec128 a);											4-component normalize, { x, y, z, w }
			void M_VNrm4x2(vec128& a, vec128& b);								4-component normalize * 2, Writes result to input vectors
			void M_VNrm4x4(vec128& a, vec128& b, vec128& c, vec128& d);			4-component normalize * 4, Writes result to input vectors

			void M_VTranspose4x4(vec128& a, vec128& b, vec128& c, vec128& d);	Vectors a,b,c,d viewed as a 4x4 matrix is transposed
			void M_VMatMul(const CMat4Dfp4& a, const CMat4Dfp4& b, CMat4Dfp4& c);	Matrix-Matrix multiply, aliasing permitted
			vec128 M_VMulMat(vec128 a, CMat4Dfp4 m);							Vector-Matrix multiply
			vec128 M_VMulMatTransp(vec128 _a, CMat4Dfp4p _Mat);					Vector-MatrixTranspose multiply
			vec128 M_VMulMat4x3(vec128 a, CMat4Dfp4 m);							Vector-Matrix multiply

		Signed Integer arithmetics:
			vec128 M_VAdds_i8(vec128 a, vec128 b);								16 * 8bit signed add with saturate, Min(0x7f, a+b)
			vec128 M_VAdds_i16(vec128 a, vec128 b);								8 * 16bit signed add with saturate, Min(0x7fff, a+b)
			vec128 M_VSubs_i8(vec128 a, vec128 b);								16 * 8bit signed subtract with saturate, Max(-0x80, a+b)
			vec128 M_VSubs_i16(vec128 a, vec128 b);								8 * 16bit signed subtract with saturate, Max(-0x8000, a+b)

			vec128 M_VMin_i8(vec128 a, vec128 b);								16 * 8bit signed minimum, Min(a, b)
			vec128 M_VMin_i16(vec128 a, vec128 b);								8 * 16bit signed minimum, Min(a, b)
			vec128 M_VMax_i8(vec128 a, vec128 b);								16 * 8bit signed maximum, Max(a, b)
			vec128 M_VMax_i16(vec128 a, vec128 b);								8 * 16bit signed maximum, Max(a, b)

		Unsigned Integer arithmetics:
			vec128 M_VAdd_u8(vec128 a, vec128 b);								16 * 8bit unsigned add, (a+b) & 0xff
			vec128 M_VAdds_u8(vec128 a, vec128 b);								16 * 8bit unsigned add with saturate, Min(0xff, a+b)
			vec128 M_VAdd_u16(vec128 a, vec128 b);								8 * 16bit unsigned add, (a+b) & 0xffff
			vec128 M_VAdds_u16(vec128 a, vec128 b);								8 * 16bit unsigned add with saturate, Min(0xffff, a+b)
			vec128 M_VAdd_u32(vec128 a, vec128 b);								4 * 32bit unsigned add, (a+b) & 0xffffffff

			vec128 M_VSub_u8(vec128 a, vec128 b);								16 * 8bit unsigned subtract, (a-b) & 0xff
			vec128 M_VSubs_u8(vec128 a, vec128 b);								16 * 8bit unsigned subtract with saturate, Max(0, a-b)
			vec128 M_VSub_u16(vec128 a, vec128 b);								8 * 16bit unsigned subtract, (a-b) & 0xffff
			vec128 M_VSubs_u16(vec128 a, vec128 b);								8 * 16bit unsigned subtract with saturate, Max(0, a-b)
			vec128 M_VSub_u32(vec128 a, vec128 b);								4 * 32bit unsigned subtract, (a-b) & 0xffffffff

			vec128 M_VMin_u8(vec128 a, vec128 b);								16 * 8bit unsigned minimum, Min(a, b)
			vec128 M_VMin_u16(vec128 a, vec128 b);								8 * 16bit unsigned minimum, Min(a, b)
			vec128 M_VMax_u8(vec128 a, vec128 b);								16 * 8bit unsigned maximum, Min(a, b)
			vec128 M_VMax_u16(vec128 a, vec128 b);								8 * 16bit unsigned maximum, Min(a, b)

			There is no integer multiply, stop looking.

		Integer shift: (Not implemented)
			vec128 M_VSar_i16(vec128 a, uint b);								8 * 16bit algebraic shift right, a >> b
			vec128 M_VShr_u16(vec128 a, uint b);								8 * 16bit shift right, a >> b
			vec128 M_VShl_u16(vec128 a, uint b);								8 * 16bit shift left, a << b
			vec128 M_VSar_i32(vec128 a, uint b);								4 * 32bit algebraic shift right, a >> b
			vec128 M_VShr_u32(vec128 a, uint b);								4 * 32bit shift right, a >> b
			vec128 M_VShl_u32(vec128 a, uint b);								4 * 32bit shift left, a << b

		Integer compare:
			vec128 M_VCmpEqMsk_u8(vec128 a, vec128 b);							16 * 8bit compare equal
			vec128 M_VCmpGTMsk_u8(vec128 a, vec128 b);							16 * 8bit unsigned compare greater than
			vec128 M_VCmpLTMsk_u8(vec128 a, vec128 b);							16 * 8bit unsigned compare less than
			vec128 M_VCmpEqMsk_i8(vec128 a, vec128 b);							16 * 8bit compare equal (same as M_VCmpEqMsk_u8)
			vec128 M_VCmpGTMsk_i8(vec128 a, vec128 b);							16 * 8bit signed compare greater than
			vec128 M_VCmpLTMsk_i8(vec128 a, vec128 b);							16 * 8bit signed compare less than
			vec128 M_VCmpEqMsk_u16(vec128 a, vec128 b);							8 * 16bit compare equal
			vec128 M_VCmpGTMsk_u16(vec128 a, vec128 b);							8 * 16bit unsigned compare greater than
			vec128 M_VCmpLTMsk_u16(vec128 a, vec128 b);							8 * 16bit unsigned compare less than
			vec128 M_VCmpEqMsk_i16(vec128 a, vec128 b);							8 * 16bit compare equal (same as M_VCmpEqMsk_u16)
			vec128 M_VCmpGTMsk_i16(vec128 a, vec128 b);							8 * 16bit signed compare greater than
			vec128 M_VCmpLTMsk_i16(vec128 a, vec128 b);							8 * 16bit signed compare less than
			vec128 M_VCmpEqMsk_u32(vec128 a, vec128 b);							4 * 32bit compare equal
			vec128 M_VCmpGTMsk_u32(vec128 a, vec128 b);							4 * 32bit unsigned compare greater than
			vec128 M_VCmpLTMsk_u32(vec128 a, vec128 b);							4 * 32bit unsigned compare less than
			vec128 M_VCmpEqMsk_i32(vec128 a, vec128 b);							4 * 32bit compare equal (same as M_VCmpEqMsk_u32)
			vec128 M_VCmpGTMsk_i32(vec128 a, vec128 b);							4 * 32bit signed compare greater than
			vec128 M_VCmpLTMsk_i32(vec128 a, vec128 b);							4 * 32bit signed compare less than

			uint M_VCmpAllEq_u8(vec128 a, vec128 b);							16 * 8bit compare equal
			uint M_VCmpAllGT_u8(vec128 a, vec128 b);							16 * 8bit unsigned compare greater than
			uint M_VCmpAllLT_u8(vec128 a, vec128 b);							16 * 8bit unsigned compare less than
			uint M_VCmpAllEq_i8(vec128 a, vec128 b);							16 * 8bit compare equal (same as M_VCmpEqMsk_u8)
			uint M_VCmpAllGT_i8(vec128 a, vec128 b);							16 * 8bit signed compare greater than
			uint M_VCmpAllLT_i8(vec128 a, vec128 b);							16 * 8bit signed compare less than
			uint M_VCmpAllEq_u16(vec128 a, vec128 b);							8 * 16bit compare equal
			uint M_VCmpAllGT_u16(vec128 a, vec128 b);							8 * 16bit unsigned compare greater than
			uint M_VCmpAllLT_u16(vec128 a, vec128 b);							8 * 16bit unsigned compare less than
			uint M_VCmpAllEq_i16(vec128 a, vec128 b);							8 * 16bit compare equal (same as M_VCmpEqMsk_u8)
			uint M_VCmpAllGT_i16(vec128 a, vec128 b);							8 * 16bit signed compare greater than
			uint M_VCmpAllLT_i16(vec128 a, vec128 b);							8 * 16bit signed compare less than
			uint M_VCmpAllEq_u32(vec128 a, vec128 b);							4 * 32bit compare equal
			uint M_VCmpAllGT_u32(vec128 a, vec128 b);							4 * 32bit unsigned compare greater than
			uint M_VCmpAllLT_u32(vec128 a, vec128 b);							4 * 32bit unsigned compare less than
			uint M_VCmpAllEq_i32(vec128 a, vec128 b);							4 * 32bit compare equal (same as M_VCmpEqMsk_u8)
			uint M_VCmpAllGT_i32(vec128 a, vec128 b);							4 * 32bit signed compare greater than
			uint M_VCmpAllLT_i32(vec128 a, vec128 b);							4 * 32bit signed compare less than

			uint M_VCmpAnyEq_u8(vec128 a, vec128 b);							16 * 8bit compare equal
			uint M_VCmpAnyGT_u8(vec128 a, vec128 b);							16 * 8bit unsigned compare greater than
			uint M_VCmpAnyLT_u8(vec128 a, vec128 b);							16 * 8bit unsigned compare less than
			uint M_VCmpAnyEq_i8(vec128 a, vec128 b);							16 * 8bit compare equal (same as M_VCmpEqMsk_u8)
			uint M_VCmpAnyGT_i8(vec128 a, vec128 b);							16 * 8bit signed compare greater than
			uint M_VCmpAnyLT_i8(vec128 a, vec128 b);							16 * 8bit signed compare less than
			uint M_VCmpAnyEq_u16(vec128 a, vec128 b);							8 * 16bit compare equal
			uint M_VCmpAnyGT_u16(vec128 a, vec128 b);							8 * 16bit unsigned compare greater than
			uint M_VCmpAnyLT_u16(vec128 a, vec128 b);							8 * 16bit unsigned compare less than
			uint M_VCmpAnyEq_i16(vec128 a, vec128 b);							8 * 16bit compare equal (same as M_VCmpEqMsk_u8)
			uint M_VCmpAnyGT_i16(vec128 a, vec128 b);							8 * 16bit signed compare greater than
			uint M_VCmpAnyLT_i16(vec128 a, vec128 b);							8 * 16bit signed compare less than
			uint M_VCmpAnyEq_u32(vec128 a, vec128 b);							4 * 32bit compare equal
			uint M_VCmpAnyGT_u32(vec128 a, vec128 b);							4 * 32bit unsigned compare greater than
			uint M_VCmpAnyLT_u32(vec128 a, vec128 b);							4 * 32bit unsigned compare less than
			uint M_VCmpAnyEq_i32(vec128 a, vec128 b);							4 * 32bit compare equal (same as M_VCmpEqMsk_u8)
			uint M_VCmpAnyGT_i32(vec128 a, vec128 b);							4 * 32bit signed compare greater than
			uint M_VCmpAnyLT_i32(vec128 a, vec128 b);							4 * 32bit signed compare less than

		Floating point trig operations: (NOT IMPLEMENTED!)
			vec128 M_VSin_Est(vec128 a);
			vec128 M_VCos_Est(vec128 a);
			vec128 M_VSinCos_Est(vec128 a);

			vec128 M_VSin(vec128 a);
			vec128 M_VCos(vec128 a);
			vec128 M_VSinCos(vec128 a);
			vec128 M_VTan(vec128 a);
			vec128 M_VASin(vec128 a);
			vec128 M_VACos(vec128 a);
			vec128 M_VASinCos(vec128 a);
			vec128 M_VATan(vec128 a);

			vec128 M_VExp(vec128 a);
			vec128 M_VLn(vec128 a);
			vec128 M_VLog(vec128 a);
			vec128 M_VPow(vec128 a, vec128 b);
			vec128 M_VMod(vec128 a, vec128 b);

		Floating point matrix operations: (NOT IMPLEMENTED!)
			void M_VMatTranspose4x4(CMat4Dfp4& m);
			void M_VMatMul(const CMat4Dfp4& a, const CMat4Dfp4& b, CMat4Dfp4& c);Matrix-Matrix multiply, aliasing permitted
			vec128 M_VMatMul(vec128 a, CMat4Dfp4 m);							Vector-Matrix multiply
			vec128 M_VMatMul4x3(vec128 a, CMat4Dfp4 m);							Vector-Matrix multiply
			void M_VMatFromQuat(vec128 a, CMat4Dfp4& m);						
			void M_VMatFromAxisRot(vec128 a, CMat4Dfp4& m);	
			void M_VMatUnit(CMat4Dfp4& m);

		Floating point quaternion operations: (NOT IMPLEMENTED!)
			vec128 M_VQuatMul(vec128 a, vec128 b);								Quat(a) * Quat(b)
			vec128 M_VQuatInv(vec128 a, vec128 b);								-Quat(a)
			vec128 M_VQuatNrm(vec128 a);
			vec128 M_VQuatLerp(vec128 a, vec128 b, vec128 c);
			vec128 M_VQuatUnit();
			vec128 M_VQuatFromMat(CMat4Dfp4 m);
			vec128 M_VQuatFromAxisRot(vec128 a);

		Floating point axis-rotation operations:								{ axis.x, axis.y, axis.z, rotation }, rotation is in starbreeze-angles [0..1]
			vec128 M_VAxisRotFromQuat(vec128 a);

		String formating:
			CStr M_VStr(vec128 a)
			CStr M_VStr_u32(vec128 a)
			CStr M_VStr_i32(vec128 a)
			CStr M_VStr_u16(vec128 a)
			CStr M_VStr_i16(vec128 a)
			CStr M_VStr_u8(vec128 a)
			CStr M_VStr_i8(vec128 a)


		Miscellaneous stuff that got cut:	(on a case by case basis because their implementation on either VMX or SSE2 would suck nuts)
			vec128 M_VMul_i8(vec128 a, vec128 b);								16 * 8bit signed multiply, a*b
			vec128 M_VMul_i16(vec128 a, vec128 b);								8 * 16bit signed multiply, a*b
			vec128 M_VMul_i32(vec128 a, vec128 b);								4 * 32bit signed multiply, a*b
			vec128 M_VMul_u8(vec128 a, vec128 b);								16 * 8bit unsigned multiply, a*b
			vec128 M_VMul_u16(vec128 a, vec128 b);								8 * 16bit unsigned multiply, a*b
			vec128 M_VMul_u32(vec128 a, vec128 b);								4 * 32bit unsigned multiply, a*b
			vec128 M_VAdds_i32(vec128 a, vec128 b);								4 * 32bit signed add with saturate, Min(0x7fffffff, a+b)
			vec128 M_VSubs_i32(vec128 a, vec128 b);								4 * 32bit signed subtract with saturate, Max(-0x80000000, a+b)
			vec128 M_VMin_i32(vec128 a, vec128 b);								4 * 32bit signed minimum, Min(a, b)
			vec128 M_VMax_i32(vec128 a, vec128 b);								4 * 32bit signed maximum, Max(a, b)
			vec128 M_VAdds_u32(vec128 a, vec128 b);								4 * 32bit unsigned add with saturate, Min(0xffffffff, a+b)
			vec128 M_VSubs_u32(vec128 a, vec128 b);								4 * 32bit unsigned subtract with saturate, Max(0, a-b)
			vec128 M_VAddc_u32(vec128 a, vec128 b);								4 * 32bit unsigned add w/ carry-out
			vec128 M_VSubc_u32(vec128 a, vec128 b);								4 * 32bit unsigned subtract w/ carry-out
			vec128 M_VMin_u32(vec128 a, vec128 b);								4 * 32bit unsigned minimum, Min(a, b)
			vec128 M_VMax_u32(vec128 a, vec128 b);								4 * 32bit unsigned maximum, Min(a, b)
			vec128 M_VSar_i8(vec128 a, vec128 b);								16 * 8bit algebraic shift right, a >> b
			vec128 M_VShr_u8(vec128 a, vec128 b);								16 * 8bit shift right, a >> b
			vec128 M_VShl_u8(vec128 a, vec128 b);								16 * 8bit shift left, a << b
			vec128 M_VSar_i16(vec128 a, vec128 b);								8 * 16bit algebraic shift right, a >> b
			vec128 M_VShr_u16(vec128 a, vec128 b);								8 * 16bit shift right, a >> b
			vec128 M_VShl_u16(vec128 a, vec128 b);								8 * 16bit shift left, a << b
			vec128 M_VSar_i32(vec128 a, vec128 b);								4 * 32bit algebraic shift right, a >> b
			vec128 M_VShr_u32(vec128 a, vec128 b);								4 * 32bit shift right, a >> b
			vec128 M_VShl_u32(vec128 a, vec128 b);								4 * 32bit shift left, a << b
			vec128 M_VRound(vec128 _a);											{ (float)Round(a.x), (float)Round(a.y), (float)Round(a.z), (float)Round(a.w) }
			vec128 M_Vfp4tou32(vec128 _a) { return __vctuxs(_a, 0); }			{ (int)a.x, (int)a.y, (int)a.z, (int)a.w }, float to uint32, truncate
			vec128 M_Vu32tofp4(vec128 _a) { return __vcfux(_a, 0); }


\*____________________________________________________________________________*/

// #define CPU_VEC128EMU

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			vec128 component access type.
\*____________________________________________________________________*/
class CVec128Access
{
public:
	union
	{
		vec128 v;
		fp4 k[4];
		int32 ki32[4];
		uint32 ku32[4];
		int16 ki16[8];
		uint16 ku16[8];
		int8 ki8[16];
		uint8 ku8[16];
		struct
		{
			fp4 x, y, z, w;
		};
	};

	CVec128Access() {}

	CVec128Access(const CVec128Access& _v)
	{
		v = _v.v;
	}

	CVec128Access(vec128 _v)
	{
		v = _v;
	}

	void operator= (const CVec128Access& _v)
	{
		v = _v.v;
	}

	void operator= (vec128 _v)
	{
		v = _v;
	}

	operator vec128()
	{
		return v;
	}
};

//----------------------------------------------------------
#define M_VSHUF(x,y,z,w) (((x) << 0) + ((y) << 2) + ((z) << 4) + ((w) << 6))

// Get comp # from shuffle mask
#define M_VCOMPX(Mask) (((Mask)) & 3)
#define M_VCOMPY(Mask) (((Mask) >> 2) & 3)
#define M_VCOMPZ(Mask) (((Mask) >> 4) & 3)
#define M_VCOMPW(Mask) (((Mask) >> 6) & 3)

#define M_VTranspose4x4(r0, r1, r2, r3)	\
{\
	vec128 t0 = M_VMrgXY(r0, r2);	\
	vec128 t1 = M_VMrgXY(r1, r3);	\
	vec128 t2 = M_VMrgZW(r0, r2);	\
	vec128 t3 = M_VMrgZW(r1, r3);	\
	r0 = M_VMrgXY(t0, t1);	\
	r1 = M_VMrgZW(t0, t1);	\
	r2 = M_VMrgXY(t2, t3);	\
	r3 = M_VMrgZW(t2, t3);	\
}

#define M_VTranspose4x3(r0, r1, r2, r3)	\
{\
	vec128 t0 = M_VMrgXY(r0, r2);	\
	vec128 t1 = M_VMrgXY(r1, r3);	\
	vec128 t2 = M_VMrgZW(r0, r2);	\
	vec128 t3 = M_VMrgZW(r1, r3);	\
	r0 = M_VMrgXY(t0, t1);	\
	r1 = M_VMrgZW(t0, t1);	\
	r2 = M_VMrgXY(t2, t3);	\
}

#define M_VSelMskRev(_Mask, a, b) M_VSelMsk(_Mask, b, a)

/*enum
{
	M_V_ALLTRUE = 2,
	M_V_ALLFALSE = 1,
};*/

//----------------------------------------------------------
#define M_VFIXED64INT(x) (((x) < 0) ? (int64(x)-1) : int64(x))
#define M_VFIXED64FRAC(x) uint64(((x)-fp4(M_VFIXED64INT(x)))*fp4(0x00fffffffffffffffLL))
#define M_VFLOATFROMFIXED128(i,f) (fp4(i) + fp4(f)/fp4(0x00fffffffffffffffLL))

#define M_VFLOATTOFIXED128(x) M_VFIXED64INT(x), M_VFIXED64FRAC(x)

template<int64 xi, uint64 xf, int64 yi, uint64 yf, int64 zi, uint64 zf, int64 wi, uint64 wf>
class TVec128Const
{
public:
	static fp4 M_ALIGN(16) ms_Const[4];
};

template<int64 xi, uint64 xf, int64 yi, uint64 yf, int64 zi, uint64 zf, int64 wi, uint64 wf>
fp4 M_ALIGN(16) TVec128Const<xi, xf, yi, yf, zi, zf, wi, wf>::ms_Const[4] =
{
	M_VFLOATFROMFIXED128(xi, xf), M_VFLOATFROMFIXED128(yi, yf), M_VFLOATFROMFIXED128(zi, zf), M_VFLOATFROMFIXED128(wi, wf)
};

//----------------------------------------------------------
template<int TMask>
class TVec128Mask
{
public:
	enum
	{
		MASK_X = (TMask & 1) ? 0xffffffff : 0x00000000,
		MASK_Y = (TMask & 2) ? 0xffffffff : 0x00000000,
		MASK_Z = (TMask & 4) ? 0xffffffff : 0x00000000,
		MASK_W = (TMask & 8) ? 0xffffffff : 0x00000000,
	};

	static uint32 M_ALIGN(16) ms_Mask[4];
};

template<int TMask>
uint32 M_ALIGN(16) TVec128Mask<TMask>::ms_Mask[4] = { TVec128Mask<TMask>::MASK_X, TVec128Mask<TMask>::MASK_Y, TVec128Mask<TMask>::MASK_Z, TVec128Mask<TMask>::MASK_W };

//----------------------------------------------------------
template<typename T, int c0, int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8, int c9, int ca, int cb, int cc, int cd, int ce, int cf>
class TVec128Const_u8
{
public:
	static T M_ALIGN(16) ms_Const[16];
};

template<typename T, int c0, int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8, int c9, int ca, int cb, int cc, int cd, int ce, int cf>
T M_ALIGN(16) TVec128Const_u8<T, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf>::ms_Const[16] = { c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf };

//----------------------------------------------------------
template<typename T, int c0, int c1, int c2, int c3, int c4, int c5, int c6, int c7>
class TVec128Const_u16
{
public:
	static T M_ALIGN(16) ms_Const[8];
};

template<typename T, int c0, int c1, int c2, int c3, int c4, int c5, int c6, int c7>
T M_ALIGN(16) TVec128Const_u16<T, c0, c1, c2, c3, c4, c5, c6, c7>::ms_Const[8] = { c0, c1, c2, c3, c4, c5, c6, c7 };

//----------------------------------------------------------
template<typename T, int64 c0, int64 c1, int64 c2, int64 c3>
class TVec128Const_u32
{
public:
	static T M_ALIGN(16) ms_Const[4];
};

template<typename T, int64 c0, int64 c1, int64 c2, int64 c3>
T M_ALIGN(16) TVec128Const_u32<T, c0, c1, c2, c3>::ms_Const[4] = { c0, c1, c2, c3 };

//----------------------------------------------------------
#define M_VConst(x, y, z, w) *((const vec128*)&TVec128Const<M_VFLOATTOFIXED128(x), M_VFLOATTOFIXED128(y), M_VFLOATTOFIXED128(z), M_VFLOATTOFIXED128(w)>::ms_Const)
#define M_VConstMsk(x, y, z, w) *((const vec128*)&TVec128Mask<((x)?1:0) + ((y)?2:0) + ((z)?4:0) + ((w)?8:0)>::ms_Mask)
#define M_VConstMsk2(_Msk) *((const vec128*)&TVec128Mask<_Msk>::ms_Mask)
#define M_VConst_u8(c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf) *((const vec128*)&TVec128Const_u8<uint8, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf>::ms_Const)
#define M_VConst_i8(c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf) *((const vec128*)&TVec128Const_u8<int8, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf>::ms_Const)
#define M_VConst_u16(c0, c1, c2, c3, c4, c5, c6, c7) *((const vec128*)&TVec128Const_u16<uint16, c0, c1, c2, c3, c4, c5, c6, c7>::ms_Const)
#define M_VConst_i16(c0, c1, c2, c3, c4, c5, c6, c7) *((const vec128*)&TVec128Const_u16<int16, c0, c1, c2, c3, c4, c5, c6, c7>::ms_Const)
#define M_VConst_u32(c0, c1, c2, c3) *((const vec128*)&TVec128Const_u32<uint32, c0, c1, c2, c3>::ms_Const)
#define M_VConst_i32(c0, c1, c2, c3) *((const vec128*)&TVec128Const_u32<int32, c0, c1, c2, c3>::ms_Const)
#define M_VScalar(x) *((const vec128*)&TVec128Const<M_VFLOATTOFIXED128(x), M_VFLOATTOFIXED128(x), M_VFLOATTOFIXED128(x), M_VFLOATTOFIXED128(x)>::ms_Const)
#define M_VScalar_u8(c0) *((const vec128*)&TVec128Const_u8<uint8, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0>::ms_Const)
#define M_VScalar_i8(c0) *((const vec128*)&TVec128Const_u8<int8, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0, c0>::ms_Const)
#define M_VScalar_u16(c0) *((const vec128*)&TVec128Const_u16<uint16, c0, c0, c0, c0, c0, c0, c0, c0>::ms_Const)
#define M_VScalar_i16(c0) *((const vec128*)&TVec128Const_u16<int16, c0, c0, c0, c0, c0, c0, c0, c0>::ms_Const)
#define M_VScalar_u32(c0) *((const vec128*)&TVec128Const_u32<uint32, c0, c0, c0, c0>::ms_Const)
#define M_VScalar_i32(c0) *((const vec128*)&TVec128Const_u32<int32, c0, c0, c0, c0>::ms_Const)

//----------------------------------------------------------
/*#ifdef COMPILER_MSVC4
	#pragma once
#endif
*/

#include "MMath_Vec128_SSE3.h"
#include "MMath_Vec128_VMX.h"
#include "MMath_Vec128_Emu.h"


#endif // __INCLUDE_XCC_MATH_VEC128






