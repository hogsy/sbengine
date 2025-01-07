#include "PCH.h"
#include "WObj_Hook.h"
#include "MFloat.h"

#include "WObj_Game.h"
#include "WObj_PosHistory.h"
#include "WObj_Damage.h"
#include "../../../../Projects/Main/GameClasses/WObj_Misc/WObj_ScenePoint.h"
#include "../WDynamicsEngine.h"

#ifdef COMPILER_MSVC
#pragma warning(disable : 4756)	// warning C4756: overflow in constant arithmetic, (Pack32/Unpack32, fix someday -JA)
#endif

//
//Impulses:
//	 0 :Stop
//	-1 :Reset
//	-2 :Reverse
//	-3 :Goto end
//




/////////////////////////////////////////////////////////////////////////////
// CWObject_Attach

void CWObject_Attach::OnCreate()
{
	MAUTOSTRIP(CWObject_Attach_OnCreate, MAUTOSTRIP_VOID);

	m_DestroyTime = -1;
	m_DestroyPhysicsTime = -1;
	m_Damage = 0;
	m_Flags = 0;
	m_iActivator = 0;
	m_iCurTimedMessage = 0;
	m_Wait = -1;
	ClientFlags() |= CWO_CLIENTFLAGS_ISATTACHOBJ;
}

void CWObject_Attach::OnImpulse(int _iImpulse, int _iSender)
{
	MAUTOSTRIP(CWObject_Attach_OnImpulse, MAUTOSTRIP_VOID);

	//ConOutL(CStrF("%i. Impulse: Object: %i  Impulse: %i   Sender: %i", m_pWServer->GetGameTick(), m_iObject, _iImpulse, _iSender));
	if (m_Wait != -1 && _iImpulse != -1)
	{
		if((m_Flags & FLAGS_WAITTYPE_MASK) == FLAGS_WAITTYPE_IMPULSE && _iImpulse == m_Wait)
		{
			// Set activator if not previously set. Perhaps we should always set activator in this case for the sake of consistency
			// but that might fuck up existing scripts, so wait until p6 to do so (if at all)
			if (m_iActivator == 0)
				m_iActivator = _iSender;

			m_Wait = -1;
			Run(m_iAnim2 + 1);
		}
		return;
	}

	if(_iImpulse == 0)
		Stop();
	else if(_iImpulse == -1)
		Reset();
	else if(_iImpulse == -2)
		Reverse();
	else if(_iImpulse == -3)
		GotoEnd();
	else
	{
		m_iActivator = _iSender;
		Run(_iImpulse);
	}
}

void CWObject_Attach::Stop()
{
	MAUTOSTRIP(CWObject_Attach_Stop, MAUTOSTRIP_VOID);

	if(m_ClientFlags & CLIENTFLAGS_RUN)
	{
		if(m_Flags & FLAGS_NEVERINTERRUPT)
		{
			m_Flags |= FLAGS_PENDINGSTOP;
		}
		else
		{
			ClientFlags() &= ~CLIENTFLAGS_RUN;
			m_CreationGameTick = m_pWServer->GetGameTick() - m_CreationGameTick;
			CVelocityfp32 Vel;
			Vel.Unit();
			SetVelocity(Vel);
			SetDirty(CWO_DIRTYMASK_COREMASK);

			GetAttachClientData(this)->m_CachedTime.MakeInvalid();
		}

		UpdateNoRefreshFlag();
	}
	ClientFlags() &= ~CLIENTFLAGS_REVERSE;
}

void CWObject_Attach::Run(int _iType)
{
	MAUTOSTRIP(CWObject_Attach_Run, MAUTOSTRIP_VOID);

	if(!(ClientFlags() & CLIENTFLAGS_RUN))
	{
		ClientFlags() |= CLIENTFLAGS_RUN;
		m_CreationGameTick = m_pWServer->GetGameTick() - m_CreationGameTick;
		SetDirty(CWO_DIRTYMASK_COREMASK);

		UpdateNoRefreshFlag();
		GetUpdatedTime();
	}
}

void CWObject_Attach::Reset()
{
	MAUTOSTRIP(CWObject_Attach_Reset, MAUTOSTRIP_VOID);

	Stop();
	m_CreationGameTick = 0;
	m_CreationGameTickFraction = 0;
	m_iCurTimedMessage = 0;
	m_Wait = -1;
	m_Flags &= ~(FLAGS_RESETFIX | FLAGS_PENDINGSTOP | FLAGS_PSEUDOSTOP);
	SetDirty(CWO_DIRTYMASK_COREMASK);

	if(m_ClientFlags & CLIENTFLAGS_PHYSICS)
	{
		CMTime Time = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0);
		CMat4Dfp32 Mat = GetRenderMatrix(m_pWServer, Time, m_pWServer->GetGameTick(), 0);
		m_pWServer->Object_SetPosition_World(m_iObject, Mat);
	}
}

void CWObject_Attach::Reverse()
{
	MAUTOSTRIP(CWObject_Attach_Reverse, MAUTOSTRIP_VOID);

	if(ClientFlags() & CLIENTFLAGS_REVERSE)
	{
		ClientFlags() &= ~CLIENTFLAGS_REVERSE;
		if(ClientFlags() & CLIENTFLAGS_RUN)
		{
			m_CreationGameTick = m_pWServer->GetGameTick() - m_CreationGameTick;
			m_CreationGameTick = m_pWServer->GetGameTick() + m_CreationGameTick;
		}
	}
	else
	{
		ClientFlags() |= CLIENTFLAGS_REVERSE;
		if(ClientFlags() & CLIENTFLAGS_RUN)
		{
			m_CreationGameTick = m_pWServer->GetGameTick() - m_CreationGameTick;
			m_CreationGameTick = m_pWServer->GetGameTick() + m_CreationGameTick;
		}
	}
	SetDirty(CWO_DIRTYMASK_COREMASK);
}

void CWObject_Attach::GotoEnd()
{
	MAUTOSTRIP(CWObject_Attach_GotoEnd, MAUTOSTRIP_VOID);
}


void CWObject_Attach::AddTimedMessage(uint _iSlot, const CTimedMessage& _Msg)
{
	MAUTOSTRIP(CWObject_Attach_AddTimedMessage, MAUTOSTRIP_VOID);

	m_lTimedMessages.SetMinLen(_iSlot + 1);
	m_lTimedMessages[_iSlot] = _Msg;
}


void CWObject_Attach::InsertTimedMessage(const CTimedMessage& _Msg)
{
	for (int i = 0; i < m_lTimedMessages.Len(); i++)
	{
		if (_Msg.m_Time < m_lTimedMessages[i].m_Time)
		{
			m_lTimedMessages.Insert(i, _Msg);
			return;
		}
	}
	m_lTimedMessages.Add(_Msg);
}


void CWObject_Attach::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
{
	MAUTOSTRIP(CWObject_Attach_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();

	// Ignore some Ipion keys that we don't want to see in the log.
	switch (_KeyHash)
	{
	case MHASH2('FRIC','TION'): // "FRICTION"
	case MHASH2('SHRI','NK'): // "SHRINK"
	case MHASH3('ELAS','TICI','TY'): // "ELASTICITY"
	case MHASH2('DENS','ITY'): // "DENSITY"
	case MHASH2('VELO','CITY'): // "VELOCITY"
	case MHASH2('STOP','PED'): // "STOPPED"
	case MHASH2('UNST','ABLE'): // "UNSTABLE"
	case MHASH3('ATTA','CH_L','INE0'): // "ATTACH_LINE0"
	case MHASH3('ATTA','CH_L','INE1'): // "ATTACH_LINE1"
	case MHASH2('HELP','AXIS'): // "HELPAXIS"
		{
	/*		Other things we could ignore if they show up in the log
			ATTACH_CIRCLE_ORIGIN
			ATTACH_CIRCLE0
			ATTACH_TARGET
			WAIT*/
			break;
		}

	case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
		{
			CFStr ModelName = CFStrF("$WORLD:%d", _pKey->GetThisValuei());
			Model_Set(0, m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName), false);
			break;
		}

	case MHASH3('DEST','ROYT','IME'): // "DESTROYTIME"
		{
			m_DestroyTime = RoundToInt(_pKey->GetThisValuef() * m_pWServer->GetGameTicksPerSecond());
			break;
		}

	case MHASH4('DEST','ROYP','HYST','IME'): // "DESTROYPHYSTIME"
		{
			m_DestroyPhysicsTime = RoundToInt(_pKey->GetThisValuef() * m_pWServer->GetGameTicksPerSecond());
			break;
		}
	
	case MHASH2('DAMA','GE'): // "DAMAGE"
		{
			m_Damage = uint16(_pKey->GetThisValuef());
			break;
		}
	
	case MHASH2('FLAG','S'): // "FLAGS"
		{
			static const char *FlagsTranslate[] =
			{
				"gamephysics", "autostart", "autoreset", "loop", "autoreverse", "pushing", "unreliable", "handdriven", "neverinterrupt", "inviswhenstopped", "blocknav", "waitspawn", "once", "usehint", "pushsync", "usehint2", "lockz", "physicsdriven", NULL
			};
		
			m_Flags = _pKey->GetThisValue().TranslateFlags(FlagsTranslate);
	//		m_Flags &= ~FLAGS_GAMEPHYSICS;
			break;
		}

	case MHASH3('LIGH','TING','MODE'): // "LIGHTINGMODE"
		{
			ClientFlags() &= ~(3 << CLIENTFLAGS_LIGHTINGSHIFT);
			ClientFlags() |= _pKey->GetThisValuei() << CLIENTFLAGS_LIGHTINGSHIFT;
			break;
		}

	case MHASH1('JUMP'): // "JUMP"
		{
			GetInitData()->m_JumpParam = _pKey->GetThisValue();
			break;
		}

	case MHASH2('RUNN','ING'): // "RUNNING"
		{
			m_Flags |= FLAGS_AUTOSTART;
			break;
		}
	
	case MHASH3('GAME','PHYS','ICS'): // "GAMEPHYSICS"
		{
			m_Flags |= FLAGS_GAMEPHYSICS;
			break;
		}

	
	default:
		{
			if (KeyName.CompareSubStr("TIMEDSOUND") == 0)
			{
				uint iSlot = atoi(KeyName.Str() + 10);
				m_lTimedSounds.SetMinLen(iSlot + 1);
				m_lTimedSounds[iSlot] = _pKey->GetThisValue();
			}

			else if(KeyName.CompareSubStr("TIMEDMESSAGE") == 0)
			{
				CTimedMessage Msg;
				Msg.Parse(_pKey->GetThisValue(), m_pWServer);

				if((Msg.m_Target == "" || Msg.m_Target == "<NULL>") && Msg.m_Time == 0 && Msg.m_Msg == 0x110)
				{
					// Have to do this for now, because Ogier had a bug which saved this type of message on all dynamics
				}
				else
				{
					uint iSlot = atoi(KeyName.Str() + 12);
					AddTimedMessage(iSlot, Msg);
				}
			}
			else
				CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Attach::Spawn(int _iSender)
{
	MAUTOSTRIP(CWObject_Attach_Spawn, MAUTOSTRIP_VOID);

	ClientFlags() &= ~(CWO_CLIENTFLAGS_INVISIBLE | CWO_CLIENTFLAGS_NOREFRESH);
	m_pWServer->Phys_InsertPosition(m_iObject, this);
	m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);

	if(m_Flags & FLAGS_GAMEPHYSICS)
	{
		int iFlags = OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
		
		if(!(m_Flags & FLAGS_BLOCKNAVIGATION))
			iFlags |= OBJECT_FLAGS_NAVIGATION;
		if(m_iModel[0] > 0)
			Model_SetPhys(m_iModel[0], false, iFlags, OBJECT_PHYSFLAGS_PUSHER | OBJECT_PHYSFLAGS_ROTATION, false);
		ClientFlags() |= CLIENTFLAGS_PHYSICS;
	}
	
	UpdateNoRefreshFlag();
}

void CWObject_Attach::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_Attach_OnFinishEvalKeys, MAUTOSTRIP_VOID);

	CWObject_Model::OnFinishEvalKeys();

	// Insert timed sounds as timed messages
	TAP<CStr> pTimedSounds = m_lTimedSounds;
	for (uint i = 0; i < pTimedSounds.Len(); i++)
	{
		CTimedMessage Msg;
		Msg.m_iSpecialTarget = CWO_SimpleMessage::SPECIAL_TARGET_THIS;
		Msg.m_Msg = OBJSYSMSG_PLAYSOUND;
		Msg.m_Param0 = 0;
		Msg.m_StrParam = pTimedSounds[i];
		Msg.m_Time = Msg.m_StrParam.GetStrMSep(";, ").Val_fp64();
		Msg.m_StrParam.Trim();

		if (Msg.m_StrParam != "")
			InsertTimedMessage(Msg);
	}
	// Remove empty messages (for instance if we don't have a "0" message)
	for (int32 i = 0; i < m_lTimedMessages.Len(); i++)
	{
		if (m_lTimedMessages[i].m_Time == -1.0f)
		{
			m_lTimedMessages.Del(i);
			i--;
		}
	}
	m_lTimedSounds.Clear();

	if(m_Flags & FLAGS_WAITSPAWN)
		ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE | CWO_CLIENTFLAGS_NOREFRESH;
	else
		Spawn(m_iObject);

	if(m_Flags & FLAGS_LOOP)
		ClientFlags() |= CLIENTFLAGS_LOOP;
	if(m_Flags & FLAGS_UNRELIABLE)
		ClientFlags() |= CLIENTFLAGS_UNRELIABLE;
	if(m_Flags & FLAGS_INVISWHENSTOPPED)
		ClientFlags() |= CLIENTFLAGS_INVISWHENSTOPPED;
	if(m_Flags & FLAGS_USEHINT)
		ClientFlags() |= CLIENTFLAGS_USEHINT;
	if(m_Flags & FLAGS_USEHINT2)
		ClientFlags() |= CLIENTFLAGS_USEHINT2;
	if (m_Flags & FLAGS_LOCKZ)
		ClientFlags() |= CLIENTFLAGS_LOCKZ;

	InitClientData();
}
 
void CWObject_Attach::OnSpawnWorld()
{
	MAUTOSTRIP(CWObject_Attach_OnSpawnWorld, MAUTOSTRIP_VOID);

	CWObject_Model::OnSpawnWorld();

	// Make sure all resources are included from the timedmessages
	for(int i = 0; i < m_lTimedMessages.Len(); i++)
		m_lTimedMessages[i].SendPrecache(m_iObject, m_pWServer);

	InitAttachPointers();

	m_DirtyMask |= CWO_DIRTYMASK_GENERAL;
}

void CWObject_Attach::OnSpawnWorld2()
{
	CWObject_Model::OnSpawnWorld2();
	m_spInitData = NULL;
}

void CWObject_Attach::OnTransform(const CMat4Dfp32 &_Mat)
{
	MAUTOSTRIP(CWObject_Attach_OnTransform, MAUTOSTRIP_VOID);

	CWObject_Model::OnTransform(_Mat);

	// This has to be done because AttachOrigin is in world-space and is specified in the map.
	// Something then has to be done when spawning from a Prefab or similar

	if(m_spInitData != NULL)
	{
		for(int i = 0; i < MAXATTACH; i++)
			if(m_spInitData->m_bAttachNode[i])
				m_spInitData->m_AttachOrigin[i] *= _Mat;
	}

	CAttachClientData *pCD = GetAttachClientData(this);
	pCD->m_TransformMat = _Mat;
	m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}

aint CWObject_Attach::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Attach_OnMessage, MAUTOSTRIP_VOID);

	switch(_Msg.m_Msg)
	{
	case OBJMSG_HOOK_GETRENDERMATRIX:
		{
			CMTime *pTime = (CMTime *)_Msg.m_Param0;
			CMat4Dfp32* pMat = (CMat4Dfp32 *)_Msg.m_Param1;
			ServerMsg_GetRenderMatrix(*pTime, *pMat);
			return 1;
		}
 	case OBJMSG_HOOK_GETCURRENTMATRIX:
 		{
 			CMTime Time = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0);
 			CMat4Dfp32* pMat = (CMat4Dfp32*)_Msg.m_Param0;
 			*pMat = GetRenderMatrix(m_pWServer, Time, -1, 0);
 			return 1;
 		}
	case OBJMSG_HOOK_GETORGMATRIX:
		{
			CMat4Dfp32 *pMat = (CMat4Dfp32 *)_Msg.m_Param0;
			*pMat = GetOrgPositionMatrix();
			return 1;
		}
	case OBJMSG_IMPULSE:
		{
			if(!((m_Flags & FLAGS_WAITSPAWN) || (m_ClientFlags & CLIENTFLAGS_DEACTIVATED)))
				OnImpulse(_Msg.m_Param0, _Msg.m_iSender);
			return 1;
		}
	case OBJSYSMSG_PARAM_SET:
		{
			m_Param = _Msg.m_Param0;
			if(m_Wait != -1)
			{
				if((m_Flags & FLAGS_WAITTYPE_MASK) == FLAGS_WAITTYPE_PARAM && m_Param == m_Wait)
				{
					// Set activator if not previously set. Perhaps we should always set activator in this case for the sake of consistency
					// but that might fuck up existing scripts, so wait until p6 to do so (if at all)
					if (m_iActivator == 0)
						m_iActivator = _Msg.m_iSender;

					m_Wait = -1;
					Run(m_iAnim2 + 1);
				}
			}
			return m_Param;
		}
	case OBJSYSMSG_PARAM_ADD:
		{
			m_Param += _Msg.m_Param0;
			if(m_Wait != -1)
			{
				if((m_Flags & FLAGS_WAITTYPE_MASK) == FLAGS_WAITTYPE_PARAM && m_Param == m_Wait)
				{
					// Set activator if not previously set. Perhaps we should always set activator in this case for the sake of consistency
					// but that might fuck up existing scripts, so wait until p6 to do so (if at all)
					if (m_iActivator == 0)
						m_iActivator = _Msg.m_iSender;

					m_Wait = -1;
					Run(m_iAnim2 + 1);
				}
			}
			return m_Param;
		}
	case OBJMSG_HOOK_GETDURATION:
		{
			int Temp = m_iAnim2;
			m_iAnim2 = _Msg.m_Param0;
			fp32 *Dur = (fp32 *)_Msg.m_Param1;
			*Dur = GetDuration();
			m_iAnim2 = Temp;
			return 1;
		}

	case OBJMSG_HOOK_WAITIMPULSE:
		{
			Stop();
			m_Wait = _Msg.m_Param0;
			m_Flags = (m_Flags & ~FLAGS_WAITTYPE_MASK) | FLAGS_WAITTYPE_IMPULSE;
			return 1;
		}

	case OBJMSG_HOOK_WAITPARAM:
		{
			// If param already is given value there's no need to wait
			if (m_Param != _Msg.m_Param0)
			{
				Stop();
				m_Wait = _Msg.m_Param0;
				m_Flags = (m_Flags & ~FLAGS_WAITTYPE_MASK) | FLAGS_WAITTYPE_PARAM;
			}
			return 1;
		}

	case OBJMSG_HOOK_SETCONTROLLER:
		{
			GetAttachClientData(this)->m_iController = (uint16)_Msg.m_Param0;
			return 1;
		}
	case OBJMSG_GAME_SPAWN:
		{
			if(_Msg.m_Param0 <= 0)
			{
				if(m_Flags & FLAGS_WAITSPAWN)
				{
					m_Flags &= ~FLAGS_WAITSPAWN;
					Spawn(_Msg.m_iSender);

					if(m_Flags & FLAGS_GAMEPHYSICS)
					{
						GetAttachClientData(this)->m_CachedTime.MakeInvalid();
						CMTime Time = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0);
						CMat4Dfp32 Mat = GetRenderMatrix(m_pWServer, Time, m_pWServer->GetGameTick(), 0);
						m_pWServer->Object_SetPosition_World(m_iObject, Mat);
					}
					return 1;
				}
			}
			else
			{
				if(!(m_Flags & FLAGS_WAITSPAWN))
				{
					Reset();
					iAnim2() = 0;
					ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE | CWO_CLIENTFLAGS_NOREFRESH;
					m_Flags |= FLAGS_WAITSPAWN;

					if(m_Flags & FLAGS_GAMEPHYSICS)
					{
						if(m_iModel[0] > 0)
							Model_SetPhys(m_iModel[0], false, 0, 0, false);
						ClientFlags() &= ~CLIENTFLAGS_PHYSICS;
					}
				}
			}
			return 0;
		}

	case OBJMSG_HOOK_GETTIME:
		{
			if(_Msg.m_DataSize == sizeof(CMTime))
			{
				*((CMTime *)_Msg.m_pData) = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0);
				return 1;
			}
			return 0;
		}
	case OBJSYSMSG_ACTIVATE:
		{
			if (_Msg.m_Param0)
			{
				//Activate
				ClientFlags() &= ~CLIENTFLAGS_DEACTIVATED;
			}
			else
			{
				//Deactivate
				ClientFlags() |= CLIENTFLAGS_DEACTIVATED;
			}
			return 1;
		}
		
