
#ifndef _INC_VPU_PLATFORM
#define _INC_VPU_PLATFORM


#define PLATFORM_VPU

#if defined(TARGET_PS3_SPU) || defined(SN_TARGET_PS3_SPU)
	#include "VPUTarget_SPU.h"
#else
	#include "../../Platform/Platform.h"
#endif

#include <math.h>

#define _PI					3.14159265358979323846264338327950288419716939937510582097494459f
#define _PI2				6.283185307179586476925286766559f
#define _PIHALF				1.5707963267948966192313216916398f
#define _NLOG				2.7182818284590f
#define _SQRT2				1.4142135623731f
#define _SQRT3				1.7320508075689f
#define _SIN30				0.5f
#define _SIN45				0.7071067812f
#define _SIN60				0.8660254038f

static M_INLINE fp32 M_Fabs(fp32 _Val)						{ return fabs(_Val); }
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
static M_INLINE fp32 M_Log10(fp32 _Val)							{ return log10f(_Val); }
static M_INLINE fp32 M_Log(fp32 _Val)								{ return logf(_Val); }
static M_INLINE fp32 M_Exp(fp32 _Val)								{ return expf(_Val); }
static M_INLINE fp32 M_Pow(fp32 _V,fp32 _V2)						{ return powf(_V,_V2); }
static M_INLINE fp32 M_Sqrt(fp32 _V)						{ return sqrtf(_V); }
static M_INLINE fp32 M_InvSqrt(fp32 _Val)							{ return 1.0/sqrtf(_Val); }

static M_INLINE fp64 M_Fabs(fp64 _Val)							{ return fabs(_Val); }
static M_INLINE fp64 M_Sqrt(fp64 _Val)						{ return sqrt(_Val); }
static M_INLINE fp64 M_InvSqrt(fp64 _Val)							{ return 1.0/sqrt(_Val); }
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

template<class T>
class TMathTemplateProperties
{
public:
	typedef struct { T k[4]; } TMatrix4RowIntrinsic;
	typedef struct { T k[4]; } TVector4Intrinsic;
};


template<>
class TMathTemplateProperties<fp32>
{
public:
	typedef vec128 TMatrix4RowIntrinsic;
	typedef vec128 TVector4Intrinsic;
};

template<>
class TMathTemplateProperties<int32>
{
public:
	typedef vec128 TMatrix4RowIntrinsic;
	typedef vec128 TVector4Intrinsic;
};

template<>
class TMathTemplateProperties<uint32>
{
public:
	typedef vec128 TMatrix4RowIntrinsic;
	typedef vec128 TVector4Intrinsic;
};



class CVec4Dfp32Aggr
{
public:
	union {
		vec128 v;
		fp32 k[4]; 
	};
};

class CVec4Dfp32 : public CVec4Dfp32Aggr
{ 
public: 
	CVec4Dfp32() {}
	CVec4Dfp32(vec128 _v) { v=_v; }
	CVec4Dfp32(fp32 _x,fp32 _y,fp32 _z,fp32 _w) { k[0]=_x; k[1]=_y; k[2]=_z; k[3]=_w; }
};
/*
template<class T>
class TVector3Aggr
{ 
public: 
	T k[3]; 
};
typedef TVector3Aggr<fp32> CVec3Dfp32Aggr;

class CVec3Dfp32 : public TVector3Aggr<fp32>
{ 
public:
	typedef TVector3Aggr<fp32> VBase;
	CVec3Dfp32(fp32 _x,fp32 _y,fp32 _z) { VBase::k[0]=_x; VBase::k[1]=_y; VBase::k[2]=_x; }
};
*/



template<class T>
class TMatrix43
{ 
public: 
	typedef typename TMathTemplateProperties<T>::TVector4Intrinsic TIntrinsic;

	union
	{
		TIntrinsic v[3];
		T k[4][3]; 
	};
};

/*
class CMat4Dfp32
{ 
public: 
	union
	{
		vec128 v[3];
		fp32 k[4][3]; 
	};
};*/

class CMat4Dfp32
{
public:
	typedef fp32 T;

	union
	{
		vec128 r[4];
		fp32 k[4][4];  // row, column
	};

