// -------------------------------------------------------------------
#include "PCH.h"

#include "MConsole.h"
#include "../MSystem.h"

// -------------------------------------------------------------------
//  CStrEdit
// -------------------------------------------------------------------
CStrEdit::CStrEdit()
{
	MAUTOSTRIP(CStrEdit_ctor, MAUTOSTRIP_VOID);
	cursorpos = 0;
	cursorblink = 0;
	str = "";
};

CStrEdit::~CStrEdit()
{
	MAUTOSTRIP(CStrEdit_dtor, MAUTOSTRIP_VOID);
	cursorpos = 0;
	cursorblink = 0;
	str = "";
};

void CStrEdit::SetStr(const CStr& _str)
{
	MAUTOSTRIP(CStrEdit_SetStr, MAUTOSTRIP_VOID);
	str = _str;
	cursorpos = str.Len();
};

void CStrEdit::Clear()
{
	MAUTOSTRIP(CStrEdit_Clear, MAUTOSTRIP_VOID);
	str = "";
	cursorpos = 0;
	cursorblink = 0;
};

bool CStrEdit::ProcessKey(const CScanKey& _Key)
{
	MAUTOSTRIP(CStrEdit_ProcessKey, false);
	if (_Key.IsASCII())
	{
		char c = _Key.GetASCII();
		char moo = '§';
		if(c != moo)
		{
			str = str.Insert(cursorpos, CStr(c));
			cursorpos++;
		}

//		str = str.insert(cursorpos, CStr::format("%.2X ",key.GetKey9()));
//		cursorpos += 3;
	}
	else
	{
		if (_Key.IsRepeat() || _Key.IsDown())
		{
			switch (_Key.GetKey9()) 
			{
			case SKEY_BACKSPACE : 
				{
					if (cursorpos > 0) str = str.Del(cursorpos-1, 1);
					cursorpos = Max((int)0, (int)(cursorpos-1));
					break;
				};
			case SKEY_DELETE : 
				{
					if (cursorpos < str.Len()) str = str.Del(cursorpos, 1);
					break;
				};
			case SKEY_CURSOR_LEFT : 
				{
					cursorpos = Max((int)0, (int)(cursorpos-1));
					break;
				};
			case SKEY_CURSOR_RIGHT : 
				{
					cursorpos = Min((int)str.Len(), (int)(cursorpos+1));
					break;
				};
			case SKEY_HOME : 
				{
					cursorpos = 0;
					break;
				};
			case SKEY_END : 
				{
					cursorpos = str.Len();
					break;
				};
			default : return FALSE;
			};
		}
	};
	return TRUE;
};

CStr CStrEdit::GetStr()
{
	MAUTOSTRIP(CStrEdit_GetStr, CStr());
	return str;
};

// -------------------------------------------------------------------
//  CConsoleClient
// -------------------------------------------------------------------
void CConsoleClient::AddToConsole()
{
	MAUTOSTRIP(CConsoleClient_AddToConsole, MAUTOSTRIP_VOID);
//	spCReferenceCount spObj = MRTC_GOM()->GetRegisteredObject("SYSTEM.CONSOLE");
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	if (!pCon) Error("AddToConsole", "No console available.");
	pCon->AddSubSystem(this);
}

void CConsoleClient::RemoveFromConsole()
{
	MAUTOSTRIP(CConsoleClient_RemoveFromConsole, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	if (!pCon) Error("RemoveFromConsole", "No console available.");
	pCon->RemoveSubSystem(this);
}

// -------------------------------------------------------------------
//  CConsoleString
// -------------------------------------------------------------------
void CConsoleString::operator= (const CConsoleString& _s)
{
	MAUTOSTRIP(CConsoleString_operator_assign, MAUTOSTRIP_VOID);
	m_Str = _s.m_Str;
	m_Time = _s.m_Time;
	m_pPrev = _s.m_pPrev;
	m_pNext = _s.m_pNext;
}

CConsoleString::CConsoleString()
{
	MAUTOSTRIP(CConsoleString_ctor, MAUTOSTRIP_VOID);
	m_pPrev = NULL;
	m_pNext = NULL;
	m_Time.Reset();
}

CConsoleString::CConsoleString(CStr _s)
{
	MAUTOSTRIP(CConsoleString_ctor_2, MAUTOSTRIP_VOID);
	m_Str = _s;
//	m_Time = CMTime::GetCPU();
	m_Time.Snapshot();
	m_pPrev = NULL;
	m_pNext = NULL;
}

CConsoleString::CConsoleString(CStr _s, CMTime _Time)
{
	MAUTOSTRIP(CConsoleString_ctor_3, MAUTOSTRIP_VOID);
	m_Str = _s;
	m_Time = _Time;
	m_pPrev = NULL;
	m_pNext = NULL;
}


// -------------------------------------------------------------------
//  CConsole::CKeyBindScript
// -------------------------------------------------------------------
CConsole::CKeyBindScript::CKeyBindScript()
{
	MAUTOSTRIP(CConsole_CKeyBindScript_ctor, MAUTOSTRIP_VOID);
	m_spExecutionContext = 0;
	m_Time = 0;
}

void CConsole::CKeyBindScript::Clear()
{
	MAUTOSTRIP(CConsole_CKeyBindScript_Clear, MAUTOSTRIP_VOID);
	m_Program = "";
	m_spExecutionContext = 0;
	m_Time = 0;
	m_spScript = NULL;
}

void CConsole::CKeyBindScript::Create(CStr _s)
{
	MAUTOSTRIP(CConsole_CKeyBindScript_Create, MAUTOSTRIP_VOID);
	m_Program = _s;
	m_spExecutionContext = 0;
	m_Time = 0;
	m_spScript = NULL;
}

void CConsole::CKeyBindScript::Compile(CScriptContext* _pParser)
{
	MAUTOSTRIP(CConsole_CKeyBindScript_Compile, MAUTOSTRIP_VOID);
	if (m_Program != "" && !m_spScript)
	{
		M_TRY
		{ 
			CStr Text = CStrF("void bindmain(int32 arg0, int32 arg1){%s; }", (char*)m_Program);
			m_spScript = _pParser->BuildScript(CStrF("NKeyBinding0x%08x", this), Text, "CConsole::CKeyBindScript::Compile"); 
			if (m_spScript)
				m_spExecutionContext = m_spScript->CreateExecutionContext();
		}
		M_CATCH(
		catch(CCException)
		{
			m_spScript = NULL;
			m_spExecutionContext = 0;
		};
		)
	}
}

void CConsole::CKeyBindScript::Stop()
{
	MAUTOSTRIP(CConsole_CKeyBindScript_Stop, MAUTOSTRIP_VOID);
	m_spScript = NULL;
	m_spExecutionContext = 0;
}

// -------------------------------------------------------------------
//  CConsole::CKeyBind
// -------------------------------------------------------------------
CConsole::CKeyBind::CKeyBind()
{
	MAUTOSTRIP(CConsole_CKeyBind_ctor, MAUTOSTRIP_VOID);
	m_ScanCode = 0;
	m_Flags = 0;
};

void CConsole::CKeyBind::Create(CStr _s, int _ScanCode)
{
	MAUTOSTRIP(CConsole_CKeyBind_Create, MAUTOSTRIP_VOID);
	m_BindDown.Create(_s);
	m_BindUp.Clear();
	m_BindRepeat.Clear();
	m_ScanCode = _ScanCode;
	m_Flags = 0;
}

void CConsole::CKeyBind::Create(CStr _Down, CStr _Up, int _ScanCode)
{
	MAUTOSTRIP(CConsole_CKeyBind_Create_2, MAUTOSTRIP_VOID);
	m_BindDown.Create(_Down);
	m_BindUp.Create(_Up);
	m_BindRepeat.Clear();
	m_ScanCode = _ScanCode;
	m_Flags = 0;
}

void CConsole::CKeyBind::Create(CStr _Down, CStr _Up, CStr _Repeat, int _ScanCode)
{
	MAUTOSTRIP(CConsole_CKeyBind_Create_3, MAUTOSTRIP_VOID);
	m_BindDown.Create(_Down);
	m_BindUp.Create(_Up);
	m_BindRepeat.Create(_Repeat);
	m_ScanCode = _ScanCode;
	m_Flags = 0;
}

void CConsole::CKeyBind::operator= (const CKeyBind& _kb)
{
	MAUTOSTRIP(CConsole_CKeyBind_operator_assign, MAUTOSTRIP_VOID);
	m_BindDown = _kb.m_BindDown;
	m_BindUp = _kb.m_BindUp;
	m_BindRepeat = _kb.m_BindRepeat;
	m_ScanCode = _kb.m_ScanCode;
	m_Flags = _kb.m_Flags;
};

void CConsole::CKeyBind::CompileAll(CScriptContext* _pParser)
{
	MAUTOSTRIP(CConsole_CKeyBind_CompileAll, MAUTOSTRIP_VOID);
	m_BindDown.Compile(_pParser);
	m_BindUp.Compile(_pParser);
	m_BindRepeat.Compile(_pParser);
}

void CConsole::CKeyBind::StopAll()
{
	MAUTOSTRIP(CConsole_CKeyBind_StopAll, MAUTOSTRIP_VOID);
	m_Flags = 0;
	m_BindDown.Stop();
	m_BindUp.Stop();
	m_BindRepeat.Stop();
}

// -------------------------------------------------------------------
//  CConsole
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CConsole, CConsoleClient);

