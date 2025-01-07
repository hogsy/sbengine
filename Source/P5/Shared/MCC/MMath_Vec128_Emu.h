#ifndef __INCLUDE_XCC_MATH_VEC128_EMU
#define __INCLUDE_XCC_MATH_VEC128_EMU

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Vec128 emulation

	Author:			Magnus Högdahl

	Copyright:		Magnus Högdahl, 2006

	History:
		2006-02-04:		Created file.





\*____________________________________________________________________________*/

#ifdef CPU_VEC128EMU
	typedef const vec128& vec128p;
	typedef const CMat4Dfp32& CMat4Dfp32p;
#endif
//----------------------------------------------------------

#ifndef M_V128IMP_LOAD

M_FORCEINLINE vec128 M_VLdMem(const void* _p) { return *((vec128*)_p); }
M_FORCEINLINE vec128 M_VLdMem64(const void* _p) { CVec128Access ret; uint64 a = *((uint64*)_p); ret.ku64[0] = a; ret.ku64[1] = a; return ret.v; }

M_FORCEINLINE vec128 M_VLdScalar(fp32 x)
{
	CVec128Access ret;
	ret.x = x;
	ret.y = x;
	ret.z = x;
	ret.w = x;
	return ret.v;
}
M_FORCEINLINE vec128 M_VLdComp_Slow(fp32 x, fp32 y, fp32 z, fp32 w)
{
	CVec128Access ret;
	ret.x = x;
	ret.y = y;
	ret.z = z;
	ret.w = w;
	return ret.v;
}
M_FORCEINLINE vec128 M_VZero() { return M_VScalar(0.0f); }
M_FORCEINLINE vec128 M_VHalf() { return M_VScalar(0.5f); }
M_FORCEINLINE vec128 M_VOne() { return M_VScalar(1.0f); }
M_FORCEINLINE vec128 M_VTwo() { return M_VScalar(2.0f); }


M_FORCEINLINE void M_VSt(vec128 a, void* _p) { *((vec128*)_p) = a; };

#endif

M_FORCEINLINE vec128 M_VPi() { return M_VScalar(3.14159265358979323f); }
M_FORCEINLINE vec128 M_VPiRcp() { return M_VScalar(1.0f/3.14159265358979323f); }
M_FORCEINLINE vec128 M_V2Pi() { return M_VScalar(2.0f*3.14159265358979323f); }
M_FORCEINLINE vec128 M_V2PiRcp() { return M_VScalar(0.5f/3.14159265358979323f); }

#if !defined(PLATFORM_SPU) && !defined(M_COMPILING_ON_VPU)
M_FORCEINLINE vec128 M_VLdScalar_i8_f32(const int8& _v) { return M_VCnv_i32_f32(M_VCnvL_i16_i32(M_VCnvL_i8_i16(M_VLdScalar_i8(_v)))); }
M_FORCEINLINE vec128 M_VLdScalar_u8_f32(const uint8& _v) { return M_VCnv_i32_f32(M_VCnvL_u16_u32(M_VCnvL_u8_u16(M_VLdScalar_u8(_v)))); }
M_FORCEINLINE vec128 M_VLdScalar_i16_f32(const int16& _v) { return M_VCnv_i32_f32(M_VCnvL_i16_i32(M_VLdScalar_i16(_v))); }
M_FORCEINLINE vec128 M_VLdScalar_u16_f32(const uint16& _v) { return M_VCnv_i32_f32(M_VCnvL_u16_u32(M_VLdScalar_u16(_v))); }
M_FORCEINLINE vec128 M_VLdScalar_i32_f32(const int32& _v) { return M_VCnv_i32_f32(M_VLdScalar_i32(_v)); }
#endif
//M_FORCEINLINE vec128 M_VLdScalar_u32_f32(const uint32& _v) { return M_VCnv_i32_f32(M_VLdScalar_u32(_v)); }

M_FORCEINLINE vec128 M_VNaN() { return M_VScalar_u32(0xffc00000); }
M_FORCEINLINE vec128 M_VInf() { return M_VScalar_u32(0x7f800000); }

#if !defined(PLATFORM_SPU) && !defined(M_COMPILING_ON_VPU)
M_FORCEINLINE vec128 M_VLd_V4f16_f32(const class CVec4Dfp2Aggr* _pV)
{
	vec128 v = M_VLd64(_pV);
	vec128 tmp = M_VMrgL_u16_u32(M_VZero(), v);
	return tmp;
}

M_FORCEINLINE vec128 M_VLd_V4u8_f32(const uint32* _pV)
{
	return M_VCnv_i32_f32(M_VCnvL_u16_u32(M_VCnvL_u8_u16(M_VLd32(_pV))));
}

#ifdef CPU_LITTLEENDIAN
M_FORCEINLINE vec128 M_VLd_Pixel32_f32(const uint32* _pV)
{
	return M_VShuf(M_VLd_V4u8_f32(_pV), M_VSHUF(2,1,0,3));
}
#else
M_FORCEINLINE vec128 M_VLd_Pixel32_f32(const uint32* _pV)
{
	return M_VShuf(M_VLd_V4u8_f32(_pV), M_VSHUF(1,2,3,0));
}
#endif


M_FORCEINLINE void M_VSt_V4f32_f16(vec128 _a, class CVec4Dfp2Aggr* _pV)
{
	vec128 ashr = M_VShrImm_u32(_a, 16);
	vec128 tmp = M_VCnvM_u32_u16(ashr, ashr);
	M_VStAny64(tmp, _pV);
}

M_FORCEINLINE void M_VSt_V4f32_u8(vec128 _a, uint32* _pV)
{
	vec128 i32 = M_VCnv_f32_i32(_a);
	vec128 i16 = M_VCnvS_i32_i16(i32, i32);
	M_VStAny32(M_VCnvS_i16_u8(i16, i16), _pV);
}

#ifdef CPU_LITTLEENDIAN
M_FORCEINLINE void M_VSt_V4f32_Pixel32(vec128 _a, uint32* _pV)
{
	M_VSt_V4f32_u8(M_VShuf(_a, M_VSHUF(2,1,0,3)), _pV);
}
#else
M_FORCEINLINE void M_VSt_V4f32_Pixel32(vec128 _a, uint32* _pV)
{
	M_VSt_V4f32_u8(M_VShuf(_a, M_VSHUF(3,0,1,2)), _pV);
}

#endif

#endif

//----------------------------------------------------------
#ifndef M_V128IMP_SHUFFLE
M_FORCEINLINE vec128 M_VShuf(vec128 _a, uint _Mask)
{
	CVec128Access ret;
	CVec128Access a(_a);
	ret.k[0] = a.k[M_VCOMPX(_Mask)];
	ret.k[1] = a.k[M_VCOMPY(_Mask)];
	ret.k[2] = a.k[M_VCOMPZ(_Mask)];
	ret.k[3] = a.k[M_VCOMPW(_Mask)];
	return ret.v;
}

M_FORCEINLINE vec128 M_VSplat(vec128 _a, uint _iComp)
{
	CVec128Access ret;
	CVec128Access a(_a);
	fp32 Splat = a.k[_iComp & 3];
	ret.k[0] = Splat;
	ret.k[1] = Splat;
	ret.k[2] = Splat;
	ret.k[3] = Splat;
	return ret.v;
}
#endif // M_V128IMP_SHUFFLE

//----------------------------------------------------------
#ifndef M_V128IMP_MERGE

M_FORCEINLINE int8 Sati8(int16 _a) { return (_a < -128) ? -128 : (_a > 127) ? 127 : _a; }
M_FORCEINLINE uint8 Satu8(int16 _a) { return (_a < 0) ? 0 : (_a > 255) ? 255 : _a; }
M_FORCEINLINE int16 Sati16(int32 _a) { return (_a < -32768) ? -32768 : (_a > 32767) ? 32767 : int16(_a); }
M_FORCEINLINE uint16 Satu16(int32 _a) { return (_a < 0) ? 0 : (_a > 65535) ? 65535 : uint16(_a); }

M_FORCEINLINE vec128 M_VMrgXY(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.k[0] = a.k[0];
	ret.k[1] = b.k[0];
	ret.k[2] = a.k[1];
	ret.k[3] = b.k[1];
	return ret.v;
}

M_FORCEINLINE vec128 M_VMrgZW(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.k[0] = a.k[2];
	ret.k[1] = b.k[2];
	ret.k[2] = a.k[3];
	ret.k[3] = b.k[3];
	return ret.v;
}

M_FORCEINLINE vec128 M_VMrgL_u16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ku16[0] = a.ku16[0];
	ret.ku16[1] = b.ku16[0];
	ret.ku16[2] = a.ku16[1];
	ret.ku16[3] = b.ku16[1];
	ret.ku16[4] = a.ku16[2];
	ret.ku16[5] = b.ku16[2];
	ret.ku16[6] = a.ku16[3];
	ret.ku16[7] = b.ku16[3];
	return ret.v;
}

M_FORCEINLINE vec128 M_VMrgH_u16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ku16[0] = a.ku16[4];
	ret.ku16[1] = b.ku16[4];
	ret.ku16[2] = a.ku16[5];
	ret.ku16[3] = b.ku16[5];
	ret.ku16[4] = a.ku16[6];
	ret.ku16[5] = b.ku16[6];
	ret.ku16[6] = a.ku16[7];
	ret.ku16[7] = b.ku16[7];
	return ret.v;
}

M_FORCEINLINE vec128 M_VMrgL_u8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ku8[0] = a.ku8[0];
	ret.ku8[1] = b.ku8[0];
	ret.ku8[2] = a.ku8[1];
	ret.ku8[3] = b.ku8[1];
	ret.ku8[4] = a.ku8[2];
	ret.ku8[5] = b.ku8[2];
	ret.ku8[6] = a.ku8[3];
	ret.ku8[7] = b.ku8[3];
	ret.ku8[8] = a.ku8[4];
	ret.ku8[9] = b.ku8[4];
	ret.ku8[10] = a.ku8[5];
	ret.ku8[11] = b.ku8[5];
	ret.ku8[12] = a.ku8[6];
	ret.ku8[13] = b.ku8[6];
	ret.ku8[14] = a.ku8[7];
	ret.ku8[15] = b.ku8[7];
	return ret.v;
}

M_FORCEINLINE vec128 M_VMrgH_u8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ku8[0] = a.ku8[8];
	ret.ku8[1] = b.ku8[8];
	ret.ku8[2] = a.ku8[9];
	ret.ku8[3] = b.ku8[9];
	ret.ku8[4] = a.ku8[10];
	ret.ku8[5] = b.ku8[10];
	ret.ku8[6] = a.ku8[11];
	ret.ku8[7] = b.ku8[11];
	ret.ku8[8] = a.ku8[12];
	ret.ku8[9] = b.ku8[12];
	ret.ku8[10] = a.ku8[13];
	ret.ku8[11] = b.ku8[13];
	ret.ku8[12] = a.ku8[14];
	ret.ku8[13] = b.ku8[14];
	ret.ku8[14] = a.ku8[15];
	ret.ku8[15] = b.ku8[15];
	return ret.v;
}

M_FORCEINLINE vec128 M_VCnvs_i32_i16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ki16[0] = Sati16(a.ki32[0]);
	ret.ki16[1] = Sati16(a.ki32[1]);
	ret.ki16[2] = Sati16(a.ki32[2]);
	ret.ki16[3] = Sati16(a.ki32[3]);
	ret.ki16[4] = Sati16(b.ki32[0]);
	ret.ki16[5] = Sati16(b.ki32[1]);
	ret.ki16[6] = Sati16(b.ki32[2]);
	ret.ki16[7] = Sati16(b.ki32[3]);
	return ret.v;
}

