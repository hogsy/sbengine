
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef _INC_MMISC
#define _INC_MMISC

#include "MCCInc.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTypeDesc, Numerical-type descriptors
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*class CTypeDesc
{
public:
	virtual fp64 GetMin() { return 0; };
	virtual fp64 GetMax() { return 0; };
	virtual fp64 GetEpsilon() { return 0; };
};

class CTypeDesc_fp32
{
public:
	static const fp32 ms_cMin;
	static const fp32 ms_cMax;
	static const fp32 ms_cEpsilon;

//	virtual fp64 GetMin() { return ms_cMin; };
//	virtual fp64 GetMax() { return ms_cMax; };
//	virtual fp64 GetEpsilon() { return ms_cEpsilon; };
};

class CTypeDesc_fp64
{
public:
	static const fp64 ms_cMin;
	static const fp64 ms_cMax;
	static const fp64 ms_cEpsilon;

//	virtual fp64 GetMin() { return ms_cMin; };
//	virtual fp64 GetMax() { return ms_cMax; };
//	virtual fp64 GetEpsilon() { return ms_cEpsilon; };
};

class CTypeDesc_int8
{
public:
	static const int ms_cMin;
	static const int ms_cMax;
	static const int ms_cEpsilon;

//	virtual fp64 GetMin() { return ms_cMin; };
//	virtual fp64 GetMax() { return ms_cMax; };
//	virtual fp64 GetEpsilon() { return ms_cEpsilon; };
};

class CTypeDesc_int16
{
public:
	static const int ms_cMin;
	static const int ms_cMax;
	static const int ms_cEpsilon;

//	virtual fp64 GetMin() { return ms_cMin; };
//	virtual fp64 GetMax() { return ms_cMax; };
//	virtual fp64 GetEpsilon() { return ms_cEpsilon; };
};

class CTypeDesc_int32
{
public:
	static const int ms_cMin;
	static const int ms_cMax;
	static const int ms_cEpsilon;

//	virtual fp64 GetMin() { return ms_cMin; };
//	virtual fp64 GetMax() { return ms_cMax; };
//	virtual fp64 GetEpsilon() { return ms_cEpsilon; };
};

class CTypeDesc_uint8
{
public:
	static const unsigned int ms_cMin;
	static const unsigned int ms_cMax;
	static const unsigned int ms_cEpsilon;

//	virtual fp64 GetMin() { return ms_cMin; };
//	virtual fp64 GetMax() { return ms_cMax; };
//	virtual fp64 GetEpsilon() { return ms_cEpsilon; };
};

class CTypeDesc_uint16
{
public:
	static const unsigned int ms_cMin;
	static const unsigned int ms_cMax;
	static const unsigned int ms_cEpsilon;

//	virtual fp64 GetMin() { return ms_cMin; };
//	virtual fp64 GetMax() { return ms_cMax; };
//	virtual fp64 GetEpsilon() { return ms_cEpsilon; };
};

class CTypeDesc_uint32
{
public:
	static const unsigned int ms_cMin;
	static const unsigned int ms_cMax;
	static const unsigned int ms_cEpsilon;

//	virtual fp64 GetMin() { return ms_cMin; };
//	virtual fp64 GetMax() { return ms_cMax; };
//	virtual fp64 GetEpsilon() { return ms_cEpsilon; };
};

class CTypeDesc_int
{
public:
	static const int ms_cMin;
	static const int ms_cMax;
	static const int ms_cEpsilon;

//	virtual fp64 GetMin() { return ms_cMin; };
//	virtual fp64 GetMax() { return ms_cMax; };
//	virtual fp64 GetEpsilon() { return ms_cEpsilon; };
};
*/

template<typename T>
struct TNumericProperties
{
	static T Min() { return static_cast<T>(0); }
	static T Max() { return static_cast<T>(0); }
	static T Epsilon() { return static_cast<T>(0); }
	static T Zero() { return static_cast<T>(0); }
	static T One() { return static_cast<T>(0); }
	static T QuietNaN() { return static_cast<T>(0); }
	static T NaN() { return static_cast<T>(0); }
};

