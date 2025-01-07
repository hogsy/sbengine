/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character class for Angelus characters.

	Author:			Patrik Willbo

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_CharAngelus implementation

	Comments:

	History:		
		051114		Created file
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_CharAngelus.h"
#include "WObj_CharAngelus_ClientData.h"

#include "../WObj_Char.h"
#include "../WObj_CharMsg.h"
#include "../WRPG/WRPGItem.h"
#include "../WRPG/WRPGChar.h"
#include "../WObj_Misc/WObj_Shell.h"
#include "../WObj_Misc/WObj_AngelusChain.h"
#include "../WObj_Misc/WObj_EffectSystem.h"
#include "../WObj_Misc/WObj_ActionCutscene.h"
#include "../WObj_Misc/WObj_AnimEventListener.h"
#include "../WObj_Misc/WObj_ActionCutsceneCamera.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WDataRes_FacialSetup.h"


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_CharAngelus, CWObject_CharAngelus::parent, 0x0100);


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_CharAngelus
|__________________________________________________________________________________________________
\*************************************************************************************************/
const CWO_CharAngelus_ClientData& CWObject_CharAngelus::GetClientData(const CWObject_CoreData* _pObj)
{
	M_ASSERT(_pObj, "[CWO_CharAngelus_ClientData] Bad this-pointer!");
	const CWO_CharAngelus_ClientData* pCD = safe_cast<const CWO_CharAngelus_ClientData>((const CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	M_ASSERT(pCD, "[CWObject_CharAngelus] No clientdata?!");
	return *pCD;
}


CWO_CharAngelus_ClientData& CWObject_CharAngelus::GetClientData(CWObject_CoreData* _pObj)
{
	M_ASSERT(_pObj, "[CWO_CharAngelus_ClientData] Bad this-pointer!");
	CWO_CharAngelus_ClientData* pCD = safe_cast<CWO_CharAngelus_ClientData>((CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	M_ASSERT(pCD, "[CWObject_CharAngelus] No clientdata?!");
	return *pCD;
}


void CWObject_CharAngelus::OnCreate()
{
	m_spRPGObject_AuraWeapon = NULL;
	m_iChain = 0;

	parent::OnCreate();
}


void CWObject_CharAngelus::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA];
	CWO_CharAngelus_ClientData* pCD = TDynamicCast<CWO_CharAngelus_ClientData>(pData);

	if (!pCD || pCD->m_pObj != _pObj || pCD->m_pWPhysState != _pWPhysState)
	{
		pCD = MNew(CWO_CharAngelus_ClientData);
		if (!pCD)
			Error_static("CWObject_CharAngelus", "Could not allocate client data!");

		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = pCD;
		pCD->Clear(_pObj, _pWPhysState);
	}

	if (!InitClientObjects(_pObj, _pWPhysState))
		Error_static("CWObject_CharAngelus", "InitClientObjects failed");
}


void CWObject_CharAngelus::OnDestroy()
{
	CWO_CharAngelus_ClientData& CD = GetClientData();
	
	if (CD.m_iAuraEffect)
		m_pWServer->Object_Destroy(CD.m_iAuraEffect);

	if (m_iChain)
		m_pWServer->Object_Destroy(m_iChain);

	m_spRPGObject_AuraWeapon = NULL;

	parent::OnDestroy();
}


void CWObject_CharAngelus::OnInitInstance(const aint* _pParam, int _nParam)
{
	CWO_CharAngelus_ClientData& CD = GetClientData();
	if (_pParam && _nParam > 0)
		CD.m_iPlayer = (int16)_pParam[0];
}


void CWObject_CharAngelus::OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer)
{
	IncludeClassFromKey("AURA_EFFECT", _pReg, _pMapData);
	IncludeClassFromKey("CHAINSYSTEM", _pReg, _pMapData);
}


void CWObject_CharAngelus::OnIncludeClass(CMapData* _pWData, CWorld_Server* _pWServer)
{
	CWObject_AngelusChain::OnIncludeClass(_pWData, _pWServer);
}


void CWObject_CharAngelus::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	CWO_CharAngelus_ClientData& CD = GetClientData();
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH3('AURA','_EFF','ECT'): // "AURA_EFFECT"
		{
			CD.m_iAuraEffect = m_pWServer->Object_Create(KeyValue, GetPositionMatrix(), m_iObject);
			break;
		}
	case MHASH4('AURA','_RPG','OBJE','CT'): // "AURA_RPGOBJECT"
		{
			// Duplicate the registry and create aura weapon
			spCRegistry spRPGObjectReg = _pKey->Duplicate();
			m_spRPGObject_AuraWeapon = CRPG_Object::CreateObject(spRPGObjectReg, m_pWServer);
			spRPGObjectReg = NULL;
			break;
		}
	case MHASH5('AURA','_RAM','PING','_TIM','E'): // "AURA_RAMPING_TIME"
		{
			int iGameObj = m_pWServer->Game_GetObjectIndex();
			CWObject_Message Msg(OBJMSG_GAME_GETDIFFICULTYLEVEL);
			aint Difficulty = m_pWServer->Message_SendToObject(Msg, iGameObj);

			switch(Difficulty)
			{
				case DIFFICULTYLEVEL_UNDEFINED:
				{
					const CRegistry* pDiff = _pKey->FindChild("UNDEFINED");
					CD.m_AuraRampingTime = (pDiff) ? (fp32)pDiff->GetThisValue().Val_fp64() : 4.0f;
				}
				break;

				case DIFFICULTYLEVEL_EASY:
				{
					const CRegistry* pDiff = _pKey->FindChild("EASY");
					CD.m_AuraRampingTime = (pDiff) ? (fp32)pDiff->GetThisValue().Val_fp64() : 4.0f;
				}
				break;

				case DIFFICULTYLEVEL_NORMAL:
				{
					const CRegistry* pDiff = _pKey->FindChild("NORMAL");
					CD.m_AuraRampingTime = (pDiff) ? (fp32)pDiff->GetThisValue().Val_fp64() : 4.0f;
				}
				break;

				case DIFFICULTYLEVEL_HARD:
				{
					const CRegistry* pDiff = _pKey->FindChild("HARD");
					CD.m_AuraRampingTime = (pDiff) ? (fp32)pDiff->GetThisValue().Val_fp64() : 4.0f;
				}
				break;
			}
			break;
		}
	case MHASH5('AURA','_RES','TING','_TIM','E'): // "AURA_RESTING_TIME"
		{
			int iGameObj = m_pWServer->Game_GetObjectIndex();
			CWObject_Message Msg(OBJMSG_GAME_GETDIFFICULTYLEVEL);
			aint Difficulty = m_pWServer->Message_SendToObject(Msg, iGameObj);

			switch(Difficulty)
			{
				case DIFFICULTYLEVEL_UNDEFINED:
				{
					const CRegistry* pDiff = _pKey->FindChild("UNDEFINED");
					CD.m_AuraRestingTime = (pDiff) ? (fp32)pDiff->GetThisValue().Val_fp64() : 4.0f;
				}
				break;

				case DIFFICULTYLEVEL_EASY:
				{
					const CRegistry* pDiff = _pKey->FindChild("EASY");
					CD.m_AuraRestingTime = (pDiff) ? (fp32)pDiff->GetThisValue().Val_fp64() : 4.0f;
				}
				break;

				case DIFFICULTYLEVEL_NORMAL:
				{
					const CRegistry* pDiff = _pKey->FindChild("NORMAL");
					CD.m_AuraRestingTime = (pDiff) ? (fp32)pDiff->GetThisValue().Val_fp64() : 4.0f;
				}
				break;

				case DIFFICULTYLEVEL_HARD:
				{
					const CRegistry* pDiff = _pKey->FindChild("HARD");
					CD.m_AuraRestingTime = (pDiff) ? (fp32)pDiff->GetThisValue().Val_fp64() : 4.0f;
				}
				break;
			}
			break;
		}
	case MHASH3('CHAI','NSYS','TEM'): // "CHAINSYSTEM"
		{
			m_iChain = m_pWServer->Object_Create(KeyValue, GetPosition(), m_iObject);
			break;
		}
	default:
		{
			parent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}


void CWObject_CharAngelus::OnFinishEvalKeys()
{
	CWO_CharAngelus_ClientData& CD = GetClientData();
	
	// Send setup to effect
	CWObject_EffectSystem* pAuraEffect = (CD.m_iAuraEffect) ? safe_cast<CWObject_EffectSystem>(m_pWServer->Object_Get(CD.m_iAuraEffect)) : NULL;
	
	if(pAuraEffect)
	{
		CWO_EffectSystem_ClientData& AuraCD = CWObject_EffectSystem::GetClientData(pAuraEffect);

		AuraCD.PauseSystem(20);
		AuraCD.SetWaitSpawn((m_Flags & PLAYER_FLAGS_WAITSPAWN) ? true : false);
		
		//CWObject_Message Msg(OBJMSG_EFFECTSYSTEM_PAUSETIME, 0, 20);
		//m_pWServer->Message_SendToObject(Msg, CD.m_iAuraEffect);

		// Send wait spawn message
		//Msg.m_Msg = OBJMSG_EFFECTSYSTEM_WAITSPAWN;
		//if(m_Flags & PLAYER_FLAGS_WAITSPAWN)
		//	Msg.m_Param0 = 1;
		//else
		//	Msg.m_Param0 = 0;
		//m_pWServer->Message_SendToObject(Msg, CD.m_iAuraEffect);
	}

	parent::OnFinishEvalKeys();
}


void CWObject_CharAngelus::SpawnCharacter(int _PhysMode, int _SpawnBehaviour)
{
	parent::SpawnCharacter(_PhysMode, _SpawnBehaviour);
	CWO_CharAngelus_ClientData& CD = GetClientData();
	
	CWObject_EffectSystem* pAuraEffect = (CD.m_iAuraEffect) ? safe_cast<CWObject_EffectSystem>(m_pWServer->Object_Get(CD.m_iAuraEffect)) : NULL;
	
	if(pAuraEffect)
	{
		CWO_EffectSystem_ClientData& AuraCD = CWObject_EffectSystem::GetClientData(pAuraEffect);
		AuraCD.SetWaitSpawn(false);
		//CWObject_Message Msg(OBJMSG_EFFECTSYSTEM_WAITSPAWN, 0);
		//m_pWServer->Message_SendToObject(Msg, CD.m_iAuraEffect);
	}
}


void CWObject_CharAngelus::UnspawnCharacter()
{
	parent::UnspawnCharacter();
	CWO_CharAngelus_ClientData& CD = GetClientData();
	CWObject_EffectSystem* pAuraEffect = (CD.m_iAuraEffect) ? safe_cast<CWObject_EffectSystem>(m_pWServer->Object_Get(CD.m_iAuraEffect)) : NULL;
	
	if(pAuraEffect)
	{
		CWO_EffectSystem_ClientData& AuraCD = CWObject_EffectSystem::GetClientData(pAuraEffect);
		AuraCD.SetWaitSpawn(true);
		//CWObject_Message Msg(OBJMSG_EFFECTSYSTEM_WAITSPAWN, 1);
		//m_pWServer->Message_SendToObject(Msg, CD.m_iAuraEffect);
	}
}


aint CWObject_CharAngelus::OnMessage(const CWObject_Message& _Msg)
{
	//Let AI catch any messages first
	if(m_spAI->OnMessage(_Msg))
		return true;

	CWO_CharAngelus_ClientData& CD = GetClientData();
	switch(_Msg.m_Msg)
	{
	case OBJMSG_CHAR_ONANIMEVENT:
		{
			CWO_CharAngelus_ClientData& CD = GetClientData();
			CXR_Anim_DataKey *pKey = (CXR_Anim_DataKey *)_Msg.m_Param0;

			if(pKey)
			{
				if(pKey->m_Type == ANIM_EVENT_TYPE_GAMEPLAY && pKey->m_Param == AG2_ANIMEVENT_GAMEPLAY_WEAPON_FIRESECONDARY)
				{
					// Handle chain tentacle here
					CWObject* pChainObj = (m_iChain) ? m_pWServer->Object_Get(m_iChain) : NULL;
					if(pChainObj)
					{
						CWObject_AngelusChain& Chain = *safe_cast<CWObject_AngelusChain>(m_pWServer->Object_Get(m_iChain));
						CWObject* pSelection = Chain.SelectTarget();
						CWObject_Message Msg(OBJMSG_CHAR_ANGELUS_TENTACLE_SETSTATE);
						const int iClassCharPlayer = m_pWServer->GetMapData()->GetResourceIndex_Class("CharPlayer");

						// Player is hit
						if (pSelection && pSelection->m_iClass == iClassCharPlayer)
						{
							Msg.m_Param0 = ANGELUS_TENTACLESTATE_HITTARGET;
							
							// Update chain activity
							Chain.ReachPosition(pSelection->GetPosition());
						}
						else if (pSelection)
						{
							Msg.m_Param0 = ANGELUS_TENTACLESTATE_HITOBJECT;

							// Update chain activity
							Chain.ReachPosition(pSelection->GetPosition());
						}
						else
							Msg.m_Param0 = ANGELUS_TENTACLESTATE_IDLE;

						// Send message to ourself to notify AI about what's going on
						m_pWServer->Message_SendToObject(Msg, m_iObject);
					}
				}
				else if(pKey->m_Type == ANIM_EVENT_TYPE_EFFECT && pKey->m_Param == 1)
				{
					CStr KeyData(pKey->Data());
					KeyData.MakeUpperCase();
					CStr KeyName = KeyData.GetStrSep(",");

					if(KeyName == "ACTIVATEWEAPON")
					{
						// Angelus has lot's of cool invisible weapons she can use too =)
						if(m_spRPGObject_AuraWeapon)
						{
							CRPG_Object_Char* pAuraWeapon = ((CRPG_Object_Char *)(CRPG_Object *)m_spRPGObject_AuraWeapon);
							CRPG_Object_Item* pItem = pAuraWeapon->FindItemByName(KeyData.GetStrSep(","));
							if(pItem)
							{
								// Force force force force
								pItem->Activate(GetPositionMatrix(), pAuraWeapon, m_iObject, 1);
							}
						}
					}
				}
			}

			return parent::OnMessage(_Msg);
		}

	case OBJMSG_CHAR_ANGELUS_AURA_GETSTATE:
		{
			return (aint)CD.m_AuraState;
		}

	case OBJMSG_CHAR_ANGELUS_AURA_SETSTATE:
		{
			CD.m_AuraStateTick = m_pWServer->GetGameTick();
			CD.m_AuraState = (uint8)_Msg.m_Param0;

			// Send timescale to any of the effect objects if needed
			CWObject_EffectSystem* pAuraEffect = (CD.m_iAuraEffect) ? safe_cast<CWObject_EffectSystem>(m_pWServer->Object_Get(CD.m_iAuraEffect)) : NULL;
	
			if(pAuraEffect)
			{
				CWO_EffectSystem_ClientData& AuraCD = CWObject_EffectSystem::GetClientData(pAuraEffect);
				switch(CD.m_AuraState)
				{
					case ANGELUS_AURASTATE_RAMPING:
					{
						AuraCD.UnpauseSystem();
						AuraCD.SetTimeScale(CD.m_AuraRampingTime);
                        //pAuraEffect->SetTimeControl(CD.m_AuraRampingTime, 0);
						//CWObject_Message Msg(OBJMSG_EFFECTSYSTEM_SETTIMECONTROL, (aint)&CD.m_AuraRampingTime, 0);
						//m_pWServer->Message_SendToObject(Msg, CD.m_iAuraEffect);
					}
					break;

					case ANGELUS_AURASTATE_USING:
					{
						AuraCD.UnpauseSystem();
						AuraCD.SetTimeScale(1.0f);
						//pAuraEffect->SetTimeControl(1.0f, 20);
						//const fp32 Normal = 1.0f;
						//CWObject_Message Msg(OBJMSG_EFFECTSYSTEM_SETTIMECONTROL, (aint)&Normal, 20);
						//m_pWServer->Message_SendToObject(Msg, CD.m_iAuraEffect);
					}
					break;

					case ANGELUS_AURASTATE_IDLE:
					{
						AuraCD.PauseSystem(20);
						//CWObject_Message Msg(OBJMSG_EFFECTSYSTEM_PAUSETIME, 0, 20);
						//m_pWServer->Message_SendToObject(Msg, CD.m_iAuraEffect);
					}
					break;

					case ANGELUS_AURASTATE_RESTING:
					{
						AuraCD.PauseSystem(0);
						//CWObject_Message Msg(OBJMSG_EFFECTSYSTEM_PAUSETIME, 0, 0);
						//m_pWServer->Message_SendToObject(Msg, CD.m_iAuraEffect);
					}
				}
			}

			return 1;
		}

	case OBJMSG_CHAR_ANGELUS_AURA_GETRAMPTIME:
		{
			return (aint)&CD.m_AuraRampingTime;
		}

	case OBJMSG_CHAR_ANGELUS_AURA_GETRESTTIME:
		{
			return (aint)&CD.m_AuraRestingTime;
		}

	case OBJMSG_CHAR_ANGELUS_AURA_GETSTATETICK:
		{
			return (aint)CD.m_AuraStateTick;
		}
	}

	return parent::OnMessage(_Msg);
}


aint CWObject_CharAngelus::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
		case OBJMSG_CHAR_ONANIMEVENT:
		{
			CWO_CharAngelus_ClientData& CD = GetClientData(_pObj);
			CXR_Anim_DataKey *pKey = (CXR_Anim_DataKey *)_Msg.m_Param0;

			if(pKey && pKey->m_Type == ANIM_EVENT_TYPE_EFFECT && pKey->m_Param == 1)
			{
				CMTime EventTime = (_Msg.m_pData && _Msg.m_DataSize >= sizeof(CMTime) ? *(CMTime*)_Msg.m_pData : CD.m_GameTime);
				CStr KeyData(pKey->Data());
				
				const CStr Type = KeyData.GetStrSep(",").UpperCase();
				if(Type == "CLOTHANIM")
				{
					// Setup cloth->anim/anim->cloth parameters
					CD.m_AnimClothParam = (uint8)KeyData.GetStrSep(",").Val_int();
					CD.m_AnimClothTime = EventTime;
					CD.m_AnimClothBlendTime = 1.0f / (fp32)KeyData.GetStrSep(",").Val_fp64();
					return 1;
				}
				
				// Offset eventtime by a tick otherwise it might start late
				//EventTime += CMTime::CreateFromTicks(1,_pWClient->GetGameTickTime());
			}
		}
		break;
	}

	return parent::OnClientMessage(_pObj, _pWClient, _Msg);
}


