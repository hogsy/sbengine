// -------------------------------------------------------------------
//  CCExceptionInfo
// -------------------------------------------------------------------
#if	!defined(M_NOEXCEPTIONINFO) && defined(PLATFORM_CONSOLE)
CFStr CCExceptionInfo::m_Location;
CFStr CCExceptionInfo::m_SourcePos;
CFStr CCExceptionInfo::m_Message;
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
MRTC_IMPLEMENT_BASE(CCException);
MRTC_IMPLEMENT(CCExceptionMemory, CCException);
MRTC_IMPLEMENT(CCExceptionFile, CCException);
MRTC_IMPLEMENT(CCExceptionGraphicsHAL, CCException);
#endif

CCExceptionInfo::CCExceptionInfo()
{
#ifndef M_NOEXCEPTIONINFO
	m_pNext = NULL;
	m_ErrCode = 0;
#endif
}

CCExceptionInfo::~CCExceptionInfo()
{
#ifndef M_NOEXCEPTIONINFO
	if (m_pNext)
		delete m_pNext;
	m_pNext = NULL;
#endif
}

void CCExceptionInfo::operator= (const CCExceptionInfo& _Ex)
{
#ifndef M_NOEXCEPTIONINFO
	m_Location = _Ex.m_Location;
	m_SourcePos = _Ex.m_SourcePos;
	m_Message = _Ex.m_Message;
/*	if (_Ex.m_Location.IsAnsi())
		m_Location.Capture((const char*)_Ex.m_Location);
	else
		m_Location.Capture((const wchar*)_Ex.m_Location);

	if (_Ex.m_SourcePos.IsAnsi())
		m_SourcePos.Capture((const char*)_Ex.m_SourcePos);
	else
		m_SourcePos.Capture((const wchar*)_Ex.m_SourcePos);

	if (_Ex.m_Message.IsAnsi())
		m_Message.Capture((const char*)_Ex.m_Message);
	else
		m_Message.Capture((const wchar*)_Ex.m_Message);*/

	m_ErrCode = _Ex.m_ErrCode;
#endif
//	m_pNext = m_pNext;
}

CFStr CCExceptionInfo::GetString() const
{
#ifndef M_NOEXCEPTIONINFO
	// Compose error string
	CFStr msg;
//	msg = m_Location + CStr(" \t");

	// Remove path
	CFStr src(m_SourcePos);
	int iDotPos = src.Find(",");
	if (iDotPos > 0)
	{
		int p;
		//while ((p = src.Find("\\")) >= 0) src = src.Del(iDotPos+2, (p-iDotPos)-1);
		//AO fix for infinite loop below. Not sure if this is correct
		while (((p = src.Find("\\")) >= 0) && (p > iDotPos + 1)) src = src.Del(iDotPos+2, (p-iDotPos)-1);
	};
//	msg += src + CStr(" \t") + m_Message;


	msg = CFStrF("%s   (%s, %s)", (const char*) m_Message, (const char*) m_Location, (const char*) src);
	return msg;
#else
	return "Exception info not available";
#endif
}

// -------------------------------------------------------------------
//  CCExceptionLog
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CCExceptionLog, CReferenceCount);

CCExceptionLog::CCExceptionLog()
{
	m_pFirst = NULL;
	Clear();
	SetMaxHistory(20);
}

CCExceptionLog::~CCExceptionLog()
{
	Clear();
}

void CCExceptionLog::Clear()
{
	M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));

	if (m_pFirst)
		delete m_pFirst;
/*	CCExceptionInfo* pInfo = m_pFirst;
	while(pInfo)
	{
		CCExceptionInfo* pNext = pInfo->m_pNext;
		delete pInfo;
		pInfo = pNext;
	}*/
	m_pFirst = NULL;
	m_nExceptions = 0;
	m_MaxExceptions = 0;
	m_iExceptionViewed = -1;
}


void CCExceptionLog::SetExceptionFile(CStr _Name)
{
	M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
	m_OutputFile = _Name;
}

