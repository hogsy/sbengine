#include "WObj_Item.h"
#include "WObj_Game.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_RPG_Item, CWObject_RPG, 0x0100);
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_RPG_CTFItem, CWObject_RPG_Item, 0x0100);

// -------------------------------------------------------------------
//  WObject_RPG_Item
// -------------------------------------------------------------------
CStr CWObject_RPG_Item::RPG_GetMouseOverIdentification(int _Flags)
{
	if (m_ClientFlags & CWO_CLIENTFLAGS_INVISIBLE) return CStr();

	CRPG_Object_Item* pItem = Item_Get();
	if (pItem) return pItem->GetDesc(0);
	return "Invalid RPG-Item.";
}


CRPG_Object_Item* CWObject_RPG_Item::Item_Get()
{
	if (!m_spRPGObject) return NULL;
	if (m_spRPGObject->GetType() == CRPG_Object::TYPE_ITEM ||
		m_spRPGObject->GetType() == CRPG_Object::TYPE_ARTIFACT) 
		return (CRPG_Object_Item*) (CRPG_Object*)m_spRPGObject;
	else
		return NULL;
}

bool CWObject_RPG_Item::Item_CreateFromRPGObject(CRPG_Object_Item* _pItem, const CMat43fp32* _pPos)
{
	if (!_pItem) return false;

	// Model
	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		m_iModel[i] = _pItem->GetModel(i);
		m_Data[0] = _pItem->GetPickupOfs(i).Pack32(256);
//			m_iModel[i] = m_pWServer->GetMapData()->GetResourceIndex_Model(_pItem->String(RPG_ITEM_STR_MODEL0+i));
	}

	// Physics
	if (_pItem->GetNumStrings() > RPG_ITEM_STR_PHYS)
	{
		CWO_PhysicsState Phys;
		Phys_AddPrimitive(_pItem->String(RPG_ITEM_STR_PHYS), &Phys);
		if (_pPos)
		{
			if (!m_pWServer->Object_SetPhysics(m_iObject, Phys, *_pPos)) return false;
		}
		else
		{
			if (!m_pWServer->Object_SetPhysics(m_iObject, Phys)) return false;
		}
	}

	CRPG_Object* p0 = (CRPG_Object_Item*)_pItem;
	CRPG_Object* p1 = m_spRPGObject;

	if (p0 != p1)
//	if (_pItem != m_spRPGObject)
		m_spRPGObject = _pItem->Duplicate();

	return true;
}

bool CWObject_RPG_Item::Item_CreateFromRPGObject(CRPG_Object_Item* _pItem, const CVec3Dfp32& _Pos)
{
	CMat43fp32 Pos;
	Pos.Unit();
	_Pos.SetMatrixRow(Pos, 3);
	return Item_CreateFromRPGObject(_pItem, &Pos);
}


CWObject_RPG_Item::CWObject_RPG_Item()
{
	m_RespawnTime = -1;
	m_RespawnCountdown = 0;
	m_PickupCountdown = RPG_ITEM_DEFAULTPICKUPLATENCY;
}

void CWObject_RPG_Item::OnCreate()
{
	CWObject_Physical::OnCreate();

	// Setup physics
/*	CWO_PhysicsState Phys;
	Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, 0, CVec3Dfp32(9.0f, 9.0, 9.0), 0, 1.0f);
	Phys.m_nPrim = 1;

	m_Phys_Elasticy = 0.4f;
	Phys.m_PhysFlags = OBJECT_PHYSFLAGS_SLIDEABLE | OBJECT_PHYSFLAGS_FRICTION;
	Phys.m_ObjectFlags = OBJECT_FLAGS_TRIGGER | OBJECT_FLAGS_PICKUP;	// | OBJECT_FLAGS_PHYSOBJECT;
	Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_PHYSMODEL;

	if(!m_pWServer->Object_SetPhysics(m_iObject, Phys))
	{
		Destroy();
		LogFile("(CWObject_RPG_Item::OnCreate) WARNING: Unable to set trigger physics state.");
		return;
	}
*/
	m_IntersectNotifyFlags = OBJECT_FLAGS_CHARACTER;
	m_PhysFlags = PHYSICAL_FLAGS_DISABLE | PHYSICAL_FLAGS_MEDIUMACCEL;

	if(m_pWServer->Registry_GetServer()->GetValuei("ROTATEPICKUPS", 1))
		m_ClientFlags |= 256;


	m_RespawnTime = m_pWServer->Registry_GetServer()->GetValuef("DEFAULTRESPAWNTIME", -1) * SERVER_TICKSPERSECOND;
}

