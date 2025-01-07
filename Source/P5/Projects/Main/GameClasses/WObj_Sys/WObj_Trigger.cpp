#include "PCH.h"

#include "WObj_Trigger.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Lights.h"
#include "../WRPG/WRPGCore.h"
#include "../WObj_Char.h"
#include "../WObj_Game/WObj_GameMod.h"
#include "../WObj_Misc/WObj_ActionCutscene.h"
#include "../WObj_AI/AI_Custom/AICore_Darkling.h"


enum
{
	class_CharPlayer =			MHASH6('CWOb','ject','_','Char','Play','er'),
	class_CharNPC =				MHASH5('CWOb','ject','_','Char','NPC'),
};


// -------------------------------------------------------------------
//  CWObject_Trigger_Ext
// -------------------------------------------------------------------
CWObject_Trigger_Ext::CWObject_Trigger_Ext()
{
	m_DelayTicks = 0;
	m_CurDelay = 0;
	m_Flags = 0;
	m_CharFlags = ~0;

	m_IntersectNotifyFlags = OBJECT_FLAGS_CHARACTER;
	m_bTrigging = false;
	m_bRunTriggerOnLeave = false;
	m_bForceRefresh = false;
	m_iLastTriggerer = -1;
	m_iBSPModel = 0;
	m_Hitpoints = 1;
	m_MaxHitpoints = 1;
	m_iMaxDamageType = 0;
	m_MinDamage = -1;
	m_MaxDamage = -1;
	m_SpecialDamage = 0;
	m_SoundLevel = 0;
	m_DamageDelayTicks = 0;

	m_DealDamage = 0;
	m_DealDamageType = 0;

	m_MsgContainer.Register(TRIGGER_MSG_INSTANT,		"MESSAGE_INSTANT");
	m_MsgContainer.Register(TRIGGER_MSG_DELAYED,		"MESSAGE_DELAYED");
	m_MsgContainer.Register(TRIGGER_MSG_ENTER,			"MESSAGE_ENTER");
	m_MsgContainer.Register(TRIGGER_MSG_LEAVE,			"MESSAGE_LEAVE");
	m_MsgContainer.Register(TRIGGER_MSG_LEAVE_DELAYED,	"MESSAGE_LDELAYED");
	m_MsgContainer.Register(TRIGGER_MSG_DAMAGED,		"MESSAGE_DAMAGED");
}

void CWObject_Trigger_Ext::SetStrInData(CStr _St)
{
	_St.MakeUpperCase();
	char *pSt = (char *)(const char *)_St.Str();
	int Len = Min(_St.Len(), CWO_NUMDATA * 4 - 1);
	memcpy(&m_Data[0], pSt, Len);
	GetStrFromData(this)[Len] = 0;
}

char *CWObject_Trigger_Ext::GetStrFromData(CWObject_CoreData *_pObj)
{
	char *pSt = (char *)&_pObj->m_Data[0];
	if(pSt[0] == 0)
		return NULL;
	return pSt;
}
	
void CWObject_Trigger_Ext::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	int KeyValuei = KeyValue.Val_int();
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();

	switch (_KeyHash)
	{
	case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
		{
			CStr ModelName = CStrF("$WORLD:%d", KeyValuei);
			m_iBSPModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
			if(!m_iBSPModel)
				Error("OnEvalKey", "Failed to acquire bsp-model.");
			break;
		}
	
	case MHASH2('MESS','AGE'): // "MESSAGE"
		{
			ConOutL("WARNING: the \"MESSAGE\" key for trigger_ext is deprecated!");
			CWO_SimpleMessage Msg;
			Msg.Parse(KeyValue, m_pWServer);
			m_MsgContainer.AddMessage(TRIGGER_MSG_INSTANT, Msg);
			break;
		}

	case MHASH2('DELA','Y'): // "DELAY"
		{
			m_DelayTicks = (int)(KeyValuef * m_pWServer->GetGameTicksPerSecond());
			if(m_DelayTicks < 0)
				m_DelayTicks = -1;
			break;
		}

	case MHASH3('DAMA','GEDE','LAY'): // "DAMAGEDELAY"
		{
			m_DamageDelayTicks = (int)(KeyValuef * m_pWServer->GetGameTicksPerSecond());
			break;
		}

	case MHASH3('SOUN','DLEV','EL'): // "SOUNDLEVEL"
		{
			m_SoundLevel = KeyValuei;
			break;
		}
	
	case MHASH3('TRIG','GERF','LAGS'): // "TRIGGERFLAGS"
		{
			static const char *TriggerFlagsStr[] = { "Sound", "Visible", "DealDamage", "$$$", "Once", "Random", "Destroyable", "Waitspawn", NULL };
			m_Flags |= KeyValue.TranslateFlags(TriggerFlagsStr);
			break;
		}

	case MHASH3('DEAL','_DAM','AGE'): // "DEAL_DAMAGE"
		{
			m_DealDamage = KeyValuei;
			break;
		}

	case MHASH4('DEAL','_DAM','AGE_','TYPE'): // "DEAL_DAMAGE_TYPE"
		{
			m_DealDamageType = KeyValuei;
			break;
		}

	case MHASH4('MAXD','AMAG','ETYP','E'): // "MAXDAMAGETYPE"
		{
			m_iMaxDamageType = KeyValuei;
			break;
		}
	
	case MHASH5('INTE','RSEC','TION','FLAG','S'): // "INTERSECTIONFLAGS"
		{
			m_IntersectNotifyFlags = KeyValue.TranslateFlags(ms_ObjectFlagsTranslate);
			if(KeyValue.LowerCase().Find("corpse") != -1)
			{
				m_IntersectNotifyFlags |= OBJECT_FLAGS_TRIGGER;
				m_Flags |= TRIGGER_FLAGS_CORPSE;
			}
			break;
		}
	
	case MHASH4('CHAR','ACTE','RTYP','ES'): // "CHARACTERTYPES"
		{
			static const char* lpCharTypes[] = { "Players", "Darklings", "Civilians", "BadGuys", "ToughGuys", "Soldiers", "MeatFaces", NULL };
			m_CharFlags = KeyValue.TranslateFlags(lpCharTypes);
		}
		break;

	case MHASH4('INTE','RSEC','TOBJ','ECTS'): // "INTERSECTOBJECTS"
		{
			CFStr St = KeyValue;
			while(St != "")
			{
				CFStr Object = St.GetStrMSep(";, ");
				Object.Trim();
				CNameHash NameHash( m_pWServer->World_MangleTargetName(Object) );
				m_lIntersectObjects.Add(NameHash);
			}
			break;
		}

	case MHASH4('EXCL','UDED','OBJE','CTS'): // "EXCLUDEDOBJECTS"
		{
			CFStr St = KeyValue;
			while (St != "")
			{
				CFStr ObjName = St.GetStrMSep(";, ");
				ObjName.Trim();
				CNameHash NameHash( m_pWServer->World_MangleTargetName(ObjName) );
				m_lExcludedObjects.Add(NameHash);
			}
			break;
		}

	case MHASH4('USE_','RPGO','BJEC','T'): // "USE_RPGOBJECT"
		{
			m_Use_RPGObject = KeyValue;
			CRPG_Object::IncludeRPGClass(m_Use_RPGObject, m_pWServer->GetMapData(), m_pWServer);
			break;
		}
	
	case MHASH3('HITP','OINT','S'): // "HITPOINTS"
		{
			m_Hitpoints = KeyValuei;
			m_MaxHitpoints = m_Hitpoints;
			break;
		}

	case MHASH3('MIND','AMAG','E'): // "MINDAMAGE"
		{
			m_MinDamage = KeyValuei;
			break;
		}
	
	case MHASH3('MAXD','AMAG','E'): // "MAXDAMAGE"
		{
			m_MaxDamage = KeyValuei;
			break;
		}
	
	case MHASH4('LINK','EDTR','IGGE','RS'): // "LINKEDTRIGGERS"
		{
			CFStr St = KeyValue;
			while(St != "")
			{
				CFStr Object = St.GetStrMSep(";, ");
				Object.Trim();
				m_lLinkedTriggers.Add(m_pWServer->World_MangleTargetName((char *)Object));
			}
			break;
		}

	case MHASH4('AUTO','TARG','ETNA','ME'): // "AUTOTARGETNAME"
		{
			SetStrInData(KeyValue);
			break;
		}

	case MHASH4('SPEC','IALD','AMAG','E'): // "SPECIALDAMAGE"
		{
			m_SpecialDamage = KeyValue.TranslateFlags(CRPG_Object::ms_DamageTypeStr);
			break;
		}

	default:
		{
			if (m_MsgContainer.OnEvalKey(_KeyHash, _pKey, m_pWServer))
				return;

			CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}
	
void CWObject_Trigger_Ext::OnIncludeClass(CMapData *_pWData, CWorld_Server *_pWServer)
{
}
	
void CWObject_Trigger_Ext::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	CWObject_Model::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
	
	if(_pReg)
		CRPG_Object::IncludeRPGClassFromKey("USE_RPGOBJECT", _pReg, _pMapData, _pWServer);
}

void CWObject_Trigger_Ext::UpdateNoRefreshFlag()
{
	if (m_CurDelay != 0 || m_bForceRefresh || 
	   m_lTriggeredDelayedLeaveMessageTimers.Len() > 0)
	{
		if(m_ClientFlags & CWO_CLIENTFLAGS_NOREFRESH)
		{
			m_ClientFlags &= ~CWO_CLIENTFLAGS_NOREFRESH;
			m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);
		}
	}
	else
		m_ClientFlags |= CWO_CLIENTFLAGS_NOREFRESH;
}

void CWObject_Trigger_Ext::Spawn(bool _bSpawn)
{
	if(_bSpawn)
	{
		m_Flags &= ~TRIGGER_FLAGS_WAITSPAWN;
		
		if(m_iBSPModel != 0)
		{
			if(m_Flags & TRIGGER_FLAGS_VISIBLE)
				Model_Set(0, m_iBSPModel, false);

			// Setup physics
			CWO_PhysicsState Phys;
			Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_PHYSMODEL, m_iBSPModel, 0, 0);
			Phys.m_nPrim = 1;

			Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_ROTATION;
			Phys.m_ObjectFlags |= OBJECT_FLAGS_TRIGGER;
			if(m_Flags & TRIGGER_FLAGS_DESTROYABLE)
			{
				Phys.m_ObjectIntersectFlags = m_IntersectNotifyFlags | OBJECT_FLAGS_PROJECTILE;
				m_IntersectNotifyFlags = 0;
			}
			else
				Phys.m_ObjectIntersectFlags = (m_IntersectNotifyFlags & OBJECT_FLAGS_PROJECTILE);
			if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
				ConOutL("§cf80WARNING: Unable to set trigger physics state.");
		}
		else if(GetPhysState().m_nPrim > 0)
		{
			CWO_PhysicsState Phys = GetPhysState();
			Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_ROTATION;
			Phys.m_ObjectFlags |= OBJECT_FLAGS_TRIGGER;
			if(m_Flags & TRIGGER_FLAGS_DESTROYABLE)
				Phys.m_ObjectIntersectFlags = m_IntersectNotifyFlags | OBJECT_FLAGS_PROJECTILE;
			else
				Phys.m_ObjectIntersectFlags = (m_IntersectNotifyFlags & OBJECT_FLAGS_PROJECTILE);
			if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
				ConOutL("§cf80WARNING: Unable to set trigger physics state.");
		}

		{
			// Force check all intersecting objects
			CWO_PhysicsState Phys = GetPhysState();
			Phys.m_ObjectIntersectFlags = m_IntersectNotifyFlags;

			int iObjThis = m_iObject;
			CWorld_Server* pServer = m_pWServer;
			TSelection<CSelection::LARGE_BUFFER> Selection;
			{
				pServer->Selection_AddBoundBox(Selection, m_IntersectNotifyFlags, GetAbsBoundBox()->m_Min, GetAbsBoundBox()->m_Max);
				const int16* pSel = NULL;
				uint nSel = pServer->Selection_Get(Selection, &pSel);
				for (uint i = 0; i < nSel; i++)
				{
					if (TestIntersection(Phys, pSel[i]))
						pServer->Message_SendToObject(CWObject_Message(OBJMSG_TRIGGER_INTERSECTION, pSel[i]), m_iObject);

					if (!pServer->Object_Get(iObjThis))
						return;  // paranoia!
				}
			}
		}

		// trigger collisions are detected when objects move, but we need to detect collisions also when the trigger position is updated
		if (m_IntersectNotifyFlags & (OBJECT_FLAGS_OBJECT | OBJECT_FLAGS_CHARACTER))
			m_pWServer->Object_AddListener(m_iObject, m_iObject, CWO_LISTENER_EVENT_MOVED);
	}
	else
	{
		if(m_iBSPModel != 0 && GetPhysState().m_nPrim == 1)
		{
			CWO_PhysicsState Phys;
			Phys.m_nPrim = 0;
			m_pWServer->Object_SetPhysics(m_iObject, Phys);
		}
		m_Flags |= TRIGGER_FLAGS_WAITSPAWN;
	}

	UpdateNoRefreshFlag();
}

void CWObject_Trigger_Ext::OnFinishEvalKeys()
{
	CWObject_Model::OnFinishEvalKeys();

	int nMsgLeave, nMsgLeaveDelayed, nMsgEnter;
	m_MsgContainer.GetMessages(TRIGGER_MSG_LEAVE, nMsgLeave);
	m_MsgContainer.GetMessages(TRIGGER_MSG_LEAVE_DELAYED, nMsgLeaveDelayed);
	m_MsgContainer.GetMessages(TRIGGER_MSG_ENTER, nMsgEnter);
	if (nMsgLeave || nMsgLeaveDelayed || nMsgEnter)
		m_bRunTriggerOnLeave = true;

	// If we're colliding with player, collide with creepingdark as well (wind)
	/*if (m_IntersectNotifyFlags & OBJECT_FLAGS_PLAYER)
		m_IntersectNotifyFlags |= OBJECT_FLAGS_CREEPING_DARK;*/
}


void CWObject_Trigger_Ext::OnSpawnWorld()
{
	MAUTOSTRIP(CWObject_Trigger_Ext_OnSpawnWorld, MAUTOSTRIP_VOID);
	CWObject_Model::OnSpawnWorld();

	m_MsgContainer.Precache(m_iObject, m_pWServer);

	//NOTE: Spawn() was called here, but has been moved to OnFinishDeltaLoad()
}


