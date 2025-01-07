#ifndef GJKSUPPORT_H
#define GJKSUPPORT_H


/*
template<typename T>
struct TNumericProperties
{
	static T Min() { return static_cast<T>(0); }
	static T Max() { return static_cast<T>(0); }
	static T Epsilon() { return static_cast<T>(0); }
	static T Zero() { return static_cast<T>(0); }
	static T One() { return static_cast<T>(0); }
};

template<>
struct TNumericProperties<fp32>
{
	static fp32 Min() { return _FP32_MIN; }
	static fp32 Max() { return _FP32_MAX; }
	static fp32 Epsilon() { return _FP32_EPSILON; }
	static fp32 Zero() { return 0.0f; }
	static fp32 One() { return 1.0f; }
};

template<>
struct TNumericProperties<fp64>
{
	static fp64 Min() { return _FP64_MIN; }
	static fp64 Max() { return _FP64_MAX; }
	static fp64 Epsilon() { return _FP64_EPSILON; }
	static fp64 Zero() { return 0.0; }
	static fp64 One() { return 1.0; }
};
*/

template <class T>
class TPlaneFunctions
{
public:
	static int Outside(const CVec3Dfp32& _p, const CVec3Dfp32& _a, const CVec3Dfp32& _b, const CVec3Dfp32& _c);
	static int OppositeSides(const CVec3Dfp32& _p1, const CVec3Dfp32& _p2,
							  const CVec3Dfp32& _a, const CVec3Dfp32& _b, const CVec3Dfp32& _c);
};

template <class T>
int TPlaneFunctions<T>::OppositeSides(const CVec3Dfp32& p, const CVec3Dfp32& a, const CVec3Dfp32& b, const CVec3Dfp32& c, const CVec3Dfp32& d)
{
	T signp = (p - a) * ((b - a) / (c - a));
	T signd = (d - a) * ((b - a) / (c - a));
	
	if (signd * signp < (1e-4 * 1e-4))
	{
		return -1;
	}
	return signp * signd < TNumericProperties<T>::Zero();
}

template <class T>
int TPlaneFunctions<T>::Outside(const CVec3Dfp32& _p, const CVec3Dfp32& _a, const CVec3Dfp32& _b, const CVec3Dfp32& _c)
{
	return (_p - _a) * ((_b - _a) / (_c - _a)) >= TNumericProperties<T>::Zero();
}



template <class T>
class TClosestPoint
{
public:
	static int ToLineSegment(const CVec3Dfp32& _p, const CVec3Dfp32& _a, const CVec3Dfp32& _b, T &_t, CVec3Dfp32& _closest);
	static int ToLineSegment(const CVec3Dfp32& _p, const CVec3Dfp32& _a, const CVec3Dfp32& _b, CVec3Dfp32& _closest);

	static int ToTriangle(const CVec3Dfp32& _p, const CVec3Dfp32& _a, const CVec3Dfp32& _b, const CVec3Dfp32& _c, CVec3Dfp32& _Closest);
	static int ToTriangle(const CVec3Dfp32& _p, const CVec3Dfp32& _a, const CVec3Dfp32& _b, const CVec3Dfp32& _c, 
						  T& _ta, T& _tb, T& _tc, CVec3Dfp32& _Closest);


	static fp32 ToLineSegmentLineSegment(const CVec3Dfp32& _p1, const CVec3Dfp32& _p2,
										const CVec3Dfp32& _q1, const CVec3Dfp32& _q2,
										fp32& s, fp32& t,
										CVec3Dfp32& _cp1, CVec3Dfp32& _cp2);

	static int ToTriangleLineSegment(const CVec3Dfp32& _p1, const CVec3Dfp32& _p2, const CVec3Dfp32& _a, const CVec3Dfp32& _b, const CVec3Dfp32& _c, 
									 T& _ta, T& _tb, T& _tc, CVec3Dfp32& _Closest1, CVec3Dfp32& _Closest2);

