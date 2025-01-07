/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_Room.cpp

	Author:			Anton Ragnarsson

	Copyright:		Copyright Starbreeze AB 2005

	History:		
		050303:		Created File
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_Room.h"
#include "WObj_ScenePoint.h"
#include "WObj_Object.h"
#include "../WNameHash.h"

//#define ROOM_DEBUG_ENABLE 
#ifdef ROOM_DEBUG_ENABLE
# define ROOM_DEBUG(x) { ConOutL(CStrF##x); M_TRACE(CStrF##x + "\n"); }
#else
# define ROOM_DEBUG(x) { }
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Message params
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum 
{ 
	TYPE_LAMPS =		M_Bit(0), 
	TYPE_OBJECTS =		M_Bit(1), 
	TYPE_CHARACTERS =	M_Bit(2), 
	TYPE_CORPSES =		M_Bit(3), 
	TYPE_MODELS =		M_Bit(4),
	TYPE_SCENEPOINTS =	M_Bit(5), 
	TYPE_WAITSPAWNED =	M_Bit(6),

	CHAR_CIVS =			M_Bit(7),
	CHAR_BADGUYS =		M_Bit(8), 
	CHAR_PLAYERS =		M_Bit(9),

	SELECT_ALL = 0, 
	SELECT_RANDOM, 
	SELECT_OLDEST, 
	SELECT_NEWEST, 
	SELECT_RANDOM2,
};

struct TypeParams
{
	uint16 m_TypeFlags;
	uint16 m_SPTypes;

	TypeParams() {}
	TypeParams(const CWObject_Message& _Msg)
	{
		CStr Str = (const char*)_Msg.m_pData;
		ParseFlags1(Str);
		m_SPTypes = 0;
	}

	void ParseFlags1(CStr& _Str)
	{
		static const char* Translate_TypeFlags[] = { "Lamps", "Objects", "Characters", "Corpses", "Models", "ScenePoints", "WaitSpawned", NULL };
		static const char* Translate_CharFlags[] = { "Civilians", "BadGuys", "Players", NULL };

		m_TypeFlags = _Str.GetStrSep("/").TranslateFlags(Translate_TypeFlags);
		m_TypeFlags |= _Str.GetStrSep("/").TranslateFlags(Translate_CharFlags) << 6;
	}

	void ParseFlags2(CStr& _Str)
	{
		m_SPTypes = _Str.GetStrSep("/").TranslateFlags(CWO_ScenePoint::ms_lTranslateType);
	}
};

struct BroadcastParams : TypeParams
{
	int8 m_Select;
	uint8 m_Num;

	BroadcastParams(const CWObject_Message& _Msg)
	{
		CStr Str = (const char*)_Msg.m_pData;
		ParseSelect(Str);
		ParseFlags1(Str);
		ParseNum(Str, _Msg.m_Param0);
		ParseFlags2(Str);
	}

	void ParseSelect(CStr& _Str)
	{
		static const char* Translate_Select[] = { "All", "Random", "Oldest", "Newest", "Random2", NULL };
		m_Select = _Str.GetStrSep("/").TranslateInt(Translate_Select);
	}

