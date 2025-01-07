#ifndef CEnvelope_h
#define CEnvelope_h

//----------------------------------------------------------------------
// CEnvelope
//----------------------------------------------------------------------

class CEnvelope
{

public:

	enum IPMethod
	{
		IPMethod_Step,
		IPMethod_Linear,
		IPMethod_Cosine,
		IPMethod_Spline,
		IPMethod_TCBSpline
	};

	class Key
	{
		public:

			fp32 m_Time;
			fp32 m_Value;
			fp32 m_Tension, m_Discontinuity, m_Bias;

			Key()
			{
				m_Time = 0.0f;
				m_Value = 0.0f;
				m_Tension = 0.0f;
				m_Discontinuity = 0.0f;
				m_Bias = 0.0f;
			}

			Key(fp32 time, fp32 value)
			{
				m_Time = time;
				m_Value = value;
				m_Tension = 0.0f;
				m_Discontinuity = 0.0f;
				m_Bias = 0.0f;
			}

			Key(fp32 time, fp32 value, fp32 tension, fp32 discontinuity, fp32 bias)
			{
				m_Time = time;
				m_Value = value;
				m_Tension = tension;
				m_Discontinuity = discontinuity;
				m_Bias = bias;
			}
	};

private:

	Key*		m_Keys;
	int			m_NumKeys;
	IPMethod	m_Method;
	bool		m_NoClip;

public:
	
	CEnvelope()
	{
		m_Keys = NULL;
		m_NumKeys = 0;
		m_Method = IPMethod_Linear;
		m_NoClip = false;
	}

	CEnvelope(Key* keys, int numKeys, IPMethod ipmethod)
	{
		Create(keys, numKeys, ipmethod);
	}

	void Create(Key* keys, int numKeys, IPMethod ipmethod, bool noclip = false)
	{
		m_Keys = keys;
		m_NumKeys = numKeys;
		m_Method = ipmethod;
		m_NoClip = noclip;
	}

	void Render(class CWireContainer* _pWC, fp32 scale, int32 linecolor, int32 pointcolor, fp32 duration);
	fp32 getTCBSpline(fp32 time);
	fp32 getValue(fp32 time);
};

//----------------------------------------------------------------------

#endif /* CEnvelope_h */
