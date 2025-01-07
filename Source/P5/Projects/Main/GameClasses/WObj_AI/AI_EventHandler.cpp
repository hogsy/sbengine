#include "PCH.h"
#include "AICore.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
#include "WObj_Aux/WObj_Team.h"
#include "../WObj_CharMsg.h"


//Event handler & Event

//The "string-to-event-ID" map
const char* CAI_EventHandler::ms_EventStrID[] =
{
	"ON_ENEMY_SPOTTED",
	"ON_HIT_UNINJURED",
	"ON_INJURED",
	"ON_STUNNED",
	"ON_DIE",
	"ON_DETECT_DISTURBANCE",
	"ON_SPAWN",
	"ON_SURPRISED",
	"ON_NOTICE_ANOMALY",
	"ON_SPOT_HOSTILE",
	"ON_ALARM",
	"ON_SPOT_FIGHT",
	"ON_SPOT_PLAYER",
	"ON_UNSPAWN",		
	"ON_END_DARKNESS",			

	NULL,
};



//Makes appropriate checks that will trigger event if it's fulfilled
void CAI_EventHandler::CEvent::OnCheck()
{
	//By default events can only be triggered explicitly, which means this method shouldn't do anything
};

CAI_EventHandler::CEvent::CEvent()
{
	MAUTOSTRIP(CAI_EventHandler_CEvent_ctor, MAUTOSTRIP_VOID);
	m_lMsgs.Clear();
	m_bIsTriggered = false;
	m_bLocked = false;
};

void CAI_EventHandler::CEvent::Init(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_EventHandler_CEvent_Init, MAUTOSTRIP_VOID);
	m_pAI = _pAI;
};


//Has the event been triggered this frame? 
bool CAI_EventHandler::CEvent::IsTriggered()
{
	MAUTOSTRIP(CAI_EventHandler_CEvent_IsTriggered, false);
	return m_bIsTriggered;
};

//Explicitly triggers the event if it's active not already triggered
//The given triggerer will be used as message's param1 if the message is 
//a script impulse or as sender otherwise.
bool CAI_EventHandler::CEvent::Raise(int _iTriggerer, uint32 _Param)
{
	MAUTOSTRIP(CAI_EventHandler_CEvent_Raise, MAUTOSTRIP_VOID);
	if (!m_pAI ||
		!m_pAI->IsValid())
		//Invalid AI, abort
		return false;

	if (!m_bIsTriggered && !m_bLocked)
	{
		//Trigger event
		m_bIsTriggered = true;

		//If there is no triggerer, use self
		if (_iTriggerer == 0)
			_iTriggerer = m_pAI->m_pGameObject->m_iObject;
		
		//Send event messages
		for (int i = 0; i < m_lMsgs.Len(); i++)
		{
			if (m_lMsgs[i].IsValid(_Param))
			{
				m_lMsgs[i].m_Msg.SendMessage(m_pAI->m_pGameObject->m_iObject, _iTriggerer, m_pAI->m_pServer);
			}
		};
		return true;
	}
	else
		return false;
};

//Lock event so that it cannot be triggered again until it is released.
void CAI_EventHandler::CEvent::Lock()
{
	m_bLocked = true;
};

//Release locked event
void CAI_EventHandler::CEvent::Release()
{
	m_bLocked = false;
};


//Advances the event one frame and implicitly triggers it if appropriate
//Does nothing if the event isn't active.
void CAI_EventHandler::CEvent::OnRefresh()
{
	//MSCOPESHORT(CAI_EventHandler::CEvent::OnRefresh);
	MAUTOSTRIP(CAI_EventHandler_CEvent_OnRefresh, MAUTOSTRIP_VOID);
	if (!m_pAI ||
		!m_pAI->IsValid())
		//Invalid AI, abort
		return;

	m_bIsTriggered = false;

	//Check event if valid
	if (IsValid())
	{
		OnCheck();
	};
};

//Clears messages
void CAI_EventHandler::CEvent::Clear()
{
	MAUTOSTRIP(CAI_EventHandler_CEvent_Clear, MAUTOSTRIP_VOID);
	m_lMsgs.Clear();
};

