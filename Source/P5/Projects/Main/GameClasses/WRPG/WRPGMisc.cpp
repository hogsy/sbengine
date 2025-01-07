#include "PCH.h"

#include "WRPGSpell.h"
#include "WRPGChar.h"
#include "../WObj_RPG.h"
#include "../WObj_Weapons/WObj_Spells.h"
#include "../WObj_Weapons/WObj_ClientDebris.h"
#include "MFloat.h"
#include "../WObj_Char.h"
#include "WRPGAmmo.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"
#include "../WObj_AI/AI_Knowledgebase.h"

//-------------------------------------------------------------------
//- CRPG_Object_Pickup ----------------------------------------------
//-------------------------------------------------------------------

class CRPG_Object_Pickup : public CRPG_Object_Item
{
public:
	CRPG_Object_Pickup()
	{
		MAUTOSTRIP(CRPG_Object_Pickup_ctor, MAUTOSTRIP_VOID);
		m_bMultiple = false;
	}
	MRTC_DECLARE;

	bool m_bMultiple;
	TArray<int16> m_lCombineFrom;
	CStr m_CombineTo;
	CWO_SimpleMessage m_Msg_Pickup;


	virtual void OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
	{
		MAUTOSTRIP(CRPG_Object_Pickup_OnIncludeClass, MAUTOSTRIP_VOID);
		CRPG_Object_Item::OnIncludeClass(_pReg, _pMapData, _pWServer);

		IncludeRPGClassFromKey("COMBINETO", _pReg, _pMapData, _pWServer);
	}

	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		MAUTOSTRIP(CRPG_Object_Pickup_OnEvalKey, false);
		
		CStr KeyName = _pKey->GetThisName();
		CStr KeyValue = _pKey->GetThisValue();
		
		int KeyValuei = KeyValue.Val_int();
		
		switch (_KeyHash)
		{
		case MHASH3('DONT','DEST','ROY'): // "DONTDESTROY"
			{
				m_bMultiple = KeyValuei != 0;
				break;
			}
		
		case MHASH3('COMB','INET','O'): // "COMBINETO"
			{
				m_CombineTo = KeyValue;
				break;
			}

		case MHASH3('COMB','INEF','ROM'): // "COMBINEFROM"
			{
				CFStr St = KeyValue;
				while(St != "")
				{
					int iType = St.GetStrMSep(",; ").Val_int();
					if(iType > 0)
						m_lCombineFrom.Add(iType);
				}
				break;
			}
		
		case MHASH5('MSG_','ONPI','CKUP','_DON','TUSE'): // "MSG_ONPICKUP_DONTUSE"
			{
				m_Msg_Pickup.Parse(KeyValue, m_pWServer);
				break;
			}

		default:
			{
				return CRPG_Object_Item::OnEvalKey(_KeyHash, _pKey);
				break;
			}
		}
		
