#ifndef __WOBJ_AUTOVAR_MOD_H
#define __WOBJ_AUTOVAR_MOD_H

/////////////////////////////////////////////////////////////////
// TAutoVar_Mod_uint8
template<class T, int ModSpeed>
class TAutoVar_Mod
{
public:
	T m_LastFixed;
	T m_Target;
	uint32 m_TargetGameTick;

	TAutoVar_Mod()
	{
		m_Target = 0;
		m_TargetGameTick = 0;
		m_LastFixed = 0;
	}

	bool Set(T _Target, int32 _GameTick, fp32 _Fraction)
	{
		// Run from both Server and from OnClientUpdate
		if(_Target != m_Target)
		{
			m_LastFixed = Get(_GameTick, _Fraction);
			m_Target = _Target;
			m_TargetGameTick = _GameTick;
			return true;
		}
		return false;
	}

	static fp32 Sinc(fp32 _x)
	{
		return M_Sin((_x - 0.5f)*_PI)*0.5f + 0.5f;
	}

	T Get(int32 _GameTick, fp32 _Fraction) const
	{
		if(m_TargetGameTick == 0)
			return T(0);

		fp32 Duration = fp32(int(_GameTick - m_TargetGameTick)) + _Fraction;
		if(Duration >= ModSpeed)
			return m_Target;
		if(Duration < 0.0f)
			return m_LastFixed;

		return T(m_LastFixed + (m_Target - m_LastFixed) * Sinc(Duration / ModSpeed));
	}

	void CopyFrom(const TAutoVar_Mod& _From)
	{
		m_Target =         _From.m_Target;
		m_TargetGameTick = _From.m_TargetGameTick;
		m_LastFixed =      _From.m_LastFixed;
	}

	void Pack(uint8*& _pD, CMapData* _pMapData) const
	{
		TAutoVar_Pack(m_Target, _pD);
		TAutoVar_Pack(m_TargetGameTick, _pD);
	}

	void Unpack(const uint8*& _pD, CMapData* _pMapData)
	{
		T Target;
		uint32 GameTick;
		TAutoVar_Unpack(Target, _pD);
		TAutoVar_Unpack(GameTick, _pD);
		Set(Target, GameTick, 0.0f);
	}
};

template<class T> class TAutoVar_Mod20 : public TAutoVar_Mod<T, 20> {};
template<class T> class TAutoVar_Mod10 : public TAutoVar_Mod<T, 10> {};
template<class T> class TAutoVar_Mod5 : public TAutoVar_Mod<T, 5> {};


/////////////////////////////////////////////////////////////////
// CAutoVar_Inventory
struct CWO_InventoryItemDesc
{
	CStr m_Name;
	CStr m_Desc;
	int16 m_iSurface;
	int16 m_nItems;
};

class CAutoVar_Inventory
{
public:
	TArray<CWO_InventoryItemDesc> m_lItems;

	void CopyFrom(const CAutoVar_Inventory& _From)
	{
		m_lItems = _From.m_lItems;
	}

	void Pack(uint8*& _pD, CMapData* _pMapData) const
	{
		uint8 nItems = m_lItems.Len();
		TAutoVar_Pack(nItems, _pD);
		for (int i = 0; i < nItems; i++)
		{
			TAutoVar_Pack(m_lItems[i].m_Name, _pD);
			TAutoVar_Pack(m_lItems[i].m_Desc, _pD);
			TAutoVar_Pack(m_lItems[i].m_iSurface, _pD);
			TAutoVar_Pack(m_lItems[i].m_nItems, _pD);
		}
	}

	void Unpack(const uint8 *&_pD, CMapData* _pMapData)
	{
		uint8 nItems;
		TAutoVar_Unpack(nItems, _pD);
		m_lItems.SetLen(nItems);
		for(int i = 0; i < nItems; i++)
		{
			TAutoVar_Unpack(m_lItems[i].m_Name, _pD);
			TAutoVar_Unpack(m_lItems[i].m_Desc, _pD);
			TAutoVar_Unpack(m_lItems[i].m_iSurface, _pD);
			TAutoVar_Unpack(m_lItems[i].m_nItems, _pD);
		}
	}
};

#endif