template<>
struct TNumericProperties<fp32>
{
	M_FORCEINLINE static fp32 Min() { return _FP32_MIN; }
	M_FORCEINLINE static fp32 Max() { return _FP32_MAX; }
	M_FORCEINLINE static fp32 Epsilon() { return _FP32_EPSILON; }
	M_FORCEINLINE static fp32 Zero() { return 0.0f; }
	M_FORCEINLINE static fp32 One() { return 1.0f; }
	M_FORCEINLINE static fp32 QuietNaN() 
	{
		uint32 nan = 0x7fc00000;
		return *( float* )&nan;
	}

	M_FORCEINLINE static fp32 NaN() 
	{
		uint32 nan = 0x7fc00001;
		return *( float* )&nan;
	}
};

template<>
struct TNumericProperties<fp64>
{
	M_FORCEINLINE static fp64 Min() { return _FP64_MIN; }
	M_FORCEINLINE static fp64 Max() { return _FP64_MAX; }
	M_FORCEINLINE static fp64 Epsilon() { return _FP64_EPSILON; }
	M_FORCEINLINE static fp64 Zero() { return 0.0; }
	M_FORCEINLINE static fp64 One() { return 1.0; }
	M_FORCEINLINE static fp64 QuietNaN() 
	{ 
		uint64 nan = 0x7ff8000000000000LL;
		return *(fp64 *) &nan;		
	}
	M_FORCEINLINE static fp64 NaN() 
	{ 
		uint64 nan = 0x7ff8000100000000LL;
		return *(fp64 *) &nan;		
	}	
};

typedef int CTypeDesc_fp32;
typedef int CTypeDesc_fp64;
typedef int CTypeDesc_int8;
typedef int CTypeDesc_int16;
typedef int CTypeDesc_int32;
typedef int CTypeDesc_uint8;
typedef int CTypeDesc_uint16;
typedef int CTypeDesc_uint32;
typedef int CTypeDesc_int;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Memory copy/fill
|__________________________________________________________________________________________________
\*************************************************************************************************/
inline void MCCDLLEXPORT MemSet(void* _pDest, int _Value, int _Count)
{
	memset(_pDest, _Value, _Count);
}

void MCCDLLEXPORT MemSetW(void* dest, int value, int count);			// Fill with 16-bit words
void MCCDLLEXPORT MemSetD(void* dest, int value, int count);			// Fill with 32-bit dwords

inline void MCCDLLEXPORT MemMove(void* _pDest, const void* _pSrc, int _Count)
{
	memmove(_pDest, _pSrc, _Count);
}

// Dest, count, value
#define FillChar(a,b,c) {				\
	if (a != NULL) MemSet(a,c,b);		\
}; 

#define FillW(a,b,c) {					\
	if (a != NULL) MemSetW(a,c,b);		\
}; 

#define FillD(a,b,c) {					\
	if (a != NULL) MemSetD(a,c,b);		\
}; 

#define _Move(a,b,c) {					\
	if ((c) > 0) {						\
		if (a == NULL) Error_static("Move", "Source NULL pointer.");		\
		if (b == NULL) Error_static("Move", "Destination NULL pointer.");	\
		MemMove(b,a,c);					\
	};									\
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Standard defs.'n templates
|__________________________________________________________________________________________________
\*************************************************************************************************/
template <class T>
inline T Sqr(T a) { T tmp(a); return tmp*tmp; }

//#define Random (((fp32)rand())*(1.0f/((fp32)RAND_MAX)))
#define Random MRTC_GetRand()->GenRand1Inclusive_fp32()

// --- Min/Max ---
//#ifdef COMPILER_CODEWARRIOR
//#define Min( a, b ) (((a) < (b)) ? a : b )
//#define Max( a, b ) (((a) > (b)) ? a : b )
//#else

// --- Min/Max Mixed-Types ---
template <class T1, class T2>
inline T1 MinMT(T1 a, T2 b)
{
	return ((a < b) ? a : b);
};

template <class T1, class T2> 
inline T1 MaxMT(T1 a, T2 b)
{
	return ((a > b) ? a : b);
};

// --- Min/Max of three values. ---
template <class T> 
inline T Min3(T a, T b, T c)
{
	if (a < b)
		return (a < c) ? a : c;
	else
		return (b < c) ? b : c;
};

template <class T> 
inline T Max3(T a, T b, T c)
{
	if (a > b)
		return (a > c) ? a : c;
	else
		return (b > c) ? b : c;
};

// -------------------------------------------------------------------
inline int Log2(int x)
{
	int i = 0;
	while (x > 1)
	{
		x >>= 1;
		i++;
	};
	return i;
};