void CCExceptionLog::SetMaxHistory(int _Max)
{
	M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
	Clear();
	m_MaxExceptions = _Max;

#ifndef M_NOEXCEPTIONINFO
	CCExceptionInfo* pLast = NULL;
	for(int i = 0; i < _Max; i++)
	{
		CCExceptionInfo* pInfo = DNew(CCExceptionInfo) CCExceptionInfo;
		if (!i)
			m_pFirst = pInfo;
		else
			pLast->m_pNext = pInfo;

		pLast = pInfo;
	}
#endif
}

void CCExceptionLog::LogException(CCExceptionInfo& _Info)
{
#ifndef M_NOEXCEPTIONINFO
	M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
//	if (m_nExceptions >= m_MaxExceptions)
	{
		// Move the last to the beginning and overwrite it.
		CCExceptionInfo* pLast = m_pFirst;
		while(pLast->m_pNext)
		{
			CCExceptionInfo* pNext = pLast->m_pNext;
			if (!pLast->m_pNext->m_pNext)
				pLast->m_pNext = NULL;
			pLast = pNext;
		}
		pLast->m_pNext = m_pFirst;
		m_pFirst = pLast;
		*pLast = _Info;
		if (m_nExceptions < m_MaxExceptions) m_nExceptions++;
		if (m_iExceptionViewed < m_nExceptions-1) m_iExceptionViewed++;

	}
/*	else
	{
		CCExceptionInfo* pInfo = DNew(CCExceptionInfo) CCExceptionInfo;
		if (!pInfo) return;
		*pInfo = _Info;
		pInfo->m_pNext = m_pFirst;
		m_pFirst = pInfo;
		m_nExceptions++;
		m_iExceptionViewed++;
	}*/
#endif
}

// -------------------------------------------------------------------
bool CCExceptionLog::ExceptionAvail()
{
	M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
	return (m_iExceptionViewed >= 0);
}

const CCExceptionInfo& CCExceptionLog::PeekExceptionInfo()
{
#ifndef M_NOEXCEPTIONINFO
	M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
	int i = m_iExceptionViewed;
	if (i < 0) return *m_pFirst;

	CCExceptionInfo* pInfo = m_pFirst;
	if (!pInfo) Error("PeekExceptionInfo", "Internal error.");
	while(i > 0 && pInfo->m_pNext)
	{
		pInfo = pInfo->m_pNext;
		i--;
	}
	return *pInfo;
#else
	static CCExceptionInfo hoho;
	return hoho;
#endif

}

CCExceptionInfo CCExceptionLog::GetExceptionInfo()
{
#ifndef M_NOEXCEPTIONINFO
	M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
	int i = m_iExceptionViewed;
	if (i < 0) return *m_pFirst;

	CCExceptionInfo* pInfo = m_pFirst;
	if (!pInfo) Error("PeekExceptionInfo", "Internal error.");
	while(i > 0 && pInfo->m_pNext)
	{
		pInfo = pInfo->m_pNext;
		i--;
	}
	m_iExceptionViewed--;

	CCExceptionInfo Tmp;
	Tmp = *pInfo;
	return Tmp;
#else
	return CCExceptionInfo();
#endif

}

const CCExceptionInfo* CCExceptionLog::GetFirst()
{
	M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
	return m_pFirst;
}

// -------------------------------------------------------------------
void CCExceptionLog::DisplayFatal()
{
	
	M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));

	CStr msg2("¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯ MOS Exception report ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯\n\n");
	CStr msg("MOS Exception report\t\n\n\n");

#ifndef M_NOEXCEPTIONINFO
	unsigned int maxfunclen = 0;
	unsigned int maxsourcelen = 0;
	unsigned int maxmsglen = 0;

	int nEx = 0;
	CCExceptionInfo* pInfo = m_pFirst;
	while(pInfo && nEx < m_nExceptions)
	{
		int l = pInfo->m_Location.Len();
		if (l > maxfunclen) maxfunclen = l;
		l = pInfo->m_SourcePos.Len();
		if (l > maxsourcelen) maxsourcelen = l;
		l = pInfo->m_Message.Len();
		if (l > maxmsglen) maxmsglen = l;

		pInfo = pInfo->m_pNext; 
		nEx++; 
	}

