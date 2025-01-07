
#include "PCH.h"

#if defined(PLATFORM_WIN) && !defined(PLATFORM_XENON)

#include "MSystem_Core.h"
#include "MContentContext.h"

#ifdef PLATFORM_XBOX
	#include <xtl.h>
#else
	#include <windows.h>
	#include <Shlobj.h>
	#include <Shellapi.h>
#endif


// -------------------------------------------------------------------
//  CSaveContentContext_Win32
// -------------------------------------------------------------------

class CSaveContentContext_Win32 : public CContentContext, public MRTC_Thread
{
protected:
	CStr m_SavePath;

	enum
	{
		REQ_TYPE_VOID = 0,

		REQ_TYPE_ENUM_CONTAINERS,
		REQ_TYPE_CONTAINER_CREATE,
		REQ_TYPE_CONTAINER_DELETE,

		REQ_TYPE_CONTAINER_MOUNT,
		REQ_TYPE_CONTAINER_QUERY_SIZE,
		REQ_TYPE_CONTAINER_QUERY_PATH,

		REQ_TYPE_ENUM_INSTANCES,
		REQ_TYPE_INSTANCE_QUERY_SIZE,
		REQ_TYPE_INSTANCE_DELETE,
		REQ_TYPE_INSTANCE_READ,
		REQ_TYPE_INSTANCE_WRITE,

		REQ_TYPE_MAX,
	};

	class CRequest
	{
	public:
		CRequest() : m_Type(REQ_TYPE_VOID), m_pNext(NULL)
		{
		}

		void Reset()
		{
			m_Container.Clear();
			m_Instance.Clear();
			m_pDestArg = NULL;
			m_LenDestArg = 0;

			m_ResultError = CONTENT_ERR_PENDING;
			m_ResultSize = 0;
			m_lResultEnum.Destroy();
		}

		int m_Type;
		CRequest* m_pNext;

		CStr m_Container;
		CStr m_Instance;
		void* m_pDestArg;
		uint32 m_LenDestArg;

		uint32 m_ResultError;
		uint32 m_ResultSize;
		TArray<CStr> m_lResultEnum;

		NThread::CEventAutoReset m_Done;
	};

	MRTC_CriticalSection m_RequestLock;
	TThinArray<CRequest> m_lRequests;
	CRequest* m_pRequestHead;
	CRequest* m_pRequestTail;
	NThread::CEventAutoResetReportable m_RequestAvail;
	
	int AllocRequestInstance();
	void QueueRequest(CRequest* _pReq);

	int AllocRequest(int _Type);
	int AllocRequest(int _Type, CStr _Container);
	int AllocRequest(int _Type, CStr _Container, CStr _Instance);
	int AllocRequest(int _Type, CStr _Container, CStr _Instance, void* _pData, uint _DataLen);
	void FreeRequest(int _iRequest);

	const char* Thread_GetName() {return "CSaveContentContext_Win32";}
public:
	CSaveContentContext_Win32() : m_pRequestHead(NULL), m_pRequestTail(NULL) {}
	void Create()
	{
		MACRO_GetSystem;
		m_SavePath = pSys->m_spRegistry->GetValue("SAVEPATH") + "SaveGames\\";
		m_lRequests.SetLen(16);
		Thread_Create();
	}

	void Destroy()
	{
		Thread_Destroy();
	}

	int Thread_Main();

	int StorageSelect(uint _iUser, uint _SaveSize) {return -1;}

	int EnumerateContainers(uint _iUser);
	int ContainerCreate(uint _iUser, CStr _Container);
	int ContainerDelete(uint _iUser, CStr _Container);

	int ContainerMount(uint _iUser, CStr _Container);
	int ContainerQuerySize(uint _iUser, CStr _Container);
	int ContainerQueryPath(uint _iUser, CStr _Container);

	int EnumerateInstances(uint _iUser, CStr _Container);
	int InstanceQuerySize(uint _iUser, CStr _Container, CStr _Instance);
	int InstanceDelete(uint _iUser, CStr _Container, CStr _Instance);
	int InstanceRead(uint _iUser, CStr _Container, CStr _Instance, void* _pDest, uint32 _MaxLen);
	int InstanceWrite(uint _iUser, CStr _Container, CStr _Instance, void* _pSrc, uint32 _Len);

	int GetRequestStatus(int _iRequest);
	int BlockOnRequest(int _iRequest);

	int PeekRequestData(int _iRequest, CContentQueryData* _pRet);
	int GetRequestData(int _iRequest, CContentQueryData* _pRet);
	int GetRequestEnumeration(int _iRequest, TArray<CStr>& _lData);
};

// -------------------------------------------------------------------
//  CSystemWin32
// -------------------------------------------------------------------

class SYSTEMDLLEXPORT CSystemWin32 : public CSystemCore
{
	MRTC_DECLARE;

protected:
#ifdef PLATFORM_WIN_PC
	int m_CmdShow;
	void* m_hInstance;
	void* m_hPrevInstance;
	void* m_hIcon;
	spCWin32AppInfo m_spWinInfo;
#endif

	CSaveContentContext_Win32 m_SaveGameContext;
public:

	DECLARE_OPERATOR_NEW


	CSystemWin32();
	virtual ~CSystemWin32();
	virtual void Create(void* this_inst, void* prev_inst, char* _cmdline, 
		 int _cmdshow, const char* _pAppClassName);
	virtual void Destroy();
	virtual void CreateSystems();
	virtual CStr GetModuleName(CStr _Name, bool _bAppendSystem = false);
	virtual aint OnMessage(const CSS_Msg& _Msg);

	virtual void Render(CDisplayContext* _pDC = NULL, int _Context = 0);
	virtual void Refresh();

	virtual int ReadOptions();				// 0 == Corrupt, 1 == Ok, 2 == NoFile
	virtual int WriteOptions();			// 0 == Failed, 1 == Ok, 2 == Corrupt

	virtual void DC_InitList();
	virtual void DC_InitVidModeFromEnvironment();
#ifdef PLATFORM_XBOX1
	// If the game is run from an xbox the video mode is initialized
	virtual void DC_SetVidModeFromDashboard();
#endif
#ifdef PLATFORM_WIN_PC
	virtual void Win32_CreateInfo();
	virtual void* Win32_GethWnd();
	virtual void* Win32_GethInstance();
	virtual void* Win32_GethAppIcon();
#endif

	virtual CStr GetUserFolder();
	virtual CStr TranslateSavePath(CStr _Path);

	virtual class CContentContext* GetContentContext(int _iContent);

	virtual void Parser_SysShowMemory();
	virtual void Parser_MemoryDebug(int _Flags);
	virtual void Parser_TestMemory();
	virtual void ExitProcess();
};

#ifdef PLATFORM_XBOX1

#include "../RndrXbox/MRndrXbox.h"
#include "../Classes/Render/MRenderUtil.h"
#include "../Classes/Win/MWinGrph.h"

void XTL_DisplayFatal(CCExceptionLog* _pExLog)
{
//	MRTC_SAFECREATEOBJECT_NOEX(spSys, "CSystem_Win32", CSystem);
//	if (!spDisplay) Error_static("XTL_DisplayFatal", "Failed to create CSystem_Win32");

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) Error_static("XTL_DisplayFatal", "No system");
	pSys->DC_Set(-1);

	LogFile("(::XTL_DisplayFatal) Progress 1...");

	MRTC_SAFECREATEOBJECT_NOEX(spDisplay, "CDisplayContextXbox", CDisplayContextXbox);
	LogFile(CStrF("(::XTL_DisplayFatal) spDisplay %.8", spDisplay));

	if (!spDisplay) Error_static("XTL_DisplayFatal", "Failed to create CDisplayContextXbox");

	LogFile("(::XTL_DisplayFatal) Progress 2...");

	spDisplay->Create();
	LogFile("(::XTL_DisplayFatal) Progress 3...");
	spDisplay->SetMode(2);
	int iWindow = spDisplay->SpawnWindow(0);
	spDisplay->ClearFrameBuffer(-1);
	CImage* pImg = spDisplay->GetFrameBuffer();
	if (!pImg) Error_static("XTL_DisplayFatal", "No framebuffer.");


	LogFile("(::XTL_DisplayFatal) " + pImg->IDString());
	LogFile(CStrF("(::XTL_DisplayFatal) Modulo %d", pImg->GetModulo()));

	LogFile("(::XTL_DisplayFatal) Progress 4...");

	CRenderContext* pRC = spDisplay->GetRenderContext();
	if (!pRC) Error_static("XTL_DisplayFatal", "No render.");

	CRC_Font Font;
//	Font.ReadFromFile(pSys->m_ExePath + "sbz1\\fonts\\clairvaux.xfc");
	Font.ReadFromFile(pSys->m_ExePath + "sbz1\\fonts\\mini_fritz.xfc");

	CRC_Viewport VP;
	VP.SetView(pImg);
	pRC->BeginScene(&VP);
	pRC->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	CRC_Util2D Util2D;
	Util2D.Begin(pRC, &VP);

	CClipRect cr(pImg->GetClipRect());
	Util2D.Rect(cr, CRct(0,0,640,480), 0xff000000);
	Util2D.Rect(cr, CRct(0,80,640,400), 0xff002000);
	Util2D.Line(cr, CPnt(0,80), CPnt(640,80), 0xff008000);
	Util2D.Line(cr, CPnt(0,400), CPnt(640,400), 0xff008000);

	Util2D.Text(pImg->GetClipRect(), &Font, 40, 30, "Enclave exception report", 0xffffffff, 40.0f);
	int y = 82;
