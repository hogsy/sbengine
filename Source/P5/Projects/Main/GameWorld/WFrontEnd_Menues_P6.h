#ifndef __FRONTEND_MENUES_P6_H__
#define __FRONTEND_MENUES_P6_H__

#include "WFrontEndMod.h"
#include "WFrontEndMod_Menus.h"

class CImageButton
{
public:
	CImageButton();
	void Render(CRC_Util2D* _pRCUtil, const CClipRect &_Clip);
	void Init(CStr _Name, int _x, int _y);

	CStr name;
	int x, y;

	bool bDisabled;
	bool bMarked;
};

class CMWnd_CubeMenu_Main_P6 : public CMWnd_CubeMenu
{
	MRTC_DECLARE;

	CStr m_BG;

public:
	CMWnd_CubeMenu_Main_P6();
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

class CMWnd_CubeMenu_Journal_P6 : public CMWnd_CubeMenu
{
	MRTC_DECLARE;

	CStr m_BG;

public:
	CMWnd_CubeMenu_Journal_P6();
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

class CMWnd_CubeMenu_Inventory : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
#define INVENTORY_WIDTH 4
#define INVENTORY_HEIGHT 8

	enum
	{
		MENU_INV,
		MENU_JOURNAL,
		MENU_VEHICLE,

		MENU_LAST,
	};

	CImageButton m_lButtons[INVENTORY_WIDTH][INVENTORY_HEIGHT];

	int m_XPos, m_YPos;
	int m_LastMarkedX, m_LastMarkedY;
	int m_Mode;

public:
	CMWnd_CubeMenu_Inventory();

	virtual void OnCreate();
	virtual	int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

class CMWnd_CubeMenu_WeaponMod : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
#define WEAPONMOD_WIDTH 6
#define WEAPONMOD_HEIGHT 8

	CImageButton m_lButtons[WEAPONMOD_WIDTH][WEAPONMOD_HEIGHT];

	int m_XPos, m_YPos;
	int m_LastMarkedX, m_LastMarkedY;

public:
	CMWnd_CubeMenu_WeaponMod();

	virtual void OnCreate();
	virtual	int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};
/*
class CMWnd_CubeMenu_Trading : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
#define WEAPONMOD_WIDTH 6
#define WEAPONMOD_HEIGHT 8

	CImageButton m_lButtons[WEAPONMOD_WIDTH][WEAPONMOD_HEIGHT];

	int m_XPos, m_YPos;
	int m_LastMarkedX, m_LastMarkedY;

public:
	CMWnd_CubeMenu_Trading();

	virtual void OnCreate();
	virtual	int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);

	bool m_bDoOnce;
};*/

class CMWnd_CubeMenu_Multiplayer_SelectServerList_P6 : public CMWnd_CubeMenu
{
	MRTC_DECLARE;

	TArray<CStr> m_List;
	int m_iSelected;

	bool	m_bNoServers;

public:
	CMWnd_CubeMenu_Multiplayer_SelectServerList_P6();
	virtual void OnActivate();
	virtual bool OnButtonPress(CMWnd* _pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr& _Name, bool _Focus);
	virtual void OnCreate();

	virtual void OnRefresh();

private:
	CMTime		m_LastUpdate;
	CMTime		m_ActivateTime;
};

#endif

