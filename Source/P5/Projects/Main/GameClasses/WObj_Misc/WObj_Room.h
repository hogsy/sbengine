/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_Room.h

	Author:			Anton Ragnarsson

	Copyright:		Copyright Starbreeze AB 2005

	Contents:		CWObject_Room
					CWO_RoomManager

	History:		
		050304:		Created File
\*____________________________________________________________________________________________*/
#ifndef __WObj_Room_h__
#define __WObj_Room_h__

#include "../WObj_Sys/WObj_Trigger.h"

class CNameHash;
class CWObject_Room;
class CWO_RoomManager;
class CWO_ScenePoint;
class CWObject_ScenePointManager;


struct IDPair
{
	int16 m_iFirst;
	int16 m_iSecond;

	M_INLINE IDPair() {}
	M_INLINE IDPair(int16 a, int16 b) : m_iFirst(a), m_iSecond(b) {}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Room
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Room : public CWObject
{
	typedef CWObject parent;
	friend class CWObject_RoomManager;
	MRTC_DECLARE_SERIAL_WOBJECT;

	enum
	{
		DATA_FLAGS = 0,	// m_Data[0] - RoomFlags

		ROOMFLAGS_WAITSPAWNED =		M_Bit(0),
		ROOMFLAGS_FORCEREFRESH =	M_Bit(1),
	};

	CVec3Dfp32	m_SafePos;
	TArray<IDPair> m_Doors;				// Indices to Door-ScenePoints

	int m_iBSPModel;
	CXR_PhysicsModel* m_pPhysModel;		// Must be valid
	bool m_bIsBroadcasting;				// When true, messages are passed on to current selection

	struct ObjectInfo
	{
		uint32 m_iObject : 16;
		uint32 m_bValid : 1;
		uint32 m_bChecked : 1;
		uint32 m_bScenepoint : 1;
		uint32 m_SortKey;
	};
	TArray<ObjectInfo> m_lCurrObjectsInside;
	TArray<ObjectInfo> m_lCurrTargets;

	bool TestObjectType(const CWObject& _Obj, uint _TypeFlags) const;
	bool TestScenePoint(const CWO_ScenePoint& _SP, uint _TypeFlags, uint _SPFlags) const;
	void SortObjects();

	void Spawn(bool _bSpawn = true);
	void FindAllDoors();
	void InitObjectsInside();
	void CreateBroadcastSelection(const CWObject_Message& _Msg);

	ObjectInfo* Find(int _iObject) const;
	bool AddObject(const CWObject& _Obj, bool _bCheckExisting = true);
	void RemoveObject(ObjectInfo* _pElem);

	CWObject_ScenePointManager* GetSPM();

public:
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnSpawnWorld();
	virtual void OnSpawnWorld2();
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);

	bool IsSpawned() const;

	bool PointIsInsideRoom(const CVec3Dfp32& _Pos) const;

	const CVec3Dfp32& GetSafePos();
	const TArray<IDPair>& GetDoors() const { return m_Doors; }
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_RoomManager
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_RoomManager
{
	CWorld_Server* m_pWServer;
	CWObject_ScenePointManager* m_pScenePointManager;

public:
	CWO_RoomManager();
	~CWO_RoomManager();

	void InitWorld(CWorld_Server* _pWServer, CWObject_ScenePointManager* _pScenePointManager);

	CWObject_Room& GetRoom(uint16 _iObject) const;
	CWObject_Room* GetRoom(const CNameHash& _NameHash) const;
	CWObject_Room* GetRoom(const CVec3Dfp32& _Position) const;
	CWObject_Room* GetRoom(const CWO_ScenePoint& _ScenePoint) const;
};

#endif // __WObj_Room_h__
