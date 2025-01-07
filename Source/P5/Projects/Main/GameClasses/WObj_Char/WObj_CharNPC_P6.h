#ifndef __WObj_CharNPC_P6_h__
#define __WObj_CharNPC_P6_h__

#include "WObj_CharNPC.h"
#include "WObj_CharNPC_ClientData_P6.h"


class CWObject_CharNPC_P6 : public CWObject_CharNPC
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	// Client data - server only interface
	const CWO_CharNPC_ClientData_P6& GetClientData() const { return GetClientData(this); }
	CWO_CharNPC_ClientData_P6& GetClientData()       { return GetClientData(this); }
	// Client data - client/server interface
	static const CWO_CharNPC_ClientData_P6& GetClientData(const CWObject_CoreData* _pObj);
	static       CWO_CharNPC_ClientData_P6& GetClientData(CWObject_CoreData* _pObj);

	virtual void OnRefresh();
	virtual void OnCreate();

	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	bool m_bDoOnce;
};



#endif // __WObj_CharNPC_P6_h__