	static int ToTetrahedron(const CVec3Dfp32& _p, const CVec3Dfp32& _a, const CVec3Dfp32& _b, const CVec3Dfp32& _c, const CVec3Dfp32& _d, 
							 T& ta, T& tb, T& tc, T& td, CVec3Dfp32& _Closest);
	static int ToTetrahedron(const CVec3Dfp32& _p, const CVec3Dfp32& _a, const CVec3Dfp32& _b, const CVec3Dfp32& _c, const CVec3Dfp32& _d, CVec3Dfp32& _Closest);
};

template <class T>
int TClosestPoint<T>::ToLineSegment(const CVec3Dfp32& _p, const CVec3Dfp32& _a, const CVec3Dfp32& _b, CVec3Dfp32& _closest)
{
	T dummy;
	return ToLineSegment(_p, _a, _b, dummy, _closest);
}

template <class T>
int TClosestPoint<T>::ToLineSegment(const CVec3Dfp32& _p, const CVec3Dfp32& _a, const CVec3Dfp32& _b, T &_t, CVec3Dfp32& _closest)
{
	CVec3Dfp32 ab = _b - _a;
	_t = (_p - _a) * ab;
	if (_t <= TNumericProperties<T>::Zero())
	{
		_t = TNumericProperties<T>::Zero();
		_closest = _a;
		return (1 << 0);
	}
	else
	{
		T denom = ab * ab;
		if (_t >= denom)
		{
			_t = TNumericProperties<T>::One();
			_closest = _b;
			return (1 << 1);
		}
		else
		{
			_t = _t / denom;
			_closest = _a + ab * _t;
			return (1 << 1 | 1 << 0);
		}
	}
}

template <class T>
int TClosestPoint<T>::ToTriangle(const CVec3Dfp32& _p, const CVec3Dfp32& _a, const CVec3Dfp32& _b, const CVec3Dfp32& _c, CVec3Dfp32& _Closest)
{
	T dummy;
	return ToTriangle(_p, _a, _b, _c, dummy, dummy, dummy, _Closest);
}

template <class T>
int TClosestPoint<T>::ToTriangle(const CVec3Dfp32& _p, const CVec3Dfp32& _a, const CVec3Dfp32& _b, const CVec3Dfp32& _c, 
								 T& _ta, T& _tb, T& _tc, CVec3Dfp32& _Closest)
{
	CVec3Dfp32 ab = _b - _a;
	CVec3Dfp32 ac = _c - _a;
	CVec3Dfp32 ap = _p - _a;
	T d1 = ab * ap;
	T d2 = ac * ap;
	if (d1 <= TNumericProperties<T>::Zero() && d2 <= TNumericProperties<T>::Zero()) 
	{
		_Closest = _a;
		_ta = T(1.0);
		_tb = T(0.0);
		_tc = T(0.0);
		return (1 << 0);
	}

	CVec3Dfp32 bp = _p - _b;
	T d3 = ab * bp;
	T d4 = ac * bp;
	if (d3 >= TNumericProperties<T>::Zero() && d4 <= d3) 
	{
		_Closest = _b;
		_ta = T(0.0);
		_tb = T(1.0);
		_tc = T(0.0);

		return (1 << 1);
	}
	
	T vc = d1*d4 - d3*d2;
	if (vc <= TNumericProperties<T>::Zero() && d1 >= TNumericProperties<T>::Zero() && d3 <= TNumericProperties<T>::Zero())
	{
		T v = d1 / (d1 - d3);
		_Closest =  _a +  ab * v;
		_ta = T(T(1.0) - v);
		_tb = T(v);
		_tc = T(0.0);


		return (1 << 1 | 1 << 0);
	}
	
	CVec3Dfp32 cp = _p - _c;
	T d5 = ab * cp;
	T d6 = ac * cp;
	if (d6 >= TNumericProperties<T>::Zero() && d5 <= d6) 
	{
		_Closest = _c;
		_ta = T(0.0);
		_tb = T(0.0);
		_tc = T(1.0);

		return (1 << 2);
	}
	
	T vb = d5*d2 - d1*d6;
	if (vb <= TNumericProperties<T>::Zero() && d2 >= TNumericProperties<T>::Zero() && d6 <= TNumericProperties<T>::Zero())
	{
		T w = d2 / (d2 - d6);
		_Closest =  _a + ac * w;
		_ta = T(T(1.0) - w);
		_tb = T(0.0);
		_tc = T(w);

		return (1 << 0 | 1 << 2);
	}
	
	T va = d3*d6 - d5*d4;
	if (va <= TNumericProperties<T>::Zero() && (d4 - d3) >= TNumericProperties<T>::Zero() && (d5 - d6) >= TNumericProperties<T>::Zero())
	{
		T w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		_Closest =  _b + (_c - _b) * w;
		_ta = T(0.0);
		_tb = T(T(1.0) - w);
		_tc = T(w);


		return (1 << 1 | 1 << 2);
	}
	
	T denom = TNumericProperties<T>::One() / (va + vb + vc);
	T v = vb * denom;
	T w = vc * denom;
	_Closest =  _a + ab * v + ac * w;
	_ta = T(T(1.0) - v - w);
	_tb = T(v);
	_tc = T(w);

	return (1 << 2 | 1 << 1 | 1 << 0);
}