#ifdef PLATFORM_SHINOBI
	pInfo = m_pFirst;
	int iEx = 0;
	while(pInfo && iEx < m_nExceptions)
	{
		// Remove path
		CStr src(pInfo->m_SourcePos);
		int iDotPos = src.Find(CStr(","));
		if (iDotPos > 0)
		{
			int p;
			//while ((p = src.Find(CStr("\\"))) >= 0) src = src.Del(iDotPos+3, (p-iDotPos)-2);
			//AO fix for infinite loop below. Not sure if this is correct
			while (((p = src.Find(CStr("\\"))) >= 0) && (p > iDotPos + 2)) src = src.Del(iDotPos+3, (p-iDotPos)-2);
		}

		msg += pInfo->m_Location+", "+src+", "+pInfo->m_Message+"+n";

		iEx++;
		pInfo = pInfo->m_pNext;
	}
#else
	// My-god-this-is-ugly-code:
	pInfo = m_pFirst;
	int iEx = 0;
	while(pInfo && iEx < m_nExceptions)
	{
		// VC++ Friendly error
		CStr src2(pInfo->m_SourcePos);
		// Remove "Line "
		src2 = src2.Del(0,5);

		int LineNum = -1;
		CStrBase::Val_int(src2.Str(), LineNum);
//		sscanf(src2, "%d", &LineNum); // Men vad ska jag använda då ??

		int iDotPos = src2.Find(CStr(","));
		if (iDotPos >= 0) 
		{
			src2 = src2.Del(0, iDotPos + 2);
		}

		msg2 += CStrF("%s(%d) : %s\n", (const char *)src2, LineNum, (const char *)pInfo->m_Message);

		// Message Box error

		msg += CStrF("%0.4X   ", iEx);
		msg += pInfo->m_Location;
		msg += CStr((char) 32, (int)(1.60f*(maxfunclen - pInfo->m_Location.Len() )));
		msg += CStr(" \t");

		// Remove path
		CStr src(pInfo->m_SourcePos);
		iDotPos = src.Find(CStr(","));
		if (iDotPos > 0) 
		{
			int p;
			//while ((p = src.Find(CStr("\\"))) >= 0) src = src.Del(iDotPos+3, (p-iDotPos)-2);
			//AO fix for infinite loop below. Not sure if this is correct
			while (((p = src.Find(CStr("\\"))) >= 0) && (p > iDotPos+ 2)) src = src.Del(iDotPos+3, (p-iDotPos)-2);
		}

		// Add sourcepos
		msg += src;
		msg += CStr((char) 32, (int)(1.60f*(maxsourcelen - pInfo->m_SourcePos.Len() )));
		msg += CStr(" \t");

		// Add message
		msg += pInfo->m_Message;
		msg += CStr((char) 32 , (int)(1.60f*(maxmsglen - pInfo->m_Message.Len() )));
		msg += CStr("\n");

		iEx++;
		pInfo = pInfo->m_pNext;
	}
#endif
#endif

	msg2 += "\n__________________________________________________________________________________________________________________________________________________________________\n";

	M_TRACEALWAYS(msg2);

#ifdef PLATFORM_WIN_PC
	MessageBoxA(NULL, msg, "MOS Exception report.", MB_ICONSTOP);
