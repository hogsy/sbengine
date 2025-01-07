#include "PCH.h"
#include "WObj_Char.h"
#include "WObj_RPG.h"
#include "WObj_Player.h"
#include "WObj_Weapons/WObj_Spells.h"
#include "WRPG/WRPGSpell.h"
#include "WRPG/WRPGChar.h"
#include "WObj_Misc/WObj_ActionCutscene.h"

#include "../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Game.h"
#include "../../../Shared/MOS/Classes/GameWorld/WDataRes_Sound.h"
#include "../../../Shared/MOS/Classes/GameWorld/Client/WClient_Core.h"
#include "../../../Shared/MOS/Classes/GameWorld/Client/WClient_Sound.h"

//#pragma optimize("", off)
//#pragma inline_depth(0)

// -------------------------------------------------------------------
//  CWObject_RPG
// -------------------------------------------------------------------
CWObject_RPG::CWObject_RPG()
{
	MAUTOSTRIP(CWObject_RPG_ctor, MAUTOSTRIP_VOID);
}

void CWObject_RPG::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CWObject_RPG_OnIncludeTemplate, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_RPG::OnIncludeTemplate, WOBJECT_RPG);
	CWObject_Physical::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

	if(_pReg)
	{
		CRegistry *pChild = _pReg->FindChild("RPGOBJECT");
		if(pChild)
			CRPG_Object::IncludeRPGRegistry(pChild, _pMapData, _pWServer);

		CRegistry *pDialogue = _pReg->FindChild("DIALOGUE");
		if(pDialogue)
			IncludeDialogue(pDialogue->GetThisValue(), _pMapData);
	}
}

int CWObject_RPG::IncludeDialogue(CStr _Dialogue, CMapData *_pMapData)
{
	MAUTOSTRIP(CWObject_RPG_IncludeDialogue, 0);
	MSCOPE(CWObject_RPG::IncludeDialogue, WOBJECT_RPG);
	int iDialogue;
	{
		// Scoped to reduce potential memory fragmentation
		CStr Dialogue = _Dialogue.GetPath() + _Dialogue.GetFilenameNoExt();
		iDialogue = _pMapData->GetResourceIndex_Dialogue(Dialogue);
	}
	if(iDialogue > 0)
	{
		CWRes_Dialogue *pRes = _pMapData->GetResource_Dialogue(iDialogue);
		if(pRes)
			pRes->PreCache(_pMapData);
	}
	return iDialogue;
}

void CWObject_RPG::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_RPG_OnEvalKey, MAUTOSTRIP_VOID);
	switch (_KeyHash)
	{
	case MHASH3('RPGO','BJEC','T'): // "RPGOBJECT"
		{
			m_spRPGObject = CRPG_Object::CreateObject(_pKey, m_pWServer);
			break;
		}

	case MHASH2('DIAL','OGUE'): // "DIALOGUE"
		{
			Data(RPG_DATAINDEX_DIALOGUEID) = IncludeDialogue(_pKey->GetThisValue(), m_pWServer->GetMapData());
			break;
		}

	default:
		{
			CWObject_Physical::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_RPG::OnLoad(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_RPG_OnLoad, MAUTOSTRIP_VOID);
	CWObject_Physical::OnLoad(_pFile);
	m_spRPGObject = CRPG_Object::CreateObject(_pFile, m_pWServer);
}

void CWObject_RPG::OnSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_RPG_OnSave, MAUTOSTRIP_VOID);
	CWObject_Physical::OnSave(_pFile);
	m_spRPGObject->Write(_pFile);
}

aint CWObject_RPG::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_RPG_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
	case OBJMSG_RPG_SPEAK_OLD:
		{
			CWRes_Dialogue* pDialogue = GetDialogueResource(this, m_pWServer);
			if (!pDialogue)
				return 0;

			uint32 ItemHash = pDialogue->IntToHash(_Msg.m_Param0);
			return PlayDialogue_Hash(ItemHash, 0, 0, 0, 0);
		}

	case OBJMSG_RPG_SPEAK:
		{
			if (!_Msg.m_pData)
				return 0;

			CStr MessageData = (const char*)_Msg.m_pData;
			uint32 ItemHash = MessageData.GetStrSep(",").StrHash();
			uint Flags = MessageData.GetStrSep(",").Val_int();
			fp32 Offset = MessageData.GetStrSep(",").Val_fp64();
			int OffsetTicks = (int)M_Ceil(Offset * m_pWServer->GetGameTicksPerSecond());
			uint32 SyncGroupID = MessageData.GetStrSep(",").StrHash();
			return PlayDialogue_Hash(ItemHash, Flags, 0, OffsetTicks, SyncGroupID);
		}
		
	case OBJMSG_RPG_GETRPGOBJECT:
		return aint((CRPG_Object *)m_spRPGObject);
	}
		
	return CWObject_Physical::OnMessage(_Msg);
}