//Adds a message 
void CAI_EventHandler::CEvent::AddMessage(int _iPos, CWO_SimpleMessage _Msg, uint32 _Param)
{
	MAUTOSTRIP(CAI_EventHandler_CEvent_AddMessage, MAUTOSTRIP_VOID);
	if (_iPos >= 0)
	{
		if (m_lMsgs.Len() <= _iPos)
		{
			m_lMsgs.SetMinLen(_iPos + 1);
		}
		m_lMsgs[_iPos].Setup(_Msg,_Param);
	}
	else
	{
		SMsg MsgStruct;
		MsgStruct.Setup(_Msg,_Param);
		m_lMsgs.Add(MsgStruct);
	}
};

//Precache event messages
void CAI_EventHandler::CEvent::OnPrecacheMessages()
{
	MAUTOSTRIP(CAI_EventHandler_CEvent_OnPrecacheMessages, MAUTOSTRIP_VOID);
	for (int i = 0; i < m_lMsgs.Len(); i++)
	{
		m_lMsgs[i].m_Msg.SendPrecache(m_pAI->m_pGameObject->m_iObject, m_pAI->m_pServer);
	};
};

//Change AI user
void CAI_EventHandler::CEvent::ReInit(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_EventHandler_CEvent_ReInit, MAUTOSTRIP_VOID);
	m_pAI = _pAI;
};

//Most events are valid only if there are any messages to send, but some have 
//other side-effects and may be valid even when there are no messages
bool CAI_EventHandler::CEvent::IsValid()
{
	//Events are by default only valid if they're not locked and have messages
	return !m_bLocked && (m_lMsgs.Len() > 0);
};


#ifndef M_RTM
//Get some debug info about the bot
CStr CAI_EventHandler::CEvent::GetDebugString()
{
	MAUTOSTRIP(CAI_EventHandler_CEvent_GetDebugString, CStr());
	if (m_lMsgs.Len() == 0)
		return CStr("");
	else
	{
		CStr str = "";
		for (int i = 0; i < m_lMsgs.Len(); i++)
		{
			str += m_lMsgs[i].m_Msg.GetDesc() + " ";
		};
		if (m_bIsTriggered)
			str = "Triggered " + str;
		else
			str = "Not triggered " + str;
		return str;
	}

};
#endif


//Triggers when the bot spots previously non-spotted enemies
void CAI_EventHandler::CEvent_OnSpotEnemy::OnCheck()
{
	if (!m_pAI)
		return;

	//Update awareness of potential enemies
	m_pAI->m_KB.OnSpotEnemies();
};

//Raising this event have some side-effects
bool CAI_EventHandler::CEvent_OnSpotEnemy::Raise(int _iTriggerer, uint32 _Param)
{
	if (!m_pAI)
		return false;

	//Send event messages and mark as triggered
	bool bRaise = CEvent::Raise(_iTriggerer,_Param);

	//Side-effects. Note that this must be done _after_ parent raise to eliminate 
	//the risk of infinite recursion due to these side-effects
	if (bRaise)
	{
		//Send a combat priority override to action handler
		m_pAI->m_AH.RequestOverride(CAI_Action::PRIO_COMBAT);

		//Once we have spotted enemy, we're never surprised
		m_pAI->m_EventHandler.LockEvent(ON_SURPRISED);
		
		//When we spot an enemy, alertness is automatically raised to watchful if previously lower
		if (m_pAI->m_KB.GetAlertness() < CAI_KnowledgeBase::ALERTNESS_WATCHFUL)
		{
			m_pAI->m_KB.SetAlertness(CAI_KnowledgeBase::ALERTNESS_WATCHFUL);
		}

		m_pAI->RaiseAlarm(_iTriggerer,CAI_AgentInfo::ENEMY,true);
	
		//Raise OnAlarm event
		m_pAI->m_EventHandler.RaiseEvent(ON_ALARM, _iTriggerer, _Param);
	}
	
	return bRaise;
};


