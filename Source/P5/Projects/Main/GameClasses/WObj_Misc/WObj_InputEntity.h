#ifndef __WOBJ_INPUTENTITY_H
#define __WOBJ_INPUTENTITY_H

#include "../WObj_Sys/WObj_Physical.h"
#include "../WObj_Messages.h"
//

class CAttachSupport
{
public:
	enum
	{
		ATTACHSUPPORT_DISABLED			= 1 << 11,
		ATTACHSUPPORT_GOTEXTRAOBJECTS	= 1 << 15,
	};
protected:
	
	int32		m_Flags;
	// Objects to attach to
	TArray<CStr> m_lAttachObjects;
	TArray<int16> m_liAttachObjects;

	void AddListenToIndex(CWorld_Server* _pWServer, int16 _iObject, int16 _Index, int32 _iMsg);
	void RemoveListenToIndex(CWorld_Server* _pWServer, int16 _iObject, int16 _Index, int32 _iMsg);
	bool StartListening(CWorld_Server* _pWServer, int16 _iObject, bool _bStart, int32 _iMsg, int16 _iSender = -1);
	void AttachSave(CCFile* _pFile);
	void AttachLoad(CCFile* _pFile);
};

#define CWObject_InputEntity_Parent CWObject
class CWObject_InputEntity : public CWObject_InputEntity_Parent, public CAttachSupport
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	enum
	{
		// If a button is pressed
		INPUTENTITY_INPUTTYPE_PRESS			= 1 << 0,
		// If a button has been released
		INPUTENTITY_INPUTTYPE_RELEASE		= 1 << 1,
		// If move stick has moved in vertical dir >= refval
		INPUTENTITY_INPUTTYPE_MOVEVERT		= 1 << 2,
		// If move stick has moved in Horiz dir >= refval
		INPUTENTITY_INPUTTYPE_MOVEHORIZ		= 1 << 3,
		// If Abs(move stick) has moved in vertical/horiz dir >= refval
		INPUTENTITY_INPUTTYPE_MOVEABS		= 1 << 4,
		// Move since last move input >= refval
		INPUTENTITY_INPUTTYPE_MOVEDELTA		= 1 << 5,

		// If look stick has moved in vertical dir >= refval
		INPUTENTITY_INPUTTYPE_LOOKVERT		= 1 << 6,
		// If look stick has moved in horiz dir >= refval
		INPUTENTITY_INPUTTYPE_LOOKHORIZ		= 1 << 7,
		// If Abs(look stick) has moved in vertical/horiz dir >= refval
		INPUTENTITY_INPUTTYPE_LOOKABS		= 1 << 8,
		
		// Move since last look input
		INPUTENTITY_INPUTTYPE_LOOKDELTA		= 1 << 9,

		// If the message should be sent only once
		INPUTENTITY_INPUTTYPE_ONCE			= 1 << 10,

		// Message currently disabled (needs to be saved...)
		
		INPUTENTITY_INPUTTYPE_FIRSTUPDATE		= 1 << 12,
		INPUTENTITY_INPUTTYPE_LOOKONCE			= 1 << 13,
		INPUTENTITY_INPUTTYPE_MOVEONCE			= 1 << 14,


		INPUTENTITY_MOVEMASK = (INPUTENTITY_INPUTTYPE_MOVEVERT | INPUTENTITY_INPUTTYPE_MOVEHORIZ),
		INPUTENTITY_LOOKMASK = (INPUTENTITY_INPUTTYPE_LOOKVERT | INPUTENTITY_INPUTTYPE_LOOKHORIZ),

		// Get message number.. (should be central...)
		OBJECT_INPUTENTITY_MESSAGE_RESET = OBJMSGBASE_MISC_INPUTENTITY,
		// Press, lastpress, move, look
		OBJECT_INPUTENTITY_MESSAGE_UPDATEINPUT,
		OBJECT_INPUTENTITY_MESSAGE_ATTACHME,

		// impulse 0 -> stop, Impulse 1 -> start/reset, impulse 2 -> destroy
	};
protected:
	class CInputMessage
	{
	public:
		CWO_SimpleMessage m_Msg;
		// If inputtype is movevert/horiz, move > totalmove
		CVec3Dfp32 m_Move;
		CVec3Dfp32 m_Look;
		int32 m_Input;
		int16 m_InputType;

		CInputMessage()
		{
			m_Move = 0.0f;
			m_Look = 0.0f;
			m_Input = 0;
			m_InputType = 0;
		}

		CInputMessage(const CInputMessage& _Other)
		{
			*this = _Other;
		}

		void operator= (const CInputMessage& _Other)
		{
			m_Msg = _Other.m_Msg;
			m_Move = _Other.m_Move;
			m_Look = _Other.m_Look;
			m_Input = _Other.m_Input;
			m_InputType = _Other.m_InputType;
		}
	};
	// Messages, that react to different types of input..?
	TArray<CInputMessage>  m_lMessages;
	

	CVec3Dfp32	m_Move;
	CVec3Dfp32	m_dMove;

	// Same restrictions as on player? ( +-0.245 in vert?)
	CVec3Dfp32	m_Look;
	CVec3Dfp32	m_dLook;
	CVec3Dfp32	m_LookStartOffset;

	int32		m_Control_Press;
	int32		m_Control_LastPress;
	fp32			m_LookThreshold;
	fp32			m_MoveThreshold;
	fp32			m_LookSampleTime;
	CMTime		m_LookStartTime;
	
	virtual void CheckMessages();
	virtual void Reset();
	void ResetLook();
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
