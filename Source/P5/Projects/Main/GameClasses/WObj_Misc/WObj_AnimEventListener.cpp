#include "PCH.h"
#include "WObj_AnimEventListener.h"
#include "../../GameWorld/WClientMod_Defines.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_AnimEventListener, CWObject_AnimEventListener_Parent, 0x0100);

void CWObject_AnimEventListener::OnCreate()
{
	CWObject_AnimEventListener_Parent::OnCreate();
	m_lMessages.Clear();
	m_lAttachObjects.Clear();
	m_Flags = 0;
}

void CWObject_AnimEventListener::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	static const char *FlagsTranslate[] =
	{
		"","","","","","","","","","","","disabled",NULL
	};

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyVal = _pKey->GetThisValue();

	// Meh, one keyname for each type I guess
	switch (_KeyHash)
	{
	case MHASH2('FLAG','S'): // "FLAGS"
		{
			m_Flags = KeyVal.TranslateFlags(FlagsTranslate);
			break;
		}
	default:
		{
			if (KeyName.CompareSubStr("EVENTMESSAGE") == 0)
			{
				// Primary attack
				CEventMessage Msg;
				Msg.m_Msg.Parse(KeyVal,m_pWServer);
				Msg.m_EventType = int(KeyName[12] - '0');

				int iSlot = atoi(KeyName.Str() + 13);
				m_lMessages.SetMinLen(iSlot + 1);
				m_lMessages[iSlot] = Msg;
			}
			else if (KeyName.CompareSubStr("LISTENTO") == 0)
			{
				// Which character we should listen to
				m_lAttachObjects.Add(KeyVal);
			}
			else
				CWObject_AnimEventListener_Parent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_AnimEventListener::OnFinishEvalKeys()
{
	CWObject_AnimEventListener_Parent::OnFinishEvalKeys();
}


void CWObject_AnimEventListener::OnRefresh()
{
	CWObject_AnimEventListener_Parent::OnRefresh();

	// Until we have successfully started listening, keep bugging
	if ((m_Flags & ATTACHSUPPORT_DISABLED) || StartListening(m_pWServer, m_iObject, 1,OBJECT_ANIMEVENTLISTENER_MESSAGE_ATTACHME))
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
}

void CWObject_AnimEventListener::OnSpawnWorld()
{
	CWObject_AnimEventListener_Parent::OnSpawnWorld();
	// Find objects that wanted the input pipethrough?

	// Send messages for precache
	for (int32 i = 0; i < m_lMessages.Len(); i++)
		m_lMessages[i].m_Msg.SendPrecache(m_iObject,m_pWServer);

	// Set nonrefreshing (invisible?)
	ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;

	// If we're not disabled send impulse 1 to ourselves
	if (!(m_Flags & ATTACHSUPPORT_DISABLED))
	{
		if (!StartListening(m_pWServer, m_iObject, 1,OBJECT_ANIMEVENTLISTENER_MESSAGE_ATTACHME))
			ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
	}
}

void CWObject_AnimEventListener::CheckMessages(int32 _EventType)
{
	// Check if any messages should be sent this pass...?
	int32 NumMessages = m_lMessages.Len();
	CEventMessage* pMessage = m_lMessages.GetBasePtr();

	for (int32 i = 0; i < NumMessages; i++)
	{
		if (pMessage[i].m_EventType == _EventType)
		{
			pMessage[i].m_Msg.SendMessage(m_iObject,m_iObject,m_pWServer);
		}
	}
}

aint CWObject_AnimEventListener::OnMessage(const CWObject_Message &_Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJECT_ANIMEVENTLISTENER_MESSAGE_ONEVENT:
		{
			// Refresh messages
			CheckMessages(_Msg.m_Param0);
			return 1;
		}
	case OBJMSG_IMPULSE:
		{
			bool bOk = true;
			if (_Msg.m_Param0 == 0)
			{
				StartListening(m_pWServer,m_iObject,0,OBJECT_ANIMEVENTLISTENER_MESSAGE_ATTACHME);
			}
			else if (_Msg.m_Param0 == 1)
			{
				StartListening(m_pWServer, m_iObject, 1,OBJECT_ANIMEVENTLISTENER_MESSAGE_ATTACHME);
			}
			else if (_Msg.m_Param0 == 2)
			{
				// Destroy ourselves
				StartListening(m_pWServer, m_iObject, 0, OBJECT_ANIMEVENTLISTENER_MESSAGE_ATTACHME);
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
				AddListenToIndex(m_pWServer, m_iObject, _Msg.m_Param1,OBJECT_ANIMEVENTLISTENER_MESSAGE_ATTACHME);
			}
			else
			{
				// Remove
				RemoveListenToIndex(m_pWServer, m_iObject, _Msg.m_Param1,OBJECT_ANIMEVENTLISTENER_MESSAGE_ATTACHME);
			}
			return 1;
		}
	default:
		return CWObject_AnimEventListener_Parent::OnMessage(_Msg);
	}
}

void CWObject_AnimEventListener::OnDeltaSave(CCFile* _pFile)
{
	// Save flags (things that might have changed...)
	// (if it's not disabled)
	if (m_liAttachObjects.Len())
		m_Flags |= ATTACHSUPPORT_GOTEXTRAOBJECTS;
	_pFile->WriteLE(m_Flags);
	AttachSave(_pFile);
}

void CWObject_AnimEventListener::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	// Load flags (things that might have changed...)
	// (if it's not disabled)
	_pFile->ReadLE(m_Flags);
	AttachLoad(_pFile);
}
