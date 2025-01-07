#include "PCH.h"
#include "WObj_InputEntity.h"
#include "../../GameWorld/WClientMod_Defines.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_InputEntity, CWObject_InputEntity_Parent, 0x0100);

void CWObject_InputEntity::OnCreate()
{
	CWObject_InputEntity_Parent::OnCreate();
	m_Control_Press = 0;
	m_Control_LastPress = 0;

	m_Move = 0.0f;
	m_dMove = 0.0f;

	// Same restrictions as on player? ( +-0.245 in vert?)
	m_Look = 0.0f;
	m_dLook = 0.0f;
	m_LookStartOffset = 0.0f;
	m_LookThreshold = 0.1f;
	m_MoveThreshold = 0.5f;
	m_LookSampleTime = 0.0f;
	m_LookStartTime.Reset();
	m_Flags = 0;
}
/*

CONTROLBITS_PRIMARY = 1,
CONTROLBITS_SECONDARY = 2,
CONTROLBITS_JUMP = 4,
CONTROLBITS_CROUCH = 8,

CONTROLBITS_BUTTON0 = 16,
CONTROLBITS_BUTTON1 = 32,
CONTROLBITS_BUTTON2 = 64,
CONTROLBITS_BUTTON3 = 128,

CONTROLBITS_DPAD_UP    = 256,
CONTROLBITS_DPAD_DOWN  = 512,
CONTROLBITS_DPAD_LEFT  = 1024,
CONTROLBITS_DPAD_RIGHT = 2048,

CONTROLBITS_BUTTON4 = 4096,
CONTROLBITS_BUTTON5 = 8192,
*/
void CWObject_InputEntity::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	static const char *FlagsTranslate[] =
	{
		"press","release","movevert","movehoriz","moveabs","movedelta",
		"lookvert","lookhoriz","lookabs","lookdelta","once","disabled",
		"firstupdate","lookonce","moveonce",NULL
	};

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyVal = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH2('FLAG','S'): // "FLAGS"
		{
			m_Flags = KeyVal.TranslateFlags(FlagsTranslate);
			break;
		}
	case MHASH4('LOOK','SAMP','LETI','ME'): // "LOOKSAMPLETIME"
		{
			m_LookSampleTime = KeyVal.Val_fp64();
			break;
		}
	default:
		{
			const static struct { const char* m_pName; uint m_InputType; uint m_Input; fp32 lx,ly, mx,my; } s_lKeys[] = 
			{
				{ "ONPRIMARY",			INPUTENTITY_INPUTTYPE_PRESS,		CONTROLBITS_PRIMARY		},	// Primary attack
				{ "ONRELEASEPRIMARY",	INPUTENTITY_INPUTTYPE_RELEASE,		CONTROLBITS_PRIMARY		},	// Primary attack
				{ "ONSECONDARY",		INPUTENTITY_INPUTTYPE_PRESS,		CONTROLBITS_SECONDARY	},	// Secondary attack
				{ "ONRELEASESECONDARY",	INPUTENTITY_INPUTTYPE_RELEASE,		CONTROLBITS_SECONDARY	},	// Secondary attack
				{ "ONJUMP",				INPUTENTITY_INPUTTYPE_PRESS,		CONTROLBITS_JUMP		},	// Jump button
				{ "ONCROUCH",			INPUTENTITY_INPUTTYPE_PRESS,		CONTROLBITS_CROUCH		},	// Crouch button
				{ "ONUSE",				INPUTENTITY_INPUTTYPE_PRESS,		CONTROLBITS_BUTTON0		},	// Button0
				{ "ONWEAPONSELECT",		INPUTENTITY_INPUTTYPE_PRESS,		CONTROLBITS_DPAD_LEFT | CONTROLBITS_DPAD_RIGHT|CONTROLBITS_DPAD_DOWN		},	// Can switch weapon with both DPadLeft and Right 
				{ "ONDARKNESS",			INPUTENTITY_INPUTTYPE_PRESS,		CONTROLBITS_BUTTON2		},	// Button2
				{ "ONRELOAD",			INPUTENTITY_INPUTTYPE_PRESS,		CONTROLBITS_BUTTON3		},	// Button3
				{ "ONMISSIONJOURNAL",	INPUTENTITY_INPUTTYPE_PRESS,		CONTROLBITS_BUTTON4		},	// Button4
				{ "ONLOOKUP",			INPUTENTITY_INPUTTYPE_LOOKVERT,  0,	 0.0f,  1.0f,		0,0 },	// Look[1] >  threshold (threshold is set in onfinishevalkeys)
				{ "ONLOOKDOWN",			INPUTENTITY_INPUTTYPE_LOOKVERT,	 0,	 0.0f, -1.0f,		0,0 },	// Look[1] < -threshold
				{ "ONLOOKLEFT",			INPUTENTITY_INPUTTYPE_LOOKHORIZ, 0,	-1.0f,  0.0f,		0,0 },	// Look[2] < -threshold
				{ "ONLOOKRIGHT",		INPUTENTITY_INPUTTYPE_LOOKHORIZ, 0,	 1.0f,  0.0f,		0,0 },	// Look[2] >  threshold
				{ "ONMOVEFWD",			INPUTENTITY_INPUTTYPE_MOVEVERT,  0, 0,0,	   0.0f,  1.0f	},	// Move[0] >  threshold
				{ "ONMOVEBWD",			INPUTENTITY_INPUTTYPE_MOVEVERT,  0, 0,0,	   0.0f, -1.0f	},	// Move[0] < -threshold
				{ "ONMOVELEFT",			INPUTENTITY_INPUTTYPE_MOVEHORIZ, 0, 0,0,	  -1.0f,  0.0f	},	// Move[1] < -threshold
				{ "ONMOVERIGHT",		INPUTENTITY_INPUTTYPE_MOVEHORIZ, 0, 0,0,	   1.0f,  0.0f	},	// Move[1] >  threshold
			};
			const uint nKeys = sizeof(s_lKeys) / sizeof(s_lKeys[0]);

			for (uint i = 0; i < nKeys; i++)
			{
				const char* pName = s_lKeys[i].m_pName;
				if (KeyName.CompareSubStr(pName) == 0)
				{
					CInputMessage Msg;
					Msg.m_Msg.Parse(KeyVal, m_pWServer);
					Msg.m_InputType = s_lKeys[i].m_InputType;
					Msg.m_Input =     s_lKeys[i].m_Input;
					Msg.m_Look.k[1] = s_lKeys[i].ly;
					Msg.m_Look.k[2] = s_lKeys[i].lx;
					Msg.m_Move.k[0] = s_lKeys[i].my;
					Msg.m_Move.k[1] = s_lKeys[i].mx;
					m_lMessages.Add(Msg);
					return;
				}
			}

			if (KeyName.CompareSubStr("LOOKTHRESHOLD") == 0)
			{
				// Remeber that vert is -0.245 <-> 0.245 or something like that..
				m_LookThreshold = KeyVal.Val_fp64();
			}
			else if (KeyName.CompareSubStr("MOVETHRESHOLD") == 0)
			{
				m_MoveThreshold = KeyVal.Val_fp64();
			}
			else if (KeyName.CompareSubStr("LISTENTO") == 0)
			{
				// Which character we should listen to
				m_lAttachObjects.Add(KeyVal);
			}
			else
				CWObject_InputEntity_Parent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_InputEntity::OnFinishEvalKeys()
{
	CWObject_InputEntity_Parent::OnFinishEvalKeys();

	// Go through all messages, check if they should have extra flags (like once)
	// and thresholds set
	int32 NumMessages = m_lMessages.Len();
	CInputMessage* pMessage = m_lMessages.GetBasePtr();

	int16 ExtraFlags = m_Flags & INPUTENTITY_INPUTTYPE_ONCE;
	for (int32 i = 0; i < NumMessages; i++)
	{
		pMessage[i].m_InputType |= ExtraFlags;
		if (pMessage[i].m_InputType & INPUTENTITY_MOVEMASK)
		{
			pMessage[i].m_Move *= m_MoveThreshold;
			pMessage[i].m_InputType |= m_Flags & INPUTENTITY_INPUTTYPE_MOVEONCE;
		}

		if (pMessage[i].m_InputType & INPUTENTITY_LOOKMASK)
		{
			pMessage[i].m_Look *= m_LookThreshold;
			pMessage[i].m_InputType |= m_Flags & INPUTENTITY_INPUTTYPE_LOOKONCE;
		}
	}
}


void CWObject_InputEntity::OnRefresh()
{
	CWObject_InputEntity_Parent::OnRefresh();

	// Until we have successfully started listening, keep bugging
	if ((m_Flags & ATTACHSUPPORT_DISABLED) || StartListening(m_pWServer, m_iObject, 1,OBJECT_INPUTENTITY_MESSAGE_ATTACHME))
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
}

void CWObject_InputEntity::OnSpawnWorld()
{
	CWObject_InputEntity_Parent::OnSpawnWorld();
	// Find objects that wanted the input pipethrough?

	// Send messages for precache
	for (int32 i = 0; i < m_lMessages.Len(); i++)
		m_lMessages[i].m_Msg.SendPrecache(m_iObject,m_pWServer);

	// Set nonrefreshing (invisible?)
	ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;

	// If we're not disabled send impulse 1 to ourselves
	if (!(m_Flags & ATTACHSUPPORT_DISABLED))
	{
		Reset();
		if (!StartListening(m_pWServer, m_iObject, 1,OBJECT_INPUTENTITY_MESSAGE_ATTACHME))
			ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
	}
}

void CWObject_InputEntity::CheckMessages()
{
	// Check if any messages should be sent this pass...?
	int32 NumMessages = m_lMessages.Len();
	CInputMessage* pMessage = m_lMessages.GetBasePtr();

	for (int32 i = 0; i < NumMessages; i++)
	{
		// if message is disabled, skip it
		if (pMessage[i].m_InputType & ATTACHSUPPORT_DISABLED)
			continue;

		int8 bInputOk = true;
		int8 bMoveOk = true;
		int8 bLookOk = true;
		if (pMessage[i].m_InputType & INPUTENTITY_INPUTTYPE_PRESS)
			bInputOk = (pMessage[i].m_Input & m_Control_Press) != 0 && !(pMessage[i].m_Input & m_Control_LastPress);
		if (pMessage[i].m_InputType & INPUTENTITY_INPUTTYPE_RELEASE)
			bInputOk = bInputOk && (pMessage[i].m_Input & ~m_Control_Press) && 
						(m_Control_LastPress & pMessage[i].m_Input);

		if (pMessage[i].m_InputType & INPUTENTITY_MOVEMASK)
		{
			CVec3Dfp32 Diff = (pMessage[i].m_InputType & INPUTENTITY_INPUTTYPE_MOVEDELTA ? m_dMove :  m_Move) - pMessage[i].m_Move;
			if (pMessage[i].m_InputType & INPUTENTITY_INPUTTYPE_MOVEABS)
			{
				Diff.k[0] = M_Fabs(Diff.k[0]);
				Diff.k[1] = M_Fabs(Diff.k[1]);
				Diff.k[2] = M_Fabs(Diff.k[2]);
			}
			// If move stick has moved in vertical dir >= refval
			if (pMessage[i].m_InputType & INPUTENTITY_INPUTTYPE_MOVEVERT)
				bMoveOk = pMessage[i].m_Move[0] > 0.0f ? Diff[0] > 0.0f : Diff[0] < 0.0f;
			if (pMessage[i].m_InputType & INPUTENTITY_INPUTTYPE_MOVEHORIZ)
				bMoveOk = bMoveOk && (pMessage[i].m_Move[1] > 0.0f ? Diff[1] > 0.0f : Diff[1] < 0.0f);
		}

		if (pMessage[i].m_InputType & INPUTENTITY_LOOKMASK)
		{
			CVec3Dfp32 Diff = (pMessage[i].m_InputType & INPUTENTITY_INPUTTYPE_LOOKDELTA ? m_dLook :  m_Look) - pMessage[i].m_Look;
			if (pMessage[i].m_InputType & INPUTENTITY_INPUTTYPE_MOVEABS)
			{
				Diff.k[0] = M_Fabs(Diff.k[0]);
				Diff.k[1] = M_Fabs(Diff.k[1]);
				Diff.k[2] = M_Fabs(Diff.k[2]);
			}
			// If move stick has moved in vertical dir >= refval
			if (pMessage[i].m_InputType & INPUTENTITY_INPUTTYPE_LOOKVERT)
				bLookOk = pMessage[i].m_Look[1] > 0.0f ? Diff[1] > 0.0f : Diff[1] < 0.0f;
			if (pMessage[i].m_InputType & INPUTENTITY_INPUTTYPE_LOOKHORIZ)
				bLookOk = bLookOk && (pMessage[i].m_Look[2] > 0.0f ? Diff[2] > 0.0f : Diff[2] < 0.0f);
		}

		if (bInputOk && bMoveOk && bLookOk)
		{
			// Send the message...
			pMessage[i].m_Msg.SendMessage(m_iObject,m_iObject,m_pWServer);

			if (pMessage[i].m_InputType & (INPUTENTITY_INPUTTYPE_ONCE | INPUTENTITY_INPUTTYPE_LOOKONCE | INPUTENTITY_INPUTTYPE_MOVEONCE))
				pMessage[i].m_InputType |= ATTACHSUPPORT_DISABLED;
		}
	}
}

void CWObject_InputEntity::Reset()
{
	// Reset input state
	m_Control_Press = 0;
	m_Control_LastPress = 0;

	m_Move = 0.0f;
	m_dMove = 0.0f;

	// Same restrictions as on player? ( +-0.245 in vert?)
	ResetLook();

	// Reset once flags
	int32 NumMessages = m_lMessages.Len();
	CInputMessage* pMessage = m_lMessages.GetBasePtr();
	for (int32 i = 0; i < NumMessages; i++)
		pMessage[i].m_InputType &= ~ATTACHSUPPORT_DISABLED;
}

// Reset movement paramters
void CWObject_InputEntity::ResetLook()
{
	m_Flags |= INPUTENTITY_INPUTTYPE_FIRSTUPDATE;

	m_Look = 0.0f;
	m_dLook = 0.0f;
	m_LookStartOffset = 0.0f;
	m_LookStartTime = m_pWServer->GetGameTime();
}

bool CAttachSupport::StartListening(CWorld_Server* _pWServer, int16 _iObject, bool _bStart, int32 _iMsg, int16 _iSender)
{
	// Start checking for input
	if (_bStart)
		m_Flags &= ~ATTACHSUPPORT_DISABLED;
	else
		m_Flags |= ATTACHSUPPORT_DISABLED;

	bool bOk = true;
	CWObject_Message Msg(_iMsg,_bStart,0,_iObject);
	int32 Len = m_lAttachObjects.Len();
	for (int32 i = 0; i < Len; i++)
	{
		if(m_lAttachObjects[i][0] == '$')
		{
			CStr m_TargetLowerCase = m_lAttachObjects[i].LowerCase();
//			int32 iTarget = -1;
			if ((m_TargetLowerCase == "$player") ||
				(m_TargetLowerCase == "$allplayers"))
			{
				CWObject_Game *pGame = _pWServer->Game_GetObject();
				bOk = false;
				for (int i = 0; i < pGame->Player_GetNum(); i++)
				{
					CWObject* pChar = pGame->Player_GetObject(i);
					if (pChar)
					{
						bOk = _pWServer->Message_SendToObject(Msg,pChar->m_iObject) != 0;
					}
				}
			}
			else if (m_TargetLowerCase == "$activator")
			{
				bOk = _pWServer->Message_SendToObject(Msg,_iSender) != 0;
			}
		}
		else
		{
			int iTarget = _pWServer->Selection_GetSingleTarget(m_lAttachObjects[i]);
			if (iTarget > 0)
				_pWServer->Message_SendToObject(Msg,iTarget);
		}
	}
	Len = m_liAttachObjects.Len();
	for (int i = 0; i < Len; i++)
	{
		_pWServer->Message_SendToObject(Msg,m_liAttachObjects[i]);
	}

	return bOk;
}

aint CWObject_InputEntity::OnMessage(const CWObject_Message &_Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJECT_INPUTENTITY_MESSAGE_RESET:
		{
			Reset();
			return 1;
		}
	case OBJECT_INPUTENTITY_MESSAGE_UPDATEINPUT:
		{
			m_Control_Press = _Msg.m_Param0;
			// Update lastpress after checkmessages, and not from message
			m_Control_LastPress = _Msg.m_Param1;

			CVec3Dfp32 Prev = m_Move;
			m_Move = _Msg.m_VecParam0;
			m_dMove = m_Move - Prev;

			if (m_LookSampleTime > 0.0f && (m_pWServer->GetGameTime() - m_LookStartTime).GetTime() > m_LookSampleTime)
				ResetLook();

			// Same restrictions as on player? ( +-0.245 in vert?)
			CVec3Dfp32 Look = _Msg.m_VecParam1;
			Look[2] = (Look[2] < 0.5f) ? Look[2] : (Look[2] - 1.0f);
			if (m_Flags & INPUTENTITY_INPUTTYPE_FIRSTUPDATE)
			{
				m_LookStartOffset[1] = Look[1];
				m_LookStartOffset[2] = Look[2];
				m_Flags &= ~INPUTENTITY_INPUTTYPE_FIRSTUPDATE;
			}
			Prev = m_Look;
			m_dLook = Look - Prev;
			// Look up down is absolute for now
			m_Look[1] = Look[1] - m_LookStartOffset.k[1];
			// Look sideways is set from first update
			m_Look[2] = Look[2] - m_LookStartOffset.k[2];

			// Refresh messages
			CheckMessages();
			return 1;
		}
	case OBJMSG_IMPULSE:
		{
			bool bOk = true;
			if (_Msg.m_Param0 == 0)
			{
				Reset();
				StartListening(m_pWServer, m_iObject, 0,OBJECT_INPUTENTITY_MESSAGE_ATTACHME, _Msg.m_iSender);
			}
			else if (_Msg.m_Param0 == 1)
			{
				Reset();
				StartListening(m_pWServer, m_iObject, 1,OBJECT_INPUTENTITY_MESSAGE_ATTACHME, _Msg.m_iSender);
			}
			else if (_Msg.m_Param0 == 2)
			{
				Reset();
				// Destroy ourselves
				StartListening(m_pWServer, m_iObject, 0,OBJECT_INPUTENTITY_MESSAGE_ATTACHME, _Msg.m_iSender);
				m_pWServer->Object_Destroy(m_iObject);
			}
			else
				bOk = false;

			return bOk;
		}
	case OBJMSG_CHAR_LISTENTOME:
		{
			// The sender wants us to listen to them, so ok then
			if (_Msg.m_Param0)
			{
				// Add, check so that
				AddListenToIndex(m_pWServer, m_iObject, _Msg.m_Param1,OBJECT_INPUTENTITY_MESSAGE_ATTACHME);
			}
			else
			{
				// Remove
				RemoveListenToIndex(m_pWServer, m_iObject, _Msg.m_Param1,OBJECT_INPUTENTITY_MESSAGE_ATTACHME);
			}
			return 1;
		}
	default:
		return CWObject_InputEntity_Parent::OnMessage(_Msg);
	}
}

