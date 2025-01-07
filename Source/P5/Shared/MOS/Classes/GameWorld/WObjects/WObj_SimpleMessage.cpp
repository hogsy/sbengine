#include "PCH.h"
#include "WObj_SimpleMessage.h"
#include "WObj_System.h"
#include "WObj_Game.h"
#include "../../../../../Projects/Main/GameClasses/WObj_Game/WObj_GameCore.h"


enum
{
	hash_WorldGlassSpawn	= MHASH4('Worl','dGla','ssSp','awn'),
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_SimpleMessage
|__________________________________________________________________________________________________
\*************************************************************************************************/

CWO_SimpleMessage::CWO_SimpleMessage()
{
	MAUTOSTRIP(CWO_SimpleMessage_ctor, MAUTOSTRIP_VOID);
	m_iSpecialTarget = -1;
	m_Msg = 0;
	m_bCondition = false;
	m_spSubMessage = NULL;
}

CWO_SimpleMessage::CWO_SimpleMessage(const CWO_SimpleMessage &_Copy)
{
	operator = (_Copy);
}

void CWO_SimpleMessage::Parse(CStr _St, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CWO_SimpleMessage_Parse, MAUTOSTRIP_VOID);
	m_Msg = _St.GetStrSep(";").Val_int();
	m_Target = _St.GetStrSep(";");
	
	m_iSpecialTarget = ResolveSpecialTargetName(m_Target);
	if (m_iSpecialTarget < 0)
		m_Target = _pWServer->World_MangleTargetName(m_Target);

	m_Param0 = _St.GetStrSep(";").Val_int();
	if(_St.Find(";") != -1)
	{
		m_StrParam = _St.GetStrSep(";");
		m_bCondition = true;
		m_iOperator = _St.GetStrSep(";").Val_int();
		m_Value = _St.GetStrSep(";").Val_int();
		if(_St != "")
		{
			m_spSubMessage = MNew(CWO_SimpleMessage);
			m_spSubMessage->Parse(_St, _pWServer);
		}
	}
	else
		m_StrParam = _St;
}

void CWO_SimpleMessage::CreateMessage(CWObject_Message &_Msg, int _iObject, int _iActivator) const
{
	MAUTOSTRIP(CWO_SimpleMessage_CreateMessage, MAUTOSTRIP_VOID);
	_Msg.m_Msg = m_Msg;
	_Msg.m_Param0 = m_Param0;
	_Msg.m_pData = const_cast<char*>((const char*)m_StrParam);
	if(_Msg.m_pData)
		_Msg.m_DataSize = m_StrParam.Len() + 1;
	else
		_Msg.m_DataSize = 0;
	_Msg.m_iSender = _iObject;
	_Msg.m_Param1 = _iActivator;

	if(_Msg.m_Msg == OBJMSG_IMPULSE)
	{
		// For backwards compability reasons, Object and Activator must be reversed
		_Msg.m_Param1 = _iObject;
		_Msg.m_iSender = _iActivator;
	}
}

void CWO_SimpleMessage::operator =(const CWO_SimpleMessage& _Msg)
{
	m_Msg = _Msg.m_Msg;
	m_Target = _Msg.m_Target;
	m_Param0 = _Msg.m_Param0;
	m_StrParam = _Msg.m_StrParam;
	m_iSpecialTarget = _Msg.m_iSpecialTarget;
	m_bCondition = _Msg.m_bCondition;
    m_iOperator = _Msg.m_iOperator;
	m_Value	= _Msg.m_Value;
	if(_Msg.m_spSubMessage)
	{
		m_spSubMessage = MNew(CWO_SimpleMessage);
		*m_spSubMessage = *(_Msg.m_spSubMessage);
	}
}

bool CWO_SimpleMessage::CheckCondition(int _Val) const
{
	switch(m_iOperator)
	{
	case OPERATOR_EQUALS:
		return _Val == m_Value;
	case OPERATOR_NOTEQUALS:
		return _Val != m_Value;
	case OPERATOR_LT:
		return _Val < m_Value;
	case OPERATOR_GT:
		return _Val > m_Value;
	default:
		return false;
	}
	return false;
}

