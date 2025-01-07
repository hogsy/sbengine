#include "PCH.h"

#ifdef PLATFORM_PS3

#include <time.h> 

#include "../../MSystem.h"
#include "MFloat.h"


#include "MInput_PS3.h"

#ifdef __LP32__
	#include <cell/keyboard/error.h>
	#include <cell/pad/error.h>
#else
/*
#define	CellPadData	CellUsbPadData

#define	cellPadInit cellUsbPadInit
#define	cellPadEnd cellUsbPadEnd
#define	cellPadGetData cellUsbPadRead
#define cellPadGetInfo cellUsbPadGetInfo

#define	CellKbData CellUsbKbData

#define	cellKbInit cellUsbKbInit
#define	cellKbEnd cellUsbKbEnd
#define cellKbRead cellUsbKbRead
#define cellKbGetInfo cellUsbKbGetInfo
#define cellKbSetRepeat cellUsbKbSetRepeat
#define cellKbSetArrangement cellUsbKbSetArrangement
#define cellKbSetReadMode cellUsbKbSetReadMode
#define cellKbSetCodeType cellUsbKbSetCodeType

#define	CELL_KB_OK CELL_USBKB_OK
#define	CELL_KB_ARRANGEMENT_101 CELL_USBKB_ARRANGEMENT_101
#define CELL_KB_RMODE_INPUTCHAR CELL_USBKB_RMODE_INPUTCHAR
#define CELL_KB_CODETYPE_ASCII CELL_USBKB_CODETYPE_ASCII

#define	CELL_PAD_OK CELL_USBPAD_OK
*/
#endif

// -------------------------------------------------------------------
//  Exception macros
// -------------------------------------------------------------------
#define PS3ErrMsg(err) CInputContext_PS3::PS3_MessageStr(err)

// -------------------------------------------------------------------
//  Implement MRTC
// -------------------------------------------------------------------
MRTC_IMPLEMENT(CPS3_Device, CObj);
MRTC_IMPLEMENT_DYNAMIC(CPS3_Device_Joystick, CPS3_Device);
MRTC_IMPLEMENT_DYNAMIC(CPS3_Device_KeyBoard, CPS3_Device);
MRTC_IMPLEMENT_DYNAMIC(CInputContext_PS3, CInputContext);


IMPLEMENT_OPERATOR_NEW(CInputContext_PS3);


void InputContextPS3_InitLibs()
{
	if (cellKbInit (INPS3_MAXKEYBOARDS) != CELL_KB_OK) 
	{
        Error("Create", "Failed to initialize USB Keyboard Library");
    }
	if (cellPadInit (INPS3_MAXJOYSTICKS) != 0 ) 
	{
        Error("Create", "Failed to initialize USB Pad Library");
    }
}

void InputContextPS3_ShutdownLibs()
{
	cellKbEnd();
	cellPadEnd();
}

// -------------------------------------------------------------------
//  CPS3_Device
// -------------------------------------------------------------------
CPS3_Device::CPS3_Device()
{
	m_PortNumber = -1;
	m_pInput = NULL;
}

CPS3_Device::~CPS3_Device()
{
	Destroy();
}

void CPS3_Device::Destroy()
{
	m_PortNumber = 0;
	m_pInput = NULL;
}

bool CPS3_Device::Create(CInputContext_PS3* _pInput, int _PortNumber, int _iDevice)
{
	// Nuke
	Destroy();

	// Create
	m_PortNumber = _PortNumber;
	m_pInput = _pInput;

	return true;
}

void CPS3_Device::OnRefresh()
{
}

void CPS3_Device::OnLogCaps()
{

}

void CPS3_Device::OnFeedback(const float _fForce1, const float _fForce2)
{
}

// -------------------------------------------------------------------
//  CPS3_Device_Joystick
// -------------------------------------------------------------------
CPS3_Device_Joystick::CPS3_Device_Joystick()
{
	m_JoystickNr = 0;
	m_bActive = false;
}

void CPS3_Device_Joystick::Destroy()
{
	CPS3_Device::Destroy();
}

bool CPS3_Device_Joystick::Create(CInputContext_PS3* _pInput, int _PortNumber, int _iDevice)
{
	if (!CPS3_Device::Create(_pInput, _PortNumber, _iDevice)) return false;

	ConOutL(CStrF("(CPS3_Device_Joystick::Create) Port %.8x, Joystick nr %d", _PortNumber, _iDevice));

	m_JoystickNr = _iDevice;

	OnClearAxes();

	return true;
}

int CPS3_Device_Joystick::OnTranslateAxis(int _iAxis, int _Data)
{
	fp32 fAxis = (fp32(_Data) / 65535.0f) * 2.0f - 1.0f;
	fp32 AxisSign = Sign(fAxis);
	fAxis = M_Fabs(fAxis);

	// Deadzone
	fAxis = Max(0.0f, (fAxis - INPS3_JOY_AXISDEADZONE)) / (1.0f - INPS3_JOY_AXISDEADZONE);

	// Power
	if (fAxis > 0.0f) fAxis = exp(log(fAxis) * INPS3_JOY_AXISPOWER);

	return RoundToInt(fAxis * AxisSign * INPS3_JOY_AXISMAXVALUE);
}