#ifndef M_RTM
	const CCExceptionInfo* pInfo = _pExLog->GetFirst();
	while(pInfo)
	{
//		pImg->DebugText(pImg->GetClipRect(), CPnt(40, y), CStrF("%s, %s, %s", pInfo->m_SourcePos.Str(), pInfo->m_Location.Str(), pInfo->m_Message.Str()).Str(), 0xff30ff30);
		Util2D.Text(pImg->GetClipRect(), &Font, 40, y, CStrF("%s", pInfo->m_Message.Str()).Str(), 0xffffffff, 25.0f);
		Util2D.Text(pImg->GetClipRect(), &Font, 40, y+23, CStrF("%s", pInfo->m_SourcePos.Str()).Str(), 0xff30ff30, 17.0f);
		Util2D.Text(pImg->GetClipRect(), &Font, 40, y+38, CStrF("%s", pInfo->m_Location.Str()).Str(), 0xff30ff30, 17.0f);
		pInfo = pInfo->m_pNext;
		y += 60;
	}
#endif

	Util2D.End();
	pRC->EndScene();

//	pImg->DebugText(pImg->GetClipRect(), CPnt(320, 240), "X", 0xffffffff);
	
//	for(int i = 0; i < 25; i++)
//		pImg->DebugText(pImg->GetClipRect(), CPnt(40, i*8), "i", 0xff30ff30);

/*	int y = 40;
	const CCExceptionInfo* pInfo = _pExLog->GetFirst();
	while(pInfo)
	{
		pImg->DebugText(pImg->GetClipRect(), CPnt(40, y), CStrF("%s, %s, %s", pInfo->m_SourcePos.Str(), pInfo->m_Location.Str(), pInfo->m_Message.Str()).Str(), 0xff30ff30);
		pInfo = pInfo->m_pNext;
	}*/
/*
	RECT Rect;
	Rect.left = 0;
	Rect.right = pImg->GetWidth();
	Rect.bottom = pImg->GetHeight();
	Rect.top = 0;

	POINT Pnt;
	Pnt.x = 0;
	Pnt.y = 0;

	TThinArray<uint8> lFrameBuffer2;
	lFrameBuffer2.SetLen(pImg->GetHeight() * pImg->GetModulo());

	void* pFrameBuffer = pImg->Lock();
	if (!pFrameBuffer)
		Error_static("XTL_DisplayFatal", "No framebuffer");
	LogFile(CStrF("(::XTL_DisplayFatal) pFB %.8x", pFrameBuffer ));

	XGSwizzleRect
	(
		pFrameBuffer, pImg->GetModulo(), &Rect, lFrameBuffer2.GetBasePtr(), 
		pImg->GetWidth(), pImg->GetHeight(), &Pnt, pImg->GetPixelSize()
	);

//	memcpy(pFrameBuffer, lFrameBuffer2.GetBasePtr(), lFrameBuffer2.ListSize());

	pImg->Unlock();
*/
	spDisplay->PageFlip();

//	OutputDebugString("(::XTL_DisplayFatal) Going to sleep...");
	while(1);
}

#endif

#ifdef PLATFORM_WIN_PC
static bool IsDebuggerAttached()
{
    DWORD dw;
    __asm
    {
        push eax    // Preserve the registers
        push ecx
        mov eax, fs:[0x18]  // Get the TIB's linear address
        mov eax, dword ptr [eax + 0x30]
        mov ecx, dword ptr [eax]    // Get the whole DWORD
        mov dw, ecx // Save it
        pop ecx // Restore the registers
        pop eax
    }
    // The 3rd byte is the byte we really need to check for the
    // presence of a debugger.
    // Check the 3rd byte
    return ((dw & 0x00010000) != 0);
}
#endif

// -------------------------------------------------------------------
//  CWin32AppInfo
// -------------------------------------------------------------------
#ifdef PLATFORM_WIN

#ifdef PLATFORM_WIN_PC

IMPLEMENT_OPERATOR_NEW(CWin32AppInfo);


CWin32AppInfo::CWin32AppInfo()
{
	m_hIcon	= NULL;
	m_hMainWnd = NULL;
	m_hInstance = NULL;
	m_hPrevInstance = NULL;
	m_CmdShow = 0;
	m_pCmdLine = NULL;
}
#endif

// -------------------------------------------------------------------
//  CSystemWin32
// -------------------------------------------------------------------

// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CSystemWin32, CSystemCore);


IMPLEMENT_OPERATOR_NEW(CSystemWin32);



CSystemWin32::CSystemWin32()
{
#ifdef PLATFORM_WIN_PC
	m_CmdShow = 0;
	m_pCmdLine = 0;
	m_hInstance = 0;
	m_hPrevInstance = 0;
	m_hIcon = 0;
#endif
}

CSystemWin32::~CSystemWin32()
{
//	Destroy();
}

#if defined(PLATFORM_XBOX) && defined(M_DEMO_XBOX)
CXboxDemoParams g_XboxDemoParams;
#endif

CStr CSystemWin32::GetModuleName(CStr _Name, bool _bSystemPath)
{
	if (_bSystemPath && m_DefaultSystem.Len())
		return _Name + "_" + m_DefaultSystem + ".dll";
	else
		return _Name + ".dll";

}

void CSystemWin32::Create(void* this_inst, void* prev_inst, char* _cmdline, 
	 int _cmdshow, const char* _pAppClassName)
{
	MSCOPE(CSystemWin32::Create, SYSTEMWIN32);

//	LogFile("(CSystemWin32::Create)");
#ifdef PLATFORM_WIN_PC
	m_hInstance = this_inst;
	m_hPrevInstance = prev_inst;
	m_CmdShow = _cmdshow;
#endif

/*	for(int i = 0; i < 20; i++)
		Sleep(1000);*/

	// Change directory to that of the executable.
	CStr CommandLine; 
#ifdef PLATFORM_XBOX
	m_ExePath = "D:\\";
	unsigned long iLaunchDataType = 0;
#ifdef PLATFORM_XBOX1
	LAUNCH_DATA Data;
	memset(&Data, 0, sizeof(LAUNCH_DATA));
	XGetLaunchInfo(&iLaunchDataType, &Data);
#if defined(PLATFORM_XBOX) && defined(M_DEMO_XBOX)
	g_XboxDemoParams.m_bLaunchedFromDemoLauncher = false;
	g_XboxDemoParams.m_KioskMode = false;
	g_XboxDemoParams.m_LaunchData.dwTimeout = 45 * 1000;
#if 0
	g_XboxDemoParams.m_KioskMode = false;
	g_XboxDemoParams.m_bLaunchedFromDemoLauncher = true;
	g_XboxDemoParams.m_LaunchData.dwTimeout = 5000;
	g_XboxDemoParams.m_LaunchData.dwRunmode = 2;
#endif
#endif
#endif

#ifndef PLATFORM_XENON
	switch(iLaunchDataType)
	{
	case LDT_FROM_DASHBOARD:
		{
			LD_FROM_DASHBOARD *pData = (LD_FROM_DASHBOARD *)&Data;
			LogFile("(CSystemWin32::Create) Launching Xbox image from Dashboard");
			
		}
		break;
	case LDT_FROM_DEBUGGER_CMDLINE:
		{
			LD_FROM_DEBUGGER_CMDLINE *pData = (LD_FROM_DEBUGGER_CMDLINE *)&Data;
			LogFile("(CSystemWin32::Create) Launching Xbox image from Debugger");
		}
		break;
	case LDT_TITLE:
		{
#ifdef M_DEMO_XBOX
			LogFile("(CSystemWin32::Create) Launching Xbox image from DemoLauncher");
			memcpy(&g_XboxDemoParams.m_LaunchData, &Data, sizeof(LAUNCH_DATA));
			CStr Launch = g_XboxDemoParams.m_LaunchData.szLaunchedXBE;
			//g_XboxDemoParams.m_LaunchData.dwRunmode
			CCFile File;
			int ClusterSize = XGetDiskClusterSize(m_OptionsFilename.GetDevice());
			if(g_XboxDemoParams.m_LaunchData.szLauncherXBE[0] == 0)
			{
				// Launching from Title Launcher
				g_XboxDemoParams.m_LaunchData.dwTimeout = 45 * 1000;
				g_XboxDemoParams.m_bLaunchedFromDemoLauncher = false;
			}
			else
				g_XboxDemoParams.m_bLaunchedFromDemoLauncher = true;

			if(g_XboxDemoParams.m_LaunchData.dwRunmode == 1)
				g_XboxDemoParams.m_KioskMode = true;
//			M_TRACEALWAYS(Launch);
			CStr Path = Launch.GetPath();
			if(Path != "")
				m_ExePath = Path;
//			m_ExePath = "D:\\Enclave\\";
#else
			LogFile("(CSystemWin32::Create) Launching Xbox image from Title");
#endif
		}
		break;
	}
#endif
	LogFile("(CSystemWin32::Create) Path is set to: " + m_ExePath);
/*
#ifndef	M_RTM
#ifndef PLATFORM_XENON
XNetStartupParams xnsp;
	memset( &xnsp, 0, sizeof( xnsp ) );
	xnsp.cfgSizeOfStruct = sizeof( XNetStartupParams );
	xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
	INT err = XNetStartup( &xnsp );
#endif	// M_RTM
#endif
*/

#else
	CommandLine = GetCommandLine();
	for (char* p = (char*)CommandLine; p && *p; p++)
		if (*p == '/') *p = '\\';

	m_ExePath = CommandLine;
	m_ExePath.Trim();

	LogFile("(CSystemWin32::Create) Commandline: " + m_ExePath);
	if (m_ExePath.CompareSubStr(CStr('"')) == 0) 
		m_ExePath = CStr(CommandLine).GetBounded(CStr('"'));
	else
		if (m_ExePath.Find(" ") >= 0) m_ExePath = m_ExePath.Copy(0, m_ExePath.Find(" "));

	if (m_ExePath.FindReverse("\\") >= 0) 
		m_ExePath = m_ExePath.Copy(0, m_ExePath.FindReverse("\\")+1);
	else
		m_ExePath = CDiskUtil::GetDir();

	m_SystemPath = m_ExePath;
	// System parsing
#ifdef PLATFORM_WIN
	{
		CStr Org = m_ExePath;
		if (Org.Len() && Org[Org.Len() - 1] == '\\')
			Org = Org.Left(Org.Len() - 1);

		int iOne = Org.FindReverse("\\");;
		if (iOne >= 0)
		{
			CStr One = Org.Copy(0, iOne);
			int iTwo = One.FindReverse("\\");
			if (iTwo >= 0)
			{
				CStr Two = One.Copy(0, iTwo +1);
				CStr System;
				System = One.RightFrom(iTwo+1);
				if (System.UpperCase() == "SYSTEM")
				{
					m_DefaultSystem = Org.RightFrom(iOne + 1);
					m_ExePath = Two;
				}
			}
		}
	}
#endif

	LogFile("(CSystemWin32::Create) ExePath: " + m_ExePath);
	if (!SetCurrentDirectory(m_ExePath)) Win32Err("- (" + m_ExePath + ", " + CommandLine + ")");
#endif

	CSystemCore::Create(_cmdline, _pAppClassName);

//	LogFile("(CSystemWin32::Create) Done");
}