aint CWObject_RPG::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_RPG_OnClientMessage, 0);
	switch(_Msg.m_Msg)
	{
	case OBJMSG_RPG_ACTIVATEDIALOGUEITEM:
		{
			// This should be overridden for more complex usage of the dialogue system
			if(!(_Msg.m_Param1 & DIALOGUEFLAGS_PAUSED))
			{
				CWRes_Dialogue *pDialogue = (CWRes_Dialogue *)_Msg.m_pData;
				if(pDialogue)
				{
					uint32 DialogueItemHash = _Msg.m_Param0;
					uint Flags = _Msg.m_Param1;
					int StartOffsetTicks = _Msg.m_Reason;
					uint32 SyncGroupID = _Msg.m_DataSize;

					uint16 Type;
					int iSound = pDialogue->GetSoundIndex_Hash(DialogueItemHash, _pWClient->GetMapData(), &Type);
					if (iSound > 0)
					{
						bool bBattle = (Type & SUBTITLE_TYPE_MASK) == SUBTITLE_TYPE_AI;
						int iPlayer = _pWClient->Player_GetLocalObject();
						bool b2D = false;
						if (Flags & DIALOGUEFLAGS_FORCE2D)// || iPlayer == _pObj->m_iObject)
						{
							b2D = true;
						}
						else if ((Type & SUBTITLE_TYPE_MASK) == SUBTITLE_TYPE_INTERACTIVE)
						{
							CWObject_Client* pPlayer = _pWClient->Object_Get(iPlayer);
							if (pPlayer && pPlayer->m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE)
								b2D = true;
						}

						int iHold = pDialogue->GetMovingHold(DialogueItemHash);
						uint32 CustomVocapAnim = pDialogue->GetCustomAnim(DialogueItemHash);
						uint16 DialogueAnimFlags = pDialogue->GetDialogueAnimFlags(DialogueItemHash);

						fp32 AttnMaxDist = pDialogue->m_AttnMaxDist * (bBattle ? 3.5f : 1.0f);
						fp32 AttnMinDist = pDialogue->m_AttnMinDist * (bBattle ? 3.5f : 1.0f);

						uint8 Channel = WCLIENT_CHANNEL_VOICE;
						if(_Msg.m_iSender == WCLIENT_ATTENUATION_2D_CUTSCENE)
							Channel = WCLIENT_CHANNEL_CUTSCENE;
						Voice_Play(_pObj, _pWClient, _pObj->GetPosition(), CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(), 0), iSound, CustomVocapAnim, DialogueAnimFlags, iHold, AttnMaxDist, AttnMinDist, b2D, StartOffsetTicks, SyncGroupID, Channel);
					}
				}
			}
			return 1;
		}
	case OBJMSG_GAME_REQUESTSUBTITLE:
		{
			return 0;
/*			int iDialogue = GetCurDialogueIndex(_pObj);
			if(iDialogue <= 0)
				return 0;
			
			const CDialogueItem *pItem = GetDialogueItem(_pObj, _pWClient, iDialogue);
			if(!pItem)
				return 0;
			
			fp32 Duration = (_pWClient->GetGameTick() - GetCurDialogueItemTick(_pObj)) * SERVER_TIMEPERFRAME;
			if(Duration < pItem->m_SubtitleDuration || pItem->m_SubtitleDuration == -1)
			{
#ifdef MRTC_USVERSION
						if(pItem->m_Flags & 1)
						{
							MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
							if(pSys)
							{
								int iConfig = pSys->GetOptions()->GetValuei("CONTROLLER_TYPE", 0);
								if(iConfig != 0)
									return 0;
							}
						}
#endif

				fp32 *pDuration = (fp32 *)_Msg.m_Param0;
				if(pDuration)
					*pDuration = Duration;
				return int(pItem);
			}
			else
				return 0;*/
		}
		
	}
	return CWObject_Physical::OnClientMessage(_pObj, _pWClient, _Msg);
}

void CWObject_RPG::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWObject_RPG_OnClientNetMsg, MAUTOSTRIP_VOID);

	if (_Msg.m_MsgType == NETMSG_PLAYDIALOGUE)
	{
		int Pos = 0;
		int TickOffset = _Msg.GetInt32(Pos);
		TickOffset -= _pWClient->GetGameTick();
		uint32 DialogueItemHash = _Msg.GetInt32(Pos);
		uint Flags = _Msg.GetInt16(Pos);
		int iMaterial = _Msg.GetInt8(Pos);
		uint32 SyncGroupID = _Msg.GetInt32(Pos);
		int8 AttnType = _Msg.GetInt8(Pos);
		if ((_pWClient->Player_GetLocalObject() == _pObj->m_iObject) && (Flags & DIALOGUEFLAGS_PLAYERFORCE2D))
			AttnType = WCLIENT_ATTENUATION_2D;

		PlayDialogue_Hash(_pObj, _pWClient, DialogueItemHash, Flags, iMaterial, AttnType, TickOffset, SyncGroupID);
	}
	else if(_Msg.m_MsgType == NETMSG_FORWARDDIALOGUE)
	{
		CWRes_Dialogue::CRefreshRes Res;
		CWRes_Dialogue *pDialogue = GetDialogueResource(_pObj, _pWClient);
		CWO_Character_ClientData *pCDSpeaker = CWObject_Character::GetClientData(_pObj);
		CMTime simulate_time = pCDSpeaker->m_GameTime;
		uint16 iterations = 0;
		while(!pDialogue->Refresh(simulate_time, _pWClient->GetGameTickTime(), pCDSpeaker->m_DialogueInstance, pCDSpeaker->m_pCurrentDialogueToken, &Res))
		{
			CWObject_Character::EvalDialogueLink_Client(_pObj, _pWClient, Res);
			simulate_time = CMTime::CreateFromTicks(_pWClient->GetGameTick() + iterations, _pWClient->GetGameTickTime(), pCDSpeaker->m_PredictFrameFrac);
			iterations++;
		}
		CWObject_Character::EvalDialogueLink_Client(_pObj, _pWClient, Res);

		pCDSpeaker->m_VoCap.StopVoice();
		pCDSpeaker->m_VoCap.ClearQueue();
	}
	else if(_Msg.m_MsgType == NETMSG_STOPDIALOGUE)
	{
		Voice_Stop(_pObj, _pWClient);
	}
	else
		CWObject_Physical::OnClientNetMsg(_pObj, _pWClient, _Msg);

}

void CWObject_RPG::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_RPG::OnClientRefresh);
	Voice_Refresh(_pObj, _pWClient, _pObj->GetPosition(), CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(), 0));
}