int CWObject_Trigger_Ext::ApplyDamage(const CWO_DamageMsg& _Damage, int _iSender)
{
	MAUTOSTRIP(CWObject_Trigger_Ext_ApplyDamage, MAUTOSTRIP_VOID);

	if(m_SpecialDamage != 0 && !(_Damage.m_DamageType & m_SpecialDamage))
		return DAMAGE_RETURN_NORMAL;

//	ConOut("ApplyDamage");

	if (!(m_Flags & TRIGGER_FLAGS_DESTROYABLE) && (GetPhysState().m_ObjectIntersectFlags & OBJECT_FLAGS_PROJECTILE))
	{
		// We're not "destroyable" but has the "projectile" flag set - do a Trigger() and let projectile continue
		Trigger(_iSender);
		UpdateNoRefreshFlag();
		return DAMAGE_RETURN_PASSTHROUGH;
	}

	if ((m_Flags & TRIGGER_FLAGS_DESTROYABLE) && (_Damage.m_Damage > m_MinDamage))
	{
		int32 Damage = _Damage.m_Damage;
		bool bDefaultBehaviour = true;
		if(m_MaxDamage != -1 && Damage > m_MaxDamage)
		{
			switch(m_iMaxDamageType)
			{
			case TRIGGER_MAXDAMAGETYPE_CLIP:
				Damage = MinMT(Damage, m_MaxDamage);
				break;

			case TRIGGER_MAXDAMAGETYPE_SPLIT:
				{
					int nSplit = Damage / m_MaxDamage;
					int DamageFrac = Damage - nSplit * m_MaxDamage;
					nSplit = MinMT(nSplit, m_Hitpoints / m_MaxDamage);
					if(DamageFrac < m_MinDamage)
						Damage -= DamageFrac;
					else
						nSplit++;

					for(int s = 0; s < nSplit; s++)
						m_MsgContainer.SendMessage(TRIGGER_MSG_DAMAGED, m_iObject, _iSender, m_pWServer);

					bDefaultBehaviour = false;
				}
				break;

			case TRIGGER_MAXDAMAGETYPE_TRIGGER:
				if(Damage > m_MaxDamage)
					m_Hitpoints = Damage;
				break;
				
			case TRIGGER_MAXDAMAGETYPE_DESTROY:
				if(Damage > m_MaxDamage)
				{
					m_pWServer->Object_Destroy(m_iObject);
					return DAMAGE_RETURN_NORMAL;
				}
				break;
			}
		}
		
		if((Damage > 0 || m_MaxDamage == 0) && bDefaultBehaviour)
		{
			m_MsgContainer.SendMessage(TRIGGER_MSG_DAMAGED, m_iObject, _iSender, m_pWServer);
		}

		if(m_Hitpoints > 0 && Damage > 0)
		{
			m_Hitpoints -= Damage;
			m_Hitpoints = MaxMT(m_Hitpoints, 0);
			iAnim1() = 255 - 255 * m_Hitpoints / m_MaxHitpoints;
			m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_GENERAL);
		}

		if(m_Hitpoints == 0)
		{
			Trigger(_iSender);
			
			for(int i = 0; i < m_lLinkedTriggers.Len(); i++)
				m_pWServer->Message_SendToTarget(CWObject_Message(OBJMSG_TRIGGER_WASDESTROYED, 0, 0, m_iObject), m_lLinkedTriggers[i]);
			
			if(m_DelayTicks != 0)
			{
				if(m_DelayTicks == -1)
				{
					m_Flags |= TRIGGER_FLAGS_WAITSPAWN;
				}
				else
				{
					m_CurDelay = m_DelayTicks;
					m_Flags |= TRIGGER_FLAGS_ONCE;
				}
				UpdateNoRefreshFlag();
			}
			else
				m_pWServer->Object_Destroy(m_iObject);
		}
		else
			m_CurDelay = m_DamageDelayTicks;
		UpdateNoRefreshFlag();
	}
	return DAMAGE_RETURN_NORMAL;
}


// returns index to m_lCurIntersectingObjects
int CWObject_Trigger_Ext::FindIntersectingObject(uint _iObject) const
{
	TAP<const CIntersectingObject> pCurrIntersecting = m_lCurIntersectingObjects;
	for (int i = 0; i < pCurrIntersecting.Len(); i++)
	{
		if (pCurrIntersecting[i].m_iObject == _iObject)
			return i;
	}
	return -1;
}


void CWObject_Trigger_Ext::AddIntersectingObject(uint _iObject)
{
	int i = FindIntersectingObject(_iObject);
	if (i >= 0)
	{
		// Already in list - update "last active" tick
		m_lCurIntersectingObjects[i].m_LastTick = m_pWServer->GetGameTick();
		return;
	}

	// Add to list of objects currently inside the trigger
	CIntersectingObject Object;
	Object.m_iObject = _iObject;
	Object.m_LastTick = m_pWServer->GetGameTick();
	m_lCurIntersectingObjects.Add(Object);

	// Listen to events for the object
	m_pWServer->Object_AddListener(_iObject, m_iObject);

	TriggerOnEnter(_iObject);
}


bool CWObject_Trigger_Ext::TestIntersection(const CWO_PhysicsState& _This, uint _iObject)
{
	CWObject* pObj = m_pWServer->Object_Get(_iObject);
	if (!pObj)
		return false;

	const CWO_PhysicsState* pPhys2 = &pObj->GetPhysState();
	CWO_PhysicsState Tmp;
	if ((pPhys2->m_ObjectFlags & m_IntersectNotifyFlags) & OBJECT_FLAGS_OBJECT)
	{
		// Physics objects needs to be handled separately (as a box)
		Tmp = *pPhys2;
		CBox3Dfp32 BBox;
		pObj->GetVisBoundBox(BBox);
		CVec3Dfp32 Offset; BBox.GetCenter(Offset);
		CVec3Dfp32 Size = (BBox.m_Max - BBox.m_Min) * 0.5f;
		Tmp.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, Size, Offset);
		Tmp.m_nPrim = 1;
		Tmp.m_ObjectIntersectFlags = OBJECT_FLAGS_TRIGGER;
		Tmp.m_ObjectFlags = OBJECT_FLAGS_OBJECT;
		pPhys2 = &Tmp;
	}

	bint bCollide = m_pWServer->Phys_IntersectStates(
			_This, *pPhys2, GetPositionMatrix(), GetPositionMatrix(), 
			pObj->GetPositionMatrix(), pObj->GetPositionMatrix(), NULL, 0, 0);

	return bCollide ? true : false;
}


uint CWObject_Trigger_Ext::UpdateIntersectingObjects()
{
	TAP<CIntersectingObject> pObjects = m_lCurIntersectingObjects;
	int GameTick = m_pWServer->GetGameTick();
	CWO_PhysicsState Phys = GetPhysState();
	Phys.m_ObjectIntersectFlags = m_IntersectNotifyFlags;

	for (int i = 0; i < pObjects.Len(); i++)
	{
		if (pObjects[i].m_LastTick + 2 < GameTick)
		{
			// Check collision to see if object has left the trigger
			uint iObject = pObjects[i].m_iObject;
			bool bCollide = TestIntersection(Phys, iObject);
			if (bCollide)
			{ // is colliding, update "last active" tick
				pObjects[i].m_LastTick = GameTick + 1;
			}
			else
			{ // need to refresh some more to let the timer time out
				m_bForceRefresh = 1; 
				UpdateNoRefreshFlag();
			}

//M_TRACE("Collision for object %d (%s): [%s]\n", iObject, m_pWServer->Object_GetName(iObject), bCollide ? "yes" : "no");

			if (pObjects[i].m_LastTick + 2 < GameTick)
			{
				// No longer intersecting. Remove from list and return
				m_lCurIntersectingObjects.Del(i);
				m_pWServer->Object_RemoveListener(iObject, m_iObject);
				return iObject;
			}
		}
	}
	return 0;
}


aint CWObject_Trigger_Ext::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Trigger_Ext_OnMessage, MAUTOSTRIP_VOID);
	switch(_Msg.m_Msg)
	{
	case OBJSYSMSG_NOTIFY_INTERSECTION : 
	case OBJMSG_TRIGGER_INTERSECTION :
		{
			//ConOutL(CStrF("%i. Intersection: Object: %i  Sender: %i", m_pWServer->GetGameTick(), m_iObject, _Msg.m_iSender));
			if(!m_bTrigging && !(m_Flags & TRIGGER_FLAGS_WAITSPAWN) && !IsDestroyed())
			{
				int iObject = _Msg.m_Param0;
				CWObject *pObj = m_pWServer->Object_Get(iObject);
				if(!pObj)
					return 0;

				if (m_Flags & TRIGGER_FLAGS_CORPSE)
				{
					if(pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_TRIGGER)
					{
						// Characters with trigger-flag are assumed dead.
						if (!pObj->IsClass(class_CharPlayer) && !pObj->IsClass(class_CharNPC))
							return 0;
					}
				}
				if (m_IntersectNotifyFlags & OBJECT_FLAGS_PICKUP && !(m_IntersectNotifyFlags & OBJECT_FLAGS_CHARACTER))
				{
					// Characters will have pickupflag set if they are going into ragdoll mode.
					// That should probably change, but till then this special code is needed
					if (pObj->IsClass(class_CharPlayer) || pObj->IsClass(class_CharNPC))
						return 0;
				}
				
				if (m_IntersectNotifyFlags & (OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PLAYER))
				{
					if (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_NEVERTRIGGER, 2), iObject) == 1)
						return 0;

					// check what kind of character pObj is, and filter through m_CharFlags
					if (pObj->IsClass(class_CharNPC))
					{
						const CAI_Core* pAI = safe_cast<CWObject_Character>(pObj)->m_spAI;
						uint CharType = 0;
						switch (pAI->m_CharacterClass)
						{
						case CAI_Core::CLASS_CIV:		CharType = TRIGGER_CHARFLAGS_CIVILIAN; break;
						case CAI_Core::CLASS_TOUGHGUY:	CharType = TRIGGER_CHARFLAGS_TOUGHGUY; break;
						case CAI_Core::CLASS_BADGUY:	CharType = TRIGGER_CHARFLAGS_BADGUY; break;
						case CAI_Core::CLASS_UNDEAD:	CharType = TRIGGER_CHARFLAGS_SOLDIER; break;

						case CAI_Core::CLASS_DARKLING:	
							CharType = safe_cast<const CAI_Core_Darkling>(pAI)->m_bMeatFace ? TRIGGER_CHARFLAGS_MEATFACE : TRIGGER_CHARFLAGS_DARKLING;
							break;
						}

						if (CharType && !(m_CharFlags & CharType))
							return 0;
					}
					else if (pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
					{
						if (!(m_CharFlags & TRIGGER_CHARFLAGS_PLAYER))
							return 0;
					}
				}

				bool bTrigger = true;
				TAP<const CNameHash> pNameList = m_lIntersectObjects;
				TAP<const CNameHash> pExcluded = m_lExcludedObjects;
				if (pNameList.Len() > 0)
				{ // Name check is required
					bTrigger = false;
					uint32 ObjNameHash = m_pWServer->Object_GetNameHash(iObject);
					if (ObjNameHash)
					{
						for (uint i = 0; i < pNameList.Len(); i++)
						{
							if (pNameList[i] == ObjNameHash)
							{
								bTrigger = true;
								break;
							}
						}
					}
				}
				else if (pExcluded.Len() > 0)
				{ // Check if object name is on the exclude list
					uint32 ObjNameHash = m_pWServer->Object_GetNameHash(iObject);
					if (ObjNameHash)
					{
						for (uint i = 0; i < pExcluded.Len(); i++)
						{
							if (pExcluded[i] == ObjNameHash)
							{
								bTrigger = false;
								break;
							}
						}
					}
				}

				if (bTrigger)
				{
					if (m_bRunTriggerOnLeave)
						AddIntersectingObject(iObject);

					Trigger(iObject);
				}
			}
			return 0;
		}

	case OBJMSG_DAMAGE:
		{
			//ConOut(CStrF("Damage. Tick: %i   CurDelay: %i", m_pWServer->GetGameTick(), m_CurDelay));
			if(!m_bTrigging && !(m_Flags & TRIGGER_FLAGS_WAITSPAWN) && m_CurDelay == 0)
			{
				const CWO_DamageMsg *pMsg = CWO_DamageMsg::GetSafe(_Msg);
				if(pMsg)
				{
					bool bOld = m_bTrigging;
					m_bTrigging = true;
					int Ret = ApplyDamage(*pMsg, _Msg.m_iSender);
					m_bTrigging = bOld;
					return Ret;
				}
			}
			return 0;
		}

	case OBJMSG_RADIALSHOCKWAVE:
		{
			if(!m_bTrigging && m_Flags & TRIGGER_FLAGS_DESTROYABLE && !(m_Flags & TRIGGER_FLAGS_WAITSPAWN) && m_CurDelay == 0)
			{
				const CWO_ShockwaveMsg *pMsg = CWO_ShockwaveMsg::GetSafe(_Msg);
				if(pMsg)
				{	
					CVec3Dfp32 Dim = GetPhysState().m_Prim[0].GetDim();
					fp32 h = Dim.k[2] * 2.0f;
//					fp32 r = Length2(Dim.k[0], Dim.k[1]);
					fp32 r = Min(Dim.k[0], Dim.k[1]);
					CVec3Dfp32 TracePos = GetPosition();
					
					CWO_DamageMsg Msg;
					if(pMsg->GetTracedDamage(m_iObject, TracePos, r, h, m_pWServer, Msg))
					{
						bool bOld = m_bTrigging;
						m_bTrigging = true;
						ApplyDamage(Msg, _Msg.m_iSender);
						m_bTrigging = bOld;
						return 1;
					}
				}
			}
			return 0;
		}
		
	case OBJMSG_TRIGGER_WASDESTROYED:
		if(m_Flags & TRIGGER_FLAGS_DESTROYABLE)
			m_pWServer->Object_Destroy(m_iObject);
		return 1;

	case OBJMSG_IMPULSE:
		{
			if(!m_bTrigging)
				Trigger(_Msg.m_iSender);
			return 1;
		}
	case OBJMSG_GAME_SPAWN:
		{
			if(_Msg.m_Param0 <= 0)
				Spawn(true);
			else
				Spawn(false);
		}
		return 1;
	case OBJMSG_GAME_ALLOWAUTOTARGET:
		{
			// Accepting HUD-data
			if((m_Flags & TRIGGER_FLAGS_DESTROYABLE) && !(m_Flags & TRIGGER_FLAGS_WAITSPAWN) && GetStrFromData(this))
				return 1;

			return 0;
		}
	case OBJMSG_TRIGGER_CONFIGURE:
		{
			// What we need to configure is flags and messages
			// Param0 == flags, param1 number of normal messages
			// Physbox size from vecparam0/1 in the message
			// delaytime the m_isender
			// m_pData is array of messages, 
			m_Flags = _Msg.m_Param0;
			m_DelayTicks = _Msg.m_iSender;
			int NumEnter = _Msg.m_Param1;
			int NumLeave = (_Msg.m_DataSize - _Msg.m_Param1 * sizeof(CWO_SimpleMessage)) / sizeof(CWO_SimpleMessage);
			CWO_SimpleMessage* pMessages = (CWO_SimpleMessage*)_Msg.m_pData;
			if (pMessages)
			{
				for (int i = 0; i < NumEnter; i++)
					m_MsgContainer.AddMessage(TRIGGER_MSG_INSTANT, pMessages[i]);
				if (NumLeave > 0)
					m_bRunTriggerOnLeave = true;
				for (int i = NumEnter; i < (NumEnter+NumLeave); i++)
					m_MsgContainer.AddMessage(TRIGGER_MSG_LEAVE, pMessages[i]);
			}
			CWO_PhysicsState Phys = GetPhysState();
			Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_ROTATION;
			Phys.m_ObjectFlags |= OBJECT_FLAGS_TRIGGER;
			CVec3Dfp32 Size = (_Msg.m_VecParam1 - _Msg.m_VecParam0);

			Phys.m_nPrim = 1;
			Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, 
				CVec3Dfp32(M_Fabs(Size.k[0]*0.5f), M_Fabs(Size.k[1]*0.5f), M_Fabs(Size.k[2]*0.5f)),0.0f);
			if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
				ConOutL("§cf80WARNING: Unable to set trigger physics state.");

			return 1;
		}
