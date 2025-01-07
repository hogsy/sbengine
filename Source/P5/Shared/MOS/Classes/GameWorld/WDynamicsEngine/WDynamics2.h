#ifndef WDYNAMICS2_H
#define WDYNAMICS2_H


#include "WDynamicsEngine2.h"

class CWD_ConstraintDescriptor
{
public:
	enum Type
	{
		BALL,
		BALLWORLD,
		HINGE,
		HINGE2,
		HINGEWORLD,
		HINGEWORLD2,
	};

	enum VectorParam
	{
		RA = 0,
		RB = 1,
		WORLDREF = 2,
		AXISA = 3,
		AXISB = 4,
		WORLDAXIS = 5,
		ANGLEAXISA = 6,
		ANGLEAXISB = 7,
	};

	enum ScalarParm
	{
		RELATIVEANGLE = 0,
		MAXANGLE = 1,
		MINANGLE = 2
	};

	CWD_ConstraintDescriptor()
	{
//#ifndef M_RTM
		m_Type = 0xffff;
		m_iObjectA = -1;
		m_iObjectB = -1;
		m_iSubObjectA = -1;
		m_iSubObjectB = -1;
		m_ID = -1;
		m_bInSimulation = false;
//#endif
	}

	CWD_ConstraintDescriptor(int _ID)
	{
		//#ifndef M_RTM
		m_Type = 0xffff;
		m_iObjectA = -1;
		m_iObjectB = -1;
		m_iSubObjectA = -1;
		m_iSubObjectB = -1;
		//#endif

		m_ID = _ID;
	}

	int GetID() const
	{
		return m_ID;
	}

	void SetVectorParam(int _i, const CVec4Dfp32& _v)
	{
		M_ASSERT(_i < MAXVECTORPARAMS, "!");
		m_lVectorParams[_i] = _v;
	}

	const CVec4Dfp32& GetVectorParam(int _i) const
	{
		M_ASSERT(_i < MAXVECTORPARAMS, "!");
		return m_lVectorParams[_i];
	}

	void SetScalarParam(int _i, fp32 _x)
	{
		M_ASSERT(_i < MAXSCALARPARAMS, "!");
		m_lScalarParms[_i] = _x;
	}

	fp32 GetScalarParam(int _i) const
	{
		M_ASSERT(_i < MAXSCALARPARAMS, "!");
		return m_lScalarParms[_i];
	}

	void SetType(uint16 _t)
	{
		m_Type = _t;
	}

	uint16 GetType() const
	{
		M_ASSERT(m_Type != 0xffff, "!");
		return m_Type;
	}

	void SetConnectedObjects(uint32 _iObjectA, uint32 _iObjectB)
	{
		m_iObjectA = _iObjectA;
		m_iObjectB = _iObjectB;
	}

	void SetConnectedSubObjects(uint32 _iSubA, uint32 _iSubB)
	{
		m_iSubObjectA = _iSubA;
		m_iSubObjectB = _iSubB;
	}

	void GetConnectedObjects(uint32 &_iObjectA, uint32 &_iObjectB) const
	{
		_iObjectA = m_iObjectA;
		_iObjectB = m_iObjectB;
	}

	void GetConnectedSubObjects(uint32 &_iSubA, uint32 &_iSubB) const
	{
		_iSubA = m_iSubObjectA;
		_iSubB = m_iSubObjectB;
	}

	void Read(CCFile* _pFile, CWorld_Server* _pWServer);
	void Write(CCFile* _pFile, CWorld_Server* _pWServer) const;

	M_INLINE bool InSimulation() const
	{
		return m_bInSimulation;
	}

	void SetInSimulation(bool _InSimulation) 
	{
		m_bInSimulation = _InSimulation;
	}

	M_INLINE bool UseOriginalFreezeThreshold() const
	{
		return m_bUseOriginalFreezeThreshold;
	}

	void SetUseOriginalFreezeThreshold(bool _bUseOriginalFreezeThreshold) 
	{
		m_bUseOriginalFreezeThreshold = _bUseOriginalFreezeThreshold;
	}

protected:
	const static int MAXVECTORPARAMS = 8;
	const static int MAXSCALARPARAMS = 8;

	int32 m_ID;
	uint16 m_Type;
	CVec4Dfp32 m_lVectorParams[MAXVECTORPARAMS];
	fp32 m_lScalarParms[MAXSCALARPARAMS];
	bool m_bInSimulation : 1;
	bool m_bUseOriginalFreezeThreshold : 1;	//Used to avoid using the lamp freeze threshold hack

	uint32 m_iObjectA, m_iObjectB;
	uint16 m_iSubObjectA, m_iSubObjectB;
};

class CWD_WorldModelCollider : public CWD_Collider
{
public:
	void OnSpawnWorld(CMapData* _pWorldData);
	void OnWorldClose();
	virtual bool Collide(const CWD_DynamicsWorld *_pWorld, 
						 CWD_RigidBody2 *_pBody,
 						 CWD_ContactInfo *_pContactInfo,
						 void *_pArgument1,
						 void *_pArgument2);

	virtual int Collide(const CWD_DynamicsWorld *_pWorld, 
						TArray<CWD_RigidBody2 *> &_lpRigidBodies,
						TAP_RCD<CWD_ContactInfo> _pContactInfo,
						void *_pArgument1,
						void *_pArgument2);

	virtual void PreApplyExternalForces(CWD_DynamicsWorld *_pWorld, void *_pArgument1, void *_pArgument2);
	virtual void ApplyExternalForces(CWD_DynamicsWorld *_pWorld, void *_pArgument1, void *_pArgument2);
	virtual void PostApplyExternalForces(CWD_DynamicsWorld *_pWorld, void *_pArgument1, void *_pArgument2);

protected:
	TArray<CWD_RigidBody2 *> m_lBouyancyObjects;
};

#endif