	void ParseNum(CStr& _Str, int _Param0)
	{
		int nMin = _Str.GetStrSep(",").Val_int();
		int nMax = _Str.GetStrSep("/").Val_int();
		if (m_Select == SELECT_RANDOM2)
		{
			// Set 'num' to random value within specified range
			m_Num = nMin + TruncToInt((nMax - nMin + 1) * MRTC_GetRand()->GenRand1Exclusive_fp32());
		}
		else
		{	// Set 'num' to the value specified
			m_Num = _Param0;
		}
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Room
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Room, CWObject_Trigger_Ext, 0x0100);

enum
{
	class_Room =				MHASH4('CWOb','ject','_','Room'),
	class_Object_Lamp =			MHASH6('CWOb','ject','_','Obje','ct_','Lamp'),
	class_CharPlayer =			MHASH6('CWOb','ject','_','Char','Play','er'),
	class_CharNPC =				MHASH5('CWOb','ject','_','Char','NPC'),
	class_Model =				MHASH5('CWOb','ject','_','Mode','l'),  
	class_Model_Ext =			MHASH6('CWOb','ject','_','Ext_','Mode','l'),
};

void CWObject_Room::OnCreate()
{
	m_pPhysModel = NULL;
	m_Data[DATA_FLAGS] = 0;
	INVALIDATE_POS(m_SafePos);
	m_bIsBroadcasting = false;

	parent::OnCreate();

	m_IntersectNotifyFlags = OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PLAYER | OBJECT_FLAGS_PHYSOBJECT;

	m_ClientFlags |= CWO_CLIENTFLAGS_INVISIBLE;
	m_ClientFlags |= CWO_CLIENTFLAGS_NOREFRESH;
}


void CWObject_Room::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
//	const CStr KeyName = _pKey->GetThisName();
	switch (_KeyHash)
	{
	case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
		{
			CFStr ModelName = CFStrF("$WORLD:%d", _pKey->GetThisValuei());
			m_iModel[0] = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
			if (!m_iModel[0])
				Error("OnEvalKey", "Failed to acquire bsp-model.");
			break;
		}

	case MHASH3('ROOM','FLAG','S'): // "ROOMFLAGS"
		{
			static const char* RoomFlagsTranslate[] = { "waitspawn", NULL };
			m_Data[DATA_FLAGS] = _pKey->GetThisValue().TranslateFlags(RoomFlagsTranslate);
			break;
		}

	default:
		parent::OnEvalKey(_KeyHash, _pKey);
	}
}


void CWObject_Room::OnSpawnWorld()
{
	MSCOPESHORT(CWObject_Room::OnSpawnWorld);

	parent::OnSpawnWorld();

	int iPhysModel = m_iModel[0];
	if (iPhysModel != 0)
	{
		// Setup physics
		CWO_PhysicsState Phys;
		Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_PHYSMODEL, iPhysModel, 0, 0);
		Phys.m_nPrim = 1;

		Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_ROTATION;
		Phys.m_ObjectFlags |= OBJECT_FLAGS_TRIGGER;
		Phys.m_ObjectIntersectFlags = 0;

		if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
			ConOutL("§cf80WARNING: Unable to set room physics state.");
	}

	// Store direct pointer to phys model
	const CWO_PhysicsState& Phys = GetPhysState();
	if (Phys.m_nPrim && Phys.m_Prim[0].m_iPhysModel)
	{
		int iPhysModel = Phys.m_Prim[0].m_iPhysModel;
		CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(iPhysModel);
		if (pModel)
			m_pPhysModel = pModel->Phys_GetInterface();
	}

	// Must have a valid physmodel
	if (!m_pPhysModel)
	{
		ROOM_DEBUG(("Room %d ('%s') is missing a valid phys-model!", m_iObject, GetName()));
		m_pWServer->Object_Destroy(m_iObject);
	}
}


void CWObject_Room::OnSpawnWorld2()
{
	bool bSpawn = IsSpawned();
	Spawn(bSpawn);
}


void CWObject_Room::Spawn(bool _bSpawn)
{
	if (_bSpawn)
	{
		// spawn
		m_Data[DATA_FLAGS] &= ~ROOMFLAGS_WAITSPAWNED;

		//
		// When a room is spawned, we go through all door scenepoints and store those who lies within the room
		// (and also store the matching door scenepoint in neighbour room)
		//
		FindAllDoors();

		// Check all objects currently inside room
		InitObjectsInside();
	}
	else
	{
		// unspawn
		m_Data[DATA_FLAGS] |= ROOMFLAGS_WAITSPAWNED;

		TAP<ObjectInfo> pObjInfo = m_lCurrObjectsInside;
		for (uint i = 0; i < pObjInfo.Len(); i++)
			m_pWServer->Object_RemoveListener(pObjInfo[i].m_iObject, m_iObject);

		m_lCurrObjectsInside.Clear();
	}
}


