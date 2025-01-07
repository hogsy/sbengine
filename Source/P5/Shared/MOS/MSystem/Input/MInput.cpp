#include "PCH.h"
#include "../MSystem.h"
#include "MInput.h"
#include "MInputCore.h"

// -------------------------------------------------------------------
//  CInputContextCore
// -------------------------------------------------------------------
bool CInputContextCore::ms_bInputSysCreated = false;

CInputContextCore::CInputContextCore()
{
	m_bRemove = false;
	ms_bInputSysCreated = true;

	{
		for(int i = 0; i < 512; i++)
			m_KBState[i] = SKEY_UP;
	}

	m_nUserSK = 0;

//	if (g_pOS->m_spCon == NULL) Error("-", "No console!");
//	g_pOS->m_spCon->AddSubSystem(this);

	m_MouseArea = CRct(0, 0, 640, 480);
	m_MousePos = CPnt(0, 0);
	m_MouseMove.x = 0;
	m_MouseMove.y = 0;
	m_MouseButtons = 0;
	m_MouseButtonsAcc = 0;
	m_ShowCursor = true;

	SetFeedbackAmount(1);

	{
		for(int i = 0; i < INPUT_MAXGAMEPADS; i++)
		{
			m_lGamePadMappings[i] = i;
			m_lLastTouchTime[i].Reset();
		}
	}

	MACRO_AddSubSystem(this);
}

CInputContextCore::~CInputContextCore()
{
	if (m_bRemove) RemoveFromConsole();
	MACRO_RemoveSubSystem(this);
	
	ms_bInputSysCreated = false;

//	if (g_pOS->m_spCon == NULL) Error("~", "No console!");
//	g_pOS->m_spCon->RemoveSubSystem(this);
	FillChar(&m_KBState,sizeof(m_KBState),0);
}

int CInputContextCore::GetCurrentShiftKeyState()
{
	int Mask = 0;
	if (!(m_KBState[SKEY_LEFT_SHIFT] & SKEY_UP) || !(m_KBState[SKEY_LEFT_SHIFT] & SKEY_UP)) 
		Mask |= SKEY_MODIFIER_SHIFT;

	if (!(m_KBState[SKEY_LEFT_ALT] & SKEY_UP) || !(m_KBState[SKEY_RIGHT_ALT] & SKEY_UP)) 
		Mask |= SKEY_MODIFIER_ALT;

	if (!(m_KBState[SKEY_LEFT_CONTROL] & SKEY_UP) || !(m_KBState[SKEY_RIGHT_CONTROL] & SKEY_UP)) 
		Mask |= SKEY_MODIFIER_CONTROL;

	 //|| !(m_KBState[SKEY_RIGHT_CONTROL] & SKEY_UP)

	return Mask;
}

void CInputContextCore::Create(const char* _pParams)
{
	m_Envelopes.Create();

	typedef TQueue<CScanKey> TQueue_CScanKey;
	
	m_spKBB = MNew1(TQueue_CScanKey, (int)CKEYBOARD_KBBSIZE);
	if (m_spKBB == NULL) MemError("-");

	m_EnvelopeInstances.Create();

	AddToConsole();
	m_bRemove = true;
}

void CInputContextCore::Update()
{
	// Make sure all envelopes are updated
	m_EnvelopeInstances.Update();
}

bool CInputContextCore::QueueScanKey(CScanKey _Scan)
{
	int iPadPhysical = _Scan.m_iDevice;
	m_lLastTouchTime[iPadPhysical] = CMTime::GetCPU();

	if (!m_spKBB->MaxPut())
		return false;

#ifdef PLATFORM_CONSOLE
	int Scan = _Scan.GetKey9();
	if (Scan >= SKEY_JOY_START && Scan <= SKEY_JOY_END)
	{
		int iPadScanCode = m_lGamePadMappings[iPadPhysical];
		if (iPadScanCode >= 0)
			m_spKBB->Put(_Scan);
	}
	else
#endif
		m_spKBB->Put(_Scan);

	return true;
}

