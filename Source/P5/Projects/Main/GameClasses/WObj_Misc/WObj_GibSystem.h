#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Hook.h"
#include "../WObj_Char.h"

/////////////////////////////////////////////////////////////////////////////
// CWObject_GibSystem

#define NEW_GIBSYSTEM

#define GIBSYSTEM_LIFETIME 200 

#define NETMSG_GIBSYSTEM_INIT 0x48

class CWObject_GibSystem : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	// NO REPLICATION ON THIS DATA. (it is only done once). The object is run only on the client.

#ifdef NEW_GIBSYSTEM
	TArray<CGibInfo> m_lGibInfo;
	CVec3Dfp32 m_GibExplosionOrigin;
	CVec3Dfp32 m_GibExplosionParams;

#else
	class CGibClientData : public CReferenceCount
	{
	public:
		TPtr<class CConstraintGib>	m_spRagdoll;
		TArray<int16> m_liGibPartModel;
		TArray<CMat4Dfp32> m_lGibPartPos;
		TArray<CMat4Dfp32> m_lGibPartLastPos;
		CVec3Dfp32 m_GibExplosionOrigin;
		CVec3Dfp32 m_GibExplosionParams;
	};
	
	static const CGibClientData *GetClientData(const CWObject_CoreData* _pObj);
	static CGibClientData *GetClientData(CWObject_CoreData* _pObj);
	CGibClientData *GetClientData();
#endif

	CWObject_GibSystem();

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey);
	virtual void OnFinishEvalKeys();


	virtual void OnRefresh();

	virtual void OnSpawnWorld();

	static void OnIncludeClass(CMapData* _pWData, CWorld_Server*);							// Called when resource-manager has received notification that the class will be used, and thus gives the class opportunity to include other classes.
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);

	virtual aint OnMessage(const CWObject_Message &_Msg);

	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWorld);

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine*, const CMat4Dfp32& _ParentMat);

	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);

	static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);

};

#ifdef NEW_GIBSYSTEM

class CWObject_ClientGib : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	static void OnClientCreate(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient);		// Called when the object is replicated to the client. This may not occur simulaneously with the server-side OnCreate()
	static void OnClientExecute(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient);		// Called only for 'client-execute' objects once every game-tick.
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine*, const CMat4Dfp32& _ParentMat);
	static int OnPhysicsEvent(CWObject_CoreData*, CWObject_CoreData*, CWorld_PhysState*, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo = NULL);
};

#endif

