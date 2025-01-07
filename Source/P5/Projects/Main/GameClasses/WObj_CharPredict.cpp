#include "PCH.h"

#include "WObj_Char.h"
#include "../GameWorld/WClientMod_Defines.h"

#include "WObj_Misc/WObj_ActionCutscenecamera.h"
#include "WObj_Misc/WObj_ActionCutscene.h"
#include "WObj_Misc/WObj_CreepingDark.h"
#include "WObj_Misc/WObj_Ledge.h"
#include "WObj_Char/WObj_CharDarkling_ClientData.h"

#include "WRPG/WRPGSpell.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character client prediction execution logic
					
	Contents:		Char_ClientProcessControl
					OnPredictPress
					ClientPredictFrame
					OnClientPredict
					etc..
\*____________________________________________________________________________________________*/

void CWObject_Character::Char_ClientProcessControl(const CControlFrame& _Msg, int& _Pos, CWObject_Client* _pObj, CWorld_Client* _pWClient, bool _bLog)
{
	MAUTOSTRIP(CWObject_Character_Char_ClientProcessControl, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Character::Char_ClientProcessControl);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if (!pCD) return;

/*	if(pCD->m_iMountedObject != 0 && (pCD->m_bIsClientRefresh && !pCD->m_bIsPredicting))
	{
		CWObject_Message Msg(OBJMSG_CHAR_PROCESSCONTROL, _pObj->m_iObject, _Pos);
		Msg.m_pData = (void *)&_Msg;
		if(_pWClient->ClientMessage_SendToObject(Msg, pCD->m_iMountedObject))
		{
			int Ctrl, Size, dTime;
			_Msg.GetCmd(_Pos, Ctrl, Size, dTime);
			_Pos += Size;
			return;
		}
	}*/

	int& Pos = _Pos;
	{	
		int Ctrl;
		int Size;
		int dTime;
//		int ID = _Msg.m_Data[Pos+3];
		_Msg.GetCmd(Pos, Ctrl, Size, dTime);
		if (Pos + Size > _Msg.m_MsgSize) return;
		int SavePos = Pos;

#ifdef LOG_CLIENTAUTHORING_CMD
		if (_bLog) M_TRACEALWAYS(CStrF("      CLIENT ID %d, Pos %d, CMD %d, dTime %d\r\n", ID, Pos, Ctrl, dTime));
#endif

		{
			if (Ctrl == CONTROL_LOOK)
			{
				if ((Size == 6) && (Char_CanLook(_pObj)))
				{
					CVec3Dfp32 dLook = _Msg.GetVecInt16_Max32768(Pos);
					dLook *= 1.0f / 65536;
					pCD->Char_ProcessControl_Look(dLook);
				}
			}
			else if (Ctrl == CONTROL_MOVE)
			{
				if (Size == 6)
				{
					CVec3Dfp32 Move = _Msg.GetVecInt16_Max32768(Pos) * (1.0f / 256.0f);
					pCD->Char_ProcessControl_Move(Move);
				}
				else
				{
					pCD->m_MoveDir = 0;
					pCD->m_Control_Move = 0;
				}
			}
#ifdef WADDITIONALCAMERACONTROL
			else if (Ctrl == CONTROL_LOOKADDITIONAL)
			{
				if ((Size == 6) && Char_CanLook(_pObj))
				{
					CVec3Dfp32 dLook = _Msg.GetVecInt16_Max32768(Pos);
					//				ConOutL("(CWObject_Character::Char_ProcessControl) dLook " + dLook.GetString());
					dLook *= (1.0f / 65536);

					// In noclip mode
					pCD->m_AdditionalCameraControlY = Min(0.249f, Max(-0.249f, pCD->m_AdditionalCameraControlY + dLook[1]));
					pCD->m_AdditionalCameraControlZ = M_FMod((fp32)pCD->m_AdditionalCameraControlZ + dLook[2] + 2.0f, 1.0f);
				}
			}
			else if (Ctrl == CONTROL_MOVEADDITIONAL)
			{
				if (Size == 6)
				{
					pCD->m_Control_MoveAdditional = _Msg.GetVecInt16_Max32768(Pos) * (1.0f / 256.0f);
				}
				else
				{
					pCD->m_Control_MoveAdditional = 0.0f;
				}
			}
#endif
			else if (Ctrl == CONTROL_STATEBITS)
			{
				if (Size == 4)
				{
					pCD->m_Control_Press = _Msg.GetInt32(Pos);

					if (pCD->m_Control_Press & CONTROLBITS_PRIMARY && 
						pCD->m_Control_Released & CONTROLBITS_PRIMARY)
						pCD->AddTilt(CVec3Dfp32(1,0,0), -0.00134f);

					pCD->m_Control_Released |= (~pCD->m_Control_Press) & (~PLAYER_CONTROLBITS_NOAUTORELEASE);
					pCD->m_Control_Released &= (~pCD->m_Control_Press) | PLAYER_CONTROLBITS_NOAUTORELEASE;
//					_Released |= ~_Press;
//					ConOutL(CStrF("Press %.8x", m_Press));
				}
			}
			else if (Ctrl == CONTROL_ANALOG)
			{
				int8 Channel = _Msg.GetInt8(Pos);
				uint8 v = _Msg.GetInt8(Pos);
				switch(Channel)
				{
					case 0:
						{
							if (pCD->m_AnalogMode & M_Bit(0))
							{
								pCD->m_Analog0 =  pCD->m_AnalogMode & M_Bit(0) ? Min(v,pCD->m_Analog0.m_Value) : v;
								if (v <= 5)
									pCD->m_AnalogMode = pCD->m_AnalogMode & ~M_Bit(0);
								break;
							}
							pCD->m_Analog0 = v;
							break;
						}
					case 1:
						{
							if (pCD->m_AnalogMode & M_Bit(1))
							{
								pCD->m_Analog1 =  pCD->m_AnalogMode & M_Bit(1) ? Min(v,pCD->m_Analog1.m_Value) : v;
								if (v <= 5)
									pCD->m_AnalogMode = pCD->m_AnalogMode & ~M_Bit(1);
								break;
							}
							pCD->m_Analog1 = v;
							break;
						}
					case 2: pCD->m_Analog2 = v; break;
					case 3: pCD->m_Analog3 = v; break;
					default:
						M_ASSERT(false, "Erroneous channel");	// Look for MAX_ANALOGCHANNELS in WClientP4.h
				}
			}
			else if (Ctrl == CONTROL_CMD)
			{
				int Cmd = (Size > 0) ? _Msg.GetInt8(Pos) : -1;
//				int d0 = (Size > 1) ? _Msg.GetInt8(Pos) : 0;
//				int d1 = (Size > 2) ? _Msg.GetInt8(Pos) : 0;
//				int d2 = (Size > 3) ? _Msg.GetInt8(Pos) : 0;
				if(Size > 1) _Msg.GetInt8(Pos);
				if(Size > 2) _Msg.GetInt8(Pos);
				if(Size > 3) _Msg.GetInt8(Pos);

				if (Cmd == CMD_NOCLIP)
				{
					// NOCLIP
//					if (Char_CheatsEnabled())
					{
						if(Char_GetPhysType(_pObj) == PLAYER_PHYS_NOCLIP)
							Char_SetPhysics(_pObj, _pWClient, NULL, PLAYER_PHYS_CROUCH);
						else if(!(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_SPECTATOR))
							Char_SetPhysics(_pObj, _pWClient, NULL, PLAYER_PHYS_NOCLIP);
					}

				}
			}

		}
		Pos = SavePos + Size;
	}

}