	void InverseOrthogonal(CMat4Dfp32 &DestMat) const
	{
		T k00 = k[0][0];
		T k11 = k[1][1];
		T k22 = k[2][2];
		T k10 = k[1][0];
		T k20 = k[2][0];
		T k21 = k[2][1];
		T k01 = k[0][1];
		T k02 = k[0][2];
		T k12 = k[1][2];
		T k30 = k[3][0];
		T k31 = k[3][1];
		T k32 = k[3][2];

		DestMat.k[0][0] = k00;
		DestMat.k[1][1] = k11;
		DestMat.k[2][2] = k22;

		DestMat.k[0][1] = k10;
		DestMat.k[0][2] = k20;
		DestMat.k[1][2] = k21;

		DestMat.k[1][0] = k01;
		DestMat.k[2][0] = k02;
		DestMat.k[2][1] = k12;
		DestMat.k[3][0] = -(k30 * k00 + k31 * k01 + k32 * k02);
		DestMat.k[3][1] = -(k30 * k10 + k31 * k11 + k32 * k12);
		DestMat.k[3][2] = -(k30 * k20 + k31 * k21 + k32 * k22);

		DestMat.k[3][3] = 1;
		DestMat.k[0][3] = 0;
		DestMat.k[1][3] = 0;
		DestMat.k[2][3] = 0;
	};
};

//typedef TMatrix43<fp32> CMat43fp32;


/*
class CStr
{

};

CStr CStrF(const char*, ...);
*/


template<class T> class TVector3;

template<typename T>
class TVector3Aggr
{
public:
	typedef CMat4Dfp32 M;
	//typedef CMat4Dfp32 M43;
	typedef TVector3Aggr<T> V;
	typedef TVector3<T> V3;

	T k[3];
	//TVector3Aggr() { }
	//TVector3Aggr(T _x, T _y, T _z) { k[0]=_x; k[1]=_y; k[2]=_z; }

	T &operator[](int i) { return k[i]; }
	const T &operator[](int i) const { return k[i]; }
	void operator = (T v) { k[0] = v; k[1] = v; k[2] = v; }
	void operator *= (T v) { k[0] *= v; k[1] *= v; k[2] *= v; }
	void operator += (T v) { k[0] += v; k[1] += v; k[2] += v; }
	void operator /= (T v) { k[0] /= v; k[1] /= v; k[2] /= v; }
	
	V operator / (const T v) const { return V3(k[0] / v, k[1] / v, k[2] / v); }
	V operator * (const T v) const { return V3(k[0] * v, k[1] * v, k[2] * v); }
	V operator + (const T v) const { return V3(k[0] + v, k[1] + v, k[2] + v); }
	V operator - (const T v) const { return V3(k[0] - v, k[1] - v, k[2] - v); }

	T operator * (const V v) const { return (k[0]*v.k[0] + k[1]*v.k[1] + k[2]*v.k[2]); }

	V operator + (const V v) const { return V3(k[0] + v.k[0], k[1] + v.k[1], k[2] + v.k[2]); }
	V operator - (const V v) const { return V3(k[0] - v.k[0], k[1] - v.k[1], k[2] - v.k[2]); }

	void operator -= (V v) { k[0] -= v.k[0]; k[1] -= v.k[1]; k[2] -= v.k[2]; }
	void operator += (V v) { k[0] += v.k[0]; k[1] += v.k[1]; k[2] += v.k[2]; }

	bool operator ==(const V v) { return k[0]==v.k[0] && k[1]==v.k[1] && k[2]==v.k[2]; }

	T DistanceSqr() const { return k[0]*k[0] + k[1]*k[1] + k[2]*k[2]; }
	T DistanceSqr(const V v) const { return (*this-v).DistanceSqr(); }

	void operator*= (const M& m)
	{
		V r;
		for (int i=0; i<3; i++)
			r.k[i] = m.k[0][i]*k[0] + m.k[1][i]*k[1] + m.k[2][i]*k[2] + m.k[3][i];
		k[0] = r.k[0];
		k[1] = r.k[1];
		k[2] = r.k[2];
	};