void CWObject_RPG_Item::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	switch (_KeyHash)
	{
	case MHASH3('RESP','AWNT','IME'): // "RESPAWNTIME"
		{
			m_RespawnTime = _pKey->GetThisValuef() * SERVER_TICKSPERSECOND;
			break;
		}

	default:
		{
			CWObject_RPG::OnEvalKey(_pKey);
			break;
		}
	}
}

void CWObject_RPG_Item::OnFinishEvalKeys()
{
	CWObject_RPG::OnFinishEvalKeys();

	if (Item_Get())
		if (!Item_CreateFromRPGObject(Item_Get()))
			ConOutL("(CWObject_RPG_Item::OnFinishEvalKeys) Failed to create RPG-item: " + Item_Get()->m_Name);

/*	m_spRPGObject = CRPG_Object::CreateObject(m_Class, m_pWServer);
	if(m_spRPGObject->GetNumAttribs() < m_lValues.Len())
		m_spRPGObject->SetNumAttribs(m_lValues.Len());
	for(int v = 0; v < m_lValues.Len(); v++)
		m_spRPGObject->Attrib(v) = m_lValues[v];

	CWObject_Physical::OnFinishEvalKeys();*/
}

int CWObject_RPG_Item::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
		case OBJSYSMSG_NOTIFY_INTERSECTION:
		case OBJMSG_TRIGGER_INTERSECTION:
		{
			if(m_RespawnCountdown == 0 && m_PickupCountdown == 0)
			{	
				m_PickupCountdown = 1;	// Only try to make a character pick this item up once every frame.
				m_ClientFlags &= ~CWO_CLIENTFLAGS_NOREFRESH;
				int iGame = m_pWServer->Selection_GetSingleTarget((char*)WSERVER_GAMEOBJNAME);

				// URGENTFIXME: Detta suger!
				spCRPG_Object spObj = m_spRPGObject->Duplicate();
//				spCRPG_Object spObj = m_spRPGObject;
				if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_PICKUP, _Msg.m_Param0, 0, m_iObject), iGame))
					if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_RPG_PICKUP, (int)(CRPG_Object *)spObj, m_RespawnTime, m_iObject), _Msg.m_Param0))
					{
//	ConOut("(CWObject_RPG_Item) Picked up.");
//#ifdef NEVER
						int iSound = Item_Get()->Item_GetPickupSoundIndex();
						if(iSound != -1)
							m_pWServer->Sound_At(GetPosition(), iSound, 0);

						m_ClientFlags |= CWO_CLIENTFLAGS_INVISIBLE;
						m_DirtyMask |= CWO_DIRTYMASK_GENERAL;
						if(m_RespawnTime == -1)
							Destroy();
						else
						{
//							m_spRPGObject = m_spRPGObject->Duplicate();
							m_RespawnCountdown = m_RespawnTime;
						}
//#endif
					}
			}
			return 0;
		}
		default:
			return CWObject_Physical::OnMessage(_Msg);
	}
}