void CWObject_Room::FindAllDoors()
{
	CWObject_ScenePointManager* pSPM = GetSPM();
	M_ASSERT(pSPM, "No scene point manager?!");
	if (!pSPM)
		return;

	const fp32 MaxDoorDistance = 32; // Connecting door scenepoints must be within 1 meter from eachother
	CBox3Dfp32 Bound = *GetAbsBoundBox();
	Bound.Grow(MaxDoorDistance);

	pSPM->Selection_Clear();
	pSPM->Selection_AddBox_Approx(Bound, CWO_ScenePoint::DOOR);
	TAP<const CWO_ScenePoint* const> pDoorScenePoints = pSPM->Selection_Get();

	for (uint i = 0; i < pDoorScenePoints.Len(); i++)
	{
		const CWO_ScenePoint* pPoint = pDoorScenePoints[i];
		const CVec3Dfp32& Pos = pPoint->GetPos();
		if (PointIsInsideRoom(Pos))
		{
			ROOM_DEBUG(("ROOM: sp '%s' found in room '%s'", pPoint->GetName().Str(), GetName()));
			// Ok, door scenepoint is inside this room. Let's try to find a corresponding scenepoint in other room
			for (uint j = 0; j < pDoorScenePoints.Len(); j++)
			{
				const CWO_ScenePoint* pOtherPoint = pDoorScenePoints[j];
				const CVec3Dfp32& OtherPos = pOtherPoint->GetPos();

				if (PointIsInsideRoom(OtherPos))
					continue;

				fp32 DistanceSqr = Pos.DistanceSqr(OtherPos);
				if (DistanceSqr < Sqr(MaxDoorDistance))
				{
					ROOM_DEBUG((" found matching sp '%s' in other room", pOtherPoint->GetName().Str()));

					// ok, I guess this is it.. 
					int ID1 = pSPM->GetScenePointIndex(pPoint);	//TODO: Use ID instead of index
					int ID2 = pSPM->GetScenePointIndex(pOtherPoint);

					m_Doors.Add(IDPair(ID1, ID2));
				}
			}
		}
	}
}


void CWObject_Room::InitObjectsInside()
{
	MSCOPESHORT(CWObject_Room::InitObjectsInside);

	// Add objects
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

		if (pObj->IsClass(class_Room))
			continue;

		AddObject(*pObj, false);
	}
	SortObjects();
}


void CWObject_Room::SortObjects()
{
//	M_TRACE("CWObject_Room::SortObjects, m_iObject = %d, nObj = %d\n", m_iObject, m_lCurrObjectsInside.Len());
	RadixSort(m_lCurrObjectsInside.GetBasePtr(), m_lCurrObjectsInside.Len());
}


void CWObject_Room::OnRefresh()
{
	MSCOPESHORT(CWObject_Room::OnRefresh);

	m_ClientFlags &= ~CWO_CLIENTFLAGS_NOREFRESH;

	// loop through objects to check for objects no longer in the room
	bool bReSort = false;
	TAP<ObjectInfo> pList = m_lCurrObjectsInside;
	for (uint i = 0; i < pList.Len(); i++)
	{
		ObjectInfo x = pList[i];
		if (!x.m_bValid && !x.m_bChecked)
		{
			// need to check if object is inside
			CWObject* pObj = m_pWServer->Object_Get(x.m_iObject);
			M_ASSERTHANDLER(pObj, "should've been removed from list!", continue);

			CVec3Dfp32 Pos;
			pObj->GetAbsBoundBox()->GetCenter(Pos);
			if (!PointIsInsideRoom(Pos))
			{
				// remove object from list
				RemoveObject(&pList[i--]);
				pList = m_lCurrObjectsInside; // update TAP<>
				bReSort = true;
				continue;
			}
		}
		// object is still in room
		pList[i].m_bChecked = true;
		pList[i].m_bValid = false;
	}

	// Update list size & order
	if (bReSort)
		SortObjects();
}


