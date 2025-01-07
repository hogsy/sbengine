
#ifdef CPU_MIPS_PS2

static M_INLINE fp4 M_Fabs(fp4 _Val) { fp4 ret; asm("abs.s	ret,_Val"); return ret; }
static M_INLINE fp4 M_Sqrt(fp4 _Val) { fp4 ret; asm("sqrt.s ret,_Val"); return ret; }
static M_INLINE fp4 M_RSqrt(fp4 _Val, fp4 _Arg) { fp4 ret; asm("rsqrt.s ret,_Val,_Arg"); return ret; }
static M_INLINE fp4 M_InvSqrt( fp4 _Val ) { return M_RSqrt( 1.0f, _Val ); }
const fp4 MIPS_Sin(fp4 _Val);
static M_INLINE const fp4 M_Sin(fp4 _Val)     { return MIPS_Sin(_Val); }
static M_INLINE const fp4 M_Cos(fp4 _Val)     { return MIPS_Sin(_Val + 1.5707963267948966192313216916398f); }
static M_INLINE const fp4 M_Tan(fp4 _Val)     { return tanf(_Val); }
static M_INLINE const fp4 M_ASin(fp4 _Val)    { return asinf(_Val); }
static M_INLINE const fp4 M_ACos(fp4 _Val)    { return acosf(_Val); }
static M_INLINE const fp4 M_ATan(fp4 _Val)    { return atanf(_Val); }
static M_INLINE const fp4 M_ATan2(fp4 _Val, fp4 _Val2)    { return atan2f(_Val, _Val2); }
static M_INLINE const fp4 M_Cosh(fp4 _Val)	  { return coshf(_Val); }
static M_INLINE const fp4 M_Sinh(fp4 _Val)	  { return sinhf(_Val); }
static M_INLINE const fp4 M_Tanh(fp4 _Val)	  { return tanhf(_Val); }
static M_INLINE const fp4 M_SinFast(fp4 _Val) { return M_Sin(_Val); }				// Input must be in range 0..2pi
static M_INLINE const fp4 M_CosFast(fp4 _Val) { return M_Cos(_Val); }				// Input must be in range 0..2pi
static M_INLINE const fp4 M_FMod(fp4 _a, fp4 _b)	{ return fmodf(_a, _b); }
static M_INLINE const fp4 M_AToF(const char *_Str) { return atoff(_Str); }
static M_INLINE const fp4 M_Log10(fp4 _Val)	  { return log10f(_Val); }
static M_INLINE const fp4 M_Log(fp4 _Val)	  { return logf(_Val); }
static M_INLINE const fp4 M_Exp(fp4 _Val)	  { return expf(_Val); }
static M_INLINE const fp4 M_Pow(fp4 _V,fp4 _V2)	  { return powf(_V,_V2); }

static M_INLINE const fp4 Clamp01(fp4 _Val)
{
	return Max( 0.0f, Min( 1.0f, _Val ) );
}

static M_INLINE int TruncToInt( fp4 _Val )
{
	return (int)_Val;
}

static M_INLINE fp4 Fraction(fp4 _Val)
{
	return _Val - TruncToInt(_Val);
}

static M_INLINE int RoundToInt( fp4 _Val )
{
	int Ret;
	fp4 fp1, fp2;
	int32 tmp1, tmp2, tmp3;
	asm (	"
				lui		tmp2, 0x8000		// Load sign mask
				mfc1	tmp1, _Val			// Move value to GPR
				lui		tmp3, 0x3f00		// Load half
				and		tmp2, tmp1, tmp2	// Mask sign bit
				or		tmp1, tmp3, tmp2	// Or signbit onto half
				mtc1	tmp1, fp2			// Move half to fpr
				add.s	fp1, fp2, _Val		// Add half to value
				cvt.w.s	fp1, fp1			// Convert to float
				mfc1	Ret, fp1			// Return value
			" );
	return Ret;
}

static M_INLINE fp4 Floor( fp4 _Val )
{
	fp4 Value = TruncToInt(_Val);
	fp4 f = Fraction(_Val);

	if( f < 0 )
	{
		Value	-= 1.0f;
	}
	
	return Value;
}

static M_INLINE fp4 Ceil( fp4 _Val )
{
	fp4 Value = TruncToInt(_Val);
	fp4 f = Fraction(_Val);

	if( f > 0 )
	{
		Value	+= 1.0f;
	}
	
	return Value;
}

static M_INLINE fp4 Sign(fp4 _x)
{
	(*(int*)&_x) &= 0x80000000;
	(*(int*)&_x) |= 0x3f800000;
	return _x;
}

static M_INLINE int FloatIsNeg(fp4 _Val)
{
	return (*((int*)&_Val) & 0x80000000);
}

static M_INLINE fp4 ClampRange(fp4 _Val,fp4 _Range)
{
	return Max( 0.0f, Min( _Range, _Val ) );
}

static M_INLINE fp4 Ceil1(fp4 _Val)
{
	return (_Val - M_Fabs(1.0f - _Val) + 1.0f)*.5f;
}

static M_INLINE fp4 Ceilx(fp4 _Val, fp4 _Range)
{
	return (_Val - M_Fabs(_Range - _Val) + _Range)*.5f;
}

static M_INLINE int RoundRGB(fp4 _r, fp4 _g, fp4 _b)
{
	return 
		(int(Ceilx(_r, 255.0f)) << 16) +
		(int(Ceilx(_g, 255.0f)) << 8) +
		(int(Ceilx(_b, 255.0f)) << 0);
}

static M_INLINE int RoundRGBA(fp4 _r, fp4 _g, fp4 _b, fp4 _a)
{
	return 
		(int(Ceilx(_a, 255.0f)) << 24) +
		(int(Ceilx(_r, 255.0f)) << 16) +
		(int(Ceilx(_g, 255.0f)) << 8) +
		(int(Ceilx(_b, 255.0f)) << 0);
}

void MCCDLLEXPORT AsyncRecp(fp4 _v);
fp4 MCCDLLEXPORT AsyncRecpGet();
void MCCDLLEXPORT AsyncRecpFree();

#endif	// CPU_MIPS_PS2