void CAttachSupport::AddListenToIndex(CWorld_Server* _pWServer, int16 _iObject, int16 _Index, int32 _iMsg)
{
	bool bOk = true;
	int32 Len = m_liAttachObjects.Len();
	for (int32 i = 0; i < Len; i++)
	{
		if (m_liAttachObjects[i] == _Index)
		{
			bOk = false;
			break;
		}
	}

	if (bOk)
	{
		m_liAttachObjects.Add(_Index);
		// If we're already running start attach
		if (!(m_Flags & ATTACHSUPPORT_DISABLED))
		{
			CWObject_Message Msg(_iMsg,true,0,_iObject);
			_pWServer->Message_SendToObject(Msg,_Index);
		}
	}
}

void CAttachSupport::RemoveListenToIndex(CWorld_Server* _pWServer, int16 _iObject, int16 _Index, int32 _iMsg)
{
	int32 Len = m_liAttachObjects.Len();
	for (int32 i = 0; i < Len; i++)
	{
		if (m_liAttachObjects[i] == _Index)
		{
			CWObject_Message Msg(_iMsg,false,0,_iObject);
			_pWServer->Message_SendToObject(Msg,_Index);
			m_liAttachObjects.Del(i);
			break;
		}
	}
}

void CWObject_InputEntity::OnDeltaSave(CCFile* _pFile)
{
	// Save input, flags and flags / message (things that might have changed...)
	// (if it's not disabled)
	if (m_liAttachObjects.Len())
		m_Flags |= ATTACHSUPPORT_GOTEXTRAOBJECTS;

	_pFile->WriteLE(m_Flags);
	if (!(m_Flags & ATTACHSUPPORT_DISABLED))
	{
		_pFile->WriteLE(m_Move.k[0]);
		_pFile->WriteLE(m_Move.k[1]);
		_pFile->WriteLE(m_Move.k[2]);
		_pFile->WriteLE(m_dMove.k[0]);
		_pFile->WriteLE(m_dMove.k[1]);
		_pFile->WriteLE(m_dMove.k[2]);
		_pFile->WriteLE(m_Look.k[0]);
		_pFile->WriteLE(m_Look.k[1]);
		_pFile->WriteLE(m_Look.k[2]);
		_pFile->WriteLE(m_dLook.k[0]);
		_pFile->WriteLE(m_dLook.k[1]);
		_pFile->WriteLE(m_dLook.k[2]);
		_pFile->WriteLE(m_LookStartOffset.k[0]);
		_pFile->WriteLE(m_LookStartOffset.k[1]);
		_pFile->WriteLE(m_LookStartOffset.k[2]);

		_pFile->WriteLE(m_Control_Press);
		_pFile->WriteLE(m_Control_LastPress);
		m_LookStartTime.Write(_pFile);

		// Save / message (don't bother with how many messages, shouldn't change...)
		for (int32 i = 0; i < m_lMessages.Len(); i++)
			_pFile->WriteLE(m_lMessages[i].m_InputType);
	}
	AttachSave(_pFile);
}

