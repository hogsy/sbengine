#include "PCH.h"
#include "WClientModWnd_Briefing.h"
#include "WFrontEndMod.h"

MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModBriefing, CMWnd)

CMWnd_ModBriefing::CMWnd_ModBriefing() {}

void CMWnd_ModBriefing::OnPaint(CRC_Util2D* _pUtil2D, const CClipRect &_Clip, const CRct &_Client)
{
	if(m_ClientInfo.m_pFrontEnd)
		m_ClientInfo.m_pFrontEnd->RenderHelpButtons(this, _pUtil2D, _Clip);
	CMWnd::OnPaint(_pUtil2D, _Clip, _Client);
}


////////////////////////
#if defined(PLATFORM_XBOX1) && defined(_DEBUG) && defined(NEVER)



void DrawTextBox(CContextHandler &_rHandler, CRC_Util2D* _pRCUtil, const CClipRect &_Clip, CStr _String)
{
	CRC_Font *pFont = _rHandler.m_pWindow->GetFont("TEXT");

	CPnt Center;
	Center.x = _Clip.clip.p1.x/2;
	Center.y = _Clip.clip.p1.y/2;
	CPnt RectSize(350, 100);
	CPnt RectSize2(350/2, 100/2);
	CPnt BorderSize(2, 2);
	CRct Rect;
	Rect.p0 = Center - RectSize2;
	Rect.p1 = Center + RectSize2;

	// shadow
	CRct OffsetedRect = Rect;
	OffsetedRect.p0 += CPnt(10,10);
	OffsetedRect.p1 += CPnt(10,10);
	_pRCUtil->Rect(_Clip, OffsetedRect, 0x40000000);

	// border
	_pRCUtil->Rect(_Clip, Rect, 0xff000000);

	// rect
	Rect.p0 += BorderSize;
	Rect.p1 -= BorderSize;
	_pRCUtil->Rect(_Clip, Rect, 0xff404040);

	CPnt TextLocation = Center;
	TextLocation.x -= _pRCUtil->TextWidth(pFont, _String)/2;
	TextLocation.y -= _pRCUtil->TextHeight(pFont, _String)/2;

	_pRCUtil->Text_Draw(_Clip, pFont, _String, TextLocation.x, TextLocation.y, 0, 0xffffffff, 0xffffffff, 0xffffffff);
}

//
// XBox Live stuff
//
CContextHandler::CContextHandler()
{
	m_pBaseContext = NULL;
	m_pCurrentContext = NULL;
	m_pQueuedContext = NULL;
}

void CContextHandler::Init(CContext *_pBaseContext)
{
	m_pBaseContext = _pBaseContext;
	m_pCurrentContext = _pBaseContext;
	m_pBaseContext->Init();
}


//
//
//
void CContextHandler::GetContexts(TArray<CContext*> &_rList)
{
	CContext *pCurrent = m_pCurrentContext;
	while(pCurrent)
	{
		_rList.Add(pCurrent);
		pCurrent = pCurrent->m_pParentContext;
	}
}

//
//
//
void CContextHandler::SetContext(CContext *_pContext)
{
	// get all contexts
	TArray<CContext*> lContexts;
	TArray<CContext*> lPrevContexts;
	GetContexts(lContexts);
	GetContexts(lPrevContexts);
	
	// remove contexts that will still be active
	CContext *pCurrent = _pContext;
	while(pCurrent)
	{
		for(int32 i = 0; i < lContexts.Len(); i++)
		{
			if(lContexts[i] == pCurrent)
			{
				lContexts.Del(i);
				lPrevContexts.Add(pCurrent);
				break;
			}
		}
		pCurrent = pCurrent->m_pParentContext;
	}

	// shutdown
	for(int32 i = 0; i < lContexts.Len(); i++)
	{
		lContexts[i]->Shutdown();
		delete lContexts[i];
	}

	// apply new context
	m_pCurrentContext = _pContext;
	
	// init new contexts
	GetContexts(lContexts);
	for(int32 i = 0; i < lContexts.Len(); i++)
	{
		bool Init = true;
		for(int32 j = 0; j < lPrevContexts.Len(); j++)
		{
			if(lContexts[i] == lPrevContexts[j])
			{
				Init = false;
				break;
			}
		}

		if(Init)
			lContexts[i]->Init();
	}

	// Active context
	m_pCurrentContext->OnActivate();
}