CVec2Dint32 CPS3_Device_Joystick::OnTranslateAxis2D(int _x, int _y)
{
	int32 ix, iy;
	fp32 fx = (fp32(_x) / 65535.0f) * 2.0f - 1.0f;
	fp32 fy = (fp32(_y) / 65535.0f) * 2.0f - 1.0f;
	fx = Sign(fx) * Max(0.0f, (M_Fabs(fx) - INPS3_JOY_AXISDEADZONE1D)) / (1.0f - INPS3_JOY_AXISDEADZONE1D);
	fy = Sign(fy) * Max(0.0f, (M_Fabs(fy) - INPS3_JOY_AXISDEADZONE1D)) / (1.0f - INPS3_JOY_AXISDEADZONE1D);
	fp32 len = Length2(fx, fy);

	fp32 lendeadzone = Max(0.0f, (len - INPS3_JOY_AXISDEADZONE)) / (1.0f - INPS3_JOY_AXISDEADZONE);
	if (lendeadzone > 0.0f)
	{
		fp32 lenrcp = 1.0f / len;

		lendeadzone = Clamp01(lendeadzone);
		if (INPS3_JOY_AXISPOWER != 1.0f)
			lendeadzone = exp(log(lendeadzone) * INPS3_JOY_AXISPOWER);

		fp32 mulcollapse = lenrcp * lendeadzone * INPS3_JOY_AXISMAXVALUE;

		ix = RoundToInt(fx * mulcollapse);
		iy = RoundToInt(fy * mulcollapse);
	}
	else
	{
		ix = 0;
		iy = 0;
	}
/*	M_TRACEALWAYS("OnTranslateAxis2D (%d, %d) => %f, %f\n", _x-32768, _y-32768, fx, fy);

	CFStr Event = CFStrF("Ctrl %.3f, %.3f", fx, fy);
	M_NAMEDEVENT(Event.GetStr(), 0xffffffff);
*/
	return CVec2Dint32(ix, iy);
}

void CPS3_Device_Joystick::OnAxisData(int _iAxis, int _Data)
{
	int i = _iAxis;
	int Vec = OnTranslateAxis(i, _Data);

	if (Vec < 0)
	{
		m_pInput->UpKey(INPS3_JOY_AXISSCAN + i*2, 0, 0, 0, m_JoystickNr);
		Vec = -Vec;
		if (Vec > 128)
			m_pInput->DownKey(INPS3_JOY_AXISSCAN + i*2+1, 0, 0, 1, Vec, 0, m_JoystickNr);
		else
			m_pInput->UpKey(INPS3_JOY_AXISSCAN + i*2+1, 0, Vec, 0, m_JoystickNr);
	}
	else
	{
		m_pInput->UpKey(INPS3_JOY_AXISSCAN + i*2+1, 0, 0, 0, m_JoystickNr);
		if (Vec > 128)
			m_pInput->DownKey(INPS3_JOY_AXISSCAN + i*2, 0, 0, 1, Vec, 0, m_JoystickNr);
		else
			m_pInput->UpKey(INPS3_JOY_AXISSCAN + i*2, 0, Vec, 0, m_JoystickNr);
	}
	
}

void CPS3_Device_Joystick::OnAxisData2D(int _iAxis0, int _iAxis1, int _Data0, int _Data1)
{
	CVec2Dint32 Vec = OnTranslateAxis2D(_Data0, _Data1);

	int xvec = Vec[0];
	int yvec = Vec[1];

//	M_TRACEALWAYS("OnAxisData2D: %d, %d, %d, %d -> %d, %d\r\n", _iAxis0, _iAxis1, _Data0, _Data1, xvec, yvec);

	if (xvec < 0)
	{
		m_pInput->UpKey(INPS3_JOY_AXISSCAN + _iAxis0*2, 0, 0, 0, m_JoystickNr);
		xvec = -xvec;
		if (xvec > 128)
			m_pInput->DownKey(INPS3_JOY_AXISSCAN + _iAxis0*2+1, 0, 0, 1, xvec, 0, m_JoystickNr);
		else
			m_pInput->UpKey(INPS3_JOY_AXISSCAN + _iAxis0*2+1, 0, xvec, 0, m_JoystickNr);
	}
	else
	{
		m_pInput->UpKey(INPS3_JOY_AXISSCAN + _iAxis0*2+1, 0, 0, 0, m_JoystickNr);
		if (xvec > 128)
			m_pInput->DownKey(INPS3_JOY_AXISSCAN + _iAxis0*2, 0, 0, 1, xvec, 0, m_JoystickNr);
		else
			m_pInput->UpKey(INPS3_JOY_AXISSCAN + _iAxis0*2, 0, xvec, 0, m_JoystickNr);
	}

	if (yvec < 0)
	{
		m_pInput->UpKey(INPS3_JOY_AXISSCAN + _iAxis1*2, 0, 0, 0, m_JoystickNr);
		yvec = -yvec;
		if (yvec > 128)
			m_pInput->DownKey(INPS3_JOY_AXISSCAN + _iAxis1*2+1, 0, 0, 1, yvec, 0, m_JoystickNr);
		else
			m_pInput->UpKey(INPS3_JOY_AXISSCAN + _iAxis1*2+1, 0, yvec, 0, m_JoystickNr);
	}
	else
	{
		m_pInput->UpKey(INPS3_JOY_AXISSCAN + _iAxis1*2+1, 0, 0, 0, m_JoystickNr);
		if (yvec > 128)
			m_pInput->DownKey(INPS3_JOY_AXISSCAN + _iAxis1*2, 0, 0, 1, yvec, 0, m_JoystickNr);
		else
			m_pInput->UpKey(INPS3_JOY_AXISSCAN + _iAxis1*2, 0, yvec, 0, m_JoystickNr);
	}
}