#ifndef M_RTM
	case OBJSYSMSG_GETDEBUGSTRING:
		if(_Msg.m_DataSize == sizeof(CStr *))
		{
			CStr *pSt = (CStr *)_Msg.m_pData;
			CWObject_Model::OnMessage(_Msg);
			if(m_Flags & TRIGGER_FLAGS_WAITSPAWN)
				*pSt += "Waiting for spawn";
			return 1;
		}
#endif

	case OBJSYSMSG_LISTENER_EVENT:
		{
			if (_Msg.m_Param0 == CWO_LISTENER_EVENT_DELETED)
			{
				int i = FindIntersectingObject(_Msg.m_iSender);
				if (i >= 0)
					m_lCurIntersectingObjects.Del(i);
				return 1;
			}
			else if (_Msg.m_Param0 == CWO_LISTENER_EVENT_MOVED)
			{
				if (_Msg.m_iSender != m_iObject)
				{
					// an object inside the trigger moved - turn on refresh
					m_bForceRefresh = 1;
					UpdateNoRefreshFlag();
				}
				else
				{
					// the trigger itself moved. check intersection with phys objects & characters
					const CBox3Dfp32& BBox = *GetAbsBoundBox();
					TSelection<CSelection::MEDIUM_BUFFER> Sel;
					m_pWServer->Selection_AddBoundBox(Sel, m_IntersectNotifyFlags, BBox.m_Min, BBox.m_Max);
					if (Sel.Len() > 0)
					{
						CWO_PhysicsState Phys = GetPhysState();
						Phys.m_ObjectIntersectFlags = m_IntersectNotifyFlags;
						CWObject_Message Msg2(OBJMSG_TRIGGER_INTERSECTION);

						for (uint i = 0; i < Sel.Len(); i++)
						{
							int iObject = Sel[i];
							if (TestIntersection(Phys, iObject))
							{
								Msg2.m_Param0 = iObject;
								OnMessage(Msg2);
							}
						}
					}
				}
				return 1;
			}
			break;
		}

	case OBJMSG_TRIGGER_GET_INTERSECT_NOTIFYFLAGS:
		return m_IntersectNotifyFlags;
	case OBJMSG_TRIGGER_SET_INTERSECT_NOTIFYFLAGS:
		m_IntersectNotifyFlags = _Msg.m_Param0;
		break;
	}
	return CWObject_Model::OnMessage(_Msg);
}
	
aint CWObject_Trigger_Ext::OnClientMessage(CWObject_Client *_pObj, CWorld_Client *_pWClient, const CWObject_Message &_Msg)
{
	MAUTOSTRIP(CWObject_Trigger_Ext_OnClientMessage, 0);

	/*
	if(_Msg.m_Msg == OBJMSG_GAME_GETTARGETHUDDATA)
	{
		CTargetHudData* pTargetHudData = (CTargetHudData*)_Msg.m_Param0;
		if (!pTargetHudData)
			return 0;
		
		pTargetHudData->m_pName = GetStrFromData(_pObj);
		pTargetHudData->m_Health = 255 - _pObj->m_iAnim1;

		if(!pTargetHudData->m_pName)
			return 0;

		return 1;
	}*/
	return 0;
}

void CWObject_Trigger_Ext::SendUseNotification(int _iPlayer)
{
	MAUTOSTRIP(CWObject_Trigger_Ext_SendUseNotification, MAUTOSTRIP_VOID);

	CWObject_Message Msg(OBJMSG_CHAR_USEINFORMATION, 0);
	Msg.m_pData = (char *)m_Use_RPGObject;
	Msg.m_DataSize = m_Use_RPGObject.Len() + 1;
	Msg.m_iSender = m_iObject;
	
	m_pWServer->Message_SendToObject(Msg, _iPlayer);
}

void CWObject_Trigger_Ext::SendRandomMessage(int _Event, int _iSender)
{
	int nMsg = 0;
	const CWO_SimpleMessage* plMsg = m_MsgContainer.GetMessages(_Event, nMsg);
	if (nMsg)
		plMsg[MRTC_RAND() % nMsg].SendMessage(m_iObject, _iSender, m_pWServer);
}


void CWObject_Trigger_Ext::UpdateMessageQueue(TThinArray<CTimedEntry>& _List, uint _Event)
{
	TAP<CTimedEntry> pList = _List;
	if (pList.Len() == 0)
		return;

	// Check delayed messages
	// - decrease all timers
	// - if reached zero, send messages and remove timer
	uint nToKeep = 0;
	for (uint i = 0; i < pList.Len(); i++)
	{
		CTimedEntry E = pList[i];
		if (E.m_nTicksLeft < 1)
		{
			// Timer reached zero - send messages
//M_TRACE("Sending delayed message (type: %d), iActivator: %d (%s)\n", _Event, E.m_iActivator, m_pWServer->Object_GetName(E.m_iActivator));
			m_MsgContainer.SendMessage(_Event, m_iObject, E.m_iActivator, m_pWServer);
		}
		else
		{
			// Store updated entry
			E.m_nTicksLeft--;
//M_TRACE("(%d - %d (%s)\n", E.m_nTicksLeft, E.m_iActivator, m_pWServer->Object_GetName(E.m_iActivator));
			pList[nToKeep++] = E;
		}
	}
	_List.SetLen(nToKeep);
	UpdateNoRefreshFlag();
}


void CWObject_Trigger_Ext::OnRefresh()
{
	MAUTOSTRIP(CWObject_Trigger_Ext_OnRefresh, MAUTOSTRIP_VOID);
	m_bForceRefresh = 0;

//M_TRACE("[Trigger refresh: %d (%s)\n", m_iObject, GetName());

	if (!(m_Flags & TRIGGER_FLAGS_WAITSPAWN) && !IsDestroyed())
	{
//		UpdateMessageQueue(m_lTriggeredDelayedEnterMessageTimers, TRIGGER_MSG_ENTER_DELAYED);
		UpdateMessageQueue(m_lTriggeredDelayedLeaveMessageTimers, TRIGGER_MSG_LEAVE_DELAYED);

		bool bDelayDone = (m_CurDelay <= 1);
		if (m_CurDelay > 0)
		{
			m_CurDelay--;
			if (m_CurDelay == 0)
			{
				bool bOld = m_bTrigging;
				m_bTrigging = true;
//				if(m_Flags & TRIGGER_FLAGS_IMPULSE_DELAYED)
//					SendImpulseToTargets(m_Impulse_Delayed, m_iLastTriggerer);

				if (m_Flags & TRIGGER_FLAGS_RANDOM)
					SendRandomMessage(TRIGGER_MSG_DELAYED, m_iLastTriggerer);
				else
					m_MsgContainer.SendMessage(TRIGGER_MSG_DELAYED, m_iObject, m_iLastTriggerer, m_pWServer);

				m_bTrigging = bOld;
				
				UpdateNoRefreshFlag();
				
				if(m_Flags & TRIGGER_FLAGS_ONCE)
					m_pWServer->Object_Destroy(m_iObject);
			}
		}

		if (bDelayDone && m_bRunTriggerOnLeave)
		{
			for (;;)
			{
				uint iSender = UpdateIntersectingObjects();
				if (!iSender)
					break;
				TriggerOnLeave(iSender);
			} 
		}
	}
}

void CWObject_Trigger_Ext::Trigger(int _iSender)
{
	MAUTOSTRIP(CWObject_Trigger_Ext_Trigger, MAUTOSTRIP_VOID);

	if(m_CurDelay == 0 && !(m_Flags & TRIGGER_FLAGS_WAITSPAWN) && !IsDestroyed())
	{
//		if(m_Flags & TRIGGER_FLAGS_IMPULSE)
//			SendImpulseToTargets(m_Impulse, _iSender);
//		ConOut("Trigger");

		if(m_Flags & TRIGGER_FLAGS_SOUND)
		{
			fp32 Noise;
			CWObject_Message Msg(OBJMSG_CHAR_NOISE);
			Msg.m_pData = &Noise;
			
			if(!m_pWServer->Message_SendToObject(Msg, _iSender))
				return;

			if(Noise < m_SoundLevel)
				return;
		}

		if(m_Flags & TRIGGER_FLAGS_DEALDAMAGE)
		{
			CWO_DamageMsg DmgMsg(m_DealDamage, m_DealDamageType);
			DmgMsg.Send(_iSender, m_iObject, m_pWServer);
		}

		bool bOld = m_bTrigging;
		m_bTrigging = true;

		if(!m_DelayTicks && (m_Flags & TRIGGER_FLAGS_ONCE))
			m_pWServer->Object_Destroy(m_iObject, true);

		if (m_Flags & TRIGGER_FLAGS_RANDOM)
			SendRandomMessage(TRIGGER_MSG_INSTANT, _iSender);
		else
			m_MsgContainer.SendMessage(TRIGGER_MSG_INSTANT, m_iObject, _iSender, m_pWServer);

		m_bTrigging = bOld;
		
		m_iLastTriggerer = _iSender;
		m_CurDelay = m_DelayTicks;
		UpdateNoRefreshFlag();
		
		m_CreationGameTick = m_pWServer->GetGameTick();
		m_CreationGameTickFraction = 0.0f;
	}
}

void CWObject_Trigger_Ext::TriggerOnLeave(int _iSender)
{
//M_TRACE("Sending OnLeave, iActivator: %d (%s)\n", _iSender, m_pWServer->Object_GetName(_iSender));

	//ConOut(CStrF("%i. Trigger on leave", m_pWServer->GetGameTick()));
	m_MsgContainer.SendMessage(TRIGGER_MSG_LEAVE, m_iObject, _iSender, m_pWServer);

	if (m_DelayTicks != 0)
	{
		// Add to queue for delayed leave messages
		int OldLen = m_lTriggeredDelayedLeaveMessageTimers.Len();
		m_lTriggeredDelayedLeaveMessageTimers.SetLen(OldLen+1);
		CTimedEntry& E = m_lTriggeredDelayedLeaveMessageTimers[OldLen];
		E.m_iActivator = _iSender;
		E.m_nTicksLeft = m_DelayTicks;
		UpdateNoRefreshFlag();
	}
}

void CWObject_Trigger_Ext::TriggerOnEnter(int _iSender)
{
//M_TRACE("Sending OnEnter, iActivator: %d (%s)\n", _iSender, m_pWServer->Object_GetName(_iSender));
	m_MsgContainer.SendMessage(TRIGGER_MSG_ENTER, m_iObject, _iSender, m_pWServer);

/*	if (m_DelayTicks != 0)
	{
		// Add to queue for delayed enter messages
		int OldLen = m_lTriggeredDelayedEnterMessageTimers.Len();
		m_lTriggeredDelayedEnterMessageTimers.SetLen(OldLen+1);
		CTimedEntry& E = m_lTriggeredDelayedEnterMessageTimers[OldLen];
		E.m_iActivator = _iSender;
		E.m_nTicksLeft = m_DelayTicks;
		UpdateNoRefreshFlag();
	}*/
}


void CWObject_Trigger_Ext::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	uint8 nCurTriggering;
	_pFile->ReadLE(nCurTriggering);
	m_lCurIntersectingObjects.SetLen(nCurTriggering);
	int i;
	for(i = 0; i < nCurTriggering; i++)
	{
		_pFile->ReadLE(m_lCurIntersectingObjects[i].m_iObject);
		_pFile->ReadLE(m_lCurIntersectingObjects[i].m_LastTick);
		uint iObj = m_lCurIntersectingObjects[i].m_iObject;
		m_pWServer->Object_AddListener(iObj, m_iObject);
	}

	int8 nTDLMT;
	_pFile->ReadLE(nTDLMT);
	m_lTriggeredDelayedLeaveMessageTimers.SetLen(nTDLMT);
	TAP<CTimedEntry> pList = m_lTriggeredDelayedLeaveMessageTimers;
	for(i = 0; i < nTDLMT; i++)
	{
		_pFile->ReadLE(pList[i].m_iActivator);
		_pFile->ReadLE(pList[i].m_nTicksLeft);
	}

	_pFile->ReadLE(m_Hitpoints);
	_pFile->ReadLE(m_iLastTriggerer);
	_pFile->ReadLE(m_Flags);
	_pFile->ReadLE(m_CurDelay);
	_pFile->ReadLE(m_ClientFlags);

	_pFile->ReadLE(m_DealDamage);
	_pFile->ReadLE(m_DealDamageType);
}

void CWObject_Trigger_Ext::OnDeltaSave(CCFile* _pFile)
{
	uint8 nCurTriggering = m_lCurIntersectingObjects.Len();
	_pFile->WriteLE(nCurTriggering);
	int i;
	for(i = 0; i < nCurTriggering; i++)
	{
		_pFile->WriteLE(m_lCurIntersectingObjects[i].m_iObject);
		_pFile->WriteLE(m_lCurIntersectingObjects[i].m_LastTick);
	}

	int8 nTDLMT = m_lTriggeredDelayedLeaveMessageTimers.Len();
	_pFile->WriteLE(nTDLMT);
	TAP<const CTimedEntry> pList = m_lTriggeredDelayedLeaveMessageTimers;
	for(i = 0; i < nTDLMT; i++)
	{
		_pFile->WriteLE(pList[i].m_iActivator);
		_pFile->WriteLE(pList[i].m_nTicksLeft);
	}

	_pFile->WriteLE(m_Hitpoints);
	_pFile->WriteLE(m_iLastTriggerer);
	_pFile->WriteLE(m_Flags);
	_pFile->WriteLE(m_CurDelay);
	_pFile->WriteLE(m_ClientFlags);

	_pFile->WriteLE(m_DealDamage);
	_pFile->WriteLE(m_DealDamageType);
}