bool CWObject_Room::TestObjectType(const CWObject& _Obj, uint _TypeFlags) const
{
	uint16 CurrType = 0;

	if (_Obj.IsClass(class_CharPlayer))
	{
		CurrType = TYPE_CHARACTERS | CHAR_PLAYERS;
	}
	else if (_Obj.IsClass(class_CharNPC))
	{
		if (CWObject_Character::Char_GetPhysType(&_Obj) == PLAYER_PHYS_DEAD)
		{
			CurrType = TYPE_CORPSES;
		}
		else
		{
			const CWObject_Character* pChar = safe_cast<const CWObject_Character>(&_Obj);
			if ((pChar->m_Flags & PLAYER_FLAGS_WAITSPAWN) && !(_TypeFlags & TYPE_WAITSPAWNED))
				return false;

			const CAI_Core* pAI = pChar->m_spAI;
			if (pAI->m_UseFlags & CAI_Core::USE_NOROOMSELECT)
				return false;

			int AICharClass = pAI->m_CharacterClass;
			if (AICharClass == CAI_Core::CLASS_CIV || AICharClass == CAI_Core::CLASS_TOUGHGUY)
			{
				CurrType = TYPE_CHARACTERS | CHAR_CIVS;
			}
			else if (AICharClass == CAI_Core::CLASS_BADGUY)
			{
				CurrType = TYPE_CHARACTERS | CHAR_BADGUYS;
			}
		}
	}
	else if (_Obj.GetPhysState().m_ObjectFlags & OBJECT_FLAGS_OBJECT)
	{
		const CWObject_Object* pObject = safe_cast<const CWObject_Object>(&_Obj);
		if ((pObject->GetFlags() & CWObject_Object::FLAGS_WAITSPAWN) && !(_TypeFlags & TYPE_WAITSPAWNED))
			return false;

		if (_Obj.IsClass(class_Object_Lamp))
			CurrType = TYPE_LAMPS;
		else
			CurrType = TYPE_OBJECTS;
	}

	return (CurrType != 0 && (CurrType & _TypeFlags) == CurrType);
}


bool CWObject_Room::TestScenePoint(const CWO_ScenePoint& _SP, uint _TypeFlags, uint _SPFlags) const
{
	// if waitspawned is set, select both spawned and non-spawned
	if (!(_TypeFlags & TYPE_WAITSPAWNED) && !_SP.IsSpawned())
		return false;

	// dynamic flag must match   (you either select all dynamic or all non-dynamic)
	uint x = _SP.GetType();
	if ((x ^ _SPFlags) & CWO_ScenePoint::DYNAMIC)
		return false;

	if (!(x & _SPFlags & CWO_ScenePoint::TYPE_MASK))
		return false;

	return true;
}