void CPS3_Device_Joystick::OnClearAxes()
{
	for(int iAxis = 0; iAxis < INPS3_MAXAXES; iAxis++)
		OnAxisData(iAxis, 32768);
}

void CPS3_Device_Joystick::OnPOVData(int _iPOV, int _Data, int _OldData)
{
	int StateDiff = _Data ^ _OldData;

	if(StateDiff & 0x10)
	{
		if(_Data & 0x10)
			m_pInput->DownKey(INPS3_JOY_POVSCAN + _iPOV * 4 + 0, 0, 0, 1, 255, 0, m_JoystickNr);
		else
			m_pInput->UpKey(INPS3_JOY_POVSCAN + _iPOV * 4 + 0, 0, 0, 0, m_JoystickNr);
	}
	if(StateDiff & 0x20)
	{
		if(_Data & 0x20)
			m_pInput->DownKey(INPS3_JOY_POVSCAN + _iPOV * 4 + 1, 0, 0, 1, 255, 0, m_JoystickNr);
		else
			m_pInput->UpKey(INPS3_JOY_POVSCAN + _iPOV * 4 + 1, 0, 0, 0, m_JoystickNr);
	}
	if(StateDiff & 0x40)
	{
		if(_Data & 0x40)
			m_pInput->DownKey(INPS3_JOY_POVSCAN + _iPOV * 4 + 2, 0, 0, 1, 255, 0, m_JoystickNr);
		else
			m_pInput->UpKey(INPS3_JOY_POVSCAN + _iPOV * 4 + 2, 0, 0, 0, m_JoystickNr);
	}
	if(StateDiff & 0x80)
	{
		if(_Data & 0x80)
			m_pInput->DownKey(INPS3_JOY_POVSCAN + _iPOV * 4 + 3, 0, 0, 1, 255, 0, m_JoystickNr);
		else
			m_pInput->UpKey(INPS3_JOY_POVSCAN + _iPOV * 4 + 3, 0, 0, 0, m_JoystickNr);
	}
}


void CPS3_Device_Joystick::OnButtonData(int _iButton, int _Data)
{
	static uint8 Lookup[12] = {10, 11, 8, 9, 3, 1, 0, 2, 5, 6, 7, 4};
	uint32 iButton = Lookup[_iButton];

	if (_Data >= 128)
		m_pInput->DownKey(INPS3_JOY_BUTTONSCAN + iButton, 0, 0, 1, _Data, 0, m_JoystickNr);
	else
		m_pInput->UpKey(INPS3_JOY_BUTTONSCAN + iButton, 0, _Data, 0, m_JoystickNr);
}