int CWO_SimpleMessage::SendMessage(int _iObject, int _iActivator, CWorld_Server *_pWServer,CWObject_Message* _pMsg) const
{
	MAUTOSTRIP(CWO_SimpleMessage_SendMessage, MAUTOSTRIP_VOID);
	if(!IsValid())
		return 0;
	
	CWObject_Message LocalMsg;
	CWObject_Message& SendMsg = _pMsg ? *_pMsg : LocalMsg;
	if (!_pMsg)
		CreateMessage(LocalMsg, _iObject, _iActivator);

	CFStr Target = m_Target;
  #ifndef M_RTM
	CStr To = m_Target;
  #endif

	int iSpecialTarget = m_iSpecialTarget;
	if (iSpecialTarget == SPECIAL_TARGET_OTHERWORLD)
	{
		Target = Target.Del(0, 1);				// skip leading '$'

		/* -- apperenly, this was no good, since things sent to $level:<obj> usually wants the effect to happen next time the level is entered 
		CFStr Level = Target.GetStrSep(":");
		if (_pWServer->m_WorldName.CompareNoCase(Level) == 0)
		{
			// if sending to the level currently being played, send instant...
			iSpecialTarget = ResolveSpecialTargetName(Target);
		}*/
	}

	int RetVal = 0;
	switch (iSpecialTarget)
	{
	case SPECIAL_TARGET_GAME:
		{
			RetVal = _pWServer->Message_SendToObject(SendMsg, _pWServer->Game_GetObjectIndex());
#ifndef M_RTM
			if(_pWServer->m_bLogMessages || _pWServer->m_bConsoleLogMessages)
				To = CStrF("%i", _pWServer->Game_GetObjectIndex());
#endif
		}
		break;

	case SPECIAL_TARGET_ACTIVATOR:
		{
			if(_iActivator <= 0)
				ConOutL("(CWO_SimpleMessage::SendMessage) Activator not known");
			else
			{
				RetVal = _pWServer->Message_SendToObject(SendMsg, _iActivator);
#ifndef M_RTM
				if(_pWServer->m_bLogMessages || _pWServer->m_bConsoleLogMessages)
					To = CStrF("%i", _iActivator);
#endif
			}
		}
		break;

	case SPECIAL_TARGET_THIS:
		{
			RetVal = _pWServer->Message_SendToObject(SendMsg, _iObject);
		}
		break;

	case SPECIAL_TARGET_PLAYER:
		{
			//Find closest player to sending object (if any)
			CWObject * pObj = _pWServer->Object_Get(_iObject);
			if (pObj)
			{
				int iObj = 0;
				fp32 DistSqr;
				fp32 MinDistSqr = _FP32_MAX;
				CWObject * pChar;
				CWObject_Game *pGame = _pWServer->Game_GetObject();
				for (int i = 0; i < pGame->Player_GetNum(); i++)
				{
					if ((pChar = pGame->Player_GetObject(i)) &&											
						((DistSqr = pObj->GetPosition().DistanceSqr(pChar->GetPosition())) < MinDistSqr))	
					{
						iObj = pChar->m_iObject;
						MinDistSqr = DistSqr;
					}
				}
				if (iObj)
				{
					RetVal = _pWServer->Message_SendToObject(SendMsg, iObj);
					#ifndef M_RTM
					if(_pWServer->m_bLogMessages)
						To = CStrF("%i", iObj);
					#endif
				}
			};
		};
		break;

	case SPECIAL_TARGET_ALLPLAYERS:
		{
			//Send message to all players
			CWObject * pChar;
			CWObject_Game *pGame = _pWServer->Game_GetObject();
			for (int i = 0; i < pGame->Player_GetNum(); i++)
			{
				if ((pChar = pGame->Player_GetObject(i)))
				{
					RetVal = _pWServer->Message_SendToObject(SendMsg, pChar->m_iObject);

					#ifndef M_RTM
					if(_pWServer->m_bLogMessages || _pWServer->m_bConsoleLogMessages)
					{
						CWObject *pObj = _pWServer->Object_Get(pChar->m_iObject);
						if(pObj)
						{
							if(i > 0)
								To += ", ";
							To += CStrF("%i", pChar->m_iObject);
						}
					}
					#endif
				}
			}
		}
		break;

	case SPECIAL_TARGET_GLASS:
		{
			int iGlassObj = _pWServer->Selection_GetSingleTarget(hash_WorldGlassSpawn);
			if (iGlassObj)
				RetVal = _pWServer->Message_SendToObject(SendMsg, iGlassObj);
		}
		break;

	case SPECIAL_TARGET_OTHERWORLD:
		{
			// Send message to another level	("$level:object")
			int iGameObject = _pWServer->Game_GetObjectIndex();
			CWObject_Message Msg;
			CreateMessage(Msg, iGameObject, iGameObject);
			CWObject_Message Msg2(OBJMSG_GAME_PIPEMESSAGETOWORLD);
			Msg2.m_pData = &Msg;
			Msg2.m_Param0 = aint(m_Target.Str() + 1);
			if(_pWServer->Message_SendToObject(Msg2, iGameObject))
				ConOut("Sucessfully sent world-message to: " + m_Target);
			else
				ConOutL("(CWO_SimpleMessage::SendMessage) Failed to send world-message to: " + m_Target);
		}
		break;

	default:
		{
			int TargetLen = Target.Len();
			if (TargetLen > 0 && Target[TargetLen - 1] == '_')		// NOTE: trailing '_' is used to disable messages
				return 0;

			TSelection<CSelection::MEDIUM_BUFFER> Selection;
			_pWServer->Selection_AddTarget(Selection, Target);

			const int16* pSel = NULL;
			int nSel = _pWServer->Selection_Get(Selection, &pSel);
			for(int i = 0; i < nSel; i++)
				RetVal = _pWServer->Message_SendToObject(SendMsg, pSel[i]);
			
		  #ifndef M_RTM
			// Note: the changes made to the To string to make it readable ingame may influence Ogier code (which is 
			// currently not in use though...) Check it if ogier is fixed to support game simulation again.
			if(_pWServer->m_bLogMessages || _pWServer->m_bConsoleLogMessages)
			{
				const int16 *piObj;
				int nObj = _pWServer->Selection_Get(Selection, &piObj);
				if (nObj == 0)
				{
					To = CStrF("No such target! (%s)", To.Str());
				}
				else
				{
					To = m_Target + " (";
					for(int i = 0; i < nObj; i++)
					{
						CWObject *pObj = _pWServer->Object_Get(piObj[i]);
						if(pObj)
						{
							if(i > 0)
								To += ", ";
							To += CStrF("%i", piObj[i]);
						}
					}
					To += ")";
				}
			}
		  #endif
		}
		break;
	}

	#ifndef M_RTM
		if(_pWServer->m_bLogMessages)
			LogMessageToArray(To, RetVal, _iObject, _iActivator, _pWServer);
		if(_pWServer->m_bConsoleLogMessages)
			LogMessageToConsole(To, RetVal, _iObject, _iActivator, _pWServer);
	#endif

	if(m_bCondition)
	{
		bool bSuccess = CheckCondition(RetVal);
		if(m_spSubMessage && bSuccess)
			return m_spSubMessage->SendMessage(_iObject, _iActivator, _pWServer);
		else
			return bSuccess;
	}
	else
		return RetVal;
}

