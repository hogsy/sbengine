#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Hook.h"


/////////////////////////////////////////////////////////////////////////////
// CWObject_RigidBody

#define RIGIDBODY_LIFETIME 200 // The rigidbody system will be deleted after 10 seconds of inactivity

class CWObject_RigidBody : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	bool m_bActivated;
	bool m_bAutoActivate;
	bool m_bPhysicsSet;
	int m_iBSPModel;
	int m_iHardImpactSound;
	int m_iSoftImpactSound;
	int m_LastActiveTick;

	TPtr<class CConstraintRigidObject>	m_spRagdoll;
	//TArray<CWO_SimpleMessage> m_lDestroyMessages;

	CWObject_RigidBody();

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey);
	virtual void OnFinishEvalKeys();



	virtual void OnSpawnWorld();

	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);

	virtual void OnRefresh();
	void ActivateRigidBody();

	virtual aint OnMessage(const CWObject_Message &_Msg);
	static int OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo);
	static bool OnIntersectLine(CWO_OnIntersectLineContext& _Context, CCollisionInfo* _pCollisionInfo = NULL);
	bool SetPhysics();
};
