
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

//#include "GC/GC_Float.h"
//#include "MFloat_PS2.h"
#include "MFloat_PS3.h"
#include "MFloat_Xenon.h"
#include "MFloat_x86.h"

void MCCDLLEXPORT MFloat_Init();
void MCCDLLEXPORT MFloat_GetSineTable(const fp32*& _pSin);

MCCDLLEXPORT const fp32 *MFloat_GetRandTable();
MCCDLLEXPORT fp32 MFloat_GetRand(int _iIndex);

#define MACRO_GetSineTable(Var) const fp32* Var; MFloat_GetSineTable(Var);

// -------------------------------------------------------------------

#define LERP(a,b,t) ((a) + ((b) - (a))*t) // Added by Mondelore

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| INDEX
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifdef NEVER

int FloatIsNeg(fp32 _Val);
fp32 Sign(fp32 _x);							// Sign(x) à la speedy gonzales
fp32 Clamp01(fp32 _Val);						// 12c
fp32 ClampRange(fp32 _Val,fp32 _Range);		// 12c
fp32 Ceil1(fp32 _Val);						// 12c
fp32 Ceilx(fp32 _Val, fp32 _Range);			// 12c
int TruncToInt(fp32 _Val);					// 25c, standard c typecast int() uses ~55 cycles.
int RoundToInt(fp32 _Val);					// 15c
void RoundToInt(const fp32* _pVal, int* _pResult, int _nVal);
#define floor Floor
#define ceil Ceil
int RoundRGB(fp32 _r, fp32 _g, fp32 _b);
void MCCDLLEXPORT AsyncRecp(fp32 _v);
fp32 MCCDLLEXPORT AsyncRecpGet();
void MCCDLLEXPORT AsyncRecpFree();

int FloatIsNeg(fp64 _Val);
fp64 Clamp01(fp64 _Val);
fp64 ClampRange(fp64 _Val,fp64 _Range);
fp64 Ceil1(fp64 _Val);
fp64 Ceilx(fp64 _Val, fp64 _Range);
int TruncToInt(fp64 _Val);
int TruncToInt64(fp64 _Val);
int RoundToInt(fp64 _Val);
fp64 SnapFloat(fp64 _Val, fp64 _Grid, fp64 _SnapTresh);

void Moderate(int& x, int newx, int& xprim, int a);
void Moderatef(fp32& _x, fp32 _newx, fp32& _xprim, int a);
fp32 GetFade(fp32 _Time, fp32 _Duration, fp32 _FadeInTime, fp32 _FadeOutTime);

// -------------------------------------------------------------------
fp32 QSini(int _x);							// 28c,  | QSin(x) - sin(x) | < 10^-4, // x = [0..65536] <=> [0..2*PI]
fp32 QCosi(int _x);							// 28c,  | QSin(x) - sin(x) | < 10^-4, // x = [0..65536] <=> [0..2*PI]
fp32 QSin(fp32 _x);							// 66c
fp32 QCos(fp32 _x);							// 66c
void QSinCosi(int _x, fp32& _Sin, fp32& _Cos);// <80c
void QSinCos(fp32 _x, fp32& _Sin, fp32& _Cos);	// <80c

fp32 QAtan(fp32 _a);							// Taylor polynom accelerated arctan-approximation.
fp32 QAcos(fp32 _a);							// Arc-cosine using QAtan and fsqrt().
fp32 QAcos2(fp32 _a);							// Arc-cosine using QAtan.

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
M_INLINE fp32 Abs(fp32 _v) { return M_Fabs(_v); }
#ifndef PLATFORM_PS2
template<>
M_INLINE fp64 Abs(fp64 _v) { return M_Fabs(_v); }
#endif	// PLATFORM_PS2
template<>
M_INLINE fp32 Min3<fp32>(fp32 _a, fp32 _b, fp32 _c)
{
	return Min( _a, Min( _b, _c ) );
}