void CWObject_CharAngelus::OnRefresh()
{
	CWO_CharAngelus_ClientData& CD = GetClientData();

	// Handle Angulus aura (the visible charging effect)
	fp32 TicksPerSecond = m_pWServer->GetGameTicksPerSecond();
	
	int32 GameTick = m_pWServer->GetGameTick();
	switch(CD.m_AuraState)
	{
		case ANGELUS_AURASTATE_RAMPING:
		{
			if((GameTick - CD.m_AuraStateTick) >= CD.m_AuraRampingTime * TicksPerSecond)
			{
				CWObject_Message Msg(OBJMSG_CHAR_ANGELUS_AURA_SETSTATE, ANGELUS_AURASTATE_IDLE);
				m_pWServer->Message_SendToObject(Msg, m_iObject);
			}
		}
		break;

		case ANGELUS_AURASTATE_RESTING:
		{
			if((GameTick - CD.m_AuraStateTick) >= CD.m_AuraRestingTime * TicksPerSecond)
			{
				CWObject_Message Msg(OBJMSG_CHAR_ANGELUS_AURA_SETSTATE, ANGELUS_AURASTATE_RAMPING);
				m_pWServer->Message_SendToObject(Msg, m_iObject);
			}
		}
		break;

		case ANGELUS_AURASTATE_USING:
		{
			if((GameTick - CD.m_AuraStateTick) >= TicksPerSecond * 1.5f)
			{
				CWObject_Message Msg(OBJMSG_CHAR_ANGELUS_AURA_SETSTATE, ANGELUS_AURASTATE_RESTING);
				m_pWServer->Message_SendToObject(Msg, m_iObject);
			}
		}
		break;
	}

	parent::OnRefresh();
}