#ifndef M_RTM
	case OBJSYSMSG_GETDEBUGSTRING:
		if(_Msg.m_DataSize == sizeof(CStr *))
		{
			CWObject_Model::OnMessage(_Msg);
			CStr *pSt = (CStr *)_Msg.m_pData;
			if(m_Flags & FLAGS_WAITSPAWN)
				*pSt += "Waiting for spawn";
			else
			{
				*pSt += CStrF("Time: %.2f", GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0).GetTime());
				if(m_Wait != -1)
					*pSt += CStrF("  Waiting for impulse %i", m_Wait);
			}

		}
		return 1;
#endif
	}

	return CWObject_Model::OnMessage(_Msg);
}

CMTime CWObject_Attach::GetTime(CWObject_CoreData *_pObj, CWorld_PhysState *_pWPhysState, int _GameTick, fp32 _TickFrac)
{
	MAUTOSTRIP(CWObject_Attach_GetTime, 0.0f);

	CWObject_Attach::CAttachClientData* pCData = GetAttachClientData(_pObj);

	if(pCData->m_iController != 0)
	{
		CMTime Time;
		CWObject_Message Msg(OBJMSG_HOOK_GETCONTROLLEDTIME);
		Msg.m_DataSize = sizeof(Time);
		Msg.m_pData = &Time;
		_pWPhysState->Phys_Message_SendToObject(Msg, pCData->m_iController);
		return Time;
	}

	fp32 TickTime = _pWPhysState->GetGameTickTime();
	CMTime Time;
	if(!(_pObj->m_ClientFlags & CLIENTFLAGS_RUN))
		//Stopped
		Time = CMTime::CreateFromTicks(_pObj->m_CreationGameTick, TickTime, _pObj->m_CreationGameTickFraction);
	else
	{
		if(_pObj->m_ClientFlags & CLIENTFLAGS_REVERSE)
			Time = CMTime::CreateFromTicks(_pObj->m_CreationGameTick - _GameTick, TickTime, _pObj->m_CreationGameTickFraction - _TickFrac);
		else
			Time = CMTime::CreateFromTicks(_GameTick - _pObj->m_CreationGameTick, TickTime, _TickFrac - _pObj->m_CreationGameTickFraction);
	}
	
	return Time;
}

void CWObject_Attach::UpdateTimedMessages(const CMTime& _Time)
{
	MAUTOSTRIP(CWObject_Attach_UpdateTimedMessages, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Attach::UpdateTimedMessages);

	if((m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED)))
	{
		// Standard timedmessages
		while((m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED)) && 
		      (m_lTimedMessages.Len() > m_iCurTimedMessage) && 
		      (_Time.Compare(m_lTimedMessages[m_iCurTimedMessage].m_Time - 0.0001f) > 0) && 
			  (m_Wait == -1))
		{
			CWObject_Message Msg;
			int iMsg = m_iCurTimedMessage;
			m_iCurTimedMessage++; //Sendmessage might cause an asynchrounous call to this method, so this must be updated first
			m_lTimedMessages[iMsg].CreateMessage(Msg, m_iObject, m_iActivator);
			m_lTimedMessages[iMsg].SendMessage(m_iObject, m_iActivator, m_pWServer);
/*
			else if(m_lTimedMessages[m_iCurTimedMessage].m_Target.CompareNoCase("$this") == 0 ||
					m_lTimedMessages[m_iCurTimedMessage].m_Target == "")
			{
				m_pWServer->Message_SendToObject(Msg, m_iObject);
			}
			else if(m_lTimedMessages[m_iCurTimedMessage].m_Target.CompareNoCase("$activator") == 0)
			{
				if(m_iActivator <= 0)
					ConOutL("(CWObject_Attach::UpdateTimedMessages) Activator not known");
				else
					m_pWServer->Message_SendToObject(Msg, m_iActivator);
			}
			else if(m_lTimedMessages[m_iCurTimedMessage].m_Target.CompareNoCase("$game") == 0)
				m_pWServer->Message_SendToObject(Msg, m_pWServer->Object_GetGameObject());

			else
				m_pWServer->Message_SendToTarget(Msg, m_lTimedMessages[m_iCurTimedMessage].m_Target);*/
		}
	}
}

void CWObject_Attach::InitAttachPointers()
{
	MAUTOSTRIP(CWObject_Attach_InitAttachPointers, MAUTOSTRIP_VOID);

	m_Data[0] = 0;
	m_Data[1] = 0;
	if(m_spInitData != NULL)
	{
		for(int i = 0; i < MAXATTACH; i++)
		{
			if(m_spInitData->m_bAttachNode[i])
			{
				if(m_spInitData->m_AttachNode[i] != "")
				{
					int iObj = m_pWServer->Selection_GetSingleTarget(m_spInitData->m_AttachNode[i]);
					CWObject *pObj = m_pWServer->Object_Get(iObj);
					if(pObj && pObj->m_ClientFlags & CWO_CLIENTFLAGS_ISATTACHOBJ)
						m_Data[i >> 1] |= iObj << ((i & 1) * 16);
					else
						m_Data[i >> 1] |= m_iObject << ((i & 1) * 16);
				}
				else
				{
					m_Data[i >> 1] |= m_iObject << ((i & 1) * 16);
				}
				m_Data[i + 2] = (m_spInitData->m_AttachOrigin[i] - GetPosition()).Pack32(1024);
			}
			else
				m_Data[i + 2] = CVec3Dfp32(0.0f).Pack32(1024);
		}
	}
}
	
void CWObject_Attach::InitClientData()
{
	MAUTOSTRIP(CWObject_Attach_InitClientData, MAUTOSTRIP_VOID);

	CAttachClientData *pCD = GetAttachClientData(this);
	if(pCD)
	{
		pCD->m_OrgPos = GetPositionMatrix();
		m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
	}
}

bool CWObject_Attach::NeedRefresh()
{
	MAUTOSTRIP(CWObject_Attach_NeedRefresh, false);

	if(m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED))
	{
		if(m_lTimedMessages.Len() > m_iCurTimedMessage)
			return true;

		if(m_Flags & FLAGS_PENDINGSTOP)
			return true;

		if(m_DestroyTime > 0)
			return true;

		if(m_DestroyPhysicsTime > 0)
			return true;
		
		if(m_ClientFlags & CLIENTFLAGS_PHYSICS)
			return true;
	}

	if(m_ClientFlags & CLIENTFLAGS_SEMIPHYSICS)
		return true;

	return false;
}

void CWObject_Attach::UpdateNoRefreshFlag()
{
	MAUTOSTRIP(CWObject_Attach_UpdateNoRefreshFlag, MAUTOSTRIP_VOID);

	// I'm not ready to turn this on quite yet. -JA

/*	if(NeedRefresh())
		ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
	else
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;*/
}

CMTime CWObject_Attach::GetUpdatedTime(CMTime *_Time)
{
	MAUTOSTRIP(CWObject_Attach_GetUpdatedTime, 0.0f);
	MSCOPESHORT(CWObject_Attach::GetUpdatedTime);

	CMTime Time = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0);
	if(_Time)
		Time = *_Time;
	if(m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED))
	{
		if(m_ClientFlags & CLIENTFLAGS_LOOP)
		{
			fp32 Duration = GetDuration();
			if(m_lTimedMessages.Len())
				Duration = Max(Duration, m_lTimedMessages[m_lTimedMessages.Len() - 1].m_Time);

			if(Duration != -1)
			{
				if(Time.Compare(Duration - 0.001f) >= 0)
				{
					UpdateTimedMessages(Time + CMTime::CreateFromSeconds(0.001f));
					if(m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED))
					{
						// Check if engine has been stopped during the last timedmessages before the loop
						// If it has been stopped, the time has already been resetted.

						// Reset time
						//m_AnimTime += Duration * SERVER_TICKSPERSECOND;
						fp32 fToAdd = Duration * m_pWServer->GetGameTicksPerSecond();
						int iToAdd = TruncToInt(fToAdd);
						m_CreationGameTick += iToAdd;
						m_CreationGameTickFraction += fToAdd - iToAdd;
						if(m_CreationGameTickFraction > 0.9999f)
//						if(m_CreationGameTickFraction >= 1.0f)
						{
							m_CreationGameTick++;
							m_CreationGameTickFraction -= 1.0f;
						}
						m_iCurTimedMessage = 0;

						// Set dirty so that client gets updated time
						m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);
					}
					Time = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0);
//					fp32 fTime = Time.GetTime();
				}
//				int iTime = int(Time / Duration);
//				Time -= iTime * Duration;
//				bLooped = true;
				
			}
			if(Time.Compare((m_pWServer->GetGameTickTime() - 0.0001f)) < 0)
			{
/*				if(m_iCurTimedMessage != 0)
				{
					if(m_iCurTimedMessage >= m_lTimedMessages.Len())
						m_iCurTimedMessage = 0;
					else
					{
						fp32 LastTimed = m_lTimedMessages[m_iCurTimedMessage - 1].m_Time;
						fp32 CurTimed = m_lTimedMessages[m_iCurTimedMessage].m_Time;
						m_iCurTimedMessage = 0;
					}
				}*/

				if(m_Flags & FLAGS_PENDINGSTOP)
				{
					int OldFlags = m_Flags & (~FLAGS_PENDINGSTOP);
					m_Flags &= ~FLAGS_NEVERINTERRUPT;
					Stop();
					m_Flags = OldFlags;
					
				}
			}
		}
		
		UpdateTimedMessages(Time);
	}
	return Time;
}

bool CWObject_Attach::Attach_SetPosition(const CMat4Dfp32 &_Mat, bool _bMoveTo)
{
	MAUTOSTRIP(CWObject_Attach_Attach_SetPosition, false);
	MSCOPESHORT(CWObject_Attach::Attach_SetPosition);

	bool bUseOldMask = (m_ClientFlags & CWO_CLIENTFLAGS_LINKINFINITE) != 0;
	int OldMask = 0;
	if(bUseOldMask)
	{
		OldMask = m_pWServer->Phys_GetDirtyMask();
		m_pWServer->Phys_SetDirtyMask(0);
	}

	bool bRes;
	if(_bMoveTo)
		bRes = m_pWServer->Object_MoveTo_World(m_iObject, _Mat);
	else
		bRes = m_pWServer->Object_SetPosition_World(m_iObject, _Mat);

	if(bUseOldMask)
		m_pWServer->Phys_SetDirtyMask(OldMask);

	return bRes;
}

static CMat4Dfp32 OldPushingMat;
static CMat4Dfp32 NewPushingMat;

void CWObject_Attach::OnRefresh()
{
	MAUTOSTRIP(CWObject_Attach_OnRefresh, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_Attach::OnRefresh, ENGINEPATH);

	//Don't do anything if deactivated. Perhaps we should do some stuff, though...
	if (m_ClientFlags & CLIENTFLAGS_DEACTIVATED)
	{
		MSCOPESHORT(Deactivated);
		return;
	}

	// Run this at the beginning, so if NoRefresh flag should be set because of below code, 
	// it'll still run one more tick.
	UpdateNoRefreshFlag();

	// This code should be moved to OnSpawnWorld, since it is possible for an engine to be activated
	// and then waitimpulsed from another engine, and the following code won't handle that.
	// It's too risky to do anything right now, though.
	//if(m_Flags & FLAGS_AUTOSTART && !(m_ClientFlags & CLIENTFLAGS_RUN) && m_Wait == -1)
	if(m_Flags & FLAGS_AUTOSTART)
	{
		Run(m_iAnim2 + 1);
		m_Flags &= ~FLAGS_AUTOSTART;
	}
	
	CMTime Time = GetUpdatedTime();
	if(m_ClientFlags & CLIENTFLAGS_SEMIPHYSICS)
	{
		Attach_SetPosition(GetRenderMatrix(m_pWServer, Time, m_pWServer->GetGameTick(), 0), false);
		
		CWO_PhysicsState Phys = GetPhysState();
		Phys.m_nPrim = 0;
		Phys.m_Prim[Phys.m_nPrim++].Create(OBJECT_PRIMTYPE_PHYSMODEL, m_iModel[0], 0, 0);
		Phys.m_PhysFlags = OBJECT_PHYSFLAGS_PUSHER | OBJECT_PHYSFLAGS_ROTATION;
		Phys.m_ObjectFlags = OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
		if(m_pWServer->Object_SetPhysics(m_iObject, Phys))
		{
			ClientFlags() &= ~CLIENTFLAGS_SEMIPHYSICS;
		}
	}
	else if(m_ClientFlags & CLIENTFLAGS_PHYSICS)
	{
//		ConOutL(CStrF("%i ", m_iObject) + (char *)GetPosition().GetFilteredString());
		CMat4Dfp32 Mat = GetRenderMatrix(m_pWServer, Time, m_pWServer->GetGameTick(), 0);
		CMat4Dfp32 Old = GetPositionMatrix();
		if(!Mat.AlmostEqual(Old, 0.01f))
		{
			if(m_Flags & FLAGS_PUSHING)
			{
				OldPushingMat = Old;
				NewPushingMat = Mat;
				Attach_SetPosition(Mat, true);

				if (!(m_Flags & FLAGS_PUSHSYNC) && CVec3Dfp32::GetRow(Mat, 3).DistanceSqr(GetPosition()) > 0.00001f)
				{
					CVec3Dfp32 dMove0;
					CVec3Dfp32 dMove1;
					CVec3Dfp32::GetRow(Mat, 3).Sub(CVec3Dfp32::GetRow(Old, 3), dMove0);
					GetPosition().Sub(CVec3Dfp32::GetRow(Old, 3), dMove1);
					fp32 FracMoved = dMove1.Length() / dMove0.Length();

					fp32 fToAdd = -(FracMoved - 1.0f);
					int iToAdd = TruncToInt(fToAdd);
					m_CreationGameTick += iToAdd;
					m_CreationGameTickFraction += fToAdd - iToAdd;
					iToAdd = TruncToInt(m_CreationGameTickFraction);
					m_CreationGameTickFraction -= iToAdd;
					m_CreationGameTick += iToAdd;

					m_DirtyMask |= CWO_DIRTYMASK_GENERAL;
				}
//				if(!Mat.AlmostEqual(GetPositionMatrix(), 0.01f))
//				{
//					ConOutD("(CWObject_Attach::OnRefresh) Failed to push");
//					ConOutL("old:    " + Old.GetString());
//					ConOutL("new:    " + GetPositionMatrix().GetString());
//					ConOutL("wanted: " + Mat.GetString());
//				}
			}
			else
			{
				if(!Attach_SetPosition(Mat, false))
				{
					if(m_Flags & FLAGS_AUTOREVERSE)
						Reverse();
					else
					{
						const fp32 LenSqr = (CVec3Dfp32::GetRow(Mat, 3) - CVec3Dfp32::GetRow(Old, 3)).LengthSqr();
						const fp32 RotDot0 = (CVec3Dfp32::GetRow(Mat, 0) * CVec3Dfp32::GetRow(Old, 0));
						const fp32 RotDot1 = (CVec3Dfp32::GetRow(Mat, 1) * CVec3Dfp32::GetRow(Old, 1));
						const fp32 RotDot2 = (CVec3Dfp32::GetRow(Mat, 2) * CVec3Dfp32::GetRow(Old, 2));
						if(LenSqr > 30 || M_Fabs(RotDot0) < 0.9f || M_Fabs(RotDot1) < 0.9f || M_Fabs(RotDot2) < 0.9f)
						{
							// Try to telefrag everything colliding to the physics-state
							bool bOk = false;
							if(m_Damage > 0)
							{
								TSelection<CSelection::LARGE_BUFFER> Selection;
								m_pWServer->Selection_AddIntersection(Selection, Mat, GetPhysState());
								CWO_DamageMsg Msg(m_Damage);
								Msg.SendToSelection(Selection, 0, m_pWServer);

								// Try to set the phys-state again and see if the telefrag improved the situation.
								if(Attach_SetPosition(Mat, false))
									bOk = true;
							}
							if(!bOk)
							{
								ClientFlags() |= CLIENTFLAGS_SEMIPHYSICS;
								CWO_PhysicsState Phys = GetPhysState();
								Phys.m_nPrim = 0;
								m_pWServer->Object_SetPhysics(m_iObject, Phys);
								Attach_SetPosition(Mat, false);
							}
						}
					}
				}
			}
			if(!(m_ClientFlags & CLIENTFLAGS_RUN))
			{
				CVelocityfp32 Vel;
				Vel.Unit();
				SetVelocity(Vel);
			}
		}
		else
		{
			if(m_Flags & FLAGS_PUSHING)
			{
				CVelocityfp32 Vel;
				Vel.Unit();
				SetVelocity(Vel);
			}
		}
	}
	
	m_DirtyMask |= GetAttachClientData(this)->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;

	int Tick = RoundToInt(Time.GetTime() * m_pWServer->GetGameTicksPerSecond());
	if(m_DestroyPhysicsTime > 0 && m_DestroyPhysicsTime <= Tick)
	{
		m_Flags &= ~FLAGS_GAMEPHYSICS;

		CWO_PhysicsState State;
		State.m_nPrim = 0;
		m_pWServer->Object_SetPhysics(m_iObject, State);
		ClientFlags() &= ~(CLIENTFLAGS_PHYSICS | CLIENTFLAGS_SEMIPHYSICS);

		m_DestroyPhysicsTime = -1;
		UpdateNoRefreshFlag();
	}

	if(m_DestroyTime > 0 && m_DestroyTime <= Tick)
		Destroy();
}

void CWObject_Attach::ParseAttach(CStr _Attach, int _iAttach)
{
	MAUTOSTRIP(CWObject_Attach_ParseAttach, MAUTOSTRIP_VOID);

	CInitData *pInitData = GetInitData();

	for(int i = 0; i < 3; i++)
		pInitData->m_AttachOrigin[_iAttach][i] = _Attach.GetStrSep(",").Val_fp64();
	pInitData->m_bAttachNode[_iAttach] = true;
	pInitData->m_AttachNode[_iAttach] = _Attach.GetStrSep(",");
	pInitData->m_AttachNode[_iAttach].Trim();
	pInitData->m_AttachNode[_iAttach] = m_pWServer->World_MangleTargetName(pInitData->m_AttachNode[_iAttach]);
}

CWObject_Attach::CInitData *CWObject_Attach::GetInitData()
{
	if(!m_spInitData)
	{
		m_spInitData = MNew(CInitData);
		if(m_spInitData == NULL)
			Error("GetInitData", "Out of memory.")

		for(int i = 0; i < MAXATTACH; i++)
		{
			m_spInitData->m_bAttachNode[i] = false;
			m_spInitData->m_AttachOrigin[i] = 0;
		}
	}

	return m_spInitData;
}

CMat4Dfp32 CWObject_Attach::GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac)
{
	MAUTOSTRIP(CWObject_Attach_GetRenderMatrix, CMat4Dfp32());

	CAttachClientData *pData = GetAttachClientData(this);
	if(pData && !(m_ClientFlags & CLIENTFLAGS_UNRELIABLE))
	{
		if(pData->m_CachedTime.Compare(_Time) == 0)
			return pData->m_CachedMat;
	}
	
	CMat4Dfp32 Mat;
	if(!_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, aint(&_Time), aint(&Mat)), m_iObject) != 0)
		Mat = GetPositionMatrix();

	if (m_ClientFlags & CLIENTFLAGS_LOCKZ)
		Mat.GetRow(3).k[2] = GetOrgPosition().k[2];

	if(pData)
	{
		pData->m_CachedTime = _Time;
		pData->m_CachedMat = Mat;
	}
	
	return Mat;
}

int CWObject_Attach::GetAttach(int _iSlot)
{
	MAUTOSTRIP(CWObject_Attach_GetAttach, 0);

	if(_iSlot & 1)
		return m_Data[_iSlot >> 1] >> 16;
	else
		return m_Data[_iSlot >> 1] & 0xffff;
}

CVec3Dfp32 CWObject_Attach::GetOrgAttachPos(int _iSlot)
{
	MAUTOSTRIP(CWObject_Attach_GetOrgAttachPos, CVec3Dfp32());

	CVec3Dfp32 Res;
	Res.Unpack32(m_Data[2 + _iSlot], 1024);
	return Res + GetOrgPosition();
}

