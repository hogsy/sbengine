#include "PCH.h"
#include "../AICore.h"


//The avoid behaviour.

//Checks if the given server index points to a valid scary person
CWObject * CAI_Behaviour_Avoid::IsValidScaryPerson(int _iObj)
{
	MAUTOSTRIP(CAI_Behaviour_Avoid_IsValidScaryPerson, NULL);
	if (!m_pAI)
		return NULL;

    //Is the indexed character a spotted hostile that have spotted us?
	CWObject * pAgent = m_pAI->IsValidAgent(_iObj);	
	CAI_AgentInfo * pInfo = m_pAI->m_KB.GetAgentInfo(_iObj);
	if (pAgent &&
		pInfo &&
		(pInfo->GetAwareness() >= CAI_AgentInfo::SPOTTED) &&
		(pInfo->GetAwarenessOfUs() >= CAI_AgentInfo::SPOTTED) &&
		(pInfo->GetRelation() >= CAI_AgentInfo::HOSTILE)
	   ) 
		return pAgent;
	else
		return NULL;
};


//Return position towards closest friend or ally who does not have any 
//enemy nearby and who can be reached without running past enemies.
CVec3Dfp32 CAI_Behaviour_Avoid::GetEscapePosition(CWObject * _pScaryPerson)
{
	MAUTOSTRIP(CAI_Behaviour_Avoid_GetEscapePosition, CVec3Dfp32());
	//DEPRECATED...

	//The latter condition will have to wait until I've implemented the rough pathfinding though, 
	//so for now I'll just avoid running towards any spotted enemies if possible.
	
	int i;

	//Get the threats we should avoid
	TArray<CWObject *> lpThreats;
	if (_pScaryPerson)
	{
		lpThreats.Add(_pScaryPerson);
	} 
	else 
	{
		CAI_AgentCriteria AC;
		AC.Relation(CAI_AgentInfo::ENEMY);
		AC.Awareness(CAI_AgentInfo::SPOTTED);
		m_pAI->m_KB.FindAllAgents(AC, &lpThreats);
	};

	//Get our allies
	TArray<CWObject *> lpAllies;
	CAI_AgentCriteria AC;
	AC.Relation(CAI_AgentInfo::FRIENDLY, CAI_AgentInfo::FRIENDLY);
	m_pAI->m_KB.FindAllAgents(AC, &lpAllies);

	//Are there any threats?
	if (lpThreats.Len())
	{
		//Divide the directions into 22.5 degree zones and asign danger values to each zone 
		//depending on distance to and number of threats.
		int liDangerZones[16];
		for (i = 0; i < 16; i++)
			liDangerZones[i] = 0;
		//Accumulate danger.
		uint16 iDangerBase;
		uint16 iDistTerm;
		uint8 iZone;
		for (i = 0; i < lpThreats.Len(); i++)
		{
			//Each threat has a danger base which is inversersely relative to the square of the distance 
			//to the threat.
			iDistTerm = (uint16)(m_pAI->m_pGameObject->GetPosition()).DistanceSqr(lpThreats[i]->GetPosition());
			iDangerBase = 16;
			while (iDistTerm > 48*48)
			{
				iDistTerm /= 2;
				iDangerBase -= 1;
			};

			if (iDangerBase <= 0)
			{
				//Too far away to be any danger
				iDangerBase = 0;
			}
			else
			{
				CAI_AgentInfo * pInfo = m_pAI->m_KB.GetAgentInfo(lpThreats[i]->m_iObject);
				if (pInfo)
				{
					//Threats that have spotted us are more dangerous
					if (pInfo->GetAwarenessOfUs() >= CAI_AgentInfo::SPOTTED)
						iDangerBase *= 2;

					//Threats in LOS are much more dangerous
					if (pInfo->CheckInfoFlags(CAI_AgentInfo::IN_LOS))
						iDangerBase *= 3;

					//Threats that are attacking us are even more dangerous
					if (pInfo->CheckInfoFlags(CAI_AgentInfo::IS_ATTACKING_US))
						iDangerBase *= 2;
				}

				//Add threat to zone it is in, as well as both neighbouring zones
				iZone = (int)(16 * m_pAI->HeadingToPosition(m_pAI->m_pGameObject->GetPosition(), lpThreats[i]->GetPosition(), 1.0f));
				liDangerZones[iZone] += iDangerBase * 2;
				liDangerZones[(iZone + 1) % 16] += iDangerBase;
				liDangerZones[(iZone + 15) % 16] += iDangerBase;
			};
		};

		//Set negative danger in zones with no dangerous neighbours.
		for (i = 0; i < 16; i++)
		{
			if (liDangerZones[i] == 0)
			{
				int j = 1;
				while ((liDangerZones[(i + j) % 16] <= 0) &&
					   (liDangerZones[(i + 16 - j) % 16] <= 0))
					   j++;
				liDangerZones[i] = -j + 1;
			};
		};

		//Find safest zone to escape in
		int iMinDanger = liDangerZones[0];
		int iSafestZone = 0;
		for (i = 1; i < 16; i++)
		{
			if (liDangerZones[i] < iMinDanger)
			{
				iSafestZone = i;
				iMinDanger = liDangerZones[i];	
			};
		};

		//If safest zone was safe enough go to closest friend or ally in this zone,
		CVec3Dfp32 Pos = CVec3Dfp32(_FP32_MAX);	
		if ((iMinDanger < 16) &&
			lpAllies.Len())
		{
			fp32 MinDistSqr = _FP32_MAX;
			fp32 DistSqr;
			CVec3Dfp32 Temp;
			for (i = 0; i < lpAllies.Len(); i++)
			{
				if ( (iSafestZone == (int)(16 * m_pAI->HeadingToPosition(m_pAI->m_pGameObject->GetPosition(), lpAllies[i]->GetPosition(), 1.0f))) &&
					 (MinDistSqr > (DistSqr = (m_pAI->m_pGameObject->GetPosition()).DistanceSqr(lpAllies[i]->GetPosition()))) &&
					 ((Temp = m_pAI->m_PathFinder.GetPathPosition(lpAllies[i])) != CVec3Dfp32(_FP32_MAX)))
				{
					MinDistSqr = DistSqr;
					Pos = Temp;
				};
			};
		};

		//If we couldn't go towards an ally and safest zone was pretty safe, go a bit in the safest direction. 
		if ((iMinDanger < 32) &&
			(Pos == CVec3Dfp32(_FP32_MAX)))
		{
			Pos = m_pAI->FindGroundPosition(iSafestZone * 0.0625f + (0.0625f/2.0f), 96, 48, 0.03125f, 0.0625f); 
	
			//If we failed to find a ground position within the safest zone, try wider zone
			if (Pos == CVec3Dfp32(_FP32_MAX))
				Pos = m_pAI->FindGroundPosition(iSafestZone * 0.0625f + (0.0625f/2.0f), 96, 48, 0.125f, 0.5f); 
		};


		return Pos;
	} 
	else
	{
		//No known threats, move to closest friend or ally. 
		//Should be path-closest of course, will fix when rough pathfinding is in place.
		CVec3Dfp32 Pos = CVec3Dfp32(_FP32_MAX);
		if (lpAllies.Len())
		{
			fp32 MinDistSqr = (m_pAI->m_pGameObject->GetPosition()).DistanceSqr(lpAllies[0]->GetPosition());
			fp32 DistSqr;
			Pos = m_pAI->m_PathFinder.GetPathPosition(lpAllies[0]);
			CVec3Dfp32 Temp;
			for (i = 1; i < lpAllies.Len(); i++)
			{
				DistSqr = (m_pAI->m_pGameObject->GetPosition()).DistanceSqr(lpAllies[i]->GetPosition());
				if (DistSqr < MinDistSqr &&
					((Temp = m_pAI->m_PathFinder.GetPathPosition(lpAllies[i])) != CVec3Dfp32(_FP32_MAX)))
				{
					MinDistSqr = DistSqr;
					Pos = Temp;
				};
			};
		};
		return Pos;
	};
};

