#ifndef _INC_MINPUT_INPS3
#define _INC_MINPUT_INPS3

/*
// +-----------------------------------------------------------------+
// | CInputContext_PS3					                             |
// +-----------------------------------------------------------------+
// | Creator:          Magnus Högdahl                                |
// | Created:          2001-02-05                                    |
// | Last modified:    2001-__-__                                    |
// |                                                                 |
// | Description:                                                    |
// |                                                                 |
// |                                                                 |
// +-----------------------------------------------------------------+
// | Copyright Starbreeze Studios AB 2001							 |
// +-----------------------------------------------------------------+
*/

#include "../MInputCore.h"

#ifdef PLATFORM_PS3

#include <types.h>
#include <stdio.h>
#include <sys/timer.h>
#if defined(__LP32__)
#include <cell/pad/libpad.h>      /* USB Gamepad Library */
#include <cell/keyboard/libkb.h>   /* USB Keyboard Library */
#else
#include <cell/pad.h>      /* USB Gamepad Library */
#include <cell/keyboard.h>   /* USB Keyboard Library */

// Ugly hack to be able to compile with old SDK
#define	CellPadInfo	CellUsbPadInfo
#define	CellKbInfo CellUsbKbInfo

#endif

// -------------------------------------------------------------------
enum
{
	INPS3_MAXAXES = 8,								// 8 Axes uses 4 scancodes
	INPS3_MAXBUTTONS = 12,							// 12 buttons uses 12 scancodes => 16 scancodes total.

	INPS3_MAXJOYSTICKS = 4,						// We enumerate at most up to 4 joysticks. (There are no scancodes allocated for more)
	INPS3_MAXKEYBOARDS = 1,						

	INPS3_JOYSTICKSTARTDEV = 0,
	INPS3_KEYBOARDSTARTDEV = INPS3_MAXJOYSTICKS,

	INPS3_JOY_BUTTONSCAN = SKEY_JOY_BUTTON00,		// Button scancode base
	INPS3_JOY_AXISSCAN = SKEY_JOY_START,		// Axes scancode base
	INPS3_JOY_POVSCAN = SKEY_JOY_POV00,		// Axes scancode base


//	INPS3_JOY_SCANOFFSET = (SKEY_JOY1_AXIS00 - SKEY_JOY0_AXIS00),	// Scancode offset for multiple joystick devices
};

const fp4 INPS3_JOY_AXISDEADZONE = 0.25f;			// 25% deadzone
const fp4 INPS3_JOY_AXISPOWER = 3.0f;				// axis = axis ^ power  (axis = [0..1])
const fp4 INPS3_JOY_AXISMAXVALUE = 255.0f;			// scancode axis value is scaled to [0..255]

// -------------------------------------------------------------------
class CPS3_JoystickData
{
public:
	uint8 m_lButtons[8];
};

class CInputContext_PS3;
// -------------------------------------------------------------------
//  CPS3_Device
// -------------------------------------------------------------------
class CPS3_Device : public CReferenceCount
{
	MRTC_DECLARE;
public:
	CInputContext_PS3* m_pInput;
	int m_PortNumber;

	CPS3_Device();
	~CPS3_Device();
	virtual void Destroy();
	virtual bool Create(CInputContext_PS3* _pInput, int _PortNumber, int _iDevice = 0);

	virtual void OnRefresh();

	virtual void OnLogCaps();

	virtual void OnFeedback(const float _fForce1, const float _fForce2);
	virtual int OnGetPadNumber() { return m_PortNumber; };
};

typedef TPtr<CPS3_Device> spCPS3_Device;

// -------------------------------------------------------------------
//  CPS3_Device_Joystick
// -------------------------------------------------------------------


class CPS3_Device_Joystick : public CPS3_Device
{
	MRTC_DECLARE;

protected:
	int m_JoystickNr;
	bint m_bActive;

	CPS3_JoystickData m_State;

public:
	CPS3_Device_Joystick();
	void Destroy();
	bool Create(CInputContext_PS3* _pInput, int _PortNumber, int _iDevice = 0);

	int OnTranslateAxis(int _iAxis, int _Data);
	void OnButtonData(int _iButton, int _Data);
	void OnAxisData(int _iAxis, int _Data);
	void OnPOVData(int _iPOV, int _Data, int _OldData);
	void OnClearAxes();
	void OnRefresh();
};


// -------------------------------------------------------------------
//  CPS3_Device_DebugKeyboard
// -------------------------------------------------------------------
class CPS3_Device_KeyBoard : public CPS3_Device
{
	MRTC_DECLARE;

	int m_KBDNr;
	int m_LastModifier;

public:
	CPS3_Device_KeyBoard();	
	virtual bool Create(CInputContext_PS3* _pInput, int _PortNumber, int _iDevice = 0);
	virtual void OnRefresh();
};



// -------------------------------------------------------------------
//  CInputContext_PS3
// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CInputContext_PS3 : public CInputContextCore
{
	MRTC_DECLARE;

protected:
	TArray<spCPS3_Device> m_lspDevices;

	CellPadInfo m_PadInfo;
	CellKbInfo m_KBInfo;
	int8	m_KBUSB2Dev[128];
	uint8	m_Dev2USB[128];


	// -------------------------------------------------------------------
	// PS3 routines
public:
	static CStr PS3_MessageStr(int err);

protected:
	bool PS3_CreateDevice(const char* _pClassName, int _PortNumber, int _iDevice);
	void PS3_CreateDevices();
	void PS3_DestroyDevices();

	// -------------------------------------------------------------------
public:


	DECLARE_OPERATOR_NEW


	CInputContext_PS3();
	~CInputContext_PS3();

	// -------------------------------------------------------------------
	// Overrides from CInputContext
	virtual void Create(const char* _pParams);
	virtual void Update();
	virtual bool IsGamepadValid(int _iPad);

	// -------------------------------------------------------------------
	// Overrides from CSubSystem
	virtual aint OnMessage(const CSS_Msg& _Msg);
	virtual void OnRefresh(int _Context);
	virtual void OnBusy(int _Context);
};

typedef TPtr<CInputContext_PS3> spCInputContext_PS3;

#endif	// PLATFORM_XBOX

#endif // _INC_MINPUT_DI