/*
bool CWObject_RPG::PlayDialogue(int _iDialogue, int _Flags, int _Material)
{
	CWRes_Dialogue *pDialogue = GetDialogueResource(this, m_pWServer);
	if(!pDialogue)
		return false;
	
//	ConOutL(CStrF("Sending: Tick %i, Object %i, Dialogue %i, Flags %i", m_pWServer->GetGameTick(), m_iObject, _iDialogue, _Flags));
	CNetMsg Msg(NETMSG_PLAYDIALOGUE);
	Msg.AddInt32(m_pWServer->GetGameTick());
	Msg.AddInt16((int16)_iDialogue);
	Msg.AddInt16((int16)_Flags);
	Msg.AddInt8((uint8)_Material);
	m_pWServer->NetMsg_SendToObject(Msg, m_iObject, 1000000);

	return true;
}

bool CWObject_RPG::PlayDialogue(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, int _iDialogue, int _Flags, uint8 _Material)
{
	MAUTOSTRIP(CWObject_RPG_PlayDialogue, false);
	CWRes_Dialogue *pDialogue = GetDialogueResource(_pObj, _pWClient);
	if(!pDialogue)
		return false;

	if(pDialogue->IsQuickSound(_iDialogue))
	{
		int iSound = pDialogue->GetSoundIndex(_iDialogue, _pWClient->GetMapData());
		if(iSound > 0)
		{
			//ConOutL(CStrF("Dialogue %i", _iDialogue));
			_pWClient->Sound_At(WCLIENT_CHANNEL_SFX, _pObj->GetPosition(), iSound, 0, _Material);
		}
	}
	else
	{
		CWObject_Message Msg(OBJMSG_RPG_ACTIVATEDIALOGUEITEM, _iDialogue, _Flags);
		Msg.m_pData = pDialogue;
		_pWClient->ClientMessage_SendToObject(Msg, _pObj->m_iObject);
	}

	return true;
}*/


bool CWObject_RPG::PlayDialogue_Hash(uint32 _DialogueItemHash, uint _Flags, int _Material, int _StartOffset, uint32 _SyncGroupID, int8 _AttnType)
{
	CWRes_Dialogue *pDialogue = GetDialogueResource(this, m_pWServer);
	if (!pDialogue)
		return false;

	//	ConOutL(CStr("Sending: Tick %i, Object %i, Dialogue %i, Flags %i", m_pWServer->GetGameTick(), m_iObject, _iDialogue, _Flags));
	CNetMsg Msg(NETMSG_PLAYDIALOGUE);
	Msg.AddInt32(m_pWServer->GetGameTick() + _StartOffset);
	Msg.AddInt32(_DialogueItemHash);
	Msg.AddInt16((int16)_Flags);
	Msg.AddInt8((uint8)_Material);
	Msg.AddInt32(_SyncGroupID);
	Msg.AddInt8(_AttnType);
	m_pWServer->NetMsg_SendToObject(Msg, m_iObject, 1000000);
	
	if (_SyncGroupID != 0)
	{
		uint32 ClientsToWaitFor = m_pWServer->Client_ClientsToWaitFor();
		int iSyncObj = m_pWServer->Selection_GetSingleTarget(_SyncGroupID);
		if (iSyncObj > 0)
			m_pWServer->Message_SendToObject(CWObject_Message(OBJSYSMSG_SOUNDSYNC, 0, ClientsToWaitFor), iSyncObj);
	}

	return true;
}


bool CWObject_RPG::PlayDialogue_Hash(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, uint32 _DialogueItemHash, uint _Flags, uint8 _Material, int _AttnType, int _StartOffset, uint32 _SyncGroupID)
{
	MAUTOSTRIP(CWObject_RPG_PlayDialogue_Hash, false);
	CWRes_Dialogue *pDialogue = GetDialogueResource(_pObj, _pWClient);
	if(!pDialogue)
		return false;

	if(pDialogue->IsQuickSound_Hash(_DialogueItemHash))
	{
		int iSound = pDialogue->GetSoundIndex_Hash(_DialogueItemHash, _pWClient->GetMapData());
		if(iSound > 0)
		{
			//ConOutL(CStr("Dialogue %i", _iDialogue));
			fp32 StartOffset = _StartOffset * _pWClient->GetGameTickTime();
			_pWClient->Sound_At(WCLIENT_CHANNEL_SFX, _pObj->GetPosition(), iSound, _AttnType, _Material, 1.0f, CVec3Dfp32(0.0f), StartOffset, _SyncGroupID);
		}
	}
	else
	{
		//TODO: we're really bending the limits of decency here when it comes to passing the params... should make a struct and pass that. 
		CWObject_Message Msg(OBJMSG_RPG_ACTIVATEDIALOGUEITEM, _DialogueItemHash, _Flags, _AttnType, _StartOffset);
		Msg.m_pData = pDialogue;
		Msg.m_DataSize = _SyncGroupID;
		_pWClient->ClientMessage_SendToObject(Msg, _pObj->m_iObject);
	}

	return true;
}


CWRes_Dialogue *CWObject_RPG::GetDialogueResource(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	int iDialogueID = GetDialogueID(_pObj);
	if(iDialogueID == 0)
		return false;
	
	return _pWPhysState->GetMapData()->GetResource_Dialogue(iDialogueID);
}

int CWObject_RPG::GetDialogueID(CWObject_CoreData *_pObj)
{
	return _pObj->m_Data[RPG_DATAINDEX_DIALOGUEID];
}

int CWObject_RPG::GetDialogueID()
{
	return m_Data[RPG_DATAINDEX_DIALOGUEID];
}

int CWObject_RPG::Voice_GetHandle(CWObject_CoreData *_pObj)
{
	//JK-FIX: Do this some other way
	CWObject_Client *pObj = TDynamicCast<CWObject_Client >(_pObj);
	if(!pObj)
		return 0;
	
	return pObj->m_ClientData[0];
}