inline bool IsPow2(int _x)
{
	return ((1 << Log2(_x)) == _x);
};

inline int GetGEPow2(int _a)
{
	int x = 1;
	while (x < _a) x += x;
	return x;
};

template<class T>
void IncPtr(T** Dest, int Value)
{
	int* _Dest = (int*) Dest;
	*_Dest += Value;
};

template <class T>
void Swap(T &a, T &b)
{
	T c;
	c = a;
	a = b; 
	b = c;
};

template <class T>
T Length2(T a, T b)
{
	return (T)M_Sqrt(Sqr(a) + Sqr(b));
};

template <class T>
T Length3(T a, T b, T c)
{
	return (T)M_Sqrt(Sqr(a) + Sqr(b) + Sqr(c));
};

// -------------------------------------------------------------------
inline int BitScanFwd32(uint32 _Value)
{
#ifdef CPU_INTELP5
	int Val = _Value;
	__asm
	{
		mov eax, [Val]
		mov ebx, -1
		or eax, eax
		jz Done
		bsf ebx, eax
Done:
		mov [Val], ebx
	}
	return Val;

#else
	if (_Value == 0) return -1;

	int cnt = 0;
	while (!(_Value & 1)) { _Value >>= 1; cnt++; }
	return cnt;
#endif
};

inline int BitScanBwd32(uint32 _Value)
{
#ifdef CPU_INTELP5
	int Val = _Value;
	__asm
	{
		mov eax, [Val]
		mov ebx, -1
		or eax, eax
		jz Done
		bsr ebx, eax
Done:
		mov [Val], ebx
	}
	return Val;

#else
	if (_Value == 0) return -1;

	int cnt = 31;
	while (!(_Value & 0x80000000)) { _Value <<= 1; cnt--; }
	return cnt;
#endif
};

inline int BitCount(uint32 _Value)
{
	int nBits = 0;
	for (int i = 0; i < 32; ++i)
	{
		if (_Value & (1 << i))
			++nBits;
	}
	return nBits;
};


template <typename t_CType>
M_INLINE void Swap_NoSwap(t_CType &_ToSwap)
{
}
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Byter order swappers
|__________________________________________________________________________________________________
\*************************************************************************************************/
inline void WordSwap_int32(int32& _Value) { Swap(((int16*)&_Value)[0], ((int16*)&_Value)[1]); };
inline void WordSwap_uint32(uint32& _Value) { Swap(((int16*)&_Value)[0], ((int16*)&_Value)[1]); };

// -------------------------------------------------------------------
#ifdef CPU_INTELP5

inline void Swap_uint16(uint16& _Value) { Swap(((int8*)&_Value)[0], ((int8*)&_Value)[1]); };
inline void Swap_int16(int16& _Value) { Swap(((int8*)&_Value)[0], ((int8*)&_Value)[1]); };

inline void ByteSwap_uint32(uint32& _Value) 
{
	__asm 
	{
		lea eax, _Value
		mov eax,[eax]
		mov ebx,[eax]
		bswap ebx
		mov [eax],ebx
	}; 
}
inline void ByteSwap_int32(int32& _Value) 
{
	__asm 
	{
		lea eax, _Value 
		mov eax,[eax]
		mov ebx,[eax]
		bswap ebx
		mov [eax],ebx
	}; 
}

inline void ByteSwap_int64(int64& _Value)
{
	int64 Val = _Value;
	int32 Val0 = (int32)(Val >> 32);
	int32 Val1 = (int32)Val;

	ByteSwap_int32(Val0);
	ByteSwap_int32(Val1);

	_Value = ((int64)Val1 << 32) | (int64)Val0;
}

inline void ByteSwap_uint64(uint64& _Value)
{
	uint64 Val = _Value;
	uint32 Val0 = (uint32)(Val >> 32);
	uint32 Val1 = (uint32)Val;

	ByteSwap_uint32(Val0);
	ByteSwap_uint32(Val1);

	_Value = ((uint64)Val1 << 32) | (uint64)Val0;
}