M_FORCEINLINE vec128 M_VCnvs_i16_i8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ki8[0] = Sati8(a.ki16[0]);
	ret.ki8[1] = Sati8(a.ki16[1]);
	ret.ki8[2] = Sati8(a.ki16[2]);
	ret.ki8[3] = Sati8(a.ki16[3]);
	ret.ki8[4] = Sati8(a.ki16[4]);
	ret.ki8[5] = Sati8(a.ki16[5]);
	ret.ki8[6] = Sati8(a.ki16[6]);
	ret.ki8[7] = Sati8(a.ki16[7]);
	ret.ki8[8] = Sati8(b.ki16[0]);
	ret.ki8[9] = Sati8(b.ki16[1]);
	ret.ki8[10] = Sati8(b.ki16[2]);
	ret.ki8[11] = Sati8(b.ki16[3]);
	ret.ki8[12] = Sati8(b.ki16[4]);
	ret.ki8[13] = Sati8(b.ki16[5]);
	ret.ki8[14] = Sati8(b.ki16[6]);
	ret.ki8[15] = Sati8(b.ki16[7]);
	return ret.v;
}

M_FORCEINLINE vec128 M_VCnvs_i16_u8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ku8[0] = Satu8(a.ki16[0]);
	ret.ku8[1] = Satu8(a.ki16[1]);
	ret.ku8[2] = Satu8(a.ki16[2]);
	ret.ku8[3] = Satu8(a.ki16[3]);
	ret.ku8[4] = Satu8(a.ki16[4]);
	ret.ku8[5] = Satu8(a.ki16[5]);
	ret.ku8[6] = Satu8(a.ki16[6]);
	ret.ku8[7] = Satu8(a.ki16[7]);
	ret.ku8[8] = Satu8(b.ki16[0]);
	ret.ku8[9] = Satu8(b.ki16[1]);
	ret.ku8[10] = Satu8(b.ki16[2]);
	ret.ku8[11] = Satu8(b.ki16[3]);
	ret.ku8[12] = Satu8(b.ki16[4]);
	ret.ku8[13] = Satu8(b.ki16[5]);
	ret.ku8[14] = Satu8(b.ki16[6]);
	ret.ku8[15] = Satu8(b.ki16[7]);
	return ret.v;
}

#endif // M_V128IMP_MERGE

//----------------------------------------------------------
#ifndef M_V128IMP_VSPLATXYZW
M_FORCEINLINE vec128 M_VSplatX(vec128 a) { return M_VSplat(a, 0); }
M_FORCEINLINE vec128 M_VSplatY(vec128 a) { return M_VSplat(a, 1); }
M_FORCEINLINE vec128 M_VSplatZ(vec128 a) { return M_VSplat(a, 2); }
M_FORCEINLINE vec128 M_VSplatW(vec128 a) { return M_VSplat(a, 3); }
#endif // M_V128IMP_VSPLATXYZW

//----------------------------------------------------------
// v0 = x0,y0,z0,x1
// v1 = y1,z1,x2,y2
// v2 = z2,x3,y3,z3
#define M_VLd_V3x4(_pV, _a, _b, _c, _d)	\
{	\
	const vec128* pV128 = (const vec128*)_pV;	\
	vec128 v0 = pV128[0];	\
	vec128 v1 = pV128[1];	\
	vec128 v2 = pV128[2];	\
	vec128 select = M_VConstMsk(1,1,1,0);	\
	vec128 ta = M_VAnd(select, v0);	\
	vec128 tb = M_VAnd(select, M_VShuf(M_VMrgXY(v1, M_VSplatW(v0)), M_VSHUF(3,0,2,3)));	\
	vec128 tc = M_VAnd(select, M_VShuf(M_VMrgZW(v1, M_VSplatX(v2)), M_VSHUF(0,2,1,3)));	\
	vec128 td = M_VAnd(select, M_VShuf(v2, M_VSHUF(1,2,3,3)));	\
	_a = ta; _b = tb; _c = tc; _d = td;	\
}

#define M_VSt_V3x4(_pV, _a, _b, _c, _d)	\
{	\
	vec128 select0 = M_VConstMsk(1,1,0,0);	\
	vec128 select1 = M_VConstMsk(1,1,1,0);	\
	vec128 select2 = M_VShuf(select0, M_VSHUF(0,2,2,2));	\
	vec128 s0 = M_VSelMsk(select1, _a, M_VSplatX(_b));		\
	vec128 s1 = M_VShuf(M_VMrgXY(M_VShuf(_b, M_VSHUF(1,2,2,2)), _c), M_VSHUF(0,2,1,3));	\
	vec128 s2 = M_VSelMsk(select2, M_VSplatZ(_c), M_VShuf(_d, M_VSHUF(0,0,1,2)));	\
	vec128*M_RESTRICT pV128 = (vec128*M_RESTRICT)(_pV);	\
	pV128[0] = s0; pV128[1] = s1; pV128[2] = s2;	\
}

//----------------------------------------------------------
/*void M_VStScalar(vec128 _a, void* _pDest, int _iComp)
{
	M_VSt(M_VSel(M_VConstMsk(1 << (_iComp)), _a, M_VLdMem(_pDest)));
}*/

/*	M_TRACEALWAYS(CStrF("s0 = %s\n", M_VStr(s0).Str() ));	\
	M_TRACEALWAYS(CStrF("s1 = %s\n", M_VStr(s1).Str() ));	\
	M_TRACEALWAYS(CStrF("s2 = %s\n", M_VStr(s2).Str() ));	\ */

//	vec128 s1 = M_VSelMskRev(select0, M_VShuf(_b, M_VSHUF(1,2,0,0)), M_VShuf(_c, M_VSHUF(0,0,0,1)));

//----------------------------------------------------------
#if !defined(M_COMPILING_ON_VPU)
M_FORCEINLINE CVec3Dfp32 M_VGetV3_Slow(vec128 a)
{
	CVec128Access v;
	v.v = a;
	return CVec3Dfp32(v.k[0], v.k[1], v.k[2]);
}

M_FORCEINLINE CVec4Dfp32 M_VGetV4_Slow(vec128 a)
{
	return CVec4Dfp32(a);
}
#endif

M_FORCEINLINE fp32 M_VGetX_Slow(vec128 a)
{
	CVec128Access v(a);
	return v.k[0];
}

M_FORCEINLINE fp32 M_VGetY_Slow(vec128 a)
{
	CVec128Access v(a);
	return v.k[1];
}
M_FORCEINLINE fp32 M_VGetZ_Slow(vec128 a)
{
	CVec128Access v(a);
	return v.k[2];
}
M_FORCEINLINE fp32 M_VGetW_Slow(vec128 a)
{
	CVec128Access v(a);
	return v.k[3];
}


//----------------------------------------------------------
#ifndef M_V128IMP_LOGICOP
M_FORCEINLINE vec128 M_VOr(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ku32[0] = a.ku32[0] | b.ku32[0];
	ret.ku32[1] = a.ku32[1] | b.ku32[1];
	ret.ku32[2] = a.ku32[2] | b.ku32[2];
	ret.ku32[3] = a.ku32[3] | b.ku32[3];
	return ret.v;
}

