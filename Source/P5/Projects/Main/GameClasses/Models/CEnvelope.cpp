#include "PCH.h"

#include "CEnvelope.h"

fp32 CEnvelope::getTCBSpline(fp32 time)
{
	MAUTOSTRIP( CEnvelope_getTCBSpline, 0.0f );
	int iKey1;
	fp32 Key0Value, Key0Time;
	fp32 Key3Value, Key3Time;

	if (!m_NoClip)
	{
		if (time < m_Keys[0].m_Time)
			return m_Keys[0].m_Value;
		
		if (time > m_Keys[m_NumKeys - 1].m_Time)
			return m_Keys[m_NumKeys - 1].m_Value;
	}
	
	iKey1 = 0;

	while (time > m_Keys[iKey1 + 1].m_Time)
		iKey1++;

	const int iKey0 = iKey1 - 1;
	const int iKey2 = iKey1 + 1;
	const int iKey3 = iKey1 + 2;
	
	Key& Key0 = m_Keys[iKey0];
	Key& Key1 = m_Keys[iKey1];
	Key& Key2 = m_Keys[iKey2];
	Key& Key3 = m_Keys[iKey3];

	if (iKey0 < 0)
	{
		Key0Time = 2.0f * Key1.m_Time - Key2.m_Time;
		Key0Value = 2.0f * Key1.m_Value - Key2.m_Value;
//			Key0Time = m_Keys[iKey1].m_Time;
//			Key0Value = m_Keys[iKey1].m_Value;
	} else
	{
		Key0Time = Key0.m_Time;
		Key0Value = Key0.m_Value;
	}

	if (iKey3 > (m_NumKeys - 1))
	{
		Key3Time = 2.0f * Key2.m_Time - Key1.m_Time;
		Key3Value = 2.0f * Key2.m_Value - Key1.m_Value;
//			Key3Time = m_Keys[iKey2].m_Time;
//			Key3Value = m_Keys[iKey2].m_Value;
	}
	else
	{
		Key3Time = Key3.m_Time;
		Key3Value = Key3.m_Value;
	}

	fp32 timefraction;
	if (iKey1 < iKey2)
		timefraction = (time - Key1.m_Time) / (Key2.m_Time - Key1.m_Time);
	else
		timefraction = 1.0f;

	// Get segment endpoints.
	const fp32 point1 = Key1.m_Value;
	const fp32 point2 = Key2.m_Value;

	// Adjust tangents/speed with time differance between segments.
	const fp32 cur = Key2.m_Time - Key1.m_Time;
	const fp32 prev = Key1.m_Time - Key0Time;
	const fp32 next = Key3Time - Key2.m_Time;
	const fp32 ecur = 0.0f * cur;

	// Calculate segment endpoint tangents.
	const fp32 tangent1 = 0.5f * (1.0f - Key1.m_Tension) * 
								(1.0f + Key1.m_Discontinuity) * 
								(1.0f + Key1.m_Bias) * 
								(Key1.m_Value - Key0Value) +
						 0.5f * (1.0f - Key1.m_Tension) * 
								(1.0f - Key1.m_Discontinuity) * 
								(1.0f - Key1.m_Bias) * 
								(Key2.m_Value - Key1.m_Value) * ((ecur + cur) / (ecur + prev));
	const fp32 tangent2 = 0.5f * (1.0f - Key2.m_Tension) * 
								(1.0f - Key2.m_Discontinuity) * 
								(1.0f + Key2.m_Bias) * 
								(Key2.m_Value - Key1.m_Value) * ((ecur + cur) / (ecur + next)) +
						 0.5f * (1.0f - Key2.m_Tension) * 
								(1.0f + Key2.m_Discontinuity) * 
								(1.0f - Key2.m_Bias) * 
								(Key3Value - Key2.m_Value);

	// Calculate parameter powers.
	const fp32 t1 = timefraction;
	const fp32 t2 = t1 * t1;
	const fp32 t3 = t1 * t2;

	// Calculate hermite polynomial coefficients.
	const fp32 m1 = 2.0f * t3 - 3.0f * t2 + 1.0f;
	const fp32 m2 = -2.0f * t3 + 3.0f * t2;
	const fp32 m3 = t3 - 2.0f * t2 + t1;
	const fp32 m4 = t3 - t2;

	// Calculate hermite polynomial.
	const fp32 point = m1 * point1 + m2 * point2 + m3 * tangent1 + m4 * tangent2;

	return point;
}

