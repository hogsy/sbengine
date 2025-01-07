#include "PCH.h"
#include "WObj_Interface_AI.h"
#include "WObj_AI/AICore.h"

//Get AI interface for other object 
CWObject_Interface_AI * CWObject_Interface_AI::GetInterface_AI(int _iObj)
{
	if (m_pWServer)
	{
		CWObject * pObj = m_pWServer->Object_Get(_iObj);
		if (pObj)
			return pObj->GetInterface_AI();
	}

	return NULL;
}

//Make all bots pause/resume behaviour according to given flags.
void CWObject_Interface_AI::PauseAllAI(int _PauseFlags)
{
	if (m_pWServer)
	{
		CWObject_Message Msg(OBJMSG_GAME_GETAIRESOURCES);
		CAI_ResourceHandler * pAIRes =  (CAI_ResourceHandler *)m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
		if (pAIRes)
			pAIRes->Pause(_PauseFlags);
	}
}

void CWObject_Interface_AI::UnpauseAllAI(int _PauseFlags)
{
	if (m_pWServer)
	{
		CWObject_Message Msg(OBJMSG_GAME_GETAIRESOURCES);
		CAI_ResourceHandler * pAIRes =  (CAI_ResourceHandler *)m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
		if (pAIRes)
			pAIRes->Unpause(_PauseFlags);
	}
}


bool CWObject_Interface_AI::AI_Jump(const CMat4Dfp32* _pMat, int32 _Flags )
{
	return false;
}