M_FORCEINLINE vec128 M_VAnd(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ku32[0] = a.ku32[0] & b.ku32[0];
	ret.ku32[1] = a.ku32[1] & b.ku32[1];
	ret.ku32[2] = a.ku32[2] & b.ku32[2];
	ret.ku32[3] = a.ku32[3] & b.ku32[3];
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

M_FORCEINLINE vec128 M_VXor(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ku32[0] = a.ku32[0] ^ b.ku32[0];
	ret.ku32[1] = a.ku32[1] ^ b.ku32[1];
	ret.ku32[2] = a.ku32[2] ^ b.ku32[2];
	ret.ku32[3] = a.ku32[3] ^ b.ku32[3];
	return ret.v;
}

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

#endif // M_V128IMP_LOGICOP

//----------------------------------------------------------
#ifndef M_V128IMP_COMPARE

M_FORCEINLINE vec128 M_VCmpEqMsk(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ku32[0] = (a.k[0] == b.k[0]) ? 0xffffffff : 0x00000000;
	ret.ku32[1] = (a.k[1] == b.k[1]) ? 0xffffffff : 0x00000000;
	ret.ku32[2] = (a.k[2] == b.k[2]) ? 0xffffffff : 0x00000000;
	ret.ku32[3] = (a.k[3] == b.k[3]) ? 0xffffffff : 0x00000000;
	return ret.v;
}

M_FORCEINLINE vec128 M_VCmpGEMsk(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ku32[0] = (a.k[0] >= b.k[0]) ? 0xffffffff : 0x00000000;
	ret.ku32[1] = (a.k[1] >= b.k[1]) ? 0xffffffff : 0x00000000;
	ret.ku32[2] = (a.k[2] >= b.k[2]) ? 0xffffffff : 0x00000000;
	ret.ku32[3] = (a.k[3] >= b.k[3]) ? 0xffffffff : 0x00000000;
	return ret.v;
}

M_FORCEINLINE vec128 M_VCmpGTMsk(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ku32[0] = (a.k[0] >  b.k[0]) ? 0xffffffff : 0x00000000;
	ret.ku32[1] = (a.k[1] >  b.k[1]) ? 0xffffffff : 0x00000000;
	ret.ku32[2] = (a.k[2] >  b.k[2]) ? 0xffffffff : 0x00000000;
	ret.ku32[3] = (a.k[3] >  b.k[3]) ? 0xffffffff : 0x00000000;
	return ret.v;
}

M_FORCEINLINE vec128 M_VCmpLEMsk(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ku32[0] = (a.k[0] <= b.k[0]) ? 0xffffffff : 0x00000000;
	ret.ku32[1] = (a.k[1] <= b.k[1]) ? 0xffffffff : 0x00000000;
	ret.ku32[2] = (a.k[2] <= b.k[2]) ? 0xffffffff : 0x00000000;
	ret.ku32[3] = (a.k[3] <= b.k[3]) ? 0xffffffff : 0x00000000;
	return ret.v;
}

M_FORCEINLINE vec128 M_VCmpLTMsk(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.ku32[0] = (a.k[0] <  b.k[0]) ? 0xffffffff : 0x00000000;
	ret.ku32[1] = (a.k[1] <  b.k[1]) ? 0xffffffff : 0x00000000;
	ret.ku32[2] = (a.k[2] <  b.k[2]) ? 0xffffffff : 0x00000000;
	ret.ku32[3] = (a.k[3] <  b.k[3]) ? 0xffffffff : 0x00000000;
	return ret.v;
}

M_FORCEINLINE uint M_VCmpAllEq(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	return (a.k[0] == b.k[0]) && (a.k[1] == b.k[1]) && (a.k[2] == b.k[2]) && (a.k[3] == b.k[3]);
}
M_FORCEINLINE uint M_VCmpAllGE(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	return (a.k[0] >= b.k[0]) && (a.k[1] >= b.k[1]) && (a.k[2] >= b.k[2]) && (a.k[3] >= b.k[3]);
}
M_FORCEINLINE uint M_VCmpAllGT(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	return (a.k[0] >  b.k[0]) && (a.k[1] >  b.k[1]) && (a.k[2] >  b.k[2]) && (a.k[3] >  b.k[3]);
}
M_FORCEINLINE uint M_VCmpAllLE(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	return (a.k[0] <= b.k[0]) && (a.k[1] <= b.k[1]) && (a.k[2] <= b.k[2]) && (a.k[3] <= b.k[3]);
}
M_FORCEINLINE uint M_VCmpAllLT(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	return (a.k[0] <  b.k[0]) && (a.k[1] <  b.k[1]) && (a.k[2] <  b.k[2]) && (a.k[3] <  b.k[3]);
}

M_FORCEINLINE uint M_VCmpAnyEq(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	return (a.k[0] == b.k[0]) || (a.k[1] == b.k[1]) || (a.k[2] == b.k[2]) || (a.k[3] == b.k[3]);
}
M_FORCEINLINE uint M_VCmpAnyGE(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	return (a.k[0] >= b.k[0]) || (a.k[1] >= b.k[1]) || (a.k[2] >= b.k[2]) || (a.k[3] >= b.k[3]);
}
M_FORCEINLINE uint M_VCmpAnyGT(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	return (a.k[0] >  b.k[0]) || (a.k[1] >  b.k[1]) || (a.k[2] >  b.k[2]) || (a.k[3] >  b.k[3]);
}
M_FORCEINLINE uint M_VCmpAnyLE(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	return (a.k[0] <= b.k[0]) || (a.k[1] <= b.k[1]) || (a.k[2] <= b.k[2]) || (a.k[3] <= b.k[3]);
}
M_FORCEINLINE uint M_VCmpAnyLT(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	return (a.k[0] <  b.k[0]) || (a.k[1] <  b.k[1]) || (a.k[2] <  b.k[2]) || (a.k[3] <  b.k[3]);
}

M_FORCEINLINE vec128 M_VSelMsk(vec128 msk, vec128 a, vec128 b) { return M_VOr(M_VAnd(a, msk), M_VAnd(b, M_VNot(msk))); }

#endif

#define M_VSelComp(_iComp, _a, _b) M_VSelMsk(M_VConstMsk2(1 << (_iComp)), _a, _b)

M_FORCEINLINE vec128 M_VSel(vec128 _CompareZero, vec128 a, vec128 b) { return M_VSelMsk(M_VCmpGEMsk(_CompareZero, M_VZero()), a, b); }	// (comparezero >= 0) ? a : b


M_FORCEINLINE vec128 M_VLd_V3_Slow(const TVector3Aggr<fp32>* _pV)
{
	vec128 m = M_VConstMsk(1, 1, 1, 0);
	vec128 a = M_VLdU(_pV);
	return M_VAnd(m, a);
}

M_FORCEINLINE vec128 M_VLd_P3_Slow(const TVector3Aggr<fp32>* _pV)
{
	vec128 one = M_VOne();
	vec128 m = M_VConstMsk(1, 1, 1, 0);
	vec128 a = M_VLdU(_pV);
	return M_VSelMsk(m, a, one);
}

M_FORCEINLINE void M_VSt_V3_Slow(vec128 _a, TVector3Aggr<fp32>* _pV)
{
	M_VStAny32(M_VSplatX(_a), _pV->k+0);
	M_VStAny32(M_VSplatY(_a), _pV->k+1);
	M_VStAny32(M_VSplatZ(_a), _pV->k+2);
}



// -------------------------------------------------------------------
/*#ifndef M_V128IMP_FLOAT_COMPAREOPERATORS

M_FORCEINLINE uint operator== (vec128 _a, vec128 _b)
{
	return M_VCmpAllEq(_a, _b);
}

M_FORCEINLINE uint operator>= (vec128 _a, vec128 _b)
{
	return M_VCmpAllGE(_a, _b);
}

M_FORCEINLINE uint operator> (vec128 _a, vec128 _b)
{
	return M_VCmpAllGT(_a, _b);
}

M_FORCEINLINE uint operator<= (vec128 _a, vec128 _b)
{
	return M_VCmpAllLE(_a, _b);
}

M_FORCEINLINE uint operator< (vec128 _a, vec128 _b)
{
	return M_VCmpAllLT(_a, _b);
}
#endif*/

//----------------------------------------------------------
#ifndef M_V128IMP_FLOAT_ARITHMETICS
M_FORCEINLINE vec128 M_VAdd(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.k[0] = a.k[0] + b.k[0];
	ret.k[1] = a.k[1] + b.k[1];
	ret.k[2] = a.k[2] + b.k[2];
	ret.k[3] = a.k[3] + b.k[3];
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

M_FORCEINLINE vec128 M_VSub(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.k[0] = a.k[0] - b.k[0];
	ret.k[1] = a.k[1] - b.k[1];
	ret.k[2] = a.k[2] - b.k[2];
	ret.k[3] = a.k[3] - b.k[3];
	return ret.v;
}
M_FORCEINLINE vec128 M_VMul(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.k[0] = a.k[0] * b.k[0];
	ret.k[1] = a.k[1] * b.k[1];
	ret.k[2] = a.k[2] * b.k[2];
	ret.k[3] = a.k[3] * b.k[3];
	return ret.v;
}

M_FORCEINLINE vec128 M_VMAdd(vec128 _a, vec128 _b, vec128 _c)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access c(_c);
	CVec128Access ret;
	ret.k[0] = a.k[0] * b.k[0] + c.k[0];
	ret.k[1] = a.k[1] * b.k[1] + c.k[1];
	ret.k[2] = a.k[2] * b.k[2] + c.k[2];
	ret.k[3] = a.k[3] * b.k[3] + c.k[3];
	return ret.v;
}

M_FORCEINLINE vec128 M_VMin(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.k[0] = Min(a.k[0], b.k[0]);
	ret.k[1] = Min(a.k[1], b.k[1]);
	ret.k[2] = Min(a.k[2], b.k[2]);
	ret.k[3] = Min(a.k[3], b.k[3]);
	return ret.v;
}
M_FORCEINLINE vec128 M_VMax(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access ret;
	ret.k[0] = Max(a.k[0], b.k[0]);
	ret.k[1] = Max(a.k[1], b.k[1]);
	ret.k[2] = Max(a.k[2], b.k[2]);
	ret.k[3] = Max(a.k[3], b.k[3]);
	return ret.v;
}
M_FORCEINLINE vec128 M_VNeg(vec128 _a)
{
	CVec128Access a(_a);
	CVec128Access ret;
	ret.k[0] = -a.k[0];
	ret.k[1] = -a.k[1];
	ret.k[2] = -a.k[2];
	ret.k[3] = -a.k[3];
	return ret.v;
}
#endif // M_V128IMP_FLOATARITHMETICS

//----------------------------------------------------------
#ifndef M_V128IMP_FLOAT_DOTPROD

M_FORCEINLINE vec128 M_VDp3(vec128 _a, vec128 _b)
{
	vec128 p = M_VMul(_a, _b);
	return M_VAdd(M_VAdd(M_VSplat(p, 0), M_VSplat(p, 1)), M_VSplat(p, 2));
}

M_FORCEINLINE vec128 M_VDp3x2(vec128 _a0, vec128 _b0, vec128p _a1, vec128p _b1)
{
	vec128 p0 = M_VMul(_a0, _b0);
	vec128 p1 = M_VMul(_a1, _b1);
	vec128 sx0 = M_VSplat(p0, 0);
	vec128 sy0 = M_VSplat(p0, 1);
	vec128 sz0 = M_VSplat(p0, 2);
	vec128 sx1 = M_VSplat(p1, 0);
	vec128 sy1 = M_VSplat(p1, 1);
	vec128 sz1 = M_VSplat(p1, 2);
	return M_VAdd(M_VAdd(M_VMrgXY(sx0, sx1), M_VMrgXY(sy0, sy1)), M_VMrgXY(sz0, sz1));
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

M_FORCEINLINE vec128 M_VDp4(vec128 _a, vec128 _b)
{
	return M_VAddh(M_VMul(_a, _b));
}

M_FORCEINLINE vec128 M_VDp4x2(vec128 _a0, vec128 _b0, vec128p _a1, vec128p _b1)
{
	vec128 p0 = M_VMul(_a0, _b0);
	vec128 p1 = M_VMul(_a1, _b1);
	vec128 s = M_VAdd(M_VMrgXY(p0, p1), M_VMrgZW(p0, p1));		// { p0.x+p0.z, p1.x+p1.z, p0.y+p0.w, p1.y+p1.w }
	return M_VAdd(s, M_VShuf(s, M_VSHUF(2,3,0,1)));
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

#endif

//----------------------------------------------------------
#ifndef M_V128IMP_FLOAT_XPROD
M_FORCEINLINE vec128 M_VXpd(vec128 a, vec128 b)
{
	vec128 a0 = M_VShuf(a, M_VSHUF(1,2,0,3));
	vec128 a1 = M_VShuf(a, M_VSHUF(2,0,1,3));
	vec128 b0 = M_VShuf(b, M_VSHUF(2,0,1,3));
	vec128 b1 = M_VShuf(b, M_VSHUF(1,2,0,3));
	return M_VSub(M_VMul(a0, b0), M_VMul(a1, b1));
}
#endif

//----------------------------------------------------------
#ifndef M_V128IMP_FLOAT_RCP_SQRT_EST

M_FORCEINLINE vec128 M_VRcp_Est(vec128 _a)
{
	CVec128Access a(_a);
	CVec128Access ret;
	ret.k[0] = 1.0f / a.k[0];
	ret.k[1] = 1.0f / a.k[1];
	ret.k[2] = 1.0f / a.k[2];
	ret.k[3] = 1.0f / a.k[3];
	return ret.v;
}

M_FORCEINLINE vec128 M_VRsq_Est(vec128 _a)
{
	CVec128Access a(_a);
	CVec128Access ret;
	ret.k[0] = 1.0f / M_Sqrt(a.k[0]);
	ret.k[1] = 1.0f / M_Sqrt(a.k[1]);
	ret.k[2] = 1.0f / M_Sqrt(a.k[2]);
	ret.k[3] = 1.0f / M_Sqrt(a.k[3]);
	return ret.v;
}

M_FORCEINLINE vec128 M_VSqrt_Est(vec128 _a)
{
	CVec128Access a(_a);
	CVec128Access ret;
	ret.k[0] = M_Sqrt(a.k[0]);
	ret.k[1] = M_Sqrt(a.k[1]);
	ret.k[2] = M_Sqrt(a.k[2]);
	ret.k[3] = M_Sqrt(a.k[3]);
	return ret.v;
}


#endif // M_V128IMP_FLOAT_RCP_SQRT_EST

//----------------------------------------------------------
#ifndef M_V128IMP_FLOAT_RCP_SQRT
#ifndef M_V128IMP_FLOAT_RCP_SQRT_EST
M_FORCEINLINE vec128 M_VRcp(vec128 a) { return M_VRcp_Est(a); }
M_FORCEINLINE vec128 M_VRsq(vec128 a) { return M_VRsq_Est(a); }
M_FORCEINLINE vec128 M_VSqrt(vec128 a) { return M_VSqrt_Est(a); }

#else
// Newton-raphson refinement versions here:

M_FORCEINLINE vec128 M_VRcp(vec128 _a)
{
	vec128 rcp = M_VRcp_Est(_a);

	// Newton-raphson for 1.0 / x
	//     y0 = reciprocal_estimate(x)
	//     y1 = y0 + y0 * (1.0 - x * y0)
	vec128 one = M_VOne();
    vec128 scale = M_VNMSub(_a, rcp, one);
    vec128 res = M_VMAdd(rcp, scale, rcp);

    // Second iteration
    //     y2 = y1 + y1 * (1.0 - x * y1)
    scale = M_VNMSub(_a, res, one);
    vec128 refine = M_VCmpEqMsk(res, res);
    res = M_VMAdd(res, scale, res);
    res = M_VSelMsk(refine, res, rcp);
	return res;
}

M_FORCEINLINE vec128 M_VRsq(vec128 _a)
{
	// Refinement (Newton-Raphson) for 1.0 / sqrt(x)
	//     y0 = reciprocal_sqrt_estimate(x)
	//     y1 = y0 + 0.5 * y0 * (1.0 - x * y0 * y0) 
	//        = y0 + y0 * (0.5 - 0.5 * x * y0 * y0)

	vec128 y0 = M_VRsq_Est(_a);
	vec128 half = M_VHalf();
//	vec128 ahalf = M_VMul(half, _a);
	vec128 zeromask = M_VCmpEqMsk(_a, M_VZero());

	return M_VSelMsk(zeromask, M_VInf(), M_VAdd(y0, M_VMul(y0,M_VSub(half, M_VMul(M_VMul(half, _a), M_VMul(y0, y0))))));
}

M_FORCEINLINE vec128 M_VSqrt(vec128 _a)
{
	vec128 y0 = M_VRsq_Est(_a);
	vec128 half = M_VHalf();
	vec128 ahalf = M_VMul(half, _a);
	vec128 zeromask = M_VCmpEqMsk(_a, M_VZero());

	return M_VSelMsk(zeromask, M_VZero(), M_VMul(M_VMAdd(y0, M_VNMSub(M_VMul(y0, y0), ahalf, half), y0), _a));
}

#endif
#endif // M_V128IMP_FLOAT_RCP_SQRT

// -------------------------------------------------------------------
M_FORCEINLINE vec128 M_VLen3(vec128 _a)
{
	return M_VSqrt(M_VDp3(_a, _a));
}

M_FORCEINLINE vec128 M_VLen3x2(vec128 _a, vec128 _b)
{
	return M_VSqrt(M_VDp3x2(_a, _a, _b, _b));
}

M_FORCEINLINE vec128 M_VLen3x3(vec128 _a, vec128 _b, vec128 _c)
{
	return M_VSqrt(M_VDp3x3(_a, _a, _b, _b, _c, _c));
}

M_FORCEINLINE vec128 M_VLen3x4(vec128 _a, vec128 _b, vec128p _c, vec128p _d)
{
	return M_VSqrt(M_VDp3x4(_a, _a, _b, _b, _c, _c, _d, _d));
}

M_FORCEINLINE vec128 M_VLen4(vec128 _a)
{
	return M_VSqrt(M_VDp4(_a, _a));
}

M_FORCEINLINE vec128 M_VLen4x2(vec128 _a, vec128 _b)
{
	return M_VSqrt(M_VDp4x2(_a, _a, _b, _b));
}

M_FORCEINLINE vec128 M_VLen4x3(vec128 _a, vec128 _b, vec128 _c)
{
	return M_VSqrt(M_VDp4x3(_a, _a, _b, _b, _c, _c));
}

M_FORCEINLINE vec128 M_VLen4x4(vec128 _a, vec128 _b, vec128p _c, vec128p _d)
{
	return M_VSqrt(M_VDp4x4(_a, _a, _b, _b, _c, _c, _d, _d));
}

// -------------------------------------------------------------------
M_FORCEINLINE vec128 M_VLenrcp3_Est(vec128 _a)
{
	return M_VRsq_Est(M_VDp3(_a, _a));
}

M_FORCEINLINE vec128 M_VLenrcp3x2_Est(vec128 _a, vec128 _b)
{
	return M_VRsq_Est(M_VDp3x2(_a, _a, _b, _b));
}

M_FORCEINLINE vec128 M_VLenrcp3x3_Est(vec128 _a, vec128 _b, vec128 _c)
{
	return M_VRsq_Est(M_VDp3x3(_a, _a, _b, _b, _c, _c));
}

M_FORCEINLINE vec128 M_VLenrcp3x4_Est(vec128 _a, vec128 _b, vec128p _c, vec128p _d)
{
	return M_VRsq_Est(M_VDp3x4(_a, _a, _b, _b, _c, _c, _d, _d));
}

M_FORCEINLINE vec128 M_VLenrcp4_Est(vec128 _a)
{
	return M_VRsq_Est(M_VDp4(_a, _a));
}

M_FORCEINLINE vec128 M_VLenrcp4x2_Est(vec128 _a, vec128 _b)
{
	return M_VRsq_Est(M_VDp4x2(_a, _a, _b, _b));
}

M_FORCEINLINE vec128 M_VLenrcp4x3_Est(vec128 _a, vec128 _b, vec128 _c)
{
	return M_VRsq_Est(M_VDp4x3(_a, _a, _b, _b, _c, _c));
}

M_FORCEINLINE vec128 M_VLenrcp4x4_Est(vec128 _a, vec128 _b, vec128p _c, vec128p _d)
{
	return M_VRsq_Est(M_VDp4x4(_a, _a, _b, _b, _c, _c, _d, _d));
}

// -------------------------------------------------------------------
M_FORCEINLINE vec128 M_VLenrcp3(vec128 _a)
{
	return M_VRsq(M_VDp3(_a, _a));
}

M_FORCEINLINE vec128 M_VLenrcp3x2(vec128 _a, vec128 _b)
{
	return M_VRsq(M_VDp3x2(_a, _a, _b, _b));
}

M_FORCEINLINE vec128 M_VLenrcp3x3(vec128 _a, vec128 _b, vec128p _c)
{
	return M_VRsq(M_VDp3x3(_a, _a, _b, _b, _c, _c));
}

M_FORCEINLINE vec128 M_VLenrcp3x4(vec128 _a, vec128 _b, vec128p _c, vec128p _d)
{
	return M_VRsq(M_VDp3x4(_a, _a, _b, _b, _c, _c, _d, _d));
}

M_FORCEINLINE vec128 M_VLenrcp4(vec128 _a)
{
	return M_VRsq(M_VDp4(_a, _a));
}

M_FORCEINLINE vec128 M_VLenrcp4x2(vec128 _a, vec128 _b)
{
	return M_VRsq(M_VDp4x2(_a, _a, _b, _b));
}

M_FORCEINLINE vec128 M_VLenrcp4x3(vec128 _a, vec128 _b, vec128 _c)
{
	return M_VRsq(M_VDp4x3(_a, _a, _b, _b, _c, _c));
}

M_FORCEINLINE vec128 M_VLenrcp4x4(vec128 _a, vec128 _b, vec128p _c, vec128p _d)
{
	return M_VRsq(M_VDp4x4(_a, _a, _b, _b, _c, _c, _d, _d));
}

// -------------------------------------------------------------------
M_FORCEINLINE vec128 M_VNrm3_Est(vec128 _a)
{
	return M_VMul(_a, M_VLenrcp3_Est(_a));
}

#define M_VNrm3x2_Est(_a, _b)	\
{	\
	vec128 rcp = M_VLenrcp3x2_Est(_a, _b);	\
	vec128 a = M_VMul(_a, M_VSplat(rcp, 0));	\
	vec128 b = M_VMul(_b, M_VSplat(rcp, 1));	\
	_a = a;	_b = b;	\
}

#define M_VNrm3x4_Est(_a, _b, _c, _d)	\
{	\
	vec128 nrm_rcp = M_VLenrcp3x4_Est(_a, _b, _c, _d);	\
	vec128 nrm_a = M_VMul(_a, M_VSplat(nrm_rcp, 0));	\
	vec128 nrm_b = M_VMul(_b, M_VSplat(nrm_rcp, 1));	\
	vec128 nrm_c = M_VMul(_c, M_VSplat(nrm_rcp, 2));	\
	vec128 nrm_d = M_VMul(_d, M_VSplat(nrm_rcp, 3));	\
	_a = nrm_a;	_b = nrm_b; _c = nrm_c;	_d = nrm_d;	\
}


M_FORCEINLINE vec128 M_VNrm4_Est(vec128 _a)
{
	return M_VMul(_a, M_VLenrcp4_Est(_a));
}

#define M_VNrm4x2_Est(_a, _b)	\
{	\
	vec128 rcp = M_VLenrcp4x2_Est(_a, _b);	\
	vec128 a = M_VMul(_a, M_VSplat(rcp, 0));	\
	vec128 b = M_VMul(_b, M_VSplat(rcp, 1));	\
	_a = a;	_b = b;	\
}

#define M_VNrm4x4_Est(_a, _b, _c, _d)	\
{	\
	vec128 rcp = M_VLenrcp4x4_Est(_a, _b, _c, _d);	\
	vec128 a = M_VMul(_a, M_VSplat(rcp, 0));	\
	vec128 b = M_VMul(_b, M_VSplat(rcp, 1));	\
	vec128 c = M_VMul(_c, M_VSplat(rcp, 2));	\
	vec128 d = M_VMul(_d, M_VSplat(rcp, 3));	\
	_a = a;	_b = b; _c = c;	_d = d;	\
}

// -------------------------------------------------------------------
M_FORCEINLINE vec128 M_VNrm3(vec128 _a)
{
	return M_VMul(_a, M_VLenrcp3(_a));
}

#define M_VNrm3x2(_a, _b)	\
{	\
	vec128 rcp = M_VLenrcp3x2(_a, _b);	\
	vec128 a = M_VMul(_a, M_VSplat(rcp, 0));	\
	vec128 b = M_VMul(_b, M_VSplat(rcp, 1));	\
	_a = a;	_b = b;	\
}

#define M_VNrm3x3(_a, _b, _c)	\
{	\
	vec128 rcp = M_VLenrcp3x3(_a, _b, _c);	\
	vec128 a = M_VMul(_a, M_VSplat(rcp, 0));	\
	vec128 b = M_VMul(_b, M_VSplat(rcp, 1));	\
	vec128 c = M_VMul(_c, M_VSplat(rcp, 2));	\
	_a = a;	_b = b;	_c = c; \
}

#define M_VNrm3x4(_a, _b, _c, _d)	\
{	\
	vec128 nrm_rcp = M_VLenrcp3x4(_a, _b, _c, _d);	\
	vec128 nrm_a = M_VMul(_a, M_VSplat(nrm_rcp, 0));	\
	vec128 nrm_b = M_VMul(_b, M_VSplat(nrm_rcp, 1));	\
	vec128 nrm_c = M_VMul(_c, M_VSplat(nrm_rcp, 2));	\
	vec128 nrm_d = M_VMul(_d, M_VSplat(nrm_rcp, 3));	\
	_a = nrm_a;	_b = nrm_b; _c = nrm_c;	_d = nrm_d;	\
}


M_FORCEINLINE vec128 M_VNrm4(vec128 _a)
{
	return M_VMul(_a, M_VLenrcp4(_a));
}

#define M_VNrm4x2(_a, _b)	\
{	\
	vec128 rcp = M_VLenrcp4x2(_a, _b);	\
	vec128 a = M_VMul(_a, M_VSplat(rcp, 0));	\
	vec128 b = M_VMul(_b, M_VSplat(rcp, 1));	\
	_a = a;	_b = b;	\
}

#define M_VNrm4x3(_a, _b, _c)	\
{	\
	vec128 rcp = M_VLenrcp4x3(_a, _b, _c);	\
	vec128 a = M_VMul(_a, M_VSplat(rcp, 0));	\
	vec128 b = M_VMul(_b, M_VSplat(rcp, 1));	\
	vec128 c = M_VMul(_b, M_VSplat(rcp, 2));	\
	_a = a;	_b = b;	_c = c; \
}

#define M_VNrm4x4(_a, _b, _c, _d)	\
{	\
	vec128 rcp = M_VLenrcp4x4(_a, _b, _c, _d);	\
	vec128 a = M_VMul(_a, M_VSplat(rcp, 0));	\
	vec128 b = M_VMul(_b, M_VSplat(rcp, 1));	\
	vec128 c = M_VMul(_c, M_VSplat(rcp, 2));	\
	vec128 d = M_VMul(_d, M_VSplat(rcp, 3));	\
	_a = a;	_b = b; _c = c;	_d = d;	\
}

// -------------------------------------------------------------------
M_FORCEINLINE vec128 M_VAbs(vec128 _a)
{
	return M_VMax(M_VNeg(_a), _a);
}

M_FORCEINLINE vec128 M_VNegMsk(vec128 a, vec128 msk)
{
	return M_VSelMsk(msk, M_VNeg(a), a);
}

M_FORCEINLINE vec128 M_VLrp(vec128 a, vec128 b, vec128 t)
{
	return M_VAdd(M_VMul(a, M_VSub(M_VOne(), t)), M_VMul(b, t));	// a*(1-t) + b*t
}

M_FORCEINLINE vec128 M_VClamp01(vec128 a)
{
	return M_VMax(M_VMin(a, M_VOne()), M_VZero());
}

#define M_VClamp(a, _min, _max) M_VMax(M_VMin(a, _max), _min)

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

M_FORCEINLINE vec128 M_VSetW1(vec128 v) 
{
	return M_VSelMsk(M_VConstMsk(1, 1, 1, 0),v,M_VOne());
}
M_FORCEINLINE vec128 M_VSetW0(vec128 v) 
{
//	return M_VSelMsk(M_VConstMsk(1, 1, 1, 0),v,M_VZero());
	return M_VAnd(M_VConst_u32(0xffffffff, 0xffffffff, 0xffffffff, 0x00000000), v);
}


M_FORCEINLINE vec128 M_VMulMat(vec128 _a, CMat4Dfp32p _Mat)
{
	vec128 r0 = _Mat.r[0];
	vec128 r1 = _Mat.r[1];
	vec128 r2 = _Mat.r[2];
	vec128 r3 = _Mat.r[3];
	vec128 tmp=M_VMul(r0,M_VSplatX(_a));
	tmp=M_VMAdd(r1,M_VSplatY(_a),tmp);
	tmp=M_VMAdd(r2,M_VSplatZ(_a),tmp);
	tmp=M_VMAdd(r3,M_VSplatW(_a),tmp);
	return tmp;
}

M_FORCEINLINE vec128 M_VMulMatTransp(vec128 _a, CMat4Dfp32p _Mat)
{
	vec128 r0 = _Mat.r[0];
	vec128 r1 = _Mat.r[1];
	vec128 r2 = _Mat.r[2];
	vec128 r3 = _Mat.r[3];
	vec128 x = M_VDp4x4(_a, r0, _a, r1, _a, r2, _a, r3);
	return x;
}

M_FORCEINLINE vec128 M_VMulMat4x3(vec128 _a, CMat4Dfp32p _Mat)
{
	vec128 r0 = _Mat.r[0];
	vec128 r1 = _Mat.r[1];
	vec128 r2 = _Mat.r[2];
	vec128 r3 = _Mat.r[3];
	vec128 tmp=M_VMul(r0,M_VSplatX(_a));
	tmp=M_VMAdd(r1,M_VSplatY(_a),tmp);
	tmp=M_VMAdd(r2,M_VSplatZ(_a),tmp);
	tmp=M_VMAdd(r3,M_VSplatW(_a),tmp);
	tmp=M_VSetW1(tmp);
	return tmp;
}

M_FORCEINLINE vec128 M_VMulMat3x3(vec128 _a, CMat4Dfp32p _Mat)
{
	vec128 r0 = _Mat.r[0];
	vec128 r1 = _Mat.r[1];
	vec128 r2 = _Mat.r[2];
	vec128 tmp=M_VMul(r0,M_VSplatX(_a));
	tmp=M_VMAdd(r1,M_VSplatY(_a),tmp);
	tmp=M_VMAdd(r2,M_VSplatZ(_a),tmp);
	tmp=M_VSetW0(tmp);
	return tmp;
}

// -------------------------------------------------------------------
#ifdef CPU_VEC128EMU
M_FORCEINLINE void M_VMatMul(const CMat4Dfp32& a, const CMat4Dfp32& b, CMat4Dfp32& c)
{
	// ~290 cyckles w/ x87

	CMat4Dfp32 m;
	for(int x = 0; x < 4; x++)
		for(int y = 0; y < 4; y++)
		{
			m.k[x][y] = a.k[x][0]*b.k[0][y] + a.k[x][1]*b.k[1][y] + a.k[x][2]*b.k[2][y] + a.k[x][3]*b.k[3][y];
		}
	c = m;
}

#else

#ifndef M_V128HINT_NATIVE_DP4
#ifndef M_V128HINT_NATIVE_MADD

M_FORCEINLINE void M_VMatMul(const CMat4Dfp32& a, const CMat4Dfp32& b, CMat4Dfp32& c)
{
	// ~83 cycles w/ SSE2
	// ~420 cycles w/ x87

	// 16 mul
	// 12 add
	// 16 splat(shuffle)

	// ALU Dependency length: 2

	vec128 b0, b1, b2, b3;
	b0 = b.r[0]; b1 = b.r[1]; b2 = b.r[2]; b3 = b.r[3];
	vec128 px0 = M_VMul(M_VSplatX(a.r[0]), b0);
	vec128 py0 = M_VMul(M_VSplatY(a.r[0]), b1);
	vec128 pz0 = M_VMul(M_VSplatZ(a.r[0]), b2);
	vec128 pw0 = M_VMul(M_VSplatW(a.r[0]), b3);
	vec128 px1 = M_VMul(M_VSplatX(a.r[1]), b0);
	vec128 py1 = M_VMul(M_VSplatY(a.r[1]), b1);
	vec128 pz1 = M_VMul(M_VSplatZ(a.r[1]), b2);
	vec128 pw1 = M_VMul(M_VSplatW(a.r[1]), b3);
	vec128 px2 = M_VMul(M_VSplatX(a.r[2]), b0);
	vec128 py2 = M_VMul(M_VSplatY(a.r[2]), b1);
	vec128 pz2 = M_VMul(M_VSplatZ(a.r[2]), b2);
	vec128 pw2 = M_VMul(M_VSplatW(a.r[2]), b3);
	vec128 px3 = M_VMul(M_VSplatX(a.r[3]), b0);
	vec128 py3 = M_VMul(M_VSplatY(a.r[3]), b1);
	vec128 pz3 = M_VMul(M_VSplatZ(a.r[3]), b2);
	vec128 pw3 = M_VMul(M_VSplatW(a.r[3]), b3);
	vec128 r0 = M_VAdd(M_VAdd(px0, py0), M_VAdd(pz0, pw0));
	vec128 r1 = M_VAdd(M_VAdd(px1, py1), M_VAdd(pz1, pw1));
	vec128 r2 = M_VAdd(M_VAdd(px2, py2), M_VAdd(pz2, pw2));
	vec128 r3 = M_VAdd(M_VAdd(px3, py3), M_VAdd(pz3, pw3));

	c.r[0] = r0;
	c.r[1] = r1;
	c.r[2] = r2;
	c.r[3] = r3;
}

M_FORCEINLINE void M_VMatMul3x3(const CMat4Dfp32& a, const CMat4Dfp32& b, CMat4Dfp32& c)
{
	vec128 b0, b1, b2, b3;
	b0 = b.r[0]; b1 = b.r[1]; b2 = b.r[2]; b3 = b.r[3];
	vec128 px0 = M_VMul(M_VSplatX(a.r[0]), b0);
	vec128 py0 = M_VMul(M_VSplatY(a.r[0]), b1);
	vec128 pz0 = M_VMul(M_VSplatZ(a.r[0]), b2);

	vec128 px1 = M_VMul(M_VSplatX(a.r[1]), b0);
	vec128 py1 = M_VMul(M_VSplatY(a.r[1]), b1);
	vec128 pz1 = M_VMul(M_VSplatZ(a.r[1]), b2);

	vec128 px2 = M_VMul(M_VSplatX(a.r[2]), b0);
	vec128 py2 = M_VMul(M_VSplatY(a.r[2]), b1);
	vec128 pz2 = M_VMul(M_VSplatZ(a.r[2]), b2);

	vec128 px3 = M_VMul(M_VSplatX(a.r[3]), b0);
	vec128 py3 = M_VMul(M_VSplatY(a.r[3]), b1);
	vec128 pz3 = M_VMul(M_VSplatZ(a.r[3]), b2);

	vec128 r0 = M_VAdd(M_VAdd(px0, py0), pz0);
	vec128 r1 = M_VAdd(M_VAdd(px1, py1), pz1);
	vec128 r2 = M_VAdd(M_VAdd(px2, py2), pz2);

	c.r[0] = M_VSetW0(r0);
	c.r[1] = M_VSetW0(r1);
	c.r[2] = M_VSetW0(r2);
	c.r[3] = M_VConst(0,0,0,1.0f);
}

#else

M_FORCEINLINE void M_VMatMul(const CMat4Dfp32& a, const CMat4Dfp32& b, CMat4Dfp32& c)
{
	// ~75 cycles w/ SSE2 (4.764 Gflops @ 3.2Ghz)
	// ~350 cycles w/ x87

	// 4 mul
	// 12 madd
	// 16 splat(shuffle)

	// ALU Dependency length: 3

	vec128 b0, b1, b2, b3;
	b0 = b.r[0]; b1 = b.r[1]; b2 = b.r[2]; b3 = b.r[3];

	vec128 r0 = M_VMAdd(M_VSplatW(a.r[0]), b3, M_VMAdd(M_VSplatZ(a.r[0]), b2, M_VMAdd(M_VSplatY(a.r[0]), b1, M_VMul(M_VSplatX(a.r[0]), b0))));
	vec128 r1 = M_VMAdd(M_VSplatW(a.r[1]), b3, M_VMAdd(M_VSplatZ(a.r[1]), b2, M_VMAdd(M_VSplatY(a.r[1]), b1, M_VMul(M_VSplatX(a.r[1]), b0))));
	vec128 r2 = M_VMAdd(M_VSplatW(a.r[2]), b3, M_VMAdd(M_VSplatZ(a.r[2]), b2, M_VMAdd(M_VSplatY(a.r[2]), b1, M_VMul(M_VSplatX(a.r[2]), b0))));
	vec128 r3 = M_VMAdd(M_VSplatW(a.r[3]), b3, M_VMAdd(M_VSplatZ(a.r[3]), b2, M_VMAdd(M_VSplatY(a.r[3]), b1, M_VMul(M_VSplatX(a.r[3]), b0))));

	c.r[0] = r0;
	c.r[1] = r1;
	c.r[2] = r2;
	c.r[3] = r3;
}


M_FORCEINLINE void M_VMatMul3x3(const CMat4Dfp32& a, const CMat4Dfp32& b, CMat4Dfp32& c)
{
	vec128 b0, b1, b2, b3;
	b0 = b.r[0]; b1 = b.r[1]; b2 = b.r[2]; b3 = b.r[3];

	vec128 r0 = M_VMAdd(M_VSplatZ(a.r[0]), b2, M_VMAdd(M_VSplatY(a.r[0]), b1, M_VMul(M_VSplatX(a.r[0]), b0)));
	vec128 r1 = M_VMAdd(M_VSplatZ(a.r[1]), b2, M_VMAdd(M_VSplatY(a.r[1]), b1, M_VMul(M_VSplatX(a.r[1]), b0)));
	vec128 r2 = M_VMAdd(M_VSplatZ(a.r[2]), b2, M_VMAdd(M_VSplatY(a.r[2]), b1, M_VMul(M_VSplatX(a.r[2]), b0)));

	c.r[0] = M_VSetW0(r0);
	c.r[1] = M_VSetW0(r1);
	c.r[2] = M_VSetW0(r2);
	c.r[3] = M_VConst(0,0,0,1.0f);
}

#endif

#else // M_V128HINT_NATIVE_DP4
M_FORCEINLINE void M_VMatMul(const CMat4Dfp32& a, const CMat4Dfp32& b, CMat4Dfp32& c)
{
	// ~220 cycles w/ SSE2
	// ~2238 cycles w/ x87

	// 16 dp4
	// 20 shuffle

	// ALU Dependency length: 1

	vec128 b0, b1, b2, b3;
	b0 = b.r[0]; b1 = b.r[1]; b2 = b.r[2]; b3 = b.r[3];
	M_VTranspose4x4(b0, b1, b2, b3);

	vec128 k00 = M_VDp4(a.r[0], b0);
	vec128 k01 = M_VDp4(a.r[0], b1);
	vec128 k02 = M_VDp4(a.r[0], b2);
	vec128 k03 = M_VDp4(a.r[0], b3);
	vec128 k10 = M_VDp4(a.r[1], b0);
	vec128 k11 = M_VDp4(a.r[1], b1);
	vec128 k12 = M_VDp4(a.r[1], b2);
	vec128 k13 = M_VDp4(a.r[1], b3);
	vec128 k20 = M_VDp4(a.r[2], b0);
	vec128 k21 = M_VDp4(a.r[2], b1);
	vec128 k22 = M_VDp4(a.r[2], b2);
	vec128 k23 = M_VDp4(a.r[2], b3);
	vec128 k30 = M_VDp4(a.r[3], b0);
	vec128 k31 = M_VDp4(a.r[3], b1);
	vec128 k32 = M_VDp4(a.r[3], b2);
	vec128 k33 = M_VDp4(a.r[3], b3);
	vec128 r0 = M_VMrgXY(M_VMrgXY(k00, k02), M_VMrgXY(k01, k03));
	vec128 r1 = M_VMrgXY(M_VMrgXY(k10, k12), M_VMrgXY(k11, k13));
	vec128 r2 = M_VMrgXY(M_VMrgXY(k20, k22), M_VMrgXY(k21, k23));
	vec128 r3 = M_VMrgXY(M_VMrgXY(k30, k32), M_VMrgXY(k31, k33));

	c.r[0] = r0;
	c.r[1] = r1;
	c.r[2] = r2;
	c.r[3] = r3;
}

M_FORCEINLINE void M_VMatMul3x3(const CMat4Dfp32& a, const CMat4Dfp32& b, CMat4Dfp32& c)
{
	// ~220 cycles w/ SSE2
	// ~2238 cycles w/ x87

	// 16 dp4
	// 20 shuffle

	// ALU Dependency length: 1

	vec128 b0, b1, b2, b3;
	b0 = b.r[0]; b1 = b.r[1]; b2 = b.r[2]; b3 = M_VConst(0,0,0,1.0f);
	M_VTranspose4x4(b0, b1, b2, b3);
	b3 = M_VConst(0,0,0,1.0f);

	vec128 k00 = M_VDp4(a.r[0], b0);
	vec128 k01 = M_VDp4(a.r[0], b1);
	vec128 k02 = M_VDp4(a.r[0], b2);
	vec128 k03 = M_VDp4(a.r[0], b3);
	vec128 k10 = M_VDp4(a.r[1], b0);
	vec128 k11 = M_VDp4(a.r[1], b1);
	vec128 k12 = M_VDp4(a.r[1], b2);
	vec128 k13 = M_VDp4(a.r[1], b3);
	vec128 k20 = M_VDp4(a.r[2], b0);
	vec128 k21 = M_VDp4(a.r[2], b1);
	vec128 k22 = M_VDp4(a.r[2], b2);
	vec128 k23 = M_VDp4(a.r[2], b3);
	vec128 r0 = M_VMrgXY(M_VMrgXY(k00, k02), M_VMrgXY(k01, k03));
	vec128 r1 = M_VMrgXY(M_VMrgXY(k10, k12), M_VMrgXY(k11, k13));
	vec128 r2 = M_VMrgXY(M_VMrgXY(k20, k22), M_VMrgXY(k21, k23));

	c.r[0] = M_VSetW0(r0);
	c.r[1] = M_VSetW0(r1);
	c.r[2] = M_VSetW0(r2);
	c.r[3] = M_VConst(0,0,0,1.0f);
}



#endif
#endif

M_FORCEINLINE void M_VMatLrpOrthNrm(const CMat4Dfp32& _Pos, const CMat4Dfp32& _Pos2, CMat4Dfp32& _Dest, vec128p _Time)
{
	vec128 p0x = _Pos.r[0];
	vec128 p0y = _Pos.r[1];
//	vec128 p0z = _Pos.r[2];
	vec128 p0w = _Pos.r[3];
	vec128 p1x = _Pos2.r[0];
	vec128 p1y = _Pos2.r[1];
//	vec128 p1z = _Pos2.r[2];
	vec128 p1w = _Pos2.r[3];
	vec128 t = _Time;
//	vec128 z = M_VZero();

	vec128 dstw = M_VLrp(p0w, p1w, t);
	vec128 dstx = M_VLrp(p0x, p1x, t);
	vec128 dstytmp = M_VLrp(p0y, p1y, t);
	vec128 dstz = M_VXpd(dstytmp, dstx);
	vec128 dsty = M_VXpd(dstz, dstx);
	M_VNrm3x3(dstx, dsty, dstz);

	_Dest.r[0] = dstx;
	_Dest.r[1] = dsty;
	_Dest.r[2] = dstz;
	_Dest.r[3] = dstw;
}

M_FORCEINLINE void M_VMatLrpOrthNrm(const CMat4Dfp32& _Pos, const CMat4Dfp32& _Pos2, CMat4Dfp32& _Dest, fp32& _Time)		// fp32& because we want the option to avoid LHS.
{
	M_VMatLrpOrthNrm(_Pos, _Pos2, _Dest, M_VLdScalar(_Time));
}

// -------------------------------------------------------------------
/*void Multiply(const TQuaternion& _Quat2, TQuaternion& _QDest) const
{
	T a0 = k[0];
	T a1 = k[1];
	T a2 = k[2];
	T a3 = k[3];
	T b0 = _Quat2.k[0];
	T b1 = _Quat2.k[1];
	T b2 = _Quat2.k[2];
	T b3 = _Quat2.k[3];
	_QDest.k[0] = a3*b0 + a0*b3 + a1*b2 - a2*b1;
	_QDest.k[1] = a3*b1 + a1*b3 + a2*b0 - a0*b2;
	_QDest.k[2] = a3*b2 + a2*b3 + a0*b1 - a1*b0;
	_QDest.k[3] = a3*b3 - a0*b0 - a1*b1 - a2*b2;
}*/

M_FORCEINLINE vec128 M_VQuatMul(vec128 a, vec128 b)
{
	vec128 s0 = M_VMul(M_VSplatW(a), b);
	vec128 s1 = M_VMul(M_VShuf(a, M_VSHUF(0,1,2,0)), M_VShuf(b, M_VSHUF(3,3,3,0)));
	vec128 s2 = M_VMul(M_VShuf(a, M_VSHUF(1,2,0,1)), M_VShuf(b, M_VSHUF(2,0,1,1)));
	vec128 s3 = M_VMul(M_VShuf(a, M_VSHUF(2,0,1,2)), M_VShuf(b, M_VSHUF(1,2,0,2)));
	vec128 negmsk = M_VConstMsk(0,0,0,1);
	s1 = M_VNegMsk(s1, negmsk);
	s2 = M_VNegMsk(s2, negmsk);
	return M_VAdd(M_VAdd(s0, s1), M_VSub(s2, s3));
}

#if !defined(M_COMPILING_ON_VPU)
M_FORCEINLINE void M_VCnv_Quat_Mat(vec128 a, CMat4Dfp32& m)
{
/*		T xs,ys,zs,wx,wy,wz,xx,xy,xz,yy,yz,zz;
		T s = T(2.0f);

		xs = k[0]*s;  ys = k[1]*s;  zs = k[2]*s;
		wx = k[3]*xs; wy = k[3]*ys; wz = k[3]*zs;
		xx = k[0]*xs; xy = k[0]*ys; xz = k[0]*zs;
		yy = k[1]*ys; yz = k[1]*zs; zz = k[2]*zs;

		_Mat.k[0][0] = (T(1.0f) - (yy+zz));
		_Mat.k[0][1] = (xy-wz);
		_Mat.k[0][2] = (xz+wy);

		_Mat.k[1][0] = (xy+wz);
		_Mat.k[1][1] = (T(1.0f) - (xx+zz));
		_Mat.k[1][2] = (yz-wx);

		_Mat.k[2][0] = (xz-wy);
		_Mat.k[2][1] = (yz+wx);
		_Mat.k[2][2] = (T(1.0f) - (xx+yy));
		
		
		ADD R4, c[A0.x+$MP+0], c[A0.x+$MP+0];
		MUL R5, R4.xyzz, c[A0.x+$MP+0].xxxx;	// xx = k[0]*xs; xy = k[0]*ys; xz = k[0]*zs;
		MUL R6, R4.yzzz, c[A0.x+$MP+0].yyzz;	// yy = k[1]*ys; yz = k[1]*zs; zz = k[2]*zs;
		MUL R4, R4.xyzz, c[A0.x+$MP+0].wwww;	// wx = k[3]*xs; wy = k[3]*ys; wz = k[3]*zs;
		ADD R8.x, R6.xxxx, R6.zzzz;				// yy+zz
		ADD R8.x, c[$BASE+8].y, -R8.x;				// 1 - yy+zz
		ADD R9, R5.yzzz, R4.zyyy;				// R9.x = xy+wz, R9.y = xz+wy
		ADD R10, c[$BASE+8].y, -R5.x;					// R10 = 1 - xx
		MOV R8.z, R9.y;							// R8.z = R9.y = xz+wy
		ADD R10, R10, -R6.zxxx;					// R10.x = 1 - xx - zz, R10.yzw = 1 - xx - yy
		ADD R8.y, R5.y, -R4.z;
		ADD R9.z, R6.y, -R4.x;
		MOV R9.y, R10.x;
		ADD R10.x, R5.z, -R4.y;
		ADD R10.y, R6.y, R4.x;
		
		*/

	// Uuuuuuuuuuuugh..........
}
#endif

// -------------------------------------------------------------------
// Signed Integer arithmetics:
#ifndef M_V128IMP_INTEGERARITHMETICS
M_FORCEINLINE vec128 M_VAdds_i8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ki8[i] = Min(0x7f, a.ki8[i] + b.ki8[i]);
	return r;
}
M_FORCEINLINE vec128 M_VAdds_i16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ki16[i] = Min(0x7fff, a.ki16[i] + b.ki16[i]);
	return r;
}
M_FORCEINLINE vec128 M_VSubs_i8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ki8[i] = Max(-0x80, a.ki8[i] - b.ki8[i]);
	return r;
}
M_FORCEINLINE vec128 M_VSubs_i16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ki16[i] = Max(-0x8000, a.ki16[i] - b.ki16[i]);
	return r;
}
M_FORCEINLINE vec128 M_VMin_i8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ki8[i] = Min(a.ki8[i], b.ki8[i]);
	return r;
}
M_FORCEINLINE vec128 M_VMin_i16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ki16[i] = Min(a.ki16[i], b.ki16[i]);
	return r;
}
M_FORCEINLINE vec128 M_VMax_i8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ki8[i] = Max(a.ki8[i], b.ki8[i]);
	return r;
}
M_FORCEINLINE vec128 M_VMax_i16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ki16[i] = Max(a.ki16[i], b.ki16[i]);
	return r;
}

