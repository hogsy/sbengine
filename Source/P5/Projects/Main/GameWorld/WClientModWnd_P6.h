#ifndef __WCLIENTMOD_P6_H__
#define __WCLIENTMOD_P6_H__

#include "MWinCtrlMod.h"
#include "WClientMod.h"
#include "WClientModWnd.h"
#include "WFrontEnd_Menues_P6.h"

class CListItem_Loot
{
public:
	CStr m_LootName;
	CStr m_GUI_Icon;
	int m_Money;
	int m_iLootIndex;

	CListItem_Loot() { m_Money = 0; m_iLootIndex = -1; };
	CListItem_Loot(CStr _LootName, CStr _Gui) :
		m_LootName(_LootName), m_GUI_Icon(_Gui)	{ m_Money = 0; m_iLootIndex = -1; };
};

class CMWnd_LootList : public CMWnd_List2
{
	MRTC_DECLARE;

public:
	TArray<CListItem_Loot> m_lChars;

	CMWnd_LootList();
	virtual void SetItemCount(int _nItems);
	virtual void AddItem(const CListItem_Loot& _Item);
	virtual void OnPaintItem(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, int _iItem);
	void OnTouchItem(int _iItem, int _bSelected);
};

class CMWnd_ModLoot_P6 : public CMWnd
{
	MRTC_DECLARE;
public:
	CMWnd_ClientInfo m_ClientInfo;
	CMWnd_ClientInfo *GetClientInfo()
	{
		return &m_ClientInfo;
	}

	CMWnd_ModLoot_P6();

	int GetItemState(const char* _pName);
	void SetItemState(const char* _pName, int _Value);
	virtual void OnCreate();

	void OnRefresh();

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);

	virtual aint OnMessage(const CMWnd_Message* _pMsg);

private:
	int		m_LastUpdateTick;
	int		m_LastLootTick;
	bool	m_bCreated:1;
	bool	m_bParentCreated:1;
};

class CMWnd_ModTrading_P6 : public CMWnd
{
	MRTC_DECLARE;
#define WEAPONMOD_WIDTH 6
#define WEAPONMOD_HEIGHT 8

	CImageButton m_lButtons[WEAPONMOD_WIDTH][WEAPONMOD_HEIGHT];

	int m_XPos, m_YPos;
	int m_LastMarkedX, m_LastMarkedY;

public:
	CMWnd_ModTrading_P6();

	virtual void OnCreate();
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
//	virtual	int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);

	bool m_bDoOnce;
};

#endif