		return true;
	}

	virtual int OnPickup(int _iObject, CRPG_Object *_pRoot, bool _bNoSound, int _iSender, bool _bNoPickupIcon = false)
	{
		MAUTOSTRIP(CRPG_Object_Pickup_OnPickup, MAUTOSTRIP_VOID);
		m_Msg_Pickup.SendMessage(_iObject, _iObject, m_pWServer);

		return CRPG_Object_Item::OnPickup(_iObject, _pRoot, _bNoSound, _iSender, _bNoPickupIcon);
	}

	virtual void OnFinishEvalKeys()
	{
		MAUTOSTRIP(CRPG_Object_Pickup_OnFinishEvalKeys, MAUTOSTRIP_VOID);
		if(m_CombineTo != "")
			m_Flags |= RPG_ITEM_FLAGS_AUTOACTIVATE;
	}
	
	virtual bool Activate(const CMat4Dfp32 &, CRPG_Object *_pRoot, int _iObject, int _Input)
	{
		MAUTOSTRIP(CRPG_Object_Pickup_Activate, false);

		if(_pRoot->GetType() == CRPG_Object::TYPE_CHAR && m_CombineTo != "")
		{
			CRPG_Object_Char *pChar = (CRPG_Object_Char *)_pRoot;
			
			spCRPG_Object spObj = CreateObject(m_CombineTo, m_pWServer);
			if(spObj)
			{
				CRPG_Object_Pickup *pPickup = safe_cast<CRPG_Object_Pickup >((CRPG_Object *)spObj);
				if(pPickup)
				{
					CRPG_Object *lpItems[32];
					int i;
					for(i = 0; i < pPickup->m_lCombineFrom.Len(); i++)
					{
						if(m_iItemType == pPickup->m_lCombineFrom[i])
							lpItems[i]  = this;
						else
							lpItems[i] = pChar->FindItemByType(pPickup->m_lCombineFrom[i]);

						if(!lpItems[i])
							break;
					}
					if(i == pPickup->m_lCombineFrom.Len())
					{
						// Char has all pickups needed for combination
						for(int i = 0; i < pPickup->m_lCombineFrom.Len(); i++)
							SendMsg(_iObject, OBJMSG_CHAR_REMOVEITEM, 0, 0, _iObject, 0, 0, 0, (char *)lpItems[i]->m_Name.Str(), lpItems[i]->m_Name.Len());
						SendMsg(_iObject, OBJMSG_CHAR_PICKUPITEM, 0, 0, _iObject, 0, 0, 0, (char *)m_CombineTo.Str(), m_CombineTo.Len());
						return true;
					}
				}
			}
		}
		return false;
	}

	virtual bool WorldUse(int _iObject, int _iTrigger, CRPG_Object *_pRoot)
	{
		MAUTOSTRIP(CRPG_Object_PickupWorldUse_, false);
		SendMsg(_iTrigger, OBJMSG_IMPULSE, 0, 0, _iObject);

		return m_bMultiple;
	}
};

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Pickup, CRPG_Object_Item);

#ifndef M_DISABLE_TODELETE
//-------------------------------------------------------------------
//- CRPG_Object_Potion ----------------------------------------------
//-------------------------------------------------------------------

class CRPG_Object_Potion : public CRPG_Object_Item
{
	MRTC_DECLARE;

	int m_Health;

	void OnCreate()
	{
		MAUTOSTRIP(CRPG_Object_Potion_OnCreate, MAUTOSTRIP_VOID);
		CRPG_Object_Item::OnCreate();

		m_Health = 0;
	}

	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
	{
		
		MAUTOSTRIP(CRPG_Object_Potion_OnEvalKey, false);
		switch (_KeyHash)
		{
		case MHASH4('POTI','ON_H','EALT','H'): // "POTION_HEALTH"
			{
				m_Health = _pKey->GetThisValuei();
				break;
			}

		default:
			{
				return CRPG_Object_Item::OnEvalKey(_pKey);
				break;
			}
		}

		return true;
	}

	virtual bool IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input = 1)
	{
		MAUTOSTRIP(CRPG_Object_Potion_IsActivatable, false);
		if (m_Flags & RPG_ITEM_FLAGS_ACTIVATED)
			return false;

		CRPG_Object_Char* pChar = GetChar(_pRoot);
		if (pChar == NULL)
			return false;

		return (pChar->Health() < pChar->MaxHealth());
	}

	virtual bool OnActivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
	{
		MAUTOSTRIP(CRPG_Object_Potion_OnActivate, false);
		if(m_Health > 0)
			SendMsg(_iObject, OBJMSG_CHAR_ADDHEALTH, m_Health);

		return true;
	}

	virtual bool MergeItem(int _iObject, CRPG_Object_Item *_pObj)
	{
		MAUTOSTRIP(CRPG_Object_Potion_MergeItem, false);
		CRPG_Object_Potion *pPotion = safe_cast<CRPG_Object_Potion >(_pObj);
		if(pPotion)
		{
			m_NumItems += pPotion->m_NumItems;
			return true;
		}
		return false;
	}
	
/*	virtual uint16 GetActivateAnim()
	{
//		return -1;
		return m_iAnimAttack;
	}*/
};

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Potion, CRPG_Object_Item);
#endif


#ifndef M_DISABLE_TODELETE

//-------------------------------------------------------------------
//- CRPG_Object_Diversion -------------------------------------------
//-------------------------------------------------------------------

class CRPG_Object_Diversion : public CRPG_Object_Item
{
	MRTC_DECLARE;