void CPS3_Device_Joystick::OnRefresh()
{
	CellPadData PadData;
//	int Error = cellPadRead(m_PortNumber, &PadData);
	int Error = cellPadGetData(m_PortNumber, &PadData);
	if(Error == 0)
	{
		if(PadData.len > 0)
		{
			if((PadData.button[6] != m_State.m_lButtons[6]) || (PadData.button[7] != m_State.m_lButtons[7]))
				OnAxisData2D(0, 1, (PadData.button[6] << 8) | PadData.button[6], 65535 - ((PadData.button[7] << 8) | PadData.button[7]));
			if((PadData.button[4] != m_State.m_lButtons[4]) || (PadData.button[5] != m_State.m_lButtons[5]))
				OnAxisData2D(2, 3, (PadData.button[4] << 8) | PadData.button[4], ((PadData.button[5] << 8) | PadData.button[5]));
/*
			if(PadData.button[4] != m_State.m_lButtons[4])
				OnAxisData(2, (PadData.button[4] << 8) | PadData.button[4]);
			if(PadData.button[5] != m_State.m_lButtons[5])
				OnAxisData(3, ((PadData.button[5] << 8) | PadData.button[5]));

			if(PadData.button[6] != m_State.m_lButtons[6])
				OnAxisData(0, (PadData.button[6] << 8) | PadData.button[6]);
			if(PadData.button[7] != m_State.m_lButtons[7])
				OnAxisData(1, 65535 - (PadData.button[7] << 8) | PadData.button[7]);
*/
			if((PadData.button[2] ^ m_State.m_lButtons[2]) & 0xf0)
				OnPOVData(0, (PadData.button[2] & 0xf0), (m_State.m_lButtons[2] & 0xf0));

			// "standard" buttons
			uint8 Button3Diff = PadData.button[3] ^ m_State.m_lButtons[3];
			for(int i = 0; i < 8; i++)
			{
				int Mask = 1 << i;
				if(Button3Diff & Mask)
					OnButtonData(i, (PadData.button[3] & Mask) ? 255 : 0);
			}

			uint8 Button2Diff = PadData.button[2] ^ m_State.m_lButtons[2];
			for(int i = 0; i < 4; i++)
			{
				int Mask = 1 << i;
				if(Button2Diff & Mask)
					OnButtonData(i + 8, (PadData.button[2] & Mask) ? 255 : 0);
			}

			memcpy(m_State.m_lButtons, PadData.button, sizeof(uint16) * CELL_PAD_MAX_CODES);
		}
	}
#if 0
	XINPUT_STATE State;
	XInputGetState(m_PortNumber, &State);

	if (State.dwPacketNumber != m_PS3State.dwPacketNumber)
	{
		// Left analogue stick
		if (m_PS3State.Gamepad.sThumbLX != State.Gamepad.sThumbLX)
			OnAxisData(0, State.Gamepad.sThumbLX+32768);
		if (m_PS3State.Gamepad.sThumbLY != State.Gamepad.sThumbLY)
			OnAxisData(1, State.Gamepad.sThumbLY+32768);

		// Right analogue stick
		if (m_PS3State.Gamepad.sThumbRX != State.Gamepad.sThumbRX)
			OnAxisData(2, State.Gamepad.sThumbRX+32768);
		if (m_PS3State.Gamepad.sThumbRY != State.Gamepad.sThumbRY)
			OnAxisData(3, State.Gamepad.sThumbRY+32768);

		// Digital pad
		if ((m_PS3State.Gamepad.wButtons & 15) != (State.Gamepad.wButtons & 15))
		{
			OnPOVData(0, State.Gamepad.wButtons & 15, m_PS3State.Gamepad.wButtons & 15);
		}

		// Old analogue buttons
		for(int i = 0; i < 4; i++)
		{
			int Mask = (1 << (i+12));
			if ((m_PS3State.Gamepad.wButtons & Mask) != (State.Gamepad.wButtons & Mask))
				OnButtonData(i, (State.Gamepad.wButtons & Mask) ? 255 : 0);
		}

		// Other digital buttons
		{
			for(int i = 4; i < 10; i++)
			{
				int Mask = (1 << i);
				if ((m_PS3State.Gamepad.wButtons & Mask) != (State.Gamepad.wButtons & Mask))
					OnButtonData(i, (State.Gamepad.wButtons & Mask) ? 255 : 0);
			}
		}

		// Analogue buttons
		{
			if (m_PS3State.Gamepad.bLeftTrigger != State.Gamepad.bLeftTrigger)
				OnButtonData(10, State.Gamepad.bLeftTrigger);
			if (m_PS3State.Gamepad.bRightTrigger != State.Gamepad.bRightTrigger)
				OnButtonData(11, State.Gamepad.bRightTrigger);
		}

		m_PS3State = State;
	}
#endif
}

// -------------------------------------------------------------------
//  CPS3_Device_DebugKeyboard
// -------------------------------------------------------------------
CPS3_Device_KeyBoard::CPS3_Device_KeyBoard() : m_KBDNr(-1), m_LastModifier(0)
{
}