/*
CConsole::CConsole()
{
	MAUTOSTRIP(CConsole_ctor, MAUTOSTRIP_VOID);
	MaxLines = CONSOLE_DEFAULTLINES;
	List.Clear();
	m_spParser = DNew(CScriptContext) CScriptContext;
	if (m_spParser == NULL) MemError("-");
	CBasicCommands bc;
	bc.Register(*m_spParser);		// Add some basic C-like commands.
	AddSubSystem(this);
};
*/

CConsoleString* CConsole::GetOutputHead()
{
	MAUTOSTRIP(CConsole_GetOutputHead, NULL);
	if (m_Head == m_Tail) return NULL;
	return &m_lOutput[Wrap(m_DisplayPos-1)];
}

CConsoleString* CConsole::GetOutputTail()
{
	MAUTOSTRIP(CConsole_GetOutputTail, NULL);
	if (m_Head == m_Tail) return NULL;
	return &m_lOutput[m_Tail];
}

CStrEdit* CConsole::GetCommandLineEdit()
{
	MAUTOSTRIP(CConsole_GetCommandLineEdit, NULL);
	return &m_CommandLineEdit;
}



IMPLEMENT_OPERATOR_NEW(CConsole);


CConsole::CConsole()
{
	MAUTOSTRIP(CConsole_ctor_2, MAUTOSTRIP_VOID);
	m_ConMode = CONST_CONSOLE_EXECUTEKEY;

	m_Head = 0;
	m_Tail = 0;
	m_DisplayPos = 0;
	m_HistoryPos = 0;
	m_CurrentTimeStamp = -1;
	m_WriteCallbackRecurse = 0;
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	m_pRDChannel = MRTC_GetRD()->CreateDebugChannel(101);
#endif
}

void CConsole::Create(int _MaxLines, CStr _ObjectName)
{
	MAUTOSTRIP(CConsole_Create, MAUTOSTRIP_VOID);
	m_ConMode = CONST_CONSOLE_EXECUTEKEY;
	m_lKeyBind.Clear();
//	m_lKeyBind.SetLen(256);
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");

	m_HistoryPos = 0;
	m_Head = 0;
	m_Tail = 0;
	m_lOutput.SetLen(_MaxLines);
//	m_MaxLines = _MaxLines;
	m_spParser = MNew(CScriptContext);
	if (m_spParser == NULL) MemError("Create");

	// Add some basic C-like commands.
	m_spLanguageCore = (CScriptClient *)(CReferenceCount *)MRTC_GetObjectManager()->CreateObject("CScriptLanguageCore");
	if (m_spLanguageCore == NULL) MemError("Create");
	m_spLanguageCore->RegisterToParser(m_spParser);

	AddSubSystem(this);
	//Parser_BindScanCode(SKEY_ESC, CStr("quit"));

	CRegistry* pReg = (pSys) ? pSys->GetEnvironment() : NULL;

	if(pReg)
	{
		m_bEnableConsole = pReg->GetValuei("CON_ENABLE", 1) != 0;
		int i = 0;
		while(pReg->Find(CStrF("CON_HISTORY%d", i)))
		{
			m_lConHistory.Add(pReg->GetValue(CStrF("CON_HISTORY%d", i), ""));
			i++;
		}
	}

//	MRTC_GOM()->RegisterObject(this, _ObjectName);
};

CConsole::~CConsole()
{
	MAUTOSTRIP(CConsole_dtor, MAUTOSTRIP_VOID);
//		MRTC_GOM()->UnregisterObject(this, "SYSTEM.CONSOLE");

	M_TRY
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		CRegistry* pReg = (pSys) ? pSys->GetEnvironment() : NULL;
		if(pReg)
		{
			int nSave = Min(10, m_lConHistory.Len());
			int i = 0;
			for(int Pos = m_HistoryPos; nSave; nSave--)
			{
				pReg->SetValue(CStrF("CON_HISTORY%d", i++), m_lConHistory[Pos]);
				Pos = (Pos+1) % m_lConHistory.Len();
			}
		}

	}
	M_CATCH(
	catch(CCException)
	{
		// Ignore.. it's pretty unimportant this.
	}
	)

	m_spLanguageCore->UnRegister(m_spParser);

	RemoveSubSystem(this);
	m_MaxLines = 0;
	m_lOutput.Clear();
};

void CConsole::FreeTempAllocations()
{
/*	for(int i = 0; i < m_lKeyBind.Len(); i++)
	{
		CKeyBind *pBind = &m_lKeyBind[i];
		if(pBind->m_BindDown.m_spScript)
			pBind->m_BindDown.m_spScript->FreeTempAllocations();
		if(pBind->m_BindUp.m_spScript)
			pBind->m_BindUp.m_spScript->FreeTempAllocations();
		if(pBind->m_BindRepeat.m_spScript)
			pBind->m_BindRepeat.m_spScript->FreeTempAllocations();
	}
*/
}

int CConsole::Wrap(int _Index) const
{
	MAUTOSTRIP(CConsole_Wrap, 0);
	if (_Index >= m_lOutput.Len())
		return _Index - m_lOutput.Len();
	else if (_Index < 0)
		return _Index + m_lOutput.Len();
	else
		return _Index;
}


