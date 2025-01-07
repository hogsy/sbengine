#ifndef _INC_MMATH_GFP2
#define _INC_MMATH_GFP2


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Graphics hardware compatible Half-float class and half-float vector class

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios AB 2004

	Contents:

	Comments:
					16-bit float structure: SEEEEEMMMMMMMMMM
					(Sign, Exponent, Mantissa)

					Might have unresolved endian issues.

	History:	
		2004-12-04:	Created gfp2/CVec4Dgfp2 based on fp2

\*____________________________________________________________________________________________*/

#include "MRTC.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| gfp2 - graphics "Half float"
|__________________________________________________________________________________________________
\*************************************************************************************************/

M_FORCEINLINE fp32 FloatFromGfp2(uint16 _gfp2)
{
	uint32 dw;
	uint32 E = (_gfp2 >> 10) & 0x1f;
	E += 48;
	dw = ((_gfp2 & 0x8000) << 16) + (uint32(_gfp2 & 0x03ff) << 14) + (E << 24);
	return *((fp32*)&dw);
}

M_FORCEINLINE uint16 Gfp2FromFloat(fp32 _x)
{
	uint32 dw = *(const uint32*)&_x;
	uint32 E = (dw & 0x7f000000) >> 24;
	if (E < 48)
		return 0;
	E -= 48;
	return (E << 10) + ((dw >> 16) & 0x8000) + ((dw & 0xffffff) >> 14);
}


#define GFP2_FROM_FLOAT(x) ( (((const uint8*)(&x))[3] & 0xfc) + (*(uint16*)(((const uint8*)(&x))+1)) >> 6 )
#define FLOATDW_FROM_GFP2(x) 

#define GFP2_ONE 16256
#define GFP2_HALF 16128
#define GFP2_ZERO 0

class MCCDLLEXPORT gfp2
{
public:
	uint16 m_Half;

	gfp2()
	{
	}

	gfp2(const fp2& _x)
	{
		m_Half = _x.m_Half;
	}

	gfp2(fp32 _x)
	{
		m_Half = Gfp2FromFloat(_x);
	}

	operator fp32() const
	{
		return FloatFromGfp2(m_Half);
	}

	void operator=(fp32 _x)
	{
		m_Half = Gfp2FromFloat(_x);
	}

	void operator=(const gfp2& _x)
	{
		m_Half = _x.m_Half;
	}

	fp32 operator* (const gfp2& _x)
	{
		return fp32(*this) * fp32(_x);
	}

	fp32 operator/ (const gfp2& _x)
	{
		return fp32(*this) / fp32(_x);
	}

	fp32 operator+ (const gfp2& _x)
	{
		return fp32(*this) + fp32(_x);
	}

	fp32 operator- (const gfp2& _x)
	{
		return fp32(*this) - fp32(_x);
	}
};

#if 0

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CVec4Dgfp2
|__________________________________________________________________________________________________
\*************************************************************************************************/
class MCCDLLEXPORT CVec4Dgfp2
{
public:
	gfp2 k[4];	// R,G,B,A order

	CVec4Dgfp2()
	{
	}

	CVec4Dgfp2(fp32 _r, fp32 _g, fp32 _b, fp32 _a)
	{
		k[0] = _r;
		k[1] = _g;
		k[2] = _b;
		k[3] = _a;
	}

	CVec4Dgfp2(const CVec4Dgfp2& _Copy)
	{
		k[0] = _Copy.k[0];
		k[1] = _Copy.k[1];
		k[2] = _Copy.k[2];
		k[3] = _Copy.k[3];
	}

	void operator=(const CVec4Dgfp2& _Copy)
	{
		k[0] = _Copy.k[0];
		k[1] = _Copy.k[1];
		k[2] = _Copy.k[2];
		k[3] = _Copy.k[3];
	}

	void SetZero()
	{
		k[0].m_Half = GFP2_ZERO;
		k[1].m_Half = GFP2_ZERO;
		k[2].m_Half = GFP2_ZERO;
		k[3].m_Half = GFP2_ZERO;
	}

