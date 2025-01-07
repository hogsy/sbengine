/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Core input context
					
	Author:			Jim Kjellin
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		030506:		Added comments
\*_____________________________________________________________________________________________*/

#ifndef _INC_MINPUTCORE
#define _INC_MINPUTCORE

#include "MInput.h"
#include "MInputEnvelope.h"

// -------------------------------------------------------------------
//  CInputContext
// -------------------------------------------------------------------

class SYSTEMDLLEXPORT CInputContextCore : public CInputContext
{
protected:
	static bool ms_bInputSysCreated;

	uint32 m_KBState[512];
	int8 m_LastKBState[512];
	int8 m_UserSK[CKEYBOARD_MAXUSERSHIFTKEYS];
	int32 m_nUserSK;

	TPtr< TQueue<CScanKey> > m_spKBB;

	CScanKey TmpDown;

	//
	fp4 m_FeedbackAmount;

	// Mouse status
	CRct m_MouseArea;
	CPnt m_MousePos;
	CPnt m_MouseMove;
	CVec2Dfp4 m_MouseSensitivity;

	int m_MouseWheel;
	fp4 m_WheelSensitivity;

	int m_MouseButtons;
	int m_MouseButtonsAcc;
	bool m_ShowCursor;
	bool m_bRemove;

	CMTime m_lLastTouchTime[INPUT_MAXGAMEPADS+1];
	int8 m_lGamePadMappings[INPUT_MAXGAMEPADS+1];

	int GetCurrentShiftKeyState();

	CInputEnvelopeList					m_Envelopes;
	CPlayerInputEnvelopeInstanceList	m_EnvelopeInstances;

	CMemCardCallback		*m_pMCCallback;
	CStr					 m_MCProductCode;

public:
	CInputContextCore();
	~CInputContextCore();

	virtual void Create(const char* _pParams);
	virtual void Update();

	// Input device interface
private:
	virtual bool QueueScanKey(CScanKey);
public:
	virtual void AddScanKey(CScanKey);
	virtual void DownKey(int _ScanCode, wchar _Char, fp8 _Time, int _nRep = 1, int _Data0 = 0, int _Data1 = 0, int _iDevice = 0);
	virtual void UpKey(int _ScanCode, fp8 _Time, int _Data0, int _Data1, int _iDevice = 0);

	virtual bool IsGamepadValid(int _iPad) { return true; }
	virtual bool IsGamepadActive(int _iPad);
	virtual void SetGamepadMapping(int _iPadPhysical, int _iPadScanCode);
	virtual int GetGamepadMapping(int _iPadPhysical);

public:
	// Scankey polling
	virtual bool KeyPressed();
	virtual CScanKey ScanKey();

	virtual bool IsPressed(int _skey);
	virtual bool IsNotPressed(int _skey);
	virtual bool IsPressed(int8 first, ... );		// Test state of multiple keys, terminate with 0
	virtual bool IsNotPressed(int8 first, ... );	// Test state of multiple keys, terminate with 0

	virtual bool AddShiftKey(int8 scancode);		// Add user shiftkeys, not implemented.
	virtual bool RemoveShiftKey(int8 scancode);

	// Mouse state polling
	virtual CPnt GetMouseMove();
	virtual CPnt GetMousePosition();
	virtual void SetMousePosition(CPnt &_P);
	virtual void SetShowCursor(bool _Val);
	virtual bool GetShowCursor();
	virtual int GetMouseButtons();
	virtual int GetMouseButtonsAcc();

	virtual void SetMouseArea(CRct _Area);

	// Overrides from CSubSystem
	virtual void OnRefresh(int _Context);
	virtual void OnBusy(int _Context);

	// Force feedback
public:
	virtual int GetPhysicalPad(int _iPadMapped);
	virtual void FlushEnvelopes(  );
	virtual void FlushEnvelopes( int _index );
	virtual void RemoveEnvelope( int _index, CInputEnvelopeInstance *_pEnvelopeInstance );
	virtual spCInputEnvelopeInstance AppendEnvelope( int _index, const CStr &_name );
	virtual spCInputEnvelopeInstance SetEnvelope( int _index, const CStr &_name );
	virtual void SetFeedbackAmount(fp4 _Amount) { m_FeedbackAmount = _Amount; }

	void Register(CScriptRegisterContext &_RegContext);

	// Memory Card interface
	virtual void ExecuteMemCardOp(CMemCardCallback *_pCallback)				{}
	virtual void SetMemCardProductCode(const CStr &_Code)					{ m_MCProductCode = _Code; }
	virtual int	 GetMaxMemCardNameLength()									{ return 0; }
	virtual bool IsMemCardIdle()											{ return false; }
	virtual void UpdateMemCard()											{}
	virtual void FlushMemCardIO()											{}
	virtual void AbortMemCardIO()											{}

	MACRO_OPERATOR_TPTR(CInputContext);
};

typedef TPtr<CInputContext> spCInputContext;

#endif // _INC_MINPUTCORE
