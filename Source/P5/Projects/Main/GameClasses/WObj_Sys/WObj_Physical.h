#ifndef _INC_WOBJ_PHYSICAL
#define _INC_WOBJ_PHYSICAL

#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"

// -------------------------------------------------------------------
//  PHYSICAL
// -------------------------------------------------------------------
#define PHYSICAL_FLAGS_MEDIUMACCEL	1
#define PHYSICAL_FLAGS_MEDIUMROTATE	2
#define PHYSICAL_FLAGS_DISABLE		4

class CPhysUtilParams
{
public:
	int16 m_PhysFlags;
	int16 m_Flags;
	fp32 m_MediumResist;
	fp32 m_Mass;
	fp32 m_Density;

	CPhysUtilParams()
	{
		m_PhysFlags = 0;
		m_Flags = 0;
		m_MediumResist = 1.0f;
		m_Mass = 1.0f;
		m_Density = 1.0f;
	};
};



void PhysUtil_Move(const CSelection& _Selection, CWObject_CoreData* _pObj, CWorld_PhysState* _pPhysState, const CPhysUtilParams& _Params, fp32 _dTime, const CVec3Dfp32& _UserVel, const CVec3Dfp32* _pMediumV, const CXR_MediumDesc* _pMediums, int _nMediums);


class CWObject_Physical : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

protected:
	int32 m_PhysFlags;
	fp32 m_MediumResist;	// How much a medium affect it's speed. Higher == More.
	fp32 m_Density;		// 1.0 = Waterdensity. 1kg/l.
	fp32 m_Mass;			// 1.0 = 1kg.

public:
	CWObject_Physical();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnRefresh();

	virtual int Phys_GetMediumSamplingPoints(CVec3Dfp32* _pRetV, int _MaxV);
	virtual CVec3Dfp32 Phys_GetUserVelocity(const CXR_MediumDesc& _MediumDesc, int& _Flags);

	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);

	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
};

#endif // _INC_WOBJ_PHYSICAL