void CWObject_Room::CreateBroadcastSelection(const CWObject_Message& _Msg)
{
	MSCOPESHORT(CWObject_Room::CreateBroadcastSelection);

	m_lCurrTargets.QuickSetLen(0);
	BroadcastParams params(_Msg);
	if (params.m_Select < 0 || (params.m_Select != SELECT_ALL && params.m_Num == 0))
		return;

	uint nObj = m_lCurrObjectsInside.Len();
	const ObjectInfo* lObjInfo = m_lCurrObjectsInside.GetBasePtr();

	// Add all intersecting objects (of matching type) to target list
	m_lCurrTargets.QuickSetLen(0);
	for (uint i = 0; i < nObj; i++)
	{
		int iObj = lObjInfo[i].m_iObject;
		CWObject* pObj = m_pWServer->Object_Get(iObj);
		if (!pObj) 
			continue;

		// check if type matches input
		if (!TestObjectType(*pObj, params.m_TypeFlags))
			continue;

		m_lCurrTargets.Add(lObjInfo[i]);
	}

	// Add models  (NOTE: not sure if we want to add models to the 'm_lCurrObjectsInside' list, or if adding them on the fly is better...)
	if (params.m_TypeFlags & TYPE_MODELS)
	{
		TSelection<CSelection::MEDIUM_BUFFER> Sel;
		m_pWServer->Selection_AddOriginInside(Sel, GetPositionMatrix(), GetPhysState());
		for (uint i = 0; i < Sel.Len(); i++)
		{
			CWObject* pObj = m_pWServer->Object_Get(Sel[i]);
			if (pObj && (pObj->IsClass(class_Model) || pObj->IsClass(class_Model_Ext)))
			{
				ObjectInfo tmp = { Sel[i], 1, 1, 0, 0 };
				m_lCurrTargets.Add(tmp);
			}
		}
	}

	// Add scenepoints
	if (params.m_TypeFlags & TYPE_SCENEPOINTS)
	{
		CWObject_ScenePointManager* pSPM = GetSPM();
		if (pSPM)
		{
			pSPM->Selection_AddInRoom(*this);
			TAP<CWO_ScenePoint* const> pScenepoints = pSPM->Selection_Get();
			for (uint i = 0; i < pScenepoints.Len(); i++)
			{
				const CWO_ScenePoint* pSP = pScenepoints[i];
				if (!TestScenePoint(*pSP, params.m_TypeFlags, params.m_SPTypes))
					continue;

				ObjectInfo tmp = { pSPM->GetScenePointIndex(pSP), 1, 1, 1, 0 };
				m_lCurrTargets.Add(tmp);
			}
		}
	}

	switch (params.m_Select)
	{
	case SELECT_ALL:
		break; // do nothing

	case SELECT_RANDOM:
	case SELECT_RANDOM2:
		{
			// Pick 'n' random objects, by removing objects until 'n' remains
			// (this is slow if there's a lot of objects in the room. then a temp buffer to pick from would be better...) 
			uint nLen = m_lCurrTargets.Len();
			while (nLen > params.m_Num)
			{
				int iRemove = int(Random * 0.99f * nLen);
				m_lCurrTargets[iRemove] = m_lCurrTargets[nLen - 1];
				m_lCurrTargets.QuickSetLen(--nLen);
			}
		}
		break;

	case SELECT_OLDEST:
	case SELECT_NEWEST:
		{
			uint nLen = m_lCurrTargets.Len();
			int nToRemove = int(nLen - params.m_Num);
			if (nToRemove > 0)
			{
				// Sort list and pick oldest/newest
				RadixSort(m_lCurrTargets.GetBasePtr(), nLen);
				if (params.m_Select == SELECT_OLDEST)
					m_lCurrTargets.Delx(params.m_Num, nToRemove);
				else
					m_lCurrTargets.Delx(0, nToRemove);
			}
		}
		break;
	}

#ifdef ROOM_DEBUG_ENABLE
	CWObject_ScenePointManager* pSPM = GetSPM();
	ROOM_DEBUG(("[%s] Current targets are:", m_pWServer->Object_GetName(m_iObject)));
	for (uint i = 0; i < m_lCurrTargets.Len(); i++)
	{
		const ObjectInfo& x = m_lCurrTargets;
		if (x.m_bScenepoint)
		{
			M_ASSERTHANDLER(pSPM, "wtf, can't add SPs without manager...", continue);
			CWO_ScenePoint* pSP = pSPM->GetScenePointFromIndex(x.m_iObject);
			const char* pName = pSP ? pSP->GetName().Str() : NULL;
			ROOM_DEBUG((" - (%s, ScenePoint)", pName));
		}
		else
		{
			uint iObj = x.m_iObject;
			CWObject* pObj = m_pWServer->Object_Get(iObj);
			const char* pName = pObj ? pObj->GetName() : NULL;
			const char* pClass = pObj ? pObj->m_pRTC->m_ClassName : NULL;
			ROOM_DEBUG((" - %d (%s, %s)", iObj, pName, pClass));
		}
	}
#endif
}