bool CWObject_Character::Char_PredictActivateItem(CWorld_Client* _pWClient, CWObject_Client* _pObj, CAutoVar_WeaponStatus* _pWeapStat, CAutoVar_WeaponStatus* _pWeapStatFirst, CWO_Character_ClientData* _pCD, CWO_Character_ClientData* _pCDFirst, int _iToken)
{
	// Check for gunkatas...
	if (_pCDFirst)
	{
		int16 iGunkataObj = _pCDFirst->m_iGunkataOpponent;
		if (_pCD->m_GameTick - _pCDFirst->m_LastGunkataOpponentCheck > 6)
		{
			int8 FocusType = (_pCD->m_FocusFrameType & SELECTION_MASK_TYPEINVALID);
			int32 iFocusObj = _pCD->m_iFocusFrameObject;
			if (FocusType && FocusType != SELECTION_CHAR)
			{
				// Try to find a char instead
				Char_FindBestOpponent(_pWClient,_pObj,iFocusObj);
			}
			_pCDFirst->m_iGunkataOpponent = iFocusObj;
			_pCDFirst->m_LastGunkataOpponentCheck = _pCD->m_GameTick;
			iGunkataObj = _pCDFirst->m_iGunkataOpponent;
		}
		if (iGunkataObj != -1 && Char_CheckGunKata(_pWClient,_pObj,_pCD->m_AnimGraph2.GetPropertyInt(PROPERTY_INT_STANCESTANDING),iGunkataObj))
			return false;

		// Play activation sound (only once though!!!)
		if ((_pCD->m_GameTick - _pWeapStatFirst->m_LastSoundTick) > 0)
		{
			_pWeapStatFirst->m_LastSoundTick = _pCD->m_GameTick + _pWeapStat->m_FireTimeOut;
			int RepeatTime = _pWeapStat->m_FireTimeOut + 1;
			if ((_pWeapStat->m_Flags & CAutoVar_WeaponStatus::WEAPONSTATUS_SINGLESHOT) || _pWeapStat->m_FireTimeOut > 3)
			{
				_pWClient->Sound_Global(WCLIENT_CHANNEL_SFX, _pWeapStat->m_iSoundActivatePlayer, 1.0f);
			}
			else
			{
				// Repeatsound...
				CNetMsg NetMsg(PLAYER_NETMSG_PLAYSOUND_REPEAT);
				NetMsg.AddInt16(_pWeapStat->m_iSoundActivatePlayer);
				NetMsg.AddInt16(RepeatTime);
				CWObject_Client* pObjFirst = _pWClient->Object_GetFirstCopy(_pObj->m_iObject);
				OnClientNetMsg(pObjFirst,_pWClient,NetMsg);
			}
		}
	}

	_pWeapStat->m_Flags |= CAutoVar_WeaponStatus::WEAPONSTATUS_TRIGGERPRESSED;

	CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_PRIMARYATTACK);
	CWAG2I_Context Context(_pObj,_pWClient,CMTime::CreateFromTicks(_pCD->m_GameTick,_pWClient->GetGameTickTime(),_pCD->m_PredictFrameFrac));
	// Anim and effects
	_pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,Impulse, 0);
	_pCD->m_WeaponAG2.GetAG2I()->SendImpulse(&Context,Impulse, _iToken);
	_pWeapStat->Fire(_pCD, (_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_GODMODE) != 0);

	return true;
}

