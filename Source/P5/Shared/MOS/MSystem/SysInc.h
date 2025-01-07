
#ifndef _INC_SYSINC
#define _INC_SYSINC

// -------------------------------------------------------------------
#ifdef SYSTEMDLL

	#ifdef SYSTEMDLLINTERFACE	
		#define SYSTEMDLLEXPORT __declspec(dllexport)
	#else
		#define SYSTEMDLLEXPORT __declspec(dllexport)
//		#define SYSTEMDLLEXPORT __declspec(dllimport)
	#endif
	
	#define MCCDLL
	#pragma warning(disable : 4251)

#else

	#define SYSTEMDLLEXPORT

#endif

// -------------------------------------------------------------------
#include "MCC.h"

// -------------------------------------------------------------------
//  CSubSystem
// -------------------------------------------------------------------
class CSS_Msg
{
public:
	int m_Msg;
	aint m_Params[4];
	fp32 m_fParams[4];
	void* m_pData;
	aint m_DataSize;

	CSS_Msg();
	CSS_Msg(int32 _Msg, 
		aint _p0 = 0, aint _p1 = 0, aint _p2 = 0, aint _p3 = 0, 
		fp32 _fp0 = 0.0f, fp32 _fp1 = 0.0f, fp32 _fp2 = 0.0f, fp32 _fp3 = 0.0f,
		void* _pData = NULL, int _DataSize = 0)
	{
		m_Msg = _Msg;
		m_Params[0] = _p0;
		m_Params[1] = _p1;
		m_Params[2] = _p2;
		m_Params[3] = _p3;
		m_fParams[0] = _fp0;
		m_fParams[1] = _fp1;
		m_fParams[2] = _fp2;
		m_fParams[3] = _fp3;
		m_pData = _pData;
		m_DataSize = _DataSize;
	};
};

// -------------------------------------------------------------------
enum
{
	CSS_MSG_VOID = 0,
	CSS_MSG_HIBERNATE = 1,							// The sub-system should try to free-up memory
	CSS_MSG_PRECHANGEDISPLAYMODE = 2,				// Notification that the display-mode will be changed by the app.
	CSS_MSG_POSTCHANGEDISPLAYMODE = 3,				// Notification that the display-mode has been changed by the app.
	CSS_MSG_PRECACHEHINT = 4,						// General precache hint
	CSS_MSG_PRECACHEHINT_TEXTURES = 5,				// Textures may need reloading..
	CSS_MSG_PRECACHEHINT_SOUNDS = 6,				// Sound may need reloading..
	CSS_MSG_INPUTERROR = 7,							// Inputsystem re

	CSS_MSG_GLOBAL_PAUSE = 8,						// Used by low level error handler
	CSS_MSG_GLOBAL_RESUME = 9,						// Used by low level error handler

#ifdef PLATFORM_WIN_PC
	CSS_MSG_WIN32_PREDESTROYWINDOW = 0x0100,		// _Params[0] == hWnd, used to notify sub-systems dependent on hWnd that a window is about be nuked.
	CSS_MSG_WIN32_POSTDESTROYWINDOW = 0x0101,		// _Params[0] == hWnd, and when this arrives, it's too late to do anything about it.
	CSS_MSG_WIN32_CREATEDWINDOW = 0x0102,			// _Params[0] == hWnd, used for display-system to notify CSystem
	CSS_MSG_WIN32_NEWMAINWINDOW = 0x0103,			// _Params[0] == hWnd, used by CSystem to notify sub-systems needing a hWnd for their operation.
#endif
};

// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CSubSystem
{
public:
	// All base methods are NOPs..
	CSubSystem();
	virtual ~CSubSystem();
	virtual aint OnMessage(const CSS_Msg& _Msg) { return 0; };
	virtual void OnRefresh(int _Context) {};
	virtual void OnBusy(int _Context) {};
};

#define MACRO_GetSystem MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM"); if (!pSys) Error_static("?", "No system.")

#define MACRO_AddSubSystem(pSubSystem) MACRO_GetSystem; pSys->System_Add(pSubSystem)
#define MACRO_RemoveSubSystem(pSubSystem) MACRO_GetSystem; pSys->System_Remove(pSubSystem)

// -------------------------------------------------------------------

#endif // _INC_SYSINC