void CWObject_RPG_Item::OnRefresh()
{
	if (m_PickupCountdown) m_PickupCountdown--;

	if(m_RespawnCountdown <= 0)
	{
		m_ClientFlags &= ~CWO_CLIENTFLAGS_INVISIBLE;
		m_DirtyMask |= CWO_DIRTYMASK_GENERAL;

		if (!m_PickupCountdown) m_ClientFlags |= CWO_CLIENTFLAGS_NOREFRESH;
	}
	else
		m_RespawnCountdown--;

//	m_pWServer->Object_MovePhysical(m_iObject);

	CWObject_Physical::OnRefresh();
}

void CWObject_RPG_Item::OnLoad(CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject_RPG::OnLoad(_pFile, _pWData, _Flags);
	_pFile->ReadLE(m_RespawnTime);
	_pFile->ReadLE(m_RespawnCountdown);
}

void CWObject_RPG_Item::OnSave(CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject_RPG::OnSave(_pFile, _pWData, _Flags);
	_pFile->WriteLE(m_RespawnTime);
	_pFile->WriteLE(m_RespawnCountdown);
}


void CWObject_RPG_Item::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	CMat43fp32 MatIP;
	fp32 IPTime = _pWClient->GetInterpolateTime() / fp32(SERVER_TIMEPERFRAME);
	_pWClient->Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

	if(_pObj->m_ClientFlags & 256)
	{
		CMat43fp32 RotMat, Res;
		RotMat.SetZRotation(-(_pWClient->GetGameTick() + IPTime) * SERVER_TIMEPERFRAME * 0.25f);
		RotMat.Multiply(MatIP, Res);
		MatIP = Res;
	}

	// Get total bounding box
/*	CBox3Dfp32 Bound;
	Bound.m_Min = 1000000.0;
	Bound.m_Max = -1000000.0;

	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		int iModel = _pObj->m_iModel[i];
		CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(iModel);

		if (pModel)
		{
			CBox3Dfp32 Box;
			pModel->GetBound_Box(Box);
			Bound.Expand(Box);
		}
	}*/

	{
/*		CVec3Dfp32 Center;
		Bound.GetCenter(Center);
		Center.MultiplyMatrix3x3(MatIP);
		CMat4Dfp32 Mat(MatIP);
		CVec3Dfp32::GetMatrixRow(Mat, 3) -= Center;
		Mat.k[3][2] += 8;*/

		CVec3Dfp32 Ofs;
		Ofs.Unpack32(_pObj->m_Data[0], 256);
		CVec3Dfp32::GetMatrixRow(MatIP, 3) += Ofs;

		for(int i = 0; i < CWO_NUMMODELINDICES; i++)
		{
			int iModel = _pObj->m_iModel[i];
			CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(iModel);
			if (pModel)
			{
				_pEngine->Render_AddModel(pModel, MatIP, _pObj->GetDefaultAnimState(_pWClient));
//				ConOut(CStrF("Obj %d, Pickup model %d, %d", _pObj->m_iObject, i, iModel));
			}
		}
	}
}

// -------------------------------------------------------------------
//  CWObject_RPG_CTFItem
// -------------------------------------------------------------------
enum
{
	RPG_CTFITEM_TEAM = RPG_ITEM_NUMATTRIB,
	RPG_CTFITEM_PICKUP,
	RPG_CTFITEM_NUMATTRIB
};