void CWO_SimpleMessage::SendPrecache(int _iObject, CWorld_Server *_pWServer) const
{
	MAUTOSTRIP(CWO_SimpleMessage_SendPrecache, MAUTOSTRIP_VOID);
	if(!IsValid())
		return;

	CWObject_Message SimpleMsg;
	CreateMessage(SimpleMsg, _iObject, -1);

	CWObject_Message Msg(OBJSYSMSG_PRECACHEMESSAGE);
	Msg.m_pData = &SimpleMsg;
	Msg.m_DataSize = sizeof(CWObject_Message);

	int Res = 0;
	
	switch(m_iSpecialTarget)
	{
	case SPECIAL_TARGET_GAME:
		Res = _pWServer->Message_SendToObject(Msg, _pWServer->Game_GetObjectIndex());
		break;

	case SPECIAL_TARGET_THIS:
	case SPECIAL_TARGET_GLASS:
		Res = _pWServer->Message_SendToObject(Msg, _iObject);
		break;
	
	case SPECIAL_TARGET_ACTIVATOR:
	case SPECIAL_TARGET_PLAYER:
	case SPECIAL_TARGET_ALLPLAYERS:
		// Precache message needs only to go to one player
		{
			CWObject_GameCore* pGame = (CWObject_GameCore*)_pWServer->Game_GetObject();
			CWObject *pChar = (pGame->Player_GetNum() > 0) ? pGame->Player_GetObject(0) : NULL;
			if(pChar)
				Res = _pWServer->Message_SendToObject(Msg, pChar->m_iObject);
			else
			{
				// KRICK lösning of the day.
				// Since we don't have a player, we have to send the precache message
				// to someone similar: We try to find a character
				// New solution, send to dummy player
				Res = _pWServer->Message_SendToObject(Msg, pGame->m_iDummyPlayer);
/*				TSelection<CSelection::LARGE_BUFFER> Selection;
				_pWServer->Selection_AddClass(Selection, "character");
				const int16* piObjs;
				int nObj = _pWServer->Selection_Get(Selection, &piObjs);
				if(nObj > 0)
					Res = _pWServer->Message_SendToObject(Msg, piObjs[0]);
				else
					ConOutL("§cf80WARNING: Couldn't find a character to send SimpleMessages precache to instead of player");
				*/
			}
		}
 		break;

	default:
		{
			// We want a return-value from Message_SendToTarget
			TSelection<CSelection::LARGE_BUFFER> Selection;
			_pWServer->Selection_AddTarget(Selection, m_Target);
			const int16* piObjs;
			int nObj = _pWServer->Selection_Get(Selection, &piObjs);
			Res = 1;
			if(nObj == 0)
			{
#ifndef M_RTM
				int Len = m_Target.Len();
				if(Len == 0 || (m_Target[Len - 1] != '_' && m_Target[0] != '$')) // Ignore targetnames that end with _
				{
					CWObject *pObj = _pWServer->Object_Get(_iObject);
					ConOutLD(CStrF("§cf80WARNING: (GP) Could not find target for SimpleMessage precache (%i, %s): %s", _iObject, pObj ? pObj->GetName() : "", GetDesc().Str()));
				}
#endif
			}
			else
			{
				for(int i = 0; i < nObj; i++)
					if(!_pWServer->Message_SendToObject(Msg, piObjs[i]))
						Res = 0;
			}
		}
		break;
	}

	if(m_spSubMessage)
		m_spSubMessage->SendPrecache(_iObject, _pWServer);
	if(Res == 0)
	{
		CWObject *pObj = _pWServer->Object_Get(_iObject);
		ConOutLD(CStrF("§cf80WARNING: (GP) SimpleMessage precache failed (%i, %s): %s", _iObject, pObj ? pObj->GetName() : "", GetDesc().Str()));
	}
}