CStr CSystemWin32::GetUserFolder()
{
#ifdef PLATFORM_XENON
	return "CACHE:\\";
#endif
#ifdef PLATFORM_WIN_PC
	TCHAR szPath[MAX_PATH];

	CStr ProgramFiles;
	if(SUCCEEDED(SHGetFolderPath(NULL, 
								CSIDL_APPDATA|CSIDL_FLAG_CREATE, 
								NULL, 
								0, 
								szPath))) 
	{
#ifdef M_DEMO
		return CStr() + szPath + "\\" + "Starbreeze\\" "DarknessDemo\\";
#else
		return CStr() + szPath + "\\" + "Starbreeze\\" "Darkness\\";
#endif
	}
#endif
	return "";
}

CStr CSystemWin32::TranslateSavePath(CStr _Path) 
{
	MACRO_GetSystem;
	CStr SavePath = pSys->GetRegistry()->GetValue("SAVEPATH");
	int ExePathLen = m_ExePath.Len();
	if (SavePath != "" && m_ExePath.CompareNoCase(_Path.Left(ExePathLen)) == 0)
	{
		return SavePath + _Path.RightFrom(ExePathLen);
	}
	return _Path;
}

void CSystemWin32::Destroy()
{
	MSCOPE(CSystemWin32::Destroy, SYSTEMWIN32);
	
	LogFile("(CSystemWin32::Destroy)");
	M_TRY
	{ 
		DC_Set(-1); 
	} 
	M_CATCH(
	catch (CCException) 
	{
	};
	)
#ifdef PLATFORM_WIN_PC
	if (m_spWinInfo != NULL) MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spWinInfo, "SYSTEM.WIN32APPINFO");
	m_spWinInfo = NULL;
#endif
	CSystemCore::Destroy();
	LogFile("(CSystemWin32::Destroy) Done");
}

void CSystemWin32::CreateSystems()
{
	MSCOPE(CSystemWin32::CreateSystems, SYSTEMWIN32);
	
	CSystemCore::CreateSystems();
#ifdef PLATFORM_WIN_PC
	Win32_CreateInfo();

	const CRegistry* pKey = GetEnvironment()->Find("BREAKONASSERT");
	if (pKey)
	{
		MRTC_GOM()->m_bBreakOnAssert = (pKey->GetThisValuei() != 0);
	}
	else
	{
	#if defined(NDEBUG)
		// If not running through a debugger, don't break on failed asserts (let them report the error and continue)
		MRTC_GOM()->m_bBreakOnAssert = IsDebuggerAttached();
	#endif
	}
	LogFile(CStrF("[SystemWin32] BreakOnAssert: %d\n", MRTC_GOM()->m_bBreakOnAssert));
#endif
}

aint CSystemWin32::OnMessage(const CSS_Msg& _Msg)
{
#ifdef PLATFORM_WIN_PC
	switch(_Msg.m_Msg)
	{
	case CSS_MSG_WIN32_PREDESTROYWINDOW :
		{
			if (m_iMainWindow >= 0 && _Msg.m_Params[1] == m_iMainWindow)
			{
				System_BroadcastMessage(CSS_Msg(CSS_MSG_WIN32_NEWMAINWINDOW, NULL, _Msg.m_Params[1]));
				m_spWinInfo->m_hMainWnd = NULL;
			}
		}
		break;

	case CSS_MSG_WIN32_CREATEDWINDOW :
		{
			if (m_iMainWindow >= 0 && _Msg.m_Params[1] == m_iMainWindow)
			{
				m_spWinInfo->m_hMainWnd = m_spDisplay->Win32_GethWnd(m_iMainWindow);
				System_BroadcastMessage(CSS_Msg(CSS_MSG_WIN32_NEWMAINWINDOW, _Msg.m_Params[0], _Msg.m_Params[1]));
			}
		}
		break;
	}
#endif
	return CSystemCore::OnMessage(_Msg);
}

void CSystemWin32::Render(CDisplayContext* _pDC, int _Context)
{
	MSCOPE(CSystemWin32::Render, SYSTEMWIN32);
	
	if (IsRendering()) return; // Error("Render", "Render-recusion.");
	if (!m_spApp) return;
	if (!_pDC || (_pDC == m_spDisplay))
	{
		_pDC = m_spDisplay;
		if (!_pDC) return;

		if (m_iMainWindow < 0)
		{
			m_iMainWindow = m_spDisplay->SpawnWindow();

			if(m_iMainWindow >= 0)
			{
				// FIXME!!! The CSS_MSG_WIN32_CREATEDWINDOW message should take care of this, but
				// m_ioMainWindow isn't set when it is called...
#ifdef PLATFORM_WIN_PC
				m_spWinInfo->m_hMainWnd = m_spDisplay->Win32_GethWnd(m_iMainWindow);
				System_BroadcastMessage(CSS_Msg(CSS_MSG_WIN32_NEWMAINWINDOW, aint(m_spWinInfo->m_hMainWnd), m_iMainWindow));
#endif
			}
		}
//		ConOutL(CStrF("hWnd %d", m_spWinInfo->m_hMainWnd));
	}
	if (!_pDC) return;
//	if (m_iMainWindow <= 0) return;
//	_pDC->SelectWindow(m_iMainWindow);
	if (_pDC->GetMode() < 0) return;

	// Return false if displaycontext isn't valid (not created yet, re-creating, etc.)
	if(!_pDC->IsValid())
		return;

	m_bIsRendering = true;
	M_TRY
	{
		CImage *spImg = _pDC->GetFrameBuffer();
		if (spImg != NULL)
		{
			CClipRect Clip = spImg->GetClipRect();
			M_TRY
			{ 
				m_spApp->OnRender(_pDC, spImg, Clip, _Context);
			}
			M_CATCH(
			catch(CCExceptionGraphicsHAL) 
			{
				Error("Render", "Render failed. (1)"); 
			}
			catch(CCException)
			{ 
				throw;
//				Error("Render", "Render failed. (2)"); 
			};
			)
		};
	}
	M_CATCH(
	catch(CCException)
	{
		m_bIsRendering = false;
		throw;
	};
	)
	m_bIsRendering = false;
};

void CSystemWin32::Refresh()
{
	MSCOPE(CSystemWin32::Refresh, SYSTEMWIN32);
	
	CSystemCore::Refresh();

	if (m_RefreshRecursion) return;
	m_RefreshRecursion++;
	M_TRY
	{
		if (m_spDisplay != NULL)
		{
/*			if (m_iMainWindow >= 0) 
				m_spWinInfo->m_hMainWnd = m_spDisplay->Win32_GethWnd(m_iMainWindow);
			else
				m_spWinInfo->m_hMainWnd = NULL;*/
		}
	}
	M_CATCH(
	catch(CCException)
	{
		m_RefreshRecursion--;
		throw;
	}
	)
	m_RefreshRecursion--;
}

int CSystemWin32::ReadOptions()
{
	// 0 == Corrupt, 1 == Ok, 2 == NoFile

	// Option registry added to the environment
	if (CDiskUtil::FileExists(m_OptionsFilename))
	{
		M_TRY
		{
			bool bCorruptFile = false;
	#ifdef PLATFORM_XBOX1
#if defined(M_DEMO_XBOX)
			CStr keyfile = "Z:\\Options.key";
#else
			CStr keyfile = "T:\\Options.key";
#endif
			if(CDiskUtil::FileExists(keyfile))
			{
				CCFile File;
				File.Open(m_OptionsFilename, CFILE_READ | CFILE_BINARY);
				int32 length = File.Length();
				TThinArray<uint8> lData;
				lData.SetLen(length);
				File.ReadLE(lData.GetBasePtr(), length);
				File.Close();
				
				XCALCSIG_SIGNATURE Signature;
				HANDLE hCalcSig = XCalculateSignatureBegin(NULL);
				DWORD res = XCalculateSignatureUpdate(hCalcSig, lData.GetBasePtr(), length);
				res =  XCalculateSignatureEnd(hCalcSig, &Signature);
				
				File.Open(keyfile, CFILE_READ | CFILE_BINARY);
				if (File.Length() != XCALCSIG_SIGNATURE_SIZE)
					bCorruptFile = true;
				else
				{
					uint8 val;
					for(int k=0; k<XCALCSIG_SIGNATURE_SIZE ;k++)
					{
						File.Read(val);
						if(val != Signature.Signature[k])
						{
							bCorruptFile = true;
							break;
						}
					}
				}
				File.Close();
			}
			else
			{
				bCorruptFile = true;
			}
	#endif
			if(!bCorruptFile)
			{	
				m_spRegistry->ReadRegistryDir("OPT", m_OptionsFilename);
				
				spCRegistry spEnv2 = m_spRegistry->Find("OPT");
				if (!spEnv2) Error("CreateSystems", "Internal error. (1)");
			}
			else
				return 0;
		}
		M_CATCH(
		catch(CCException)
		{
			return 0;
		}
		)
	}
	else
	{
		ConOutL("(CSystemCore::CreateSystems) " + m_OptionsFilename + " doesn't exist.");
		return 2;
	}

	return 1;
}