CVec3Dfp32 CWObject_Attach::GetOrgAttachPosRel(int _iSlot)
{
	MAUTOSTRIP(CWObject_Attach_GetOrgAttachPosRel, CVec3Dfp32());

	CVec3Dfp32 Res;
	Res.Unpack32(m_Data[2 + _iSlot], 1024);
	return Res;
}

CMat4Dfp32 CWObject_Attach::GetOrgAttachMatrix(int _iSlot)
{
	MAUTOSTRIP(CWObject_Attach_GetOrgAttachMatrix, CMat4Dfp32());

	CMat4Dfp32 Mat = GetOrgPositionMatrix();
	GetOrgAttachPos(_iSlot).SetMatrixRow(Mat, 3);
	return Mat;
}

CVec3Dfp32 CWObject_Attach::GetAttachPos(CWorld_PhysState *_pWPhysState, int _iSlot, const CMTime& _Time, int32 _GameTick, fp32 _TickFraction)
{
	MAUTOSTRIP(CWObject_Attach_GetAttachPos, CVec3Dfp32());

	CMat4Dfp32 Mat = GetAttachMatrix(_pWPhysState, _iSlot, _Time, _GameTick, _TickFraction);
	return CVec3Dfp32::GetMatrixRow(Mat, 3);
}

CVec3Dfp32 CWObject_Attach::GetOrgPosition()
{
	MAUTOSTRIP(CWObject_Attach_GetOrgPosition, CVec3Dfp32());
	return CVec3Dfp32::GetRow(GetAttachClientData(this)->m_OrgPos, 3);
}

const CMat4Dfp32 &CWObject_Attach::GetOrgPositionMatrix()
{
	MAUTOSTRIP(CWObject_Attach_GetOrgPositionMatrix, *NULL);
	return GetAttachClientData(this)->m_OrgPos;
}

CMat4Dfp32 CWObject_Attach::GetAttachMatrix(CWorld_PhysState *_pWPhysState, int _iSlot, const CMTime& _Time, int32 _GameTick, fp32 _TickFraction, bool _bForceTime)
{
	MAUTOSTRIP(CWObject_Attach_GetAttachMatrix, CMat4Dfp32());

	int iAttach = GetAttach(_iSlot);
	if(iAttach == m_iObject)
	{
		return GetOrgAttachMatrix(_iSlot);
	}
	else
	{
		CWObject_CoreData *pObj = _pWPhysState->Object_GetCD(iAttach);
		if(pObj && pObj->m_ClientFlags & CWO_CLIENTFLAGS_ISATTACHOBJ)
		{
			CMat4Dfp32 Mat;
			if(_bForceTime)
				Mat = ((CWObject_Attach *)pObj)->GetRenderMatrix(_pWPhysState, _Time, _GameTick, _TickFraction);
			else
				Mat = ((CWObject_Attach *)pObj)->GetRenderMatrix(_pWPhysState, GetTime(pObj, _pWPhysState, _GameTick, _TickFraction), _GameTick, _TickFraction);
			CMat4Dfp32 OrgAttach = GetOrgAttachMatrix(_iSlot);
			CMat4Dfp32 ParentOrg = ((CWObject_Attach *)pObj)->GetOrgPositionMatrix();
			CMat4Dfp32 IParentOrg, dParent, Res;
			ParentOrg.InverseOrthogonal(IParentOrg);
			IParentOrg.Multiply(Mat, dParent);
			OrgAttach.Multiply(dParent, Res);
			return Res;
		}
		else
		{
			CMat4Dfp32 Mat;
			Mat.Unit();
			GetOrgAttachPos(_iSlot).SetMatrixRow(Mat, 3);
			return Mat;
		}
	}
}

fp32 CWObject_Attach::GetFloat(int _iSlot)
{
	MAUTOSTRIP(CWObject_Attach_GetFloat, 0.0f);

	return *(fp32 *)&m_Data[_iSlot];
}

void CWObject_Attach::SetFloat(int _iSlot, fp32 _Value)
{
	MAUTOSTRIP(CWObject_Attach_SetFloat, MAUTOSTRIP_VOID);

	m_Data[_iSlot] = *(int *)(&_Value);
}

int CWObject_Attach::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWObject_Attach_OnPhysicsEvent, 0);

	if(_Event == CWO_PHYSEVENT_PREPUSHED)
	{
/*		fp32 LenSqr = (CVec3Dfp32::GetRow(NewPushingMat, 3) - CVec3Dfp32::GetRow(OldPushingMat, 3)).LengthSqr();
		fp32 RotDot0 = (CVec3Dfp32::GetRow(NewPushingMat, 0) * CVec3Dfp32::GetRow(OldPushingMat, 0));
		fp32 RotDot1 = (CVec3Dfp32::GetRow(NewPushingMat, 1) * CVec3Dfp32::GetRow(OldPushingMat, 1));
		fp32 RotDot2 = (CVec3Dfp32::GetRow(NewPushingMat, 2) * CVec3Dfp32::GetRow(OldPushingMat, 2));
		if(LenSqr > 50 || M_Fabs(RotDot0) < 0.9f || M_Fabs(RotDot1) < 0.9f || M_Fabs(RotDot2) < 0.9f)
		{	
			CWObject_Attach *pAttach = (CWObject_Attach *)_pObj;
			if(pAttach && pAttach->m_ClientFlags &  CWO_CLIENTFLAGS_ISATTACHOBJ && pAttach->m_Damage > 0)
			{
				CWO_DamageMsg Msg(pAttach->m_Damage, 0);
				Msg.Send(_pObjOther->m_iObject, pAttach->m_iObject, pAttach->m_pWServer);
			}
		}*/
	}
	else if(_Event == CWO_PHYSEVENT_BLOCKING)
	{
		CWObject_Attach *pAttach = (CWObject_Attach *)_pObj;
		if(pAttach && pAttach->m_ClientFlags &  CWO_CLIENTFLAGS_ISATTACHOBJ && pAttach->m_Damage > 0)
		{
			CWO_DamageMsg Msg(pAttach->m_Damage, 0);
			Msg.Send(_pObjOther->m_iObject, pAttach->m_iObject, pAttach->m_pWServer);
			return SERVER_PHYS_HANDLED;
		}
		return SERVER_PHYS_ABORT;
	}

	return CWObject_Model::OnPhysicsEvent(_pObj, _pObjOther, _pPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);
}

int CWObject_Attach::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const
{
	MAUTOSTRIP(CWObject_Attach_OnCreateClientUpdate, 0);

	const CAttachClientData *pCD = GetAttachClientData(this);
	if(!pCD)
		Error_static("CWObject_Attach::OnCreateClientUpdate", "Unable to pack client update.");

	int Flags = 0;
	if(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
		Flags = CWO_CLIENTUPDATE_AUTOVAR;
	uint8* pD = _pData;
	pD += CWObject_Model::OnCreateClientUpdate(_iClient, _pClObjInfo, _pObj, _pData, Flags);
	if (pD - _pData == 0)
		return pD - _pData;

	pCD->AutoVar_Pack(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT, pD, m_pWServer->GetMapData());

	return pD - _pData;
}

int CWObject_Attach::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	MAUTOSTRIP(CWObject_Attach_OnClientUpdate, 0);
	MSCOPESHORT(CWObject_Attach::OnClientUpdate);

	const uint8* pD = &_pData[CWObject_Model::OnClientUpdate(_pObj, _pWClient, _pData, _Flags)];
	if (_pObj->m_iClass == 0 || pD - _pData == 0) return pD - _pData;

	if(_pObj->m_bAutoVarDirty)
	{
		GetAttachClientData(_pObj)->AutoVar_Unpack(pD, _pWClient->GetMapData());
		GetAttachClientData(_pObj)->m_CachedTime.MakeInvalid();
	}

	if(!(_pObj->m_ClientFlags & CLIENTFLAGS_PHYSICS) && !(_pObj->m_ClientFlags & CLIENTFLAGS_PHYSICS_DRIVEN) && (_Flags & CWO_ONCLIENTUPDATEFLAGS_DOLINK) && safe_cast<CWorld_ClientCore>(_pWClient)->GetClientState() == WCLIENT_STATE_INGAME)
	{
		CMTime Time = GetTime(_pObj, _pWClient, _pWClient->GetGameTick(), 0);
		CMat4Dfp32 Mat = ((CWObject_Attach *)_pObj)->GetRenderMatrix(_pWClient, Time, _pWClient->GetGameTick(), 0);
		_pWClient->Object_ForcePosition_World(_pObj->m_iObject, Mat);
		_Flags &= ~CWO_ONCLIENTUPDATEFLAGS_DOLINK;
	}

	return (uint8*)pD - _pData;
}

void CWObject_Attach::OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	MAUTOSTRIP(CWObject_Attach_OnClientLoad, MAUTOSTRIP_VOID);

	CWObject_Model::OnClientLoad(_pObj, _pWorld, _pFile, _pWData, _Flags);
	GetAttachClientData(_pObj)->AutoVar_Read(_pFile, _pWData);
}

void CWObject_Attach::OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	MAUTOSTRIP(CWObject_Attach_OnClientSave, MAUTOSTRIP_VOID);

	CWObject_Model::OnClientSave(_pObj, _pWorld, _pFile, _pWData, _Flags);
	GetAttachClientData(_pObj)->AutoVar_Write(_pFile, _pWData);
}

void CWObject_Attach::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Attach_OnClientRefresh, MAUTOSTRIP_VOID);
	MSCOPESHORT( CWObject_Attach::OnClientRefresh );
	if (_pObj->m_ClientFlags & CLIENTFLAGS_DEACTIVATED)
		return;

	CWObject_Model::OnClientRefresh(_pObj, _pWClient);

//	if(!(_pObj->m_ClientFlags & CLIENTFLAGS_PHYSICS))
	{
		if(_pObj->m_ClientFlags & CLIENTFLAGS_RUN || _pObj->m_ClientFlags & CLIENTFLAGS_UNRELIABLE)
		{
			// Fix for the culling problems where the object is moved out of it's server bounding-box
			// This code should actually be run only if the object _do_ have physics, since it would
			// reduced the amount of network bandwidth generated by moving around objects. That stopped
			// working when OnClientRefresh was removed from the client-mirrors.
			CMTime Time = GetTime(_pObj, _pWClient, _pWClient->GetGameTick(), 0);
			CMat4Dfp32 Mat = ((CWObject_Attach *)_pObj)->GetRenderMatrix(_pWClient, Time, _pWClient->GetGameTick(), 0);
			if(!Mat.AlmostEqual(_pObj->GetPositionMatrix(), 0.01f))
				_pWClient->Object_ForcePosition_World(_pObj->m_iObject, Mat);
		}
	}
}

void CWObject_Attach::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Attach_OnClientRender, MAUTOSTRIP_VOID);

	if((_pObj->m_ClientFlags & CLIENTFLAGS_INVISWHENSTOPPED) && !(_pObj->m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED)))
		return;
	if (_pObj->m_ClientFlags & CLIENTFLAGS_DEACTIVATED)
		return;

	bool bMat = false;
	CMat4Dfp32 Mat;
	
	CMTime Time;
	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
		if(pModel)
		{
			if(!bMat)
			{
				Time = GetTime(_pObj, _pWClient, _pWClient->GetGameTick()/* - 1*/, _pWClient->GetRenderTickFrac());
				Mat = ((CWObject_Attach *)_pObj)->GetRenderMatrix(_pWClient, Time, _pWClient->GetGameTick(), _pWClient->GetRenderTickFrac());
				bMat = true;
			}
			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient, i);
			AnimState.m_AnimTime0 = Time;
			AnimState.m_AnimTime1 = Time;
			switch((_pObj->m_ClientFlags >> CLIENTFLAGS_LIGHTINGSHIFT) & 3)
			{
			case 0: AnimState.m_AnimAttr0 = Time.GetTime() /*CMTIMEFIX*/; break;
			case 1: AnimState.m_AnimAttr0 = 0; break;
			case 2: AnimState.m_AnimAttr0 = 1; break;
			}
			AnimState.m_Data[3] = ~_pObj->m_Data[3];

			if (_pObj->m_ClientFlags & CLIENTFLAGS_USEHINT2) // && Time.Compare(0) == 0)
			{
				CXW_Surface* pSurf = _pEngine->m_pSC->GetSurface("creepdark01");
				if (pSurf)
					AnimState.m_lpSurfaces[0] = pSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
			}
			else if (_pObj->m_ClientFlags & CLIENTFLAGS_USEHINT) // && Time.Compare(0) == 0)
			{
				CXW_Surface* pSurf = _pEngine->m_pSC->GetSurface("pickupglow");
				if (pSurf)
					AnimState.m_lpSurfaces[0] = pSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
			}

			_pEngine->Render_AddModel(pModel, Mat, AnimState, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_SURF0_ADD);
		}
	}
}

void CWObject_Attach::OnDeltaSave(CCFile *_pFile)
{
	CWObject_Model::OnDeltaSave(_pFile);

	uint8 bWaitSpawn = (m_Flags & FLAGS_WAITSPAWN) ? 1 : 0;
	_pFile->WriteLE(bWaitSpawn);

	_pFile->WriteLE(m_Flags);
	_pFile->WriteLE(m_ClientFlags);
	int32 CreationTick = m_CreationGameTick;
	if(m_ClientFlags & CLIENTFLAGS_RUN)
		CreationTick = m_pWServer->GetGameTick() - m_CreationGameTick;
	_pFile->WriteLE(CreationTick);
	_pFile->WriteLE(m_CreationGameTickFraction);
	_pFile->WriteLE(m_iCurTimedMessage);
	_pFile->WriteLE(m_Wait);
	_pFile->WriteLE(m_iActivator);
	_pFile->WriteLE(m_iAnim2);
}

void CWObject_Attach::OnDeltaLoad(CCFile *_pFile, int _Flags)
{
	CWObject_Model::OnDeltaLoad(_pFile, _Flags);

	uint8 bWaitSpawn;
	_pFile->ReadLE(bWaitSpawn);
	if(!bWaitSpawn && (m_Flags & FLAGS_WAITSPAWN))
		Spawn(0);

	_pFile->ReadLE(m_Flags);
	_pFile->ReadLE(m_ClientFlags);
	
	_pFile->ReadLE(m_CreationGameTick);
	// Allow object to have been "paused" during world absence
	if(m_ClientFlags & CLIENTFLAGS_RUN)
		m_CreationGameTick = m_pWServer->GetGameTick() - m_CreationGameTick;
	
	_pFile->ReadLE(m_CreationGameTickFraction);
	_pFile->ReadLE(m_iCurTimedMessage);
	_pFile->ReadLE(m_Wait);
	_pFile->ReadLE(m_iActivator);
	_pFile->ReadLE(m_iAnim2);

	if(m_ClientFlags & CLIENTFLAGS_PHYSICS)
	{
		CMTime Time = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0);
		CMat4Dfp32 Mat = GetRenderMatrix(m_pWServer, Time, m_pWServer->GetGameTick(), 0);
		m_pWServer->Object_SetPosition_World(m_iObject, Mat);
	}

	m_DirtyMask |= GetAttachClientData(this)->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Attach, CWObject_Model, 0x0100);

