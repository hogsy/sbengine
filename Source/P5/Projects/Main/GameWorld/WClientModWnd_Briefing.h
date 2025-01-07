#ifndef __WCLIENTMODWND_BRIEFING_H
#define __WCLIENTMODWND_BRIEFING_H

#include "MWinCtrlMod.h"
#include "WClientModWnd.h"

// -------------------------------------------------------------------
//  CMWnd_ModBriefing
// -------------------------------------------------------------------
class CMWnd_ModBriefing : public CMWnd
{
	MRTC_DECLARE;

public:
	CMWnd_ClientInfo m_ClientInfo;
	CMWnd_ClientInfo *GetClientInfo()
	{
		return &m_ClientInfo;
	}

	CMWnd_ModBriefing();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};



/*
#if defined(PLATFORM_XBOX1) && defined(_DEBUG) && defined(NEVER)
	#include <xtl.h>
	#include <Xonline.h>

	//
	// XBox Live stuff
	//
	class CContext;

	class CContextHandler
	{
		CContext *m_pBaseContext;
		CContext *m_pCurrentContext;
		CContext *m_pQueuedContext;

		void GetContexts(TArray<CContext*> &_rList);
		void SetContext(CContext *_pContext);
	public:
		CMWnd *m_pWindow;
		fp32 m_Time;

		CContextHandler();
		void Init(CContext *_pBaseContext);
		void QueueContext(CContext *_pContext) { m_pQueuedContext = _pContext; }
		void Update();
		void Paint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
		void ProcessButton(int32 _Button);
	}; 

	class CTask
	{
	public:
		XONLINETASK_HANDLE m_hTask;
		void Update();
		bool Failed();
		bool IsOK();
		bool IsDone();
	};

	class CLiveHandler
	{
	public:
		enum
		{
			LOGINSTATUS_LOGGEDIN,
			LOGINSTATUS_LOGGINGIN,

		};

		int32 GetLoginStatus();

		bool IsLoggedOn();
		void UpdateTasks();
	};

	//
	//
	//
	class CContext
	{
	public:
		enum
		{
			FLAG_NOBACK=1,	// if set, you can't go back to this context
		};

		uint32 m_Flags;

		CContextHandler &m_rHandler;
		CContext *m_pParentContext;

		CContext(CContextHandler &_rHandler, CContext *_pParent = NULL)
		: m_rHandler(_rHandler), m_pParentContext(_pParent), m_Flags(0)
		{}

		virtual void Init(){};		// called when the context is created
		virtual void Update(){};	// every frame
		virtual void Shutdown(){};	// when the context is destroyed

		virtual void OnButton(int32 _Button);	// when a button is pressed and this context is the current
												// Actions taken: Queues the parent context if SKEY_GUI_CANCEL is pressed

		virtual void OnActivate(){};			// when the context becomes the current context
		
		virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client){};

		CContext *GetBackContext(); // Gets the context that we should back to
		void GoBack(); // queues the parent context
	};

	//
	//
	//
	class CContext_ChooseUser : public CContext
	{
	public:
		CContext_ChooseUser(CContextHandler &_rHandler, CContext *_pParent=NULL)
		: CContext(_rHandler, _pParent) {}

		virtual void Init();
		virtual void Update();
		virtual void Shutdown();
		virtual void OnButton(int32 _Button);
		virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);

		XONLINE_USER m_aUsers[32];
		int32 m_NumUsers;

		int32 m_MadeSelection;
		int32 m_CurrentSelection;
	};

	//
	//
	//
	class CContextTask : public CContext
	{
	public:
		XONLINETASK_HANDLE m_TaskHandle;
		CStr m_TaskName;

		CContextTask(CContextHandler &_rHandler, CContext *_pParent=NULL)
		: CContext(_rHandler, _pParent), m_TaskHandle(0) { m_Flags = FLAG_NOBACK; }

		virtual void ProcessResult(HRESULT _Result) {};
		virtual void Update();
		virtual void Shutdown();
		virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
	};

	//
	//
	//
	class CContext_Logon : public CContextTask
	{
	public:
		XONLINE_USER m_User;
		bool m_GotConnection;

		CContext_Logon(CContextHandler &_rHandler, CContext *_pParent=NULL)
		: CContextTask(_rHandler, _pParent) {}

		virtual void Init();
		virtual void ProcessResult(HRESULT _Result);
	};
	
	//
	//
	//
	class CContext_EnumContent : public CContextTask
	{
		uint8 *m_pEnumBuffer;
		int32 m_BufferSize;
	public:
		XONLINEOFFERING_INFO **m_pOfferingInfo;
		int32 m_nOfferings;
		
		CContext_EnumContent(CContextHandler &_rHandler, CContext *_pParent=NULL)
		: CContextTask(_rHandler, _pParent) { m_pEnumBuffer = NULL; }

		virtual void Init();
		virtual void ProcessResult(HRESULT _Result);
	};

	//
	//
	//
	//
	//class CContext_EnumDetails : public CContextTask
	//{
	//public:
	//	CContext_EnumContent *m_pContentTask;
	//	int32 m_CurrentDetail;

	//	CContext_EnumDetails(CContextHandler &_rHandler, CContext *_pParent=NULL)
	//	: CContextTask(_rHandler, _pParent) {}

	//	void Enum(int32 _Index);

	//	virtual void Init();
	//	virtual void ProcessResult(HRESULT _Result);
	//};

	//
	//
	//
	class CContext_ChooseContent : public CContext
	{
	public:
		XONLINEOFFERING_INFO **m_pOfferingInfo;
		int32 m_NumOfferings;

		int32 m_CurrentSelection;

		CContext_ChooseContent(CContextHandler &_rHandler, CContext *_pParent=NULL)
		: CContext(_rHandler, _pParent) {}

		virtual void Init();
		virtual void Update();
		virtual void Shutdown();
		virtual void OnButton(int32 _Button);
		virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
	};

	//
	//
	//
	class CContext_StringDisplay : public CContext
	{
	public:
		CContext_StringDisplay(CContextHandler &_rHandler, CContext *_pParent=NULL)
		: CContext(_rHandler, _pParent) {}

		CStr m_String;
		virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
	};

	//
	//
	//
	class CMWnd_XBoxLive : public CMWnd
	{
	MRTC_DECLARE;
		int32 m_Client;
		int32 m_State;

	public:
		CMWnd_ClientInfo m_ClientInfo;
		CMWnd_ClientInfo *GetClientInfo()
		{
			return &m_ClientInfo;
		}

		CContextHandler m_ContextHandler;

		CMWnd_XBoxLive();
		virtual ~CMWnd_XBoxLive();
		virtual aint OnMessage(const CMWnd_Message* _pMsg);
		virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
	};

#endif
*/
#endif