int CWO_SimpleMessage::ResolveSpecialTargetName(const char* _pName)
{
	if (!_pName || !_pName[0])
	{
		return SPECIAL_TARGET_THIS;
	}
	else if (_pName[0] == '$')
	{
		if (CStrBase::CompareNoCase(_pName, "$this") == 0)
			return SPECIAL_TARGET_THIS;

		else if (CStrBase::CompareNoCase(_pName, "$activator") == 0)
			return SPECIAL_TARGET_ACTIVATOR;

		else if (CStrBase::CompareNoCase(_pName, "$game") == 0)
			return SPECIAL_TARGET_GAME;

		else if (CStrBase::CompareNoCase(_pName, "$player") == 0)
			return SPECIAL_TARGET_PLAYER;

		else if (CStrBase::CompareNoCase(_pName, "$allplayers") == 0)
			return SPECIAL_TARGET_ALLPLAYERS;

		else if (CStrBase::CompareNoCase(_pName, "$worldglassspawn") == 0)
			return SPECIAL_TARGET_GLASS;

		else if (strchr(_pName, ':') != NULL)
			return SPECIAL_TARGET_OTHERWORLD;
	}
	return -1;
};



#ifdef _DEBUG
#define MACRO_WRITEVERIFY {uint32 Apa = 0x81920467; _pFile->WriteLE(Apa);}
#define MACRO_READVERIFY {uint32 Apa; _pFile->ReadLE(Apa); M_ASSERT(Apa == 0x81920467, CStrF("Load/save mismatch in file '%s' on line %i", __FILE__, __LINE__)); };
#else
#define MACRO_WRITEVERIFY
#define MACRO_READVERIFY
#endif


//Save message to file
void CWO_SimpleMessage::OnDeltaSave(CCFile* _pFile) const
{
	MACRO_WRITEVERIFY;

	int32 Temp32;
	int8 Temp8;
	Temp32 = m_Msg; _pFile->WriteLE(Temp32);
	m_Target.Write(_pFile);
	Temp32 = m_Param0; _pFile->WriteLE(Temp32);
	m_StrParam.Write(_pFile);
	_pFile->WriteLE(m_iSpecialTarget);

	//Check if there are any conditional stuff. Don't think I need to check if there actually is a submessage, 
	//but I'll do it to be on the safe side. Conditional without submessage is completely useles anyway.
	if (m_bCondition && m_spSubMessage)
	{
		//Save conditional stuff. 
		Temp8 = 1; _pFile->WriteLE(Temp8);
		_pFile->WriteLE(m_iOperator);
		Temp32 = m_Value; _pFile->WriteLE(Temp32);
		m_spSubMessage->OnDeltaSave(_pFile);
	}
	else
	{
		//No conditional stuff
		Temp8 = 0; _pFile->WriteLE(Temp8);
	}

	MACRO_WRITEVERIFY;
};

