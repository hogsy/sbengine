#ifndef _INC_WOBJ_SPAWNER
#define _INC_WOBJ_SPAWNER

#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"

class CWObject_Spawner_P6 : public CWObject
{
#define parent CWObject
	MRTC_DECLARE_SERIAL_WOBJECT

public:
	CWObject_Spawner_P6();
	~CWObject_Spawner_P6();

	virtual void OnSpawnWorld2();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnRefresh();

	CStr	m_SpawnObjectTemplate;
	CStr	m_SpawnObjectName;
	int		m_MinSpawnTime;
	int		m_RandSpawnTime;
	int		m_LastCheck;
	int		m_TimeLeft;
};

#endif

