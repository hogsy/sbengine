#include "PCH.h"
#include "../WObj_Char.h"
#include "../../Shared/MOS/Classes/GameContext/WGameContext.h"

#define FADETICKS 4

#define CUTSCENE_SYNC_BEEP

// -------------------------------------------------------------------
//  CWObject_Cutscene
// -------------------------------------------------------------------
class CWObject_Cutscene : public CWObject
{
public:
	CWObject_Cutscene()
	{
		m_iCurClip = 0;
		m_iType = 0;
	}

	MRTC_DECLARE_SERIAL_WOBJECT;

	TArray<CWO_SimpleMessage> m_lMessages;

	int m_FadeTick;
	TArray<int> m_lCharacters;
	TArray<int> m_lCharacterActiveStates;
	TArray<int> m_liLights;
	TArray<CMat4Dfp32> m_lCharPos;
	TArray<TArray<CStr> > m_lSeq;
	TArray<TArray<int> > m_lLightStates;

	TArray<int> m_lCameras;
	TArray<fp32> m_lFOV;
	TArray<int> m_lClipTicks;
	int m_Flags;
	int m_iCurClip;
	int m_iPlayerObj;
	int m_iType;
	int16 m_iSyncedSound;
	CFStr m_SyncedSound;
#ifdef	CUTSCENE_SYNC_BEEP
	int16 m_iBeepSound;
#endif	// CUTSCENE_SYNC_BEEP

	class CInitData : public CReferenceCount
	{
	public:
		CStr m_Characters;
		CStr m_Lights;
		TArray<CStr> m_lClips;
	};

	TPtr<CInitData> m_spInitData;

	virtual void OnCreate()
	{
		m_spInitData = MNew(CInitData);
		m_Flags = 0;
	}

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		CStr KeyName = _pKey->GetThisName();
		CStr KeyValue = _pKey->GetThisValue();
		