void CConsole::Write(const CStr& str)
{
	MAUTOSTRIP(CConsole_Write, MAUTOSTRIP_VOID);
	M_LOCK(*(MRTC_GOM()->m_pGlobalLock));
	MSCOPE(CConsole::Write, SYSTEMCONSOLE);
	MSCOPE_DISABLE;
	
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	{
		mint Size = str.Len();
		m_pRDChannel->SendPacket(str.Str(), Size);
	}
#endif

#if !defined(M_RTM) || defined(PLATFORM_WIN_PC)
//	while (m_lOutput.Len() >= m_MaxLines) m_lOutput.Del(0);
	if (m_lOutput.Len() - ((m_lOutput.Len() + m_Head - m_Tail) % m_lOutput.Len()) < 1)
	{
		if (m_lOutput[m_Tail].m_pNext)
			m_lOutput[m_Tail].m_pNext->m_pPrev = NULL;
		m_lOutput[m_Tail].m_pNext = NULL;
		m_lOutput[m_Tail].m_pPrev = NULL;
		m_Tail++;
		m_Tail %= m_lOutput.Len();
	}

	CStr s;
	s.Capture(str.StrAny(), str.Len(), str.GetFmt());

	m_lOutput[m_Head].m_Str = s;
//	m_lOutput[m_Head].m_Time = CMTime::GetCPU();
	m_lOutput[m_Head].m_Time.Snapshot();
	if (m_Head != m_Tail)
	{
		m_lOutput[Wrap(m_Head-1)].m_pNext = &m_lOutput[m_Head];
		m_lOutput[m_Head].m_pPrev = &m_lOutput[Wrap(m_Head-1)];
		m_lOutput[m_Head].m_pNext = NULL;
	}

	m_Head++;
	m_DisplayPos++;
	m_Head %= m_lOutput.Len();
	m_DisplayPos %= m_lOutput.Len();

	if (!m_WriteCallbackRecurse)
	{
		m_WriteCallbackRecurse++;
		M_TRY
		{
			for(int i = 0; i < m_lWriteCallbacks.Len(); i++)
				if (m_lWriteCallbacks[i].m_pfnWrite != NULL)
					m_lWriteCallbacks[i].m_pfnWrite(str, m_lWriteCallbacks[i].m_pContext);
		}
		M_CATCH(
		catch(CCException)
		{
			m_WriteCallbackRecurse--;
			throw;
		}
		)
		m_WriteCallbackRecurse--;
	}

#endif
};


void CConsole::WriteExceptions()
{
	MAUTOSTRIP(CConsole_WriteExceptions, MAUTOSTRIP_VOID);
	M_LOCK(*(MRTC_GOM()->m_pGlobalLock));	
	MSCOPE(CConsole::WriteExceptions, SYSTEMCONSOLE);

	MACRO_GetRegisterObject(CCExceptionLog, pLog, "SYSTEM.EXCEPTIONLOG");
	if (pLog)
	{
		while(pLog->ExceptionAvail())
			Write("§cf00ERROR: " + pLog->GetExceptionInfo().GetString());
	}

//	while (CCException::ErrorAvail()) Write(CCException::ErrorMsg());
};

fp8 CConsole::GetBindingTimeStamp()
{
	MAUTOSTRIP(CConsole_GetBindingTimeStamp, 0.0);
	return m_CurrentTimeStamp;
}

CStr CConsole::GetCommandSyntax(CStr _Cmd)
{
	MAUTOSTRIP(CConsole_GetCommandSyntax, CStr());
	if (!m_lCommands.Len())
	{
		m_lCommands = m_spParser->GetRootNameSpace()->GetSymbols();
	}

	int CmdLen = _Cmd.Len();
	for(int i = 0; i < m_lCommands.Len(); i++)
	{
		CStr& Spec = m_lCommands[i];
		int Comma = Spec.Find(",");
		if (Comma >= 0 && Comma <= CmdLen)
		{
			if (strncmp((char*)Spec, (char*)_Cmd, Comma) == 0)
			{
				// Disqualify if command is complex.
				if (Spec.FindOneOf("(){}[];") >= 0) return "";

				// Nope.. return the syntax.
				CFStr Syntax = Spec.CopyFrom(Comma*2+1+5+5).GetStr();
				Syntax.Trim();
				return CStr((CStrBase&)Syntax);
			}
		}
	}
	return "";
}

CStr CConsole::ResolveSimpleSyntax(CStr _Script)
{
	MAUTOSTRIP(CConsole_ResolveSimpleSyntax, CStr());
	_Script.Trim();
	const char *pStr = _Script;
	CStr NewScript;
	while (*pStr)
	{
		NScript::ParseWhiteSpace(pStr);
		
		//Make sure we're not encountering a comma here...
		while( (*pStr) == ',' )
			++pStr;

		const char *pStrStart = pStr;
		while (*pStr && (*pStr) != ' ' && (*pStr) != ',')
			++pStr; // Parse for space or ,
		CStr Ident;
		Ident.Capture(pStrStart, pStr - pStrStart);

		if (Ident != "")
		{
			NScript::CSymbol *pSymbol = m_spParser->GetRootNameSpace()->GetSymbol(Ident, true);
			if (pSymbol && pSymbol->m_ClassType & NScript::ESymbol_Function)
			{
				NewScript += Ident + "(";

				// A function lets parse it's parameters
				NScript::CSymbol_Function *pFunction = (NScript::CSymbol_Function *)pSymbol;
				int nFunc = pFunction->m_splArgumentTypes.Len();
				for (int i = 0; i < nFunc; ++i)
				{
					if (!*pStr)
					{
						break;
					}
					NScript::ParseWhiteSpace(pStr);
					if (*pStr == ',')
						++pStr;
					NScript::ParseWhiteSpace(pStr);
					const char *pStrStart = pStr;
					if (*pStrStart == '"')
					{
						// Lets parse as a string
						++pStr;
						while (*pStr && (*pStr) != '"')
							++pStr;

						if ((*pStr) == '"')
							++pStr;

						CStr Ident;
						Ident.Capture(pStrStart, pStr - pStrStart);

						if (i)
							NewScript += "," + Ident;
						else
							NewScript += Ident;
					}
					else
					{
						if (NStr::CharIsNumber(*pStr) || (*pStr == '-' && NStr::CharIsNumber(*(pStr+1))))
						{
							while (*pStr && (*pStr) != ' ' && (*pStr) != ',')
								++pStr; // Parse for space or ,

							CStr Ident;
							Ident.Capture(pStrStart, pStr - pStrStart);

							if (i)
								NewScript += "," + Ident;
							else
								NewScript += Ident;
						}
						else
						{
							while (*pStr && (*pStr) != ' ' && (*pStr) != ',')
								++pStr; // Parse for space or ,

							CStr Ident;
							Ident.Capture(pStrStart, pStr - pStrStart);

							NScript::CSymbol *pSymbol = m_spParser->GetRootNameSpace()->GetSymbol(Ident, true);

							if (pSymbol)
							{
								// Symbol found lets just assume we want the var
							}
							else
							{
								// No symbol found lets just assume we want a string
								if (i)
									NewScript += ",\"" + Ident + "\"";
								else
									NewScript += "\"" + Ident + "\"";
							}
						}
					}
				}
				NewScript += ");";
			}
			else
			{
				NewScript += Ident + " ";
			}
		}
	}

	return NewScript;
}

