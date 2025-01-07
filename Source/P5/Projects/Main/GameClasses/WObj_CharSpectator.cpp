#include "PCH.h"

//#ifndef M_RTM

#include "WObj_CharSpectator.h"
#include "WRPG/WRPGChar.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Spectator and SpectatorIntermission
\*____________________________________________________________________________________________*/

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Spectator
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CWObject_Spectator::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_Spectator_OnFinishEvalKeys, MAUTOSTRIP_VOID);
	if(!Char())
	{
		m_spRPGObject = (CRPG_Object_Char *)(CRPG_Object *)CRPG_Object::CreateObject("Char", m_pWServer);
		if(!m_spRPGObject)
			Error("OnFinishEvalKeys", "Could not create default RPG object");
	}

	SpawnCharacter(PLAYER_PHYS_SPECTATOR);
}

void CWObject_Spectator::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	if(_Msg.m_MsgType == PLAYER_NETMSG_MP_GETTEMPLATES)
	{
		CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		CRegistry *pGlobalOpt = pSys->GetRegistry()->FindChild("OPT");
		CStr SelectedTemplateHuman = pGlobalOpt->GetValue("MP_HUMANTEMPLATE", "multiplayer_Mob_01");
		CStr SelectedTemplateDarkling = pGlobalOpt->GetValue("MP_DARKLINGTEMPLATE", "multiplayer_Darkling_05");

		int State = _pWClient->GetMapData()->GetState();
		_pWClient->GetMapData()->SetState(State & ~WMAPDATA_STATE_NOCREATE);
		int iRc = _pWClient->GetMapData()->GetResourceIndex_Registry("Registry\\Sv");
		_pWClient->GetMapData()->SetState(State);
		CRegistry *pReg = _pWClient->GetMapData()->GetResource_Registry(iRc);

		if(pReg)
		{
			CRegistry *pTemplates = pReg->FindChild("TEMPLATES");
			CRegistry *pTemlate = pTemplates->FindChild(SelectedTemplateDarkling.Str());

			if(pTemlate)
				ConExecute(CStrF("mp_setdarklingmodelname(\"%s\")", pTemlate->GetValue("ModelDarkling").Str()));
			ConExecute(CStrF("mp_setmodelname(\"%s\")", SelectedTemplateHuman.Str()));
		}

		return;
	}
	return CWObject_Character::OnClientNetMsg(_pObj, _pWClient, _Msg);
}

void CWObject_Spectator::OnRefresh()
{
	MAUTOSTRIP(CWObject_Spectator_OnRefresh, MAUTOSTRIP_VOID);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD) return;

	pCD->m_GameTick += 1;
	int Diff = pCD->m_GameTick - m_pWServer->GetGameTick();
	if(Diff > 0 || Diff < -4)
		pCD->m_GameTick = m_pWServer->GetGameTick();
	Char_SetGameTickDiff(pCD->m_GameTick - m_pWServer->GetGameTick());
	pCD->m_GameTime = PHYSSTATE_TICKS_TO_TIME(pCD->m_GameTick, m_pWServer);

//	pCD->m_GameTime += CMTime::CreateFromSeconds(SERVER_TIMEPERFRAME);

	if (!OnPumpControl()) return;

	m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}

void CWObject_Spectator::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Spectator_OnClientRefresh, MAUTOSTRIP_VOID);
}

aint CWObject_Spectator::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Spectator_OnClientMessage, 0);
/*	switch(_Msg.m_Msg)
	{
	case OBJSYSMSG_GETCAMERA:
		{
			int Res = OnGetCamera(_pObj, _pWClient, _Msg);
			if (_Msg.m_DataSize != sizeof(CMat4Dfp32)) return 0;
			CMat4Dfp32& Camera = *(CMat4Dfp32*)_Msg.m_pData;
			Camera.RotX_x_M(-0.25f);
			Camera.RotY_x_M(0.25f);
			return Res;
		}
	}*/

	return CWObject_Character::OnClientMessage(_pObj, _pWClient, _Msg);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Spectator, CWObject_CharPlayer, 0x0100);


void CWObject_SpectatorIntermission::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_SpectatorIntermission_OnFinishEvalKeys, MAUTOSTRIP_VOID);
	CWObject_Spectator::OnFinishEvalKeys();
	m_ClientFlags |= PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK;
}

aint CWObject_SpectatorIntermission::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_SpectatorIntermission_OnClientMessage, 0);
	switch(_Msg.m_Msg)
	{
	case OBJSYSMSG_GETCAMERA:
		{
			CMat4Dfp32& Camera = *(CMat4Dfp32*)_Msg.m_pData;
			Camera = _pObj->GetPositionMatrix();

			int iParent = _pWClient->Object_GetParent(_pObj->m_iObject);
			if(iParent > 0)
			{
				int iParentParent = _pWClient->Object_GetParent(iParent);
				if(iParentParent > 0)
					iParent = iParentParent;

				_pWClient->ClientMessage_SendToObject(_Msg, iParent);
			}
			
			Camera.RotX_x_M(-0.25f);
			Camera.RotY_x_M(0.25f);

			return 1;
		}
	}
	
	return CWObject_Character::OnClientMessage(_pObj, _pWClient, _Msg);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_SpectatorIntermission, CWObject_Spectator, 0x0100);

//#endif