class CRPG_Object_CTFItem : public CRPG_Object_Item
{
	MRTC_DECLARE;
public:

	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		switch (_KeyHash)
		{
		case MHASH4('RPG_','CTFI','TEM_','TEAM'): // "RPG_CTFITEM_TEAM"
			{
				Attrib(RPG_CTFITEM_TEAM) = _pKey->GetThisValuei();
				break;
			}
		default:
			{
				break;
			}
		}
		switch (_KeyHash)
		{
		case MHASH5('RPG_','CTFI','TEM_','PICK','UP'): // "RPG_CTFITEM_PICKUP"
			{
				Attrib(RPG_CTFITEM_PICKUP) = _pKey->GetThisValuei();
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

	virtual void OnCreate()
	{
		CRPG_Object_Item::OnCreate();
		SetNumAttribs(RPG_CTFITEM_NUMATTRIB);
	}

	virtual CRPG_Attrib &CTFItem_Team() { return Attrib(RPG_CTFITEM_TEAM); };
	virtual CRPG_Attrib &CTFItem_Pickup() { return Attrib(RPG_CTFITEM_PICKUP); };
};

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_CTFItem, CRPG_Object_Item);

// -------------------------------------------------------------------
void CWObject_RPG_CTFItem::OnCreate()
{
	CWObject_RPG_Item::OnCreate();

	m_Resting = 0;
	m_Team = -1;
}

void CWObject_RPG_CTFItem::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	switch (_KeyHash)
	{
	case MHASH1('TEAM'): // "TEAM"
		{
			m_Team = _pKey->GetThisValuef();
			break;
		}
	default:
		{
			CWObject_RPG_Item::OnEvalKey(_pKey);
			break;
		}
	}
}


int CWObject_RPG_CTFItem::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
		case OBJSYSMSG_NOTIFY_INTERSECTION:
		case OBJMSG_TRIGGER_INTERSECTION:
			{
				if(m_RespawnCountdown == 0 && m_PickupCountdown == 0)
				{	
					int iGame = m_pWServer->Selection_GetSingleTarget((char*)WSERVER_GAMEOBJNAME);

					CRPG_Object_Item* pItem = Item_Get();
					if (pItem)
					{
						int PickupID = pItem->Attrib(RPG_CTFITEM_TEAM);
						if (m_Team >= 0) PickupID |= 0x0100;

						switch(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_PICKUP, _Msg.m_Param0, PickupID, m_iObject), iGame))
						{
						case 1 :
							if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_RPG_PICKUP, (int)(CRPG_Object *)m_spRPGObject, m_RespawnTime, m_iObject), _Msg.m_Param0))
							{
								m_ClientFlags |= CWO_CLIENTFLAGS_INVISIBLE;
								m_DirtyMask |= CWO_DIRTYMASK_GENERAL;
								m_RespawnCountdown = 1;
							}
							break;

						case 2 :
							m_pWServer->Object_Destroy(m_iObject);
							return 0;

						default :
							{
							}
						}
					}
				}
				return 0;
			}

		case OBJMSG_PHYSICS_DAMAGE :
			{
				CWObject* pObj = m_pWServer->Object_Get(_Msg.m_iSender);
				CRPG_Object_Item* pItem = Item_Get();
				if (pItem && (!pObj || (pObj && !MRTC_ISKINDOF(pObj, "CWObject_Character") )))
				{
					int iGame = m_pWServer->Selection_GetSingleTarget((char*)WSERVER_GAMEOBJNAME);

					int PickupID = pItem->Attrib(RPG_CTFITEM_TEAM);
					switch(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_LOSTPICKUP, m_iObject, PickupID), iGame))
					{
					case 2 : m_pWServer->Object_Destroy(m_iObject); return 0;
					default : ;
					}
					return 0;
				}
				else
					return CWObject_RPG_Item::OnMessage(_Msg);
			}
	
		case OBJMSG_IMPULSE :
			{
				m_RespawnCountdown = _Msg.m_Param0;
				return 0;
			}

		default:
			return CWObject_RPG_Item::OnMessage(_Msg);
	}
}

void CWObject_RPG_CTFItem::OnRefresh()
{
	if (m_PickupCountdown) m_PickupCountdown--;
	if(m_RespawnCountdown <= 0)
	{
		m_ClientFlags &= ~CWO_CLIENTFLAGS_INVISIBLE;
		m_DirtyMask |= CWO_DIRTYMASK_GENERAL;
	}
	else
		m_RespawnCountdown--;

	CWObject_Physical::OnRefresh();
}