void CWObject_RPG::Voice_Play(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CVec3Dfp32& _Pos, const CVec3Dfp32 &_Orientation, int _iSound, uint32 _CustomVocapAnim, uint16 _DialogueAnimFlags, int _iHold, fp32 _MaxAttDist, fp32 _MinAttDist, bool _b2D, int _StartOffset, uint32 _SyncGroupID, uint8 _Channel)
{
	CWorld_ClientCore* pCore = safe_cast<CWorld_ClientCore >(_pWClient);
	M_ASSERTHANDLER(pCore, "no client?!", return);

	bint bStartPaused = false;
	if (_SyncGroupID)
	{
		bStartPaused = true;
		// Make sure sync group is created even if sound fails
		pCore->Sound_AddVoiceToSyncGroup(-1, _SyncGroupID);
	}


	// check so we can play sounds
	if (!pCore || pCore->m_spSound == NULL || !pCore->Sound_IsActive())
		return;

	// get the wave description
	CSC_SFXDesc *pDesc = _pWClient->GetMapData()->GetResource_SoundDesc(_iSound);

	if(!pDesc)
		return;

	// Get wave id
	int32 iW = pDesc->GetPlayWaveId();
	if(iW < 0)
		return;
	
	// destroy old sound ?
	if (_pObj->m_ClientData[0] > 0)
	{
		pCore->m_spSound->Voice_Destroy(_pObj->m_ClientData[0]);
		_pObj->m_ClientData[0] = 0;
	}

	fp32 Delay = _StartOffset * _pWClient->GetGameTickTime();
	// start the new sound and set proper parameters
	int32 WaveID = pDesc->GetWaveId(iW);
	if(WaveID > 0)
	{
		if(_b2D)
		{
			CSC_VoiceCreateParams CreateParams;
			pCore->m_spSound->Voice_InitCreateParams(CreateParams, pDesc->GetCategory());

			CreateParams.m_hChannel = pCore->m_hChannels[_Channel];
			CreateParams.m_bStartPaused = bStartPaused;
			CreateParams.m_MinDelay = Delay;

			CreateParams.m_3DProperties.m_MinDist += _MinAttDist;
			CreateParams.m_3DProperties.m_MaxDist += _MaxAttDist;
			CreateParams.m_3DProperties.m_Position = _Pos;
			CreateParams.m_3DProperties.m_Orientation = _Orientation;
			CreateParams.m_3DProperties.m_Velocity = _pObj->GetMoveVelocity();

			_pObj->m_ClientData[0] = pCore->Sound_CreateVoice( pDesc, iW, CreateParams);
		}
		else
		{
			CSC_VoiceCreateParams3D CreateParams; // Notice the 3D after CSC_VoiceCreateParams
			pCore->m_spSound->Voice_InitCreateParams(CreateParams, pDesc->GetCategory());

			CreateParams.m_hChannel = pCore->m_hChannels[_Channel];
			CreateParams.m_bStartPaused = bStartPaused;
			CreateParams.m_MinDelay = Delay;

			CreateParams.m_3DProperties.m_MinDist += _MinAttDist;
			CreateParams.m_3DProperties.m_MaxDist += _MaxAttDist;
			CreateParams.m_3DProperties.m_Position = _Pos;
			CreateParams.m_3DProperties.m_Orientation = _Orientation;
			CreateParams.m_3DProperties.m_Velocity = _pObj->GetMoveVelocity();

			_pObj->m_ClientData[0] = pCore->Sound_CreateVoice( pDesc, iW, CreateParams);
		}

		// find lipdata if any
		if(pCore->m_spSound->LSD_Get(WaveID))
		{
			// time to fetch the client data
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
			if(pCD)
			{
				pCD->m_LipSync.SetVoice(WaveID, _pObj->m_ClientData[0]);
				if(pCD->m_lAnim_VoCap.Len() > 0)
				{
					pCD->m_VoCap.Init(pCD->m_lAnim_VoCap.GetBasePtr(), pCD->m_lAnim_VoCap.Len());
					CMTime Time = CMTime::CreateFromTicks(pCD->m_GameTick, _pWClient->GetGameTickTime(), pCD->m_PredictFrameFrac);
					pCD->m_VoCap.SetVoice(pCore->m_spSound, _pWClient->GetMapData(), WaveID, _CustomVocapAnim, _iHold, _pObj->m_ClientData[0], Time, Delay, _DialogueAnimFlags);
					if (pCD->m_RenderAttached != 0)
						pCD->m_VocapOrigin.Unit();
					else
					{
						pCD->m_VocapOrigin = _pObj->GetPositionMatrix();
						CVoCap_AnimItem* pQueue = pCD->m_VoCap.m_spAnimQueue;
						if(pQueue && pQueue->m_spCurSequence)
						{
							CMat4Dfp32 RotMat,Temp;
							vec128 Move;
							CQuatfp32 Rot;
							pQueue->m_spCurSequence->EvalTrack0(CMTime::CreateFromSeconds(0.0f), Move, Rot);
							Move = M_VNeg(Move);
							Move = M_VMul(Move, M_VLdScalar(pCD->m_CharGlobalScale * (1.0f - pCD->m_CharSkeletonScale)));
							Move = M_VSetW1(Move);
							Rot.Inverse();
							Rot.CreateMatrix(RotMat);
							pCD->m_VocapOrigin.Multiply3x3(RotMat,Temp);
							Move = M_VMulMat4x3(Move, pCD->m_VocapOrigin);
							pCD->m_VocapOrigin = Temp;
							pCD->m_VocapOrigin.r[3] = M_VSetW1(Move);
						}
					}
				}
			}
		}
	}

	int hVoice = _pObj->m_ClientData[0];
	if (_SyncGroupID && hVoice > 0)
		pCore->Sound_AddVoiceToSyncGroup(hVoice, _SyncGroupID);
}


void CWObject_RPG::Voice_Stop(CWObject_Client *_pObj, CWorld_Client *_pWClient)
{
	CWorld_ClientCore *pCore = safe_cast<CWorld_ClientCore >(_pWClient);

	// check so we can play sounds
	if(!pCore || pCore->m_spSound == NULL || !pCore->Sound_IsActive())
		return;

	if(_pObj->m_ClientData[0])
		pCore->m_spSound->Voice_Destroy(_pObj->m_ClientData[0]);
}

