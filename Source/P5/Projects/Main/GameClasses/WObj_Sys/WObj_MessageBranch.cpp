#include "PCH.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AutoVar.h"
#include "WObj_Trigger.h"

// -------------------------------------------------------------------
//  CWObject_MessageBranch
// -------------------------------------------------------------------
class CWObject_MessageBranch : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	TArray<CWO_SimpleMessage> m_lMessages;

	CWObject_MessageBranch()
	{
		MAUTOSTRIP(CWObject_MessageBranch_ctor, MAUTOSTRIP_VOID);
	}

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		MAUTOSTRIP(CWObject_MessageBranch_OnEvalKey, MAUTOSTRIP_VOID);

		if(_pKey->GetThisName().CompareSubStr("MSG_IMPULSE") == 0)
		{
			int Index = _pKey->GetThisName().Copy(11, 1024).Val_int();
			if(Index > 0)
			{
				m_lMessages.SetLen(Max(Index + 1, m_lMessages.Len()));
				m_lMessages[Index].Parse(_pKey->GetThisValue(), m_pWServer);
			}
		}

		else
			CWObject::OnEvalKey(_pKey);
	}

	virtual void OnSpawnWorld()
	{
		MAUTOSTRIP(CWObject_MessageBranch_OnSpawnWorld, MAUTOSTRIP_VOID);
		CWObject::OnSpawnWorld();
		
		for(int i = 0; i < m_lMessages.Len(); i++)
			m_lMessages[i].SendPrecache(m_iObject, m_pWServer);
	}

	virtual aint OnMessage(const CWObject_Message& _Msg)
	{
		MAUTOSTRIP(CWObject_MessageBranch_OnMessage, 0);
		if(_Msg.m_Msg == OBJMSG_IMPULSE && _Msg.m_Param0 > 0 && _Msg.m_Param0 < m_lMessages.Len())
		{
			m_lMessages[_Msg.m_Param0].SendMessage(m_iObject, _Msg.m_iSender, m_pWServer);
			return 1;
		}
		else
			return CWObject::OnMessage(_Msg);
	}
	
	virtual void OnRefresh()
	{
		MAUTOSTRIP(CWObject_MessageBranch_OnRefresh, MAUTOSTRIP_VOID);
		CWObject::OnRefresh();
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_MessageBranch, CWObject, 0x0100);