// -------------------------------------------------------------------
// Unsigned Integer arithmetics:
M_FORCEINLINE vec128 M_VAdd_u8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ku8[i] = a.ku8[i] + b.ku8[i];
	return r;
}
M_FORCEINLINE vec128 M_VAdds_u8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ku8[i] = (uint8)Min(0xff, (int)a.ku8[i] + (int)b.ku8[i]);
	return r;
}
M_FORCEINLINE vec128 M_VAdd_u16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ku16[i] = a.ku16[i] + b.ku16[i];
	return r;
}
M_FORCEINLINE vec128 M_VAdds_u16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ku16[i] = (uint16)Min(0xffff, (int)a.ku16[i] + (int)b.ku16[i]);
	return r;
}
M_FORCEINLINE vec128 M_VAdd_u32(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 4; i++)
		r.ku32[i] = a.ku32[i] + b.ku32[i];
	return r;
}

M_FORCEINLINE vec128 M_VSub_u8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ku8[i] = a.ku8[i] - b.ku8[i];
	return r;
}
M_FORCEINLINE vec128 M_VSubs_u8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ku8[i] = (uint8)Max(0, (int)a.ku8[i] + (int)b.ku8[i]);
	return r;
}
M_FORCEINLINE vec128 M_VSub_u16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ku16[i] = a.ku16[i] - b.ku16[i];
	return r;
}
M_FORCEINLINE vec128 M_VSubs_u16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ku16[i] = (uint16)Max(0, (int)a.ku16[i] + (int)b.ku16[i]);
	return r;
}
M_FORCEINLINE vec128 M_VSub_u32(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 4; i++)
		r.ku32[i] = a.ku32[i] - b.ku32[i];
	return r;
}