	int m_LastActivationTick;
	int m_ActivationDelayTicks;

public:
	virtual void OnCreate();
	virtual bool IsEquippable(CRPG_Object* _pRoot);
	virtual bool IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input = 1);
	virtual bool OnActivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
};

void CRPG_Object_Diversion::OnCreate()
{
	CRPG_Object_Item::OnCreate();

	m_LastActivationTick = -1;
	m_ActivationDelayTicks = 10;
}

bool CRPG_Object_Diversion::IsEquippable(CRPG_Object* _pRoot)
{
	return true;
}

bool CRPG_Object_Diversion::IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input)
{
	if (m_LastActivationTick == -1)
		return true;

	int CurrentTick = m_pWServer->GetGameTick();
	if (m_LastActivationTick + m_ActivationDelayTicks < CurrentTick)
		return true;

	return false;
}

bool CRPG_Object_Diversion::OnActivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	m_LastActivationTick = m_pWServer->GetGameTick();
	CVec3Dfp32 Fwd(4096.0f, 0.0f, 0.0f);

	CVec3Dfp32 Start = CVec3Dfp32::GetRow(_Mat, 3);
	CVec3Dfp32 Stop = Fwd * _Mat;
	m_pWServer->Debug_RenderWire(Start, Stop, 0xffa0a0a0, 10.0f, true);


	CCollisionInfo Info;
	if(m_pWServer->Phys_IntersectLine(Start, Stop, 0, OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_CHARACTER, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &Info) && Info.m_bIsValid)
	{
		ConOut("Collision");

		// Collision
		m_pWServer->Debug_RenderWire(Start, Info.m_Pos, 0xffa0a0ff, 10.0f, true);

		// Create AI phenomenon
		/*
		CAI_Phenomenon_Global FenShui(m_pWServer, CAI_Phenomenon::DISTURBANCE, Info.m_Pos, m_LastActivationTick+1, 0.0f, 1024);
		CAI_KnowledgeBase::Global_NewPhenomenon(FenShui);
		*/
	};

	return true;
}

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Diversion, CRPG_Object_Item);
#endif

//-------------------------------------------------------------------
//- CRPG_Object_Monitor -------------------------------------------
//-------------------------------------------------------------------
#ifndef M_DISABLE_TODELETE

class CRPG_Object_SurveillanceMonitor : public CRPG_Object_Item
{
	MRTC_DECLARE;

	bool m_bActive;

public:
	virtual void OnCreate();
	virtual bool IsEquippable(CRPG_Object* _pRoot);
	virtual bool IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input = 1);
	virtual bool OnActivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool Deactivate(const CMat4Dfp32 &, CRPG_Object *_pRoot, int _iObject, int _Input);
};

void CRPG_Object_SurveillanceMonitor::OnCreate()
{
	CRPG_Object_Item::OnCreate();
	m_bActive = false;
}

bool CRPG_Object_SurveillanceMonitor::IsEquippable(CRPG_Object* _pRoot)
{
	return true;
}

bool CRPG_Object_SurveillanceMonitor::IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input)
{
	if (m_bActive)
		return false;

	return true;
}

bool CRPG_Object_SurveillanceMonitor::OnActivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	ConOut("Activate");
	m_bActive = true;

	int iCameraObject = 0;

	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pWServer->Selection_AddClass(Selection, "detector_camera");
	const int16* pSelections;
	int nSelections = m_pWServer->Selection_Get(Selection, &pSelections);

	if (nSelections > 0)
	{
		iCameraObject = pSelections[0];
	};

	if (iCameraObject != 0)
	{
		CWObject_Message Msg(OBJMSG_CHAR_SETCURRENTSURVEILLANCECAMERA, iCameraObject);
		m_pWServer->Message_SendToObject(Msg, _iObject);
	};

	return true;
}

bool CRPG_Object_SurveillanceMonitor::Deactivate(const CMat4Dfp32 &, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	m_bActive = false;
	ConOut("Deactivate");

	CWObject_Message Msg(OBJMSG_CHAR_SETCURRENTSURVEILLANCECAMERA, 0);
	m_pWServer->Message_SendToObject(Msg, _iObject);
	
	return true;
}

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_SurveillanceMonitor, CRPG_Object_Item);
#endif
