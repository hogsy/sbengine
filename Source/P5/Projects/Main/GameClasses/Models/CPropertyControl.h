#ifndef CPropertyControl_h
#define CPropertyControl_h

#include "PCH.h"
#include "MFloat.h"

#include "CSinRandTable.h"
#include "ModelsMisc.h"

//----------------------------------------------------------------------
// CPropertyControl
//----------------------------------------------------------------------

class CPropertyControl
{
public:

	enum
	{
		VALUETYPE_ZERO				= 0x01,
		VALUETYPE_CONSTANT			= 0x02,
		VALUETYPE_LINEAR			= 0x04,
		VALUETYPE_RANDOMCONSTANT	= 0x08,
		VALUETYPE_RANDOMFLUCT		= 0x10,
	};

	fp32 m_StartMin, m_StartMax;
	fp32 m_EndMin, m_EndMax;
	fp32 m_FluctTimescale;
	fp32 m_FluctTimeSpread;

/*
	bool m_bZero;
	bool m_bConstant;
	bool m_bLinear;
	bool m_bRandomConstant;
	bool m_bRandomFluct;
*/

	int m_ValueType;

	CPropertyControl()
	{
		Set(0.0f);
		SetSpeed(0.0f);
		m_FluctTimeSpread = 0;

		m_ValueType = VALUETYPE_ZERO;
/*
		m_bZero = false;
		m_bConstant = false;
		m_bLinear = false;
		m_bRandomConstant = false;
		m_bRandomFluct = false;
*/
	}

	void SetMin(fp32 _Min) { m_StartMin = m_EndMin = _Min; }
	void SetMax(fp32 _Max) { m_StartMax = m_EndMax = _Max; }
	void SetStart(fp32 _Start) { m_StartMin = m_StartMax = _Start; }
	void SetEnd(fp32 _End) { m_EndMin = m_EndMax = _End; }
	void Set(fp32 _Value) { m_StartMin = m_StartMax = m_EndMin = m_EndMax = _Value; }
	void SetSpeed(fp32 _Speed) { m_FluctTimescale = _Speed; }

	void Optimize()
	{
		if ((m_StartMin == m_StartMax) && (m_EndMin == m_EndMax))
		{
			if (m_StartMin == m_EndMin)
			{
				if (m_StartMin == 0.0f)
				{
					//m_bZero = true;
					m_ValueType = VALUETYPE_ZERO;
				}
				else
				{
					//m_bConstant = true;
					m_ValueType = VALUETYPE_CONSTANT;
				}
			}
			else
			{
				//m_bLinear = true;
				m_ValueType = VALUETYPE_LINEAR;
			}
		}
		else
		{
			if (m_FluctTimescale == 0)
				//m_bRandomConstant = true;
				m_ValueType = VALUETYPE_RANDOMCONSTANT;
			else
				//m_bRandomFluct = true;
				m_ValueType = VALUETYPE_RANDOMFLUCT;
		}
	}

	fp32 GetConstant() const
	{
		return m_StartMin;
	}

	fp32 GetLinear(fp32 _TimeFraction) const
	{
		return LERP(m_StartMin, m_EndMin, _TimeFraction);
	}

	fp32 GetRandomS(fp32 _TimeFraction, uint32& _Randseed) const
	{
		fp32 amp;
		amp = GetRand(_Randseed);
		fp32 _min = LERP(m_StartMin, m_EndMin, _TimeFraction);
		fp32 _max = LERP(m_StartMax, m_EndMax, _TimeFraction);
		return LERP(_min, _max, amp);
	}

	fp32 GetFluctS(fp32 _Time, fp32 _TimeFraction, const CSinRandTable1024x8* M_RESTRICT _SinRandTable, uint32& _Randseed) const
	{
		fp32 amp;
		amp = _SinRandTable->GetRand(_Time * m_FluctTimescale);
		fp32 _min = LERP(m_StartMin, m_EndMin, _TimeFraction);
		fp32 _max = LERP(m_StartMax, m_EndMax, _TimeFraction);
		return LERP(_min, _max, amp);
	}

	CVec3Dfp32 GetRandomV(fp32 _TimeFraction, uint32& _Randseed) const
	{
		CVec3Dfp32 amp;

		amp[0] = GetRand(_Randseed);
		amp[1] = GetRand(_Randseed);
		amp[2] = GetRand(_Randseed);

		fp32 _min = LERP(m_StartMin, m_EndMin, _TimeFraction);
		fp32 _max = LERP(m_StartMax, m_EndMax, _TimeFraction);

		CVec3Dfp32 value;

		value[0] = LERP(_min, _max, amp[0]);
		value[1] = LERP(_min, _max, amp[1]);
		value[2] = LERP(_min, _max, amp[2]);

		return value;
	}

	CVec3Dfp32 GetFluctV(fp32 _Time, fp32 _TimeFraction, const CSinRandTable1024x8* M_RESTRICT  _SinRandTable, uint32& _Randseed) const
	{
		CVec3Dfp32 amp;

		amp[0] = _SinRandTable->GetRand((_Time + m_FluctTimeSpread * GetRand(_Randseed)) * m_FluctTimescale);
		amp[1] = _SinRandTable->GetRand((_Time + m_FluctTimeSpread * GetRand(_Randseed)) * m_FluctTimescale);
		amp[2] = _SinRandTable->GetRand((_Time + m_FluctTimeSpread * GetRand(_Randseed)) * m_FluctTimescale);

		fp32 _min = LERP(m_StartMin, m_EndMin, _TimeFraction);
		fp32 _max = LERP(m_StartMax, m_EndMax, _TimeFraction);

		CVec3Dfp32 value;

		value[0] = LERP(_min, _max, amp[0]);
		value[1] = LERP(_min, _max, amp[1]);
		value[2] = LERP(_min, _max, amp[2]);

		return value;
	}

};

#endif /* CPropertyControl_h */
