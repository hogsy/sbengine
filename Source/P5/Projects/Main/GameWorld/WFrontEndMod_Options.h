
#ifndef __WFRONTENDMOD_OPTIONS_H
#define __WFRONTENDMOD_OPTIONS_H

#include "WFrontEndMod_Menus.h"

#if defined(PLATFORM_XBOX1)
// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Live_Message : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	CStr m_Message;
	CStr m_NextScreen;
	
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void OnCreate();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Live_Account : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	virtual bool OnButtonPress(CMWnd *_pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);
	virtual void OnCreate();
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Live_PassCode : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	int8 m_aPasscode[4];
	int32 m_Current;

	void Reset();
	void PerformCheck();
	void Add(int8 _p);

	virtual void OnCreate();
	virtual	int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Live_Login : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	int32 m_State;
	virtual void OnCreate();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Live_UpdateContentList : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	//CMWnd_CubeMenu_Live_Login();
	virtual void OnCreate();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

// ---------------------------------------------------------------------------------------------
/*
class CMWnd_CubeMenu_Live_Content : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	virtual bool GetListItem(int32 &_Index, CStr &_Name, bool _Focus);
	virtual void OnCreate();
};*/
#ifdef XBOX_SUPPORT_DLCONTENT
// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Live_DownloadContent : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	virtual void OnCreate();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};
#endif

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Friends : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	int32 m_CurrentSelected;

	virtual bool OnButtonPress(CMWnd *_pFocus);
	virtual bool OnOtherButtonPress(CMWnd *_pFocus, int32 _Button);

	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);
	virtual void AfterItemRender(int32 _Index);
	virtual void OnCreate();
	virtual void OnDestroy();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Friends_TakeAction : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	enum
	{
		EType_GotInvite=0,
		EType_Offline,
		EType_Joinable,
		EType_GotRequest,
		EType_SentRequest,

		EType_ConfirmRemove,
		EType_ConfirmBlock,
		EType_ConfirmCancel, // do we need this?

		EAction_Bail=0,
		EAction_Join,
		EAction_Block,
		EAction_Remove,
		EAction_ConfirmBlock,
		EAction_ConfirmRemove,
		EAction_AcceptInvite,
		EAction_DeclineInvite,
		EAction_AcceptRequest,
		EAction_DeclineRequest,
		EAction_CancelRequest
	};

	int32 m_Action;
	int32 m_Type;
	int32 m_NumButtons;

	virtual bool OnButtonPress(CMWnd *_pFocus);
	virtual void OnCreate();
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);

	/*
	virtual void OnDestroy();
	*/
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

#endif // PLATFORM_XBOX1


#endif