aint CWObject_Room::OnMessage(const CWObject_Message& _Msg)
{
	MSCOPESHORT(CWObject_Room::OnMessage);

	// Send message to current targets?
	if (m_bIsBroadcasting)
	{
		MSCOPESHORT(PerformBroadcast);
		CWObject_ScenePointManager* pSPM = GetSPM();
		int nLastRet = 0;
		TAP<const ObjectInfo> pTargets = m_lCurrTargets;
		for (uint i = 0; i < pTargets.Len(); i++)
		{
			CWO_ScenePoint* pSP = NULL;
			CWObject* pObj = NULL;

			if (pTargets[i].m_bScenepoint)
			{
				pSP = pSPM->GetScenePointFromIndex(pTargets[i].m_iObject);
				if (pSP)
					nLastRet = pSPM->Message_SendToScenePoint(_Msg, pSP);
			}
			else
			{
				pObj = m_pWServer->Object_Get(pTargets[i].m_iObject);
				if (pObj)
					nLastRet = pObj->OnMessage(_Msg);
			}

		  #ifndef M_RTM
			if (m_pWServer->m_bConsoleLogMessages)
			{
				const char* pName = pSP ? pSP->GetName().Str() : pObj ? pObj->GetName() : "???";
				const char* pType = pSP ? "scenepoint" : pObj ? "object" : "???";
				ConOutL(CStrF(" - passed message 0x%X on to %s %d (%s)", _Msg.m_Msg, pType, pTargets[i].m_iObject, pName));
			}
		  #endif
		}

		m_bIsBroadcasting = false;
		return nLastRet;
	}

	// No broadcast, handle message...
	switch (_Msg.m_Msg)
	{
	case OBJMSG_ROOM_BROADCAST:
		{
			MSCOPESHORT(OBJMSG_ROOM_BROADCAST);

			// The broadcast message will prepare a target list for coming (non-room) messages
			CreateBroadcastSelection(_Msg);
			uint nSelected = m_lCurrTargets.Len();
			if (nSelected)
				m_bIsBroadcasting = true;		// TODO: there's a big problem with this. since the message to be broadcasted relies on the simplemessage conditional message system, the condition can be set up so that the broadcast isn't performed, which causes the broadcast flag to remain set. :/
 			return nSelected;
		}

	case OBJMSG_ROOM_GETNUMINSIDE:
		{
			MSCOPESHORT(OBJMSG_ROOM_GETNUMINSIDE);

			TypeParams params(_Msg);
			uint nObj = m_lCurrObjectsInside.Len();
			const ObjectInfo* lpObj = m_lCurrObjectsInside.GetBasePtr();

			uint nInside = 0;
			for (uint i = 0; i < nObj; i++)
			{
				int iObj = lpObj[i].m_iObject;
				CWObject* pObj = m_pWServer->Object_Get(iObj);
				if (pObj && TestObjectType(*pObj, params.m_TypeFlags))
					nInside++;
			}
			return nInside;
		}

	case OBJMSG_GAME_SPAWN:
		{
			bool bSpawn = (_Msg.m_Param0 <= 0);
			Spawn(bSpawn);
			return (bSpawn ? 1 : 0);
		}
		break;

	case OBJSYSMSG_PRECACHEMESSAGE:
		{
			return parent::OnMessage(_Msg);
		}

	case OBJSYSMSG_NOTIFY_INTERSECTION: 
		{
			//ConOutL(CStrF("%i. Intersection: Object: %i  Sender: %i", m_pWServer->GetGameTick(), m_iObject, _Msg.m_iSender));
			if (IsSpawned() && !IsDestroyed())
			{
				int iObj = _Msg.m_Param0;
				CWObject* pObj = m_pWServer->Object_Get(iObj);
				M_ASSERTHANDLER(pObj, "[ROOM] why the hell send message about non-existing object?!", return 0);

				if (AddObject(*pObj, true))
					SortObjects();
			}
			return 1;
		}

	case OBJSYSMSG_LISTENER_EVENT:
		{
			uint Event = _Msg.m_Param0;
			if (Event & CWO_LISTENER_EVENT_DELETED)
			{
				ROOM_DEBUG(("[ROOM] - got event 'DELETED' for object %d", _Msg.m_iSender));
				ObjectInfo* pEntry = Find(_Msg.m_iSender);
				if (pEntry)
				{
					RemoveObject(pEntry);
					SortObjects();
				}
			}
			else if (Event & CWO_LISTENER_EVENT_MOVED) 
			{
				ObjectInfo* pEntry = Find(_Msg.m_iSender);
				if (pEntry)
				{ // object moved - mark as unchecked and enable OnRefresh
					pEntry->m_bChecked = false;
					m_ClientFlags &= ~CWO_CLIENTFLAGS_NOREFRESH;
				}
			}
			return 1;
		}

	// Scenepoint message. These are normally sent to $game, but can also to be sent to all scenepoints in a specific room.
	case OBJMSG_GAME_SCENEPOINT_SPAWN:
	case OBJMSG_GAME_SCENEPOINT_UNSPAWN:
	case OBJMSG_GAME_SCENEPOINT_ACTIVATE:
	case OBJMSG_GAME_SCENEPOINT_RAISEPRIO:
	case OBJMSG_GAME_SCENEPOINT_RESTOREPRIO:
		{
			CWObject_ScenePointManager* pSPM = GetSPM();
			pSPM->Selection_Clear();
			const char* pName = (const char*)_Msg.m_pData;
			if (pName && *pName)
			{ // add only if both name and room matches
				pSPM->Selection_AddByName(pName);
				pSPM->Selection_RemoveNotInRoom(*this);
			}
			else
			{ // add all points in the room
				pSPM->Selection_AddInRoom(*this);
			}
			return pSPM->Message_SendToSelection(_Msg);
		}
	}
	return parent::OnMessage(_Msg);
}