void CWObject_CharAngelus::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_CharAngelus::OnClientRefresh);
	CWO_CharAngelus_ClientData& CD = GetClientData(_pObj);

	// Switch blend mode when it's time to do so.
	switch(CD.m_AnimClothParam)
	{
		case 0:
		{
			const CMTime BlendTime = _pWClient->GetGameTime() - CD.m_AnimClothTime;
			if(BlendTime.GetTime() * CD.m_AnimClothBlendTime >= 1.0f)
				CD.m_AnimClothParam = 3;
		}
		break;

		case 1:
		{
			const CMTime BlendTime = _pWClient->GetGameTime() - CD.m_AnimClothTime;
			if(BlendTime.GetTime() * CD.m_AnimClothBlendTime >= 1.0f)
				CD.m_AnimClothParam = 2;
		}
		break;
	}

	parent::OnClientRefresh(_pObj, _pWClient);
}


void CWObject_CharAngelus::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	// Copied from WObject_Character::OnClientRender with player removed stuff
	MAUTOSTRIP(CWObject_CharAngelus_OnClientRender, MAUTOSTRIP_VOID);

	if(Char_GetPhysType(_pObj) == PLAYER_PHYS_NOCLIP || _pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_BRIEFING)
		return;
	
	CRegistry* pUserReg = _pWClient->Registry_GetUser();
#ifdef M_RTM
	int bNoRender = (pUserReg) ? pUserReg->GetValuei("NOCHARRENDER") : 0;
	if (bNoRender)
		return;
#endif
	
	MSCOPE(CWObject_CharAngelus::OnClientRender, CHARACTER);

	CWO_CharAngelus_ClientData& CD = GetClientData(_pObj);
	if(CD.m_ExtraFlags & PLAYER_EXTRAFLAGS_HIDDEN)
		return;

	int iPass = _pEngine->GetVCDepth();
	bool bMirrored = (_pEngine->GetVCDepth() > 0);
	bool bCutSceneView = (_pObj->m_ClientFlags & (PLAYER_CLIENTFLAGS_CUTSCENE | PLAYER_CLIENTFLAGS_DIALOGUE)) != 0;
	bool bThirdPerson = (CD.m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON | PLAYER_EXTRAFLAGS_FORCETHIRPERSON)) != 0;
	bool bZooming = ((CD.m_MaxZoom > 1.0f) && (CD.m_Camera_CurBehindOffset.LengthSqr() < Sqr(50.0f)));

	// Do whatever interpolation or extrapolation we should do.
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	CMat4Dfp32 MatIP;
	{
		if (CD.m_LastTeleportTick == CD.m_GameTick)
		{
			MatIP = _pObj->GetPositionMatrix();
		}
		else
		{
			fp64 PeriodScale = _pWClient->GetModeratedFramePeriod() * _pWClient->GetGameTicksPerSecond();
			Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime / PeriodScale);
		}
	}

	if(CD.m_RenderAttached != 0)
	{
		CWObject_Client *pObj = _pWClient->Object_Get(CD.m_RenderAttached);
		if(pObj)
		{
			IPTime = _pWClient->GetRenderTickFrac();
			CWObject_Message Msg(OBJMSG_HOOK_GETCURRENTMATRIX, (aint)&MatIP);

			if(!_pWClient->ClientMessage_SendToObject(Msg, CD.m_RenderAttached))
				Interpolate2(pObj->GetLastPositionMatrix(), pObj->GetPositionMatrix(), MatIP, IPTime);
			_pWClient->Debug_RenderMatrix(MatIP, 0.05f, false);
		}
	}

	if(_pObj->m_iModel[0] == 0)
		return;

	// Get all models and skeletons and create anim track mask
	CXR_Model* lpObjectModels[CWO_NUMMODELINDICES];
	CXR_Skeleton* lpObjectModelSkel[CWO_NUMMODELINDICES];
	int liVariations[CWO_NUMMODELINDICES];
#ifndef M_RTM
	int liLOD[CWO_NUMMODELINDICES];
	memset(liLOD, 0, sizeof(liLOD));