void CWObject_Character::OnPredictPress(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint32& _Press, const uint32& _Released, CPhysUtilParams& _Params)
{
	MAUTOSTRIP(CWObject_Character_OnPredictPress, MAUTOSTRIP_VOID);
	
	CWO_Character_ClientData* pCD = GetClientData(_pObj);
	if (Char_GetControlMode(_pObj) == PLAYER_CONTROLMODE_LEDGE2)
	{
		CWAG2I_Context AGContext(_pObj,_pWClient,CMTime::CreateFromTicks(pCD->m_GameTick,_pWClient->GetGameTickTime(),pCD->m_PredictFrameFrac));
		CWObject_Ledge::OnPress(&AGContext,pCD,pCD->m_Control_Press,pCD->m_Control_LastPress);
		pCD->m_Control_LastPress = pCD->m_Control_Press;
		return;
	}

	// Predict crouch
	if (((pCD->m_Control_Press & CONTROLBITS_CROUCH) && !(pCD->m_Control_LastPress & CONTROLBITS_CROUCH)) ||
		(!(pCD->m_Control_Press & CONTROLBITS_CROUCH) && (pCD->m_Control_LastPress & CONTROLBITS_CROUCH)))
	{
		/*const CWO_PhysicsState& Phys = _pObj->GetPhysState();
		bool bForce = !(!(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOCHARACTERCOLL) ? 
			(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_GHOST || pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOWORLDCOLL) ||
			(Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSOBJECT) : true);

		if ((pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOPHYSFLAG) || (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOANIMPHYS))
		{
			if (Phys.m_ObjectIntersectFlags & OBJECT_FLAGS_ANIMPHYS)
				bForce = true;
		}
		else
		{
			if (!(Phys.m_ObjectIntersectFlags & OBJECT_FLAGS_ANIMPHYS))
				bForce = true;
		}

		if ((pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOCHARCOLL) || (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOCHARACTERCOLL))
		{
			if (Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSOBJECT)
				bForce = true;
		}
		else
		{
			if (!(Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSOBJECT) && !(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOCHARACTERCOLL))
				bForce = true;
		}

		if ((pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_NOCOLLISIONSUPPORTED) && Char_GetPhysType(_pObj) != PLAYER_PHYS_DEAD)
		{
			if ((pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_NOCOLLISION) ||
				(pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_INVISIBLE))
			{
				// Set
				bForce = bForce || (Phys.m_ObjectIntersectFlags & OBJECT_FLAGS_PHYSOBJECT) != 0;
			}
			else
			{
				bForce = bForce || !(Phys.m_ObjectIntersectFlags & OBJECT_FLAGS_PHYSOBJECT);
			}
		}*/
		int Type;
		if (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_FORCESETPHYS)
			Type = PLAYER_PHYS_CROUCH;
		else
			Type = ((pCD->m_Control_Press & CONTROLBITS_CROUCH) && !(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOCROUCH)) ? PLAYER_PHYS_CROUCH : PLAYER_PHYS_STAND;
		if(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_SPECTATOR)
			Type = PLAYER_PHYS_SPECTATOR;
		CWObject_Character::Char_SetPhysics(_pObj, _pWClient, NULL, Type);
	}

	if (!(pCD->m_Disable & PLAYER_DISABLE_ATTACK) && 
		CWObject_Character::Char_GetPhysType(_pObj) != PLAYER_PHYS_DEAD &&
		CWObject_Character::Char_GetControlMode(_pObj) != PLAYER_CONTROLMODE_ACTIONCUTSCENE)
	{
		CWObject_Client* pObjFirst = _pWClient->Object_GetFirstCopy(_pObj->m_iObject);
		CWO_Character_ClientData* pCDFirst = pObjFirst ? CWObject_Character::GetClientData(pObjFirst) : NULL;
		if (!(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK) && 
			!(pCD->m_AnimGraph2.GetStateFlagsLoCombined() & (AG2_STATEFLAG_BLOCKACTIVE|AG2_STATEFLAG_EQUIPPING)))
		{
			if((pCD->m_Control_Press & CONTROLBITS_PRIMARY && pCD->m_PostAnimSystem.GetSafeToFirePrim()) ||
				(pCD->m_Control_Press & CONTROLBITS_SECONDARY && pCD->m_PostAnimSystem.GetSafeToFireSec()))
			{
				pCD->m_WeaponAG2.UpdateItemProperties(NULL,NULL,_pObj,pCD);
				CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_PRIMARYATTACK);
				if ((pCD->m_Control_Press & CONTROLBITS_PRIMARY) /*&& !(pCD->m_AnimGraph2.GetStateFlagsLoToken2() & AG2_STATEFLAG_RELOADING)*/ &&
					pCD->m_WeaponStatus0.OkToFire(pCD,_pWClient))
				{
					Char_PredictActivateItem(_pWClient, _pObj, &pCD->m_WeaponStatus0, &pCDFirst->m_WeaponStatus0, pCD, pCDFirst,0);
				}
				if ((pCD->m_Control_Press & CONTROLBITS_SECONDARY) /*&& !(pCD->m_AnimGraph2.GetStateFlagsLoToken3() & AG2_STATEFLAG_RELOADING)*/ &&
					pCD->m_WeaponStatus1.OkToFire(pCD,_pWClient))
				{
					Char_PredictActivateItem(_pWClient, _pObj, &pCD->m_WeaponStatus1, &pCDFirst->m_WeaponStatus1, pCD, pCDFirst,1);
				}
			}
		}
		if ((pCD->m_Control_LastPress & CONTROLBITS_PRIMARY) && !(pCD->m_Control_Press & CONTROLBITS_PRIMARY))
		{
			if (pCD->m_WeaponStatus0.m_Flags & CAutoVar_WeaponStatus::WEAPONSTATUS_TRIGGERPRESSED)
			{
				// Deactivate repeat sound
				// Repeatsound...
				if ((pCD->m_WeaponStatus0.m_FireTimeOut <= 3) && 
					!(pCD->m_WeaponStatus0.m_Flags & CAutoVar_WeaponStatus::WEAPONSTATUS_SINGLESHOT))
				{
					CNetMsg NetMsg(PLAYER_NETMSG_PLAYSOUND_REPEAT);
					OnClientNetMsg(pObjFirst,_pWClient,NetMsg);
				}
			}
			pCD->m_WeaponStatus0.m_Flags &= ~CAutoVar_WeaponStatus::WEAPONSTATUS_TRIGGERPRESSED;
		}
		if ((pCD->m_Control_LastPress & CONTROLBITS_SECONDARY) && !(pCD->m_Control_Press & CONTROLBITS_SECONDARY))
		{
			if (pCD->m_WeaponStatus1.m_Flags & CAutoVar_WeaponStatus::WEAPONSTATUS_TRIGGERPRESSED)
			{
				// Repeatsound...
				if ((pCD->m_WeaponStatus1.m_FireTimeOut <= 3) && 
					!(pCD->m_WeaponStatus1.m_Flags & CAutoVar_WeaponStatus::WEAPONSTATUS_SINGLESHOT))
				{
					CNetMsg NetMsg(PLAYER_NETMSG_PLAYSOUND_REPEAT);
					OnClientNetMsg(pObjFirst,_pWClient,NetMsg);
				}
			}
			pCD->m_WeaponStatus1.m_Flags &= ~CAutoVar_WeaponStatus::WEAPONSTATUS_TRIGGERPRESSED;
		}
	}

	pCD->m_Control_LastPress = pCD->m_Control_Press;