int CSystemWin32::WriteOptions()
{
	// // 0 == Failed, 1 == Ok, 2 == Corrupt

#ifdef PLATFORM_XBOX
//	CStr tmpName = "T:\\Options.tmp";
#else
	CStr tmpName = m_OptionsFilename;
#endif

	M_TRY
	{
		//spCRegistry spEnv = m_spRegistry->Find("ENV");
		int ClusterSize = 16384;
#ifdef PLATFORM_XBOX1
		ClusterSize = XGetDiskClusterSize(m_OptionsFilename.GetDevice());
		if (CDiskUtil::FileExists(m_OptionsFilename))
			CDiskUtil::DelFile(m_OptionsFilename);
#if defined(M_DEMO_XBOX)
		if (CDiskUtil::FileExists("Z:\\Options.key"))
			CDiskUtil::DelFile("Z:\\Options.key");
#else
		if (CDiskUtil::FileExists("T:\\Options.key"))
			CDiskUtil::DelFile("T:\\Options.key");
#endif
#endif
		{
			CCFile File;
			File.OpenExt(m_OptionsFilename, CFILE_WRITE | CFILE_UNICODE, NO_COMPRESSION, NORMAL_COMPRESSION, 0, -1, ClusterSize);
			m_spRegistry->WriteRegistryDir("OPT", &File);
			File.Close();
		}

#ifdef PLATFORM_XBOX1

		// ----------------------------------------------------------------------------
		// Calculate the signature
		CCFile File;
		File.Open(m_OptionsFilename, CFILE_READ | CFILE_BINARY);
		int32 length = File.Length();
		TThinArray<uint8> lData;
		lData.SetLen(length);
		File.ReadLE(lData.GetBasePtr(), length);
		File.Close();

		HANDLE hCalcSig = XCalculateSignatureBegin(NULL);
		DWORD res = XCalculateSignatureUpdate(hCalcSig, lData.GetBasePtr(), length);

		XCALCSIG_SIGNATURE Signature;
		res =  XCalculateSignatureEnd(hCalcSig, &Signature);
		
#if defined(M_DEMO_XBOX)
		File.OpenExt("Z:\\Options.key", CFILE_WRITE | CFILE_BINARY, NO_COMPRESSION, NORMAL_COMPRESSION, 0, -1, ClusterSize);
#else
		File.OpenExt("T:\\Options.key", CFILE_WRITE | CFILE_BINARY, NO_COMPRESSION, NORMAL_COMPRESSION, 0, -1, ClusterSize);
#endif
		for(int i = 0; i < XCALCSIG_SIGNATURE_SIZE; i++)
			File.Write((uint8)Signature.Signature[i]); 
		File.Close();	
#endif
	}
	M_CATCH(
	catch(...)	
	{
		LogFile("(CSystemCore::SaveOptions) Error writing options " + m_OptionsFilename);
		CDiskUtil::DelFile(m_OptionsFilename);
		return 0;	
	}
	)
	return 1;
}

/*
class CDisplayContextXDF : public CDisplayContext
{
protected:
	MRTC_DECLARE;
public:

	spCRenderContext m_spRenderContext;
	CImage m_Image;

	CDisplayContextXDF()
	{
		m_Image.Create(640, 480, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
	}

	~CDisplayContextXDF()
	{
	}

	virtual CPnt GetScreenSize(){return CPnt(640, 480);};

	virtual void Create()
	{
	}

	virtual void SetMode(int nr)
	{
	}

	virtual void ModeList_Init()
	{
	}

	virtual int SpawnWindow(int _Flags = 0)
	{
		return 0;
	}

	virtual void DeleteWindow(int _iWnd)
	{
	}

	virtual void SelectWindow(int _iWnd)
	{
	}

	virtual void SetWindowPosition(int _iWnd, CRct _Rct)
	{
	}

	virtual void SetPalette(spCImagePalette _spPal)
	{
	}

	virtual CImage* GetFrameBuffer()
	{
		return &m_Image;
	}

	virtual void ClearFrameBuffer(int _Buffers = (CDC_CLEAR_COLOR | CDC_CLEAR_ZBUFFER | CDC_CLEAR_STENCIL), int _Color = 0)
	{
	}

	virtual CRenderContext* GetRenderContext()
	{
		return m_spRenderContext;
	}

	virtual int Win32_CreateFromWindow(void* _hWnd, int _Flags = 0)
	{
		return 0;
	}

	virtual int Win32_CreateWindow(int _WS, void* _pWndParent, int _Flags = 0)
	{
		return 0;
	}

	virtual void Win32_ProcessMessages()
	{
	}

	virtual void* Win32_GethWnd(int _iWnd = 0)
	{
		return 0;
	}

	virtual void Parser_Modes()
	{
	}

	virtual void Register(CScriptRegisterContext & _RegContext)
	{
		CDisplayContext::Register(_RegContext);
	}

};

MRTC_IMPLEMENT_DYNAMIC(CDisplayContextXDF, CDisplayContext);
*/

#include "Raster/MRCCore.h"

class CDisplayContextNULL : public CDisplayContext
{
protected:
	MRTC_DECLARE;
public:

	CImage m_Image;

	CDisplayContextNULL()
	{
		m_Image.Create(640, 480, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
	}

	~CDisplayContextNULL()
	{
	}

	virtual CPnt GetScreenSize() { return CPnt(640, 480); };
	virtual CPnt GetMaxWindowSize() { return CPnt(640, 480); };

	virtual void Create()
	{
		CDisplayContext::Create();
	}

	virtual void SetMode(int nr)
	{
	}

	virtual void ModeList_Init()
	{
	}

	virtual int SpawnWindow(int _Flags = 0)
	{
		return 0;
	}

	virtual void DeleteWindow(int _iWnd)
	{
	}

	virtual void SelectWindow(int _iWnd)
	{
	}

	virtual void SetWindowPosition(int _iWnd, CRct _Rct)
	{
	}

	virtual void SetPalette(spCImagePalette _spPal)
	{
	}

	virtual CImage* GetFrameBuffer()
	{
		return &m_Image;
	}

	virtual void ClearFrameBuffer(int _Buffers = (CDC_CLEAR_COLOR | CDC_CLEAR_ZBUFFER | CDC_CLEAR_STENCIL), int _Color = 0)
	{
	}

	int GetMode()
	{
		return 0;
	}


	class CRenderContextNULL : public CRC_Core
	{
	public:

		CDisplayContextNULL *m_pDisplayContext;
		

		void Internal_RenderPolygon(int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CVec4Dfp32* _pCol = NULL, const CVec4Dfp32* _pSpec = NULL, /*const fp32* _pFog = NULL,*/
									const CVec4Dfp32* _pTV0 = NULL, const CVec4Dfp32* _pTV1 = NULL, const CVec4Dfp32* _pTV2 = NULL, const CVec4Dfp32* _pTV3 = NULL, int _Color = 0xffffffff)
		{
		}

		CRenderContextNULL()
		{
		}

		~CRenderContextNULL()
		{
		}

		void Create(CObj* _pContext, const char* _pParams)
		{
			CRC_Core::Create(_pContext, _pParams);

			MACRO_GetSystem;

			m_Caps_TextureFormats = -1;
			m_Caps_DisplayFormats = -1;
			m_Caps_ZFormats = -1;
			m_Caps_StencilDepth = 8;
			m_Caps_AlphaDepth = 8;
			m_Caps_Flags = -1;

			m_Caps_nMultiTexture = 16;
			m_Caps_nMultiTextureCoords = 8;
			m_Caps_nMultiTextureEnv = 8;

		}

		const char* GetRenderingStatus() { return ""; }
		virtual void Flip_SetInterval(int _nFrames){};

		void Attrib_Set(CRC_Attributes* _pAttrib){}
		void Attrib_SetAbsolute(CRC_Attributes* _pAttrib){}
		void Matrix_SetRender(int _iMode, const CMat4Dfp32* _pMatrix){}

		virtual void Texture_PrecacheFlush(){}
		virtual void Texture_PrecacheBegin( int _Count ){}
		virtual void Texture_PrecacheEnd(){}
		virtual void Texture_Precache(int _TextureID){}

		virtual int Texture_GetBackBufferTextureID() {return 0;}
		virtual int Texture_GetFrontBufferTextureID() {return 0;}
		virtual int Texture_GetZBufferTextureID() {return 0;}
		virtual int Geometry_GetVBSize(int _VBID) {return 0;}

		void Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles){}
		void Render_IndexedTriangleStrip(uint16* _pIndices, int _Len){}
		void Render_IndexedWires(uint16* _pIndices, int _Len){}
		void Render_IndexedPolygon(uint16* _pIndices, int _Len){}
		void Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen){}
		void Render_VertexBuffer(int _VBID){}
		void Render_Wire(const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, CPixel32 _Color){}
		void Render_WireStrip(const CVec3Dfp32* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color){}
		void Render_WireLoop(const CVec3Dfp32* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color){}

		virtual void Geometry_PrecacheFlush(){}
		virtual void Geometry_PrecacheBegin( int _Count ){}
		virtual void Geometry_PrecacheEnd(){}
		virtual void Geometry_Precache(int _VBID){}
		virtual CDisplayContext* GetDC(){ return m_pDisplayContext;}

		void Register(CScriptRegisterContext & _RegContext){}

	};

	TPtr<CRenderContextNULL> m_spRenderContext;


	virtual CRenderContext* GetRenderContext(class CRCLock* _pLock)
	{
		if (!m_spRenderContext)
		{
			m_spRenderContext = MNew(CRenderContextNULL);
			m_spRenderContext->m_pDisplayContext = this;
			m_spRenderContext->Create(this, "");

		}

		return m_spRenderContext;
	}

	virtual int Win32_CreateFromWindow(void* _hWnd, int _Flags = 0)
	{
		return 0;
	}

	virtual int Win32_CreateWindow(int _WS, void* _pWndParent, int _Flags = 0)
	{
		return 0;
	}

	virtual void Win32_ProcessMessages()
	{
	}

	virtual void* Win32_GethWnd(int _iWnd = 0)
	{
		return 0;
	}

	virtual void Parser_Modes()
	{
	}

	virtual void Register(CScriptRegisterContext & _RegContext)
	{
		CDisplayContext::Register(_RegContext);
	}

};

