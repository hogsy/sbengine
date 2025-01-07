#ifndef _INC_AI_EVENTHANDLER
#define _INC_AI_EVENTHANDLER

#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"

//Helper class to hendle event triggering
class CAI_EventHandler
{
public:
	//Events
	enum{
		INVALID_EVENT = -1,
		MIN_EVENT = 0,

		ON_SPOT_ENEMY = MIN_EVENT,	//Trigger when the bot spots previously non-spotted enemies
		ON_HIT_UNINJURED,			//Triggers when the bot was hit but not hurt
		ON_INJURED,					//Trigger when the bot becomes injured
		ON_STUNNED,					//Trigger when the bot is stunned
		ON_DIE,						//Trigger when the bot dies
		ON_DETECT_DISTURBANCE,		//Trigger when the bot detects an enemy
		ON_SPAWN,					//Trigger when the bots AI runs for the first time (which is when it's spawned)
		ON_SURPRISED,				//Trigger when the bot is surprised, e.g. by the sudden appearance of an enemy
		ON_NOTICE_ANOMALY,			//Trigger when the bot notices an anomaly
		ON_SPOT_HOSTILE,			//Trigger when we spot a hostile, or raise hostiliy level
		ON_ALARM,					//Trigger when an enemy is spotted, when there's an alarm or when an enemy or potentially hostile player is detected
		ON_SPOT_FIGHT,				//Trigger when the bot notices a fight phenomenon
		ON_SPOT_PLAYER,				//Trigger when the bot spots the player
		ON_UNSPAWN,					//Triggered when unspawned			
		ON_END_DARKNESS,			//Triggered when Darkjackie stops using darkness

		MAX_EVENT,
	};

	//The "string-to-event-ID" map
	static const char* ms_EventStrID[];

private:
		struct SMsg
		{
			// Note we really should add an operator member and an IsValid call to
			// check the param
			uint32 m_Param;	// Param to check against, 0 means invalid
			CWO_SimpleMessage m_Msg;

			void Setup(CWO_SimpleMessage& _MSg, uint32 _Param = 0)
			{
				m_Param = _Param;
				m_Msg = _MSg;
			};
			bool IsValid(uint32 _Param)
			{
				if (m_Param)
				{
					return(m_Param == _Param);
				}
				else
				{
					return(true);
				}
			}
		};

	//The internal representation of an event, base class
	class CEvent
	{
	protected:
		//The AI
		CAI_Core * m_pAI;

		//The list of messages to send when the event is triggered
		TArray<SMsg> m_lMsgs;
		
		//Keeps track on whether the event has been triggered this frame or not.
		bool m_bIsTriggered;

		//Keep track of whether event is locked or not.
		bool m_bLocked;
		
		//Makes appropriate checks that will trigger event if it's fulfilled
		virtual void OnCheck();

	public:
		//Constructor and initializer
		CEvent();
		virtual void Init(CAI_Core * _pAI);

		//Has the event been triggered this frame? 
		bool IsTriggered();

		//Explicitly triggers the event if it's active and not already triggered
		//The given triggerer will be used as message's param1 if the message is 
		//a script impulse or as sender otherwise. Fails if previously triggered 
		//or locked.
		virtual bool Raise(int _iTriggerer, uint32 _Param);

		//Lock event so that it cannot be triggered again until it is released.
		void Lock();

		//Release locked event
		void Release();
		
		//Advances the event one frame and implicitly triggers it if appropriate
		//Does nothing if the event isn't active.
		void OnRefresh();

		//Clears messages
		void Clear();

		//Adds an event message 
		virtual void AddMessage(int _iPos, CWO_SimpleMessage _Msg, uint32 _Param);

		//Precache event messages
		void OnPrecacheMessages();

		//Change AI user
		void ReInit(CAI_Core * _pAI);

		//Most events are valid only if there are any messages to send, but some have 
		//other side-effects and may be valid even when there are no messages
		virtual bool IsValid();

#ifndef M_RTM
		//Get some debug info about the bot
		virtual CStr GetDebugString();
#endif
		friend class CAI_EventHandler;
	};

	//The event subclasses implement the Check method, and have suitable attributes
	class CEvent_OnSpotEnemy : public CEvent
	{
		//Triggers when the bot spots previously non-spotted enemies
		virtual void OnCheck();
	public:
		//Raising this event have some side-effects
		virtual bool Raise(int _iTriggerer, uint32 _Param);