//Since this event sometimes must be implicitly raised, it's always valid unless locked
bool CAI_EventHandler::CEvent_OnSpotEnemy::IsValid()
{
	return !m_bLocked;
};



//Triggers when the bot detects previously non-detected enemies
void CAI_EventHandler::CEvent_OnDetectDisturbance::OnCheck()
{
	if (!m_pAI)
		return;

	//Update awareness of potential enemies (who might give rise to disturbances)
	m_pAI->m_KB.OnDetectEnemies();
};

//Raising this event have some side-effects
bool CAI_EventHandler::CEvent_OnDetectDisturbance::Raise(int _iTriggerer, uint32 _Param)
{
	if (!m_pAI || !m_pAI->m_pServer || !m_pAI->m_pGameObject)
		return false;

	//Send event messages and mark as triggered
	bool bRaise = CEvent::Raise(_iTriggerer,_Param);

	//Side-effects. Note that this must be done _after_ parent raise to eliminate 
	//the risk of infinite recursion due to these side-effects
	if (bRaise)
	{
		//Send an alert priority override to action handler
		m_pAI->m_AH.RequestOverride(CAI_Action::PRIO_ALERT);

		//Detect threat...fix proper...
		CWObject* pObj = m_pAI->m_pServer->Object_Get(_iTriggerer);
		if (pObj)
		{
			//Object related disturbance
			//Raise OnAlarm event if we've detected a hostile
			if (m_pAI->m_KB.DefaultRelation(_iTriggerer) >= CAI_AgentInfo::HOSTILE)
			{
				m_pAI->m_EventHandler.RaiseEvent(ON_ALARM,_iTriggerer);
			}
		}
	}
	
	return bRaise;
};


//Since this event sometimes must be implicitly raised, it's always valid unless locked
bool CAI_EventHandler::CEvent_OnDetectDisturbance::IsValid()
{
	return !m_bLocked;
};


//Triggers when the bot is surprised by something
void CAI_EventHandler::CEvent_OnSurprised::OnCheck()
{
	if (!m_pAI)
		return;

	//Bot can get surprised by spotting enemies, so spotting info should be updated
	m_pAI->m_KB.OnSpotEnemies();

	//Otherwise, any surprises should be explicitly triggered
};

//Raising this event have some side-effects
bool CAI_EventHandler::CEvent_OnSurprised::Raise(int _iTriggerer,uint32 _Param)
{
	if (!m_pAI)
		return false;

	//Send event messages and mark as triggered
	bool bRaise = CEvent::Raise(_iTriggerer,_Param);

	//Side-effects. Note that this must be done _after_ parent raise to eliminate 
	//the risk of infinite recursion due to these side-effects
	if (bRaise)
	{
		// Play some kind of surprised noise?
		// Once event has been properly raised, we're never surprised again unless event is explicitly released
		Lock();
	}

	return bRaise;
};

//Since this event sometimes must be implicitly raised, it's always valid unless locked
bool CAI_EventHandler::CEvent_OnSurprised::IsValid()
{
	return !m_bLocked;
};


//Triggers when the bot noticed previously unnoticed anomalies
void CAI_EventHandler::CEvent_OnNoticeAnomaly::OnCheck()
{
	if (!m_pAI)
		return;

	// *** NOTE: Report distraction related NOTICE ***
	//Update awareness of potential enemies (who might give rise to anomalies)
	m_pAI->m_KB.OnDetectEnemies();
	// ***
};

//Raising this event have some side-effects
bool CAI_EventHandler::CEvent_OnNoticeAnomaly::Raise(int _iTriggerer,uint32 _Param)
{
	if (!m_pAI)
		return false;

	//Send event messages and mark as triggered
	bool bRaise = CEvent::Raise(_iTriggerer,_Param);

	//Side-effects. Note that this must be done _after_ parent raise to eliminate 
	//the risk of infinite recursion due to these side-effects
	if (bRaise)
	{
		//Send an alert priority override to action handler
		m_pAI->m_AH.RequestOverride(CAI_Action::PRIO_ALERT);
	}
	
	return bRaise;
};


