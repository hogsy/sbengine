#include "PCH.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AutoVar.h"
#include "WObj_Trigger.h"

// -------------------------------------------------------------------
//  CWObject_MessagePipe
// -------------------------------------------------------------------
class CWObject_MessagePipe : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	int m_iType;
	TArray<CStr> m_lTargets;

	TArray<int16> m_liTargets;
	TArray<int8> m_liStates;
	int m_Index;

	enum
	{
		STATE_OPEN = 0,
		STATE_BLOCKED,
	};
	
	CWObject_MessagePipe()
	{
		MAUTOSTRIP(CWObject_MessagePipe_ctor, MAUTOSTRIP_VOID);
		m_iType = 0;
		m_Index = 0;
	}

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		MAUTOSTRIP(CWObject_MessagePipe_OnEvalKey, MAUTOSTRIP_VOID);
		switch (_KeyHash)
		{
		case MHASH1('TYPE'): // "TYPE"
			{
				static const char *FlagsTranslate[] =
				{
					"sequential", "random", NULL
				};
			
				m_iType = _pKey->GetThisValue().TranslateInt(FlagsTranslate);
				break;
			}

		case MHASH2('TARG','ETS'): // "TARGETS"
			{
				CStr Targets = _pKey->GetThisValue();
				while(Targets != "")
					m_lTargets.Add(m_pWServer->World_MangleTargetName(Targets.GetStrMSep(",; ")));
				break;
			}

		default:
			{
				CWObject::OnEvalKey(_pKey);
				break;
			}
		}
	}

	virtual void OnSpawnWorld()
	{
		MAUTOSTRIP(CWObject_MessagePipe_OnSpawnWorld, MAUTOSTRIP_VOID);
		CWObject::OnSpawnWorld();
		
		for(int t = 0; t < m_lTargets.Len(); t++)
		{
			TSelection<CSelection::LARGE_BUFFER> Selection;
			m_pWServer->Selection_AddTarget(Selection, m_lTargets[t]);
			int16* pSel = NULL;
			int nSel = m_pWServer->Selection_Get(Selection, (const int16**)&pSel);
			if(nSel > 0)
				m_liTargets.Insertx(m_liTargets.Len(), pSel, nSel);			
			m_pWServer->Selection_Pop();
		}
		m_liStates.SetLen(m_liTargets.Len());
		memset(m_liStates.GetBasePtr(), 0, m_liStates.Len() * sizeof(int8));
	}

	virtual aint OnMessage(const CWObject_Message& _Msg)
	{
		MAUTOSTRIP(CWObject_MessagePipe_OnMessage, 0);

		if(_Msg.m_Msg == OBJMSG_MESSAGEPIPE_SETSTATE)
		{
			int i;
			for(i = 0; i < m_liTargets.Len(); i++)
				if(m_liTargets[i] == _Msg.m_iSender)
				{
					m_liStates[i] = _Msg.m_Param0;
					break;
				}
			if(i == m_liTargets.Len())
				ConOut("§cf80WARNING: (CWObject_MessagePipe::OnMessage) Received a SetState message from an unknown sender");			
			return 1;
		}
		else if(m_liTargets.Len())
		{
			int iTarget = 0;
			if(m_iType == 0)
			{
				int i;
				for(i = 0; i < m_liTargets.Len(); i++)
				{
					while(m_Index >= m_liTargets.Len())
						m_Index -= m_liTargets.Len();
					
					iTarget = m_liTargets[m_Index];
					if(m_liStates[m_Index++] == STATE_OPEN)
						break;
				}
				if(i == m_liTargets.Len())
					ConOut("§cf80WARNING: (CWObject_MessagePipe::OnMessage) All targets was blocked. Message lost");
			}
			else if(m_iType == 1)
			{
				TStaticArray<int16, 256> lOpen;
				int nTargets = Min(m_liTargets.Len(), 255);
				for(int i = 0; i < nTargets; i++)
					if(m_liStates[i] == STATE_OPEN)
						lOpen.Add(m_liTargets[i]);

				if(lOpen.Len() == 0)
					ConOut("§cf80WARNING: (CWObject_MessagePipe::OnMessage) All targets was blocked. Message lost");
				else
					iTarget = lOpen[MRTC_RAND() % lOpen.Len()];
			}

			m_pWServer->Message_SendToObject(_Msg, iTarget);
		}
		return CWObject::OnMessage(_Msg);
	}
	
	virtual void OnRefresh()
	{
		MAUTOSTRIP(CWObject_MessagePipe_OnRefresh, MAUTOSTRIP_VOID);

		CWObject::OnRefresh();
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_MessagePipe, CWObject, 0x0100);