#endif

	{
		for(int i = 0; i < CWO_NUMMODELINDICES; i++)
		{
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
			if (!pModel)
			{
				lpObjectModels[i] = NULL;
				lpObjectModelSkel[i] = NULL;
				continue;
			}

			// Since the code below manually fetches the LOD models, we need to manually resolve variation index aswell
			CXR_AnimState Tmp;
			pModel = pModel->OnResolveVariationProxy(Tmp, Tmp);
			liVariations[i] = Tmp.m_Variation;

			{
#ifndef M_RTM
				pModel = pModel->GetLOD(MatIP, _pEngine->GetVC()->m_W2VMat, _pEngine, &liLOD[i]);
#else
				pModel = pModel->GetLOD(MatIP, _pEngine->GetVC()->m_W2VMat, _pEngine);
#endif
			}
			lpObjectModels[i] = pModel;

			CXR_Skeleton* pSkel = pModel->GetSkeleton();
			lpObjectModelSkel[i] = pSkel;
		}
	}

	CXR_Model* pModel = lpObjectModels[0];
	CXR_Skeleton* pSkel = lpObjectModelSkel[0];
	if (!pSkel)
		return;

	// Init animstate
	CVec3Dfp32 AnimatedPos;
	CXR_AnimState Anim;
	Anim.m_pSkeletonInst	= CXR_VBMHelper::Alloc_SkeletonInst(_pEngine->GetVBM(), pSkel->m_lNodes.Len(), pSkel->m_nUsedRotations, pSkel->m_nUsedMovements);
	if (!Anim.m_pSkeletonInst)
		return;

	OnGetAnimState(_pObj, _pWClient, pSkel, 0, MatIP, IPTime, Anim, &AnimatedPos, true, -1.0f);
	Anim.m_pModelInstance = _pObj->m_lModelInstances[0];

	const fp32 GameTickTime = _pWClient->GetGameTickTime();

	// WB hack
	if(CD.m_Anim_FreezeTick != 0)
		Anim.m_AnimTime0 = CMTime::CreateFromTicks(CD.m_GameTick - CD.m_Anim_FreezeTick, GameTickTime, IPTime);

	uint32 GlobalOnRenderFlags = 0;
	if(CD.m_ElectroShockTick != 0)
	{
		CMTime Duration = CD.m_GameTime - CMTime::CreateFromTicks(CD.m_ElectroShockTick, GameTickTime);
		if(Duration.GetTime() < 4)
		{
			CXW_Surface* pSurf = _pEngine->GetSC()->GetSurface("ELECTROSHOCK");
			if(pSurf)
			{
				Anim.m_lpSurfaces[1] = pSurf;
				Anim.m_AnimTime1 = Duration;
				GlobalOnRenderFlags |= CXR_MODEL_ONRENDERFLAGS_SURF0_ADD;
			}
		}
	}

	// Angelus "skin" glow effect
	int32 GameTick = (int32)_pWClient->GetGameTick();
	switch(CD.m_AuraState)
	{
		case ANGELUS_AURASTATE_IDLE:
			Anim.m_AnimTime0 = CMTime::CreateFromSeconds(1.0f);
			//Anim.m_AnimTime0.Reset();
			//Anim.m_Data[0] = 0x80808080;
			//Anim.m_Data[1] = 0x00000000;
			break;

		case ANGELUS_AURASTATE_RAMPING:
		{
			const fp32 RampTime = MinMT(1.0, (((GameTick - CD.m_AuraStateTick) * GameTickTime) / CD.m_AuraRampingTime) + ((GameTickTime * IPTime) / CD.m_AuraRampingTime));
			Anim.m_AnimTime0 = CMTime::CreateFromSeconds(RampTime);
//			Anim.m_AnimTime0.Reset();
//			uint8 Color = (uint8)TruncToInt(MinMT(128.0f, RampTime * 128.0f));
//			Anim.m_Data[0] = (Color << 24) | (Color << 16) | (Color << 8) | (Color);
//			Anim.m_Data[1] = 0x00000000;
		}
		break;

		case ANGELUS_AURASTATE_RESTING:
			Anim.m_AnimTime0 = CMTime::CreateFromSeconds(0.0f);
//			Anim.m_AnimTime0.Reset();
//			Anim.m_Data[0] = 0x00000000;
//			Anim.m_Data[1] = 0x00000000;
			break;

		case ANGELUS_AURASTATE_USING:
		{
			const fp32 UseTime = 1.0f + (((GameTick - CD.m_AuraStateTick) * GameTickTime) + ((GameTickTime * IPTime) / CD.m_AuraRampingTime));
			Anim.m_AnimTime0 = CMTime::CreateFromSeconds(UseTime);
//			Anim.m_AnimTime0.Reset();
//			uint8 Color = (uint8)TruncToInt(MinMT(127.0f, (1.0f - UseTime) * 128.0f));
//			Anim.m_Data[0] = 0x80808080 + ((Color << 24) | (Color << 16) | (Color << 8) | (Color));
//			Anim.m_Data[1] = 0x00000000;
		}
		break;

		default:
			Anim.m_AnimTime0 = CMTime::CreateFromSeconds(0);
//			Anim.m_AnimTime0.Reset();
//			Anim.m_Data[0] = 0x00000000;
//			Anim.m_Data[1] = 0x00000000;
			break;
	}

	CXR_SkeletonInstance* pSkelInstance = Anim.m_pSkeletonInst;
	if (!pSkelInstance || !pModel) return;
	
	const int MaxModels = 6;
	int nModels = 0;
	uint16 lModelOnRenderFlags[MaxModels];
	CXR_AnimState lModelAnims[MaxModels];
	CXR_Model* lpModels[MaxModels];
	CMat4Dfp32 lModelPos[MaxModels];
	FillChar(lModelOnRenderFlags, sizeof(lModelOnRenderFlags), 0);

	int GUIDOffset = 0x2000;
	int ModelGUID = _pObj->m_iObject;

	// Fade the character
	if(bCutSceneView)
	{
		CD.m_iWeaponFade = 255;
	}
	
	// Nightvision lighting
	if((Char_GetPhysType(_pObj) != PLAYER_PHYS_DEAD))
	{
		int iObj = _pWClient->Player_GetLocalObject();

		CWO_CameraEffects NVInfo;
		CWObject_Message Msg(OBJMSG_CHAR_GETNVINTENSITY);
		Msg.m_pData = &NVInfo;

		if(_pWClient->ClientMessage_SendToObject(Msg, iObj))
		{
			fp32 Intens = 0;
			if(NVInfo.m_Nightvision > 0)
				Intens += 0.3f * NVInfo.m_Nightvision;
		}
	}

	bool bFlashLight = (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_FLASHLIGHT) != 0;
	if(bFlashLight)
		Anim.m_Data[3] = _pObj->m_iObject * 5 + 0x4000;

	lpModels[nModels] = pModel;
	lModelAnims[nModels] = Anim;
	lModelAnims[nModels].m_GUID = ModelGUID;
	lModelOnRenderFlags[nModels] = CXR_MODEL_ONRENDERFLAGS_MAXLOD;
	ModelGUID += GUIDOffset;

	CMat4Dfp32 OnlyZRot = MatIP;
	CVec3Dfp32(0, 0, 1).SetMatrixRow(OnlyZRot, 2);
	OnlyZRot.RecreateMatrix(2, 0);
	lModelPos[nModels] = OnlyZRot;

	nModels++;

	CMat4Dfp32 ExtraModelMat;
	ExtraModelMat = MatIP;

	for (int iModel = 1; iModel < CWO_NUMMODELINDICES; iModel++)
	{
		CXR_Model *pModel = lpObjectModels[iModel];
		if ((pModel != NULL) && true)
		{
			lpModels[nModels] = pModel;
			lModelPos[nModels] = ExtraModelMat;
			lModelAnims[nModels] = Anim;
			lModelAnims[nModels].m_AnimTime0 = CD.m_GameTime;
			lModelAnims[nModels].m_pModelInstance = _pObj->m_lModelInstances[iModel];
			lModelAnims[nModels].m_GUID = ModelGUID;
			lModelOnRenderFlags[nModels] = CXR_MODEL_ONRENDERFLAGS_MAXLOD;
			nModels++;
		}
		ModelGUID += GUIDOffset;
	}

	if(!(Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD))
	{
		if(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_LASERBEAM)
		{
			// Laserbeam from head
			CMat4Dfp32 Mat;
			CXR_AnimState AnimState;
			CAutoVar_AttachModel Model;
			Model.m_iAttachPoint[0] = 1;
			Model.m_iAttachRotTrack = PLAYER_ROTTRACK_HEAD;
			Model.m_iModel[0] = CD.m_iLaserBeam;
			CXR_Model *pHeadModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[1]);
			CXR_Skeleton *pHeadSkel = pHeadModel ? (CXR_Skeleton*)pHeadModel->GetParam(MODEL_PARAM_SKELETON) : NULL;
			bool bValid = false;
			if(pHeadSkel)
			{
				CXR_Model *pModel;
				if(Model.GetModel0_RenderInfo(_pWClient->GetMapData(), NULL,pSkelInstance, pHeadSkel, CMTime::CreateFromTicks(CD.m_GameTick, GameTickTime, IPTime) , AnimState, Mat, pModel, _pWClient))
					bValid = true;
			}
			if(bValid)
			{
				AnimState.m_Data[3] = 0xffff0000;

				CCollisionInfo Info;
				Info.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
				AnimState.m_AnimAttr0 = 256; // Max length
				CVec3Dfp32 &From = CVec3Dfp32::GetRow(Mat, 3);
				CVec3Dfp32 To = From + CVec3Dfp32::GetRow(Mat, 0) * AnimState.m_AnimAttr0;
				if(_pWClient->Phys_IntersectLine(From, To, 0, OBJECT_FLAGS_PHYSMODEL /*| OBJECT_FLAGS_PHYSOBJECT*/, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &Info, _pObj->m_iObject) && Info.m_bIsValid)
					AnimState.m_AnimAttr0 *= Info.m_Time;

				CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(CD.m_iLaserBeam);
				//AnimState.m_AnimTime0.Reset();

				_pEngine->Render_AddModel(pModel, Mat, AnimState);
			}
		}

		if(CD.m_iFlagIndex != -1)
		{
			CMat4Dfp32 Mat;
			Mat = CD.m_pObj->GetPositionMatrix();

			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(CD.m_iFlagIndex);

			_pEngine->Render_AddModel(pModel, Mat, AnimState);
		}
	}


	{
		CXR_Skeleton* pSkel = pModel ? (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON) : NULL;
		
		// Render items
		if(!(CD.m_ActionCutSceneCameraMode & CActionCutsceneCamera::ACS_CAMERAMODE_ACTIVE) && !(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE))
		{
			if(CD.m_Item0_Model.IsValid() && !(CD.m_Item0_Flags & RPG_ITEM_FLAGS_NORENDERMODEL) && (CD.m_Item0_Flags & RPG_ITEM_FLAGS_EQUIPPED) && !(CD.m_AnimGraph2.GetStateFlagsLoCombined() & AG2_STATEFLAG_NOITEMRENDER))
			{
				if(CD.m_Item0_Model.GetModel0_RenderInfo(_pWClient->GetMapData(), _pEngine, pSkelInstance, pSkel, CMTime::CreateFromTicks(CD.m_GameTick, GameTickTime, IPTime), 
														lModelAnims[nModels], lModelPos[nModels], lpModels[nModels],_pWClient))
				{
					lModelAnims[nModels].m_AnimTime0 = CMTime::CreateFromSeconds(fp32(CD.m_Item0_Num) + 0.001f);
					lModelAnims[nModels].m_Data[3] = lModelAnims[0].m_Data[3];

					CD.m_Item0_Model.RenderExtraModels(_pWClient->GetMapData(), _pEngine, pSkelInstance, pSkel, lModelPos[nModels], CMTime::CreateFromTicks(CD.m_GameTick, GameTickTime, IPTime), _pObj->m_iObject);

					nModels++;
				}
			}

			if(CD.m_Item1_Model.IsValid() && !(CD.m_Item1_Flags & RPG_ITEM_FLAGS_NORENDERMODEL) && (CD.m_Item1_Flags & RPG_ITEM_FLAGS_EQUIPPED) && !(CD.m_AnimGraph2.GetStateFlagsLoCombined() & AG2_STATEFLAG_NOITEMRENDERSECONDARY))
			{
				if(CD.m_Item1_Model.GetModel0_RenderInfo(_pWClient->GetMapData(), _pEngine, pSkelInstance, pSkel, CMTime::CreateFromTicks(CD.m_GameTick, GameTickTime, IPTime),
														lModelAnims[nModels], lModelPos[nModels], lpModels[nModels],_pWClient))
				{
					CD.m_Item1_Model.RenderExtraModels(_pWClient->GetMapData(), _pEngine, pSkelInstance, pSkel, lModelPos[nModels], CMTime::CreateFromTicks(CD.m_GameTick, GameTickTime, IPTime), _pObj->m_iObject);
					lModelAnims[nModels].m_Data[3] = lModelAnims[0].m_Data[3];
					nModels++;
				}
			}
		}

		if(CD.m_Item2_Model.IsValid())
		{
			if(CD.m_Item2_Model.GetModel0_RenderInfo(_pWClient->GetMapData(), _pEngine, pSkelInstance, pSkel, CMTime::CreateFromTicks(CD.m_GameTick, GameTickTime, IPTime),
				lModelAnims[nModels], lModelPos[nModels], lpModels[nModels],_pWClient))
			{
				CD.m_Item2_Model.RenderExtraModels(_pWClient->GetMapData(), _pEngine, pSkelInstance, pSkel, lModelPos[nModels], CMTime::CreateFromTicks(CD.m_GameTick, GameTickTime, IPTime), _pObj->m_iObject);
				lModelAnims[nModels].m_Data[3] = lModelAnims[0].m_Data[3];
				nModels++;
			}
		}

		// Render all models
		{
			int Flags = 0;
			if(_pObj->m_ClientFlags & CWO_CLIENTFLAGS_LINKINFINITE)
				Flags |= CXR_MODEL_ONRENDERFLAGS_NOCULL;

			if (!_pWClient->m_Render_CharacterShadows)
				Flags |= CXR_MODEL_ONRENDERFLAGS_NOSHADOWS;

			uint16 iExcludeLight = 0;
			if(CD.m_iAuraEffect)
			{
				CWObject_Message Msg(OBJMSG_EFFECTSYSTEM_GETLIGHTGUID, (aint)&iExcludeLight);
				_pWClient->ClientMessage_SendToObject(Msg, CD.m_iAuraEffect);

				// If no light guid was fetched, set default
				if(!iExcludeLight)
					iExcludeLight = _pObj->m_iObject * 5 + 0x4003;
			}
			
			for(int i = 0; i < nModels; i++)
			{
				int NewFlags = Flags | GlobalOnRenderFlags | lModelOnRenderFlags[i];
				bool bRenderHead = true;

				//lModelAnims[i].m_ExcludeLightGUID = _pObj->m_iObject*5 + 0x4003;	// Exclude flash light if it's on
				//lModelAnims[i].m_ExcludeLightGUID = CD.m_ExcludeLightGUID;
				lModelAnims[i].m_ExcludeLightGUID = iExcludeLight;
				lModelAnims[i].m_iObject = _pObj->m_iObject;
				lModelAnims[i].m_Variation = liVariations[i];
				if(!_pEngine->Render_AddModel(lpModels[i], lModelPos[i], lModelAnims[i], XR_MODEL_STANDARD, NewFlags))
				{
//					int i = 0;
				}
			}
		}
	}
}


