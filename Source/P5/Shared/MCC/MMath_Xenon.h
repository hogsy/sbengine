
#if defined(PLATFORM_XENON)

template <>
fp32 M_INLINE CVec3Dfp32::AngleFromVector(fp32 x, fp32 z)
{
	fp32 absx = M_Fabs(x);
	fp32 absz = M_Fabs(z);

	if (absx > absz)
	{
		fp32 v = M_ATan(absz/absx) * ((1.0f / _PI) * 0.5f);

		return M_FSel(x, M_FSel(z, v, -v), M_FSel(z, -v, v) + 0.5f);
	}
	else
	{
		fp32 v = M_ATan(absx/absz) * ((1.0f / _PI) * 0.5f);

		return M_FSel(z, M_FSel(x, -v, v), M_FSel(x, v, -v) + 0.5f) + 0.25f;
	}
}

template <>
fp64 M_INLINE CVec3Dfp64::AngleFromVector(fp64 x, fp64 z)
{
	fp64 absx = M_Fabs(x);
	fp64 absz = M_Fabs(z);

	if (absx > absz)
	{
		fp64 v = M_ATan(absz/absx) * ((1.0 / _PI) * 0.5);

		return M_FSel(x, M_FSel(z, v, -v), M_FSel(z, -v, v) + 0.5);
	}
	else
	{
		fp64 v = M_ATan(absx/absz) * ((1.0 / _PI) * 0.5);

		return M_FSel(z, M_FSel(x, -v, v), M_FSel(x, v, -v) + 0.5) + 0.25;
	}
}

template<>
bool M_INLINE CBox3Dfp32::IsInside(const CBox3Dfp32& _Box) const
{
	fp32 Inside0 = M_FSel(_Box.m_Max.k[0] - m_Min.k[0], 1.0f, -1.0f);
	fp32 Inside1 = M_FSel(_Box.m_Max.k[1] - m_Min.k[1], 1.0f, -1.0f);
	fp32 Inside2 = M_FSel(_Box.m_Max.k[2] - m_Min.k[2], 1.0f, -1.0f);

	fp32 Inside3 = M_FSel(m_Max.k[0] - _Box.m_Min.k[0], 1.0f, -1.0f);
	fp32 Inside4 = M_FSel(m_Max.k[1] - _Box.m_Min.k[1], 1.0f, -1.0f);
	fp32 Inside5 = M_FSel(m_Max.k[2] - _Box.m_Min.k[2], 1.0f, -1.0f);

	fp32 Resolve = M_FSel(Inside5, 1.0f, M_FSel(Inside4, 1.0f, M_FSel(Inside3, 1.0f, M_FSel(Inside2, 1.0f, M_FSel(Inside1, 1.0f, M_FSel(Inside0, 1.0f, 0.0f))))));

	return Resolve > 0.0f;
}

template <>
void M_INLINE CBox3Dfp32::Grow(const V& _v)
{
	m_Min.k[0] += M_FSel(_v[0], 0.0f, _v[0]);
	m_Max.k[0] += M_FSel(_v[0], _v[0], 0.0f);
	m_Min.k[1] += M_FSel(_v[1], 0.0f, _v[1]);
	m_Max.k[1] += M_FSel(_v[1], _v[1], 0.0f);
	m_Min.k[2] += M_FSel(_v[2], 0.0f, _v[2]);
	m_Max.k[2] += M_FSel(_v[2], _v[2], 0.0f);
}

#endif // PLATFORM_XENON