#if 0
static int g_ScanToVirtual[] = 
{
//		0							1							2							3							4							5							
//		6							7							8							9							a							b
//		c							d							e							f
/*0*/	0							,CELLKEYC_ESCAPE		,CELLKEYC_1				,CELLKEYC_2				,CELLKEYC_3				,CELLKEYC_4
		,CELLKEYC_5				,CELLKEYC_6				,CELLKEYC_7				,CELLKEYC_8				,CELLKEYC_9				,CELLKEYC_0				
		,CELLKEYC_MINUS			,/*VK_OEM_PLUS*/			,CELLKEYC_BS			,CELLKEYC_TAB
/*1*/	,CELLKEYC_Q				,CELLKEYC_W				,CELLKEYC_E				,CELLKEYC_R				,CELLKEYC_T				,CELLKEYC_Y				
		,CELLKEYC_U				,CELLKEYC_I				,CELLKEYC_O				,CELLKEYC_P				,0/*VK_OEM_4			*/	,0/*VK_OEM_6			*/	
		,CELLKEYC_ENTER			,VK_LCONTROL				,CELLKEYC_A				,CELLKEYC_S
/*2*/	,CELLKEYC_D				,CELLKEYC_F				,CELLKEYC_G				,CELLKEYC_H				,CELLKEYC_J				,CELLKEYC_K				
		,CELLKEYC_L				,0/*VK_OEM_1			*/	,0/*VK_OEM_7			*/	,0/*VK_OEM_3			*/	,VK_LSHIFT					,0/*VK_OEM_5			*/	
		,CELLKEYC_Z				,CELLKEYC_X				,CELLKEYC_C				,CELLKEYC_V
/*3*/	,CELLKEYC_B				,CELLKEYC_N				,CELLKEYC_M				,CELLKEYC_COMMA			,CELLKEYC_PERIOD		,0/*VK_OEM_2*/				
		,0							,VK_MULTIPLY				,VK_LMENU					,VK_SPACE					,VK_CAPITAL					,CELLKEYC_F1			
		,CELLKEYC_F2			,CELLKEYC_F3			,CELLKEYC_F4			,CELLKEYC_F5
/*4*/	,CELLKEYC_F6			,CELLKEYC_F7			,CELLKEYC_F8			,CELLKEYC_F9			,CELLKEYC_F10			,VK_PAUSE					
		,VK_SCROLL					,CELLKEYC_KPAD_7		,CELLKEYC_KPAD_8		,CELLKEYC_KPAD_9		,VK_SUBTRACT				,CELLKEYC_KPAD_4		
		,CELLKEYC_KPAD_5		,CELLKEYC_KPAD_6		,VK_ADD						,CELLKEYC_KPAD_1
/*5*/	,CELLKEYC_KPAD_2		,CELLKEYC_KPAD_3		,CELLKEYC_KPAD_0		,VK_DECIMAL					,0							,0							
		,VK_OEM_102					,CELLKEYC_F11			,CELLKEYC_F12			,VK_CLEAR					,0							,0							
		,0							,0							,0							,0
/*6*/	,0							,0							,0							,0							,0							,0							
		,0							,0							,0							,0							,0							,0							
		,0							,0							,0							,0
/*7*/	,0							,0							,0							,0							,0							,0							
		,0							,0							,0							,0							,0							,0							
		,0							,0							,0							,0
/*8*/	,0							,0							,0							,0							,0							,0							
		,0							,0							,0							,0							,0							,0							
		,0							,0							,0							,0
/*9*/	,VK_MEDIA_PREV_TRACK		,0							,0							,0							,0							,0							
		,0							,0							,0							,VK_MEDIA_NEXT_TRACK		,0							,0							
		,0							,VK_RCONTROL				,0							,0
/*a*/	,VK_VOLUME_MUTE				,VK_LAUNCH_APP2				,VK_MEDIA_PLAY_PAUSE		,0							,VK_MEDIA_STOP				,0							
		,0							,0							,0							,0							,0							,0							
		,0							,0							,VK_VOLUME_DOWN				,0
/*b*/	,VK_VOLUME_UP				,0							,VK_BROWSER_HOME			,0							,0							,VK_DIVIDE					
		,VK_RSHIFT					,VK_SNAPSHOT				,VK_RMENU					,0							,0							,0							
		,0							,0							,0							,0
/*c*/	,0							,0							,0							,0							,0							,VK_NUMLOCK					
		,0							,VK_HOME					,VK_UP						,VK_PRIOR					,0							,VK_LEFT					
		,0							,VK_RIGHT					,0							,VK_END
/*d*/	,VK_DOWN					,VK_NEXT					,VK_INSERT					,VK_DELETE					,0							,0							
		,0							,0							,0							,0							,0							,VK_LWIN					
		,VK_RWIN					,VK_APPS					,0							,0
/*e*/	,0							,0							,0							,0							,0							,VK_BROWSER_SEARCH			
		,VK_BROWSER_FAVORITES		,VK_BROWSER_REFRESH			,VK_BROWSER_STOP			,VK_BROWSER_FORWARD			,VK_BROWSER_BACK			,VK_LAUNCH_APP1				
		,VK_LAUNCH_MAIL				,VK_LAUNCH_MEDIA_SELECT		,0							,0
/*f*/	,0							,0							,0							,0							,0							,0							
		,0							,0							,0							,0							,0							,0							
		,0							,0							,0							,0
};
 // Fix this later
#else 
static int g_ScanToVirtual[256] = {0};
#endif

static int g_KeyToScan[256] = {

//00
	0, 0, 0, 0, 0, 0, 0, 0, SKEY_BACKSPACE, SKEY_TAB, SKEY_RETURN, 0, 0, 0, 0, 0,
//10
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//20
	SKEY_SPACE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//30
	SKEY_0, SKEY_1, SKEY_2, SKEY_3, SKEY_4, SKEY_5, SKEY_6, SKEY_7, SKEY_8, SKEY_9, 0, 0, 0, 0, 0, 0,
//40
	0, SKEY_A, SKEY_B, SKEY_C, SKEY_D, SKEY_E, SKEY_F, SKEY_G, SKEY_H, SKEY_I, SKEY_J, SKEY_K, SKEY_L, SKEY_M, SKEY_N, SKEY_O,
//50
	SKEY_P, SKEY_Q, SKEY_R, SKEY_S, SKEY_T, SKEY_U, SKEY_V, SKEY_W, SKEY_X, SKEY_Y, SKEY_Z, 0, 0, 0, 0, 0,
//60
	SKEY_PARAGRAPH, SKEY_A, SKEY_B, SKEY_C, SKEY_D, SKEY_E, SKEY_F, SKEY_G, SKEY_H, SKEY_I, SKEY_J, SKEY_K, SKEY_L, SKEY_M, SKEY_N, SKEY_O,
//70
	SKEY_P, SKEY_Q, SKEY_R, SKEY_S, SKEY_T, SKEY_U, SKEY_V, SKEY_W, SKEY_X, SKEY_Y, SKEY_Z, 0, 0, 0, SKEY_PARAGRAPH, 0,
//80
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//90
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//a0
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//b0
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//c0
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//d0
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//e0
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//f0
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

};

static int g_VirtualToScan[256];
static uint g_PS3KeyboardDebug = 0;

