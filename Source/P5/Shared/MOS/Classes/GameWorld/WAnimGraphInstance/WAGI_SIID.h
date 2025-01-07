#ifndef WAGI_SIID_h
#define WAGI_SIID_h

//--------------------------------------------------------------------------------

#include "../../../XR/XRAnimGraph/AnimGraphDefs.h"
#include "../WAnimGraphInstance/WAGI_Defs.h"

//--------------------------------------------------------------------------------

// CWAGI_StateInstanceID
class CWAGI_SIID
{
	public:

		CMTime				m_EnqueueTime;
		int16				m_iEnterAction;

	public:

		CWAGI_SIID() { Clear(); }

		void Clear()
		{
			m_EnqueueTime = AGI_UNDEFINEDTIME;
			m_iEnterAction = AG_ACTIONINDEX_NULL;
		}

		CWAGI_SIID(CMTime _EnqueueTime, int16 _iEnterAction)
		{
			m_EnqueueTime = _EnqueueTime;
			m_iEnterAction = _iEnterAction;
		}

		CMTime GetEnqueueTime() const { return m_EnqueueTime; }
		int16 GetEnterActionIndex() const { return m_iEnterAction; }

		bool IsValid() { return ((m_EnqueueTime.AlmostEqual(AGI_UNDEFINEDTIME) == false) && (m_iEnterAction != AG_ACTIONINDEX_NULL)); }

		CStr GetString() const
		{
			return CStrF("ET %3.2f, iEA %d", m_EnqueueTime, m_iEnterAction);
		}

};

//--------------------------------------------------------------------------------

#endif /* WAGI_SIID_h */
