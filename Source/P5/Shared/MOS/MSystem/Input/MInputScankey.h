/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Engine input scancodes
					
	Author:			Jim Kjellin
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		030506:		Added comments
\*_____________________________________________________________________________________________*/

#ifndef _INC_MINPUTSCANKEY
#define _INC_MINPUTSCANKEY

enum
{
#ifdef PLATFORM_CONSOLE
	INPUT_MAXGAMEPADS=4,
#else
	INPUT_MAXGAMEPADS=1,
#endif

// -------------------------------------------------------------------
//  CScanKey defintions
// -------------------------------------------------------------------
	CKEYBOARD_MAXUSERSHIFTKEYS=16,	// nBits left in a dword.
	CKEYBOARD_KBBSIZE=64,			// Keyboard buffer

	SKEY_MODIFIER_SHIFT		=0x200,
	SKEY_MODIFIER_ALT		=0x400,
	SKEY_MODIFIER_CONTROL	=0x800,

	SKEY_CURSOR_LEFT	=0x14b,
	SKEY_CURSOR_UP		=0x148,
	SKEY_CURSOR_RIGHT	=0x14d,
	SKEY_CURSOR_DOWN	=0x150,

	SKEY_INSERT			=0x152,
	SKEY_DELETE			=0x153,
	SKEY_HOME			=0x147,
	SKEY_END			=0x14f,
	SKEY_PAGEUP			=0x149,
	SKEY_PAGEDOWN		=0x151,

	SKEY_BACKSPACE		=0x0e,
	SKEY_RETURN			=0x1c,
	SKEY_TAB			=0x0f,
	SKEY_SPACE			=0x39,
	SKEY_ESC			=0x01,

	SKEY_F1				=0x3b,
	SKEY_F2				=0x3c,
	SKEY_F3				=0x3d,
	SKEY_F4				=0x3e,
	SKEY_F5				=0x3f,
	SKEY_F6				=0x40,
	SKEY_F7				=0x41,
	SKEY_F8				=0x42,
	SKEY_F9				=0x43,
	SKEY_F10			=0x44,
	SKEY_F11			=0x57,
	SKEY_F12			=0x58,

	SKEY_LEFT_SHIFT		=0x2a,
	SKEY_RIGHT_SHIFT	=0x36,
	SKEY_LEFT_CONTROL	=0x1d,
	SKEY_RIGHT_CONTROL	=0x11d,
	SKEY_LEFT_ALT		=0x38,
	SKEY_RIGHT_ALT		=0x138,
	SKEY_LWIN			=0x15b,
	SKEY_RWIN			=0x15c,
	SKEY_APPS			=0x15d,

	SKEY_CAPSLOCK		=0x3a,
	SKEY_PARAGRAPH		=0x29,

	SKEY_1			=0x02,
	SKEY_2			=0x03,
	SKEY_3			=0x04,
	SKEY_4			=0x05,
	SKEY_5			=0x06,
	SKEY_6			=0x07,
	SKEY_7			=0x08,
	SKEY_8			=0x09,
	SKEY_9			=0x0a,
	SKEY_0			=0x0b,

	SKEY_Q			=0x10,
	SKEY_W			=0x11,
	SKEY_E			=0x12,
	SKEY_R			=0x13,
	SKEY_T			=0x14,
	SKEY_Y			=0x15,
	SKEY_U			=0x16,
	SKEY_I			=0x17,
	SKEY_O			=0x18,
	SKEY_P			=0x19,

	SKEY_A			=0x1e,
	SKEY_S			=0x1f,
	SKEY_D			=0x20,
	SKEY_F			=0x21,
	SKEY_G			=0x22,
	SKEY_H			=0x23,
	SKEY_J			=0x24,
	SKEY_K			=0x25,
	SKEY_L			=0x26,

	SKEY_Z			=0x2c,
	SKEY_X			=0x2d,
	SKEY_C			=0x2e,
	SKEY_V			=0x2f,
	SKEY_B			=0x30,
	SKEY_N			=0x31,
	SKEY_M			=0x32,

// Joystick scans:
//
// "Axis" is used for all input types: sticks, sliders and buttons.
// An axis is 2-dimensional, but for buttons and sliders only one dimension is used.
// Axis state is stored in m_Data of CScanKey

	SKEY_JOY_START=0x80,
#ifdef PLATFORM_XBOX1

	SKEY_JOY0_AXIS00 = SKEY_JOY_START,
	SKEY_JOY0_AXIS01,
	SKEY_JOY0_AXIS02,
	SKEY_JOY0_AXIS03,
	SKEY_JOY0_AXIS04,
	SKEY_JOY0_AXIS05,
	SKEY_JOY0_AXIS06,
	SKEY_JOY0_AXIS07,
	SKEY_JOY0_AXIS08,
	SKEY_JOY0_AXIS09,
	SKEY_JOY0_AXIS0A,
	SKEY_JOY0_AXIS0B,
	SKEY_JOY0_AXIS0C,
	SKEY_JOY0_AXIS0D,
	SKEY_JOY0_AXIS0E,
	SKEY_JOY0_AXIS0F,