TPtr<CScript> CConsole::CompileScript(CStr _Script, bool _bAddMain, bool _bResolveSimpleSyntax, const char *_pSourceFile, int _LineOffset, bool _ShowErrors)
{
	MAUTOSTRIP(CConsole_CompileScript, NULL);
	CStr Script;
	int Offset = 0;
	if (_bAddMain)
	{
		Offset = 12;
		Script = CStrF("void main(){%s; }", (char*)_Script);
	}
	else
		Script = _Script;

	TPtr<CScript> spScript = m_spParser->BuildScript(CStrF("NConsoleTemp%x", _Script.Str()), Script, _pSourceFile, Offset, _LineOffset, _ShowErrors && !_bResolveSimpleSyntax); 
	if (spScript)
		return spScript;
	else if (_bResolveSimpleSyntax)
		return CompileScript(ResolveSimpleSyntax(_Script), _bAddMain, false, _pSourceFile, _LineOffset, _ShowErrors);
	return NULL;
}

void CConsole::StartBinding(CKeyBind* pBind, int _Type, fp8 _Time, CScanKey _ScanKey)
{
	MAUTOSTRIP(CConsole_StartBinding, MAUTOSTRIP_VOID);
	MSCOPE(CConsole::StartBinding, SYSTEMCONSOLE);
	pBind->CompileAll(m_spParser);

	if (_Type == 4)
	{
		if (pBind->m_BindRepeat.m_spExecutionContext && !(pBind->m_Flags & 4))
		{
			pBind->m_Flags |= 4;
			pBind->m_BindRepeat.m_Args[0] = _ScanKey.m_Data[0];
			pBind->m_BindRepeat.m_Args[1] = _ScanKey.m_Data[1];
			pBind->m_BindRepeat.m_Time = _Time;
		}
	}
	else if (_Type == 2)
	{
		if (pBind->m_BindUp.m_spExecutionContext && !(pBind->m_Flags & 2))
		{
			pBind->m_Flags |= 2;
			pBind->m_BindUp.m_Args[0] = _ScanKey.m_Data[0];
			pBind->m_BindUp.m_Args[1] = _ScanKey.m_Data[1];
			pBind->m_BindUp.m_Time = _Time;
		}
	}
	else if (_Type == 1)
	{
		if (pBind->m_BindDown.m_spExecutionContext && !(pBind->m_Flags & 1))
		{
			pBind->m_Flags |= 1;
			pBind->m_BindDown.m_Args[0] = _ScanKey.m_Data[0];
			pBind->m_BindDown.m_Args[1] = _ScanKey.m_Data[1];
			pBind->m_BindDown.m_Time = _Time;
		}
	}
}

void CConsole::ExecuteBinding(CKeyBind* pBind, CKeyBindScript* pBindScript, int _Flags)
{
	MAUTOSTRIP(CConsole_ExecuteBinding, MAUTOSTRIP_VOID);
	MSCOPE(CConsole::ExecuteBinding, SYSTEMCONSOLE);
	m_CurrentTimeStamp = pBindScript->m_Time;
	pBindScript->m_Time += CONSOLE_WAIT_TIME_STAMP_OFFSET;

	M_TRY
	{
		TPtr<CScript> spScript = pBindScript->m_spScript;

		int32 *Args[CONSOLE_NUMBINDARGS];
		for (int i = 0; i < CONSOLE_NUMBINDARGS; ++i)
		{
			Args[i] = &(pBindScript->m_Args[i]);
		}
		if (pBindScript->m_spExecutionContext->Execute("bindmain", (void **)Args, CONSOLE_NUMBINDARGS, NULL))
			pBind->m_Flags &= ~_Flags;
	}
	M_CATCH(
	catch(CCException)
	{
		ConOut(CStrF("§cf80WARNING: Binding (%d) for scan %d raised an exception.", _Flags, pBind->m_ScanCode));
		pBindScript->m_spScript = NULL;
		pBind->m_Flags &= ~_Flags;
	}
	)

	m_CurrentTimeStamp = -1;
}

void CConsole::ExecuteBinding(CKeyBind* _pBind)
{
	MAUTOSTRIP(CConsole_ExecuteBinding_2, MAUTOSTRIP_VOID);
	if (!_pBind->m_Flags) return;
	_pBind->CompileAll(m_spParser);

	if (_pBind->m_Flags & 1)
		ExecuteBinding(_pBind, &_pBind->m_BindDown, 1);
	if (_pBind->m_Flags & 2)
		ExecuteBinding(_pBind, &_pBind->m_BindUp, 2);
	if (_pBind->m_Flags & 4)
		ExecuteBinding(_pBind, &_pBind->m_BindRepeat, 4);
}

void CConsole::StopBinding(CKeyBind* pBind)
{
	MAUTOSTRIP(CConsole_StopBinding, MAUTOSTRIP_VOID);
	pBind->StopAll();
}


void CConsole::ExecuteScripts()
{
	MAUTOSTRIP(CConsole_ExecuteScripts, MAUTOSTRIP_VOID);
	MSCOPESHORT(CConsole::ExecuteScripts);
	M_LOCK(m_ScriptLock);
	for(int i = 0; i < m_lKeyBind.Len(); i++)
		ExecuteBinding(&m_lKeyBind[i]);
}

void CConsole::KillScripts()
{
	MAUTOSTRIP(CConsole_KillScripts, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lKeyBind.Len(); i++)
		StopBinding(&m_lKeyBind[i]);
}

void CConsole::ClearOutput()
{
	for(int i = 0; i < m_lOutput.Len(); i++)
	{
		m_lOutput[i].m_pPrev = NULL;
		m_lOutput[i].m_pNext = NULL;
		m_lOutput[i].m_Time.Reset();
		m_lOutput[i].m_Str = "";
	}
}

void CConsole::ClearHistory()
{
	m_lConHistory.Clear();
}

void CConsole::ExecuteStringLooseSyntax(CStr _Str, const char *_pSourceFile, int _LineOffset)
{
	MAUTOSTRIP(CConsole_ExecuteString, MAUTOSTRIP_VOID);
	MSCOPESHORT(CConsole::ExecuteString); //AR-SCOPE

	if (_Str.Len() == 0) 
		return;
	M_LOCK(m_ScriptLock);
	TPtr<CScript> spScript = CompileScript(_Str, true, true, _pSourceFile, _LineOffset);
	if (spScript != NULL) 
		spScript->CreateExecutionContext()->Execute();
}

void CConsole::ExecuteString(CStr _Str, const char *_pSourceFile, int _LineOffset)
{
	MAUTOSTRIP(CConsole_ExecuteString, MAUTOSTRIP_VOID);
	MSCOPESHORT(CConsole::ExecuteString); //AR-SCOPE

	if (_Str.Len() == 0) 
		return;
	M_LOCK(m_ScriptLock);
	TPtr<CScript> spScript = CompileScript(_Str, true, false, _pSourceFile, _LineOffset);
	if (spScript != NULL) 
		spScript->CreateExecutionContext()->Execute();
};

void CConsole::AddToHistory(CStr line)
{
	MAUTOSTRIP(CConsole_AddToHistory, MAUTOSTRIP_VOID);
	for(int iH = 0; iH < m_lConHistory.Len(); iH++)
		if (m_lConHistory[iH] == line)
		{
			m_lConHistory.Del(iH);
			m_lConHistory.Insert(0, line);
			m_HistoryPos = 0;
			return;
		}

	while (m_lConHistory.Len() >= CONSOLE_HISTORYLEN) m_lConHistory.Del(0);
	m_lConHistory.Insert(0, line);
	m_HistoryPos = 0;
};

