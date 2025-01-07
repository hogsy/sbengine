
#ifdef PLATFORM_PS3

static M_INLINE fp32 __frsqrte(fp32 _Val)
{
	fp32 ret;
	asm("frsqrte %0, %1" : "=f"(ret) : "f"(_Val));
	return ret;
}

static M_INLINE fp64 __fsqrt(fp64 _Val)
{
	fp64 ret;
	asm("fsqrt %0, %1" : "=f"(ret) : "f"(_Val));
	return ret;
}

static M_INLINE fp32 __fsqrts(fp32 _Val)
{
	fp32 ret;
	asm("fsqrts %0, %1" : "=f"(ret) : "f"(_Val));
	return ret;
}

static M_INLINE fp64 __fsel(fp64 _Comp, fp64 _GE, fp64 _LT)
{
	fp64 ret;
	asm("fsel %0, %1, %2, %3" : "=f"(ret) : "f"(_Comp), "f"(_GE), "f"(_LT));
	return ret;
}

static M_INLINE fp64 __fabs(fp64 _Val)
{
	fp64 ret;
	asm("fabs %0, %1" : "=f"(ret) : "f"(_Val));
	return ret;
}

static M_INLINE const fp32 M_Fabs(fp32 _Val)						{ return __fabs(_Val); }
static M_INLINE fp32 M_Sin(fp32 _Val)								{ return sinf(_Val); }
static M_INLINE fp32 M_Cos(fp32 _Val)								{ return cosf(_Val); }
static M_INLINE fp32 M_Tan(fp32 _Val)								{ return tanf(_Val); }
static M_INLINE fp32 M_ATan(fp32 _Val)							{ return atanf(_Val); }
static M_INLINE fp32 M_ATan2(fp32 _Val, fp32 _Val2)				{ return atan2f(_Val, _Val2); }
static M_INLINE fp32 M_Cosh(fp32 _Val)							{ return coshf(_Val); }
static M_INLINE fp32 M_Sinh(fp32 _Val)							{ return sinhf(_Val); }
static M_INLINE fp32 M_Tanh(fp32 _Val)							{ return tanhf(_Val); }
static M_INLINE fp32 M_SinFast(fp32 _Val)							{ return M_Sin(_Val); }						// Input must be in range 0..2pi
static M_INLINE fp32 M_CosFast(fp32 _Val)							{ return M_Cos(_Val); }						// Input must be in range 0..2pi
static M_INLINE fp32 M_FMod(fp32 _a, fp32 _b)						{ return fmodf(_a, _b); }
static M_INLINE fp64 M_AToF(const char *_Str)					{ return atof(_Str); }
static M_INLINE fp32 M_Log10(fp32 _Val)							{ return log10f(_Val); }
static M_INLINE fp32 M_Log(fp32 _Val)								{ return logf(_Val); }
static M_INLINE fp32 M_Exp(fp32 _Val)								{ return expf(_Val); }
static M_INLINE fp32 M_Pow(fp32 _V,fp32 _V2)						{ return powf(_V,_V2); }
static M_INLINE fp32 M_FSel(fp32 _Comp, fp32 _ArgGE, fp32 _ArgLT)	{ return __fsel(_Comp, _ArgGE, _ArgLT); }	// If _Comp < 0 then _ArgLT else _ArgGE

static M_INLINE fp64 M_Fabs(fp64 _Val)							{ return __fabs(_Val); }
static M_INLINE const fp32 M_Sqrt(int _Val)						{ return __fsqrts((fp32)_Val); }
static M_INLINE const fp64 M_Sqrt(fp64 _Val)						{ return __fsqrt(_Val); }
static M_INLINE fp64 M_InvSqrt(fp64 _Val)							{ return 1.0/__fsqrt(_Val); }
static M_INLINE fp64 M_Sin(fp64 _Val)								{ return sin(_Val); }
static M_INLINE fp64 M_Cos(fp64 _Val)								{ return cos(_Val); }
static M_INLINE fp64 M_Tan(fp64 _Val)								{ return tan(_Val); }
static M_INLINE fp64 M_ASin(fp64 _Val)							{ return asin(_Val); }
static M_INLINE fp64 M_ACos(fp64 _Val)							{ return acos(_Val); }
static M_INLINE fp64 M_ATan(fp64 _Val)							{ return atan(_Val); }
static M_INLINE fp64 M_ATan2(fp64 _Val, fp64 _Val2)				{ return atan2(_Val, _Val2); }
static M_INLINE fp64 M_Cosh(fp64 _Val)							{ return cosh(_Val); }
static M_INLINE fp64 M_Sinh(fp64 _Val)							{ return sinh(_Val); }
static M_INLINE fp64 M_Tanh(fp64 _Val)							{ return tanh(_Val); }
static M_INLINE fp64 M_FMod(fp64 _a, fp64 _b)						{ return fmod(_a, _b); }
static M_INLINE fp64 M_Log10(fp64 _Val)							{ return log10(_Val); }
static M_INLINE fp64 M_Log(fp64 _Val)								{ return log(_Val); }
static M_INLINE fp64 M_Exp(fp64 _Val)								{ return exp(_Val); }
static M_INLINE fp64 M_Pow(fp64 _V,fp64 _V2)						{ return pow(_V,_V2); }
static M_INLINE fp64 M_FSel(fp64 _Comp, fp64 _ArgGE, fp64 _ArgLT)	{ return __fsel(_Comp, _ArgGE, _ArgLT); }	// If _Comp < 0 then _ArgLT else _ArgGE