//Since this event sometimes must be implicitly raised, it's always valid unless locked
bool CAI_EventHandler::CEvent_OnNoticeAnomaly::IsValid()
{
	return !m_bLocked;
};


//Triggers when the bot spots a hostile or raises hostility level
void CAI_EventHandler::CEvent_OnSpotHostile::OnCheck()
{
	if (!m_pAI)
		return;

	//Update awareness of potential enemies
	m_pAI->m_KB.OnSpotEnemies();
};

//Raising this event have some side-effects
bool CAI_EventHandler::CEvent_OnSpotHostile::Raise(int _iTriggerer,uint32 _Param)
{
	if (!m_pAI)
		return false;

	//Send event messages and mark as triggered
	bool bRaise = CEvent::Raise(_iTriggerer,_Param);

	//Side-effects. Note that this must be done _after_ parent raise to eliminate 
	//the risk of infinite recursion due to these side-effects
	if (bRaise)
	{
		//Send a danger priority override to action handler
		m_pAI->m_AH.RequestOverride(CAI_Action::PRIO_DANGER);

		//New hostile spotted
		m_pAI->m_iHostile = _iTriggerer;
		m_pAI->m_bNewHostility = true;

		//Raise OnAlarm event
		// We really can't raise the alarm here as we've only spotted a hostile
		// m_pAI->m_EventHandler.RaiseEvent(ON_ALARM, _iTriggerer);
	}
	
	return bRaise;
};


//Since this event sometimes must be implicitly raised, it's always valid unless locked
bool CAI_EventHandler::CEvent_OnSpotHostile::IsValid()
{
	return !m_bLocked;
};

//Raising this event have some side-effects
bool CAI_EventHandler::CEvent_OnSpotFight::Raise(int _iTriggerer,uint32 _Param)
{
	if (!m_pAI)
		return false;

	//Send event messages and mark as triggered
	bool bRaise = CEvent::Raise(_iTriggerer,_Param);

	//Side-effects. Note that this must be done _after_ parent raise to eliminate 
	//the risk of infinite recursion due to these side-effects
	if (bRaise)
	{
		//Fight spotted, increase alertness to watchful and spot fighter
		// *** Should I say DIALOGUE_IDLE_ALARM_FIGHT? m_pAI->SingleNoise("Spotted fight",CAI_Device_Sound::SPOT);

		if (m_pAI->m_KB.GetAlertness() < CAI_KnowledgeBase::ALERTNESS_WATCHFUL)
			m_pAI->m_KB.SetAlertness(CAI_KnowledgeBase::ALERTNESS_WATCHFUL);

		CAI_AgentInfo * pInfo = m_pAI->m_KB.GetAgentInfo(_iTriggerer);
		if (pInfo && (pInfo->GetAwareness() < CAI_AgentInfo::SPOTTED))
		{
			pInfo->SetAwareness(CAI_AgentInfo::SPOTTED,true);
			pInfo->SetCorrectSuspectedPosition();
		}

		//Raise alarm event if we're security sensitive (i.e. will care about fights)
		if (m_pAI->m_SecurityTolerance >= 0)
			m_pAI->m_EventHandler.RaiseEvent(ON_ALARM, _iTriggerer);
	}
	
	return bRaise;
};

//Checks that the given event id is valid
bool CAI_EventHandler::IsValidEvent(int _iEvent)
{
	MAUTOSTRIP(CAI_EventHandler_IsValidEvent, false);
	return ((_iEvent >= MIN_EVENT) &&
		    (_iEvent < MAX_EVENT));
};


//Constructor
CAI_EventHandler::CAI_EventHandler()
{
	MAUTOSTRIP(CAI_EventHandler_ctor, MAUTOSTRIP_VOID);
};

//Initializer
void CAI_EventHandler::Init(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_EventHandler_Init, MAUTOSTRIP_VOID);
	m_pAI = _pAI;

	//Initialize events and events list
	ReInit(_pAI);

	for (int i = MIN_EVENT; i < MAX_EVENT; i++)
	{
		m_lEvents[i]->Init(m_pAI);

		//Clear all events and refresh them once
		m_lEvents[i]->Clear();
		m_lEvents[i]->OnRefresh();
	}
};