	SKEY_JOY1_AXIS00,
	SKEY_JOY1_AXIS01,
	SKEY_JOY1_AXIS02,
	SKEY_JOY1_AXIS03,
	SKEY_JOY1_AXIS04,
	SKEY_JOY1_AXIS05,
	SKEY_JOY1_AXIS06,
	SKEY_JOY1_AXIS07,
	SKEY_JOY1_AXIS08,
	SKEY_JOY1_AXIS09,
	SKEY_JOY1_AXIS0A,
	SKEY_JOY1_AXIS0B,
	SKEY_JOY1_AXIS0C,
	SKEY_JOY1_AXIS0D,
	SKEY_JOY1_AXIS0E,
	SKEY_JOY1_AXIS0F,

	SKEY_JOY2_AXIS00,
	SKEY_JOY2_AXIS01,
	SKEY_JOY2_AXIS02,
	SKEY_JOY2_AXIS03,
	SKEY_JOY2_AXIS04,
	SKEY_JOY2_AXIS05,
	SKEY_JOY2_AXIS06,
	SKEY_JOY2_AXIS07,
	SKEY_JOY2_AXIS08,
	SKEY_JOY2_AXIS09,
	SKEY_JOY2_AXIS0A,
	SKEY_JOY2_AXIS0B,
	SKEY_JOY2_AXIS0C,
	SKEY_JOY2_AXIS0D,
	SKEY_JOY2_AXIS0E,
	SKEY_JOY2_AXIS0F,

	SKEY_JOY3_AXIS00,
	SKEY_JOY3_AXIS01,
	SKEY_JOY3_AXIS02,
	SKEY_JOY3_AXIS03,
	SKEY_JOY3_AXIS04,
	SKEY_JOY3_AXIS05,
	SKEY_JOY3_AXIS06,
	SKEY_JOY3_AXIS07,
	SKEY_JOY3_AXIS08,
	SKEY_JOY3_AXIS09,
	SKEY_JOY3_AXIS0A,
	SKEY_JOY3_AXIS0B,
	SKEY_JOY3_AXIS0C,
	SKEY_JOY3_AXIS0D,
	SKEY_JOY3_AXIS0E,
	SKEY_JOY3_AXIS0F,

	SKEY_JOY_END = SKEY_JOY3_AXIS0F,
#else

	SKEY_JOY_AXIS00_POS = SKEY_JOY_START,
	SKEY_JOY_AXIS00_NEG,
	SKEY_JOY_AXIS01_POS,
	SKEY_JOY_AXIS01_NEG,
	SKEY_JOY_AXIS02_POS,
	SKEY_JOY_AXIS02_NEG,
	SKEY_JOY_AXIS03_POS,
	SKEY_JOY_AXIS03_NEG,
	SKEY_JOY_AXIS04_POS,
	SKEY_JOY_AXIS04_NEG,
	SKEY_JOY_AXIS05_POS,
	SKEY_JOY_AXIS05_NEG,
	SKEY_JOY_AXIS06_POS,
	SKEY_JOY_AXIS06_NEG,
	SKEY_JOY_AXIS07_POS,
	SKEY_JOY_AXIS07_NEG,
	SKEY_JOY_AXIS08_POS,
	SKEY_JOY_AXIS08_NEG,
	SKEY_JOY_AXIS09_POS,
	SKEY_JOY_AXIS09_NEG,
	SKEY_JOY_AXIS0A_POS,
	SKEY_JOY_AXIS0A_NEG,
	SKEY_JOY_AXIS0B_POS,
	SKEY_JOY_AXIS0B_NEG,
	SKEY_JOY_AXIS0C_POS,
	SKEY_JOY_AXIS0C_NEG,
	SKEY_JOY_AXIS0D_POS,
	SKEY_JOY_AXIS0D_NEG,
	SKEY_JOY_AXIS0E_POS,
	SKEY_JOY_AXIS0E_NEG,
	SKEY_JOY_AXIS0F_POS,
	SKEY_JOY_AXIS0F_NEG,

	SKEY_JOY_BUTTON00,
	SKEY_JOY_BUTTON01,
	SKEY_JOY_BUTTON02,
	SKEY_JOY_BUTTON03,
	SKEY_JOY_BUTTON04,
	SKEY_JOY_BUTTON05,
	SKEY_JOY_BUTTON06,
	SKEY_JOY_BUTTON07,
	SKEY_JOY_BUTTON08,
	SKEY_JOY_BUTTON09,
	SKEY_JOY_BUTTON0A,
	SKEY_JOY_BUTTON0B,
	SKEY_JOY_BUTTON0C,
	SKEY_JOY_BUTTON0D,
	SKEY_JOY_BUTTON0E,
	SKEY_JOY_BUTTON0F,

