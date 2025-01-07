
#include "PCH.h"

#include "CSinRandTable.h"

CSinRandTable1024x8 g_SinRandTable;

template<>
CSinRandTable<1024, 8>::CSinRandTable()
{
	fp32 pi2 = 2.0f * _PI;
	fp32 x, y, dy;

	for (int i = 0; i < 1024; i++) {
		x = (fp32)i / (fp32)1024;
		y = 0;

		dy = M_Sin(3.0f * pi2 * (x + 0.0f));
		if (dy < -1.0f) dy = -1.0f;
		if (dy > +1.0f) dy = +1.0f;
		y += 0.5f * dy;

		dy = M_Sin(7.0f * pi2 * (x + 0.3f));
		if (dy < -1.0f) dy = -1.0f;
		if (dy > +1.0f) dy = +1.0f;
		y += 0.25f * dy;

		dy = M_Sin(13.0f * pi2 * (x + 0.7f));
		if (dy < -1.0f) dy = -1.0f;
		if (dy > +1.0f) dy = +1.0f;
		y += 0.125f * dy;

		m_Table[i] = 0.5f + 0.5f * (y / 0.875f);
	}

	m_iTableSize = 1024 - 1;
	m_fTableSize = 1024 - 1;
	m_iAccuricy = (1 << 8) - 1;
	m_fAccuricy = (1 << 8) - 1;
	m_fInvAccuricy = 1.0f / m_fAccuricy;
	m_fSizeAccScale = m_fTableSize * m_fAccuricy;
}

template<>
fp32 CSinRandTable<1024, 8>::GetRand(fp32 _Time) const
{
	fp32 value;

	fp32 y1, y2;
	int32 x, xi;
	fp32 xf;

	x = RoundToInt(m_fSizeAccScale * _Time);

	xi = (x >> 8) & m_iTableSize;
	xf = (x & m_iAccuricy) * m_fInvAccuricy;

	y1 = m_Table[xi];
	y2 = m_Table[(xi + 1) & m_iTableSize];

	value = LERP(y1, y2, xf);

	return value;
}
