
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Part of MRTC.h

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios AB 1996,2003

	Contents:

	Comments:

	History:	
		030711:		File header added

\*____________________________________________________________________________________________*/

#if defined(M_RTM) && !defined(PLATFORM_WIN_PC)
#define M_NOEXCEPTIONINFO
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CCExceptionInfo
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CCExceptionInfo
{
public:
#ifndef M_NOEXCEPTIONINFO
#ifdef PLATFORM_CONSOLE
	//AR-TEMP
	// CodeWarrior always reserves space for all local variables at once,
	// and GameCube only has 64KB stack to play with, so I made these smaller..
	static CFStr m_Location;
	static CFStr m_SourcePos;
	static CFStr m_Message;
#else
	CFStr m_Location;
	CFStr m_SourcePos;
	CFStr m_Message;
#endif
	int m_ErrCode;
	CCExceptionInfo* m_pNext;
#endif

	CCExceptionInfo();
	~CCExceptionInfo();
	void operator= (const CCExceptionInfo& _Ex);
	CFStr GetString() const;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CCExceptionLog
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Contains a history of the last n exceptions.

	Comment:			This object is by default registred in the
						object manager as "SYSTEM.EXCEPTIONLOG".
						The creation and registration in performed in
						the main function of the application (MMain.cpp)
\*____________________________________________________________________*/

class CCExceptionLog : public CReferenceCount
{
	MRTC_DECLARE;

	CCExceptionInfo* m_pFirst;
	CStr m_OutputFile;
	int m_nExceptions;
	int m_MaxExceptions;
	int m_iExceptionViewed;

public:
	CCExceptionLog();
	~CCExceptionLog();

	void Clear();

	virtual void SetExceptionFile(CStr _Name);
	virtual void SetMaxHistory(int _Max);
	virtual void LogException(CCExceptionInfo& _Info);

	virtual bool ExceptionAvail();
	virtual const CCExceptionInfo& PeekExceptionInfo();
	virtual CCExceptionInfo GetExceptionInfo();

	virtual const CCExceptionInfo* GetFirst();

	virtual void DisplayFatal();
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CCException
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Base class for all exceptions. Created and
						thrown by using the Error or Error_static
						macros.
\*____________________________________________________________________*/

class CCException //: public CReferenceCount
{
public:
	uint32 m_Magic;
	CCExceptionInfo m_Info;
private:
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	MRTC_DECLARE;
#endif

	virtual void LogThis();

public:
	virtual void operator= (const CCException _Ex);
#ifdef M_NOEXCEPTIONINFO
	CCException();
#else
	CCException(const CObj* _pObj, const char* _pLocation, const char* _pSourcePos, const char* _pMessage, int _ErrCode = 0);
#endif
	virtual int GetErrorCode();
	virtual const CCExceptionInfo& GetExceptionInfo() const;

	static CFStr GetSourcePosStr(int _Line, const char* _pFileName);
};

// -------------------------------------------------------------------
#define MACRO_EXCEPT_SOURCE_POS CCException::GetSourcePosStr(__LINE__,  __FILE__)
#ifdef M_RTM
#define DTraceCurrentFilePos(_Msg) 
#else
#define DTraceCurrentFilePos(_Msg) M_TRACEALWAYS("%s(%d) : %s\n", __FILE__, __LINE__, _Msg)
#endif

#if M_EXCEPTIONS
//|| !defined(M_RTM)

#ifdef M_NOEXCEPTIONINFO
#define Error(_Location, _Msg) \
{ DTraceCurrentFilePos("Exception"); throw CCException(); }

#define Error_static(_Location, _Msg) \
{ DTraceCurrentFilePos("Exception"); throw CCException(); }

#define DbgErr(_Msg) \
{ DTraceCurrentFilePos("Exception"); throw CCException(); }

#else
#define Error(_Location, _Msg) \
{ DTraceCurrentFilePos("Exception"); throw CCException(this, _Location, MACRO_EXCEPT_SOURCE_POS, _Msg); }

#define Error_static(_Location, _Msg) \
{ DTraceCurrentFilePos("Exception"); throw CCException(NULL, _Location, MACRO_EXCEPT_SOURCE_POS, _Msg); }

#define DbgErr(_Msg) \
{ DTraceCurrentFilePos("Exception"); throw CCException(this, "?", MACRO_EXCEPT_SOURCE_POS, _Msg); }
#endif

#else
#define Error(_Location, _Msg) {M_BREAKPOINT;}
#define Error_static(_Location, _Msg)  {M_BREAKPOINT;}
#define DbgErr(_Msg)  {M_BREAKPOINT;}
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CCExceptionMemory
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CCExceptionMemory : public CCException
{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	MRTC_DECLARE;
#endif
public:

#ifdef M_NOEXCEPTIONINFO
	CCExceptionMemory();
#else
	CCExceptionMemory(const CObj* _pObj, const char* _pLocationStr, const char* _pSourcePosStr);
#endif
};

#if M_EXCEPTIONS
//|| !defined(M_RTM)
#ifdef M_NOEXCEPTIONINFO
#define MemError(_Location) \
{DTraceCurrentFilePos("Exception"); throw CCExceptionMemory();}

#define MemError_static(_Location) \
{DTraceCurrentFilePos("Exception"); throw CCExceptionMemory();}

#else

#define MemError(_Location) \
{DTraceCurrentFilePos("Exception"); throw CCExceptionMemory(this, _Location, MACRO_EXCEPT_SOURCE_POS);}

#define MemError_static(_Location) \
{DTraceCurrentFilePos("Exception"); throw CCExceptionMemory(NULL, _Location, MACRO_EXCEPT_SOURCE_POS);}

#endif
#else
#define MemError(_Location) {M_BREAKPOINT;}
#define MemError_static(_Location) {M_BREAKPOINT;}
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CCExceptionFile
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CCExceptionFile : public CCException
{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	MRTC_DECLARE;
#endif
public:
#ifdef M_NOEXCEPTIONINFO
	CCExceptionFile();
#else
	CCExceptionFile(const CObj* _pObj, const char* _pLocationStr, const char* _pSourcePosStr, const char* _pMessageStr, int _ErrCode);
#endif
};

#if M_EXCEPTIONS
//|| !defined(M_RTM)

#ifdef M_NOEXCEPTIONINFO
#define FileError(_Location, _Msg, _ErrCode) \
{DTraceCurrentFilePos("Exception"); throw CCExceptionFile();}
#define FileError_static(_Location, _Msg, _ErrCode) \
{DTraceCurrentFilePos("Exception"); throw CCExceptionFile();}
#else
#define FileError(_Location, _Msg, _ErrCode) \
{DTraceCurrentFilePos("Exception"); throw CCExceptionFile(this, _Location, MACRO_EXCEPT_SOURCE_POS, _Msg, _ErrCode);}
#define FileError_static(_Location, _Msg, _ErrCode) \
{DTraceCurrentFilePos("Exception"); throw CCExceptionFile(NULL, _Location, MACRO_EXCEPT_SOURCE_POS, _Msg, _ErrCode);}
#endif

#else
#define FileError(_Location, _Msg, _ErrCode) {M_FILEERROR;}
#define FileError_static(_Location, _Msg, _ErrCode) {M_FILEERROR;}
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CCExceptionFile
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CCExceptionGraphicsHAL : public CCException
{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	MRTC_DECLARE;
#endif
public:
	CCExceptionGraphicsHAL(const CObj* _pObj, const char* _pLocation, const char* _pSourcePos, const char* _pMessage, int _ErrCode = 0);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Win32 Error Helper
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifdef PLATFORM_WIN_PC

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Comments:			Checks for an error and throws a CCException 
						with a proper win32 error message.
						Please note that this macro won't work unless
						windows.h is included.
\*____________________________________________________________________*/
#ifndef M_NOEXCEPTIONINFO

#define Win32Err(_Str)						\
{		\
	LPVOID lpMsgBuf;						\
	int Err = GetLastError();				\
	if (Err != 0)							\
	{										\
		int Len = FormatMessage(			\
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, \
			NULL, Err,						\
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
			(LPTSTR) &lpMsgBuf, 0, NULL);	\
			CStr ErrorStr;\
		if (Len > 0)						\
			ErrorStr = CStrF("Win32: %s", (char*) lpMsgBuf);\
		else								\
			ErrorStr = CStrF("Invalid Win32 error code %d", Err); \
			DTraceCurrentFilePos(ErrorStr + "(" + _Str +")");\
			Error(_Str, CStrF("Win32: %s", (char*) lpMsgBuf))	\
	}	\
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Comments:			Same as Win32Err but doesn't assume this points
						to a subclass of CObj.
\*____________________________________________________________________*/

#define Win32Err_static(_Str)				\
{ 		\
	LPVOID lpMsgBuf;						\
	int Err = GetLastError();				\
	if (Err != 0)							\
	{										\
		int Len = FormatMessage(			\
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, \
			NULL, Err,						\
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
			(LPTSTR) &lpMsgBuf, 0, NULL);	\
			CStr ErrorStr;\
		if (Len > 0)						\
			ErrorStr = CStrF("Win32: %s", (char*) lpMsgBuf);\
		else								\
			ErrorStr = CStrF("Invalid Win32 error code %d", Err); \
		DTraceCurrentFilePos(ErrorStr + "(" + _Str +")");\
		Error_static(_Str, ErrorStr)	\
	}	\
}

#else

#define Win32Err(_Str)						\
{DTraceCurrentFilePos("Exception"); 		\
	int Err = GetLastError();				\
	if (Err != 0)							\
	{										\
			Error(_Str, ""); \
	}	\
}

#define Win32Err_static(_Str)				\
{DTraceCurrentFilePos("Exception"); 		\
	int Err = GetLastError();				\
	if (Err != 0)							\
	{										\
		Error_static(_Str, ""); \
	}	\
}
#endif // M_NOEXCEPTIONINFO

#endif // PLATFORM_WIN_PC
