
#ifndef _INC_MFLOAT
#define _INC_MFLOAT

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Standard math functions and performance helpers.

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios AB 1996,2003

	Contents:

	Comments:		Functions in math.h should not be used. Instead of sin use M_Sin, etc..

	History:	
		030711:		File header added

\*____________________________________________________________________________________________*/

#include "MRTC.h"
#include "MMisc.h"

#ifdef PLATFORM_DOLPHIN
#include "GC/GC_Float.h"
#endif
#include "MFloat_PS2.h"
#include "MFloat_PS3.h"
#include "MFloat_Xenon.h"
#include "MFloat_x86.h"

void MCCDLLEXPORT MFloat_Init();
void MCCDLLEXPORT MFloat_GetSineTable(const fp4*& _pSin);

MCCDLLEXPORT const fp4 *MFloat_GetRandTable();
MCCDLLEXPORT fp4 MFloat_GetRand(int _iIndex);

#define MACRO_GetSineTable(Var) const fp4* Var; MFloat_GetSineTable(Var);

// -------------------------------------------------------------------

#define LERP(a,b,t) ((a) + ((b) - (a))*t) // Added by Mondelore

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| INDEX
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifdef NEVER

int FloatIsNeg(fp4 _Val);
fp4 Sign(fp4 _x);							// Sign(x) à la speedy gonzales
fp4 Clamp01(fp4 _Val);						// 12c
fp4 ClampRange(fp4 _Val,fp4 _Range);		// 12c
fp4 Ceil1(fp4 _Val);						// 12c
fp4 Ceilx(fp4 _Val, fp4 _Range);			// 12c
int TruncToInt(fp4 _Val);					// 25c, standard c typecast int() uses ~55 cycles.
int RoundToInt(fp4 _Val);					// 15c
void RoundToInt(const fp4* _pVal, int* _pResult, int _nVal);
#define floor Floor
#define ceil Ceil
int RoundRGB(fp4 _r, fp4 _g, fp4 _b);
void MCCDLLEXPORT AsyncRecp(fp4 _v);
fp4 MCCDLLEXPORT AsyncRecpGet();
void MCCDLLEXPORT AsyncRecpFree();

int FloatIsNeg(fp8 _Val);
fp8 Clamp01(fp8 _Val);
fp8 ClampRange(fp8 _Val,fp8 _Range);
fp8 Ceil1(fp8 _Val);
fp8 Ceilx(fp8 _Val, fp8 _Range);
int TruncToInt(fp8 _Val);
int TruncToInt64(fp8 _Val);
int RoundToInt(fp8 _Val);
fp8 SnapFloat(fp8 _Val, fp8 _Grid, fp8 _SnapTresh);

void Moderate(int& x, int newx, int& xprim, int a);
void Moderatef(fp4& _x, fp4 _newx, fp4& _xprim, int a);
fp4 GetFade(fp4 _Time, fp4 _Duration, fp4 _FadeInTime, fp4 _FadeOutTime);

// -------------------------------------------------------------------
fp4 QSini(int _x);							// 28c,  | QSin(x) - sin(x) | < 10^-4, // x = [0..65536] <=> [0..2*PI]
fp4 QCosi(int _x);							// 28c,  | QSin(x) - sin(x) | < 10^-4, // x = [0..65536] <=> [0..2*PI]
fp4 QSin(fp4 _x);							// 66c
fp4 QCos(fp4 _x);							// 66c
void QSinCosi(int _x, fp4& _Sin, fp4& _Cos);// <80c
void QSinCos(fp4 _x, fp4& _Sin, fp4& _Cos);	// <80c

fp4 QAtan(fp4 _a);							// Taylor polynom accelerated arctan-approximation.
fp4 QAcos(fp4 _a);							// Arc-cosine using QAtan and fsqrt().
fp4 QAcos2(fp4 _a);							// Arc-cosine using QAtan.

#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Standard math
|__________________________________________________________________________________________________
\*************************************************************************************************/

template <class T> 
T Abs(T _v)
{
	if (_v < 0) return -_v;
	return _v;
}

template<>
M_INLINE fp4 Abs(fp4 _v) { return M_Fabs(_v); }
#ifndef PLATFORM_PS2
template<>
M_INLINE fp8 Abs(fp8 _v) { return M_Fabs(_v); }
#endif	// PLATFORM_PS2
template<>
M_INLINE fp4 Min3<fp4>(fp4 _a, fp4 _b, fp4 _c)
{
	return Min( _a, Min( _b, _c ) );
}