MRTC_IMPLEMENT_DYNAMIC_NO_IGNORE(CDisplayContextNULL, CDisplayContext);

void CSystemWin32::DC_InitList()
{
	static int Test = (mint)&g_ClassRegCDisplayContextNULL;
#if defined(PLATFORM_XENON)
	m_lspDC.Add(MNew4(CDisplayContextDesc, "", "PS3", "PS3 DisplayContext", "CDisplayContextPS3") );
#elif defined(PLATFORM_XBOX1)
	m_lspDC.Add(MNew4(CDisplayContextDesc, "", "Xenon", "Xenon DisplayContext", "CDisplayContextXenon") );
	m_lspDC.Add(MNew4(CDisplayContextDesc, "", "Xbox", "Xbox DisplayContext", "CDisplayContextXbox") );
#else
	m_lspDC.Add(MNew4(CDisplayContextDesc, "RndrGL.DLL", "OpenGL", "OpenGL DisplayContext", "CDisplayContextGL") );
	m_lspDC.Add(MNew4(CDisplayContextDesc, "", "XDF", "XDF DisplayContext", "CDisplayContextXDF") );
	m_lspDC.Add(MNew4(CDisplayContextDesc, "", "NULL", "NULL DisplayContext", "CDisplayContextNULL") );
#endif
//	m_lspDC.Add(MNew4(CDisplayContextDesc, "RndrGlide.DLL", "Glide3x", "Glide DisplayContext", "CDisplayContextGlide") );
//	m_lspDC.Add(MNew4(CDisplayContextDesc, "RndrSW.DLL", "Software", "Software DisplayContext", "CDisplayContextSW") );
//	m_lspDC.Add(MNew4(CDisplayContextDesc, "RndrD3D.DLL", "Direct3D", "Direct3D DisplayContext", "CDisplayContextD3D") );
//	m_lspDC.Add(MNew4(CDisplayContextDesc, "RndrD3D8.DLL", "Direct3D8", "Direct3D8 DisplayContext", "CDisplayContextD3D8") );
	//m_lspDC.Add(MNew4(CDisplayContextDesc, "RndrD3D9.DLL", "Direct3D9", "Direct3D9 DisplayContext", "CDisplayContextD3D9") );
//	m_lspDC.Add(MNew4(CDisplayContextDesc, "RndrGLStripped.DLL", "OpenGLStripped", "OpenGL DisplayContext", "CDisplayContextGLStripped") );
};

void CSystemWin32::DC_InitVidModeFromEnvironment()
{
#ifdef PLATFORM_XBOX1
	DC_SetVidModeFromDashboard();
#else
	CSystemCore::DC_InitVidModeFromEnvironment();
#endif
}


#ifdef PLATFORM_XBOX1

void CSystemWin32::DC_SetVidModeFromDashboard()
{
	MSCOPE(CSystemCore::DC_SetVidModeFromDashboard, SYSTEMWIN32);
	   
	// Standard resolution 
	int width	= 640;
	int height	= 480;
	int bpp		= 32;

	DWORD _dwPack			= XGetAVPack();
    DWORD _dwVideoFlags		= XGetVideoFlags();
	DWORD _dwVideoStandard	= XGetVideoStandard();

	fp32 ScreenAspect = 1.0;
	fp32 WideScreenAspect = 1.0;
	fp32 PixelAspect = 1.0;
		
	if (0) 
	{
		// PAL Settings
		if(_dwVideoStandard == XC_VIDEO_STANDARD_PAL_I)
		{
			if(_dwVideoFlags & XC_VIDEO_FLAGS_PAL_60Hz)	
			{
				m_spDisplay->SetRefreshRate(60);		//Used in MWindowD3D8.cpp (bool CD3D8Window::ReCreate) 
				width	= 720;
				height	= 480;
				ScreenAspect *= 19.0/18.0;
				WideScreenAspect *= 11.0/10.0;
			}
			else
			{
				m_spDisplay->SetRefreshRate(50);
				width	= 720;
				height	= 576;
			//	ScreenAspect *= 40.0/39.0;
			//	WideScreenAspect *= 19.0/18.0;
			
			}
		}
		else if((_dwVideoStandard == XC_VIDEO_STANDARD_NTSC_J) || (_dwVideoStandard == XC_VIDEO_STANDARD_NTSC_M))
		{
			m_spDisplay->SetRefreshRate(60);
			width	= 720;
			height	= 480;

			ScreenAspect *= 40.0/39.0;
			WideScreenAspect *= 19.0/18.0;
		//	Aspect *= 10.0/11.0;
		}
	}
	else
	{
		// Override the optimal resolution to use as little memory as possible
		if(_dwVideoStandard == XC_VIDEO_STANDARD_PAL_I)
		{
			if(_dwVideoFlags & XC_VIDEO_FLAGS_PAL_60Hz)	
			{
				m_spDisplay->SetRefreshRate(60);		//Used in MWindowD3D8.cpp (bool CD3D8Window::ReCreate) 
				width	= 640;
				height	= 480;
				ScreenAspect *= 1.044070f;
				WideScreenAspect *= 1.052485f;
			}
			else 
			{
				m_spDisplay->SetRefreshRate(50);
				width	= 640;
				height	= 480;
				ScreenAspect *= 1.033048f;
				WideScreenAspect *= 1.028226f;
			
			}
		}
		else if((_dwVideoStandard == XC_VIDEO_STANDARD_NTSC_J) || (_dwVideoStandard == XC_VIDEO_STANDARD_NTSC_M))
		{
			m_spDisplay->SetRefreshRate(60);
			width	= 640;
			height	= 480;

			ScreenAspect *= 1.043576f;
			WideScreenAspect *= 1.031637f;
		}
	}


/*	if((_dwVideoFlags & XC_VIDEO_FLAGS_HDTV_480p))
	{
		// Only allow 480p since the others will take up too much memory
		int width	= 720;//720
		int height	= 480;
		m_spDisplay->SetProgressive(true);
	}
	else
	{
		// Set the scan to be interlaced (standard)
		m_spDisplay->SetProgressive(false);
	}*/

#ifndef M_DEMO_XBOX
	//
	// kma: Force No widescreen is when you don't have time to finish everything so the gui looks
	// good in it.
	//
	MACRO_GetRegisterObject( CSystem, pSys, "SYSTEM" );
	int32 bForceNoWideScreen = pSys->GetEnvironment()->GetValuei("VID_FORCENOWIDESCREEN", 0);

	// Widescreen settings
	if(_dwVideoFlags & XC_VIDEO_FLAGS_WIDESCREEN && !bForceNoWideScreen)
	{
		m_spDisplay->SetWidescreen(true);
		ScreenAspect = WideScreenAspect * ((16.0/9.0) / (4.0/3.0));
	}
#endif
	PixelAspect *= 1.0 / (((double)width / (double)height) / ((4.0/3.0) * (ScreenAspect)));

	int ForceWidth = pSys->GetEnvironment()->GetValuei("VID_FORCEWIDTH", -1);
	int ForceHeight = pSys->GetEnvironment()->GetValuei("VID_FORCEHEIGHT", -1);
	fp32 ForceScreenAspect = pSys->GetEnvironment()->GetValuef("VID_FORCESCREENASPECT", -1);
	fp32 ForcePixelAspect = pSys->GetEnvironment()->GetValuef("VID_FORCEPIXELASPECT", -1);

	if (ForceWidth > 0)
		width = ForceWidth;
	if (ForceHeight > 0)
		height = ForceHeight;
	if (ForceScreenAspect > 0)
		ScreenAspect = ForceScreenAspect;
	if (ForcePixelAspect > 0)
		PixelAspect = ForcePixelAspect;
	
	m_spDisplay->SetScreenAspect(ScreenAspect);
	m_spDisplay->SetPixelAspect(PixelAspect);

//	 || (_dwVideoFlags & XC_VIDEO_FLAGS_HDTV_720p) || (_dwVideoFlags & XC_VIDEO_FLAGS_HDTV_1080i)
	// Check for HDTV


	// Set the video mode
	int format = CImage::BPP2Format(bpp);
	int iMode = m_spDisplay->ModeList_Find(width, height, format, -1);
	if (iMode < 0) iMode = m_spDisplay->ModeList_Find(width, height, -1, -1);
	if (iMode < 0) iMode = m_spDisplay->ModeList_Find(width, -1, -1, -1);
	if (iMode < 0) iMode = m_spDisplay->ModeList_Find(640, 480, -1, -1);
	if (iMode < 0)
	{
		ConOut("(CSystemCore) Can't find video mode. Choosing mode 0.");
		iMode = 0;
	}
	else
		ConOutL(CStrF("(CSystemCore) Best match for VID_MODE %d,%d,%d is mode %d", width, height, bpp, iMode));


	ConOutL(CStrF("(CSystemCore) Initializing video mode %d",iMode));
	m_spDisplay->SetMode(iMode);
	m_bIsRendering = false;

	// Notify all subsystems
	System_BroadcastMessage(CSS_Msg(CSS_MSG_POSTCHANGEDISPLAYMODE));
}

#endif

#ifdef PLATFORM_WIN_PC


void CSystemWin32::Win32_CreateInfo()
{
	MSCOPE(CSystemWin32::Win32_CreateInfo, SYSTEMWIN32);
	
	if (m_spWinInfo != NULL) return;

	m_spWinInfo = MNew(CWin32AppInfo);
	if (!m_spWinInfo) MemError("Win32_CreateInfo");
	m_spWinInfo->m_hMainWnd = NULL;
	m_spWinInfo->m_hInstance = m_hInstance;
	m_spWinInfo->m_hPrevInstance = m_hPrevInstance;
	m_spWinInfo->m_CmdShow = m_CmdShow;
	m_spWinInfo->m_pCmdLine = m_pCmdLine;

	if (!MRTC_GOM()->RegisterObject((CReferenceCount*) m_spWinInfo, "SYSTEM.WIN32APPINFO"))
	{
		m_spWinInfo = NULL;
		Error("Win32_CreateInfo", "Failed to register win32appinfo-object.");
	}
}

