#ifndef __WOBJ_ANIMEVENTLISTENER_H
#define __WOBJ_ANIMEVENTLISTENER_H

#include "../WObj_Sys/WObj_Physical.h"
#include "../WObj_Messages.h"
#include "WObj_InputEntity.h"
//
#define CWObject_AnimEventListener_Parent CWObject
class CWObject_AnimEventListener : public CWObject_AnimEventListener_Parent,public CAttachSupport
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	enum
	{
		// Get message number.. (should be central...)
		OBJECT_ANIMEVENTLISTENER_MESSAGE_ONEVENT = OBJMSGBASE_MISC_ANIMEVENTLISTENER,
		OBJECT_ANIMEVENTLISTENER_MESSAGE_ATTACHME,

		// impulse 0 -> stop, Impulse 1 -> start/reset, impulse 2 -> destroy
	};
protected:
	class CEventMessage
	{
	public:
		CWO_SimpleMessage m_Msg;
		int32 m_EventType;

		CEventMessage()
		{
			m_EventType = -1;
		}

		CEventMessage(const CEventMessage& _Other)
		{
			*this = _Other;
		}

		void operator= (const CEventMessage& _Other)
		{
			m_Msg = _Other.m_Msg;
			m_EventType = _Other.m_EventType;
		}
	};
	// Messages, that react to different types of input..?
	TArray<CEventMessage>  m_lMessages;
	
	virtual void CheckMessages(int32 _EventType);
	//bool StartListening(bool _bStart);
public:
	virtual void OnCreate();
	virtual void OnRefresh();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnSpawnWorld();
	virtual aint OnMessage(const CWObject_Message &_Msg);

	virtual void OnDeltaSave(CCFile* _pFile);
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
};
#endif