#endif

	if (m_OutputFile.Len())
	{
		M_TRY
		{
			// Try create a logfile-object.
			MRTC_ObjectManager* pObjMgr = MRTC_GetObjectManager();
			CObj* pObj = pObjMgr->CreateObject("CLogFile");
			if (pObj)
			{
				M_TRY
				{
					// Succeeded, cast to a CLogFile object
#if defined(COMPILER_RTTI) || defined(M_FAKEDYNAMICCAST)
					ILogFile* pLog = TDynamicCast<ILogFile>(pObj);
#else
					ILogFile* pLog = (ILogFile*)(pObj);
#endif
					if (pLog)
					{
						// Ok
						pLog->Create(m_OutputFile);
						pLog->Log(msg);
					}
				}
				M_CATCH(
				catch(CCException)
				{
					delete pObj;
					throw;
					M_BREAKPOINT;
				}
				)
				delete pObj;
			}
		}
		#ifdef PLATFORM_WIN_PC
		M_CATCH(
		catch(CCException _Exception)
		{
			M_TRACEALWAYS("MRTC: Failed to save exception log to %s\n", m_OutputFile.Str());

				MessageBoxA(NULL, "Failed to save exception log to " + m_OutputFile + "\n" + 
					_Exception.GetExceptionInfo().GetString(), 
					"MOS Exception report.", MB_ICONSTOP);
		}
		)
#else
		M_CATCH(
		catch(CCException _Exception)
		{
			M_TRACEALWAYS("MRTC: Failed to save exception log to %s\n", m_OutputFile.Str());

		}
		)
		#endif
	}
}

// -------------------------------------------------------------------
//  CCException
// -------------------------------------------------------------------
void CCException::LogThis()
{
#if defined(PLATFORM_WIN_PC) || defined(PLATFORM_DOLPHIN)
	LogToSystemLog(CFStrF("ERROR: %s", m_Info.GetString().Str()).Str());
#endif
	MACRO_GetRegisterObject(CCExceptionLog, pLog, "SYSTEM.EXCEPTIONLOG");
	if (pLog) pLog->LogException(m_Info);
}

void CCException::operator= (const CCException _Ex)
{
	M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
	m_Info = _Ex.m_Info;
}

#ifdef PLATFORM_DOLPHIN
//AR-TEMP, didn't want to put this in MRTC_String.h at the moment..
template<int TMaxLen>
inline TFStr<TMaxLen>& operator = (TFStr<TMaxLen>& _Str1, const CFStrBase& _Str2)
{
	_Str1.Capture(_Str2.GetStr());
	return _Str1;
}
#endif

#if defined(PLATFORM_XBOX1) && defined(M_EnableExceptionDumps)
extern uint32 g_DumpException(struct _EXCEPTION_POINTERS *info, bool _bUserException);
#endif

#ifdef M_NOEXCEPTIONINFO
CCException::CCException()
#else
CCException::CCException(const CObj* _pObj, const char* _pLocation, const char* _pSourcePos, const char* _pMessage, int _ErrCode)
#endif
{
	m_Magic = 0xfe58105a;
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	if (MRTC_GetRD() && MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_Exceptions)
		MRTC_GetRD()->SendData(ERemoteDebug_Exception, 0, 0, true, true);
#endif
#ifndef M_NOEXCEPTIONINFO
#ifdef	PLATFORM_PS2
	scePrintf( "-- Exception -- (%s) (%s) (%s)\n", _pLocation, _pSourcePos, _pMessage );
#endif
#ifdef COMPILER_RTTI
	CFStr ClassName;
	if (_pObj)
	{
		ClassName = typeid(*_pObj).name();
//		ClassName = ClassName.Del(0, 6);
	}
	else
		ClassName = "";

#else
	CFStr ClassName = (_pObj) ? _pObj->MRTC_ClassName() : "";
#endif
	const char* pStr = (!_pObj) ? "" : (const char*) ClassName;
	m_Info.m_Location = CFStrF("%s::%s", pStr, _pLocation);
//	m_Info.m_Location = CStrF("%s::%s", (!_pObj) ? "" : _pObj->MRTC_ClassName(), _pLocation);
	m_Info.m_SourcePos = _pSourcePos;
	m_Info.m_Message = _pMessage;
	m_Info.m_ErrCode = _ErrCode;
#endif

	LogThis();

#if defined(PLATFORM_XBOX1) && defined(M_EnableExceptionDumps) && 0
	_EXCEPTION_POINTERS Info;
	CONTEXT Context;
	EXCEPTION_RECORD Record;
	Info.ContextRecord = &Context;
	Info.ExceptionRecord = &Record;
	mint RegEBP;
	__asm
	{
		mov RegEBP, ebp
	}

	Context.Ebp = RegEBP;
	g_DumpException(&Info, true);
#endif
}