	void SetOne()
	{
		k[0].m_Half = GFP2_ONE;
		k[1].m_Half = GFP2_ONE;
		k[2].m_Half = GFP2_ONE;
		k[3].m_Half = GFP2_ONE;
	}

	void SetScalar(fp32 _x)
	{
		gfp2 x = _x;
		k[0].m_Half = x.m_Half;
		k[1].m_Half = x.m_Half;
		k[2].m_Half = x.m_Half;
		k[3].m_Half = x.m_Half;
	}

	bool IsOne() const
	{
		return ((k[0].m_Half ^ GFP2_ONE) | (k[1].m_Half ^ GFP2_ONE) | (k[2].m_Half ^ GFP2_ONE) | (k[3].m_Half ^ GFP2_ONE)) == 0;
	}

	int IsNotOne() const
	{
		return (k[0].m_Half ^ GFP2_ONE) | (k[1].m_Half ^ GFP2_ONE) | (k[2].m_Half ^ GFP2_ONE) | (k[3].m_Half ^ GFP2_ONE);
	}

	bool IsZero() const
	{
		return (k[0].m_Half | k[1].m_Half | k[2].m_Half | k[3].m_Half) == 0;
	}

	int IsNotZero() const
	{
		return k[0].m_Half | k[1].m_Half | k[2].m_Half | k[3].m_Half;
	}

	friend void MCCDLLEXPORT operator<< (CVec4Dfp32& _Dst, const CVec4Dgfp2& _Src);
	friend void MCCDLLEXPORT operator<< (CVec4Dgfp2& _Dst, const CVec4Dfp32& _Src);


	static void Add(const CVec4Dgfp2& _Oper1, const CVec4Dgfp2& _Oper2, CVec4Dgfp2& _Dest);
	static void Scale(const CVec4Dgfp2& _Oper1, fp32 _Factor, CVec4Dgfp2& _Dest);
	static void Multiply(const CVec4Dgfp2& _Oper1, const CVec4Dgfp2& _Oper2, CVec4Dgfp2& _Dest);
	static void MultiplyAdd(const CVec4Dgfp2& _Oper1, const CVec4Dgfp2& _Oper2, const CVec4Dgfp2& _Oper3, CVec4Dgfp2& _Dest);
	static void Lerp(const CVec4Dgfp2& _Oper1, const CVec4Dgfp2& _Oper2, CVec4Dgfp2& _Dest, fp32 _t);

	void Add(const CVec4Dgfp2& _Oper, CVec4Dgfp2& _Dest) const
	{
		Add(*this, _Oper, _Dest);
	}

	void Scale(fp32 _Factor, CVec4Dgfp2& _Dest) const
	{
		Scale(*this, _Factor, _Dest);
	}

	void Multiply(const CVec4Dgfp2& _Oper, CVec4Dgfp2& _Dest) const
	{
		Multiply(*this, _Oper, _Dest);
	}

	void MultiplyAdd(const CVec4Dgfp2& _Oper1, const CVec4Dgfp2& _Oper2, CVec4Dgfp2& _Dest) const
	{
		MultiplyAdd(*this, _Oper1, _Oper2, _Dest);
	}

	void Lerp(const CVec4Dgfp2& _Oper, fp32 _t, CVec4Dgfp2& _Dest) const
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
		return CStrF("(%f, %f, %f, %f)", (fp32)k[0], (fp32)k[1], (fp32)k[2], (fp32)k[3]);
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
		CVec4Dfp32 v;
		v.ParseString(_s);
		*this << v;
	}

	void ParseColor(const CStr& _s, bool _bHexUnit = true);


	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile) const;
};

void MCCDLLEXPORT operator<< (CVec4Dfp32& _Dst, const CVec4Dgfp2& _Src);
void MCCDLLEXPORT operator<< (CVec4Dgfp2& _Dst, const CVec4Dfp32& _Src);
#endif 

#endif // _INC_MMATH_GFP2