fp32 CEnvelope::getValue(fp32 time)
{
	MAUTOSTRIP( CEnvelope_getValue, 0.0f );
	if (m_Method == IPMethod_TCBSpline)
	{
		return getTCBSpline(time);
	}

	if (m_Keys == NULL)
		return 0.0f;

	if (m_NumKeys < 2)
		return m_Keys[0].m_Value;

	if (time <= m_Keys[0].m_Time)
		return m_Keys[0].m_Value;

	if (time >= m_Keys[m_NumKeys - 1].m_Time)
		return m_Keys[m_NumKeys - 1].m_Value;

	int iKey = 0;
	fp32 fraction = 0.0f;

	while (time > m_Keys[iKey + 1].m_Time)
		iKey++;

	if (m_Method == IPMethod_Step)
		return m_Keys[iKey].m_Value;

	if (iKey >= (m_NumKeys - 1)) {
		iKey--;
		fraction = 1.0f;
	} else {
		fraction = (time - m_Keys[iKey].m_Time) / (m_Keys[iKey + 1].m_Time - m_Keys[iKey].m_Time);
	}

	if (m_Method == IPMethod_Linear) {
		fp32 value = m_Keys[iKey].m_Value + fraction * (m_Keys[iKey + 1].m_Value - m_Keys[iKey].m_Value);
		return value;
	}

	if (m_Method == IPMethod_Cosine) {
		fraction = 0.5f - 0.5f * M_Cos(fraction * _PI);
		fp32 value = m_Keys[iKey].m_Value + fraction * (m_Keys[iKey + 1].m_Value - m_Keys[iKey].m_Value);
		return value;
	}

	if (m_Method == IPMethod_Spline)
	{
		int i0, i1, i2, i3;
		i1 = iKey;
		i0 = i1 - 1;
		i2 = i1 + 1;
		i3 = i1 + 2;

		if (i0 < 0)
			i0 = 0;

		if (i2 >= m_NumKeys)
			i2 = m_NumKeys - 1;

		if (i3 >= m_NumKeys)
			i3 = m_NumKeys - 1;

		const fp32 t = fraction;
		const fp32 t2 = t*t;
		const fp32 t3 = t2*t;
		const fp32 m0 = -t3 + 2*t2 - t;
		const fp32 m1 = t3 - 2*t2  + 1;
		const fp32 m2 = -t3 + t2 + t;
		const fp32 m3 = t3 - t2;
		const fp32 value = m0 * m_Keys[i0].m_Value + m1 * m_Keys[i1].m_Value + m2 * m_Keys[i2].m_Value + m3 * m_Keys[i3].m_Value;

		return value;
	}

	return 0.0f;
}

void CEnvelope::Render(CWireContainer* _pWC, fp32 scale, int32 linecolor, int32 pointcolor, fp32 duration)
{
	MAUTOSTRIP( CEnvelope_Render, MAUTOSTRIP_VOID );
	if (_pWC == NULL)
		return;

	for (int iKey = 0; iKey < m_NumKeys; iKey++)
	{
		_pWC->RenderVertex(CVec3Dfp32(m_Keys[iKey].m_Time * scale, 0, m_Keys[iKey].m_Value * scale), pointcolor, duration, true);
	}

	const fp32 start = m_Keys[0].m_Time - 0.5f;
	const fp32 end = m_Keys[m_NumKeys - 1].m_Time + 0.5f;

	CVec3Dfp32 p1, p2;
	p1 = 0;

	for (fp32 t = start; t <= end; t += ((end - start) / 1000.0f))
	{
		p2 = CVec3Dfp32(t * scale, 0, getValue(t) * scale);
		_pWC->RenderWire(p1, p2, linecolor, duration, true);
		p1 = p2;
	}
}
