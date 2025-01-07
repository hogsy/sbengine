#include "PCH.h"
#include "WObj_CharNPC_ClientData_P6.h"

MRTC_IMPLEMENT(CWO_CharNPC_ClientData_P6, CWO_CharNPC_ClientData);

CWO_CharNPC_ClientData_P6::CWO_CharNPC_ClientData_P6()
{
};


void CWO_CharNPC_ClientData_P6::Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	m_Money = 0;
	parent::Clear(_pObj, _pWPhysState);
};


void CWO_CharNPC_ClientData_P6::Copy(const CWO_CharNPC_ClientData_P6& _CD)
{
	m_Money = _CD.m_Money;
	m_lLoot = _CD.m_lLoot;
	parent::Copy(_CD);
};