	/*
	void operator*= (const M43& m)
	{
		V r;
		for (int i=0; i<3; i++)
			r.k[i] = m.k[0][i]*k[0] + m.k[1][i]*k[1] + m.k[2][i]*k[2] + m.k[3][i];
		k[0] = r.k[0];
		k[1] = r.k[1];
		k[2] = r.k[2];
	}*/

	V operator- () const
	{
		return V3(-k[0],-k[1],-k[2]);
	};


	void MultiplyMatrix(const M& m)
	{
		fp32 x = k[0];
		fp32 y = k[1];
		fp32 z = k[2];
		for (int i = 0; i < 3; i++)
			k[i] = m.k[0][i]*x + m.k[1][i]*y + m.k[2][i]*z + m.k[3][i];
	}

	void MultiplyMatrix3x3(const M& m)
	{
		fp32 x = k[0];
		fp32 y = k[1];
		fp32 z = k[2];
		for (int i = 0; i < 3; i++)
			k[i] = m.k[0][i]*x + m.k[1][i]*y + m.k[2][i]*z;
	}

	void Add(const V& a, V& dest) const
	{
		dest.k[0] = k[0] + a.k[0];
		dest.k[1] = k[1] + a.k[1];
		dest.k[2] = k[2] + a.k[2];
	}

	void Sub(const V& a, V& dest) const
	{
		dest.k[0] = k[0] - a.k[0];
		dest.k[1] = k[1] - a.k[1];
		dest.k[2] = k[2] - a.k[2];
	}

	void Combine(const V& a, T t, V& dest) const
	{
		dest.k[0] = k[0] + t*a.k[0];
		dest.k[1] = k[1] + t*a.k[1];
		dest.k[2] = k[2] + t*a.k[2];
	}

	void Lerp(const V& _Other, T t, V& dest) const
	{
		T invt = 1.0f - t;

		T k0 = k[0];
		T k1 = k[1];
		T k2 = k[2];
		T o0 = _Other.k[0];
		T o1 = _Other.k[1];
		T o2 = _Other.k[2];

		dest.k[0] = k0 * invt + t * o0;
		dest.k[1] = k1 * invt + t * o1;
		dest.k[2] = k2 * invt + t * o2;
	}
};

typedef TVector3Aggr<fp32> CVec3Dfp32Aggr;

template<typename T>
class TVector3 : public TVector3Aggr<T>
{
public:

	typedef TVector3Aggr<T> VBase;

	//T k[3];
	TVector3()
	{
	}

	TVector3(T val) 
	{
		VBase::k[0] = val;
		VBase::k[1] = val;
		VBase::k[2] = val;
	}

	TVector3(T x, T y, T z)
	{
		VBase::k[0] = x;
		VBase::k[1] = y;
		VBase::k[2] = z;
	}

	TVector3(const VBase& _v)
	{
		T x = _v.k[0];
		T y = _v.k[1];
		T z = _v.k[2];
		VBase::k[0] = x;
		VBase::k[1] = y;
		VBase::k[2] = z;
	};
};

typedef TVector3<fp32> CVec3Dfp32;
typedef TVector3<int16> CVec3Dint16;
typedef TVector3<int32> CVec3Dint32;




class CPlane3Dfp32 
{
public:
	union
	{
		vec128 v;
		struct
		{
			CVec3Dfp32Aggr n;	// Normal
			fp32 d;			// Distance
		};
	};
};

template<class T>
class TCapsule
{
public:
	typedef typename TMathTemplateProperties<T>::TVector4Intrinsic TIntrinsic;

	union
	{
		struct  
		{
			TVector3Aggr<T> m_Point1;
			T m_w1;
		};
		TIntrinsic m_Pointv1;
	};
	union
	{
		struct  
		{
			TVector3Aggr<T> m_Point2;
			T m_w2;
		};
		TIntrinsic m_Pointv2;
	};
	union 
	{	
		struct  
		{
			T m_Radius;
			T m_InvSqrDistance; // 1 / Sqr(m_Point2 - m_Point1)
			int32 m_UserValue;
			uint32 m_Flags;
		};
		TIntrinsic m_Misc;
	};