void CWObject_Trigger_Ext::OnFinishDeltaLoad()
{
	CWObject_Model::OnFinishDeltaLoad();
	Spawn(!(m_Flags & TRIGGER_FLAGS_WAITSPAWN));
}


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_Ext, CWObject_Model, 0x0100);

#if !defined(M_DISABLE_TODELETE) || 1

// -------------------------------------------------------------------
// Trigger_SoftSpot
// -------------------------------------------------------------------
CWObject_Trigger_SoftSpot::CWObject_Trigger_SoftSpot()
{
	m_bRunTriggerOnLeave = true;
}

void CWObject_Trigger_SoftSpot::TriggerOnEnter(int _iSender)
{
	CWObject_Message Msg(OBJMSG_CHAR_ENTERSOFTSPOT);
	m_pWServer->Message_SendToObject(Msg, _iSender);
}

void CWObject_Trigger_SoftSpot::TriggerOnLeave(int _iSender)
{
	CWObject_Message Msg(OBJMSG_CHAR_LEAVESOFTSPOT);
	m_pWServer->Message_SendToObject(Msg, _iSender);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_SoftSpot, CWObject_Trigger_Ext, 0x0100);


// -------------------------------------------------------------------
// Trigger_SneakZone
// -------------------------------------------------------------------
CWObject_Trigger_SneakZone::CWObject_Trigger_SneakZone()
{
	m_IntersectNotifyFlags = OBJECT_FLAGS_PLAYER;
	m_bOccupied = false;

	m_bRunTriggerOnLeave = true;
}

void CWObject_Trigger_SneakZone::Trigger(int _iSender)
{
	CWObject *pObj = m_pWServer->Object_Get(_iSender);
	if (!m_bOccupied && pObj && CWObject_Character::Char_GetPhysType(pObj) == PLAYER_PHYS_CROUCH)
	{
		CWObject_Message Msg(OBJMSG_CHAR_ENTERSNEAK);
		m_pWServer->Message_SendToObject(Msg, _iSender);
		m_bOccupied = true;
	}
}

void CWObject_Trigger_SneakZone::TriggerOnLeave(int _iSender)
{
	if (!m_bOccupied)
	{
		ConOutL(CStr("TriggerOnLeave called on an unoccupied sneaktrigger"));
		return;
	}

	m_bOccupied = false;
	CWObject_Message Msg(OBJMSG_CHAR_LEAVESNEAK);
	m_pWServer->Message_SendToObject(Msg, _iSender);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_SneakZone, CWObject_Trigger_Ext, 0x0100);

#endif



// -------------------------------------------------------------------
// Trigger_RestrictedZone
// -------------------------------------------------------------------
CWObject_Trigger_RestrictedZone::CWObject_Trigger_RestrictedZone()
: CWObject_Trigger_Ext()
{
	m_IntersectNotifyFlags = OBJECT_FLAGS_PLAYER;

	m_ClearanceLevel = 10;
	m_bRunTriggerOnLeave = true;
}

void CWObject_Trigger_RestrictedZone::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	switch (_KeyHash)
	{
	case MHASH4('CLEA','RANC','ELEV','EL'): // "CLEARANCELEVEL"
		{
			m_ClearanceLevel = _pKey->GetThisValuei();
			break;
		}
	default:
		{
			CWObject_Trigger_Ext::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Trigger_RestrictedZone::Trigger(int _iSender)
{
	CWObject_Message Msg;
  	Msg.m_Msg = OBJMSG_CHAR_RAISEREQUIREDCLEARANCELEVEL;
	Msg.m_Param0 = m_ClearanceLevel;
	m_pWServer->Message_SendToObject(Msg, _iSender);
}

void CWObject_Trigger_RestrictedZone::TriggerOnLeave(int _iSender)
{
	CWObject_Message Msg;
	Msg.m_Msg = OBJMSG_CHAR_CLEARREQUIREDCLEARANCELEVEL;
	m_pWServer->Message_SendToObject(Msg, _iSender);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_RestrictedZone, CWObject_Engine_Path, 0x0100);


#ifndef M_DISABLE_TODELETE
// -------------------------------------------------------------------
// Trigger_Message
// -------------------------------------------------------------------
CWObject_Trigger_Message::CWObject_Trigger_Message()
{
	MAUTOSTRIP(CWObject_Trigger_Message_ctor, MAUTOSTRIP_VOID);
	m_Timer = 0;
	m_Wait = 0;
}

void CWObject_Trigger_Message::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Trigger_Message_OnEvalKey, MAUTOSTRIP_VOID);
	
	const CStr KeyName = _pKey->GetThisName();
	
	switch (_KeyHash)
	{
	case MHASH2('MESS','AGE'): // "MESSAGE"
		{
			m_Msg = _pKey->GetThisValue();
			break;
		}
	
	case MHASH1('WAIT'): // "WAIT"
		{
			m_Wait = _pKey->GetThisValuef() * SERVER_TICKSPERSECOND;
			break;
		}
	
	default:
		{
			CWObject_Trigger::OnEvalKey(_pKey);
			break;
		}
	}
}

int CWObject_Trigger_Message::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Trigger_Message_OnMessage, 0);
	if(_Msg.m_Msg == OBJSYSMSG_NOTIFY_INTERSECTION)
	{
		if(m_Timer > 0)
			m_Timer--;
		else
		{
			if(m_Msg != "")
				ConOut(m_Msg);
			Trigger(_Msg.m_iSender);
			if(m_Wait != 0)
				m_Timer = m_Wait;
			else
				m_pWServer->Object_Destroy(m_iObject);
		}
		return 0;
	}
	
	else
		return CWObject_Trigger::OnMessage(_Msg);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_Message, CWObject_Trigger, 0x0100);

#endif


// -------------------------------------------------------------------
//  TRIGGER_DAMAGE
// -------------------------------------------------------------------
CWObject_Trigger_Damage::CWObject_Trigger_Damage()
{
	m_Damage = 20;
	//	m_FramesPerHit = 20/2;
	m_FramesPerHit = 1;
	m_DamageType = DAMAGETYPE_UNDEFINED;
	m_bActive = false;
	m_LastFrameSent = 0;

	m_TriggerObjFlags = OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PICKUP;
	m_IntersectNotifyFlags = m_TriggerObjFlags;
}


void CWObject_Trigger_Damage::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();
	if(_KeyHash == MHASH2('DAMA','GE'))
	{
		m_Damage = _Value.Val_int();
		return;
	}
	else if(_KeyHash == MHASH3('DAMA','GETY','PE'))
	{
		m_DamageType = (uint32)_Value.Val_int();
		return;
	}
	else if(_KeyHash == MHASH4('HITS','PERS','ECON','D'))
	{
		if(_Value.Val_int() == -1)
			m_FramesPerHit = -1;
		else
			m_FramesPerHit = (int16)TruncToInt(m_pWServer->GetGameTicksPerSecond() / (fp32)_Value.Val_int());
		return;
	}
	CWObject_Trigger::OnEvalKey(_KeyHash, _pKey);
}


aint CWObject_Trigger_Damage::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJSYSMSG_NOTIFY_INTERSECTION : 
	case OBJMSG_TRIGGER_INTERSECTION :
		{
			int Tick = m_pWServer->GetGameTick();
			if (m_FramesPerHit == -1 || (Tick != m_LastFrameSent && (Tick % m_FramesPerHit) == 0))
			{
				CWO_DamageMsg Msg(m_FramesPerHit == -1 ? 1000000 : m_Damage * TruncToInt( m_FramesPerHit / m_pWServer->GetGameTicksPerSecond()), m_DamageType);
				Msg.Send(_Msg.m_Param0, m_iObject, m_pWServer);
				m_LastFrameSent = Tick;
			}
		}
		return 0;

	default:
		return CWObject::OnMessage(_Msg);
	}
}


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_Damage, CWObject_Trigger, 0x0100);


// -------------------------------------------------------------------
//  Trigger_WaterJump
// -------------------------------------------------------------------
/*class CWObject_Trigger_WaterJump : public CWObject_Trigger_Ext
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:	
	CVec3Dfp32 m_Velocity;

	CWObject_Trigger_WaterJump()
	{
		m_Velocity = CVec3Dfp32(10, 0, 20);
		m_IntersectNotifyFlags = OBJECT_FLAGS_USEABLE;
	}

	virtual void Trigger(int _iSender)
	{
		CWObject *pObj = m_pWServer->Object_Get(_iSender);
		if(pObj)
		{
			CMat4Dfp32 Mat = pObj->GetPositionMatrix();
			CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 2);
			Mat.RecreateMatrix(2, 0);
			CVec3Dfp32 Vel = m_Velocity;
			Vel.MultiplyMatrix3x3(Mat);
			m_pWServer->Object_SetVelocity(_iSender, Vel);
			CWObject_Character *pChar = safe_cast<CWObject_Character >(pObj);
		}
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_WaterJump, CWObject_Trigger_Ext, 0x0100);*/

// -------------------------------------------------------------------
//  Trigger_Checkpoint
// -------------------------------------------------------------------
class CWObject_Trigger_Checkpoint : public CWObject_Trigger_Ext
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	bool m_bIgnoreTension;
	CStr m_DeathSeqPast;
	CStr m_DeathSeqPresent;
	CStr m_DeathSeqFuture;
	CStr m_DeathSeqSound;

public:	
	CWObject_Trigger_Checkpoint()
	{
		m_IntersectNotifyFlags = OBJECT_FLAGS_PLAYER;
		m_Flags |= TRIGGER_FLAGS_ONCE;
		m_bIgnoreTension = false;
	}

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		CStr KeyValue = _pKey->GetThisValue();

		switch (_KeyHash)
		{
		case MHASH4('IGNO','RE_T','ENS','ION'):
			{
				m_bIgnoreTension = true;
				return;
			}
		case MHASH2('DS_P','AST'): // "DS_PAST"
			{
				m_DeathSeqPast = KeyValue;
				return;
			}
		case MHASH3('DS_P','RESE','NT'): // "DS_PRESENT"
			{
				m_DeathSeqPresent = KeyValue;
				return;
			}
		case MHASH3('DS_F','UTUR','E'): // "DS_FUTURE"
			{
				m_DeathSeqFuture = KeyValue;
				return;
			}
		case MHASH2('DS_S','OUND'):
			{
				m_DeathSeqSound = KeyValue;
				return;
			}
		}
		CWObject_Trigger_Ext::OnEvalKey(_KeyHash, _pKey);
	}

	virtual void Trigger(int _iSender)
	{
		CWObject_Message Msg(OBJMSG_GAMEP4_CHECKPOINT);
		Msg.m_iSender = _iSender;
		CStr DeathSeqs = CStrF("%s/%s/%s/%s", m_DeathSeqPast.Str(), m_DeathSeqPresent.Str(), m_DeathSeqFuture.Str(), m_DeathSeqSound.Str());
		Msg.m_pData = (void *)DeathSeqs.Str();
		if(m_bIgnoreTension)
			Msg.m_Param0 = 1;
		if(m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex()))
			m_pWServer->Object_Destroy(m_iObject);
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_Checkpoint, CWObject_Trigger_Ext, 0x0100);

// -------------------------------------------------------------------
//  CWObject_Trigger_RPG
// -------------------------------------------------------------------
//This class was used for the "pickup-portals" in Sorcery CTF
/*
class CWObject_Trigger_RPG : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	
public:
	enum
	{
		CLIENTFLAGS_LIGHTID = (1 << CWO_CLIENTFLAGS_USERSHIFT),
		CLIENTFLAGS_LIGHTENABLE = (2 << CWO_CLIENTFLAGS_USERSHIFT),
	};
	
	spCRPG_Object m_spRPGObject;
	int16 m_Wait;
	int16 m_RechargeTime;
	int16 m_ImpulseOn;
	int16 m_ImpulseOff;
	
	CWObject_Trigger_RPG()
	{
		m_IntersectNotifyFlags = OBJECT_FLAGS_CHARACTER;
		m_RechargeTime = -1;
		m_Wait = 0;
		m_ImpulseOn = 0;
		m_ImpulseOff = 1;
	}
		  
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		switch (_KeyHash)
		{
		case MHASH3('RPGO','BJEC','T'): // "RPGOBJECT"
			{
				m_spRPGObject = CRPG_Object::CreateObject(_pKey, m_pWServer);
				break;
			}
		
		case MHASH3('RECH','ARGE','TIME'): // "RECHARGETIME"
			{
				m_RechargeTime = _pKey->GetThisValuef() * SERVER_TICKSPERSECOND;
				break;
			}
		
		case MHASH3('IMPU','LSE_','ON'): // "IMPULSE_ON"
			{
				m_ImpulseOn = _pKey->GetThisValuei();
				break;
			}
		
		case MHASH3('IMPU','LSE_','OFF'): // "IMPULSE_OFF"
			{
				m_ImpulseOff = _pKey->GetThisValuei();
				break;
			}
		
		case MHASH2('LIGH','TID'): // "LIGHTID"
			{
				m_iAnim0 = _pKey->GetThisValuei();
				m_ClientFlags |= CLIENTFLAGS_LIGHTID | CLIENTFLAGS_LIGHTENABLE;
				break;
			}
		
		case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
			{
				CStr ModelName = CStrF("$WORLD:%d", _pKey->GetThisValuei());
				int iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
				if (!iModel) Error("OnEvalKey", "Failed to acquire world-model.");
				if (!m_iModel[0]) m_iModel[0] = iModel;
			
				// Setup physics
				CWO_PhysicsState Phys;
				Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_PHYSMODEL, iModel, 0, 0);
				Phys.m_nPrim = 1;
			
				Phys.m_PhysFlags = 0;
				Phys.m_ObjectFlags = OBJECT_FLAGS_TRIGGER;
				if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
					ConOutL("§cf80WARNING: Unable to set trigger physics state.");
			
				// Set bound-box.
				{
					CBox3Dfp32 Box;
					m_pWServer->GetMapData()->GetResource_Model(m_iModel[0])->GetBound_Box(Box);
					m_pWServer->Object_SetVisBox(m_iObject, Box.m_Min, Box.m_Max);
				}
				break;
			}
		
		default:
			{
				CWObject_Model::OnEvalKey(_pKey);
				break;
			}
		}
	}
	
	virtual void OnFinishEvalKeys()
	{
		if(!m_spRPGObject)
		{
			ConOutL("CWObject_Trigger_RPG::OnFinishEvalKeys, No RPGObject specified");
			Destroy();
		}
	}
	
	virtual aint OnMessage(const CWObject_Message& _Msg)
	{
		if(_Msg.m_Msg == OBJSYSMSG_NOTIFY_INTERSECTION)
		{
			if(m_Wait == 0)
			{
//				if(m_spRPGObject->GetType() == CRPG_Object::TYPE_ARTIFACT)
//				{
//					CRPG_Object_Artifact *pObject = (CRPG_Object_Artifact *)(CRPG_Object *)m_spRPGObject;
//					pObject->NumCharges() = 1;
//				}
//				if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_RPG_ACTIVATE, int((CRPG_Object *)m_spRPGObject), 0, m_iObject), _Msg.m_Param0) != 0)
				{
					m_Wait = m_RechargeTime;
					if(GetTarget())
						m_pWServer->Message_SendToTarget(CWObject_Message(OBJMSG_IMPULSE, m_ImpulseOff, 0, m_iObject), m_Target);
					if(m_ClientFlags & CLIENTFLAGS_LIGHTID)
						m_ClientFlags = m_ClientFlags & ~CLIENTFLAGS_LIGHTENABLE;
				}
			}
			return 0;
		}
		else
			return CWObject_Model::OnMessage(_Msg);
	}
	
	virtual void OnRefresh()
	{
		CWObject_Model::OnRefresh();
		
		if(m_Wait > 0)
		{
			m_Wait--;
			if(m_Wait == 0)
			{
				m_pWServer->Message_SendToTarget(CWObject_Message(OBJMSG_IMPULSE, m_ImpulseOn, 0, m_iObject), m_Target);
				if(m_ClientFlags & CLIENTFLAGS_LIGHTID)
					m_ClientFlags |= CLIENTFLAGS_LIGHTENABLE;
			}
		}
		
		//Could this be needed?
		//		m_spRPGObject->OnProcess(NULL);
	}
	
	virtual void OnLoad(CCFile* _pFile)
	{
		CWObject_Model::OnLoad(_pFile);
	}
	
	virtual void OnSave(CCFile* _pFile)
	{
		CWObject_Model::OnSave(_pFile);
	}
	
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine)
	{
		if(_pObj->m_ClientFlags & CLIENTFLAGS_LIGHTID)
		{
			// Control LightID
			if(_pObj->m_ClientFlags & CLIENTFLAGS_LIGHTENABLE)
				_pEngine->Render_Light_State(_pObj->m_iAnim0, 1.0f);
			else
				_pEngine->Render_Light_State(_pObj->m_iAnim0, 0);
		}
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_RPG, CWObject_Model, 0x0100);
*/


