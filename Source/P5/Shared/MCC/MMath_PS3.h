
#if defined(PLATFORM_PS3)

template <>
fp4 M_INLINE TVector3Aggr<fp4>::AngleFromVector(fp4 x, fp4 z)
{
	fp4 absx = M_Fabs(x);
	fp4 absz = M_Fabs(z);

	if (absx > absz)
	{
		fp4 v = M_ATan(absz/absx) * ((1.0f / _PI) * 0.5f);

		return M_FSel(x, M_FSel(z, v, -v), M_FSel(z, -v, v) + 0.5f);
	}
	else
	{
		fp4 v = M_ATan(absx/absz) * ((1.0f / _PI) * 0.5f);

		return M_FSel(z, M_FSel(x, -v, v), M_FSel(x, v, -v) + 0.5f) + 0.25f;
	}
}

template <>
fp8 M_INLINE TVector3Aggr<fp8>::AngleFromVector(fp8 x, fp8 z)
{
	fp8 absx = M_Fabs(x);
	fp8 absz = M_Fabs(z);

	if (absx > absz)
	{
		fp8 v = M_ATan(absz/absx) * ((1.0 / _PI) * 0.5);

		return M_FSel(x, M_FSel(z, v, -v), M_FSel(z, -v, v) + 0.5);
	}
	else
	{
		fp8 v = M_ATan(absx/absz) * ((1.0 / _PI) * 0.5);

		return M_FSel(z, M_FSel(x, -v, v), M_FSel(x, v, -v) + 0.5) + 0.25;
	}
}

template <>
void M_INLINE CQuatfp4::Interpolate(const TQuaternion& _Other, TQuaternion& _Dest, fp4 _t) const
{
	fp4 a0 = k[0];
	fp4 a1 = k[1];
	fp4 a2 = k[2];
	fp4 a3 = k[3];

	fp4 dot = (a0 * _Other.k[0] + a1 * _Other.k[1] + a2 * _Other.k[2] + a3 * _Other.k[3]);
	fp4 u = M_FSel(dot, 1.0f - _t, _t - 1.0f);

	fp4 d0 = a0 * u + _Other.k[0] * _t;
	fp4 d1 = a1 * u + _Other.k[1] * _t;
	fp4 d2 = a2 * u + _Other.k[2] * _t;
	fp4 d3 = a3 * u + _Other.k[3] * _t;
	fp4 InvLen = M_InvSqrt(Sqr(d0) + Sqr(d1) + Sqr(d2) + Sqr(d3));
#ifndef M_RTM
	if (InvLen == (fp4) 0) return;
#endif
	_Dest.k[0] = d0 * InvLen;
	_Dest.k[1] = d1 * InvLen;
	_Dest.k[2] = d2 * InvLen;
	_Dest.k[3] = d3 * InvLen;
}

template <>
void M_INLINE CQuatfp8::Interpolate(const TQuaternion& _Other, TQuaternion& _Dest, fp4 _t) const
{
	fp8 a0 = k[0];
	fp8 a1 = k[1];
	fp8 a2 = k[2];
	fp8 a3 = k[3];

	fp8 dot = (a0 * _Other.k[0] + a1 * _Other.k[1] + a2 * _Other.k[2] + a3 * _Other.k[3]);
	fp8 u = M_FSel(dot, 1.0 - _t, _t - 1.0);

	fp8 d0 = a0 * u + _Other.k[0] * _t;
	fp8 d1 = a1 * u + _Other.k[1] * _t;
	fp8 d2 = a2 * u + _Other.k[2] * _t;
	fp8 d3 = a3 * u + _Other.k[3] * _t;
	fp8 InvLen = M_InvSqrt(Sqr(d0) + Sqr(d1) + Sqr(d2) + Sqr(d3));
#ifndef M_RTM
	if (InvLen == (fp8) 0) return;
#endif
	_Dest.k[0] = d0 * InvLen;
	_Dest.k[1] = d1 * InvLen;
	_Dest.k[2] = d2 * InvLen;
	_Dest.k[3] = d3 * InvLen;
}

template<>
bool M_INLINE CBox3Dfp4::IsInside(const CBox3Dfp4& _Box) const
{
	fp4 Inside0 = M_FSel(_Box.m_Max.k[0] - m_Min.k[0], 1.0f, -1.0f);
	fp4 Inside1 = M_FSel(_Box.m_Max.k[1] - m_Min.k[1], 1.0f, -1.0f);
	fp4 Inside2 = M_FSel(_Box.m_Max.k[2] - m_Min.k[2], 1.0f, -1.0f);

	fp4 Inside3 = M_FSel(m_Max.k[0] - _Box.m_Min.k[0], 1.0f, -1.0f);
	fp4 Inside4 = M_FSel(m_Max.k[1] - _Box.m_Min.k[1], 1.0f, -1.0f);
	fp4 Inside5 = M_FSel(m_Max.k[2] - _Box.m_Min.k[2], 1.0f, -1.0f);

	fp4 Resolve = M_FSel(Inside5, 1.0f, M_FSel(Inside4, 1.0f, M_FSel(Inside3, 1.0f, M_FSel(Inside2, 1.0f, M_FSel(Inside1, 1.0f, M_FSel(Inside0, 1.0f, 0.0f))))));

	return Resolve > 0.0f;
}

template <>
void M_INLINE CBox3Dfp4::Grow(const V& _v)
{
	m_Min.k[0] += M_FSel(_v[0], 0.0f, _v[0]);
	m_Max.k[0] += M_FSel(_v[0], _v[0], 0.0f);
	m_Min.k[1] += M_FSel(_v[1], 0.0f, _v[1]);
	m_Max.k[1] += M_FSel(_v[1], _v[1], 0.0f);
	m_Min.k[2] += M_FSel(_v[2], 0.0f, _v[2]);
	m_Max.k[2] += M_FSel(_v[2], _v[2], 0.0f);
}


#endif // PLATFORM_PS3
