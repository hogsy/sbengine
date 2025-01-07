#ifndef _INC_WOBJ_TEAM
#define _INC_WOBJ_TEAM

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Team class definition
					
	Author:			anders olsson
					
	Copyright:		Copyright Starbreeze Studios AB 2002
					
	Contents:		CWObject_Team
					
	History:		
		020902:		Created File
\*____________________________________________________________________________________________*/	


#include "../AI_Def.h"
#include "../AI_Auxiliary.h"


//Team-related messages
enum
{
	//These messages should objects belonging to teams be able to handle
	OBJMSG_TEAM_GETTEAM = OBJMSGBASE_TEAM,	//Get the param0'th team the object belongs to
	OBJMSG_TEAM_NUMTEAMS,					//Get the number of teams the object belongs to
	OBJMSG_TEAM_JOINTEAM,					//Add the object to team param0
	OBJMSG_TEAM_BELONGTOTEAM,				//Check if object belongs to team param0
	OBJMSG_TEAM_HEARVOICE,					//Check if object can hear voice-communication from sender 

	//These messages is handled by the team class
	OBJMSG_TEAM_GETRELATION,	//Get the team's default relation to team param0
	OBJMSG_TEAM_COMMUNICATE,	//Relay the message pointed at by data to all friendlies that can be
								//reached by normal communication. Don't send message back to sender
	OBJMSG_TEAM_CLEARANCE,		//Get default clearance level for team members
	OBJMSG_TEAM_COMMUNICATIONS,	//Get communication mode of team
	OBJMSG_TEAM_LEAVETEAM,		//Remove the object from team param0

	//Reserved messages. Do not change these values.
	OBJMSG_TEAM_ALARM = 0x64a0,	//Param0 is object that caused the alarm, param1 is alarm level (see below), 
								//sender is object that raises alarm, reason (optional) is senders 
								//relation with cause-object 
	OBJMSG_TEAM_AWARE = 0x64a1,	//Param0 is object that caused the awareness, param1 is NOTICE,DETECT or SPOT, 
								//sender is object that raises alarm, reason (optional) is senders 
								//relation with cause-object
	OBJMSG_TEAM_FIGHT = 0x64a2,	//Param0 is object that caused the awareness, param1 is NOTICE,DETECT or SPOT, 
								//sender is object that raises alarm, reason (optional) is senders 
								//relation with cause-object 
	OBJMSG_TEAM_RANGE_ALARM = 0x64a4,	//Param0 is object that caused the alarm, param1 is alarm level (see below), 
								//sender is object that raises alarm, reason (optional) is senders 
								//relation with cause-object 
	OBJMSG_TEAM_FOUND_CORPSE = 0x64a5,	//Param0 is perp, Param1 victim, Sender is observer,m_VecParam0 is pos
								//All bots that receive this msg should remove the victim (Param1) from their deathlists
	OBJMSG_TEAM_SEARCH_OFF = 0x64a6,	// Param0 is the suspect, param1 is the voice variant
	// OBJMSG_ALERT_TEAM_SEARCH_OFF = 0x64a7,	// Not used, slot available for other use.
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			Auxiliary AI team class
						
	Comments:		Object contains info about a team. This class 
					should be changed to an auxiliary game class
					and moved inside the game object in time.
\*____________________________________________________________________*/
class CWObject_Team : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:

	//Team communications
	enum {
		NONE = 0,
		VOICE,
		DIRECT,
	};
protected:
	//Relation with other members of own team
	int m_InternalRelation;

	//Default relations (see CAI_AgentInfo) versus other teams
	TIntHashMap<int> m_lRelations;

	//Default relation versus teams that haven't been defined in hash above
	int m_DefaultRelation;

	//Team names and default relations. These list is emptied when relations 
	//are set up (i.e. during OnSpawnWorld)
	TArray<CStr> m_lRelNamesFromKey;
	TArray<int> m_lRelationsFromKey;

	static const char * ms_TranslateComs[];
	int m_Communications;

	//Team default clearance level
	int m_Clearance;

	//Team default alarm level (i.e. how serious team members take team alarms)
	int m_AlarmLevel;

public:
	CWObject_Team();

	//Get object "parameters" from keys
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	//Set up default relations (done in second pass since teams may get created in first pass)
	virtual void OnSpawnWorld2();

	//Object message interface
	virtual aint OnMessage(const CWObject_Message& _Msg);

	//Get default relation (see CAI_AgentInfo) for this team versus the given object (agent)
	int DefaultRelation(int _iObj);

	//Relay message by normal communication to all friendlies except sender of message
	void Communicate(const CWObject_Message& _Msg);
};


#endif