//Create message from file
void CWO_SimpleMessage::OnDeltaLoad(CCFile* _pFile)
{
	MACRO_READVERIFY;

	int32 Temp32;
	int8 Temp8;
	_pFile->ReadLE(Temp32); m_Msg = Temp32; 
	m_Target.Read(_pFile);
	_pFile->ReadLE(Temp32); m_Param0 = Temp32; 
	m_StrParam.Read(_pFile);
	_pFile->ReadLE(m_iSpecialTarget);

	//Check if there are any conditional stuff. 
	_pFile->ReadLE(Temp8); m_bCondition = (Temp8 != 0) ? true : false;
	if (m_bCondition)
	{
		//Load conditional stuff. 
		_pFile->ReadLE(m_iOperator);
		_pFile->ReadLE(Temp32); m_Value = Temp32; 
		m_spSubMessage = MNew(CWO_SimpleMessage);
		m_spSubMessage->OnDeltaLoad(_pFile);
	}

	MACRO_READVERIFY;
};


#ifndef M_RTM
CFStr CWO_SimpleMessage::GetDesc() const
{
	MAUTOSTRIP(CWO_SimpleMessage_GetDesc, CFStr());
	CFStr Target = m_Target;
	if(Target == "")
		Target = "$this";

	return CFStrF("Msg: 0x%x  Target: %s  Param0: %i  StrParam: %s", m_Msg, Target.LowerCase().Str(), m_Param0, m_StrParam.Str());
}

void CWO_SimpleMessage::LogMessageToArray(CStr To, int ReturnValue, int _iObject, int _iActivator, CWorld_Server *_pWServer) const
{
	CStr sObj = GetObjectDebugName(_iObject, _pWServer, false);
	CStr sActivator = GetObjectDebugName(_iActivator, _pWServer, false);
	
	CStr Log = CStrF("%.2f|%s|0x%x|%s|%i|%s|%s|%s|%s", _pWServer->GetGameTime().GetTime(), 
			 sObj.Str(), m_Msg, m_Target.LowerCase().Str(), m_Param0, m_StrParam.Str(),
			 To.Str(), sActivator.Str(), GetDesc().Str());
	_pWServer->m_lMessageLog.Add(Log);
}

void CWO_SimpleMessage::LogMessageToConsole(CStr To, int ReturnValue, int _iObject, int _iActivator, CWorld_Server *_pWServer) const
{
	CStr sObj = GetObjectDebugName(_iObject, _pWServer, true);
	CStr sActivator = GetObjectDebugName(_iActivator, _pWServer, true);
	CStr sTarget = (m_Target[0] == '$') ? CStrF("%s (%s)", To.Str(), m_Target.Str()) : To;
	CStr sMsg = GetMessageDebugName(m_Msg);
	CStr sParam = (m_Msg == 0x111) ? CStrF("AIImpulse: %s", GetAIImpulseDebugName(m_Param0).Str()) : CStrF("Param0: %i", m_Param0);

	CStr Log = CStrF("Message at %.2f, Msg: %s, From: %s, To: %s, %s, StrParam: %s, Activator: %s, Result: %i", 
			         _pWServer->GetGameTime().GetTime(), 
					 sMsg.Str(), 
					 sObj.Str(),
					 sTarget.Str(),
					 sParam.Str(),
					 m_StrParam.Str(),
					 sActivator.Str(), 
					 ReturnValue);
	ConOutL(Log);
}

CStr CWO_SimpleMessage::GetObjectDebugName(int _iObject, CWorld_Server *_pWServer, bool bNameFirst) const
{
	CWObject *pObj = _pWServer->Object_Get(_iObject);
	CStr sObj;
	if(pObj && CFStr(pObj->GetName()) != "")
		if (bNameFirst)
			sObj = CStrF("%s (%i)", CFStr(pObj->GetName()).LowerCase().Str(), pObj->m_iObject);
		else
			sObj = CStrF("%i (%s)", pObj->m_iObject, CFStr(pObj->GetName()).LowerCase().Str());
	else
		sObj = CStrF("%i", _iObject);
	return sObj;
}