int CCException::GetErrorCode()
{ 
#ifndef M_NOEXCEPTIONINFO
	return m_Info.m_ErrCode;
#else
	return 0;
#endif
};

const CCExceptionInfo& CCException::GetExceptionInfo() const
{
	return m_Info;
}

CFStr CCException::GetSourcePosStr(int _Line, const char* _pFileName)
{
	return CFStrF("Line %d, %s", _Line,  _pFileName);
}

#ifdef M_NOEXCEPTIONINFO
void CCException::DoThrow()
{
#if M_EXCEPTIONS
	throw CCException();
#else
	M_BREAKPOINT;
#endif
}

#else
void CCException::DoThrow(const CObj* _pObj, const char* _pLocation, const char* _pSourcePos, const char* _pMessage, int _ErrCode)
{
#if M_EXCEPTIONS
	throw CCException(_pObj, _pLocation, _pSourcePos, _pMessage, _ErrCode);
#else
	M_BREAKPOINT;
#endif
}

#endif

// -------------------------------------------------------------------
//  CCExceptionMemory
// -------------------------------------------------------------------
#ifdef M_NOEXCEPTIONINFO
CCExceptionMemory::CCExceptionMemory() :
	CCException()
#else
CCExceptionMemory::CCExceptionMemory(const CObj* _pObj, const char* _pLocation, const char* _pSourcePos) :
	CCException(_pObj, _pLocation, _pSourcePos, "Out of memory.")
#endif
{
}

#ifdef M_NOEXCEPTIONINFO
void CCExceptionMemory::DoThrow()
{
#if M_EXCEPTIONS
	throw CCExceptionMemory();
#else
	M_BREAKPOINT;
#endif
}

#else
void CCExceptionMemory::DoThrow(const CObj* _pObj, const char* _pLocationStr, const char* _pSourcePosStr)
{
#if M_EXCEPTIONS
	throw CCExceptionMemory(_pObj, _pLocationStr, _pSourcePosStr);
#else
	M_BREAKPOINT;
#endif
}

#endif

// -------------------------------------------------------------------
//  CCExceptionFile
// -------------------------------------------------------------------
#ifdef M_NOEXCEPTIONINFO
CCExceptionFile::CCExceptionFile() :
	CCException()
#else
CCExceptionFile::CCExceptionFile(const CObj* _pObj, const char* _pLocation, const char* _pSourcePos, const char* _pMessage, int _ErrCode) :
	CCException(_pObj, _pLocation, _pSourcePos, 
		(_ErrCode) ? CFStr(strerror(_ErrCode)) + CFStr("  (") + CFStr(_pMessage) + CFStr(")") : CFStr(_pMessage), _ErrCode)
#endif
{
}

#ifdef M_NOEXCEPTIONINFO
void CCExceptionFile::DoThrow()
{
#if M_EXCEPTIONS
	throw CCExceptionFile();
#else
	M_BREAKPOINT;
#endif
}

#else
void CCExceptionFile::DoThrow(const CObj* _pObj, const char* _pLocationStr, const char* _pSourcePosStr, const char* _pMessageStr, int _ErrCode)
{
#if M_EXCEPTIONS
	throw CCExceptionFile(_pObj, _pLocationStr, _pSourcePosStr, _pMessageStr, _ErrCode);
#else
	M_BREAKPOINT;
#endif
}

#endif

// -------------------------------------------------------------------
//  CCExceptionGraphicsHAL
// -------------------------------------------------------------------
CCExceptionGraphicsHAL::CCExceptionGraphicsHAL(const CObj* _pObj, const char* _pLocation, const char* _pSourcePos, const char* _pMessage, int _ErrCode) :
#ifdef M_NOEXCEPTIONINFO
	CCException()
#else
	CCException(_pObj, _pLocation, _pSourcePos, _pMessage, _ErrCode)
#endif
{
}

