
#ifdef CPU_X86

#include <xmmintrin.h>

static M_INLINE const fp4 M_Fabs(fp4 _Val)						{ return fabsf(_Val); }
static M_INLINE fp4 M_Sqrt(fp4 _Val)							{ return sqrtf(_Val); }
static M_INLINE fp4 M_InvSqrt(fp4 _Val)							{ return 1.0f / sqrtf(_Val); }
static M_INLINE fp4 M_Sin(fp4 _Val)								{ return sinf(_Val); }
static M_INLINE fp4 M_Cos(fp4 _Val)								{ return cosf(_Val); }
static M_INLINE fp4 M_Tan(fp4 _Val)								{ return tanf(_Val); }
static M_INLINE fp4 M_ATan(fp4 _Val)							{ return atanf(_Val); }
static M_INLINE fp4 M_ATan2(fp4 _Val, fp4 _Val2)    			{ return atan2f(_Val, _Val2); }
static M_INLINE fp4 M_Cosh(fp4 _Val)							{ return coshf(_Val); }
static M_INLINE fp4 M_Sinh(fp4 _Val)							{ return sinhf(_Val); }
static M_INLINE fp4 M_Tanh(fp4 _Val)							{ return tanhf(_Val); }
static M_INLINE fp4 M_SinFast(fp4 _Val)							{ return M_Sin(_Val); }			// Input must be in range 0..2pi
static M_INLINE fp4 M_CosFast(fp4 _Val)							{ return M_Cos(_Val); }			// Input must be in range 0..2pi
static M_INLINE fp4 M_FMod(fp4 _a, fp4 _b)						{ return fmodf(_a, _b); }
static M_INLINE fp8 M_AToF(const char *_Str)					{ return atof(_Str); }
static M_INLINE fp4 M_Log10(fp4 _Val)							{ return log10f(_Val); }
static M_INLINE fp4 M_Log(fp4 _Val)								{ return logf(_Val); }
static M_INLINE fp4 M_Exp(fp4 _Val)								{ return expf(_Val); }
static M_INLINE fp4 M_Pow(fp4 _V,fp4 _V2)						{ return powf(_V,_V2); }
static M_INLINE fp4 M_FSel(fp4 _Comp, fp4 _ArgGE, fp4 _ArgLT)	{ return (_Comp < -0.0f) ? _ArgLT : _ArgGE; }	// If _Comp < 0 then _ArgLT else _ArgGE

static M_INLINE fp8 M_Fabs(fp8 _Val)    						{ return fabs(_Val); }
static M_INLINE const fp4 M_Sqrt(int _Val)  					{ return sqrtf((fp4)_Val); }
static M_INLINE const fp8 M_Sqrt(fp8 _Val)  					{ return sqrt(_Val); }
static M_INLINE fp4 M_InvSqrt(fp8 _Val) 						{ return 1.0 / sqrt(_Val); }
static M_INLINE fp8 M_Sin(fp8 _Val)    	 						{ return sin(_Val); }
static M_INLINE fp8 M_Cos(fp8 _Val)     						{ return cos(_Val); }
static M_INLINE fp8 M_Tan(fp8 _Val)     						{ return tan(_Val); }
static M_INLINE fp8 M_ASin(fp8 _Val)    						{ return asin(_Val); }
static M_INLINE fp8 M_ACos(fp8 _Val)    						{ return acos(_Val); }
static M_INLINE fp8 M_ATan(fp8 _Val)    						{ return atan(_Val); }
static M_INLINE fp8 M_ATan2(fp8 _Val, fp8 _Val2)    			{ return atan2(_Val, _Val2); }
static M_INLINE fp8 M_Cosh(fp8 _Val)	  						{ return cosh(_Val); }
static M_INLINE fp8 M_Sinh(fp8 _Val)	  						{ return sinh(_Val); }
static M_INLINE fp8 M_Tanh(fp8 _Val)	  						{ return tanh(_Val); }
static M_INLINE fp8 M_FMod(fp8 _a, fp8 _b)						{ return fmod(_a, _b); }
static M_INLINE fp8 M_Log10(fp8 _Val)	  						{ return log10(_Val); }
static M_INLINE fp8 M_Log(fp8 _Val)	  							{ return log(_Val); }
static M_INLINE fp8 M_Exp(fp8 _Val)								{ return exp(_Val); }
static M_INLINE fp8 M_Pow(fp8 _V,fp8 _V2)						{ return pow(_V,_V2); }
static M_INLINE fp8 M_FSel(fp8 _Comp, fp8 _ArgGE, fp8 _ArgLT)	{ return (_Comp < -0.0) ? _ArgLT : _ArgGE; }	// If _Comp < 0 then _ArgLT else _ArgGE


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
	return ( M_Fabs(_Val) - M_Fabs(_Val-1.f) + 1.f )*.5f;
}

static M_INLINE fp8 Clamp01(fp8 _Val)
{
	return ( M_Fabs(_Val) - M_Fabs(_Val-1.f) + 1.f )*.5f;
}

#ifdef CPU_SSE
static M_INLINE int TruncToInt( fp4 _Val )
{
	int a;
	__asm
	{
        CVTTSS2SI eax, [_Val]
		mov [a], eax
	}
	return a;
}