struct S {template <typename T> void f() {} }; template <> inline void S::f<int>() {}

//struct S { template <typename T> void f() {} template <> void S::f<int>() {} };

bool CConsole::ProcessKey(const CScanKey& key, int _Mode)
{
	MAUTOSTRIP(CConsole_ProcessKey, false);
	M_LOCK(m_ScriptLock);
	if (_Mode == CONST_CONSOLE_AUTO) _Mode = m_ConMode;

	if (_Mode == CONST_CONSOLE_EXECUTEKEY)
	{
		// Check for §.
/*		if (m_bEnableConsole)
		{
			if (m_ConMode == CONST_CONSOLE_INPUTKEY && key.GetKey16() == (SKEY_PARAGRAPH))
			{
				m_ConMode = CONST_CONSOLE_EXECUTEKEY;
				return TRUE;
			}
			else if ((m_ConMode == CONST_CONSOLE_EXECUTEKEY) && key.GetKey16() == (SKEY_PARAGRAPH + SKEY_UP)) 
			{
				m_ConMode = CONST_CONSOLE_INPUTKEY;
				return TRUE;
			}
		}
*/
		// Execute!
		{
			// Look for exact match first
			CKeyBind* pBind = FindBinding(key.GetKey16() & 0x3fff);

			// Not found, look for a non-shift key binding.
			if (!pBind && ((key.GetKey16() & 0x3e00) != 0))
			{
//				ConOut(CStrF("Looked for %.4x", key.GetKey16()));
				pBind = FindBinding(key.GetKey16() & 0x01ff);
			}

			if (pBind)
			{
				// Was the key repeated, released or pressed?
				if (pBind->HasRepeatScript())
					StartBinding(pBind, 4, key.m_Time, key);
				
				if (key.IsRepeat())
				{
				}
				else if (key.IsDown())
				{
					if (pBind->HasDownScript())
						StartBinding(pBind, 1, key.m_Time, key);
				}
				else
					if (pBind->HasUpScript())
						StartBinding(pBind, 2, key.m_Time, key);
				//ExecuteString(pBind->m_ProgramUp);

				ExecuteBinding(pBind);
			}

			// Look for exact match first
/*			CKeyBind* pBind = FindBinding(key.GetKey16() & 0x3fff);
			CKeyBind* pBind2 = FindBinding(key.GetKey9());

			// Not found, look for a non-shift key binding.
			if (pBind)
			{
//				M_TRACEALWAYS("pBind %s\n", pBind->m_BindDown.m_Program.Str());

				// Was the key repeated, released or pressed?
				if (pBind->HasRepeatScript()) StartBinding(pBind, 4, key.m_Time, key);

				if (key.IsRepeat())
				{
				}
				else if (key.IsDown())
				{
					if (pBind->HasDownScript()) StartBinding(pBind, 1, key.m_Time, key);
				}
				else
					if (pBind->HasUpScript()) StartBinding(pBind, 2, key.m_Time, key);
					//ExecuteString(pBind->m_ProgramUp);

				ExecuteBinding(pBind);
			}

			// Also execute any nonkey binding

			// Not found, look for a non-shift key binding.
			if (pBind != pBind2 && pBind2)
			{
				pBind = pBind2;
//				M_TRACEALWAYS("pBind2 %s\n", pBind->m_BindDown.m_Program.Str());
				// Was the key repeated, released or pressed?
				if (pBind->HasRepeatScript()) StartBinding(pBind, 4, key.m_Time, key);

				if (key.IsRepeat())
				{
				}
				else if (key.IsDown())
				{
					if (pBind->HasDownScript()) StartBinding(pBind, 1, key.m_Time, key);
				}
				else
					if (pBind->HasUpScript()) StartBinding(pBind, 2, key.m_Time, key);
					//ExecuteString(pBind->m_ProgramUp);

				ExecuteBinding(pBind);
			}*/
		}

/*		int KeyID = key.GetKey16();
		int nBind = m_lKeyBind.Len();
		for(int i = 0; i < nBind; i++)
			if (m_lKeyBind[i].m_ScanCode == KeyID)
			{
				ExecuteString(m_lKeyBind[i].m_Program);
				return TRUE;
				break;
			}*/

//		if ((_KeyID < 0) || (_KeyID >= m_lKeyBind.Len())) Error("ProcessKey", "Invalid Key.");
//		ExecuteString(m_lKeyBind[_KeyID].ProgramStr);
		return FALSE;
	}
	else
	{
//ConOut(CStrF("Key %.8x, %d", key.m_ScanKey32, key.m_Char));
		if (!m_CommandLineEdit.ProcessKey(key)) 
		{
			switch (key.GetKey16()) 
			{
			case SKEY_ESC :
				m_CommandLineEdit.SetStr(CStr(""));
				break;

			case SKEY_TAB : 
				{
					TList_Vector<CStr> lCmd;
					lCmd = m_spParser->GetRootNameSpace()->GetSymbols();

					CStr Cmd = m_CommandLineEdit.GetStr();
					Cmd.Trim();

					int nFound = 0;
					int iFirst = -1;
					int MinEqualLength = 65535;
					CStr CurrentEqual;
					for(int i = 0; i < lCmd.Len(); i++)
					{
						if (!Cmd.Len() || lCmd[i].CompareSubStr(Cmd) == 0)
						{
							if (nFound)
							{
								if (nFound == 1) ConOut(lCmd[iFirst]);
								ConOut(lCmd[i]);
								nFound++;
							}
							else
							{
								CurrentEqual = lCmd[i];
								CurrentEqual.SetChar(CurrentEqual.Find(","), 0);

								MinEqualLength = CurrentEqual.Len();

								iFirst = i;
								nFound++;
							}

							CStr CompareStr = lCmd[i];
							CompareStr.SetChar(CompareStr.Find(","), 0);

							for (int j = 0; j < CurrentEqual.Len(); ++j)
							{
								if (CurrentEqual.Len() <= j || CompareStr.Len() <= j)
								{
									if (j < MinEqualLength)
										MinEqualLength = j;									

									break;
								}

								if (CompareStr[j] != CurrentEqual[j])
								{
									if (j < MinEqualLength)
										MinEqualLength = j;
									break;
								}
							}
							
						}
					}


					if (nFound == 1)
						m_CommandLineEdit.SetStr(lCmd[iFirst].GetStrSep(",") + " ");
					else
					{
						if (nFound && MinEqualLength > Cmd.Len())
						{

							char TempCap[4096];
							strncpy(TempCap, CurrentEqual,MinEqualLength);
							TempCap[MinEqualLength] = 0;
							m_CommandLineEdit.SetStr(CStrF("%s", TempCap));
						}
					}

						

/*					for(int i = 0; i < lCmd.Len(); i++)
						if (lCmd[i].CompareSubStr(Cmd) == 0)
						{
							CStr s = lCmd[i].GetStrSep(",");
							if ((s == Cmd) && (i < lCmd.Len()-1))
								s = lCmd[i+1].GetStrSep(",");

							s.Trim();
							m_CommandLineEdit.SetStr(s + " ");
							break;
						}*/
				}
				break;
			case SKEY_TAB + SKEY_MODIFIER_SHIFT: 
				{
					TList_Vector<CStr> lCmd;
					lCmd = m_spParser->GetRootNameSpace()->GetSymbols();

					CStr Cmd = m_CommandLineEdit.GetStr();
					Cmd.Trim();
					for(int i = 0; i < lCmd.Len(); i++)
						if (lCmd[i].CompareSubStr(Cmd) == 0)
						{
							CStr s = lCmd[i].GetStrSep(",");
							if ((s == Cmd) && (i > 0))
							{
								s = lCmd[i-1].GetStrSep(",");
								s.Trim();
								m_CommandLineEdit.SetStr(s + " ");
								break;
							}
						}
				}
				break;
			case SKEY_PARAGRAPH :
				{
					if (m_bEnableConsole)
					{
						if (m_ConMode == CONST_CONSOLE_INPUTKEY) 
							m_ConMode = CONST_CONSOLE_EXECUTEKEY;
					}
						else
					{
						return false;
					}
					break;
				};
			case SKEY_PARAGRAPH + SKEY_UP:
				{
					if (m_bEnableConsole)
					{
						if (m_ConMode == CONST_CONSOLE_EXECUTEKEY) 
							m_ConMode = CONST_CONSOLE_INPUTKEY;
					}
					else
					{
						return false;
					}
					break;
				};
			case SKEY_RETURN : 
				{
					AddToHistory(m_CommandLineEdit.GetStr());
					CStr Exe = m_CommandLineEdit.GetStr();
					m_CommandLineEdit.Clear();
					ExecuteStringLooseSyntax(Exe);
					break;
				};
			case SKEY_CURSOR_UP :
				{
					if (m_lConHistory.Len())
					{
						m_CommandLineEdit.SetStr(m_lConHistory[m_HistoryPos]);
						m_HistoryPos = (m_HistoryPos + 1) % m_lConHistory.Len();
					}
					break;
				};
			case SKEY_CURSOR_DOWN :
				{
					if (m_lConHistory.Len())
					{
						m_HistoryPos = (m_HistoryPos + m_lConHistory.Len() - 1) % m_lConHistory.Len();
						m_CommandLineEdit.SetStr(m_lConHistory[m_HistoryPos]);
					}
					break;
				};
			case SKEY_PAGEUP :
				{
					int i = 10;
					while(i && (m_DisplayPos != m_Tail))
					{
						m_DisplayPos = Wrap(m_DisplayPos-1);
						i--;
					}
					break;
				};
			case SKEY_PAGEDOWN :
				{
					int i = 10;
					while(i && (m_DisplayPos != m_Head))
					{
						m_DisplayPos = Wrap(m_DisplayPos+1);
						i--;
					}
					break;
				};
			default : return FALSE;
			};
			return TRUE;
		}
		else 
		{
	//		Write(m_CommandLineEdit.GetStr());
			return TRUE;
		}
	};
};