		//Since this event sometimes must be implicitly raised, it's always valid unless locked
		virtual bool IsValid();
	};

	class CEvent_OnDetectDisturbance : public CEvent
	{
		//Triggers when the bot detects a disturbance
		virtual void OnCheck();
	public:
		//Raising this event have some side-effects
		virtual bool Raise(int _iTriggerer, uint32 _Param);

		//Since this event sometimes must be implicitly raised, it's always valid unless locked
		virtual bool IsValid();
	};

	class CEvent_OnSurprised : public CEvent
	{
		//Triggers when the bot is surprised
		virtual void OnCheck();

	public:
		//Raising this event have some side-effects
		virtual bool Raise(int _iTriggerer, uint32 _Param);

		//Since this event sometimes must be implicitly raised, it's always valid unless locked
		virtual bool IsValid();
	};

	class CEvent_OnNoticeAnomaly : public CEvent
	{
		//Triggers when the bot notices an anomaly
		virtual void OnCheck();
	public:
		//Raising this event have some side-effects
		virtual bool Raise(int _iTriggerer, uint32 _Param);

		//Since this event sometimes must be implicitly raised, it's always valid unless locked
		virtual bool IsValid();
	};

	class CEvent_OnSpotHostile : public CEvent
	{
		//Triggers when the bot spots a hostile or raises hostility level
		virtual void OnCheck();
	public:
		//Raising this event have some side-effects
		virtual bool Raise(int _iTriggerer, uint32 _Param);

		//Since this event sometimes must be implicitly raised, it's always valid unless locked
		virtual bool IsValid();
	};

	class CEvent_OnSpotFight : public CEvent
	{
	public:
		//Raising this event have some side-effects
		virtual bool Raise(int _iTriggerer, uint32 _Param = 0);
	};

	//The events to handle, and a list of pointers for convenience 
	//(I'm trying this out for efficiency reasons; the event objects will all be stack-allocated, and the pointer list is static once initialized)
	CEvent_OnSpotEnemy			m_OnSpotEnemy;
	CEvent						m_OnHitUninjured;
	CEvent						m_OnInjured;
	CEvent						m_OnStunned;
	CEvent						m_OnDie;
	CEvent_OnDetectDisturbance	m_OnDetectDisturbance;
	CEvent						m_OnSpawn;
	CEvent_OnSurprised			m_OnSurprised;	
	CEvent_OnNoticeAnomaly		m_OnNoticeAnomaly;
	CEvent_OnSpotHostile		m_OnSpotHostile;
	CEvent						m_OnAlarm;
	CEvent_OnSpotFight			m_OnSpotFight;
	CEvent						m_OnSpotPlayer;	
	CEvent						m_OnUnspawn;	
	CEvent						m_OnEndDarkness;	
	CEvent * m_lEvents[MAX_EVENT];

	//Pointer to the AI
	CAI_Core * m_pAI;

	//Checks if the given event id is valid
	bool IsValidEvent(int _iEvent);

public:
	//Constructor and initializer
	CAI_EventHandler();
	void Init(CAI_Core * _pAI = NULL);
	
	//Explicitly triggers the given event. 
	//This method should only be used for triggering events  under special circumstances.
	void RaiseEvent(int _iEvent,int _iTriggerer,uint32 _Param = 0);

	//Clears the messages for the given event, or for all events if no argument is given
	void ClearEvent(int _iEvent = INVALID_EVENT);

	//Locks/releases the given event
	void LockEvent(int _iEvent);
	void ReleaseEvent(int _iEvent);

	//Adds given message to given event
	void AddEventMessage(int _iEvent, int _iPos, const CWO_SimpleMessage& _Msg, uint32 _Param = 0);

	//Adds given message to given event, where the event is identified in the name of the 
	//key and the message and target is contained in the value string
	void AddEventMessage(const CRegistry* _pKey);

	//Advances the event handler one frame, triggering any fulfilled events.
	//The RaiseEvent method can be used to complement this method when an event
	//should be raised in special circumstances.
	void OnRefresh();

	//Precaches all event messages
	void OnPrecacheMessages();

	//Change AI user
	void ReInit(CAI_Core * _pAI);

	//Savegame
	void OnDeltaLoad(CCFile* _pFile);
	void OnDeltaSave(CCFile* _pFile);

#ifndef M_RTM
	//Get some debug info about the bot
	virtual CStr GetDebugString();
#endif
}; 


#endif