CStr CWO_SimpleMessage::GetMessageDebugName(int Msg)
{
	// Message mappings should really be parsed directly from nodetype into a hash...
	char * res = "<Name not known>";
	switch (Msg)
	{
	case 0x111: res = "AIImpulse"; break;
	case 0x1027: res = "CutsceneFov"; break; 
	case 0x30: res = "Destroy"; break;
	case 0x1042: res = "DialTelephone"; break;
	case 0x1040: res = "DisableQuicksave"; break;
	case 0x3b: res = "GetDistance"; break;
	case 0x40: res = "GetRandom"; break;
	case 0x110: res = "Impulse"; break;
	case 0x3c: res = "Param_Add"; break;
	case 0x3e: res = "Param_ClearFlag"; break;
	case 0x42: res = "Param_CopyFrom"; break;
	case 0x3a: res = "Param_Get"; break;
	case 0x3f: res = "Param_GetFlag"; break;
	case 0x41: res = "Param_Randomize"; break;
	case 0x39: res = "Param_Set"; break;
	case 0x3d: res = "Param_SetFlag"; break;
	case 0x31: res = "PlaySound"; break;
	case 0x102a: res = "SetDescName"; break;
	case 0x102f: res = "SetFocusFrameOffset"; break;
	case 0x1043: res = "SetDialogueProxy"; break;
	case 0x35: res = "SetModel"; break;
	case 0x1012: res = "SetModelFlags"; break;
	case 0x33: res = "SetSound"; break;
	case 0x1029: res = "SetUseName"; break;
	case 0x100b: res = "Spawn"; break;
	case 0x163: res = "SwingDoorOpen"; break;
	case 0x164: res = "SwingDoorLock"; break;
	case 0x37: res = "Teleport"; break;
	case 0x220: res = "ExtStaticImpulse"; break;
	case 0x133: res = "ActivateCheckpoint"; break;
	case 0x10c: res = "ChangeWorld"; break;
	case 0x15a: res = "ConExecute"; break;
	case 0x12e: res = "DebugMsg"; break;
	case 0x1009: res = "EndCutscene"; break;
	case 0x12a: res = "FadeScreen"; break;
	case 0x12b: res = "FadeSound"; break;
	case 0x153: res = "LevelExits"; break;
	case 0x159: res = "PauseAllAIActions"; break;
	case 0x124: res = "Play2DSound"; break;
	case 0x158: res = "RemoveGameMsg"; break;
	case 0x1065: res = "SetAttachExtraModel"; break;
	case 0x150: res = "SPSpawn"; break;
	case 0x151: res = "SPUnSpawn"; break;
	case 0x15c: res = "SPRaisePrio"; break;
	case 0x15d: res = "SPRestorePrio"; break;
	case 0x152: res = "SPActivate"; break;
	case 0x15b: res = "SetMusic"; break;
	case 0x134: res = "SetNVRange"; break;
	case 0x12c: res = "ShowGameMsg"; break;
	case 0x15f: res = "ShowGameSurface"; break;
	case 0x12d: res = "ShowInfoScreen"; break;
	case 0x123: res = "Quit"; break;
	case 0x155: res = "SetGamestyle"; break;
	case 0x15e: res = "ModifySpawnFlags"; break;
	case 0x1018: res = "Run behaviour"; break;
	case 0x101f: res = "Stop behaviour"; break;
	case 0x1013: res = "Restriction"; break;
	case 0x660C: res = "Facial"; break;
	case 0x1025: res = "Pause action"; break;
	case 0x1014: res = "AddItem"; break;
	case 0x1015: res = "AddHealth"; break;
	case 0x1038: res = "ActivateItem"; break;
	case 0x1054: res = "AnimImpulse"; break;
	case 0x1055: res = "CanActivateItem"; break;
	case 0x1016: res = "DestroyItem"; break;
	case 0x103c: res = "Disable"; break;
	case 0x1037: res = "DragDoll"; break;
	case 0x103d: res = "EquipItem"; break;
	case 0x102d: res = "GetHealth"; break;
	case 0x1033: res = "GetNumItems"; break;
	case 0x1066: res = "HideModel"; break;
	case 0x1030: res = "Immobile"; break;
	case 0x100d: res = "Immune"; break;
	case 0x1028: res = "IncreaseMaxHealth"; break;
	case 0x1058: res = "ListenToMe"; break;
	case 0x1053: res = "LockToParent"; break;
	case 0x103b: res = "NeverTrigger"; break;
	case 0x1026: res = "PerformAnimAction"; break;
	case 0x1017: res = "PlayAnim"; break;
	case 0x2000: res = "PlayAnimAnimModel"; break;
	case 0x1035: res = "Push"; break;
	case 0x101d: res = "RaiseNoiseLevel"; break;
	case 0x101c: res = "RaiseVisibility"; break;
	case 0x1034: res = "RenderAttached"; break;
	case 0x1020: res = "OSetApproachItem"; break;
	case 0x1021: res = "SetApproachItem"; break;
	case 0x101e: res = "SetAimingMode"; break;
	case 0x1057: res = "SetAnimPhys"; break;
	case 0x103f: res = "SetDropItem"; break;
	case 0x1060: res = "SetMountedCamera"; break;
	case 0x1056: res = "SetMountedLook"; break;
	case 0x1039: res = "ShowExtraItem"; break;
	case 0x102c: res = "SetFlashLight"; break;
	case 0x102b: res = "SetLaserBeam"; break;
	case 0x1062: res = "SetMissionCompleted"; break;
	case 0x103a: res = "SetNightVision"; break;
	case 0x1023: res = "SetPhysFlags"; break;
	case 0x1031: res = "SetSpecialGrab"; break;
	case 0x100c: res = "SpeakOld"; break;
	case 0x200a: res = "Speak"; break;
	case 0x1036: res = "Stun"; break;
	case 0x1063: res = "SyncAnimToChar"; break;
	case 0x1064: res = "SetPlayerFlags"; break;
	case 0x64a0: res = "Team_Alarm"; break;
	case 0x1061: res = "UpdateItemDesc"; break;
	case 0x1051: res = "AddDarkling"; break;
	case 0x1052: res = "ModifyDarknessPowers"; break;
	case 0x100e: res = "AI Control"; break;
	case 0x1008: res = "BeginCutscene"; break;
	case 0x101b: res = "BeginDialogue"; break;
	case 0x100f: res = "Mount"; break;
	case 0x1032: res = "Respawn"; break;
	case 0x101a: res = "Rumble"; break;
	case 0x113: res = "SetZFog"; break;
	case 0x103e: res = "SetCameraEffects"; break;
	case 0x1050: res = "SetPagerNumber"; break;
	case 0x1059: res = "SetDialogueItem"; break;
	case 0x1011: res = "ShakeCamera"; break;
	case 0x102e: res = "UseDNAWeapons"; break;
	case 0x216: res = "GetSequence"; break;
	case 0x211: res = "WaitImpulse"; break;
	case 0x212: res = "WaitParam"; break;
	case 0x5591: res = "RFCDelay"; break;
	case 0x5590: res = "RFCReset"; break;
	case 0x112: res = "LightImpulse"; break;
	case 0x114: res = "LightMorph"; break;
	case 0x214: res = "SetProjMap"; break;
	case 0x218: res = "RoomBroadcast"; break;
	case 0x219: res = "GetNumInside"; break;
	case 0x213: res = "PushObject"; break;
	case 0x221: res = "Object_BreakPart"; break;
	case 0x222: res = "Object_SpawnModel"; break;
	case 0x224: res = "Object_SetVisibilityMask"; break;
	case 0x225: res = "Object_PlayAnim"; break;
	case 0x226: res = "Object_SetDynamic"; break; 
	case 0x223: res = "TV_SetChannel"; break;
	case 0x4078: res = "FXObj Impulse"; break;
	}
	return CStrF("%s (0x%x)", res, Msg);
}

