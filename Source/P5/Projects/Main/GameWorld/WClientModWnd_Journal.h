
#ifndef _INC_MWINCLIENTWNDJOURNALMOD
#define _INC_MWINCLIENTWNDJOURNALMOD


#include "WClientModWnd.h"
#include "MWinCtrlMod.h"
#include "../Exe/WGameContextMain.h"

// -------------------------------------------------------------------
//  Phobos: CMWnd_ModMissionJournal,
// -------------------------------------------------------------------
/*
class CMWnd_P4ItemList : public CMWnd_List2
{
	MRTC_DECLARE;
	// Which object to get
	spCRegistry_Dynamic		m_spItems;
	CWClient_Mod*			m_pWClient;
	CPnt					m_ListPos;
	int						m_ItemSpacing;
public:
	int m_Item0Count;

	CMWnd_P4ItemList();

	//virtual void SetItemCount(int _nItems);
	virtual void SetList(spCRegistry_Dynamic _spItems);

	//virtual void EvaluateKey(CMWnd_Param* _pParam, CStr _Key, CStr _Value);
	virtual void OnPaintList(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, CRC_Font *_pF);
	virtual void OnPaintItem(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, int _iItem, CRC_Font *_pF);

	//virtual void SelectItem(int _iItem);
	inline void SetClient(CWClient_Mod* _pWClient) { m_pWClient = _pWClient; }
};
*/
/*
#define CMWnd_ModMissionJournalParent CMWnd_ModTexture
class CMWnd_ModMissionJournal : public CMWnd_ModMissionJournalParent, public CMWnd_ClientInfo
{
	MRTC_DECLARE;
	// x = 0 in real world = CenterX on map
	fp32		m_CenterX;
	// y = 0 in real world = CenterY on map
	fp32		m_CenterY;

	// Scale of coordinate in x,y direction
	fp32		m_ScaleX;
	fp32		m_ScaleY;
	// So final pos (x,y) = (m_CenterX + RealX * m_ScaleX, m_CenterY + RealY * m_ScaleY)

	// Sprite for positioning pointer thingy....
	CStr	m_PointerName;
	CPnt	m_PointerRange;				
	fp32		m_AngleOffset;
	CMWnd_P4ItemList	m_InventoryList;
	void PaintInventory(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
	void PaintMap(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, class CWO_MapInfo *_pInfo);
	void PaintObjectives(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
public:
	CMWnd_ModMissionJournal();
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void PaintTexture(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	void DrawText(const char *_pSt, CPnt _Pnt, CRC_Font *_pFont, CRC_Util2D* _pRCUtil, CClipRect _Clip,
		fp32 _Fade, fp32 _Size = -1, int Color = 0x00808080);
};*/

typedef CMWnd_CubeLayout CMWnd_CubeIngameScreen_Parent;
class CMWnd_CubeIngameScreen : public CMWnd_CubeIngameScreen_Parent
{
	MRTC_DECLARE;
public:
	CMWnd_ClientInfo m_ClientInfo;
	CMWnd_ClientInfo *GetClientInfo()
	{
		return &m_ClientInfo;
	}

	CCubeUser m_CubeUser;
	virtual void DoMapping(CWFrontEnd_Mod *_pFrontEnd);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual	int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
};

//
//
//
class CMWnd_CubeCommonLayout : public CMWnd_CubeIngameScreen
{
	MRTC_DECLARE;
public:
	CMWnd *m_pDescription;
	//TArray<CMWnd*> m_lButtons;
	
	//virtual bool GetListItem(int32 &_Index, CStr &_Name, CStr &_Desc) {return false;}; // should return false when no more there are nomore items

	virtual void OnCreate();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

//
//
//
class CMWnd_Mission_Objectives : public CMWnd_CubeCommonLayout
{
	MRTC_DECLARE;
public:
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);
};

//
//
//
class CMWnd_Mission_Inventory : public CMWnd_CubeCommonLayout
{
	MRTC_DECLARE;
public:
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);
};


//
//
//
class CMWnd_Mission_PhoneBook : public CMWnd_CubeCommonLayout
{
	MRTC_DECLARE;
public:
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);
};


//
//
//
class CMWnd_Mission_Collectibles : public CMWnd_CubeCommonLayout
{
	MRTC_DECLARE;
public:
	CExtraContentKey m_Key;

	virtual void OnCreate();
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);
};

//
//
//
class CMWnd_Mission_Map : public CMWnd_CubeIngameScreen
{
	MRTC_DECLARE;
public:
	//virtual bool GetListItem(int32 &_Index, CStr &_Name, CStr &_Desc);
	virtual void OnCreate();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

/*
typedef CMWnd_CubeLayout CMWnd_ModMissionJournalParent;
class CMWnd_ModMissionJournal : public CMWnd_ModMissionJournalParent, public CMWnd_ClientInfo
{
	MRTC_DECLARE;
public:
	CCubeUser m_CubeUser;

	CMWnd_ModMissionJournal();
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};*/


#endif
