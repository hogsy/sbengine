#ifndef __WOBJ_SIMPLEMESSAGE_H
#define __WOBJ_SIMPLEMESSAGE_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Simple Message system

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWO_SimpleMessage
					CWO_SimpleMessageContainer
\*____________________________________________________________________________________________*/


#include "MRTC.h"
#include "MDA.h"

class CWorld_Server;
class CWObject_Message;
class CRegistry;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_SimpleMessage
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			Simple Message
						
	Comments:		Parses. Precaches and Sends messages defined by
					leveldesingers
\*____________________________________________________________________*/

class CWO_SimpleMessage;
typedef TPtr<CWO_SimpleMessage> spCWO_SimpleMessage;

class CWO_SimpleMessage : public CReferenceCount
{
public:
	int m_Msg;
	CStr m_Target;
	int m_Param0;
	CStr m_StrParam;
	int8 m_iSpecialTarget;

	bool m_bCondition;
	uint8 m_iOperator;
	int m_Value;
	spCWO_SimpleMessage m_spSubMessage;

	enum
	{
		SPECIAL_TARGET_THIS = 0,
		SPECIAL_TARGET_ACTIVATOR,
		SPECIAL_TARGET_GAME,
		SPECIAL_TARGET_PLAYER,		//Closest player at time of sending
		SPECIAL_TARGET_ALLPLAYERS,  //
		SPECIAL_TARGET_GLASS,
		SPECIAL_TARGET_OTHERWORLD,
	};

	enum
	{
		OPERATOR_EQUALS = 0,
		OPERATOR_NOTEQUALS,
		OPERATOR_GT,
		OPERATOR_LT,
	};
	CWO_SimpleMessage();
	CWO_SimpleMessage(const CWO_SimpleMessage &_Copy);

	void Parse(CStr _St, CWorld_Server *_pWServer);
	void CreateMessage(CWObject_Message &_Msg, int _iObject, int _iActivator) const;
	int SendMessage(int _iObject, int _iActivator, CWorld_Server *_pWServer, CWObject_Message* _pMsg = NULL) const;
	void SendPrecache(int _iObject, CWorld_Server *_pWServer) const;	
	static int ResolveSpecialTargetName(const char* _pName);

	bool CheckCondition(int _Val) const;

	operator bool() const { return IsValid(); }
	bool IsValid() const { return m_Msg != 0; }

	void operator =(const CWO_SimpleMessage& _Msg);

	//Save message to file
	void OnDeltaSave(CCFile* _pFile) const;
	//Create message from file
	void OnDeltaLoad(CCFile* _pFile);

#ifndef M_RTM
	CFStr GetDesc() const;
private:
	void LogMessageToArray(CStr To, int ReturnValue, int _iObject, int _iActivator, CWorld_Server *_pWServer) const;
	void LogMessageToConsole(CStr To, int ReturnValue, int _iObject, int _iActivator, CWorld_Server *_pWServer) const;
	CStr GetObjectDebugName(int _iObject, CWorld_Server *_pWServer, bool bNameFirst) const;
	static CStr GetMessageDebugName(int Msg);
	static CStr GetAIImpulseDebugName(int Msg);
#endif
};



/**********************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_SimpleMessageContainer
|_______________________________________________________________________________________________
\**********************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			Container for simple messages
						
	Comments:		Simplifies parsing and handling of numerous simple
					message keys
\*____________________________________________________________________*/

class CWO_SimpleMessageContainer
{
	TArray<CStr> m_lTranslate;
	TArray<TArray<CWO_SimpleMessage> > m_lMsgs;

public:
	void Register(int _iID, const char *_pKey);
	bool OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey, CWorld_Server *_pWServer);
	void AddMessage(int _Event, const CWO_SimpleMessage& _Msg);
	void Precache(int _iObject, CWorld_Server *_pWServer);
	void SendMessage(int _Event, int _iObject, int _iActivator, CWorld_Server *_pWServer);
	CWO_SimpleMessage *GetMessages(int _Event, int &_nMessages);
};

#endif