CStr CWO_SimpleMessage::GetAIImpulseDebugName(int AIImpulse)
{
	// AI impulse mappings should really be parsed directly from nodetype into a hash...
	char * res = "<Name not known>";
	switch (AIImpulse)
	{
	case 12: res = "Run behaviour"; break;
	case 129: res = "Run behaviour loop"; break;
	case 34: res = "Force behaviour"; break;
	case 13: res = "Stop behaviour"; break;
	case 14: res = "Push"; break;
	case 36: res = "Aggressive"; break;
	case 32: res = "Aim"; break;
	case 31: res = "Attack"; break;
	case 110: res = "Allow attacks"; break;
	case 45: res = "Target"; break;
	case 21: res = "Continuous Look"; break;
	case 19: res = "Cont. Soft Look"; break;
	case 30: res = "Snap look"; break;
	case 22: res = "Crouch"; break;
	case 62: res = "Die"; break;
	case 7: res = "Damage"; break;
	case 38: res = "Projectile immune"; break;
	case 127: res = "DragDoll"; break;
	case 128: res = "Minimum stance"; break;
	case 126: res = "Maximum stance"; break;
	case 58: res = "Undead stuntime"; break;
	case 101: res = "Next of kin"; break;
	case 15: res = "Ghost Mode"; break;
	case 18: res = "Fly Follow Me"; break;
	case 1: res = "Follow Me"; break;
	case 17: res = "Follow Path"; break;
	case 9: res = "Force FM Relative"; break;
	case 3: res = "Force Follow Me"; break;
	case 23: res = "Jump"; break;
	case 29: res = "Look at object"; break;
	case 28: res = "Move to object"; break;
	case 2: res = "Pause"; break;
	case 0: res = "Release"; break;
	case 104: res = "Release SPs"; break;
	case 102: res = "Teleport to SP"; break;
	case 103: res = "Jump to SP"; break;
	case 10: res = "AI_Voice"; break;
	case 11: res = "Detect Character"; break;
	case 39: res = "Investigate object"; break;
	case 41: res = "Notice player at"; break;
	case 121: res = "Enemy relation"; break;
	case 122: res = "Friend relation"; break;
	case 124: res = "More hostile"; break;
	case 123: res = "Default relation"; break;
	case 24: res = "Switch Weapon"; break;
	case 25: res = "Switch Item"; break;
	case 16: res = "Teleport Follow Me"; break;
	case 37: res = "Unspot All"; break;
	case 26: res = "Use Weapon"; break;
	case 27: res = "Use Item"; break;
	case 68: res = "Explore"; break;
	case 69: res = "Hold"; break;
	case 65: res = "Patrol"; break;
	case 113: res = "Move to ScenePoint"; break;
	case 112: res = "ScenePointOverride"; break;
	case 50: res = "Activation Range"; break;
	case 56: res = "Alertness"; break;
	case 42: res = "Awareness"; break;
	case 44: res = "FOV"; break;
	case 55: res = "Health"; break;
	case 53: res = "Hearing Range"; break;
	case 54: res = "Idle Speed"; break;
	case 43: res = "Sight Range"; break;
	case 63: res = "Security"; break;
	case 40: res = "Team add"; break;
	case 47: res = "Team remove"; break;
	case 59: res = "Active"; break;
	case 60: res = "Range"; break;
	case 61: res = "Object"; break;
	case 120: res = "Special"; break;
	}
	return CStrF("%s (%i)", res, AIImpulse);
}