M_FORCEINLINE vec128 M_VMin_u8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ku8[i] = Min(a.ku8[i], b.ku8[i]);
	return r;
}
M_FORCEINLINE vec128 M_VMin_u16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ku16[i] = Min(a.ku16[i], b.ku16[i]);
	return r;
}
M_FORCEINLINE vec128 M_VMax_u8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ku8[i] = Max(a.ku8[i], b.ku8[i]);
	return r;
}
M_FORCEINLINE vec128 M_VMax_u16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ku16[i] = Max(a.ku16[i], b.ku16[i]);
	return r;
}

M_FORCEINLINE vec128 M_VSar_i16(vec128 _a, uint b)
{
	CVec128Access a(_a);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ki16[i] = a.ki16[i] >> b;
	return r;
}

M_FORCEINLINE vec128 M_VShr_u16(vec128 _a, uint b)
{
	CVec128Access a(_a);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ku16[i] = a.ku16[i] >> b;
	return r;
}

M_FORCEINLINE vec128 M_VShl_u16(vec128 _a, uint b)
{
	CVec128Access a(_a);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ku16[i] = a.ku16[i] << b;
	return r;
}

M_FORCEINLINE vec128 M_VSar_i32(vec128 _a, uint b)
{
	CVec128Access a(_a);
	CVec128Access r;
	for(int i = 0; i < 4; i++)
		r.ki32[i] = a.ki32[i] >> b;
	return r;
}

