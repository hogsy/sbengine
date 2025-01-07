#ifndef _INC_MSYSTEM
#define _INC_MSYSTEM

#if defined(PLATFORM_XBOX1) && defined(M_DEMO_XBOX)

	#include <xtl.h>


typedef LD_DEMO DEMO_LAUNCH_DATA;

class CXboxDemoParams
{
public:
	DEMO_LAUNCH_DATA m_LaunchData;
	bool m_bLaunchedFromDemoLauncher;
	bool m_KioskMode;
};

extern CXboxDemoParams g_XboxDemoParams;

#endif

#include "Raster/MTexture.h"
#include "Raster/MDisplay.h"
#include "Raster/MRender.h"
#include "MNetwork.h"
#include "Input/MInput.h"
#include "Misc/MRegistry.h"
#include "Script/MScript.h"
#include "Misc/MConsole.h"
#include "Misc/MLocalizer.h"

// -------------------------------------------------------------------
//  CWin32AppInfo
// -------------------------------------------------------------------
#ifdef PLATFORM_WIN_PC

class CWin32AppInfo : public CReferenceCount
{
public:

	DECLARE_OPERATOR_NEW

	CWin32AppInfo();

	void* m_hMainWnd;
	void* m_hInstance;
	void* m_hPrevInstance;
	void* m_hIcon;

	int m_CmdShow;
	char* m_pCmdLine;
};

typedef TPtr<CWin32AppInfo> spCWin32AppInfo;

#endif

// -------------------------------------------------------------------
//  CApplication
// -------------------------------------------------------------------
class CSystem;

class SYSTEMDLLEXPORT CApplication : public CConsoleClient, public CSubSystem
{
protected:
	MRTC_ObjectManager* m_pObjMgr;
	CSystem* m_pSystem;

public:
	CApplication();
	virtual ~CApplication();
	virtual void Create();
	virtual void OnRender(CDisplayContext* _pDisplay, CImage* _pFrame, CClipRect _Clip) pure;
	virtual void OnRender(CDisplayContext* _pDisplay, CImage* _pFrame, CClipRect _Clip, int _Context);
	virtual void DoModal() pure;
};

typedef TPtr<CApplication> spCApplication;

// -------------------------------------------------------------------
//  CSystem
// -------------------------------------------------------------------
class CDisplayContextDesc : public CReferenceCount
{
public:
	int LoadedDll;
	CStr m_FileName;
	CStr m_Name;
	CStr m_Desc;
	CStr m_DCClass;

	CDisplayContextDesc(CStr _FileName, CStr _Name, CStr _Desc, CStr _DCClass)
	{
		LoadedDll = 0;
		m_FileName = _FileName;
		m_Name = _Name;
		m_Desc = _Desc;
		m_DCClass = _DCClass;
	}
};
typedef TPtr<CDisplayContextDesc> spCDisplayContextDesc;

// -------------------------------------------------------------------
class CDisplayContextContainer : public CReferenceCount
{
public:
	bool DllLoaded;
	spCDisplayContext m_spDC;
	spCDisplayContextDesc m_spDCDesc;

	CDisplayContextContainer(spCDisplayContextDesc _spDesc);
	~CDisplayContextContainer();
};

typedef TPtr<CDisplayContextContainer> spCDisplayContextContainer;

// -------------------------------------------------------------------
class CSystem : public CConsoleClient, public CSubSystem
{
public:
	bool m_bBreakRequested;

	CStr m_CommandLine;
	CStr m_ExePath;
	CStr m_SystemPath;

	// Some useful objects.
	spCConsole m_spCon;
	spCInputContext m_spInput;
	spCRegistry m_spRegistry;

	int m_iMainWindow;
	spCDisplayContext m_spDisplay;
	spCTextureContext m_spTC;

	// -------------------------------
	virtual void Create(void* this_inst, void* prev_inst, char* cmdline, int _cmdshow, const char* _pAppClassName) pure;
	virtual void Destroy() pure;

	virtual CStr GetModuleName(CStr _Name, bool _bAppendSystem = false) pure;

	virtual void CreateInput() pure;
	virtual void CreateTextureContext(int _TextureIDHeap, int _TextureCacheHeap) pure;
	