// -------------------------------------------------------------------
// Trigger_AIDie
// -------------------------------------------------------------------
#ifndef M_DISABLE_TODELETE
class CWObject_Trigger_AIDie : public CWObject_Trigger
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	
	struct CNPCItem
	{
		int m_iObject;
		int m_FirstTick;
		int m_LastTick;
	};

	CVec3Dfp32 m_Velocity;
	int m_Delay;

	TArray<CNPCItem> m_lNPCs;

public:
	CWObject_Trigger_AIDie()
	{
		m_TriggerObjFlags = OBJECT_FLAGS_CHARACTER;
		m_IntersectNotifyFlags = m_TriggerObjFlags;
		m_Delay = 2 * SERVER_TICKSPERSECOND;
		m_Velocity = 0;
	}

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
	{
		CStr KeyName = _pKey->GetThisName();

		switch (_KeyHash)
		{
		case MHASH2('VELO','CITY'): // "VELOCITY"
			{
				m_Velocity.ParseString(_pKey->GetThisValue());
				m_Velocity *= SERVER_TIMEPERFRAME;
				break;
			}
		
		case MHASH4('ACTI','VATI','ONDE','LAY'): // "ACTIVATIONDELAY"
			{
				m_Delay = _pKey->GetThisValuef() * SERVER_TICKSPERSECOND;
				break;
			}

		default:
			{
				CWObject_Trigger::OnEvalKey(_pKey);
				break;
			}
		}
	}

	void AddNPC(int _iSender)
	{
		int i;
		for(i = 0; i < m_lNPCs.Len(); i++)
		{
			if(m_lNPCs[i].m_iObject == _iSender)
			{
				m_lNPCs[i].m_LastTick = m_pWServer->GetGameTick();
				return;
			}
		}

		CNPCItem Item;
		Item.m_iObject = _iSender;
		Item.m_FirstTick = m_pWServer->GetGameTick();
		Item.m_LastTick = m_pWServer->GetGameTick();
		m_lNPCs.Add(Item);
	}

	virtual void Trigger(int _iSender = -1)
	{
		CWObject *pObj = m_pWServer->Object_Get(_iSender);
		// Only for NPC's
		if(pObj && !(pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER))
			AddNPC(_iSender);
	}

	void OnRefresh()
	{
		int iCurTick = m_pWServer->GetGameTick();
		for(int i = m_lNPCs.Len() - 1; i >= 0; i--)
		{
			if(m_lNPCs[i].m_LastTick < iCurTick - 20)
			{
				m_lNPCs.Del(i);
				continue;
			}

			if(m_Velocity != 0)
				m_pWServer->Object_SetVelocity(m_lNPCs[i].m_iObject, m_Velocity);

			int Duration = iCurTick - m_lNPCs[i].m_FirstTick;
			if(Duration > m_Delay)
			{
				CWO_DamageMsg Msg(10000, DAMAGETYPE_UNDEFINED);
				Msg.Send(m_lNPCs[i].m_iObject, m_iObject, m_pWServer);
			}
		}
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_AIDie, CWObject_Trigger, 0x0100);
#endif

// -------------------------------------------------------------------
//  Trigger_Lamp
// -------------------------------------------------------------------

