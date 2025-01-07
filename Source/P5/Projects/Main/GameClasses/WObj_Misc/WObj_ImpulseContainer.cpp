#include "PCH.h"
#include "WObj_ImpulseContainer.h"

bool CWO_ImpulseContainer::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey, CWorld_Server *_pWServer)
{
	if(_pKey->GetThisName().CompareSubStr("ONIMPULSE") == 0)
	{
		CStr St = _pKey->GetThisValue();
		int iImpulse = St.GetStrSep(";").Val_int();

		int i;
		for(i = 0; i < m_lImpulses.Len(); i++)
			if(m_lImpulses[i].m_iImpulse == iImpulse)
				break;

		if(i == m_lImpulses.Len())
			m_lImpulses.Add(CWO_Impulse(iImpulse));

		CWO_SimpleMessage Msg;
		Msg.Parse(St, _pWServer);
		m_lImpulses[i].m_lMessages.Add(Msg);
	}
	else
		return false;

	return true;
}

int CWO_ImpulseContainer::OnImpulse(int _iImpulse, int _iObject, int _iActivator, CWorld_Server *_pWServer)
{
	for(int i = 0; i < m_lImpulses.Len(); i++)
		if(m_lImpulses[i].m_iImpulse == _iImpulse)
		{
			for(int j = 0; j < m_lImpulses[i].m_lMessages.Len(); j++)
				m_lImpulses[i].m_lMessages[j].SendMessage(_iObject, _iActivator, _pWServer);
			return 1;
		}
	return 0;
}

void CWO_ImpulseContainer::SendPrecache(int _iObject, CWorld_Server *_pWServer)
{
	for(int i = 0; i < m_lImpulses.Len(); i++)
		for(int j = 0; j < m_lImpulses[i].m_lMessages.Len(); j++)
			m_lImpulses[i].m_lMessages[j].SendPrecache(_iObject, _pWServer);
}