M_FORCEINLINE vec128 M_VShr_u32(vec128 _a, uint b)
{
	CVec128Access a(_a);
	CVec128Access r;
	for(int i = 0; i < 4; i++)
		r.ku32[i] = a.ku32[i] >> b;
	return r;
}

M_FORCEINLINE vec128 M_VShl_u32(vec128 _a, uint b)
{
	CVec128Access a(_a);
	CVec128Access r;
	for(int i = 0; i < 4; i++)
		r.ku32[i] = a.ku32[i] << b;
	return r;
}
#endif // M_V128IMP_INTEGERARITHMETICS
// -------------------------------------------------------------------
// Integer compare:
#ifndef M_V128IMP_INTEGERCOMPARE

M_FORCEINLINE vec128 M_VCmpEqMsk_u8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ku8[i] = (a.ku8[i] == b.ku8[i]) ? 0xff : 0x00;
	return r;
}
M_FORCEINLINE vec128 M_VCmpGTMsk_u8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ku8[i] = (a.ku8[i] > b.ku8[i]) ? 0xff : 0x00;
	return r;
}
M_FORCEINLINE vec128 M_VCmpLTMsk_u8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ku8[i] = (a.ku8[i] < b.ku8[i]) ? 0xff : 0x00;
	return r;
}
M_FORCEINLINE vec128 M_VCmpEqMsk_i8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ki8[i] = (a.ki8[i] == b.ki8[i]) ? 0xff : 0x00;
	return r;
}
M_FORCEINLINE vec128 M_VCmpGTMsk_i8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ki8[i] = (a.ki8[i] > b.ki8[i]) ? 0xff : 0x00;
	return r;
}
M_FORCEINLINE vec128 M_VCmpLTMsk_i8(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 16; i++)
		r.ki8[i] = (a.ki8[i] < b.ki8[i]) ? 0xff : 0x00;
	return r;
}
M_FORCEINLINE vec128 M_VCmpEqMsk_u16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ku16[i] = (a.ku16[i] == b.ku16[i]) ? 0xffff : 0x0000;
	return r;
}
M_FORCEINLINE vec128 M_VCmpGTMsk_u16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ku16[i] = (a.ku16[i] > b.ku16[i]) ? 0xffff : 0x0000;
	return r;
}
M_FORCEINLINE vec128 M_VCmpLTMsk_u16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ku16[i] = (a.ku16[i] < b.ku16[i]) ? 0xffff : 0x0000;
	return r;
}
M_FORCEINLINE vec128 M_VCmpEqMsk_i16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ki16[i] = (a.ki16[i] == b.ki16[i]) ? 0xffff : 0x0000;
	return r;
}
M_FORCEINLINE vec128 M_VCmpGTMsk_i16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ki16[i] = (a.ki16[i] > b.ki16[i]) ? 0xffff : 0x0000;
	return r;
}
M_FORCEINLINE vec128 M_VCmpLTMsk_i16(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 8; i++)
		r.ki16[i] = (a.ki16[i] < b.ki16[i]) ? 0xffff : 0x0000;
	return r;
}
M_FORCEINLINE vec128 M_VCmpEqMsk_u32(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 4; i++)
		r.ku32[i] = (a.ku32[i] == b.ku32[i]) ? 0xffffffff : 0x0000;
	return r;
}
M_FORCEINLINE vec128 M_VCmpGTMsk_u32(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 4; i++)
		r.ku32[i] = (a.ku32[i] > b.ku32[i]) ? 0xffffffff : 0x0000;
	return r;
}
M_FORCEINLINE vec128 M_VCmpLTMsk_u32(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 4; i++)
		r.ku32[i] = (a.ku32[i] < b.ku32[i]) ? 0xffffffff : 0x0000;
	return r;
}
M_FORCEINLINE vec128 M_VCmpEqMsk_i32(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 4; i++)
		r.ki32[i] = (a.ki32[i] == b.ki32[i]) ? 0xffffffff : 0x0000;
	return r;
}
M_FORCEINLINE vec128 M_VCmpGTMsk_i32(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 4; i++)
		r.ki32[i] = (a.ki32[i] > b.ki32[i]) ? 0xffffffff : 0x0000;
	return r;
}
M_FORCEINLINE vec128 M_VCmpLTMsk_i32(vec128 _a, vec128 _b)
{
	CVec128Access a(_a);
	CVec128Access b(_b);
	CVec128Access r;
	for(int i = 0; i < 4; i++)
		r.ki32[i] = (a.ki32[i] < b.ki32[i]) ? 0xffffffff : 0x0000;
	return r;
}

