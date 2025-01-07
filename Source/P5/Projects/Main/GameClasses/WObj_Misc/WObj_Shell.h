/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_Shell.h

	Author:			Patrik Willbo

	Copyright:		Copyright Starbreeze AB 2005

	Contents:		CWObject_Shell
					

	History:		
		051006:		Created File, first pass mockup
\*____________________________________________________________________________________________*/
#ifndef __WObj_Shell_h__
#define __WObj_Shell_h__

//#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"

enum
{
	CWO_SHELL_NETMSG_NA				= 0,
	CWO_SHELL_NETMSG_SPAWNSHELL,					// Spawn shell message

	SHELLDATA_PREV_FREE				= -2,
	SHELLDATA_PREV_LAST				= -1,

	SHELLDATA_FLAGS_MOVING			= M_Bit(0),		// Movement updates on shells
	SHELLDATA_FLAGS_FIRSTTICK		= M_Bit(1),		// Shell has just been spawned (We DO NOT want to update it!)
	SHELLDATA_FLAGS_WORLDBOUNDING	= M_Bit(2),		// Is the shell falling endlessly
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Shell_SpawnData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_Shell_SpawnData
{
public:
	CWO_Shell_SpawnData() {}
	~CWO_Shell_SpawnData() {}

	void AddNetMsgData(CNetMsg& _Msg) const;
	void GetNetMsgData(const CNetMsg& _Msg, int& _iPos); //, CWO_Shell_SpawnData _SpawnData);

	CMat4Dfp32	m_AttachMat;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Shell_Type
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_Shell_Type
{
public:
	CWO_Shell_Type();
	~CWO_Shell_Type();
	
	void OnEvalKey(CWorld_Server* _pWServer, const CRegistry* _pKey);

	static void OnIncludeTemplate(const CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer);
	static void IncludeModelFromKey(const CStr _Key, const CRegistry* _pReg, CMapData* _pMapData);
	static void IncludeSoundFromKey(const CStr _Key, const CRegistry* _pReg, CMapData* _pMapData);

	void Pack(uint8*& _pD) const;
	void Unpack(const uint8*& _pD);

	void Write(CCFile* _pFile, CMapData* _pMapData) const;
	void Read(CCFile* _pFile, CMapData* _pMapData);

	int32		m_iModel;
	CXR_Model*	m_pModel;
	CVec3Dfp32	m_Direction;
	CVec3Dfp32	m_Force;
	int32		m_iSound;
	fp32		m_OffsetZ;
};
static M_INLINE void TAutoVar_Write(const CWO_Shell_Type& _ShellType, CCFile* _pFile, CMapData* _pMapData)	{ _ShellType.Write(_pFile, _pMapData); }
static M_INLINE void TAutoVar_Read(CWO_Shell_Type& _ShellType, CCFile* _pFile, CMapData* _pMapData)			{ _ShellType.Read(_pFile, _pMapData); }


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Shell_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_Shell_ClientData : public CReferenceCount, public CAutoVarContainer
{
private:
	friend class CWObject_Shell;
	friend class CWO_ShellManager;
	AUTOVAR_SETCLASS(CWO_Shell_ClientData, CAutoVarContainer);

	class CShellData
	{
	public:
		CShellData() : m_iPrev(SHELLDATA_PREV_FREE) {}
		~CShellData() {}
		
		void Init(CWorld_Client* _pWClient, const CWO_Shell_Type& _ShellType, const CMat4Dfp32& _Position, const fp32& _CollisionZ, const int32& _iPrev = -1);

		CMat4Dfp32	m_Position;
		CVec3Dfp32	m_Velocity;
		fp32		m_Life;
		uint16		m_Rand;
		fp32		m_CollisionZ;
		uint8		m_iShellType;
		
		int16		m_iPrev;
		int32		m_iModel;

		uint8		m_Flags;
	};

	class CShellOwnerData
	{
	public:
		CShellOwnerData();
		~CShellOwnerData() {}

		void OnRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, CWO_Shell_ClientData::CShellData* _pShellData, CWO_Shell_Type* _pShellTypes, const fp32& _TickTime, const fp32& _IPTime);
		void OnRefresh(CWorld_Client* _pWClient, CBox3Dfp32& _BoundingBox, const fp32& _TickTime, CWO_Shell_ClientData::CShellData* _pShellData, CWO_Shell_Type* _pShellTypes);

		int32				m_iOwner;		// Which object who owns this collection
		int16				m_iFirst;		// Which shell was the last spawned shell ?
		uint32				m_nShells;		// Number of shells in total for this owner
	};

	// Replicated
	TThinArray<CWO_Shell_Type>	m_lShellTypes;
    	
	// Not replicated: Only exist on client
	TThinArray<CShellOwnerData>	m_lOwners;
	TThinArray<CShellOwnerData*>m_lSortedOwners;
	TThinArray<CShellData>		m_lShells;

	uint16						m_nOwners;
	uint16						m_MaxShells;
	uint16						m_MaxOwners;
	uint16						m_iShell;
	uint16						m_iOldest;

	CWorld_PhysState*			m_pWPhysState;

	CAUTOVAR_OP(CAutoVar_int8, m_bTest, DIRTYMASK_0_1);

	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_bTest)
	AUTOVAR_PACK_END