void CInputContextCore::AddScanKey(CScanKey _Scan)
{
	QueueScanKey(_Scan);

	m_KBState[_Scan.m_ScanKey32 & 511] = _Scan.m_ScanKey32;
}

void CInputContextCore::DownKey(int _ScanCode, wchar _Char, fp8 _Time, int _nRep, int _Data0, int _Data1, int _iDevice)
{
	CScanKey k;
	k.m_ScanKey32 = int(_ScanCode);
	if (_ScanCode < 0x80 || (_ScanCode >= 0x100 && _ScanCode < 0x180))
		k.m_ScanKey32 += GetCurrentShiftKeyState();
	k.m_Char = _Char;
	k.m_Time = _Time;
	k.m_Data[0] = _Data0;
	k.m_Data[1] = _Data1;
	k.m_ScanKey32 &= ~SKEY_UP;
	k.m_iDevice = _iDevice;

	// If the scan was already 'down', set the repeat flag.
	if (!(m_KBState[k.m_ScanKey32 & 511] & SKEY_UP))
		k.m_ScanKey32 |= SKEY_REPEAT;

	// Add to queue
	QueueScanKey(k);
	
	// Set scan state
	m_KBState[k.m_ScanKey32 & 511] = k.m_ScanKey32;

	// Set mouse state
	if ((_ScanCode & 511) == SKEY_MOUSEMOVEREL)
	{
		m_MouseMove.x += _Data0;
		m_MousePos.x += _Data0;
		m_MouseMove.y += _Data1;
		m_MousePos.y += _Data1;
	}
	else if ((_ScanCode & 511) >= SKEY_MOUSE1 && (_ScanCode & 511) <= SKEY_MOUSE3)
	{
		int iButton = (_ScanCode & 511) - SKEY_MOUSE1;
		m_MouseButtons |= 1 << iButton;
		m_MouseButtonsAcc |= 1 << iButton;
	}

//ConOut(CStrF("DownKey %d, %d, %d", scancode, data0, data1));
};

void CInputContextCore::UpKey(int _ScanCode, fp8 _Time, int _Data0, int _Data1, int _iDevice)
{
	CScanKey k;
	k.m_ScanKey32 = m_KBState[_ScanCode & 511];
	k.m_ScanKey32 |= SKEY_UP;
	k.m_ScanKey32 &= ~SKEY_REPEAT;
	k.m_Time = _Time;
	k.m_Data[0] = _Data0;
	k.m_Data[1] = _Data1;
	k.m_iDevice = _iDevice;

	// If the scan was already 'up', set the repeat flag.
	if (m_KBState[k.m_ScanKey32 & 511] & SKEY_UP)
		k.m_ScanKey32 |= SKEY_REPEAT;

	// Add to queue
	QueueScanKey(k);

	// Set scan state
	m_KBState[_ScanCode & 511] = k.m_ScanKey32;

	// Set mousebutton state
	if ((_ScanCode & 511) >= SKEY_MOUSE1 && (_ScanCode & 511) <= SKEY_MOUSE3)
	{
		int iButton = (_ScanCode & 511) - SKEY_MOUSE1;
		m_MouseButtons &= ~(1 << iButton);
	}
};

bool CInputContextCore::KeyPressed()
{
//	Update();
	return (!m_spKBB->Empty());
};

CScanKey CInputContextCore::ScanKey()
{
	while (!KeyPressed()) Update();
	CScanKey k = m_spKBB->Get();
	return k;
};

bool CInputContextCore::IsPressed(int _skey)
{
	return !(m_KBState[_skey] & SKEY_UP);
}

bool CInputContextCore::IsNotPressed(int _skey)	
{ 
	return (m_KBState[_skey] & SKEY_UP) != 0;
}

bool CInputContextCore::IsPressed(int8 first, ... )
{
	va_list arg;
	va_start(arg,first);
	int8 code = first;
	while (code != 0)
	{
		if (m_KBState[code] & SKEY_UP) return FALSE;
#ifdef COMPILER_GNU
		code = (int8)va_arg(arg, int);
#else
		code = va_arg(arg, int8);
#endif
	};
	return TRUE;
};