static M_INLINE fp32 M_Sqrt(fp32 _Val)
{
	fp32 HalfArg = 0.5f * _Val;
	const fp32 OneAndHalf = 1.5f;

	fp32 estimate = __frsqrte(_Val);					// 5 bit estimate
	estimate = estimate * (OneAndHalf - HalfArg * estimate * estimate);
	estimate = estimate * (OneAndHalf - HalfArg * estimate * estimate);
	estimate = estimate * (OneAndHalf - HalfArg * estimate * estimate);

	return __fsel(-_Val, 0.0f, _Val * estimate);	// sqrt(x) == x * (1 / sqrt(x))     [NOTE: will return 0.0f for x <= 0.0f]
}

static M_INLINE fp32 M_InvSqrt(fp32 _Val)
{
	fp32 estimate, estimate2;
	fp32 HalfArg = 0.5f * _Val;
	const fp32 OneAndHalf = 1.5f;

	estimate = estimate2 = __frsqrte(_Val);  // 5 bit estimate
	estimate = estimate * (OneAndHalf - HalfArg * estimate * estimate);
	estimate = estimate * (OneAndHalf - HalfArg * estimate * estimate);
	estimate = estimate * (OneAndHalf - HalfArg * estimate * estimate);

	return __fsel(-_Val, estimate2, estimate);
}


static M_INLINE fp32 M_ASin(fp32 _Val)
{
	M_ASSERT(fabsf(_Val) < 1.01f, "Invalid input to asin()");
	_Val = Clamp(_Val, -1.0f, 1.0f);
	return asinf(_Val);
}

static M_INLINE fp32 M_ACos(fp32 _Val)
{
	M_ASSERT(fabsf(_Val) < 1.01f, "Invalid input to acos()");
	_Val = Clamp(_Val, -1.0f, 1.0f);
	return acosf(_Val);
}


static M_INLINE fp32 Clamp01(fp32 _Val)
{
	return __fsel(_Val, __fsel(_Val - 1.0f, 1.0f, _Val), 0.0f);
}

static M_INLINE fp64 Clamp01(fp64 _Val)
{
	return __fsel(_Val, __fsel(_Val - 1.0, 1.0, _Val), 0.0);
}

static M_INLINE int TruncToInt( fp32 _Val )
{
	return (int)_Val;
}

static M_INLINE int TruncToInt( fp64 _Val )
{
	return (int)_Val;
}

static M_INLINE fp32 Fraction(fp32 _Val)
{
	fp32 frac, t;
	asm("fctidz %1, %2\n"
		"fcfid %1, %1\n"
		"fsub %0, %2, %1\n"
		: "=f"(frac), "=&f"(t) : "f"(_Val));
	return frac;
}

static M_INLINE fp64 Fraction(fp64 _Val)
{
	fp64 frac, t;
	asm("fctidz %1, %2\n"
		"fcfid %1, %1\n"
		"fsub %0, %2, %1\n"
		: "=f"(frac), "=&f"(t) : "f"(_Val));
	return frac;
}

static M_INLINE fp32 Floor(fp32 _Value)
{
	fp32 ret, frac, t;
	static fp32 One = 1.0f;
	fp32 Zero;
	asm("fsub %3, %3, %3\n"
		"fctidz %1, %4\n"
		"fcfid %1, %1\n"
		"fsub %2, %4, %1\n"
		"fsel %2, %2, %3, %5\n"
		"fsub %0, %1, %2\n"
		: "=&f"(ret), "=&f"(frac), "=&f"(t), "=&f"(Zero) : "f"(_Value), "f"(One));

	return ret;
}