void CContextHandler::Update()
{
	
	CMTime Time = CMTime::GetCPU();
	m_Time = Time.GetTime();

	//
	CContext *pCurrent = m_pCurrentContext;
	while(pCurrent)
	{
		pCurrent->Update();
		pCurrent = pCurrent->m_pParentContext;
	}

	if(m_pQueuedContext)
	{
		SetContext(m_pQueuedContext);
		m_pQueuedContext = NULL;
	}
}


void CContextHandler::Paint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	if(m_pCurrentContext)
		m_pCurrentContext->OnPaint(_pRCUtil, _Clip, _Client);
}

void CContextHandler::ProcessButton(int32 _Button)
{
	if(m_pCurrentContext)
		m_pCurrentContext->OnButton(_Button);
}

//
//
//

CContext *CContext::GetBackContext()
{
	CContext *pContext = m_pParentContext;

	while(pContext)
	{
		if(!(pContext->m_Flags&FLAG_NOBACK))
			break;

		pContext = pContext->m_pParentContext;
	}

	return pContext;
}

void CContext::GoBack()
{
	m_rHandler.QueueContext(GetBackContext());
}

void CContext::OnButton(int32 _Button)
{
	if(_Button == SKEY_GUI_CANCEL)
		GoBack();
}


//
//
//
void CContext_ChooseUser::Init()
{
	m_CurrentSelection = 0;
}

void CContext_ChooseUser::Update()
{
	XOnlineGetUsers(m_aUsers, (DWORD*)&m_NumUsers);
	
	if(m_CurrentSelection >= m_NumUsers) m_CurrentSelection = m_NumUsers-1;
	if(m_CurrentSelection < 0) m_CurrentSelection = 0;
}

void CContext_ChooseUser::Shutdown()
{
}

void CContext_ChooseUser::OnButton(int32 _Button)
{
	if(_Button == SKEY_GUI_UP) m_CurrentSelection--;
	if(_Button == SKEY_GUI_DOWN) m_CurrentSelection++;

	if(_Button == SKEY_GUI_OK && m_CurrentSelection >= 0 && m_CurrentSelection < m_NumUsers)
	{
		CContext_Logon *pLogonContext = DNew(CContext_Logon) CContext_Logon(m_rHandler, this);
		pLogonContext->m_User = m_aUsers[m_CurrentSelection];
		m_rHandler.QueueContext(pLogonContext);
	}

	CContext::OnButton(_Button);
}

void CContext_ChooseUser::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	// Render
	CRC_Font *pFont = m_rHandler.m_pWindow->GetFont("TEXT");

	CRct Rect;
	Rect.p0 = _Clip.clip.p0;
	Rect.p1 = _Clip.clip.p1;
	_pRCUtil->Rect(_Clip, Rect, 0xff404040);

	_pRCUtil->Text_Draw(_Clip, pFont, L"Choose User", 50, 50, 0, 0xffffffff, 0xffffffff, 0xffffffff);

	for(int32 i = 0; i < m_NumUsers; i++)
	{
		CStr GamerTag = CStr(m_aUsers[i].szGamertag).Unicode();
		uint32 Color = 0xffffffff;
	
		if(i == m_CurrentSelection)
			Color = 0xffff4040;

		_pRCUtil->Text_Draw(_Clip, pFont, (wchar*)GamerTag.StrW(), 100, 100+i*25, 0, Color, Color, Color);
	}
}


//
//
//
void CContextTask::Update()
{
	if(!m_TaskHandle)
	{
		GoBack();
		return;
	}

	HRESULT Res = XOnlineTaskContinue(m_TaskHandle);
	if(Res != XONLINETASK_S_RUNNING)
		ProcessResult(Res);
}

void CContextTask::Shutdown()
{
	if(m_TaskHandle)
	{
		XOnlineTaskClose(m_TaskHandle);
		m_TaskHandle = 0;
	}
}