		switch (_KeyHash)
		{
		case MHASH3('CHAR','ACTE','RS'): // "CHARACTERS"
			{
				m_spInitData->m_Characters = KeyValue;
				break;
			}
		
		case MHASH2('LIGH','TS'): // "LIGHTS"
			{
				m_spInitData->m_Lights = KeyValue;
				break;
			}

		case MHASH1('TYPE'): // "TYPE"
			{
				m_iType = _pKey->GetThisValuei();
				break;
			}

		case MHASH3('SYNC','EDSO','UND'): // "SYNCEDSOUND"
			{
				m_SyncedSound = KeyValue;
				m_iSyncedSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
#ifdef	CUTSCENE_SYNC_BEEP
				MACRO_GetSystemEnvironment(pEnv);
				if(pEnv)
				{
					CStr CutsceneBeep= pEnv->GetValue("SND_CUTSCENEBEEP", "" );
					if( CutsceneBeep.Len() > 0 )
						m_iBeepSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(CutsceneBeep.Str());
				}
#endif	// CUTSCENE_SYNC_BEEP
				break;
			}

		case MHASH2('FLAG','S'): // "FLAGS"
			{
				static const char *FlagsTranslate[] =
				{
					"NoSoundVolumes", "NoMusic", "NoSoundEffects", "NoSoundAmbients", "NoSoundVoice", NULL
				};

				m_Flags = KeyValue.TranslateFlags(FlagsTranslate);
				break;
			}

		default:
			{
				if(KeyName.CompareSubStr("CLIP") == 0)
				{
					int iClip = KeyName.Copy(4, 1024).Val_int();
					m_spInitData->m_lClips.SetLen(Max(iClip + 1, m_spInitData->m_lClips.Len()));
					m_spInitData->m_lClips[iClip] = KeyValue;
				}
				else
					CWObject::OnEvalKey(_KeyHash, _pKey);
				break;
			}
		}
	}

	virtual void OnSpawnWorld()
	{
		CWObject::OnSpawnWorld();

		while(m_spInitData->m_Characters != "")
			m_lCharacters.Add(m_pWServer->Selection_GetSingleTarget(m_spInitData->m_Characters.GetStrSep(",")));

		while(m_spInitData->m_Lights != "")
			m_liLights.Add(m_pWServer->Selection_GetSingleTarget(m_spInitData->m_Lights.GetStrSep(",")));

		int i;
		m_lSeq.SetLen(m_spInitData->m_lClips.Len());
	
		m_lLightStates.SetLen(m_spInitData->m_lClips.Len());
		fp32 AccumTime = 0;
		for(i = 0; i < m_spInitData->m_lClips.Len(); i++)
		{
			CStr Val = m_spInitData->m_lClips[i];
			m_lCameras.Add(m_pWServer->Selection_GetSingleTarget(Val.GetStrSep(",")));
			m_lFOV.Add(Val.GetStrSep(",").Val_fp64());
	//		fp32 Time = Val.GetStrSep(",").Val_fp64();

			fp32 Duration;
			CWObject_Message Msg(OBJMSG_HOOK_GETDURATION);
			Msg.m_Param0 = 0;
			Msg.m_Param1 = (aint)&Duration;
				
			m_pWServer->Message_SendToObject(Msg,m_lCameras[i]);
			Duration -= m_pWServer->GetGameTickTime();

			AccumTime +=Duration;

// Until the first 	cutscene is fixed, we'll go with the same old. shit		
			m_lClipTicks.Add(RoundToInt(AccumTime * m_pWServer->GetGameTicksPerSecond()));
	//		m_lClipTicks.Add(RoundToInt(Time * SERVER_TICKSPERSECOND));
			int c;
			for(c = 0; c < m_lCharacters.Len(); c++)
			{
				CStr St = Val.GetStrSep(",");
				m_lSeq[i].Add(St);

				// Precache anim
				if(St != "")
				{
					CStr Container = St.GetStrSep(":");
					m_pWServer->GetMapData()->GetResourceIndex_Anim(Container);
					m_pWServer->GetMapData()->GetResourceIndex_Anim(Container + "_f");
				}
			}

			for(c = 0; c < m_liLights.Len(); c++)
				m_lLightStates[i].Add(Val.GetIntSep(","));
		}

		if(m_lClipTicks.Len())
			m_FadeTick = m_lClipTicks[m_lClipTicks.Len() - 1] - FADETICKS;
		else
			m_FadeTick = -1;

		m_spInitData = NULL;
		
		for(int i = 0; i < m_lMessages.Len(); i++)
			m_lMessages[i].SendPrecache(m_iObject, m_pWServer);
	}

	virtual aint OnMessage(const CWObject_Message& _Msg)
	{
		switch(_Msg.m_Msg)
		{
		case OBJMSG_IMPULSE:
			if(m_CreationGameTick == 0)
			{
				if(m_iSyncedSound != 0)
				{
					MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
					if( !m_pWServer->m_bSkipSound )
					{
						CGameContext::CGC_Command Cmd(CGameContext::CGC_COMMAND_PLAYCUTSCENESOUND);
						Cmd.m_lFParam[0] = m_SyncedSound;
#ifdef	CUTSCENE_SYNC_BEEP
						MACRO_GetSystemEnvironment(pEnv);
						if(pEnv)
						{
							CStr CutsceneBeep= pEnv->GetValue("SND_CUTSCENEBEEP", "" );
							Cmd.m_lFParam[1] = CutsceneBeep;
						}
#endif	// CUTSCENE_SYNC_BEEP
						pGame->AddPendingCommand(Cmd);
					}
				}

				return StartCutscene(_Msg.m_iSender);
			}
			else
				return 0;
		
		case OBJSYSMSG_GETCAMERA:
			{
				CMTime Time = CMTime::CreateFromTicks(GetTime1(this, m_pWServer->GetGameTick() - m_iAnim1), m_pWServer->GetGameTickTime(), -m_CreationGameTickFraction);
				CMat4Dfp32 *pMat = (CMat4Dfp32 *)_Msg.m_pData;
				CWObject_Message Msg(OBJMSG_HOOK_GETRENDERMATRIX, aint(&Time), aint(pMat));
				return m_pWServer->Message_SendToObject(Msg, m_iAnim0);
			}
		}
		return CWObject::OnMessage(_Msg);
	}
	
	static aint OnClientMessage(CWObject_Client *_pObj, CWorld_Client *_pWClient, const CWObject_Message &_Msg)
	{
		switch(_Msg.m_Msg)
		{
		case OBJSYSMSG_GETCAMERA:
			{
				if(_pObj->m_iAnim0 == 0)
					return 0;
				CMTime Time = CMTime::CreateFromTicks(GetTime1(_pObj, _pWClient->GetGameTick() - _pObj->m_iAnim1), _pWClient->GetGameTickTime(), _pWClient->GetRenderTickFrac() - _pObj->m_CreationGameTickFraction);
//				LogFile(CStrF("Tick: %i, Camera %i, Time %.2f", _pWClient->GetGameTick(), _pObj->m_iAnim0, Time));
				CMat4Dfp32 *pMat = (CMat4Dfp32 *)_Msg.m_pData;
				CWObject_Message Msg(OBJMSG_HOOK_GETRENDERMATRIX, aint(&Time), aint(pMat));
				return _pWClient->ClientMessage_SendToObject(Msg, _pObj->m_iAnim0);
			}
	 		case OBJSYSMSG_GETFOV:
		 	{
				return _pObj->m_iAnim2;
			}
			return 1;

		}
		return CWObject::OnClientMessage(_pObj, _pWClient, _Msg);
	}

	int StartCutscene(int _iSender)
	{
		//AO: If sender isn't a player, use first player
		m_iPlayerObj = _iSender;
		CWObject_Game * pGame = m_pWServer->Game_GetObject();
		if (pGame && (pGame->Player_GetNum() > 0))
		{
			bool bPlayer = false;
			for (int i = 0; i < pGame->Player_GetNum(); i++)
			{
				if (pGame->Player_GetObjectIndex(i) == _iSender)
				{
					bPlayer = true;
					break;
				}
			}
			if (!bPlayer)
			{
				m_iPlayerObj = pGame->Player_GetObjectIndex(0);
			}
		}

		m_iCurClip = -1;
		m_ClientFlags |= CWO_CLIENTFLAGS_LINKINFINITE;

//		{
//			CWObject *pObj = m_pWServer->Object_Get(_iSender);
//			pObj->m_iObject;
//		}

		if(m_lClipTicks.Len() > 0)
		{
			CWObject_Message BeginCutscene(OBJMSG_CHAR_BEGINCUTSCENE, m_iType);
			BeginCutscene.m_iSender = m_iObject;
			m_pWServer->Message_SendToObject(BeginCutscene, m_iPlayerObj);
		}

		m_CreationGameTick = m_pWServer->GetGameTick();
		m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_ANIM);

		for(int i = 0; i < m_lCharacters.Len(); i++)
		{
			CMat4Dfp32 Mat;
			Mat.Unit();
			int State = 0;
			if(m_lCharacters[i] > 0)
			{
				CWObject *pObj = m_pWServer->Object_Get(m_lCharacters[i]);
				if(pObj)
				{
					Mat = pObj->GetPositionMatrix();

					CWObject_Interface_AI* pObjAI = pObj->GetInterface_AI();
					if( pObjAI )
					{
						State = pObjAI->AI_GetActivationState();
						pObjAI->AI_SetActivationState( CAI_Core::STATE_ALWAYSACTIVE );
					}
				}
			}
			m_lCharPos.Add(Mat);
			m_lCharacterActiveStates.Add(State);
		}

		if(m_Flags != 0)
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_MODIFYFLAGS, m_Flags), m_pWServer->Game_GetObjectIndex());

		RefreshCutscene();

		return 1;
	}

	static int32 GetTime1(CWObject_CoreData *_pObj, int32 _GameTick)
	{
		return _GameTick - _pObj->m_CreationGameTick;
	}

	void RefreshCutscene()
	{
		if(m_CreationGameTick != 0 && m_lClipTicks.Len() > 0)
		{
			fp32 Time = GetTime1(this, m_pWServer->GetGameTick()) - m_CreationGameTickFraction;
			if(RoundToInt(Time) == m_FadeTick)
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_FADESCREEN, (aint)(1000 * FADETICKS * m_pWServer->GetGameTickTime())), m_pWServer->Game_GetObjectIndex());

			while(m_iCurClip == -1 || Time > m_lClipTicks[m_iCurClip])
			{
				m_iCurClip++;
				if(m_iCurClip >= m_lClipTicks.Len())
				{
					// End cutscene
					CWObject_Message BeginCutscene(OBJMSG_CHAR_ENDCUTSCENE);
					m_pWServer->Message_SendToObject(BeginCutscene, m_iPlayerObj);
					m_CreationGameTick = 0;
					m_CreationGameTickFraction = 0;
					m_ClientFlags &= ~CWO_CLIENTFLAGS_LINKINFINITE;

					for(int i = 0; i < m_lCharacters.Len(); i++)
					{
						CWObject* pObj = m_pWServer->Object_Get(m_lCharacters[i]);
						if(!pObj) continue;
						CWObject_Interface_AI* pObjAI = pObj->GetInterface_AI();
						if(!pObjAI) continue;
						pObjAI->AI_SetActivationState( m_lCharacterActiveStates[i] );
					}

					if(m_Flags != 0 && m_lClipTicks.Len() != 0)
						m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_MODIFYFLAGS, 0, m_Flags), m_pWServer->Game_GetObjectIndex());

					return;
				}

				iAnim0() = m_lCameras[m_iCurClip];
				if(m_iCurClip > 0)
					iAnim1() = m_lClipTicks[m_iCurClip - 1];
				else
					iAnim1() = 0;
				iAnim2() = (int16)m_lFOV[m_iCurClip];

				//CWObject_Message Msg(OBJMSG_CHAR_SETCUTSCENEFOV, m_lFOV[m_iCurClip]);
				//m_pWServer->Message_SendToObject(Msg, m_iPlayerObj);
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1), m_lCameras[m_iCurClip]);

				for(int i = 0; i < m_liLights.Len(); i++)
				{
					int iLight = m_liLights[i];
					if (iLight > 0)
					{
						int iState = m_lLightStates[m_iCurClip][i];
						m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, iState), iLight);					
					}
				}

				for(int i = 0; i < m_lCharacters.Len(); i++)
				{
					int iChar = m_lCharacters[i];
					if (iChar > 0)
					{
						if(m_iCurClip != 0)
						{
							CWObject_Message Msg(OBJSYSMSG_TELEPORT, 3);
							Msg.m_pData = &m_lCharPos[i];
							m_pWServer->Message_SendToObject(Msg, m_lCharacters[i]);
						}
						const char *pAnim = m_lSeq[m_iCurClip][i];
						if(pAnim)
						{
							m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_HIDEMODEL, 0), m_lCharacters[i]);

							CFStr Facial = pAnim;
							CFStr Res = Facial.GetStrSep(":") + "_f";
							Res += ":" + Facial;
							CWObject_Message Msg(OBJMSG_CHAR_PLAYANIM);
							Msg.m_pData = (void *)pAnim;
							m_pWServer->Message_SendToObject(Msg, m_lCharacters[i]);

							CWObject_Message Msg2(OBJMSG_CHAR_PLAYANIM);
							Msg2.m_pData = (void *)Res.Str();
							Msg2.m_Reason = 1;
							m_pWServer->Message_SendToObject(Msg2, m_lCharacters[i]);
						}
						else
							m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_HIDEMODEL, 1), m_lCharacters[i]);
					}
				}
			}
		}
	}
	
	virtual void OnRefresh()
	{
		CWObject::OnRefresh();

		RefreshCutscene();
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Cutscene, CWObject, 0x0100);