void CWObject_RPG::Voice_Refresh(CWObject_Client *_pObj, CWorld_Client *_pWClient, const CVec3Dfp32 &_Pos, const CVec3Dfp32 &_Orientation, fp32 _Occlusion)
{
	CWorld_ClientCore *pCore = safe_cast<CWorld_ClientCore >(_pWClient);

	if(pCore && pCore->m_spSound != NULL && _pObj->m_ClientData[0] != 0)
	{
		if(!pCore->m_spSound->Voice_IsPlaying(_pObj->m_ClientData[0]))
		{
			_pObj->m_ClientData[0] = 0;

			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
			if(pCD)
				pCD->m_LipSync.SetVoice(-1, -1);
		}
		else
		{
			pCore->m_spSound->Voice3D_SetOrigin(_pObj->m_ClientData[0], _Pos);
			pCore->m_spSound->Voice3D_SetOrientation(_pObj->m_ClientData[0], _Orientation);
			pCore->m_spSound->Voice3D_SetVelocity(_pObj->m_ClientData[0], _pObj->GetMoveVelocity());
			

			// Thin door -1800, 0.66f 
			CSC_3DOcclusion Prop;
//			Prop.m_Occlusion_LFRatio = 0.65f;
//			Prop.m_Occlusion_HFLevel = 0.25f + (1.0f - _Occlusion) * 0.75f;//1.0f - _Occlusion * 0.5f;
			Prop.m_Occlusion = _Occlusion;
			pCore->m_spSound->Voice3D_SetOcclusion(_pObj->m_ClientData[0], Prop);
		}
	}
}


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_RPG, CWObject_Physical, 0x0100);

// -------------------------------------------------------------------
//  CWObject_Item
// -------------------------------------------------------------------
void CWObject_Item::OnCreate()
{
	CWObject_Object::OnCreate();
	MAUTOSTRIP(CWObject_Item_OnCreate, MAUTOSTRIP_VOID);
	m_iItemRepickCountdown = 0;
	m_PhysAttrib.m_Elasticy = 0.4f;
	m_IntersectNotifyFlags = OBJECT_FLAGS_PLAYER;
//	m_AnimTime = MRTC_RAND() % 255;
//	m_iAnim0 = MRTC_RAND();
	m_nPhysIdleTicks = 0;
	m_Flags = 0; 
//	m_iModel[1] = m_pWServer->GetMapData()->GetResourceIndex_Model("glimmer2");
	m_iPickupAnim = -1;

	m_IntersectNotifyFlags |= OBJECT_FLAGS_CREEPING_DARK;
}

void CWObject_Item::OnClientPrecacheClass(CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	CWObject_Object::OnClientPrecacheClass(_pWClient, _pEngine);

	CXW_Surface* pOrgSurf = _pEngine->m_pSC->GetSurface("pickupglow");
	if(pOrgSurf)
	{
		CXW_Surface* pSurf = pOrgSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
		if(pSurf)
			pSurf->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	}
}

void CWObject_Item::TagAnimationsForPrecache(class CWAG2I_Context* _pContext, class CWAG2I* _pAgi)
{
	if (m_spDummyObject)
		m_spDummyObject->TagAnimationsForPrecache(_pContext, _pAgi);
}

void CWObject_Item::AttachCharacter(CWObject *_pChar, int _iItemSlot)
{
	MAUTOSTRIP(CWObject_Item_AttachCharacter, MAUTOSTRIP_VOID);
	m_pWServer->Object_AddChild(_pChar->m_iObject, m_iObject);
	ClientFlags() |= CLIENTFLAGS_ATTACHED;
	m_iAnim1 = _iItemSlot;

	{
		CWO_PhysicsState Phys;
		Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(13, 13, 28), CVec3Dfp32(0, 0, 28));
		Phys.m_nPrim = 1;

		Phys.m_ObjectFlags = OBJECT_FLAGS_PICKUP;
		Phys.m_ObjectIntersectFlags = 0;
		Phys.m_PhysFlags = OBJECT_PHYSFLAGS_OFFSET;
	
		m_IntersectNotifyFlags = OBJECT_FLAGS_PLAYER;
		m_iItemRepickCountdown = int(m_pWServer->GetGameTick() + m_pWServer->GetGameTicksPerSecond());

		if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
		{
			m_pWServer->Object_Destroy(m_iObject);
		
			return;
		}
	}
}