/////////////////////////////////////////////////////////////////////////////
// CWObject_Hook
void CWObject_Hook::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
{
	MAUTOSTRIP(CWObject_Hook_OnEvalKey, MAUTOSTRIP_VOID);

	switch (_KeyHash)
	{
	case MHASH2('ATTA','CH'): // "ATTACH"
		{
			ParseAttach(_pKey->GetThisValue(), 0);
			break;
		}
	
	default:
		{
			CWObject_Attach::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

CMat4Dfp32 CWObject_Hook::GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac)
{
	MAUTOSTRIP(CWObject_Hook_GetRenderMatrix, CMat4Dfp32());

	if(!GetAttach(0))
	{
		return GetPositionMatrix();
//		return CWObject_Attach::GetRenderMatrix(_pWPhysState, _Time, m_pWServer->GetGameTick());
	}
	else
		return GetAttachMatrix(_pWPhysState, 0, _Time, _GameTick, _TickFrac);
}

CWObject_Hook::CAttachClientData_Hook *CWObject_Hook::GetAttachClientData_Hook(CWObject_CoreData *_pObj)
{
	return (CAttachClientData_Hook *)GetAttachClientData(_pObj);
}


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Hook, CWObject_Attach, 0x0100);

/////////////////////////////////////////////////////////////////////////////
// CWObject_Engine_Rotate
void CWObject_Engine_Rotate::OnCreate()
{
	MAUTOSTRIP(CWObject_Engine_Rotate_OnCreate, MAUTOSTRIP_VOID);

	CWObject_Hook::OnCreate();
	
	SetFloat(7, 0.25f);
	SetFloat(6, 1.0f);
	SetFloat(5, 0);
	SetFloat(4, 0);
	
	m_ClientFlags |= CLIENTFLAGS_LOOP;
}

void CWObject_Engine_Rotate::OnTransform(const CMat4Dfp32 &_Mat)
{
	MAUTOSTRIP(CWObject_Engine_Rotate_OnTransform, MAUTOSTRIP_VOID);

	CWObject_Hook::OnTransform(_Mat);

	CVec3Dfp32 Axis = GetAttachClientData_Hook(this)->m_HelpAxis;
	Axis.MultiplyMatrix3x3(_Mat);
	GetAttachClientData_Hook(this)->m_HelpAxis = Axis;
}

void CWObject_Engine_Rotate::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
{
	MAUTOSTRIP(CWObject_Engine_Rotate_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();

	switch (_KeyHash)
	{
	case MHASH1('AXIS'): // "AXIS"
		{
			CVec3Dfp32 Axis;
			Axis.ParseString(_pKey->GetThisValue());
			Axis.Normalize();
			GetAttachClientData_Hook(this)->m_HelpAxis = Axis;
			break;
		}
	
	case MHASH2('INTE','RVAL'): // "INTERVAL"
		{
			CStr st = _pKey->GetThisValue();
			SetFloat(6, st.GetStrSep(",").Val_fp64());
			SetFloat(5, st.GetStrSep(",").Val_fp64());
			ClientFlags() &= ~CLIENTFLAGS_LOOP;
			break;
		}
	
	case MHASH2('SPEE','D'): // "SPEED"
		{
			SetFloat(7, _pKey->GetThisValuef());
			break;
		}
	
	case MHASH3('ACCE','LERA','TION'): // "ACCELERATION"
		{
			SetFloat(4, _pKey->GetThisValuef());
			break;
		}
	
	default:
		{
			CWObject_Hook::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

fp32 CWObject_Engine_Rotate::GetDuration()
{
	MAUTOSTRIP(CWObject_Engine_Rotate_GetDuration, 0.0f);

	if(GetFloat(4) == 0 && GetFloat(7) != 0)
		return 1.0f / GetFloat(7);

	return -1;
}

CMat4Dfp32 CWObject_Engine_Rotate::GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac)
{
	MAUTOSTRIP(CWObject_Engine_Rotate_GetRenderMatrix, CMat4Dfp32());

	fp32 TimeRunning = GetFloat(6);
	fp32 TimeStopped = GetFloat(5);
	fp32 TotIntervalTime = TimeRunning + TimeStopped;
	int nInterval = _Time.GetNumModulus(TotIntervalTime);
	fp32 CurInterval = _Time.GetTimeModulus(TotIntervalTime);
	
	CMTime Time;

	if(CurInterval > TimeRunning)
		Time = CMTime::CreateFromTicks(nInterval + 1, TimeRunning);
	else
		Time = CMTime::CreateFromTicks(nInterval, TimeRunning) + CMTime::CreateFromSeconds(CurInterval);
	
	CVec3Dfp32 Axis = GetAttachClientData_Hook(this)->m_HelpAxis;
	if(Axis == CVec3Dfp32(0))
		Axis = CVec3Dfp32(0, 0, 1);
	fp32 Speed = GetFloat(7);
	fp32 Accel = GetFloat(4);
	CAxisRotfp32 AxisRot(Axis, Time.GetTimeModulusScaled(Speed, 1.0f) + Time.GetTimeSqrModulusScaled(Accel, 1.0f));
	CMat4Dfp32 RotMat, Rot, Res;
	AxisRot.CreateMatrix(RotMat);
	
	if(GetAttach(0))
	{
		CMat4Dfp32 CurAttach = GetAttachMatrix(_pWPhysState, 0, _Time, _GameTick, _TickFrac);
		CMat4Dfp32 OrgAttach = GetOrgPositionMatrix();
		CMat4Dfp32 OrgRot;
		OrgAttach.Multiply(RotMat, OrgRot);
		CVec3Dfp32::GetRow(OrgAttach, 3).SetRow(OrgRot, 3);
		CMat4Dfp32 IOrgAttach, dMat;
		OrgAttach.InverseOrthogonal(IOrgAttach);
		IOrgAttach.Multiply(CurAttach, dMat);
		OrgRot.Multiply(dMat, Res);
	}
	else
	{
//		CMat4Dfp32 Org = GetOrgPositionMatrix();
//		Org.Multiply3x3(RotMat, Res);
//		CVec3Dfp32::GetRow(Org, 3).SetRow(Res, 3);
		RotMat.Multiply(GetOrgPositionMatrix(), Res);
//		ConOut(CStrF("CWObject_Engine_Path %f %s", _Time, CVec3Dfp32::GetRow(Res, 0).GetFilteredString().Str()));
	}
	return Res;
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Engine_Rotate, CWObject_Hook, 0x0100);

/////////////////////////////////////////////////////////////////////////////
// CWObject_Engine_Wheel
#ifndef M_DISABLE_CURRENTPROJECT
void CWObject_Engine_Wheel::OnCreate()
{
	MAUTOSTRIP(CWObject_Engine_Wheel_OnCreate, MAUTOSTRIP_VOID);

	CWObject_Hook::OnCreate();
}

void CWObject_Engine_Wheel::OnTransform(const CMat4Dfp32 &_Mat)
{
	MAUTOSTRIP(CWObject_Engine_Wheel_OnTransform, MAUTOSTRIP_VOID);

	CWObject_Hook::OnTransform(_Mat);

	CVec3Dfp32 Axis = GetAttachClientData_Hook(this)->m_HelpAxis;
	Axis.MultiplyMatrix3x3(_Mat);
	GetAttachClientData_Hook(this)->m_HelpAxis = Axis;
}

void CWObject_Engine_Wheel::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
{
	MAUTOSTRIP(CWObject_Engine_Wheel_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH1('AXIS'): // "AXIS"
		{
			CVec3Dfp32 Axis;
			Axis.ParseString(KeyValue);
			Axis.Normalize();
			GetAttachClientData_Hook(this)->m_HelpAxis = Axis;
			break;
		}
	
	case MHASH4('ATTA','CH_R','ADIU','S'): // "ATTACH_RADIUS"
		{
			ParseAttach(KeyValue, 1);
			break;
		}
	
	default:
		{
			CWObject_Hook::OnEvalKey(_pKey);
			break;
		}
	}
}

CMat4Dfp32 CWObject_Engine_Wheel::GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac)
{
	MAUTOSTRIP(CWObject_Engine_Wheel_GetRenderMatrix, CMat4Dfp32());

/*	fp32 TimeRunning = GetFloat(6);
	fp32 TimeStopped = GetFloat(5);
	fp32 TotIntervalTime = TimeRunning + TimeStopped;

	int nInterval = _Time.GetNumModulus(TotIntervalTime);
	fp32 CurInterval = _Time.GetTimeModulus(TotIntervalTime);
	
	CMTime Time;

	if(CurInterval > TimeRunning)
		Time = CMTime::CreateFromTicks(nInterval + 1, TimeRunning);
	else
		Time = CMTime::CreateFromTicks(nInterval, TimeRunning) + CMTime::CreateFromSeconds(CurInterval);

*/
	CVec3Dfp32 Axis = GetAttachClientData_Hook(this)->m_HelpAxis;
	if(Axis == 0)
		Axis = CVec3Dfp32(0, 0, 1);
//	fp32 Speed = GetFloat(7);
//	fp32 Accel = GetFloat(4);

	fp32 Radius = (GetOrgAttachPos(0) - GetOrgAttachPos(1)).Length();
	CVec3Dfp32 Dir = GetOrgAttachPos(0) - GetAttachPos(_pWPhysState, 0, _Time, _GameTick, _TickFrac);
	fp32 Length = Dir.Length();
	if(Radius == 0)
		Radius = 64;
	fp32 Angle = Length / (Radius * _PI2);
	if(Dir * Axis < 0)
		Angle = -Angle;

	CAxisRotfp32 AxisRot(Axis, Angle);
	CMat4Dfp32 RotMat, Rot, Res;
	AxisRot.CreateMatrix(RotMat);
	
	if(GetAttach(0))
	{
		CMat4Dfp32 CurAttach = GetAttachMatrix(_pWPhysState, 0, _Time, _GameTick, _TickFrac);
		CMat4Dfp32 OrgAttach = GetOrgPositionMatrix();
		CMat4Dfp32 OrgRot;
		OrgAttach.Multiply(RotMat, OrgRot);
		CVec3Dfp32::GetRow(OrgAttach, 3).SetRow(OrgRot, 3);
		CMat4Dfp32 IOrgAttach, dMat;
		OrgAttach.InverseOrthogonal(IOrgAttach);
		IOrgAttach.Multiply(CurAttach, dMat);
		OrgRot.Multiply(dMat, Res);
	}
	else
	{
//		CMat4Dfp32 Org = GetOrgPositionMatrix();
//		Org.Multiply3x3(RotMat, Res);
//		CVec3Dfp32::GetRow(Org, 3).SetRow(Res, 3);
		RotMat.Multiply(GetOrgPositionMatrix(), Res);
//		ConOut(CStrF("CWObject_Engine_Path %f %s", _Time, CVec3Dfp32::GetRow(Res, 0).GetFilteredString().Str()));
	}
	return Res;
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Engine_Wheel, CWObject_Hook, 0x0100);
#endif

#ifndef M_DISABLE_CURRENTPROJECT
/////////////////////////////////////////////////////////////////////////////
// CWObject_Hook_NoRotate
CMat4Dfp32 CWObject_Hook_NoRotate::GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac)
{
	MAUTOSTRIP(CWObject_Hook_NoRotate_GetRenderMatrix, CMat4Dfp32());

	CMat4Dfp32 Mat = CWObject_Hook::GetRenderMatrix(_pWPhysState, _Time, _GameTick, _TickFrac);
	CMat4Dfp32 Org = GetOrgPositionMatrix();
	CVec3Dfp32::GetRow(Mat, 3).SetRow(Org, 3);
	return Org;
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Hook_NoRotate, CWObject_Hook, 0x0100);
#endif

/////////////////////////////////////////////////////////////////////////////
// CWObject_Hook_To_Target
void CWObject_Hook_To_Target::OnCreate()
{
	MAUTOSTRIP(CWObject_Hook_To_Target_OnCreate, MAUTOSTRIP_VOID);

	CWObject_Hook::OnCreate();
}

void CWObject_Hook_To_Target::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
{
	MAUTOSTRIP(CWObject_Hook_To_Target_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH4('ATTA','CH_T','ARGE','T'): // "ATTACH_TARGET"
		{
			ParseAttach(KeyValue, 1);
			break;
		}

	case MHASH2('HELP','AXIS'): // "HELPAXIS"
		{
			CVec3Dfp32 Axis;
			Axis.ParseString(KeyValue);
			Axis.Normalize();
			GetAttachClientData_Hook(this)->m_HelpAxis = Axis;
			m_ClientFlags |= CLIENTFLAGS_HELPAXIS;
			break;
		}
	
	default:
		{
			CWObject_Hook::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Hook_To_Target::OnTransform(const CMat4Dfp32 &_Mat)
{
	MAUTOSTRIP(CWObject_Hook_To_Target_OnTransform, MAUTOSTRIP_VOID);

	CWObject_Hook::OnTransform(_Mat);

	CVec3Dfp32 Axis = GetAttachClientData_Hook(this)->m_HelpAxis;
	Axis.MultiplyMatrix3x3(_Mat);
	GetAttachClientData_Hook(this)->m_HelpAxis = Axis;
}

CMat4Dfp32 CWObject_Hook_To_Target::GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac)
{
	MAUTOSTRIP(CWObject_Hook_To_Target_GetRenderMatrix, CMat4Dfp32());

	if(!GetAttach(1))
		return CWObject_Hook::GetRenderMatrix(_pWPhysState, _Time, _GameTick, _TickFrac);
	else
	{
		CMat4Dfp32 Pos = CWObject_Hook::GetRenderMatrix(_pWPhysState, _Time, _GameTick, _TickFrac);
		CVec3Dfp32 &O = CVec3Dfp32::GetMatrixRow(Pos, 3);
		
		CVec3Dfp32 D0 = GetOrgAttachPos(1);
		CVec3Dfp32 D1 = GetAttachPos(_pWPhysState, 1, _Time, _GameTick, _TickFrac);

/*		{
			_pWPhysState->Debug_RenderMatrix(Pos, 0.05f, false);
			_pWPhysState->Debug_RenderMatrix(GetAttachMatrix(_pWPhysState, 1, _Time, _GameTick), 0.05f, false);
			_pWPhysState->Debug_RenderWire(O, D1, 0xffffffff, 0.05f, false);
		}*/
		
		CMat4Dfp32 M0;
		M0.Unit();
		
		CVec3Dfp32 Axis;
		if(!(m_ClientFlags & CLIENTFLAGS_HELPAXIS))
		{
			CPlane3Dfp32 Plane;
			if((D0 - D1).LengthSqr() > 0.01f)
				Plane.Create(O, D0, D1);
			else
				Plane.Create(O, GetOrgPosition(), D1);
			Axis = Plane.n;
		}
		else
		{
			Axis = GetAttachClientData_Hook(this)->m_HelpAxis;
			if(Axis == CVec3Dfp32(0))
				Axis = CVec3Dfp32(0, 0, 1);
		}
		
		Axis.SetMatrixRow(M0, 2);
		CMat4Dfp32 M1 = M0;
		(D0 - GetOrgPosition()).SetMatrixRow(M0, 0);
		(D1 - O).SetMatrixRow(M1, 0);
		M0.RecreateMatrix(0, 2);
		M1.RecreateMatrix(0, 2);

		CMat4Dfp32 InvM0, MRes, MRes2;
		M0.InverseOrthogonal(InvM0);
		InvM0.Multiply(M1, MRes);

		MRes.Multiply(GetOrgPositionMatrix(), MRes2);
		CVec3Dfp32::GetMatrixRow(Pos, 3).SetMatrixRow(MRes2, 3);
		return MRes2;
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Hook_To_Target, CWObject_Attach, 0x0100);

#ifndef M_DISABLE_CURRENTPROJECT
/////////////////////////////////////////////////////////////////////////////
// CWObject_Hook_To_Line
void CWObject_Hook_To_Line::OnCreate()
{
	MAUTOSTRIP(CWObject_Hook_To_Line_OnCreate, MAUTOSTRIP_VOID);

	CWObject_Hook::OnCreate();
}

void CWObject_Hook_To_Line::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
{
	MAUTOSTRIP(CWObject_Hook_To_Line_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH3('ATTA','CH_L','INE0'): // "ATTACH_LINE0"
		{
			ParseAttach(KeyValue, 1);
			break;
		}
	
	case MHASH3('ATTA','CH_L','INE1'): // "ATTACH_LINE1"
		{
			ParseAttach(KeyValue, 2);
			break;
		}
	
	switch (_KeyHash)
	{
	case MHASH2('HELP','AXIS'): // "HELPAXIS"
		{
			CVec3Dfp32 Axis;
			Axis.ParseString(KeyValue);
			Axis.Normalize();
			GetAttachClientData_Hook(this)->m_HelpAxis = Axis;
			m_ClientFlags |= CLIENTFLAGS_HELPAXIS;
			break;
		}
	
	default:
		{
			CWObject_Hook::OnEvalKey(_pKey);
			break;
		}
	}
}

void CWObject_Hook_To_Line::OnTransform(const CMat4Dfp32 &_Mat)
{
	MAUTOSTRIP(CWObject_Hook_To_Line_OnTransform, MAUTOSTRIP_VOID);

	CWObject_Hook::OnTransform(_Mat);

	CVec3Dfp32 Axis = GetAttachClientData_Hook(this)->m_HelpAxis;
	Axis.MultiplyMatrix3x3(_Mat);
	GetAttachClientData_Hook(this)->m_HelpAxis = Axis;
}

CMat4Dfp32 CWObject_Hook_To_Line::GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac)
{
	MAUTOSTRIP(CWObject_Hook_To_Line_GetRenderMatrix, CMat4Dfp32());

	if(!GetAttach(1) || !GetAttach(2))
		return CWObject_Hook::GetRenderMatrix(_pWPhysState, _Time, _GameTick, _TickFrac);
	else
	{
		CMat4Dfp32 Pos = CWObject_Hook::GetRenderMatrix(_pWPhysState, _Time, _GameTick, _TickFrac);
		CVec3Dfp32 &O = CVec3Dfp32::GetMatrixRow(Pos, 3);
		
		CVec3Dfp32 A = GetAttachPos(_pWPhysState, 1, _Time, _GameTick, _TickFrac);
		CVec3Dfp32 B = GetAttachPos(_pWPhysState, 2, _Time, _GameTick, _TickFrac);

		CVec3Dfp32 AB = (B - A).Normalize();
		CVec3Dfp32 OA = A - O;
		
		CVec3Dfp32 T = A - AB * (AB * OA);
		
		float d2;
		{
			float dSqr = (O - T).LengthSqr();
			float lSqr = (GetOrgAttachPos(1) - GetOrgPosition()).LengthSqr();
			if(lSqr < dSqr)
				d2 = 0.0001f;
			else
				d2 = M_Sqrt(lSqr - dSqr);
		}
		
		CVec3Dfp32 D0 = (T + AB * d2);
		CVec3Dfp32 D1 = (T - AB * d2);
		
		CVec3Dfp32 D;
		// Choose solution closest to Attach 2
		if((D0 - B).LengthSqr() < (D1 - B).LengthSqr())
			D = D0;
		else
			D = D1;
		
		{
			CMat4Dfp32 Mat;
			Mat.Unit();
			D.SetRow(Mat, 3);
			_pWPhysState->Debug_RenderMatrix(Mat, 0.05f, false);
		}
		
		CMat4Dfp32 M0;
		M0.Unit();
		
		CVec3Dfp32 Axis;
		if(!(m_ClientFlags & CLIENTFLAGS_HELPAXIS))
		{
			CPlane3Dfp32 Plane;
			if((D0 - D1).LengthSqr() > 0.01f)
				Plane.Create(O, D0, D1);
			else
				Plane.Create(O, GetOrgPosition(), D1);
			Axis = Plane.n;
		}
		else
		{
			Axis = GetAttachClientData_Hook(this)->m_HelpAxis;
			if(Axis == 0)
				Axis = CVec3Dfp32(0, 0, 1);
		}
		
		Axis.SetMatrixRow(M0, 2);
		CMat4Dfp32 M1 = M0;
		(GetOrgAttachPos(1) - GetOrgPosition()).SetMatrixRow(M0, 0);
		(D - O).SetMatrixRow(M1, 0);
		M0.RecreateMatrix(0, 2);
		M1.RecreateMatrix(0, 2);
		
		CMat4Dfp32 InvM0, MRes, MRes2;
		M0.InverseOrthogonal(InvM0);
		InvM0.Multiply(M1, MRes);

		MRes.Multiply(GetOrgPositionMatrix(), MRes2);
		CVec3Dfp32::GetMatrixRow(Pos, 3).SetMatrixRow(MRes2, 3);

		_pWPhysState->Debug_RenderMatrix(MRes2, 0.05f, false);
		
		return MRes2;
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Hook_To_Line, CWObject_Attach, 0x0100);

/////////////////////////////////////////////////////////////////////////////
// CWObject_Hook_To_Circle
void CWObject_Hook_To_Circle::OnCreate()
{
	MAUTOSTRIP(CWObject_Hook_To_Circle_OnCreate, MAUTOSTRIP_VOID);

	CWObject_Hook::OnCreate();
}

void CWObject_Hook_To_Circle::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
{
	MAUTOSTRIP(CWObject_Hook_To_Circle_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH5('ATTA','CH_C','IRCL','E_OR','IGIN'): // "ATTACH_CIRCLE_ORIGIN"
		{
			ParseAttach(KeyValue, 1);
			break;
		}
	
	case MHASH4('ATTA','CH_C','IRCL','E0'): // "ATTACH_CIRCLE0"
		{
			ParseAttach(KeyValue, 2);
			break;
		}
	
	case MHASH2('HELP','AXIS'): // "HELPAXIS"
		{
			CVec3Dfp32 Axis;
			Axis.ParseString(KeyValue);
			Axis.Normalize();
			GetAttachClientData_Hook(this)->m_HelpAxis = Axis;
			m_ClientFlags |= CLIENTFLAGS_HELPAXIS;
			break;
		}
	
	default:
		{
			CWObject_Hook::OnEvalKey(_pKey);
			break;
		}
	}
}

void CWObject_Hook_To_Circle::OnTransform(const CMat4Dfp32 &_Mat)
{
	MAUTOSTRIP(CWObject_Hook_To_Circle_OnTransform, MAUTOSTRIP_VOID);

	CWObject_Hook::OnTransform(_Mat);

	CVec3Dfp32 Axis = GetAttachClientData_Hook(this)->m_HelpAxis;
	Axis.MultiplyMatrix3x3(_Mat);
	GetAttachClientData_Hook(this)->m_HelpAxis = Axis;
}

CMat4Dfp32 CWObject_Hook_To_Circle::GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac)
{
	MAUTOSTRIP(CWObject_Hook_To_Circle_GetRenderMatrix, CMat4Dfp32());

	if(!GetAttach(1) || !GetAttach(2))
		return CWObject_Hook::GetRenderMatrix(_pWPhysState, _Time, _GameTick, _TickFrac);
	else
	{
		CMat4Dfp32 Pos = CWObject_Hook::GetRenderMatrix(_pWPhysState, _Time, _GameTick, _TickFrac);
		CVec3Dfp32 &O = CVec3Dfp32::GetMatrixRow(Pos, 3);
		
		CVec3Dfp32 CircleOrigin = GetAttachPos(_pWPhysState, 1, _Time, _GameTick, _TickFrac);
		CVec3Dfp32 CirclePos = GetAttachPos(_pWPhysState, 2, _Time, _GameTick, _TickFrac);
		
		float dc = CircleOrigin.Distance(O);
		float r = CircleOrigin.Distance(CirclePos);
		float l = GetOrgPosition().Distance(GetOrgAttachPos(2));
		
		CMat4Dfp32 Mat;
		Mat.Unit();
		(O - CircleOrigin).SetMatrixRow(Mat, 0);
		(CirclePos - CircleOrigin).SetMatrixRow(Mat, 1);
		
		Mat.RecreateMatrix(0, 1);
		
		CVec3Dfp32 Axis = GetAttachClientData_Hook(this)->m_HelpAxis;
		if(Axis == 0)
			Axis = CVec3Dfp32(0, 0, 1);
		if(Axis * CVec3Dfp32::GetMatrixRow(Mat, 2) > 0.0f)
		{
			(CircleOrigin - CirclePos).SetMatrixRow(Mat, 1);
			Mat.RecreateMatrix(0, 1);
		}
		
		CircleOrigin.SetMatrixRow(Mat, 3);
		
		float x = -(l * l - r * r - dc * dc) / (2 * dc);
		float y = M_Sqrt(Max(0.0f, Sqr(r) - Sqr(x)));
		
		CVec3Dfp32 D0 = CVec3Dfp32(x, y, 0) * Mat;
		CVec3Dfp32 D1 = CVec3Dfp32(x, -y, 0) * Mat;
		CVec3Dfp32 D = D0;
		CMat4Dfp32 M0;
		M0.Unit();
		
		CMat4Dfp32 M1 = M0;
		(GetOrgAttachPos(2) - GetOrgPosition()).SetMatrixRow(M0, 0);
		(D - O).SetMatrixRow(M1, 0);
		M0.RecreateMatrix(0, 2);
		M1.RecreateMatrix(0, 2);
		
		CMat4Dfp32 InvM0, MRes, MRes2;
		M0.InverseOrthogonal(InvM0);
		InvM0.Multiply(M1, MRes);
		
		MRes.Multiply(GetOrgPositionMatrix(), MRes2);
		CVec3Dfp32::GetMatrixRow(Pos, 3).SetMatrixRow(MRes2, 3);
		return MRes2;
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Hook_To_Circle, CWObject_Attach, 0x0100);
#endif

#if 0
static CMat4Dfp32 ParseRotMatString(CStr _s)
{
	MAUTOSTRIP(ParseRotMatString, CMat4Dfp32());
	CMat4Dfp32 Mat;
	
	char* pStr = (char*) _s;
	if(!pStr)
	{
		Mat.Unit();
		return Mat;
	}
	
	CQuatfp32 Q;
	CVec3Dfp32 V;
	
	int pos = 0;
	int len = _s.Len();
	for(int i = 0; i < 4; i++)
	{
		pos = CStr::GoToDigit(pStr, pos, len);
		Q.k[i] = M_AToF(&pStr[pos]);
		pos = CStr::SkipADigit(pStr, pos, len);
	}
	for(int j = 0; j < 3; j++)
	{
		pos = CStr::GoToDigit(pStr, pos, len);
		V.k[j] = M_AToF(&pStr[pos]);
		pos = CStr::SkipADigit(pStr, pos, len);
	}
	
	Q.Normalize();
	Q.CreateMatrix(Mat);
	V.SetMatrixRow(Mat, 3);
	return Mat;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CWObject_Engine_Path
CWObject_Engine_Path::CWObject_Engine_Path()
{
	MAUTOSTRIP(CWObject_Engine_Path_ctor, MAUTOSTRIP_VOID);
	m_iAnim0 = -1;
	m_iUserPortal = -1;
	m_iDynamicModel = 0;
	m_iObjectLinkedToEP = 0;
	m_nTicksFailed = 0;
	m_Mass = 100.0f;
	m_Falloff = 50.0f;
	m_LastTargetPos = 0.0f;
	m_PosOffset = 0.0f;
	m_bWorldCollision = true;
	m_bPathIsForObjects = false;
	m_RotOffset.Unit();
	m_iCurBlockMsg = 0;
	m_iCurResumeMsg = 0;
}

void CWObject_Engine_Path::OnCreate()
{
	CWObject_Hook::OnCreate();
}

void CWObject_Engine_Path::OnFinishEvalKeys()
{
	CWObject_Hook::OnFinishEvalKeys();

	if (m_iAnim0 != -1)
	{
		GetAttachClientData_Engine_Path(this)->m_iXWData = m_pWServer->GetMapData()->GetResourceIndex_XWData("$WORLD:0");

		CWO_PosHistory* pCData = GetClientData(this);
		if (!pCData->IsValid())
			LoadPath(m_pWServer, this, m_iAnim0);

		if (pCData->IsValid())
		{
			// Calculate delta-matrix to relocate path data (enable sharing of path data)
			CMat4Dfp32 PathMat0;
			pCData->GetMatrix(0, 0.0f, false, 0, PathMat0);
			const CMat4Dfp32& PosMat0 = GetOrgPositionMatrix();

			CMat4Dfp32 InvPathMat0, PathRelMat;
			PathMat0.InverseOrthogonal(InvPathMat0);
			M_VMatMul(InvPathMat0, PosMat0, PathRelMat);

			GetAttachClientData_Engine_Path(this)->m_PathRelMat = PathRelMat;
		}
	}
		


/*
//	m_Flags	|= FLAGS_RUNONCE;

	for( int i = 0; i < CWO_NUMMODELINDICES; i++ )
	{
		if( m_iModel[i] > 0 )
		{
			CXR_Model *pModel = m_pWServer->GetMapData()->GetResource_Model( m_iModel[i] );
			if( pModel && pModel->GetParam( CXR_MODEL_PARAM_ISALIVE ) )
			{
				// This is only true for particlesystems for now.
				m_Flags	&= ~FLAGS_RUNONCE;
				break;
			}
		}
	}
*/
}

CWObject_Engine_Path::CAttachClientData_Engine_Path *CWObject_Engine_Path::GetAttachClientData_Engine_Path(CWObject_CoreData *_pObj)
{
	return (CAttachClientData_Engine_Path *)GetAttachClientData(_pObj);
}

void CWObject_Engine_Path::Run(int _iType)
{
	MAUTOSTRIP(CWObject_Engine_Path_Run, MAUTOSTRIP_VOID);
	if(!(m_ClientFlags & CLIENTFLAGS_RUN) && !(m_Flags & FLAGS_PSEUDOSTOP) )
	{
		ClientFlags() |= CLIENTFLAGS_RUN;
		
		if(m_Time.GetTime() >= GetDuration())
			m_Time = CMTime::CreateFromSeconds(0.0f);

		if(_iType - 1 != m_iAnim2)
		{
			m_Time = CMTime::CreateFromSeconds(0.0f);
			m_CreationGameTick = m_pWServer->GetGameTick();
			iAnim2() = _iType - 1;
			m_iCurTimedMessage = 0;

			if(m_ClientFlags & CLIENTFLAGS_PHYSICS)
			{
				CMTime Time = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0);
				CMat4Dfp32 Mat = GetRenderMatrix(m_pWServer, Time, m_pWServer->GetGameTick(), 0);
				m_pWServer->Object_SetPosition_World(m_iObject, Mat);
			}
		}
		else
		{
			if(m_Flags & FLAGS_RESETFIX)
			{
				m_CreationGameTick = m_pWServer->GetGameTick();
				m_iCurTimedMessage = 0;
				m_Flags &= ~FLAGS_RESETFIX;

				if(m_ClientFlags & CLIENTFLAGS_PHYSICS)
				{
					CMTime Time = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0);
					CMat4Dfp32 Mat = GetRenderMatrix(m_pWServer, Time, m_pWServer->GetGameTick(), 0);
					m_pWServer->Object_SetPosition_World(m_iObject, Mat);
				}
			}
			else
				m_CreationGameTick = m_pWServer->GetGameTick() - m_CreationGameTick;
		}
		
		if(m_iUserPortal != -1)
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_FUNCPORTAL_SETSTATE, 1, 0, m_iObject), m_iUserPortal);
		
		SetDirty(CWO_DIRTYMASK_COREMASK);

		UpdateNoRefreshFlag();
		GetUpdatedTime();

		if(m_Flags & FLAGS_PHYSICS_DRIVEN && ClientFlags() & CLIENTFLAGS_RUN)	//we can have received a stop msg already
		{
			if(m_iObjectLinkedToEP)
			{
				CWObject *pObj = m_pWServer->Object_Get(m_iObjectLinkedToEP);
				if(pObj)
				{
					CWO_PhysicsState PhysState(pObj->GetPhysState());
					PhysState.m_PhysFlags |= OBJECT_PHYSFLAGS_PHYSMOVEMENT;
					m_pWServer->Object_SetPhysics(m_iObjectLinkedToEP, PhysState);
					m_pWServer->Object_InitRigidBody(m_iObjectLinkedToEP, false);

					m_pWServer->Phys_SetStationary(m_iObjectLinkedToEP, false);
				}
			}
			else if(!m_bPathIsForObjects)
			{
				CWO_PhysicsState PhysState(GetPhysState());
				PhysState.m_PhysFlags |= OBJECT_PHYSFLAGS_PHYSMOVEMENT;
				m_pWServer->Object_SetPhysics(m_iObject, PhysState);
				m_pWServer->Object_InitRigidBody(m_iObject, false);
				m_pWServer->Phys_SetStationary(m_iObject, false);
			}
		}
	}
	else
	{
	/*			if(_iType - 1 != m_iAnim2)
	{
				m_AnimTime = m_pWServer->GetGameTick();
				m_iAnim2 = _iType - 1;
				SetDirty(CWO_DIRTYMASK_COREMASK);
	}*/
	}
}

void CWObject_Engine_Path::Stop()
{
	MAUTOSTRIP(CWObject_Attach_Stop, MAUTOSTRIP_VOID);

	if(m_ClientFlags & CLIENTFLAGS_RUN)
	{
		if(m_Flags & FLAGS_NEVERINTERRUPT)
		{
			m_Flags |= FLAGS_PENDINGSTOP;
		}
		else
		{
			ClientFlags() &= ~CLIENTFLAGS_RUN;
			m_CreationGameTick = m_pWServer->GetGameTick() - m_CreationGameTick;
			CVelocityfp32 Vel;
			Vel.Unit();
			SetVelocity(Vel);
			SetDirty(CWO_DIRTYMASK_COREMASK);

			GetAttachClientData(this)->m_CachedTime.MakeInvalid();
		}

		UpdateNoRefreshFlag();
	}
	ClientFlags() &= ~CLIENTFLAGS_REVERSE;

	if(m_Flags & FLAGS_PHYSICS_DRIVEN)
	{
		if(m_iObjectLinkedToEP)
		{
			CWObject *pObj = m_pWServer->Object_Get(m_iObjectLinkedToEP);
			if(pObj)
			{
				CWO_PhysicsState PhysState(pObj->GetPhysState());
				PhysState.m_PhysFlags &= ~OBJECT_PHYSFLAGS_PHYSMOVEMENT;
				m_pWServer->Object_SetPhysics(m_iObjectLinkedToEP, PhysState);
				m_pWServer->Object_InitRigidBody(m_iObjectLinkedToEP, false);
				m_pWServer->Phys_SetStationary(m_iObjectLinkedToEP, true);
			}
		}
		else if(!m_bPathIsForObjects)
		{
			CWO_PhysicsState PhysState(GetPhysState());
			PhysState.m_PhysFlags &= ~OBJECT_PHYSFLAGS_PHYSMOVEMENT;
			m_pWServer->Object_SetPhysics(m_iObject, PhysState);
			m_pWServer->Object_InitRigidBody(m_iObject, false);
			m_pWServer->Phys_SetStationary(m_iObject, true);
		}
	}
}

void CWObject_Engine_Path::GotoEnd()
{
	MAUTOSTRIP(CWObject_Engine_Path_GotoEnd, MAUTOSTRIP_VOID);
	if(m_ClientFlags & CLIENTFLAGS_RUN)
		Stop();

	CWO_PosHistory *pCData = GetClientData(this);
	
	if(m_iAnim0 != -1 && !pCData->IsValid())
		LoadPath(m_pWServer, this, m_iAnim0);

	if(pCData->m_lSequences.Len() > m_iAnim2)
	{
		m_CreationGameTick = int32(pCData->m_lSequences[m_iAnim2].GetDuration() * m_pWServer->GetGameTicksPerSecond());
		SetDirty(CWO_DIRTYMASK_COREMASK);
	}
}

//Check if we're between two consecutive keyframes with almost same position
bool CWObject_Engine_Path::IsStop(CWO_PosHistory * _pPH, int _iSeq, int _iKeyFrame, fp32 _Epsilon)
{
	//A stop can occur at any position except at the last keyframe
	//(this means an looping engine path cannot have a stop between last and first keyframe)
	if ((_iKeyFrame < 0) || (_iKeyFrame > _pPH->m_lSequences[_iSeq].GetNumKeyframes() - 2))
		return false;

	return _pPH->m_lSequences[_iSeq].GetPos(_iKeyFrame).AlmostEqual(_pPH->m_lSequences[_iSeq].GetPos(_iKeyFrame + 1), _Epsilon);
}

bool CWObject_Engine_Path::IsStop(CWO_PosHistory * _pPH, int _iSeq, fp32 _Time, fp32 _Epsilon)
{
	if(_iSeq < 0 || _iSeq >= _pPH->m_lSequences.Len())
		return false;

	return IsStop(_pPH, _iSeq, _pPH->m_lSequences[_iSeq].GetTimeIndex(_Time), _Epsilon);
}

// Sets the enginepath time and if _bSendMessages is true, send messages
void CWObject_Engine_Path::SetTime(fp32 _Time, bool _bSendMessages)
{
	if (!(m_Flags & FLAGS_NEVERINTERRUPT) && 
		(m_Wait == -1))
	{
		CWO_PosHistory * pPH = GetClientData(this);
		if (pPH)
		{
			//Make sure path has been loaded
			if(m_iAnim0 != -1 && !pPH->IsValid())
				LoadPath(m_pWServer, this, m_iAnim0);

			if(!pPH->IsValid())
				return;

			ClientFlags() |= CLIENTFLAGS_PROPELLED;	
			fp32 Time = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0).GetTime();/*CMTIMEFIX*/
			fp32 NewTime = _Time;
			bool bLoop = (m_ClientFlags & CLIENTFLAGS_LOOP) != 0;

			//To propel path we first stop path to avoid any normal movement
			Stop();

			//Propel path to time
			m_CreationGameTick = int32(NewTime * m_pWServer->GetGameTicksPerSecond());/*CMTIMEFIX*/
			m_CreationGameTickFraction = (NewTime * m_pWServer->GetGameTicksPerSecond()) - m_CreationGameTick;/*CMTIMEFIX*/ 

			if ((m_lTimedMessages.Len())&&
				((bLoop && (NewTime < Time))||
				(NewTime >= pPH->m_lSequences[0].GetDuration())))
			{
				//We've looped or reached end. Make sure we raise all messages at end of path and reset current timed message
				UpdateTimedMessages(CMTime::CreateFromSeconds(Max(pPH->m_lSequences[0].GetDuration(), m_lTimedMessages[m_lTimedMessages.Len() - 1].m_Time) + 0.001f) /*CMTIMEFIX*/);
				if (bLoop)
				{	// Only reset m_iCurTimedMessage when looping
					m_iCurTimedMessage = 0;
				}
			}
		}
	}
};

//Propel path forward to given distance sending any messages as normal. If _AllowStops is true the 
//path won't be propelled past a stop in the path. (i.e. two keyframes at the same position) until 
//the path has run normally past the stop. If the _XYOnly param is true the path will be propelled 
//to the given distance from the position in the XY-plane, i.e. height is ignored.
void CWObject_Engine_Path::PropelPath(fp32 _Distance, bool _bAllowStops, bool _XYOnly, bool _bPropel, CMat4Dfp32* _pMat)
{
	//Currently we always use first sequence of engine path when propelling

	//Can't propel a path with neverinterrupt flag or one waiting for impulse
	if (!(m_Flags & FLAGS_NEVERINTERRUPT) && 
  		(m_Wait == -1))
	{
		CWO_PosHistory * pPH = GetClientData(this);
		if (pPH)
		{
			//Make sure path has been loaded
			if(m_iAnim0 != -1 && !pPH->IsValid())
				LoadPath(m_pWServer, this, m_iAnim0);

			if(!pPH->IsValid())
				return;

			//We need to calculate the time we should set to make the path move
			//the desired distance, based on current time.
			if (_bPropel)
			{
				ClientFlags() |= CLIENTFLAGS_PROPELLED;
			}
			fp32 Time = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0).GetTime();/*CMTIMEFIX*/
			fp32 NewTime = Time;
			bool bLoop = (m_ClientFlags & CLIENTFLAGS_LOOP) != 0;

			//To propel path we first stop path to avoid any normal movement
			Stop();

			//Run one tick if at stop and stops are allowed
			if (_bAllowStops && IsStop(pPH, 0, Time))
			{
				NewTime = Time + m_pWServer->GetGameTickTime();
			}
			else
			{
				//Get current position. We will propel the path to the given distance from this position.
				//This means path may "skip" some key frame positions if distance is great.
				CVec3Dfp32 CurPos = CVec3Dfp32::GetRow(GetRenderMatrix(m_pWServer, CMTime::CreateFromSeconds(Time) /*CMTIMEFIX*/, m_pWServer->GetGameTick(), 0), 3);
				if (_XYOnly)
					CurPos[2] = 0;

				//Get current keyframe and set some other auxiliary stuff
				int iCurKF = pPH->m_lSequences[0].GetTimeIndex(Time);
				int nKF = pPH->m_lSequences[0].GetNumKeyframes();
				fp32 DistSqr = Sqr(_Distance);	
	
				//Find the first keyframe position further away than the distance. For linear paths the we 
				//should propel the path to some point in the preceding keyframe-interval and although this 
				//is also true for splines, this is not necessarily the first point along the path at given 
				//distance. This should perhaps be fixed.
				int iKF;
				CVec3Dfp32 Pos;
				fp32 KFDistSqr;
				int iBestKF = iCurKF + 1;
				fp32 BestKFDistSqr = 0;
				bool bStop = false;
				bool bFound = false;
				for (int i = 1; i <= nKF; i++)
				{
					//Iterate over keyframes from iCurKF+1..nKF-1 or iCurKF+1..iCurKF if looping path
					iKF = (bLoop ? (iCurKF + i) % nKF : iCurKF + i);

					//Stop at end (will only happen if not looping)	
					if (iKF >= nKF)
					{
						iBestKF = nKF - 1;
						break;
					}

					//Get keyframe position
					Pos = pPH->m_lSequences[0].GetPos(iKF);
					if (_XYOnly)
						Pos[2] = 0;

					//Stop if we find a keyframe position further away than distance
					KFDistSqr = CurPos.DistanceSqr(Pos);
					if (KFDistSqr >= DistSqr)
					{
						bFound = true;
						iBestKF = iKF;
						break;
					}

					//Stop if path stops are allowed and we reach one
					if (_bAllowStops && IsStop(pPH, 0, iKF))
					{
						bStop = true;
						iBestKF = iKF;
						break;
					}

					//Update "best keyframe" stuff
					if (KFDistSqr > BestKFDistSqr)
					{
						iBestKF = iKF;
						BestKFDistSqr = KFDistSqr;
					}
				}

				if (bStop)
				{
					//We've reached a stop. Propel path to keyframe position.
					NewTime = pPH->m_lSequences[0].GetTime(iBestKF);
				}
				else if (!bFound)
				{
					//No point the given distance away could be found, Propel the path to the keyframe position 
					//furthest away. Once again, this is incorrect for splines, since there actually might be a 
					//correct point along the path that's between two keyframe positions, or at least a point 
					//between keyframes that is further away.
					NewTime = pPH->m_lSequences[0].GetTime(iBestKF);
				}
				else
				{
					//Found keyframe at end of the interval we should propel path to.
					//Time depends on type of path
					// m_iAnim1 == 0 Linear, m_iAnim1 == 1 Spline, m_iAnim1 == 2 Step, m_iAnim1 == 3 BackmanSpline
					if ((m_iAnim1 < 0) || (m_iAnim1 == 2) || (iBestKF == 0))
					{
						//Step or we're propelling to start keyframe on a looping path (which is also a step)
						//Always propel to keyframe
						NewTime = pPH->m_lSequences[0].GetTime(iBestKF);
					}	// 0 = Linear, 1 = Spline, 2 = Step, 3 = BackmanSpline
					else if (m_iAnim1 == 0)
					{
						//Linear. If we don't have to leave our current interval we're just propelling
						//path along a straight line at constant speed which is easy.
						if (iBestKF == iCurKF + 1)
						{
							//Same interval. This should be the most common case, and is cheaper to 
							//compute than the general case
							CVec3Dfp32 StartPos = pPH->m_lSequences[0].GetPos(iCurKF);
							if (_XYOnly)
								StartPos[2] = 0;
							CVec3Dfp32 EndPos = pPH->m_lSequences[0].GetPos(iBestKF);
							if (_XYOnly)
								EndPos[2] = 0;

							//Speed of this keyframe interval. Note that distance (and therefore velocity)
							//cannot be zero, since then the end keyframe wouldn't be the best keyframe
							fp32 KFLength = pPH->m_lSequences[0].GetKeyframeLength(iCurKF);
							fp32 Vel = (KFLength > 0) ? StartPos.Distance(EndPos) / KFLength : 0;
							if (Vel == 0)
							{
								//Should not occur
								NewTime = pPH->m_lSequences[0].GetTime(iBestKF); 
							}
							else
							{
								//Time when path has moved given distance
								NewTime = (Time + _Distance / Vel);
							}
						}
						else
						{
							//Not in same interval. Time can be calculated by 2nd degree equation:
							//  (p1 - p0)^2 * t^2 + 2 * (p1 - p0) * (p0 - o) * t + (p0 - o)^2 - d^2 = 0
							//where:
							//  p1 = position of keyframe iBestKF
							//  p0 = position of keyframe iBestKF-1
							//  o  = current position 
							//  d  = the distance to propel
							//  t  = the fraction (0..1) of the time from keyframes iBestKF-1 to iBestKF
							CVec3Dfp32 p1p0Diff = pPH->m_lSequences[0].GetPos(iBestKF) - pPH->m_lSequences[0].GetPos(iBestKF - 1);
							if (_XYOnly)
								p1p0Diff[2] = 0;
							CVec3Dfp32 p0oDiff = pPH->m_lSequences[0].GetPos(iBestKF - 1) - CurPos;
							if (_XYOnly)
								p0oDiff[2] = 0;

							//Koefficients for the equation at^2 + bt + c = 0
							fp32 a = p1p0Diff.LengthSqr();
							fp32 b = 2 * (p1p0Diff * p0oDiff);
							fp32 c = p0oDiff.LengthSqr() - DistSqr;

							//Solutions (valid solution is on the interval 0..1)
							fp32 t1, t2;

							//Solve equation
							if (SolveP2(a, b, c, t1, t2))
							{
								//Equation is solvable
								fp32 TStart = pPH->m_lSequences[0].GetTime(iBestKF - 1);
								fp32 TEnd = pPH->m_lSequences[0].GetTime(iBestKF);
								t1 = (TEnd - TStart) * t1 + TStart;
								if ((t1 > Time) && (t1 <= TEnd))
								{
									//t1 is solution
									NewTime = t1;
								}
								else
								{
									//t1 not solution, test t2
									t2 = (TEnd - TStart) * t2 + TStart;
									if ((t2 > Time) && (t2 <= TEnd))
									{
										//t2 is solution
										NewTime = t2;
									}
									else
									{
										//Both solutions incorrect! If this occurs, I've made an algebraic error...AO
										NewTime = pPH->m_lSequences[0].GetTime(iBestKF);										
									}
								}
							}
							else
							{
								//Couldn't solve equation, just propel to best keyframe
								NewTime = pPH->m_lSequences[0].GetTime(iBestKF); 
							}
						}
					}	// 0 = Linear, 1 = Spline, 2 = Step, 3 = BackmanSpline
					else if ((m_iAnim1 == 1)||(m_iAnim1 == 3))
					{
						//Spline. Inaccurate for now; doesn't compute the exact position but will instead
						//guess a time and search for a suitable position given the result of the guess.
						fp32 TGuess;
						CVec3Dfp32 StartPos;;
						CVec3Dfp32 EndPos = pPH->m_lSequences[0].GetPos(iBestKF);
						if (_XYOnly)
							EndPos[2] = 0;
						if (iBestKF == iCurKF + 1)
						{
							//We're propelling path to another position within the same keyframe interval.
							//Use velocity from current position to end of interval for guess
							StartPos = CurPos;
							fp32 Vel = CurPos.Distance(EndPos) / (pPH->m_lSequences[0].GetTime(iBestKF) - Time);
							if (Vel == 0)
							{
								//Should not occur
								TGuess = pPH->m_lSequences[0].GetTime(iBestKF); 
							}
							else
							{
								TGuess = Time + _Distance / Vel;
							}
						}
						else
						{
							//Propelling path into another keyframe interval. Use distamce to start and end 
							//positions to make guess
							StartPos = pPH->m_lSequences[0].GetPos(iBestKF - 1);
							if (_XYOnly)
								StartPos[2] = 0;

							fp32 StartDist = CurPos.Distance(StartPos);
							fp32 EndDist = CurPos.Distance(EndPos);
							if (StartDist == EndDist)
							{
								//Should not occur
								TGuess = pPH->m_lSequences[0].GetTime(iBestKF); 
							}
							else
							{
								fp32 TFrac = (_Distance - StartDist) / (EndDist - StartDist);
								TGuess = pPH->m_lSequences[0].GetKeyframeLength(iBestKF - 1) * TFrac + pPH->m_lSequences[0].GetTime(iBestKF - 1); 
							}
						}

						//By now we should have a pretty good guess. Start refining guess until we've got sufficient quality
						//For now the guess must be at least at the desired distance and not more than 10% past it.
						//However only allow no more than 3 iterations, then we use best guess which is at least at the desired distance.
						CVec3Dfp32 GuessPos;
						fp32 GuessDistSqr;
						fp32 TMin = ((iBestKF == iCurKF + 1) ? Time : pPH->m_lSequences[0].GetTime(iBestKF - 1));
						fp32 TMax = pPH->m_lSequences[0].GetTime(iBestKF);
						fp32 TBestGuess = pPH->m_lSequences[0].GetTime(iBestKF);
						fp32 TBestDistSqr = CurPos.DistanceSqr(EndPos);
						fp32 MaxDelta = ((TGuess > Time) ? TGuess - Time : pPH->m_lSequences[0].GetDuration() + TGuess - Time);
						for (int i = 0; i < 3; i++)
						{
							//The guessing algorithm might be incorrect with complex splines... will test some...
							GuessPos = CVec3Dfp32::GetRow(GetRenderMatrix(m_pWServer, CMTime::CreateFromSeconds(TGuess)/*CMTIMEFIX*/, m_pWServer->GetGameTick(), 0), 3);
							if (_XYOnly)
								GuessPos[2] = 0;
							GuessDistSqr = CurPos.DistanceSqr(GuessPos);

							if (GuessDistSqr < DistSqr)
							{
								//Too close, adjust to later time
								TMin = TGuess;
								TGuess = TGuess + Min((TMax - TGuess) / 2, MaxDelta);								
							}
							else if (GuessDistSqr > (1.1f*1.1f)*DistSqr)
							{		
								//Too far. Check if this guess is best one so far.
								if (GuessDistSqr < TBestDistSqr)
								{
									TBestGuess = TGuess;
									TBestDistSqr = GuessDistSqr;
								}
								
								//Adjust to earlier time
								TMax = TGuess;
								TGuess = TGuess - Min((TGuess - TMin) / 2, MaxDelta);								
							}
							else
							{
								//Good enough guess!
								TBestGuess = TGuess;
								break;
							}

							//Increase allowed guess-delta each iteration
							MaxDelta *= 2;
						}

						//Use best guess
						NewTime = TBestGuess;
					}
				}
			}

			if (_pMat)
			{
				*_pMat = GetRenderMatrix(m_pWServer, CMTime::CreateFromSeconds(NewTime), m_pWServer->GetGameTick(), 0);/*CMTIMEFIX*/
			}

			//Propel path to time
			if (_bPropel)
			{
				m_CreationGameTick = int32(NewTime * m_pWServer->GetGameTicksPerSecond());/*CMTIMEFIX*/
				m_CreationGameTickFraction = (NewTime * m_pWServer->GetGameTicksPerSecond()) - m_CreationGameTick;/*CMTIMEFIX*/ 

				if (m_lTimedMessages.Len() &&
					((bLoop && (NewTime < Time)) || (NewTime >= pPH->m_lSequences[0].GetDuration())))
				{
					//We've looped or reached end. Make sure we raise all messages at end of path and reset current timed message
					UpdateTimedMessages(CMTime::CreateFromSeconds(Max(pPH->m_lSequences[0].GetDuration(), m_lTimedMessages[m_lTimedMessages.Len() - 1].m_Time) + 0.001f) /*CMTIMEFIX*/);
					if (bLoop)
					{	// Only reset m_iCurTimedMessage when looping
						m_iCurTimedMessage = 0;
					}
				}
			}
		}
	}
}


//Position matrix of propelled path is normal render position matrix or position 
//of last keyframe with the direction of render matrix if stops are allowed and 
//path is between two engine paths with same position
CMat4Dfp32 CWObject_Engine_Path::PropelledPosition(bool _AllowStops)
{
	if (!m_pWServer)
		return GetPositionMatrix();
	
	fp32 Time = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0).GetTime();/*CMTIMEFIX*/

	//If stops are allowed, check if we're at a one
	if (_AllowStops)
	{
		//Two consecutive keyframes less than 1 unit apart is considered a stop, except 
		//between last and first keyframe on a looping path
		CWO_PosHistory * pPH = GetClientData(this);
		if (pPH && IsStop(pPH, 0, Time))
		{
			//Stop found
			int iCurKF = pPH->m_lSequences[0].GetTimeIndex(Time);
			CMat4Dfp32 Res;
			if ((iCurKF >= 0) && (iCurKF < pPH->m_lSequences[0].GetNumKeyframes() - 1) &&
				(pPH->m_lSequences[0].GetRot(iCurKF).DotProd(pPH->m_lSequences[0].GetRot(iCurKF + 1)) > 0.99f))
			{
				//Same rotation, use first keyframe rotation
				pPH->m_lSequences[0].GetRot(iCurKF).CreateMatrix(Res);
			}
			else
			{
				//Different rotation, use interpolated rotation
				Res = GetRenderMatrix(m_pWServer, CMTime::CreateFromSeconds(Time)/*CMTIMEFIX*/, m_pWServer->GetGameTick(), 0);	
			}

			//Always use first keyframe position
			pPH->m_lSequences[0].GetPos(iCurKF).SetRow(Res, 3);
			return Res;
		}
	}

	//Not stop, use render position
	return GetRenderMatrix(m_pWServer, CMTime::CreateFromSeconds(Time), m_pWServer->GetGameTick(), 0);/*CMTIMEFIX*/
}