template <class T>
fp32 TClosestPoint<T>::ToLineSegmentLineSegment(const CVec3Dfp32& p1, const CVec3Dfp32& q1, 
											   const CVec3Dfp32& p2, const CVec3Dfp32& q2, 
											   fp32& s, fp32& t,
											   CVec3Dfp32& c1, CVec3Dfp32& c2)
{
	CVec3Dfp32 d1 = q1 - p1;
	CVec3Dfp32 d2 = q2 - p2;
	CVec3Dfp32 r = p1 - p2;
	fp32 a = d1 * d1;
	fp32 e = d2 * d2;
	fp32 f = d2 * r;

	//fp32 s,t;

	const fp32 EPSILON = 0.0001f;

#if 1
	if (a <= EPSILON && e <= EPSILON) {
		c1 = p1;
		c2 = p2;
		return (c1 - c2) * (c1 - c2);
	}
	if (a <= EPSILON) {
		s = 0.0f;
		t = f / e; // s = 0 => t = (b*s + f) / e = f / e
		t = Clamp01(t);
	} else 
#endif

	{
		float c = d1 * r;
#if 1
		if (e <= EPSILON) {
			t = 0.0f;
			s = Clamp01(-c / a); // t = 0 => s = (b*t - c) / a = -c / a
		} else 
#endif
		{
			float b = d1 * d2;
			float denom = a*e-b*b;

			if (denom != 0.0f) {
				s = Clamp01((b*f - c*e) / denom);
			} else s = 0.0f;

			t = (b*s + f) / e;

//			_Type = 2;

			if (t < 0.0f) {
				t = 0.0f;
//				_Type = 0;
				s = Clamp01(-c / a);
			} else if (t > 1.0f) {
				t = 1.0f;
//				_Type = 2;
				s = Clamp01((b - c) / a);
			}
		}
	}

	c1 = p1;
	d1 *= s;
	c1 += d1;

	c2 = p2;
	d2 *= t;
	c2 += d2;
	//c1 = p1 + d1 * s;
	//c2 = p2 + d2 * t;
	return (c1 - c2) * (c1 - c2);
}