// -------------------------------------------------------------------
#elif defined(PLATFORM_DOLPHIN)
inline void Swap_uint16(register uint16& _Value)
{
	register int32 tmp;
	asm
	{
		lhz tmp, 0(_Value)
		sthbrx tmp, r0, _Value
	}
}
inline void Swap_int16(register int16& _Value)
{
	register int32 tmp;
	asm
	{
		lhz tmp, 0(_Value)
		sthbrx tmp, r0, _Value
	}
}
inline void ByteSwap_uint32(register uint32& _Value)
{
	register int32 tmp;
	asm
	{
		lwz tmp, 0(_Value)
		stwbrx tmp, r0, _Value
	}
}
inline void ByteSwap_int32(register int32& _Value)
{
	register int32 tmp;
	asm
	{
		lwz tmp, 0(_Value)
		stwbrx tmp, r0, _Value
	}
}

// -------------------------------------------------------------------
#elif defined(PLATFORM_XENON)
unsigned short      _byteswap_ushort            (unsigned short     value);
unsigned long       _byteswap_ulong             (unsigned long      value);
unsigned __int64    _byteswap_uint64            (unsigned __int64   value);

inline void Swap_uint16(uint16& _Value) 
{ 
	uint16 Temp = _Value;
	_Value = _byteswap_ushort(Temp);
};
inline void Swap_int16(int16& _Value) 
{ 
	uint16 Temp = _Value;
	_Value = _byteswap_ushort(Temp);
};
inline void Swap_wchar(wchar& _Value) 
{ 
	uint16 Temp = _Value;
	_Value = _byteswap_ushort(Temp);
};

inline void ByteSwap_uint32(uint32& _Value) 
{
	uint32 Temp = _Value;
	_Value = _byteswap_ulong(Temp);
}
inline void ByteSwap_int32(int32& _Value) 
{
	uint32 Temp = _Value;
	_Value = _byteswap_ulong(Temp);
}

inline void ByteSwap_uint64(uint64& _Value)
{
	uint64 Temp = _Value;
	_Value = _byteswap_uint64(Temp);
}

inline void ByteSwap_int64(int64& _Value)
{
	uint64 Temp = _Value;
	_Value = _byteswap_uint64(Temp);
}
#else

//inline void Swap_uint16(uint16& _Value) { Swap(((int8*)&_Value)[0], ((int8*)&_Value)[1]); };
//inline void Swap_int16(int16& _Value) { Swap(((int8*)&_Value)[0], ((int8*)&_Value)[1]); };

//inline void Swap_wchar(wchar& _Value) { Swap(((int8*)&_Value)[0], ((int8*)&_Value)[1]); };

inline void Swap_uint16(uint16& _Value)
{
	uint16 Val = _Value;
	_Value = ((Val & 0xff) << 8) | ((Val >> 8) & 0xff);
}

inline void Swap_int16(int16& _Value)
{
	int16 Val = _Value;
	_Value = ((Val & 0xff) << 8) | ((Val >> 8) & 0xff);
}

inline void Swap_wchar(wchar& _Value)
{
	wchar Val = _Value;
	_Value = ((Val & 0xff) << 8) | ((Val >> 8) & 0xff);
}

inline void ByteSwap_uint32(uint32& _Value) 
{
	uint32 Val = _Value;
	_Value = ((Val & 0xff) << 24) | ((Val & 0xff00) << 8) | ((Val >> 8) & 0xff00) | ((Val >> 24) & 0xff);
}
inline void ByteSwap_int32(int32& _Value) 
{
	int32 Val = _Value;
	_Value = ((Val & 0xff) << 24) | ((Val & 0xff00) << 8) | ((Val >> 8) & 0xff00) | ((Val >> 24) & 0xff);
}

inline void ByteSwap_int64(int64& _Value)
{
	int64 Val = _Value;
	int32 Val0 = (int32)(Val >> 32);
	int32 Val1 = (int32)Val;

	ByteSwap_int32(Val0);
	ByteSwap_int32(Val1);

	_Value = ((int64)Val1 << 32) | (int64)Val0;
}

inline void ByteSwap_uint64(uint64& _Value)
{
	uint64 Val = _Value;
	uint32 Val0 = (uint32)(Val >> 32);
	uint32 Val1 = (uint32)Val;

	ByteSwap_uint32(Val0);
	ByteSwap_uint32(Val1);

	_Value = ((uint64)Val1 << 32) | (uint64)Val0;
}

#endif


inline void ByteSwap_fp32(fp32& _Value) { ByteSwap_uint32(*(uint32*)&_Value); }
inline void ByteSwap_fp64(fp64& _Value) { ByteSwap_uint64(*(uint64*)&_Value); }

// -------------------------------------------------------------------
#ifdef CPU_LITTLEENDIAN