void* CSystemWin32::Win32_GethWnd()
{
	if (!m_spDisplay) return NULL;
	if (m_iMainWindow <= 0) return NULL;
	if (m_spWinInfo != NULL) m_spWinInfo->m_hMainWnd = m_spDisplay->Win32_GethWnd(m_iMainWindow);
	return m_spDisplay->Win32_GethWnd();
}

void* CSystemWin32::Win32_GethInstance()
{
	return m_hInstance;
}

void* CSystemWin32::Win32_GethAppIcon()
{
	return m_hIcon;
}
#endif

// #ifndef PLATFORM_XBOX
#if 0

   #include <tlhelp32.h>
   #include <vdmdbg.h>

typedef BOOL (CALLBACK *PROCENUMPROC)( DWORD, WORD, LPSTR,
  LPARAM ) ;

typedef struct
{
  DWORD          dwPID ;
  PROCENUMPROC   lpProc ;
  DWORD          lParam ;
  BOOL           bEnd ;
} EnumInfoStruct ;

static BOOL WINAPI Win32_EnumProcs( PROCENUMPROC lpProc, LPARAM lParam )
{
  OSVERSIONINFO  osver ;
  HINSTANCE      hInstLib ;
  HINSTANCE      hInstLib2 ;
  HANDLE         hSnapShot ;
  PROCESSENTRY32 procentry ;
  BOOL           bFlag ;
  LPDWORD        lpdwPIDs ;
  DWORD          dwSize, dwSize2, dwIndex ;
  HMODULE        hMod ;
  HANDLE         hProcess ;
  char           szFileName[ MAX_PATH ] ;
  EnumInfoStruct sInfo ;

  // ToolHelp Function Pointers.
  HANDLE (WINAPI *lpfCreateToolhelp32Snapshot)(DWORD,DWORD) ;
  BOOL (WINAPI *lpfProcess32First)(HANDLE,LPPROCESSENTRY32) ;
  BOOL (WINAPI *lpfProcess32Next)(HANDLE,LPPROCESSENTRY32) ;

  // PSAPI Function Pointers.
  BOOL (WINAPI *lpfEnumProcesses)( DWORD *, DWORD cb, DWORD * );
  BOOL (WINAPI *lpfEnumProcessModules)( HANDLE, HMODULE *,
     DWORD, LPDWORD );
  DWORD (WINAPI *lpfGetModuleFileNameEx)( HANDLE, HMODULE,
     LPTSTR, DWORD );

  // VDMDBG Function Pointers.
  INT (WINAPI *lpfVDMEnumTaskWOWEx)( DWORD,
     TASKENUMPROCEX  fp, LPARAM );


  // Check to see if were running under Windows95 or
  // Windows NT.
  osver.dwOSVersionInfoSize = sizeof( osver ) ;
  if( !GetVersionEx( &osver ) )
  {
     return FALSE ;
  }

  // If Windows NT:
  if( osver.dwPlatformId == VER_PLATFORM_WIN32_NT )
  {

     // Load library and get the procedures explicitly. We do
     // this so that we don't have to worry about modules using
     // this code failing to load under Windows 95, because
     // it can't resolve references to the PSAPI.DLL.
     hInstLib = LoadLibraryA( "PSAPI.DLL" ) ;
     if( hInstLib == NULL )
        return FALSE ;

     hInstLib2 = LoadLibraryA( "VDMDBG.DLL" ) ;
     if( hInstLib2 == NULL )
        return FALSE ;

     // Get procedure addresses.
     lpfEnumProcesses = (BOOL(WINAPI *)(DWORD *,DWORD,DWORD*))
        GetProcAddress( hInstLib, "EnumProcesses" ) ;
     lpfEnumProcessModules = (BOOL(WINAPI *)(HANDLE, HMODULE *,
        DWORD, LPDWORD)) GetProcAddress( hInstLib,
        "EnumProcessModules" ) ;
     lpfGetModuleFileNameEx =(DWORD (WINAPI *)(HANDLE, HMODULE,
        LPTSTR, DWORD )) GetProcAddress( hInstLib,
        "GetModuleFileNameExA" ) ;
     lpfVDMEnumTaskWOWEx =(INT(WINAPI *)( DWORD, TASKENUMPROCEX,
        LPARAM))GetProcAddress( hInstLib2, "VDMEnumTaskWOWEx" );
     if( lpfEnumProcesses == NULL ||
        lpfEnumProcessModules == NULL ||
        lpfGetModuleFileNameEx == NULL ||
        lpfVDMEnumTaskWOWEx == NULL)
        {
           FreeLibrary( hInstLib ) ;
           FreeLibrary( hInstLib2 ) ;
           return FALSE ;
        }

     // Call the PSAPI function EnumProcesses to get all of the
     // ProcID's currently in the system.
     // NOTE: In the documentation, the third parameter of
     // EnumProcesses is named cbNeeded, which implies that you
     // can call the function once to find out how much space to
     // allocate for a buffer and again to fill the buffer.
     // This is not the case. The cbNeeded parameter returns
     // the number of PIDs returned, so if your buffer size is
     // zero cbNeeded returns zero.
     // NOTE: The "HeapAlloc" loop here ensures that we
     // actually allocate a buffer large enough for all the
     // PIDs in the system.
     dwSize2 = 256 * sizeof( DWORD ) ;
     lpdwPIDs = NULL ;
     do
     {
        if( lpdwPIDs )
        {
           HeapFree( GetProcessHeap(), 0, lpdwPIDs ) ;
           dwSize2 *= 2 ;
        }
        lpdwPIDs = (unsigned long*)HeapAlloc( GetProcessHeap(), 0, dwSize2 );
        if( lpdwPIDs == NULL )
        {
           FreeLibrary( hInstLib ) ;
           FreeLibrary( hInstLib2 ) ;
           return FALSE ;
        }
        if( !lpfEnumProcesses( lpdwPIDs, dwSize2, &dwSize ) )
        {
           HeapFree( GetProcessHeap(), 0, lpdwPIDs ) ;
           FreeLibrary( hInstLib ) ;
           FreeLibrary( hInstLib2 ) ;
           return FALSE ;
        }
     }while( dwSize == dwSize2 ) ;

     // How many ProcID's did we get?
     dwSize /= sizeof( DWORD ) ;

     // Loop through each ProcID.
     for( dwIndex = 0 ; dwIndex < dwSize ; dwIndex++ )
     {
        szFileName[0] = 0 ;
        // Open the process (if we can... security does not
        // permit every process in the system).
        hProcess = OpenProcess(
           PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
           FALSE, lpdwPIDs[ dwIndex ] ) ;
        if( hProcess != NULL )
        {
           // Here we call EnumProcessModules to get only the
           // first module in the process this is important,
           // because this will be the .EXE module for which we
           // will retrieve the full path name in a second.
           if( lpfEnumProcessModules( hProcess, &hMod,
              sizeof( hMod ), &dwSize2 ) )
           {
              // Get Full pathname:
              if( !lpfGetModuleFileNameEx( hProcess, hMod,
                 szFileName, sizeof( szFileName ) ) )
              {
                 szFileName[0] = 0 ;
                }
           }
           CloseHandle( hProcess ) ;
        }
        // Regardless of OpenProcess success or failure, we
        // still call the enum func with the ProcID.
        if(!lpProc( lpdwPIDs[dwIndex], 0, szFileName, lParam))
           break ;

        // Did we just bump into an NTVDM?
        if( _stricmp( szFileName+(strlen(szFileName)-9),
           "NTVDM.EXE")==0)
        {
           // Fill in some info for the 16-bit enum proc.
           sInfo.dwPID = lpdwPIDs[dwIndex] ;
           sInfo.lpProc = lpProc ;
           sInfo.lParam = lParam ;
           sInfo.bEnd = FALSE ;
           // Enum the 16-bit stuff.
/*           lpfVDMEnumTaskWOWEx( lpdwPIDs[dwIndex],
              (TASKENUMPROCEX) Enum16,
              (LPARAM) &sInfo);*/

           // Did our main enum func say quit?
           if(sInfo.bEnd)
              break ;
        }
     }

     HeapFree( GetProcessHeap(), 0, lpdwPIDs ) ;
     FreeLibrary( hInstLib2 ) ;

  // If Windows 95:
  }else if( osver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
  {


     hInstLib = LoadLibraryA( "Kernel32.DLL" ) ;
     if( hInstLib == NULL )
        return FALSE ;

     // Get procedure addresses.
     // We are linking to these functions of Kernel32
     // explicitly, because otherwise a module using
     // this code would fail to load under Windows NT,
     // which does not have the Toolhelp32
     // functions in the Kernel 32.
     lpfCreateToolhelp32Snapshot=
        (HANDLE(WINAPI *)(DWORD,DWORD))
        GetProcAddress( hInstLib,
        "CreateToolhelp32Snapshot" ) ;
     lpfProcess32First=
        (BOOL(WINAPI *)(HANDLE,LPPROCESSENTRY32))
        GetProcAddress( hInstLib, "Process32First" ) ;
     lpfProcess32Next=
        (BOOL(WINAPI *)(HANDLE,LPPROCESSENTRY32))
        GetProcAddress( hInstLib, "Process32Next" ) ;
     if( lpfProcess32Next == NULL ||
        lpfProcess32First == NULL ||
        lpfCreateToolhelp32Snapshot == NULL )
     {
        FreeLibrary( hInstLib ) ;
        return FALSE ;
     }

     // Get a handle to a Toolhelp snapshot of the systems
     // processes.
     hSnapShot = lpfCreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 0 ) ;
     if( hSnapShot == INVALID_HANDLE_VALUE )
     {
        FreeLibrary( hInstLib ) ;
        return FALSE ;
     }

     // Get the first process' information.
     procentry.dwSize = sizeof(PROCESSENTRY32) ;
     bFlag = lpfProcess32First( hSnapShot, &procentry ) ;

     // While there are processes, keep looping.
     while( bFlag )
     {
        // Call the enum func with the filename and ProcID.
        if(lpProc( procentry.th32ProcessID, 0,
           procentry.szExeFile, lParam ))
        {
           procentry.dwSize = sizeof(PROCESSENTRY32) ;
           bFlag = lpfProcess32Next( hSnapShot, &procentry );
        }else
           bFlag = FALSE ;
     }


  }else
     return FALSE ;

  // Free the library.
  FreeLibrary( hInstLib ) ;

  return TRUE ;
}