void CContextTask::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	if(m_pParentContext)
		m_pParentContext->OnPaint(_pRCUtil, _Clip, _Client);
	
	if(m_TaskName.Len() > 0)
		DrawTextBox(m_rHandler, _pRCUtil, _Clip, m_TaskName);
}

//
//
//
void CContext_Logon::Init()
{
	m_Flags = FLAG_NOBACK;
	m_TaskName = L"Logging on";

	// get all users
	m_GotConnection = false;

	DWORD aServices[] = {XONLINE_BILLING_OFFERING_SERVICE};
	XONLINE_USER aUsers[XONLINE_MAX_LOGON_USERS] = {0};
	memcpy(&aUsers[0], &m_User, sizeof(XONLINE_USER));
	XOnlineLogon(aUsers, aServices, 1, NULL, &m_TaskHandle);
}

void CContext_Logon::ProcessResult(HRESULT _Result)
{
	if(_Result == XONLINE_S_LOGON_CONNECTION_ESTABLISHED)
	{
		if(!m_GotConnection)
		{
			m_GotConnection = true;

			// next context
			CContext_EnumContent *pContext = DNew(CContext_EnumContent) CContext_EnumContent(m_rHandler, this);
			m_rHandler.QueueContext(pContext);
		}
	}
	else
	{
		// PROCESS ERRORS
		CContext_StringDisplay *pStringContext = DNew(CContext_StringDisplay) CContext_StringDisplay(m_rHandler, m_pParentContext);
		m_rHandler.QueueContext(pStringContext);
		pStringContext->m_String.Capture(L"Logon Failure");
	}
}

//
//
//
void CContext_EnumContent::Init()
{
	m_Flags = FLAG_NOBACK;
	m_TaskName = L"Enumerating content";

	// enumerate offerings
	XONLINEOFFERING_ENUM_PARAMS OfferingParams;
	OfferingParams.dwOfferingType = XONLINE_OFFERING_SUBSCRIPTION|XONLINE_OFFERING_CONTENT;
	OfferingParams.dwBitFilter = 0xFFFFFFFF;
	OfferingParams.dwDescriptionIndex = 0;
	OfferingParams.wStartingIndex = 0;
	OfferingParams.wMaxResults = 100;

    // Determine the buffer size required for enumeration
    m_BufferSize = XOnlineOfferingEnumerateMaxSize(&OfferingParams, 0); 
    
    // Allocate the enumeration buffer
    m_pEnumBuffer = DNew(BYTE) BYTE[m_BufferSize];
	HRESULT Res = XOnlineOfferingEnumerate(0, &OfferingParams, m_pEnumBuffer, m_BufferSize, NULL, &m_TaskHandle);
}

void CContext_EnumContent::ProcessResult(HRESULT _Result)
{
	if(FAILED(_Result))
		GoBack();
	else if(_Result == XONLINETASK_S_SUCCESS)
	{
		BOOL More = FALSE;
		HRESULT Result = XOnlineOfferingEnumerateGetResults(m_TaskHandle, &m_pOfferingInfo, (DWORD*)&m_nOfferings, &More);

		CContext_ChooseContent *pContext = DNew(CContext_ChooseContent) CContext_ChooseContent(m_rHandler, m_pParentContext);
		m_rHandler.QueueContext(pContext);
		pContext->m_NumOfferings = m_nOfferings;
		pContext->m_pOfferingInfo = m_pOfferingInfo;
	}
}

//
//
//
/*
void CContext_EnumDetails::Init()
{
	m_CurrentDetail = 0;
	Enum(m_CurrentDetail);
}

void CContext_EnumDetails::Enum(int32 _Index)
{
	m_pContentTask->m_nOfferings[_Index].
	XOnlineOfferingDetails(UserIndex, offerid, XC_LANGUAGE_ENGLISH, 0, NULL, 65536, NULL, m_TaskHandle);
}

void CContext_EnumDetails::ProcessResult()
{
	if(FAILED(_Result))
		GoBack();
	else
	{

	}
}*/