class CWObject_Trigger_Lamp : public CWObject_Trigger_Ext
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:	
	enum
	{
		FLAGS_MODELSHADOW = 1,
		FLAGS_UNBREAKABLE = 2,
		FLAGS_NOPHYS	  = 4,

		STATE_ON = 0,
		STATE_OFF = 1,
		STATE_BROKEN = 2,
	};

	CStr m_Light;
	CStr m_MasterLight;
	CStr m_ProjectionMap;
	CVec3Dfp32 m_ProjMapOrigin;
	TArray<CWO_SimpleMessage> m_lMessages_LightsOn;
	TArray<CWO_SimpleMessage> m_lMessages_LightsOff;


	virtual void OnCreate()
	{
		CWObject_Trigger_Ext::OnCreate();

		m_IntersectNotifyFlags = OBJECT_FLAGS_PROJECTILE;
		m_DelayTicks = -1;
		m_Flags = TRIGGER_FLAGS_DESTROYABLE;
		m_Hitpoints = 0;
		m_ProjMapOrigin = 0.0f;
	}

	static void PrecacheSurface(CXW_Surface* _pSurf, CXR_Engine* _pEngine)
	{
		MAUTOSTRIP(PrecacheSurface, MAUTOSTRIP_VOID);
		CXW_Surface* pSurf = _pSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
		pSurf->InitTextures(false);
		pSurf->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	}

	static void PrecacheSurface(const char* _pSurf, CXR_Engine* _pEngine)
	{
		MAUTOSTRIP(PrecacheSurface_2, MAUTOSTRIP_VOID);
		CXW_Surface* pSurf = _pEngine->m_pSC->GetSurface(_pSurf);
		if (pSurf)
			PrecacheSurface(pSurf, _pEngine);
	}

	static void OnClientPrecacheClass(CWorld_Client* _pWClient, CXR_Engine* _pEngine)
	{
		CWObject_Trigger_Ext::OnClientPrecacheClass(_pWClient, _pEngine);
		PrecacheSurface("ELECTROSHOCK", _pEngine);
	}

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
	{
		CStr KeyName = _pKey->GetThisName();
		CStr KeyValue = _pKey->GetThisValue();

		switch (_KeyHash)
		{
		case MHASH2('LIGH','T'): // "LIGHT"
			{
				m_Light = m_pWServer->World_MangleTargetName(KeyValue);
				break;
			}

		case MHASH3('LAMP','_FLA','GS'): // "LAMP_FLAGS"
			{
				static const char *FlagsStr[] = { "ModelShadow", "Unbreakable", "NoPhys",NULL };
				m_Data[5] |= KeyValue.TranslateFlags(FlagsStr);
				if(m_Data[5] & FLAGS_UNBREAKABLE)
					m_Flags &= ~TRIGGER_FLAGS_DESTROYABLE;
				break;
			}

		case MHASH3('MAST','ERLI','GHT'): // "MASTERLIGHT"
			{
				m_MasterLight = m_pWServer->World_MangleTargetName(KeyValue);
				break;
			}

		case MHASH3('INIT','IALS','TATE'): // "INITIALSTATE"
			{
				iAnim0() = KeyValue.Val_int();
				break;
			}

		case MHASH4('LIGH','T_PR','OJMA','P'): // "LIGHT_PROJMAP"
			{
				m_ProjectionMap = KeyValue;
				break;
			}

		case MHASH5('LIGH','T_PR','OJMA','PORI','GIN'): // "LIGHT_PROJMAPORIGIN"
			{
				m_ProjMapOrigin.ParseString(KeyValue);
				break;
			}

		default:
			{
				if(KeyName.CompareSubStr("MESSAGE_LIGHTSON") == 0)
				{
					uint iSlot = atoi(KeyName.Str() + 16);
					m_lMessages_LightsOn.SetMinLen(iSlot + 1);
					m_lMessages_LightsOn[iSlot].Parse(KeyValue, m_pWServer);
				}

				else if(KeyName.CompareSubStr("MESSAGE_LIGHTSOFF") == 0)
				{
					uint iSlot = atoi(KeyName.Str() + 17);
					m_lMessages_LightsOff.SetMinLen(iSlot + 1);
					m_lMessages_LightsOff[iSlot].Parse(KeyValue, m_pWServer);
				}
				else
					return CWObject_Trigger_Ext::OnEvalKey(_KeyHash, _pKey);
				break;
			}
		}
	}

	virtual void OnFinishEvalKeys()
	{
		CWObject_Trigger_Ext::OnFinishEvalKeys();

		if (m_iBSPModel)
			Model_Set(0, m_iBSPModel, false);
	}

	virtual void OnSpawnWorld()
	{
		CWObject_Trigger_Ext::OnSpawnWorld();

		if (m_Light != "")
		{
			SafeSendToTarget(CWObject_Message(OBJMSG_IMPULSE, m_iAnim0 ? CWObject_Light::MSG_OFF : CWObject_Light::MSG_ON), m_Light);
			CWObject* pObj = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget(m_Light));
			if (pObj)
			{
				m_Data[7] = pObj->OnMessage(CWObject_Message(OBJMSG_LIGHT_GETINDEX));
				m_Data[6] = pObj->m_iObject;
			}

			// Set projection map on the light
			if (m_ProjectionMap != "")
			{
				CWObject_Message Msg(OBJMSG_LIGHT_INITPROJMAP, mint(m_ProjectionMap.Str()), 0, m_iObject, 0, m_ProjMapOrigin);
				pObj->OnMessage(Msg);
			}
		}

		if(m_MasterLight != "")
		{
			bint bActive = (m_iAnim0 == STATE_ON) ? 1 : 0;
			SafeSendToTarget(CWObject_Message(OBJMSG_IMPULSE, CWObject_Light::MSG_SLAVE_REG, bActive), m_MasterLight);
			m_Data[7] = -1;
		}

		int i;
		for(i = 0; i < m_lMessages_LightsOn.Len(); i++)
			m_lMessages_LightsOn[i].SendPrecache(m_iObject, m_pWServer);
		for(i = 0; i < m_lMessages_LightsOff.Len(); i++)
			m_lMessages_LightsOff[i].SendPrecache(m_iObject, m_pWServer);
	}

	// Set physics and stuff...
	virtual void Spawn(bool _bSpawn)
	{
		if(_bSpawn)
		{
			m_Flags &= ~TRIGGER_FLAGS_WAITSPAWN;

			if(m_iBSPModel != 0)
			{
				if(m_Flags & TRIGGER_FLAGS_VISIBLE)
					Model_Set(0, m_iBSPModel, false);

				// Setup physics
				CWO_PhysicsState Phys;
				Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_PHYSMODEL, m_iBSPModel, 0, 0);
				Phys.m_nPrim = 1;

				Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_ROTATION;
				Phys.m_ObjectFlags |= OBJECT_FLAGS_TRIGGER | OBJECT_FLAGS_NAVIGATION;
				if(m_Flags & TRIGGER_FLAGS_DESTROYABLE)
				{
					Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_PROJECTILE;
					m_IntersectNotifyFlags = 0;
				}
				else
					Phys.m_ObjectIntersectFlags = 0;
				if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
					ConOutL("§cf80WARNING: Unable to set trigger physics state.");
			}
			else if(GetPhysState().m_nPrim > 0)
			{
				CWO_PhysicsState Phys = GetPhysState();
				Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_ROTATION;
				Phys.m_ObjectFlags |= OBJECT_FLAGS_TRIGGER | OBJECT_FLAGS_NAVIGATION;
				if(m_Flags & TRIGGER_FLAGS_DESTROYABLE)
					Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_PROJECTILE;
				else
					Phys.m_ObjectIntersectFlags = 0;
				if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
					ConOutL("§cf80WARNING: Unable to set trigger physics state.");
			}
			else
			{
				CWO_PhysicsState Phys = GetPhysState();
				/*int iFlags = OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
				Model_SetPhys(m_iModel[0], false, iFlags, OBJECT_PHYSFLAGS_PUSHER | OBJECT_PHYSFLAGS_ROTATION, false);*/
				CXR_Model* pM = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
				if (pM)
				{
					CBox3Dfp32 Box;
					pM->GetBound_Box(Box);
					Phys.m_PhysFlags = OBJECT_PHYSFLAGS_OFFSET | OBJECT_PHYSFLAGS_ROTATION;
					Phys.m_ObjectFlags = (m_Data[5] & FLAGS_NOPHYS) ? 
						OBJECT_FLAGS_TRIGGER : 
						OBJECT_FLAGS_STATIC | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_TRIGGER | OBJECT_FLAGS_NAVIGATION;

					if(m_Flags & TRIGGER_FLAGS_DESTROYABLE)
						Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_PROJECTILE;
					else
						Phys.m_ObjectIntersectFlags = 0;
					CVec3Dfp32 Size = (Box.m_Max - Box.m_Min);
					CVec3Dfp32 Offset;
					Box.GetCenter(Offset);
					Phys.m_nPrim = 1;
					Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, 
						CVec3Dfp32(M_Fabs(Size.k[0]*0.5f), M_Fabs(Size.k[1]*0.5f), M_Fabs(Size.k[2]*0.5f)),Offset);
					if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
						ConOutL("§cf80WARNING: Unable to set object physics state.");
				}
			}

			{
				// Force check all intersecting objects
				CWO_PhysicsState Phys = GetPhysState();
				Phys.m_ObjectIntersectFlags = m_IntersectNotifyFlags;

				int iObj = m_iObject;
				CWorld_Server* pServer = m_pWServer;
				TSelection<CSelection::LARGE_BUFFER> Selection;
				{
					pServer->Selection_AddIntersection(Selection, GetPositionMatrix(), Phys);
					const int16* pSel = NULL;
					int nSel = pServer->Selection_Get(Selection, &pSel);
					for(int i = 0; i < nSel; i++)
					{
						pServer->Message_SendToObject(CWObject_Message(OBJMSG_TRIGGER_INTERSECTION, pSel[i]), m_iObject);
						if(!pServer->Object_Get(iObj))
							return;
					}
				}
			}
		}
		else
		{
			if(m_iBSPModel != 0 && GetPhysState().m_nPrim == 1)
			{
				CWO_PhysicsState Phys;
				Phys.m_nPrim = 0;
				m_pWServer->Object_SetPhysics(m_iObject, Phys);
			}
			m_Flags |= TRIGGER_FLAGS_WAITSPAWN;
		}

		UpdateNoRefreshFlag();
	}

	void SafeSendToTarget(const CWObject_Message &_Msg, const char *_pTarget)
	{
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddTarget(Selection, _pTarget);
		const int16 *pSel;
		int nSel = m_pWServer->Selection_Get(Selection, &pSel);
		for(int i = 0; i < nSel; i++)
		{
			CWObject *pObj = m_pWServer->Object_Get(pSel[i]);
			if(pObj->m_iClass == m_iClass)
				ConOutL(CStrF("§cf80WARNING: (GP) Detected a Lamp with same name as Light! (%s)", pObj->GetName()));
			else
				m_pWServer->Message_SendToObject(_Msg, pSel[i]);
		}
	}

	virtual aint OnMessage(const CWObject_Message &_Msg)
	{
		switch(_Msg.m_Msg)
		{
		case OBJMSG_IMPULSE:
		case OBJMSG_LIGHT_IMPULSE:
			if(!(m_Flags & TRIGGER_FLAGS_WAITSPAWN))
			{
				if(_Msg.m_Param0 == CWObject_Light::MSG_BREAK)
				{
					Trigger(_Msg.m_iSender);
				}
				else if (m_iAnim0 != STATE_BROKEN)
				{
					int Msg = _Msg.m_Param0;
					if(Msg == CWObject_Light::MSG_TOGGLE)
						Msg = m_iAnim0 ? CWObject_Light::MSG_OFF : CWObject_Light::MSG_ON;

					if(Msg == CWObject_Light::MSG_ON)
					{
						// This should not be sent if the light is broken. Fix this after Riddick GM
						for(int i = 0; i < m_lMessages_LightsOn.Len(); i++)
							m_lMessages_LightsOn[i].SendMessage(m_iObject, _Msg.m_iSender, m_pWServer);
						iAnim0() = STATE_ON;
					}
					else if(Msg == CWObject_Light::MSG_OFF)
					{
						for(int i = 0; i < m_lMessages_LightsOff.Len(); i++)
							m_lMessages_LightsOff[i].SendMessage(m_iObject, _Msg.m_iSender, m_pWServer);
						iAnim0() = STATE_OFF;
					}
					m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);

					if(m_Light != "")
						SafeSendToTarget(_Msg, m_Light);

					if (m_MasterLight != "")
					{
						if (m_iAnim0 == STATE_ON)
							SafeSendToTarget(CWObject_Message(OBJMSG_IMPULSE, CWObject_Light::MSG_SLAVE_ON), m_MasterLight);
						else if (m_iAnim0 == STATE_OFF)
							SafeSendToTarget(CWObject_Message(OBJMSG_IMPULSE, CWObject_Light::MSG_SLAVE_OFF), m_MasterLight);
					}
				}
			}
			return 1;

		case OBJMSG_CHAR_STUN:
			{
				m_Data[4] = m_pWServer->GetGameTick();
				return 1;
			}
		}

		return CWObject_Trigger_Ext::OnMessage(_Msg);
	}

	virtual void Trigger(int _iSender)
	{
		if (m_iAnim0 != STATE_BROKEN)
		{
			if (m_Light != "")
			{
				SafeSendToTarget(CWObject_Message(OBJMSG_IMPULSE, CWObject_Light::MSG_BREAK), m_Light);

				TSelection<CSelection::LARGE_BUFFER> Selection;
				m_pWServer->Selection_AddTarget(Selection,m_Light);
				const int16 *pSel;
				int nSel = m_pWServer->Selection_Get(Selection, &pSel);
				for(int i = 0; i < nSel; i++)
				{
					CWObject* pObj = m_pWServer->Object_Get(pSel[i]);
					if (pObj)
					{
						m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_BROKEN_LIGHT,pObj->m_iObject),m_pWServer->Game_GetObjectIndex());
						break;
					}
				}
			}

			if (m_MasterLight != "")
				SafeSendToTarget(CWObject_Message(OBJMSG_IMPULSE, CWObject_Light::MSG_SLAVE_OFF, 1), m_MasterLight);

			iAnim0() = STATE_BROKEN;
		}

		CWObject_Trigger_Ext::Trigger(_iSender);
	}

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
	{
		fp32 IPTime = _pWClient->GetRenderTickFrac();

		// Interpolate matrix
		CMat4Dfp32 MatIP;
		int32 iParentObject = _pObj->GetParent();
		if(iParentObject > 0)
		{
			//fp32 Time = 0;
			// get parent matrix
			CMat4Dfp32 ParentMatrix;
			CWObject_Message RenderMat(OBJMSG_HOOK_GETCURRENTMATRIX,(aint)(&ParentMatrix)); 
			aint ok = _pWClient->Phys_Message_SendToObject(RenderMat,iParentObject);

			if (ok)
				_pObj->GetLocalPositionMatrix().Multiply(ParentMatrix, MatIP);
			else
				Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);
		}
		else
		{
			Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);
		}

		int LightGUID = 0;
		CPixel32 Color(0);
		if(_pObj->m_Data[7] == -1)
		{
			// This is masterlight controlled
			if(_pObj->m_iAnim0 == 0)
				Color = -1;
		}
		else if(_pObj->m_Data[7])
		{
			CVec3Dfp32 Intens;
			LightGUID = _pEngine->m_pSceneGraphInstance->SceneGraph_Light_GetGUID(_pObj->m_Data[7]);
			_pEngine->m_pSceneGraphInstance->SceneGraph_Light_GetIntensity(_pObj->m_Data[7], Intens);

			Color.R() = RoundToInt(Clamp01(Intens[0] * 2.0f) * 255.0f);
			Color.G() = RoundToInt(Clamp01(Intens[1] * 2.0f) * 255.0f);
			Color.B() = RoundToInt(Clamp01(Intens[2] * 2.0f) * 255.0f);
			Color.A() = 255;
		}
		else if(_pObj->m_Data[6])
		{
			CVec3Dfp32 Intens;
			LightGUID = _pObj->m_Data[6];
			int iLight = _pEngine->m_pSceneGraphInstance->SceneGraph_Light_GetIndex(_pObj->m_Data[6]);
			_pEngine->m_pSceneGraphInstance->SceneGraph_Light_GetIntensity(iLight, Intens);

			Color.R() = RoundToInt(Clamp01(Intens[0]) * 255.0f);
			Color.G() = RoundToInt(Clamp01(Intens[1]) * 255.0f);
			Color.B() = RoundToInt(Clamp01(Intens[2]) * 255.0f);
			Color.A() = 255;
		}

		CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
		AnimState.m_Data[1] = Color;
		if (!(_pObj->m_Data[5] & FLAGS_MODELSHADOW))
			AnimState.m_NoShadowLightGUID = LightGUID;

		uint32 RenderFlags = 0;
		if(_pObj->m_Data[4] != 0)
		{
			fp32 Duration = ((_pWClient->GetGameTick() - _pObj->m_Data[4]) + _pWClient->GetRenderTickFrac()) * _pWClient->GetGameTickTime();
			if(Duration < 4)
			{
				CXW_Surface* pSurf = _pEngine->GetSC()->GetSurface("ELECTROSHOCK");
				if(pSurf)
				{
					AnimState.m_lpSurfaces[1] = pSurf;
					AnimState.m_AnimTime0.Reset();
					AnimState.m_AnimTime1 = CMTime::CreateFromSeconds(Duration);
					RenderFlags |= CXR_MODEL_ONRENDERFLAGS_SURF0_ADD;
				}
			}
		}

		for(int i = 0; i < CWO_NUMMODELINDICES; i++)
		{
			if(_pObj->m_iModel[i] > 0)
			{
				CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
				if(pModel)
					_pEngine->Render_AddModel(pModel, MatIP, AnimState, XR_MODEL_STANDARD, RenderFlags);
			}
		}
	}

	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags)
	{
		CWObject_Model::OnDeltaLoad(_pFile, _Flags);
		_pFile->ReadLE(m_iAnim0);
	}

	virtual void OnDeltaSave(CCFile* _pFile)
	{
		CWObject_Model::OnDeltaSave(_pFile);
		_pFile->WriteLE(m_iAnim0);
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_Lamp, CWObject_Trigger_Ext, 0x0100);

// -------------------------------------------------------------------
//  Trigger_Surface
// -------------------------------------------------------------------
class CWObject_Trigger_Surface : public CWObject_Trigger_Ext
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:	
	virtual void OnCreate()
	{
		CWObject_Trigger_Ext::OnCreate(),

		m_IntersectNotifyFlags = OBJECT_FLAGS_PROJECTILE;
		m_DelayTicks = -1;
		m_Flags = TRIGGER_FLAGS_DESTROYABLE | TRIGGER_FLAGS_VISIBLE;
		m_CreationGameTick = 0;
		m_CreationGameTickFraction = 0.0f;
	}

	virtual void Trigger(int _iSender)
	{
		CWObject_Trigger_Ext::Trigger(_iSender);

		SetAnimTick(m_pWServer, 0, 0);
		SetDirty(CWO_DIRTYMASK_ANIM);
	}

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
	{
		for(int i = 0; i < CWO_NUMMODELINDICES; i++)
		{
			if(_pObj->m_iModel[i] > 0)
			{
				CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
				if(pModel)
				{
					CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
					if(_pObj->m_CreationGameTick > 0)
						AnimState.m_AnimTime0 = CMTime::CreateFromTicks(_pObj->GetAnimTick(_pWClient), _pWClient->GetGameTickTime(), _pWClient->GetRenderTickFrac() - _pObj->GetAnimTickFraction());
					else
						AnimState.m_AnimTime0.Reset();
					_pEngine->Render_AddModel(pModel, _pObj->GetPositionMatrix(), AnimState);
				}
			}
		}
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_Surface, CWObject_Trigger_Ext, 0x0100);

// -------------------------------------------------------------------
//  Trigger_Pickup
// -------------------------------------------------------------------
void CWObject_Trigger_Pickup::OnCreate()
{
	CWObject_Trigger_Ext::OnCreate();

	m_IntersectNotifyFlags = OBJECT_FLAGS_CHARACTER;
	m_DelayTicks = -1;
	m_Flags = TRIGGER_FLAGS_VISIBLE;
	m_CreationGameTick = 0;
	m_CreationGameTickFraction = 0.0f;

	m_SpawnModeFlags	= PICKUP_SPAWNFLAG_DM | PICKUP_SPAWNFLAG_TDM | PICKUP_SPAWNFLAG_CTF | PICKUP_SPAWNFLAG_SURVIVOR | PICKUP_SPAWNFLAG_LASTHUMAN | PICKUP_SPAWNFLAG_SHAPESHIFTER | PICKUP_SPAWNFLAG_DARKLINGS | PICKUP_SPAWNFLAG_DVH;
	m_TicksLeft			= 0;
	m_SpawnTime			= 0;
	m_iSound			= 0;
	m_Param0			= 0;
	m_Type				= PICKUP_UNKNOWN;
}

void CWObject_Trigger_Pickup::Spawn(bool _bSpawn)
{
	CWObject_Trigger_Ext::Spawn(_bSpawn);
	if(!m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_CANPICKUPSPAWN, m_SpawnModeFlags), m_pWServer->Game_GetObjectIndex()))
		m_pWServer->Object_Destroy(m_iObject);
}