void CConsole::ParserStateChanged()
{
	MAUTOSTRIP(CConsole_ParserStateChanged, MAUTOSTRIP_VOID);
	m_lCommands.Clear();
}

void CConsole::AddSubSystem(CConsoleClient* _pSubsys)
{
	MAUTOSTRIP(CConsole_AddSubSystem, MAUTOSTRIP_VOID);
	m_lSubSystems.Add(_pSubsys);
	ParserStateChanged();
	_pSubsys->RegisterToParser(m_spParser);
};

void CConsole::RemoveSubSystem(CConsoleClient* _pSubsys)
{
	MAUTOSTRIP(CConsole_RemoveSubSystem, MAUTOSTRIP_VOID);
	KillScripts();

	ParserStateChanged();
	_pSubsys->UnRegister(m_spParser);
	int len = m_lSubSystems.Len();
	for (int i = 0; i < len; i++)
		if (_pSubsys == m_lSubSystems[i])
		{
			m_lSubSystems.Del(i);
			break;
		};
};

void CConsole::Parser_Echo(CStr _s)
{
	MAUTOSTRIP(CConsole_Parser_Echo, MAUTOSTRIP_VOID);
//	Write(CStr::Format("%d", _a));
	Write(_s);
};

void CConsole::Parser_Cls()
{
	MAUTOSTRIP(CConsole_Parser_Cls, MAUTOSTRIP_VOID);
	Write("'cls()' is broken.");
//	for(int i = 0; i < 50; i++) Write("");
};

void CConsole::Parser_History()
{
	MAUTOSTRIP(CConsole_Parser_History, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lConHistory.Len(); Write(m_lConHistory[i++]))
	{
	
	}
};

void CConsole::Parser_ListCommands()
{
	MAUTOSTRIP(CConsole_Parser_ListCommands, MAUTOSTRIP_VOID);
	TList_Vector<CStr> StrList = m_spParser->GetRootNameSpace()->GetSymbols();
	

	int l = StrList.Len();
	for (int i = 0; i < l; Write(StrList[i++]))
	{
	}
};


CConsole::CKeyBind* CConsole::FindBinding(int _KeyID)
{
	MAUTOSTRIP(CConsole_FindBinding, NULL);
	for(int i = 0; i < m_lKeyBind.Len(); i++)
		if (m_lKeyBind[i].m_ScanCode == _KeyID) return &m_lKeyBind[i];
	return NULL;
}

void CConsole::Parser_Bind3(CStr _Key, CStr _Down, CStr _Up, CStr _Repeat)
{
	MAUTOSTRIP(CConsole_Parser_Bind3, MAUTOSTRIP_VOID);
//ConOutL(CStrF("Bind2 %s, %s, %s", (char*)_Key, (char*) _Down, (char*)_Up));

	int ScanCode;
	if (_Key.Find("+") >= 0)
		ScanCode = CScanKey::GetScanKey32Comb(_Key);
	else
		ScanCode = CScanKey::GetScanCode(_Key);

	if (!ScanCode)
	{
		ConOutL("Invalid key '" + _Key + "'");
		return;
	}

	int bClear = (_Down == "") && (_Up == "") && (_Repeat == "");
	for(int i = 0; i < m_lKeyBind.Len(); i++)
		if (m_lKeyBind[i].m_ScanCode == ScanCode)
		{
			if (bClear)
				m_lKeyBind.Del(i);
			else
			{
				StopBinding(&m_lKeyBind[i]);
				m_lKeyBind[i].Create(_Down, _Up, _Repeat, ScanCode);
				//JK-NOTE: This will throw a ton of exceptions when binding unregistered commands
				//m_lKeyBind[i].CompileAll(m_spParser); //AR-ADD - we want to compile as early as possible to prevent fragmentation.
			}
			return;
		}

	if (!bClear)
	{
		CKeyBind Bind;
		Bind.Create(_Down, _Up, _Repeat, ScanCode);
		//JK-NOTE: This will throw a ton of exceptions when binding unregistered commands
		//Bind.CompileAll(m_spParser); //AR-ADD - we want to compile as early as possible to prevent fragmentation.
		m_lKeyBind.Add(Bind);
	}

//	if ((_KeyID < 0) || (_KeyID >= m_lKeyBind.Len())) Error("Parser_Bindkey", "Invalid Key ID.");
//	m_lKeyBind[_KeyID].ProgramStr = _ParsStr;
};

