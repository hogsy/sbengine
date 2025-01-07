
#ifndef __WDYNAMICS_H_INCLUDED
#define __WDYNAMICS_H_INCLUDED

#define DYNAMICS_KEEP_STATE

#include "WDynamicsEngine.h"

/*
class CW_ServerRigidBody : public CReferenceCount, public CWD_RigidBody2
{
public:

};
*/

class CWD_RigidBody : public CRigidBody
{
public:
	CWD_RigidBody(CWorld *_pWorld, CWObject_CoreData *_pCoreData);

	CWObject_CoreData *m_pCoreData;

	static void GetBoundBox(CMapData* _pWorldData, const CWObject_CoreData* _pObj, CBox3Dfp32& _Box);
protected:
};

class CWD_DynamicsUtil
{
public:
	struct SPhysicalProperties
	{
		fp32 m_Mass;
		fp32 m_Volume;
		CVec3Dfp32 m_CenterOfMass;
		CVec3Dfp32 m_InertiaTensor;
	};

	static bool GetPhysicalProperties(CMapData* _pWorldData, const CWO_PhysicsState& _PhysState, SPhysicalProperties& _Result);
	static void GetPhysicalProperties(CMapData* _pWorldData, const CWObject* _pObj, fp32& _Mass, CVec3Dfp32& _CenterOfMass, CVec3Dfp32& _InertiaTensor);
};


class CWO_DynamicsCollider : public IWorldCollider
{
public:
	static CWO_DynamicsCollider *GetInstance();

	void Init(CMapData* _pMapData);

	virtual void PreCollide(CWorld *_pWorld);

	virtual bool Collide(const CWorld *_world, 
						 CRigidBody *_pBody,
						 CContactInfo *_pContactInfo,
						 void *_pArgument1,
						 void *_pArgument2);

	virtual int Collide(const CWorld *_pWorld, 
						const TArray<CRigidBody *>& _BodyList,
						CContactInfo *_pContactInfo, 
						int _MaxCollisions,
						void *_pArgument1,
						void *_pArgument2);

	virtual void PreApplyExternalForces(void *_pArgument1, void *_pArgument2);
	virtual void ApplyExternalForces(void *_pArgument1, void *_pArgument2);
	virtual void PostApplyExternalForces(void *_pArgument1, void *_pArgument2);

	virtual void GetBoundingBox(const CRigidBody *_pRigidBody, CBox3Dfp64& _Box, void *_pArgument1, void *_pArgument2);

protected:
	static CWO_DynamicsCollider *m_pInstance;

	CWO_DynamicsCollider();

	int Collide(const CWD_RigidBody *_pBody1,
				const CWD_RigidBody *_pBody2,
				CContactInfo *_pCollisionInfo, 
				int _MaxCollisions,
				CMapData *_pWorldData);

	int CollideAsBoxes(const CWD_RigidBody *_pBody1,
					   const CWD_RigidBody *_pBody2,
					   CContactInfo *_pCollisionInfo, 
					   int _MaxCollisions,
					   CMapData *_pWorldData);

	int Collide2(const CWD_RigidBody *_pBody1,
				 const CWObject_CoreData *pObj2,
				 CContactInfo *_pCollisionInfo, 
				 int _MaxCollisions,
				 CMapData *_pWorldData);

	int CollideSolidBsp(const CWD_RigidBody *_pBody1,
						CXR_IndexedSolidContainer32 *_pSolidContainer,
						const CWObject_CoreData *pObj2,
						CContactInfo *_pCollisionInfo, 
						int _MaxCollisions,
						CMapData *_pWorldData,
						CWorld_PhysState *_pPhysState);

	int Collide3(const CWD_RigidBody *_pBody1,
				 const CWObject_CoreData *pObj2,
				 CContactInfo *_pCollisionInfo, 
				 int _MaxCollisions,
				 CMapData *_pWorldData,
				 CWorld_PhysState *_pPhysState);

	// NEW

	class ColliderContext
	{
	public:

		bool IsTested(int _iObject1, int _iObject2)
		{
			return false;
		}

		void SetTested(int _iObject1, int _iObject2)
		{
		}

	};

	int DoCollidePair(const CWorld *_pWorld, 
					  ColliderContext *_pColliderContext,
					  CWObject_CoreData *_pObject1,
					  CWObject_CoreData *_pObject2,
					  CContactInfo *_pCollisionInfo, 
					  int _MaxCollisions,
					  CMapData *_pWorldData,
					  CWorld_PhysState *_pPhysState);

	int CollideSolidBsp2(const CWObject_CoreData *_pObject1,
						const CWObject_CoreData *_pObject2,
						CXR_Model *_pModel1, 
						CXR_Model *_pModel2,
						const CWO_PhysicsPrim& _PhysicsPrim1,
						const CWO_PhysicsPrim& _PhysicsPrim2,
						CContactInfo *_pContactInfo, 
						int _MaxCollisions,
						CMapData *_pWorldData,
						CWorld_PhysState *_pPhysState);

	TArray<CRigidBody *> m_lBouyancyObjects;

};

class CWO_DynamicsDebugRenderer : public IDynamicsDebugRenderer
{
public:
	CWO_DynamicsDebugRenderer(CWorld_PhysState *_pPhysState);
	virtual void Render(const CContactInfo& _ContactInfo);
	virtual void Render(const CRigidBody *_pRigidBody);


protected:
	CWorld_PhysState* m_pWPhysState;
};

#endif	// __WDYNAMICS_H_INCLUDED