/*
	if (Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD)
		return;

	CWO_Character_ClientData *pCD= CWObject_Character::GetClientData(_pObj);
	if (!pCD) return;
*/
	// Attack animation prediction.
/*	if(!(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_WAIT))
	{
		if((_Press & CONTROLBITS_PRIMARY) && pCD->m_Item0_ActivateAnim != -1)
			Char_SetAnimTorsoSequence(_pObj, _pWClient, pCD->m_Item0_ActivateAnim);
		
		else if((_Press & CONTROLBITS_SECONDARY) && pCD->m_Item1_ActivateAnim != -1)
			Char_SetAnimTorsoSequence(_pObj, _pWClient, pCD->m_Item1_ActivateAnim);

		else if((_Press & CONTROLBITS_SECONDARY) && (pCD->m_Item1_Flags & RPG_ITEM_FLAGS_CANBLOCK))
			_pObj->m_ClientFlags |= PLAYER_CLIENTFLAGS_BLOCKING;
	}

	if(_Released & CONTROLBITS_SECONDARY && (pCD->m_Item1_Flags & RPG_ITEM_FLAGS_CANBLOCK))
		_pObj->m_ClientFlags &= ~PLAYER_CLIENTFLAGS_BLOCKING;*/
}

int CWO_Character_ClientData::OnClientPredictControlFrame(CWorld_Client* _pWClient, CWObject_Client* _pPredict, CControlFrame& _Ctrl, bool _bFullFrame, bool _bLog)
{
	MAUTOSTRIP(CWO_Character_ClientData_OnClientPredictControlFrame, 0);
	MSCOPE(CWO_Character_ClientData::OnClientPredictControlFrame, CHARACTER);
	if (_pPredict->m_ClientFlags & PLAYER_CLIENTFLAGS_SERVERCONTROL)
	{
		int CmdIDEnd = -1;
		int ServerPos = _Ctrl.GetCmdStartPos();
		while(ServerPos < _Ctrl.m_MsgSize)
		{
			CCmd Cmd;
			_Ctrl.GetCmd(ServerPos, Cmd);
			CmdIDEnd = Cmd.m_ID;
		}
		return CmdIDEnd;
	}

//	const_cast<CMat4Dfp32&>(_pPredict->GetLastPositionMatrix()) = _pPredict->GetPositionMatrix();
	_pPredict->OverrideLastPositionMatrix( _pPredict->GetPositionMatrix() );

	int CmdTotalIDStart = -1;	// These are for debugging only
	int CmdTotalIDEnd = -1;

	// Create selection
	CBox3Dfp32 Bound = *_pPredict->GetAbsBoundBox();
	{
		CBox3Dfp32 Bound2(Bound);
		Bound2.m_Min += _pPredict->GetMoveVelocity();
		Bound2.m_Max += _pPredict->GetMoveVelocity();
		Bound2.m_Min -= 16.0f;
		Bound2.m_Max += 16.0f;
		Bound.Expand(Bound2);
	}


	CWO_Character_ClientData* pCD = this;

	bool bPredicted = CWObject_Player::IsPredicted(_pPredict, _pWClient);	// This might seem odd, but ClientPredictFrame is run on all local players.

//	if(!(_pPredict->m_ClientFlags & PLAYER_CLIENTFLAGS_SPECTATOR))
//		int i = 0;

	
	// Not dead?
	int PhysType = CWObject_Character::Char_GetPhysType(_pPredict);
	if (PhysType != PLAYER_PHYS_DEAD)
	{
		if (PhysType != PLAYER_PHYS_NOCLIP)
		{
			// Phobos: Added check for "NOCROUCH" flag so that the physbox won't get "crouched" when the character is not
			// Try with crouched if we need to force set physics...
			int Type;
			if (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_FORCESETPHYS)
				Type = PLAYER_PHYS_CROUCH;
			else
				Type = ((pCD->m_Control_Press & CONTROLBITS_CROUCH) && !(_pPredict->m_ClientFlags & PLAYER_CLIENTFLAGS_NOCROUCH)) ? PLAYER_PHYS_CROUCH : PLAYER_PHYS_STAND;
			if(_pPredict->m_ClientFlags & PLAYER_CLIENTFLAGS_SPECTATOR)
				Type = PLAYER_PHYS_SPECTATOR;
			CWObject_Character::Char_SetPhysics(_pPredict, _pWClient, NULL, Type);
		}
 
		CPhysUtilParams Params;	// Dummy
		CWObject_Character::OnPredictPress(_pPredict, _pWClient, pCD->m_Control_Press, pCD->m_Control_Released, Params);
	}
	else
	{
	}

	{
		MSCOPE(PredictedClientRefresh, CHARACTER);
		
		CWObject_Character::OnClientRefresh_TrueClient(pCD, _pPredict, _pWClient);
		CWObject_Character::OnRefresh_ServerPredicted(pCD, _pPredict, _pWClient);
	}
	// Yes all of this is a hack to be sure Riddick is shown in actioncutscenes
	if (CWObject_Character::Char_GetControlMode(_pPredict) == PLAYER_CONTROLMODE_ACTIONCUTSCENE)
	{
		uint32 CameraMode = pCD->m_ActionCutSceneCameraMode;
		uint32 ACSType = (uint32)pCD->m_ControlMode_Param2;
		uint32 Flags = pCD->m_ControlMode_Param4;
		if ((Flags & ACTIONCUTSCENE_FLAGS_USEFLOATINGSTART) &&
			((ACSType == ACTIONCUTSCENE_TYPE_USEHEALTHSTATION) || (CameraMode & CActionCutsceneCamera::ACS_CAMERAMODE_ACTIVE)))
		{
			CMat4Dfp32 Mat;
			if (CWObject_ActionCutscene::GetAnimPosition(Mat, pCD, _pWClient))
			{
				Mat.Unit3x3();
				//ConOut(CStrF("Now: %s Last: %s",CVec3Dfp32::GetMatrixRow(Mat,3).GetString().GetStr(),_pPredict->GetLastPosition().GetString().GetStr()));
				_pWClient->Object_ForcePosition_World(_pPredict->m_iObject, Mat);
				//_pWClient->Debug_RenderAABB(*_pPredict->GetAbsBoundBox());
			}
		}
		else
		{
			// Set correct position
			//ConOut(CStrF("SETTING OLD POS: %s", CVec3Dfp32::GetMatrixRow(pCD->m_DialoguePlayerPos,3).GetString().GetStr()));
			CWObject_Client* pObjPlayer = (CWObject_Client*)_pWClient->Object_GetCD(_pWClient->Player_GetLocalObject());
			CRegistry* pUserReg = _pWClient->Registry_GetUser();
			uint bNoPredict = (pUserReg) ? pUserReg->GetValuei("NOPREDICT") : 0;
			if (!bNoPredict)
			{
				CWObject_Client* pCrapACS = _pWClient->Object_GetFirstCopy(_pPredict->m_iObject);
				while(pCrapACS)
				{
					CWO_Character_ClientData *pCDCrap = CWObject_Character::GetClientData(pCrapACS);
					if (pCDCrap)
						pCDCrap->m_PredMiss_dPos = 0.0f;
					pObjPlayer->OverridePositionMatrix(pCD->m_DialoguePlayerPos);
					pObjPlayer->OverrideLastPositionMatrix(pCD->m_DialoguePlayerPos);
					pCrapACS = pCrapACS->GetNext();
				}
			}
			if (pObjPlayer)
			{
				pObjPlayer->OverridePositionMatrix(pCD->m_DialoguePlayerPos);
				pObjPlayer->OverrideLastPositionMatrix(pCD->m_DialoguePlayerPos);
			}
			_pWClient->Object_SetPosition_World(_pPredict->m_iObject, pCD->m_DialoguePlayerPos);
			_pWClient->Object_ForcePosition_World(_pPredict->m_iObject, pCD->m_DialoguePlayerPos);

			//_pPredict->m_LastPos = pCD->m_DialoguePlayerPos;
		}
	}

//	ConOutL(CStrF("Client %f", pCD->m_GameTime));
	pCD->m_bIsFullFramePrediction = _bFullFrame;

	TSelection<CSelection::LARGE_BUFFER> PhysSelection;
	pCD->m_PhysSelection = &PhysSelection;
	_pWClient->Selection_AddBoundBox(*pCD->m_PhysSelection, 
		_pPredict->GetPhysState().m_ObjectFlags | 
		_pPredict->GetPhysState().m_ObjectIntersectFlags,
		Bound.m_Min, Bound.m_Max);

	pCD->m_Control_Look_Wanted = CWObject_Character::GetLook(_pPredict->GetPositionMatrix());
	pCD->Char_UpdateLook(0.0f); // FIXME: UPDATELOOK

	int CmdIDStart = -1;
	int CmdIDEnd = -1;

	int OldTime = 0;
	int DataPos = _Ctrl.GetCmdStartPos();
	int SkipTime = 0;
	bool bSkippedLast = false;

	int nUserVel = 1;
	CVec3Dfp32 Vel = 0;
	pCD->m_Control_Released &= (~pCD->m_Control_Press) | PLAYER_CONTROLBITS_NOAUTORELEASE;

	// MEMOPT: Removed
//	pCD->ControlSnapshot();
	pCD->m_PredictFrameFrac = 0;

	while(DataPos < _Ctrl.m_MsgSize)
	{
//		int LastPress = pCD->m_Control_Press;
//		CVec3Dfp32 LastMove = pCD->m_Control_Move;

//					if (bNoPredict) continue;

		CCmd Cmd;
		int DataPos2 = DataPos;
		_Ctrl.GetCmd(DataPos2, Cmd);

		if (CmdIDStart == -1) CmdIDStart = Cmd.m_ID;
		CmdIDEnd = Cmd.m_ID;

		if (CWObject_Character::Phys_ControlAffectsMovement(_Ctrl, DataPos, _pPredict, pCD))
//		if ((Cmd.m_Cmd == CONTROL_MOVE && pCD->m_Control_Move != LastMove && (Params.m_Flags & 1)) ||
//			(Cmd.m_Cmd == CONTROL_STATEBITS && pCD->m_Control_Press != LastPress))
		{
			int Time = Cmd.m_dTime;
			if (Time - OldTime > 0)
			{
				fp32 dT = fp32(Time - OldTime) / 128.0f;

//#ifdef LOG_PREDICTION
				if (_bLog) M_TRACEALWAYS(CStrF("  Predicting ID %d, dTime %d, Ctrl %d, Size %d, Move %s, Look %s, Vel %s\r\n", Cmd.m_ID, Cmd.m_dTime, Cmd.m_Cmd, Cmd.m_Size, (char*)pCD->m_Control_Move.GetString() , (char*)pCD->m_Control_Look.GetString(), (char*)Vel.GetString() ));
//#endif
				M_ASSERT(pCD->m_pObj == _pPredict, "!");
				if (_bLog) M_TRACEALWAYS(CStrF("    Predict Move from %s\r\n", pCD->m_pObj->GetPosition().GetString().GetStr()));
				pCD->Phys_Move(*pCD->m_PhysSelection, dT, Vel, bPredicted);
				if (_bLog) M_TRACEALWAYS(CStrF("    Predict Move to %s\r\n", pCD->m_pObj->GetPosition().GetString().GetStr()));
				OldTime = Time;
			}
			else
				if (_bLog) M_TRACEALWAYS(CStrF("  Predicting ID %d, dTime %d, Ctrl %d, Size %d, Move %s, Look %s\r\n", Cmd.m_ID, Cmd.m_dTime, Cmd.m_Cmd, Cmd.m_Size, (char*)pCD->m_Control_Move.GetString() , (char*)pCD->m_Control_Look.GetString()));

			nUserVel++;

			// MEMOPT: Removed
			//pCD->ControlSnapshot();

			bSkippedLast = false;
		}
		else
		{
			bSkippedLast = true;
			SkipTime = Cmd.m_dTime;
		}

		pCD->m_PredictFrameFrac = fp32(Cmd.m_dTime) / 128.0f;
		CWObject_Character::Char_ClientProcessControl(_Ctrl, DataPos, _pPredict, _pWClient, _bLog);
		{
			CPhysUtilParams Params;	// <-- Useless, remove param from OnPress.
			CWObject_Character::OnPredictPress(_pPredict, _pWClient, pCD->m_Control_Press, pCD->m_Control_Released, Params);
			pCD->m_Control_Released &= (~pCD->m_Control_Press) | PLAYER_CONTROLBITS_NOAUTORELEASE;
		}
	}

	// Move the remaining period of time for this frame, IF there's any additional frames left to execute.
	int Time = OldTime;
	if (_bFullFrame)
		Time = 128;
	else if (bSkippedLast)
		Time = SkipTime;
		
	if (Time - OldTime > 0)
	{
//		if (_bLog) ConOutL(CStrF("Predict Frame remain %d", Time - OldTime));

		fp32 dT = fp32(Time - OldTime) / 128.0f;
		M_ASSERT(pCD->m_pObj == _pPredict, "!");
		if (_bLog) M_TRACEALWAYS(CStrF("    Fraction Move from %s\r\n", pCD->m_pObj->GetPosition().GetString().GetStr()));
		pCD->Phys_Move(*pCD->m_PhysSelection, dT, Vel, bPredicted);
		if (_bLog) M_TRACEALWAYS(CStrF("    Fraction Move to %s\r\n", pCD->m_pObj->GetPosition().GetString().GetStr()));
	}

	// Set orientation according to the look.
 	CWorld_PhysState* pPhysState = _pWClient;
	CMat4Dfp32 Mat;
	pCD->Char_UpdateLook(fp32(Time)/128);
	
	// Change direction according to what relanimpos wants (if active)
	// Should moderate this bitch instead?
	/*if (pCD->m_RelAnimPos.HasDirection())
	{
		//CVec3Dfp32 TargetAngle;
		//pCD->m_RelAnimPos.GetTargetDirection(pCD->m_Control_Look_Wanted);
		pCD->m_RelAnimPos.ModerateLook(pCD);
		fp32 FrameFrac = fp32(Time)/128;
		// Only look on opponent if he's alive...
		pCD->m_Control_Look_Wanted[2] = LERP(pCD->m_FightLastAngleX, pCD->m_FightTargetAngleX, 
			FrameFrac);
		pCD->m_Control_Look_Wanted[1] = LERP(pCD->m_FightLastAngleY, pCD->m_FightTargetAngleY, 
			FrameFrac);
		
		pCD->m_Control_Look = pCD->m_Control_Look_Wanted;
	}*/

	if (!pCD->IsDarkling())
	{
		pCD->m_Control_Look_Wanted.CreateMatrixFromAngles(0, Mat);
		if (!(_pPredict->m_ClientFlags & PLAYER_CLIENTFLAGS_NOLOOK) && !(pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOLOOK) &&
			!(CWObject_Character::Char_GetControlMode(_pPredict) == PLAYER_CONTROLMODE_LEDGE2) &&
			!(CWObject_Character::Char_GetControlMode(_pPredict) == PLAYER_CONTROLMODE_HANGRAIL))
			pPhysState->Object_SetRotation(_pPredict->m_iObject, Mat);
	}

	if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_CREEPINGCAM)
	{
		CWObject_CoreData* pCreep = _pWClient->Object_GetCD(pCD->m_iCreepingDark);
		CWObject_CreepingDark::CWObject_SetCreepOrientation(_pWClient, pCreep, pCD->m_Creep_Control_Look_Wanted.m_Value);
	}

	if (_bLog)
		M_TRACEALWAYS(CStrF("  Predicted ID (%s) %d->%d from %s to %s, Move %s, Look %s\r\n", (_bFullFrame) ? "Full" : "Partial", CmdIDStart, CmdIDEnd, (char*)_pPredict->GetLastPosition().GetString(), (char*)_pPredict->GetPosition().GetString(), (char*)pCD->m_Control_Move.GetString() , (char*)pCD->m_Control_Look.GetString()));

	if (CmdTotalIDStart == -1)
		CmdTotalIDStart = CmdIDStart;
	CmdTotalIDEnd = CmdIDEnd;

	pCD->m_PhysSelection = NULL;
	pCD->m_PredictLastCmdID = CmdTotalIDEnd;

	pCD->m_PredictFrameFrac = fp32(Time) / 128.0f;


