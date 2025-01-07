/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Main input context
					
	Author:			Jim Kjellin
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		030506:		Added comments
\*_____________________________________________________________________________________________*/

#ifndef _INC_MINPUT
#define _INC_MINPUT

#include "MCC.h"
#include "../Raster/MImage.h"
#include "../Misc/MConsole.h"

// forward declarations
class CInputEnvelopeInstance;
typedef TPtr<CInputEnvelopeInstance> spCInputEnvelopeInstance;


// -------------------------------------------------------------------
//  CInputContext
// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CInputContext : public CConsoleClient, public CSubSystem
{
public:
	virtual void Create(const char* _pParams) pure;
	virtual void Update() pure;

	// Input device interface
	virtual void AddScanKey(CScanKey) pure;
	virtual void DownKey(int _ScanCode, wchar _Char, fp8 _Time, int _nRep = 1, int _Data0 = 0, int _Data1 = 0, int _iDevice = 0) pure;
	virtual void UpKey(int _ScanCode, fp8 _Time, int _Data0, int _Data1, int _iDevice = 0) pure;

	// Scankey polling
	virtual bool KeyPressed() pure;
	virtual CScanKey ScanKey() pure;

	virtual bool IsPressed(int _skey) pure;
	virtual bool IsNotPressed(int _skey) pure;
	virtual bool IsPressed(int8 first, ... ) pure;		// Test state of multiple keys, terminate with 0
	virtual bool IsNotPressed(int8 first, ... ) pure;	// Test state of multiple keys, terminate with 0

	virtual bool AddShiftKey(int8 scancode) pure;		// Add user shiftkeys, not implemented.
	virtual bool RemoveShiftKey(int8 scancode) pure;

	virtual bool IsGamepadValid(int _iPad) pure;
	virtual bool IsGamepadActive(int _iPad) pure;
	virtual void SetGamepadMapping(int _iPadPhysical, int _iPadScanCode) pure;
	virtual int GetGamepadMapping(int _iPadPhysical) pure;

	// Mouse state polling
	virtual CPnt GetMouseMove() pure;
	virtual CPnt GetMousePosition() pure;
	virtual void SetMousePosition(CPnt &_P) pure;
	virtual void SetShowCursor(bool _Val) pure;
	virtual bool GetShowCursor() pure;
	virtual int GetMouseButtons() pure;
	virtual int GetMouseButtonsAcc() pure;

	virtual void SetMouseArea(CRct _Area) pure;

	// Feedback envelope interface
	virtual void SetFeedbackAmount(fp4 _Amount) pure;
	virtual void FlushEnvelopes(  ) pure;
	virtual void FlushEnvelopes( int _index ) pure;
	virtual void RemoveEnvelope( int _index, CInputEnvelopeInstance *_pEnvelopeInstance ) pure;
	virtual spCInputEnvelopeInstance AppendEnvelope( int _index, const CStr &_name ) pure;
	virtual spCInputEnvelopeInstance SetEnvelope( int _index, const CStr &_name ) pure;

	// Memory Card interface
	// Only one of these can be active at once. Arguments are specified in
	// the callback object that are passed to ExecuteMemCardOp.
	// Arguments listed below. Additional arguments can be specified on some
	// platforms (PS2 writes icon data when saving an entry for instance)
	enum EMemCardOp
	{
		eCheckMemCard,			// [in]m_iPort, [out]m_Size is space available
		eListMemCardEntries,	// [in]m_iPort, [in]m_Entry (Search string), [out]m_pNames, [in/out]m_MaxEntries = sizeof(CStr array), number of actual entries is stored when finished
		eDoesMemCardEntryExist,	// [in]m_iPort, [in]m_Entry (result is called back as an error, exist or does not exist)
		eSaveMemCardEntry,		// [in]m_iPort, [in]m_Entry, [in]m_pData, [in]m_Size
		eLoadMemCardEntry,		// [in]m_iPort, [in]m_Entry, [out]m_pData, [in]m_Size
		eDeleteMemCardEntry,	// [in]m_iPort, [in]m_Entry
		eFormatMemCard,			// [in]m_iPort
		ePollMemCard,			// [in]m_iPort (Continuously poll memory card until stopped or until an error is caught)
	};
	enum EMemCardError
	{
		eMCSuccessful   =   0, // Callback is called with this value when the operation is completed successfully
		eMCFailed       =   1, // Fatal error
		eMCNotDetected  =   2, // No memory card detected
		eMCUnformatted  =   4, // Memory card is unformatted
		eMCNoSpace      =   8, // Not enough space available
		eMCCardChanged  =  16, // Card changed since last access
		eMCDataExist    =  32, // Data about to be created already exist (overwrite?)
		eMCNoDataExist  =  64, // Data about to be created already exist (overwrite?)
		eMCWrongFormat  = 128, // Memory card is unformatted

		eMCCustom       = 256, // Add platform specific errors from here on (only power of two! so masking works...)
	};
	virtual void			 ExecuteMemCardOp(class CMemCardCallback *_pCallback) pure;
	virtual void			 SetMemCardProductCode(const CStr &_Code) pure;	// Code provided by publisher. It's used to create unique names for this product
	virtual int			 GetMaxMemCardNameLength() pure;				// Maximum number of characters in save names
	virtual bool			 IsMemCardIdle() pure;
	virtual void			 FlushMemCardIO() pure;
	virtual void			 AbortMemCardIO() pure;
	virtual void			 UpdateMemCard() pure;
	virtual void  SetIconDefinition(int _Icon, const CStr &_IconDefName) {}

	MACRO_OPERATOR_TPTR(CInputContext);
};