	virtual void System_Add(CSubSystem* _pSubSys) pure;
	virtual void System_Remove(CSubSystem* _pSubSys) pure;
	virtual void System_BroadcastMessage(const CSS_Msg& _Msg) pure;
	virtual int System_SendMessage(const char* _pSysName, const CSS_Msg& _Msg) pure;	// (not implemented)

	// CSubSystem overrides:
	virtual aint OnMessage(const CSS_Msg& _Msg) pure;

	virtual void DoModal() pure;

	virtual void Parser_Quit() pure;

	virtual bool IsRendering() pure;
	virtual void Render(CDisplayContext* _pDC = NULL, int _Context = 0) pure;
	virtual void Refresh() pure;
	virtual void InitCPUFrequencyConstant() pure;

	virtual CApplication* GetApp() pure;
	virtual void DestroyApp() pure;

	virtual CRegistry* GetRegistry() pure;
	virtual CRegistry* GetEnvironment() pure;
	virtual CRegistry* GetOptions(bool _create = false) pure;
	virtual int ReadOptions() pure;			// 0 == Corrupt, 1 == Ok, 2 == NoFile
	virtual int WriteOptions() pure;		// 0 == Failed, 1 == Ok, 2 == Corrupt

	virtual spCDisplayContextContainer CreateDisplayContext(const char* _pName) pure;

	virtual void DC_Set(int _nr) pure;
	virtual void DC_SetName(const char *_pClassName) pure;
	virtual void DC_SetPtr(spCReferenceCount _spDisplayContext) pure;
	virtual void DC_InitFromEnvironment() pure;
	virtual void DC_InitVidModeFromEnvironment() pure;

	virtual void WriteEnvironment() pure;

	virtual CStr TranslateSavePath(CStr _Path) pure;

	virtual CStr GetCaption() pure;
	virtual void SetCaption(CStr _Caption) pure;

#ifdef PLATFORM_WIN_PC
	virtual void Win32_CreateInfo() pure;
	virtual void* Win32_GethWnd() pure;
	virtual void* Win32_GethInstance() pure;
	virtual void* Win32_GethAppIcon() pure;
#endif
};

void SYSTEMDLLEXPORT ConsoleWrite(CStr _s);
void SYSTEMDLLEXPORT ConsoleWriteL(CStr _s);
bool SYSTEMDLLEXPORT ConExecuteImp(CStr _Script, const char *_File, int _Line);
bool SYSTEMDLLEXPORT ConExecuteMTImp(CStr _Script, const char *_File, int _Line);

#ifndef M_RTM
#define ConExecute(_ToExecute) ConExecuteImp(_ToExecute, __FILE__, __LINE__-1)
#define ConExecuteMT(_ToExecute) ConExecuteMTImp(_ToExecute, __FILE__, __LINE__-1)
#else
#define ConExecute(_ToExecute) ConExecuteImp(_ToExecute, "ConExecute", 0)
#define ConExecuteMT(_ToExecute) ConExecuteMTImp(_ToExecute, "ConExecute", 0)
#endif

#if defined(M_RTM) && !defined(MRTC_ENABLE_REMOTEDEBUGGER) && !defined(PLATFORM_WIN_PC)
	#define ConOut(s) ((void)(0))
	#define ConOutL(s) ((void)(0))
	#define ConOutLD(s) ((void)(0))
	#define ConOutD(s) ((void)(0))
#else
#define ConOut(s) ConsoleWrite(s)
#define ConOutL(s) ConsoleWriteL(s)
	#if defined(M_RTM)
		#define ConOutLD(s) ((void)(0))
		#define ConOutD(s) ((void)(0))
	#else
		#define ConOutLD(s) ConsoleWriteL(s)
		#define ConOutD(s) ConsoleWrite(s)
	#endif
#endif



#define MACRO_GetSystemRegistry(Name)                       \
CRegistry* Name = NULL;                                     \
{                                                           \
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");      \
	if (pSys)                                               \
		Name = pSys->GetRegistry();                         \
}

#define MACRO_GetSystemEnvironment(Name)                    \
CRegistry* Name = NULL;                                     \
{                                                           \
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");      \
	if (pSys)                                               \
		Name = pSys->GetEnvironment();                      \
}


#endif // _INC_MSYSTEM