// Returns a safe pos guaranteed to be inside the room but not neccessarily walkable
const CVec3Dfp32& CWObject_Room::GetSafePos()
{
	MSCOPESHORT(CWObject_Room::GetSafePos);
	M_ASSERTHANDLER(m_pWServer, "no server?!", return m_SafePos);

	if (!VALID_POS(m_SafePos))
	{
		CWObject_ScenePointManager* pSPM = GetSPM();
		if (pSPM)
		{
			uint SPMask = CWO_ScenePoint::ROAM | CWO_ScenePoint::COVER | CWO_ScenePoint::TACTICAL;
			CWO_ScenePoint* pSP = pSPM->FindRandomInRoom(SPMask, this);
			if (pSP)
				m_SafePos = pSP->GetPosition();
		}
	}
	return m_SafePos;
}

bool CWObject_Room::PointIsInsideRoom(const CVec3Dfp32& _Pos) const
{
	MSCOPESHORT(CWObject_Room::PointIsInsideRoom);
	M_ASSERT(m_pPhysModel, "Room is missing phys-model! (should have been removed from OnSpawnWorld...)");

	CXR_PhysicsContext PhysContext(GetPositionMatrix());
	m_pPhysModel->Phys_Init(&PhysContext);
	int nMedium = m_pPhysModel->Phys_GetMedium(&PhysContext, _Pos);
	enum { MediumMask = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID };
	if (nMedium & MediumMask)
	{
		return true;
	}

	return false;
}


bool CWObject_Room::AddObject(const CWObject& _Obj, bool _bCheckExisting)
{
	MSCOPESHORT(CWObject_Room::AddObject);

	if (!TestObjectType(_Obj, ~0))
		return false;

	CVec3Dfp32 Pos;
	_Obj.GetAbsBoundBox()->GetCenter(Pos);
	if (!PointIsInsideRoom(Pos))
		return false;

	int iObj = _Obj.m_iObject;
	if (_bCheckExisting)
	{
		ObjectInfo* pEntry = Find(iObj);
		if (pEntry)
		{
			pEntry->m_bValid = true;
			return false;
		}
	}

	ROOM_DEBUG(("[ROOM] adding object '%s' (%d, %s) to room '%s' (%d)", 
		_Obj.GetName(), iObj, _Obj.m_pRTC->m_ClassName, GetName(), m_iObject));

	uint nLen = m_lCurrObjectsInside.Len();
	m_lCurrObjectsInside.QuickSetLen(++nLen);
	ObjectInfo& E = m_lCurrObjectsInside[nLen - 1];
	E.m_iObject = iObj;
	E.m_bValid = false;
	E.m_bChecked = false;
	E.m_bScenepoint = false;
	E.m_SortKey = m_pWServer->GetGameTick();

	// we want to get notified when the object moves or disapperas
	m_pWServer->Object_AddListener(iObj, m_iObject);

	return true;
}


void CWObject_Room::RemoveObject(ObjectInfo* _pElem)
{
	M_ASSERTHANDLER(_pElem, "ERROR in CWObject_Room::RemoveObject", return);
	uint iObject = _pElem->m_iObject;

#ifndef M_RTM
	CWObject* pObj = m_pWServer->Object_Get(iObject);
	const char* pName = pObj ? pObj->GetName() : NULL;
	const char* pClass = pObj ? pObj->m_pRTC->m_ClassName : NULL;
	ROOM_DEBUG(("[ROOM] removing object '%s' (%d, %s) from room '%s' (%d)", pName, iObject, pClass, GetName(), m_iObject));
#endif

	// remove listener
	m_pWServer->Object_RemoveListener(iObject, m_iObject);

	// remove from internal list
	TAP_RCD<ObjectInfo> pList = m_lCurrObjectsInside;
	uint iElem = _pElem - pList.m_pArray;
	pList[iElem] = pList[pList.Len() - 1]; // move last to empty slot
	m_lCurrObjectsInside.QuickSetLen(pList.Len() - 1);
}