void CAttachSupport::AttachSave(CCFile* _pFile)
{
	// Save extra objects that might have attached
	if (m_Flags & ATTACHSUPPORT_GOTEXTRAOBJECTS)
	{
		int16 Len = m_liAttachObjects.Len();
		_pFile->WriteLE(Len);
		for (int32 i = 0; i < Len; i++)
			_pFile->WriteLE(m_liAttachObjects[i]);
	}
}

void CAttachSupport::AttachLoad(CCFile* _pFile)
{
	// Save extra objects that might have attached
	if (m_Flags & ATTACHSUPPORT_GOTEXTRAOBJECTS)
	{
		int16 Len;
		_pFile->ReadLE(Len);
		m_liAttachObjects.SetLen(Len);
		for (int32 i = 0; i < Len; i++)
			_pFile->ReadLE(m_liAttachObjects[i]);
	}
}

void CWObject_InputEntity::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	_pFile->ReadLE(m_Flags);
	
	if (!(m_Flags & ATTACHSUPPORT_DISABLED))
	{
		_pFile->ReadLE(m_Move.k[0]);
		_pFile->ReadLE(m_Move.k[1]);
		_pFile->ReadLE(m_Move.k[2]);
		_pFile->ReadLE(m_dMove.k[0]);
		_pFile->ReadLE(m_dMove.k[1]);
		_pFile->ReadLE(m_dMove.k[2]);
		_pFile->ReadLE(m_Look.k[0]);
		_pFile->ReadLE(m_Look.k[1]);
		_pFile->ReadLE(m_Look.k[2]);
		_pFile->ReadLE(m_dLook.k[0]);
		_pFile->ReadLE(m_dLook.k[1]);
		_pFile->ReadLE(m_dLook.k[2]);
		_pFile->ReadLE(m_LookStartOffset.k[0]);
		_pFile->ReadLE(m_LookStartOffset.k[1]);
		_pFile->ReadLE(m_LookStartOffset.k[2]);

		_pFile->ReadLE(m_Control_Press);
		_pFile->ReadLE(m_Control_LastPress);
		m_LookStartTime.Read(_pFile);

		// Save / message (don't bother with how many messages, shouldn't change...)
		for (int32 i = 0; i < m_lMessages.Len(); i++)
			_pFile->ReadLE(m_lMessages[i].m_InputType);
	}
	AttachLoad(_pFile);
}