static M_INLINE fp64 Floor(fp64 _Value)
{
	fp64 ret, frac, t;
	static fp64 One = 1.0;
	fp64 Zero;
	asm("fsub %3, %3, %3\n"
		"fctidz %1, %4\n"
		"fcfid %1, %1\n"
		"fsub %2, %4, %1\n"
		"fsel %2, %2, %3, %5\n"
		"fsub %0, %1, %2\n"
		: "=&f"(ret), "=&f"(frac), "=&f"(t), "=&f"(Zero) : "f"(_Value), "f"(One));

	return ret;
}

static M_INLINE fp32 Ceil(fp32 _Value)
{
	fp32 ret, frac, t;
	static fp32 One = 1.0f;
	fp32 Zero;
	asm("fsub %3, %3, %3\n"
		"fctidz %1, %4\n"
		"fcfid %1, %1\n"
		"fsub %2, %4, %1\n"
		"fneg %2, %2\n"
		"fsel %2, %2, %3, %5\n"
		"fadd %0, %1, %2\n"
		: "=&f"(ret), "=&f"(frac), "=&f"(t), "=&f"(Zero) : "f"(_Value), "f"(One));

	return ret;
}

static M_INLINE fp64 Ceil(fp64 _Value)
{
	fp64 ret, frac, t;
	static fp64 One = 1.0;
	fp64 Zero;
	asm("fsub %3, %3, %3\n"
		"fctidz %1, %4\n"
		"fcfid %1, %1\n"
		"fsub %2, %4, %1\n"
		"fneg %2, %2\n"
		"fsel %2, %2, %3, %5\n"
		"fadd %0, %1, %2\n"
		: "=&f"(ret), "=&f"(frac), "=&f"(t), "=&f"(Zero) : "f"(_Value), "f"(One));

	return ret;
}

static M_INLINE int RoundToInt( fp32 _Val )
{
	return (int)(_Val + __fsel(_Val, 0.5, -0.5));
}
static M_INLINE int RoundToInt( fp64 _Val )
{
	return (int)(_Val + __fsel(_Val, 0.5, -0.5));
}

static M_INLINE fp32 Sign(fp32 _x)
{
	return __fsel(_x, 1.0f, -1.0f);
}

static M_INLINE int FloatIsNeg(fp32 _Val)
{
	return (int)((_Val < 0.0f) ? 1 : 0);
}

static M_INLINE int FloatIsNeg(fp64 _Val)
{
	return (int)((_Val < 0.0f) ? 1 : 0);
}

static M_INLINE fp32 ClampRange(fp32 _Val, fp32 _Range)
{
	return __fsel(_Val, __fsel(_Val - _Range, _Range, _Val), 0.0f);
}

static M_INLINE fp64 ClampRange(fp64 _Val, fp64 _Range)
{
	return __fsel(_Val, __fsel(_Val - _Range, _Range, _Val), 0.0);
}

static M_INLINE fp32 Ceil1(fp32 _Val)
{
	return (_Val - M_Fabs(1.0f - _Val) + 1.0f)*.5f;
}

static M_INLINE fp32 Ceilx(fp32 _Val, fp32 _Range)
{
	return (_Val - M_Fabs(_Range - _Val) + _Range)*.5f;
}

static M_INLINE int RoundRGB(fp32 _r, fp32 _g, fp32 _b)
{
	return 
		(int(Ceilx(_r, 255.0f)) << 16) +
		(int(Ceilx(_g, 255.0f)) << 8) +
		(int(Ceilx(_b, 255.0f)) << 0);
}

static M_INLINE int RoundRGBA(fp32 _r, fp32 _g, fp32 _b, fp32 _a)
{
	return 
		(int(Ceilx(_a, 255.0f)) << 24) +
		(int(Ceilx(_r, 255.0f)) << 16) +
		(int(Ceilx(_g, 255.0f)) << 8) +
		(int(Ceilx(_b, 255.0f)) << 0);
}

void MCCDLLEXPORT AsyncRecp(fp32 _v);
fp32 MCCDLLEXPORT AsyncRecpGet();
void MCCDLLEXPORT AsyncRecpFree();



#endif	// PLATFORM_PS3