bool CPS3_Device_KeyBoard::Create(CInputContext_PS3* _pInput, int _PortNumber, int _iDevice)
{
	m_KBDNr = _PortNumber;

	for (int i = 0; i < 256; ++i)
		g_VirtualToScan[i] = 0;
	for (int i = 0; i < 256; ++i)
	{
		if (g_ScanToVirtual[i])
		{
			if (i >= 128)
				g_VirtualToScan[g_ScanToVirtual[i]] = i+128;
			else
				g_VirtualToScan[g_ScanToVirtual[i]] = i;
		}
	}

	return CPS3_Device::Create(_pInput, _PortNumber, _iDevice);	
}

void CPS3_Device_KeyBoard::OnRefresh()
{
	while (1)
	{
		CellKbData KBData;   /* keyboard data buffer */
		int32 ret = cellKbRead (m_KBDNr, &KBData);
	    
		if (CELL_KB_OK != ret)
		{
			break;
		}
		if (KBData.len == 0)
			break;

		if(g_PS3KeyboardDebug)
			M_TRACEALWAYS("Led 0x%08x MKey 0x%08x\n", KBData.led, KBData.mkey);
/*
		int Modifier = KBData.mkey;
		if(m_LastModifier != Modifier)
		{
			int Diff = m_LastModifier ^ Modifier;
			if(Diff & 1)
			{
				if(Modifier & 1)
					m_pInput->DownKey(SKEY_MODIFIER_CONTROL, 0, 0, 0);
				else
					m_pInput->UpKey(SKEY_MODIFIER_CONTROL, 0, 0, 0);
			}
			if(Diff & 2)
			{
				if(Modifier & 2)
					m_pInput->DownKey(SKEY_MODIFIER_SHIFT, 0, 0, 0);
				else
					m_pInput->UpKey(SKEY_MODIFIER_SHIFT, 0, 0, 0);
			}
			if(Diff & 4)
			{
				if(Modifier & 4)
					m_pInput->DownKey(SKEY_MODIFIER_ALT, 0, 0, 0);
				else
					m_pInput->UpKey(SKEY_MODIFIER_ALT, 0, 0, 0);
			}
		}
*/

		int Modifier = ((KBData.mkey & 1)?SKEY_MODIFIER_CONTROL:0) + ((KBData.mkey & 2)?SKEY_MODIFIER_SHIFT:0) + ((KBData.mkey & 4)?SKEY_MODIFIER_ALT:0);
		for (int i = 0; i < KBData.len; ++i)
		{
			int KeyCode = KBData.keycode[i];
			if(g_PS3KeyboardDebug)
				M_TRACEALWAYS("KeyData 0x%04x\n", KeyCode);
			if(KeyCode > 0 && KeyCode < 256)
			{
				int ScanCode = g_KeyToScan[KeyCode] + Modifier;
				m_pInput->DownKey(ScanCode, KeyCode, 0, 0);
				m_pInput->UpKey(ScanCode, 0, 0, 0);
			}
			else
			{
				int ScanCode = -1;
				switch(KeyCode)
				{
				case 0x8029: ScanCode = SKEY_ESC; break;
				case 0x803a: ScanCode = SKEY_F1; break;
				case 0x803b: ScanCode = SKEY_F2; break;
				case 0x803c: ScanCode = SKEY_F3; break;
				case 0x803d: ScanCode = SKEY_F4; break;
				case 0x803e: ScanCode = SKEY_F5; break;
				case 0x803f: ScanCode = SKEY_F6; break;
				case 0x8040: ScanCode = SKEY_F7; break;
				case 0x8041: ScanCode = SKEY_F8; break;
				case 0x8042: ScanCode = SKEY_F9; break;
				case 0x8043: ScanCode = SKEY_F10; break;
				case 0x8044: ScanCode = SKEY_F11; break;
				case 0x8045: ScanCode = SKEY_F12; break;

				case 0x8049: ScanCode = SKEY_INSERT; break;
				case 0x804a: ScanCode = SKEY_HOME; break;
				case 0x804b: ScanCode = SKEY_PAGEUP; break;
				case 0x804c: ScanCode = SKEY_DELETE; break;
				case 0x804d: ScanCode = SKEY_END; break;
				case 0x804e: ScanCode = SKEY_PAGEDOWN; break;

				case 0x804f: ScanCode = SKEY_CURSOR_RIGHT; break;
				case 0x8050: ScanCode = SKEY_CURSOR_LEFT; break;
				case 0x8051: ScanCode = SKEY_CURSOR_DOWN; break;
				case 0x8052: ScanCode = SKEY_CURSOR_UP; break;
				}

				if(ScanCode != -1)
				{
					m_pInput->DownKey(ScanCode + Modifier, 0, 0, 0);
					m_pInput->UpKey(ScanCode + Modifier, 0, 0, 0);
				}
			}
		}
	}

#if 0
	XINPUT_STATE State;
	XInputGetState(m_PortNumber, &State);

	while (1)
	{
		XINPUT_KEYSTROKE KeyStroke;
		if (XInputGetKeystroke(m_PortNumber, XINPUT_FLAG_KEYBOARD, &KeyStroke) != ERROR_SUCCESS)
			break;

		if (KeyStroke.VirtualKey >= 0 && KeyStroke.VirtualKey < 256)
		{
			int ScanCode = g_VirtualToScan[KeyStroke.VirtualKey];
			if (KeyStroke.Flags & XINPUT_KEYSTROKE_KEYUP)
				m_pInput->UpKey(ScanCode, 0, 0, 0);
			else
				m_pInput->DownKey(ScanCode, KeyStroke.Unicode, 0, 0);
		}
	}
#endif
}