static M_INLINE void SwapLE(uint8& _Var) {};
static M_INLINE void SwapLE(int8& _Var) {};
static M_INLINE void SwapLE(uint16& _Var) {};
static M_INLINE void SwapLE(int16& _Var) {};
static M_INLINE void SwapLE(uint32& _Var) {};
static M_INLINE void SwapLE(int32& _Var) {};
static M_INLINE void SwapLE(uint64& _Var) {};
static M_INLINE void SwapLE(int64& _Var) {};
static M_INLINE void SwapLE(fp32& _Var) {};
static M_INLINE void SwapLE(fp64& _Var) {};

#ifdef	PLATFORM_PS2
M_INLINE void SwapLE(wchar& _Var) {};
#endif

#else

static M_INLINE void SwapLE(uint8& _Var) {};
static M_INLINE void SwapLE(int8& _Var) {};
static M_INLINE void SwapLE(uint16& _Var) { Swap_uint16(_Var); };
static M_INLINE void SwapLE(int16& _Var) { Swap_int16(_Var); };
static M_INLINE void SwapLE(uint32& _Var) { ByteSwap_uint32(_Var); };
static M_INLINE void SwapLE(int32& _Var) { ByteSwap_int32(_Var); };
static M_INLINE void SwapLE(uint64& _Var) { ByteSwap_uint64(_Var); };
static M_INLINE void SwapLE(int64& _Var) { ByteSwap_int64(_Var); };
static M_INLINE void SwapLE(fp32& _Var) { ByteSwap_fp32(_Var); };
static M_INLINE void SwapLE(fp64& _Var) { ByteSwap_fp64(_Var); };

#endif

// -------------------------------------------------------------------
#ifdef CPU_LITTLEENDIAN

	#define MACRO_MAKE_SWITCH_LE_FUNC(Name, T, SwitchFunc)	\
	inline void Name(T* _pElem, int _nElem) {}

#else

	#define MACRO_MAKE_SWITCH_LE_FUNC(Name, T, SwitchFunc)	\
	inline void Name(T* _pElem, int _nElem)						\
	{														\
			for(int i = 0; i < _nElem; SwitchFunc(_pElem[i++])) {}	\
	}

#endif

#ifdef CPU_BIGENDIAN

	#define MACRO_MAKE_SWITCH_BE_FUNC(Name, T, SwitchFunc)	\
	inline void Name(T* _pElem, int _nElem) {}

#else

	#define MACRO_MAKE_SWITCH_BE_FUNC(Name, T, SwitchFunc)	\
	inline void Name(T* _pElem, int _nElem)						\
	{														\
			for(int i = 0; i < _nElem; SwitchFunc(_pElem[i++])) {};	\
	}

#endif

MACRO_MAKE_SWITCH_LE_FUNC(SwitchArrayLE_int16, int16, Swap_int16)
MACRO_MAKE_SWITCH_LE_FUNC(SwitchArrayLE_uint16, uint16, Swap_uint16)
MACRO_MAKE_SWITCH_LE_FUNC(SwitchArrayLE_int32, int32, ByteSwap_int32)
MACRO_MAKE_SWITCH_LE_FUNC(SwitchArrayLE_uint32, uint32, ByteSwap_uint32)
MACRO_MAKE_SWITCH_LE_FUNC(SwitchArrayLE_fp32, fp32, ByteSwap_fp32)
MACRO_MAKE_SWITCH_LE_FUNC(SwitchArrayLE_fp64, fp64, ByteSwap_fp64)

MACRO_MAKE_SWITCH_BE_FUNC(SwitchArrayBE_int16, int16, Swap_int16)
MACRO_MAKE_SWITCH_BE_FUNC(SwitchArrayBE_uint16, uint16, Swap_uint16)
MACRO_MAKE_SWITCH_BE_FUNC(SwitchArrayBE_int32, int32, ByteSwap_int32)
MACRO_MAKE_SWITCH_BE_FUNC(SwitchArrayBE_uint32, uint32, ByteSwap_uint32)
MACRO_MAKE_SWITCH_BE_FUNC(SwitchArrayBE_fp32, fp32, ByteSwap_fp32)
MACRO_MAKE_SWITCH_BE_FUNC(SwitchArrayBE_fp64, fp64, ByteSwap_fp64)