void CWObject_Trigger_Pickup::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	int32 Valuei = _pKey->GetThisValuei();
	CStr Value = _pKey->GetThisValue();
	uint32 ValueHash = StringToHash(Value);
	fp32 Valuef = _pKey->GetThisValuef();

	switch (_KeyHash)
	{
	case MHASH3('MODE','FLAG','S'):	//"MODEFLAGS"
		{
			static const char *SpawnModeFlags[] = { "DeathMatch", "TeamDeathMatch", "CaptureTheFlag", "Survivor", "LastHuman", "Shapeshifter", "DarklingsVsDarklings", "DarklingsVsHumans", NULL };
			m_SpawnModeFlags = Value.TranslateFlags(SpawnModeFlags);
			break;
		}
	case MHASH3('SPAW','NTIM','E'): // "SPAWNTIME"
		{
			m_SpawnTime = Valuei;
			break;
		}
	case MHASH2('HEAL','TH'): // "HEALTH"
		{
			m_Param0 = Valuei;
			break;
		}
	case MHASH1('TEAM'): // "TEAM"
		{
			m_Param0 = Valuei;
			break;
		}
	case MHASH2('SOUN','D'): // "SOUND"
		{
			m_iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(Value);
			break;
		}
	case MHASH2('WEAP','ON'): // "WEAPON"
		{
			m_WeaponTemplate = Value;
			break;
		}
	case MHASH3('MAX_','HEAL','TH'): // "MAX_HEALTH"
		{
			m_Param0 = Valuei;
			break;
		}
	case MHASH3('shie','ld_v','alue'):
		{
			m_Param0 = Valuei;
			break;
		}
	case MHASH4('SPEE','D_MU','LTIP','LIER'): // "SPEED_MULTIPLIER"
		{
			m_Multiplier = Valuef;
			break;
		}
	case MHASH5('DAMA','GE_M','ULTI','PLIE','R'): // "DAMAGE_MULTIPLIER"
		{
			m_Multiplier = Valuef;
			break;
		}
	case MHASH2('DURA','TION'): // "DURATION"
		{
			m_Duration = Valuei;
			break;
		}
	case MHASH3('AMMO','_TYP','E'): // "AMMO_TYPE"
		{
			switch(ValueHash) 
			{
			case MHASH2('pist','ol'):
			    m_Param0 = 1;
				break;
			case MHASH2('rifl','e'):
			    m_Param0 = 8;
				break;
			case MHASH2('shot','gun'):
			    m_Param0 = 20;
				break;
			}			
			break;
		}
	case MHASH1('TYPE'): // "TYPE"
		{
			switch(ValueHash) 
			{
			case MHASH2('heal','th'):
				m_Type = PICKUP_HEALTH;			    
				break;
			case MHASH1('ammo'):
			    m_Type = PICKUP_AMMO;
				break;
			case MHASH2('weap','on'):
			    m_Type = PICKUP_WEAPON;
				break;
			case MHASH1('flag'):
				m_Type = PICKUP_FLAG;
				break;
			case MHASH3('heal','th_b','oost'):
				m_Type = PICKUP_HEALTH_BOOST;
				break;
			case MHASH3('spee','d_bo','ost'):
				m_Type = PICKUP_SPEED_BOOST;
				break;
			case MHASH3('dama','ge_b','oost'):
				m_Type = PICKUP_DAMAGE_BOOST;
				break;
			case MHASH2('shie','ld'):
				m_Type = PICKUP_SHIELD;
				break;
			case MHASH3('invi','sibi','lity'):
				m_Type = PICKUP_INVISIBILITY;
				break;
			}
			break;
		}
	case MHASH2('MODE','L'): // "MODEL"
		{
			m_ModelName = Value;
			CWObject_Trigger_Ext::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	default:
		{
			CWObject_Trigger_Ext::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Trigger_Pickup::OnFinishEvalKeys()
{
	if(m_Type == PICKUP_UNKNOWN)
		ConOutL("Pickup object is missing type information");

	CWObject_Trigger_Ext::OnFinishEvalKeys();

	if(m_WeaponTemplate.Len())
	{
		spCRPG_Object spObj = CRPG_Object::CreateObject(m_WeaponTemplate, m_pWServer);
		m_spDummyObject = (CRPG_Object_Item *)(CRPG_Object *)spObj;
		if(!m_spDummyObject)
		{
			ConOutL(CStrF("§cf00ERROR Failed to create dummyobject(%s) in TriggerPickup(%s)\n", m_WeaponTemplate.Str(), GetName()));
			m_pWServer->Object_Destroy(m_iObject);
			return;
		}
	}
}

void CWObject_Trigger_Pickup::Trigger(int _iSender)
{
	CWObject_Trigger_Ext::Trigger(_iSender);
}

void CWObject_Trigger_Pickup::UpdateNoRefreshFlag()
{
	if(m_ClientFlags & CWO_CLIENTFLAGS_NOREFRESH)
	{
		m_ClientFlags &= ~CWO_CLIENTFLAGS_NOREFRESH;
		m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);
	}
}

void CWObject_Trigger_Pickup::OnRefresh()
{
//	m_pWServer->Object_MovePhysical(m_iObject);
	if(m_TicksLeft)
	{
		m_TicksLeft--;
		if(!m_TicksLeft)
			m_pWServer->Object_Destroy(m_iObject);
	}
}

int CWObject_Trigger_Pickup::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	switch(_Event)
	{
	case CWO_PHYSEVENT_GETACCELERATION :
		{
			_pMat->Unit();
			_pMat->k[3][2] = -1.0f;
			return SERVER_PHYS_HANDLED;
		}

	case CWO_PHYSEVENT_IMPACT :
		{
			if (_pObj)
			{
				CVelocityfp32 Vel = _pObj->GetVelocity();
				Vel.m_Move *= 0.9f;
				Vel.m_Rot.m_Angle *= 0.9f;

				//				Swap(Vel.m_Rot.m_Axis[0], Vel.m_Rot.m_Axis[1]);
				//				Swap(Vel.m_Rot.m_Axis[1], Vel.m_Rot.m_Axis[2]);

				if (_pCollisionInfo && _pCollisionInfo->m_bIsValid)
				{
					const fp32 Radius = 6.0f;

					CVec3Dfp32 N = _pCollisionInfo->m_Plane.n;

					CVec3Dfp32 MoveProj;
					Vel.m_Move.Combine(N, -(N * Vel.m_Move), MoveProj);

					fp32 ProjVel = MoveProj.Length();
					fp32 TargetRotVel = ProjVel / (Radius * _PI2);

					CVec3Dfp32 Axis2;
					_pObj->GetMoveVelocity().CrossProd(_pCollisionInfo->m_Plane.n, Axis2);
					//					_pCollisionInfo->m_Plane.n.CrossProd(_pObj->GetMoveVelocity(), Axis2);
					Axis2.Normalize();
					Vel.m_Rot.m_Axis.Lerp(Axis2, 0.5f, Vel.m_Rot.m_Axis);
					Vel.m_Rot.m_Axis.Normalize();
					Vel.m_Rot.m_Angle = LERP(Vel.m_Rot.m_Angle, TargetRotVel, 0.5f);
				}

				//				Vel.m_Rot.m_Axis = -Vel.m_Rot.m_Axis;
				_pPhysState->Object_SetVelocity(_pObj->m_iObject, Vel);
			}
			if (_pCollisionInfo && _pCollisionInfo->m_bIsValid)
			{
				if (_pCollisionInfo->m_Plane.n[2] > 0.8f)
					_pObj->m_Data[0] |= 2;	// On ground
			}
			return SERVER_PHYS_DEFAULTHANDLER;
		}

	default :
		return SERVER_PHYS_DEFAULTHANDLER;
	}
}

aint CWObject_Trigger_Pickup::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg) 
	{
	case OBJSYSMSG_NOTIFY_INTERSECTION : 
	case OBJMSG_TRIGGER_INTERSECTION :
		{
			//This will remove this object
			CWObject_Message Msg;
			Msg.m_Msg = OBJMSG_CHAR_PICKUP;
			Msg.m_iSender = _Msg.m_Param0;
			Msg.m_Param0 = m_iObject;
			Msg.m_Param1 = m_TicksLeft;
			m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());

			return 0;
		}
	case OBJMSG_TRIGGER_PICKUP_SETTICKSLEFT:
		{
			m_TicksLeft = _Msg.m_Param0;	
		}
		return 1;
	}
	return CWObject_Model::OnMessage(_Msg);
}

void CWObject_Trigger_Pickup::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		if(_pObj->m_iModel[i] > 0)
		{
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
			if(pModel)
			{
				CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
				if(_pObj->m_CreationGameTick > 0)
					AnimState.m_AnimTime0 = CMTime::CreateFromTicks(_pObj->GetAnimTick(_pWClient), _pWClient->GetGameTickTime(), _pWClient->GetRenderTickFrac() - _pObj->GetAnimTickFraction());
				else
					AnimState.m_AnimTime0.Reset();
				CMat4Dfp32 ObjPos = _pObj->GetPositionMatrix();
				ObjPos.RotZ_x_M(((fp32)_pWClient->GetGameTick() + _pWClient->GetRenderTickFrac()) / 150.0f);
				ObjPos.GetRow(3) += CVec3Dfp32(0.0f, 0.0f, 1.0f) * M_Sin(((fp32)_pWClient->GetGameTick() + _pWClient->GetRenderTickFrac()) / 15.0f);
				_pEngine->Render_AddModel(pModel, ObjPos, AnimState);
			}
		}
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_Pickup, CWObject_Trigger_Ext, 0x0100);


// -------------------------------------------------------------------
//  TRIGGER_ACCPAD
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_AccPad, CWObject_Trigger, 0x0100);

CWObject_Trigger_AccPad::CWObject_Trigger_AccPad() :
m_Vel(0, 0, 32)
{
	m_TriggerObjFlags = OBJECT_FLAGS_CHARACTER /*| OBJECT_FLAGS_PHYSOBJECT*/;
	m_IntersectNotifyFlags = m_TriggerObjFlags;
	m_TimeOut = 0;
}

void CWObject_Trigger_AccPad::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Trigger_AccPad_OnEvalKey, MAUTOSTRIP_VOID);
	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH2('VELO','CITY'): // "VELOCITY"
		{
			m_Vel.ParseString(_Value);
			return;
			break;
		}
	default:
		{
			CWObject_Trigger::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

aint CWObject_Trigger_AccPad::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Trigger_AccPad_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
	case OBJSYSMSG_NOTIFY_INTERSECTION : 
	case OBJMSG_TRIGGER_INTERSECTION :
		{
			if (!m_TimeOut)
			{
				//				CWObject* pObj = m_pWServer->Object_Get(_Msg.m_Param0);
				m_pWServer->Object_SetVelocity(_Msg.m_Param0, m_Vel);
				m_TimeOut = 4;
				m_ClientFlags &= ~CWO_CLIENTFLAGS_NOREFRESH;
			}
			return 0;
		}
	default :
		return CWObject_Trigger::OnMessage(_Msg);
	}
}

void CWObject_Trigger_AccPad::OnRefresh()
{
	MAUTOSTRIP(CWObject_Trigger_AccPad_OnRefresh, MAUTOSTRIP_VOID);
	if (m_TimeOut) m_TimeOut--;
	CWObject_Trigger::OnRefresh();
	if (m_TimeOut) m_ClientFlags &= ~CWO_CLIENTFLAGS_NOREFRESH;
}




// -------------------------------------------------------------------
//  CWObject_Phys_Anim
// -------------------------------------------------------------------
class CWObject_Phys_Anim : public CWObject_Model
{
public:
	MRTC_DECLARE_SERIAL_WOBJECT;

	virtual void Model_SetPhys(int _iModel, bool _bAdd, int _ObjectFlags, int _PhysFlags, bool _bNoPhysReport)
	{
		MAUTOSTRIP(CWObject_Phys_Anim_SetPhys, MAUTOSTRIP_VOID);

		CXR_Model* pM = m_pWServer->GetMapData()->GetResource_Model(_iModel);
		if (pM)
		{
			CXR_PhysicsModel* pPhysModel = pM->Phys_GetInterface();
			if (pPhysModel)
			{
				// Setup physics
				CWO_PhysicsState Phys = GetPhysState();

				if (!_bAdd) Phys.m_nPrim = 0;
				if (Phys.m_nPrim >= CWO_MAXPHYSPRIM)
					Error("Model_SetPhys", "Too many physics-primitives.");
				Phys.m_Prim[Phys.m_nPrim++].Create(OBJECT_PRIMTYPE_PHYSMODEL, _iModel, 0, 0);

				Phys.m_PhysFlags = _PhysFlags;
				Phys.m_ObjectFlags = OBJECT_FLAGS_ANIMPHYS | OBJECT_FLAGS_NAVIGATION; //Currently phys_anim is always rendered in navgrid
				if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
					LogFile("§cf80WARNING: Unable to set model physics state.");
			}
			else if(_bNoPhysReport)
				ConOutL("§cf80WARNING (CWObject_Model::Model_SetPhys): Model was not a physics-model.");
		}
		else
			ConOutL("§cf80WARNING (CWObject_Model::Model_SetPhys): Invalid model-index.");
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Phys_Anim, CWObject_Model, 0x0100);

// -------------------------------------------------------------------
//  CWObject_MusicVolume
// -------------------------------------------------------------------
class CWObject_MusicVolume : public CWObject_Trigger_Ext
{
public:
	TArray<int> m_lMusic;

	struct CActive
	{
		int m_iSender;
		TArray<int> m_lOldMusic;
	};
	TArray<CActive> m_lActive;

	MRTC_DECLARE_SERIAL_WOBJECT;

	virtual void OnCreate()
	{
		CWObject_Trigger_Ext::OnCreate(),
		m_bRunTriggerOnLeave = true;
	}

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		if(_pKey->GetThisName().CompareSubStr("MUSICTRACK") == 0)
		{
			int iTrack = _pKey->GetThisName().Copy(10, 1024).Val_int();
			int Len = m_lMusic.Len();
			m_lMusic.SetLen(Max(iTrack + 1, Len));
			for(int i = Len; i < iTrack; i++)
				m_lMusic[i] = 0;
			m_lMusic[iTrack] = m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
		}
		else
			CWObject_Trigger_Ext::OnEvalKey(_KeyHash, _pKey);
	}

	int FindSender(int _iSender)
	{
		for(int i = 0; i < m_lActive.Len(); i++)
			if(m_lActive[i].m_iSender == _iSender)
				return i;
		return -1;
	}

	virtual void TriggerOnEnter(int _iSender)
	{
		int iActive = FindSender(_iSender);
		if(iActive != -1)
			return;

		CActive Active;
		Active.m_lOldMusic.SetLen(m_lMusic.Len());
		for(int i = 0; i < m_lMusic.Len(); i++)
		{
			if(m_lMusic[i] != 0)
				Active.m_lOldMusic[i] = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_SETMUSIC, i, m_lMusic[i]), m_pWServer->Game_GetObjectIndex());
			else
				Active.m_lOldMusic[i] = 0;
		}
		Active.m_iSender = _iSender;
		m_lActive.Add(Active);
	}

	virtual void TriggerOnLeave(int _iSender)
	{
		int iActive = FindSender(_iSender);
		if(iActive == -1)
			return;

		CActive *pActive = &m_lActive[iActive];
		for(int i = 0; i < pActive->m_lOldMusic.Len(); i++)
		{
			if(pActive->m_lOldMusic[i] != 0)
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_SETMUSIC, i, pActive->m_lOldMusic[i]), m_pWServer->Game_GetObjectIndex());
		}
		m_lActive.Del(iActive);
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_MusicVolume, CWObject_Trigger_Ext, 0x0100);


// -------------------------------------------------------------------
//  CWObject_UseProxy
// -------------------------------------------------------------------
class CWObject_UseProxy : public CWObject_Model
{
public:
	MRTC_DECLARE_SERIAL_WOBJECT;

	virtual void OnCreate()
	{
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;

		CWO_PhysicsState Phys = GetPhysState();
		Phys.m_ObjectFlags |= OBJECT_FLAGS_PICKUP;
		m_pWServer->Object_SetPhysics(m_iObject, Phys);
	}

	virtual aint OnMessage(const CWObject_Message& _Msg)
	{
		switch(_Msg.m_Msg)
		{
		case OBJMSG_ACTIONCUTSCENE_SETDIALOGUEPROXY:
			if(_Msg.m_pData == NULL)
				m_iAnim2 = 0;
			else
				m_iAnim2 = m_pWServer->Selection_GetSingleTarget((const char *)_Msg.m_pData);
			return 1;

		case OBJMSG_CHAR_USE:
			if(m_iAnim2 != 0)
				return m_pWServer->Message_SendToObject(_Msg, m_iAnim2);
			return 0;

		case OBJMSG_CHAR_GETCHOICES:
			if(m_iAnim2 != 0)
				return m_pWServer->Message_SendToObject(_Msg, m_iAnim2);
			return 0;
		}

		return CWObject::OnMessage(_Msg);
	}

	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
	{
		switch (_Msg.m_Msg)
		{
		case OBJMSG_CHAR_GETCHOICES:
			if(_pObj->m_iAnim2 != 0)
				return _pWClient->ClientMessage_SendToObject(_Msg, _pObj->m_iAnim2);
			return 0;
		}
		return CWObject_Model::OnClientMessage(_pObj, _pWClient, _Msg);
	}

	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags)
	{
		CWObject_Model::OnDeltaLoad(_pFile, _Flags);
		_pFile->ReadLE(m_iAnim2);
	}

	virtual void OnDeltaSave(CCFile* _pFile)
	{
		CWObject_Model::OnDeltaSave(_pFile);
		_pFile->WriteLE(m_iAnim2);
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_UseProxy, CWObject_Model, 0x0100);



