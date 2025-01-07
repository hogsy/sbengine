#include "PCH.h"
#include "WObj_Team.h"
#include "../AI_KnowledgeBase.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Team object implementation
					
	Author:			anders olsson
					
	Copyright:		Copyright Starbreeze Studios AB 2002
					
	Contents:		CWObject_Team
					
	History:		
		020902:		Created File
\*____________________________________________________________________________________________*/


//Statics
const char * CWObject_Team::ms_TranslateComs[] = 
{
	"none", "voice", "direct", NULL
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Constructor
\*____________________________________________________________________*/
CWObject_Team::CWObject_Team()
{
	m_InternalRelation = CAI_AgentInfo::FRIENDLY;
	m_lRelations.Create(CAI_AgentInfo::RELATION_INVALID, 5);	
	m_DefaultRelation = CAI_AgentInfo::NEUTRAL;
	m_Communications = VOICE;
	m_Clearance = 0;

	m_lRelNamesFromKey.SetLen(10);
	m_lRelationsFromKey.SetLen(10);
	memset(m_lRelationsFromKey.GetBasePtr(), 0, m_lRelationsFromKey.ListSize());
	m_bNoSave = true;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Get object "parameters" from keys
\*____________________________________________________________________*/
void CWObject_Team::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH4('DEFA','ULTR','ELAT','ION'): // "DEFAULTRELATION"
		{
			m_DefaultRelation = KeyValue.TranslateInt(CAI_AgentInfo::ms_TranslateRelation);
			break;
		}
	case MHASH4('INTE','RNAL','RELA','TION'): // "INTERNALRELATION"
		{
			m_InternalRelation = KeyValue.TranslateInt(CAI_AgentInfo::ms_TranslateRelation);
			break;
		}
	case MHASH4('COMM','UNIC','ATIO','NS'): // "COMMUNICATIONS"
		{
			m_Communications = KeyValue.TranslateInt(ms_TranslateComs);
			break;
		}
	case MHASH3('CLEA','RANC','E'): // "CLEARANCE"
		{
			m_Clearance = _pKey->GetThisValuei();
			break;
		}
	default:
		{
			if (KeyName.CompareSubStr("TEAM") == 0)
			{	//Insert team name at given position (TEAM0 at 0 etc)
				int iPos = (KeyName.RightFrom(4)).Val_int();
				if (m_lRelNamesFromKey.ValidPos(iPos))
					m_lRelNamesFromKey[iPos] = m_pWServer->World_MangleTargetName(KeyValue);
			}
			else if (KeyName.CompareSubStr("RELATION") == 0)
			{
				//Insert relation value at given position (RELATION0 at 0 etc)
				int iPos = (KeyName.RightFrom(8)).Val_int();
				if (m_lRelationsFromKey.ValidPos(iPos))
				{
					int Relation = KeyValue.TranslateInt(CAI_AgentInfo::ms_TranslateRelation);
					m_lRelationsFromKey[iPos] = Relation;
				}
			}
			else
 				CWObject::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Set up default relations
\*____________________________________________________________________*/
void CWObject_Team::OnSpawnWorld2()
{
	//Set up team default relation info
	for (int i = 0; i < Min(m_lRelNamesFromKey.Len(), m_lRelationsFromKey.Len()); i++)
	{
		//Assume unique names for teams...
		int Team = m_pWServer->Selection_GetSingleTarget(m_lRelNamesFromKey[i]);
		if (Team > 0)
		{
			int Relation = m_lRelationsFromKey[i];
			if ((Relation > CAI_AgentInfo::UNKNOWN) && (Relation <= CAI_AgentInfo::ENEMY))
			{
				//Valid default relation, add to hash
				m_lRelations.Add(Team, Relation);
			}
		}
	}
	m_lRelNamesFromKey.Destroy();
	m_lRelationsFromKey.Destroy();
	
	//Add internal relation to relations list
	m_lRelations.Add(m_iObject, m_InternalRelation);

	CWObject::OnSpawnWorld2();
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Object message interface
	Returns:			Suitable value or 0 if message was not handled
\*____________________________________________________________________*/
aint CWObject_Team::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_TEAM_GETRELATION:
		{
			return DefaultRelation(_Msg.m_Param0);
		}
	case OBJMSG_TEAM_COMMUNICATE:
		{
			CWObject_Message * pMsg = (CWObject_Message*)_Msg.m_pData;
			if (pMsg)
			{
				Communicate(*pMsg);
			}
			return 1;
		}
	case OBJMSG_TEAM_CLEARANCE:
		{
			return m_Clearance;
		}
	case OBJMSG_TEAM_COMMUNICATIONS:
		{
			return m_Communications;
		}
	case OBJMSG_TEAM_ALARM:
	case OBJMSG_TEAM_RANGE_ALARM:
		{
			CWObject_Message Alarm = _Msg;
			Communicate(Alarm);
			return 1;
		}
	case OBJMSG_TEAM_FIGHT:
		{
			CWObject_Message Fight = _Msg;
			Communicate(Fight);
			return 1;
		}
	case OBJMSG_TEAM_AWARE:
		{
			CWObject_Message Aware = _Msg;
			Communicate(Aware);
			return 1;
		}
	case OBJMSG_TEAM_FOUND_CORPSE:
		{
			CWObject_Message Corpse = _Msg;
			Communicate(Corpse);
			return 1;
		}
	case OBJMSG_TEAM_SEARCH_OFF:
		{
			CWObject_Message Call = _Msg;
			Communicate(Call);
			return 1;
		}

	default:
		{
			int ResultSuper = CWObject::OnMessage(_Msg);
			// return(ResultSuper);
			if ((!ResultSuper)/* && (m_Communications != NONE)*/)
			{
				//Get all agents
				CWObject_Message Msg(OBJMSG_GAME_GETAIRESOURCES);
				CAI_ResourceHandler* pAIResources = (CAI_ResourceHandler*)m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
				if (pAIResources)
				{
					CSimpleIntList* plAgents = &(pAIResources->m_KBGlobals.ms_lAgents);
					CWObject_Message IsTeamMember(OBJMSG_TEAM_BELONGTOTEAM, m_iObject);
					for (int i = 0; i < plAgents->Length(); i++)
					{
						if (plAgents->IsValid(i))
						{
							if ((_Msg.m_iSender != plAgents->Get(i)) && //Don't communicate with self
								m_pWServer->Message_SendToObject(IsTeamMember, plAgents->Get(i)))
							{
								//Team member found, communicate
								ResultSuper |= m_pWServer->Message_SendToObject(_Msg,plAgents->Get(i));
							}
						}
					}
				}
			}
			return(ResultSuper);
		}

	}

	return(0);
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Get default relation (see CAI_AgentInfo) 
						for this team versus the given object (agent)
						
	Parameters:			
		_iObj:			Object id of agent
						
	Returns:			Highest relation value (hate is stronger than 
						love ;) ) of this team's relation versus the 
						given agents teams. The only exception is that
						friendly counts as "higher" than neutral and
						unknown.
\*____________________________________________________________________*/
int CWObject_Team::DefaultRelation(int _iObj)
{
	int WorstRelation = CAI_AgentInfo::RELATION_INVALID;
	bool bFriend = false;

	//Get ai interface of object
	CWObject * pObj = m_pWServer->Object_Get(_iObj);
	CWObject_Interface_AI * pAIObj = (pObj) ? pObj->GetInterface_AI() : NULL;
	if (!pAIObj)
		return WorstRelation;

	//Get teams, and check relations versus those teams
	uint16 liTeams[32];
	uint nTeams = pAIObj->AI_GetTeams(liTeams, sizeof_buffer(liTeams));
	for (int i = 0; i < nTeams; i++)
	{
		int iTeam = liTeams[i];			
		if (iTeam)
		{
			int Relation = m_lRelations.Get(iTeam);
			if (Relation > WorstRelation)
			{
				WorstRelation = Relation;
			}
			if (Relation == CAI_AgentInfo::FRIENDLY)
			{
				bFriend = true;
			}
		}
	}

	//Default relation only applies if this team has no relation to any of the teams object belongs to.
	if (WorstRelation == CAI_AgentInfo::RELATION_INVALID)
		return m_DefaultRelation;
	else if (bFriend && (WorstRelation < CAI_AgentInfo::UNFRIENDLY))
		return CAI_AgentInfo::FRIENDLY;
	else
		return WorstRelation;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Relay message by normal communication to all 
						friendlies except sender of message
						
	Parameters:			
		_Msg:			Message to be relayed
\*____________________________________________________________________*/
void CWObject_Team::Communicate(const CWObject_Message& _Msg)
{
	// A team must be able to communicate to communicate
	// (How this could have been missed beats me)
	if (m_Communications == NONE)
	{
		return;
	}

	//Get all agents
	CWObject_Message Msg(OBJMSG_GAME_GETAIRESOURCES);
	CAI_ResourceHandler* pAIResources = (CAI_ResourceHandler*)m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
	if (pAIResources)
	{
		CSimpleIntList* plAgents = &(pAIResources->m_KBGlobals.ms_lAgents);
		if ((_Msg.m_Msg == OBJMSG_TEAM_FIGHT)||(_Msg.m_Msg == OBJMSG_TEAM_RANGE_ALARM))
		{	// This message is handled a little different as we ignore m_Communications
			//Can communicate to all team members instantly
			CWObject_Message IsTeamMember(OBJMSG_TEAM_BELONGTOTEAM, m_iObject);
			for (int i = 0; i < plAgents->Length(); i++)
			{
				if (plAgents->IsValid(i))
				{
					if ((_Msg.m_iSender != plAgents->Get(i)) && //Don't communicate with self
						m_pWServer->Message_SendToObject(IsTeamMember, plAgents->Get(i)))
					{
						//Team member found, communicate
						m_pWServer->Message_SendToObject(_Msg, plAgents->Get(i));
					}
				}
			}
		}
		else
		{
			switch (m_Communications)
			{
			case VOICE:
				{
					//Can communicate to all team members that hear this
					CWObject_Message IsTeamMember(OBJMSG_TEAM_BELONGTOTEAM,m_iObject);
					CWObject_Message Hear(OBJMSG_TEAM_HEARVOICE);
					Hear.m_iSender = _Msg.m_iSender;
					for (int i = 0; i < plAgents->Length(); i++)
					{
						if (plAgents->IsValid(i))
						{
							if ((_Msg.m_iSender != plAgents->Get(i))&& //Don't communicate with self
								m_pWServer->Message_SendToObject(IsTeamMember, plAgents->Get(i))&&
								m_pWServer->Message_SendToObject(Hear, plAgents->Get(i)))
							{
								//Team member found, communicate
								m_pWServer->Message_SendToObject(_Msg, plAgents->Get(i));
							}
						}
					}
				}
				break;
			case DIRECT:
				{
					//Can communicate to all team members instantly
					CWObject_Message IsTeamMember(OBJMSG_TEAM_BELONGTOTEAM, m_iObject);
					for (int i = 0; i < plAgents->Length(); i++)
					{
						if (plAgents->IsValid(i))
						{
							if ((_Msg.m_iSender != plAgents->Get(i)) && //Don't communicate with self
								m_pWServer->Message_SendToObject(IsTeamMember, plAgents->Get(i)))
							{
								//Team member found, communicate
								m_pWServer->Message_SendToObject(_Msg, plAgents->Get(i));
							}
						}
					}
				}
				break;
			}
		}
	}
};


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Team, CWObject, 0x0100);