template<>
M_INLINE fp32 Max3<fp32>(fp32 _a, fp32 _b, fp32 _c)
{
	return Max( _a, Max( _b, _c ) );
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Float utilities
|__________________________________________________________________________________________________
\*************************************************************************************************/
static M_INLINE void RoundToInt(const fp32* _pVal, int* _pResult, int _nVal)
{
	for(int i = 0; i < _nVal; i++) _pResult[i] = RoundToInt(*_pVal);
}


//	Better to just have the fp64 versions to get rid of all ambiguity.

#define M_Floor	Floor
#define M_Ceil	Ceil
// -------------------------------------------------------------------
#if !defined(PLATFORM_PS2) && !defined(PLATFORM_DOLPHIN)
static M_INLINE fp64 Ceil1(fp64 _Val)
{
	return (_Val - M_Fabs(1.0f - _Val) + 1.0f)*.5;
}

static M_INLINE fp64 Ceilx(fp64 _Val, fp64 _Range)
{
	return (_Val - M_Fabs(_Range - _Val) + _Range)*.5;
}
#endif

#define floor Floor
#define ceil Ceil

#ifdef	CPU_SOFTWARE_FP64
static M_INLINE fp32 SnapFloat(fp32 _Val, fp32 _Grid, fp32 _SnapTresh)
{
	fp32 Val = fp32(RoundToInt(_Val / _Grid)) * _Grid;
	if (M_Fabs(Val - _Val) > _SnapTresh) Val = _Val;
	return Val;
}
#else
static M_INLINE fp64 SnapFloat(fp64 _Val, fp64 _Grid, fp64 _SnapTresh)
{
	fp64 Val = fp64(RoundToInt(_Val / _Grid)) * _Grid;
	if (M_Fabs(Val - _Val) > _SnapTresh) Val = _Val;
	return Val;
}
#endif

// -------------------------------------------------------------------
static M_INLINE void Moderate(int& x, int newx, int& xprim, int a)
{
	int xbiss = (int)((-fp32(a)*fp32(xprim) - fp32(x-newx)*Sqr(fp32(a))*0.25f) * (1.0f/256.0f));
	xprim += xbiss;
	x += (xprim >> 8);
}

static M_INLINE void Moderatef(fp32& _x, fp32 _newx, fp32& _xprim, int a)
{
	int xprim = RoundToInt(_xprim*256.0f);
	int x = RoundToInt(_x*256.0f);
	int newx = RoundToInt(_newx*256.0f);
	Moderate(x, newx, xprim, a);
	_xprim = fp32(xprim) * (1.0f/256.0f);
	_x = fp32(x) * (1.0f/256.0f);
}


// -------------------------------------------------------------------

static M_INLINE fp32 GetFade(fp32 _Time, fp32 _Duration, fp32 _FadeInTime, fp32 _FadeOutTime)
{
	fp32 RevTime = _Duration - _Time;
	fp32 Fade, FadeIn, FadeOut;

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

static M_INLINE fp32 QSini(int _x)		// 28c,  abs( QSin(x) - sin(x) ) < 10^-4
{
	// x = [0..65535] <=> [0..2*PI]

	MACRO_GetSineTable(pSin);

	_x &= 0xffff;
	int Pos = _x >> 8;
	int Frac = _x & 0xff;
	fp32 s0 = pSin[Pos];
	fp32 s1 = pSin[(Pos+1) & 0xff];
	return s0 + (s1-s0)*fp32(Frac)*(1.0f/256.0f);
}

static M_INLINE fp32 QCosi(int _x)		// 28c,  abs( QSin(x) - sin(x) ) < 10^-4
{
	// x = [0..65535] <=> [0..2*PI]

	MACRO_GetSineTable(pSin);

	_x += 16384;
	_x &= 0xffff;
	int Pos = _x >> 8;
	int Frac = _x & 0xff;
	fp32 s0 = pSin[Pos];
	fp32 s1 = pSin[(Pos+1) & 0xff];
	return s0 + (s1-s0)*fp32(Frac)*(1.0f/256.0f);
}

static M_INLINE fp32 QSin(fp32 _x)		// 66c
{
	// x = [0..65535] <=> [0..2*PI]
	// Is the above really correct? Shouldn't the input range be [0..1]?!

	MACRO_GetSineTable(pSin);

	int x = RoundToInt(_x*(65536.0f/2.0f/_PI));
	x &= 0xffff;
	int Pos = x >> 8;
	int Frac = x & 0xff;

	fp32 s0 = pSin[Pos];
	fp32 s1 = pSin[(Pos+1) & 0xff];
	return s0 + (s1-s0)*fp32(Frac)*(1.0f/256.0f);
}

static M_INLINE fp32 QCos(fp32 _x)		// 66c
{
	// x = [0..65535] <=> [0..2*PI]
	// Is the above really correct? Shouldn't the input range be [0..1]?!

	MACRO_GetSineTable(pSin);

	int x = RoundToInt(_x*(65536.0f/2.0f/_PI));
	x += 16384;
	x &= 0xffff;
	int Pos = x >> 8;
	int Frac = x & 0xff;

	fp32 s0 = pSin[Pos];
	fp32 s1 = pSin[(Pos+1) & 0xff];
	return s0 + (s1-s0)*fp32(Frac)*(1.0f/256.0f);
}
static M_INLINE void QSinCosi(int _x, fp32& _Sin, fp32& _Cos)
{
	// x = [0..65535] <=> [0..2*PI]

	MACRO_GetSineTable(pSin);

	_x &= 0xffff;
	int Pos = _x >> 8;
	int Frac = _x & 0xff;
	fp32 s0 = pSin[Pos];
	fp32 s1 = pSin[(Pos+1) & 0xff];
	fp32 c0 = pSin[(Pos+(MFLOAT_SINETABLESIZE >> 2)) & 0xff];
	fp32 c1 = pSin[(Pos+(MFLOAT_SINETABLESIZE >> 2) + 1) & 0xff];
	_Sin = s0 + (s1-s0)*fp32(Frac)*(1.0f/256.0f);
	_Cos = c0 + (c1-c0)*fp32(Frac)*(1.0f/256.0f);
}

static M_INLINE void QSinCos(fp32 _x, fp32& _Sin, fp32& _Cos)
{
	// x = [0..65535] <=> [0..2*PI]

	MACRO_GetSineTable(pSin);

	int x = RoundToInt(_x*(65536.0f/2.0f/_PI));
	x &= 0xffff;
	int Pos = x >> 8;
	int Frac = x & 0xff;
	fp32 s0 = pSin[Pos];
	fp32 s1 = pSin[(Pos+1) & 0xff];
	fp32 c0 = pSin[(Pos+(MFLOAT_SINETABLESIZE >> 2)) & 0xff];
	fp32 c1 = pSin[(Pos+(MFLOAT_SINETABLESIZE >> 2) + 1) & 0xff];
	_Sin = s0 + (s1-s0)*fp32(Frac)*(1.0f/256.0f);
	_Cos = c0 + (c1-c0)*fp32(Frac)*(1.0f/256.0f);
}

static M_INLINE void QSinCosUnit(fp32 _x, fp32& _Sin, fp32& _Cos)
{
	// x = [0..65536] <=> [0..1]

	MACRO_GetSineTable(pSin);

	int x = RoundToInt(_x*65536.0f);
	x &= 0xffff;
	int Pos = x >> 8;
	int Frac = x & 0xff;
	fp32 s0 = pSin[Pos];
	fp32 s1 = pSin[(Pos+1) & 0xff];
	fp32 c0 = pSin[(Pos+(MFLOAT_SINETABLESIZE >> 2)) & 0xff];
	fp32 c1 = pSin[(Pos+(MFLOAT_SINETABLESIZE >> 2) + 1) & 0xff];
	_Sin = s0 + (s1-s0)*fp32(Frac)*(1.0f/256.0f);
	_Cos = c0 + (c1-c0)*fp32(Frac)*(1.0f/256.0f);
}

#define MACRO_GetSinCos(var)	\
	fp32 sin##var, cos##var;		\
	QSinCosi(var, sin##var, cos##var);


static M_INLINE fp32 QAtan(fp32 _a)
{
	fp32 a = _a;
	if (a > 1.0f)
	{
		a = 1.0f / a;
		fp32 a2 = Sqr(a);
		fp32 a4 = Sqr(a2);
		return
			fp32(_PI) / 2.0f -
			(a - 
			a*a2/3.0f + 
			a*a4 / 5.0f - 
			a*a2*a4 / 7.0f);
	}
	else
	{
		fp32 a2 = Sqr(a);
		fp32 a4 = Sqr(a2);
		return
			a - 
			a*a2/3.0f + 
			a*a4 / 5.0f - 
			a*a2*a4 / 7.0f;
	}
}

static M_INLINE fp32 QAcos(fp32 _a)
{
	if(_a < 0.0f)
		return _PI - QAtan(M_Sqrt(1.0f - Sqr(_a)) / -_a);
	else
		return QAtan(M_Sqrt(1.0f - Sqr(_a)) / _a);
}

static M_INLINE fp32 QAcos2(fp32 _a)
{
	if(_a < 0.0f)
		return _PI - QAtan(M_Sqrt(1.0f - Sqr(_a)) / -_a);
	else
		return QAtan(M_Sqrt(1.0f - Sqr(_a)) / _a);
}


static M_INLINE int FloatIsNAN(const fp32 &_Number)
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

static M_INLINE int FloatIsInfinite(const fp32 &_Number)
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

static M_INLINE bool FloatInRange( const fp32& _Number, const fp32 _Low, const fp32 _High )
{
	return ( ( _Number >= _Low ) && ( _Number <= _High ) );
}

static M_INLINE bool FloatInRange( const fp64& _Number, const fp64 _Low, const fp64 _High )
{
	return ( ( _Number >= _Low ) && ( _Number <= _High ) );
}

static M_INLINE int FloatIsInvalid(const fp32 &_Number)
{
	return (((*((uint32*)&_Number)) & 0x7F800000) == 0x7F800000);
}

static M_INLINE int FloatIsNAN(const fp64 &_Number)
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

static M_INLINE int FloatIsInfinite(const fp64 &_Number)
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

static M_INLINE int FloatIsInvalid(const fp64 &_Number)
{
	return ((*((uint64*)&_Number)) & 0x7FF0000000000000LL) == 0x7FF0000000000000LL;
}

// -------------------------------------------------------------------

static M_INLINE uint32 FloatPack32(fp32 _X, fp32 _Max)
{
	return (uint32)(Clamp01((_X / (_Max * 2.0f)) + 0.5f) * (fp32)0xFFFFFFFF);
}

static M_INLINE fp32 FloatUnpack32(uint32 _I, fp32 _Max)
{
	return (((fp32)(_I & 0xFFFFFFFF) / (fp32)0xFFFFFFFF) - 0.5f) * (_Max * 2.0f);
}

// -------------------------------------------------------------------

static M_INLINE uint16 FloatPack16(fp32 _X, fp32 _Max)
{
	return (uint16)(Clamp01((_X / (_Max * 2.0f)) + 0.5f) * (fp32)0xFFFF);
}

static M_INLINE fp32 FloatUnpack16(uint16 _I, fp32 _Max)
{
	return (((fp32)(_I & 0xFFFF) / (fp32)0xFFFF) - 0.5f) * (_Max * 2.0f);
}

// -------------------------------------------------------------------

static M_INLINE uint8 FloatPack8(fp32 _X, fp32 _Max)
{
	return (uint8)(Clamp01((_X / (_Max * 2.0f)) + 0.5f) * (fp32)0xFF);
}

static M_INLINE fp32 FloatUnpack8(uint8 _I, fp32 _Max)
{
	return (((fp32)(_I & 0xFF) / (fp32)0xFF) - 0.5f) * (_Max * 2.0f);
}

// -------------------------------------------------------------------

static M_INLINE uint32 FloatPack(fp32 _X, fp32 _Max, uint8 _nBits)
{
	uint32 MaxBits = ((1 << _nBits) - 1);
	return (uint32)(Clamp01((_X / (_Max * 2.0f)) + 0.5f) * (fp32)MaxBits);
}

static M_INLINE fp32 FloatUnpack(uint8 _I, fp32 _Max, uint8 _nBits)
{
	uint32 MaxBits = ((1 << _nBits) - 1);
	return (((fp32)(_I & MaxBits) / (fp32)MaxBits) - 0.5f) * (_Max * 2.0f);
}

// -------------------------------------------------------------------

#endif // _INC_MFLOAT