// -------------------------------------------------------------------
//  CInputContext_PS3
// -------------------------------------------------------------------
CStr CInputContext_PS3::PS3_MessageStr(int err)
{
	return "PS3_MessageStr is not implemented.";
};

// -------------------------------------------------------------------
bool CInputContext_PS3::PS3_CreateDevice(const char* _pClassName, int _PortNumber, int _iDevice)
{
	spCReferenceCount spObj = (CReferenceCount*) MRTC_GetObjectManager()->GetClassRegistry()->CreateObject(_pClassName);
	TPtr<CPS3_Device> spDevice = safe_cast<CPS3_Device>((CReferenceCount*)spObj);
	if (!spDevice) return false;

	if (!spDevice->Create(this, _PortNumber, _iDevice)) return false;

	m_lspDevices[_iDevice] = spDevice;
	return true;
}



// -------------------------------------------------------------------
void CInputContext_PS3::PS3_CreateDevices()
{
//	ConOutL(CStr("(CInputContext_PS3::PS3_CreateDevices) Creating input devices..."));
	bint bChanged = false;

	CellPadInfo PadInfo;/* connection PadInformation buffer */
    if (cellPadGetInfo (&PadInfo) == CELL_PAD_OK) 
	{
		for (int i = 0; i < PadInfo.max_connect; ++i)
		{
			int iDev = INPS3_JOYSTICKSTARTDEV + i;

            if (!PadInfo.status[i]) 
			{
				if (m_PadInfo.status[i])
				{
					if (m_lspDevices[iDev])
					{
						bChanged = true;
						m_lspDevices[iDev] = NULL;
						m_lLastTouchTime[iDev].Reset();
					}
				}
            }
			else
			{
				if (!m_PadInfo.status[i])
				{
					if (!m_lspDevices[iDev])
					{
						m_lspDevices[iDev] = NULL;
						if (!PS3_CreateDevice("CPS3_Device_Joystick", i, iDev))
							ConOutL(CStrF("§cf80WARNING: (CInputContext_PS3::PS3_CreateDevices) Unable to create joystick device %d", iDev));
						else
						{
							ConOutL(CStrF("(CInputContext_PS3::PS3_CreateDevices) Created joystick device %d", iDev));
							bChanged = true;
						}
					}
				}
			}

		}

		m_PadInfo = PadInfo; // Save old state
    }

	CellKbInfo KBInfo;

    if (cellKbGetInfo(&KBInfo) == CELL_KB_OK) 
	{
		for (int i = 0; i < KBInfo.max_connect; ++i)
		{
			int iDev = INPS3_KEYBOARDSTARTDEV + i;
            if (!KBInfo.status[i]) 
			{
				if (m_KBInfo.status[i])
				{
					int8 iDev = m_KBUSB2Dev[i];
					if (m_lspDevices[iDev])
					{
						bChanged = true;
						m_lspDevices[iDev] = NULL;
						m_lLastTouchTime[iDev].Reset();
						m_KBUSB2Dev[i] = -1;
					}
				}
            }
			else
			{
				if (!m_KBInfo.status[i])
				{
					if(m_KBUSB2Dev[i] < 0)
					{
						// New keyboard
						m_KBUSB2Dev[i] = iDev;
						if (!m_lspDevices[iDev])
						{
							m_lspDevices[iDev] = NULL;
							if (!PS3_CreateDevice("CPS3_Device_KeyBoard", i, iDev))
								ConOutL(CStrF("§cf80WARNING: (CInputContext_PS3::PS3_CreateDevices) Unable to create keyboard device %d", iDev));
							else
							{
								ConOutL(CStrF("(CInputContext_PS3::PS3_CreateDevices) Created keyboard keyboard %d", iDev));
								bChanged = true;
							}
						}
					}
				}
			}

		}

		m_KBInfo = KBInfo; // Save old state
    }


	if (bChanged)
		DownKey(SKEY_JOY_INSERTREMOVE, 0, 0);

#if 0
	int bChanged = false;
	// Try to create 4 joystick/gamepad devices.
	int NumQuery = 4;
	int QueryIndex = 0;
	for (int j = 0; j < 2; ++j)
	{
		for(int i = 0; i < NumQuery; i++)
		{
			int iDev = QueryIndex+i;
			XINPUT_CAPABILITIES Caps;
			int Success = XInputGetCapabilities(i, QueryType, &Caps);
			if (Success == ERROR_SUCCESS)
			{
				m_lLastTouchTime[iDev] = CMTime::GetCPU();
				if (Caps.Type == XINPUT_DEVTYPE_GAMEPAD)
				{
					if (!m_lspDevices[iDev] || m_lspDevices[iDev]->m_DeviceType != XINPUT_DEVTYPE_GAMEPAD)
					{
						m_lspDevices[iDev] = NULL;
						if (!PS3_CreateDevice("CPS3_Device_Joystick", i, iDev))
							ConOutL(CStrF("§cf80WARNING: (CInputContext_PS3::PS3_CreateDevices) Unable to create joystick device %d", iDev));
						else
						{
							ConOutL(CStrF("(CInputContext_PS3::PS3_CreateDevices) Created joystick device %d", iDev));
							bChanged = true;
						}
					}
				}
				else if (Caps.Type == XINPUT_DEVTYPE_USB_KEYBOARD)
				{
					if (!m_lspDevices[iDev] || m_lspDevices[iDev]->m_DeviceType != XINPUT_DEVTYPE_USB_KEYBOARD)
					{
						m_lspDevices[iDev] = NULL;
						if (!PS3_CreateDevice("CPS3_Device_KeyBoard", i, iDev))
							ConOutL(CStrF("§cf80WARNING: (CInputContext_PS3::PS3_CreateDevices) Unable to create keyboard device %d", iDev));
						else
						{
							ConOutL(CStrF("(CInputContext_PS3::PS3_CreateDevices) Created joystick keyboard %d", iDev));
							bChanged = true;
						}
					}
				}
				else
				{
					if (m_lspDevices[iDev])
					{
						bChanged = true;
						m_lspDevices[iDev] = NULL;
						m_lLastTouchTime[iDev].Reset();
					}
				}
			}
			else
			{
				if (m_lspDevices[iDev])
				{
					bChanged = true;
					m_lspDevices[iDev] = NULL;
					m_lLastTouchTime[iDev].Reset();
				}
			}
		}
		QueryType = XINPUT_FLAG_KEYBOARD;
		NumQuery = 1;
		QueryIndex = 4;
	}

	if (bChanged)
		DownKey(SKEY_JOY_INSERTREMOVE, 0, 0);
#endif
}