CWO_PosHistory *CWObject_Engine_Path::GetClientData(CWObject_CoreData *_pObj)
{
	MAUTOSTRIP(CWObject_Engine_Path_GetClientData, NULL);
	if(!_pObj->m_lspClientObj[1])
	{
		_pObj->m_lspClientObj[1] = MNew(CWO_PosHistory);
		if(!_pObj->m_lspClientObj[1])
			Error_static("CWObject_Engine_Path::GetClientData", "Out of memory.")
	}
	return (CWO_PosHistory *)(CReferenceCount *)_pObj->m_lspClientObj[1];
}


void CWObject_Engine_Path::LoadPath(CWorld_PhysState* _pPhysState, CWObject_CoreData *_pObj, int _iIndex)
{
	MAUTOSTRIP(CWObject_Engine_Path_LoadPath, MAUTOSTRIP_VOID);
	MSCOPE(LoadPath, ENGINEPATH);

	CAttachClientData_Engine_Path* pCD = GetAttachClientData_Engine_Path(_pObj);
	if (pCD->m_iXWData)
	{
		TAP<const uint8> pData = _pPhysState->GetMapData()->GetResource_XWData(pCD->m_iXWData, _iIndex);
		if (pData.GetBasePtr())
			GetClientData(_pObj)->LoadPath(pData.GetBasePtr(), pCD->m_TransformMat);
	}
}