template <class T>
int TClosestPoint<T>::ToTriangleLineSegment(const CVec3Dfp32& _p1, const CVec3Dfp32& _p2, 
											const CVec3Dfp32& _a, const CVec3Dfp32& _b, const CVec3Dfp32& _c, 
											T& _ta, T& _tb, T& _tc,
											CVec3Dfp32& _Closest1,
											CVec3Dfp32& _Closest2)
{
	fp32 s, t;
	CVec3Dfp32 tmp1, tmp2;
	fp32 mindistsq = _FP32_MAX;
	
	fp32 d = ToLineSegmentLineSegment(_p1, _p2, _a, _b, s, t, tmp1, tmp2);
	if (d < mindistsq)
	{
		mindistsq = d;
		_Closest1 = tmp1;
		_Closest2 = tmp2;
	}
	d = ToLineSegmentLineSegment(_p1, _p2, _b, _c, s, t, tmp1, tmp2);
	if (d < mindistsq)
	{
		mindistsq = d;
		_Closest1 = tmp1;
		_Closest2 = tmp2;
	}
	d = ToLineSegmentLineSegment(_p1, _p2, _c, _a, s, t, tmp1, tmp2);
	if (d < mindistsq)
	{
		mindistsq = d;
		_Closest1 = tmp1;
		_Closest2 = tmp2;
	}

	ToTriangle(_p1, _a, _b, _c, tmp1);
	d = _p1.DistanceSqr(tmp1);
	if (d < mindistsq)
	{
		mindistsq = d;
		_Closest1 = _p1;
		_Closest2 = tmp1;
	}

	ToTriangle(_p2, _a, _b, _c, tmp1);
	d = _p2.DistanceSqr(tmp1);
	if (d < mindistsq)
	{
		mindistsq = d;
		_Closest1 = _p2;
		_Closest2 = tmp1;
	}

	// TODO: Returnera en mask samt barycent.
	// Dessa nedan är inte rätt ;-)
	_ta = 10000;
	_tb = 10000;
	_tc = 10000;

	return -1;
}


template <class T>
int TClosestPoint<T>::ToTetrahedron(const CVec3Dfp32& _p, const CVec3Dfp32& _a, const CVec3Dfp32& _b, const CVec3Dfp32& _c, const CVec3Dfp32& _d, 
									CVec3Dfp32& _Closest)
{
	T dummy;
	return ToTetrahedron(_p, _a, _b, _c, _d, dummy, dummy, dummy, dummy, _Closest);
}

#define BITX(x,i) ((x >> i) & 1)