// -------------------------------------------------------------------
void CInputContext_PS3::PS3_DestroyDevices()
{
	ConOutL(CStr("(CInputContext_PS3::PS3_DestroyDevices) Destroying input devices..."));
	m_lspDevices.Clear();
}

// -------------------------------------------------------------------
CInputContext_PS3::CInputContext_PS3()
{
}

CInputContext_PS3::~CInputContext_PS3()
{
	LogFile("(CInputContext_PS3::~CInputContext_PS3)");
	MACRO_RemoveSubSystem(this);
	
	M_TRY
	{
		PS3_DestroyDevices();
	}
	M_CATCH(
	catch(CCException)
	{
		LogFile("(CInputContext_PS3::~CInputContext_PS3) Exception during PS3_DestroyDevices()!");
	}
	catch(...)
	{
		LogFile("(CInputContext_PS3::~CInputContext_PS3) Hardware exception during PS3_DestroyDevices()!");
	}
	)
	LogFile("(CInputContext_PS3::~CInputContext_PS3) Done");
}

void CInputContext_PS3::Create(const char* _pParams)
{
	//	LogFile("(CInputContext_PS3::Create)");
	CInputContextCore::Create(_pParams);
	memset(&m_PadInfo, 0, sizeof(m_PadInfo));
	memset(&m_KBInfo, 0, sizeof(m_KBInfo));
	memset(m_KBUSB2Dev, 0xff, sizeof(m_KBUSB2Dev));

	MACRO_AddSubSystem(this);

	m_lspDevices.SetLen(INPS3_MAXJOYSTICKS + INPS3_MAXKEYBOARDS);

    for (int i = 0; i < INPS3_MAXKEYBOARDS; i++) 
	{
/*
		if ((cellKbSetRepeat (i, 30, 2)) != CELL_KB_OK) 
		{
			Error("Create", "Failed to initialize USB Keyboard Library attrib");
		}
		if ((cellKbSetArrangement (i, CELL_KB_ARRANGEMENT_101))!= CELL_KB_OK) 
		{
			Error("Create", "Failed to initialize USB Keyboard Library attrib");
		}
 */
		if ((cellKbSetReadMode (i, CELL_KB_RMODE_INPUTCHAR))!= CELL_KB_OK) 
		{
			Error("Create", "Failed to initialize USB Keyboard Library attrib");
		}
		if ((cellKbSetCodeType (i, CELL_KB_CODETYPE_ASCII))!= CELL_KB_OK) 
		{
			Error("Create", "Failed to initialize USB Keyboard Library attrib");
		}
    }


	PS3_CreateDevices();

	Update();
}

// -------------------------------------------------------------------
void CInputContext_PS3::Update()
{
	M_LOCK(m_InputLock);
	// Any device changes?
	PS3_CreateDevices();

	// Refresh all devices
	M_TRY
	{
//		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");

		for(int iDev = 0; iDev < m_lspDevices.Len(); iDev++)
		{
			CPS3_Device* pDev = m_lspDevices[iDev];
			if (pDev)
			{
				pDev->OnRefresh();
			}
		}
	}
	M_CATCH(
	catch(CCException)
	{
	}
	)
} 

bool CInputContext_PS3::IsGamepadValid(int _iPad)
{
	if(_iPad < 0 || _iPad >= 4)
		return false;

	return m_lspDevices[_iPad] != NULL;
}

aint CInputContext_PS3::OnMessage(const CSS_Msg& _Msg)
{
	return 0;
}

#endif // PLATFORM_PS3