//
//
//
void CContext_ChooseContent::Init()
{
	m_CurrentSelection = 0;
}

void CContext_ChooseContent::Update()
{}

void CContext_ChooseContent::Shutdown()
{

}

void CContext_ChooseContent::OnButton(int32 _Button)
{
	if(_Button == SKEY_GUI_UP) m_CurrentSelection--;
	if(_Button == SKEY_GUI_DOWN) m_CurrentSelection++;

	if(m_CurrentSelection >= m_NumOfferings) m_CurrentSelection = m_NumOfferings-1;
	if(m_CurrentSelection < 0) m_CurrentSelection = 0;

	if(_Button == SKEY_GUI_OK)
	{
		// do fetch details here
		/*
		CContext_Logon *pLogonContext = DNew(CContext_Logon) CContext_Logon(m_rHandler, this);
		pLogonContext->m_User = m_aUsers[m_CurrentSelection];
		m_rHandler.QueueContext(pLogonContext);
		*/
	}
	
	CContext::OnButton(_Button);
}

void CContext_ChooseContent::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{

	CRct Rect;
	Rect.p0 = _Clip.clip.p0;
	Rect.p1 = _Clip.clip.p1;
	_pRCUtil->Rect(_Clip, Rect, 0xff404040);

	//
	CRC_Font *pFont = m_rHandler.m_pWindow->GetFont("TEXT");
	_pRCUtil->Text_Draw(_Clip, pFont, L"Choose Content", 50, 50, 0, 0xffffffff, 0xffffffff, 0xffffffff);

	for(int32 i = 0; i < m_NumOfferings; i++)
	{
		CStr OfferString = CStrF("%8x", m_pOfferingInfo[m_CurrentSelection]->OfferingId).Unicode();
		uint32 Color = 0xffffffff;
	
		if(i == m_CurrentSelection)
			Color = 0xffff8080;

		_pRCUtil->Text_Draw(_Clip, pFont, (wchar*)OfferString.StrW(), 100, 100+i*25, 0, Color, Color, Color);
	}
}

//
//
//
void CContext_StringDisplay::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	if(m_pParentContext)
		m_pParentContext->OnPaint(_pRCUtil, _Clip, _Client);

	DrawTextBox(m_rHandler, _pRCUtil, _Clip, m_String);

	//CRC_Font *pFont = m_rHandler.m_pWindow->GetFont("CLAIRVAUX");
	//_pRCUtil->Text_Draw(_Clip, pFont, m_String, 50, 50, 0, 0xffffffff, 0xffffffff, 0xffffffff);
}

//
//
//
MRTC_IMPLEMENT_DYNAMIC(CMWnd_XBoxLive, CMWnd)
CMWnd_XBoxLive::CMWnd_XBoxLive()
{
	m_Client = 0;
	m_ContextHandler.m_pWindow = this;
	XOnlineStartup(NULL);
}

CMWnd_XBoxLive::~CMWnd_XBoxLive()
{
	XOnlineCleanup();
}

int CMWnd_XBoxLive::OnMessage(const CMWnd_Message* _pMsg)
{
	if(_pMsg->m_Msg == WMSG_KEY)
	{
		CScanKey Key;
		Key.m_ScanKey32 = _pMsg->m_Param0;
		Key.m_Char = _pMsg->m_Param1;
		CScanKey OriginalKey;
		OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
		OriginalKey.m_Char = _pMsg->m_Param3;
		
		if(Key.IsDown())
			m_ContextHandler.ProcessButton(Key.GetKey9());
	}

	return 0;
}

void CMWnd_XBoxLive::OnPaint(CRC_Util2D *_pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	if(!m_Client)
	{
		// INIT HERE
		m_Client = 1;
		m_State = 1;
		m_ContextHandler.Init(DNew(CContext_ChooseUser) CContext_ChooseUser(m_ContextHandler));
	}

	m_ContextHandler.Update();

	CMWnd::OnPaint(_pRCUtil, _Clip, _Client);
	m_ContextHandler.Paint(_pRCUtil, _Clip, _Client);
}


//////////////////////////
#endif // PLATFORM_XBOX1