///////////////////////////
// TRIGGER LOOK ///////////
///////////////////////////
void CWObject_Trigger_Look::OnCreate()
{
	CWObject_Trigger_Look_Parent::OnCreate();
	m_iLookObject = 0;

	// How tight we must look at the object
	m_LookAccuracy = M_Cos(25.0f*0.5f*_PI/180.0f);
	m_LookTime.Reset();
	m_NeededLookTime = 0.0f;
	m_LastLookTick = 0;
	// While we have objects inside the trigger, run checks
	m_bRunTriggerOnLeave = true;
}

void CWObject_Trigger_Look::OnSpawnWorld()
{
	CWObject_Trigger_Look_Parent::OnSpawnWorld();
	// Can't know that the position has been created until we spawn
	m_iLookObject = m_pWServer->Selection_GetSingleTarget(m_LookObject);
	for(int32 i = 0; i < m_lLookLeaveMessages.Len(); i++)
		m_lLookLeaveMessages[i].SendPrecache(m_iObject, m_pWServer);
}

void CWObject_Trigger_Look::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyVal = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH3('LOOK','OBJE','CT'): // "LOOKOBJECT"
		{
			m_LookObject = KeyVal;
			break;
		}
	case MHASH3('LOOK','ACCU','RACY'): // "LOOKACCURACY"
		{
			// 0-90
			m_LookAccuracy = M_Cos(KeyVal.Val_fp64() * 0.5f * _PI / 180.0f);
			break;
		}
	case MHASH4('NEED','EDLO','OKTI','ME'): // "NEEDEDLOOKTIME"
		{
			// How long we must look at the target before triggering (in seconds)
			m_NeededLookTime = KeyVal.Val_fp64();
			break;
		}
	default:
		{
			if(KeyName.Find("LOOKLEAVEMESSAGE") != -1)
			{
				CWO_SimpleMessage Msg;
				Msg.Parse(KeyVal, m_pWServer);
				m_lLookLeaveMessages.Add(Msg);
			}
			else
				CWObject_Trigger_Look_Parent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

aint CWObject_Trigger_Look::OnMessage(const CWObject_Message& _Msg)
{
	return CWObject_Trigger_Look_Parent::OnMessage(_Msg);
}

void CWObject_Trigger_Look::OnRefresh()
{
	// Go through all current intersecting objects and see if we intersect something
	for (int32 i = 0; i < m_lCurIntersectingObjects.Len(); i++)
	{
		int iObj = m_lCurIntersectingObjects[i].m_iObject;
		if (!IsDestroyed() && CheckLook(iObj))
			CWObject_Trigger_Look_Parent::Trigger(iObj);
	}
	// Check if we got any leave messages
	CheckLookActive(m_pWServer->GetGameTick());

	CWObject_Trigger_Look_Parent::OnRefresh();
}

void CWObject_Trigger_Look::Trigger(int _iSender)
{
	if (!IsDestroyed() && CheckLook(_iSender))
		CWObject_Trigger_Look_Parent::Trigger(_iSender);
}

bool CWObject_Trigger_Look::CheckLook(int32 _iObject)
{
	bool bOk = false;
	if (m_iLookObject > 0)
	{
		CWObject_CoreData* pObjTarget = m_pWServer->Object_GetCD(m_iLookObject);
		if (!pObjTarget)
			return false;
		CVec3Dfp32 TargetPos(0.0f,0.0f,0.0f);
		// If target is a character get the center of it (not the feet)
		if (pObjTarget->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER)
			pObjTarget->GetAbsBoundBox()->GetCenter(TargetPos);
		else
			TargetPos = pObjTarget->GetPosition();

		// Check for current intersecting objects if they look at our target
		// For now it's enough if one is looking for the trigger to go... 
		CVec3Dfp32 LookPos[2];
		CWObject_Message Msg(OBJMSG_CHAR_GETLOOKDIRANDPOSITION);
		Msg.m_pData = &LookPos;
		Msg.m_DataSize = 2*sizeof(CVec3Dfp32);
		bool bLookOk = false;
		if (m_pWServer->Message_SendToObject(Msg,_iObject))
		{
			CVec3Dfp32 Dir = TargetPos - LookPos[0];
			//m_pWServer->Debug_RenderWire(LookPos[0],LookPos[0] + LookPos[1]*50.0f,0xff7f7f7f,15.0f);
			//m_pWServer->Debug_RenderWire(LookPos[0],TargetPos,0xff7f7f7f,15.0f);
			Dir.Normalize();
			if ((Dir * LookPos[1]) >= m_LookAccuracy)
				bLookOk = true;
		}
		if (m_pWServer->GetGameTick() > m_LastLookTick + 5)
			m_LookTime.Reset();
		m_LastLookTick = m_pWServer->GetGameTick();
		if (bLookOk)
		{
			AddLookActive(_iObject,m_LastLookTick);

			if (m_LookTime.IsReset())
				m_LookTime = m_pWServer->GetGameTime();
		}
		else
		{
			m_LookTime.Reset();
		}

		// Send messages and whatever..
		if (!m_LookTime.IsReset() &&
			(m_pWServer->GetGameTime() - m_LookTime).GetTime() >= m_NeededLookTime)
			bOk = true;
	}

	return bOk;
}

void CWObject_Trigger_Look::AddLookActive(int16 _iObject, int32 _LookTick)
{
	// Check current objects
	for (int32 i = 0; i < m_lLookActiveObjects.Len(); i++)
	{
		if (m_lLookActiveObjects[i].m_iLookObject == _iObject)
		{
			m_lLookActiveObjects[i].m_LookTick = _LookTick;
			return;
		}
	}

	struct LookObject Obj;
	Obj.m_iLookObject = _iObject;
	Obj.m_LookTick = _LookTick;
	m_lLookActiveObjects.Add(Obj);
}

void CWObject_Trigger_Look::CheckLookActive(int32 _CurrentTick, bool _bForceSend, int _iForceSend)
{
	// If they haven't gotten updated this tick, send the messages and remove object
	for (int32 i = 0; i < m_lLookActiveObjects.Len(); i++)
	{
		if ((_bForceSend && m_lLookActiveObjects[i].m_iLookObject == _iForceSend) || m_lLookActiveObjects[i].m_LookTick < _CurrentTick)
		{
			for(int32 j = 0; j < m_lLookLeaveMessages.Len(); j++)
				m_lLookLeaveMessages[j].SendMessage(m_iObject, m_lLookActiveObjects[i].m_iLookObject, m_pWServer);

			m_lLookActiveObjects.Del(i);
			i--;
		}
	}
}
void CWObject_Trigger_Look::TriggerOnLeave(int _iSender)
{
	CWObject_Trigger_Look_Parent::TriggerOnLeave(_iSender);
	CheckLookActive(m_pWServer->GetGameTick(),true,_iSender);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_Look, CWObject_Trigger_Look_Parent, 0x0100);

// -------------------------------------------------------------------
//  TRIGGER_HIDEVOLUME
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Trigger_HideVolume, CWObject_Trigger_Ext, 0x0100);

CWObject_Trigger_HideVolume::CWObject_Trigger_HideVolume()
{
	m_HideFlags = FLAGS_CHARACTERS | FLAGS_OBJECTS;
}

void CWObject_Trigger_HideVolume::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyVal = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH2('FLAG','S'): // "LOOKOBJECT"
		{
			static const char *TriggerFlagsStr[] = { "StartHidden", "Characters", "Objects", NULL };
			m_HideFlags = KeyVal.TranslateFlags(TriggerFlagsStr);
			break;
		}
	default:
		{
			CWObject_Trigger_Ext::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Trigger_HideVolume::OnSpawnWorld2()
{
	InitObjectsInside();
}

void CWObject_Trigger_HideVolume::OnRefresh(void)
{
	CWObject_Trigger_Ext::OnRefresh();
	UpdateObjectsInside();
}

bool CWObject_Trigger_HideVolume::PointIsInsideRoom(const CVec3Dfp32& _Pos) const
{
//	M_ASSERT(m_pPhysModel, "Room is missing phys-model! (should have been removed from OnSpawnWorld...)");

	CXR_PhysicsContext PhysContext(GetPositionMatrix());
	CWO_PhysicsState PhysState = GetPhysState();
	int iPhysModel = -1;
	for(int i = 0; i < PhysState.m_nPrim; i++)
	{
		if(PhysState.m_Prim[i].m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL)
			iPhysModel = PhysState.m_Prim[i].m_iPhysModel;
	}
	CXR_Model* pM = m_pWServer->GetMapData()->GetResource_Model(iPhysModel);
	if (pM)
	{
		CXR_PhysicsModel* pPhysModel = pM->Phys_GetInterface();
		pPhysModel->Phys_Init(&PhysContext);
		int nMedium = pPhysModel->Phys_GetMedium(&PhysContext, _Pos);
		enum { MediumMask = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID };
		if (nMedium & MediumMask)
		{
			return true;
		}
	}

	return false;
}

void CWObject_Trigger_HideVolume::InitObjectsInside()
{
	MSCOPESHORT(CWObject_Trigger_HideVolume::InitObjectsInside);

	CWO_PhysicsState Phys = GetPhysState();
	Phys.m_ObjectIntersectFlags = m_IntersectNotifyFlags;

	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pWServer->Selection_AddOriginInside(Selection, GetPositionMatrix(), Phys);
	const int16* pSel = NULL;
	int nSel = m_pWServer->Selection_Get(Selection, &pSel);

	m_lCurrObjectsInside.QuickSetLen(0);
	for (int j = 0; j < nSel; j++)
	{
		int iObj = pSel[j];
		CWObject* pObj = m_pWServer->Object_Get(iObj);
		if (!pObj) 
			continue;

		int iClass = pObj->m_iClass;
		if (iClass == m_iClass)
			continue;

		TryToAddObject(iObj);
	}
}

void CWObject_Trigger_HideVolume::UpdateObjectsInside()
{
	MSCOPESHORT(CWObject_Trigger_HideVolume::UpdateObjectsInside);

	// Objects are added from 'OBJSYSMSG_NOTIFY_INTERSECTION'
	// This function will only check the current list and remove invalid objects
	int* pObjInfo = m_lCurrObjectsInside.GetBasePtr();
	uint nObj = m_lCurrObjectsInside.Len();
	for (uint i = 0; i < nObj; i++)
	{
		CWObject* pObj = m_pWServer->Object_Get(pObjInfo[i]);
		if (pObj)
		{
			CVec3Dfp32 Pos;
			pObj->GetAbsBoundBox()->GetCenter(Pos);
			if (PointIsInsideRoom(Pos))
				continue; // all is ok!
			pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
		}
		pObjInfo[i--] = pObjInfo[--nObj];
	}

	// Update list size & order
	if (nObj != m_lCurrObjectsInside.Len())
	{
		m_lCurrObjectsInside.QuickSetLen(nObj);
	}
}

bool CWObject_Trigger_HideVolume::TryToAddObject(int _iObj)
{
	CWObject *pObj = m_pWServer->Object_Get(_iObj);
	if(!pObj)
		return false;

	uint16 ObjectFlags = pObj->GetPhysState().m_ObjectFlags;
	uint32 CheckFlags = 0;
	if(m_HideFlags & FLAGS_CHARACTERS)
		CheckFlags |= OBJECT_FLAGS_CHARACTER;

	if(m_HideFlags & FLAGS_OBJECTS)
		CheckFlags |= OBJECT_FLAGS_OBJECT;

	if(!(ObjectFlags & CheckFlags))
		return false;

	if(pObj->ClientFlags() & CWO_CLIENTFLAGS_INVISIBLE)
		return false;

	//First check if the object already is inside
	for(int i = 0; i < m_lCurrObjectsInside.Len(); i++)
	{
		if(m_lCurrObjectsInside[i] == _iObj)
			return false;
	}

	uint nLen = m_lCurrObjectsInside.Len();
	m_lCurrObjectsInside.QuickSetLen(++nLen);
	m_lCurrObjectsInside[nLen - 1] = _iObj;
	return true;
}

void CWObject_Trigger_HideVolume::UpdateNoRefreshFlag()
{
	m_ClientFlags &= ~CWO_CLIENTFLAGS_NOREFRESH;
}

aint CWObject_Trigger_HideVolume::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_IMPULSE:
		{
			if(_Msg.m_Param0 == 0)
			{
				m_HideFlags |= FLAGS_HIDDEN;
				CWO_PhysicsState Phys = GetPhysState();

				TSelection<CSelection::LARGE_BUFFER> Selection;
				m_pWServer->Selection_AddOriginInside(Selection, GetPositionMatrix(), Phys);
				const int16* pSel = NULL;
				int nSel = m_pWServer->Selection_Get(Selection, &pSel);
				for(int i = 0; i < nSel; i++)
				{
					CWObject *pObj = m_pWServer->Object_Get(pSel[i]);
					uint16 ObjectFlags = pObj->GetPhysState().m_ObjectFlags;
					uint32 CheckFlags = 0;
					if(m_HideFlags & FLAGS_CHARACTERS)
						CheckFlags |= OBJECT_FLAGS_CHARACTER;

					if(m_HideFlags & FLAGS_OBJECTS)
						CheckFlags |= OBJECT_FLAGS_OBJECT;

					if(ObjectFlags & CheckFlags)
						pObj->ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;
				}
			}
			else
			{
				m_HideFlags  &= ~FLAGS_HIDDEN;
				for(int i = 0; i < m_lCurrObjectsInside.Len(); i++)
				{
					CWObject *pObj = m_pWServer->Object_Get(m_lCurrObjectsInside[i]);
					if(pObj)
						pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
				}
			}
		}
	case OBJSYSMSG_NOTIFY_INTERSECTION: 
		{
			if (!IsDestroyed())
			{
				int iObj = _Msg.m_Param0;
				
				if(TryToAddObject(iObj))
				{
					if(m_HideFlags & FLAGS_HIDDEN)
					{
						CWObject *pObj = m_pWServer->Object_Get(iObj);
						pObj->ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;
					}
				}
			}
			return 0;
		}
	default :
		return CWObject_Trigger_Ext::OnMessage(_Msg);
	}
}