void CWObject_Engine_Path::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
{
	MAUTOSTRIP(CWObject_Engine_Path_OnEvalKey, MAUTOSTRIP_VOID);
	
	const CStr KeyName = _pKey->GetThisName();
	
	int KeyValuei = _pKey->GetThisValuei();
	fp32 KeyValuef = _pKey->GetThisValuef();
	
	switch (_KeyHash)
	{
	case MHASH3('RESO','URCE','DATA'): // "RESOURCEDATA"
		{
			m_iAnim0 = KeyValuei;
			break;
		}
	
	case MHASH1('TYPE'): // "TYPE"
		{
			m_iAnim1 = KeyValuei;
			break;
		}
	
	case MHASH3('USER','PORT','AL'): // "USERPORTAL"
		{
			GetInitData()->m_UserPortal = _pKey->GetThisValue();
			break;
		}

	case MHASH4('BSPM','ODEL','INDE','X'):
		{
			CFStr28 ModelName;
			ModelName.CaptureFormated("$WORLD:%d", _pKey->GetThisValuei());
			int iMod = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
			Model_Set(0, iMod, false);
			break;
		}

	case MHASH4('BSPM','ODEL','INDE','X1'):
		{
			CFStr28 ModelName;
			ModelName.CaptureFormated("$WORLD:%d", _pKey->GetThisValuei());
			m_iDynamicModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
			break;
		}

	case MHASH3('EP_O','BJEC','T'):
		{
			m_TargetName = _pKey->GetThisValue();
			break;
		}

	case MHASH1('MASS'):
		{
			m_Mass = KeyValuef;
			break;
		}

	case MHASH2('FALL','OFF'):
		{
			m_Falloff = KeyValuef;
			break;
		}

	case MHASH4('NO_W','ORLD','_COL','LIDE'):
		{
			m_bWorldCollision = false;
			break;
		}
	
	default:
		{
			if(KeyName.CompareSubStr("MSG_ONBLOCK") == 0)
			{
				CTimedMessage Msg;
				Msg.Parse(_pKey->GetThisValue(), m_pWServer);

				AddBlockMsg(Msg);				
			}
			else if(KeyName.CompareSubStr("MSG_ONRESUME") == 0)
			{
				CTimedMessage Msg;
				Msg.Parse(_pKey->GetThisValue(), m_pWServer);

				AddResumeMsg(Msg);				
			}
			else
				CWObject_Hook::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Engine_Path::AddBlockMsg(CTimedMessage _Msg)
{
	for(int i = 0; i < m_lOnBlockMsgs.Len(); i++)
	{
		if(_Msg.m_Time < m_lOnBlockMsgs[i].m_Time)
		{
			m_lOnBlockMsgs.Insert(i, _Msg);
			return;
		}
	}

	m_lOnBlockMsgs.Add(_Msg);
}

void CWObject_Engine_Path::AddResumeMsg(CTimedMessage _Msg)
{
	for(int i = 0; i < m_lOnResumeMsgs.Len(); i++)
	{
		if(_Msg.m_Time < m_lOnResumeMsgs[i].m_Time)
		{
			m_lOnResumeMsgs.Insert(i, _Msg);
			return;
		}
	}

	m_lOnResumeMsgs.Add(_Msg);
}

void CWObject_Engine_Path::OnSpawnWorld()
{
	m_BackupPhysState.Clear();

	MAUTOSTRIP(CWObject_Engine_Path_OnSpawnWorld, MAUTOSTRIP_VOID);
	CWObject_Hook::OnSpawnWorld();

	if(m_spInitData != NULL && m_spInitData->m_UserPortal != "")
	{
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddTarget(Selection, m_spInitData->m_UserPortal);
		const int16 *pSel;
		int nSel = m_pWServer->Selection_Get(Selection, &pSel);
		if(nSel > 1)
			Error("OnSpawnWorld", "Only one userportal may be targeted from Engine_Path");
		if(nSel > 0)
		{
			m_iUserPortal = pSel[0];
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_FUNCPORTAL_SETCONTROLLER, 0, 0, m_iObject), m_iUserPortal);
		}
	}

	// Validate path data
	if (m_iAnim0 != -1)
	{
		CAttachClientData_Engine_Path* pCD = GetAttachClientData_Engine_Path(this);
		TAP<const uint8> pData = m_pWServer->GetMapData()->GetResource_XWData(pCD->m_iXWData, m_iAnim0);
		if (!pData.Len())
			ConOutL(CStrF("§cf00ERROR: Resource data is missing for engine path %s (%d)", GetName(), m_iObject));

		// If we're a child object, adjust the 'pathrelmat' to work in local space 
		if (GetParent() > 0)
		{
			CMat4Dfp32 Tmp;
			GetPositionMatrix().InverseOrthogonal(Tmp);						// Tmp = Pos^-1
			M_VMatMul(Tmp, GetLocalPositionMatrix(), Tmp);					// Tmp = Pos^-1 x LocalPos
			M_VMatMul(pCD->m_PathRelMat, Tmp, Tmp);				// Tmp = PathRelMat0 x (Pos^-1 x LocalPos)
			pCD->m_PathRelMat = Tmp;
		}
	}

	if(m_spInitData && m_spInitData->m_JumpParam != "")
	{
		CAttachClientData_Engine_Path *pCD = GetAttachClientData_Engine_Path(this);
		pCD->m_JumpTick = RoundToInt(m_spInitData->m_JumpParam.GetStrMSep(",; ").Val_fp64() * m_pWServer->GetGameTicksPerSecond()) - 1;
		pCD->m_JumpIndex = m_pWServer->Selection_GetSingleTarget(m_spInitData->m_JumpParam);
	}

	m_DirtyMask |= GetAttachClientData_Engine_Path(this)->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;

	CAttachClientData_Engine_Path *pCD = GetAttachClientData_Engine_Path(this);

	if(m_Flags & FLAGS_PHYSICS_DRIVEN)
	{
		m_ClientFlags |= CLIENTFLAGS_PHYSICS_DRIVEN;

		if(m_TargetName.Len())	
		{
			AttachObject();			
		}
		else
		{
			CWO_PhysicsState Phys = GetPhysState();

			if(!Phys.m_nPrim)
			{	
				if(m_iDynamicModel)
					Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_PHYSMODEL, m_iDynamicModel, CVec3Dfp32(0.0f, 0.0f, 0.0f), CVec3Dfp32(0.0f, 0.0f, 0.0f));
				else
				{
					const CBox3Dfp32 *BBox = GetAbsBoundBox();
					Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, 0, CVec3Dfp32((BBox->m_Max.k[0] - BBox->m_Min.k[0]) / 2.0f, (BBox->m_Max.k[1] - BBox->m_Min.k[1]) / 2.0f, (BBox->m_Max.k[2] - BBox->m_Min.k[2]) / 2.0f), CVec3Dfp32(0.0f, 0.0f, 0.0f));
				}
				Phys.m_nPrim = 1;
			}
			Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_PUSHER | OBJECT_PHYSFLAGS_PHYSICSCONTROLLED;
			Phys.m_ObjectFlags |= OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
			Phys.m_ObjectIntersectFlags |= OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_OBJECT;

			if(m_bWorldCollision)
				Phys.m_ObjectIntersectFlags |= OBJECT_FLAGS_WORLD;
			else
				Phys.m_MediumFlags &= ~XW_MEDIUM_SOLID;

			SetMass(m_Mass);

			m_BackupPhysState = Phys;

			m_pWServer->Object_SetPhysics(m_iObject, Phys);
			m_pWServer->Object_InitRigidBody(m_iObject, false);

			m_pWServer->Phys_SetStationary(m_iObject, true);
		}

		ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

//		bool bStationary = !(m_Root.m_Flags & FLAGS_SPAWNACTIVE);
//		m_pWServer->Phys_SetStationary(m_iObject, bStationary);
	}

	for(int i = 0; i < m_lOnBlockMsgs.Len(); i++)
		m_lOnBlockMsgs[i].SendPrecache(m_iObject, m_pWServer);

	for(int i = 0; i < m_lOnResumeMsgs.Len(); i++)
		m_lOnResumeMsgs[i].SendPrecache(m_iObject, m_pWServer);
}

void CWObject_Engine_Path::AttachObject()
{
	m_iObjectLinkedToEP = 0;
	m_bPathIsForObjects = true;
	TSelection<CSelection::SMALL_BUFFER> Selection;
	m_pWServer->Selection_AddTarget(Selection, m_TargetName);

	int16* pSel = NULL;
	int nSel = m_pWServer->Selection_Get(Selection, (const int16**)&pSel);

	if(nSel == 1)
	{
		m_iObjectLinkedToEP = pSel[0];

		CWObject *pObj = m_pWServer->Object_Get(m_iObjectLinkedToEP);
		if(pObj)
		{
			CQuatfp32 r0; 

			CMat4Dfp32 Pos = GetPositionMatrix();
			CMat4Dfp32 ObjPos = pObj->GetPositionMatrix();
			r0.Create(ObjPos); 
			r0.Inverse();
			CQuatfp32 r1;
			r1.Create(Pos); 
			CQuatfp32 dRot = r0;
			dRot *= r1; 
			CAxisRotfp32 Rot = dRot;

			dRot.SetMatrix(m_RotOffset);

			CVec3Dfp32 ThisPos = GetRenderMatrix(m_pWServer, m_Time, m_pWServer->GetGameTick(), 0.0f).GetRow(3);

			CVec3Dfp32 WVec = pObj->GetPosition() - ThisPos;
			CMat4Dfp32 InvObjMat;
			pObj->GetPositionMatrix().Inverse(InvObjMat);
			InvObjMat.UnitNot3x3();
			m_PosOffset = WVec * InvObjMat;
			if(!pObj->m_pRigidBody2)
			{
				ConOutL(CStrF("§cf80WARNING: Physic driven engine path(Name: %s) target object %s has no rigid body information, creating it\n", GetName(), m_TargetName.Str()));
				CWO_PhysicsState Phys = pObj->GetPhysState();
				if(!Phys.m_nPrim)
				{	//Object has no phys primitives, lets create a box
					const CBox3Dfp32 *BBox = pObj->GetAbsBoundBox();
					Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, 0, CVec3Dfp32((BBox->m_Max.k[0] - BBox->m_Min.k[0]) / 2.0f, (BBox->m_Max.k[1] - BBox->m_Min.k[1]) / 2.0f, (BBox->m_Max.k[2] - BBox->m_Min.k[2]) / 2.0f), CVec3Dfp32(0.0f, 0.0f, 0.0f));
					Phys.m_nPrim = 1;
				}

				//						Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_APPLYROTVEL | OBJECT_PHYSFLAGS_ROTATION | OBJECT_PHYSFLAGS_PUSHER | OBJECT_PHYSFLAGS_PHYSMOVEMENT | OBJECT_PHYSFLAGS_PHYSICSCONTROLLED;
				Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_PUSHER | OBJECT_PHYSFLAGS_PHYSICSCONTROLLED;
				Phys.m_ObjectFlags |= OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
				Phys.m_ObjectIntersectFlags |= OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_OBJECT;

				if(m_bWorldCollision)
					Phys.m_ObjectIntersectFlags |= OBJECT_FLAGS_WORLD;
				else
					Phys.m_MediumFlags &= ~XW_MEDIUM_SOLID;

				m_pWServer->Object_SetPhysics(m_iObjectLinkedToEP, Phys);
				m_pWServer->Object_InitRigidBody(m_iObjectLinkedToEP, false);

				m_pWServer->Phys_SetStationary(m_iObjectLinkedToEP, true);
			}

			CWObject_Message Msg(OBJMSG_OBJECT_SETUP, m_iObject);
			m_pWServer->Message_SendToObject(Msg, m_iObjectLinkedToEP);
		}
		else
			ConOutL(CStrF("§cf80WARNING: Physic driven engine path(Name: %s) has target object %s but it couldn't be found\n", GetName(), m_TargetName.Str()));
	}
	else
		ConOutL(CStrF("§cf80WARNING: Physic driven engine path(Name: %s) has target object %s but it couldn't be found, or multiple copies were found\n", GetName(), m_TargetName.Str()));
}

