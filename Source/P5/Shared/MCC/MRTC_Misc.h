
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Part of MRTC.h

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios AB 1996,2003

	Contents:

	Comments:

	History:	
		030711:		File header added

\*____________________________________________________________________________________________*/

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Progress-interface helper-functions
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Comment:			A yet to be used general-purpose progress
						report and retrival system.
\*____________________________________________________________________*/

void MRTC_InstallProgressHandler(spCReferenceCount _spObj);
void MRTC_RemoveProgressHandler(spCReferenceCount _spObj);
void MRTC_PushProgress(const char* _pLevelName = NULL);
void MRTC_PopProgress();
void MRTC_SetProgress(fp32 _p, const char* _pTaskName = NULL);
void MRTC_SetProgressText(const char* _pTaskName = NULL);
void MRTC_InitProgressCount(int _Count, const char* _pTaskName = NULL);
void MRTC_IncProgress(const char* _pTaskName = NULL);


#define INDEXRAMPLEN 128

extern const uint32 g_IndexRamp32[INDEXRAMPLEN];
extern const uint16 g_IndexRamp16[INDEXRAMPLEN];


template <typename t_CError>
class CGenerateCompileTimeError
{
public:
	void GenerateError()
	{
		t_CError::StaticError();
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Log-macro
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Logging utility function.
	
	Comment:			The default log output depend on the target platform.
						The output can be redirected by replacing
						the SYSTEM.LOG object in the object-manager
						with a custom handler inherited from the 
						ILogFile interface.

						Users should use the LogFile to properly heed 
						the DEBUG_LOGFILEENBLE define.
\*____________________________________________________________________*/

void LogToSystemLog(const CStr& _s);
void LogToSystemLog(const char* _pStr);

#ifdef DEBUG_LOGFILEENBLE
	#define LogFile(_Msg) LogToSystemLog(_Msg)
#else
	#define LogFile(_Msg) ((void)(0))
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Functionality used by autostrip
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			Used to log usage of something.
					That something is identified by a unique textstring
					which will be logged only once (or never).
\*____________________________________________________________________*/
#if defined(MRTC_AUTOSTRIPLOGGER)
class CUsageLogger
{
public:
	CUsageLogger(const char *_pLogString, int _MaxEntries = 500);
	~CUsageLogger();

	void Log(const char *_pIdentifier);

private:
	const char*        m_pLogString;
	int                m_iStringHashIndex;	
	class CStringHash* m_pStringHash;
};
#endif