void CWObject_Item::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Item_OnEvalKey, MAUTOSTRIP_VOID);
	switch (_KeyHash)
	{
	case MHASH3('RPGO','BJEC','T'): // "RPGOBJECT"
		{
			CRPG_Object::IncludeRPGClass(_pKey->GetThisValue(), m_pWServer->GetMapData(), m_pWServer, true);
			SetItem(_pKey->GetThisValue());
			break;
		}

	case MHASH2('FLAG','S'): // "FLAGS"
		{
			static const char *FlagsTranslate[] =
			{
				"rotate", "fall", "waitspawn", "physset", "nodynamic", NULL
			};

			CStr Val = _pKey->GetThisValue();
			m_Flags = _pKey->GetThisValue().TranslateFlags(FlagsTranslate);
			if(m_Flags & FLAGS_ROTATE)
				ClientFlags() |= CLIENTFLAGS_ROTATE;
			if(m_Flags & FLAGS_WAITSPAWN)
				ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;
			break;
		}

	case MHASH3('PICK','UPAN','IM'): // "PICKUPANIM"
		{
			CWObject_Message Msg(OBJMSG_GAME_RESOLVEANIMHANDLE);
			Msg.m_pData = (char *)_pKey->GetThisValue();
			m_iPickupAnim = m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
			break;
		}

	default:
		{
			if(_pKey->GetThisName().CompareSubStr("MSG_ONPICKUP") == 0)
			{
				if(!m_Msg_OnPickup.IsValid())
					m_Msg_OnPickup.Parse(_pKey->GetThisValue(), m_pWServer);
			}
			else
				CWObject_Object::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

bool CWObject_Item::SetItem(const char *_pName)
{
	MAUTOSTRIP(CWObject_Item_SetItem, false);
	m_ItemTemplate = _pName;
	spCRPG_Object spObj = CRPG_Object::CreateObject(_pName, m_pWServer);
	m_spDummyObject = TDynamicCast<CRPG_Object_Item>((CRPG_Object*)spObj);
	if(m_spDummyObject)
	{
		m_spDummyObject->m_iPhysModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel("Phys\\P_ItemBox.xw:2");
		iModel(0) = m_spDummyObject->m_Model.m_iModel[0];
		return true;
	}
	else
		return false;
}

bool CWObject_Item::SetItem(CRPG_Object_Item *_pObj)
{
	MAUTOSTRIP(CWObject_Item_SetItem_2, false);
	m_ItemTemplate = (char *)_pObj->m_Name;
	m_spDummyObject = _pObj;
	if(m_spDummyObject)
	{
		iModel(0) = m_spDummyObject->m_Model.m_iModel[0];
		m_spDummyObject->m_iPhysModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel("Phys\\P_ItemBox.xw:2");
		CreatePhys();
		return true;
	}
	else
		return false;
}

void CWObject_Item::DetachCharacter(CWObject *_pChar, CRPG_Object_Item *_pItem, const CMat4Dfp32 &_Pos)
{
	MAUTOSTRIP(CWObject_Item_DetachCharacter, MAUTOSTRIP_VOID);
	m_pWServer->Object_RemoveChild(_pChar->m_iObject, m_iObject);
	ClientFlags() &= ~CLIENTFLAGS_ATTACHED;

	SetItem(_pItem->m_Name);
	SetPosition(_Pos);
	SetVelocity(CVec3Dfp32((Random - 0.5f) * 3, (Random - 0.5f) * 3, Random * 5));

	{
		CWO_PhysicsState Phys;
		Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(9.0f, 9.0f, 9.0f), 0);
		Phys.m_nPrim = 1;

		Phys.m_ObjectFlags = 0; // was OBJECT_FLAGS_DEBRIS;  -mh
		Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
		Phys.m_PhysFlags = 
			OBJECT_PHYSFLAGS_ROTATION |
//			OBJECT_PHYSFLAGS_PUSHER |
//			OBJECT_PHYSFLAGS_PUSHABLE |
			OBJECT_PHYSFLAGS_SLIDEABLE |
			OBJECT_PHYSFLAGS_FRICTION |
			OBJECT_PHYSFLAGS_PHYSMOVEMENT |
//			OBJECT_PHYSFLAGS_ROTFRICTION_SLIDE |
//			OBJECT_PHYSFLAGS_ROTFRICTION_ROT |
//			OBJECT_PHYSFLAGS_OFFSET |
			OBJECT_PHYSFLAGS_ROTREFLECT |
			OBJECT_PHYSFLAGS_ROTVELREFLECT;
	
		m_IntersectNotifyFlags = OBJECT_FLAGS_PLAYER;
		m_iItemRepickCountdown = int(m_pWServer->GetGameTick() + m_pWServer->GetGameTicksPerSecond());

		if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
		{
			m_pWServer->Object_Destroy(m_iObject);
			return;
		}
	}
}

aint CWObject_Item::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Item_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
	case OBJSYSMSG_NOTIFY_INTERSECTION:
		{
			if (m_pWServer->GetGameTick() < m_iItemRepickCountdown)
				return 0;
			if(m_spDummyObject && !IsDestroyed())
			{
				// Check if the item can be merged, if so auto pick it up.......
				// Check if the item can be merged, if so auto pick it up.......
				bool bCanPickupUniqueGun = ((m_spDummyObject->m_Flags2 & RPG_ITEM_FLAGS2_UNIQUE) &&	m_spDummyObject->CanPickup());
				if (!(m_spDummyObject->m_Flags & RPG_ITEM_FLAGS_NOMERGE) || bCanPickupUniqueGun)
				{
					m_iItemRepickCountdown = int(m_pWServer->GetGameTick() + m_pWServer->GetGameTicksPerSecond());
					// Send message to char, asking if item exists
					if (bCanPickupUniqueGun || m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_CANPICKUPITEM,m_spDummyObject->m_iItemType),_Msg.m_Param0))
					{
						CWObject_Message Msg(OBJMSG_RPG_AVAILABLEPICKUP, (m_spDummyObject->m_iItemType), aint((CRPG_Object_Item *)(CReferenceCount *)m_spDummyObject));
						Msg.m_Reason = 1; //Don't autoequip this item. 
						Msg.m_pData = (char *)m_ItemTemplate;
						Msg.m_DataSize = m_ItemTemplate.GetMax();
						Msg.m_iSender = m_iObject;

						if(m_pWServer->Message_SendToObject(Msg, _Msg.m_Param0))
						{
							m_Msg_OnPickup.SendMessage(m_iObject, _Msg.m_Param0, m_pWServer);
							Destroy();
							return true;
						}
					}
				}
			}
			return false;
		}
	case OBJMSG_RPG_AVAILABLEPICKUP:
		if(!(m_Flags & FLAGS_WAITSPAWN))
		{
			if(m_spDummyObject && !IsDestroyed() && !(m_spDummyObject->m_Flags & RPG_ITEM_FLAGS_NOPICKUP))
			{
				CWObject_Message Msg(OBJMSG_RPG_AVAILABLEPICKUP, (m_spDummyObject->m_iItemType), aint((CRPG_Object_Item *)(CReferenceCount *)m_spDummyObject));
				Msg.m_pData = (char *)m_ItemTemplate;
				Msg.m_DataSize = m_ItemTemplate.GetMax();
				Msg.m_iSender = m_iObject;

				if(m_pWServer->Message_SendToObject(Msg, _Msg.m_Param0))
				{
					m_Msg_OnPickup.SendMessage(m_iObject, _Msg.m_Param0, m_pWServer);
					Destroy();
				}
				return 0;
			}
		}
		return 0;

	case OBJMSG_RPG_REPLACEPICKUP:
		{
			m_Msg_OnPickup.SendMessage(m_iObject, _Msg.m_iSender, m_pWServer);
			// Make sure this only runs once
			m_Msg_OnPickup.m_Msg = 0;

			m_iItemRepickCountdown = _Msg.m_Param0;
/*			if(_Msg.m_Param1)
				m_Flags |= FLAGS_FALL;
			else
				m_Flags &= ~FLAGS_FALL;*/

			return SetItem((CRPG_Object_Item *)_Msg.m_pData);
		}

	case OBJMSG_RPG_ISPICKUP:
		if(m_Flags & FLAGS_WAITSPAWN)
			return false;
		else
			return true;

	case OBJMSG_RPG_GETPICKUPANIM:
		return m_iPickupAnim;

	case OBJMSG_GAME_SPAWN:
		{
			if(!(m_Flags & FLAGS_WAITSPAWN))
				return 0;
			m_Flags &= ~FLAGS_WAITSPAWN;
			ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
			//m_pWServer->Phys_InsertPosition(m_iObject, this);
			//m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);
			CreatePhys();
			return 1;
		}
	case OBJMSG_CHAR_GETUSENAME:
		{
			CFStr *pSt = (CFStr *)_Msg.m_pData;
			if(!pSt || !m_spDummyObject)
				return 0;

			*pSt = m_spDummyObject->GetItemName();
			return 1;
		}
	case OBJMSG_CHAR_GETDESCNAME:
		{
			CFStr *pSt = (CFStr *)_Msg.m_pData;
			if(!pSt || !m_spDummyObject)
				return 0;

			*pSt = m_spDummyObject->GetItemDesc();
			/*if (m_spDummyObject->m_AnimProperty == AG_EQUIPPEDITEMCLASS_ASSAULT)
			{
				if (!m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_USEDNAWEAPONS,2),_Msg.m_iSender) == 1)
					*pSt = m_spDummyObject->GetItemDesc();
			}*/
			
			return 1;
		}
	case OBJMSG_CHAR_SETUSENAME:
		{
			if(!_Msg.m_pData || !m_spDummyObject)
				return 0;

			m_spDummyObject->m_Name = (const char *)_Msg.m_pData;
			return 1;
		}
	case OBJMSG_CHAR_SETDESCNAME:
		{
			if(!_Msg.m_pData || !m_spDummyObject)
				return 0;

			m_spDummyObject->m_Name = (const char *)_Msg.m_pData;
			return 1;
		}
	case OBJMSG_CHAR_GETFOCUSOFFSET:
		{
			if(m_spDummyObject == NULL)
				return 0;

			return m_spDummyObject->m_FocusFrameOffset;
		}
	case OBJMSG_CHAR_SETFOCUSFRAMEOFFSET:
		{
			if(m_spDummyObject == NULL)
				return 0;

			m_spDummyObject->m_FocusFrameOffset = (int8)_Msg.m_Param0;
			return true;
		}
	case OBJMSG_RPG_SETTEMPPICKUPSURFACE:
		m_CreationGameTick = m_pWServer->GetGameTick();
		Data(0) = _Msg.m_Param0;
		Data(1) = m_pWServer->GetGameTick() + _Msg.m_Param1;
		return 1;

	case OBJMSG_ACTIONCUTSCENE_GETPICKUPITEMTYPE:
		{
			if(m_spDummyObject && !IsDestroyed())
			{
				if (_Msg.m_pData && _Msg.m_DataSize >= sizeof(int32))
				{
					*(int32*)_Msg.m_pData = m_spDummyObject->m_AnimProperty;
				}
				return m_spDummyObject->m_iItemType;
			}
			break;
		}

	}

	return CWObject_Object::OnMessage(_Msg);
}