	void Write(CCFile* _pFile, CMapData* _pMapData);
	void Read(CCFile* _pFile, CMapData* _pMapData);
	
public:
	CWO_Shell_ClientData(CWorld_PhysState* _pWPhysState);

	void IncludeShellType(const int8& _iShellType, CWorld_Server* _pWServer);
	void AllocateData(const uint8& _MaxOwners, const uint16& _MaxShells);
	void OnRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine);
	void OnRefresh(CWorld_Client* _pWClient, CBox3Dfp32& _BoundingBox);
	void SpawnShell(CWorld_Client* _pWClient, const int8& _iShellType, const CWO_Shell_SpawnData& _SpawnData, const int32& _iOwner, const fp32& _CollisionZ);

	int  OnCreateClientUpdate(uint8* _pData) const;
	int  OnClientUpdate(const uint8* _pData);

private:
	int32 FindOwnerContainer(const int32& _iOwner, const int32& _First = 0, const int32& _Last = -1, const bool _bInsert = true);
	void UpdateOwnerContainers(uint16 _iShell);
	void QuickSortOwners_r(CShellOwnerData** _pShellOwners, const int32& _Len);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Shell
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Shell : public CWObject_Model
{
private:
	static const CWO_Shell_ClientData& GetClientData(const CWObject_CoreData* _pObj);
	static       CWO_Shell_ClientData& GetClientData(      CWObject_CoreData* _pObj);

	friend class CWO_ShellManager;
	typedef CWObject_Model CWObject_Shell_Parent;
	MRTC_DECLARE_SERIAL_WOBJECT;

	uint8		m_nMaxOwners;
	uint16		m_nMaxShells;
public:
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnRefresh();

	virtual void IncludeShellType(const int8& _iShellType, CWorld_Server* _pWServer);
    static void  OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer);
	static void  OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static void  OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
	static void  OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void  OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	virtual int  OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags)  const;
	static int   OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static void OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);
	static void OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);

	// Net msg helpers
	virtual void AddNetMsgData(CNetMsg& _Msg, const int8& _ShellType);
	virtual void GetNetMsgData(const CNetMsg& _Msg, int& _iPos, CWO_Shell_Type& _ShellType);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_ShellManager
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_ShellManager
{
private:
	CWorld_Server*	m_pWServer;
	CWObject_Game*	m_pGameObject;
	int32			m_iContainerObject;

public:
	CWO_ShellManager();
	~CWO_ShellManager();

	virtual void IncludeShellType(const int8& _iShellType, CWorld_Server* _pWServer);
	
	static void  IncludeTemplate(const char* _pTemplate, CMapData* _pMapData);
	static void  IncludeClass(const char* _pClassName, CMapData* _pMapData);
	static void  OnIncludeClass(CMapData* _pMapData, CWorld_Server *_pWServer);
    
	virtual void SpawnWorld(CWObject_Game* _pGameObject, CWorld_Server* _pWServer);
	virtual void SpawnShell(const int32& _iOwner, const int8& _ShellType, const CWO_Shell_SpawnData& _SpawnData);
	virtual void CloseWorld();
};

#endif