	SKEY_JOY_POV00,
	SKEY_JOY_POV01,
	SKEY_JOY_POV02,
	SKEY_JOY_POV03,
	SKEY_JOY_POV04,
	SKEY_JOY_POV05,
	SKEY_JOY_POV06,
	SKEY_JOY_POV07,
	SKEY_JOY_POV08,
	SKEY_JOY_POV09,
	SKEY_JOY_POV0A,
	SKEY_JOY_POV0B,
	SKEY_JOY_POV0C,
	SKEY_JOY_POV0D,
	SKEY_JOY_POV0E,
	SKEY_JOY_POV0F,

	SKEY_JOY_END = SKEY_JOY_POV0F,
#endif

	SKEY_GUISTART		=0xE0,
	SKEY_GUI_UP			=SKEY_GUISTART,
	SKEY_GUI_DOWN,
	SKEY_GUI_LEFT,
	SKEY_GUI_RIGHT,
	SKEY_GUI_BUTTON0,
	SKEY_GUI_BUTTON1,
	SKEY_GUI_BUTTON2,
	SKEY_GUI_BUTTON3,
	SKEY_GUI_BUTTON4,
	SKEY_GUI_BUTTON5,
	SKEY_GUIEND,
	SKEY_JOY_INSERTREMOVE = SKEY_GUIEND,


	SKEY_MOUSE1=		0xf0,
	SKEY_MOUSE2,
	SKEY_MOUSE3,
	SKEY_MOUSE4,
	SKEY_MOUSE5,
	SKEY_MOUSE6,
	SKEY_MOUSE7,
	SKEY_MOUSE8,
	SKEY_MOUSEWHEELUP,
	SKEY_MOUSEWHEELDOWN,
	SKEY_MOUSEMOVE,
	SKEY_MOUSEMOVEREL,

	// Aliases
	SKEY_GUI_OK			=SKEY_GUI_BUTTON0,
	SKEY_GUI_CANCEL		=SKEY_GUI_BUTTON1,
	SKEY_GUI_START		=SKEY_GUI_BUTTON4,
	SKEY_GUI_BACK		=SKEY_GUI_BUTTON5,

	SKEY_REPEAT=		0x4000,
	SKEY_UP=			0x8000,
};


// -------------------------------------------------------------------
class CScanCodeName
{
public:
	int m_ScanCode;
	int m_ScanKey32;
	char* m_pName;
};

// -------------------------------------------------------------------
//  CScanKey
// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CScanKey : public CObj
{
public:
	int m_ScanKey32;
	int m_Data[2];
	fp8 m_Time;				// Seconds
	wchar m_Char;
	uint16 m_iDevice;

	static CScanCodeName ms_ScanCodeNames[];

	// 0..7		Scancode
	// 8..15	Std shift-key flags
	// 16..31	User defined shift-key flags
	// Data[0]  MouseX
	// Data[1]  MouseY
	// m_iDevice Joystick number or other numbered device

	CScanKey() 
	{ 
		m_ScanKey32 = 0; m_Char = 0; m_Data[0] = 0; m_Data[1] = 0; m_iDevice = 0;
	};

	const int GetKey9() const { return (m_ScanKey32 & 0x1ff); };
	const int GetStdShift() const { return ((m_ScanKey32 & 0xfe00) >> 9); };
	const int GetKey16() const { return (m_ScanKey32 & 0xffff); };
	const int GetUserShift() const { return ((m_ScanKey32 & 0xffff0000) >> 16); };

	void SetPosX(int _v) { m_Data[0] = _v; };
	void SetPosY(int _v) { m_Data[1] = _v; };
	const int GetPosX() const { return m_Data[0]; };
	const int GetPosY() const { return m_Data[1]; };

	const bool IsDown() const { return ((m_ScanKey32 & SKEY_UP) == 0); };
	const bool IsRepeat() const { return ((m_ScanKey32 & SKEY_REPEAT) != 0); };

	bool IsASCII() const;
	char GetASCII() const;

	static int GetScanCode(CStr _s);
	static int GetScanKey32(CStr _s);
	static int GetScanKey32Comb(CStr _s);
	static CStr GetScanCodeName(int _Scan);
	static CStr GetScanKey32Name(int _ScanKey);
	static CStr GetLocalizedKeyName(int _ScanKey);

//	static CScanCodeName* GetScanCodeNames();
//	static int GetNumScanCodeNames();
};

#endif // _INC_MINPUTSCANKEY