extern fp32 AngleAdjust(fp32 _AngleA, fp32 _AngleB);
bool CWObject_CharAngelus::OnGetAnimState(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CXR_Skeleton* _pSkel, int _iCacheSlot, const CMat4Dfp32 &_Pos, fp32 _IPTime, CXR_AnimState& _Anim, CVec3Dfp32 *_pRetPos, bool _bIgnoreCache, fp32 _ClothScaleOverride)
{
	MAUTOSTRIP(CWObject_CharAngelus_OnGetAnimState, false);
	MSCOPE(CWObject_CharAngelus::OnGetAnimState, CHARANGELUS);
	
	CWO_CharAngelus_ClientData& CD = GetClientData(_pObj);
	CWObject_CoreData* pObj = _pObj;

	CMat4Dfp32 MatBody, MatLook;
	int Char_ControlMode = Char_GetControlMode(_pObj);

	// Get turn correction angle
	fp32 BodyAngle = 0.0f;
	{
		fp32 LastAngle = CD.m_TurnCorrectionLastAngle;
		LastAngle += AngleAdjust(LastAngle, CD.m_TurnCorrectionTargetAngle);
		if (M_Fabs(LastAngle - CD.m_TurnCorrectionTargetAngle) > 0.001f)
			BodyAngle = LERP(LastAngle, CD.m_TurnCorrectionTargetAngle, _IPTime);
		else
			BodyAngle = CD.m_TurnCorrectionTargetAngle;
	}

	if (CD.m_RenderAttached != 0 && CD.m_ExtraFlags & PLAYER_EXTRAFLAGS_FORCEATTACHMATRIX)
	{
		MatLook = _Pos;
		MatBody = _Pos;

		// Angle error version
		// Modify (will not effect camera hopefully)
		// Get look from opponent and diff with our own...
		if (CD.m_iMountedObject != 0)
		{
			CVec3Dfp32 Body = GetLook(CVec3Dfp32::GetMatrixRow(MatLook,0));
			CWObject_CoreData* pObj = _pWPhysState->Object_GetCD(CD.m_iMountedObject);
			CWO_Character_ClientData* pCDOther = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
			if (pCDOther)
			{
				Body[2] -= pCDOther->m_TurnCorrectionTargetAngle;
				Body[1] -= pCDOther->m_TurnCorrectionTargetAngleVert;
			}
			Body.CreateMatrixFromAngles(0, MatLook);
		}
	}
	else if (Char_ControlMode == PLAYER_CONTROLMODE_ANIMATION)
	{
		MatLook = _Pos;
		MatBody = MatLook;
		if(CD.m_RenderAttached == 0)
		{
			CVec3Dfp32(0, 0, 1).SetMatrixRow(MatBody, 2);
			MatBody.RecreateMatrix(2,0);
		}

		// Must use some crapor here to separate look/body
		CVec3Dfp32 Body = GetLook(CVec3Dfp32::GetMatrixRow(MatBody,0));

		// Angle error version
		if (Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD)
			BodyAngle = 0.0f;

		if (CD.m_iMountedObject != 0)
		{
			// Modify (will not effect camera hopefully)
			// Get look from opponent and diff with our own...
			CVec3Dfp32 Body = GetLook(CVec3Dfp32::GetMatrixRow(MatLook,0));
			CWObject_CoreData* pObj = _pWPhysState->Object_GetCD(CD.m_iMountedObject);
			CWO_Character_ClientData* pCDOther = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
			if (pCDOther && !pCDOther->m_CameraUtil.IsActive(CAMERAUTIL_MODE_MOUNTEDCAM))
			{
				Body[2] = pCDOther->m_Control_Look_Wanted[2];
				Body[1] = pCDOther->m_Control_Look_Wanted[1];
			}
			Body.CreateMatrixFromAngles(0, MatLook);
			CVec3Dfp32::GetMatrixRow(MatBody,3).SetMatrixRow(MatLook,3);
		}
		else
		{
			// Angle error version
			Body.k[2] += BodyAngle;
			Body.CreateMatrixFromAngles(2, MatBody);
			CVec3Dfp32::GetMatrixRow(MatLook,3).SetMatrixRow(MatBody,3);
		}

	}
	else
	{
		MatLook = _Pos;	
		MatBody = MatLook;

		{
			CVec3Dfp32(0, 0, 1).SetMatrixRow(MatBody, 2);
			MatBody.RecreateMatrix(2, 0);
		
			// Must use some crapor here to separate look/body
			CVec3Dfp32 Body = GetLook(CVec3Dfp32::GetMatrixRow(MatBody,0));

			// Angle error version
			Body.k[2] += BodyAngle;

			fp32 Target,Last;
			Target = CD.m_TurnCorrectionTargetAngle;
			Last = CD.m_TurnCorrectionLastAngle;
			Body.CreateMatrixFromAngles(2, MatBody);

			CVec3Dfp32::GetMatrixRow(_Pos, 3).SetMatrixRow(MatBody, 3);
			CVec3Dfp32::GetMatrixRow(_Pos, 3).SetMatrixRow(MatLook, 3);
		}
	}

	if(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE)
	{
		MatLook = CD.m_DialoguePlayerPos;
		MatBody = MatLook;
		CVec3Dfp32(0, 0, 1).SetMatrixRow(MatBody, 2);
		MatBody.RecreateMatrix(2, 0);
	}

	fp32 StepUp = LERP(CD.m_LastStepUpDamper, CD.m_StepUpDamper, _IPTime);
	MatBody.k[3][2] += StepUp;
	MatLook.k[3][2] += StepUp;

	CXR_SkeletonInstance* pSkelInstance = CD.m_lspSkelInst[_iCacheSlot];
	if (_pSkel && !pSkelInstance && (((_iCacheSlot == 0) && (CD.m_iMountedObject != 0)) || (_iCacheSlot != 0)))
	{
		MRTC_SAFECREATEOBJECT(spSkel, "CXR_SkeletonInstance", CXR_SkeletonInstance);
		CD.m_lspSkelInst[_iCacheSlot] = spSkel;
		spSkel->Create(_pSkel->m_lNodes.Len());
		pSkelInstance = spSkel;
		_bIgnoreCache = true;
	}

	if(_Anim.m_pSkeletonInst)
	{
		pSkelInstance	= _Anim.m_pSkeletonInst;
		_bIgnoreCache	= true;
	}

	if(_bIgnoreCache || 
		CD.m_lLastSkeletonInstanceSkel[_iCacheSlot] != _pSkel || 
		CD.m_lLastSkeletonInstanceTick[_iCacheSlot] != CD.m_GameTick || 
		CD.m_lLastSkeletonInstanceIPTime[_iCacheSlot] != _IPTime)
	{
		MSCOPESHORT( CacheMiss );

		CVec3Dfp32 RetPos = 0;
		CXR_Model *pModel = _pWPhysState->GetMapData()->GetResource_Model(pObj->m_iModel[0]);
		if (pModel)
		{
			CXR_Skeleton* pSkel = _pSkel;

			// Eval skeleton
			if (pSkel && pSkelInstance)
			{
				// Eval animation
				MSCOPE(EvalAnim, CHARANGELUS);

				if (CD.m_CharSkeletonScale != 0.0f && CD.m_Aim_SkeletonType == 0)
				{
					// Set model scale
					static const int BoneSet[] =
					{
						PLAYER_ROTTRACK_ROOT, PLAYER_ROTTRACK_RLEG, PLAYER_ROTTRACK_RKNEE, 
						PLAYER_ROTTRACK_RFOOT, PLAYER_ROTTRACK_LLEG, PLAYER_ROTTRACK_LKNEE, 
						PLAYER_ROTTRACK_LFOOT, PLAYER_ROTTRACK_SPINE, PLAYER_ROTTRACK_TORSO, 
						PLAYER_ROTTRACK_NECK, PLAYER_ROTTRACK_HEAD, PLAYER_ROTTRACK_RSHOULDER, 
						PLAYER_ROTTRACK_RARM, PLAYER_ROTTRACK_RELBOW,  
						PLAYER_ROTTRACK_RHAND, PLAYER_ROTTRACK_LSHOULDER, PLAYER_ROTTRACK_LARM,
						PLAYER_ROTTRACK_LELBOW, PLAYER_ROTTRACK_LHAND,
						PLAYER_ROTTRACK_RHANDATTACH, PLAYER_ROTTRACK_LHANDATTACH, 
						PLAYER_ROTTRACK_CAMERA
							
					};
					static const int BoneSetLen = sizeof(BoneSet) / sizeof(int);

					pSkelInstance->ApplyScale(CD.m_CharSkeletonScale, BoneSet, BoneSetLen);
				}

				CXR_AnimLayer Layers[32];
				int nLayers = Char_GetAnimLayers(_pObj, _Pos, _pWPhysState, _IPTime, Layers);

				fp32 AnimCamBlend = 0.0f;

				bool bRagdoll = false;//CD.m_RagdollClient.IsValid();
				{
					MSCOPESHORT(EvalAnim_SkeletonEval);
					CMat4Dfp32 ObjWMatJunk(MatBody);
#ifdef INCLUDE_OLD_RAGDOLL
					if(bRagdoll)
					{
						CD.m_RagdollClient.EvalAnim(_IPTime,Layers,nLayers,pSkel,pSkelInstance,ObjWMatJunk,0);
						RetPos = CVec3Dfp32::GetRow(ObjWMatJunk,3);
					}
					else
#endif // INCLUDE_OLD_RAGDOLL
					{
						// Hack for broken animgraphs
						for(int i = 0; i < nLayers; i++)
						{
							if(Layers[i].m_iBlendBaseNode == 10)
								Layers[i].m_Flags = CXR_ANIMLAYER_ADDITIVEBLEND;
						}

						// Animation camera blendin
						fp32 CamBlendIn = 0.0f;
						fp32 CamBlendOut = 1.0f;
						if (CD.m_AnimCamBlendInStart)
						{
							int32 GTDiff = CD.m_GameTick - CD.m_AnimCamBlendInStart;
							CamBlendIn = (((fp32)GTDiff)+_IPTime) * (1.0f / CHARANGELUS_BLENDTIME);
							CamBlendIn = Min(CamBlendIn, 1.0f);
						}
						if (CD.m_AnimCamBlendOutStart && (CD.m_AnimCamBlendOutStart > CD.m_AnimCamBlendInStart))
						{
							int32 GTDiff = CD.m_GameTick - CD.m_AnimCamBlendOutStart;
							CamBlendOut = (((fp32)GTDiff)+_IPTime) * (1.0f / CHARANGELUS_BLENDTIME);
							CamBlendOut = Min(1.0f, CamBlendOut);
							CamBlendOut = 1.0f - CamBlendOut;
						}
						AnimCamBlend = CamBlendIn * CamBlendOut;

						if(nLayers >= 32)
							Error_static("OnGetAnimState", "Too many layers. Stacktrashing...");

						// FIXME: Store this statically and don't send mask when those move tracks aren't going to be used. (NULL instead of &FaceForceMoveTracks)
						CXR_Anim_TrackMask FaceForceMoveTracks;
						FaceForceMoveTracks.Clear();
						for(int i = 21; i < 38; i++)
							FaceForceMoveTracks.m_TrackMaskMove.Enable(i);

						pSkel->EvalAnim(Layers, nLayers, pSkelInstance, ObjWMatJunk, 0, &FaceForceMoveTracks);
						RetPos = CVec3Dfp32::GetRow(ObjWMatJunk,3);

						// Evaluate muscle groups
						if (CD.m_iFacialSetup)
						{
							CFacialSetup* pFacialSetup = _pWPhysState->GetMapData()->GetResource_FacialSetup(CD.m_iFacialSetup);
							if (pFacialSetup)
							{
								enum { FACEDATA_BASETRACK = 21 };
								enum { FACEDATA_MAXCHANNELS = 51 };
								fp32 lFaceData[FACEDATA_MAXCHANNELS];
								pFacialSetup->GetFaceData(pSkelInstance, FACEDATA_BASETRACK, FACEDATA_MAXCHANNELS, lFaceData);
								pFacialSetup->Eval(lFaceData, pSkelInstance);

								const CMat4Dfp32& BaseMat = pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_NECK];
								pSkel->InitEvalNode(PLAYER_ROTTRACK_HEAD, BaseMat, pSkelInstance);
							}
						}
					}
				}

				{
					MSCOPESHORT(PostAnimEffects);

					if (!bRagdoll && 
						(Char_ControlMode != PLAYER_CONTROLMODE_LADDER)&&
						(Char_ControlMode != PLAYER_CONTROLMODE_ACTIONCUTSCENE)&&
						(Char_ControlMode != PLAYER_CONTROLMODE_LEDGE2) &&
						(Char_ControlMode != PLAYER_CONTROLMODE_HANGRAIL) &&
						(CD.m_Aim_SkeletonType != 0 || CD.m_ForcedAimingMode != 0))
					{
						CMat4Dfp32 IMB, RH;
						MatBody.InverseOrthogonal(IMB);
						CQuatfp32 QUnit, QRel, Q;
						QUnit.Unit();

						CQuatfp32 QRelIK[5];
						bool lbQRelIK[5];
						memset(lbQRelIK, 0, sizeof(lbQRelIK));
						
						const fp32 BlendValue = (CD.m_PostAnimSystem.m_Anim_IKLookBlend - _IPTime * PLAYER_ANIMBLENDSPEEDVISUAL);
						for(int i = 0; i < 2; i++)
						{
							int iMode;
							if(i == 0)
							{
								iMode = CD.m_PostAnimSystem.m_Anim_iCurIKLookMode;
								if(CD.m_PostAnimSystem.m_ForcedAimingMode != 0)
									iMode = CD.m_PostAnimSystem.m_ForcedAimingMode - 1;
							}
							else
								iMode = CD.m_PostAnimSystem.m_Anim_iLastIKLookMode;

							fp32 Blend = (i > 0) ? Max(1.0f - BlendValue, 0.0f) : Max(BlendValue, 0.0f);
							Blend = CHARANGELUS_SINC(Blend);

							switch(iMode)
							{
							case AIMING_NORMAL:
								// From spine and up
								{
									CMat4Dfp32 ZLook = MatLook;
									CVec3Dfp32(0,0,1).SetRow(ZLook, 2);
									ZLook.RecreateMatrix(2, 0); // Z Look-rotation
									ZLook.Multiply(IMB, RH);
									QRel.Create(RH); // QRel == Z Look-rotation

									CQuatfp32 Q2, Q3;
									MatLook.Multiply(IMB, RH);
									Q2.Create(RH); // Q2 = Look-rotation

									QRel.Lerp(Q2, 0.75f, Q3); // 25% Z Look-rotation + 75% Look-rotation
									QUnit.Lerp(Q3, 1.0f / 4, Q); // Split over 4 bones

									CHARANGELUS_APPLYQUAT(Q, QRelIK[0], lbQRelIK[0], Blend); // Apply on the two spine-bones
									CHARANGELUS_APPLYQUAT(Q, QRelIK[1], lbQRelIK[1], Blend);

									Q.Multiply(Q, Q);
									Q.Inverse();
									Q2.Multiply(Q, QRel);
									QUnit.Lerp(QRel, 1.0f / 2, Q); // Split rest of rotation over 2 bones
									CHARANGELUS_APPLYQUAT(Q, QRelIK[2], lbQRelIK[2], Blend);
									CHARANGELUS_APPLYQUAT(Q, QRelIK[3], lbQRelIK[3], Blend);
									break;
								}
							case AIMING_BODY:
								// Only spine (for aiming)
								MatLook.Multiply(IMB, RH);
								QRel.Create(RH);

								QUnit.Lerp(QRel, 1.0f / 2, Q);
								CHARANGELUS_APPLYQUAT(Q, QRelIK[0], lbQRelIK[0], Blend);
								CHARANGELUS_APPLYQUAT(Q, QRelIK[1], lbQRelIK[1], Blend);
								break;

							case AIMING_CROUCHBODY:
								MatLook.Multiply(IMB, RH);
								QRel.Create(RH);
								CHARANGELUS_APPLYQUAT(QRel, QRelIK[1], lbQRelIK[1], Blend);
								break;

							case AIMING_HEAD:
								// Only neck and head
								{
									MatLook.Multiply(IMB, RH);
									QRel.Create(RH);

									QUnit.Lerp(QRel, 1.0f / 2, Q);
									CHARANGELUS_APPLYQUAT(Q, QRelIK[2], lbQRelIK[2], Blend);
									CHARANGELUS_APPLYQUAT(Q, QRelIK[3], lbQRelIK[3], Blend);
									break;
								}
							case AIMING_FULLBODY:
								// Whole body (no aiming)
								{
									MatLook.Multiply(IMB, RH);
									QRel.Create(RH);
									CHARANGELUS_APPLYQUAT(QRel, QRelIK[4], lbQRelIK[4], Blend);
									break;
								}
							case AIMING_MINIGUN:
								// Only spine (for aiming)
								{
									CMat4Dfp32 Look = MatLook;
									Look.k[0][2] = Max(Look.k[0][2], -0.1f);
									Look.RecreateMatrix(0, 2);
									Look.Multiply(IMB, RH);
									QRel.Create(RH);

									QUnit.Lerp(QRel, 1.0f / 2, Q);
									CHARANGELUS_APPLYQUAT(Q, QRelIK[0], lbQRelIK[0], Blend);
									CHARANGELUS_APPLYQUAT(Q, QRelIK[1], lbQRelIK[1], Blend);

									CQuatfp32 Q2, Q3;
									MatLook.Multiply(IMB, RH);
									Q2.Create(RH);

									Q = QRel;
									Q.Inverse();
									Q2.Multiply(Q, QRel);
									QUnit.Lerp(QRel, 1.0f / 2, Q);
									CHARANGELUS_APPLYQUAT(Q, QRelIK[2], lbQRelIK[2], Blend);
									CHARANGELUS_APPLYQUAT(Q, QRelIK[3], lbQRelIK[3], Blend);
									break;
								}
								break;
							default:
								// None
								break;
							}

							if(CD.m_PostAnimSystem.m_Anim_iCurIKLookMode == CD.m_PostAnimSystem.m_Anim_iLastIKLookMode || CD.m_PostAnimSystem.m_Anim_IKLookBlend <= 0)
								break;
						}

						{
							if(lbQRelIK[4])
								RotateBoneAbsolute(QRelIK[4], pSkelInstance, PLAYER_ROTTRACK_ROOT, 0);
							if(lbQRelIK[0])
								RotateBoneAbsolute(QRelIK[0], pSkelInstance, PLAYER_ROTTRACK_SPINE, PLAYER_ROTTRACK_ROOT);
							if(lbQRelIK[1])
								RotateBoneAbsolute(QRelIK[1], pSkelInstance, PLAYER_ROTTRACK_TORSO, PLAYER_ROTTRACK_SPINE);
							if(lbQRelIK[2])
								RotateBoneAbsolute(QRelIK[2], pSkelInstance, PLAYER_ROTTRACK_NECK, PLAYER_ROTTRACK_TORSO);
							if(lbQRelIK[3])
								RotateBoneAbsolute(QRelIK[3], pSkelInstance, PLAYER_ROTTRACK_HEAD, PLAYER_ROTTRACK_NECK);
						}

						// We don't do this when the char is dead as it may
						// fuck up ragdoll things
						if(Char_GetPhysType(_pObj) != PLAYER_PHYS_DEAD)
						{
							// "Global" scale should only be set on root node
							CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(pSkelInstance->m_pBoneTransform[0],3);
							CMat43fp32 GlobalScale;
							CMat4Dfp32 Temp;
							GlobalScale.Unit();
							fp32 TestScale = CD.m_CharGlobalScale;
							GlobalScale.k[0][0] *= TestScale;
							GlobalScale.k[1][1] *= TestScale;
							GlobalScale.k[2][2] *= TestScale;
							Pos = Pos - Pos * TestScale;
							Pos.SetMatrixRow(GlobalScale,3);
							pSkelInstance->m_pBoneTransform[0].Multiply(GlobalScale,Temp);

							pSkel->EvalNode(PLAYER_ROTTRACK_ROOT, &Temp, pSkelInstance);
						}

						if(_Anim.m_Anim0 == 0 && (CD.m_iMountedObject != 0) && pSkelInstance->m_nBoneTransform > PLAYER_ROTTRACK_CAMERA)
						{
							if (AnimCamBlend > 0.0f)
							{
								// Only need to be done if blending
								if (AnimCamBlend < 1.0f)
								{
									CVec3Dfp32 Pos(CVec3Dfp32::GetMatrixRow(pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA],3));
									CQuatfp32 Mek,Mek2,Mek3;
									Mek.Create(pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA]);
									Mek2.Create(MatLook);
									Mek2.Lerp(Mek, AnimCamBlend, Mek3);
									Mek3.CreateMatrix3x3(pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA]);
									Pos.SetMatrixRow(pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA],3);
								}
							}
							else
							{
								for(int i = 0; i < 3; i++)
									for(int j = 0; j < 3; j++)
										pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA].k[i][j] = MatLook.k[i][j];
							}

							// 1. Get position from head track  (x = Head.Local * Head.Transform)
							// 2. Set M = Camera.Transform
							// 3. Change M.T so that 'x' is preserved:
							//      x = x*M = x*R + T  -->  T = x - x*R
							CVec3Dfp32 HeadPos = _pSkel->m_lNodes[PLAYER_ROTTRACK_HEAD].m_LocalCenter + CVec3Dfp32(3.5f, 0.0f, 1.75f);
							HeadPos *= pSkelInstance->GetBoneTransform(PLAYER_ROTTRACK_HEAD);

							CMat4Dfp32& CameraMat = pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA];
							CVec3Dfp32 x = _pSkel->m_lNodes[PLAYER_ROTTRACK_CAMERA].m_LocalCenter;
							x.MultiplyMatrix3x3(CameraMat);
							CameraMat.GetRow(3) = HeadPos - x; // adjust for different rotation and localcenter
						}
					}
				}

				OnGetAnimStateCloth(_pObj, _pWPhysState, pSkel, _pSkel, pSkelInstance, _iCacheSlot, _IPTime, _ClothScaleOverride);
			}
		}

		if(!_bIgnoreCache)
		{
			CD.m_lLastSkeletonInstanceTick[_iCacheSlot] = CD.m_GameTick;
			CD.m_lLastSkeletonInstanceIPTime[_iCacheSlot] = _IPTime;
			CD.m_lLastSkeletonInstancePos[_iCacheSlot] = RetPos;
			CD.m_lLastSkeletonInstanceSkel[_iCacheSlot] = _pSkel;
		}
		else if(!_Anim.m_pSkeletonInst)
			CD.m_lLastSkeletonInstanceTick[_iCacheSlot] = -1;

		if(_pRetPos)
			*_pRetPos	= RetPos;
	}
	else
	{
		MSCOPESHORT( CacheHit );
		if(_pRetPos)
			*_pRetPos = CD.m_lLastSkeletonInstancePos[_iCacheSlot];
	}

	_Anim.m_pSkeletonInst = pSkelInstance;
	_Anim.m_SurfaceOcclusionMask = 0;
	_Anim.m_SurfaceShadowOcclusionMask = 0;
	_Anim.m_AnimTime0 = CD.m_GameTime;

	return 1;
}