CWObject_Room::ObjectInfo* CWObject_Room::Find(int _iObject) const
{
	uint nObj = m_lCurrObjectsInside.Len();
 	const ObjectInfo* M_RESTRICT pCurr = m_lCurrObjectsInside.GetBasePtr() + nObj - 1;
	for (uint i = nObj; i > 0; i--, pCurr--)
	{
		if (pCurr->m_iObject == _iObject)
			return const_cast<ObjectInfo*>(pCurr);
	}
	return NULL;
}


bool CWObject_Room::IsSpawned() const
{ 
	return (m_Data[DATA_FLAGS] & ROOMFLAGS_WAITSPAWNED) == 0; 
}


CWObject_ScenePointManager* CWObject_Room::GetSPM()
{
	return (CWObject_ScenePointManager*)m_pWServer->Message_SendToObject(
		CWObject_Message(OBJMSG_GAME_GETSCENEPOINTMANAGER), m_pWServer->Game_GetObjectIndex());
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_RoomManager
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWO_RoomManager::CWO_RoomManager()
	: m_pWServer(NULL)
{
}

CWO_RoomManager::~CWO_RoomManager()
{
}


void CWO_RoomManager::InitWorld(CWorld_Server* _pWServer, CWObject_ScenePointManager* _pScenePointManager)
{
	M_ASSERT(_pWServer, "Must pass valid server!");
	M_ASSERT(_pScenePointManager, "Must pass valid scenepoint manager!");

	m_pWServer = _pWServer;
	m_pScenePointManager = _pScenePointManager;
}


CWObject_Room& CWO_RoomManager::GetRoom(uint16 _iObject) const
{
	CWObject* pObj = m_pWServer->Object_Get(_iObject);
	if (!pObj)
		Error_static("CWO_RoomManager::GetRoom", CStrF("Room does not exist! (iObject = %d)", _iObject));

	return *safe_cast<CWObject_Room>(pObj);
}


CWObject_Room* CWO_RoomManager::GetRoom(const CNameHash& _NameHash) const
{
	int iObject = m_pWServer->Selection_GetSingleTarget(_NameHash);
	if (iObject > 0)
	{
		CWObject* pObj = m_pWServer->Object_Get(iObject);
		M_ASSERTHANDLER(pObj, "Non-existing object in selection?!", return NULL);

		if (pObj->IsClass(class_Room))
		{
			CWObject_Room* pRoom = safe_cast<CWObject_Room>(pObj);
			if (pRoom->IsSpawned())
				return pRoom;
		}
	}
	return NULL;
}


CWObject_Room* CWO_RoomManager::GetRoom(const CVec3Dfp32& _Position) const
{
	MSCOPESHORT(CWO_RoomManager::GetRoom_By_Position);

	CWObject_Room* pRet = NULL;

	// Select all rooms close to this point
	TSelection<CSelection::MEDIUM_BUFFER> Selection;
	m_pWServer->Selection_AddBoundSphere(Selection, OBJECT_FLAGS_TRIGGER, _Position, 1.0f);

	const int16* pSel = NULL;
	uint nSel = m_pWServer->Selection_Get(Selection, &pSel);
	for (uint i = 0; i < nSel; i++)
	{
		CWObject* pObj = m_pWServer->Object_Get(pSel[i]);
		M_ASSERTHANDLER(pObj, "Non-existing object in selection?!", continue);
		if (!pObj->IsClass(class_Room))
			continue;

		// Check if point is inside room
		CWObject_Room* pRoom = safe_cast<CWObject_Room>(pObj);
		if (!pRoom->IsSpawned())
			continue;

		if (pRoom->PointIsInsideRoom(_Position))
		{
			pRet = pRoom;
			break;
		}
	}
	return pRet;
}


CWObject_Room* CWO_RoomManager::GetRoom(const CWO_ScenePoint& _ScenePoint) const
{
	//TODO: If this is too slow, it's easy to add a m_iRoom-member to CWO_ScenePoint
	return GetRoom( _ScenePoint.GetPos());
}