typedef struct _PROCESS_MEMORY_COUNTERS {
    DWORD cb;
    DWORD PageFaultCount;
    DWORD PeakWorkingSetSize;
    DWORD WorkingSetSize;
    DWORD QuotaPeakPagedPoolUsage;
    DWORD QuotaPagedPoolUsage;
    DWORD QuotaPeakNonPagedPoolUsage;
    DWORD QuotaNonPagedPoolUsage;
    DWORD PagefileUsage;
    DWORD PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;

typedef PROCESS_MEMORY_COUNTERS *PPROCESS_MEMORY_COUNTERS;


typedef BOOL (WINAPI *pfnGetProcessMemoryInfo)(
  HANDLE Process,  // handle to the process
  PPROCESS_MEMORY_COUNTERS ppsmemCounters,
                   // structure that receives information
  DWORD cb         // size of the structure
);

CALLBACK Win32_GetProcessMemoryInfo(DWORD _dw, WORD _w, LPSTR _lpStr, LPARAM _lParam)
{
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, _dw);
	if (hProcess)
	{
		ConOutL(CStrF("Opened %.8x, %d, %s, %d", _dw, _w, _lpStr, _lParam));
		HMODULE hModule = GetModuleHandle(_lpStr);
		ConOutL(CStrF("hMod %.8x", hModule));

		if (hModule)
		{
			pfnGetProcessMemoryInfo pfnFunc = (pfnGetProcessMemoryInfo)GetProcAddress(hModule, "GetProcessMemoryInfo");
			ConOutL(CStrF("pfn %.8x", pfnFunc));
		}


		CloseHandle(hProcess);
	}
	else
		ConOutL(CStrF("Failed to open %.8x, %d, %s, %d", _dw, _w, _lpStr, _lParam));

	return true;
}

#endif

void CSystemWin32::Parser_SysShowMemory()
{
	ConOut("(CSystemWin32::Parser_SysShowMemory) Not implemented.");

/*	DWORD ProcessID = GetCurrentProcessId();

	Win32_EnumProcs( Win32_GetProcessMemoryInfo, 0);
*/	

/*	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessID);
	if (hProcess)
	{

		PROCESS_MEMORY_COUNTERS MemCount;
		FillChar(&MemCount, sizeof(MemCount), 0);

		GetProcessMemoryInfo(hProcess, &MemCount, sizeof(MemCount));

		ConOutL("-------------------------------------------------------------------");
		ConOutL("Process memory counters:");
		ConOutL("-------------------------------------------------------------------");
		ConOutL(CStrF("PageFaults               %d", MemCount.PageFaultCount));
		ConOutL(CStrF("WorkingSetSize           %d, peak %d", MemCount.WorkingSetSize, MemCount.PeakWorkingSetSize));
		ConOutL(CStrF("PageFileUsage            %d, peak %d", MemCount.PagefileUsage, MemCount.PeakPagefileUsage));
		ConOutL(CStrF("QuotaPagedPoolUsage      %d, peak %d", MemCount.QuotaPagedPoolUsage, MemCount.PeakPagefileUsage));
		ConOutL(CStrF("QuotaNonPagedPoolUsage   %d, peak %d", MemCount.QuotaNonPagedPoolUsage, MemCount.PeakQuotaNonPagedPoolUsage));
		ConOutL("-------------------------------------------------------------------");
		CloseHandle(hProcess);
	}
	else
		ConOutL("(CSystemWin32::Parser_SysShowMemory) Invalid process handle.");*/
}

void CSystemWin32::Parser_MemoryDebug(int _Flags)
{
#ifdef MRTC_MEMORYDEBUG
	int Flags = 0;
	if (_Flags & 1) Flags |= _CRTDBG_ALLOC_MEM_DF;
	if (_Flags & 2) Flags |= _CRTDBG_DELAY_FREE_MEM_DF;
	if (_Flags & 4) Flags |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(Flags);
#else
	ConOut("This executable is not compiled for memory-debuging.");
#endif
}

void CSystemWin32::Parser_TestMemory()
{
#ifdef MRTC_MEMORYDEBUG
	if (!_CrtCheckMemory())
		ConOutL("(CSystemWin32::Parser_TestMemory) Memory check failure.");
	else
		ConOutL("(CSystemWin32::Parser_TestMemory) Memory check ok.");
#else
	ConOut("This executable is not compiled for memory-debuging.");
#endif
}

void CSystemWin32::ExitProcess()
{
	m_bBreakRequested = true;

}

CContentContext* CSystemWin32::GetContentContext(int _iContext)
{
	switch(_iContext)
	{
	case CONTENT_SAVEGAMES:
		return &m_SaveGameContext;
	}

	return NULL;
}

int CSaveContentContext_Win32::EnumerateContainers(uint _iUser)
{
	return AllocRequest(REQ_TYPE_ENUM_CONTAINERS);
}

int CSaveContentContext_Win32::ContainerCreate(uint _iUser, CStr _Container)
{
	return AllocRequest(REQ_TYPE_CONTAINER_CREATE, _Container);
}

int CSaveContentContext_Win32::ContainerDelete(uint _iUser, CStr _Container)
{
	return AllocRequest(REQ_TYPE_CONTAINER_DELETE, _Container);
}

int CSaveContentContext_Win32::ContainerQuerySize(uint _iUser, CStr _Container)
{
	return AllocRequest(REQ_TYPE_CONTAINER_QUERY_SIZE, _Container);
}

int CSaveContentContext_Win32::ContainerMount(uint _iUser, CStr _Container)
{
	return AllocRequest(REQ_TYPE_CONTAINER_MOUNT, _Container);
}

int CSaveContentContext_Win32::ContainerQueryPath(uint _iUser, CStr _Container)
{
	return AllocRequest(REQ_TYPE_CONTAINER_QUERY_PATH, _Container);
}

int CSaveContentContext_Win32::EnumerateInstances(uint _iUser, CStr _Container)
{
	return AllocRequest(REQ_TYPE_ENUM_INSTANCES, _Container);
}

int CSaveContentContext_Win32::InstanceRead(uint _iUser, CStr _Container, CStr _Instance, void* _pDest, uint32 _MaxLen)
{
	return AllocRequest(REQ_TYPE_INSTANCE_READ, _Container, _Instance, _pDest, _MaxLen);
}

int CSaveContentContext_Win32::InstanceWrite(uint _iUser, CStr _Container, CStr _Instance, void* _pSrc, uint32 _Len)
{
	return AllocRequest(REQ_TYPE_INSTANCE_WRITE, _Container, _Instance, _pSrc, _Len);
}

int CSaveContentContext_Win32::InstanceQuerySize(uint _iUser, CStr _Container, CStr _Instance)
{
	return AllocRequest(REQ_TYPE_INSTANCE_QUERY_SIZE, _Container, _Instance);
}

int CSaveContentContext_Win32::InstanceDelete(uint _iUser, CStr _Container, CStr _Instance)
{
	return AllocRequest(REQ_TYPE_INSTANCE_DELETE, _Container, _Instance);
}


int CSaveContentContext_Win32::PeekRequestData(int _iRequest, CContentQueryData* _pRet)
{
	if(_iRequest < 0 || _iRequest >= m_lRequests.Len())
		return CONTENT_ERR_INVALID_HANDLE;

	M_LOCK(m_RequestLock);
	CRequest* pReq = &m_lRequests[_iRequest];
	if(pReq->m_Type == REQ_TYPE_VOID)
		return CONTENT_ERR_INVALID_HANDLE;

	if(_pRet)
		_pRet->m_Data = pReq->m_ResultSize;

	return pReq->m_ResultError;
}

int CSaveContentContext_Win32::GetRequestData(int _iRequest, CContentQueryData* _pRet)
{
	if(_iRequest < 0 || _iRequest >= m_lRequests.Len())
		return CONTENT_ERR_INVALID_HANDLE;

	M_LOCK(m_RequestLock);
	CRequest* pReq = &m_lRequests[_iRequest];
	if(pReq->m_Type == REQ_TYPE_VOID)
		return CONTENT_ERR_INVALID_HANDLE;

	int ResultError = pReq->m_ResultError;
	if(_pRet)
	{
		_pRet->m_Data = pReq->m_ResultSize;
	}

	FreeRequest(_iRequest);

	return ResultError;
}

int CSaveContentContext_Win32::GetRequestEnumeration(int _iRequest, TArray<CStr>& _lData)
{
	if(_iRequest < 0 || _iRequest >= m_lRequests.Len())
		return CONTENT_ERR_INVALID_HANDLE;

	M_LOCK(m_RequestLock);
	CRequest* pReq = &m_lRequests[_iRequest];
	if(pReq->m_Type == REQ_TYPE_VOID)
		return CONTENT_ERR_INVALID_HANDLE;

	if(pReq->m_Type != REQ_TYPE_ENUM_CONTAINERS && pReq->m_Type != REQ_TYPE_ENUM_INSTANCES)
		return CONTENT_ERR_UNSUPPORTED;

	if(pReq->m_ResultError != CONTENT_OK)
		return pReq->m_ResultError;

	_lData = pReq->m_lResultEnum;

	FreeRequest(_iRequest);

	return CONTENT_OK;
}