void CConsole::Parser_Bind2(CStr _Key, CStr _Down, CStr _Up)
{
	MAUTOSTRIP(CConsole_Parser_Bind2, MAUTOSTRIP_VOID);
	Parser_Bind3(_Key, _Down, _Up, CStr(""));
}

void CConsole::Parser_Bind(CStr _Key, CStr _ParsStr)
{
	MAUTOSTRIP(CConsole_Parser_Bind, MAUTOSTRIP_VOID);
	Parser_Bind3(_Key, _ParsStr, CStr(""), CStr(""));
}


void CConsole::Parser_UnBind(CStr _Key)
{
	MAUTOSTRIP(CConsole_Parser_UnBind, MAUTOSTRIP_VOID);
	int ScanCode = CScanKey::GetScanKey32Comb(_Key);
	if (!ScanCode)
	{
		ConOutL("Invalid key '" + _Key + "'");
		return;
	}
	
	for(int i = 0; i < m_lKeyBind.Len(); i++)
	{
		if (m_lKeyBind[i].m_ScanCode == ScanCode) 
		{
			m_lKeyBind.Del(i);
			return;
		}
	}
}

void CConsole::Parser_BindRep(CStr _Key, CStr _Repeat)
{
	MAUTOSTRIP(CConsole_Parser_BindRep, MAUTOSTRIP_VOID);
	Parser_Bind3(_Key, CStr(""), CStr(""), _Repeat);
}

void CConsole::Parser_BindRep2(CStr _Key, CStr _Repeat, CStr _Up)
{
	MAUTOSTRIP(CConsole_Parser_BindRep2, MAUTOSTRIP_VOID);
	Parser_Bind3(_Key, CStr(""), _Up, _Repeat);
}

void CConsole::Parser_BindScanCode2(int _KeyID, CStr _Down, CStr _Up)
{
	MAUTOSTRIP(CConsole_Parser_BindScanCode2, MAUTOSTRIP_VOID);
	if(_KeyID == 0)
		return;
	int bClear = (_Down == "") && (_Up == "");
	for(int i = 0; i < m_lKeyBind.Len(); i++)
		if (m_lKeyBind[i].m_ScanCode == _KeyID)
		{
			if (bClear)
				m_lKeyBind.Del(i);
			else
			{
				StopBinding(&m_lKeyBind[i]);
				m_lKeyBind[i].Create(_Down, _Up, _KeyID);
			}
			return;
		}

	if (!bClear)
	{
		CKeyBind Bind;
		Bind.Create(_Down, _Up, _KeyID);
		m_lKeyBind.Add(Bind);
	}

//	if ((_KeyID < 0) || (_KeyID >= m_lKeyBind.Len())) Error("Parser_Bindkey", "Invalid Key ID.");
//	m_lKeyBind[_KeyID].ProgramStr = _ParsStr;
};

void CConsole::Parser_BindScanCode3(int _KeyID, CStr _Down, CStr _Up, CStr _Repeat)
{
	MAUTOSTRIP(CConsole_Parser_BindScanCode2, MAUTOSTRIP_VOID);
	if(_KeyID == 0)
		return;
	int bClear = (_Down == "") && (_Up == "") && (_Repeat == "");
	for(int i = 0; i < m_lKeyBind.Len(); i++)
		if (m_lKeyBind[i].m_ScanCode == _KeyID)
		{
			if (bClear)
				m_lKeyBind.Del(i);
			else
			{
				StopBinding(&m_lKeyBind[i]);
				m_lKeyBind[i].Create(_Down, _Up, _Repeat, _KeyID);
			}
			return;
		}

	if (!bClear)
	{
		CKeyBind Bind;
		Bind.Create(_Down, _Up, _Repeat, _KeyID);
		m_lKeyBind.Add(Bind);
	}
}

void CConsole::Parser_BindScanCode(int _KeyID, CStr _ParsStr)
{
	MAUTOSTRIP(CConsole_Parser_BindScanCode, MAUTOSTRIP_VOID);
	Parser_BindScanCode2(_KeyID, _ParsStr, CStr(""));
}

void CConsole::Parser_ShowBindScanCode(int _KeyID)
{
	MAUTOSTRIP(CConsole_Parser_ShowBindScanCode, MAUTOSTRIP_VOID);
	CKeyBind* pBind = FindBinding(_KeyID);
	if (pBind)
	{
		ConOut(CStrF("%.4x = '%s', '%s', '%s'", _KeyID, (char*)pBind->m_BindDown.m_Program, (char*)pBind->m_BindUp.m_Program, (char*)pBind->m_BindRepeat.m_Program));
	}
	else
		ConOut(CStr("No binding."));
}

void CConsole::Parser_ShowBind(CStr _Key)
{
	MAUTOSTRIP(CConsole_Parser_ShowBind, MAUTOSTRIP_VOID);
	int ScanCode = CScanKey::GetScanKey32Comb(_Key);
	if (!ScanCode)
	{
		ConOutL("Invalid key '" + _Key + "'");
		return;
	}

	CKeyBind* pBind = FindBinding(ScanCode);
	if (pBind)
	{
		ConOut(CStrF("%s = '%s', '%s', '%s'", (const char*)_Key, (char*)pBind->m_BindDown.m_Program, (char*)pBind->m_BindUp.m_Program, (char*)pBind->m_BindRepeat.m_Program));
	}
	else
		ConOut("No binding for '" + _Key + "'");
}

void CConsole::Parser_BindList()
{
	MAUTOSTRIP(CConsole_Parser_BindList, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lKeyBind.Len(); i++)
	{
		CKeyBind* pBind = &m_lKeyBind[i];
		int Scan = pBind->m_ScanCode;

		CStr Name = CScanKey::GetScanKey32Name(Scan & 0x1ff);
		if (Name == "") Name = "Unknown";

		for(int i = 0; i < 7; i++)
		{
			const char* pName = CScanKey::GetScanKey32Name(Scan & (1 << (i+8)));
			if (pName) Name += CStrF("+%s", pName);
		}

		ConOut(CStrF("'%s' =§ceb8§x150'%s', '%s', '%s'", (char*)Name, (char*)pBind->m_BindDown.m_Program, (char*)pBind->m_BindUp.m_Program, (char*)pBind->m_BindRepeat.m_Program));
	}
}

void CConsole::Parser_ClearBindList()
{
	MAUTOSTRIP(CConsole_Parser_ClearBindList, MAUTOSTRIP_VOID);

	for(int i = 0; i < m_lKeyBind.Len(); i++)
		StopBinding(&m_lKeyBind[i]);
	m_lKeyBind.Clear();
}

void CConsole::Parser_BindListScan()
{
	MAUTOSTRIP(CConsole_Parser_BindListScan, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lKeyBind.Len(); i++)
	{
		CKeyBind* pBind = &m_lKeyBind[i];
		ConOut(CStrF("%.4x =§ceb8§x150'%s', '%s', '%s'", pBind->m_ScanCode, (char*)pBind->m_BindDown.m_Program, (char*)pBind->m_BindUp.m_Program, (char*)pBind->m_BindRepeat.m_Program));
	}
}

void CConsole::Parser_GlobalArrayFlags(int _Flg)
{
	MAUTOSTRIP(CConsole_Parser_GlobalArrayFlags, MAUTOSTRIP_VOID);
	CListVectorCore<CObj>::SetGlobalFlags(_Flg);
}