bool CInputContextCore::IsNotPressed(int8 first, ... )
{
	va_list arg;
	va_start(arg,first);
	int8 code = first;
	while (code != 0)
	{
		if (!(m_KBState[code] & SKEY_UP)) return FALSE;
#ifdef COMPILER_GNU
		code = (int8)va_arg(arg, int);
#else
		code = va_arg(arg, int8);
#endif
	};
	return TRUE;
};

bool CInputContextCore::AddShiftKey(int8 scancode)
{
	if (m_nUserSK >= CKEYBOARD_MAXUSERSHIFTKEYS) return FALSE;
	m_UserSK[m_nUserSK++] = scancode;
	return TRUE;
};

bool CInputContextCore::RemoveShiftKey(int8 scancode)
{
	int32 cnt = 0,i;
	while ((m_UserSK[cnt] != scancode) && (cnt++<m_nUserSK))
	{
	}
	if (cnt == m_nUserSK) return FALSE;
	for (i = cnt; i < m_nUserSK-1; i++)
	{
		m_UserSK[i] = m_UserSK[i + 1];
	}
	m_nUserSK--;
	return TRUE;
};

// -------------------------------------------------------------------
bool CInputContextCore::IsGamepadActive(int _iPad)
{
	CMTime Time = CMTime::GetCPU();
	if( (Time - m_lLastTouchTime[_iPad]).GetTime() < 1.0f )
		return true;

	int iLatestTouchPad = -1;
	CMTime LatestTime;
	for( int i = 0; i < INPUT_MAXGAMEPADS; i++ )
	{
		if( m_lLastTouchTime[i].Compare(LatestTime) > 0)
		{
			LatestTime = m_lLastTouchTime[i];
			iLatestTouchPad	= i;
		}
	}

	return ( iLatestTouchPad == _iPad );
}

void CInputContextCore::SetGamepadMapping(int _iPadPhysical, int _iPadScanCode)
{
	if (_iPadPhysical < 0 || _iPadPhysical >= INPUT_MAXGAMEPADS)
		Error("SetGamepadMapping", CStrF("Invalid physical pad %d", _iPadPhysical));
	if (_iPadScanCode >= INPUT_MAXGAMEPADS)
		Error("SetGamepadMapping", CStrF("Invalid scancode pad %d", _iPadScanCode));

	m_lGamePadMappings[_iPadPhysical] = _iPadScanCode;
}

int CInputContextCore::GetGamepadMapping(int _iPadPhysical)
{
	if (_iPadPhysical < 0 || _iPadPhysical >= INPUT_MAXGAMEPADS)
		Error("SetGamepadMapping", CStrF("Invalid physical pad %d", _iPadPhysical));
	return m_lGamePadMappings[_iPadPhysical];
}

// -------------------------------------------------------------------
CPnt CInputContextCore::GetMouseMove()
{
	CPnt Move = m_MouseMove;
	m_MouseMove = CPnt(0, 0);
	return Move;
}

int CInputContextCore::GetMouseButtons()
{
	return m_MouseButtons;
}

int CInputContextCore::GetMouseButtonsAcc()
{
	int Ret = m_MouseButtonsAcc;
	if (m_MouseButtons != m_MouseButtonsAcc)
		ConOut(CStrF("ButtonAcc %d => %d", m_MouseButtonsAcc, m_MouseButtons));
	m_MouseButtonsAcc = m_MouseButtons;
	return Ret;
}

CPnt CInputContextCore::GetMousePosition()
{
	m_MousePos.Bound(m_MouseArea.p0, m_MouseArea.p1);
	return m_MousePos;
}

void CInputContextCore::SetMouseArea(CRct _Area)
{
	m_MouseArea = _Area;
	m_MousePos.Bound(m_MouseArea.p0, m_MouseArea.p1);
}

void CInputContextCore::SetMousePosition(CPnt &_P)
{
	m_MousePos = _P;
	m_MousePos.Bound(m_MouseArea.p0, m_MouseArea.p1);
}

