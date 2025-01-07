
#ifdef PLATFORM_PS3

static M_INLINE fp4 __frsqrte(fp4 _Val)
{
	fp4 ret;
	asm("frsqrte %0, %1" : "=f"(ret) : "f"(_Val));
	return ret;
}

static M_INLINE fp8 __fsqrt(fp8 _Val)
{
	fp8 ret;
	asm("fsqrt %0, %1" : "=f"(ret) : "f"(_Val));
	return ret;
}

static M_INLINE fp4 __fsqrts(fp4 _Val)
{
	fp4 ret;
	asm("fsqrts %0, %1" : "=f"(ret) : "f"(_Val));
	return ret;
}

static M_INLINE fp8 __fsel(fp8 _Comp, fp8 _GE, fp8 _LT)
{
	fp8 ret;
	asm("fsel %0, %1, %2, %3" : "=f"(ret) : "f"(_Comp), "f"(_GE), "f"(_LT));
	return ret;
}

static M_INLINE fp8 __fabs(fp8 _Val)
{
	fp8 ret;
	asm("fabs %0, %1" : "=f"(ret) : "f"(_Val));
	return ret;
}

static M_INLINE const fp4 M_Fabs(fp4 _Val)						{ return __fabs(_Val); }
static M_INLINE fp4 M_Sin(fp4 _Val)								{ return sinf(_Val); }
static M_INLINE fp4 M_Cos(fp4 _Val)								{ return cosf(_Val); }
static M_INLINE fp4 M_Tan(fp4 _Val)								{ return tanf(_Val); }
static M_INLINE fp4 M_ATan(fp4 _Val)							{ return atanf(_Val); }
static M_INLINE fp4 M_ATan2(fp4 _Val, fp4 _Val2)				{ return atan2f(_Val, _Val2); }
static M_INLINE fp4 M_Cosh(fp4 _Val)							{ return coshf(_Val); }
static M_INLINE fp4 M_Sinh(fp4 _Val)							{ return sinhf(_Val); }
static M_INLINE fp4 M_Tanh(fp4 _Val)							{ return tanhf(_Val); }
static M_INLINE fp4 M_SinFast(fp4 _Val)							{ return M_Sin(_Val); }						// Input must be in range 0..2pi
static M_INLINE fp4 M_CosFast(fp4 _Val)							{ return M_Cos(_Val); }						// Input must be in range 0..2pi
static M_INLINE fp4 M_FMod(fp4 _a, fp4 _b)						{ return fmodf(_a, _b); }
static M_INLINE fp8 M_AToF(const char *_Str)					{ return atof(_Str); }
static M_INLINE fp4 M_Log10(fp4 _Val)							{ return log10f(_Val); }
static M_INLINE fp4 M_Log(fp4 _Val)								{ return logf(_Val); }
static M_INLINE fp4 M_Exp(fp4 _Val)								{ return expf(_Val); }
static M_INLINE fp4 M_Pow(fp4 _V,fp4 _V2)						{ return powf(_V,_V2); }
static M_INLINE fp4 M_FSel(fp4 _Comp, fp4 _ArgGE, fp4 _ArgLT)	{ return __fsel(_Comp, _ArgGE, _ArgLT); }	// If _Comp < 0 then _ArgLT else _ArgGE

static M_INLINE fp8 M_Fabs(fp8 _Val)							{ return __fabs(_Val); }
static M_INLINE const fp4 M_Sqrt(int _Val)						{ return __fsqrts((fp4)_Val); }
static M_INLINE const fp8 M_Sqrt(fp8 _Val)						{ return __fsqrt(_Val); }
static M_INLINE fp8 M_InvSqrt(fp8 _Val)							{ return 1.0/__fsqrt(_Val); }
static M_INLINE fp8 M_Sin(fp8 _Val)								{ return sin(_Val); }
static M_INLINE fp8 M_Cos(fp8 _Val)								{ return cos(_Val); }
static M_INLINE fp8 M_Tan(fp8 _Val)								{ return tan(_Val); }
static M_INLINE fp8 M_ASin(fp8 _Val)							{ return asin(_Val); }
static M_INLINE fp8 M_ACos(fp8 _Val)							{ return acos(_Val); }
static M_INLINE fp8 M_ATan(fp8 _Val)							{ return atan(_Val); }
static M_INLINE fp8 M_ATan2(fp8 _Val, fp8 _Val2)				{ return atan2(_Val, _Val2); }
static M_INLINE fp8 M_Cosh(fp8 _Val)							{ return cosh(_Val); }
static M_INLINE fp8 M_Sinh(fp8 _Val)							{ return sinh(_Val); }
static M_INLINE fp8 M_Tanh(fp8 _Val)							{ return tanh(_Val); }
static M_INLINE fp8 M_FMod(fp8 _a, fp8 _b)						{ return fmod(_a, _b); }
static M_INLINE fp8 M_Log10(fp8 _Val)							{ return log10(_Val); }
static M_INLINE fp8 M_Log(fp8 _Val)								{ return log(_Val); }
static M_INLINE fp8 M_Exp(fp8 _Val)								{ return exp(_Val); }
static M_INLINE fp8 M_Pow(fp8 _V,fp8 _V2)						{ return pow(_V,_V2); }
static M_INLINE fp8 M_FSel(fp8 _Comp, fp8 _ArgGE, fp8 _ArgLT)	{ return __fsel(_Comp, _ArgGE, _ArgLT); }	// If _Comp < 0 then _ArgLT else _ArgGE