fp32 CWObject_Engine_Path::GetDuration()
{
	MAUTOSTRIP(CWObject_Engine_Path_GetDuration, 0.0f);
	CWO_PosHistory *pCData = GetClientData(this);
	
	if(m_iAnim0 != -1 && !pCData->IsValid())
		LoadPath(m_pWServer, this, m_iAnim0);

	if(m_iAnim2 >= 0 && m_iAnim2 < pCData->m_lSequences.Len())
		return pCData->m_lSequences[m_iAnim2].GetDuration();

	return -1;
}

bool CWObject_Engine_Path::NeedRefresh()
{
	MAUTOSTRIP(CWObject_Engine_Path_NeedRefresh, false);
	if(CWObject_Hook::NeedRefresh())
		return true;

	if((m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED)) && (m_Flags & FLAGS_AUTORESET))
		return true;

	return false;
}

void CWObject_Engine_Path::OnRefresh()
{
	MAUTOSTRIP(CWObject_Engine_Path_OnRefresh, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_Engine_Path::OnRefresh, ENGINEPATH);

	//Don't do anything if deactivated. Perhaps we shoulddo some stuff, though...
	if (m_ClientFlags & CLIENTFLAGS_DEACTIVATED)
		return;

//	int iSize = sizeof(CWObject_Engine_Path);
	if((m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED)) && (m_Flags & FLAGS_AUTORESET))
	{
		CMTime Time = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0);
		CWO_PosHistory *pCData = GetClientData(this);

		if(m_ClientFlags & CLIENTFLAGS_REVERSE)
		{
			if(Time.Compare(0) <= 0)
			{
				Stop();
				if(m_iUserPortal != -1)
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_FUNCPORTAL_SETSTATE, 0, 0, m_iObject), m_iUserPortal);
				m_Flags |= FLAGS_RESETFIX;
				if(m_Flags & FLAGS_ONCE)
					Destroy();
/*				if(m_Flags & FLAGS_RUNONCE)
				{
					m_Flags	= ( m_Flags & ~FLAGS_RUNONCE ) | FLAGS_PSEUDOSTOP;
					ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
				}*/
			}
		}
		else
		{
			fp32 TotTime = -1;
			if(m_iAnim0 != -1 && !pCData->IsValid())
				LoadPath(m_pWServer, this, m_iAnim0);
			if(pCData->m_lSequences.Len() > m_iAnim2)
				TotTime = pCData->m_lSequences[m_iAnim2].GetDuration();
			if(m_lTimedMessages.Len())
				TotTime = Max(TotTime, m_lTimedMessages[m_lTimedMessages.Len() - 1].m_Time);

			if(TotTime != -1 && Time.Compare(TotTime+0.001f) > 0)
			{
				UpdateTimedMessages(Time);
				Stop();
				if(m_iUserPortal != -1)
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_FUNCPORTAL_SETSTATE, 0, 0, m_iObject), m_iUserPortal);
				m_Flags |= FLAGS_RESETFIX;
				if(m_Flags & FLAGS_ONCE)
					Destroy();
/*				if( m_Flags & FLAGS_RUNONCE )
				{
					m_Flags	= ( m_Flags & ~FLAGS_RUNONCE ) | FLAGS_PSEUDOSTOP;
					ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
				}*/
			}
		}
	}

	//object can have been deactived by itself in UpdateTimedMessages()
	if(m_ClientFlags & CLIENTFLAGS_DEACTIVATED)
		return;
	
	if(!(m_Flags & FLAGS_PHYSICS_DRIVEN))
		CWObject_Hook::OnRefresh();
	else
		MoveObject();

	if (m_ClientFlags & CLIENTFLAGS_PROPELLED)
		ClientFlags() &= ~CLIENTFLAGS_PROPELLED;	
}

void CWObject_Engine_Path::MoveObject(void)
{
	if(m_ClientFlags & CLIENTFLAGS_DEACTIVATED)
		return;

	if(m_Flags & FLAGS_AUTOSTART)
	{
		Run(m_iAnim2 + 1);
		m_Flags &= ~FLAGS_AUTOSTART;
	}

	if(!(m_ClientFlags & CLIENTFLAGS_RUN))
		return;

	CWObject *pObj = NULL;
	if(m_iObjectLinkedToEP)
		pObj = m_pWServer->Object_Get(m_iObjectLinkedToEP);

	GetUpdatedTime(&m_Time);	
	//object can have been deactived by itself in GetUpdatedTime()
	if(m_ClientFlags & CLIENTFLAGS_DEACTIVATED)
		return;
	CMTime NextTime = m_Time + CMTime::CreateFromTicks(1, m_pWServer->GetGameTickTime(), 0.0f);
	CMat4Dfp32 TargetMat = GetRenderMatrix(m_pWServer, NextTime, m_pWServer->GetGameTick(), 0.0f);
	CMat4Dfp32 OldMat = GetRenderMatrix(m_pWServer, m_Time, m_pWServer->GetGameTick(), 0.0f);

	CVec3Dfp32 EPMove = TargetMat.GetRow(3) - OldMat.GetRow(3);
	
	CMat4Dfp32 CurMat;
	if(pObj)
	{
		CurMat = pObj->GetPositionMatrix();
		CMat4Dfp32 ToWorld = CurMat;
		ToWorld.UnitNot3x3();
		TargetMat.GetRow(3) += m_PosOffset * ToWorld;
	}
	else
		CurMat = GetPositionMatrix();

	CVec3Dfp32 Move = TargetMat.GetRow(3) - CurMat.GetRow(3);
	
	fp32 MoveVel = Move.Length();
	fp32 EPVel = EPMove.Length();
	if(EPVel < 0.001f)
		EPVel = 0.0f;

	bool bStationary = false;
	if(pObj)
		bStationary = m_pWServer->Phys_IsStationary(pObj->m_iObject);
	else
		bStationary = m_pWServer->Phys_IsStationary(m_iObject);
	if(MoveVel && bStationary)
	{
		if(pObj)
			m_pWServer->Phys_SetStationary(pObj->m_iObject, false);
		else
			m_pWServer->Phys_SetStationary(m_iObject, false);
	}

	bool bAddTime = true;
	bool bTryFullSpeed = false;
	if(m_LastTargetPos.AlmostEqual(CurMat.GetRow(3), 0.1f))
		bTryFullSpeed = true;
	if(MoveVel > EPVel * 1.25f && EPVel > 0.2f && !bTryFullSpeed)
	{
		bAddTime = false;
		m_CreationGameTick++;	//Must offset creation time so we will get the correct time if we do a GetUpdatedTime without sending in our time
		if(MoveVel > 1.0f)
		{
			Move.Normalize();
			fp32 scale = 0.5f;
			if(m_nTicksFailed > 5)
				scale = 0.1f;
			Move.k[0] *= EPVel * scale;
			Move.k[1] *= EPVel * scale;
			Move.k[2] *= EPVel;	//Gravity fix, could most likely be done better, but it will do for now
			MoveVel = Move.Length();
		}
		m_nTicksFailed++;
		if(m_nTicksFailed > 20)
		{
			if(m_Flags & FLAGS_AUTOREVERSE)
				m_ClientFlags ^= CLIENTFLAGS_REVERSE;
		}
		if(m_nTicksFailed > 1)
		{
			SendBlockedMsg();
			m_iCurResumeMsg = 0;
		}
	}
	else
	{
		if(m_nTicksFailed > 1)
			SendResumeMsg();
		m_nTicksFailed = 0;
		m_iCurBlockMsg = 0;
	}
	
	m_LastTargetPos = CurMat.GetRow(3) + Move;

	CQuatfp32 r0; 

	CMat4Dfp32 StartMat;
	m_RotOffset.Multiply3x3(CurMat, StartMat);

	r0.Create(StartMat); 
	r0.Inverse();
	CQuatfp32 r1;
	r1.Create(TargetMat); 
	CQuatfp32 dRot = r0;
	dRot *= r1;
	CAxisRotfp32 Rot = dRot;

	if(m_bPathIsForObjects && !pObj)
		bAddTime = true;

	//update time?
	if(m_ClientFlags & CLIENTFLAGS_RUN && bAddTime)
	{
		if(!(m_ClientFlags & CLIENTFLAGS_REVERSE))
			m_Time += CMTime::CreateFromTicks(1, m_pWServer->GetGameTickTime(), 0.0f);
		else
			m_Time -= CMTime::CreateFromTicks(1, m_pWServer->GetGameTickTime(), 0.0f);
	}

	if(pObj)
	{
		m_pWServer->Object_SetVelocity(pObj->m_iObject, Move);
		m_pWServer->Object_SetRotVelocity(pObj->m_iObject, Rot);
	}
	else if(!m_bPathIsForObjects)
	{
		m_pWServer->Object_SetVelocity(m_iObject, Move);
		m_pWServer->Object_SetRotVelocity(m_iObject, Rot);
	}
}

int CWObject_Engine_Path::CheckFalloff(fp32 _Impact)
{
	if(_Impact >= m_Falloff)
	{
		m_iObjectLinkedToEP = 0;
		Stop();
		m_ClientFlags |= CLIENTFLAGS_DEACTIVATED;
		return 1;
	}
	return 0;
}

void CWObject_Engine_Path::SendBlockedMsg(void)
{
	fp32 seconds = m_nTicksFailed * m_pWServer->GetGameTickTime();
	CWObject_Message Msg;
	for(int i = m_iCurBlockMsg; i < m_lOnBlockMsgs.Len(); i++)
	{
		if(m_lOnBlockMsgs[i].m_Time <= seconds)
		{
			int iObj = m_iObject;
			CWObject *pObj = NULL;

			if(m_iObjectLinkedToEP)
				pObj = m_pWServer->Object_Get(m_iObjectLinkedToEP);

			if(pObj) 
				iObj = pObj->m_iObject;

			m_lOnBlockMsgs[i].CreateMessage(Msg, iObj, -1);
			m_lOnBlockMsgs[i].SendMessage(iObj, -1, m_pWServer);
			m_iCurBlockMsg = i + 1;
		}
	}
}

void CWObject_Engine_Path::SendResumeMsg(void)
{
	fp32 seconds = m_nTicksFailed / m_pWServer->GetGameTicksPerSecond();
	CWObject_Message Msg;
	for(int i = m_iCurResumeMsg; i < m_lOnResumeMsgs.Len(); i++)
	{
		if(m_lOnResumeMsgs[i].m_Time <= seconds)
		{
			int iObj = m_iObject;
			CWObject *pObj = NULL;

			if(m_iObjectLinkedToEP)
				pObj = m_pWServer->Object_Get(m_iObjectLinkedToEP);

			if(pObj) 
				iObj = pObj->m_iObject;

			m_lOnResumeMsgs[i].CreateMessage(Msg, iObj, -1);
			m_lOnResumeMsgs[i].SendMessage(iObj, -1, m_pWServer);
			m_iCurResumeMsg = i + 1;
		}
	}
}

int CWObject_Engine_Path::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	switch(_Event)
	{
	case CWO_PHYSEVENT_DYNAMICS_COLLISION:
		{
//			_pObj->m_pRigidBody->SetAngularVelocity(CVec3Dfp64(0.0f, 0.0f, 0.0f)); 
			return SERVER_PHYS_DEFAULTHANDLER;
		}
	}
	return CWObject_Hook::OnPhysicsEvent(_pObj, _pObjOther, _pPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);
}


int CWObject_Engine_Path::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	int Res = CWObject_Hook::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);
	CWO_PosHistory *pCData = GetClientData(_pObj);
	if(_pObj->m_iAnim0 != -1 && !pCData->IsValid())
		LoadPath(_pWClient, _pObj, _pObj->m_iAnim0);
	return Res;
}


void CWObject_Engine_Path::OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject_Attach::OnClientLoad(_pObj, _pWorld, _pFile, _pWData, _Flags);
}

void CWObject_Engine_Path::OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject_Attach::OnClientSave(_pObj, _pWorld, _pFile, _pWData, _Flags);
}


aint CWObject_Engine_Path::OnMessage(const CWObject_Message &_Msg)
{
	MAUTOSTRIP(CWObject_Engine_Path_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAME_SPAWN:
		{
			if(m_Flags & FLAGS_PHYSICS_DRIVEN)
			{
				if(_Msg.m_Param0 == 0)
				{	//Spawn
					if(m_iObjectLinkedToEP)
					{
						CWO_PhysicsState Phys = m_BackupPhysState;
						m_pWServer->Object_SetPhysics(m_iObjectLinkedToEP, Phys);
						m_pWServer->Object_InitRigidBody(m_iObjectLinkedToEP, false);
						m_pWServer->Phys_SetStationary(m_iObjectLinkedToEP, true);
					}
					else if(!m_bPathIsForObjects)
					{
						CWO_PhysicsState Phys = m_BackupPhysState;
						m_pWServer->Object_SetPhysics(m_iObject, Phys);
						m_pWServer->Object_InitRigidBody(m_iObject, false);
						m_pWServer->Phys_SetStationary(m_iObject, true);
					}
				}
				else 
				{
					if(m_iObjectLinkedToEP)
					{
						CWObject *pObj = m_pWServer->Object_Get(m_iObjectLinkedToEP);
						if(pObj)
						{
							m_BackupPhysState = pObj->GetPhysState();
							CWO_PhysicsState Phys;
							Phys.Clear();
							m_pWServer->Object_SetPhysics(m_iObjectLinkedToEP, Phys);
							m_pWServer->Object_InitRigidBody(m_iObjectLinkedToEP, false);
						}
					}
					else if(!m_bPathIsForObjects)
					{
						m_BackupPhysState = GetPhysState();
						CWO_PhysicsState Phys;
						Phys.Clear();
						m_pWServer->Object_SetPhysics(m_iObject, Phys);
						m_pWServer->Object_InitRigidBody(m_iObject, false);
					}
				}
			}

			CWObject_Hook::OnMessage(_Msg);
		}
		return 1;

	case OBJMSG_ENGINEPATH_PROPELPATH:	
		{
			bool bAllowStops = (_Msg.m_Param0 & EPFLAGS_ALLOW_STOPS) ? true : false;
			bool bXYOnly = (_Msg.m_Param0 & EPFLAGS_XYONLY) ? true : false;
			bool bPropel = (_Msg.m_Param0 & EPFLAGS_PROPEL) ? true : false;
			CMat4Dfp32 * Res = (CMat4Dfp32 *)_Msg.m_pData;
			if (Res)
			{
				PropelPath(_Msg.m_VecParam0[0], bAllowStops, bXYOnly, bPropel, Res);
			}
			else
			{
				PropelPath(_Msg.m_VecParam0[0], bAllowStops, bXYOnly, bPropel, NULL);
			}
			/*
			//If a transform matrix is provided, we set this to new
			//propelled tranform matrix of engine path
			if (Res)
			{
				*Res = PropelledPosition(_Msg.m_Param0 != 0);
			}
			*/
		}
		return 1;
	case OBJMSG_ENGINEPATH_SETTIME:
		{
			bool bIsRunning = false;
			if(m_ClientFlags & CLIENTFLAGS_RUN)
			{
				bIsRunning = true;
			}
			SetTime(_Msg.m_VecParam0[0], _Msg.m_Param0 != 0);
			if (bIsRunning)
			{
				Run(1);
			}
			//If a transform matrix is provided, we set this to new
			//propelled tranform matrix of engine path
			CMat4Dfp32 * pMat = (CMat4Dfp32 *)_Msg.m_pData;
			if (pMat)
			{
				CMTime Time = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0);
				*pMat = GetRenderMatrix(m_pWServer, Time, m_pWServer->GetGameTick(), 0);
			}
		};
		return 1;
	case OBJSYSMSG_GETCAMERA:
		{
			CMTime Time = GetTime(this, m_pWServer, m_pWServer->GetGameTick(), 0);
			CMTime JumpTime = PHYSSTATE_TICKS_TO_TIME(GetAttachClientData_Engine_Path(this)->m_JumpTick, m_pWServer);
			if(JumpTime.Compare(0) > 0 && Time.Compare(JumpTime) > 0)
				return m_pWServer->Message_SendToObject(_Msg, GetAttachClientData_Engine_Path(this)->m_JumpIndex);
			
			CMat4Dfp32 *pMat = (CMat4Dfp32*)_Msg.m_pData;
			*pMat = GetRenderMatrix(m_pWServer, Time, m_pWServer->GetGameTick(), 0);
			return 1;
		}
	case OBJMSG_HOOK_GETSEQUENCE:
		return m_iAnim2 + 1;

	case OBJMSG_ENGINEPATH_CHECKFALLOFF:
		{
			return CheckFalloff(_Msg.m_VecParam0.k[0]);
		}

	case OBJMSG_ENGINEPATH_DETACH_PHYSICS_OBJECT:
		{
			if(m_iObjectLinkedToEP)
			{
				int iObj = m_iObjectLinkedToEP;
				m_ClientFlags |= CLIENTFLAGS_DEACTIVATED;
				if(_Msg.m_Param0)
				{
					m_pWServer->Object_SetVelocity(iObj, CVec3Dfp32(0.0f, 0.0f, 0.0f));
					CAxisRotfp32 Axis; Axis.Unit();
					m_pWServer->Object_SetRotVelocity(iObj, Axis);
				}
				m_iObjectLinkedToEP = 0;
			}

			return 1;
		}

	case OBJMSG_ENGINEPATH_ATTACH_PHYSICS_OBJECT:
		{
			m_TargetName = (char *)_Msg.m_pData;

			AttachObject();
			return 1;
		}

#ifndef M_RTM
	case OBJSYSMSG_GETDEBUGSTRING:
		if(_Msg.m_DataSize == sizeof(CStr *))
		{
			CWObject_Hook::OnMessage(_Msg);
			CStr *pSt = (CStr *)_Msg.m_pData;
			*pSt += CStrF("  Sequence: %i", m_iAnim2);
		}
		return 1;
#endif
	}
	return CWObject_Hook::OnMessage(_Msg);
}