static M_INLINE int RoundToInt(fp4 _Val)
{
	int32 a;
	__declspec(align(16)) static int32 MaskVal = 0x80000000;
	__declspec(align(16)) static int32 OrVal = 0x3f000000;
	__asm
	{
		MOVSS xmm0, [_Val]
		MOVSS xmm1, xmm0
		ANDPS xmm1, [MaskVal]
		ORPS xmm1, [OrVal]
		ADDSS xmm0, xmm1
		CVTTSS2SI eax, xmm0
		mov [a], eax
	}
	return a;
}
/*
static M_INLINE void RoundToInt(const fp4* _pVal, int* _pResult, int _nVal)
{
	fp4 Half = 0.5f;
	__asm
	{
		mov esi, [_pVal];
		mov edi, [_pResult];
		mov ecx, [_nVal]
		MOVSS xmm1, [Half]
Lp:
			MOVSS xmm0, xmm1
			ADDSS xmm0, dword ptr [esi]
			CVTSS2SI eax, xmm0
			mov [edi], eax
			add esi, 4
			dec ecx
			lea edi, [edi+4]
		jnz Lp
	}
}
*/
#else
static M_INLINE int RoundToInt(fp4 _Val)
{
	int a;
	__asm
	{
		fld [_Val]
		fistp [a]
	}
	return a;
}

static M_INLINE int TruncToInt( fp4 _Val )
{
	return (int)_Val;
}
#endif

static M_INLINE int RoundToInt(fp8 _Val)
{
	int a;
	__asm
	{
		fld [_Val]
		fistp [a]
	}
	return a;
}

static M_INLINE fp4 Fraction(fp4 _Val)
{
	return _Val - TruncToInt(_Val);
}

static M_INLINE fp8 Fraction(fp8 _Val)
{
	return _Val - TruncToInt(_Val);
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

static M_INLINE fp8 Floor(fp8 _Val)
{
	fp8 Value = TruncToInt(_Val);
	fp8 f = Fraction(_Val);

	if( f < 0 )
	{
		Value	-= 1.0;
	}
	
	return Value;
}

static M_INLINE fp8 Ceil(fp8 _Val)
{
	fp8 Value = TruncToInt(_Val);
	fp8 f = Fraction(_Val);

	if( f > 0 )
	{
		Value	+= 1.0;
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

static M_INLINE int FloatIsNeg(fp8 _Val)
{
	return (((int32*)&_Val)[1] & 0x80000000);
}

static M_INLINE fp4 ClampRange(fp4 _Val,fp4 _Range)
{
	return ( M_Fabs(_Val) - M_Fabs(_Val-_Range) + _Range )*.5f;
}

static M_INLINE fp8 ClampRange(fp8 _Val,fp8 _Range)
{
	return ( M_Fabs(_Val) - M_Fabs(_Val-_Range) + _Range )*.5;
}

static M_INLINE fp4 Ceil1(fp4 _Val)
{
	return (_Val - M_Fabs(1.0f - _Val) + 1.0f)*.5f;
}

static M_INLINE fp4 Ceilx(fp4 _Val, fp4 _Range)
{
	return (_Val - M_Fabs(_Range - _Val) + _Range)*.5f;
}


#ifdef CPU_INTELP5

static M_INLINE int RoundRGB(fp4 _r, fp4 _g, fp4 _b)
{
	int Col;
	int Col2;
	__asm
	{
		fld [_r]
		fistp [Col2]
		fld [_g]
		mov eax, [Col2]
		fistp [Col2]
		shl eax, 16
		mov ebx, [Col2]
		fld [_b]
		shl ebx, 8
		fistp [Col2]
		or eax, ebx
		mov ebx, [Col2]
		or eax, ebx

		mov [Col], eax
	}
	return Col;
}
#else
static M_INLINE int RoundRGB(fp4 _r, fp4 _g, fp4 _b)
{
	return 
		(int(Ceilx(_r, 255.0f)) << 16) +
		(int(Ceilx(_g, 255.0f)) << 8) +
		(int(Ceilx(_b, 255.0f)) << 0);
}
#endif

static M_INLINE int RoundRGBA(fp4 _r, fp4 _g, fp4 _b, fp4 _a)
#ifdef CPU_INTELP5
{
	int Col;
	int Col2;
	__asm
	{
		fld [_a]
		fistp [Col2]
		mov eax, [Col2]
		shl eax, 24

		fld [_r]
		fistp [Col2]
		mov ebx, [Col2]
		shl ebx, 16
		or eax, ebx

		fld [_g]
		fistp [Col2]
		mov ebx, [Col2]
		shl ebx, 8
		or eax, ebx

		fld [_b]
		fistp [Col2]
		mov ebx, [Col2]
		or eax, ebx

		mov [Col], eax
	}
	return Col;
}
#else
{
	return 
		(int(Ceilx(_a, 255.0f)) << 24) +
		(int(Ceilx(_r, 255.0f)) << 16) +
		(int(Ceilx(_g, 255.0f)) << 8) +
		(int(Ceilx(_b, 255.0f)) << 0);
}
#endif

#ifdef CPU_INTEL_P5

static M_INLINE void AsyncRecp(fp4 _v)
{
	__asm
	{
		fld1
		fld [_v]
		fdivp st(1), st
	}
}

static M_INLINE fp4 AsyncRecpGet()
{
	fp4 Ret;
	__asm fstp [Ret];
	return Ret;
}

static M_INLINE void AsyncRecpFree()
{
	__asm ffree st(0);
}

#else

void MCCDLLEXPORT AsyncRecp(fp4 _v);
fp4 MCCDLLEXPORT AsyncRecpGet();
void MCCDLLEXPORT AsyncRecpFree();

#endif



#endif	// CPU_X86
