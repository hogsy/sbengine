#ifndef __WOBJ_DETECTOR_H
#define __WOBJ_DETECTOR_H

#if !defined(M_DISABLE_TODELETE) || 1

#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Hook.h"
#include "../WObj_Messages.h"

// -------------------------------------------------------------------
// Detector_Camera
// -------------------------------------------------------------------
class CWObject_Detector_Camera : public CWObject_Engine_Path
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	float m_FOV;
	float m_RangeNear;
	float m_RangeFar;
	TArray<CWO_SimpleMessage> m_lMessages;

	bool m_bReactOnPlayer;
	bool m_bReactOnNPCs;

public:
	CWObject_Detector_Camera();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnSpawnWorld();
	virtual void OnRefresh();
};

class CWObject_Detector_Camera_SecurityLow : public CWObject_Detector_Camera
{
	MRTC_DECLARE_SERIAL_WOBJECT;
};

class CWObject_Detector_Camera_SecurityNormal : public CWObject_Detector_Camera
{
	MRTC_DECLARE_SERIAL_WOBJECT;
};

class CWObject_Detector_Camera_SecurityHigh : public CWObject_Detector_Camera
{
	MRTC_DECLARE_SERIAL_WOBJECT;
};

#endif

#endif // __WOBJ_DETECTOR_H