//Constructor
CAI_Behaviour_Avoid::CAI_Behaviour_Avoid(CAI_Core * _pAI, int _iScaryPerson, int _InvalidationDelay)
	: CAI_Behaviour(_pAI)
{
	MAUTOSTRIP(CAI_Behaviour_Avoid_ctor, MAUTOSTRIP_VOID);
	m_iScaryPerson = _iScaryPerson;
	m_InvalidationDelay = _InvalidationDelay;
	m_InvalidationTimer = 0;
	m_EscapePos = CVec3Dfp32(_FP32_MAX);
	m_iCurrentEscapeIndex = 0;
	m_Type = AVOID;
};


//Will try to get out of scary persons LOS and get far away from him, preferrably towards friends, 
//taking evasive manouevers when in LOS. Bot will only fight back if cornered.
//If no scary person is specified, the bot will try to avoid any enemy.
void CAI_Behaviour_Avoid::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Avoid_OnTakeAction, MAUTOSTRIP_VOID);
	//Can't take action if we don't have a valid AI
	if (!m_pAI || !m_pAI->IsValid())
		return;

	//Get useful weapon
	if (m_pAI->m_Weapon.GetWielded()->IsHarmless())
	{
		m_pAI->m_Weapon.SwitchToBestWeapon();
	};

	//If we have a valid sub-behaviour, it should choose the action we should take
	if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
	{
		m_spSubBehaviour->OnTakeAction();
	}
	else
	{
		//We don't have a sub-behaviour to follow
		m_spSubBehaviour = NULL;

		//Do we have a valid scary person? Otherwise any enemy who has spotted us is treated as a scary person.
		CWObject * pScaryPerson = IsValidScaryPerson(m_iScaryPerson);
		
		//Should we get a new escape position?
		if ( (m_EscapePos == CVec3Dfp32(_FP32_MAX)) ||
			 (m_pAI->m_pGameObject->GetPosition().DistanceSqr(m_EscapePos) < 32*32) )
		{
			m_EscapePos = GetEscapePosition(pScaryPerson);
		};
		
		//Run away! Take evasive manoeuvers when necessary, run straight otherwise.
		bool bCornered = false;
		if (!m_pAI->OnEvade(CVec3Dfp32(_FP32_MAX), m_EscapePos, &bCornered))
		{
			if (bCornered)
			{
				//Can't evade. Act according to personality...not implemented, just cower
				m_pAI->m_DeviceStance.Use(CAI_Device_Stance::CROUCHING);
				m_pAI->OnIdle();
			}
			//Should we step aside for someone?
			else if (m_pAI->OnStepAside(m_EscapePos))
			{
				m_pAI->ResetPathSearch();
			}
			else
			{
				//Move to escape position
				m_pAI->OnMove(m_EscapePos);
			};
		};
	};
};
 