typedef TPtr<CInputContext> spCInputContext;

// -------------------------------------------------------------------

spCInputContext SYSTEMDLLEXPORT MCreateInputContext();

// -------------------------------------------------------------------
//  CMemCardCallback 
// -------------------------------------------------------------------
class CMemCardCallback
{
public:
	CMemCardCallback() :
		m_Operation(CInputContext::ePollMemCard),
		m_iPort(0),
		m_Entry(""),
		m_pData(0),
		m_Size(0),
		m_IgnoreErrors(0)
	{
	}
	CMemCardCallback(CInputContext::EMemCardOp _Operation, int _iPort, CStr _Entry = "", void *_pData = 0, int _Size = 0) :
		m_Operation(_Operation),
		m_iPort(_iPort),
		m_Entry(_Entry),
		m_pData(_pData),
		m_Size(_Size),
		m_IgnoreErrors(0),
		m_Icon(0)
	{
	}
	virtual ~CMemCardCallback()										{}

	enum EResponse
	{
		eAbort,
		eContinue,
	};
	
	// Ignoring an error is the same as returning eContinue from the Callback
	void							 IgnoreError(uint32 _Errors)	{ m_IgnoreErrors = _Errors; }
	bool							 Continue(uint32 _Error)		{ if(m_IgnoreErrors & _Error) return true; return Callback(_Error) == eContinue; }

	virtual EResponse				 Callback(uint32 _Error) pure;

	CInputContext::EMemCardOp		 GetOp() const							{ return m_Operation; }
	void							 SetOp(CInputContext::EMemCardOp _Op)	{ m_Operation = _Op; }

	int								 GetPort() const				{ return m_iPort; }
	void							 SetPort(int _iPort)			{ m_iPort = _iPort; }
	const CStr						&GetEntry() const				{ return m_Entry; }
	void							 SetEntry(const CStr &_Entry)	{ m_Entry = _Entry.Ansi(); }

	void							*GetData() const				{ return m_pData; }
	void							 SetData(void *_pData)			{ m_pData = _pData; }
	CStr							*GetNames() const				{ return m_pNames; }
	void							 SetNames(CStr *_pNames)		{ m_pNames = _pNames; }

	int								 GetMaxEntries() const			{ return m_MaxEntries; }
	void							 SetMaxEntries(int _nEntries)	{ m_MaxEntries = _nEntries; }
	int								 GetSize() const				{ return m_Size; }
	void							 SetSize(int _Size)				{ m_Size = _Size; }

	int								 GetIcon() const				{ return m_Icon; }
	void							 SetIcon(int _Icon)				{ m_Icon = _Icon; }

	const CStr						&GetSaveInfo()					{ return m_SaveInfo; }
	void							 SetSaveInfo(const CStr &_Info)	{ m_SaveInfo = _Info; }

protected:

	CInputContext::EMemCardOp		 m_Operation;
	// Arguments for the operation. Not all of these are necessary in all operations
	int								 m_iPort;
	CStr							 m_Entry;
	CStr							 m_SaveInfo;	// This is appended to the name shown in the memory card browser
	union
	{
		void						*m_pData;
		CStr						*m_pNames;
	};
	union
	{
		int							 m_MaxEntries;
		int							 m_Size;
	};
	
	int								 m_IgnoreErrors;
	int								 m_Icon;
};

// -------------------------------------------------------------------
#endif // _INC_MINPUT