	TCapsule() {}
	TCapsule(const TCapsule<T>& _capsule)
	{
		m_Pointv1=_capsule.m_Pointv1;
		m_Pointv2=_capsule.m_Pointv2;
		m_Misc=_capsule.m_Misc;
	}
};



template<class T>
class TPtr
{
	T *m_pPtr;
public:
	TPtr() : m_pPtr(0x0) {}
	TPtr(T *ptr) : m_pPtr(ptr) {}

	T *operator ->() const { return m_pPtr; }
	operator T*() const { return m_pPtr; }
};


// stub class for box
template<typename TCONTAINER, typename T>
class TBox
{
public:
	typedef TVector3<T> V;
	TCONTAINER m_Min;
	TCONTAINER m_Max;

	void Transform(const CMat4Dfp32& _Mat, TBox& _Dest) const
	{
		// Transforms and expands box so that it still is a valid bounding box for the original space.
		TCONTAINER E;
		TCONTAINER C;
		m_Max.Lerp(m_Min, T(0.5), C);
		m_Max.Sub(C, E);

		for(int k = 0; k < 3; k++)
			_Dest.m_Max.k[k] = M_Fabs(_Mat.k[0][k]*E[0]) + M_Fabs(_Mat.k[1][k]*E[1]) + M_Fabs(_Mat.k[2][k]*E[2]);

		_Dest.m_Min = -_Dest.m_Max;
		C *= _Mat;
		_Dest.m_Min += C;
		_Dest.m_Max += C;
	}

	void Transform(const CMat4Dfp32& _Mat, TBox& _Dest)
	{
		// Transforms and expands box so that it still is a valid bounding box for the original space.
		V E;
		V C;
		m_Max.Lerp(m_Min, T(0.5), C);
		m_Max.Sub(C, E);

		for(int k = 0; k < 3; k++)
			_Dest.m_Max.k[k] = M_Fabs(_Mat.k[0][k]*E[0]) + M_Fabs(_Mat.k[1][k]*E[1]) + M_Fabs(_Mat.k[2][k]*E[2]);

		_Dest.m_Min = -_Dest.m_Max;
		C *= _Mat;
		_Dest.m_Min += C;
		_Dest.m_Max += C;
	}

