#ifndef __WDELTAGAMESTATE_H
#define __WDELTAGAMESTATE_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			IO class to help load and save states of multiple levels

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWorld_DeltaGameState
\*____________________________________________________________________________________________*/

class CWorld_ServerCore;

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				DeltaGameState keeps track of the gamestate
						through multiple separate levels. It also
						handles the save-game IO.
						
	Comments:			The delta in the name stands for the assumtion
						that each level is spawned in the same way
						every time. Only data contaning the changes
						since spawn is stored.
\*____________________________________________________________________*/
class CWorld_DeltaGameState : public CReferenceCount
{
public:
	CWorld_DeltaGameState();
	~CWorld_DeltaGameState();

	void ChangeLevel(CWorld_ServerCore* _pCore, const char* _pNewLevel, int _Flags = SERVER_CHANGEWORLD_KEEPGAME);
	bool SaveGame(CWorld_ServerCore* _pCore, CStr _SaveName, bool _bDotest);
	void LoadGame(CWorld_ServerCore* _pCore, CStr _SaveName);
	bool CanSave(CWorld_ServerCore* _pCore);

	dllvirtual void PrecacheInfo(const char* _pResource, int _Type); // Called during OnSave for additinal resources that needs to be preached during load
	dllvirtual bool SendMessageToLevel(const char* _pTarget, const CWObject_Message& _Msg);
	dllvirtual bool ApplyNextPendingPlayer(CWorld_ServerCore* _pCore, int _iObject, int _Flags);
	dllvirtual bool GetNextPendingPlayerPos(CMat4Dfp32& _Mat);
	dllvirtual CStr GetNextPendingPlayerClass(int* _piObj = NULL, int* _pGUID = NULL, int16* _pFlags = NULL);
	dllvirtual void ClearSaves();
	dllvirtual bool RefreshBlockedWorldMessages();
	dllvirtual void RegisterOrgObjects(CWorld_ServerCore* _pCore);
	dllvirtual bool IsFirstLevelVisit(CWorld_ServerCore* _pCore, CStr _Level = "");
	dllvirtual void ResetSaveInfo(const char* _pLevel);
	dllvirtual void ModifyLocalSpawnFlags(const char* _pLevel, bool _bSet, uint32 _Spawnflags);
	dllvirtual bool GetHasInsertedPlayerGUID() { return m_bHasInsertedGUID; }
	dllvirtual void ClearInsertedPlayerGUID() { m_bHasInsertedGUID = 0; }

protected:
	class CPendingPlayer
	{
	public:
		CPendingPlayer() {}
		CPendingPlayer(const CPendingPlayer& _Other)
		{
			m_lData = _Other.m_lData;
			m_Position = _Other.m_Position;
			m_SpecialClass = _Other.m_SpecialClass;
		}

		CPendingPlayer& operator = (const CPendingPlayer& _Other)
		{
			m_lData = _Other.m_lData;
			m_Position = _Other.m_Position;
			m_SpecialClass = _Other.m_SpecialClass;
			return *this;
		}

		TArray<uint8> m_lData;
		CMat4Dfp32 m_Position;
		CStr m_SpecialClass;
		int32 m_Flags;

		int32 m_PendingPlayerObj;
		int32 m_PendingPlayerGUID;
	};

	class CDeltaWorldData
	{
	public:
		CDeltaWorldData(){}
		CDeltaWorldData(const CDeltaWorldData& _Other)
		{
			m_Level	= _Other.m_Level;
			m_lData = _Other.m_lData;
			m_DataSize = _Other.m_DataSize;
			m_LocalSpawnFlags = _Other.m_LocalSpawnFlags;
		}

		CDeltaWorldData& operator = (const CDeltaWorldData& _Other)
		{
			m_Level	= _Other.m_Level;
			m_lData = _Other.m_lData;
			m_DataSize = _Other.m_DataSize;
			m_LocalSpawnFlags = _Other.m_LocalSpawnFlags;
			return *this;
		}

		CFStr m_Level;
		TArray<uint8> m_lData;
		int m_DataSize; // Only updated during save
		uint32 m_LocalSpawnFlags;
		TThinArray<uint32> m_lResourceMappings;
	};

	TArray<CPendingPlayer> m_lPendingPlayers;
	TArray<CPendingPlayer> m_lPendingPlayerPrecacheInfo;
	TArray<CDeltaWorldData> m_lDeltaWorldData;
	TArray<CFStr> m_lPrecacheInfoNames;
	TArray<uint8> m_lPrecacheInfoTypes;

	// Objects that are not spawned, but needs to be put back into the savefile
	struct SInactiveObj
	{
		uint32 m_GUID;
		uint32 m_SpawnMask;
		TThinArray<uint8> m_Data;

		SInactiveObj()
			: m_GUID(0), m_SpawnMask(0) { }

		SInactiveObj(uint32 _GUID, CCFile* _pFile, uint _nSize, uint32 _SpawnMask = 0)
			: m_GUID(_GUID)
			, m_SpawnMask(_SpawnMask)
		{
			m_Data.SetLen(_nSize);
			_pFile->Read(m_Data.GetBasePtr(), _nSize);
		}
	};
	TArray<SInactiveObj> m_lInactiveOrg;	// objects supposed to be spawned from "ORGOBJECTS", but failed spawnmask (needs to be stored back, though)
	TArray<SInactiveObj> m_lInactiveNew;	// objects supposed to be spawned from "NEWOBJECTS", but failed spawnmask (needs to be stored back, though)
	TArray<uint32>       m_lSpawnedOrg;		// list of spawned orig-objects' GUIDs, to know at save time which objects were deleted


	class CBlockedWorldMessage
	{
	public:
		CStr m_Target;
		int m_Msg;
		int m_Param0;
		TArray<uint8> m_lData;
	};
	TArray<CBlockedWorldMessage> m_lBlockedWorldMessages;

	int32 m_nDeltaWorldData; // Only updated during save
	int32 m_SaveInfoSize; // Only updated during save
	TArray<uint8> m_lLastSaveInfo;
	
	uint8 m_bChangeLevel :1;
	uint8 m_bHasInsertedGUID :1;

	void CaptureSaveInfo(CWorld_ServerCore* _pCore, TArray<uint8> &_lData);
	void CaptureLevel(CWorld_ServerCore* _pCore);
	void CapturePendingPlayers(CWorld_ServerCore *_pCore);
	void ClearPendingPlayers();

	void LoadLevel(CWorld_ServerCore* _pCore, const char *_pNewLevel, int _Flags = SERVER_CHANGEWORLD_KEEPGAME);

	bool ApplyDeltaSave(CWorld_ServerCore* _pCore, TArray<uint8> _lData, int _Flags);
	TArray<uint8> CreateDeltaSave(CWorld_ServerCore* _pCore);
};

#endif
