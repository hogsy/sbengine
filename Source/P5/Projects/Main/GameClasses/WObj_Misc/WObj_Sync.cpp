/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_Sync.cpp

	Author:			Anton Ragnarsson

	Copyright:		Copyright Starbreeze AB 2006

	Contents:		CWObject_Sync

	History:		
		061003:		Created File
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Hook.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WPackets.h"



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Sync
|
| Used to synchronize several streamed sounds and script messages.
|
| When a sound is played (via 'PlaySound' or 'Speak' messages), an optional 'sync group'
| parameter can be added, which is the name of an CWObject_Sync.
| This is converted to a 32 bit hash and sent to all clients in the WPACKED_SOUND message.
| When the clients starts a sound with a specified sync id, they will add the created voice to
| a list for that sync id. When all voices in a sync group are ready to play the client
| will send a net msg to the sync group on server telling it's ready.
| When all clients are ready to go, the sync group object will tell the server to broadcast
| a sync message to the clients, and they will unpause the created voices in the specified group.
| The sync group object will also start sending all user specified timed messages.
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Sync : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	typedef CWObject_Attach::CTimedMessage CTimedMessage;

	// One flag for each client we're waiting for sync commands from (max 32)
	uint32 m_ClientsWaitMask;
	bool m_bWaitingForSync;

	// Waiting for impulse (set to -1 if not waiting)
	int m_Wait;

	// Reference time for sending timed messages 
	CMTime m_StartSendTime;
	uint m_iNextMessage;

	// These messages are sent as soon as a sync command has arrived from all clients
	TArray<CTimedMessage> m_lMessages;

public:
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		CStr KeyName = _pKey->GetThisName();
		if (KeyName.CompareSubStr("MSG_READY") == 0)
		{
			int iSlot = atoi(KeyName.Str() + 9);
			m_lMessages.SetMinLen(iSlot + 1);
			m_lMessages[iSlot].Parse(_pKey->GetThisValue(), m_pWServer);
		}
		else
		{
			CWObject::OnEvalKey(_KeyHash, _pKey);
		}
	}

	virtual void OnSpawnWorld()
	{
		CWObject::OnSpawnWorld();

		m_ClientsWaitMask = 0;
		m_bWaitingForSync = false;
		m_StartSendTime.MakeInvalid();
		m_iNextMessage = 0;
		m_Wait = -1;

		TAP<const CTimedMessage> pMessages = m_lMessages;
		for (uint i = 0; i < pMessages.Len(); i++)
			pMessages[i].SendPrecache(m_iObject, m_pWServer);

		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
	}

	virtual void OnRefresh()
	{
		CWObject::OnRefresh();

		if (!m_StartSendTime.IsInvalid())
			UpdateTimedMessages();
	}

	void UpdateTimedMessages()
	{
		fp32 CurrTime = (m_pWServer->GetGameTime() - m_StartSendTime).GetTime();

		TAP<const CTimedMessage> pMessages = m_lMessages;
		for (uint i = m_iNextMessage; i < pMessages.Len(); i++)
		{
			const CTimedMessage& Msg = pMessages[i];
			if (Msg.m_Time > CurrTime)
				break;

			Msg.SendMessage(m_iObject, 0, m_pWServer);
			m_iNextMessage = i + 1;
		}

		if (m_iNextMessage >= pMessages.Len())
		{
			m_StartSendTime.MakeInvalid();
			m_iNextMessage = 0;
			ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
		}
	}

	void CheckSendSync()
	{
		if (m_bWaitingForSync)
		{
			if (!m_ClientsWaitMask && m_Wait == -1)
			{
				m_bWaitingForSync = false;

		//		M_TRACE("[SYNC] clients are done, tell clients to start playing!\n");
				CNetMsg Msg(WPACKET_SOUNDSYNC);
				Msg.AddInt32( GetNameHash() );
				m_pWServer->Net_PutMsgAll(Msg);

				// Start sending timed messages
				m_StartSendTime = m_pWServer->GetGameTime();
				m_iNextMessage = 0;
				ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
				UpdateTimedMessages();
			}
		}
	}

	virtual aint OnMessage(const CWObject_Message& _Msg)
	{
		switch (_Msg.m_Msg)
		{
		case OBJSYSMSG_SOUNDSYNC:
			{
				if (_Msg.m_Param0 == 0)
				{
					// set list of clients to wait for
		//			M_TRACE("OBJSYSMSG_SOUNDSYNC, adding client to wait for: 0x%X\n", _Msg.m_Param1);
					m_ClientsWaitMask = _Msg.m_Param1;
					m_bWaitingForSync = true;
				}
				else if (_Msg.m_Param0 == 1)
				{
					// a client is done - remove from waiting list
					m_ClientsWaitMask &= ~_Msg.m_Param1;
		//			M_TRACE("OBJSYSMSG_SOUNDSYNC, got sync from client: 0x%X\n", _Msg.m_Param1);
					if (!m_ClientsWaitMask)
					{
		//				M_TRACE("OBJSYSMSG_SOUNDSYNC, all clients ready!\n");
						CheckSendSync();
					}
				}
			}
			return 0;

		// Start waiting for impulse
		case OBJMSG_HOOK_WAITIMPULSE:
			m_Wait = _Msg.m_Param0;
			return 1;

		// Check to see if impulse matches what we're waiting for, if so check if we should start playing
		case OBJMSG_IMPULSE:
			if (_Msg.m_Param0 == m_Wait)
			{
				m_Wait = -1;
				CheckSendSync();
				return 1;
			}
			return 0;

		default:
			return CWObject::OnMessage(_Msg);
		}
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Sync, CWObject, 0x0100);