template <class T>
int TClosestPoint<T>::ToTetrahedron(const CVec3Dfp32& _p, const CVec3Dfp32& _a, const CVec3Dfp32& _b, const CVec3Dfp32& _c, const CVec3Dfp32& _d, 
									T& _ta, T& _tb, T& _tc, T& _td, CVec3Dfp32& _Closest)
{
	int ConvexMask = 0;
	_Closest = _p;
	T bestSqDist = TNumericProperties<T>::Max();
	T ta, tb, tc;
//	int side1 = TPlaneFunctions<T>::OppositeSides(_p, _a, _b, _c, _d);
//	int side2 = TPlaneFunctions<T>::OppositeSides(_p, _a, _c, _d, _b);
//	int side3 = TPlaneFunctions<T>::OppositeSides(_p, _a, _d, _b, _c);
//	int side4 = TPlaneFunctions<T>::OppositeSides(_p, _b, _d, _c, _a);
	TPlaneFunctions<T>::OppositeSides(_p, _a, _b, _c, _d);
	TPlaneFunctions<T>::OppositeSides(_p, _a, _c, _d, _b);
	TPlaneFunctions<T>::OppositeSides(_p, _a, _d, _b, _c);
	TPlaneFunctions<T>::OppositeSides(_p, _b, _d, _c, _a);

/*
	if (side1 < 0 || side2 < 0 || side3 < 0 || side4 < 0)
	{
		return -1;
	}
*/

//	if (TPlaneFunctions<T>::Outside(_p, _a, _b, _c)) {
	if (TPlaneFunctions<T>::OppositeSides(_p, _a, _b, _c, _d)) {
		CVec3Dfp32 q;
		int tm = TClosestPoint<T>::ToTriangle(_p, _a, _b, _c, ta, tb, tc, q);
		T sqDist = (q - _p) * (q - _p);
		if (sqDist < bestSqDist) 
		{
//			bestSqDist = sqDist, _Closest = q, ConvexMask = (0 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
			bestSqDist = sqDist, _Closest = q, ConvexMask = (BITX(0,3) | BITX(tm,2) << 2 | BITX(tm,1) << 1 | BITX(tm,0) << 0);

			ConvexMask = 0;
			ConvexMask |= BITX(tm,0) << 0;
			ConvexMask |= BITX(tm,1) << 1;
			ConvexMask |= BITX(tm,2) << 2;
//			ConvexMask |= BITX(tm,2) << 3;


			_ta = ta;
			_tb = tb;
			_tc = tc;
			_td = T(0.0);
		}
	}
//	if (TPlaneFunctions<T>::Outside(_p, _a, _c, _d)) {
	if (TPlaneFunctions<T>::OppositeSides(_p, _a, _c, _d, _b)) {
		CVec3Dfp32 q;
		int tm = TClosestPoint<T>::ToTriangle(_p, _a, _c, _d, ta, tb, tc, q);
		T sqDist = (q - _p) * (q - _p);
		if (sqDist < bestSqDist) 
		{
		
//			bestSqDist = sqDist, _Closest = q, ConvexMask = (1 << 3 | 1 << 2 | 0 << 1 | 1 << 0);
			bestSqDist = sqDist, _Closest = q, ConvexMask = (BITX(tm,2) << 3 | BITX(tm,1) << 2 | BITX(0,2) << 1 | BITX(tm,0) << 0);

			ConvexMask = 0;
			ConvexMask |= BITX(tm,0) << 0;
			//ConvexMask |= BITX(tm,3) << 1;
			ConvexMask |= BITX(tm,1) << 2;
			ConvexMask |= BITX(tm,2) << 3;

			_ta = ta;
			_tb = T(0.0);
			_tc = tb;
			_td = tc;
		}
	}
//	if (TPlaneFunctions<T>::Outside(_p, _a, _d, _b)) {
	if (TPlaneFunctions<T>::OppositeSides(_p, _a, _d, _b, _c)) {
		CVec3Dfp32 q;
		int tm = TClosestPoint<T>::ToTriangle(_p, _a, _d, _b, ta, tb, tc, q);
		T sqDist = (q - _p) * (q - _p);
		if (sqDist < bestSqDist) 
		{
//			bestSqDist = sqDist, _Closest = q, ConvexMask = (1 << 3 | 0 << 2 | 1 << 1 | 1 << 0);
			bestSqDist = sqDist, _Closest = q, ConvexMask = (BITX(tm,1) << 3 | BITX(0,2) << 2 | BITX(tm,2) << 1 | BITX(tm,0) << 0);

			ConvexMask = 0;
			ConvexMask |= BITX(tm,0) << 0;
			ConvexMask |= BITX(tm,2) << 1;
			//ConvexMask |= BITX(tm,1) << 2;
			ConvexMask |= BITX(tm,1) << 3;

			_ta = ta;
			_tb = tc;
			_tc = T(0.0);
			_td = tb;
		}
	}
//	if (TPlaneFunctions<T>::Outside(_p, _b, _d, _c)) {
	if (TPlaneFunctions<T>::OppositeSides(_p, _b, _d, _c, _a)) {
		CVec3Dfp32 q;
		int tm = TClosestPoint<T>::ToTriangle(_p, _b, _d, _c, ta, tb, tc, q);
		T sqDist = (q - _p) * (q - _p);
		if (sqDist < bestSqDist) 
		{
//			bestSqDist = sqDist, _Closest = q, ConvexMask = (1 << 3 | 1 << 2 | 1 << 1 | 0 << 0);
			bestSqDist = sqDist, _Closest = q, ConvexMask = (BITX(tm,1) << 3 | BITX(tm,2) << 2 | BITX(tm,0) << 1 | BITX(0,0) << 0);

			ConvexMask = 0;
			//ConvexMask |= BITX(tm,0) << 0;
			ConvexMask |= BITX(tm,0) << 1;
			ConvexMask |= BITX(tm,2) << 2;
			ConvexMask |= BITX(tm,1) << 3;

			_ta = T(0.0);
			_tb = ta;
			_tc = tc;
			_td = tb;
		}
	}

	return ConvexMask;
}

#endif