void CInputContextCore::SetShowCursor(bool _Val)
{
	m_ShowCursor = _Val;
}

bool CInputContextCore::GetShowCursor()
{
	return m_ShowCursor;
}


// -------------------------------------------------------------------
void CInputContextCore::OnRefresh(int _Context)
{
	Update();
}

void CInputContextCore::OnBusy(int _Context)
{
	Update();
}

// -------------------------------------------------------------------
int CInputContextCore::GetPhysicalPad(int _iPadMapped)
{
	int iPadPhys = -1;

	for(int i = 0; i < INPUT_MAXGAMEPADS; i++)
		if (m_lGamePadMappings[i] == _iPadMapped && IsGamepadActive(i))
		{
			iPadPhys = i;
		}

	return iPadPhys;
}


spCInputEnvelopeInstance CInputContextCore::AppendEnvelope( int _index, const CStr &_name )
{
	int iPadPhys = GetPhysicalPad(_index);
	if (iPadPhys < 0)
		return NULL;

	CInputEnvelope *pEnvelope = m_Envelopes.FindEnvelope( _name );
	return m_EnvelopeInstances.AppendEnvelope( iPadPhys, pEnvelope );
}

void CInputContextCore::FlushEnvelopes( )
{
	m_EnvelopeInstances.FlushEnvelopes();
}

void CInputContextCore::FlushEnvelopes( int _index )
{
	int iPadPhys = GetPhysicalPad(_index);
	if (iPadPhys < 0)
		return;

	m_EnvelopeInstances.FlushEnvelopes( iPadPhys );
}

spCInputEnvelopeInstance CInputContextCore::SetEnvelope( int _index, const CStr &_name )
{
	int iPadPhys = GetPhysicalPad(_index);
	if (iPadPhys < 0)
		return NULL;

	CInputEnvelope *pEnvelope = m_Envelopes.FindEnvelope( _name );
	return m_EnvelopeInstances.SetEnvelope( iPadPhys, pEnvelope );
}

void CInputContextCore::RemoveEnvelope( int _index, CInputEnvelopeInstance *_pEnvelopeInstance )
{
	int iPadPhys = GetPhysicalPad(_index);
	if (iPadPhys < 0)
		return;

	m_EnvelopeInstances.RemoveEnvelope( iPadPhys, _pEnvelopeInstance );
}

// -------------------------------------------------------------------
void CInputContextCore::Register(CScriptRegisterContext & _RegContext)
{	
	_RegContext.RegFunction("in_setgamepadmapping", this, &CInputContextCore::SetGamepadMapping);
}

// -------------------------------------------------------------------
spCInputContext MCreateInputContext()
{
#ifdef PLATFORM_SHINOBI
	spCInputContext spIC = (CInputContext*) MRTC_GetObjectManager()->CreateObject("CInputContext_Shinobi");

#elif defined PLATFORM_XBOX1
	spCInputContext spIC = (CInputContext*) MRTC_GetObjectManager()->CreateObject("CInputContext_XTL");

#elif defined PLATFORM_XENON
	spCInputContext spIC = (CInputContext*) MRTC_GetObjectManager()->CreateObject("CInputContext_Xenon");

#elif defined PLATFORM_WIN
	spCInputContext spIC = (CInputContext*) MRTC_GetObjectManager()->CreateObject("CInputContext_DInput");

#elif defined PLATFORM_DOLPHIN
	spCInputContext spIC = (CInputContext*) MRTC_GetObjectManager()->CreateObject("CInputContext_Dolphin");
	
#elif defined PLATFORM_PS2
	spCInputContext spIC = (CInputContext*) MRTC_GetObjectManager()->CreateObject("CInputContext_PS2");
	
#elif defined PLATFORM_PS3
	spCInputContext spIC = (CInputContext*) MRTC_GetObjectManager()->CreateObject("CInputContext_PS3");

#else
	#error "Implement this"
	
#endif
	if(spIC != NULL)
		spIC->Create(NULL);

	return spIC;
}