int CSaveContentContext_Win32::GetRequestStatus(int _iRequest)
{
	if(_iRequest < 0 || _iRequest >= m_lRequests.Len())
		return CONTENT_ERR_INVALID_HANDLE;

	M_LOCK(m_RequestLock);
	CRequest* pReq = &m_lRequests[_iRequest];
	if(pReq->m_Type == REQ_TYPE_VOID)
		return CONTENT_ERR_INVALID_HANDLE;

	return pReq->m_ResultError;
}

int CSaveContentContext_Win32::BlockOnRequest(int _iRequest)
{
	if(_iRequest < 0 || _iRequest >= m_lRequests.Len())
		return CONTENT_ERR_INVALID_HANDLE;

	do
	{
		CRequest* pReq = NULL;
		{
			M_LOCK(m_RequestLock);
			pReq = &m_lRequests[_iRequest];
			if(pReq->m_Type == REQ_TYPE_VOID)
				return CONTENT_ERR_INVALID_HANDLE;

			if(pReq->m_ResultError != CONTENT_ERR_PENDING)
				return pReq->m_ResultError;
		}

		// If we got here then the handle is proper, wait for it to be done
		pReq->m_Done.Wait();
	} while(1);

	return CONTENT_ERR_UNKNOWN;
}

int CSaveContentContext_Win32::Thread_Main()
{
	m_QuitEvent.ReportTo(&m_RequestAvail);
	while(!Thread_IsTerminating())
	{
		m_RequestAvail.Wait();
		if(Thread_IsTerminating())
			break;

		do 
		{
			// Find a request to service
			CRequest* pReq = NULL;
			{
				M_LOCK(m_RequestLock);
				if(m_pRequestHead == NULL)
					break;

				pReq = m_pRequestHead;
				m_pRequestHead = pReq->m_pNext;
				if(m_pRequestHead == NULL)
					m_pRequestTail = NULL;
				pReq->m_pNext = NULL;
			}

			switch(pReq->m_Type)
			{
			default:
				pReq->m_ResultError = CONTENT_ERR_UNSUPPORTED;
				break;

			case REQ_TYPE_ENUM_CONTAINERS:
				{
					CDirectoryNode Dir;
					Dir.ReadDirectory(m_SavePath + pReq->m_Container + "\\*");
					int nf = Dir.GetFileCount();
					for(int i = 0; i < nf; i++)
					{
						CDir_FileRec* pF = Dir.GetFileRec(i);
						if(Dir.IsDirectory(i))
						{
							if(pF->m_Name != "." && pF->m_Name != "..")
							{
								pReq->m_lResultEnum.Add(pF->m_Name);
							}
						}
					}

					pReq->m_ResultSize = pReq->m_lResultEnum.Len();
					pReq->m_ResultError = CONTENT_OK;
					break;
				}

			case REQ_TYPE_CONTAINER_DELETE:
				{
					CStr Path = m_SavePath + pReq->m_Container;
					if(CDiskUtil::DelTree(Path))
						pReq->m_ResultError = CONTENT_OK;
					else
						pReq->m_ResultError = CONTENT_ERR_GENERIC_WRITE;
					break;
				}
			case REQ_TYPE_CONTAINER_CREATE:
				{
					CStr Path = m_SavePath + pReq->m_Container;
					if(CDiskUtil::CreatePath(Path))
						pReq->m_ResultError = CONTENT_OK;
					else
						pReq->m_ResultError = CONTENT_ERR_GENERIC_WRITE;
					break;
				}

			case REQ_TYPE_ENUM_INSTANCES:
				{
					CDirectoryNode Dir;
					Dir.ReadDirectory(m_SavePath + pReq->m_Container + "\\*");
					int nf = Dir.GetFileCount();
					for(int i = 0; i < nf; i++)
					{
						CDir_FileRec* pF = Dir.GetFileRec(i);
						if(!Dir.IsDirectory(i))
						{
							pReq->m_lResultEnum.Add(pF->m_Name);
						}
					}

					pReq->m_ResultSize = pReq->m_lResultEnum.Len();
					pReq->m_ResultError = CONTENT_OK;
					break;
				}

			case REQ_TYPE_INSTANCE_DELETE:
				{
					CStr FileName = m_SavePath + pReq->m_Container + "\\" + pReq->m_Instance;
					if(CDiskUtil::DelFile(FileName))
						pReq->m_ResultError = CONTENT_OK;
					else
						pReq->m_ResultError = CONTENT_ERR_GENERIC_WRITE;
					break;
				}

			case REQ_TYPE_INSTANCE_QUERY_SIZE:
				{
					CStr Path = m_SavePath + pReq->m_Container;
					if(CDiskUtil::DirectoryExists(Path))
					{
						CStr DataName = Path + "\\" + pReq->m_Instance;

						if(CDiskUtil::FileExists(DataName))
						{
							pReq->m_ResultSize = CDiskUtil::GetFileSize(DataName);
							pReq->m_ResultError = CONTENT_OK;
						}
						else
							pReq->m_ResultError = CONTENT_ERR_INSTANCE_NOT_FOUND;
					}
					else
						pReq->m_ResultError = CONTENT_ERR_CONTAINER_NOT_FOUND;
					break;
				}

			case REQ_TYPE_INSTANCE_READ:
				{
					CStr Path = m_SavePath + pReq->m_Container;
					if(CDiskUtil::DirectoryExists(Path))
					{
						CStr DataName = Path + "\\" + pReq->m_Instance;

						if(CDiskUtil::FileExists(DataName))
						{
							uint32 ReadData = 0;
							try
							{
								CCFile File;
								File.Open(DataName, CFILE_READ | CFILE_BINARY);
								ReadData = Min(pReq->m_LenDestArg, (uint32)File.Length());
								File.Read(pReq->m_pDestArg, ReadData);
								File.Close();

								pReq->m_ResultSize = ReadData;
								pReq->m_ResultError = CONTENT_OK;
							}
							catch (CCException)
							{
								pReq->m_ResultError = CONTENT_ERR_GENERIC_READ;
							}
						}
						else
							pReq->m_ResultError = CONTENT_ERR_INSTANCE_NOT_FOUND;
					}
					else
						pReq->m_ResultError = CONTENT_ERR_CONTAINER_NOT_FOUND;
					break;
				}
			case REQ_TYPE_INSTANCE_WRITE:
				{
					CStr Path = m_SavePath + pReq->m_Container;
					if(CDiskUtil::DirectoryExists(Path))
					{
						CStr DataName = Path + "\\" + pReq->m_Instance;

						try
						{
							CCFile File;
							File.Open(DataName, CFILE_WRITE | CFILE_BINARY | CFILE_TRUNC);
							File.Write(pReq->m_pDestArg, pReq->m_LenDestArg);
							File.Close();

							pReq->m_ResultSize = pReq->m_LenDestArg;
							pReq->m_ResultError = CONTENT_OK;
						}
						catch (CCException)
						{
							pReq->m_ResultError = CONTENT_ERR_GENERIC_WRITE;
						}
					}
					else
						pReq->m_ResultError = CONTENT_ERR_CONTAINER_NOT_FOUND;
					break;
				}
			}

			// Make sure data is written before we signal that it is available
			M_EXPORTBARRIER;
			pReq->m_Done.Signal();

		} while(1);

	}

	return 0;
}

void CSaveContentContext_Win32::FreeRequest(int _iRequest)
{
	M_LOCK(m_RequestLock);
	m_lRequests[_iRequest].m_Type = REQ_TYPE_VOID;
	m_lRequests[_iRequest].Reset();
}

int CSaveContentContext_Win32::AllocRequestInstance()
{
	M_LOCK(m_RequestLock);
	TAP_RCD<CRequest> pRequests(m_lRequests);
	for(int i = 0; i < pRequests.Len(); i++)
	{
		if(pRequests[i].m_Type == REQ_TYPE_VOID)
		{
			pRequests[i].Reset();
			return i;
		}
	}

	return -1;
}

void CSaveContentContext_Win32::QueueRequest(CRequest* _pReq)
{
	{
		M_LOCK(m_RequestLock);
		if(m_pRequestHead)
			m_pRequestTail->m_pNext = _pReq;
		else
			m_pRequestHead = _pReq;
		m_pRequestTail = _pReq;
	}
	m_RequestAvail.Signal();
}

int CSaveContentContext_Win32::AllocRequest(int _Type)
{
	int Request = AllocRequestInstance();
	if(Request >= 0)
	{
		CRequest* pReq = &m_lRequests[Request];
		pReq->m_Type = _Type;
		QueueRequest(pReq);
	}

	return Request;
}

int CSaveContentContext_Win32::AllocRequest(int _Type, CStr _Container)
{
	int Request = AllocRequestInstance();
	if(Request >= 0)
	{
		CRequest* pReq = &m_lRequests[Request];
		pReq->m_Type = _Type;
		pReq->m_Container = _Container;
		QueueRequest(pReq);
	}

	return Request;
}

int CSaveContentContext_Win32::AllocRequest(int _Type, CStr _Container, CStr _Instance)
{
	int Request = AllocRequestInstance();
	if(Request >= 0)
	{
		CRequest* pReq = &m_lRequests[Request];
		pReq->m_Type = _Type;
		pReq->m_Container = _Container;
		pReq->m_Instance = _Instance;
		QueueRequest(pReq);
	}

	return Request;
}

int CSaveContentContext_Win32::AllocRequest(int _Type, CStr _Container, CStr _Instance, void* _pDest, uint _DestLen)
{
	int Request = AllocRequestInstance();
	if(Request >= 0)
	{
		CRequest* pReq = &m_lRequests[Request];
		pReq->m_Type = _Type;
		pReq->m_Container = _Container;
		pReq->m_Instance = _Instance;
		pReq->m_pDestArg = _pDest;
		pReq->m_LenDestArg = _DestLen;
		QueueRequest(pReq);
	}

	return Request;
}

#endif
#endif