void CWObject_Item::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Object_OnClientRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Object::OnClientRender);

	CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
	AnimState.m_AnimTime1 = CMTime::CreateFromSeconds(0.0f);

	uint LightIndex = _pObj->m_Data[DATA_LAMP_LIGHTINDEX];
	if (LightIndex)
	{
		// Non-lamp object with lightindex - implies no shadow should be cast from object
		uint LightGUID = _pEngine->m_pSceneGraphInstance->SceneGraph_Light_GetGUID(LightIndex);
		AnimState.m_NoShadowLightGUID = LightGUID;
	}

	
	AnimState.m_Data[1] = 0; // usercolor (for lamps)

	// Add pickupglow for the pickups
	uint32 RenderFlags = 0;
	{
		CXW_Surface* pOrgSurf = _pEngine->m_pSC->GetSurface("pickupglow");
		if(pOrgSurf)
		{
			CXW_Surface* pSurf = pOrgSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
			if (pSurf)
			{
				AnimState.m_lpSurfaces[0] = pSurf;
				RenderFlags |= CXR_MODEL_ONRENDERFLAGS_SURF0_ADD;
			}
		}
	}

	DoRender(_pObj, _pWClient, _pEngine, AnimState,RenderFlags);


/*	MAUTOSTRIP(CWObject_Item_OnClientRender, MAUTOSTRIP_VOID);
	if(_pObj->m_ClientFlags & CLIENTFLAGS_ATTACHED)
	{
	}
	else
	{
		CMat4Dfp32 MatIP;
		fp32 IPTime = _pWClient->GetRenderTickFrac();
		Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

		CMat4Dfp32 Mat;
		if(_pObj->m_ClientFlags & CLIENTFLAGS_ROTATE)
		{
			fp32 TickTime = _pWClient->GetGameTickTime();
			CMTime Time = CMTime::CreateFromTicks(_pObj->GetAnimTick(_pWClient), TickTime, _pWClient->GetRenderTickFrac() - _pObj->m_CreationGameTickFraction);
			CMat4Dfp32 Rot;
			Rot.SetZRotation(Time.GetTimeModulusScaled(0.01f / TickTime, 1.0f));
			Rot.Multiply(MatIP, Mat);
		}
		else
			Mat = MatIP;

		int iModel = _pObj->m_iModel[0];
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(iModel);
		if (pModel)
		{
			uint32 RenderFlags = 0;
			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
			//AnimState.m_AnimTime1 = AnimState.m_AnimTime0;
			//AnimState.m_AnimTime0.Reset();
			{
				CXW_Surface* pOrgSurf = _pEngine->m_pSC->GetSurface("pickupglow");
				if(pOrgSurf)
				{
					CXW_Surface* pSurf = pOrgSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
					if (pSurf)
					{
						AnimState.m_lpSurfaces[0] = pSurf;
						RenderFlags |= CXR_MODEL_ONRENDERFLAGS_SURF0_ADD;
					}
				}
			}
			if(_pObj->m_Data[1] != 0 && _pObj->m_Data[1] >= _pWClient->GetGameTick())
			{
				CXW_Surface* pSurf = _pWClient->GetMapData()->GetResource_Surface(_pObj->m_Data[0]);
				if (pSurf)
				{
					AnimState.m_lpSurfaces[0] = pSurf;
					RenderFlags |= CXR_MODEL_ONRENDERFLAGS_SURF0_ADD;
				}
			}
			_pEngine->Render_AddModel(pModel, Mat, AnimState, XR_MODEL_STANDARD, RenderFlags);
		}
	}*/
}