//Explicitly triggers the given event. If the object ID of the object which caused the 
//event to be raised is specified, any impulses which lack an object parameter will use 
//this object as parameter.
//This method should only be used for triggering events  under special circumstances.
void CAI_EventHandler::RaiseEvent(int _iEvent, int _iTriggerer, uint32 _Param)
{
	MAUTOSTRIP(CAI_EventHandler_RaiseEvent, MAUTOSTRIP_VOID);
	//Check for AI  and argument validity
	if (!m_pAI ||
		!IsValidEvent(_iEvent) ||
		!m_pAI->IsValid() )
		//Something's not right, abort.
		return;

	//Raise this event, and report this to action handler if actually raised
	if (m_lEvents[_iEvent]->Raise(_iTriggerer,_Param))
		m_pAI->m_AH.ReportEvent(_iEvent,_iTriggerer,_Param);
};


//Clears the impulses for the given event, or for all events if no argument is given
void CAI_EventHandler::ClearEvent(int _iEvent)
{
	MAUTOSTRIP(CAI_EventHandler_ClearEvent, MAUTOSTRIP_VOID);
	if (IsValidEvent(_iEvent))
		m_lEvents[_iEvent]->Clear();
	else
		for (int i = 0; i < MAX_EVENT; i++)
			m_lEvents[i]->Clear();
};


//Locks the given event
void CAI_EventHandler::LockEvent(int _iEvent)
{
	if (IsValidEvent(_iEvent))
	{
		m_lEvents[_iEvent]->Lock();
	}
};

//Releases the given event
void CAI_EventHandler::ReleaseEvent(int _iEvent)
{
	if (IsValidEvent(_iEvent))
		m_lEvents[_iEvent]->Release();
};


//Adds given message to given event
void CAI_EventHandler::AddEventMessage(int _iEvent, int _iPos, const CWO_SimpleMessage& _Msg, uint32 _Param)
{
	MAUTOSTRIP(CAI_EventHandler_AddEventMessage, MAUTOSTRIP_VOID);
	if (IsValidEvent(_iEvent))
	{
		m_lEvents[_iEvent]->AddMessage(_iPos,_Msg, _Param);		
	};
};


//Adds given message to given event, where the event is identified in the name of the 
//key and the message and target is contained in the value string
void CAI_EventHandler::AddEventMessage(const CRegistry* _pKey)
{
	MAUTOSTRIP(CAI_EventHandler_AddEventMessage_2, MAUTOSTRIP_VOID);
	if (!m_pAI ||
		!m_pAI->m_pServer ||
		!_pKey || 
		(_pKey->GetThisName().Len() < 10) || 
		(_pKey->GetThisValue().Len() < 1) )
		return;

	int iEvent = INVALID_EVENT;
	CStr sEvent = _pKey->GetThisName().Right(_pKey->GetThisName().Len() - 9);
	CStr sNbr = sEvent.Right(1);
	int iPos = sNbr.Val_int();
	sEvent = sEvent.Del(sEvent.Len()-1,1);
	for (int i = MIN_EVENT; i < MAX_EVENT; i++)
	{
		if (sEvent.CompareSubStr(ms_EventStrID[i]) == 0)
		{
			iEvent = i;
			break;
		};
	};

	//Parse simple message
	CWO_SimpleMessage Msg;
	uint32 Param = 0;
	CStr SValue = _pKey->GetThisValue();
	if (SValue.CompareSubStr("0x") != 0)
	{
		Param = SValue.GetStrSep(";").Val_int();
	}
	Msg.Parse(SValue, m_pAI->m_pServer);

	//Add event message
	AddEventMessage(iEvent,iPos,Msg,Param);
};