bool CWObject_CharAngelus::OnGetAnimStateCloth(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CXR_Skeleton* _pSkel, CXR_Skeleton* _pOrgSkel, CXR_SkeletonInstance* _pSkelInstance, const int& _iCacheSlot, const fp32& _IPTime, const fp32& _ClothScaleOverride)
{
#ifndef CLOTH_DISABLE
	CWO_CharAngelus_ClientData& CD = GetClientData(_pObj);
	if (_pWPhysState->IsClient() && _iCacheSlot == 0 && _ClothScaleOverride != -2.0f)
	{
		CXR_Model *models[CWO_NUMMODELINDICES];
		int nmodels= 0;

		for(int i = 0; i < 3; i++)
		{
			if(_pObj->m_iModel[i] > 0)
			{
				models[nmodels]= _pWPhysState->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
				nmodels++;
			}
		}

		if (_pSkel->m_lCloth.Len() > 0 && _pSkel->m_lCloth.Len() != CD.m_lCharacterCloth.Len())
		{
			CD.m_lCharacterCloth.SetLen(_pSkel->m_lCloth.Len());
			M_TRACEALWAYS("resizing m_lCharacterCloth\n");
		}

		for (int j=0; j<_pSkel->m_lCloth.Len(); j++) {
			if (!CD.m_lCharacterCloth[j].m_IsCreated)
			{
				CD.m_lCharacterCloth[j].Create(j,_pOrgSkel,_pSkelInstance, models, nmodels);
				CD.m_lCharacterCloth[j].m_IsCreated= true;

				// Make sure we are using the right blend mode
				switch(CD.m_AnimClothParam)
				{
					case 0:
						CD.m_lCharacterCloth[j].SetBlendMode(CLOTH_SKINBLEND);
						break;
					
					case 1:
						CD.m_lCharacterCloth[j].SetBlendMode(CLOTH_ANIMATIONBLEND);
						break;

					case 2:
						CD.m_lCharacterCloth[j].SetBlendMode(CLOTH_SKINBLEND);
						break;
				}
			}

			CXR_SkeletonCloth &cloth = CD.m_lCharacterCloth[j];
			CMTime time = CMTime::CreateFromTicks(CD.m_GameTick, _pWPhysState->GetGameTickTime(), _IPTime);		
			if (time.Compare(cloth.m_LastUpdate) > 0) 
			{
				fp32 dt = 0;

				if (cloth.m_LastUpdate.IsInvalid())
				{	
					cloth.m_LastUpdate = time;
				}

				dt = (time - cloth.m_LastUpdate).GetTime();
				cloth.m_LastUpdate = time;
				if (dt > 1.0f/30.0f) dt = 1.0f/30.0f;

				fp32 ClothAnimScale = _ClothScaleOverride != -1.0f ? _ClothScaleOverride : CD.m_AnimGraph2.GetClothAnimScale();

                fp32 FloorOffset = _pObj->GetPosition()[2] + 0.05f * 32.0f;
				
				// Quick hack, lower floor offset slightly downwards when dead
				if((Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD))
					FloorOffset -= 5.75f;

				CD.m_lCharacterCloth[j].SetFloorOffset(FloorOffset);

				int32 ClothSimFreq = CD.m_AnimGraph2.GetClothSimFreq();
				/*
				if(ClothSimFreq < 0)
					ClothSimFreq = 30;
				if(ClothAnimScale < 0)
					ClothAnimScale = 0.5f;
				*/

				if(_pSkel->m_lCloth[j].m_Name.UpperCase() == "CHAIN")
				{
					// Step cloth with correct blend mode
					switch(CD.m_AnimClothParam)
					{
						// Blend 0 -> 1
						case 0:
						{
							// Make sure we are using correct blend mode
							if(CD.m_lCharacterCloth[j].GetBlendMode() != CLOTH_ANIMATIONBLEND)
								CD.m_lCharacterCloth[j].SetBlendMode(CLOTH_ANIMATIONBLEND);

							const CMTime BlendTime = CMTime::CreateFromTicks(_pWPhysState->GetGameTick(), _pWPhysState->GetGameTickTime(), _IPTime) - CD.m_AnimClothTime;
							const fp32 ClothBlend = MinMT(1.0f, BlendTime.GetTime() * CD.m_AnimClothBlendTime);
							//CD.m_lCharacterCloth[j].Step2(models, nmodels, _pOrgSkel, _pSkelInstance, dt, ClothAnimScale, ClothSimFreq, _pWPhysState);
							//CD.m_lCharacterCloth[j].Step2(models, nmodels, _pOrgSkel, _pSkelInstance, dt, ClothBlend, ClothSimFreq, _pWPhysState);
							CD.m_lCharacterCloth[j].Step(models, nmodels, _pOrgSkel, _pSkelInstance, dt, 1.0f, ClothSimFreq, _pWPhysState);
							//CD.m_lCharacterCloth[j].Step2(models, nmodels, _pOrgSkel, _pSkelInstance, dt, 0.0f, ClothSimFreq, _pWPhysState);
							//CD.m_lCharacterCloth[j].Step2(models, nmodels, _pOrgSkel, _pSkelInstance, dt, 0.5f, ClothSimFreq, _pWPhysState);
						}
						break;

						// Blend 1 -> 0
						case 1:
						{
							// Make sure we are using correct blend mode
							if(CD.m_lCharacterCloth[j].GetBlendMode() != CLOTH_ANIMATIONBLEND)
								CD.m_lCharacterCloth[j].SetBlendMode(CLOTH_ANIMATIONBLEND);

							const CMTime BlendTime = CMTime::CreateFromTicks(_pWPhysState->GetGameTick(), _pWPhysState->GetGameTickTime(), _IPTime) - CD.m_AnimClothTime;
							const fp32 ClothBlend = 1.0f - MinMT(1.0f, BlendTime.GetTime() * CD.m_AnimClothBlendTime * 50.0f);
							//CD.m_lCharacterCloth[j].Step2(models, nmodels, _pOrgSkel, _pSkelInstance, dt, ClothAnimScale, ClothSimFreq, _pWPhysState);
							//CD.m_lCharacterCloth[j].Step2(models, nmodels, _pOrgSkel, _pSkelInstance, dt, ClothBlend, ClothSimFreq, _pWPhysState);
							CD.m_lCharacterCloth[j].Step(models, nmodels, _pOrgSkel, _pSkelInstance, dt, 1.0f, ClothSimFreq, _pWPhysState);
							//CD.m_lCharacterCloth[j].Step2(models, nmodels, _pOrgSkel, _pSkelInstance, dt, 0.0f, ClothSimFreq, _pWPhysState);
							//CD.m_lCharacterCloth[j].Step2(models, nmodels, _pOrgSkel, _pSkelInstance, dt, 0.5f, ClothSimFreq, _pWPhysState);
						}
						break;

						// Skin blend
						case 2:
							// Make sure we are using correct blend mode
							if(CD.m_lCharacterCloth[j].GetBlendMode() != CLOTH_SKINBLEND)
								CD.m_lCharacterCloth[j].SetBlendMode(CLOTH_SKINBLEND);
							
							CD.m_lCharacterCloth[j].Step(models, nmodels, _pOrgSkel, _pSkelInstance, dt, ClothAnimScale, ClothSimFreq, _pWPhysState);
							break;

						// Full ANIMATIONBLEND
						case 3:
							// Make sure we are using correct blend mode
							if(CD.m_lCharacterCloth[j].GetBlendMode() != CLOTH_ANIMATIONBLEND)
								CD.m_lCharacterCloth[j].SetBlendMode(CLOTH_ANIMATIONBLEND);

							//CD.m_lCharacterCloth[j].Step2(models, nmodels, _pOrgSkel, _pSkelInstance, dt, ClothAnimScale, ClothSimFreq, _pWPhysState);
							//CD.m_lCharacterCloth[j].Step2(models, nmodels, _pOrgSkel, _pSkelInstance, dt, 1.0f, ClothSimFreq, _pWPhysState);
							//CD.m_lCharacterCloth[j].Step2(models, nmodels, _pOrgSkel, _pSkelInstance, dt, 1.0f, ClothSimFreq, _pWPhysState);
							CD.m_lCharacterCloth[j].Step(models, nmodels, _pOrgSkel, _pSkelInstance, dt, 1.0f, ClothSimFreq, _pWPhysState);
							//CD.m_lCharacterCloth[j].Step2(models, nmodels, _pOrgSkel, _pSkelInstance, dt, 0.0f, ClothSimFreq, _pWPhysState);
							//CD.m_lCharacterCloth[j].Step2(models, nmodels, _pOrgSkel, _pSkelInstance, dt, 0.5f, ClothSimFreq, _pWPhysState);
							break;
					}
				}
				else
					CD.m_lCharacterCloth[j].Step(models, nmodels, _pOrgSkel, _pSkelInstance, dt, ClothAnimScale, ClothSimFreq, _pWPhysState);
			}
		}
	}
#endif
	return true;
}

