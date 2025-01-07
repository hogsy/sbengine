#ifndef WAG2I_SIID_h
#define WAG2I_SIID_h

//--------------------------------------------------------------------------------

#include "../../../XR/XRAnimGraph2/AnimGraph2Defs.h"
#include "../WAnimGraph2Instance/WAG2I_Defs.h"

//--------------------------------------------------------------------------------

// CWAG2I_StateInstanceID
class CWAG2I_SIID
{
	public:

		CMTime				m_EnqueueTime;
		int16				m_iEnterMoveToken;
		CAG2AnimGraphID		m_iAnimGraph;

	public:

		CWAG2I_SIID() { Clear(); }

		void Clear()
		{
			m_EnqueueTime = AG2I_UNDEFINEDTIME;
			m_iEnterMoveToken = AG2_MOVETOKENINDEX_NULL;
			m_iAnimGraph = -1;
		}

		CWAG2I_SIID(CMTime _EnqueueTime,int16 _iEnterMoveToken, CAG2AnimGraphID _iAnimGraph)
		{
			m_EnqueueTime = _EnqueueTime;
			m_iEnterMoveToken = _iEnterMoveToken;
			m_iAnimGraph = _iAnimGraph;
		}

		M_AGINLINE CMTime GetEnqueueTime() const { return m_EnqueueTime; }
		M_AGINLINE int16 GetEnterMoveTokenIndex() const { return m_iEnterMoveToken; }
		M_AGINLINE CAG2AnimGraphID GetAnimGraphIndex() const { return m_iAnimGraph;	}

		bool IsValid() { return ((m_EnqueueTime.Compare(AG2I_UNDEFINEDTIME) != 0) && (m_iEnterMoveToken != AG2_MOVETOKENINDEX_NULL)); }

		void OnCreateClientUpdate(uint8*& _pData) const
		{
			PTR_PUTCMTIME(_pData, m_EnqueueTime);
			PTR_PUTINT16(_pData, m_iEnterMoveToken);
			PTR_PUTINT8(_pData, m_iAnimGraph);
		}
		void OnClientUpdate(const uint8*& _pData)
		{
			PTR_GETCMTIME(_pData, m_EnqueueTime);
			PTR_GETINT16(_pData, m_iEnterMoveToken);
			PTR_GETINT8(_pData, m_iAnimGraph);
		}

		CStr GetString() const
		{
			return CStrF("ET %3.2f, iEMT %d iAG: %d", m_EnqueueTime.GetTime(), m_iEnterMoveToken, m_iAnimGraph);
		}

		void Write(CCFile* _pFile) const;

		void Read(CCFile* _pFile);
};

//--------------------------------------------------------------------------------

#endif /* WAG2I_SIID_h */