template<>
M_INLINE fp4 Max3<fp4>(fp4 _a, fp4 _b, fp4 _c)
{
	return Max( _a, Max( _b, _c ) );
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Float utilities
|__________________________________________________________________________________________________
\*************************************************************************************************/
static M_INLINE void RoundToInt(const fp4* _pVal, int* _pResult, int _nVal)
{
	for(int i = 0; i < _nVal; i++) _pResult[i] = RoundToInt(*_pVal);
}


//	Better to just have the fp8 versions to get rid of all ambiguity.

#define M_Floor	Floor
#define M_Ceil	Ceil
// -------------------------------------------------------------------
#if !defined(PLATFORM_PS2) && !defined(PLATFORM_DOLPHIN)
static M_INLINE fp8 Ceil1(fp8 _Val)
{
	return (_Val - M_Fabs(1.0f - _Val) + 1.0f)*.5;
}

static M_INLINE fp8 Ceilx(fp8 _Val, fp8 _Range)
{
	return (_Val - M_Fabs(_Range - _Val) + _Range)*.5;
}
#endif

#define floor Floor
#define ceil Ceil

#ifdef	CPU_SOFTWARE_FP8
static M_INLINE fp4 SnapFloat(fp4 _Val, fp4 _Grid, fp4 _SnapTresh)
{
	fp4 Val = fp4(RoundToInt(_Val / _Grid)) * _Grid;
	if (M_Fabs(Val - _Val) > _SnapTresh) Val = _Val;
	return Val;
}
#else
static M_INLINE fp8 SnapFloat(fp8 _Val, fp8 _Grid, fp8 _SnapTresh)
{
	fp8 Val = fp8(RoundToInt(_Val / _Grid)) * _Grid;
	if (M_Fabs(Val - _Val) > _SnapTresh) Val = _Val;
	return Val;
}
#endif

// -------------------------------------------------------------------
static M_INLINE void Moderate(int& x, int newx, int& xprim, int a)
{
	int xbiss = (int)((-fp4(a)*fp4(xprim) - fp4(x-newx)*Sqr(fp4(a))*0.25f) * (1.0f/256.0f));
	xprim += xbiss;
	x += (xprim >> 8);
}

static M_INLINE void Moderatef(fp4& _x, fp4 _newx, fp4& _xprim, int a)
{
	int xprim = RoundToInt(_xprim*256.0f);
	int x = RoundToInt(_x*256.0f);
	int newx = RoundToInt(_newx*256.0f);
	Moderate(x, newx, xprim, a);
	_xprim = fp4(xprim) * (1.0f/256.0f);
	_x = fp4(x) * (1.0f/256.0f);
}

// -------------------------------------------------------------------

static M_INLINE fp4 GetFade(fp4 _Time, fp4 _Duration, fp4 _FadeInTime, fp4 _FadeOutTime)
{
	fp4 RevTime = _Duration - _Time;
	fp4 Fade, FadeIn, FadeOut;

	if (_Time > 0)
	{
		if (_Time < _FadeInTime)
			FadeIn = _Time / _FadeInTime;
		else
			FadeIn = 1.0f;
	}
	else
	{
		FadeIn = 0.0f;
	}

	if (_Duration > 0)
	{
		if (RevTime > 0)
		{
			if (RevTime < _FadeOutTime)
				FadeOut = RevTime / _FadeOutTime;
			else
				FadeOut = 1.0f;
		}
		else
		{
			FadeOut = 0.0f;
		}
	}
	else
	{
		FadeOut = 1.0f;
	}

	Fade = Min(FadeIn, FadeOut);

	return Fade;
}

// -------------------------------------------------------------------
#define MFLOAT_SINETABLESIZE	256
#define MFLOAT_RANDTABLESIZE	512

static M_INLINE fp4 QSini(int _x)		// 28c,  abs( QSin(x) - sin(x) ) < 10^-4
{
	// x = [0..65535] <=> [0..2*PI]

	MACRO_GetSineTable(pSin);

	_x &= 0xffff;
	int Pos = _x >> 8;
	int Frac = _x & 0xff;
	fp4 s0 = pSin[Pos];
	fp4 s1 = pSin[(Pos+1) & 0xff];
	return s0 + (s1-s0)*fp4(Frac)*(1.0f/256.0f);
}

static M_INLINE fp4 QCosi(int _x)		// 28c,  abs( QSin(x) - sin(x) ) < 10^-4
{
	// x = [0..65535] <=> [0..2*PI]

	MACRO_GetSineTable(pSin);

	_x += 16384;
	_x &= 0xffff;
	int Pos = _x >> 8;
	int Frac = _x & 0xff;
	fp4 s0 = pSin[Pos];
	fp4 s1 = pSin[(Pos+1) & 0xff];
	return s0 + (s1-s0)*fp4(Frac)*(1.0f/256.0f);
}

static M_INLINE fp4 QSin(fp4 _x)		// 66c
{
	// x = [0..65535] <=> [0..2*PI]
	// Is the above really correct? Shouldn't the input range be [0..1]?!

	MACRO_GetSineTable(pSin);

	int x = RoundToInt(_x*(65536.0f/2.0f/_PI));
	x &= 0xffff;
	int Pos = x >> 8;
	int Frac = x & 0xff;

	fp4 s0 = pSin[Pos];
	fp4 s1 = pSin[(Pos+1) & 0xff];
	return s0 + (s1-s0)*fp4(Frac)*(1.0f/256.0f);
}

static M_INLINE fp4 QCos(fp4 _x)		// 66c
{
	// x = [0..65535] <=> [0..2*PI]
	// Is the above really correct? Shouldn't the input range be [0..1]?!

	MACRO_GetSineTable(pSin);

	int x = RoundToInt(_x*(65536.0f/2.0f/_PI));
	x += 16384;
	x &= 0xffff;
	int Pos = x >> 8;
	int Frac = x & 0xff;

	fp4 s0 = pSin[Pos];
	fp4 s1 = pSin[(Pos+1) & 0xff];
	return s0 + (s1-s0)*fp4(Frac)*(1.0f/256.0f);
}
static M_INLINE void QSinCosi(int _x, fp4& _Sin, fp4& _Cos)
{
	// x = [0..65535] <=> [0..2*PI]

	MACRO_GetSineTable(pSin);

	_x &= 0xffff;
	int Pos = _x >> 8;
	int Frac = _x & 0xff;
	fp4 s0 = pSin[Pos];
	fp4 s1 = pSin[(Pos+1) & 0xff];
	fp4 c0 = pSin[(Pos+(MFLOAT_SINETABLESIZE >> 2)) & 0xff];
	fp4 c1 = pSin[(Pos+(MFLOAT_SINETABLESIZE >> 2) + 1) & 0xff];
	_Sin = s0 + (s1-s0)*fp4(Frac)*(1.0f/256.0f);
	_Cos = c0 + (c1-c0)*fp4(Frac)*(1.0f/256.0f);
}

static M_INLINE void QSinCos(fp4 _x, fp4& _Sin, fp4& _Cos)
{
	// x = [0..65535] <=> [0..2*PI]

	MACRO_GetSineTable(pSin);

	int x = RoundToInt(_x*(65536.0f/2.0f/_PI));
	x &= 0xffff;
	int Pos = x >> 8;
	int Frac = x & 0xff;
	fp4 s0 = pSin[Pos];
	fp4 s1 = pSin[(Pos+1) & 0xff];
	fp4 c0 = pSin[(Pos+(MFLOAT_SINETABLESIZE >> 2)) & 0xff];
	fp4 c1 = pSin[(Pos+(MFLOAT_SINETABLESIZE >> 2) + 1) & 0xff];
	_Sin = s0 + (s1-s0)*fp4(Frac)*(1.0f/256.0f);
	_Cos = c0 + (c1-c0)*fp4(Frac)*(1.0f/256.0f);
}

static M_INLINE void QSinCosUnit(fp4 _x, fp4& _Sin, fp4& _Cos)
{
	// x = [0..65536] <=> [0..1]

	MACRO_GetSineTable(pSin);

	int x = RoundToInt(_x*65536.0f);
	x &= 0xffff;
	int Pos = x >> 8;
	int Frac = x & 0xff;
	fp4 s0 = pSin[Pos];
	fp4 s1 = pSin[(Pos+1) & 0xff];
	fp4 c0 = pSin[(Pos+(MFLOAT_SINETABLESIZE >> 2)) & 0xff];
	fp4 c1 = pSin[(Pos+(MFLOAT_SINETABLESIZE >> 2) + 1) & 0xff];
	_Sin = s0 + (s1-s0)*fp4(Frac)*(1.0f/256.0f);
	_Cos = c0 + (c1-c0)*fp4(Frac)*(1.0f/256.0f);
}

#define MACRO_GetSinCos(var)	\
	fp4 sin##var, cos##var;		\
	QSinCosi(var, sin##var, cos##var);


static M_INLINE fp4 QAtan(fp4 _a)
{
	fp4 a = _a;
	if (a > 1.0f)
	{
		a = 1.0f / a;
		fp4 a2 = Sqr(a);
		fp4 a4 = Sqr(a2);
		return
			fp4(_PI) / 2.0f -
			(a - 
			a*a2/3.0f + 
			a*a4 / 5.0f - 
			a*a2*a4 / 7.0f);
	}
	else
	{
		fp4 a2 = Sqr(a);
		fp4 a4 = Sqr(a2);
		return
			a - 
			a*a2/3.0f + 
			a*a4 / 5.0f - 
			a*a2*a4 / 7.0f;
	}
}

static M_INLINE fp4 QAcos(fp4 _a)
{
	if(_a < 0.0f)
		return _PI - QAtan(M_Sqrt(1.0f - Sqr(_a)) / -_a);
	else
		return QAtan(M_Sqrt(1.0f - Sqr(_a)) / _a);
}

static M_INLINE fp4 QAcos2(fp4 _a)
{
	if(_a < 0.0f)
		return _PI - QAtan(M_Sqrt(1.0f - Sqr(_a)) / -_a);
	else
		return QAtan(M_Sqrt(1.0f - Sqr(_a)) / _a);
}


static M_INLINE int FloatIsNAN(const fp4 &_Number)
{
	if (( (*((uint32*)&_Number)) & 0x7F800000) == 0x7F800000)
	{
		if ((*((uint32*)&_Number)) & 0x007FFFFF)
			return true;
		else
			return false;
	}
	else
		return false;
}

static M_INLINE int FloatIsInfinite(const fp4 &_Number)
{
	if (((*((uint32*)&_Number)) & 0x7F800000) == 0x7F800000)
	{
		if (!((*((uint32*)&_Number)) & 0x007FFFFF))
			return true;
		else 
			return false;
	}
	else 
		return false;
}

static M_INLINE bool FloatInRange( const fp4& _Number, const fp4 _Low, const fp4 _High )
{
	return ( ( _Number >= _Low ) && ( _Number <= _High ) );
}

static M_INLINE bool FloatInRange( const fp8& _Number, const fp8 _Low, const fp8 _High )
{
	return ( ( _Number >= _Low ) && ( _Number <= _High ) );
}

static M_INLINE int FloatIsInvalid(const fp4 &_Number)
{
	return (((*((uint32*)&_Number)) & 0x7F800000) == 0x7F800000);
}

static M_INLINE int FloatIsNAN(const fp8 &_Number)
{
	if (((*((uint64*)&_Number)) & 0x7FF0000000000000LL) == 0x7FF0000000000000LL)
	{
		if ((*((uint64*)&_Number)) & 0x000FFFFFFFFFFFFFLL)
			return true;
		else
			return false;
	}
	else
		return false;
}

static M_INLINE int FloatIsInfinite(const fp8 &_Number)
{
	if (((*((uint64*)&_Number)) & 0x7FF0000000000000LL) == 0x7FF0000000000000LL)
	{
		if (!((*((uint64*)&_Number)) & 0x000FFFFFFFFFFFFFLL))
			return true;
		else
			return false;
	}
	else
		return false;
}

static M_INLINE int FloatIsInvalid(const fp8 &_Number)
{
	return ((*((uint64*)&_Number)) & 0x7FF0000000000000LL) == 0x7FF0000000000000LL;
}

// -------------------------------------------------------------------

static M_INLINE uint32 FloatPack32(fp4 _X, fp4 _Max)
{
	return (uint32)(Clamp01((_X / (_Max * 2.0f)) + 0.5f) * (fp4)0xFFFFFFFF);
}

static M_INLINE fp4 FloatUnpack32(uint32 _I, fp4 _Max)
{
	return (((fp4)(_I & 0xFFFFFFFF) / (fp4)0xFFFFFFFF) - 0.5f) * (_Max * 2.0f);
}

// -------------------------------------------------------------------

static M_INLINE uint16 FloatPack16(fp4 _X, fp4 _Max)
{
	return (uint16)(Clamp01((_X / (_Max * 2.0f)) + 0.5f) * (fp4)0xFFFF);
}

static M_INLINE fp4 FloatUnpack16(uint16 _I, fp4 _Max)
{
	return (((fp4)(_I & 0xFFFF) / (fp4)0xFFFF) - 0.5f) * (_Max * 2.0f);
}

// -------------------------------------------------------------------

static M_INLINE uint8 FloatPack8(fp4 _X, fp4 _Max)
{
	return (uint8)(Clamp01((_X / (_Max * 2.0f)) + 0.5f) * (fp4)0xFF);
}

static M_INLINE fp4 FloatUnpack8(uint8 _I, fp4 _Max)
{
	return (((fp4)(_I & 0xFF) / (fp4)0xFF) - 0.5f) * (_Max * 2.0f);
}

// -------------------------------------------------------------------

static M_INLINE uint32 FloatPack(fp4 _X, fp4 _Max, uint8 _nBits)
{
	uint32 MaxBits = ((1 << _nBits) - 1);
	return (uint32)(Clamp01((_X / (_Max * 2.0f)) + 0.5f) * (fp4)MaxBits);
}

static M_INLINE fp4 FloatUnpack(uint8 _I, fp4 _Max, uint8 _nBits)
{
	uint32 MaxBits = ((1 << _nBits) - 1);
	return (((fp4)(_I & MaxBits) / (fp4)MaxBits) - 0.5f) * (_Max * 2.0f);
}

// -------------------------------------------------------------------

#endif // _INC_MFLOAT
