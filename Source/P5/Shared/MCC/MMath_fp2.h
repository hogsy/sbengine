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
		fp4 m_Float;
	};
};


#define FP2_ONE 16256
#define FP2_HALF 16128
#define FP2_ZERO 0

class MCCDLLEXPORT fp2
{
public:
	uint16 m_Half;

	fp2()
	{
	}

	fp2(const fp2& _x)
	{
		m_Half = _x.m_Half;
	}

	fp2(fp4 _x)
	{
		CConvfp2 Conv;
		Conv.m_Float = _x;
#ifdef CPU_BIGENDIAN
		m_Half = Conv.m_Words[0];
#else
		m_Half = Conv.m_Words[1];
#endif

	}

	operator fp4() const
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

	void operator=(fp4 _x)
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

	fp4 operator* (const fp2& _x)
	{
		return fp4(*this) * fp4(_x);
	}

	fp4 operator/ (const fp2& _x)
	{
		return fp4(*this) / fp4(_x);
	}

	fp4 operator+ (const fp2& _x)
	{
		return fp4(*this) + fp4(_x);
	}

	fp4 operator- (const fp2& _x)
	{
		return fp4(*this) - fp4(_x);
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

class CMClosestAssign
{
public:
	static fp2 Assign(fp2 _Type, fp8 _Value)
	{
		fp2 Val0 = _Value;
		fp2 Val1 = _Value;
		fp2 Val2 = _Value;
		Val1.m_Half++;
		Val2.m_Half--;
		fp2 Ret = Val0;
		fp8 Best = M_Fabs(_Value - (fp8)Val0);
		fp8 Test = M_Fabs(_Value - (fp8)Val1);
		if (Test < Best)
		{
			Ret = Val1;
			Best = Test;
		}
		Test = M_Fabs(_Value - (fp8)Val2);
		if (Test < Best)
			Ret = Val1;
		return Ret;
	}
	static fp4 Assign(fp4 _Type, fp8 _Value)
	{
		fp4 Val0 = _Value;
		fp4 Val1 = _Value;
		fp4 Val2 = _Value;
		((uint32 &)Val1)++;
		((uint32 &)Val2)--;
		fp4 Ret = Val0;
		fp8 Best = M_Fabs(_Value - (fp8)Val0);
		fp8 Test = M_Fabs(_Value - (fp8)Val1);
		if (Test < Best)
		{
			Ret = Val1;
			Best = Test;
		}
		Test = M_Fabs(_Value - (fp8)Val2);
		if (Test < Best)
			Ret = Val1;
		return Ret;
	}
	static fp8 Assign(fp8 _Type, fp8 _Value)
	{
		return _Value;
	}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CVec4Dfp2
|__________________________________________________________________________________________________
\*************************************************************************************************/
class MCCDLLEXPORT CVec4Dfp2
{
public:
	typedef fp2 CDataType;

	fp2 k[4];	// R,G,B,A order

	CVec4Dfp2()
	{
	}

	CVec4Dfp2(fp4 _SetTo)
	{
		k[0] = _SetTo;
		k[1] = _SetTo;
		k[2] = _SetTo;
		k[3] = _SetTo;
	}

	CVec4Dfp2(fp4 _r, fp4 _g, fp4 _b, fp4 _a)
	{
		k[0] = _r;
		k[1] = _g;
		k[2] = _b;
		k[3] = _a;
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

	void operator=( fp4 _SetTo)
	{
		k[0] = _SetTo;
		k[1] = _SetTo;
		k[2] = _SetTo;
		k[3] = _SetTo;
	}

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

	void SetScalar(fp4 _x)
	{
		fp2 x = _x;
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

	friend void MCCDLLEXPORT operator<< (CVec4Dfp4& _Dst, const CVec4Dfp2& _Src);
	friend void MCCDLLEXPORT operator<< (CVec4Dfp2& _Dst, const CVec4Dfp4& _Src);


	static void Add(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, CVec4Dfp2& _Dest);
	static void Scale(const CVec4Dfp2& _Oper1, fp4 _Factor, CVec4Dfp2& _Dest);
	static void Multiply(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, CVec4Dfp2& _Dest);
	static void MultiplyAdd(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, const CVec4Dfp2& _Oper3, CVec4Dfp2& _Dest);
	static void Lerp(const CVec4Dfp2& _Oper1, const CVec4Dfp2& _Oper2, CVec4Dfp2& _Dest, fp4 _t);

	void Add(const CVec4Dfp2& _Oper, CVec4Dfp2& _Dest) const
	{
		Add(*this, _Oper, _Dest);
	}

	void Scale(fp4 _Factor, CVec4Dfp2& _Dest) const
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

	void Lerp(const CVec4Dfp2& _Oper, fp4 _t, CVec4Dfp2& _Dest) const
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
	
	CStr GetString() const
	{
		return CStrF("(%f, %f, %f, %f)", (fp4)k[0], (fp4)k[1], (fp4)k[2], (fp4)k[3]);
	}

	CFStr GetFilteredString(int _iType = 0) const
	{
		switch(_iType)
		{
		case 0: return CFStrF("%s,%s,%s,%s", (const char *)CFStr::GetFilteredString(k[0]), (const char *)CFStr::GetFilteredString(k[1]), (const char *)CFStr::GetFilteredString(k[2]), (const char *)CFStr::GetFilteredString(k[3]));
		case 1: return CFStrF("%s %s %s %s", (const char *)CFStr::GetFilteredString(k[0]), (const char *)CFStr::GetFilteredString(k[1]), (const char *)CFStr::GetFilteredString(k[2]), (const char *)CFStr::GetFilteredString(k[3]));
		}
		return CFStr();
	};

	void ParseString(const CStr& _s)
	{
		CVec4Dfp4 v;
		v.ParseString(_s);
		*this << v;
	}

	void ParseColor(const CStr& _s, bool _bHexUnit = true);


	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile) const;
	void SwapLE();
};

void MCCDLLEXPORT operator<< (CVec4Dfp4& _Dst, const CVec4Dfp2& _Src);
void MCCDLLEXPORT operator<< (CVec4Dfp2& _Dst, const CVec4Dfp4& _Src);

#endif // _INC_MMATH_FP2

