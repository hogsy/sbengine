#ifndef _INC_MMATH_FP2
#define _INC_MMATH_FP2


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Half-float class and half-float vector class

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios AB 2004

	Contents:

	Comments:
					Might have unresolved endian issues.

	History:	
		040427:		Stand alone fp2/CVec4Dfp2 code written

		041125:		Added classes to MCC

\*____________________________________________________________________________________________*/

#include "MRTC.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| fp2 - "Half float"
|__________________________________________________________________________________________________
\*************************************************************************************************/

struct CConvfp2
{
	union
	{
		uint16 m_Words[2];
		fp32 m_Float;
	};

	void operator=(fp32 _x)
	{
		m_Float = _x;
	}

	uint16 Getfp2() const
	{
#ifdef CPU_BIGENDIAN
		return m_Words[0];
#else
		return m_Words[1];
#endif
	}
};


#define FP2_ONE 16256
#define FP2_HALF 16128
#define FP2_ZERO 0

class MCCDLLEXPORT fp2aggr
{
public:
	uint16 m_Half;

	void Set(fp2aggr _x)
	{
		m_Half = _x.m_Half;
	}

	void Set(fp32 _x)	// LHS stall guaranteed, not for real-time use
	{
		CConvfp2 Conv;
		Conv.m_Float = _x;
#ifdef CPU_BIGENDIAN
		m_Half = Conv.m_Words[0];
#else
		m_Half = Conv.m_Words[1];
#endif

	}

	fp32 Getfp32() const	// LHS stall guaranteed, not for real-time use
	{
		CConvfp2 Conv;
#ifdef CPU_BIGENDIAN
		Conv.m_Words[0] = m_Half;
		Conv.m_Words[1] = 0;
#else
		Conv.m_Words[0] = 0;
		Conv.m_Words[1] = m_Half;
#endif
		return Conv.m_Float;
	}

/*	void operator=(fp32 _x)
	{
		CConvfp2 Conv;
		Conv.m_Float = _x;
#ifdef CPU_BIGENDIAN
		m_Half = Conv.m_Words[0];
#else
		m_Half = Conv.m_Words[1];
#endif
	}

	void operator=(const fp2& _x)
	{
		m_Half = _x.m_Half;
	}

	fp32 operator* (const fp2& _x)
	{
		return fp32(*this) * fp32(_x);
	}

	fp32 operator/ (const fp2& _x)
	{
		return fp32(*this) / fp32(_x);
	}

	fp32 operator+ (const fp2& _x)
	{
		return fp32(*this) + fp32(_x);
	}

	fp32 operator- (const fp2& _x)
	{
		return fp32(*this) - fp32(_x);
	}
*/

	bool IsZero() const
	{
		return m_Half == 0;
	}
	bool IsOne() const
	{
		return m_Half == FP2_ONE;
	}
	void Read(CCFile* _pF)
	{
		_pF->ReadLE(m_Half);
	}

	void Write(CCFile* _pF) const
	{
		_pF->WriteLE(m_Half);
	}

	void SwapLE()
	{
		::SwapLE(m_Half);
	}
};


class MCCDLLEXPORT fp2 : public fp2aggr
{
public:
	fp2()
	{
	}

	fp2(const fp2& _x)
	{
		m_Half = _x.m_Half;
	}

	fp2(fp32 _x)	// LHS stall guaranteed, not for real-time use
	{
		CConvfp2 Conv;
		Conv.m_Float = _x;
#ifdef CPU_BIGENDIAN
		m_Half = Conv.m_Words[0];
#else
		m_Half = Conv.m_Words[1];
#endif

	}
};

typedef fp2 fp16;
typedef TVector3<fp16> CVec3Dfp16;

template<>
M_INLINE void TVector3Aggr<fp16>::Read(CCFile* _pFile)
{
	k[0].Read(_pFile);
	k[1].Read(_pFile);
	k[2].Read(_pFile);
}