static M_INLINE fp4 M_Sqrt(fp4 _Val)
{
	fp4 HalfArg = 0.5f * _Val;
	const fp4 OneAndHalf = 1.5f;

	fp4 estimate = __frsqrte(_Val);					// 5 bit estimate
	estimate = estimate * (OneAndHalf - HalfArg * estimate * estimate);
	estimate = estimate * (OneAndHalf - HalfArg * estimate * estimate);
	estimate = estimate * (OneAndHalf - HalfArg * estimate * estimate);

	return __fsel(-_Val, 0.0f, _Val * estimate);	// sqrt(x) == x * (1 / sqrt(x))     [NOTE: will return 0.0f for x <= 0.0f]
}

static M_INLINE fp4 M_InvSqrt(fp4 _Val)
{
	fp4 estimate, estimate2;
	fp4 HalfArg = 0.5f * _Val;
	const fp4 OneAndHalf = 1.5f;

	estimate = estimate2 = __frsqrte(_Val);  // 5 bit estimate
	estimate = estimate * (OneAndHalf - HalfArg * estimate * estimate);
	estimate = estimate * (OneAndHalf - HalfArg * estimate * estimate);
	estimate = estimate * (OneAndHalf - HalfArg * estimate * estimate);

	return __fsel(-_Val, estimate2, estimate);
}


static M_INLINE fp4 M_ASin(fp4 _Val)
{
	M_ASSERT(fabsf(_Val) < 1.01f, "Invalid input to asin()");
	_Val = Clamp(_Val, -1.0f, 1.0f);
	return asinf(_Val);
}

static M_INLINE fp4 M_ACos(fp4 _Val)
{
	M_ASSERT(fabsf(_Val) < 1.01f, "Invalid input to acos()");
	_Val = Clamp(_Val, -1.0f, 1.0f);
	return acosf(_Val);
}


static M_INLINE fp4 Clamp01(fp4 _Val)
{
	return __fsel(_Val, __fsel(_Val - 1.0f, 1.0f, _Val), 0.0f);
}

static M_INLINE fp8 Clamp01(fp8 _Val)
{
	return __fsel(_Val, __fsel(_Val - 1.0, 1.0, _Val), 0.0);
}

static M_INLINE int TruncToInt( fp4 _Val )
{
	return (int)_Val;
}

static M_INLINE int TruncToInt( fp8 _Val )
{
	return (int)_Val;
}

static M_INLINE fp4 Fraction(fp4 _Val)
{
	fp4 frac, t;
	asm("fctidz %1, %2\n"
		"fcfid %1, %1\n"
		"fsub %0, %2, %1\n"
		: "=f"(frac), "=&f"(t) : "f"(_Val));
	return frac;
}

static M_INLINE fp8 Fraction(fp8 _Val)
{
	fp8 frac, t;
	asm("fctidz %1, %2\n"
		"fcfid %1, %1\n"
		"fsub %0, %2, %1\n"
		: "=f"(frac), "=&f"(t) : "f"(_Val));
	return frac;
}

static M_INLINE fp4 Floor(fp4 _Value)
{
	fp4 ret, frac, t;
	static fp4 One = 1.0f;
	fp4 Zero;
	asm("fsub %3, %3, %3\n"
		"fctidz %1, %4\n"
		"fcfid %1, %1\n"
		"fsub %2, %4, %1\n"
		"fsel %2, %2, %3, %5\n"
		"fsub %0, %1, %2\n"
		: "=&f"(ret), "=&f"(frac), "=&f"(t), "=&f"(Zero) : "f"(_Value), "f"(One));

	return ret;
}

static M_INLINE fp8 Floor(fp8 _Value)
{
	fp8 ret, frac, t;
	static fp8 One = 1.0;
	fp8 Zero;
	asm("fsub %3, %3, %3\n"
		"fctidz %1, %4\n"
		"fcfid %1, %1\n"
		"fsub %2, %4, %1\n"
		"fsel %2, %2, %3, %5\n"
		"fsub %0, %1, %2\n"
		: "=&f"(ret), "=&f"(frac), "=&f"(t), "=&f"(Zero) : "f"(_Value), "f"(One));

	return ret;
}

static M_INLINE fp4 Ceil(fp4 _Value)
{
	fp4 ret, frac, t;
	static fp4 One = 1.0f;
	fp4 Zero;
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

static M_INLINE fp8 Ceil(fp8 _Value)
{
	fp8 ret, frac, t;
	static fp8 One = 1.0;
	fp8 Zero;
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

static M_INLINE int RoundToInt( fp4 _Val )
{
	return (int)(_Val + __fsel(_Val, 0.5, -0.5));
}
static M_INLINE int RoundToInt( fp8 _Val )
{
	return (int)(_Val + __fsel(_Val, 0.5, -0.5));
}

static M_INLINE fp4 Sign(fp4 _x)
{
	return __fsel(_x, 1.0f, -1.0f);
}

static M_INLINE int FloatIsNeg(fp4 _Val)
{
	return (int)((_Val < 0.0f) ? 1 : 0);
}

static M_INLINE int FloatIsNeg(fp8 _Val)
{
	return (int)((_Val < 0.0f) ? 1 : 0);
}

static M_INLINE fp4 ClampRange(fp4 _Val, fp4 _Range)
{
	return __fsel(_Val, __fsel(_Val - _Range, _Range, _Val), 0.0f);
}

static M_INLINE fp8 ClampRange(fp8 _Val, fp8 _Range)
{
	return __fsel(_Val, __fsel(_Val - _Range, _Range, _Val), 0.0);
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



#endif	// PLATFORM_PS3
