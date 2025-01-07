#ifndef __WObj_TranquillizerDart_h
#define __WObj_TranquillizerDart_h

#if !defined(M_DISABLE_TODELETE) || 1
#include "WObj_Projectile_SE.h"

class CWObject_TranquillizerDart : public CWObject_ProjectileCluster
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	int m_TranquillizerEffect;

public:
	virtual void OnCreate();
	//static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnImpact(int _iProjectile, const CVec3Dfp32& _Pos, uint16 _iObject = 0, CCollisionInfo* _pCInfo = NULL);
};
#endif

#endif // __WObj_TranquillizerDart_h