// -------------------------------------------------------------------
//  Math acceleration funtions.
// -------------------------------------------------------------------
//static M_INLINE fp32 M_Sqrt(fp32 _Val);
//static M_INLINE fp32 M_InvSqrt(fp32 _Val);
/*
#define CONST_INVERSEMAX 1024

const int g_cSqrtTabShiftSize = 8;

class MCCDLLEXPORT CMathAccel : public CReferenceCount
{
public:
	int* m_pIntInverseTab;
	fp32* m_pFp32InverseTab;
	short m_sqrttab[0x100];

	CMathAccel();
	void InitInverseTable_fp32();
	void InitInverseTable_Fixed16();
	void InitSqrtTable();

#if defined(PLATFORM_DOLPHIN) || defined(PLATFORM_PS3)
	static inline fp32 fsqrt(fp32 _Val)    { return M_Sqrt(_Val); }
	static inline fp32 finvsqrt(fp32 _Val) { return M_InvSqrt(_Val); }
#else
	       fp32 fsqrt(fp32 _Val) const;
	inline fp32 finvsqrt(fp32 _Val) const { return 1.0f / fsqrt(_Val); }
#endif

	int FastDivInt(int taljare, int namnare);
	int FastMulDiv64(int factor1, int factor2, int namnare);
	int FastDivFp32Int(fp32 taljare, int namnare);

	~CMathAccel()
	{
		if (m_pIntInverseTab) { delete[] m_pIntInverseTab; m_pIntInverseTab = NULL; }
		if (m_pFp32InverseTab) { delete[] m_pFp32InverseTab; m_pFp32InverseTab = NULL; }
	};
};

extern CMathAccel* GetMathAccel();
*/

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CCRC32Checker
|__________________________________________________________________________________________________
\*************************************************************************************************/
class MCCDLLEXPORT CMDA_CRC32Checker
{
public:
	uint32 m_CurrentCRC;

	CMDA_CRC32Checker();
	~CMDA_CRC32Checker();
	
	void ResetCRC();
	void AddBlock(const void *block, int size);
	void AddStr(const char *block);
	void AddFile(class CCFile* _pFile);
};

class MCCDLLEXPORT CMDA_CRC32CheckerTable
{
public:
	uint32 m_Entry[256];

	CMDA_CRC32CheckerTable();
	void CalcTables();
};

extern MCCDLLEXPORT CMDA_CRC32CheckerTable g_CRCTable;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| RadixSort
|__________________________________________________________________________________________________
\*************************************************************************************************/
void InternalRadixSort(void *_pBuf, void *_pTempBuf, int _nNumItems, 
                       int _nSizeofItem, int _nSortkeyOffset, int _nSortkeySize);

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		RadixSort<T>.

	Comments:		Sorts an array..

	Arguments:		_pBuf     - Input data
					_nItems   - Number of elements in _pBuf
					_pTempBuf - Temporary buffer to use during sorting.
					            Pass NULL if you want the function to
					            allocate one for you..

	Notes:			The class T must have a member called m_SortKey.

	Author:			Anton Ragnarsson
\*____________________________________________________________________*/
template<class T>
M_INLINE void RadixSort(T* _pBuf, int _nItems, T* _pTempBuf = NULL)
{
	mint SizeOf = sizeof(_pBuf->m_SortKey);
	M_OFFSET(T, m_SortKey, Offset);
//	enum { nSortkeySize   = M_SIZEOF(T, m_SortKey) };
//	enum { nSortkeyOffset = M_OFFSETOF(T, m_SortKey)  };

	InternalRadixSort(_pBuf, _pTempBuf, _nItems, 
	                  sizeof(T), Offset, SizeOf);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| foreach
|
| Macro for easy iterating of an TArray/TThinArray.
| Example usage:
|
| foreach (CVec3Dfp32, v, m_lVertices)
|     v.k[2] += 1.0f;
|
| The generated code (with optimizations enabled) is equal to doing the iteration manually
| (i.e. don't be afraid of the nasty-looking macro code)
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define foreach(ElemType, ElemName, ArrayName) \
	if (0){} else \
	for (uint __n = (ArrayName).Len(), __x=1; __x; __x=0) \
	for (ElemType* __pElem = (ArrayName).GetBasePtr(); __n > 0; --__n, ++__pElem) \
	if (bool __b = true) \
	for (ElemType& ElemName = *__pElem; __b; __b=false)
		


#endif // _INC_MMISC