/*if (_bLog)
	ConOutL(CStrF("RETIRED %.8x, Tick %d, Z %f -> %f = %f, X %f -> %f = %f", 
		_pPredict, pCD->m_GameTick, 
		pCD->m_Anim_LastBodyAngleZ, pCD->m_Anim_BodyAngleZ, LERP(pCD->m_Anim_LastBodyAngleZ, pCD->m_Anim_BodyAngleZ, pCD->m_PredictFrameFrac),
		pCD->m_Anim_LastBodyAngleX, pCD->m_Anim_BodyAngleX, LERP(pCD->m_Anim_LastBodyAngleX, pCD->m_Anim_BodyAngleX, pCD->m_PredictFrameFrac)
		));*/

	return CmdTotalIDEnd;
}

int CWO_Character_ClientData::OnClientCheckPredictionMisses(CWorld_Client* _pWClient, CWObject_Client* _pObj, CWObject_Client* _pPredict, bool _bLog)
{
	m_PredMiss_Tick = _pWClient->GetGameTick()/*+(m_LastUpdateNumTicks-1)*/;
	_pPredict->GetPosition().Sub(_pObj->GetPosition(), m_PredMiss_dPos);
	if (m_PredMiss_dPos.LengthSqr() > Sqr(0.001f))
	{

		CWObject_Player::Player_InvalidatePredictions(_pObj->GetNext());
/*LogFile(CStrF("PredMiss Tick %d, LUNT %d, %f - %f = %f", 
	_pWClient->GetGameTick(), m_LastUpdateNumTicks, 
	_pPredict->GetPosition()[0], _pObj->GetPosition()[0], m_PredMiss_dPos[0]));*/

//		m_PredMiss_dPos *= 1.0f / fp32(m_LastUpdateNumTicks);
	}

#ifndef M_RTM
	fp32 Miss = _pPredict->GetPosition().Distance(_pObj->GetPosition());
	if (Miss > 0.001f)
	{
  #ifndef LOG_CLIENTAUTHORING
		CRegistry* pUserReg = _pWClient->Registry_GetUser();
		int bShowMiss = (pUserReg) ? pUserReg->GetValuei("SHOWMISS") : 0;
		if (bShowMiss)
  #endif //LOG_CLIENTAUTHORING
			M_TRACEALWAYS(CStrF("Prediction miss by %f units. (Predict(0x%.8x) %s != Object(0x%.8x) %s)\r\n", Miss, 
				_pPredict, (char*)_pPredict->GetPosition().GetString(), 
				_pObj, (char*)_pObj->GetPosition().GetString()));
	}
  #ifdef LOG_CLIENTAUTHORING
	else
		M_TRACEALWAYS(CStrF("Prediction success ID %d, Pred %.8x, Base %.8x, %s\r\n", m_PredictLastCmdID, _pPredict, _pObj, (char*)_pPredict->GetPosition().GetString() ));
  #endif
#endif

	return 0;
}