#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_SimpleMessageContainer
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CWO_SimpleMessageContainer::Register(int _iID, const char *_pKey)
{
	MAUTOSTRIP(CWO_SimpleMessageContainer_Register, MAUTOSTRIP_VOID);
	if(_iID >= m_lTranslate.Len())
		m_lTranslate.SetLen(_iID + 1);

#ifndef M_RTM
	if(m_lTranslate[_iID] != "")
		Error_static("CWO_SimpleMessageContainer::Register", "ID already defined");
#endif

	m_lTranslate[_iID] = _pKey;
}

bool CWO_SimpleMessageContainer::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CWO_SimpleMessageContainer_OnEvalKey, false);
	
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	uint nTranslate = m_lTranslate.Len();
	for (uint i = 0; i < nTranslate; i++)
	{
		const CStr& str = m_lTranslate[i];
		if ((str.Len() > 0) && (KeyName.CompareSubStr(str) == 0))
		{
			m_lMsgs.SetMinLen(i + 1);
			uint iSlot = atoi(KeyName.Str() + str.Len());
			m_lMsgs[i].SetMinLen(iSlot + 1);
			m_lMsgs[i][iSlot].Parse(KeyValue, _pWServer);
			return true;
		}
	}

	return false;
}

void CWO_SimpleMessageContainer::AddMessage(int _Event, const CWO_SimpleMessage& _Msg)
{
	m_lMsgs.SetMinLen(_Event + 1);
	m_lMsgs[_Event].Add(_Msg);
}

void CWO_SimpleMessageContainer::Precache(int _iObject, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CWO_SimpleMessageContainer_Precache, MAUTOSTRIP_VOID);
	m_lMsgs.OptimizeMemory();
	for(int i = 0; i < m_lMsgs.Len(); i++)
	{
		m_lMsgs[i].OptimizeMemory();
		for(int j = 0; j < m_lMsgs[i].Len(); j++)
			m_lMsgs[i][j].SendPrecache(_iObject, _pWServer);
	}
}

void CWO_SimpleMessageContainer::SendMessage(int _Event, int _iObject, int _iActivator, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CWO_SimpleMessageContainer_SendMessage, MAUTOSTRIP_VOID);
	if(_Event < 0 || _Event >= m_lMsgs.Len())
		return;

	for(int j = 0; j < m_lMsgs[_Event].Len(); j++)
		m_lMsgs[_Event][j].SendMessage(_iObject, _iActivator, _pWServer);
}

CWO_SimpleMessage *CWO_SimpleMessageContainer::GetMessages(int _Event, int &_nMessages)
{
	if(_Event < 0 || _Event >= m_lMsgs.Len())
	{
		_nMessages = 0;
		return NULL;
	}

	_nMessages = m_lMsgs[_Event].Len();
	if(_nMessages == 0)
		return NULL;

	return m_lMsgs[_Event].GetBasePtr();
}