/*void CWObject_Item::OnRefresh()
{
	MAUTOSTRIP(CWObject_Item_OnRefresh, MAUTOSTRIP_VOID);

	if(!(m_ClientFlags & CLIENTFLAGS_ATTACHED) && (m_Flags & FLAGS_FALL))
	{
		if (!(m_nPhysIdleTicks > ITEM_PHYS_IDLETICKS) &&
			(GetMoveVelocity().LengthSqr() < 0.0001f))
		{
			CVec3Dfp32 Vel = GetMoveVelocity();
			Vel[2] -= 1.0f;
			{
				// No friction hack
				Vel[0] *= 0.95f;
				if(Abs(Vel[0]) < 0.1f)
					Vel[0] = 0;
				Vel[1] *= 0.95f;
				if(Abs(Vel[1]) < 0.1f)
					Vel[1] = 0;
			}
			SetVelocity(Vel);
			m_pWServer->Object_MovePhysical(m_iObject);

			if (GetMoveVelocity().LengthSqr() < 0.0001f)
				m_nPhysIdleTicks++;
			else
				m_nPhysIdleTicks = 0;
		}
	}

	CWObject_Physical::OnRefresh();
}*/

void CWObject_Item::OnDeltaSave(CCFile* _pFile)
{
	m_ItemTemplate.Write(_pFile);
	_pFile->WriteLE(m_Flags);
}

void CWObject_Item::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	m_ItemTemplate.Read(_pFile);
	SetItem(m_ItemTemplate);

	bool bWaitSpawned = ((m_Flags & FLAGS_WAITSPAWN) != 0);
	_pFile->ReadLE(m_Flags);
	if (bWaitSpawned && !(m_Flags & FLAGS_WAITSPAWN))
	{
		//Spawn object
		ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
		m_pWServer->Phys_InsertPosition(m_iObject, this);
		m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);
	}
}

void CWObject_Item::OnSpawnWorld()
{
	MAUTOSTRIP(CWObject_Item_OnSpawnWorld, MAUTOSTRIP_VOID);
	CWObject_Object::OnSpawnWorld();

	m_Msg_OnPickup.SendPrecache(m_iObject, m_pWServer);
	CreatePhys();
}

void CWObject_Item::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	CWObject_Object::OnIncludeClass(_pWData,_pWServer);
	_pWData->GetResourceIndex_BSPModel("Phys\\P_ItemBox.xw");
}

void CWObject_Item::CreatePhys()
{
	if (!(m_Flags & (FLAGS_PHYSSET|FLAGS_WAITSPAWN)) && m_spDummyObject)
	{
		m_Flags |= FLAGS_PHYSSET;
		if (!(m_Flags & FLAGS_NODYNAMIC))
		{
			CWObject_Object::SPart Part;
			CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_spDummyObject->m_iPhysModel);
			CBox3Dfp32 Box;
			if (pModel)
			{
				pModel->GetBound_Box(Box);
			}
			else
			{
				Box.m_Min = CVec3Dfp32(-8.0f,-4.0f,-4.0f);
				Box.m_Max = CVec3Dfp32(8.0f,4.0f,4.0f);
			}
			Part.m_iModel[0] = m_spDummyObject->m_Model.m_iModel[0];
			// Mass must be over 30 if we are going to grab it...
			Part.m_Mass = 15.0f;
			//Part.m_PhysPrim.m_iPhysModel = pAttachModel->m_iModel[0];
			Part.m_PhysPrim.m_iPhysModel_Dynamics = m_spDummyObject->m_iPhysModel;//pAttachModel->m_iModel[0];
			Part.m_PhysPrim.m_BoundBox.m_Max = Box.m_Max;
			Part.m_PhysPrim.m_BoundBox.m_Min = Box.m_Min;
			Part.m_Flags |= FLAGS_SPAWNACTIVE|FLAGS_GAMEPHYSICS;
			//Part.m_lAttach_DemonArm.Add(CVec3Dfp32(0.0f,0.0f,0.0f));
			//AddPart(Part);
				
			aint lParams[2];
			lParams[0] = NULL;
			lParams[1] = (aint)&Part;
			OnInitInstance(lParams,2);
			ObjectSpawn(true);
		}
		else
		{
			iModel(0) = m_spDummyObject->m_Model.m_iModel[0];
			CWO_PhysicsState Phys;
			Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(9.0f, 9.0f, 9.0f), 0);
			Phys.m_nPrim = 1;

			Phys.m_ObjectFlags = OBJECT_FLAGS_PICKUP;
			Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
			Phys.m_PhysFlags = 	OBJECT_PHYSFLAGS_ROTATION | OBJECT_PHYSFLAGS_SLIDEABLE |
				OBJECT_PHYSFLAGS_FRICTION | OBJECT_PHYSFLAGS_PHYSMOVEMENT |
				OBJECT_PHYSFLAGS_ROTREFLECT | OBJECT_PHYSFLAGS_ROTVELREFLECT;

			m_IntersectNotifyFlags = OBJECT_FLAGS_PLAYER;

			m_pWServer->Object_SetPhysics(m_iObject, Phys);
		}
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Item, CWObject_Object, 0x0100);