//Resets escape position stuff and terminates subbehaviour.
void CAI_Behaviour_Avoid::Reset()
{
	MAUTOSTRIP(CAI_Behaviour_Avoid_Reset, MAUTOSTRIP_VOID);
	m_spSubBehaviour = NULL;
	m_EscapePos = CVec3Dfp32(_FP32_MAX);
	m_iCurrentEscapeIndex = 0;
	m_InvalidationTimer = 0;	
};


//Behaviour is valid when threatened by scary person.
bool CAI_Behaviour_Avoid::IsValid()
{
	MAUTOSTRIP(CAI_Behaviour_Avoid_IsValid, false);
	if (!m_pAI || !m_pAI->m_spPersonality)
		return false;

	if (IsValidScaryPerson(m_iScaryPerson))
	{
		//Always valid when there's a given scary person
		return true;
	}
	else
	{	
		if (m_pAI->m_spPersonality->IsInDanger())
		{
			//In danger, behaviour is valid and invalidation timer is reset
			m_InvalidationTimer = 0;
			return true;
		}
		else if (m_InvalidationTimer < m_InvalidationDelay)
		{
			//Not in danger, but invalidation delay isn't up yet
			m_InvalidationTimer++;
			return true;
		}
		else
		{
			//Not in danger, and invalidation delay is up. We can relax.
			return false;
		}
	}
};


//Re-init scary person
void CAI_Behaviour_Avoid::ReInit(int iParam1, int iParam2, const CVec3Dfp32& VecParam1, const CVec3Dfp32& VecParam2)
{
	MAUTOSTRIP(CAI_Behaviour_Avoid_ReInit, MAUTOSTRIP_VOID);
	Reset();	
	m_iScaryPerson = iParam1;
	m_InvalidationDelay = iParam2;
};


//Always extra high path prio
int CAI_Behaviour_Avoid::GetPathPrio(fp32 _CameraScore)
{
	MAUTOSTRIP(CAI_Behaviour_Avoid_GetPathPrio, 0);
	int Base;
	if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
		Base = m_spSubBehaviour->GetPathPrio(_CameraScore);
	else
		Base = _CameraScore * 128;

	return Base + 128;
};



