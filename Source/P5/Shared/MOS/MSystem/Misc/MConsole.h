#ifndef _INC_MOS_CON
#define _INC_MOS_CON

#include "../Script/MScript.h"
#include "../Raster/MImage.h"
#include "../Input/MInputScankey.h"

#define CONSOLE_WAIT_TIME_STAMP_OFFSET	(1.0f/20.0f)

#define CONSOLE_NUMBINDARGS	2	// If changed, some code need change too.

// -------------------------------------------------------------------
//  CStrEdit
// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CStrEdit : public CObj
{
	CStr str;
	int32 cursorpos;
	int32 cursorblink;

public:
	CStrEdit();
	~CStrEdit();
	void SetStr(const CStr& _str);
	void Clear();
	bool ProcessKey(const CScanKey& _Key);
	CStr GetStr();
	int32 GetCursorPos() { return cursorpos; };
};

// -------------------------------------------------------------------
//  CConsoleClient
// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CConsoleClient : public CScriptClient
{
public:
	virtual void AddToConsole();
	virtual void RemoveFromConsole();

//	virtual void DeclareInterface(CScriptContext& _pParser) {};
//	virtual void RemoveInterface(CScriptContext& _pParser) {};
};

// typedef TPtr<CConsoleClient> spCConsoleClient;

// -------------------------------------------------------------------
//  CConsole
// -------------------------------------------------------------------
#define CONSOLE_DEFAULTLINES 512
#define CONSOLE_HISTORYLEN 32

#define CONST_CONSOLE_AUTO 0
#define CONST_CONSOLE_INPUTKEY 1
#define CONST_CONSOLE_EXECUTEKEY 2

class CConsoleString : public CReferenceCount
{
public:
	CStr m_Str;
	CMTime m_Time;
	CConsoleString* m_pPrev;
	CConsoleString* m_pNext;

	void operator= (const CConsoleString& _s);
	CConsoleString();
	CConsoleString(CStr _s);
	CConsoleString(CStr _s, CMTime _Time);
};

class SYSTEMDLLEXPORT CConsole : public CConsoleClient
{
	friend class CXRealityApp;
	friend class CConsoleDlg;
	MRTC_DECLARE;

	CStr m_ObjectName;

	TArray<CConsoleString> m_lOutput;
	int m_Head;
	int m_Tail;
	int m_DisplayPos;
	
	int Wrap(int _Index) const;

	int m_MaxLines;
	bool m_bEnableConsole;

	CStrEdit m_CommandLineEdit;
	TArray<CConsoleClient*> m_lSubSystems;
	/* 
	(i)		Console clients* can't be sp's.
			Since CConsole is a client, CConsole would become undeletable since it's 
			SubSystemList would own CConsole and thus itself. => "owner-loop"
	*/

	TArray<CStr> m_lConHistory;
	int m_HistoryPos;
	int m_ConMode;

	TArray<CStr> m_lCommands;

	// ------------------------------------------

	class CKeyBindScript
	{
	public:
		CStr m_Program;
		TPtr<CScript> m_spScript;
		spCScriptExecutionContext m_spExecutionContext;
		fp64 m_Time;
		int32 m_Args[CONSOLE_NUMBINDARGS];

		CKeyBindScript();
		void Clear();
		void Create(CStr _s);
		void Compile(CScriptContext* _pParser);
		void Stop();
	};

	class CKeyBind : public CReferenceCount
	{
	public:
		CKeyBindScript m_BindDown;
		CKeyBindScript m_BindUp;
		CKeyBindScript m_BindRepeat;
		
		int m_ScanCode;
		int m_Flags;

		bool HasDownScript() { return (m_BindDown.m_Program != "") != 0; };
		bool HasUpScript() { return (m_BindUp.m_Program != "") != 0; };
		bool HasRepeatScript() { return (m_BindRepeat.m_Program != "") != 0; };

		CKeyBind();
		void Create(CStr _s, int _ScanCode);
		void Create(CStr _Down, CStr _Up, int _ScanCode);
		void Create(CStr _Down, CStr _Up, CStr _Repeat, int _ScanCode);
		void operator= (const CKeyBind& _kb);
		void CompileAll(CScriptContext* _pParser);
		void StopAll();
	};

	// ------------------------------------------
	class CScriptInstance
	{
	public:
		int m_Instance;
		TPtr<CScript> m_spScript;

		void operator= (const CScriptInstance& _Inst)
		{
			m_Instance = _Inst.m_Instance;
			m_spScript = _Inst.m_spScript;
		}
	};

	// ------------------------------------------
	TArray<CKeyBind> m_lKeyBind;
	TArray<CScriptInstance> m_lScriptInstances;

	fp64 m_CurrentTimeStamp;

	class CWriteCallback
	{
	public:
		void(*m_pfnWrite)(const CStr& _Write, void* _pContext);
		void* m_pContext;