//Advances the event handler one frame, triggering any fulfilled events.
//The RaiseEvent method can be used to complement this method when an event
//should be raised in special circumstances.
void CAI_EventHandler::OnRefresh()
{ 
	MAUTOSTRIP(CAI_EventHandler_OnRefresh, MAUTOSTRIP_VOID);
	//Refresh events
	for (int i = 0; i < MAX_EVENT; i++)
	{
		m_lEvents[i]->OnRefresh();
	};
};


//Precaches all event messages
void CAI_EventHandler::OnPrecacheMessages()
{
	MAUTOSTRIP(CAI_EventHandler_OnPrecacheMessages, MAUTOSTRIP_VOID);
	if (!m_pAI || !m_pAI->m_pServer || !m_pAI->m_pGameObject)
		return;

	for (int i = 0; i < MAX_EVENT; i++)
		m_lEvents[i]->OnPrecacheMessages();
};

//Change AI user
void CAI_EventHandler::ReInit(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_EventHandler_ReInit, MAUTOSTRIP_VOID);
	m_pAI = _pAI;

	//Re-initialize events and events list
	m_lEvents[ON_SPOT_ENEMY] = &m_OnSpotEnemy;
	m_lEvents[ON_HIT_UNINJURED] = &m_OnHitUninjured;
	m_lEvents[ON_INJURED] = &m_OnInjured;
	m_lEvents[ON_STUNNED] = &m_OnStunned;
	m_lEvents[ON_DIE] = &m_OnDie;
	m_lEvents[ON_DETECT_DISTURBANCE] = &m_OnDetectDisturbance;
	m_lEvents[ON_SPAWN] = &m_OnSpawn;
	m_lEvents[ON_SURPRISED] = &m_OnSurprised;
	m_lEvents[ON_NOTICE_ANOMALY] = &m_OnNoticeAnomaly;
	m_lEvents[ON_SPOT_HOSTILE] = &m_OnSpotHostile;
	m_lEvents[ON_ALARM] = &m_OnAlarm;
	m_lEvents[ON_SPOT_FIGHT] = &m_OnSpotFight;
	m_lEvents[ON_SPOT_PLAYER] = &m_OnSpotPlayer;
	m_lEvents[ON_UNSPAWN] = &m_OnUnspawn;
	m_lEvents[ON_END_DARKNESS] = &m_OnEndDarkness;
	for (int i = 0; i < MAX_EVENT; i++)
		m_lEvents[i]->ReInit(_pAI);
};

void CAI_EventHandler::OnDeltaLoad(CCFile* _pFile)
{
	M_ASSERT(MAX_EVENT <= 32, "CAI_EventHandler::OnDeltaLoad bitfield depleted");	// Because we save the events in a 32bit data

	uint32 EventField;
	_pFile->ReadLE(EventField);

	for (int iEvent=0; iEvent<MAX_EVENT; iEvent++)
	{
		if (EventField & (1 << iEvent))
		{
			m_lEvents[iEvent]->Lock();
		}
		else
			m_lEvents[iEvent]->Release();
	};
};

void CAI_EventHandler::OnDeltaSave(CCFile* _pFile)
{
	M_ASSERT(MAX_EVENT <= 32, "CAI_EventHandler::OnDeltaLoad bitfield depleted");	// Because we save the events in a 32bit data

	uint32 EventField = 0;
	for (int iEvent=0; iEvent<MAX_EVENT; iEvent++)
	{
		if (m_lEvents[iEvent]->m_bLocked)
			EventField |= (1 << iEvent);
	};

	_pFile->WriteLE(EventField);
};

#ifndef M_RTM
//Get some debug info about the bot
CStr CAI_EventHandler::GetDebugString()
{
	MAUTOSTRIP(CAI_EventHandler_GetDebugString, CStr());
	CStr str = "";
	for (int i = MIN_EVENT; i < MAX_EVENT; i++)
	{
		CStr sEvent = ms_EventStrID[i];
		CStr sMsgs = m_lEvents[i]->GetDebugString();
		if ((sEvent != CStr("")) &&
			(sMsgs != CStr("")))
			str += sEvent + sMsgs;	
	};

	if (str == CStr(""))
		return str;
	else
		return "Events: " + str;
};
#endif