void CConsole::Parser_ExecuteScriptFile(CStr _FileName)
{
	MAUTOSTRIP(CConsole_Parser_ExecuteScriptFile, MAUTOSTRIP_VOID);
	//AR-CHANGE: if '_FileName' is to be sent as reference, it should be const
	//           C++ doesn't allow temporaries to be sent as references, although MSVC allows it..
	const CStr* pFileName = &_FileName;
	CStr FileNameTmp; // only created if needed
	
	//Get absolute filename
	if (_FileName.GetDevice() == "")
	{
		//Relative path given, add system exepath
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		FileNameTmp = pSys->m_ExePath + _FileName;
		pFileName = &FileNameTmp;
	};

	if (!CDiskUtil::FileExists(*pFileName))
	{
		ConOut("(CConsole::Parser_ExecuteScriptFile) Invalid filename: " + *pFileName);
		return;
	}
	
	ConOut("Executing " + *pFileName);

	//Open file and catch file exceptions
	M_TRY
	{
		CCFile File;
		File.Open(*pFileName, CFILE_READ | CFILE_BINARY);
		
		//Read file
		TArray<char> Buffer;
		Buffer.SetLen(File.Length() + Buffer.ElementSize());
		File.Read(Buffer.GetBasePtr(), Buffer.ListSize());
		File.Close();

		//Terminate string
		Buffer[Buffer.Len() - 1] = 0;

//		m_spParser->TraceSymbolTree(false);
		//Execute script
		TPtr<CScript> spScript = m_spParser->BuildScript(CStrF("NExecTemp0x%08x", &pFileName), Buffer.GetBasePtr(), FileNameTmp);

		if (spScript)
			spScript->CreateExecutionContext()->Execute();
	}
	M_CATCH(
	catch (CCExceptionFile _Ex)
	{
		//This ensures that any file-related messages are thrown from this method, which is proper
		FileError("Parser_ExecuteScriptFile", (_Ex.GetExceptionInfo()).m_Message, 0);
	};
	)

};

void CConsole::Register(CScriptRegisterContext & _RegContext)
{
	CConsoleClient::Register(_RegContext);
	_RegContext.RegFunction("echo", this, &CConsole::Parser_Echo);
	_RegContext.RegFunction("ConOut", this, &CConsole::Parser_Echo);
	_RegContext.RegFunction("cls", this, &CConsole::Parser_Cls);
	_RegContext.RegFunction("history", this, &CConsole::Parser_History);
	_RegContext.RegFunction("commands", this, &CConsole::Parser_ListCommands);
	_RegContext.RegFunction("help", this, &CConsole::Parser_ListCommands);
	_RegContext.RegFunction("bindlistscan", this, &CConsole::Parser_BindListScan);
	_RegContext.RegFunction("bindlist", this, &CConsole::Parser_BindList);

	_RegContext.RegFunction("showbind", this, &CConsole::Parser_ShowBind);
	_RegContext.RegFunction("unbind", this, &CConsole::Parser_UnBind);
	_RegContext.RegFunction("bind", this, &CConsole::Parser_Bind);
	_RegContext.RegFunction("bind2", this, &CConsole::Parser_Bind2);
	_RegContext.RegFunction("bind3", this, &CConsole::Parser_Bind3);
	_RegContext.RegFunction("bindrep", this, &CConsole::Parser_BindRep);
	_RegContext.RegFunction("bindrep2", this, &CConsole::Parser_BindRep2);

	_RegContext.RegFunction("showbindscan", this, &CConsole::Parser_ShowBindScanCode);
	_RegContext.RegFunction("bindscan", this, &CConsole::Parser_BindScanCode);
	_RegContext.RegFunction("bindscan2", this, &CConsole::Parser_BindScanCode2);

	_RegContext.RegFunction("mcc_gaf", this, &CConsole::Parser_GlobalArrayFlags);
	_RegContext.RegFunction("exec", this, &CConsole::Parser_ExecuteScriptFile);
};

void CConsole::AddWriteCallback(void(*_pfnWrite)(const CStr& _Str, void* _pContext), void* _pContext)
{
	MAUTOSTRIP(CConsole_AddWriteCallback, MAUTOSTRIP_VOID);
	int i = 0;
	for(i = 0; i < m_lWriteCallbacks.Len(); i++)
		if (m_lWriteCallbacks[i].m_pfnWrite == NULL) break;

	if (i == m_lWriteCallbacks.Len())
		i = m_lWriteCallbacks.Add(CWriteCallback());

	m_lWriteCallbacks[i].m_pfnWrite = _pfnWrite;
	m_lWriteCallbacks[i].m_pContext = _pContext;
}

void CConsole::RemoveWriteCallback(void(*_pfnWrite)(const CStr& _Str, void* _pContext), void* _pContext)
{
	MAUTOSTRIP(CConsole_RemoveWriteCallback, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lWriteCallbacks.Len(); i++)
		if (m_lWriteCallbacks[i].m_pfnWrite == _pfnWrite)
		{
			m_lWriteCallbacks[i].Clear();
			return;
		}
}

// --------------------------------
//  CConsoleRender
// --------------------------------
void CConsoleRender::Render(CConsole* pCon, CClipRect cr, CImage* img, CPnt pos)
{
	MAUTOSTRIP(CConsoleRender_Render, MAUTOSTRIP_VOID);
	if (pCon == NULL) return;
	CClipRect imgrect = img->GetClipRect();
	cr += imgrect;

/*	CConsoleString* pStr = pCon->GetOutputHead();

	int yp = pos.y;
	for (int y = 0; (y < pCon->m_lOutput.Len()); y++)
	{
		img->DebugText(cr, CPnt(pos.x, yp), pCon->m_lOutput[y], 0xffffff);
		yp += 8;
		if (!cr.VisibleLine(yp)) break;
	};*/
};

void CConsoleRender::RenderRect(CConsole* pCon, CClipRect cr, CImage* img, CRct pos)
{
	MAUTOSTRIP(CConsoleRender_RenderRect, MAUTOSTRIP_VOID);
	MSCOPESHORT(CConsoleRender::RenderRect); //AR-SCOPE

	if (pCon == NULL) return;
	CClipRect imgrect = img->GetClipRect();
	cr += imgrect;
	CClipRect conrect = CClipRect(pos.p0, pos.p1, pos.p0);
	conrect += cr;
	int h = (pos.p1.y - pos.p0.y);
	int fnth = img->DebugTextHeight();

	int yp = h-fnth;
	CStr cl = CStr("> ") + pCon->m_CommandLineEdit.GetStr();
	img->DebugText(conrect, CPnt(pos.p0.x, yp), cl, 0xffffff);
	img->DebugText(conrect, CPnt(pos.p0.x + img->DebugTextLen(cl.Copy(0, pCon->m_CommandLineEdit.GetCursorPos()+2)), yp), CStr("_"), 0xffffff);
	yp -= fnth;

	CConsoleString* pStr = pCon->GetOutputHead();

	while(pStr && yp > -fnth)
	{
		img->DebugText(conrect, CPnt(pos.p0.x, yp), pStr->m_Str, 0xffffff);
		yp -= fnth;
		pStr = pStr->m_pNext;
	}

/*	for (int y = pCon->m_lOutput.Len()-1; (y >= 0); y--)
	{
		img->DebugText(conrect, CPnt(pos.p0.x, yp), pCon->m_lOutput[y], 0xffffff);
		yp -= fnth;
	};*/
};