template<> 
M_INLINE void TVector3Aggr<fp16>::Write(CCFile* _pFile) const
{
	k[0].Write(_pFile);
	k[1].Write(_pFile);
	k[2].Write(_pFile);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMClosestAssign
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CMClosestAssign
{
public:
	static fp2 Assign(fp2 _Type, fp64 _Value)
	{
		fp2 Val0; Val0.Set(fp32(_Value));
		fp2 Val1; Val1.Set(fp32(_Value));
		fp2 Val2; Val2.Set(fp32(_Value));
		Val1.m_Half++;
		if (Val2.m_Half != 0)
			Val2.m_Half--;
		fp2 Ret = Val0;
		fp64 Best = M_Fabs(_Value - (fp64)Val0.Getfp32());
		fp64 Test = M_Fabs(_Value - (fp64)Val1.Getfp32());
		if (Test < Best)
		{
			Ret = Val1;
			Best = Test;
		}
		Test = M_Fabs(_Value - (fp64)Val2.Getfp32());
		if (Test < Best)
			Ret = Val2;
		return Ret;
	}
	static fp32 Assign(fp32 _Type, fp64 _Value)
	{
		fp32 Val0 = _Value;
		fp32 Val1 = _Value;
		fp32 Val2 = _Value;
		((uint32 &)Val1)++;
		if (((uint32 &)Val2) != 0)
			((uint32 &)Val2)--;
		fp32 Ret = Val0;
		fp64 Best = M_Fabs(_Value - (fp64)Val0);
		fp64 Test = M_Fabs(_Value - (fp64)Val1);
		if (Test < Best)
		{
			Ret = Val1;
			Best = Test;
		}
		Test = M_Fabs(_Value - (fp64)Val2);
		if (Test < Best)
			Ret = Val2;
		return Ret;
	}
	static fp64 Assign(fp64 _Type, fp64 _Value)
	{
		return _Value;
	}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CVec4Dfp2
|__________________________________________________________________________________________________
\*************************************************************************************************/
class M_ALIGN(8) MCCDLLEXPORT CVec4Dfp2Aggr
{
public:
	typedef fp2aggr CDataType;

	fp2aggr k[4];	// R,G,B,A order

/*	void operator=( fp32 _SetTo)
	{
		fp2 tmp = _SetTo;
		k[0] = tmp;
		k[1] = tmp;
		k[2] = tmp;
		k[3] = tmp;
	}*/

	void SetZero()
	{
		k[0].m_Half = FP2_ZERO;
		k[1].m_Half = FP2_ZERO;
		k[2].m_Half = FP2_ZERO;
		k[3].m_Half = FP2_ZERO;
	}

	void SetOne()
	{
		k[0].m_Half = FP2_ONE;
		k[1].m_Half = FP2_ONE;
		k[2].m_Half = FP2_ONE;
		k[3].m_Half = FP2_ONE;
	}

	void SetScalar(fp32 _x)		// 1 LHS
	{
		fp2 x; x.Set(_x);
		k[0].m_Half = x.m_Half;
		k[1].m_Half = x.m_Half;
		k[2].m_Half = x.m_Half;
		k[3].m_Half = x.m_Half;
	}

	bool IsOne() const
	{
		return ((k[0].m_Half ^ FP2_ONE) | (k[1].m_Half ^ FP2_ONE) | (k[2].m_Half ^ FP2_ONE) | (k[3].m_Half ^ FP2_ONE)) == 0;
	}

	int IsNotOne() const
	{
		return (k[0].m_Half ^ FP2_ONE) | (k[1].m_Half ^ FP2_ONE) | (k[2].m_Half ^ FP2_ONE) | (k[3].m_Half ^ FP2_ONE);
	}

	bool IsZero() const
	{
		return (k[0].m_Half | k[1].m_Half | k[2].m_Half | k[3].m_Half) == 0;
	}

	int IsNotZero() const
	{
		return k[0].m_Half | k[1].m_Half | k[2].m_Half | k[3].m_Half;
	}

	friend M_FORCEINLINE void operator<< (CVec4Dfp32& _Dst, const CVec4Dfp2Aggr& _Src);
	friend M_FORCEINLINE void operator<< (CVec4Dfp2Aggr& _Dst, const CVec4Dfp32& _Src);
/*

	static void Add(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, CVec4Dfp2& _Dest);
	static void Scale(const CVec4Dfp2& _Oper1, fp32 _Factor, CVec4Dfp2& _Dest);
	static void Multiply(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, CVec4Dfp2& _Dest);
	static void MultiplyAdd(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, const CVec4Dfp2& _Oper3, CVec4Dfp2& _Dest);
	static void Lerp(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, CVec4Dfp2& _Dest, fp32 _t);

	void Add(const CVec4Dfp2& _Oper, CVec4Dfp2& _Dest) const
	{
		Add(*this, _Oper, _Dest);
	}

	void Scale(fp32 _Factor, CVec4Dfp2& _Dest) const
	{
		Scale(*this, _Factor, _Dest);
	}

	void Multiply(const CVec4Dfp2& _Oper, CVec4Dfp2& _Dest) const
	{
		Multiply(*this, _Oper, _Dest);
	}

	void MultiplyAdd(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, CVec4Dfp2& _Dest) const
	{
		MultiplyAdd(*this, _Oper1, _Oper2, _Dest);
	}

	void Lerp(const CVec4Dfp2& _Oper, fp32 _t, CVec4Dfp2& _Dest) const
	{
		Lerp(*this, _Oper, _Dest, _t);
	}

	uint32 GetPixel32() const
	{
		uint32 Pixel32 = 
			(RoundToInt(ClampRange(k[0]*255.0f, 255)) << 16) +
			(RoundToInt(ClampRange(k[1]*255.0f, 255)) << 8) +
			(RoundToInt(ClampRange(k[2]*255.0f, 255)) << 0) +
			(RoundToInt(ClampRange(k[3]*255.0f, 255)) << 24);
		return Pixel32;
	}
*/	
	CStr GetString() const
	{
		return CStrF("(%f, %f, %f, %f)", k[0].Getfp32(), k[1].Getfp32(), k[2].Getfp32(), k[3].Getfp32());
	}

	CFStr GetFilteredString(int _iType = 0) const
	{
		switch(_iType)
		{
		case 0: return CFStrF("%s,%s,%s,%s", (const char *)CFStr::GetFilteredString(k[0].Getfp32()), (const char *)CFStr::GetFilteredString(k[1].Getfp32()), (const char *)CFStr::GetFilteredString(k[2].Getfp32()), (const char *)CFStr::GetFilteredString(k[3].Getfp32()));
		case 1: return CFStrF("%s %s %s %s", (const char *)CFStr::GetFilteredString(k[0].Getfp32()), (const char *)CFStr::GetFilteredString(k[1].Getfp32()), (const char *)CFStr::GetFilteredString(k[2].Getfp32()), (const char *)CFStr::GetFilteredString(k[3].Getfp32()));
		}
		return CFStr();
	};

	void ParseString(const CStr& _s)
	{
		CVec4Dfp32 v;
		v.ParseString(_s);
		*this << v;
	}

	void ParseColor(const CStr& _s, bool _bHexUnit = true);


	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile) const;
	void SwapLE();
};

class M_ALIGN(8) MCCDLLEXPORT CVec4Dfp2 : public CVec4Dfp2Aggr
{
public:
	CVec4Dfp2()
	{
	}

	CVec4Dfp2(fp32 _SetTo)
	{
		fp2 tmp; tmp.Set(_SetTo);
		k[0] = tmp;
		k[1] = tmp;
		k[2] = tmp;
		k[3] = tmp;
	}

	CVec4Dfp2(fp32 _x, fp32 _y, fp32 _z, fp32 _w)		// 1 LHS stall
	{
		CConvfp2 c[4];
		c[0] = _x; c[1] = _y; c[2] = _z; c[3] = _w;
		k[0].m_Half = c[0].Getfp2();
		k[1].m_Half = c[1].Getfp2();
		k[2].m_Half = c[2].Getfp2();
		k[3].m_Half = c[3].Getfp2();
	}

	CVec4Dfp2(const CVec4Dfp2& _Copy)
	{
		k[0] = _Copy.k[0];
		k[1] = _Copy.k[1];
		k[2] = _Copy.k[2];
		k[3] = _Copy.k[3];
	}

	void operator=(const CVec4Dfp2& _Copy)
	{
		k[0] = _Copy.k[0];
		k[1] = _Copy.k[1];
		k[2] = _Copy.k[2];
		k[3] = _Copy.k[3];
	}

};


M_FORCEINLINE void operator<< (CVec4Dfp32& _Dst, const CVec4Dfp2Aggr& _Src)
{
	_Dst.v = M_VLd_V4f16_f32(&_Src);
}

M_FORCEINLINE void operator<< (CVec4Dfp2Aggr& _Dst, const CVec4Dfp32& _Src)
{
	M_VSt_V4f32_f16(_Src.v, &_Dst);
}

#endif // _INC_MMATH_FP2