M_FORCEINLINE uint M_VMask_u8_EmuOnly(vec128 _a)
{
	CVec128Access a(_a);
	return 
		(uint(a.ku8[0] >> 7) << 0) |
		(uint(a.ku8[1] >> 7) << 1) |
		(uint(a.ku8[2] >> 7) << 2) |
		(uint(a.ku8[3] >> 7) << 3) |
		(uint(a.ku8[4] >> 7) << 4) |
		(uint(a.ku8[5] >> 7) << 5) |
		(uint(a.ku8[6] >> 7) << 6) |
		(uint(a.ku8[7] >> 7) << 7) |
		(uint(a.ku8[8] >> 7) << 8) |
		(uint(a.ku8[9] >> 7) << 9) |
		(uint(a.ku8[10] >> 7) << 10) |
		(uint(a.ku8[11] >> 7) << 11) |
		(uint(a.ku8[12] >> 7) << 12) |
		(uint(a.ku8[13] >> 7) << 13) |
		(uint(a.ku8[14] >> 7) << 14) |
		(uint(a.ku8[15] >> 7) << 15);
}

M_FORCEINLINE uint M_VCmpAllEq_u8(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpEqMsk_u8(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllGT_u8(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpGTMsk_u8(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllLT_u8(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpLTMsk_u8(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllEq_i8(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpEqMsk_i8(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllGT_i8(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpGTMsk_i8(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllLT_i8(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpLTMsk_i8(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllEq_u16(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpEqMsk_u16(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllGT_u16(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpGTMsk_u16(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllLT_u16(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpLTMsk_u16(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllEq_i16(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpEqMsk_i16(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllGT_i16(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpGTMsk_i16(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllLT_i16(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpLTMsk_i16(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllEq_u32(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpEqMsk_u32(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllGT_u32(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpGTMsk_u32(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllLT_u32(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpLTMsk_u32(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllEq_i32(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpEqMsk_i32(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllGT_i32(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpGTMsk_i32(a, b)) == 0xffff; }
M_FORCEINLINE uint M_VCmpAllLT_i32(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpLTMsk_i32(a, b)) == 0xffff; }

M_FORCEINLINE uint M_VCmpAnyEq_u8(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpEqMsk_u8(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyGT_u8(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpGTMsk_u8(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyLT_u8(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpLTMsk_u8(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyEq_i8(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpEqMsk_i8(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyGT_i8(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpGTMsk_i8(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyLT_i8(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpLTMsk_i8(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyEq_u16(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpEqMsk_u16(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyGT_u16(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpGTMsk_u16(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyLT_u16(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpLTMsk_u16(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyEq_i16(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpEqMsk_i16(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyGT_i16(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpGTMsk_i16(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyLT_i16(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpLTMsk_i16(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyEq_u32(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpEqMsk_u32(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyGT_u32(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpGTMsk_u32(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyLT_u32(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpLTMsk_u32(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyEq_i32(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpEqMsk_i32(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyGT_i32(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpGTMsk_i32(a, b)) != 0; }
M_FORCEINLINE uint M_VCmpAnyLT_i32(vec128 a, vec128 b) { return M_VMask_u8_EmuOnly(M_VCmpLTMsk_i32(a, b)) != 0; }

#endif // M_V128IMP_INTEGERCOMPARE

// -------------------------------------------------------------------
#if !defined(M_COMPILING_ON_VPU)
M_FORCEINLINE CStr M_VStr(vec128 a)
{
	CVec128Access v;
	v.v = a;
	return CStrF("(%.10f, %.10f, %.10f, %.10f)", v.k[0], v.k[1], v.k[2], v.k[3]);
}

M_FORCEINLINE CStr M_VStr_u32(vec128 a)
{
	CVec128Access v;
	v.v = a;
	return CStrF("(%.8x, %.8x, %.8x, %.8x)", v.ku32[0], v.ku32[1], v.ku32[2], v.ku32[3]);
}

M_FORCEINLINE CStr M_VStr_i32(vec128 a)
{
	CVec128Access v;
	v.v = a;
	return CStrF("(%d, %d, %d, %d)", v.ki32[0], v.ki32[1], v.ki32[2], v.ki32[3]);
}

M_FORCEINLINE CStr M_VStr_u16(vec128 a)
{
	CVec128Access v;
	v.v = a;
	return CStrF("(%.4x, %.4x, %.4x, %.4x, %.4x, %.4x, %.4x, %.4x)", v.ku16[0], v.ku16[1], v.ku16[2], v.ku16[3], v.ku16[4], v.ku16[5], v.ku16[6], v.ku16[7]);
}

M_FORCEINLINE CStr M_VStr_i16(vec128 a)
{
	CVec128Access v;
	v.v = a;
	return CStrF("(%d, %d, %d, %d, %d, %d, %d, %d)", v.ki16[0], v.ki16[1], v.ki16[2], v.ki16[3], v.ki16[4], v.ki16[5], v.ki16[6], v.ki16[7]);
}

M_FORCEINLINE CStr M_VStr_u8(vec128 a)
{
	CVec128Access v;
	v.v = a;
	return CStrF("(%.2x, %.2x, %.2x, %.2x, %.2x, %.2x, %.2x, %.2x, %.2x, %.2x, %.2x, %.2x, %.2x, %.2x, %.2x, %.2x)", 
		v.ku8[0], v.ku8[1], v.ku8[2], v.ku8[3], v.ku8[4], v.ku8[5], v.ku8[6], v.ku8[7],
		v.ku8[8], v.ku8[9], v.ku8[10], v.ku8[11], v.ku8[12], v.ku8[13], v.ku8[14], v.ku8[15]);
}

M_FORCEINLINE CStr M_VStr_i8(vec128 a)
{
	CVec128Access v;
	v.v = a;
	return CStrF("(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)", 
		v.ki8[0], v.ki8[1], v.ki8[2], v.ki8[3], v.ki8[4], v.ki8[5], v.ki8[6], v.ki8[7],
		v.ki8[8], v.ki8[9], v.ki8[10], v.ki8[11], v.ki8[12], v.ki8[13], v.ki8[14], v.ki8[15]);
}
#endif

/*M_FORCEINLINE CStr M_VStr_u16(vec128 a)
{
	CVec128Access v;
	v.v = a;
	return XSF("(%.8x, %.8x, %.8x, %.8x)", v.ku32[0], v.ku32[1], v.ku32[2], v.ku32[3]);
}

M_FORCEINLINE CStr M_VStr_i16(vec128 a)
{
	CVec128Access v;
	v.v = a;
	return XSF("(%d, %d, %d, %d)", v.ki32[0], v.ki32[1], v.ki32[2], v.ki32[3]);
}*/


#if !defined(M_V128IMP_FLOAT_OPERATORS) && !defined(PLATFORM_XENON)

M_FORCEINLINE vec128 operator- (vec128 _a, vec128 _b)
{
	return M_VSub(_a, _b);
}

M_FORCEINLINE vec128 operator- (vec128 _a)
{
	return M_VNeg(_a);
}

M_FORCEINLINE vec128 operator+ (vec128 _a, vec128 _b)
{
	return M_VAdd(_a, _b);
}

#endif
/*
M_FORCEINLINE vec128 operator* (vec128 _a, vec128 _b)
{
	return M_VDp4(_a, _b);
}

M_FORCEINLINE vec128 operator~ (vec128 _a, vec128 _b)
{
	return M_VDp4(_a, _b);
}

M_FORCEINLINE vec128 operator/ (vec128 _a, vec128 _b)
{
	return M_VXpd(_a, _b);
}

M_FORCEINLINE void operator+= (vec128& _a, vec128 _b)
{
	_a = M_VAdd(_a, _b);
}

M_FORCEINLINE void operator-= (vec128& _a, vec128 _b)
{
	_a = M_VSub(_a, _b);
}
*/

#ifndef M_V128IMP_SIN
static M_FORCEINLINE vec128 M_VSin_Est(vec128 _ang)
{
	vec128 negone = M_VSub(M_VZero(), M_VOne());
	vec128 pi = M_VScalar(3.14159265358979323846264338327950288419716939937510582097494459f);
	vec128 selmask = M_VCmpGEMsk(_ang, M_VZero());
	vec128 sign = M_VSelMsk(selmask, M_VOne(), negone);
	vec128 absang = M_VMul(_ang, sign);	// Negate ang if negative
	vec128 x1 = M_VSub(absang, M_VMul(M_VTrunc(M_VMul(absang, M_VScalar(1.0f / 6.283185307179586476925286766559f))), M_VScalar(6.283185307179586476925286766559f)));	// 0-2pi) range

	{
		// Limit range to 0-pi)
		vec128 withinpimask = M_VCmpGTMsk(x1, pi);
		vec128 withinpiscale = M_VSelMsk(withinpimask, negone, M_VOne());
		vec128 withinpisub = M_VSelMsk(withinpimask, pi, M_VZero());

		sign = M_VMul(sign, withinpiscale);
		x1 = M_VSub(x1, withinpisub);
	}

	{
		// Limit range to 0-pi/2)
		vec128 withinhalfpimask = M_VCmpGTMsk(x1, M_VScalar(1.5707963267948966192313216916398f));
		vec128 invang = M_VSub(pi, x1);
		x1 = M_VSelMsk(withinhalfpimask, invang, x1);
	}

	vec128 s2 = M_VScalar(-0.16666f);
	vec128 s3 = M_VScalar(0.0083143f);
	vec128 s4 = M_VScalar(-0.00018542f);

	vec128 x2 = M_VMul(x1, x1);
	vec128 x3 = M_VMul(x2, x1);
	vec128 x5 = M_VMul(x2, x3);
	vec128 x7 = M_VMul(x2, x5);

	return M_VMul(sign, M_VMAdd(s4, x7, M_VMAdd(s3, x5, M_VMAdd(s2, x3, x1))));
}

static M_FORCEINLINE vec128 M_VCos_Est(vec128 _ang)
{
	return M_VSin_Est(M_VAdd(_ang, M_VScalar(1.5707963267948966192313216916398f)));
}

static M_FORCEINLINE vec128 M_VSinCos_Est(vec128 _ang)
{
	return M_VSin_Est(M_VAdd(_ang, M_VConst(0.0f, 1.5707963267948966192313216916398f, 0.0f, 1.5707963267948966192313216916398f)));
}

#endif	// M_V128IMP_SIN


M_FORCEINLINE vec128 M_VWrap(vec128 _a, vec128 _mod, vec128 _modrcp)
{
	vec128 isneg = M_VCmpLTMsk(_a, M_VZero());
	vec128 x = M_VMul(_a, _modrcp);
	vec128 xif = M_VTrunc(x);
	vec128 xr = M_VSub(x, xif);
	xr = M_VMul(xr, _mod);
	xr = M_VSelMsk(isneg, M_VAdd(xr, _mod), xr);
	return xr;
}

M_FORCEINLINE vec128 M_VWrapOne(vec128 _a)
{
	vec128 isneg = M_VCmpLTMsk(_a, M_VZero());
	vec128 x = _a;
	vec128 xif = M_VTrunc(x);
	vec128 xr = M_VSub(x, xif);
	xr = M_VSelMsk(isneg, M_VAdd(xr, M_VOne()), xr);
	return xr;
}

M_FORCEINLINE vec128 M_VWrap2pi(vec128 _a)
{
	vec128 isneg = M_VCmpLTMsk(_a, M_VZero());
	vec128 x = M_VMul(_a, M_V2PiRcp());
	vec128 xif = M_VTrunc(x);
	vec128 xr = M_VSub(x, xif);
	vec128 pi2 = M_V2Pi();
	xr = M_VMul(xr, pi2);
	xr = M_VSelMsk(isneg, M_VAdd(xr, pi2), xr);
	return xr;
}

M_FORCEINLINE vec128 M_VCos(vec128 _a)
{
	vec128 x = M_VWrap2pi(_a);

	vec128 x2 = M_VMul(x, x);
	vec128 x4 = M_VMul(x2, x2);
	vec128 x6 = M_VMul(x2, x4);
	vec128 x8 = M_VMul(x4, x4);
	vec128 x10 = M_VMul(x6, x4);
	vec128 x12 = M_VMul(x6, x6);
	vec128 x14 = M_VMul(x6, x8);
	vec128 x16 = M_VMul(x8, x8);
	vec128 x18 = M_VMul(x10, x8);
	vec128 x20 = M_VMul(x10, x10);
	vec128 x22 = M_VMul(x10, x12);

	vec128 c0 = M_VOne();
	vec128 c1 = M_VScalar(-0.5f);
	vec128 c2 = M_VScalar(4.166666667e-2f);
	vec128 c3 = M_VScalar(-1.388888889e-3f);
	vec128 c4 = M_VScalar(2.480158730e-5f);
	vec128 c5 = M_VScalar(-2.755731922e-7f);
	vec128 c6 = M_VScalar(2.087675699e-9f);
	vec128 c7 = M_VScalar(-1.147074560e-11f);
	vec128 c8 = M_VScalar(4.779477332e-14f);
	vec128 c9 = M_VScalar(-1.561920697e-16f);
	vec128 c10 = M_VScalar(4.110317623e-19f);
	vec128 c11 = M_VScalar(-8.896791392e-22f);

/*	XMGLOBALCONST XMVECTOR  g_XMCosCoefficients0    = {1.0f, -0.5f, 4.166666667e-2f, -1.388888889e-3f};
	XMGLOBALCONST XMVECTOR  g_XMCosCoefficients1    = {2.480158730e-5f, -2.755731922e-7f, 2.087675699e-9f, -1.147074560e-11f};
	XMGLOBALCONST XMVECTOR  g_XMCosCoefficients2    = {4.779477332e-14f, -1.561920697e-16f, 4.110317623e-19f, -8.896791392e-22f};
*/
	vec128 r0 = M_VMAdd(c1, x2, c0);
	r0 = M_VMAdd(c2, x4, r0);
	r0 = M_VMAdd(c3, x6, r0);
	vec128 r4 = M_VMul(c5, x10);
	r4 = M_VMAdd(c6, x12, r4);
	r4 = M_VMAdd(c7, x14, r4);
	vec128 r3 = M_VMul(c4, x8);
	r3 = M_VMAdd(c8, x16, r3);
	vec128 r8 = M_VMul(c9, x18);
	r8 = M_VMAdd(c10, x20, r8);
	r8 = M_VMAdd(c11, x22, r8);

	r0 = M_VAdd(r0, r3);
	r4 = M_VAdd(r4, r8);

	vec128 c = M_VAdd(r0, r4);

	return c;
}

M_FORCEINLINE vec128 M_VSinCos(vec128 _a)
{
	return M_VCos(M_VAdd(_a, M_VConst(-3.14159265358979323f/4.0f, 0, -3.14159265358979323f/4.0f, 0)));
}

M_FORCEINLINE vec128 M_VSinSinCosCos(vec128 _a)
{
	return M_VCos(M_VAdd(_a, M_VConst(-3.14159265358979323f/4.0f, -3.14159265358979323f/4.0f, 0, 0)));
}

M_FORCEINLINE vec128 M_VCnv_AA_Quat(vec128 _aa)
{
	vec128 lenrcp = M_VLenrcp3(_aa);
	vec128 halfangle = M_VMul(M_VHalf(), M_VSplatW(_aa));
	vec128 sincos = M_VSinCos(halfangle);
	vec128 quat = M_VSelMsk(M_VConstMsk(1,1,1,0), M_VMul(_aa, M_VMul(lenrcp, M_VSplatX(sincos))), sincos);
	return quat;
}

M_FORCEINLINE void M_VCnv_AA_Quat_x2(vec128 _aa0, vec128 _aa1, vec128& _quat0, vec128& _quat1)
{
	vec128 lenrcp = M_VLenrcp3x2(_aa0, _aa1);
	vec128 half = M_VHalf();
	vec128 halfangles = M_VMul(half, M_VMrgL(M_VSplatW(_aa0), M_VSplatW(_aa1)));
	vec128 sincos = M_VSinSinCosCos(halfangles);
	vec128 msk = M_VConstMsk(1,1,1,0);
	vec128 sincoslenrcp = M_VMul(sincos, lenrcp);
	vec128 quat0 = M_VSelMsk(msk, M_VMul(_aa0, M_VSplatX(sincoslenrcp)), M_VSplatZ(sincos));
	vec128 quat1 = M_VSelMsk(msk, M_VMul(_aa1, M_VSplatY(sincoslenrcp)), sincos);
	_quat0 = quat0;
	_quat1 = quat1;
}

M_FORCEINLINE void M_VCnv_AA_Quat_x4(vec128 _aa0, vec128 _aa1, vec128 _aa2, vec128p _aa3, vec128& _quat0, vec128& _quat1, vec128& _quat2, vec128& _quat3)
{
	vec128 lenrcp = M_VLenrcp3x4(_aa0, _aa1, _aa2, _aa3);
	vec128 half = M_VHalf();
	vec128 halfangles01 = M_VMul(half, M_VMrgL(M_VSplatW(_aa0), M_VSplatW(_aa1)));
	vec128 halfangles23 = M_VMul(half, M_VMrgL(M_VSplatW(_aa2), M_VSplatW(_aa3)));
	vec128 sincos01 = M_VSinSinCosCos(halfangles01);
	vec128 sincos23 = M_VSinSinCosCos(halfangles23);
	vec128 msk = M_VConstMsk(1,1,1,0);
	vec128 sincos01lenrcp = M_VMul(sincos01, M_VShuf(lenrcp, M_VSHUF(0,1,0,1)));
	vec128 sincos23lenrcp = M_VMul(sincos23, M_VShuf(lenrcp, M_VSHUF(2,3,2,3)));
	vec128 quat0 = M_VSelMsk(msk, M_VMul(_aa0, M_VSplatX(sincos01lenrcp)), M_VSplatZ(sincos01));
	vec128 quat1 = M_VSelMsk(msk, M_VMul(_aa1, M_VSplatY(sincos01lenrcp)), sincos01);
	vec128 quat2 = M_VSelMsk(msk, M_VMul(_aa2, M_VSplatX(sincos23lenrcp)), M_VSplatZ(sincos23));
	vec128 quat3 = M_VSelMsk(msk, M_VMul(_aa3, M_VSplatY(sincos23lenrcp)), sincos23);
	_quat0 = quat0;
	_quat1 = quat1;
	_quat2 = quat2;
	_quat3 = quat3;
}

#endif // __INCLUDE_XCC_MATH_VEC128_EMU