		void Clear()
		{
			m_pfnWrite = NULL;
			m_pContext = NULL;
		}

		CWriteCallback()
		{
			Clear();
		}
	};

	TArray<CWriteCallback> m_lWriteCallbacks;
	int m_WriteCallbackRecurse;

	MRTC_CriticalSection m_ScriptLock;

	void AddToHistory(CStr _Str);

	CStr GetCommandSyntax(CStr Cmd);
	CStr ResolveSimpleSyntax(CStr _Script);
	TPtr<CScript> CompileScript(CStr _Script, bool _bAddMain, bool _bResolveSimpleSyntax, const char *_pSourceFile, int _LineOffset = 0, bool _ShowErrors = true);

	void ExecuteStringLooseSyntax(CStr _Str, const char *_pSourceFile = "CConsole::ExecuteString", int _LineOffset = 0);

	void StartBinding(CKeyBind* pBind, int _Type, fp64 _Time, CScanKey _ScanKey);
	void ExecuteBinding(CKeyBind* pBind, CKeyBindScript* pBindScript, int _Flags);
	void ExecuteBinding(CKeyBind* pBind);
	void StopBinding(CKeyBind* pBind);

	void KillScripts();

	CKeyBind* FindBinding(int _KeyID);

protected:
	void Parser_Echo(CStr _s);
	void Parser_Cls();
	void Parser_History();
	void Parser_ListCommands();
	void Parser_BindList();
	void Parser_BindListScan();
	void Parser_ClearBindList();
	void Parser_UnBind(CStr _Key);
	void Parser_Bind(CStr _Key, CStr _Down);
	void Parser_Bind2(CStr _Key, CStr _Down, CStr _Up);
	void Parser_Bind3(CStr _Key, CStr _Down, CStr _Up, CStr _Repeat);
	void Parser_BindRep(CStr _Key, CStr _Repeat);
	void Parser_BindRep2(CStr _Key, CStr _Repeat, CStr _Up);
	void Parser_BindScanCode(int _KeyID, CStr);
	void Parser_BindScanCode2(int _KeyID, CStr, CStr);
	void Parser_BindScanCode3(int _KeyID, CStr, CStr, CStr _Repeat);
	void Parser_ShowBind(CStr _Key);
	void Parser_ShowBindScanCode(int _KeyID);
	void Parser_GlobalArrayFlags(int);
public:
	void Parser_ExecuteScriptFile(CStr _FileName);

public:
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	MRTC_RemoteDebugChannel *m_pRDChannel;
	MRTC_RemoteDebugChannel *m_pRDChannelTrace;
#endif

	TPtr<CScriptContext> m_spParser;
	spCScriptClient m_spLanguageCore;

	CConsoleString* GetOutputHead();
	CConsoleString* GetOutputTail();
//	TArray<CStr>* GetOutput();
	CStrEdit* GetCommandLineEdit();


	DECLARE_OPERATOR_NEW

	CConsole();
	virtual void Create(int _MaxLines, CStr _ObjectName);
	~CConsole();
	void FreeTempAllocations();

	void Register(CScriptRegisterContext &_RegContext);

	void Write(const CStr& _Str);
	void WriteExceptions();

	fp64 GetBindingTimeStamp();				// -1 if invalid

	void ExecuteString(CStr _Str, const char *_pSourceFile = "CConsole::ExecuteString", int _LineOffset = 0);

	void ExecuteScripts();

	void ClearOutput();
	void ClearHistory();

	bool ProcessKey(const CScanKey& _Key, int _Mode = CONST_CONSOLE_AUTO);
	void SetMode(int _Mode) { m_ConMode = _Mode; };
	int GetMode() { return m_ConMode; }

	virtual void ParserStateChanged();
	void AddSubSystem(CConsoleClient* _pSubsys);
	void RemoveSubSystem(CConsoleClient* _pSubsys);

	void AddWriteCallback(void(*_pfnCallback)(const CStr& _Str, void* _pContext), void* _pContext);
	void RemoveWriteCallback(void(*_pfnCallback)(const CStr& _Str, void* _pContext), void* _pContext);

	friend class CConsoleRender;
};

typedef TPtr<CConsole> spCConsole;

// -------------------------------------------------------------------
//  CConsoleRender, den här kan man ärva om man vill rita snyggare än vad DebugText gör. :)
// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CConsoleRender : public CReferenceCount
{
protected:
//	TArray<CStr>* GetConsoleOutput(CConsole* pCon) { return &pCon->m_lOutput; }

public:
	virtual void Render(CConsole* pCon, CClipRect cr, CImage* img, CPnt pos);
	virtual void RenderRect(CConsole* pCon, CClipRect cr, CImage* img, CRct pos);
};

#endif // _INC_MOS_CON