CMat4Dfp32 CWObject_Engine_Path::GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac)
{
	MAUTOSTRIP(CWObject_Engine_Path_GetRenderMatrix, CMat4Dfp32());
	CWO_PosHistory *pCData = GetClientData(this);
	
	if(m_iAnim0 != -1 && !pCData->IsValid())
		LoadPath(_pWPhysState, this, m_iAnim0);
	
	if(m_iAnim2 < 0 || m_iAnim2 >= pCData->m_lSequences.Len())
	{
		if(m_iAnim2 != 0)
			ConOutLD(CStrF("§cf80WARNING: (GP) CWObject_Engine_Path, Sequence %i does not exist", m_iAnim2));

		if(GetAttach(0))
		{
			CMat4Dfp32 OAttach = GetOrgAttachMatrix(0);
			CMat4Dfp32 Attach = GetAttachMatrix(_pWPhysState, 0, _Time, _GameTick, _TickFrac);
			CMat4Dfp32 IAttach, IOAttach, DAttach;
			OAttach.InverseOrthogonal(IOAttach);
			IOAttach.Multiply(Attach, DAttach);

			CMat4Dfp32 Res;
			GetOrgPositionMatrix().Multiply(DAttach, Res);
			return Res;
		}
		else
			return GetOrgPositionMatrix();
	}

	CMat4Dfp32 Mat;
	if (pCData->GetMatrix(m_iAnim2, _Time.GetTime() /*CMTIMEFIX*/, (m_ClientFlags & CLIENTFLAGS_LOOP) != 0, m_iAnim1, Mat))
	{
		const CMat4Dfp32& PathRelMat = GetAttachClientData_Engine_Path(this)->m_PathRelMat;
		M_VMatMul(Mat, PathRelMat, Mat);

		if(GetAttach(0))
		{
/*			CMat4Dfp32 OAttach;
			OAttach.Unit();
			GetOrgAttachPos(0).SetMatrixRow(OAttach, 3);
			CMat4Dfp32 Attach = GetAttachMatrix(_pWPhysState, 0, _Time, _GameTick);
			CMat4Dfp32 IOAttach, DAttach;
			OAttach.InverseOrthogonal(IOAttach);
			IOAttach.Multiply(Attach, DAttach);

			CMat4Dfp32 Tmp;
			DAttach.Multiply(Mat, Tmp);
			Mat = Tmp;*/


			CMat4Dfp32 OAttach = GetOrgAttachMatrix(0);
			CMat4Dfp32 Attach = GetAttachMatrix(_pWPhysState, 0, _Time, _GameTick, _TickFrac);
			CMat4Dfp32 IAttach, IOAttach, DAttach;
			OAttach.InverseOrthogonal(IOAttach);
			IOAttach.Multiply(Attach, DAttach);

			CMat4Dfp32 Tmp;
			Mat.Multiply(DAttach, Tmp);
			Mat = Tmp;
		}

		if (GetParent() > 0)
		{
			CWObject_CoreData* pObjParent = _pWPhysState->Object_GetCD(GetParent());
			if (pObjParent)
			{
				CMat4Dfp32 ParentMatIP;
				Interpolate2(pObjParent->GetLastPositionMatrix(), pObjParent->GetPositionMatrix(), ParentMatIP, _TickFrac);
				M_VMatMul(Mat, ParentMatIP, Mat);
			}
		}
		return Mat;
	}
	else
	{
		return GetPositionMatrix();
	}
}

void CWObject_Engine_Path::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Engine_Path_OnClientRefresh, MAUTOSTRIP_VOID);
	MSCOPESHORT( CWObject_Engine_Path::OnClientRefresh );
//	CWObject_Attach::OnClientRefresh(_pObj, _pWClient);
//	if(!(_pObj->m_ClientFlags & CLIENTFLAGS_PHYSICS))

	if(!(_pObj->m_ClientFlags & CLIENTFLAGS_PHYSICS_DRIVEN))
	{
		// Fix for the culling problems where the object is moved out of it's server bounding-box
		// This code should actually be run only if the object _do_ have physics, since it would
		// reduced the amount of network bandwidth generated by moving around objects. That stopped
		// working when OnClientRefresh was removed from the client-mirrors.
		CMTime Time = GetTime(_pObj, _pWClient, _pWClient->GetGameTick(), 0);
		CMat4Dfp32 Mat = ((CWObject_Attach *)_pObj)->GetRenderMatrix(_pWClient, Time, _pWClient->GetGameTick(), 0);
		if(!Mat.AlmostEqual(_pObj->GetPositionMatrix(), 0.01f))
			_pWClient->Object_ForcePosition_World(_pObj->m_iObject, Mat);
	}
//	_pWClient->Debug_RenderMatrix(_pObj->GetPositionMatrix(), 20, false);
	if (_pObj->m_ClientFlags & CLIENTFLAGS_DEACTIVATED)
		return;
	
	bool bValid = false;
	CMat4Dfp32 Mat;
	CMTime Time;
	
	for(int i = 0; i < CWO_NUMCLIENTOBJ; i++)
	{
		if(_pObj->m_lModelInstances[i])
		{
			if(!bValid)
			{
				Time = GetTime(_pObj, _pWClient, _pWClient->GetGameTick(), 0);
				Mat = ((CWObject_Attach *)_pObj)->GetRenderMatrix(_pWClient, Time, _pWClient->GetGameTick(), 0);
				bValid = true;
			}

			int Flags;
			

			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
			if(pModel && pModel->GetParam(CXR_MODEL_PARAM_TIMEMODE) == CXR_MODEL_TIMEMODE_CONTINUOUS)
			{
				Time = _pWClient->GetGameTime();
				Flags = ~(_pObj->m_Data[3]);
				if (Flags == 1)
				{
					Flags = 2;
				}
			}
			else
			{
				Flags = (_pObj->m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED)) != 0;
			}

			CXR_ModelInstanceContext Context(Time.GetNumTicks(_pWClient->GetGameTicksPerSecond()), _pWClient->GetGameTickTime(), _pObj, _pWClient);
			_pObj->m_lModelInstances[i]->OnRefresh(Context, &Mat, 1, Flags);
		}
	}
}

void CWObject_Engine_Path::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Engine_Path_OnClientRender, MAUTOSTRIP_VOID);
	if((_pObj->m_ClientFlags & CLIENTFLAGS_INVISWHENSTOPPED) && !(_pObj->m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED)))
		return;
	if (_pObj->m_ClientFlags & CLIENTFLAGS_DEACTIVATED)
		return;

	bool bMat = false;
	CMat4Dfp32 Mat;
	
	CAttachClientData_Engine_Path *pCD = GetAttachClientData_Engine_Path(_pObj);

	CMTime Time;
	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
		if(pModel)
		{
			if(!bMat)
			{
				Time = GetTime(_pObj, _pWClient, _pWClient->GetGameTick()/* - 1*/, _pWClient->GetRenderTickFrac());
				Mat = ((CWObject_Attach *)_pObj)->GetRenderMatrix(_pWClient, Time, _pWClient->GetGameTick(), _pWClient->GetRenderTickFrac());
				bMat = true;
			}
			
			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient, i);
			
/*			if (pModel->GetParam(CXR_MODEL_PARAM_NEEDPATHDATA))
				AnimState.m_pspClientData = &(_pObj->m_lspClientObj[1]);
			else
				AnimState.m_pspClientData = &(_pObj->m_lspClientObj[2]);*/
			
			if(pModel && pModel->GetParam(CXR_MODEL_PARAM_TIMEMODE) == CXR_MODEL_TIMEMODE_CONTINUOUS)
			{
				AnimState.m_AnimTime0 = CMTime::CreateFromTicks(_pWClient->GetGameTick(), _pWClient->GetGameTickTime(), _pWClient->GetRenderTickFrac());
				AnimState.m_Data[3] = ~(_pObj->m_Data[3]);
				AnimState.m_AnimTime1 = CMTime::CreateFromSeconds(0.0f);
			}
			else
			{
				AnimState.m_AnimTime0 = Time;
				AnimState.m_Data[3] = (_pObj->m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED)) != 0;
				AnimState.m_AnimTime1 = CMTime::CreateFromSeconds(0.0f);
			}

/*			switch((uint8)GetAttachClientData(_pObj)->m_iLightingMode)
			{
			case 0: AnimState.m_AnimAttr0 = Time; break;
			case 1: AnimState.m_AnimAttr0 = 0; break;
			case 2: AnimState.m_AnimAttr0 = 1; break;
			}*/

			AnimState.m_AnimAttr0 = 0;

			if (_pObj->m_ClientFlags & CLIENTFLAGS_USEHINT2) // && Time.Compare(0) == 0)
			{
				if (Time.Compare(0) == 0)
					AnimState.m_AnimTime0 = CMTime::CreateFromTicks(_pWClient->GetGameTick(), _pWClient->GetGameTickTime(), _pWClient->GetRenderTickFrac());

				CXW_Surface* pSurf = _pEngine->m_pSC->GetSurface("creepdark01");
				if (pSurf)
					AnimState.m_lpSurfaces[0] = pSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
			}
			else if (_pObj->m_ClientFlags & CLIENTFLAGS_USEHINT) // && Time.Compare(0) == 0)
			{
				if (Time.Compare(0) == 0)
					AnimState.m_AnimTime0 = CMTime::CreateFromTicks(_pWClient->GetGameTick(), _pWClient->GetGameTickTime(), _pWClient->GetRenderTickFrac());

				CXW_Surface* pSurf = _pEngine->m_pSC->GetSurface("pickupglow");
				if (pSurf)
					AnimState.m_lpSurfaces[0] = pSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
			}

			_pEngine->Render_AddModel(pModel, Mat, AnimState, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_SURF0_ADD);
		}
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Engine_Path, CWObject_Hook, 0x0100);

/////////////////////////////////////////////////////////////////////////////
// CWObject_Dynamic

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Dynamic, CWObject_Engine_Path, 0x0100);

/////////////////////////////////////////////////////////////////////////////
// CWObject_MergedSolid
class CWObject_MergedSolid : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	
public:
	bool m_bInvis:1;
	bool m_bActive:1;
	int32 m_iBSPModel2;
	
	CWObject_MergedSolid()
	{
		MAUTOSTRIP(CWObject_MergedSolid_ctor, MAUTOSTRIP_VOID);
		
		m_bInvis = false;
		m_bActive = false;
		m_iBSPModel2 = -1;
	}
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
	{
		MAUTOSTRIP(CWObject_MergedSolid_OnEvalKey, MAUTOSTRIP_VOID);
		
		CStr KeyName = _pKey->GetThisName();
		
		switch (_KeyHash)
		{
		case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
				{
					CStr ModelName = CStrF("$WORLD:%d", _pKey->GetThisValuei());
		/*
					int XRMode = m_pWServer->Registry_GetGame()->GetValuei("XR_MODE", 0, 0);
					if (XRMode == 1)
						Model_Set(0, m_pWServer->GetMapData()->GetResourceIndex_BSPModel2(ModelName), false);
					else if (XRMode == 2)
						Model_Set(0, m_pWServer->GetMapData()->GetResourceIndex_BSPModel3(ModelName), false);
					else
						Model_Set(0, m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName), false);
		*/
					Model_Set(0, m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName), false);
				break;
				}
		case MHASH4('BSPM','ODEL','INDE','X1'): // "BSPMODELINDEX1"
			{
				CStr ModelName = CStrF("$WORLD:%d", _pKey->GetThisValuei());
				m_iBSPModel2 = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
				break;
			}
		case MHASH3('LIGH','TING','MODE'): // "LIGHTINGMODE"
				{
					iAnim1() = _pKey->GetThisValuei();
				break;
				}
		default:
			{
					CWObject_Model::OnEvalKey(_KeyHash, _pKey);
				break;
			}
		}
	}
	
	void SetChildrenInvisible(bool _bInvis)
	{
		MAUTOSTRIP(CWObject_MergedSolid_SetChildrenInvisible, MAUTOSTRIP_VOID);
//		int nChildren = m_pWServer->Object_GetNumChildren(m_iObject);
		int iChild = m_pWServer->Object_GetFirstChild(m_iObject);
		while(iChild != 0)
		{
			CWObject_Attach *pObj = (CWObject_Attach *)m_pWServer->Object_Get(iChild);
			if(pObj && pObj->m_ClientFlags & CWO_CLIENTFLAGS_ISATTACHOBJ && strlen(pObj->GetName()) == 0 && !(pObj->m_Flags & CWObject_Attach::FLAGS_WAITSPAWN))
			{
				if(_bInvis)
					pObj->ClientFlags() |= (CWO_CLIENTFLAGS_INVISIBLE | CWO_CLIENTFLAGS_NOREFRESH);
				else
					pObj->ClientFlags() &= ~(CWO_CLIENTFLAGS_INVISIBLE | CWO_CLIENTFLAGS_NOREFRESH);
				
				m_pWServer->Phys_InsertPosition(pObj->m_iObject, pObj);
				m_pWServer->Object_SetDirty(iChild, CWO_DIRTYMASK_COREMASK);
			}
			iChild = m_pWServer->Object_GetNextChild(iChild);
		}
		m_bInvis = _bInvis;
	}
	
	virtual void OnSpawnWorld2()
	{
		MAUTOSTRIP(CWObject_MergedSolid_OnSpawnWorld2, MAUTOSTRIP_VOID);
		CWObject_Model::OnSpawnWorld2();
		
		SetChildrenInvisible(true);
	}
	
	virtual aint OnMessage(const CWObject_Message &_Msg)
	{
		MAUTOSTRIP(CWObject_MergedSolid_OnMessage, 0);
		switch(_Msg.m_Msg)
		{
		case OBJMSG_IMPULSE:
		case OBJMSG_GAME_SPAWN:
			if(m_bInvis)
			{
				SetChildrenInvisible(false);
				iModel(0) = 0;
				if(m_iBSPModel2 != -1)
					m_bActive = true;
			}
			//break;
		case OBJSYSMSG_SETMODEL:
		case OBJMSG_MODEL_SETFLAGS:
			{
				int iObj = GetFirstChild();
				while(iObj)
				{
					m_pWServer->Message_SendToObject(_Msg, iObj);
					
					CWObject *pObj = m_pWServer->Object_Get(iObj);
					if(!pObj)
						iObj = 0;
					else
						iObj = pObj->GetNextChild();
				}
				
				return 1;
			}
			
		default:
			return CWObject_Model::OnMessage(_Msg);
		}
		
		return CWObject_Model::OnMessage(_Msg);
	}

	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags)
	{
		CWObject_Model::OnDeltaLoad(_pFile, _Flags);
		uint8 bInvis;
		_pFile->ReadLE(bInvis);
		if (!bInvis)
		{
			SetChildrenInvisible(false);
			iModel(0) = 0;
		}
	}

	virtual void OnDeltaSave(CCFile* _pFile)
	{
		CWObject_Model::OnDeltaSave(_pFile);
		uint8 bInvis = m_bInvis;
		_pFile->WriteLE(bInvis);
	}

	virtual void OnRefresh(void)
	{
		CWObject_Model::OnRefresh();
		if(m_bActive)
		{
			int iChild = m_pWServer->Object_GetFirstChild(m_iObject);
			while(iChild != 0)
			{
				CWObject_Attach *pObj = (CWObject_Attach *)m_pWServer->Object_Get(iChild);
				if(pObj && pObj->m_ClientFlags & CWO_CLIENTFLAGS_ISATTACHOBJ && strlen(pObj->GetName()) == 0)
				{
					fp32 Duration = pObj->GetDuration();
					CMTime Time = pObj->GetTime(pObj, m_pWServer, m_pWServer->GetGameTick(), 0.0f);
					fp32 real_time = Time.GetTime();
					if(real_time < Duration)
						return;
				}
				iChild = m_pWServer->Object_GetNextChild(iChild);
			}

			iChild = m_pWServer->Object_GetFirstChild(m_iObject);
			while(iChild != 0)
			{
				CWObject_Attach *pObj = (CWObject_Attach *)m_pWServer->Object_Get(iChild);
				if(pObj && pObj->m_ClientFlags & CWO_CLIENTFLAGS_ISATTACHOBJ && strlen(pObj->GetName()) == 0)
				{
					int32 del_child = iChild;
					iChild = m_pWServer->Object_GetNextChild(iChild);
					m_pWServer->Object_Destroy(del_child);
				}
				else
					iChild = m_pWServer->Object_GetNextChild(iChild);
			}

			m_bActive = false;
			Model_Set(0, m_iBSPModel2, true);
		}
	}

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
	{
		MAUTOSTRIP(CWObject_MergedSolid_OnClientRender, MAUTOSTRIP_VOID);
		CMat4Dfp32 MatIP;
		fp32 IPTime = _pWClient->GetRenderTickFrac();
		Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);
		
		for(int i = 0; i < 1; i++)
		{
			if(_pObj->m_iModel[i] > 0)
			{
				CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
				if(pModel)
				{
					CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient, i);
					AnimState.m_Data[3] = ~(_pObj->m_Data[3]);
					
					switch(_pObj->m_iAnim1)
					{
					case 1: AnimState.m_AnimAttr0 = 0; break;
					case 2: AnimState.m_AnimAttr0 = 1; break;
					}
					
					_pEngine->Render_AddModel(pModel, MatIP, AnimState);
				}
			}
		}
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_MergedSolid, CWObject_Model, 0x0100);

void CWObject_Dynamic::OnCreate()
{
	CWObject_Engine_Path::OnCreate();
//	m_Flags	|= FLAGS_RUNONCE;
}

void CWObject_TimedMessages::OnCreate()
{
	m_iCurrentMessage = 0;
	m_Flags = 0;
	m_StartTime = CMTime::CreateFromSeconds(0.0f);
}

void CWObject_TimedMessages::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	CStr KeyName = _pKey->GetThisName();
	if (_KeyHash == MHASH2('FLAG','S'))
	{
		static const char *FlagsTranslate[] =
		{
			"autostart", "autoreset",NULL
		};

		m_Flags = _pKey->GetThisValue().TranslateFlags(FlagsTranslate);
		//		m_Flags &= ~FLAGS_GAMEPHYSICS;
	}
	else if(KeyName.CompareSubStr("TIMEDMESSAGE") == 0)
	{
		struct CWObject_Attach::CTimedMessage Msg;
		Msg.Parse(_pKey->GetThisValue(), m_pWServer);

		if((Msg.m_Target == "" || Msg.m_Target == "<NULL>") && Msg.m_Time == 0 && Msg.m_Msg == 0x110)
		{
			// Have to do this for now, because Ogier had a bug which saved this type of message on all dynamics
		}
		else
		{
			uint iSlot = atoi(KeyName.Str() + 12);
			m_lTimedMessages.SetMinLen(iSlot + 1);
			m_lTimedMessages[iSlot] = Msg;
		}
	}
	else
		CWObject::OnEvalKey(_KeyHash, _pKey);
}

void CWObject_TimedMessages::OnFinishEvalKeys()
{
	// Remove empty messages (for instance if we don't have a "0" message)
	for (int32 i = 0; i < m_lTimedMessages.Len(); i++)
	{
		if (m_lTimedMessages[i].m_Time == -1.0f)
		{
			m_lTimedMessages.Del(i);
			i--;
		}
	}

	if (m_Flags & TIMEDMESSAGES_FLAG_AUTOSTART)
		ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;
	else
		ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE | CWO_CLIENTFLAGS_NOREFRESH;
}

void CWObject_TimedMessages::OnSpawnWorld()
{
	// Make sure all resources are included from the timedmessages
	for(int i = 0; i < m_lTimedMessages.Len(); i++)
		m_lTimedMessages[i].SendPrecache(m_iObject, m_pWServer);
}

aint CWObject_TimedMessages::OnMessage(const CWObject_Message &_Msg)
{
	if (_Msg.m_Msg == OBJMSG_IMPULSE)
	{
		if(_Msg.m_Param0 == 0)
		{
			ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
		}
		else if(_Msg.m_Param0 == -1)
		{
			m_iCurrentMessage = 0;
			m_StartTime = CMTime::CreateFromSeconds(0.0f);
			ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
		}
		else
		{
			m_iCurrentMessage = 0;
			if (_Msg.m_Reason == 'CT')
				m_StartTime = *((CMTime*)(void*)_Msg.m_Param1);
			else
				m_StartTime = CMTime::CreateFromTicks(m_pWServer->GetGameTick(),m_pWServer->GetGameTickTime());

			// Refresh time
			ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
			// Run one refresh now incase we've not refreshed on this tick
			UpdateMessages();
		}
		return 1;
	}
	
	return CWObject::OnMessage(_Msg);
}

void CWObject_TimedMessages::UpdateMessages()
{
	CMTime CurTime = CMTime::CreateFromTicks(m_pWServer->GetGameTick(),m_pWServer->GetGameTickTime()) - m_StartTime;
	TAP_RCD<struct CWObject_Attach::CTimedMessage> lMessages = m_lTimedMessages;
	while (lMessages.Len() > m_iCurrentMessage && (CurTime.Compare(lMessages[m_iCurrentMessage].m_Time - 0.001f) > 0))
	{
		CMTime LocalTime = m_StartTime + CMTime::CreateFromSeconds(lMessages[m_iCurrentMessage].m_Time);
		CWObject_Message Msg;
		int iMsg = m_iCurrentMessage;
		m_iCurrentMessage++;
		lMessages[iMsg].CreateMessage(Msg, m_iObject, 0);
		Msg.m_Reason = 'CT';
		Msg.m_Param1 = (aint)(void*)&LocalTime;
		lMessages[iMsg].SendMessage(m_iObject, 0, m_pWServer,&Msg);
	}
	// Stop refreshing
	if (m_iCurrentMessage >= m_lTimedMessages.Len())
	{
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
		if (m_Flags & TIMEDMESSAGES_FLAG_AUTORESET)
		{
			// Reset
			m_iCurrentMessage = 0;
			m_StartTime = CMTime::CreateFromSeconds(0.0f);
		}
	}
}

void CWObject_TimedMessages::OnRefresh()
{
	CWObject::OnRefresh();
	UpdateMessages();
}
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_TimedMessages, CWObject, 0x0100);
