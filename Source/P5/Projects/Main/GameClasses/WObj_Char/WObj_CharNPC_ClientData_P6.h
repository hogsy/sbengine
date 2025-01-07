#ifndef __WObj_CharNPC_ClientData_P6_h__
#define __WObj_CharNPC_ClientData_P6_h__

#include "WObj_CharNPC_ClientData.h"

struct SLootItem
{
	SLootItem()
		: m_LootType(-1) {}

	CStr	m_LootObject;
	CStr	m_LootTemplate;
	CStr	m_Info;
	CStr	m_GUI_Icon;
	int32	m_LootType;
	int32	m_SubType;	//Used or weapon mods

	void Pack(uint8*& _pD) const
	{
		TAutoVar_Pack(m_LootObject, _pD);
		TAutoVar_Pack(m_LootTemplate, _pD);
		TAutoVar_Pack(m_Info, _pD);
		TAutoVar_Pack(m_GUI_Icon, _pD);
		TAutoVar_Pack(m_LootType, _pD);
		TAutoVar_Pack(m_SubType, _pD);
	}

	void Unpack(const uint8*& _pD)
	{
		TAutoVar_Unpack(m_LootObject, _pD);
		TAutoVar_Unpack(m_LootTemplate, _pD);
		TAutoVar_Unpack(m_Info, _pD);
		TAutoVar_Unpack(m_GUI_Icon, _pD);
		TAutoVar_Unpack(m_LootType, _pD);
		TAutoVar_Unpack(m_SubType, _pD);
	}
};



class CWO_CharNPC_ClientData_P6 : public CWO_CharNPC_ClientData
{
	typedef CWO_CharNPC_ClientData parent;
	MRTC_DECLARE;
	AUTOVAR_SETCLASS(CWO_CharNPC_ClientData_P6, parent);

public:
	CWO_CharNPC_ClientData_P6();
	void Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	virtual void Copy(const CWO_CharNPC_ClientData_P6& _CD);

	CAUTOVAR_OP(CAutoVar_int32, m_Money, DIRTYMASK_0_3);
	CAUTOVAR(TAutoVar_StaticArray_(SLootItem, 20), m_lLoot, DIRTYMASK_1_2);
	
	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_Money)
	AUTOVAR_PACK_VAR(m_lLoot)
	AUTOVAR_PACK_END
	
//	int					m_Money;
//	TArray<SLootItem>	m_lLoot;
};


#endif // __WObj_CharNPC_ClientData_P6_h__