	bool IntersectLine(const V& _p0, const V& _p1, V& _RetHitPos) const
	{
		V p0(_p0);
		V p1(_p1);

		if (p0.k[0] < p1.k[0])
		{
			if (p0.k[0] > m_Max.k[0]) return false;
			if (p1.k[0] < m_Min.k[0]) return false;

			if (p0.k[0] < m_Min.k[0])
			{
				T t = (m_Min.k[0] - p0.k[0]) / (p1.k[0] - p0.k[0]);
				p0.k[0] = m_Min.k[0];
				p0.k[1] += t*(p1.k[1] - p0.k[1]);
				p0.k[2] += t*(p1.k[2] - p0.k[2]);
			}
			if (p1.k[0] > m_Max.k[0])
			{
				T t = -(p1.k[0] - m_Max.k[0]) / (p1.k[0] - p0.k[0]);
				p1.k[0] = m_Max.k[0];
				p1.k[1] += t*(p1.k[1] - p0.k[1]);
				p1.k[2] += t*(p1.k[2] - p0.k[2]);
			}
		}
		else
		{
			if (p1.k[0] > m_Max.k[0]) return false;
			if (p0.k[0] < m_Min.k[0]) return false;

			if (p1.k[0] < m_Min.k[0])
			{
				T t = (m_Min.k[0] - p1.k[0]) / (p0.k[0] - p1.k[0]);
				p1.k[0] = m_Min.k[0];
				p1.k[1] += t*(p0.k[1] - p1.k[1]);
				p1.k[2] += t*(p0.k[2] - p1.k[2]);
			}
			if (p0.k[0] > m_Max.k[0])
			{
				T t = -(p0.k[0] - m_Max.k[0]) / (p0.k[0] - p1.k[0]);
				p0.k[0] = m_Max.k[0];
				p0.k[1] += t*(p0.k[1] - p1.k[1]);
				p0.k[2] += t*(p0.k[2] - p1.k[2]);
			}
		}

		if (p0.k[1] < p1.k[1])
		{
			if (p0.k[1] > m_Max.k[1]) return false;
			if (p1.k[1] < m_Min.k[1]) return false;

			if (p0.k[1] < m_Min.k[1])
			{
				T t = (m_Min.k[1] - p0.k[1]) / (p1.k[1] - p0.k[1]);
				p0.k[0] += t*(p1.k[0] - p0.k[0]);
				p0.k[1] = m_Min.k[1];
				p0.k[2] += t*(p1.k[2] - p0.k[2]);
			}
			if (p1.k[1] > m_Max.k[1])
			{
				T t = -(p1.k[1] - m_Max.k[1]) / (p1.k[1] - p0.k[1]);
				p1.k[0] += t*(p1.k[0] - p0.k[0]);
				p1.k[1] = m_Max.k[1];
				p1.k[2] += t*(p1.k[2] - p0.k[2]);
			}
		}
		else
		{
			if (p1.k[1] > m_Max.k[1]) return false;
			if (p0.k[1] < m_Min.k[1]) return false;

			if (p1.k[1] < m_Min.k[1])
			{
				T t = (m_Min.k[1] - p1.k[1]) / (p0.k[1] - p1.k[1]);
				p1.k[0] += t*(p0.k[0] - p1.k[0]);
				p1.k[1] = m_Min.k[1];
				p1.k[2] += t*(p0.k[2] - p1.k[2]);
			}
			if (p0.k[1] > m_Max.k[1])
			{
				T t = -(p0.k[1] - m_Max.k[1]) / (p0.k[1] - p1.k[1]);
				p0.k[0] += t*(p0.k[0] - p1.k[0]);
				p0.k[1] = m_Max.k[1];
				p0.k[2] += t*(p0.k[2] - p1.k[2]);
			}
		}

		if (p0.k[2] < p1.k[2])
		{
			if (p0.k[2] > m_Max.k[2]) return false;
			if (p1.k[2] < m_Min.k[2]) return false;

			if (p0.k[2] < m_Min.k[2])
			{
				T t = (m_Min.k[2] - p0.k[2]) / (p1.k[2] - p0.k[2]);
				p0.k[0] += t*(p1.k[0] - p0.k[0]);
				p0.k[1] += t*(p1.k[1] - p0.k[1]);
				p0.k[2] = m_Min.k[2];
			}
			if (p1.k[2] > m_Max.k[2])
			{
				T t = -(p1.k[2] - m_Max.k[2]) / (p1.k[2] - p0.k[2]);
				p1.k[0] += t*(p1.k[0] - p0.k[0]);
				p1.k[1] += t*(p1.k[1] - p0.k[1]);
				p1.k[2] = m_Max.k[2];
			}
		}
		else
		{
			if (p1.k[2] > m_Max.k[2]) return false;
			if (p0.k[2] < m_Min.k[2]) return false;

			if (p1.k[2] < m_Min.k[2])
			{
				T t = (m_Min.k[2] - p1.k[2]) / (p0.k[2] - p1.k[2]);
				p1.k[0] += t*(p0.k[0] - p1.k[0]);
				p1.k[1] += t*(p0.k[1] - p1.k[1]);
				p1.k[2] = m_Min.k[2];
			}
			if (p0.k[2] > m_Max.k[2])
			{
				T t = -(p0.k[2] - m_Max.k[2]) / (p0.k[2] - p1.k[2]);
				p0.k[0] += t*(p0.k[0] - p1.k[0]);
				p0.k[1] += t*(p0.k[1] - p1.k[1]);
				p0.k[2] = m_Max.k[2];
			}
		}

	//ConOut("After: " + p0.GetString() + p1.GetString());
		_RetHitPos = p0;

		return true;
	}
};

typedef TBox<CVec3Dfp32, fp32> CBox3Dfp32;

struct CPixel32 
{
private:
	union
	{
		uint32 p32;
		uint8 b[4]; // PC/XBox:  b,g,r,a (0xaarrggbb)
					// GameCube: r,g,b,a (0xrrggbbaa)
	};
};

#endif

